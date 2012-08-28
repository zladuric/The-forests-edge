#include <ctype.h>
#include <sys/types.h>
#include <stdio.h>
#include <syslog.h>
#include "define.h"
#include "struct.h"


bool water_logged( const room_data* room )
{
  if( !room )
    return false;

  return is_set( terrain_table[ room->sector_type ].flags, TFLAG_WATER );
}


bool deep_water( const char_data *ch, const room_data *room )
{
  if( !room
      && !( room = ch->in_room ) )
    return false;

  const int tflags = terrain_table[ room->sector_type ].flags;

  if( !is_set( tflags, TFLAG_WATER ) )
    return false;

  if( is_set( tflags, TFLAG_DEEP ) || is_set( tflags, TFLAG_SUBMERGED ) )
    return true;

  if( ch
      && ch->Size() < SIZE_DOG )
    return true;

  return false;
}


bool is_submerged( const char_data *ch, const room_data *room )
{
  if( !room
      && !( ch && ( room = ch->in_room ) ) )
    return false;

  const int tflags = terrain_table[ room->sector_type ].flags;

  if( !is_set( tflags, TFLAG_WATER ) )
    return false;

  if( is_set( tflags, TFLAG_SUBMERGED ) )
    return true;

  if( !ch )
    return false;

  const int pos = ch->mod_position();

  if( ch->position < POS_STANDING
      || pos == POS_SWIMMING
      || pos == POS_DROWNING )
    return true;

  return false;
}


void enter_water( char_data* ch )
{
  if( strip_affect( ch, AFF_FIRE_SHIELD, false ) ) {
    fsend( ch, "Your fire shield is extinguished by the water." );
    fsend_seen( ch, "The flames surrounding %s are extinguished by the water.", ch );
  }
  
  if( strip_affect( ch, AFF_ICE_SHIELD, false ) ) {
    fsend( ch, "Your ice shield is slowed and melted by the water." );
    fsend_seen( ch, "The whirling ice crystals  surrounding %s are slowed and melted by the water.", ch );
  }
  
  if( strip_affect( ch, AFF_ION_SHIELD, false ) ) {
    fsend( ch, "Your ion shield is shorted out by the water!" );
    fsend_seen( ch, "%s's ion shield is shorted out by the water!", ch );
    if( damage_shock( ch, 0, 2*spell_damage( SPELL_SHOCK, 10 ),
		      "*The water shock" ) )
      return;
  }
}


/*
 *   WATER BASED SPELLS
 */


/*
bool spell_water_breathing( char_data* ch, char_data* victim, void*, 
			    int level, int duration )
{
  return spell_affect( ch, victim, level, duration,
		       SPELL_WATER_BREATHING, AFF_WATER_BREATHING );
}
*/
