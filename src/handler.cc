#include <sys/types.h>
#include <stdio.h>
#include <ctype.h>
#include "define.h"
#include "struct.h"


/*
 *   LIST GENERATING FUNCTIONS
 */


int get_trust( const char_data *ch )
{
  if( !ch )
    return 0;

  if( !ch->pcdata )
    return LEVEL_HERO - 1;

  if( ch->pcdata->trust != 0 )
    return ch->pcdata->trust;

  return ch->Level();
}


/*
 *   WEIRD ROUTINES
 */


player_data *rand_player( room_data *room, char_data *ch )
{
  player_array potential_targets;

  // Build a set of _all_ players in the room.  Seen, unseen, you name it!
  for( int j = 0; j < room->contents.size; j++ ) {
     player_data *a_player = player( room->contents[j] );
     if ( a_player
	  && invis_level( a_player ) < LEVEL_BUILDER
	  && a_player != ch
	  && ( !ch || a_player->Seen( ch ) )
	  && a_player->shdata ) {
       potential_targets += a_player;
     }
  }
  
  if( potential_targets.size == 0 )
    return 0;

  // Pick a player at random from that set.
  return potential_targets[ number_range( 0, potential_targets.size - 1 ) ];
}


/*
char_data *rand_victim( char_data *ch )
{
  char_data*  rch;
  char_array potential_targets;

  if ( !ch->array )
     return 0;
  
  // Count # people that ch sees in the room, or that is fighting ch.
  for( int j = 0; j < ch->array->size; j++ ) {
    rch = character( ch->array->list[j] ); 
    if ( rch
	 && rch != ch
	 && ( rch->Seen( ch ) || rch->fighting == ch )
	 ) {
      potential_targets += rch;
    }
  }
  
  if( potential_targets.size == 0 )
    return 0;
  
  // Pick a character at random from that set.
  return potential_targets[ number_range( 0, potential_targets.size - 1 ) ];
}
*/
