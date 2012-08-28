#include <sys/types.h>
#include <stdio.h>
#include <syslog.h>
#include "define.h"
#include "struct.h"


void do_kick( char_data* ch, const char *argument )
{
  if( is_confused_pet( ch )
      || !is_humanoid( ch )
      || is_mounted( ch, "kick" )
      || is_entangled( ch, "kick" ) )
    return;
  
  char_data *victim;

  if( !*argument ) {
    if ( !( victim = ch->fighting ) ) {
      send( ch, "Who do you want to kick?\n\r" );
      return;
    }
    victim->Select( 1 );

  } else if( !( victim = get_victim( ch, argument, "kick" ) ) ) {
    return;
  }

  if( victim == ch ) {
    send( ch, "Kicking yourself is pointless.\n\r" );
    return;
  }

  /*
  if( ch->pcdata && ch->get_skill( SKILL_KICK ) == UNLEARNT ) {
    send( ch, "You are untrained in the art of kicking.\n\r" );
    return;
  }
  */

  if( victim->mount ) {
    send( ch, "You can't kick a mounted person.\n\r" );
    return;
  }

  if( !can_kill( ch, victim ) )
    return;

  if( !set_fighting( ch, victim ) )
    return;

  remove_bit( ch->status, STAT_WIMPY );
  strip_affect( ch, AFF_INVISIBLE );
  leave_shadows( ch );

  const int skill = ch->species ? 5 : ch->get_skill( SKILL_KICK );
  const int mod = 4*skill - 20;
  const int die = ( skill + 3 ) / 2;  // 1 at level 0, 6 at level 10.

  attack( ch, victim, "kick", 0, 0, roll_dice( 2,die ), mod );

  ch->improve_skill( SKILL_KICK );

  set_delay( ch, 20 );
}



void do_charge( char_data* ch, const char *argument )
{
  if( is_mob( ch ) )
    return;

  if( !*argument ) {
    send( ch, "Charge whom?\n\r" );
    return;
  }

  char_data*  victim;
  if( !( victim = get_victim( ch, argument, "charge" ) ) ) 
    return;
  
  charge( ch, victim );
}


bool charge( char_data *ch, char_data *victim )
{
  int skill = ch->get_skill( SKILL_CHARGE );

  if( skill == UNLEARNT ) {
    send( ch, "Charging is not part of your repertoire.\n\r" );
    return false;
  }

  if( is_mounted( ch, "charge" )
      || is_entangled( ch, "charge" )
      || is_fighting( ch, "charge" )
      || is_drowning( ch, "charge" ) ) {
    return false;
  }
  
  if( victim == ch ) {
    send( ch, "How can you charge against yourself?\n\r" );
    return false;
  }
  
  obj_data *wield, *secondary, *shield;
  get_wield( ch, wield, secondary, shield );

  if( !wield ) {
    send( ch, "You must wield a weapon to charge at someone.\n\r" );
    return false;
  }

  if( !can_kill( ch, victim ) )
    return false;
  
  if( !set_fighting( ch, victim ) )
    return false;
    
  remove_bit( ch->status, STAT_WIMPY );
  strip_affect( ch, AFF_INVISIBLE );
  leave_shadows( ch );
  
  int roll = number_range( 1, 20 )
    +  ch->get_skill( SKILL_CHARGE )
    + ( ch->Dexterity() - victim->Dexterity() );

  if( roll < 12  ) {
    fsend( ch, "You attempt to charge %s, but miss and fall down.",
	   victim );
    fsend( victim,
	  "%s attempts to charge you, but %s misses and falls down.",
	   ch, ch->He_She( victim ) );
    fsend_seen( ch,
		"%s attempts to charge %s, but %s misses and falls down.",
		ch, victim, ch->He_She( ) );
    ch->position = POS_RESTING;
    set_delay( ch, 20 );
  } else if( attack( ch, victim, "charge", 0, wield, -1, 5*(skill-5), ATT_CHARGE ) ) {
    ch->improve_skill( SKILL_CHARGE );
    set_delay( ch, 10 );
  } else {
    fsend( ch, "You charge at %s, but %s moves out of the way.",
	   victim, victim->He_She( ) );
    fsend( victim,
	  "%s charges at you, but you move out of %s way.",
	   ch, ch->His_Her( victim ) );
    fsend_seen( ch,
		"%s charges at %s, but %s moves out of the way.",
		ch, victim, victim->He_She( ) );
    set_delay( ch, 15 );
  }

  return true;
}
