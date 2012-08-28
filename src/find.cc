#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "define.h"
#include "struct.h"


const char *const find_one = "You may only %s one thing at a time.";
const char *const find_zero = "Zero times anything is always nothing.\n\r";
const char *const find_keyword = "You must specify at least one keyword.\n\r";
const char *const find_few = "Only %s thing%s found matching \"%s\".";


exit_data *one_exit( char_data *ch, const char *argument, const char *text,
		     room_data *room )
{
  if( !*argument ) {
    if( text && text != empty_string ) {
      if( !ch->pcdata ) {
	fsend_seen( ch, "%s looks around in confusion.", ch );
      } else {
	fsend( ch, "%s which exit?", text );
      }
    }
    return 0;
  }

  return exit( one_visible( ch, argument, text,
			    (visible_array*) &room->exits, EXIT_DATA ) );
}


obj_data* one_object( char_data* ch, const char *argument, const char *text,
		      thing_array* a1,
		      thing_array* a2,
		      thing_array* a3 )
{
  if( !*argument ) {
    if( text && text != empty_string ) {
      if( !ch->pcdata ) {
	fsend_seen( ch, "%s looks around in confusion.", ch );
      } else {
	fsend( ch, "%s what?", text );
      }
    }
    return 0;
  }
  
  return object( one_thing( ch, argument, text,
			    a1, a2, a3,
			    OBJ_DATA ) );
}


char_data* one_character( char_data* ch, const char *argument, const char *text,
			  thing_array* a1,
			  thing_array* a2,
			  thing_array* a3,
			  bool seen )
{
  if( !*argument ) {
    if( text && text != empty_string ) {
      if( !ch->pcdata ) {
	fsend_seen( ch, "%s looks around in confusion.", ch );
      } else {
	fsend( ch, "%s whom?", text );
      }
    }
    return 0;
  }
  
  return character( one_thing( ch, argument, text, 
			       a1, a2, a3,
			       CHAR_DATA,
			       seen ) );
}


mob_data *one_mob( char_data* ch, const char *argument, const char *text,
		    thing_array* a1,
		    thing_array* a2,
		    thing_array* a3 )
{
  if( !*argument ) {
    if( text && text != empty_string ) {
      if( !ch->pcdata ) {
	fsend_seen( ch, "%s looks around in confusion.", ch );
      } else {
	fsend( ch, "%s whom?", text );
      }
    }
    return 0;
  }
  
  return mob( one_thing( ch, argument, text, 
			 a1, a2, a3,
			 MOB_DATA ) );
}


player_data *one_player( char_data* ch, const char *argument, const char *text,
			 thing_array* a1,
			 thing_array* a2,
			 thing_array* a3 )
{
  if( !*argument ) {
    if( text && text != empty_string ) {
      if( !ch->pcdata ) {
	fsend_seen( ch, "%s looks around in confusion.", ch );
      } else {
	fsend( ch, "%s which player?", text );
      }
    }
    return 0;
  }
  
  return player( one_thing( ch, argument, text,
			    a1, a2, a3,
			    PLAYER_DATA ) );
}


thing_data* one_thing( char_data* ch, const char *argument, const char *text,
		       thing_array* a1,
		       thing_array* a2,
		       thing_array* a3,
		       int type,
		       bool seen )
{
  return (thing_data*) one_visible( ch, argument, text,
				    (visible_array*) a1, type,
				    (visible_array*) a2, type,
				    (visible_array*) a3, type,
				    0, -1,
				    0, -1,
				    seen );
}


/*
 *   ERROR MESSAGES
 */


static void not_found( char_data* ch,
		       const visible_array *const *const array, const int *const types,
		       int count, char* keywords )
{
  if( array[0] == (visible_array*) &ch->contents && !array[1] ) {
    if( !*keywords ) 
      send( ch, "You aren't carrying anything.\n\r" );
    else
      fsend( ch,
        "You %s carrying %s item%s matching \"%s\".",
        count == 0 ? "aren't" : "are only",
        count == 0 ? "any" : number_word( count ),
        count == 1 ? "" : "s",
        keywords );
  }
  
  else if( array[0] == (visible_array*) &ch->wearing && !array[1] ) {
    if( !*keywords ) 
      send( ch, "You aren't wearing anything.\n\r" );
    else
      fsend( ch,
	     "You %s wearing %s item%s matching \"%s\".",
	     count == 0 ? "aren't" : "are only",
	     count == 0 ? "any" : number_word( count ),
	     count == 1 ? "" : "s",
	     keywords );
  }
  
  else if( array[0] == (visible_array*) ch->array && !array[1] ) {
    const char *what = "thing";
    if( types ) {
      switch( types[0] ) {
      case OBJ_DATA:
	what = "object";
	break;
      case CHAR_DATA:
	what = "being";
	break;
      case PLAYER_DATA:
	what = "player";
	break;
      case MOB_DATA:
	what = "victim";
	break;
      case EXIT_DATA:
	what = "exit";
	break;
      }
    }

    if( !*keywords )
      send( ch, "Nothing found.\n\r" );
    else
      fsend( ch,
	     "The room %s contain%s %s %s%s matching \"%s\".",
	     count == 0 ? "doesn't" : "only",
	     count == 0 ? "" : "s",
	     count == 0 ? "any" : number_word( count ),
	     what,
	     count == 1 ? "" : "s",
	     keywords );

  } else if( count == 0 ) {
    if( !*keywords ) 
      send( ch, "Nothing found.\n\r" );
    else
      fsend( ch, "Nothing found matching \"%s\".",
	     keywords );

  } else {
    fsend( ch, find_few,
	   number_word( count ),
	   count == 1 ? "" : "s", keywords ); 
  }
}


visible_data *one_visible( char_data* ch, const char *argument, const char *text,
			   visible_array* a1, int type1,
			   visible_array* a2, int type2,
			   visible_array* a3, int type3,
			   visible_array* a4, int type4,
			   visible_array* a5, int type5,
			   bool seen )
{
  char tmp  [ MAX_INPUT_LENGTH ];
  int number;
  int count  = 0;
  const visible_array *const array [ 5 ] = { a1, a2, a3, a4, a5 };
  const int types [ 5 ] = { type1, type2, type3, type4, type5 };

  if( !*argument ) {
    if( text && text != empty_string ) {
      if( !ch->pcdata ) {
	fsend_seen( ch, "%s looks around in confusion.", ch );
      } else {
	fsend( ch, "What do you wish to %s?", text );
      }
    }
    return 0;
  }

  const bool numbered = isdigit( *argument );

  if( ( number = smash_argument( tmp, argument ) ) == 0 ) {
    if( text != empty_string )
      send( ch, find_zero );
    return 0;
  }

  if( number < 0 || *argument ) {
    if( text != empty_string )
      fsend( ch, find_one, text );
    return 0;
  }

  if( !*tmp ) {
    if( text != empty_string )
      send( ch, find_keyword );
    return 0;
  }

  const bool scored = !numbered && ( number == 1 );

  visible_data *best = 0;
  int best_score = 0;
  int score;

  for( int i = 0; i < 5 && array[i]; ++i ) {
    const int type = types[i];
    const bool mob_arr = ( array[i] == (visible_array*) &mob_list );
    for( int j = 0; j < *array[i]; ++j ) {
      if( mob_arr ) {
	mob_data *npc = mob_list[j];
	if( ch->Level( ) < LEVEL_APPRENTICE
	    && npc->in_room
	    && npc->in_room->area->status != AREA_OPEN ) {
	  // Prevent world spells from finding mobs in non-open areas.
	  continue;
	}
      }
      visible_data *visible = array[i]->list[j];
      thing_data *th = thing( visible );
      if( th && ! th->Is_Valid( ) ) {
	// mob_list, player_list, obj_list contain invalid (extracted) entries.
	continue;
      }
      switch( type ) {
      case THING_DATA:
	if( !thing( visible ) )
	  continue;
	break;
      case OBJ_DATA:
	{
	  obj_data *obj = object( visible );
	  if( !obj )
	    continue;
	  char_data *rch = character( obj->array->where );
	  if( rch
	      && rch != ch
	      && obj->array == &rch->wearing
	      && !has_holylight( ch ) ) {
	    int m = -1;
	    for( int k = j+1;
		 k < rch->wearing && ((obj_data*)rch->wearing[k])->position == obj->position;
		 ++k ) {
	      if( is_set( ((obj_data*)rch->wearing[k])->extra_flags, OFLAG_COVER ) ) {
		m = k-1;
	      }
	    }
	    if( m != -1 ) {
	      j = m;
	      continue;
	    }
	  }
	}
	break;
      case CHAR_DATA:
	if( !character( visible ) )
	  continue;
	break;
      case PLAYER_DATA:
	if( !player( visible ) )
	  continue;
	break;
      case MOB_DATA:
	if( !mob( visible ) )
	  continue;
	break;
      case EXIT_DATA:
	if( !exit( visible ) )
	  continue;
	// Warning: this assumes the array is ch->in_room->exits.
	if( !seen && !ch->in_room->Seen( ch ) )
	  continue;
	break;
      }
      if( ( seen || visible->Seen( ch ) )
	  && ( score = is_name( tmp, visible->Keywords( ch ), scored ) ) ) {
	if( !scored ) {
	  const int sel = number - count;
	  const int num = visible->Number( );
	  if( num >= sel ) {
	    visible->Select( 1 );
	    if( th ) {
	      th->temp = sel-1;
	    }
	    return visible;
	  }
	  count += num;
	} else if( score > best_score ) {
	  best_score = score;
	  best = visible;
	}
      }
    }
  }

  if( best ) {
    best->Select( 1 );
    if( thing_data *th = thing( best ) ) {
      th->temp = 0;
    }
    return best;
  }

  if( text != empty_string )
    not_found( ch, array, types, count, tmp );
  
  return 0;
}


/* 
 *   SEVERAL THINGS
 */


thing_array *several_things( char_data* ch, const char *argument, const char *text,
			     thing_array* a1,
			     thing_array* a2,
			     thing_array* a3 )
{
  return (thing_array*) several_visible( ch, argument, text,
					 (visible_array*) a1,
					 (visible_array*) a2,
					 (visible_array*) a3 );
}


visible_array *several_visible( char_data* ch, const char *argument, const char *text,
				visible_array* a1,
				visible_array* a2,
				visible_array* a3,
				visible_array* a4 )
{
  if( !*argument ) {
    if( text != empty_string )
      send( ch, "What do you wish to %s?\n\r", text );
    return 0;
  }

  const int size = 4;
  const visible_array *const array [ size ] = { a1, a2, a3, a4 };  
  visible_array *output = new visible_array;
  char tmp [ MAX_INPUT_LENGTH ];
  int count = 0;
    
  while( *argument ) {
    int number;
    bool numbered = isdigit( *argument );
    
    count = 0;

    if( ( number = smash_argument( tmp, argument ) ) != INT_MIN ) {
      if( !*tmp ) {
	send( ch, "You must specify at least one keyword.\n\r" );
	delete output;
	return 0;
      }
    }
    
    if( number == 0 ) {
      send( ch, "Zero times an item is always nothing.\n\r" );
      delete output;
      return 0;
    }
    
    bool scored = !numbered && ( number == 1 );
    
    visible_data *best = 0;
    int best_score = 0;
    int score = 0;
    bool found = false;
    
    for( int i = 0; i < size && array[i]; ++i ) {
      for( int j = 0; j < *array[i]; ++j ) {
	visible_data *visible = array[i]->list[j];
	if( thing_data *th = thing( visible ) ) {
	  if( !th->Is_Valid( ) )
	    continue;
	}
	if( obj_data *obj = object( visible ) ) {
	  if( is_set( obj->extra_flags, OFLAG_NOSHOW ) ) {
	    continue;
	  }
	}
	if( !visible->Seen( ch )
	    || *tmp && !( score = is_name( tmp, visible->Keywords( ch ), scored ) )
	    || ( number == INT_MIN && ch == visible ) )
	  continue;
	if( !scored ) {
	  count += visible->Number( );
	  if( number > 0 ) {
	    if( count < number ) 
	      continue;
	    found = true;
	    const int pos = output->find( visible );
	    if( pos == -1 ) {
	      visible->Select( 1 );
	      output->append( visible );
	    } else if( visible->Selected( ) < visible->Number( ) ) {
	      visible->Select( visible->Selected( ) + 1 );
	    }
	    i = size;	// Force i-loop to exit.
	    break;	// Exit j-loop.
	  }
	  if( number != INT_MIN && count >= -number ) {
	    found = true;
	    if( output->includes( visible ) ) {
	      visible->Select( max( visible->Selected( ), visible->Number( ) - number - count ) );
	    } else {
	      output->append( visible );
	      visible->Select( visible->Number( ) - number - count );
	    }
	    i = size;	// Force i-loop to exit.
	    break;	// Exit j-loop.
	  }
	  // Players HATE <x>*something -> "only <n> things found matching ..."
	  //	  if( number == INT_MIN )
	  found = true;
	  *output += visible;
	  visible->Select_All( );
	  
	} else if( score > best_score ) {
	  best_score = score;
	  best = visible;
	}
      }
    }
    
    if( scored && best ) {
      found = true;
      const int pos = output->find( best );
      if( pos == -1 ) {
	best->Select( 1 );
	output->append( best );
      } else if( best->Selected( ) < best->Number( ) ) {
	best->Select( best->Selected( ) + 1 );
      }
    }
    
    if( !found ) {
      if( text != empty_string ) {
	not_found( ch, array, 0, count, tmp ); 
      }
      //      delete output;  
      //      return 0;
    }
  }

  if( output->is_empty( ) ) {
    delete output;  
    return 0;
  }

  return output;
}
