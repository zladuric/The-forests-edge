#include <ctype.h>
#include <sys/types.h>
#include <stdio.h>
#include <syslog.h>
#include "define.h"
#include "struct.h"


const index_data cold_index [] = {
  { "has no effect on",   "have no effect on",   0 },
  { "cools",              "cool",                7 },
  { "chills",             "chill",              15 },
  { "numbs",              "numbs",              35 },
  { "blisters",           "blister",            50 },
  { "FROSTS",             "FROST",              75 },
  { "FREEZES",            "FREEZE",            100 },
  { "* STIFFENS *",       "* STIFFEN *",       150 },
  { "* HARDENS *",        "* HARDEN *",        200 },
  { "** ICES **",         "** ICE **",         275 },
  { "** SOLIDIFIES **",   "** SOLIDIFY **",    350 },
  { "*** PETRIFIES ***",  "*** PETRIFY ***",    -1 }
};


bool damage_cold( char_data* victim, char_data* ch, int damage,
		  const char* string, bool plural,
		  const char *die )
{
  if( victim->is_affected( AFF_SANCTUARY ) )
    damage = 0;
  else
    add_percent_average( damage, -victim->Save_Cold( ) );

  dam_message( victim, ch, damage, string,
	       lookup( cold_index, damage, plural ) );
  
  return inflict( victim, ch, damage, die );
}


int Obj_Data :: vs_cold( )
{
  int save = 100;

  for( int i = 0; i < table_max[TABLE_MATERIAL]; i++ ) 
    if( is_set( pIndexData->materials, i ) )
      save = min( save, material_table[i].save_cold );

  if( pIndexData->item_type != ITEM_ARMOR 
      || pIndexData->item_type != ITEM_WEAPON ) 
    return save;

  return save+value[0]*(100-save)/(value[0]+2);
}


/*
 *   COLD SPELLS
 */


/*
bool spell_resist_cold( char_data* ch, char_data* victim, void*,
			int level, int duration )
{
  return spell_affect( ch, victim, level, duration,
		       SPELL_RESIST_COLD, AFF_RESIST_COLD );
}
*/


/*
bool spell_chilling_touch( char_data* ch, char_data* victim, void*,
  int level, int )
{
  damage_cold( victim, ch, spell_damage( SPELL_CHILLING_TOUCH, level ),
	       "*the touch of ice" );
  
  return true; 
}


bool spell_freeze( char_data* ch, char_data* victim, void*, int level, int )
{
  damage_cold( victim, ch, spell_damage( SPELL_FREEZE, level ),
	       "*the sphere of cold" );
  
  return true; 
}


bool spell_ice_storm( char_data* ch, char_data* victim, void*, int level,
  int )
{
  damage_cold( victim, ch, spell_damage( SPELL_ICE_STORM, level ),
	       "*the icy blast" );

  return true; 
}

bool spell_ice_lance( char_data* ch, char_data* victim, void*, int level,
  int )
{
  damage_cold( victim, ch, spell_damage( SPELL_ICE_LANCE, level ),
	       "*the frigid pierce" );
  
  return true;
}
*/


bool spell_ice_shield( char_data* ch, char_data* victim, void*,
		       int level, int duration )
{
  //  strip_affect( victim, AFF_INVISIBLE );
  leave_shadows( victim );

  if( is_submerged( victim ) ) {
    send( victim, "The water around you cools noticeably.\n\r" );
    send_seen( victim, "Nothing happens." );
    return false;
  }
  
  if( victim->is_affected( AFF_FIRE_SHIELD ) ) {
    send( victim, "A cloud of steam surrounds you, but quickly dissipates.\n\r" );
    fsend_seen( victim, "A cloud of steam surrounds %s, but quickly dissipates.", victim );
    return false;
  }

  return spell_affect( ch, victim, level, duration,
		       SPELL_ICE_SHIELD, AFF_ICE_SHIELD );
}
