#include <ctype.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include "define.h"
#include "struct.h"


const char *dflag_name [MAX_DFLAG] = {
  "is.door", "closed", "locked", "secret",
  "pickproof", "no.show", "no.open", "reset.closed", "reset.locked",
  "reset.open", "requires.climb", "searchable",
  "plural", "no.scan", "no.mob", "no.pass", "no.blink", "no.flee",
  "no.path" };


/*
 *   EXIT_DATA CLASS
 */


static void dereference( char_data *wch, exit_data *exit )
{
  if( !wch->Is_Valid( ) )
    return;

  for( int i = wch->events.size-1; i >= 0; --i ) {
    event_data *event = wch->events[i];
    if( event->func == execute_path ) {
      path_data *path = (path_data*) event->pointer;
      path->ignore -= exit;
    }
  }
}


static void dereference( room_data *room, exit_data *exit )
{
  for( int i = room->events.size-1; i >= 0; --i ) {
    event_data *event = room->events[i];
    if( event->func == update_affect ) {
      affect_data *affect = (affect_data*) event->pointer;
      if( affect->target == exit ) {
	remove_affect( room, affect );
	extract( event );
      }
    }
  }
}


Exit_Data :: Exit_Data( )
  : to_room(0), exit_info(0), save_info(0), key(-1),
    size(SIZE_OGRE), direction(0), light(0), affected_by(0),
    name(empty_string), keywords(empty_string)
{
  record_new( sizeof( exit_data ), MEM_EXIT );
}


Exit_Data :: ~Exit_Data( )
{
  record_delete( sizeof( exit_data ), MEM_EXIT );

  free_string( name,     MEM_EXIT );
  free_string( keywords, MEM_EXIT );

  // Currently only modifies path affects.
  //  for( int i = 0; i < player_list; ++i ) 
  //    dereference( player_list[i], this );

  for( int i = 0; i < mob_list; i++ ) 
    dereference( mob_list[i], this );
}


/*
const char* Exit_Data :: Seen_Name( const char_data* ch, int, bool ) const
{
  return Name( ch );
}
*/


const char *Exit_Data :: Name( const char_data* ch, int, bool ) const
{
  char* tmp = static_string( );

  const char *exit_name = ( name && name != empty_string ) ? name : "exit";

  if( ch ) {
    if( !ch->pcdata
	|| !is_set( ch->pcdata->message, MSG_DOOR_DIRECTION ) ) {
      if( !Seen( ch )
	  || !ch->in_room->Seen( ch ) ) {
	return "something";
      }
      snprintf( tmp, THREE_LINES, "the %s", exit_name );
      return tmp;
    }
    
    if( !Seen( ch ) 
	|| !ch->in_room->Seen( ch ) ) {
      snprintf( tmp, THREE_LINES, "something %s",
		dir_table[ direction ].where );
      return tmp;
    }
  }

  snprintf( tmp, THREE_LINES, "the %s %s", exit_name,
	    dir_table[ direction ].where );  
  return tmp;
}


const char *exit_verb( char_data * ch, exit_data *exit )
{
  if( !exit->name
      || !*exit->name
      || !is_set( exit->exit_info, EX_PLURAL )
      || !exit->Seen( ch ) ) {
    return "is";
  }

  return "are";
}


const char *Exit_Data :: Keywords( char_data* ch )
{
  char *tmp = static_string( );

  snprintf( tmp, THREE_LINES, "%s 1 %s %s",
	    keywords, dir_table[ direction ].name,
	    Name( ch ) );

  return tmp;
}


bool Exit_Data :: Seen( const char_data* ch ) const
{
  if( !is_set( exit_info, EX_CLOSED ) ) 
    return true;

  // Note: switched imms' holylight works.
  if( has_holylight( ch ) )
    return true;

  if( is_set( exit_info, EX_NO_SHOW ) )
    return false;
 
  if( is_set( exit_info, EX_SECRET ) 
      && ch
      && !ch->seen_exits.includes( const_cast<exit_data *>( this ) ) ) 
    return false;

  return true;
}


/*
 *   DISK ROUTINES
 */


static exit_data *add_exit( room_data* room, int dir )
{
  exit_data *exit = new exit_data;
  exit->direction = (unsigned char)dir;

  for( int i = 0; ; ++i )
    if( i == room->exits.size || dir < (int) room->exits[i]->direction ) {
      insert( room->exits.list, room->exits.size, exit, i );
      break;
    }
  
  return exit;
}


void read_exits( FILE* fp, room_data* room, int vnum )
{
  char        letter;

  while (true) {
    if( ( letter = fread_letter( fp ) ) != 'D' )
      break;

    exit_data *exit = add_exit( room, fread_number( fp ) );

    exit->name        = fread_string( fp, MEM_EXIT );
    exit->keywords    = fread_string( fp, MEM_EXIT );
    exit->exit_info   = fread_number( fp );
    exit->key         = fread_number( fp );
    exit->to_room     = (room_data*) fread_number( fp );
    exit->light       = (unsigned char)fread_number( fp );
    exit->size        = (unsigned char)fread_number( fp );

    if( exit->direction > MAX_DIR_COMPASS ) 
      panic( "Fread_rooms: vnum %d has bad exit direction.", vnum );
  }

  ungetc( letter, fp );
}


/*
 *   DISPLAY ROUTINES
 */


static void show_dflags( char_data *ch, exit_data *exit )
{
  bool found = false;

  for( int i = 0; i < MAX_DFLAG; ++i ) {
    if( i == EX_CLOSED
	|| !is_set( exit->exit_info, i ) )
      continue;
    if( !found ) {
      send( ch, "      ( %s", dflag_name[i] );
      found = true;
    } else {
      send( ch, ", %s", dflag_name[i] );
    }
  }

  if( found )
    send( ch, " )\n\r" );
}


void do_exits( char_data* ch, const char * )
{
  if( !ch->Can_See( true ) )
    return;

  bool found = false;
  char *buf = static_string( );

  for( int i = 0; i < ch->in_room->exits; ++i ) {
    exit_data *exit = ch->in_room->exits[i];
    if( !exit->Seen( ch ) )
      continue;
    if( !found ) {
      send_underlined( ch, "Obvious Exits\n\r" );
      found = true;
    }
    if( is_apprentice( ch ) ) {
      if( !is_set( exit->exit_info, EX_CLOSED ) ) {
	if( exit->name != empty_string ) {
	  strcpy( buf, exit->name );
	  capitalize( buf );
	  send( ch, "%-5s - %s to %s [%d]\n\r",
		dir_table[ exit->direction ].name,
		buf,
		exit->to_room->name,
		exit->to_room->vnum );
	} else {
	  send( ch, "%-5s - %s [%d]\n\r",
		dir_table[ exit->direction ].name,
		exit->to_room->name,
		exit->to_room->vnum );
	}
      } else {
	if( exit->name != empty_string ) {
	  send( ch, "%-5s - Closed %s to %s [%d]\n\r",
		dir_table[ exit->direction ].name,
		exit->name,
		exit->to_room->name,
		exit->to_room->vnum
		);
	} else {
	  send( ch, "%-5s - Closed exit to %s [%d]\n\r",
		dir_table[ exit->direction ].name,
		exit->to_room->name,
		exit->to_room->vnum
		);
	}
      }
      show_dflags( ch, exit );
    } else {
      if( !is_set( exit->exit_info, EX_CLOSED ) ) {
	send( ch, "%-5s - %s\n\r",
	      dir_table[ exit->direction ].name,
	      exit->to_room->is_dark( ch ) ? "Too dark to tell" : exit->to_room->name );
      } else {
	send( ch, "%-5s - Closed %s\n\r",
	      dir_table[ exit->direction ].name, exit->name );
      }
    }
  }
  
  if( !found )
    send( ch, "You see no obvious exits.\n\r" );
}


int exits_prompt( char* tmp, char_data* ch, int color )
{
  exit_data*  exit;
  char*     string  = tmp;
  bool       water  = false;
  int        exits  = 0;

  if( !ch->Can_See( ) ) {
    strcpy( tmp, "??" );
    return 2;
  }
  
  if( !ch->in_room->Seen( ch ) ) {
    strcpy( tmp, "-Dark-" );
    return 6;
  }
  
  for( int i = 0; i < ch->in_room->exits; i++ ) {
    exit = ch->in_room->exits[i];
    
    if( !exit->Seen( ch ) )
      continue;
    
    if( !is_set( exit->exit_info, EX_CLOSED ) 
	&& water_logged( exit->to_room ) ) {
      if( !water ) {
        strcpy( string, blue( ch ) );
        string += strlen( string );
        water = true;
      }
    } else if( water ) {
      strcpy( string, color_code( ch, color ) );
      string += strlen( string );
      water = false;
    }
    
    *string = *dir_table[ exit->direction ].name;
    
    if( !is_set( exit->exit_info, EX_CLOSED ) )
      *string = toupper( *string );
    
    ++exits;
    ++string;
  } 
  
  if( water ) {
    strcpy( string, color_code( ch, color ) );
  } else if( string == tmp ) {
    strcpy( tmp, "none" );
    return 4;
  } else {
    *string = '\0';
  }
  
  return exits;
}  


void autoexit( char_data* ch )
{ 
  if( !is_set( ch->pcdata->pfile->flags, PLR_AUTO_EXIT ) )
    return;

  char buf [ FOUR_LINES ];

  *buf = '\0';

  for( int i = 0; i < ch->in_room->exits; i++ ) {
    exit_data *exit = ch->in_room->exits[i];
    if( !exit->Seen( ch ) )
      continue;
    if( !*buf )
      strcpy( buf, "[Exits:" );
    room_data *room = exit->to_room;
    if( is_builder( ch ) ) {
      int len = strlen( buf );
      snprintf( buf+len, FOUR_LINES-len, " #%d", room->vnum  );
    }
    int len = strlen( buf );
    if( water_logged( room ) ) 
      snprintf( buf+len, FOUR_LINES-len, " %s%s%s", blue( ch ),
		dir_table[ exit->direction ].name, normal( ch ) );
    else
      snprintf( buf+len, FOUR_LINES-len, " %s",
		dir_table[ exit->direction ].name );
  }
  
  if( !*buf ) {
    send( ch, "[Exits: none]\n\r" );
  } else {
    strcat( buf, "]\n\r" );
    send( ch, buf );
  }
}


/*
 *   RANDOM EXIT ROUTINE
 */


exit_data *random_movable_exit( char_data *ch, room_data *room,
				bool open, bool blink, bool flee )
{
  if( !room )
    room = ch->in_room;

  exit_array exits;
  
  for( int i = 0; i < room->exits; ++i ) {
    exit_data *exit = room->exits[i];
    if( blink
	&& is_set( exit->exit_info, EX_NO_BLINK ) )
      continue;
    if( flee
	&& is_set( exit->exit_info, EX_NO_FLEE ) )
      continue;
    int move, type;
    if( ch->Can_Move( 0, room, exit, (int)exit->direction, move, type, false, open ) ) {
      exits.append( exit );
      // If there's a player/mob that we're aggro to, give that exit preference.
      bool wander = !ch->pcdata
	&& !is_set( ch->status, STAT_PET )
	&& !is_set( ch->status, STAT_FORCED )
	&& !blink
	&& !flee;
      if( wander
	  && ch->hit > ch->max_hit / 2
	  && scan_aggro( ch, exit ) ) {
	exits.append( exit );
	if( is_set( ch->species->act_flags, ACT_PREDATOR ) ) {
	  exits.append( exit );
	}
      }
    }
  }

  if( exits.size == 0 ) {
    return 0;
  }

  return exits[ number_range( 0, exits.size - 1 ) ];
}


/*
exit_data* random_open_exit( char_data *ch, room_data* room )
{
  int count = 0;

  // If you can't fit, it's not really open.

  for( int i = 0; i < room->exits; i++ ) {
    exit_data *exit = room->exits[i];
    if( !is_set( exit->exit_info, EX_CLOSED )
	&& ch->Size( ) <= exit->size
	&& ch->Size( ) <= exit->to_room->size
	&& ( !ch->mount
	     || ch->mount->in_room != room
	     || ch->mount->Size( ) <= exit->size
	        && ch->mount->Size( ) <= exit->to_room->size ) ) {
      ++count;
    }
  }

  if( count == 0 )
    return 0;

  count = number_range( 1, count );

  for( int i = 0; i < room->exits; i++ ) {
    exit_data *exit = room->exits[i];
    if( !is_set( exit->exit_info, EX_CLOSED )
	&& ch->Size( ) <= exit->size
	&& ch->Size( ) <= exit->to_room->size
	&& ( !ch->mount
	     || ch->mount->in_room != room
	     || ch->mount->Size( ) <= exit->size
	        && ch->mount->Size( ) <= exit->to_room->size )
	&& --count == 0  )
      return exit;
  }

  return 0;
}
*/


/*
exit_data* random_exit( room_data* room )
{
  if( room->exits == 0 )
    return 0;

  int i = number_range( 0, room->exits.size-1 );

  return room->exits[i];
}
*/



/*
 *   SUPPORT ROUTINES
 */


int direction_arg( const char *& argument )
{
  for( int i = 0; i < MAX_DOOR; i++ )
    if( matches( argument, dir_table[i].name ) )
      return i;

  return -1;
}


exit_data* exit_direction( room_data* room, int door )
{
  for( int i = 0; i < room->exits; ++i )
    if( (int)room->exits[i]->direction == door )
      return room->exits[i];

  return 0;
}


exit_data *reverse( exit_data *exit )
{
  if( exit->direction >= MAX_DIR_COMPASS )
    return 0;

  exit_array& array = exit->to_room->exits;
  int door = dir_table[ exit->direction ].reverse;

  for( int i = 0; i < array; ++i )
    if( (int)array[i]->direction == door )
      return array[i];

  return 0;
}


/*
 *   OPEN/CLOSE ROUTINES
 */


static void open_object( char_data* ch, obj_data* obj )
{
  if( obj->pIndexData->item_type != ITEM_CONTAINER ) {
    fsend( ch, "%s is not a container.", obj );
    return;
  }
  
  if( !is_set( obj->value[1], CONT_CLOSED ) ) {
    fsend( ch, "%s is already open.", obj );
    return;
  }
  
  if( !is_set( obj->value[1], CONT_CLOSEABLE ) ) {
    fsend( ch, "%s isn't something you can open.", obj );
    return;
  }
  
  if( is_set( obj->value[1], CONT_LOCKED ) ) {
    fsend( ch, "%s is locked.", obj );
    return;
  }

  for( int i = obj->contents.size - 1; i >= 0; i-- ) {
    obj_data *trap;
    if ( ( trap = object( obj->contents[i] ) )
	 && trap->pIndexData->item_type == ITEM_TRAP ) {
      oprog_data *oprog;
      for( oprog = trap->pIndexData->oprog; oprog; oprog = oprog->next ) {
	if( oprog->trigger == OPROG_TRIGGER_USE ) {
	  trap->Select( 1 );
	  push( );
	  clear_variables( );
	  var_obj = trap;
	  var_ch = ch;    
	  var_room = ch->in_room;
	  var_container = obj;
	  oprog->execute( );
	  pop( );
	  return;
	}
      }
      if( !oprog ) {
	bug( "Inside trap object %d has no USE trigger.",
	    trap->pIndexData->vnum );
	trap->Extract();
      }
    }
  }

  if( obj->array->where != ch )
    leave_shadows( ch );

  obj = (obj_data *) obj->From( 1, true );
  remove_bit( obj->value[1], CONT_CLOSED );
  obj->To( );

  obj_act_spam( MSG_STANDARD, ch, obj, 0, "open", "opens" );

  set_delay( ch, 10 );
}


const default_data open_msg [] =
{
  { "to_char", "You open $d.", -1 },
  { "to_room", "$n opens $d.", -1 },
  { "to_side", "$d opens.", -1 },
  { "", "", -1 }
};


bool open_door( char_data* ch, exit_data* exit ) 
{
  if( !is_set( exit->exit_info, EX_ISDOOR ) ) {
    send( ch, "There is no door or wall there.\n\r" );
    return false;
  }
  
  if( !is_set( exit->exit_info, EX_CLOSED ) ) {
    fsend( ch, "%s %s already open.",
	   exit, exit_verb( ch, exit ) );
    return false;
  }
  
  if( is_set( exit->exit_info, EX_LOCKED ) ) {
    fsend( ch, "%s %s locked.",
	   exit, exit_verb( ch, exit ) );
    return false;
  }

  if( is_set( exit->exit_info, EX_NO_OPEN ) ) {
    fsend( ch, "You see no way to open %s.", exit );
    return false;
  }

  if( is_set( exit->affected_by, AFF_WIZLOCK ) ) {
    fsend( ch, "%s seems to be jammed shut.", exit );
    return false;
  }

  leave_shadows( ch );

  room_data *room = ch->in_room;

  action_data *action;

  for( action = room->action; action; action = action->next )
    if( action->trigger == TRIGGER_OPEN_DOOR
	&& is_set( action->flags, (int)exit->direction ) ) {
      push( );
      clear_variables( );
      var_ch = ch;
      var_room = room;
      var_exit = exit;
      var_def = open_msg;
      var_def_type = -1;
      const int result = action->execute( );
      pop( );
      if( !result
	  || ch->in_room != room )
        return false;
      break;
    }
  
  act( ch, prog_msg( action, open_msg[0] ), 0, 0, 0, 0, exit );
  act_notchar( prog_msg( action, open_msg[1] ), ch, 0, 0, 0, exit );
  remove_bit( exit->exit_info, EX_CLOSED );

  if( exit_data *back = reverse( exit ) ) {
    room_data *to = exit->to_room; 
    act_room( to, prog_msg( action, open_msg[2] ), ch, 0, 0, 0, back );
    if( is_set( back->affected_by, AFF_WIZLOCK ) ) {
      for( int i = 0; i < to->affected; ++i ) {
	affect_data *aff = to->affected[i];
	if( aff->type == AFF_WIZLOCK
	    && aff->target == back ) {
	  remove_affect( to, aff );
	  break;
	}
      }
    }
    remove_bit( back->exit_info, EX_CLOSED );
  }

  set_delay( ch, 10 );
  return true;
}


void do_open( char_data* ch, const char *argument )
{
  visible_data *vis;
  if( !( vis = one_visible( ch, argument, "open",
			    (visible_array *) &ch->in_room->exits, EXIT_DATA,
			    (visible_array *) ch->array, OBJ_DATA,
			    (visible_array *) &ch->contents, OBJ_DATA,
			    (visible_array *) &ch->wearing, OBJ_DATA ) ) )
    return;

  if( obj_data *obj = object( vis ) ) {
    if( !is_confused_pet( ch ) ) {
      include_closed = false;
      open_object( ch, obj );
      include_closed = true;
    }
    return;
  }
  
  if( exit_data *door = exit( vis ) ) {
    if( ch->pcdata
	|| !ch->leader
	|| !is_set( ch->status, STAT_ORDERED )
	|| is_set( ch->species->act_flags, ACT_OPEN_DOORS ) ) {
      open_door( ch, door );
    }
    return;
  }

  //  send( ch, "%s isn't something you can open.\n\r", thing );
}


/*
 *   CLOSE
 */


static void close_object( char_data* ch, obj_data* obj )
{
  if( obj->pIndexData->item_type != ITEM_CONTAINER ) {
    fsend( ch, "%s isn't a container.", obj );
    return;
  }
  
  if( is_set( obj->value[1], CONT_CLOSED ) ) {
    fsend( ch, "%s is already closed.", obj );
    return;
  }
  
  if( !is_set( obj->value[1], CONT_CLOSEABLE ) ) {
    fsend( ch, "You can't close %s.", obj );
    return;
  }

  Content_Array *array = obj->array;

  if( array->where != ch )
    leave_shadows( ch );

  obj = (obj_data *) obj->From( 1, true );
  set_bit( obj->value[1], CONT_CLOSED );
  obj->To( );
  
  obj_act_spam( MSG_STANDARD, ch, obj, 0, "close", "closes" );

  set_delay( ch, 10 );
}


const default_data close_msg [] =
{
  { "to_char",  "You close $d.", -1 },
  { "to_room",  "$n closes $d.", -1 },
  { "to_side",  "$d closes.", -1 },
  { "", "", -1 }
};


bool close_door( char_data* ch, exit_data* exit ) 
{
  if( !is_set( exit->exit_info, EX_ISDOOR ) ) {
    send( ch, "There is nothing there that closes.\n\r" );
    return false;
  }
  
  if( is_set( exit->exit_info, EX_CLOSED ) ) {
    fsend( ch, "%s %s already closed.",
	   exit, exit_verb( ch, exit ) );
    return false;
  }
  
  if( is_set( exit->exit_info, EX_NO_OPEN ) ) {
    fsend( ch, "You don't see any way to close %s.", exit );
    return false;
  }

  leave_shadows( ch );

  room_data *room = ch->in_room;

  action_data *action;

  for( action = room->action; action; action = action->next )
    if( action->trigger == TRIGGER_CLOSE_DOOR
	&& is_set( action->flags, (int)exit->direction ) ) {
      push( );
      clear_variables( );
      var_ch = ch;
      var_room = room;
      var_exit = exit;
      var_def = close_msg;
      var_def_type = -1;
      const int result = action->execute( );
      pop( );
      if( !result
	  || ch->in_room != room )
        return false;
      break;
    }

  act( ch, prog_msg( action, close_msg[0] ), 0, 0, 0, 0, exit );
  act_notchar( prog_msg( action, close_msg[1] ), ch, 0, 0, 0, exit );
  set_bit( exit->exit_info, EX_CLOSED );

  if( exit_data *back = reverse( exit ) ) {
    room_data *to = exit->to_room;
    act_room( to, prog_msg( action, close_msg[2] ), 0, 0, 0, 0, back );
    set_bit( back->exit_info, EX_CLOSED );
  }

  set_delay( ch, 10 );
  return false;
}


void do_close( char_data* ch, const char *argument )
{  
  visible_data *vis;
  if( !( vis = one_visible( ch, argument, "close",
			    (visible_array *) &ch->in_room->exits, EXIT_DATA,
			    (visible_array *) ch->array, OBJ_DATA,
			    (visible_array *) &ch->contents, OBJ_DATA,
			    (visible_array *) &ch->wearing, OBJ_DATA ) ) )
    return;

  if( obj_data *obj = object( vis ) ) {
    if( !is_confused_pet( ch ) ) {
      include_closed = false;
      close_object( ch, obj );
      include_closed = true;
    }
    return;
  }

  if( exit_data *door = exit( vis ) ) {
    if( ch->pcdata
	|| !ch->leader
	|| !is_set( ch->status, STAT_ORDERED )
	|| is_set( ch->species->act_flags, ACT_OPEN_DOORS ) ) {
      close_door( ch, door );   
    }
    return;
  }
  
  //  send( ch, "%s isn't something you can close.\n\r", thing );
}


/*
 *   KEYS
 */


obj_data *has_key( char_data* ch, int vnum )
{
  if( vnum < 0 )
    return 0;
  
  obj_data*   obj;
  obj_data*  obj2;

  for( int i = 0; i < ch->contents; i++ ) {
    if( !( obj = object( ch->contents[i] ) ) )
      continue;
    if( obj->pIndexData->vnum == vnum ) {
      obj->Select( 1 );
      return obj;
    }
    if( obj->pIndexData->item_type == ITEM_KEYRING )
      for( int j = 0; j < obj->contents; j++ ) {
        if( ( obj2 = object( obj->contents[j] ) )
	    && obj2->pIndexData->vnum == vnum ) {
          obj2->Select( 1 );
          return obj2;
	}
      }
  }

  for( int i = 0; i < ch->wearing; i++ ) {
    if( !( obj = object( ch->wearing[i] ) ) )
      continue;
    if( obj->pIndexData->vnum == vnum ) {
      obj->Select( 1 );
      return obj;
    }
    if( obj->pIndexData->item_type == ITEM_KEYRING )
      for( int j = 0; j < obj->contents; j++ ) {
        if( ( obj2 = object( obj->contents[j] ) )
	    && obj2->pIndexData->vnum == vnum ) {
          obj2->Select( 1 );
          return obj2;
	}
      }
  }

  return 0;
}


/*
 *   LOCK ROUTINES
 */


const default_data lock_msg [] =
{
  { "to_char",  "You lock $p with $P.", -1 },
  { "to_room",  "$n locks $p with $P.", -1 },
  { "", "", -1 }
};


static void lock_object( char_data* ch, obj_data* obj )
{
  if( obj->pIndexData->item_type != ITEM_CONTAINER ) {
    fsend( ch, "%s cannot be locked.", obj );
    return;
  }
  
  if( !is_set( obj->value[1], CONT_CLOSED ) ) {
    fsend( ch, "%s is not closed.", obj );
    return;
  }
  
  // Why tell 'em this?
  //    if( obj->pIndexData->value[2] < 0 ) {
  //      send( ch, "It can't be locked.\n\r" );
  //      return;
  //      }
  
  if( is_set( obj->value[1], CONT_LOCKED ) ) {
    fsend( ch, "%s is already locked.", obj );
    return;
  }
  
  obj_data*         key;
  oprog_data*     oprog;

  if( !( key = has_key( ch, obj->pIndexData->value[2] ) ) ) {
    fsend( ch, "You lack the key needed to lock %s.", obj );
    return;
  }
  
  for( oprog = obj->pIndexData->oprog; oprog; oprog = oprog->next )
    if( oprog->trigger == OPROG_TRIGGER_LOCK ) {
      push( );
      clear_variables( );
      var_ch = ch;
      var_room = ch->in_room;
      var_obj = key;
      var_container = obj;
      var_def = lock_msg;
      var_def_type = -1;
      const int result = oprog->execute( obj );
      pop( );
      if( !result
	  || !obj->Is_Valid( ) )
	return;
      break;
    }
  
  act( ch, prog_msg( oprog, lock_msg[0] ), 0, 0, obj, key );
  act_notchar( prog_msg( oprog, lock_msg[1] ), ch, 0, obj, key );

  obj = (obj_data *) obj->From( 1, true );
  set_bit( obj->value[1], CONT_LOCKED );
  obj->To( );
}


void do_lock( char_data* ch, const char *argument )
{
  visible_data *vis;
  if( !( vis = one_visible( ch, argument, "lock",
			    (visible_array *) &ch->in_room->exits, EXIT_DATA,
			    (visible_array *) ch->array, OBJ_DATA,
			    (visible_array *) &ch->contents, OBJ_DATA ) ) )
    return;
  
  if( obj_data *obj = object( vis ) ) {
    if( !is_confused_pet( ch ) ) {
      lock_object( ch, obj );
    }
    return;
  }

  if( exit_data *door = exit( vis ) ) {
    if( ch->pcdata
	|| !ch->leader
	|| !is_set( ch->status, STAT_ORDERED )
	|| is_set( ch->species->act_flags, ACT_OPEN_DOORS ) ) {
      lock_door( ch, door );   
    }
    return;
  }

  //  send( ch, "%s isn't something you can lock.\n\r", thing );
}

 
const default_data lock_door_msg [] =
{
  { "to_char",  "You lock $d with $p.", -1 },
  { "to_room",  "$n locks $d with $p.", -1 },
  { "to_side",  "You hear a key turn in $d.", -1 },
  { "", "", -1 }
};


bool lock_door( char_data* ch, exit_data* exit )
{  
  if( !is_set( exit->exit_info, EX_CLOSED ) ) {
    fsend( ch, "%s %s not closed.",
	   exit, exit_verb( ch, exit ) );
    return false;
  }
  
  // Why tell 'em this?
  //  if( exit->key < 0 ) {
  //    send( ch, "%s can't be locked.\n\r", exit );
  //    return false;
  //    }

  if( is_set( exit->exit_info, EX_LOCKED ) ) {
    fsend( ch, "%s %s already locked.",
	   exit, exit_verb( ch, exit ) );
    return false;
  }
  
  obj_data *key;
  if( !( key = has_key( ch, exit->key ) ) ) {
    fsend( ch, "You lack the key needed to lock %s.", exit );
    return false;
  }
  
  room_data *room = ch->in_room;

  action_data *action;

  for( action = room->action; action; action = action->next )
    if( action->trigger == TRIGGER_LOCK_DOOR
	&& is_set( action->flags, (int)exit->direction ) ) {
      push( );
      clear_variables( );
      var_ch = ch;
      var_obj = key;
      var_room = room;
      var_exit = exit;
      var_def = lock_door_msg;
      var_def_type = -1;
      const int result = action->execute( );
      pop( );
      if( !result
	  || ch->in_room != room )
        return false;
      break;
    }
  
  act( ch, prog_msg( action, lock_door_msg[0] ), 0, 0, key, 0, exit );
  act_notchar( prog_msg( action, lock_door_msg[1] ), ch, 0, key, 0, exit );
  set_bit( exit->exit_info, EX_LOCKED );
  
  if( exit_data *back = reverse( exit ) ) {
    room_data *to = exit->to_room;
    act_room( to, prog_msg( action, lock_door_msg[2] ), 0, 0, 0, 0, back );
    set_bit( back->exit_info, EX_LOCKED );
  }

  return true;
}


/*
 *   UNLOCK ROUTINES
 */


const default_data unlock_msg [] =
{
  { "to_char",  "You unlock $p with $P.", -1 },
  { "to_room",  "$n unlocks $p with $P.", -1 },
  { "", "", -1 }
};


static void unlock_object( char_data *ch, obj_data *obj )
{
  if( obj->pIndexData->item_type != ITEM_CONTAINER ) {
    fsend( ch, "%s can't be unlocked.", obj );
    return;
  }
  
  if( !is_set( obj->value[1], CONT_CLOSED ) ) {
    fsend( ch, "%s isn't closed.", obj );
    return;
  }
  
  // Why tell 'em this?
  //    if( obj->pIndexData->value[2] < 0 ) {
  //      send( ch, "%s can't be unlocked.\n\r", obj );
  //      return;
  //    }
  
  if( !is_set( obj->value[1], CONT_LOCKED ) ) {
    fsend( ch, "%s is already unlocked.", obj );
    return;
  }
  
  obj_data*         key;
  oprog_data*     oprog;

  if( !( key = has_key( ch, obj->pIndexData->value[2] ) ) ) {
    fsend( ch, "You lack the key needed to unlock %s.", obj );
    return;
  }
  
  for( oprog = obj->pIndexData->oprog; oprog; oprog = oprog->next )
    if( oprog->trigger == OPROG_TRIGGER_UNLOCK ) {
      push( );
      clear_variables( );
      var_ch = ch;
      var_room = ch->in_room;
      var_obj = key;
      var_container = obj;
      var_def = unlock_msg;
      var_def_type = -1;
      const int result = oprog->execute( obj );
      pop( );
      if( !result
	  || !obj->Is_Valid( ) )
	return;
      break;
    }

  act( ch, prog_msg( oprog, unlock_msg[0] ), 0, 0, obj, key );
  act_notchar( prog_msg( oprog, unlock_msg[1] ), ch, 0, obj, key );
  
  obj = (obj_data *) obj->From( 1, true );
  remove_bit( obj->value[1], CONT_LOCKED );
  obj->To( );
}


void do_unlock( char_data* ch, const char *argument )
{
  visible_data *vis;
  if( !( vis = one_visible( ch, argument, "unlock",
			    (visible_array *) &ch->in_room->exits, EXIT_DATA,
			    (visible_array *) ch->array, OBJ_DATA,
			    (visible_array *) &ch->contents, OBJ_DATA ) ) )
    return;
  

  if( obj_data *obj = object( vis ) ) {
    if( !is_confused_pet( ch ) ) {
      unlock_object( ch, obj );
    }
    return;
  }

  if( exit_data *door = exit( vis ) ) {
    if( ch->pcdata
	|| !ch->leader
	|| !is_set( ch->status, STAT_ORDERED )
	|| is_set( ch->species->act_flags, ACT_OPEN_DOORS ) ) {
      unlock_door( ch, door );
    }
    return;
  }

  //  send( ch, "%s isn't something you can unlock.\n\r", thing );
}
  

const default_data unlock_door_msg [] =
{
  { "to_char",  "You unlock $d with $p.", -1 },
  { "to_room",  "$n unlocks $d with $p.", -1 },
  { "to_side",  "You hear a key turn in $d.", -1 },
  { "", "", -1 }
};


bool unlock_door( char_data* ch, exit_data* exit )
{ 
  if( !is_set( exit->exit_info, EX_CLOSED ) ) {
    fsend( ch, "%s %s not closed.",
	   exit, exit_verb( ch, exit ) );
    return false;
  }

  // Why tell 'em this?
  //  if( exit->key < 0 ) {
  //    send( ch, "You see no obvious way to unlock %s.\n\r", exit );
  //    return false;
  //  }

  if( !is_set( exit->exit_info, EX_LOCKED ) ) {
    fsend( ch, "%s %s already unlocked.",
	   exit, exit_verb( ch, exit ) );
    return false;
  }

  obj_data *key;

  if( !( key = has_key( ch, exit->key ) ) ) {
    fsend( ch, "You lack the key needed to unlock %s.", exit );
    return false;
  }
  
  room_data *room = ch->in_room;

  action_data *action;

  for( action = room->action; action; action = action->next )
    if( action->trigger == TRIGGER_UNLOCK_DOOR
	&& is_set( action->flags, (int)exit->direction ) ) {
      push( );
      clear_variables( );
      var_ch = ch;
      var_obj = key;
      var_room = room;
      var_def = unlock_door_msg;
      var_def_type = -1;
      const int result = action->execute( );
      pop( );
      if( !result
	  || ch->in_room != room )
        return true;
      break;
    }

  act( ch, prog_msg( action, unlock_door_msg[0] ), 0, 0, key, 0, exit );
  act_notchar( prog_msg( action, unlock_door_msg[1] ), ch, 0, key, 0, exit );
  remove_bit( exit->exit_info, EX_LOCKED );

  if( exit_data *back = reverse( exit ) ) {
    room_data *to = exit->to_room;
    act_room( to, prog_msg( action, unlock_door_msg[2] ), 0, 0, 0, 0, back );
    remove_bit( back->exit_info, EX_LOCKED );
  }
  
  return true;
}


static void pick_object( char_data *ch, obj_data *obj )
{
  if( obj->pIndexData->item_type != ITEM_CONTAINER ) {
    fsend( ch, "%s isn't a container.", obj );
    return;
  }
  
  if( !is_set( obj->value[1], CONT_CLOSED ) ) { 
    fsend( ch, "%s is not closed.", obj );
    return;
  }
  
  // See if the player has a pick.
  obj_data *pick = find_type( ch, ch->contents, ITEM_LOCK_PICK );
  if( !pick ) {
    fsend ( ch, "You don't seem to have anything with which to pick the lock on %s.",
	    obj );
    return;
  }
  
  if( !is_set( obj->value[1], CONT_LOCKED ) ) {
    fsend( ch, "%s is already unlocked.", obj );
    return;
  }
  
  if( has_key( ch, obj->pIndexData->value[2] ) ) {
    fsend( ch, "You decide not to pick the lock on %s since you have the key.",
	   obj );
    return;
  }
  
  if( ch->species
      || !ch->check_skill( SKILL_PICK_LOCK )
      || is_set( obj->value[1], CONT_PICKPROOF )
      ) {
    
    // Sometimes a lockpick breaks.
    // Chance is 20% at skill level 1, 2% at level 10.
    if ( number_range( 1, 100 ) <= 22 - 2*ch->get_skill( SKILL_PICK_LOCK ) ) {
      fsend( ch, "You break your %s attempting to pick the lock on %s.",
	     pick->Seen_Name( ch, 1, true ), obj );
      fsend_seen( ch, "%s breaks %s attempting to pick the lock on %s.",
		  ch, pick, obj );
      pick->Extract(1);
      
    } else {
      fsend( ch, "You fail to pick the lock on %s.", obj );
      fsend_seen( ch, "%s fails to pick the lock on %s.", ch, obj );
    }
    
    if ( is_set( obj->value[1], CONT_PICKPROOF ) ) {
      send(ch, "You are fairly sure this lock is impossible to pick.\n\r");
    }
    
    return;
  }      
  
  fsend( ch, "You pick the lock on %s using %s.", obj, pick );
  fsend_seen( ch, "%s picks the lock on %s using %s.", ch, obj, pick );
  
  obj = (obj_data *) obj->From( 1, true );
  remove_bit( obj->value[1], CONT_LOCKED );
  obj->To( );
  
  ch->improve_skill( SKILL_PICK_LOCK );
  set_delay( ch, 10 );
}


void do_pick( char_data* ch, const char *argument )
{
  visible_data *vis;
  if( !( vis = one_visible( ch, argument, "pick",
			    (visible_array *) &ch->in_room->exits, EXIT_DATA,
			    (visible_array *) ch->array, OBJ_DATA,
			    (visible_array *) &ch->contents, OBJ_DATA ) ) )
    return;

  obj_data *obj;
  if( ( obj = object( vis ) ) ) {
    if( !is_confused_pet( ch ) ) {
      pick_object( ch, obj );
    }
    return;
  }
  
  if( exit_data *door = exit( vis ) ) {
    if( ch->pcdata
	|| !ch->leader
	|| !is_set( ch->status, STAT_ORDERED )
	|| ch->get_skill( SKILL_PICK_LOCK ) != UNLEARNT ) {
      pick_door( ch, door );
    }
    return;
  }
}


bool pick_door( char_data* ch, exit_data* exit )
{
  if( !is_set( exit->exit_info, EX_CLOSED ) ) {
    fsend( ch, "%s %s not closed.",
	   exit, exit_verb( ch, exit ) );
    return false;
  }

  if( exit->key < 0 ) {
    fsend( ch, "You see no keyhole in %s to pick.", exit );
    return false;
  }

  // See if the player has a pick.
  obj_data *pick = find_type( ch, ch->contents, ITEM_LOCK_PICK );
  if( !pick ) {
    fsend( ch, "You don't seem to have anything with which to pick the lock on %s.",
	   exit );
    return false;
  }
  
  if( !is_set( exit->exit_info, EX_LOCKED ) ) {
    fsend( ch, "%s %s already unlocked.",
	   exit, exit_verb( ch, exit ) );
    return false;
  }

  if( has_key( ch, exit->key ) ) {
    fsend( ch, "You decide not to pick the lock on %s since you have the key.",
	   exit );
    return false;
  }
  
  if( ch->species
      || !ch->check_skill( SKILL_PICK_LOCK, pick->value[0] )
      || is_set( exit->exit_info, EX_PICKPROOF )
      ) {

    // Sometimes a lockpick breaks.
    // Chance is 20% at skill level 1, 2% at level 10.
    if( number_range(1, 100) <= 22 - 2*ch->get_skill( SKILL_PICK_LOCK ) ) {
      fsend( ch, "You break your %s attempting to pick the lock on %s.",
	     pick->Seen_Name(ch, 1, true), exit );
      fsend_seen( ch, "%s breaks %s attempting to pick the lock on %s.",
		  ch, pick, exit);
      pick->Extract(1);
    } else {
      fsend( ch, "You fail to pick the lock on %s.", exit );
      fsend_seen( ch, "%s fails to pick the lock on %s.", ch, exit );
    }

    if ( is_set( exit->exit_info, EX_PICKPROOF ) ) {
      send( ch, "You are fairly sure this lock is impossible to pick.\n\r" );
    }

    return false;
  }      
  
  fsend( ch, "You pick the lock on %s using %s.", exit, pick );
  fsend_seen( ch, "%s picks the lock on %s using %s.", ch, exit, pick );

  remove_bit( exit->exit_info, EX_LOCKED );
  ch->improve_skill( SKILL_PICK_LOCK );

  if( ( exit = reverse( exit ) ) ) 
    remove_bit( exit->exit_info, EX_LOCKED );

  set_delay( ch, 10 );
  return true;
}


/*
 *   KNOCK
 */


void do_knock( char_data* ch, const char *argument )
{
  if( is_confused_pet( ch ) )
    return;

  visible_data *vis;
  if( !( vis = one_visible( ch, argument, "knock",
			    (visible_array*) &ch->in_room->exits, EXIT_DATA ) ) )
    return;
  
  if( exit_data *door = exit( vis ) ) {
    knock_door( ch, door );
    return;
  }

  //  send( ch, "Knocking on %s would serve no purpose.\n\r", thing );
}


const default_data knock_door_msg [] =
{
  { "to_char",  "You knock on $d.", -1 },
  { "to_room",  "$n knocks on $d.", -1 },
  { "to_side",  "You hear a knock on $d.", -1 },
  { "", "", -1 }
};


void knock_door( char_data* ch, exit_data* exit )
{
  if( !is_set( exit->exit_info, EX_ISDOOR ) ) {
    send( ch, "There is no door %s to knock on.\n\r",
      dir_table[ exit->direction ].where );
    return;
  }
  
  if( !is_set( exit->exit_info, EX_CLOSED ) ) {
    fsend( ch, "%s %s not closed, so there is no reason to knock.",
	   exit, exit_verb( ch, exit ) );
    return;
  }

  room_data *room = ch->in_room;

  action_data *action;

  for( action = room->action; action; action = action->next )
    if( action->trigger == TRIGGER_KNOCK_DOOR
	&& is_set( action->flags, (int)exit->direction  ) ) {
      push( );
      clear_variables( );
      var_ch = ch;
      var_room = room;
      var_exit = exit;
      var_def = knock_door_msg;
      var_def_type = -1;
      const bool result = action->execute( );
      pop( );
      if( !result
	  || ch->in_room != room )
        return;
      break;
    }
  
  act( ch, prog_msg( action, knock_door_msg[0] ), 0, 0, 0, 0, exit );
  act_notchar( prog_msg( action, knock_door_msg[1] ), ch, 0, 0, 0, exit );

  if( exit_data *back = reverse( exit ) ) {
    room_data *to = exit->to_room;
    for( int i = 0; i < to->contents; ++i ) {
      if( char_data *rch = character( to->contents[i] ) ) {
	if( rch->Can_Hear( ) ) {
	  act( rch, prog_msg( action, knock_door_msg[2] ), 0, 0, 0, 0, back );
	}
      }
    }
  }

  set_delay( ch, 10 );
}


/*
 *   DOOR BASH ROUTINE
 */


void bash_door( char_data* ch, exit_data* exit )
{
  if( is_mounted( ch, "bash doors" )
      || is_entangled( ch, "bash doors" )
      || is_drowning( ch, "bash doors" ) )
    return;
  
  if( !is_set( exit->exit_info, EX_ISDOOR ) ) {
    send( ch, "There is no door %s to bash.\n\r",
	  dir_table[ exit->direction ].where );
    return;
  }
  
  if( !is_set( exit->exit_info, EX_CLOSED ) ) {
    fsend( ch, "%s %s not closed and thus there is no point in bashing.",
	   exit->name, exit_verb( ch, exit ) );
    return;
  }
  
  if( exit->direction == DIR_UP || exit->direction  == DIR_DOWN ) {
    send( ch, "You can't effectively bash doors in the ceiling or ground.\n\r" );
    return;
  }

  fsend( ch,
	 "You take a running charge and throw yourself at %s, but do no damage to it.",
	 exit->name );
  fsend_seen( ch,
	      "%s takes a running charges and throws %sself at %s, but does no damage to it.",
	      ch, ch->Him_Her( ), exit->name );

  set_delay( ch, 10 );
}


/*
 *   ONLINE EDITING ROUTINES
 */


static room_data *create_room( char_data* ch )
{
  area_data *area = ch->in_room->area;

  room_data*  room;
  int         vnum  = 1;

  for( vnum = area->room_first->vnum+1; ; vnum++ ) {
    if( !( room = get_room_index( vnum, false ) ) )
      break;
    if( vnum > MAX_ROOM
	|| room->area != area ) {
      send( ch, "Area is out of room numbers.\n\r" );
      return 0;
    }
  }

  if( !area->loaded ) {
    area->load_text( );
  }

  if( !area->act_loaded ) {
    area->load_actions( );
  }

  //  area->modified = true;
  //  area->seen = true;
  //  area->dirty = true;
  area->act_dirty = true;

  room = new room_data( area, vnum );
  room->Set_Description( ch, "Under Construction.\n\r" );
  room->name = alloc_string( ch->in_room->name, MEM_ROOM );

  vcopy( room->room_flags, ch->in_room->room_flags, 2 );

  room->sector_type  = ch->in_room->sector_type;
  room->size         = ch->in_room->size;

  remove_bit( room->room_flags, RFLAG_BANK );
  remove_bit( room->room_flags, RFLAG_SHOP );
  remove_bit( room->room_flags, RFLAG_OFFICE );
  remove_bit( room->room_flags, RFLAG_PET_SHOP );
  remove_bit( room->room_flags, RFLAG_AUCTION_HOUSE );
  remove_bit( room->room_flags, RFLAG_ALTAR );
  remove_bit( room->room_flags, RFLAG_ALTAR_GOOD );
  remove_bit( room->room_flags, RFLAG_ALTAR_NEUTRAL );
  remove_bit( room->room_flags, RFLAG_ALTAR_EVIL );
  remove_bit( room->room_flags, RFLAG_ALTAR_LAW );
  remove_bit( room->room_flags, RFLAG_ALTAR_NEUTRAL2 );
  remove_bit( room->room_flags, RFLAG_ALTAR_CHAOS );
  remove_bit( room->room_flags, RFLAG_SAVE_ITEMS );

  room_data *prev = get_room_index( vnum-1, false );
  room->next = prev->next;
  prev->next = room;

  //  append( ch->in_room->area->room_first, room );

  char buf [ TWO_LINES ];
  snprintf( buf, TWO_LINES, "Room.%d", room->vnum );
  delete_file( ROOM_DIR, buf, false );
  delete_file( ROOM_PREV_DIR, buf, false );

  return room;
}


static void create_exit( wizard_data* ch, int dir, const char *argument, bool one_way )
{
  room_data *room = ch->in_room;

  if( !ch->can_edit( room ) ) 
    return;

  if( exit_direction( room, dir ) ) {
    fsend( ch, "An exit already exists %s.",
	   dir_table[ dir ].where );
    return;
  }
  
  if( !*argument ) {
    if( !( room = create_room( ch ) ) )
      return;
  } else {
    if( !( room = get_room_index( atoi( argument ), false ) ) ) {
      send( ch, "No room with that vnum exists.\n\r" );
      return;
    }
    if( !one_way
	&& exit_direction( room, dir_table[dir].reverse ) ) {
      send( ch, "Door returning already exists.\n\r" );  
      return;
    }
    if( !ch->can_edit( room, false ) ) {
      send( ch, "You don't have permission to tunnel into that area.\n\r" );
      return;
    }
  }
  
  int size = min( ch->in_room->size, room->size );

  exit_data *exit = add_exit( ch->in_room, dir );
  exit->to_room = room;
  exit->size = size;

  ch->exit_edit = exit;
  ch->in_room->area->modified = true;

  if( !one_way ) {
    exit = add_exit( room, dir_table[dir].reverse );
    exit->to_room = ch->in_room;
    exit->size = size;
    room->area->modified = true;
  }

  fsend( ch, "%s-way exit %s to room %d added.",
	 one_way ? "One" : "Two", dir_table[dir].name, room->vnum );
}


static void delete_exit( wizard_data* imm, exit_data* exit, bool one_way )
{
  if( !imm->can_edit( imm->in_room ) ) 
    return;

  exit_data *back;
  
  if( ( back = reverse( exit ) ) && !one_way ) {
    if( !imm->can_edit( exit->to_room, false ) ) {
      send( imm,
	    "You don't have permission to delete the return exit.\n\r" );
      return;
    }
    exit->to_room->area->modified = true;
    imm->exit_edit = back;
    extract( imm, offset( &imm->exit_edit, imm ), "exit" );
    exit->to_room->exits -= back;
    dereference( exit->to_room, back );
    delete back;
  }
  
  imm->in_room->area->modified = true;
  imm->exit_edit = exit;
  extract( imm, offset( &imm->exit_edit, imm ), "exit" );
  imm->in_room->exits -= exit;
  dereference( imm->in_room, exit );
  delete exit;
  
  fsend( imm, "You remove %sthe exit %s.",
	 back && one_way ? "just this side of " : "",
	 dir_table[ exit->direction ].where );
}


void do_dedit( char_data* ch, const char *argument )
{
  wizard_data*    imm;
  if( !( imm = wizard( ch ) ) )
    return;
  
  int           flags;

  if( !get_flags( ch, argument, &flags, "1", "dedit" ) )
    return;

  exit_data *exit;

  if( !strncasecmp( argument, "del", 3 )
      && matches( argument, "delete" ) ) {
    if( ( exit = (exit_data*) one_visible( ch, argument, "dedit delete", 
					   (visible_array*) &ch->in_room->exits, EXIT_DATA ) ) )
      delete_exit( imm, exit, is_set( flags, 0 ) );
    return;
  }
  
  if( !strncasecmp( argument, "new", 3 )
      && matches( argument, "new" ) ) {
    int dir;
    if( !*argument ) {
      send( ch, "Syntax: dedit new <direction> [vnum]\n\r" );
    } else if( ( dir = direction_arg( argument ) ) != -1 ) 
      create_exit( imm, dir, argument, is_set( flags, 0 ) );
    return;
  }
  
  if( !*argument
      && imm->exit_edit ) {
    fsend( ch, "You stop editing the exit %s.",
	   dir_table[ imm->exit_edit->direction ].where );
    imm->exit_edit = 0;
    return;
  }

  if( !( exit = (exit_data*) one_visible( ch, argument, "dedit", 
					  (visible_array*) &ch->in_room->exits, EXIT_DATA ) ) )
    return;
  
  imm->exit_edit = exit;
  
  fsend( ch, "Dflag, dstat, and dset now act on the exit %s.",
	dir_table[ exit->direction ].where );
}


void do_dflag( char_data* ch, const char *argument )
{
  wizard_data *imm;
  if( !( imm = wizard( ch ) ) )
    return;

  int flags;
  if( !get_flags( ch, argument, &flags, "1", "dflag" ) )
    return;

  exit_data *exit;
  if( !( exit = imm->exit_edit ) ) {
    send( ch, "Use dedit to specify direction.\n\r" );
    return;
  }
  
  if( !*argument ) {
    display_flags( "Door", &dflag_name[0], &dflag_name[1],
		   &exit->exit_info, MAX_DFLAG, ch );
    return;
  }
  
  if( !ch->can_edit( ch->in_room ) ) 
    return;
  
  for( int i = 0; i < MAX_DFLAG; i++ ) {
    exit_data *back;
    if( fmatches( argument, dflag_name[i] ) ) {
      ch->in_room->area->modified = true;
      switch_bit( exit->exit_info, i );
      if( ( back = reverse( exit ) )
	  && !is_set( flags, 0 ) ) {
	exit->to_room->area->modified = true;
        assign_bit( back->exit_info, i, is_set( exit->exit_info,i ) );
      }
      fsend( ch, "%s set %s on %sthe %s exit.",
	     dflag_name[i], true_false( &exit->exit_info, i ),
	     back && !is_set( flags, 0 ) ? "both sides of " : "",
	     dir_table[ exit->direction ].name );
      return;
    }
  }
  
  send( ch, "Unknown flag - see dflag with no arguments for list.\n\r" );
}


void do_dstat( char_data* ch, const char *argument )
{
  wizard_data *imm;
  if( !( imm = wizard( ch ) ) )
    return;
  
  exit_data *exit;
  if( !*argument ) { 
    if( !( exit = imm->exit_edit ) ) {
      send( ch, "Use dedit to specify direction.\n\r" );
      return;
    }
  } else if( !( exit = (exit_data*) one_visible( ch, argument, "dstat",
						 (visible_array*) &ch->in_room->exits, EXIT_DATA ) ) )
    return;
   
  send( ch, "[%s]\n\r", dir_table[ exit->direction ].name );
  send( ch, "  Leads to : %s (%d)\n\r",
	exit->to_room->name, exit->to_room->vnum );

  obj_clss_data *key;
  if( !( key = get_obj_index( exit->key ) ) )  
    send( ch, "       Key : None\n\r" );
  else  
    send( ch, "       Key : %s (%d)\n\r", key->Name( ), exit->key );

  send( ch, "      Name : %s\n\r",   exit->name );
  send( ch, "  Keywords : %s\n\r",   exit->keywords );
  send( ch, "     Light : %d%%\n\r", (int)exit->light );
  send( ch, "      Size : %s\n\r",   size_name[ exit->size ] );
}


void do_dset( char_data* ch, const char *argument )
{ 
  wizard_data*   imm;
  exit_data*    exit;
  obj_data*      obj;
  int          flags;

  if( !( imm = wizard( ch ) ) )
    return;

  if( !get_flags( ch, argument, &flags, "1", "set" ) )
    return;
  
  if( !*argument ) {
    do_dstat( ch, "" );
    return;
  }
  
  if( !ch->can_edit( ch->in_room ) )
    return;

  if( !( exit = imm->exit_edit ) ) {
    send( ch, "You must specify a direction with dedit.\n\r" );
    return;
  }
  
  exit_data *back = 0;
  if( !is_set( flags, 0 ) ) {
    back = reverse( exit );
  }

  char buf [ THREE_LINES ];
  if( back ) {
    snprintf( buf, THREE_LINES, "both sides of the %s exit", dir_table[ exit->direction ].name );
  } else {
    snprintf( buf, THREE_LINES, "the %s exit", dir_table[ exit->direction ].name );
  }

  if( matches( argument, "key" ) ) {
    if( !*argument ) {
      fsend( ch, "Key for lock on %s set to none.", buf );
      exit->key = -1;
      ch->in_room->area->modified = true;
      if( back ) {
	exit->to_room->area->modified = true;
	back->key = -1;
      }
      return;
    }
    if( !( obj = one_object( ch, argument,
			     "key", &ch->contents ) ) )
      return;
    if( obj->pIndexData->item_type != ITEM_KEY ) {
      fsend( ch, "%s isn't a key.", obj );
      return;
    }  
    fsend( ch, "Key for lock on %s set to %s.\n\r",
	   buf, obj->pIndexData->Name( ) );
    exit->key = obj->pIndexData->vnum;
    ch->in_room->area->modified = true;
    if( back ) {
      back->key = obj->pIndexData->vnum;
      exit->to_room->area->modified = true;
    }
    return;
  }

  {
    int tmp = exit->size;

    class type_field type_list[] = {
      { "size",  MAX_SIZE,  &size_name[0],  &size_name[1],  &tmp, false },
      { "" }
    };
    
    if( const char *response = process( type_list, ch, buf, argument ) ) {
      if( *response ) {
	exit->size = tmp;
	ch->in_room->area->modified = true;
	if( back ) {
	  back->size = exit->size;
	  exit->to_room->area->modified = true;
	}
      }
      return;
    }
  }

  {
    class string_field string_list[] = {
      { "name",      MEM_EXIT,  &exit->name,       0 },
      { "",          0,         0,                 0 },   
    };
    
    if( const char *response = process( string_list, ch, buf, argument ) ) {
      if( *response ) {
	ch->in_room->area->modified = true;
	if( back ) {
	  free_string( back->name, MEM_EXIT );
	  back->name = alloc_string( exit->name, MEM_EXIT );
	  exit->to_room->area->modified = true;
	}
      }
      return;
    }
  }

  {
    class string_field string_list[] = {
      { "keywords",  MEM_EXIT,  &exit->keywords,   0 },
      { "",          0,         0,                 0 },   
    };
    
    if( const char *response = process( string_list, ch, buf, argument ) ) {
      if( *response ) {
	ch->in_room->area->modified = true;
	if( back ) {
	  free_string( back->keywords, MEM_EXIT );
	  back->keywords = alloc_string( exit->keywords, MEM_EXIT );
	  exit->to_room->area->modified = true;
	}
      }
      return;
    }
  }

  {
    int tmp = exit->light;

    class int_field int_list[] = {
      { "light",             0,  100,  &tmp     },
      { "",                  0,    0,  0                }
    };

    if( const char *response = process( int_list, ch, buf, argument ) ) {
      if( *response ) {
	exit->light = tmp;
	ch->in_room->area->modified = true;
	if( back ) {
	  back->light = exit->light;
	  exit->to_room->area->modified = true;
	}
      }
      return;
    }
  }

  send( ch, "Unknown field - See help dset.\n\r" );
}


/*
 *   SPELLS
 */


bool spell_wizard_lock( char_data *ch, char_data*, void *vo, int level, int duration )
{
  if( null_caster( ch, SPELL_WIZARD_LOCK ) )
    return false;

  exit_data *door = (exit_data*) vo;

  if( !is_set( door->exit_info, EX_ISDOOR )
      || !is_set( door->exit_info, EX_CLOSED ) ) {
    send( ch, "You see no closed door there.\n\r" );
    return false;
  }

  exit_data *back = reverse( door );
  
  if( is_set( door->exit_info, EX_NO_OPEN )
      || is_set( door->affected_by, AFF_WIZLOCK )
      || !back
      || is_set( back->exit_info, EX_NO_OPEN ) 
      || is_set( back->affected_by, AFF_WIZLOCK ) ) {
    send( ch, "Nothing happens.\n\r" );
    return false;
  }

  if( ( door->size - SIZE_HUMAN )*2 > level
      || ( back->size - SIZE_HUMAN )*2 > level ) {
    fsend( ch, "You are not skilled enough to wizard lock %s.", door );
    return false;
  }

  return spell_affect( ch, door->to_room, back, false, level, duration,
		       SPELL_WIZARD_LOCK, AFF_WIZLOCK );
}


bool spell_ward( char_data *ch, char_data*, void *vo, int level, int duration )
{
  if( null_caster( ch, SPELL_WARD ) )
    return false;

  exit_data *door = (exit_data*) vo;
  exit_data *back = reverse( door );

  if( is_set( door->affected_by, AFF_WARD )
      || !back
      || is_set( back->affected_by, AFF_WARD ) ) {
    send( ch, "Nothing happens.\n\r" );
    return false;
  }

  if( ( door->size - SIZE_HUMAN )*2 > level
      || ( back->size - SIZE_HUMAN )*2 > level ) {
    fsend( ch, "You are not skilled enough to ward %s.", door );
    return false;
  }

  room_data *room = ch->in_room;

  /*
  for( int i = 0; i < room->contents; ++i ) {
    if( char_data *rch = character( room->contents[i] ) ) {
      if( rch->shdata->race == RACE_UNDEAD
	  && !is_set( rch->status, STAT_PET ) ) {
	rch->shown = 1;
	fsend( ch, "You cannot create a ward with %s in the room.", rch );
	return false;
      }
    }
  }
  */

  return spell_affect( ch, room, door, true, level, duration,
		       SPELL_WARD, AFF_WARD );
}
