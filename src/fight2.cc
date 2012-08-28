#include <sys/types.h>
#include <stdio.h>
#include <syslog.h>
#include "define.h"
#include "struct.h"


/*
 *   SUPPORT SUBROUTINES
 */


char_data *get_victim( char_data* ch, const char *argument, const char *msg )
{
  char_data*    victim;
  char_data*  opponent = ch->fighting;

  if( !*argument ) {
    if( opponent )
      opponent->Select( 1 );
    return opponent;
  }

  if( !( victim = one_character( ch, argument, msg, ch->array ) ) ) {
    return 0;
  }

  if( opponent && opponent != victim ) {
    send( ch, "You are already fighting someone else.\n\r" );
    return 0;
  }
  
  return victim;
}


/*
 *   DO_KILL FUNCTION
 */


void do_kill( char_data* ch, const char *argument )
{
  char_data*    victim;

  if( !( victim = one_character( ch, argument, "kill", ch->array ) ) )
    return;
  
  if( is_entangled( ch, "attack" ) ) {
    return;
  }

  if( victim == ch ) {
    send( ch, "Typing quit is easier.\n\r" );
    return;
  }
  
  obj_data *wield, *secondary, *shield;
  get_wield( ch, wield, secondary, shield );

  if( ch->mount && !wield ) {
    send( ch, "You can't attack without a weapon while mounted.\n\r" );
    return;
  }
  
  if( ch->fighting == victim ) {
    fsend( ch, "You are already attacking %s!", victim );
    return;
  }
  
  if( !can_kill( ch, victim ) )
    return;

  if( !set_fighting( ch, victim ) )
    return;
  
  remove_bit( ch->status, STAT_WIMPY );
  strip_affect( ch, AFF_INVISIBLE );
  leave_shadows( ch );

  fight_round( ch );
}


/*
 *   DISARM
 */


bool can_disarm( char_data *ch, char_data *victim, obj_data *wield, bool msg )
{
  if( is_set( wield->extra_flags, OFLAG_NO_DISARM )
      || !wield->removable( 0 ) ) { 
    if( msg )
      fsend( ch, "%s wielded by %s is impossible to disarm.",
	     wield, victim->Name( ch ) );
    return false;
  }
  
  return true;
}


void do_disarm( char_data* ch, const char * )
{
  if( is_confused_pet( ch ) )
    return;

  int skill = ch->get_skill( SKILL_DISARM );

  if( ch->pcdata 
      && skill == UNLEARNT ) {
    send( ch, "You don't know how to disarm opponents.\n\r" );
    return;
  }
  
  if( not_fighting( ch, "disarm someone" )
      || is_entangled( ch, "disarm someone" ) ) {
    return;
  }

  obj_data *wield, *secondary, *shield;
  get_wield( ch, wield, secondary, shield );

  if( !wield
      && secondary
      && ch->get_skill( SKILL_OFFHAND_ATTACK ) != UNLEARNT ) {
    wield = secondary;
  }

  if( !wield ) {
    send( ch, "You must wield a weapon to disarm.\n\r" );
    return;
  }

  char_data *victim = ch->fighting;
  victim->Select( 1 );

  get_wield( victim, wield, secondary, shield );

  if( !wield ) {
    if( obj_data *obj = victim->Wearing( WEAR_HELD_R, LAYER_BASE ) ) {
      if( obj->pIndexData->item_type == ITEM_GARROTE
	  || obj->pIndexData->item_type == ITEM_STAFF
	  || obj->pIndexData->item_type == ITEM_WAND ) {
	wield = obj;
      }
    }
  }

  if( !secondary ) {
    if( obj_data *obj = victim->Wearing( WEAR_HELD_L, LAYER_BASE ) ) {
      if( obj->pIndexData->item_type == ITEM_STAFF
	  || obj->pIndexData->item_type == ITEM_WAND ) {
	secondary = obj;
      }
    }
  }

  if( !wield || secondary && number_range( 1, 4 ) == 1 )
    wield = secondary;

  if( skill == 10 &&
      ( !wield || shield && number_range( 1, 5 ) == 1 ) )
    wield = shield;

  if( !wield ) {
    if( !secondary && !shield ) {
      if( skill == 10 ) {
	send( ch, "Your opponent is not wielding a weapon or shield.\n\r" );
      } else {
	send( ch, "Your opponent is not wielding a weapon.\n\r" );
      }
    } else {
      fsend( ch, "You fail to disarm %s.", victim );
    }
    return;
  }
  
  const int dex = ch->Dexterity();

  if( !can_disarm( ch, victim, wield, true ) ) {
    set_delay( ch, 22 - ch->get_skill( SKILL_DISARM )/2 - dex/5 );
    return;
  }

  if( victim->position >= POS_SLEEPING ) {
    int fail = is_set( victim->status, STAT_TWO_HAND ) ? -70 : -60;
    
    if( !victim->Seen( ch ) ) {
      fail -= 20;
    }
    
    if( !ch->check_skill( SKILL_DISARM, fail + 4*( dex - victim->Dexterity() ) ) ) {
      fsend( ch, "You fail to disarm %s.", victim );
      spam_char( victim, "%s tries to disarm you, but fails.", ch );
      spam_room( ch, "%s tries to disarm %s, but fails.", ch, victim );
      set_delay( ch, 22 - ch->get_skill( SKILL_DISARM )/2 - dex/5 );
      return;
    } 
  }

  char *tmp = static_string( );
  strcpy( tmp, ch->Name( victim ) );
  capitalize( tmp );
    
  fsend_color( victim, COLOR_SKILL, "+++ %s disarms your %s! +++",
	       tmp, wield->Name( victim, 1, true ) );
  fsend_color( ch, COLOR_SKILL, "You disarm %s, sending %s flying!",
	       victim, wield );
  fsend_color_seen( victim, COLOR_SKILL, "%s disarms %s, sending %s flying!",
		    ch, victim, wield );
  
  if( ( wield = object( wield->From( 1 ) ) ) ) {
    wield->To( victim );
  }

  ch->improve_skill( SKILL_DISARM );

  record_damage( victim, ch );

  set_delay( ch, 32 - ch->get_skill( SKILL_DISARM ) - dex/4 );
}


/*
 *   KICK, PUNCH, BITE ROUTINES
 */


void do_punch( char_data* ch, const char *argument )
{
  char_data*     victim;

  if( is_confused_pet( ch )
      || !is_humanoid( ch )
      || is_mounted( ch, "punch" )
      || is_entangled( ch, "punch" ) ) 
    return;

  
  if( !*argument ) {
    if ( !( victim = ch->fighting ) ) {
      send( ch, "Who do you want to punch?\n\r" );
      return;
    }
    victim->Select( 1 );
  } else if( !( victim = get_victim( ch, argument, "punch" ) ) ) {
    return;
  }
  
  if( victim == ch ) {
    send( ch, "Punching yourself is pointless.\n\r" );
    return;
  }

  /*
  if( ch->pcdata && ch->get_skill( SKILL_PUNCH ) == UNLEARNT ) {
    send( ch, "You are untrained in the art of punching.\n\r" );
    return;
  }
  */

  if( !can_kill( ch, victim ) )
    return;

  if( !set_fighting( ch, victim ) )
    return;

  remove_bit( ch->status, STAT_WIMPY );
  strip_affect( ch, AFF_INVISIBLE );
  leave_shadows( ch );

  const bool claw = uses_claws( ch );
  const int skill = ch->species ? 5 : ch->get_skill( SKILL_PUNCH );
  const int mod = 4*skill - 20;
  const char *noun = claw ? "claw" : attack_noun( 0 );
  const char *verb = claw ? "claw" : attack_verb( 0 );
  const int die = claw ? 6 : 4;
  const int plus = skill/3;

  attack( ch, victim, noun, verb, 0, roll_dice( 1,die )+plus, mod );

  ch->improve_skill( SKILL_PUNCH );
  
  set_delay( ch, 20 );
}


void do_bite( char_data* ch, const char *argument )
{
  if( is_confused_pet( ch )
      || is_mounted( ch, "bite" )
      || is_entangled( ch, "bite" ) ) 
    return;

  char_data *victim;

  if( !*argument ) {
    if( !( victim = ch->fighting ) ) {
      send( ch, "Who do you want to bite?\n\r" );
      return;
    }
    victim->Select( 1 )
;
  } else if( !( victim = get_victim( ch, argument, "bite" ) ) ) {
    return;
  }
  
  if( victim == ch ) {
    send( ch, "Biting yourself is pointless.\n\r" );
    return;
  }
  
  if( ch->shdata->race != RACE_LIZARD
      && ch->shdata->race != RACE_TROLL
      && ch->shdata->race != RACE_GOBLIN ) {
    send( ch, "Your teeth are not long or sharp enough for biting to be an effective attack.\n\r" );
    return;
  }
  
  if( !can_kill( ch, victim ) )
    return;

  if( !set_fighting( ch, victim ) )
    return;

  remove_bit( ch->status, STAT_WIMPY );
  strip_affect( ch, AFF_INVISIBLE );
  leave_shadows( ch );

  attack( ch, victim, "bite", 0, 0, roll_dice( 2, 4 ), 0 );

  set_delay( ch, 20 );
}


/*
 *   SPIN KICK ROUTINE
 */



void do_spin_kick( char_data* ch, const char *arg )
{
  char_data*       rch;
  bool           found  = false;

  if( is_confused_pet( ch )
      || is_mounted( ch, "spin kick" )
      || is_entangled( ch, "spin kick" )
      || is_drowning( ch, "spin kick" )
      || !is_humanoid( ch ) )
    return;

  if( ch->get_skill( SKILL_SPIN_KICK ) == UNLEARNT ) {
    send( ch, "You are untrained in that skill.\n\r" );
    return;
  }
  
  for( int i = 0; i < ch->array->size; i++ ) {
    if( ( rch = character( ch->array->list[i] ) )
	&& ( rch->fighting == ch || ch->fighting == rch ) ) {
      if( !found ) {
	found = true;
	remove_bit( ch->status, STAT_WIMPY );
	strip_affect( ch, AFF_INVISIBLE );
	leave_shadows( ch );
	send( ch, "You leap in the air, spinning rapidly.\n\r" );
	fsend_seen( ch,
		    "%s leaps into the air, becoming a deadly blur.", 
		    ch );
      }
      attack( ch, rch, "spin kick", 0, 0,
	      roll_dice( 5, 10 + ch->get_skill( SKILL_SPIN_KICK ) ), 0);
    }
  }
  
  if( !found ) {
    send( ch, "You are not fighting anyone!\n\r" );
    return;
  }
  
  ch->improve_skill( SKILL_SPIN_KICK );
  set_delay( ch, 25 );
}


void do_melee( char_data* ch, const char *arg )
{
  if( is_mounted( ch, "melee" )
      || is_ridden( ch, "melee" )
      || is_entangled( ch, "melee" )
      || is_drowning( ch, "melee" )
      || not_fighting( ch, "melee" ) )
    return;
  
  if( ch->get_skill( SKILL_MELEE ) == UNLEARNT ) {
    send( ch, "You are untrained in that skill.\n\r" );
    return;
  }
  
  bool found = false;
  
  for( int i = 0; i < ch->array->size; i++ ) {
    if( char_data *rch = character( ch->array->list[i] ) ) {
      if( !rch->fighting
	  || !can_kill( ch, rch, false ) )
	continue;
      if( rch->fighting == ch
	  || ch->fighting == rch
	  || rch->fighting->leader == ch
	  || is_same_group( rch->fighting, ch ) && !is_same_group( rch, ch ) ) {
	if( !found ) {
	  found = true;
	  remove_bit( ch->status, STAT_WIMPY );
	  strip_affect( ch, AFF_INVISIBLE );
	  leave_shadows( ch );
	  send_color( ch, COLOR_SKILL, "You spin about, striking at all your foes!" );
	  send( ch, "\n\r" );
	  fsend_color_seen( ch, COLOR_SKILL, "%s spins, striking out at all %s foes!",
			    ch, ch->His_Her( ) );
	}
	attack( ch, rch, "MELEE", 0, 0,
		roll_dice( 5, 8+ch->get_skill(SKILL_MELEE) ), 0 );
      }
    }
  }
  
  if( !found ) {
    send( ch, "No melee opponents found.\n\r" );
    return;
  }

  ch->improve_skill( SKILL_MELEE );
  set_delay( ch, 32 );
}
