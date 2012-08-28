#include <ctype.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include "define.h"
#include "struct.h"


/*
 *   SWITCH/RETURN
 */


void do_switch( char_data* ch, const char *argument )
{
  if( is_mob( ch ) )
    return;
  
  char_data *victim;

  player_data *pc = player( ch );

  if( ch->Level() >= LEVEL_BUILDER && *argument ) {
    if( !( victim = one_character( ch, argument, "switch", ch->array ) ) )
      return;
    
    if( !victim->species ) {
      send( ch, "You cannot switch to players.\n\r" );
      return;
    }
    
    if( victim->pcdata ) {
      send( ch, "Character in use.\n\r" );
      return;
    }

  } else {
    if( !( victim = pc->familiar ) ) {
      send( ch, "You don't have a familiar to switch to!\n\r" );
      return;
    }

    if( ch->mana < 20 ) {
      send( ch, "You don't have enough energy to switch to your familiar.\n\r" );
      return;
    }
      
    if( victim->pcdata ) {
      send( ch, "Character in use.\n\r" );
      return;
    }

    ch->mana -= 20;
  }
  
  link_data *link = ch->link;
  
  link->character = victim;
  pc->switched = victim;
  pc->link = 0;
  victim->link = link;
  victim->pcdata = pc->pcdata;
  //  victim->timer = current_time;
  victim->pcdata->burden = victim->get_burden( );  // Prevent burden change messages.

  fsend( victim, "You find yourself in the body of %s.", victim->Seen_Name( ch ) );
}


void do_return( char_data* ch, const char * )
{
  if( not_player( ch ) )
    return;

  if( !ch->link || ch->link->player == ch ) {
    send( ch, "You aren't switched.\n\r" );
    return;
  }

  send( ch, "You return to your original body.\n\r" );

  link_data *link = ch->link;
  player_data *pc = link->player;
  link->character = pc;
  pc->link = ch->link; 
  ch->link = 0;
  ch->pcdata = 0;
  pc->switched = 0;
  pc->pcdata->burden = pc->get_burden( );  // Prevent burden change messages.
}


/*
 *   SUMMONING SPELLS
 */


static bool find_familiar( char_data* ch, obj_data* obj, int level,
			   int species_list, int obj_list )
{
  player_data*      pc;
  species_data*    species;
  int                    i;

  if( !obj ) {
    bug( "Find_Familiar: Null Object as reagent!?" );
    return false;
  }

  if( !( pc = player( ch ) ) )
    return false;

  if( ch->Level() < LEVEL_APPRENTICE
      && is_set( ch->pcdata->pfile->flags, PLR_FAMILIAR ) ) {
    send( ch, "Nothing happens.\n\r" );
    send( ch, "You can only summon one familiar per level.\n\r" );
    return false;
  }
  
  if( pc->familiar ) {
    send( ch, "Nothing happens.\n\r" );
    send( ch, "You can only have one familiar at a time.\n\r" );
    return false;
  }
  
  for( i = 0; i < 10; i++ )
    if( obj->pIndexData->vnum
	== list_value[ obj_list ][ 10*ch->Align_Good_Evil()+i ] )
      break;
  
  if( i == 10 ) {
    send( ch, "Nothing happens.\n\r" );
    return false;
  }
  
  fsend( *ch->array, "%s disintegrates in a burst of blue flame.", obj );
  obj->Extract( 1 );
  
  if( number_range( 0,100 ) < 50-7*level+10*i ) {
    send( ch, "You feel the summoning fail.\n\r" );
    return false;
  }
  
  i = list_value[ species_list ][ 10*ch->Align_Good_Evil()+i ];
  
  if( !( species = get_species( i ) ) ) {
    bug( "Find_familiar: unknown species." );
    return false;
  }
  
  mob_data *familiar = new Mob_Data( species );
  pc->familiar = familiar;

  set_bit( familiar->status, STAT_PET );
  set_bit( familiar->status, STAT_FAMILIAR );
  set_bit( ch->pcdata->pfile->flags, PLR_FAMILIAR );

  remove_bit( familiar->status, STAT_AGGR_ALL );
  remove_bit( familiar->status, STAT_AGGR_GOOD );
  remove_bit( familiar->status, STAT_AGGR_EVIL );
  remove_bit( familiar->status, STAT_AGGR_LAWFUL );
  remove_bit( familiar->status, STAT_AGGR_CHAOTIC );
  
  //  delay_wander( new event_data( execute_wander, familiar ) );

  familiar->To( *ch->array );

  fsend( ch,
	 "%s appears in response to your summons.",
	 familiar );

  add_follower( familiar, ch );

  return true;
}


bool spell_lesser_summoning( char_data* ch, char_data*, void* vo,
  int level, int )
{
  if( null_caster( ch, SPELL_LESSER_SUMMONING ) ) 
    return false;
  
  obj_data *obj = object( (thing_data *) vo );
  if( !obj ) {
    bug( "Lesser_Summoning: Null reagent object." );
    return false;
  }

  return find_familiar( ch, obj, level,
			 LIST_LS_SPECIES, LIST_LS_REAGENT );
}


bool spell_find_familiar( char_data* ch, char_data*, void* vo,
			  int level, int )
{
  if( null_caster( ch, SPELL_FIND_FAMILIAR ) ) 
    return false;

  obj_data *obj = object( (thing_data *) vo );
  if( !obj ) {
    bug( "Find_Familiar: Null reagent object." );
    return false;
  }

  return find_familiar( ch, obj, level,
			LIST_FF_SPECIES, LIST_FF_REAGENT );
}


bool spell_request_ally( char_data* ch, char_data*, void* vo,
  int level, int )
{
  if( null_caster( ch, SPELL_REQUEST_ALLY ) ) 
    return false;

  obj_data *obj = object( (thing_data *) vo );
  if( !obj ) {
    bug( "Request_Ally: Null reagent object." );
    return false;
  }

  return find_familiar( ch, obj, level,
			LIST_RA_SPECIES, LIST_RA_REAGENT );
}


static bool find_buddy( char_data* ch, obj_data* obj, int level,
			int species_list, int obj_list )
{
  if( !obj ) {
    bug( "Find_Buddy: Null Object as reagent!?" );
    return false;
  }

  player_data *pc;

  if( !( pc = player( ch ) ) )
    return false;

  int i;

  for( i = 0; i < 10; i++ )
    if( obj->pIndexData->vnum
	== list_value[ obj_list ][ 10*ch->Align_Good_Evil()+i ] )
      break;

  if( i == 10 ) {
    send( ch, "Nothing happens.\n\r" );
    return false;
  }

  fsend( *ch->array, "%s disintegrates in a burst of blue flame.", obj );
  obj->Extract( 1 );

  if( number_range( 0,100 ) < 50-7*level+10*i ) {
    send( ch, "You feel the summoning fail.\n\r" );
    return false;
  }

  i = list_value[ species_list ][ 10*ch->Align_Good_Evil()+i ];

  species_data *species;

  if( !( species = get_species( i ) ) ) {
    bug( "Find_buddy: unknown species." );
    return false;
  }
  
  mob_data *buddy = new Mob_Data( species );

  set_bit( buddy->status, STAT_PET );

  remove_bit( buddy->status, STAT_AGGR_ALL );
  remove_bit( buddy->status, STAT_AGGR_GOOD );
  remove_bit( buddy->status, STAT_AGGR_EVIL );
  remove_bit( buddy->status, STAT_AGGR_LAWFUL );
  remove_bit( buddy->status, STAT_AGGR_CHAOTIC );

  //  delay_wander( new event_data( execute_wander, buddy ) );

  buddy->To( *ch->array );

  fsend( ch,
	 "%s appears in response to your summons.",
	 buddy );
  
  add_follower( buddy, ch );

  return true;
}


bool spell_animate_clay( char_data* ch, char_data*, void*, int, int )
{
  if( null_caster( ch, SPELL_ANIMATE_CLAY ) )
    return true;
  
  if( has_elemental( ch ) ) {
    send( ch, "You can only control one golem at a time.\n\r" );
    return false;
  }
  
  species_data *species = get_species( MOB_CLAY_GOLEM );
  
  if( !species ) {
    bug( "Animate_Clay: NULL species." );
    return true;
  }

  mob_data *golem = new Mob_Data( species );
  set_bit( golem->status, STAT_PET );

  //  delay_wander( new event_data( execute_wander, golem ) );
  golem->To( *ch->array );

  add_follower( golem, ch );

  return true;
}


bool spell_construct_golem( char_data* ch, char_data*, void* vo, 
			    int level, int )
{
  if( null_caster( ch, SPELL_CONSTRUCT_GOLEM ) ) 
    return false;
  
  if( has_elemental( ch ) ) {
    send( ch, "You can only control one golem at a time.\n\r" );
    return false;
  }
  
  obj_data *obj = object( (thing_data *) vo );
  if( !obj ) {
    bug( "Construct_Golem: Null reagent object." );
    return false;
  }

  return find_buddy( ch, obj, level,
		     LIST_CG_SPECIES, LIST_CG_REAGENT );
}


bool spell_conjure_elemental( char_data* ch, char_data*, void* vo,
			      int level, int )
{
  if( null_caster( ch, SPELL_CONJURE_ELEMENTAL ) ) 
    return false;

  if( has_elemental( ch ) ) {
    send( ch, "You can only control one elemental at a time.\n\r" );
    return false;
  }

  obj_data *obj = object( (thing_data *) vo );
  if( !obj ) {
    bug( "Conjure_Elemental: Null reagent object." );
    return false;
  }

  return find_buddy( ch, obj, level,
		     LIST_CE_SPECIES, LIST_CE_REAGENT );
}


bool spell_find_mount( char_data* ch, char_data*, void* vo,
		       int level, int )
{
  if( null_caster( ch, SPELL_FIND_MOUNT ) ) 
    return false;

  if( has_mount( ch ) )  {
    return false;
  }

  obj_data *obj = object( (thing_data *) vo );
  if( !obj ) {
    bug( "Find_Mount: Null reagent object." );
    return false;
  }

  return find_buddy( ch, obj, level,
		     LIST_FM_SPECIES, LIST_FM_REAGENT );
}
