#include <ctype.h>
#include <sys/types.h>
#include <stdio.h>
#include "define.h"
#include "struct.h"


void do_hands( char_data* ch, const char *argument )
{
  char_data*  victim;
  int             hp;
 
  if( ch->species || ch->pcdata->clss != CLSS_PALADIN ) {
    send( ch, "Only paladins can lay hands to heal.\n\r" );
    return;
  }

  int skill_hands = ch->get_skill( SKILL_LAY_HANDS );
  if( skill_hands == UNLEARNT ) {
    send( ch, "You don't know how to lay hands to heal.\n\r" );
    return;
  } 
  
  if( !*argument ) {
    victim = ch;
  } else {
    if( !( victim = one_character( ch, argument, "lay hands on",
				   ch->array ) ) )
      return;
    if( victim != ch && ch->position < POS_STANDING ) {
      cant_message( ch, "lay hands on someone else", ch->position_name( ) );
      return;
    }
  }
  
  if( is_fighting( ch, "lay hands" )
      || is_entangled( ch, "lay hands" )
      || is_drowning( ch, "lay hands" ) ) {
    return;
  }
    
  if( victim->fighting ) {
    send( ch, "You can't lay hands on someone who is fighting!\n\r" );
    return;
  }
  
  if( victim->shdata->race == RACE_UNDEAD ) {
    send( ch, "The gods will not permit the laying of hands upon their stolen property.\n\r" );
    return;
  }

  if( ch->mana < 2 ) {
    send( ch, "You don't have enough energy to do that.\n\r" );
    return;
  }
  
  if( ( hp = victim->max_hit-victim->hit ) == 0 ) {
    if( ch == victim ) 
      send( ch, "You aren't hurt.\n\r" );
    else 
      fsend( ch, "%s is not hurt.", victim );
    return;
  }

  if( victim == ch ) {
    send( ch, "You lay hands on yourself.\n\r" );
    fsend_seen( ch, "%s lays hands on %sself.",
		ch, ch->Him_Her( ) );
  } else {    
    fsend( ch, "You lay hands on %s.", victim );
    fsend( victim, "%s lays hands on you.", ch );
    fsend_seen( ch, "%s lays hands on %s.", ch, victim );
  }
  
  int skill_regen = ch->get_skill( SKILL_REGENERATION );

  int max_heal = min( hp/2, ch->mana );
  int heal = (int) ( max_heal * ( 0.5 + skill_hands*0.05 + skill_regen*0.1 ) );

  if( heal == 0 ) {
    send( ch, "Nothing happens.\n\r" );
    send_seen( ch, "Nothing happens.\n\r" );
    return;
  }

  if( hp > max_heal ) {
    ch->mana -= max_heal;
  } else {
    ch->mana -= (int) ( hp * ( skill_hands*0.05 ) * ( skill_regen*0.1 ) );
  }

  //  victim->hit += (int) ( max_heal * ( 0.5 + skill_hands*0.05 + skill_regen*0.1 ) );
  //  victim->hit = min( victim->max_hit, victim->hit );

  victim->hit = min( victim->max_hit, victim->hit + heal );

  update_pos( victim );
  update_max_move( victim );

  if( skill_regen && ( skill_hands == 10 || number_range( 0, 1 ) == 0 ) )
    ch->improve_skill( SKILL_REGENERATION );
  else
    ch->improve_skill( SKILL_LAY_HANDS );

  set_delay( ch, 20 );
}
