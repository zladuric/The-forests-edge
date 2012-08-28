#include <ctype.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "define.h"
#include "struct.h"


const char *action_trigger[ MAX_ATN_TRIGGER ] = {
  "none", "entering room",
  "random", "leaving room", "random_always", "sacrifice", "time", "attack",
  "open_door", "searching",
  "close_door", "lock_door", "unlock_door", "knock_door",
  "sun", "moon",
  "cast", "describe", "reset", "path", "init" };


const char* action_flags [ MAX_ATN_TRIGGER ][ 10 ] =
{
  { "loop_follower", "mounted", "resting", "sleeping", "fighting",
    "inventory", "worn", "room", "" },
  { "north", "east", "south", "west", "up", "down", "extra", "any", "transfer", "" },
  { "" },
  { "north", "east", "south", "west", "up", "down", "extra", "any", "transfer", "" },
  { "" },
  { "" },
  { "" },
  { "" },
  { "north", "east", "south", "west", "up", "down", "" },
  { "" },
  { "north", "east", "south", "west", "up", "down", "" },
  { "north", "east", "south", "west", "up", "down", "" },
  { "north", "east", "south", "west", "up", "down", "" },
  { "north", "east", "south", "west", "up", "down", "" },
  { "" },
  { "" },
  { "" },
  { "" },
  { "" },
  { "north", "east", "south", "west", "up", "down", "" },
  { "" }
};


const default_data *action_msgs [ MAX_ATN_TRIGGER ] =
{
  0,
  entering_msg,
  0,
  leaving_msg,
  0,
  0,
  0,
  0,
  open_msg,
  0,
  close_msg,
  lock_door_msg,
  unlock_door_msg,
  knock_door_msg,
  0,
  0,
  0,
  0,
  0,
  0,
  0
};


static action_array timed_actions;
static action_array random_actions;


action_data :: action_data( room_data *room )
  : next(0), trigger(TRIGGER_NONE), value(0), flags(0),
    command(empty_string), target(empty_string),
    room(room)
{
  record_new( sizeof( action_data ), MEM_ACTION );
  append( room->action, this );
}


action_data :: ~action_data( )
{
  record_delete( sizeof( action_data ), MEM_ACTION );
  free_string( command, MEM_ACTION );
  free_string( target, MEM_ACTION );
  invalidate( );
}


void action_data :: compile( )
{
  area_data *area = room->area;

  if( !area->act_loaded ) {
    area->load_actions( );
  }

  program_data::compile( );
}


void action_data :: decompile( )
{
  area_data *area = room->area;

  if( !area->act_loaded ) {
    program_data::Extra_Descr( ).delete_list( );
  }

  program_data::decompile( );
}


int action_data :: execute( thing_data *owner )
{
  area_data *area = room->area;

  area->used = true;

  if( !owner )
    owner = var_room;

  return program_data::execute( owner );
}


void action_data :: validate( )
{
  if( trigger == TRIGGER_TIME ) {
    timed_actions += this;
  } else if( trigger == TRIGGER_RANDOM
	     || trigger == TRIGGER_RANDOM_ALWAYS ) {
    random_actions += this;
  } else if( trigger == TRIGGER_SUN
	     || trigger == TRIGGER_MOON ) {
    weather_actions += this;
  }
}


void action_data :: invalidate( )
{
  timed_actions -= this;
  random_actions -= this;
  weather_actions -= this;
}


void action_data :: Set_Code( const char *text )
{
  area_data *area = room->area;

  if( !area->act_loaded ) {
    area->load_actions( );
  }

  program_data::Set_Code( text );

  area->act_used = true;
  area->act_dirty = true;
  area->modified = true;
}


void action_data :: Edit_Code( char_data *ch, const char *text )
{
  area_data *area = room->area;

  if( !area->act_loaded ) {
    area->load_actions( );
  }

  program_data::Edit_Code( ch, text );

  area->act_used = true;

  if( *text ) {
    area->act_dirty = true;
    area->modified = true;
  }
}


const char *action_data :: Code( ) const
{
  area_data *area = room->area;

  if( !area->act_loaded ) {
    area->load_actions( );
  }

  area->act_used = true;

  return program_data::Code( );
}


extra_array& action_data :: Extra_Descr( )
{
  area_data *area = room->area;

  if( !binary ) {
    if( !area->act_loaded ) {
      area->load_actions( );
    }
    area->act_used = true;
  }

  return program_data :: Extra_Descr( ); 
}


/*
 *   DISPLAY ROUTINE
 */


void action_data :: display( char_data* ch ) const
{
  if( !room ) {
    page( ch, "%-10s %-10s %s\n\r", "??", "Acode", "Null Room??" );
    return;
  }

  int i = 1;

  for( action_data *action = room->action; action != this; action = action->next, i++ );

  page( ch, "Room %-10d Acode %-8d %s\n\r", room->vnum, i, room->name );
}


const char *Keywords( const action_data *action )
{
  if( *action_flags[ action->trigger ][0] ) {
    const char *list [ 10 ];
    int j = 0;
    
    for( int i = 0; *action_flags[action->trigger][i]; i++ ) {
      if( is_set( action->flags, i ) ) {
	list[j++] = action_flags[action->trigger][i];
      }
    }
    
    return word_list( list, j, false );
  }

  switch( action->trigger ) {
  case TRIGGER_SACRIFICE:
    {
      obj_clss_data *clss = get_obj_index( action->value );
      if( !clss ) {
	return "[BUG - invalid object vnum]";
      }
      return clss->Name();
    }
    break;
  case TRIGGER_TIME:
    char *tmp = static_string( );
    if( action->value >= 0 ) {
      if( action->value >= 24*60 ) {
	return "[BUG - invalid time]";
      }
      snprintf( tmp, THREE_LINES, "at %02d:%02d",
		action->value/60, action->value%60 );
    } else {
      snprintf( tmp, THREE_LINES, "every %d MUD-minutes (%d seconds real time)",
		-action->value,
		-action->value*6 );
    }
    return tmp;
    break;
  }

  return empty_string;
}


/*
 *   DISK ROUTINES
 */


void Room_Data :: read_action( FILE *fp )
{
  action_data *act = new action_data( this );

  act->command = fread_string( fp, MEM_ACTION );
  act->target  = fread_string( fp, MEM_ACTION );

  act->read( fp );

  act->trigger = fread_number( fp );
  act->value   = fread_number( fp );
  act->flags   = fread_number( fp );

  act->validate( );
}


void Room_Data :: write_actions( FILE *fp ) const
{
  bool temp_load = !area->act_loaded;

  if( temp_load ) {
    area->load_actions( );
  }

  for( action_data *act = action; act; act = act->next ) {
    fprintf( fp, "A\n" );
    fwrite_string( fp, act->command );
    fwrite_string( fp, act->target );
    act->write( fp );
    //    fwrite_string( fp, act->program_data::Code( ) );    
    //    write_extras( fp, act->program_data::Extra_Descr( ) );
    fprintf( fp, "!\n" );
    
    fprintf( fp, "%d %d %d\n", act->trigger, act->value, act->flags );
  }

  if( temp_load ) {
    area->clear_actions( );
  }
}



/*
 *   EXECUTION ROUTINES
 */


static bool check_action_flags( char_data *ch, int flags, bool msg = true )
{
  if( ch->position < POS_SLEEPING
      || ( !is_set( flags, AFLAG_SLEEPING )
	   && ch->position == POS_SLEEPING )
      || ( !is_set( flags, AFLAG_RESTING )
	   && ( ch->position == POS_RESTING
		|| ch->position == POS_MEDITATING ) ) ) {
    if( msg )
      pos_message( ch );
    return false;
  }
  
  const char *const message = msg ? empty_string : 0;

  if( !is_set( flags, AFLAG_MOUNTED )
      && ( is_mounted( ch, message )
	   || is_ridden( ch, message ) ) ) {
    return false;
  }

  if( !is_set( flags, AFLAG_FIGHTING )
      && is_fighting( ch, message ) ) {
    return false;
  }

  return true;
}


/*
static void action_followers( char_data *ch, const char *argument, char_array& followers, action_data *action, int flags )
{
  char_array list = ch->followers;

  push( );

  for( int i = 0; i < list; ++i ) {
    char_data *follower = list[i];
    if( !follower->Is_Valid( )
	|| !follower->array
	|| follower->array != ch->array
	|| !check_action_flags( follower, flags, false ) )
      continue;
    disrupt_spell( follower );
    clear_variables( );
    var_ch   = follower;
    var_room = Room( follower->array->where );
    var_cmd = ?;
    var_arg = argument;
    action->execute( );
  }

  pop( );
}
*/


class target_data
{
public:
  const char *name;
  void **pointer;
  arg_enum arg_type;
  int data_type;
};


static thing_array *var_list_p = &var_list;


static const target_data target_list [] = {
  { "thing",	(void**)&var_thing,	THING,		THING_DATA	},
  { "obj",	(void**)&var_obj,	OBJECT,		OBJ_DATA	},
  { "rch",	(void**)&var_rch,	CHARACTER,	CHAR_DATA	},
  { "mob",	(void**)&var_mob,	CHARACTER,	CHAR_DATA	},
  { "victim",	(void**)&var_victim,	CHARACTER,	CHAR_DATA	},
  { "exit",	(void**)&var_exit,	EXIT,		EXIT_DATA	},
  { "list",	(void**)&var_list_p,	THING_LIST,	-1		},
  { "",		0,			NONE, 		-1		}
};


bool action_target( const action_data *action, int type, int n )
{
  if( !*action->command )
    return false;

  const char *target = action->target;

  if( *target != '*' )
    return false;

  ++target;

  int i = 0;

  while( true ) {
    if( !*target_list[i].name ) {
      return false;
    }
    if( word_match( target, target_list[i].name ) ) {
      break;
    }
    ++i;
  }

  if( target_list[i].data_type != type )
    return false;

  skip_spaces( target );
  
  while( *target ) {
    
    int vnum;
    if( !number_arg( target, vnum ) ) {
      return false;
    }
    if( vnum == n ) {
      return true;
    }
  }

  return false;
}


bool mob_target( const action_data *action, const species_data *species )
{
  if( !*action->command )
    return false;

  const char *target = action->target;

  if( *target != '*' )
    return false;

  ++target;

  int i = 0;

  while( true ) {
    if( !*target_list[i].name ) {
      return false;
    }
    if( word_match( target, target_list[i].name ) ) {
      break;
    }
    ++i;
  }

  if( target_list[i].arg_type != CHARACTER )
    return false;

  skip_spaces( target );
  
  while( *target ) {
    
    int vnum;
    if( !number_arg( target, vnum ) ) {
      return false;
    }
    if( vnum == species->vnum ) {
      return true;
    }
  }

  return false;
}


static bool match_target( char_data *ch, action_data *action,
			  const char *command, const char *argument, const char *target )
{
  // Set variables early, for code_bug() messages.
  clear_variables( );
  var_ch = ch;
  var_room = action->room;
  var_cmd = command;
  var_arg = argument;

  int i = 0;

  while( true ) {
    if( !*target_list[i].name ) {
      code_bug( "Action: bad target \"*%s\".", target );
      return false;
    }
    if( word_match( target, target_list[i].name ) ) {
      break;
    }
    ++i;
  }

  visible_array *where[3] = { 0, 0, 0 };

  if( target_list[i].arg_type == CHARACTER ) {
    where[0] = (visible_array*) &action->room->contents;
  } else if( target_list[i].arg_type == EXIT ) {
    where[0] = (visible_array*) &action->room->exits;
  } else {
    int j = 0;
    if( is_set( action->flags, AFLAG_INVENTORY ) ) {
      where[j++] = (visible_array*) &ch->contents;
    }
    if( is_set( action->flags, AFLAG_WORN ) ) {
      where[j++] = (visible_array*) &ch->wearing;
    }
    if( is_set( action->flags, AFLAG_ROOM ) ) {
      where[j++] = (visible_array*) &action->room->contents;
    }
    if( j == 0 ) {
      code_bug( "Action: no location aflags set.", target );
      return false;
    }
  }

  if( target_list[i].arg_type == THING_LIST ) {
    skip_spaces( target );

    if( *target ) {
      code_bug( "Action: cannot select \"%s\" with variable %s.", target, target_list[i].name );
      return false;
    }

    thing_array *array = several_things( ch, argument, empty_string,
					 (thing_array*) where[0],
					 (thing_array*) where[1],
					 (thing_array*) where[2] );

    if( !array ) {
      return false;
    }

    *(thing_array*)*target_list[i].pointer = *array;
    
    delete array;

  } else {
    visible_data *vis = one_visible( ch, argument, empty_string,
				     where[0], target_list[i].data_type,
				     where[1], target_list[i].data_type,
				     where[2], target_list[i].data_type );
    
    if( !vis ) {
      return false;
    }
    
    // Set the target var_*.
    //    (visible_data*)*target_list[i].pointer = vis;
    *(visible_data**)target_list[i].pointer = vis;
    
    skip_spaces( target );
    
    while( *target ) {
      
      if( target_list[i].arg_type == OBJECT ) {
	obj_data *obj = (obj_data*) vis;
	int vnum;
	if( !number_arg( target, vnum ) ) {
	  code_bug( "Action: bad object vnum \"%s\".", target );
	  return false;
	}
	if( vnum == obj->pIndexData->vnum )
	  break;
	
      } else if( target_list[i].arg_type == CHARACTER ) {
	char_data *who = (char_data*) vis;
	mob_data *npc = mob( who );
	if( !npc ) {
	  return false;
	}
	int vnum;
	if( !number_arg( target, vnum ) ) {
	  code_bug( "Action: bad mob vnum \"%s\".", target );
	  return false;
	}
	if( vnum == npc->species->vnum )
	  break;
	
      } else if( target_list[i].arg_type == EXIT ) {
	exit_data *exit = (exit_data*) vis;
	
	int j = 0;
	while( true ) {
	  if( j == MAX_DIR_COMPASS ) {
	    code_bug( "Action: bad exit direction \"%s\".", target );
	    return false;
	  }
	  if( exact_match( target, dir_table[j].name ) ) {
	    break;
	  }
	  ++j;
	}
	if( exit->direction == j )
	  break;
	
      } else {
	code_bug( "Action: cannot select \"%s\" with variable %s.", target, target_list[i].name );
	return false;
      }
      
      if( !*target ) {
	return false;
      }
    }
  }

  disrupt_spell( ch );
  
  const int result = action->execute( );

  return !result;
}


bool check_actions( char_data* ch, const char *command, const char *argument, int c )
{
  static char buf [ MAX_INPUT_LENGTH ];
  room_data *room;

  if( !ch->array
      || !( room = Room( ch->array->where ) ) )
    return false;
  
  for( action_data *action = room->action; action; action = action->next ) {
    if( const char *cmd = member( command, action->command, c ) ) {
      if( *action->target == '*' ) {
	one_word( cmd, buf );
	return match_target( ch, action, buf, argument, action->target + 1 );

      } else if ( !*action->target
		  || member( argument, action->target ) ) {
	if( !check_action_flags( ch, action->flags ) )
	  return true;
	
	/*
	  if( is_set( action->flags, AFLAG_LOOP_FOLLOWER ) ) 
	  list = follower_list( ch );
	*/
	
	disrupt_spell( ch );
	
	// cmd may have multiple words.
	one_word( cmd, buf );

	clear_variables( );
	var_ch   = ch;
	var_room = room;
	var_cmd  = buf;
	var_arg  = argument;

	const int result = action->execute( );

	return !result;

	//	if( !action->execute( ) ) 
	//	  return true; 
	
	/*
	  char_data*       rch;
	  int                i;
	  if( list ) {
	  for( i = 0; i < list->length; i++ ) {
	  rch = (char_data*) list->pntr[i];
	  if( rch->array == room
	  && check_action_flags( rch, action->flags, false ) ) {
	  var_ch   = rch;
	  var_room = room;  
	  execute( action );
	  }
	  }
	  delete list;
	  }
	*/
	
	//	return false;
      }
    }
  }
  
  return false;
}


/*
 *   RANDOM ACODES
 */


void random_update( )
{
  for( int i = 0; i < random_actions; ++i ) {
    action_data *action = random_actions[i];
    room_data *room = action->room;
    player_data *pl = 0;
    if( action->trigger == TRIGGER_RANDOM ) {
      pl = rand_player( room );
    }
    if( action->trigger == TRIGGER_RANDOM_ALWAYS
	|| pl && action->trigger == TRIGGER_RANDOM ) {
      if( number_range( 0, 999 ) < action->value ) {
	clear_variables( );
	var_room = room; 
	var_ch     = pl;
	//	var_victim = 0;
	//	var_mob    = 0; 
	action->execute( );
      }
    } else if( action->trigger != TRIGGER_RANDOM ) {
      bug( "Random_Update: Non-random action in update list, room %d.",
	   room->vnum );
    }
  }
}


/*
 *   TIMER ACODES
 */


void action_update( )
{
  time_data start;
  gettimeofday( &start, 0 );

  for( int i = 0; i < timed_actions; ++i ) {
    action_data *action = timed_actions[i];
    room_data *room = action->room;
    if( action->trigger == TRIGGER_TIME ) {
      if( action->value >= 0 ) {
	if( ( unsigned ) action->value == (weather.minute+60*weather.hour ) ) {
	  clear_variables( );
	  var_room = room;
	  action->execute( );
	}
      } else {
	if( weather.tick % (-action->value) == 0 ) {
	  clear_variables( );
	  var_room = room;
	  action->execute( );
	}
      }
    }
  }

  pulse_time[ TIME_TIME_ACODE ] = stop_clock( start );
}


/*
 *   EDITING ROUTINES
 */


static action_data *find_action( char_data *ch, room_data *room, int i )
{
  action_data *action = 0;

  if( i >= 1 ) {
    int j = i;
    for( action = room->action ; action && j != 1; action = action->next, --j );
  }

  if( !action ) {
    send( ch, "No action number %d.\n\r", i );
  }

  return action;
}


/*
static void extract( action_data *action, wizard_data *wizard )
{
  for( int i = 0; i < action->data; ++i ) {
    wizard->adata_edit = action->data[i];
    extract( wizard, offset( &wizard->adata_edit, wizard ), "adata" );
  }

  wizard->action_edit = action;
  extract( wizard, offset( &wizard->action_edit, wizard ), "action" );
}
*/


void do_aedit( char_data *ch, const char *argument )
{
  action_data*   action;
  room_data*       room;
  int                 i;

  if( !( room = Room( ch->array->where ) ) ) {
    send( ch, "You aren't in a room.\n\r" );
    return;
  }

  wizard_data *imm = wizard( ch );

  if( !imm )
    return;

  if( !*argument ) {
    if( !( action = room->action ) ) {
      send( ch, "This room has no actions.\n\r" );
      return;
    }
    size_t len = 20;
    for( action = room->action; action; action = action->next ) {
      len = max( len, strlen( action->command ) );
    }
    page_underlined( ch, "   #  %*s  %s\n\r", len, "Trigger", "Target" );
    for( i = 0, action = room->action; action; action = action->next ) {
      if( action->trigger == TRIGGER_NONE ) {
	page( ch, "[%2d]  %*s  %s\n\r",
	      ++i, len, action->command, action->target );
      } else {
	const char *const keys = Keywords( action );
	if( !*keys ) {
	  page( ch, "[%2d]  %s%*s%s  %s\n\r", ++i,
		color_code( ch, COLOR_MILD ),
		len, action_trigger[action->trigger],
		color_code( ch, COLOR_DEFAULT ),
		*action->command ? "" : action->target );
	} else {
	  page( ch, "[%2d]  %s%*s  %s%s\n\r", ++i,
		color_code( ch, COLOR_MILD ),
		len, action_trigger[action->trigger],
		keys,
		color_code( ch, COLOR_DEFAULT ) );
	}
	if( *action->command ) {
	  page( ch, "      %*s  %s\n\r",
		len, action->command, action->target );
	}
      }
	/*

      if( *action->command )
        snprintf( tmp, TWO_LINES, "[%2d] %-20s %s - %s\n\r", ++i,
		  "Command",
		  action->command, action->target );
      else {
	//	switch( action->trigger ) {
	//	case TRIGGER_CAST:
	//	case TRIGGER_DESCRIBE:
	//	  snprintf( tmp, TWO_LINES, "[%2d] %-20s %s\n\r", ++i,
	//		    action_trigger[action->trigger], action->target );
	//	  break;
	//	default:
	snprintf( tmp, TWO_LINES, "[%2d] %-20s %s\n\r", ++i,
		  action_trigger[action->trigger], Keywords( action ) );
	//	  break;
	//	}
        tmp[5] = toupper( tmp[5] );
      }
      page( ch, tmp );
	*/
    }
    return;
  }
  
  if( !ch->can_edit( room ) )
    return;
  
  if( number_arg( argument, i ) ) {
    if( !( action = find_action( ch, room, i ) ) ) {
      return;
    }
    int j;
    if( isdigit( *argument ) && number_arg( argument, j ) ) {
      if( j == i ) {
        send( ch, "Moving an action to where it already is does nothing interesting.\n\r" ); 
        return;
      }
      if( j < 1 || j > count( room->action ) ) {
	send( ch, "You can only move an action to a sensible position.\n\r" );
	return;
      }
      if( !room->area->act_loaded )
	room->area->load_actions( );
      if( j == 1 ) {
        remove( room->action, action );
        action->next = room->action;
        room->action = action;
      } else {
        remove( room->action, action );
        action_data *prev = locate( room->action, j-1 );
        action->next = prev->next;
        prev->next = action;

      }
      room->area->act_dirty = true;
      room->area->modified = true;
      send( ch, "Action %d moved to position %d.\n\r", i, j );
      return;
    }
    imm->action_edit = action;
    imm->adata_edit  = 0;
    send( ch, "You now edit action %d.\n\r", i );
    return;
  }
  
  if( exact_match( argument, "new" ) ) {
    if( !room->area->act_loaded )
      room->area->load_actions( );
    room->area->act_dirty = true;
    room->area->modified = true;
    action = new action_data( room );
    if( *argument ) {
      for( i = 0; i < MAX_ATN_TRIGGER; ++i ) {
	if( !strcasecmp( argument, action_trigger[i] ) ) {
	  action->trigger = i;
	  fsend( ch, "Action added with %s trigger.", action_trigger[i] );
	  break;
	}
      }
      if( i == MAX_ATN_TRIGGER ) {
	action->command = alloc_string( argument, MEM_ACTION );
	fsend( ch, "Action added with command trigger \"%s\".", action->command );
      }
    } else {
      send( ch, "Action added with no trigger.\n\r" );
    }
    action->validate( );
    imm->action_edit = action;
    imm->adata_edit = 0;
    return;
  }
  
  if( exact_match( argument, "delete" ) ) {
    if( !*argument ) {
      if( !( action = imm->action_edit ) ) {
	send( ch, "You aren't editing any action.\n\r" );
	return;
      }
    } else {
      if( !number_arg( argument, i ) ) {
	send( ch, "Syntax: aedit delete [#]\n\r" );
	return;
      }
      if( !( action = find_action( ch, room, i ) ) ) {
	return;
      }
    }
    if( !room->area->act_loaded )
      room->area->load_actions( );
    room->area->act_dirty = true;
    room->area->modified = true;
    send( ch, "Action deleted.\n\r" );
    action_data *old_edit = 0;
    if( imm->action_edit != action ) {
      old_edit = imm->action_edit;
      imm->action_edit = action;
    }
    extract( imm, offset( &imm->action_edit, imm ), "action" );
    remove( room->action, action );  
    delete action;
    imm->action_edit = old_edit;
    return;
  }

  send( ch, "Illegal syntax.\n\r" );
  return;
}


void do_aflag( char_data* ch, const char *argument )
{
  wizard_data *imm = wizard( ch );

  if( !imm )
    return;

  action_data*     action;
  int                 max;

  if( !( action = imm->action_edit ) ) {
    send( ch, "You aren't editing any action.\n\r" );
    return;
  }

  for( max = 0; *action_flags[action->trigger][max]; ++max );

  if( max == 0 ) {
    send( ch, "That type of trigger has no action flags.\n\r" );
    return;
  }

  if( !*argument ) {
    display_flags( "Action",
		   &action_flags[action->trigger][0],
		   &action_flags[action->trigger][1],
		   &action->flags, max, ch );
    return;
  }
  
  room_data *room = action->room;

  //  if( !ch->can_edit( room ) )
  //    return;

  if( !set_flags( &action_flags[action->trigger][0],
		  &action_flags[action->trigger][1],
		  &action->flags, 0,
		  max, 0,
		  ch, argument, "action",
		  false, true ) ) 
    return;
  
  room->area->modified = true;
}


static void show_flags( char_data *ch, action_data *action )
{
  const char *const *flags = action_flags[ action->trigger ];

  if( !*flags[0] )
    return;

  page( ch, "%10s : ", "Flags" );

  bool found = false;

  for( int i = 0; *flags[i]; ++i ) {
    if( is_set( action->flags, i ) ) {
      if( !found ) {
	found = true;
	page( ch, flags[i] );
      } else {
	page( ch, ", %s", flags[i] );
      }
    }
  }

  if( !found ) {
    page( ch, "none\n\r" );
  } else {
    page( ch, "\n\r" );
  }
}


void do_astat( char_data* ch, const char *argument )
{
  wizard_data *imm = wizard( ch );

  if( !imm )
    return;

  action_data *action;

  if( *argument ) {
    int value;
    if( !number_arg( argument, value ) ) {
      send( ch, "Syntax: astat [<acode #>]\n\r" );
      return;
    }
    if( !( action = find_action( ch, ch->in_room, value ) ) ) {
      return;
    }
  } else if( !( action = imm->action_edit ) ) {
    send( ch, "You aren't editing any action.\n\r" );
    return;
  }

  page( ch, "%10s : %s\n\r", "Trigger", action_trigger[ action->trigger ] );
  page( ch, "%10s : %d\n\r", "Value", action->value );
  page( ch, "%10s : %s\n\r", "Command", action->command );
  page( ch, "%10s : %s\n\r", "Target", action->target );
  
  show_flags( ch, action );

  /*
  // Use buf for long string length.
  char buf [ 3*MAX_STRING_LENGTH ];
  snprintf( buf, 3*MAX_STRING_LENGTH, "\n\r[Code]\n\r%s\n\r",
	    action->Code( ) );
  page( ch, buf );
  */
  page( ch, "\n\r[Code]\n\r%s\n\r", action->Code( ) );
  show_extras( ch, action->Extra_Descr( ) );
}


void do_aset( char_data* ch, const char *argument )
{
  wizard_data *imm = wizard( ch );

  if( !imm )
    return;

  action_data *action;

  if( !( action = imm->action_edit ) ) {
    send( ch, "You aren't editing any action.\n\r" );
    return;
  }
  
  if( !*argument ) {
    do_astat( ch, argument );
    return;
  }
  
  room_data *room = action->room;
  area_data *area = room->area;

  //  if( !ch->can_edit( room ) )
  //    return;

#define att  action_trigger

  class type_field type_list[] = {
    { "trigger",  MAX_ATN_TRIGGER,  &att[0],  &att[1],  &action->trigger, true },
    { "" }
  };

#undef att

  if( const char *result = process( type_list, ch, "aset", argument ) ) {
    if( *result ) {
      action->invalidate( );
      action->validate( );
      area->modified = true;
    }
    return;
  }

  class string_field string_list[] = {
    { "target",    MEM_ACTION,  &action->target,   0 },
    { "command",   MEM_ACTION,  &action->command,  0 },
    { "",          0,           0,                 0 }
  };

  if( const char *result = process( string_list, ch, "aset", argument ) ) {
    if( *result )
      area->modified = true;
    return;
  }

  class int_field int_list[] = {
    { "value",      INT_MIN,  INT_MAX,  &action->value },
    { "",                 0,        0,               0 }
  };

  if( const char *result = process( int_list, ch, "aset", argument ) ) {
    if( *result )
      area->modified = true;
    return;
  }

  /*
  if( matches( argument, "value" ) ) {
    action->value = atoi( argument );
    send( ch, "Value on action set to %d.\n\r", action->value );
    area->modified = true;
    return;
  }
  */
  
  send( ch, "Syntax: aset <field> <value>\n\r" );
}


void do_acode( char_data* ch, const char *argument )
{
  wizard_data *imm = wizard( ch );

  if( !imm )
    return;
  
  action_data *action;

  if( !( action = imm->action_edit ) ) {
    send( ch, "You aren't editing any action.\n\r" );
    return;
  }

  room_data *room = action->room;

  //  if( *argument && !ch->can_edit( room ) )
  //    return;

  action->Edit_Code( ch, argument );

  room->area->used = true;  // Prevent area decompile.

  if( *argument || !action->binary ) {
    var_ch = ch;
    action->compile( );
  }
}


void do_adata( char_data* ch, const char *argument )
{
  wizard_data *imm = wizard( ch );

  if( !imm )
    return;

  action_data *action;

  if( !( action = imm->action_edit ) ) {
    send( ch, "You aren't editing any action.\n\r" );
    return;
  }

  room_data *room = action->room;
  area_data *area = room->area;

  //  if( *argument && !ch->can_edit( room ) )
  //    return;

  if( imm->adata_edit ) {
    if( exact_match( argument, "exit" ) ) {
      imm->adata_edit = 0;
      send( ch, "Adata now operates on the data list.\n\r" );
      return;
    }

    imm->adata_edit->text
      = edit_string( ch, argument, imm->adata_edit->text, MEM_EXTRA, true );

    if( *argument ) {
      area->act_dirty = true;
      area->modified = true;
    }

  } else {
    if( !*argument )
      show_defaults( ch, action->trigger, action_msgs, -1 );
    
    if( edit_extra( action->Extra_Descr( ), imm, offset( &imm->adata_edit, imm ),
		    argument, "adata" ) ) {
      area->act_dirty = true;
      area->modified = true;
    }
  }
  
  area->act_used = true;
  area->used = true;  // Prevent area decompile.

  if( *argument || !action->binary ) {
    var_ch = ch;
    action->compile( );
  }
}
