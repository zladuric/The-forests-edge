#include <sys/types.h>
#include <stdio.h>
#include <syslog.h>
#include "define.h"
#include "struct.h"



void do_energize( char_data* ch, const char *argument )
{
  char_data*  victim;
  int           mana;
  
  if( ch->species
      || ch->pcdata->clss != CLSS_MONK ) {
    send( ch, "Only monks can transfer energy.\n\r" );
    return;
  }
  
  int level = ch->get_skill( SKILL_TRANSFER_ENERGY );

  if( level == UNLEARNT ) {
    send( ch, "You don't know how to transfer energy.\n\r" );
    return;
  } 
  
  if( !( victim = one_character( ch, argument, "energize", ch->array ) ) ) {
    return;
  }

  if( is_fighting( ch, "transfer energy" ) ) {
    return;
  }
  
  if( victim->fighting ) {
    send( ch,
	  "You can't transfer energy to someone who is fighting!\n\r" );
    return;
  }

  if( ch->mana < 2 ) {
    send( ch, "You don't have enough energy to transfer.\n\r" );
    return;
  }
  
  if( ch == victim ) {
    send( ch, "You cannot transfer energy to yourself.\n\r" );
    return;
  }

  if( ( mana = victim->max_mana-victim->mana ) == 0 ) {
    send( ch, "%s does not need any more mana.\n\r", victim );
    return;
  }

  fsend( ch, "You transfer energy to %s.", victim );
  fsend( victim, "%s transfers energy to you.", ch );
  fsend_seen( ch, "%s transfers energy to %s.", ch, victim );

  double cure = (double)level*0.1; 
  int heal = number_range( 70, 120 );

  if( ch->mana-heal > 0 ) {
    victim->mana += (int)((double)heal*cure);
    ch->mana -= heal;
  } else {
    victim->mana += (int)((double)ch->mana*cure);
    ch->mana = 0;
  }

  victim->mana = min( victim->max_mana, victim->mana );

  update_pos( victim );

  ch->improve_skill( SKILL_TRANSFER_ENERGY );
}
