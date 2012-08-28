#include <sys/types.h>
#include <stdio.h>
#include <syslog.h>
#include "define.h"
#include "struct.h"


/* 
 *   CAN_KILL ROUTINES
 */


bool in_sanctuary( char_data* ch, bool msg )
{    
  const room_data *room  = Room( ch->array->where ); 

  if( room && is_set( room->room_flags, RFLAG_SAFE ) ) {
    if( msg ) 
      send( ch, "You are in a sanctuary.\n\r" );
    return true;
  }
  
  return false;
}


bool can_pkill( char_data* ch, char_data* victim, bool msg )
{
  room_data *room = Room( ch->array->where );
  
  if( room ) {
    if( is_set( room->room_flags, RFLAG_ARENA ) )
      return true;
    if( is_set( room->room_flags, RFLAG_NO_PKILL ) ) {
      if( msg )
        send( ch, "You can't kill players here.\n\r" );
      return false; 
    }
  }
  
  // victim == 0 used to test for pkill rooms only.
  if( !victim )
    return true;
  
  /*
  if( ch->pcdata && associate( ch, victim ) ) {
    if( msg )
      fsend( ch, "You would never attack %s.", victim );
    return false;
  }
  */

  /*
  if( ch->shdata->race <= RACE_LIZARD 
      && victim->shdata->race <= RACE_LIZARD ) {
    if( msg ) {
      const char* race = plyr_race_table[ victim->shdata->race ].name; 
      send( ch, "You would never kill a%s %s.\n\r",
	    isvowel( *race ) ? "n" : "", race );
    }
    return false;
  }
  */
  
  return true;
}


bool can_kill( char_data* ch, char_data* victim, bool msg )
{
  if( !ch )
    return true;

  if( ch == victim ) {
    return false;
  }
  
  char_data *fighting = victim->fighting;

  if( ch->fighting == victim || fighting == ch )
    return true;
  
  // You can assist your pets.
  if( fighting
      && fighting->species
      && is_set( fighting->status, STAT_PET )
      && fighting->leader == ch )
    return true;

  /*
  // Pets can assist you.
  if( fighting
      && is_set( ch->status, STAT_PET )
      && fighting == ch->leader )
    return true;
  */

  if( in_sanctuary( ch, msg ) )
    return false; 
  
  if( victim->is_affected( AFF_SANCTUARY ) ) {
    if( msg )
      fsend( ch, "You cannot attack %s.", victim );
    return false;
  }
  
  if( victim->species ) {
    if( program_data *program = victim->species->attack ) {
      if( !program->binary ) {
	program->compile( );
	if( program->corrupt ) {
	  if( msg ) {
	    fsend( ch,
		   "Attack program for %s is invalid - please contact a god.", victim );
	  }
	  bug( "Invalid attack prog: %d.", victim->species->vnum );
	  return false;
	}
	victim->species->used = true;
      }
    } else {
      if( msg ) {
        fsend( ch,
	       "%s has no attack program - please contact an immortal.", victim );
      }
      bug( "No attack prog: %d.", victim->species->vnum );
      return false;
    }
  }

  if( victim == ch->mount ) {
    if( msg ) {
      send( ch, "You cannot attack your own mount!\n\r" );
    }
    return false;
  }

  if( victim == ch->rider ) {
    if( msg ) {
      send( ch, "You cannot attack your rider!\n\r" );
    }
    return false;
  }

  if( privileged( ch, LEVEL_APPRENTICE ) )
    return true;

  if( privileged( victim, LEVEL_APPRENTICE )
      && !can_pkill( ch, 0, false ) ) {
    if( msg )
      send( ch, "Attacking an immortal here would be certain death.\n\r" );
    return false;
  }

  if( !ch->pcdata ) {
    if( !ch->leader || !is_set( ch->status, STAT_PET ) )
      return true;
    ch = ch->leader;
  }

  if( victim->pcdata
      && !can_pkill( ch, victim, msg ) ) 
    return false;

  if( victim->species
      && victim->leader
      && is_set( victim->status, STAT_PET ) ) {
    if( ch == victim->leader ) {
      if( msg )
        fsend( ch, "You may not attack your own pet.", victim );
      return false;
    }
    if( !victim->leader->species
	&& !can_pkill( ch, victim->leader, false ) ) {
      if( msg )
        fsend( ch,
	       "%s belongs to another player and attacking it is forbidden.",
	       victim );
      return false;
    }
  }
  
  return true;
}


bool trigger_attack( char_data* ch, char_data* victim )
{
  if( victim && victim->species ) {
    for( mprog_data *mprog = victim->species->mprog; mprog; mprog = mprog->next ) {
      if( mprog->trigger == MPROG_TRIGGER_ATTACK ) {
	push( );
	clear_variables( );
        var_ch = ch;
        var_mob = victim; 
        var_room = ch->in_room;
	const int result = mprog->execute( );
	pop( );
        if( !result
	    || !ch->Is_Valid( )
	    || !victim->Is_Valid( )
	    || !ch->in_room
	    || ch->in_room != victim->in_room ) {
          return false;
	}
	break;
      }
    }
  }
  
  for( action_data *action = ch->in_room->action; action; action = action->next ) {
    if( action->trigger == TRIGGER_ATTACK ) {
      push( );
      clear_variables( );
      var_ch = ch;
      var_victim = victim; 
      var_room = ch->in_room;
      const int result = action->execute( );
      pop( );
      if( !result
	  || !ch->Is_Valid( )
	  || !victim->Is_Valid( )
	  || !ch->in_room
	  || ch->in_room != victim->in_room ) {
        return false;
      }
      break;
    }
  }

  return true;
}


/*
 *   AGGRESSIVE ROUTINES
 */


bool join_fight( char_data* victim, char_data* ch, char_data* rch )
{
  // ch is attacker.
  // victim is victim.
  // rch is potential combatant.

  if( rch == ch )
    return false;
  
  if( rch == victim )
    return true;
  
  if( rch->pcdata ) {
    // can_kill() has to come last:
    return is_set( rch->pcdata->pfile->flags, PLR_AUTO_ASSIST ) 
      && is_same_group( rch, victim )
      && !is_same_group( rch, ch )
      && can_kill( rch, ch, false );
  }
  
  // Note: rch->pcdata is false hereafter... it's a mob or pet.

  if( is_set( rch->status, STAT_PET ) ) {
    return rch->leader == victim
      && ( !victim->pcdata
	   || is_set( victim->pcdata->pfile->flags, PLR_PET_ASSIST ) );
  }
  
  if( is_set( victim->status, STAT_PET ) ) {
    // Non-player leaders assist pets.
    if( victim->leader == rch ) {
      return true;
    }
  } else if( rch->species
	     && victim->species 
	     && is_set( rch->species->act_flags, ACT_ASSIST_GROUP ) ) {
    if( victim->species->nation != NATION_NONE
	&& rch->species->nation == victim->species->nation )
      return true;
    if( victim->species->group != GROUP_NONE
	&& victim->species->group == rch->species->group )
      return true;
    if( victim == rch->leader
	|| rch == victim->leader )
      return true;
  }

  return false;
}
 

bool is_aggressive( char_data* ch, char_data* victim )
{
  if( ch == victim
      || ch->position <= POS_SLEEPING )
    return false;

  char_data *opponent;

  if( ( opponent = victim->fighting ) )
    if( join_fight( opponent, victim, ch ) ) {
      return true;
    }

  if( ch->pcdata
      || is_set( victim->in_room->room_flags, RFLAG_SAFE )
      || !victim->Seen( ch ) )
    return false;

  if( ch->shdata->race == RACE_PLANT
      && victim->is_affected( AFF_PROT_PLANTS ) )
    return false;
  
  if( victim->pcdata ) {
    if( ( !is_set( ch->status, STAT_PET )
	  || !ch->leader
	  || ch->leader->species
	  || can_pkill( ch, 0, false ) )
	&& is_enemy( ch, victim ) ) {
      return true;
    }
    if( privileged( victim, LEVEL_BUILDER ) ) {
      return false;
    }
    if( is_set( ch->status, STAT_AGGR_ALL ) ) {
      return true;
    }
  }
  
  if( is_evil( victim ) ) {
    if( is_set( ch->status, STAT_AGGR_EVIL ) )
      return true;
  } else if( is_good( victim ) ) {
    if( is_set( ch->status, STAT_AGGR_GOOD ) )
      return true;
  }

  if( is_lawful( victim ) ) {
    if( is_set( ch->status, STAT_AGGR_LAWFUL ) )
      return true;
  } else if( is_chaotic( victim ) ) {
    if( is_set( ch->status, STAT_AGGR_CHAOTIC ) )
      return true;
  }

  //  if( is_set( ch->status, STAT_PET ) )
  //    return false;
  
  return false;
}


/*
 *   START/STOP FIGHT ROUTINES
 */


void renter_combat( char_data* ch )
{
  ch->aggressive.clear();

  char_array enemies;
  char_array aggro1;
  char_array aggro2;
  
  bool attack_any = ch->species
    && ( ch->Intelligence() <= 6 )
    && is_set( ch->status, STAT_AGGR_ALL );
  
  for( int i = 0; i < *ch->array; ++i ) {
    char_data *rch;
    if( !( rch = character( ch->array->list[i] ) )
	|| rch == ch )
      continue;
    
    if( ch->species
	&& ch->position > POS_SLEEPING
	&& rch->species
	&& rch->position > POS_SLEEPING
	&& rch->species->group != GROUP_NONE
	&& rch->species->group == ch->species->group ) {
      share_enemies( ch, rch );
      share_enemies( rch, ch );
    }
    
    if( is_aggressive( ch, rch ) ) {
      if( is_enemy( ch, rch ) )
	enemies += rch;
      else if( attack_any || !rch->species )
	aggro1 += rch;
      else
	aggro2 += rch;
    }
  }
  
  while( enemies > 0 ) {
    int j = number_range( 0, enemies.size-1 );
    ch->aggressive += enemies[j];
    enemies.remove(j);
  }
  
  while( aggro1 > 0 ) {
    int j = number_range( 0, aggro1.size-1 );
    ch->aggressive += aggro1[j];
    aggro1.remove(j);
  }
  
  while( aggro2 > 0 ) {
    int j = number_range( 0, aggro2.size-1 );
    ch->aggressive += aggro2[j];
    aggro2.remove(j);
  }

  init_attack( ch );
}


void init_attack( char_data* ch, char_data* victim )
{
  if( victim )
    ch->aggressive += victim;

  if( ch->active.time != -1 ) 
    return;

  if( find_event( ch, execute_leap ) )
    return;
  /*
  for( int i = 0; i < ch->events.size; i++ )
    if( ch->events[i]->func == execute_leap )
      return;
  */

  if( !victim ) {
    if( ch->aggressive.is_empty() )
      return;
    victim = ch->aggressive[0];
    if ( !victim->Is_Valid() ) {
      bug( "init_attack: victim is invalid." );
      ch->aggressive -= victim;
      return;
    }
  }

  event_data *event = new event_data( execute_leap, ch );
  event->pointer = (void*) victim; 
  add_queue( event, 26 - ch->Dexterity( )/2 );
}


void react_attack( char_data *ch, char_data *victim )
{
  if( !ch
      || ch == victim
      || victim->position <= POS_DEAD
      || ch->in_room != victim->in_room )
    return;
  
  // Allow fleeing from ch, even if ch isn't attacking anyone.
  // Why? ch may have just killed your tank and hasn't leapt yet.
  set_bit( ch->status, STAT_FLEE_FROM );

  // Stop following a leader who attacks you.
  if( victim->leader == ch )
    stop_follower( victim );

  // Stop following when you attack leader.
  if( ch->leader == victim )
    stop_follower( ch );

  if( ch->array ) {
    for( int i = 0; i < *ch->array; i++ ) {
      char_data *rch;
      if( ( rch = character( ch->array->list[i] ) )
	  && rch != ch
	  && rch != victim ) {
	if( join_fight( victim, ch, rch ) ) {
	  init_attack( rch, ch );
	} else if( join_fight( ch, victim, rch ) ) {
	  init_attack( rch, victim );
	}
      }
    }
  }

  // This is queued after the joins so the leap will be run first.
  init_attack( victim, ch );
}


void stop_fight( char_data* ch, bool special )
{
  set_fighting( ch, 0 );
  disrupt_spell( ch );
  ch->aggressive.clear( );
  stop_events( ch, execute_leap );
  
  for( int i = 0; i < *ch->array; i++ ) {
    char_data *rch;

    if( !( rch = character( ch->array->list[i] ) ) )
      continue;

    // Berserkers keep on attacking slept mobs.
    if( special
	&& is_berserk( rch ) )
      continue;

    if( rch->cast ) {
      if( rch->cast->target == ch
	  && skill_spell_table[rch->cast->spell].type != STYPE_WORLD
	  && skill_spell_table[rch->cast->spell].type != STYPE_SUMMON ) {
	disrupt_spell( rch );
      } else {
	rch->cast->audience -= ch;
      }
    }

    rch->aggressive -= ch;
    
    if( rch->fighting == ch ) {
      set_fighting( rch, 0 );
    }
    
    if( //is_berserk( rch ) &&
        !rch->fighting
        && rch->aggressive.is_empty() ) {
      // Fight's over, nothing to see here.
      remove_bit( rch->status, STAT_BERSERK );
      remove_bit( rch->status, STAT_FOCUS );
      remove_bit( rch->status, STAT_FLEE_FROM );
    }
    
    for( int j = rch->events-1; j >= 0; j-- ) {
      if( rch->events[j]->pointer == (void*) ch ) {
        extract( rch->events[j] );
	init_attack( rch );
      }
    }
  }
}


void update_aggression( char_data *ch )
{
  if( room_data *room = ch->in_room ) {
    for( int i = 0; i < room->contents; ++i ) {
      char_data *rch;
      if( !( rch = character( room->contents[i] ) )
 	  || rch == ch )
	continue;
      if( is_aggressive( rch, ch ) ) 
	init_attack( rch, ch );
    }
  }
}


void update_aggression( room_data *room )
{
  for( int i = 0; i < room->contents; ++i ) {
    if( char_data *rch = character( room->contents[i] ) ) {
      update_aggression( rch );
    }
  }
}
