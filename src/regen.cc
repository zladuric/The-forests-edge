#include <sys/types.h>
#include <stdio.h>
#include "define.h"
#include "struct.h"

/*
void execute_hunger( event_data *event )
{
  player_data *pl = player( event->owner );

  if( !pl->was_in_room ) {
    int amount = -1;
    if( pl->Level() >= LEVEL_DEMIGOD && !pl->species ) {
      amount = 1;
    } else if( pl->is_affected( AFF_SUSTENANCE ) ) {
      amount = 0;
    }
    gain_condition( pl, COND_FULL, amount, true );
  }

  int delay = plyr_race_table[pl->shdata->race].hunger_time * PULSE_PER_SECOND;

  //  add_queue( event, number_range( (delay+1)/2, 3*delay/2 ) );
}


void execute_thirst( event_data *event )
{
  player_data *pl = player( event->owner );

  if( !pl->was_in_room ) {
    int amount = -1;
    if( pl->Level() >= LEVEL_DEMIGOD && !pl->species ) {
      amount = 1;
    } else if( pl->is_affected( AFF_SUSTENANCE ) ) {
      amount = 0;
    }
    gain_condition( pl, COND_THIRST, amount, true );
  }

  int delay = plyr_race_table[pl->shdata->race].thirst_time * PULSE_PER_SECOND;
  if( arid( pl ) ) {
    //  if( pl->in_room && pl->in_room->sector_type == SECT_DESERT ) {
    delay /= 2;
  }

  //  add_queue( event, number_range( (delay+1)/2, 3*delay/2 ) );
}


void execute_drunk( event_data *event )
{
  player_data *pl = player( event->owner );

  if( !pl->was_in_room ) {
    int amount = ( 3 + pl->pcdata->condition[COND_ALCOHOL] )/4;
    pl->pcdata->condition[COND_ALCOHOL] -= amount;
    gain_drunk( pl, amount-1 );
  }

  int delay = plyr_race_table[pl->shdata->race].drunk_time * PULSE_PER_SECOND;
  //  add_queue( event, number_range( (delay+1)/2, 3*delay/2 ) );
}
*/

bool condition_update( char_data* ch )
{
  //  if( ch->species )
  //    return false;

  if( privileged( ch, LEVEL_DEMIGOD ) ) {
    // Gods don't have silly mortal problems like hunger, thirst, or drunkeness.
    ch->condition[ COND_FULL ] = 50;
    ch->condition[ COND_THIRST ] = 50;
    ch->condition[ COND_ALCOHOL ] = 0;
    ch->condition[ COND_DRUNK ] = 0;

  } else {
    int amount = 0;

    const bool in_death = ch->in_room ? ( ch->in_room->vnum == ROOM_DEATH ) : false;

    // Hunger.
    if( !ch->is_affected( AFF_SUSTENANCE ) ) {
      amount = ( ch->shdata->race == RACE_TROLL ? -2 : -1 );
    }
    gain_condition( ch, COND_FULL, amount, true );
    
    const int full = ch->condition[ COND_FULL ];

    if( full < -30 ) {
      if( inflict( ch, 0, number_range( 1, ( -full - 29 )/2 ), "starvation", true ) ) {
	if( in_death ) {
	  send( ch, "Closing your link for your own protection.\n\r" );
	  disconnect( ch );
	}
	return true;
      }
    }

    // Thirst.
    amount = 0;
    if( !ch->is_affected( AFF_SUSTENANCE ) ) {
      amount = ( ch->shdata->race == RACE_LIZARD ? number_range( -1, -2 ) : -1 );
      if( arid( ch ) )
	amount *= 2;
    }
    gain_condition( ch, COND_THIRST, amount, true );
    
    const int thirst = ch->condition[ COND_THIRST ];

    if( thirst < -30 ) {
      if( inflict( ch, 0, number_range( 1, ( -thirst - 29 )/2 ), "dehydration", true ) ) {
	if( in_death ) {
	  send( ch, "Closing your link for your own protection.\n\r" );
	  disconnect( ch );
	}
	return true;
      }
    }

    // Hangover.
    amount = ( 3+ch->condition[COND_ALCOHOL] )/4;
    ch->condition[COND_ALCOHOL] -= amount;
    gain_drunk( ch, amount-1 );
  }

  return false;
}


/*
 *   DRUNK ROUTINES
 */


static const char *const drunk_inc [] = {
  "",
  "You feel fairly intoxicated.",
  "You stumble and realize you are roaring drunk.",
  "You feel queasy and realize you drank more than you should have.",
  "You feel very ill and the world is spinning rapidly."
};

static const char *const drunk_dec [] = {
  "You feel quite sober now.",
  "You feel less intoxicated.",
  "The alcohol begins to wear off, though the walls are still moving.",
  "Your stomach settles, but you doubt you can walk.",
  "The world stops spinning right and shifts to the left.",
};


void gain_drunk( char_data *ch, int amount )
{
  int *condition = ch->condition;
 
  const int new_cond = max( 0, condition[ COND_DRUNK ]+amount );
  
  if( new_cond == condition[ COND_DRUNK ] )
    return;
   
  const int tolerance = ch->tolerance( );
  const int new_level = min( 4, new_cond/tolerance );
  const int level = min( 4, condition[ COND_DRUNK ]/tolerance );

  condition[ COND_DRUNK ] = new_cond;

  if( level == new_level )
    return;
    
  send( ch, level > new_level
	? drunk_dec[new_level]
	: drunk_inc[new_level] );
  send( ch, "\n\r" );
}


/*
 *   HUNGER/THIRST ROUTINES
 */


void gain_condition( char_data* ch, int iCond, int value, bool regen )
{
  //  if( ch->species )
  //    return;
 
  const int old_cond = ch->condition[ iCond ];
  const int new_cond = max( ch->species ? 0 : -50, min( old_cond+value, 50 ) );
  
  ch->condition[ iCond ] = new_cond;
    
  if( value < 0 || value == 0 && regen ) {
    if( iCond == COND_FULL
	&& new_cond < 0
	&& ch->pcdata
	&& is_set( ch->pcdata->message, MSG_HUNGER ) ) {
      send( ch, "You are %s\n\r",
	    new_cond < -20 ? ( new_cond < -30 ? "starving to death!" : "ravenous." )
	    : ( new_cond < -10 ? "hungry." : "mildly hungry." ) );
    } else if( iCond == COND_THIRST
	       && new_cond < 0
	       && ch->pcdata
	       && is_set( ch->pcdata->message, MSG_THIRST ) ) {
      send( ch, "You%s\n\r",
	    new_cond < -20 ? ( new_cond < -30 ? " are dying of thirst!" : "r throat is parched." )
	    : ( new_cond < -10 ? " are thirsty." : "r throat feels dry." ) );
    }
    return;
  }

  if( iCond == COND_FULL && value != 999 ) {
    if( old_cond <= 40 && new_cond > 40 && !privileged( ch, LEVEL_DEMIGOD ) )
      send( ch, "You feel full.\n\r" );
    else if( old_cond < 0 && new_cond >= 0 )
      send( ch, "You no longer feel hungry.\n\r" );
  } else if( iCond == COND_THIRST && value != 999 ) {
    if( old_cond <= 40 && new_cond > 40 && !privileged( ch, LEVEL_DEMIGOD ) )
      send( ch, "You can drink no more.\n\r" ); 
    else if( old_cond < 0 && new_cond >= 0 )
      send( ch, "You quench your thirst.\n\r" );
  }
}


/*
 *   HIT/MANA/MOVE REGENERATION
 */


/*
void regen_update( void )
{
  char_data*          ch;
  //  struct timeval   start;
  
  //  gettimeofday( &start, 0 );

  for( int i = 0; i < mob_list; i++ ) {
    ch = mob_list[i];
    if( ch->Is_Valid( ) ) {
      if( ch->position >= POS_STUNNED )
        regenerate( ch );
      if( ch->position == POS_STUNNED )
        update_pos( ch );
    }
  }
  
  for( int i = 0; i < player_list; i++ ) {
    ch = player_list[i];
    if( ch->In_Game( ) && !ch->was_in_room ) {
      if( ch->position >= POS_STUNNED )
        regenerate( ch );
      if( ch->position == POS_STUNNED )
        update_pos( ch );
      if( ch->pcdata->prac_timer > 0 )
        --ch->pcdata->prac_timer;
    }
  }
  
  //  pulse_time[ TIME_REGEN ] = stop_clock( start );
}
*/


/*
 *   REGENERATE ROUTINE
 */


int tenth( int value )
{
  value += sign( value )*number_range( 0,9 );

  return value/10;
}


// Every 6 seconds +/-.
void execute_regen( event_data *event )
{
  char_data *ch = (char_data*) event->owner;
  
  // Confused when not fighting.
  if( is_confused( ch )
      && ch->active.time == -1
      && number_range( 0, 3 ) == 0 ) {
    confused_char( ch );
  }

  if( ch->species && ch->Is_Valid( )
      || !ch->species && ch->In_Game( ) && !ch->was_in_room ) {
    if( ch->position >= POS_STUNNED )
      regenerate( ch );
    if( ch->position == POS_STUNNED )
      update_pos( ch );
    if( !ch->species )
      if( ch->pcdata->prac_timer > 0 )
	--ch->pcdata->prac_timer;
  }

  set_regen( ch );
}


void regenerate( char_data* ch )
{
  bool down = ( ch->hit < ch->max_hit );
  int val = tenth( ch->Hit_Regen( ) );
  ch->hit += val;
  if( val > 0 ) {
    if( ch->position == POS_MEDITATING
	&& ch->mana != 0
	&& ch->pcdata
	&& down ) {
      if( number_range( 0, 1 ) == 1 ) {
	ch->improve_skill( SKILL_TRANCE );
      } else {
	ch->improve_skill( SKILL_MEDITATE );
      }
    }
    if( ch->hit >= ch->max_hit ) {
      ch->hit = ch->max_hit;    
      if( ch->pcdata
	  && is_set( ch->pcdata->message, MSG_MAX_HIT )
	  && down )
	send( ch, "You are now at maximum hitpoints.\n\r" );
    }
  }
  update_max_move( ch );

  /*
  if( ch->hit < ch->max_hit ) {
    //      && !ch->is_affected( AFF_POISON ) ) {
    int val = tenth( ch->Hit_Regen( ) );
    ch->hit += val;
    if( ch->position == POS_MEDITATING
	&& ch->mana != 0
	&& val > 0
	&& ch->pcdata ) {
      ch->improve_skill( SKILL_MEDITATE );
      ch->improve_skill( SKILL_TRANCE );
    }
    if( ch->hit >= ch->max_hit ) {
      ch->hit = ch->max_hit;    
      if( ch->pcdata
	  && is_set( ch->pcdata->message, MSG_MAX_HIT ) )
	send( ch, "You are now at maximum hitpoints.\n\r" );
    }
    update_max_move( ch );
  }
  */


  const bool zero = ( ch->mana == 0 );
  down = ( ch->mana < ch->max_mana );
  val = tenth( ch->Mana_Regen( ) );
  ch->mana += val;
  if( ch->mana >= ch->max_mana ) {
    ch->mana = ch->max_mana;
    if( ch->pcdata
	&& is_set( ch->pcdata->message, MSG_MAX_ENERGY )
	&& down )
      send( ch, "You are now at maximum energy.\n\r" );

  } else if( ch->mana <= 0 ) {
    if( ch->mana < 0 ) {
      ch->mana = 0; 
      if( !ch->leech_list.is_empty() ) {
	if( ch != ch->leech_list[0]->target ) {
	  fsend( ch,
		 "You are unable to continue supporting the %s affect on %s.",
		 aff_char_table[ch->leech_list[0]->type].name,
		 ch->leech_list[0]->target );
	  if( char_data *victim = character( ch->leech_list[0]->target ) ) {
	    fsend( victim,
		   "The energy supply to the %s affect on you is cut.",
		   aff_char_table[ch->leech_list[0]->type].name );
	  }
	  
	} else {
	  fsend( ch, "You are unable to support the %s affect on yourself.",
		 aff_char_table[ch->leech_list[0]->type].name );
	}
	
	remove_leech( ch->leech_list[0] );
      }
    }
    if( ch->position == POS_MEDITATING
	&& !zero
	&& val < 0 ) {
      send( ch, "Your energy is depleted, reducing the benefit of further meditation.\n\r" );
    }
  }

  /*
  if( ch->mana < ch->max_mana
      || ( ch->position == POS_MEDITATING && ch->hit < ch->max_hit ) ) {
    // Note: regen may be negative for meditating monks.
    bool mana_down = ( ch->mana < ch->max_mana );
    ch->mana += tenth( ch->Mana_Regen( ) );
    //    ch->mana = max( ch->mana, 0 );
    if( ch->mana >= ch->max_mana ) {
      ch->mana = ch->max_mana;
      if( ch->pcdata
	  && is_set( ch->pcdata->message, MSG_MAX_ENERGY )
	  && mana_down )
        send( ch, "You are now at maximum energy.\n\r" );
    }
  }
  */
  
  down = ( ch->move < ch->max_move );
  ch->move += tenth( ch->Move_Regen( ) );
  if( ch->move >= ch->max_move ) {
    ch->move = ch->max_move;
    if( ch->hit == ch->max_hit && ch->pcdata
	&& ( ch->position != POS_STANDING || ch->mount )
	&& is_set( ch->pcdata->message, MSG_MAX_MOVE )
	&& down )
      send( ch, "You are now at maximum movement.\n\r" );

  } else if( ch->move < 0 ) {
    ch->move = 0;
  }

  /*
  if( ch->move < ch->max_move ) {
    ch->move += tenth( ch->Move_Regen( ) );
    //    ch->move = max( ch->move, 0 );
    if( ch->move >= ch->max_move ) {
      ch->move = ch->max_move;
      if( ch->hit == ch->max_hit && ch->pcdata
	  && ( ch->position != POS_STANDING || ch->mount )
	  && is_set( ch->pcdata->message, MSG_MAX_MOVE ) )
        send( ch, "You are now at maximum movement.\n\r" );
    }
  }
  */
}


/*
 *   REGEN FUNCTIONS
 */


int char_data :: Hit_Regen( )
{
  if( is_affected( AFF_POISON ) ) 
    return 0;

  int mod = hit_regen;
  int regen = 5 + ( base_hit + Level()*( Constitution( ) - 12 )/2 )/8;

  if( shdata->race < MAX_PLYR_RACE ) 
    regen += plyr_race_table[shdata->race].hp_bonus;

  if( !species ) {
    regen += clss_table[pcdata->clss].hit_bonus;

    const int full = condition[ COND_FULL ];

    if( full < -20 ) {
      regen = 0;
    } else if( full < 0 ) {
      regen = ( regen * ( full + 20 ) ) / 20;
    }

    if( full < -30 ) {
      mod = 0;
    } else if( full < 0 ) {
      mod = ( mod * ( full + 30 ) ) / 30;
    }

    const int thirst = condition[ COND_THIRST ];

    if( thirst < -20 ) {
      regen = 0;
    } else if( thirst < 0 ) {
      regen = ( regen * ( thirst + 20 ) ) / 20;
    }

    if( thirst < -30 ) {
      mod = 0;
    } else if( thirst < 0 ) {
      mod = ( mod * ( thirst + 30 ) ) / 30;
    }

    if( is_drunk( ) ) 
      regen += 3;

  } else {
    if( is_set( species->act_flags, ACT_ZERO_REGEN ) ) 
      return 0;
    
    if( !leader ) 
      regen *= 2;
    
    regen = min( regen, max_hit/10 );
  }
  
  if( position == POS_MEDITATING && mana != 0 ) {
    regen += regen*( (int)get_skill( SKILL_MEDITATE )
		     +(int)get_skill( SKILL_TRANCE ) )/15;
  }

  if( position == POS_SLEEPING || position == POS_MEDITATING ) {
    if( pos_obj ) {
      regen = 8*regen/3;
    } else {
      regen *= 2;
    }
  } else if( position == POS_RESTING ) {
    if( pos_obj ) {
      regen *= 2;
    } else {
      regen = 3*regen/2;
    }
  }

  if( is_affected( AFF_REGENERATION ) )
    regen = 5*regen/4;
  
  /*
  if( is_affected( AFF_POISON ) ) 
    regen /= 4;
  */
  
  if( regen > 1 )
    regen /= 2;

  regen += mod;

  return regen;
}


int char_data :: Mana_Regen( )
{
  int mod = mana_regen;
  int mana = base_mana + Level()*Intelligence( )/4;
  int regen = 5+mana/12;

  if( shdata->race < MAX_PLYR_RACE )
    regen += plyr_race_table[shdata->race].mana_bonus;
  
  if( !species ) {
    regen += clss_table[pcdata->clss].mana_bonus;
    
    const int full = condition[ COND_FULL ];

    if( full < -20 ) {
      regen = 0;
    } else if( full < 0 ) {
      regen = ( regen * ( full + 20 ) ) / 20;
    }

    if( full < -30 ) {
      mod = 0;
    } else if( full < 0 ) {
      mod = ( mod * ( full + 30 ) ) / 30;
    }

    const int thirst = condition[ COND_THIRST ];

    if( thirst < -20 ) {
      regen = 0;
    } else if( thirst < 0 ) {
      regen = ( regen * ( thirst + 20 ) ) / 20;
    }

    if( thirst < -30 ) {
      mod = 0;
    } else if( thirst < 0 ) {
      mod = ( mod * ( thirst + 30 ) ) / 30;
    }
  }
  
  if( position == POS_MEDITATING && hit < max_hit )
    return mod - leech_regen( this ) - 60;

  if( position == POS_SLEEPING ) {
    if( pos_obj ) {
      regen = 8*regen/3;
    } else {
      regen *= 2;
    }
  } else if( position == POS_RESTING ) {
    if( pos_obj ) {
      regen *= 2;
    } else {
      regen = 3*regen/2;
    }
  }

  regen += mod - leech_regen( this );
   
  return regen;
}


int char_data :: Move_Regen( )
{
  int mod = move_regen;
  int regen = ( 80 + base_move - 100 )/8 + Dexterity( );

  if( shdata->race < MAX_PLYR_RACE ) 
    regen += plyr_race_table[shdata->race].move_bonus;
  
  if( !species ) {
    regen += clss_table[pcdata->clss].move_bonus;
    
    const int full = condition[ COND_FULL ];

    if( full < 0 ) {
      regen = ( regen * ( full + 50 ) ) / 50;
    }

    if( full < -30 ) {
      mod = 0;
    } else if( full < 0 ) {
      mod = ( mod * ( full + 30 ) ) / 30;
    }

    const int thirst = condition[ COND_THIRST ];

    if( thirst < 0 ) {
      regen = ( regen * ( thirst + 50 ) ) / 50;
    }

    if( thirst < -30 ) {
      mod = 0;
    } else if( thirst < 0 ) {
      mod = ( mod * ( thirst + 30 ) ) / 30;
    }

    if( is_drunk( ) )
      regen += 3;
  }

  if( position == POS_SLEEPING ) {
    if( pos_obj ) {
      regen = 8*regen/3;
    } else {
      regen *= 2;
    }
  } else if( position == POS_RESTING || position == POS_MEDITATING ) {
    if( pos_obj ) {
      regen *= 2;
    } else {
      regen = 3*regen/2;
    }
  }

  if( is_affected( AFF_POISON ) ) 
    regen /= 2;
  
  regen += mod;
  
  return regen;
}
