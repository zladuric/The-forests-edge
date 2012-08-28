#include <ctype.h>
#include <sys/types.h>
#include <stdio.h>
#include <syslog.h>
#include "define.h"
#include "struct.h"


const index_data shock_index [] = 
{
  { "has no effect on",    "have no effect on",   0 },
  { "tingles",         	   "tingle",              7 },
  { "zaps",            	   "zap",           	 15 },
  { "charges",         	   "charge",        	 30 },
  { "jolts",           	   "jolt",          	 50 },
  { "SHOCKS",          	   "SHOCK",         	 75 },
  { "ELECTRIFIES",     	   "ELECTRIFY",     	100 },
  { "* ELECTROCUTES *",	   "* ELECTROCUTE *",   140 },
  { "* IONIZES *",     	   "* IONIZE *",    	200 },
  { "** ATOMIZES **",  	   "** ATOMIZE **",     275 },
  { "** GALVANIZES **",    "** GALVANIZE **",   350 },
  { "*** NEBULIZES ***",   "*** NEBULIZE ***",   -1 }
};


void water_shock( char_data *ch, char_data *victim, int damage )
{
  room_data *room = victim->in_room;

  for( int i = room->contents.size - 1; i >= 0; --i ) {
    char_data *rch;
    if( ( rch = character( room->contents[i] ) )
	//	&& ( ch == rch
	//	     || can_kill( ch, rch, false ) )
	&& is_submerged( rch ) ) {
      damage_shock( rch, ch, damage,
		    "*The water shock" );
      if( can_kill( ch, rch, false ) ) {
	react_attack( ch, rch );
      }
    }
  }
}


bool damage_shock( char_data* victim, char_data* ch, int damage,
		   const char* string, bool plural,
		   const char *die )
{
  if( victim->is_affected( AFF_SANCTUARY ) )
    damage = 0;
  else
    add_percent_average( damage, -victim->Save_Shock( ) );
  
  dam_message( victim, ch, damage, string,
	       lookup( shock_index, damage, plural ) );  

  return inflict( victim, ch, damage, die );
}


/* 
 *   ELECTRICITY BASED SPELLS
 */

/*

bool spell_resist_shock( char_data* ch, char_data* victim, void*,
			 int level, int duration )
{
  return spell_affect( ch, victim, level, duration,
		       SPELL_RESIST_SHOCK, AFF_RESIST_SHOCK );
}
*/


bool spell_ion_shield( char_data* ch, char_data* victim, void*,
		       int level, int duration )
{
  //  strip_affect( victim, AFF_INVISIBLE );
  leave_shadows( victim );

  if( is_submerged( victim ) ) {
    send( *victim->array, "The water prevents the shield of sparks from forming.\n\r" );
    water_shock( ch, victim, 2*spell_damage( SPELL_SHOCK, level ) );
    return false;
  }

  return spell_affect( ch, victim, level, duration,
		       SPELL_ION_SHIELD, AFF_ION_SHIELD );
}


/*
bool spell_call_lightning( char_data* ch, char_data* victim, void*,
			   int level, int )
{
  if( is_submerged( 0, victim->in_room ) ) {
    water_shock( ch, victim, SPELL_CALL_LIGHTNING, level );
    return false;
  }
  
  damage_shock( victim, ch, spell_damage( SPELL_CALL_LIGHTNING, level ),
		"*A bolt of lightning" );
  return true;
}


bool spell_lightning_bolt( char_data* ch, char_data* victim, void*,
			   int level, int )
{
  if( is_submerged( 0, victim->in_room ) ) {
    water_shock( ch, victim, SPELL_LIGHTNING_BOLT, level );
    return false;
  }

  damage_shock( victim, ch, spell_damage( SPELL_LIGHTNING_BOLT, level ),
		"*The brilliant bolt of lightning" );

  return true;
}
*/


bool spell_arc_lightning( char_data *ch, char_data *victim, void*,
			  int level, int )
{
  room_data *room = victim->in_room;
  const int dam = spell_damage( SPELL_ARC_LIGHTNING, level );

  if( is_submerged( 0, room ) ) {
    water_shock( ch, victim, 2*dam );
    return false;
  }

  while( victim ) {
    damage_shock( victim, ch, dam,
		  "*The lethal arcs of lightning", true );
    if( number_range( 0, 9 ) != 0
	|| !( victim = random_pers( room ) )
	|| victim == ch 
	|| !can_kill( ch, victim, false ) )
      break;
  }

  return true;
}


bool spell_chain_lightning( char_data* ch, char_data* victim, void*,
			    int level, int )
{
  room_data *room = victim->in_room;
  const int dam = spell_damage( SPELL_CHAIN_LIGHTNING, level );

  if( is_submerged( 0, room ) ) {
    water_shock( ch, victim, 2*dam );
    return false;
  }
 
  while( victim ) {
    damage_shock( victim, ch, dam,
		  "*The bifurcating lightning bolt" );
    if( number_range( 0, 3 ) == 0
	|| !( victim = random_pers( room ) )
	|| victim == ch 
	|| !can_kill( ch, victim, false ) )
      break;
  }

  return true;
}


/*
bool spell_shock( char_data* ch, char_data* victim, void*,
		  int level, int )
{
  if( is_submerged( 0, victim->in_room ) ) {
    water_shock( ch, victim, SPELL_SHOCK, level );
    return false;
  }
  
  damage_shock( victim, ch, spell_damage( SPELL_SHOCK, level ),
		"*The blue arcs of energy", true );

  return true;
}
*/
