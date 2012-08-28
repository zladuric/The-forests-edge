#include <ctype.h>
#include <sys/types.h>
#include <stdio.h>
#include <syslog.h>
#include "define.h"
#include "struct.h"


/*
 *   TRACK_DATA CLASS
 */

track_data :: track_data( )
{
  record_new( sizeof( track_data ), MEM_TRACK );
}


track_data :: ~track_data( )
{
  record_delete( sizeof( track_data ), MEM_TRACK );
}


/*
 *  DO_TRACK ROUTINE
 */


void do_track( char_data* ch, const char * )
{
  if( is_mob( ch ) )
    return;

  if( is_set( ch->pcdata->pfile->flags, PLR_TRACK ) ) {
    remove_bit( ch->pcdata->pfile->flags, PLR_TRACK );
    send( ch, "You stop tracking.\n\r" );
    return;
  }

  if( is_mounted( ch, "track" ) )
    return;

  if( ch->get_skill( SKILL_TRACK ) == UNLEARNT ) {
    send( ch, "You don't know how to track.\n\r" );
    return;
  }
  
  set_bit( ch->pcdata->pfile->flags, PLR_TRACK );
  
  send( ch, "You are now tracking.\n\r" );
  send( ch, "[Tracking increases movement cost by 2 points.]\n\r" );
}


/*
 *   MAKE TRACK ROUTINE
 */


bool can_track( room_data *room )
{
  return is_set( terrain_table[ room->sector_type ].flags, TFLAG_TRACKS );
    /*
      !water_logged( room )
      && !midair( 0, room );
    */
}


void make_tracks( char_data* ch, room_data* room, int door )
{
  if( ch->mount )
    ch = ch->mount;

  if( ch->can_fly( )
      || ch->can_float()
      || !can_track( room )
      || invis_level( ch ) >= LEVEL_BUILDER ) 
    return;

  track_data *track  = new track_data;
  track->decay_time  = current_time+number_range( 300,1000 );
  track->to_dir      = door;
  track->race        = ch->shdata->race;   
  track->next        = room->track;
  room->track        = track;

  track_data *next;

  for( track_data *prev = 0; track; track = next ) {
    next = track->next;
    if( ( track->decay_time -= 150 ) < current_time ) {
      if( !prev )
        room->track = next;
      else
        prev->next = next;
      delete track;
    } else
      prev = track;
  }
}


/*
 *   SHOW TRACK ROUTINE
 */


void show_tracks( char_data *ch )
{
  room_data *room = ch->in_room;

  if( ch->species
      || !room->track
      || !is_set( ch->pcdata->pfile->flags, PLR_TRACK ) 
      || !room->Seen( ch ) ) 
    return;
  
  const int num = count( room->track );

  int track_race [ num ];
  int track_num [ num ];
  int track_dir [ num ];

  vzero( track_num, num );

  const int level = ch->get_skill( SKILL_TRACK );
  const int now = current_time;
  const int time1 = now + 500 - 50*level;
  const int time2 = now + 750 - 75*level;
  bool found = false;

  for( track_data *track = room->track; track; track = track->next ) {
    if( track->decay_time > time1 ) {
      int race;
      if( track->decay_time > time2 ) {
	race = track->race;
      } else {
	race = -1;
      }
      for( int i = 0; i < num; ++i ) {
	if( track_num[i] == 0 ) {
	  track_race[i] = race;
	  track_num[i] = 1;
	  track_dir[i] = track->to_dir;
	  break;
	} else if( track_race[i] == race
		   && track_dir[i] == track->to_dir ) {
	  ++track_num[i];
	  break;
	}
      }
      found = true;
    }
  }

  for( int i = 0; i < num; ++i ) {
    if( track_num[i] == 0 )
      break;
    if( i == 0 ) {
      send( ch, "\n\r" );
    }
    if( track_num[i] == 1 ) {
      fsend( ch, "You see %s tracks heading %s.",
	    track_race[i] < 0 ? "unknown" : race_table[ track_race[i] ].name,
	    dir_table[ track_dir[i] ].name );
    } else {
      send( ch, "You see %s sets of %s tracks heading %s.\n\r",
	    number_word( track_num[i], ch ),
	    track_race[i] < 0 ? "unknown" : race_table[ track_race[i] ].name,
	    dir_table[ track_dir[i] ].name );
    }
  }

  if( found )
    ch->improve_skill( SKILL_TRACK );
}
