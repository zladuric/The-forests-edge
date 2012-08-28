#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include "define.h"
#include "struct.h"


/*
 *   ANIMATE DEAD SPELLS
 */


static bool null_corpse( obj_data* corpse, int spell )
{
  if( !corpse ) {
    bug( "%s: Null pointer for corpse.", skill_spell_table[spell-SPELL_FIRST].name );
    return true;
  }

  return false;
}


static bool can_animate( char_data* ch, species_data* species,
			 species_data* undead )
{
  if( species->shdata->race == RACE_UNDEAD ) {
    if( ch ) {
      fsend( ch, "That corpse has died twice already - it is too mangled to\
 be animated again." );
    }
    return false;
  }
  
  if( !undead ) {
    if( ch )
      send( ch, "That corpse cannot be animated.\n\r" );
    return false;
  }
  
  if( ch && undead->shdata->level > ch->Level()/2 ) {
    send( ch, "You are not powerful enough to animate that corpse.\n\r" );
    return false;
  }

  if( ch && undead->shdata->level + pet_levels( ch ) > ch->Level() ) {
    send( ch, "You cannot animate that corpse while controlling your current pets.\n\r" );
    return false;
  }

  return true;
}


bool spell_animate_dead( char_data* ch, char_data*, void* vo,
  int level, int )
{
  obj_data *corpse = (obj_data*) vo;

  if( null_corpse( corpse, SPELL_ANIMATE_DEAD ) )
    return false;

  Content_Array *array = corpse->array;
  thing_data *where = array->where;

  if( !Room( where ) ) {
    bug( "Animate Dead: corpse \"%s\" is not in a room.", corpse );
    return false;
  }

  species_data *species;
  
  if( !( species = get_species( corpse->value[1] ) ) ) {
    bug( "Animate Dead: Corpse \"%s\" of unknown species.", corpse );
    return false;
  }

  species_data *zombie;

  if( !can_animate( ch, species, zombie = get_species( species->zombie ) ) )
    return false;

  if( ch && number_range( 1, 20 ) > 9+level ) {
    send( ch, "The ritual fails; nothing remains of the corpse but a few ashes.\n\r" );
    extract_corpse( corpse );
    return false;
  }

  char_data *remains = new Mob_Data( zombie );

  if( ch ) {
    set_bit( remains->status, STAT_PET );
    set_bit( remains->status, STAT_TAMED );

    remove_bit( remains->status, STAT_AGGR_ALL );
    remove_bit( remains->status, STAT_AGGR_GOOD );
    remove_bit( remains->status, STAT_AGGR_EVIL );
    remove_bit( remains->status, STAT_AGGR_LAWFUL );
    remove_bit( remains->status, STAT_AGGR_CHAOTIC );
  }

  remains->To( *array );

  fsend_seen( remains, "%s slowly stands up.", remains );

  extract_corpse( corpse );

  if( ch )
    add_follower( remains, ch );

  return true;
}


bool spell_greater_animation( char_data* ch, char_data*, void* vo,
			      int level, int )
{
  obj_data *corpse = (obj_data*) vo;

  if( null_corpse( corpse, SPELL_GREATER_ANIMATION ) )
    return false;

  Content_Array *array = corpse->array;
  thing_data *where = array->where;

  if( !Room( where ) ) {
    bug( "Greater Animation: corpse \"%s\" is not in a room.", corpse );
    return false;
  }

  species_data *species;

  if( !( species = get_species( corpse->value[1] ) ) ) {
    bug( "Greater Animation: Corpse \"%s\" of unknown species.", corpse );
    return false;
  }

  if( !is_set( species->act_flags, ACT_HAS_SKELETON ) ) {
    if( ch ) {
      const char *pos = ch->in_room->position( );
      fsend( ch, "%s did not contain a skeleton and so trying to animate it\
 was doomed to failure.  All that remains %s are a few ashes.",
	     corpse,
	     *pos ? pos : "here" );
      fsend_seen( ch, "%s frowns as %s turns to ashes.", ch, corpse );
    }
    corpse->Extract(1);
    return false;
  }

  species_data *skeleton;

  if( !can_animate( ch, species, skeleton = get_species( species->skeleton ) ) )
    return false;

  if( ch && number_range( 1, 20 ) > 9+level ) {
    send( ch, "The ritual fails; nothing remains of the corpse but a few ashes.\n\r" );
    extract_corpse( corpse );
    return false;
  }

  char_data *remains = new Mob_Data( skeleton );

  if( ch ) {
    set_bit( remains->status, STAT_PET );
    set_bit( remains->status, STAT_TAMED );
    
    remove_bit( remains->status, STAT_AGGR_ALL );
    remove_bit( remains->status, STAT_AGGR_GOOD );
    remove_bit( remains->status, STAT_AGGR_EVIL );
    remove_bit( remains->status, STAT_AGGR_LAWFUL );
    remove_bit( remains->status, STAT_AGGR_CHAOTIC );
  }

  remains->To( *array );

  fsend_seen( remains, "%s slowly stands up.", remains );

  extract_corpse( corpse );

  if( ch )
    add_follower( remains, ch );

  return true;
}


/*
 *   BANISHMENT SPELLS 
 */


bool spell_turn_undead( char_data* ch, char_data* victim, void*,
  int level, int )
{
  if( victim->shdata->race != RACE_UNDEAD ) 
    return false;
  
  damage_physical( victim, ch, spell_damage( SPELL_TURN_UNDEAD, level ),
		   "*the enchantment" );

  return true;
}


bool spell_banishment( char_data* ch, char_data* victim, void*,
  int level, int )
{
  if( victim->shdata->race != RACE_UNDEAD ) 
    return false;
  
  damage_physical( victim, ch, spell_damage( SPELL_BANISHMENT, level ),
		   "*the enchantment" );

  return true;
}
