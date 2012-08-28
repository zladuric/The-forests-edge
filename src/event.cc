#include <ctype.h>
#include <sys/types.h>
#include <stdio.h>
#include <syslog.h>
#include "define.h"
#include "struct.h"


event_data*  event_queue  [ QUEUE_LENGTH ];
int           event_pntr  = 0;


event_data :: event_data( event_func* f, thing_data* t )
{
  record_new( sizeof( event_data ), MEM_EVNT );

  func = f;
  loop = 0;
  time = -1;
  owner = t;
  pointer = 0;

  t->events += this;
}


event_data :: event_data( )
{
  record_new( sizeof( event_data ), MEM_EVNT );

  func = 0;
  loop = 0;
  time = -1;
  owner = 0;
  pointer = 0;
}


event_data :: ~event_data( )
{
  if( time == -2 ) {
    roach( "Attempting to delete event twice." );
    return;
  }

  record_delete( sizeof( event_data ), MEM_EVNT );

  if( time != -1 ) {
    roach( "Deleting active event." );
    roach( "-- Event Owner = %s", owner->Seen_Name( ) );
    roach( "--  Event Type = %s", name( this ) );
  }

  if( func == execute_path ) {
    delete (path_data*) pointer;
  }

  time = -2;
}


/*
 *   DELAY FUNCTIONS
 */


int modify_delay( char_data* ch, int delay )
{
  if( !ch )
    return delay;

  bool slow = ch->is_affected( AFF_SLOW );
  bool haste = ch->is_affected( AFF_HASTE );
  
  if( haste && !slow ) {
    return 2*(delay+1)/3;
  } else if( slow && !haste ) {
    return 3*delay/2;
  }
  
  return delay;
}


void set_delay( char_data* ch, int delay, bool mod )
{
  if( !ch->Is_Valid( ) )
    return;

  remove_bit( ch->status, STAT_WAITING );

  unlink( &ch->active );

  int d = mod ? modify_delay( ch, delay ) : delay;

  add_queue( &ch->active, d );
}


void set_min_delay( char_data* ch, int delay, bool mod )
{
  if( !ch->Is_Valid( ) )
    return;

  int d = mod ? modify_delay( ch, delay ) : delay;

  if( ch->active.time != -1 ) {
    int til = time_till( &ch->active );
    d = max( d, til );
    if( d == til )
      return;
  }

  remove_bit( ch->status, STAT_WAITING );

  unlink( &ch->active );
  add_queue( &ch->active, d );
}


void set_update( char_data* ch )
{
  if( !ch->Is_Valid( ) )
    return;

  unlink( &ch->update );

  add_queue( &ch->update, affect_delay( ) );
}


void set_regen( char_data* ch )
{
  if( !ch->Is_Valid( ) )
    return;

  unlink( &ch->regen );

  add_queue( &ch->regen, number_range( PULSE_MOBILE/2, 3*PULSE_MOBILE/2 ) );
}


/*
 *   SUPPORT ROUTINES
 */


event_data *find_event ( thing_data *thing, event_func *func )
{
  for( int i = 0; i < thing->events; ++i ) {
    event_data *event = thing->events[i];
    if( event->func == func )
      return event;
  }

  return 0;
}


int time_till( event_data* event )
{
  if( event->time < 0 )
    return QUEUE_LENGTH+1;

  return( (event->time-event_pntr)%QUEUE_LENGTH );
}


void add_queue( event_data* event, int delay )
{
  if( event->time != -1 ) {
    roach( "Attempting to add non-idle event to queue." );
    roach( "-- Owner = %s", event->owner );
    roach( "--  Type = %s", name( event ) );
    roach( "--  Time = %d", event->time );
    return;
  }
 
  if( delay > QUEUE_LENGTH ) {
    roach( "Attempting to add event with delay > queue length." );
    roach( "-- Owner = %s", event->owner );
    roach( "--  Type = %s", name( event ) );
    delay = QUEUE_LENGTH;
  }

  const int pos = (event_pntr+delay)%QUEUE_LENGTH;

  event->loop      = event_queue[pos];
  event->time      = pos;
  event_queue[pos] = event;
}


const char *name( event_data *event )
{
  if( event->func == next_action )    return "Fight";
  if( event->func == execute_wander ) return "Wander";
  if( event->func == execute_drown )  return "Drown";
  if( event->func == execute_fall )   return "Fall";
  if( event->func == execute_leap )   return "Leap";
  if( event->func == update_affect )  return "Obj Affect";
  if( event->func == execute_decay )  return "Decay";
  if( event->func == execute_junk )   return "Junk";
  if( event->func == execute_path )   return "Path";
  if( event->func == execute_update )   return "Update";
  if( event->func == execute_regen )   return "Regen";
  if( event->func == execute_obj_fall )   return "Obj_Fall";
  if( event->func == execute_obj_rise )   return "Obj_Rise";
  if( event->func == execute_obj_float )   return "Obj_Float";
  if( event->func == execute_obj_sink )   return "Obj_Sink";
  if( event->func == execute_mob_timer )   return "Mob Timer";
  //  if( event->func == execute_hunger )  return "Hunger";
  //  if( event->func == execute_thirst )  return "Thirst";
  //  if( event->func == execute_drunk )  return "Drunk";

  return "Unknown";
}


/* 
 *   EXTRACT
 */


void stop_events( thing_data* thing, event_func* func )
{
  event_data*  event;

  for( int i = thing->events.size-1; i >= 0; i-- ) {
    event = thing->events[i];
    if( !func || event->func == func ) {
      event->owner = 0;
      thing->events.remove( i );
      extract( event );
    }
  }
}


void extract( event_data* event )
{
  if( event->owner ) {
    event->owner->events -= event;
    event->owner = 0;
  }

  unlink( event );
  extract_events += event;
  //  delete event;
}


void unlink( event_data* event )
{
  event_data* prev;
  int         time;

  if( ( time = event->time ) < 0 )
    return;

  time        = event->time;
  prev        = event_queue[time];
  event->time = -1;

  if( prev == event ) {
    event_queue[time] = prev->loop;
    return;
  }

  for( ; prev->loop != event; prev = prev->loop );

  prev->loop = event->loop;
  event->loop = 0;
}


/*
 *   MAIN EVENT HANDLERS
 */
  

void event_update( void )
{
  time_data start;
  gettimeofday( &start, 0 );

  while( event_data *event = event_queue[ event_pntr ] ) {
      
    if( event->time != event_pntr ) {
      roach( "Event_Update: Event with wrong time." );
      roach( "-- Owner = %s", event->owner );  
      panic( "--  Time = %d", event->time );
    }

    event_queue[ event_pntr ] = event->loop;
    event->time               = -1;
    event->loop               = 0;

    ( *event->func )( event );
  }

  event_pntr = (event_pntr+1)%QUEUE_LENGTH;

  pulse_time[ TIME_EVENT ] = stop_clock( start );
}
