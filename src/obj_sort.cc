#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include "define.h"
#include "struct.h"


/*
 *   THING_FUNCS
 */


thing_data *antiobj( thing_data *thing, char_data *ch, thing_data* )
{
  if( privileged( ch, LEVEL_BUILDER ) )
    return thing;

  obj_data *obj = (obj_data*) thing;
  obj_clss_data *obj_clss = obj->pIndexData;

  if( ( ch->shdata->race < MAX_PLYR_RACE && is_set( obj_clss->anti_flags, ANTI_HUMAN+ch->shdata->race ) )
      || ( !ch->species && is_set( obj_clss->anti_flags, ANTI_MAGE+ch->pcdata->clss ) )
      || is_set( obj_clss->anti_flags, ANTI_GOOD+ch->Align_Good_Evil( ) )
      || is_set( obj_clss->anti_flags, ANTI_LAWFUL+ch->Align_Law_Chaos( ) ) )
    return 0;

  if( ( ch->sex == SEX_MALE || ch->sex == SEX_HERMAPHRODITE )
      && is_set( obj_clss->anti_flags, ANTI_MALE ) )
    return 0;

  if( ( ch->sex == SEX_FEMALE || ch->sex == SEX_HERMAPHRODITE )
      && is_set( obj_clss->anti_flags, ANTI_FEMALE ) )
    return 0;

  return obj;
}


thing_data *levellimit( thing_data *thing, char_data *ch, thing_data* )
{
  if( privileged( ch, LEVEL_BUILDER ) )
    return thing;

  obj_data *obj = (obj_data*) thing;

  if( ch->Level( ) < obj->Level( ) ) 
    return 0;

  if( ch->Remort( ) < obj->pIndexData->remort )
    return 0;

  return obj;
}


thing_data *corpse( thing_data* thing, char_data* ch, thing_data* )
{
  obj_data* obj = object( thing );

  if( obj->pIndexData->item_type == ITEM_CORPSE )
    return 0;

  for( int i = 0; i < obj->contents; ++i )
    if( !corpse( obj->contents[i], ch ) )
      return 0;

  return obj;
}


thing_data *stolen( thing_data* thing, char_data* ch, thing_data* )
{
  if( obj_data *obj = object( thing ) ) {
    
    if( !obj->Belongs( ch ) )
      return 0;
    
    //    for( int i = 0; i < obj->contents; i++ )
    //      if( !stolen( obj->contents[i], ch ) )
    //	return 0;
  }

  return thing;
}


thing_data *stolen_contents( thing_data* thing, char_data* ch, thing_data* )
{
  if( obj_data *obj = object( thing ) ) {
    
    //    if( !obj->Belongs( ch ) )
    //      return 0;
    
    for( int i = 0; i < obj->contents; ++i ) {
      if( !stolen( obj->contents[i], ch )
	  || !stolen_contents( obj->contents[i], ch ) )
	return 0;
    }
  }

  return thing;
}


thing_data* same( thing_data* obj, char_data*, thing_data* container )
{
  if( obj != container
      || obj->Selected( ) < obj->Number( ) ) {
    return obj;
  }

  return 0;
}


thing_data* cursed( thing_data *thing, char_data *ch, thing_data *container )
{ 
  obj_data* obj = object( thing );

  return( obj->droppable( ch ) ? obj : 0 );
}


thing_data* cant_take( thing_data* thing, char_data* ch, thing_data* )
{
  obj_data* obj;
  
  // For mimics; thing may be a character.
  if( !( obj = object( thing ) ) )
    return 0;

  /*  
  if( obj->array != ch->array 
      || can_wear( obj, ITEM_TAKE ) )
    return thing;

  return 0;
  */

  return can_wear( obj, ITEM_TAKE ) ? thing : 0;
}


thing_data* sat_on( thing_data* thing, char_data*, thing_data* )
{
  obj_data *obj = object( thing );

  if ( !obj )
    return thing;

  if ( obj->pIndexData->item_type == ITEM_CHAIR
       && !obj->contents.is_empty() ) {
    return 0;
  }

  return obj;
}


thing_data* on_top( thing_data* thing, char_data*, thing_data* )
{
  obj_data *obj = object( thing );

  if ( !obj )
    return thing;

  if ( obj->pIndexData->item_type == ITEM_TABLE
       && !obj->contents.is_empty() ) {
    return 0;
  }

  return obj;
}


thing_data *heavy( thing_data* thing, char_data* ch, thing_data* )
{
  obj_data *obj = object( thing );

  const int n = obj->Weight( obj->Selected( ) );
  int m = ch->Capacity( );

  if( m >= n || n <= 0 ) {
    return obj;
  }

  const int w = obj->Weight( 1 );

  if( ( m /= w ) <= 0 )
    return 0;

  if( obj->array ) {  
    thing = obj->From( m, true );
    thing->To( );
  }

  return thing;
}


thing_data* many( thing_data* thing, char_data* ch, thing_data* )
{
  obj_data *obj = object( thing );

  const int n = obj->Count( obj->Selected( ) );
  const int m = ch->can_carry_n( ) - ch->contents.number;

  if( m >= n || n <= 0 ) {
    return obj;
  }

  if( m <= 0 )
    return 0;

  if( obj->array ) {
    thing = obj->From( m, true );
    thing->To( );
  }

  return thing;
}


thing_data* wont_fit( thing_data* thing, char_data*, thing_data* container )
{
  obj_data *obj = object( thing );

  const int n = obj->Weight( obj->Selected( ) );
  int m = container->Capacity( );

  if( m >= n || n <= 0 ) {
    return obj;
  }
  
  const int w = obj->Weight( 1 );

  if( ( m /= w ) <= 0 )
    return 0;

  thing = obj->From( m, true );
  thing->To( );

  return thing;
}


thing_data* no_room( thing_data* thing, char_data* ch, thing_data* )
{ 
  player_data*  pc  = (player_data*) ch;
  obj_data*    obj  = (obj_data*) thing;

  const int      n  = obj->Weight( obj->Selected( ) );
  int      m  = BANK_WEIGHT + ( pc->remort * BANK_BONUS ) - pc->locker.weight;

  if( obj->pIndexData->item_type == ITEM_MONEY )
    return obj; 

  if( m >= n || n <= 0 ) {
    return obj;
  }

  const int w = thing->Weight(1);

  if( ( m /= w ) <= 0 )
    return 0;

  thing = obj->From( m, true );
  thing->To( );

  return thing;
}


thing_data *to_char( thing_data* thing, char_data* ch, thing_data* )
{
  thing = thing->From( thing->Selected( ) );
  thing->To( ch );

  return thing;
}


/*
 *   REHASH  
 */



/* This function takes a thing_array, and copies the number of elements in each
   step of the array into the appropriate selected field. */
int select( thing_array& list )
{
  for( int i = 0; i < list; ++i ) {
    list[i]->Select_All( );
  }

  return list.size;
}  


/* This function takes a thing_array and a character, moving the number
   of elements in each stop of the array that the character can see into the
   appropriate selected field. */
int select( thing_array& list, char_data* ch )
{
  int total = 0;

  for( int i = 0; i < list; ++i ) {
    if( list[i]->Seen( ch ) ) {
      list[i]->Select_All( );
      ++total;
    } else {
      list[i]->Select( 0 );
    }
  }

  return total;
}  


/* This function steps through the array, moving all of the selected items
   into the shown field.  Then it steps through the array, searching to see
   if any elements look the same to the character.  If any elements do,
   it combines them.  The result is that the shown field holds the number
   of elements of the given description that the character can see. */
void rehash( char_data* ch, thing_array& list, bool auction )
{
  for( int i = 0; i < list; ++i ) {
    list[i]->Show( list[i]->Selected( ) );
  }
  
  for( int i = 0; i < list-1; ++i ) {
    if( list[i]->Shown( ) > 0 ) {
      for( int j = i+1; j < list; ++j ) {
	if( list[j]->Shown( ) > 0 && look_same( ch, list[i], list[j], auction ) ) {
	  list[i]->Show( list[i]->Shown( ) + list[j]->Shown( ) );
	  list[i]->temp += list[j]->temp;
	  list[j]->Show( 0 );
	  list[j]->temp = 0;
	}
      }
    }
  }
}


void rehash_weight( char_data* ch, thing_array& list, bool auction )
{
  thing_data* t1;
  thing_data* t2;
  
  for( int i = 0; i < list; i++ ) {  
    t1 = list[i];
    t1->Show( t1->Selected( ) );
    t1->temp = t1->Weight( t1->Shown( ) );
  }
  
  for( int i = 0; i < list-1; ++i ) 
    if( ( t1 = list[i] )->Shown( ) > 0 ) 
      for( int j = i+1; j < list; ++j ) 
        if( ( t2 = list[j] )->Shown( ) > 0 && look_same( ch, t1, t2, auction ) ) {
          t1->Show( t1->Shown( ) + t2->Shown( ) );
          t1->temp += t2->temp;
          t2->Show( 0 );
	  t2->temp = 0;
	}
}


/*
 *   SORT ROUTINE
 */


void sort_objects( char_data* ch, thing_array& array, thing_data* container,
		   int n, thing_array* subset, thing_func** func )
{
  for( int i = 0; i < array; ++i ) {
    thing_data *thing = array[i];
    for( int j = 0; ; ++j ) {
      thing_data *unsorted = ( func[j] ? ( *func[j] )( thing, ch, container ) : thing );
      if( j == n-1 ) {
	if( unsorted ) {
	  subset[j] += unsorted;
	}
	break;
      }
      if( unsorted != thing ) {
        subset[j] += thing;
      }
      if( !( thing = unsorted ) ) {
        break;
      }
    }
  }
}


/*
 *   LIST ROUTINES
 */


bool first_list = true;
bool prev_long = false;


void page_priv( char_data* ch, thing_array* array, const char* msg1,
		thing_data* container, const char* msg2, const char* msg3,
		bool pager )
{
  if( !array ) {
    first_list = true;
    prev_long  = false;
    return;
  }

  if( none_shown( *array ) ) {
    return;
  }

  /* With multiple items, we always set include_closed before listing them.
     So, we need to set it for rehash, too.
     But, we retain the old value for containers and single contents.
     FIX ME: add separate include_* variables for container vs. contents.
  */
  bool save_closed = include_closed;
  include_closed = true;

  rehash( ch, *array );

  include_closed = save_closed;

  if( thing_data *thing = one_shown( *array ) ) {
    if( prev_long ) {
      if( pager ) {
	page( ch, "\n\r" );
      } else {
	send( ch, "\n\r" );
      }
    }
    if( !container ) {
      if( !msg1 ) {
	if( pager ) {
	  fpage( ch, "%s %s.", thing, 
		 thing->Shown( ) == 1 ? msg2 : msg3 );
	} else {
	  fsend( ch, "%s %s.", thing, 
		 thing->Shown( ) == 1 ? msg2 : msg3 );
	}
      } else if( msg2 != empty_string ) {
	if( pager ) {
	  fpage( ch, "%s %s %s.", msg1,
		 thing, msg2 );
	} else {
	  fsend( ch, "%s %s %s.", msg1,
		 thing, msg2 );
	}
      } else {
	if( pager ) {
	  fpage( ch, "You %s %s.", msg1, thing );
	} else {
	  fsend( ch, "You %s %s.", msg1, thing );
	}
      }
    } else if( container == thing ) {
      if( pager ) {
	fpage( ch, "You %s %s %s.", msg1,
	       thing, msg2 );
      } else {
	fsend( ch, "You %s %s %s.", msg1,
	       thing, msg2 );
      }
    } else {
      char tmp [ SIX_LINES ];
      snprintf( tmp, SIX_LINES, "%s", tostring( container, ch ) );
      include_closed = true;
      if( msg2 != empty_string ) {
	if( msg3 != empty_string ) {
	  if( pager ) {
	    fpage( ch, "You %s %s %s %s %s.", msg1, thing, msg2, tmp, msg3 );
	  } else {
	    fsend( ch, "You %s %s %s %s %s.", msg1, thing, msg2, tmp, msg3 );
	  }
	} else {
	  if( pager ) {
	    fpage( ch, "You %s %s %s %s.", msg1, thing, msg2, tmp );
	  } else {
	    fsend( ch, "You %s %s %s %s.", msg1, thing, msg2, tmp );
	  }
	}
      } else if( msg3 != empty_string ) {
	if( pager ) {
	  fpage( ch, "%s %s you %s %s.", msg3, tmp, msg1, thing );
	} else {
	  fsend( ch, "%s %s you %s %s.", msg3, tmp, msg1, thing );
	}
      } else {
	if( pager ) {
	  fpage( ch, "%s %s %s.", tmp, msg1, thing );
	} else {
	  fsend( ch, "%s %s %s.", tmp, msg1, thing );
	}
      }
    }
    first_list = false;
    prev_long  = false;
    return;
  }
  
  if( !first_list ) {
    if( pager ) {
      page( ch, "\n\r" );
    } else {
      send( ch, "\n\r" );
    }
  }
  
  first_list = false;
  prev_long  = true;
  
  if( !container ) {
    if( !msg1 ) {
      if( pager ) {
	fpage( ch, "%s:", msg3 );
      } else {
	fsend( ch, "%s:", msg3 );
      }
    } else if( msg2 != empty_string ) {
      if( pager ) {
	fpage( ch, "%s %s:", msg1, msg2 ); 
      } else {
 	fsend( ch, "%s %s:", msg1, msg2 ); 
      }
    } else {
      if( pager ) {
	fpage( ch, "You %s:", msg1 );
      } else {
	fsend( ch, "You %s:", msg1 );
      }
    }
  } else if( msg2 != empty_string ) {
    if( msg3 != empty_string ) {
      if( pager ) {
	fpage( ch, "You %s %s %s %s:", msg1, msg2, container, msg3 );
      } else {
	fsend( ch, "You %s %s %s %s:", msg1, msg2, container, msg3 );
      }
    } else {
      if( pager ) {
	fpage( ch, "You %s %s %s:", msg1, msg2, container );
      } else {
	fsend( ch, "You %s %s %s:", msg1, msg2, container );
      }
    }
  } else if( msg3 != empty_string ) {
    if( pager ) {
      fpage( ch, "%s %s you %s:", msg3, container, msg1 );
    } else {
      fsend( ch, "%s %s you %s:", msg3, container, msg1 );
    }
  } else {
    if( pager ) {
      fpage( ch, "%s %s:", container, msg1 );
    } else {
      fsend( ch, "%s %s:", container, msg1 );
    }
  }

  include_closed = true;

  for( int i = 0; i < *array; i++ ) {
    thing_data *thing = array->list[i];
    if( thing->Shown( ) > 0 ) {
      if( pager ) {
	page( ch, "  %s\n\r", thing );
      } else {
	send( ch, "  %s\n\r", thing );
      }
    }
  }
}


void page_publ( char_data* ch, thing_array* array, const char* msg1,
		thing_data* container, const char* msg2, const char* msg3, const char *msg4,
		bool pager )
{
  if( none_shown( *array ) ) {
    return;
  }

  /* TO CHARACTER */

  rehash( ch, *array );

  const bool for3 = !strcmp( msg3, "for" );

  if( thing_data *thing = one_shown( *array ) ) { 
    if( prev_long ) {
      if( pager ) {
	page( ch, "\n\r" ); 
      } else {
	send( ch, "\n\r" ); 
      }
    }
    if( for3 ) {
      if( pager ) {
	fpage( ch, "You %s %s %s %s for %d cp.",
	       msg1, thing, msg2, container, thing->temp );
      } else {
	fsend( ch, "You %s %s %s %s for %d cp.",
	       msg1, thing, msg2, container, thing->temp );
      }
    } else if( container ) {
      if( pager ) {
	fpage( ch, "You %s %s %s %s%s.",
	       msg1, thing, msg2, container, msg3 );
      } else {
	fsend( ch, "You %s %s %s %s%s.",
	       msg1, thing, msg2, container, msg3 );
      }
    } else {
      if( pager ) {
	fpage( ch, "You %s %s%s.", msg1, thing, msg3 );
      } else {
	fsend( ch, "You %s %s%s.", msg1, thing, msg3 );
      }
    }
  } else {
    if( !first_list ) {
      if( pager ) {
	page( ch, "\n\r" );
      } else {
	send( ch, "\n\r" );
      }
    }
    if( for3 ) {
      if( pager ) {
	fpage( ch, "You %s %s %s:",
	       msg1, msg2, container );
      } else {
	fsend( ch, "You %s %s %s:",
	       msg1, msg2, container );
      }
    } else if( container ) {
      if( pager ) {
	fpage( ch, "You %s %s %s%s:",
	       msg1, msg2, container, msg3 );
      } else {
	fsend( ch, "You %s %s %s%s:",
	       msg1, msg2, container, msg3 );
      }
    } else {
      if( pager ) {
	fpage( ch, "You %s%s:", msg1, msg3 );
      } else {
	fsend( ch, "You %s%s:", msg1, msg3 );
      }
    }
    for( int i = 0; i < *array; ++i ) {
      thing = array->list[i];
      if( thing->Shown( ) > 0 ) {
        if( for3 ) {
	  if( pager ) {
	    page( ch, "  %s for %d cp\n\r",
		  thing, thing->temp );
	  } else {
	    send( ch, "  %s for %d cp\n\r",
		  thing, thing->temp );
	  }
        } else {
	  if( pager ) {
	    page( ch, "  %s\n\r", thing );
	  } else {
	    send( ch, "  %s\n\r", thing );
	  }
	}
      }
    }
  }

  first_list = false;


  /* TO ROOM OCCUPANTS */

  // Irregular plural.
  if( msg4 && *msg4 ) {
    msg1 = msg4;
  }

  char_data *rch;

  for( int i = 0; i < *ch->array; i++ ) { 
    if( !( rch = character( ch->array->list[i] ) )
	|| rch == ch
	|| ( !ch->Seen( rch ) && rch != character( container ) )
	|| !rch->Accept_Msg( ch ) )
      continue;

    rehash( rch, *array );

    if( thing_data *thing = one_shown( *array ) ) {
      if( for3 ) {
        fsend( rch, "%s %ss %s %s %s.",
	       ch, msg1, thing, msg2, container );
      } else if( container ) {
        fsend( rch, "%s %ss %s %s %s%s.",
	       ch, msg1, thing, msg2,
	       container == rch ? "you" : container->Name( rch ),
	       msg3 );
      } else {
        fsend( rch, "%s %ss %s%s.", ch, msg1, thing, msg3 );
      }
      continue;
    }
    
    if( rch->pcdata && !is_set( rch->pcdata->message, MSG_MULTIPLE_ITEMS ) ) {
      if( for3 ) {
        fsend( rch, "%s %ss several items %s %s.",
	       ch, msg1, msg2, container );
      } else if( container ) {
        fsend( rch, "%s %ss several items %s %s%s.",
	       ch, msg1, msg2,
	       container == rch ? "you" : container->Name( rch ),
	       msg3 );
      } else {
        fsend( rch, "%s %ss several items%s.", ch, msg1, msg3 );
      }
      continue;
    }
    
    if( for3 ) {
      fsend( rch, "%s %ss %s %s:",
	     ch, msg1, msg2, container );
    } else if( container ) {
      fsend( rch, "%s %ss %s %s%s:",
	     ch, msg1, msg2,
	     container == rch ? "you" : container->Name( rch ),
	     msg3 );
    } else {
      fsend( rch, "%s %ss%s:", ch, msg1, msg3 );
    }
    
    for( int j = 0; j < *array; j++ ) {
      thing_data *thing = array->list[j];
      if( thing->Shown( ) > 0 ) {
        send( rch, "  %s\n\r", thing );
      }
    }
  }
}


/*
 *   SEND_PRIV
 */


void send_priv( char_data *ch, thing_array* array, const char* msg1,
		thing_data* container )
{
  rehash( ch, *array );

  if( none_shown( *array ) ) {
    fsend( ch, "%s %s nothing.", container, msg1 );
    return;
  }

  if( thing_data *thing = one_shown( *array ) ) {
    fsend( ch, "%s %s %s.", container, msg1, thing );
    return;
  }
  
  fsend( ch, "%s %s:", container, msg1 );
  
  for( int i = 0; i < *array; i++ ) {
    thing_data *thing = array->list[i];
    if( thing->Shown( ) > 0 ) {
      send( ch, "  %s\n\r", thing );
    }
  }
}


/*
 *   SEND_PUBL
 */

void send_publ( char_data* ch, thing_array* array,
		const char *msg1,
		const char *msg2 )
{
  char_data *rch;

  for( int i = 0; i < *ch->array; i++ ) {
    if( !( rch = character( ch->array->list[i] ) )
	|| rch == ch
	|| !ch->Seen( rch ) )
      continue;
    
    select( *array, rch );
    rehash( rch, *array );

    if( none_shown( *array ) ) {
      fsend( rch, "%s %s.", ch, msg1 );
    } else if( thing_data *thing = one_shown( *array ) ) {
      fsend( rch, "%s %s, %s %s.", ch, msg1, msg2, thing );
    } else {
      fsend( rch, "%s %s, %s:", ch, msg1, msg2 );
      for( int j = 0; j < *array; j++ ) {
        thing_data *thing = array->list[j];
        if( thing->Shown( ) > 0 ) {
          send( rch, "  %s\n\r", thing );
	}
      }
    }
  }
}
