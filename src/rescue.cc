#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include "define.h"
#include "struct.h"


/*
 *   ASSIST ROUTINE
 */


void do_assist( char_data* ch, const char *argument )
{
  char_data*    victim;
  char_data*  opponent;

  if( !( victim = one_character( ch, argument, "assist", ch->array ) ) )
    return;
  
  if( is_entangled( ch, "assist someone" ) ) {
    return;
  }

  if( victim == ch ) {
    send( ch, "You can't assist yourself.\n\r" );
    return;
  }
  
  if( !( opponent = victim->fighting ) ) {
    fsend( ch, "%s isn't fighting anyone.", victim );
    return;
  }
  
  if( victim->species ) {
    send( ch, "You can only assist players.\n\r" );
    return;
  }
  
  if( opponent == ch ) {
    send( ch, "Perhaps you should let the fight finish on its own.\n\r" );
    return;
  }
  
  if( is_fighting( ch, "assist someone" ) ) {
    return;
  }
  
  if( !can_kill( ch, opponent ) )
    return;
  
  if( !set_fighting( ch, opponent ) )
    return;
  
  remove_bit( ch->status, STAT_WIMPY );
  strip_affect( ch, AFF_INVISIBLE );
  leave_shadows( ch );
  
  fight_round( ch );
}


/*
 *   RESCUE ROUTINE
 */


void do_rescue( char_data* ch, const char *argument )
{
  if( is_confused( ch, "rescue" ) ) {
    return;
  }

  char_data*  victim;

  if( !( victim = one_character( ch, argument, "rescue", ch->array ) ) )
    return;

  if( victim == ch ) {
    send( ch, "What about fleeing instead?\n\r" );
    return;
  }

  if( ch->pcdata
      && victim->species
      && !is_set( victim->status, STAT_PET ) ) { 
    send( ch, "You are unable to rescue monsters.\n\r" );
    return;
  }
  
  if( ch->fighting == victim ) {
    send( ch, "You can't rescue someone you are attacking!\n\r" );
    return;
  }
  
  if( victim->fighting == ch ) {
    send( ch, "You can't rescue someone who is attacking you!\n\r" );
    return;
  }
  
  char_data*     rch;
  char_array    list;

  for( int i = 0; i < victim->in_room->contents; i++ ) 
    if( ( rch = character( victim->in_room->contents[i] ) )
	&& rch->fighting == victim ) {
      if( !can_kill( ch, rch ) ) {
        return;
      }
      list += rch;
    }

  if( list.is_empty() ) {
    fsend( ch, "No one is attacking %s.", victim );
    return;
  }
  
  if( ch->get_skill( SKILL_RESCUE ) == UNLEARNT ) {
    fsend( ch, "You fail to rescue %s.", victim );
    set_delay( ch, 5 );
    return;
  }

  if( !ch->check_skill( SKILL_RESCUE, 2*ch->Dexterity() - 20 ) ) {
    fsend( ch, "You fail to rescue %s.", victim );
    if( ch->Seen( victim ) ) {
      fsend( victim, "%s attempts to rescue you, but fails.", ch );
    }
    set_delay( ch, 15 );
    return;
  }

  char *buf = static_string( );
  strcpy( buf, ch->Name( victim ) );
  capitalize( buf );

  fsend_color( ch, COLOR_SKILL, "You rescue %s!", victim );
  fsend_color( victim, COLOR_SKILL, "++ %s rescues you! ++", buf );
  fsend_color_seen( ch, COLOR_SKILL, "%s rescues %s!", ch, victim );
  
  ch->improve_skill( SKILL_RESCUE );

  if( victim->fighting ) {
    if( !set_fighting( ch, victim->fighting ) )
      return;
    //    remove_bit( victim->status, STAT_BERSERK );
    //    remove_bit( victim->status, STAT_FOCUS );
    set_fighting( victim, 0 );
  } else {
    if( !set_fighting( ch, list[0] ) )
      return;
  }

  for( int i = 0; i < list; i++ ) {
    set_fighting( list[i], ch );
  }
  
  remove_bit( ch->status, STAT_WIMPY );
  strip_affect( ch, AFF_INVISIBLE );
  leave_shadows( ch );
  
  set_delay( ch, 15 );
}


void do_berserk( char_data* ch, const char *) 
{
  if( not_fighting( ch, "go berserk" )
      || is_confused( ch, "go berserk" )
      || is_parrying( ch, "go berserk" ) )
    return;

  if( is_set( ch->status, STAT_BERSERK ) ) {
    send( ch, "You are already in a frenzy beyond control.\n\r" );
    return;
  }
  
  if( ch->get_skill( SKILL_BERSERK ) == UNLEARNT ) {
    send( ch, "You don't know how to incite yourself into a berserk rage.\n\r" );
    return;
  }

  if( is_silenced( ch ) ) {
    send_color( ch, COLOR_SKILL,
		"You attack with renewed fury!" );
    send( ch, "\n\r" );
    fsend_color( *ch->array, COLOR_SKILL,
		 "%s attacks with renewed fury!", ch );
  } else {
    send_color( ch, COLOR_SKILL,
		"You scream a cry of war, and attack with renewed fury!" );
    send( ch, "\n\r" );
    fsend_color( *ch->array, COLOR_SKILL,
		 "%s screams a cry of war, and attacks with renewed fury!", ch );
  }

  set_bit( ch->status, STAT_BERSERK ); 
  ch->improve_skill( SKILL_BERSERK );  
}


void do_focus( char_data* ch, const char *) 
{
  if( not_fighting( ch, "focus" )
      || is_confused( ch, "focus" )
      || is_parrying( ch, "focus" ) )
    return;

  if( is_set( ch->status, STAT_FOCUS ) ) {
    send( ch, "You are already focused on the battle.\n\r" );
    return;
  }
  
  if( ch->get_skill( SKILL_FOCUS ) == UNLEARNT ) {
    send( ch, "You don't know how to focus your battle skills.\n\r" );
    return;
  }

  if( ch->mana < 10 ) {
    send( ch, "You don't have enough energy to focus your battle skills.\n\r" );
    return;
  }

  send_color( ch, COLOR_SKILL,
	      "Your eyes narrow and your battle skills sharpen as you focus on the melee." );
  send( ch, "\n\r" );
  fsend_color( *ch->array, COLOR_SKILL,
	       "%s's eyes narrow as %s strikes with superior precision!",
	       ch, ch->He_She( ) );

  set_bit( ch->status, STAT_FOCUS ); 
  ch->improve_skill( SKILL_FOCUS );  
}
