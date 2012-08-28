#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include "define.h"
#include "struct.h"


const void *code_path( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];
  thing_data *th = (thing_data*) argument[1];
  int range = (int) argument[2];
  int delay = (int) argument[3];

  if( !ch ) {
    code_bug( "Path: character is null." );
    return 0;
  }

  if( ch->pcdata ) {
    code_bug( "Path: character is not a mob." );
    return 0;
  }

  if( !th ) {
    // Null goal == cancel path, if any.
    stop_events( ch, execute_path );
    stop_events( ch, execute_wander );
    delay_wander( new event_data( execute_wander, ch ) );
    return 0;
  }
  
  if( !Room( th ) && !character( ch ) ) {
    code_bug( "Path: goal is not a room or character." );
    return 0;
  }
  
  if( range == 0 ) {
    range = 24;
  } else if( range < 0 ) {
    code_bug( "Path: negative range." );
    return 0;
  }

  if( delay == 0 ) {
    delay = 50;
  } else if( delay < 0 ) {
    code_bug( "Path: negative delay." );
    return 0;
  } else if( delay < 5 ) {
    delay = 5;
  }

  if( th == ch->in_room ) {
    return 0;
  }

  set_bit( ch->status, STAT_RESPOND );

  mark_range( th, range, mark_respond, 0, 0, delay );

  if( !is_set( ch->status, STAT_RESPOND ) ) {
    event_data *event = add_path( ch, th, range, delay, false );
    clear_range( th );
    if( event ) {
      path_data *path = (path_data*) event->pointer;
      if( !path->valid ) {
	retry_path( event, 0 );
	if( !path->valid ) {
	  code_bug( "Path: no valid path found." );
	  extract( event );
	  delay_wander( new event_data( execute_wander, ch ) );
	  return 0;
	}
      }

      //add_queue( event, number_range( (delay+1)/2, 3*delay/2 ) );
      execute_path( event );
      return event;
    }

    code_bug( "Path: no route found." );
    return 0;
  }

  clear_range( th );
  remove_bit( ch->status, STAT_RESPOND );
  code_bug( "Path: goal not within range." );

  return 0;
}


const void *code_path_next( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];

  if( !ch ) {
    code_bug( "Path_Next: character is null." );
    return 0;
  }

  if( !ch->species ) {
    code_bug( "Path_Next: character is not a mob." );
    return 0;
  }

  event_data *event = find_event( ch, execute_path );

  if( !event )
    return 0;

  path_data *path  = (path_data*) event->pointer;
  
  if( !path->valid || path->step == path->length )
    return 0;

  int dir = path->directions[ path->step ];
  exit_data *exit = exit_direction( ch->in_room, dir );
  
  return exit;
}


const void *code_path_end( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];

  if( !ch ) {
    code_bug( "Path_End: character is null." );
    return 0;
  }

  if( !ch->species ) {
    code_bug( "Path_End: character is not a mob." );
    return 0;
  }

  event_data *event = find_event( ch, execute_path );

  if( !event )
    return 0;

  path_data *path  = (path_data*) event->pointer;
  
  if( !path->valid )
    return 0;

  return path->goal;
}


const void *code_path_retry( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];

  if( !ch ) {
    code_bug( "Path_Retry: character is null." );
    return 0;
  }

  if( !ch->species ) {
    code_bug( "Path_Retry: character is not a mob." );
    return 0;
  }

  event_data *event = find_event( ch, execute_path );

  if( !event ) {
    code_bug( "Path_Retry: no path." );
    return 0;
  }

  path_data *path  = (path_data*) event->pointer;

  path->valid = false;

  return 0;
}


const void *code_path_target( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];

  if( !ch ) {
    code_bug( "Path_Target: character is null." );
    return 0;
  }

  if( !ch->species ) {
    code_bug( "Path_Target: character is not a mob." );
    return 0;
  }

  event_data *event = find_event( ch, execute_path );

  if( !event )
    return 0;

  path_data *path  = (path_data*) event->pointer;
  
  if( !path->valid )
    return 0;

  return path->summoner;
}
