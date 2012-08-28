#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include "define.h"
#include "struct.h"


obj_data *find_type( char_data *ch, thing_array& array, int type )
{
  obj_data* obj;
  
  for( int i = 0; i < array; i++ )
    if( ( obj = object( array[i] ) )
	&& obj->Seen( ch )
	&& obj->pIndexData->item_type == type ) {
      obj->Select( 1 );
      return obj;
    }

  return 0;
}


obj_data *find_vnum( thing_array& array, int vnum )
{
  obj_data* obj;
  
  for( int i = 0; i < array; i++ )
    if( ( obj = object( array[i] ) )
	&& obj->pIndexData->vnum == vnum ) {
      obj->Select( 1 );
      return obj;
    }

  return 0;
}


obj_data *find_oflag( thing_array& array, int flag )
{
  obj_data* obj;
  
  for( int i = 0; i < array; i++ ) {
    if( ( obj = object( array[i] ) )
	&& is_set( obj->extra_flags, flag ) ) {
      obj->Select( 1 );
      return obj;
    }
  }
  
  return 0;
}


/*
 *   OBJECT MANLIPULATION ROUTINES
 */


void browse( char_data *ch, const char *argument,
	     Content_Array *where, const char *msg )
{
  obj_data *obj = 0;
  char arg [ MAX_INPUT_LENGTH ];
  
  while( *argument ) {
    argument = one_argument( argument, arg );
    
    obj = one_object( ch, arg, "browse", (thing_array*) where );
    
    if( !obj ) {
      break;
    }
    
    if( !obj->pIndexData->is_container( ) ) {
      fpage( ch, "%s isn't a container.", obj );
      obj = 0;
      break;
    }
    
    where = &obj->contents;
  }
  
  if( obj ) {
    if( msg ) {
      page_title( ch, msg );
    }
    
    show_contents( ch, obj );
  }
}


void do_inventory( char_data* ch, const char *argument )
{
  char_data *victim;
  bool loaded = false;

  if( has_permission( ch, PERM_PLAYERS )
      && *argument ) {
    in_character = false;

    char arg [ MAX_INPUT_LENGTH ];

    argument = one_argument( argument, arg );

    if( pfile_data *pfile = find_pfile( arg ) ) {

      if( pfile != ch->pcdata->pfile
	  && pfile->trust >= get_trust( ch ) ) {
	fsend( ch, "You cannot view the inventory of %s.", pfile->name );
	return;
      }
      
      if( !( victim = find_player( pfile ) ) ) {
	link_data link;
	link.connected = CON_PLAYING;
	if( !load_char( &link, pfile->name, PLAYER_DIR ) ) {
	  bug( "Load_players: error reading player file. (%s)", pfile->name );
	  return;
	}
	victim = link.player;
	loaded = true;
      }
      
    } else {
      if( !( victim = one_mob( ch, arg, "inventory", (thing_array*) ch->array ) ) )
	return;
    }

    if( *argument ) {
      if( ch != victim ) {
	snprintf( arg, MAX_INPUT_LENGTH, "Inventory for %s", victim->Name( ch ) );
      }
      browse( ch, argument, &victim->contents,
	      ( ch == victim ) ? 0 : arg );

      if( loaded ) {
	page( ch, "\n\r" );
	page_centered( ch, "[ Player file was loaded from disk. ]" );
	victim->Extract();
	extracted.delete_list();
      }
      return;
    }

  } else {

    if( is_confused_pet( ch ) )
      return;
    
    if( !ch->can_carry() ) {
      send( ch, "You are unable to carry items.\n\r" );
      return;
    }

    victim = ch;
  }
  
  char        long_buf  [ MAX_STRING_LENGTH ];
  char             buf  [ MAX_STRING_LENGTH ];
  char          string  [ MAX_STRING_LENGTH ];
  obj_data*        obj;
  bool         nothing  = true;
  const char*     name;
  int             wght  = 0;
  int           i, col;

  *long_buf = '\0';
  
  for( i = 0; i < victim->contents; ++i ) {
    obj = (obj_data*) victim->contents[i];
    if( obj->pIndexData->item_type == ITEM_MONEY ) { 
      obj->Select( 0 );
      wght += obj->Weight( );
    } else if( obj->Seen( ch ) ) {
      obj->Select_All( );
    } else {
      obj->Select( 0 );
    }
  }
  
  rehash_weight( ch, victim->contents );
  
  if( victim != ch ) {
    page_title( ch, "Inventory for %s", victim );
  }

  page( ch, "Coins: %d = [%s ]    Weight: %.2f lbs\n\r\n\r",
	get_money( victim ), coin_phrase( victim ), double( wght )/100.0 );
  
  strcpy( string, "Item                          Num  Wgt" );
  page( ch, "%s%s   %s\n\r", bold_v( ch ), string, string );
  strcpy( string, "----                          ---  ---");
  page( ch, "%s   %s%s\n\r", string, string, normal( ch ) );

  char *lb = long_buf;

  for( col = i = 0; i < victim->contents; i++ ) {
    obj_data *obj = (obj_data*) victim->contents[i];
 
    if( obj->Shown( ) == 0 )
      continue;

    name = obj->Name( ch );
    nothing = false;

    if( strlen( name ) < 30 ) {
      snprintf( buf, MAX_STRING_LENGTH,
		"%-30s%3s%5s%s",
		name,
		int3( obj->Shown( ) ),
		float3( obj->temp ),
		++col%2 == 0 ? "\n\r" : "   " );
      page( ch, buf );
    } else {
      lb += sprintf( lb,
		     "%-71s%3s%5s\n\r",
		     trunc( name, 70 ).c_str(),
		     int3( obj->Shown( ) ),
		     float3( obj->temp ) );
    }
  }

  if( col%2 == 1 )
    page( ch, "\n\r" );

  if( *long_buf ) {
    if( col != 0 ) {
      page( ch, "\n\r" );
    }
    page( ch, long_buf );
  }

  if( nothing ) {
    page( ch, "< empty >\n\r" ); 
  }

  i = victim->get_burden( );

  page( ch, "\n\r  Carried: %6.2f lbs   (%s%s%s)\n\r",
    float( (double)victim->contents.weight/100.0 ),
    color_scale( ch, i ), burden_name[i], normal( ch ) );
  page( ch, "     Worn: %6.2f lbs\n\r",
    float( (double)victim->wearing.weight/100.0 ) );
  page( ch, "   Number: %6d       ( Max = %d )\n\r",
    victim->contents.number, victim->can_carry_n( ) );

  if( loaded ) {
    page( ch, "\n\r" );
    page_centered( ch, "[ Player file was loaded from disk. ]" );
    victim->Extract();
    extracted.delete_list();
  }
}


/*
 *   JUNK ROUTINE
 */


static const char *const empty_msg =
"You beg and plead to the mad goddess Neughlen for what you so foolishly\
 cast aside.  Strange, twisted laughter fills your mind with madness and\
 when it fades, you remain empty-handed.";


static const char *const junk_undo_msg =
"Realizing your foolishness, you beg and plead to Neughlen for a second\
 chance at your forfeited possessions.  Amused by your indecision and\
 pleased by your pathetic whining, Neughlen restores what was abandoned to\
 your little mortal paws.";


static thing_data *nojunk( thing_data* thing, char_data *ch, thing_data* )
{
  obj_data *obj;

  if( !( obj = object( thing ) ) )
    return thing;
 
  if( !stolen( obj, ch ) || !stolen_contents( obj, ch ) )
    return 0;

  return is_set( obj->extra_flags, OFLAG_NO_JUNK ) ? 0 : thing;
}


static thing_data *junk( thing_data* thing, char_data*, thing_data* )
{
  return thing;
}


void execute_junk( event_data* event )
{
  player_data* pc = (player_data*) event->owner;

  extract( pc->junked );
  extract( event );
}
  

void junk( char_data *ch, thing_array& array, bool pager )
{
  if( array.is_empty( ) )
    return;

  ch->Select( 1 );

  thing_array   subset  [ 3 ];
  thing_func*     func  [ 3 ]  = { cursed, nojunk, junk };

  sort_objects( ch, array, 0, 3, subset, func );

  page_priv( ch, 0, empty_string, 0, empty_string, empty_string, pager );
  page_priv( ch, &subset[0], "can't let go of", 0, empty_string, empty_string, pager );
  page_priv( ch, &subset[1], "can't junk", 0, empty_string, empty_string, pager );
  page_publ( ch, &subset[2], "junk", 0, empty_string, empty_string, 0, pager );

  if( !subset[2].is_empty() ) {
    if( player_data *pc = player( ch ) ) {
      stop_events( pc, execute_junk );
      extract( pc->junked );
      for( int i = 0; i < subset[2]; i++ ) {
        obj_data *obj = (obj_data*) subset[2][i];
        obj = (obj_data*) obj->From( obj->Selected( ) );
        obj->To( pc->junked );
      }
      event_data *event = new event_data( execute_junk, pc );
      add_queue( event, 2000 );
    } else {
      extract( subset[2] );
    }
  }

  array = subset[2];

  standard_delay( ch, subset[2] );
}


void do_junk( char_data* ch, const char *argument )
{
  player_data *pc = player( ch );

  int flags = 0;
  if( pc
      && has_permission( ch, PERM_PLAYERS )
      && !get_flags( ch, argument, &flags, "b", "junk" ) ) {
    return;
  }

  if( is_set( flags, 0 ) ) {
    in_character = false;
    char arg [ MAX_INPUT_LENGTH ];

    if( !pc || *argument ) {
      argument = one_argument( argument, arg );
      if( !( pc = one_player( ch, arg, "view the junked items of",
			      (thing_array*) &player_list ) ) ) {
	return;
      }
    }
    if( ch != pc
	&& get_trust( pc ) >= get_trust( ch ) ) {
      fsend( ch, "You cannot view the junked items of %s.", pc );
      return;
    }
    if( pc->junked.is_empty( ) ) {
      if( pc == ch ) {
	fsend( ch, "You haven't junked any items recently." );
      } else {
	fsend( ch, "%s hasn't junked any items recently.", pc );
      }
      return;
    }
    if( *argument ) {
      snprintf( arg, MAX_INPUT_LENGTH, "Junked by %s", pc->Name( ch ) );
      browse( ch, argument, &pc->junked, arg );
    } else {
      select( pc->junked );
      rehash_weight( ch, pc->junked );
      page_title( ch, "Junked by %s", pc );
      page_underlined( ch,
		       "Item                                                                    Weight\n\r" );
      for( int i = 0; i < pc->junked; ++i ) {
	if( obj_data *obj = object( pc->junked[i] ) ) {
	  if( obj->Shown( ) > 0 ) {
	    page( ch, "%-70s %7.2f\n\r",
		  obj->Seen_Name( ch, obj->Shown( ), true ),
		  0.01*(double)obj->temp );
	  }
	}
      }
    }
    return;
  }

  thing_array*  array;
  obj_data*        obj;

  if( !strcasecmp( argument, "undo" ) ) {
    if( !pc ) {
      send( ch, "Only players may junk undo.\n\r" );
      return;
    }
    if( pc->junked.is_empty() ) {
      fsend( ch, empty_msg );
      return;
    }

    fpage( ch, junk_undo_msg );
    page( ch, "\n\r" );

    page_priv( ch, 0, empty_string );
    page_priv( ch, &pc->junked, 0, 0,
	       "appears in a flash of light",
	       "appear in a flash of light" );

    // *** FIX ME: what if player can't carry the junk?
    for( int i = pc->junked-1; i >= 0; i-- ) {
      if ( ( obj = object( pc->junked[i] ) ) ) {
	obj->From( obj->Number( ) ); 
        obj->To( ch );
      }
    }

    stop_events( ch, execute_junk );
    return;
  }

  if( !( array = several_things( ch, argument, "junk", &ch->contents ) ) ) 
    return;

  junk( ch, *array );

  delete array;
}


/*
 *   DROP ROUTINES
 */


thing_data* drop( thing_data* thing, char_data* ch, thing_data* )
{
  thing = thing->From( thing->Selected( ) );
  
  if( obj_data *obj = object( thing ) )
    set_owner( obj, 0, ch );
  
  thing->To( *ch->array );

  return thing;
}


void drop( char_data *ch, thing_array& array, bool pager )
{
  if( array.is_empty( ) )
    return;

  ch->Select( 1 );

  thing_array   subset  [ 2 ];
  thing_func*     func  [ 2 ]  = { cursed, drop };
  
  sort_objects( ch, array, 0, 2, subset, func );
  
  char where [ THREE_LINES ];
  *where = '\0';

  const char *pos = ch->in_room->drop( );
  if( *pos ) {
    snprintf( where, THREE_LINES, " %s", pos );
  }

  msg_type = MSG_STANDARD;
  
  page_priv( ch, 0, empty_string, 0, empty_string, empty_string, pager );
  page_priv( ch, &subset[0], "can't let go of", 0, empty_string, empty_string, pager );
  page_publ( ch, &subset[1], "drop", 0, empty_string, where, 0, pager );

  array = subset[1];

  standard_delay( ch, subset[1] );
}


void do_drop( char_data* ch, const char *argument )
{
  if( newbie_abuse( ch ) )
    return;

  thing_array *array;

  if( !( array = several_things( ch, argument, "drop", &ch->contents ) ) ) 
    return;
 
  drop( ch, *array );

  delete array;
}
