#include <ctype.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "define.h"
#include "struct.h"


Path_Data :: Path_Data( )
  : summoner(0), step(0), length(0), directions(0),
    range(100), delay(50),
    interrupt(true), valid(true), retry(false),
    notify(0)
{
  record_new( sizeof( help_data ), MEM_PATH );
}


Path_Data :: ~Path_Data( )
{
  record_delete( sizeof( help_data ), MEM_PATH );

  if( directions ) {
    delete [] directions;
    record_delete( length*sizeof( int ), -MEM_PATH );
  }
}


/*
 *   DISTANCE FUNCTIONS 
 */


static room_data *get_center( thing_data *th )
{
  room_data *center = 0;

  if( char_data *ch = character( th ) ) {
    center = ch->in_room;
    /*
  } else if( obj_data *obj = object( th ) ) {
    if( obj->array->where ) {
      if( char_data *ch = character( obj->array->where ) ) {
	if( obj->array == &ch->wearing
	    || obj->array == &ch->contents ) {
	  center = ch->in_room;
	}
      } else {
	center = Room( obj->array->where );
      }
    }
    */
  } else {
    center = Room( th );
  }

  return center;
}


// Call function for everyone within range.
void mark_range( thing_data *th, int range, path_func *function,
		 const char *arg1, exit_array *ignore,
		 int delay )
{
  room_data *center = get_center( th );

  if( !center ) {
    bug( "Mark_Range: center is not valid." );
    return;
  }

  if( range > MAX_PATH_RANGE )
    range = MAX_PATH_RANGE;

  room_array list, next;

  center->distance  = 0;
  list += center;

  /* LOOP THROUGH ROOMS */

  for( int i = 1; i <= range; ++i ) {
    for( int j = 0; j < list; ++j ) { 
      room_data *room = list[j];
      for( int k = 0; k < room->exits; ++k ) {
        exit_data *exit = room->exits[k];
	exit_data *back = reverse( exit );
	if( !back
	    || ignore && ignore->includes( back ) )
	  continue;
        room_data *to_room = exit->to_room;
        if( to_room->distance == INT_MAX ) {
          to_room->distance = i;
          next += to_room;
          for( int n = 0; n < to_room->contents; ++n ) {
            if( char_data *rch = character( to_room->contents[n] ) ) {
              if( ( *function )( rch, th, arg1,
				 back->direction, i, range, delay ) ) {
		return;
	      }
	    }
	  }
	}
      }
    }
    if( next.is_empty() )
      return;
    list.clear( );
    list.swap( next );
  }
}


void clear_range( thing_data *th )
{
  room_data *center = get_center( th );

  if( !center ) {
    bug( "Clear_Range: center is not valid." );
    return;
  }

  room_array list, next;

  list += center;
  center->distance = INT_MAX;

  while( true ) {
    for( int i = 0; i < list; ++i ) {
      room_data *room = list[i];
      for( int j = 0; j < room->exits; j++ ) {
        room_data *to_room = room->exits[j]->to_room;
        if( to_room->distance != INT_MAX ) {
          to_room->distance = INT_MAX;
          next += to_room;
	}
      }
    }
    if( next.is_empty() )
      return;
    list.clear( );
    list.swap( next );
  }
}


void exec_range( thing_data *th, int range, path_func* function,
		 const char *arg1, exit_array *ignore,
		 int delay )
{
  mark_range( th, range, function, arg1, ignore, delay );
  clear_range( th );
}


/*
 *   PATH ROUTINES
 */


bool mark_respond( char_data *ch, thing_data *goal, const char *string,
			  int, int, int range, int delay )
{
  if( is_set( ch->status, STAT_RESPOND ) ) {
    remove_bit( ch->status, STAT_RESPOND );
    return true;
  }

  return false;
}


static path_data *make_path( char_data *ch, int range,
			     path_data *path,
			     bool interrupt )
{
  room_data *room = ch->in_room;

  if( room->distance == 0 )
    return 0;

  unsigned length = 0;
  int *dir = new int [ room->distance ];

  while( room->distance != 0 ) {
    int max_move = INT_MAX;
    room_data *next = 0;
    for( int i = 0; i < room->exits; ++i ) {
      exit_data *exit = room->exits[i];
      if( path && path->ignore.includes( exit ) )
	continue;
      room_data *to_room = exit->to_room;
      if( to_room->distance == room->distance - 1 ) {
	int move = terrain_table[ to_room->sector_type ].mv_cost;
	if( move < max_move ) {
	  dir[length] = exit->direction;
	  next = to_room;
	  max_move = move;
	  if( move == 0 )
	    break;
	}
      }
    }
    if( max_move == INT_MAX ) {
      delete [] dir;
      return 0;
    }
    room = next;
    ++length;
  }

  bool create = !path;

  if( create ) {
    path = new path_data;
  } else {
    record_delete( path->length*sizeof( int ), -MEM_PATH );
    delete [] path->directions;
  }

  path->interrupt = interrupt;
  path->step = 0;
  path->length = length;
  path->range = range;
  path->directions = dir;
  path->valid = true;

  record_new( length*sizeof( int ), -MEM_PATH );

  // Don't validate summon/whistle paths, so responders will move as close as they can.
  if( path->interrupt ) {
    return path;
  }

  // Tell Can_Move() to ignore some restrictions.
  set_bit( ch->status, STAT_RESPOND );

  room = ch->in_room;
  for( unsigned i = 0; i < path->length; ++i ) {
    int move, type;
    int dir = path->directions[ i ];
    exit_data *exit = exit_direction( room, dir );
    if( is_set( exit->exit_info, EX_NO_PATH )
	|| !ch->Can_Move( 0, room, exit, dir, move, type, false, true, false, true ) ) {
      path->valid = false;
      path->retry = true;
      path->ignore += exit;
    }
    room = exit->to_room;
  }

  remove_bit( ch->status, STAT_RESPOND );

  return path;
}


event_data *add_path( char_data *ch, thing_data *goal, int range, int delay, bool interrupt )
{
  char_data *summoner = character( goal );
  room_data *room = summoner ? summoner->in_room : Room( goal );
  path_data *path = make_path( ch, range, 0, interrupt );

  if( !path )
    return 0;

  stop_events( ch, execute_path );
  stop_events( ch, execute_wander );

  event_data *event = new event_data( execute_path, ch );
  event->pointer = (void*) path;
  path->summoner = summoner;
  path->goal = room;
  path->delay = delay;

  return event;
}


static bool exec_retry_path( char_data *victim, thing_data *th, const char*,
			     int, int, int range, int delay )
{
  if( is_set( victim->status, STAT_RESPOND ) ) {
    remove_bit( victim->status, STAT_RESPOND );
    if( event_data *event = find_event( victim, execute_path ) ) {
      path_data *path = (path_data*) event->pointer;

      make_path( victim, range, path, path->interrupt );
      return true;
    }

    bug( "Retry_Path: no path event." );
    bug( "-- Char = %s in room %d.", victim->Name( ), victim->in_room->vnum );
    bug( "-- Destination = %s (%d).", th->Name( ), ((room_data*)th)->vnum );
    return true;
  }

  return false;
}


bool retry_path( event_data *event, exit_data *exit )
{
  path_data *path = (path_data*) event->pointer;

  if( exit ) {
    path->ignore += exit;
  }

  path->valid = false;

  // New range: enough to retrace steps plus original range.
  const int range = path->range+path->step;

  char_data *ch = (char_data*) event->owner;

  do {
    set_bit( ch->status, STAT_RESPOND );
    path->retry = false;
    exec_range( path->goal, range, exec_retry_path, empty_string, &path->ignore );
  } while( !path->valid && path->retry );

  remove_bit( ch->status, STAT_RESPOND );

  // This allows trying paths disregarding certain exits,
  // without mandating that the exit can never be used. e.g.: flee.
  if( exit && !path->valid ) {
    path->ignore -= exit;
  }

  return path->valid;
}


// Longer max wander delay.
// Makes mobs hang out longer at the path destination.
static void long_wander( event_data *event )
{
  char_data *ch = (char_data*) event->owner;
  const int wander = ch->species->wander;
 
  add_queue( event, number_range( wander/10, 2*wander ) );
}


static bool path_trigger( event_data *event, exit_data *exit )
{
  char_data *ch = (char_data*) event->owner;
  path_data *path = (path_data*) event->pointer;
  room_data *room = ch->in_room;
  char_data *summoner = path->summoner;
  bool interrupt = path->interrupt;

  if( !exit ) {
    extract( event );
  }

  if( !interrupt ) {
    if( mob_data *npc = mob( ch ) ) {
      for( mprog_data *mprog = npc->species->mprog; mprog; mprog = mprog->next ) {
	if( mprog->trigger == MPROG_TRIGGER_PATH ) {
	  push( );
	  clear_variables( );
	  var_ch = var_mob = ch;
	  var_room = room;
	  var_exit = exit;
	  var_victim = summoner;
          const int result = mprog->execute( );
          pop( );
 
	  if( !ch->Is_Valid( ) ) {
	    return true;
	  }

	  if( !result && event->Is_Valid( ) ) {
	    extract( event );
	  }

	  break;
	}
      }
    }
  }

  for( action_data *action = room->action; action; action = action->next ) {
    if( action->trigger == TRIGGER_PATH
	&& ( exit
	     ? is_set( action->flags, exit->direction )
	     : action->flags == 0 ) ) {
      push( );
      clear_variables( );
      var_ch = ch;
      var_room = room;
      var_exit = exit;
      var_victim = summoner;
      const int result = action->execute( );
      pop( );

      if( !ch->Is_Valid( ) ) {
	return true;
      }
      
      if( !result && event->Is_Valid( ) ) {
	extract( event );
      }

      break;
    }
  }

  if( !event->Is_Valid( ) ) {
    if( !find_event( ch, execute_path ) ) {
      if( !exit ) {
	long_wander( new event_data( execute_wander, ch ) );
      } else {
	delay_wander( new event_data( execute_wander, ch ) );
      }
    }
    return true;
  }

  if( !path->valid ) {
    // Trigger moved char, invalidating path.
    add_queue( event, number_range( (path->delay+1)/2, 3*path->delay/2 ) );
    return true;
  }

  return false;
}


void execute_path( event_data* event )
{
  char_data *ch = (char_data*) event->owner;
  path_data *path = (path_data*) event->pointer;
  room_data *room = ch->in_room;
  room_data *goal = path->goal;
  char_data *summoner = path->summoner;

  if( room == goal ) {
    path_trigger( event, 0 );
    if( path->notify ) {
      fsend_color( path->notify, COLOR_WIZARD,
		   "Path: complete at goal room." );
      fsend_color( path->notify, COLOR_WIZARD,
		   "-- Char = %s in room %d.",
		   ch->Seen_Name(), room->vnum );
      fsend_color( path->notify, COLOR_WIZARD,
		   "-- Goal = room %d.",
		   goal->vnum );
      if( summoner ) {
	fsend_color( path->notify, COLOR_WIZARD,
		     "-- Summoner = %s in room %d.",
		     summoner->Seen_Name( ), summoner->in_room->vnum );
      }
    }
    return;
  }

  if( summoner ) {	// Will be 0 if deceased or goal is a room.
    for( int i = 0; i < room->contents; ++i ) {
      if( room->contents[i] == summoner ) {
        path_trigger( event, 0 );
	if( path->notify ) {
	  fsend_color( path->notify, COLOR_WIZARD,
		       "Path: complete at summoner." );
	  fsend_color( path->notify, COLOR_WIZARD,
		       "-- Char = %s in room %d.",
		       ch->Seen_Name(), room->vnum );
	  fsend_color( path->notify, COLOR_WIZARD,
		       "-- Goal = room %d.",
		       goal->vnum );
	  if( path->summoner ) {
	    fsend_color( path->notify, COLOR_WIZARD,
			 "-- Summoner = %s in room %d.",
			 summoner->Seen_Name( ), summoner->in_room->vnum );
	  }
	}
	return;
      }
    }
  }

  if( !path->valid ) {
    // ch has been moved, but not by this function.
    retry_path( event, 0 );
  }

  if( path->valid ) {
    int dir = path->directions[ path->step ];
    exit_data *exit = exit_direction( room, dir );
    
    if( !exit ) {
      // Exit has been removed. Find another path.
      path->valid = false;
      execute_path( event );
      return;
    }

    if( behave( ch, exit, path->interrupt ) ) {
      // ch did something else.
      if( event->Is_Valid( ) ) {
	if( path->valid ) {
	  add_queue( event, number_range( (path->delay+1)/2, 3*path->delay/2 ) );
	} else {
	  execute_path( event );
	}
      }
      return;
    }

    if( path_trigger( event, exit ) ) {
      // ch is now dead, wandering, or re-pathed.
      return;
    }

    // Tell move_char() and can_move() to ignore some restrictions.
    set_bit( ch->status, STAT_RESPOND );

    if( move_char( ch, dir, false ) ) {
      ++path->step;
      if( ch->in_room == exit->to_room )
	path->valid = true;
      
    } else if( ch->Is_Valid( )
	       && !opponent( ch )
	       && !is_entangled( ch, 0 )
	       && ch->position > POS_RESTING ) {
      retry_path( event, exit );
    }
    
    remove_bit( ch->status, STAT_RESPOND );

    // Movement or door triggers may have killed ch.
    if( !ch->Is_Valid( ) )
      return;
  }

  if( path->valid
      && ( !path->interrupt || number_range( 1, 30 ) != 1 ) ) {
    add_queue( event, number_range( (path->delay+1)/2, 3*path->delay/2 ) );
    return;
  }
  
  if( !path->valid && !path->interrupt ) {
    if( path->notify ) {
      fsend_color( path->notify, COLOR_WIZARD,
		   "Path: unable to continue." );
      fsend_color( path->notify, COLOR_WIZARD,
		   "-- Char = %s in room %d.",
		   ch->Seen_Name(), ch->in_room->vnum );
      fsend_color( path->notify, COLOR_WIZARD,
		   "-- Goal = room %d.",
		   path->goal->vnum );
      if( path->summoner ) {
	fsend_color( path->notify, COLOR_WIZARD,
		     "-- Summoner = %s in room %d.",
		     path->summoner->Seen_Name( ), path->summoner->in_room->vnum );
      }
      
   } else {
      bug( "Execute_Path: no path found." );
      bug( "-- Char = %s in room %d.", ch->Seen_Name(), ch->in_room->vnum );
      bug( "-- Goal = room %d.", path->goal->vnum );
      if( path->summoner ) {
	bug( "-- Summoner = %s in room %d.",
	     path->summoner->Seen_Name( ), path->summoner->in_room->vnum );
      }
    }
  }

  extract( event );
  delay_wander( new event_data( execute_wander, ch ) );
}


/*
 *   PATH COMMAND
 */


static thing_data *find_victim( char_data* ch, const char *argument )
{
  int i;

  if( number_arg( argument, i ) )
    return get_room_index( i, false );

  char_data *victim;

  if( ( victim = one_character( ch, argument, empty_string,
				(thing_array*) &player_list,
				(thing_array*) &mob_list ) ) )
    return victim;
  
  return 0;
}


void do_path( char_data *ch, const char *argument )
{
  int flags;
  if( !get_flags( ch, argument, &flags, "v", "path" ) )
    return;

  if( !*argument ) {
   // Show all paths.
    bool found = false;
    for( int i = 0; i < mob_list; ++i ) {
      mob_data *mob = mob_list[i];
      if( !mob->Is_Valid( ) )
	continue;
      if( event_data *event = find_event( mob, execute_path ) ) {
	path_data *path = (path_data*) event->pointer;
	if( !found ) {
	  page_title( ch, "Active Paths" );
	  found = true;
	}
	mob->Select( 1 );
	page( ch, "\n\r" );
	page( ch, "%s [%d] at %s [%d]%s\n\r",
	      mob, mob->species->vnum, mob->in_room, mob->in_room->vnum,
	      path->valid ? ":" : " (invalid):" );
	page( ch, "  Goal: %s [%d]\n\r", path->goal->name, path->goal->vnum );
	if( path->summoner && path->summoner->Is_Valid( ) ) {
	  path->summoner->Select( 1 );
	  if( path->summoner->species ) {
	    page( ch, "  Summoner: %s [%d] at %s [%d]\n\r",
		  path->summoner, path->summoner->species->vnum,
		  path->summoner->in_room, path->summoner->in_room->vnum );
	  } else {
	    page( ch, "  Summoner: %s [player] at %s [%d]\n\r",
		  path->summoner,
		  path->summoner->in_room, path->summoner->in_room->vnum );
	  }
	}
	page( ch, "  Step: %u/%u\n\r", path->step, path->length );

	if( is_set( flags, 0 ) ) {
	  if( find_event( mob, execute_wander ) ) {
	    fpage( ch, "*** %s [%d] at %s [%d] has both path and wander events!",
		   mob, mob->species->vnum, mob->in_room, mob->in_room->vnum );
	  }
	}

      } else if( is_set( flags, 0 ) ) {
	if( !find_event( mob, execute_wander ) ) {
	  fpage( ch, "*** %s [%d] at %s [%d] has no path or wander event!",
		 mob, mob->species->vnum, mob->in_room, mob->in_room->vnum );
	}
      }
    }
    if( !found ) {
      send( ch, "No active paths.\n\r" );
    }

    return;
  }

  char arg [ MAX_INPUT_LENGTH ];
  char_data *victim;

  if( !two_argument( argument, "to", arg ) ) {
    //    send( ch, "Syntax: path <mob> [to] <destination>\n\r" );
    if( !( victim = one_character( ch, argument, "path",
				   ch->array ) ) ) {
      return;
    }
    if( event_data *event = find_event( victim, execute_path ) ) {
      extract( event );
      delay_wander( new event_data( execute_wander, victim ) );
      fsend( ch, "%s returned to wandering.", victim );
    } else {
      fsend( ch, "%s has no path.", victim );
    }
    return;

  } else {
    if( !( victim = one_character( ch, arg, "path",
				   ch->array ) ) ) {
      return;
    }
  }

  if( victim->pcdata
      || ( is_set( victim->status, STAT_PET )
	   && victim->leader != ch
	   && get_trust( victim->leader ) >= get_trust( ch ) ) ) {
    fsend( ch, "You cannot path %s.", victim );
    return;
  }

  if( !*argument ) {
    return;
  }

  thing_data *to;
  if( !( to = find_victim( ch, argument ) ) ) {
    send( ch, "No such location.\n\r" );
    return;
  }
  
  const int range = MAX_PATH_RANGE;
  const int delay = 32;

  set_bit( victim->status, STAT_RESPOND );

  mark_range( to, range, mark_respond, 0, 0, delay );

  if( !is_set( victim->status, STAT_RESPOND ) ) {
    event_data *event = add_path( victim, to, range, delay, false );
    clear_range( to );
    if( event ) {
      path_data *path = (path_data*) event->pointer;
      path->notify = ch;
      if( !path->valid ) {
	retry_path( event, 0 );
	if( !path->valid ) {
	  if( room_data *room = Room( to ) ) {
	    fsend( ch, "No valid path to room %s [%d].", room, room->vnum );
	  } else {
	    char_data *rch = character( to );
	    fsend( ch, "No valid path to character %s in room %s [%d].",
		   rch, rch->in_room, rch->in_room->vnum );
	  }
	  extract( event );
	  delay_wander( new event_data( execute_wander, victim ) );
	  return;
	}
      }

      //add_queue( event, number_range( (delay+1)/2, 3*delay/2 ) );
      execute_path( event );
      return;
    }

    //      delay_wander( new event_data( execute_wander, victim ) );
    if( room_data *room = Room( to ) ) {
      fsend( ch, "No route found to room %s [%d].", room, room->vnum );
    } else {
      char_data *rch = character( to );
      fsend( ch, "No route found to character %s in room %s [%d].",
	     rch, rch->in_room, rch->in_room->vnum );
    }
    return;
  }

  clear_range( to );
  remove_bit( victim->status, STAT_RESPOND );
  //  delay_wander( new event_data( execute_wander, victim ) );

  if( room_data *room = Room( to ) ) {
    fsend( ch, "Room %s [%d] not within range.", room, room->vnum );
  } else {
    char_data *rch = character( to );
    fsend( ch, "Character %s in room %s [%d] not within range.",
	   rch, rch->in_room, rch->in_room->vnum );
  }
}


/* 
 *   WHISTLE ROUTINE
 */


static const char *const whistle_msg = 
  "Your incessant whistling seems to have attracted a disgruntled mail\
 daemon.\n\rIt stamps up to you and mutters a few strange words, then in\
 an\n\runcharacteristic gesture smiles happily and waves to you.  Finally\
 with\n\ra snap of his fingers it disappears in a cloud of dark smoke.\n\r";


obj_data* has_whistle( char_data* ch )
{
  obj_data* obj;

  if( ( obj = ch->Wearing( WEAR_HELD_R, LAYER_BASE ) )
      && obj->pIndexData->item_type == ITEM_WHISTLE ) 
    return obj;
  
  if( ( obj = ch->Wearing( WEAR_HELD_L, LAYER_BASE ) )
      && obj->pIndexData->item_type == ITEM_WHISTLE ) 
    return obj;

  return 0;
}


static bool hear_whistle( char_data* victim, thing_data*, const char*,
			  int dir, int, int range, int delay )
{
  if( victim->position == POS_EXTRACTED 
      || !victim->Can_Hear() ) 
    return false;

  fsend( victim, "You hear a whistle from somewhere %s.",
	 dir_table[dir].where );

  return false;
}


static bool respond_whistle( char_data* victim, thing_data *th, const char*,
			     int dir, int, int range, int delay )
{
  char_data *ch = (char_data*)th;

  if( victim->position == POS_EXTRACTED 
      || !victim->Can_Hear() ) 
    return false;

  if( victim->pcdata
      || !is_set( victim->status, STAT_PET )
      || victim->leader != ch )
    return false;
  
  if( victim->position == POS_RESTING
      || victim->position == POS_MEDITATING ) 
    do_stand( victim, "" );

  if( is_set( victim->status, STAT_FAMILIAR ) )
    send( ch, "You sense that your familiar heard you.\n\r" );

  if( event_data *event = add_path( victim, ch, range, delay ) ) {
    add_queue( event, number_range( (delay+1)/2, 3*delay/2 ) );
  }

  return false;
}


void do_whistle( char_data* ch, const char * )
{
  player_data*        pc;
  int              range  = 15;

  if( !can_talk( ch, "whistle" ) )
    return;

  if( obj_data *obj = has_whistle( ch ) ) {
    oprog_data *oprog;
    for( oprog = obj->pIndexData->oprog; oprog; oprog = oprog->next ) {
      if( oprog->trigger == OPROG_TRIGGER_USE ) {
	obj->Select( 1 );
	clear_variables( );
	//        var_victim = 0;
	//        var_mob = 0;
        var_room = ch->in_room;
        var_ch = ch;
	var_obj = obj;
	var_def = use_msg;
	var_def_type = ITEM_WHISTLE;
        if( !oprog->execute( ) )
          return;
        break;
      }
    }
    act( ch, prog_msg( oprog, use_msg[2] ), 0, 0, obj );
    act_notchar( prog_msg( oprog, use_msg[3] ), ch, 0, obj );
    range = obj->value[0];

  } else {
    if( ch->shdata->race == RACE_LIZARD ) {
      send( ch,
	    "You try to whistle but the best you can do is hiss loudly.\n\r" );
      fsend( *ch->array,
	     "%s hisses loudly - you are not sure what it means.", ch );
      return;
    }
    send( ch, "You whistle as loud as you can.\n\r" );
    fsend( *ch->array, "%s whistles loudly.", ch );
  }

  exec_range( ch, range, hear_whistle );
  exec_range( ch, range, respond_whistle );

  if( ( pc = player( ch ) )
      && !privileged( pc, LEVEL_BUILDER )
      && pc->whistle++ >= 3 ) {
    send( ch, whistle_msg );
    spell_silence( ch, ch, 0, 10, 5 );
  }
}


/*
 *   SUMMON_HELP ROUTINE
 */


static bool respond_summon( char_data* victim, thing_data *th, const char*,
			    int, int, int range, int delay )
{
  // ch is summoner.
  char_data *ch = (char_data*) th;

  if( ch->pcdata ) {
    bug( "Respond_summon: summoner is not a mob." );
    return true;
  }

  if( victim->pcdata
      || victim->position < POS_RESTING )
    return false;
  
  // If the mob would flee, it probably won't respond.
  if( victim->hit < victim->max_hit/4
      && is_set( victim->species->act_flags, ACT_WIMPY )
      && number_range( 0, 5 ) != 0 )
    return false;

  const int nation = ch->species->nation;
  const int group = ch->species->group;
  
  if( ( nation == NATION_NONE
	|| nation != victim->species->nation )
      && ( group == GROUP_NONE
	   || group != victim->species->group ) )
    return false;
  
  for( mprog_data *mprog = victim->species->mprog; mprog; mprog = mprog->next ) {
    if( mprog->trigger == MPROG_TRIGGER_SUMMON ) {
      clear_variables( );
      var_ch = ch;
      var_mob = victim;
      var_room = ch->in_room;
      if( !mprog->execute( )
	  || !victim->Is_Valid( ) )
	return false;
      break;
    }
  }

  if( is_set( victim->species->act_flags, ACT_SUMMONABLE ) ) {
    if( victim->position == POS_RESTING )
      do_stand( victim, "" );
    remove_bit( victim->status, STAT_SENTINEL );
    if( event_data *event = add_path( victim, ch, range, delay ) ) {
      add_queue( event, number_range( (delay+1)/2, 3*delay/2 ) );
    }
  }

  return false;
}


void summon_help( char_data *ch, char_data*, int range, int delay )
{
  exec_range( ch, range, respond_summon, 0, 0, delay );
}


/*
 *   MAP ROUTINES
 */


void do_map( char_data* ch, const char *argument )
{
  int x, y;
  int flags;
  bool scan = false;

  if( is_apprentice( ch ) ) {
    if( !get_flags( ch, argument, &flags, "nlvp", "map" ) )
      return;

    x = 59;
    y = 23;
    
    if( is_set( flags, 1 ) ) {
      x = 115; y = 39;
    } else if( is_set( flags, 2 ) ) {
      x = 143; y = 59;
    } else if( is_set( flags, 3 ) ) {
      x = 15; y = 15;
      scan = true;
    } else {
      x = 59; y = 23;
    }
    
  } else {
    if( !get_flags( ch, argument, &flags, "n", "map" ) )
      return;
    if( ch->position < POS_STANDING ) {
      pos_message( ch );
      return;
    }
    if( is_fighting( ch )
	|| !ch->Can_See( true ) ) {
      return;
    }
    x = 15; y = 15;
    scan = true;
  }

  show_map( ch, 0, y, x, is_set( flags, 0 ), scan );
}


static bool plot( char (*map)[143], int (*color)[143], int (*nums)[143],
		  int length, int width, int pos,
		  char letter, char col, int vnum,
		  int& top, int& bottom,
		  int *terrain, int sector )
{
  const int l = pos%1024-512;
  const int x = (pos/1024)%1024-512+width/2;
  const int y = (pos/1024/1024)-512+length/2;

  if( l != 0
      || x < 0 || x >= width
      || y < 0 || y >= length ) 
    return false;

  top = min( top, y );
  bottom = max( bottom, y );

  if( terrain ) {
    set_bit( terrain, sector );
  }

  if( map[y][x] != ' ' ) {
    if( map[y][x] != letter || vnum > 0 && nums[y][x] != vnum ) {
      color[y][x] = COLOR_BOLD_RED;
    }
    if( map[y][x] != letter && map[y][x] != '*' ) {
      map[y][x] = '?';
    }
  } else {
    map[y][x] = letter;
    color[y][x] = col;
    nums[y][x] = vnum;
  }

  return letter == '*';
}


static int offset( int pos, int direction )
{
  int l = pos%1024;
  int x = (pos/1024)%1024;
  int y = (pos/1024/1024);

  switch( direction ) {
    case DIR_NORTH :  if( y-- == 0 )    return -1;  break;
    case DIR_SOUTH :  if( y++ == 1024 ) return -1;  break;
    case DIR_WEST  :  if( x-- == 0 )    return -1;  break;
    case DIR_EAST  :  if( x++ == 1024 ) return -1;  break;
    case DIR_UP    :  if( l-- == 0 )    return -1;  break;
    case DIR_DOWN  :  if( l++ == 1024 ) return -1;  break;
  }

  return y*1024*1024+x*1024+l;
}


static char room_sym( char_data *ch, room_data *room )
{
  if( room == ch->in_room )
    return '*';

  exit_data *exit;

  const bool up = ( exit = exit_direction( room, DIR_UP ) ) && exit->Seen( ch );
  const bool down = ( exit = exit_direction( room, DIR_DOWN ) ) && exit->Seen( ch );

  return up ? ( down ? 'X' : '>' ) : ( down ? '<' : 'O' );
}


// NOTE: length and width MUST be 1 less than multples of 4.
void show_map( char_data* ch, room_data *room, int length, int width, bool mono, bool scan )
{
  const char *const key[] = { 
    "Key:",
    "",
    "*  you",
    "O  room",
    scan ? "?  unknown" : "?  overlap",
    "+  door",
    "|  ns exit",
    "-  ew exit",
    ">  up exit",
    "<  down exit",
    "X  u&d exit",
    0
  };
   
  const char sym_exit[] = { '|', '-', '|', '-' };

  char              map  [ 59 ][ 143 ];
  int             color  [ 59 ][ 143 ];
  int              nums  [ 59 ][ 143 ];
  int               top  = length/2;
  int            bottom  = length/2;
  int               pos;

  int terrain[ (table_max[ TABLE_TERRAIN ]+31)/32 ];
  vzero( terrain, (table_max[ TABLE_TERRAIN ]+31)/32 );
    
  if( !room ) {
    room = ch->in_room;
  }
    
  bool shown = ( room == ch->in_room );

  for( int i = 0; i < length; ++i ) { 
    for( int j = 0; j < width; ++j ) {
      map[i][j] = ' ';
      color[i][j] = COLOR_DEFAULT;
      nums[i][j] = -1;
    }
    map[i][width] = '\0';
  }
  
  map[length/2][width/2] = room_sym( ch, room );
  if( !scan || room->Seen( ch ) ) {
    color[length/2][width/2] = terrain_table[room->sector_type].color;
    set_bit( terrain, room->sector_type );
  }
  nums[length/2][width/2] = room->vnum;
  
  if( scan ) {
    room->distance = 1024*1024*512+1024*512+512;
    top = 0;
    bottom = length-1;
    unsigned count =  ( ch->get_skill( SKILL_SCAN ) == UNLEARNT ) ? 1 : 2;
    shown = true;

    room_array list1, list2, list3;    
    list1 += room;
    list3 += room;

    // Generate a much smaller, simpler map for players.
    while( !list1.is_empty() ) {
      for( int i = 0; i < list1; ++i ) {
	room_data *room = list1[i];
	bool from_dark = !room->Seen( ch );
	for( int i = 0; i < room->exits; ++i ) {
	  exit_data *exit = room->exits[i];
	  // Do not follow up/down exits.
	  if( !exit->Seen( ch )
	      || exit->direction > DIR_WEST )
	    continue;
	  room_data *to_room = exit->to_room;
	  const bool to_dark = !to_room->Seen( ch );
	  if( from_dark && to_dark )
	    continue;
	  pos = offset( room->distance, exit->direction );
	  plot( map, color, nums, length, width, pos,
		is_set( exit->exit_info, EX_ISDOOR ) ? '+' : sym_exit[ exit->direction ],
		COLOR_DEFAULT, -1,
		top, bottom,
		0, 0 );
	  if( is_set( exit->exit_info, EX_CLOSED ) )
	    continue;
	  if( count != 0 ) {
	    if( to_room->area->status != AREA_OPEN )
	      continue;
	    if( to_room->distance == INT_MAX
		&& ( pos = offset( pos, exit->direction ) ) != -1 ) {
	      const bool noscan = is_set( exit->exit_info, EX_NO_SCAN );
	      const char sym = ( noscan || to_dark ) ? '?' : room_sym( ch, to_room );
	      const int col = ( noscan || to_dark ) ? COLOR_DEFAULT : terrain_table[to_room->sector_type].color;
	      plot( map, color, nums, length, width, pos,
		    sym,
		    col,
		    room->vnum,
		    top, bottom,
		    terrain, to_room->sector_type );
	      if( !noscan ) {
		to_room->distance = pos;
		list2 += to_room;
	      }
	      list3 += to_room;
	    }
	  }
	}
      }
      --count;
      list1.clear( );
      list1.swap( list2 );
    }

    for( int i = 0; i < list3; ++i ) {
      list3[i]->distance = INT_MAX;
    }
    
  } else {
    room->distance = 1024*1024*512+1024*512+512;

    room_array list1, list2;    
    list1 += room;
    
    while( !list1.is_empty() ) {
      for( int i = 0; i < list1; ++i ) {
	room_data *room = list1[i];
	for( int j = 0; j < room->exits; ++j ) {
	  exit_data *exit = room->exits[j];
	  if( !exit->Seen( ch ) )
	    continue;
	
	  // Plot doors.
	  pos = offset( room->distance, exit->direction );
	  plot( map, color, nums, length, width, pos,
		is_set( exit->exit_info, EX_ISDOOR ) ? '+' : sym_exit[ exit->direction ],
		COLOR_DEFAULT, -1,
		top, bottom,
		0, 0 );
	
	  room_data *to_room = exit->to_room;

	  if( !is_apprentice( ch ) && to_room->area->status != AREA_OPEN )
	    continue;

	  // Plot rooms.
	  if( to_room->distance == INT_MAX
	      && ( pos = offset( pos, exit->direction ) ) != -1 ) {
	    const int col = terrain_table[to_room->sector_type].color;
	    shown = plot( map, color, nums, length, width, pos,
			  room_sym( ch, to_room ),
			  col,
			  room->vnum,
			  top, bottom,
			  terrain, to_room->sector_type ) || shown;
	    to_room->distance = pos;
	    list2 += to_room;
	  }
	}
      }
      list1.clear( );
      list1.swap( list2 );
    }

    for( area_data *area = area_list; area; area = area->next )
      for( room = area->room_first; room; room = room->next )
	room->distance = INT_MAX;
  }

  // Clean up color key.
  int t = 0;
  for( int i = 0; i < table_max[ TABLE_TERRAIN ]; ++i ) {
    if( is_set( terrain, i ) ) {
      if( terrain_table[i].color != COLOR_DEFAULT ) {
	++t;
      } else {
	remove_bit( terrain, i );
      }
    }
  }

  /* DISPLAY MAP */
  
  send( ch, "\n\r" );

  char buf [ MAX_STRING_LENGTH ];

  int k = 0;
  int s = 0;

  for( int i = 0; i <= bottom-top || key[k] || t > 0 && !mono; ++i ) {
    *buf = '\0';
    int b = 0;
    int last_color = COLOR_DEFAULT;
    if( i > bottom-top ) {
      send( ch, "%*s", width, "" );
    } else {
      for( int j = 0; j < width; ++j ) {
	if( nums[i+top][j] == -1
	    && map[i+top][j] != ' ' ) {
	  if( (i+top)%2 == 0 ) {
	    // N/S exit.
	    if( i > 1
		&& i < bottom-top 
		&& color[i+top-1][j] == color[i+top+1][j] ) {
	      color[i+top][j] = color[i+top-1][j];
	    }
	  } else {
	    // E/W exit.
	    if( j > 1
		&& j < width-1 
		&& color[i+top][j-1] == color[i+top][j+1] ) {
	      color[i+top][j] = color[i+top][j-1];
	    }
	  }
	}
	if( map[i+top][j] == '*' ) {
	  last_color = -1;
	  if( mono ) {
	    b += snprintf( buf+b, MAX_STRING_LENGTH, "*" );
	  } else {
	    last_color = color[i+top][j];
	    b += snprintf( buf+b, MAX_STRING_LENGTH, "%s*%s",
			   color_reverse( ch, last_color ),
			   color_code( ch, last_color ) );
	  }
	} else if( color[i+top][j] != last_color && !mono ) {
	  last_color = color[i+top][j];
	  b += snprintf( buf+b, MAX_STRING_LENGTH, "%s%c",
			 color_code( ch, last_color ), map[i+top][j] );
	} else {
	  b += snprintf( buf+b, MAX_STRING_LENGTH, "%c",
			 map[i+top][j] );
	}
      }
      send( ch, buf );
    }

    if( !shown && key[k] && *key[k] == '*' ) {
      ++k;
    }
    if( key[k] ) {
      // Basic map key.
      if( mono ) {
	send( ch, "     %s", key[k++] );
      } else {
	send_color( ch, COLOR_DEFAULT, "     %s", key[k++] );
      }
    } else if( t > 0 && !mono ) {
      // Color key.
      while( !is_set( terrain, s ) ) ++s;
      send_color( ch, terrain_table[s].color, "     (%s)", terrain_table[s].name );
      ++s;
      --t;
    }
    send( ch, "\n\r" );
  }
}
