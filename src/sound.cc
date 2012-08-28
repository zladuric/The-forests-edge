#include <ctype.h>
#include <sys/types.h>
#include <stdio.h>
#include <syslog.h>
#include "define.h"
#include "struct.h"


bool char_data :: Can_Hear ( bool msg ) const
{
  if( position <= POS_SLEEPING )
    return false;

  if( is_affected( AFF_DEAFNESS ) ) {
    if( msg ) {
      send( this, "You can't hear a thing!" );
    }
    return false;
  }

  return true;
}


/*
 *   SOUND DAMAGE ROUTINES
 */


const index_data sound_index [] = 
{
  { "has no effect on",     "have no effect on",      0 },
  { "startles",             "startle",                3 },
  { "jars",                 "jar",                    7 },
  { "jangles",              "jangle",                15 },
  { "grates",               "grate",                 30 },
  { "vibrates",             "vibrate",               50 },
  { "SHAKES",               "SHAKE",                 75 },
  { "DEAFENS",              "DEAFEN",               100 },
  { "* RESONATES *",        "* RESONATE *",         125 },
  { "* BLASTS *",           "* BLAST *",            150 },
  { "* THUNDERS *",         "* THUNDER *",          200 },
  { "** RENDS **",          "** REND **",           250 },
  { "** SHATTERS **",       "** SHATTER **",        300 },
  { "*** DETONATES ***",    "*** DETONATE ***",      -1 }
};


bool damage_sound( char_data* victim, char_data* ch, int damage,
		   const char* string, bool plural,
		   const char *die )
{
  if( victim->is_affected( AFF_SANCTUARY ) )
    damage = 0;
  else
    add_percent_average( damage, -victim->Save_Sound( ) );

  dam_message( victim, ch, damage, string,
	       lookup( sound_index, damage, plural ) );
  
  return inflict( victim, ch, damage, die );
}


/* 
 *   SOUND BASED SPELLS
 */


bool spell_deafen( char_data* ch, char_data* victim, void*,
		   int level, int duration )
{
  if( victim->shdata->race == RACE_UNDEAD ) {
    fsend( ch, "%s is not affected.", victim );
    return true;
  }
  
  if( !can_kill( ch, victim ) )
    return true;
  
  if( makes_save( victim, ch, RES_MAGIC, SPELL_DEAFEN, level ) ) {
    send( victim, "Your ears burn for a moment, but the feeling passes.\n\r" );
    return true;
  }
  
  spell_affect( ch, victim, level, duration, SPELL_DEAFEN, AFF_DEAFNESS );
  
  return true;
}
