#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include "define.h"
#include "struct.h"


static bool can_assist( char_data* ch, char_data* victim )
{
  if( !ch || !victim || ch->species )
    return true;
  
  if( ch->pcdata->clss != CLSS_CLERIC
      && ch->pcdata->clss != CLSS_PALADIN )
    return true;
  
  if( ch->Align_Distance( victim ) > 2 ) {
    //  fsend( ch,
    //	   "You feel the energy drain from you, but for some reason the spirits refuse your request." );
    return false;
  }

  if( victim->shdata->race == RACE_UNDEAD ) {
    return false;
  }

  return true;
}


/*
 *   HEALING SPELLS
 */


static const char *const heal_message[] = {
  "You were not wounded so were not healed.\n\r",
  "%s was not wounded so was not healed.",
  
  "You are completely healed.\n\r",
  "%s is completely healed.",
  
  "Most of your wounds are gone.\n\r",
  "Most of %s's wounds disappear.",
  
  "Your wounds feel quite a bit better.\n\r",
  "%s looks quite a bit less injured.",
  
  "You are slightly healed.\n\r",
  "%s looks a little bit better.",
  
  "You don't feel any better.\n\r",
  "%s doesn't look any better."
};


bool heal_victim( char_data* ch, char_data* victim, int hp, int duration )
{
  if( duration == -4 || duration == -3 )
    return false;

  if( !can_assist( ch, victim ) ) {
    send( victim, heal_message[10] );
    fsend_seen( victim, heal_message[11], victim );
    return false;
  }

  if( victim->hit >= victim->max_hit ) {
    send( victim, heal_message[0] );
    fsend_seen( victim, heal_message[1], victim );
    return false;
  }

  const int damage = victim->max_hit-victim->hit;

  victim->hit = min( victim->max_hit, victim->hit+hp );

  const unsigned i = ( damage <= hp
		       ? 1
		       : ( 2*damage < 3*hp
			   ? 2
			   : ( damage < 3*hp
			       ? 3
			       : 4 ) ) );
 
  send( victim, heal_message[2*i] );
  fsend_seen( victim, heal_message[2*i+1], victim );

  update_pos( victim );
  update_max_move( victim );
  record_damage( victim->fighting, ch );    // Provides an experience share.

  return true;
}


/*
bool spell_cure_light( char_data* ch, char_data* victim, void*,
		       int level, int duration )
{
  return heal_victim( ch, victim, spell_damage( SPELL_CURE_LIGHT, level ), duration )
    && number_range( 0, 1 ) == 1;
}


bool spell_cure_serious( char_data* ch, char_data* victim, void*,
			 int level, int duration )
{
  return heal_victim( ch, victim, spell_damage( SPELL_CURE_SERIOUS, level ), duration )
    && number_range( 0, 1 ) == 1;
}
*/


bool spell_group_serious( char_data* ch, char_data*, void*,
			  int level, int duration )
{
  if( null_caster( ch, SPELL_GROUP_SERIOUS ) )
    return false;
  
  if( duration == -4 || duration == -3 )
    return false;

  char_data* gch;
  bool any = false;

  for( int i = 0; i < *ch->array; ++i ) 
    if( ( gch = character( ch->array->list[i] ) )
	&& is_same_group( gch, ch )
	&& heal_victim( ch, gch, spell_damage( SPELL_GROUP_SERIOUS, level ), duration ) )
      any = true;
  
  return any;
}


/*
bool spell_cure_critical( char_data* ch, char_data* victim, void*,
			  int level, int duration )
{
  return heal_victim( ch, victim, spell_damage( SPELL_CURE_CRITICAL, level ), duration )
    && number_range( 0, 1 ) == 1;
}
*/


bool spell_group_critical( char_data* ch, char_data*, void*,
			   int level, int duration )
{
  if( null_caster( ch, SPELL_GROUP_CRITICAL ) )
    return false;
  
  if( duration == -4 || duration == -3 )
    return false;

  char_data* gch;
  bool any = false;

  for( int i = 0; i < *ch->array; ++i ) 
    if( ( gch = character( ch->array->list[i] ) )
	&& is_same_group( gch, ch )
	&& heal_victim( ch, gch, spell_damage( SPELL_GROUP_CRITICAL, level ), duration ) )
      any = true;
  
  return any;

}


/*
bool spell_heal( char_data* ch, char_data* victim, void*,
		 int level, int duration )
{
  return heal_victim( ch, victim, spell_damage( SPELL_HEAL, level ), duration );
}


bool spell_restoration( char_data* ch, char_data* victim, void*,
			int level, int duration )
{
  return heal_victim( ch, victim, spell_damage( SPELL_RESTORATION, level ), duration );
}
*/


/*
 *   REMOVE CURSE
 */


bool spell_remove_curse( char_data* ch, char_data* victim, void*,
			 int level, int duration )
{
  if( duration == -4 || duration == -3 )
    return false;

  obj_data*   obj;
  int       count = 0;

  if( !consenting( victim, ch, "curse removal" ) ) 
    return false;

  bool any = false;

  for( int i = victim->wearing.size-1; i >= 0; --i ) {
    if( !( obj = object( victim->wearing[i] ) )
	|| obj->removable( victim ) )
      continue;
    
    if( count++ > level/5 ) 
      return any;

    fsend( victim, "%s which you are wearing turns to dust.", obj );
    fsend_seen( victim, "%s which %s is wearing turns to dust.", obj, victim );
    obj->Extract( );
    any = true;
  }

  for( int i = victim->contents.size-1; i >= 0; --i ) {
    if( !( obj = object( victim->contents[i] ) )
	|| obj->droppable( victim ) )
      continue;

    if( count++ > level/5 ) 
      return any;

    fsend( victim, "%s which you are carrying turns to dust.", obj );
    fsend_seen( victim, "%s which %s is carrying turns to dust.", obj, victim );
    obj->Extract( );
  }
  
  if( count == 0 ) 
    if( victim == ch ) 
      send( ch, "You weren't carrying anything cursed.\n\r" );
    else 
      fsend( ch, "%s wasn't carrying anything cursed.", victim );

  return any;
}


/*
 *   POISON/DISEASE
 */


bool spell_cure_disease( char_data* ch, char_data* victim, void*,
			 int level, int duration )
{
  if( duration == -4 || duration == -3 )
    return false;

  if( !victim->is_affected( AFF_PLAGUE ) ) {
    if( ch != victim )
      fsend( ch, "%s wasn't diseased.", victim );
    return false;
  }

  if( shorten_affect( victim, AFF_PLAGUE, level*2 ) > 0 ) {
    fsend( victim, "You feel a little better, but your disease is not yet cured." );
    fsend_seen( victim, "%s looks a little better.", victim );

  } else if( victim->is_affected( AFF_PLAGUE ) ) {
    if( ch != victim ) {
      fsend( ch, "%s is still diseased!", victim );
    }
    send( victim, "You are still diseased!\n\r" );
  }

  //  strip_affect( victim, AFF_PLAGUE );

  return true;
}


bool spell_cure_poison( char_data* ch, char_data* victim, void*,
			int level, int duration )
{
  if( duration == -4 || duration == -3 )
    return false;

  const bool is_drunk = ( victim->condition[COND_ALCOHOL] > 0
			  || victim->condition[COND_DRUNK] > 0 );
  
  if( !is_drunk && !victim->is_affected( AFF_POISON ) ) {
    if( ch && ch != victim )
      fsend( ch, "%s wasn't poisoned.", victim );
    return false;
  }

  victim->condition[ COND_ALCOHOL ] = 0;
  const int amount = min( level * 5, victim->condition[ COND_DRUNK ] );
  if( is_drunk ) {
    send( victim, "Some of the alcohol in your system is neutralized.\n\r" );
    gain_drunk( victim, -amount );
    if( victim->condition[COND_DRUNK] == 0 ) {
      fsend_seen( victim, "%s is quite sober now.", victim );
    } else {
      fsend_seen( victim, "%s seems less intoxicated.", victim );
    }
  }

  if( shorten_affect( victim, AFF_POISON, level*2 ) > 0 ) {
    fsend( victim, "You feel a little better, but some poison remains in your body." );
    fsend_seen( victim, "%s looks a little better.", victim );

  } else if( victim->is_affected( AFF_POISON ) ) {
    if( ch && ch != victim ) 
      fsend( ch, "%s is still poisoned!", victim );
    send( victim, "You are still poisoned!\n\r" );
  }

  return true;
}


/*
 *   SILENCE SPELL
 */


/*
bool spell_gift_of_tongues( char_data* ch, char_data* victim, void*,
			    int level, int duration )
{
  return spell_affect( ch, victim, level, duration,
		       SPELL_GIFT_OF_TONGUES, AFF_TONGUES );
}
*/


bool spell_silence( char_data* ch, char_data* victim, void*,
		    int level, int duration )
{
  if( duration == -4 || duration == -3 )
    return false;

  if( makes_save( victim, ch, RES_MAGIC, SPELL_SILENCE, level ) ) {
    send( ch, "Nothing happens.\n\r" );
    send_seen( ch, "Nothing happens.\n\r", ch );
    return false;
  }

  return spell_affect( ch, victim, level, duration,
			SPELL_SILENCE, AFF_SILENCE );
}


bool spell_augury( char_data* ch, char_data*, void* vo, int level, int duration )
{
  // Fill.
  if( duration == -4 )
    return false;

  obj_data *obj = (obj_data*) vo;

  if( !ch || !obj )
    return false;
  
  if( !obj->Belongs( ch ) ) {
    fsend( ch, "%s glows black and you sense its true owner is %s.",
	   obj, obj->owner->name );
    return true;
  }

  send( ch, "Nothing happens.\n\r" );
  return false;
}


/*
bool spell_true_sight( char_data* ch, char_data* victim, void*, 
		       int level, int duration )
{
  return spell_affect( ch, victim, level, duration,
		       SPELL_TRUE_SIGHT, AFF_TRUE_SIGHT );
}


bool spell_protect_life( char_data* ch, char_data* victim, void*,
			 int level, int duration )
{
  return spell_affect( ch, victim, level, duration,
		       SPELL_PROTECT_LIFE, AFF_LIFE_SAVING ); 
}


bool spell_sense_life( char_data* ch, char_data* victim, void*, 
		       int level, int duration )
{
  return spell_affect( ch, victim, level, duration,
		       SPELL_SENSE_LIFE, AFF_SENSE_LIFE );

}
*/


/*
 *   CREATION SPELLS
 */


bool spell_create_water( char_data* ch, char_data* victim, void* vo,
			 int level, int duration )
{
  obj_data*         obj  = (obj_data*) vo;
  Content_Array*  where;
  int            liquid;
  int             metal;

  // Fill, drink.
  if( duration == -4 || duration == -1 ) {
    return false;
  }

  /* DIP */
  if( duration == -3 ) {
    if( ( metal = obj->metal( ) ) != 0 && number_range( 0, 3 ) == 0 ) {
      if( obj->rust == 3 ) {
        fsend( ch, "%s disintegrates into worthless pieces.", obj );
        fsend_seen( ch, "%s disintegrates into worthless pieces.", obj );
        obj->Extract( 1 );
      } else {
	obj = (obj_data *) obj->From( 1, true );
        fsend( ch, "%s %s.", obj, material_table[ metal ].rust_verb );
        fsend_seen( ch, "%s %s.", obj, material_table[ metal ].rust_verb );
        ++obj->rust;
	obj->To( );
      }
    }
    return false;
  }      

  /* THROWING, REACTING */ 

  if( !ch || victim )
    return false;

  /* CAST */

  liquid = LIQ_WATER;
  where = obj->array;

  if( number_range( 0, 99 ) < level ) {
    while( true ) {
      if( ( liquid = number_range( 0, table_max[ TABLE_LIQUID ] - 1 ) ) == LIQ_WATER
	  || liquid_table[ liquid ].create )
        break;
    }
  }
  
  if( obj->value[1] != 0
      && liquid != obj->value[2] ) {
    liquid = LIQ_SLIME;
  }

  obj = (obj_data*) obj->From( 1 );

  if( liquid == LIQ_POISON ) {
    set_bit( obj->value[3], CONSUME_POISON );
  }

  if( is_set( obj->value[3], CONSUME_PLAGUE ) ) {
    if( number_range( 0, 1 ) == 1 )
      remove_bit( obj->value[3], CONSUME_PLAGUE );
  }

  obj->value[1] += 100*level;
  obj->value[2] = liquid;

  include_empty  = false;
  include_liquid = false;

  // Obj placed back in inv before messages so player can always see it.

  if( obj->value[1] >= obj->value[0] ) {
    obj->value[1] = obj->value[0];
    set_bit( obj->extra_flags, OFLAG_KNOWN_LIQUID );
    obj->To( *where );
    fsend( ch, "%s fills to overflowing with %s.", obj, liquid_table[ liquid ].name );
    fsend_seen( ch, "%s fills to overflowing with %s.", obj, liquid_table[ liquid ].name );
  } else {
    set_bit( obj->extra_flags, OFLAG_KNOWN_LIQUID );
    obj->To( *where );
    fsend( ch, "%s fills partially up with %s.", obj, liquid_table[ liquid ].name );
    fsend_seen( ch, "%s fills partially up with %s.", obj, liquid_table[ liquid ].name );
  }

  include_empty  = true;
  include_liquid = true;

  if( ch->Capacity( ) < 0 ) {
    const char *drop = ch->in_room->drop( );
    if( *drop ) {
      fsend( ch, "You can't carry %s, so it falls %s.", obj, drop );
      fsend_seen( ch, "%s can't carry %s, so it falls %s.", ch, obj, drop );
    } else {
      fsend( ch, "You can't carry %s, so you drop it.", obj );
      fsend_seen( ch, "%s can't carry %s, so %s drops it.", ch, obj, ch->He_She( ) );
    }
    obj = (obj_data *) obj->From(1);
    obj->To( ch->in_room );
  }

  react_filled( ch, obj, liquid );

  return true;    
}


/*
 *   OBJECT SPELLS
 */

bool spell_purify( char_data* ch, char_data* victim, void*,
		   int level, int time )
{
  if( time == -4 || time == -3 )
    return false;

  if( !victim->is_affected( AFF_SLEEP )
      && !victim->is_affected( AFF_CURSE )
      && !victim->is_affected( AFF_SLOW )
      && !victim->is_affected( AFF_CONFUSED ) ) {
    send( ch, "Nothing happens.\n\r" );
    send_seen( ch, "Nothing happens.\n\r" );
    return false;
  }

  // Removes same level of sleep.
  int dur = duration( SPELL_SLEEP, level );

  if( shorten_affect( victim, AFF_SLEEP, dur ) > 0 ) {
    fsend( victim, "You drift toward consciousness, but soon fall back into a deep slumber." );
    fsend_seen( victim, "%s looks a little less sleepy.", victim );

  } else if( victim->is_affected( AFF_SLEEP ) ) {
    if( ch != victim ) {
      fsend( ch, "%s is still magically slept!", victim );
    }
    send( victim, "You are still magically slept!\n\r" );
  }

  // Removes same level of curse.
  dur = duration( SPELL_CURSE, level );

  if( shorten_affect( victim, AFF_CURSE, dur ) > 0 ) {
    fsend( victim, "You feel a portion of the gods' favor return to you." );
    fsend_seen( victim, "%s seems a little less forsaken.", victim );

  } else if( victim->is_affected( AFF_CURSE ) ) {
    if( ch != victim ) {
      fsend( ch, "%s is still cursed!", victim );
    }
    send( victim, "You are still cursed!\n\r" );
  }

  // Removes same level of slow.
  dur = duration( SPELL_SLOW, level );

  if( shorten_affect( victim, AFF_SLOW, dur ) > 0 ) {
    fsend( victim, "You feel some of your old quickness returning." );
    fsend_seen( victim, "%s appears a bit less sluggish.", victim );

  } else if( victim->is_affected( AFF_SLOW ) ) {
    if( ch != victim ) {
      fsend( ch, "%s is still slowed!", victim );
    }
    send( victim, "You are still slowed!\n\r" );
  }

  // Removes same level of confuse.
  dur = duration( SPELL_CONFUSE, level );

  if( shorten_affect( victim, AFF_CONFUSED, dur ) > 0 ) {
    fsend( victim, "You feel some of your mental sharpness returning." );
    fsend_seen( victim, "%s seems a bit less confused.", victim );

  } else if( victim->is_affected( AFF_CONFUSED ) ) {
    if( ch != victim ) {
      fsend( ch, "%s is still confused!", victim );
    }
    send( victim, "You are still confused!\n\r" );
  }

  return true;
}


bool spell_sanctify( char_data* ch, char_data*, void *vo, int level, int duration )
{
  // Fill.
  if( duration == -4 )
    return false;

  obj_data *obj = (obj_data*) vo;

  if( is_set( obj->extra_flags, OFLAG_SANCT ) ) {
    if( ch ) {
      send( ch, "Nothing happens.\n\r" );
      send_seen( ch, "Nothing happens.\n\r" );
    }
    return false;
  }

  /*
  int val;

  if( obj->pIndexData->item_type == ITEM_WEAPON ) {
    Dice_Data dice( obj->value[1] );
    // Avg damage + 10*ench.
    val = dice.average() + 10*obj->value[0];
  } else {
    // 3*AC + 10*ench.
    val = 3*obj->value[1] + 10*obj->value[0];
  }

  //int dam =  obj->value[0]*(obj->value[1]+1);
  */

  if( obj->pIndexData->level > 9*level ) {
    // *** FIX ME: show to room even if !ch.
    if( ch ) {
      fsend( *ch->array,
	     "%s glows with a pale blue light which quickly fades.", 
	     obj );
    }
    return false;
  }

  // *** FIX ME: show to room even if !ch.
  if( ch )
    fsend( *ch->array,
	   "%s glows with a pale blue light, which slowly fades as if being absorbed into the item.",
	   obj );

  obj = (obj_data *) obj->From( 1, true );
  set_bit( obj->extra_flags, OFLAG_SANCT );
  obj->To( );

  return true;
}


/*
 *   PROTECTION SPELLS
 */


/*
bool spell_armor( char_data* ch, char_data* victim, void*, int level,
		  int duration )
{
  return spell_affect( ch, victim, level, duration,
		       SPELL_ARMOR, AFF_ARMOR );
}


bool spell_bless( char_data* ch, char_data* victim, void*, int level,
		  int duration )
{
  return spell_affect( ch, victim, level, duration,
		       SPELL_BLESS, AFF_BLESS );
}
*/



bool spell_protect_good( char_data* ch, char_data* victim, void*,
			 int level, int duration )
{
  if( duration == -4 || duration == -3 )
    return false;

  if( !is_evil( victim ) ) {
    if( ch && ch != victim ) {
      send( ch, "Nothing happens.\n\r" );
    }
    send( victim, "Nothing happens.\n\r" );
    return false;
  }

  return spell_affect( ch, victim, level, duration,
		       SPELL_PROTECT_GOOD, AFF_PROTECT_GOOD );
}


bool spell_protect_evil( char_data* ch, char_data* victim, void*,
			 int level, int duration )
{
  if( duration == -4 || duration == -3 )
    return false;

  if( !is_good( victim ) ) {
    if( ch && ch != victim ) {
      send( ch, "Nothing happens.\n\r" );
    }
    send( victim, "Nothing happens.\n\r" );
    return false;
  }

  return spell_affect( ch, victim, level, duration,
		       SPELL_PROTECT_EVIL, AFF_PROTECT_EVIL );
}


bool spell_protect_law( char_data* ch, char_data* victim, void*,
			int level, int duration )
{
  if( duration == -4 || duration == -3 )
    return false;

  if( !is_chaotic( victim ) ) {
    if( ch && ch != victim ) {
      send( ch, "Nothing happens.\n\r" );
    }
    send( victim, "Nothing happens.\n\r" );
    return false;
  }

  return spell_affect( ch, victim, level, duration,
		       SPELL_PROTECT_LAW, AFF_PROTECT_LAW );
}


bool spell_protect_chaos( char_data* ch, char_data* victim, void*,
			int level, int duration )
{
  if( duration == -4 || duration == -3 )
    return false;

  if( !is_lawful( victim ) ) {
    if( ch && ch != victim ) {
      send( ch, "Nothing happens.\n\r" );
    }
    send( victim, "Nothing happens.\n\r" );
    return false;
  }

  return spell_affect( ch, victim, level, duration,
		       SPELL_PROTECT_CHAOS, AFF_PROTECT_CHAOS );
}


/*
 *   SUSTENANCE
 */


/*
bool spell_sustenance( char_data* ch, char_data* victim, void*,
			 int level, int duration )
{
  return spell_affect( ch, victim, level, duration,
		       SPELL_SUSTENANCE, AFF_SUSTENANCE );
}
*/


/*
 *   WRATH
 */


bool spell_holy_wrath( char_data *ch, char_data *victim, void*,
		       int level, int duration )
{
  if( duration == -4 || duration == -3 )
    return false;

  if( ch
      && ch->Align_Good_Evil() != victim->Align_Good_Evil()
      && ch->Align_Law_Chaos() != victim->Align_Law_Chaos() ) {
    fsend( ch, "You are unable to incite %s into a holy rage.", victim );
    fsend( victim, "%s is unable to incite you into a holy rage.", ch );
    return false;
  }

  return spell_affect( ch, victim, level, duration,
		       SPELL_HOLY_WRATH, AFF_WRATH );
}


/*
 *   DEATH
 */


bool spell_resurrect( char_data*, char_data*, void*, int, int )
{
  return true;
}


/*
 *   FEAR
 */


bool spell_fear( char_data* ch, char_data* victim, void*, int level, int duration )
{
  if( duration == -4 || duration == -3 )
    return false;

  if( victim->shdata->race == RACE_UNDEAD ) {
    send( *victim->array, "Nothing happens.\n\r" );
    return false;
  }

  if( is_berserk( victim ) ) {
    if( ch ) {
      fsend( ch, "%s is enraged with battle frenzy, and is immune to fear.", victim );
      fsend( victim, "In your battle frenzy, %s fails to make you afraid!", ch );
      fsend( *ch->array, "%s fails to make %s afraid.",
	     ch, victim );
    } else {
      fsend( victim, "In your battle frenzy, you easily shrug off a momentary wave of fear." );
      fsend_seen( victim, "%s cringes for a moment, but shakes off %s fear.",
		  victim, victim->His_Her( ) );
    }
    return false;
  }

  if( makes_save( victim, ch, RES_MIND, SPELL_FEAR, level ) ) {
    if( ch ) {
      fsend( ch, "You fail to make %s afraid!", victim );
      fsend( victim, "%s fails to make you afraid!", ch );
      fsend( *ch->array, "%s fails to make %s afraid.",
	     ch, victim );
    } else {
      fsend( victim, "You feel afraid for a moment, but manage to steel your resolve." );
      fsend_seen( victim, "%s cringes for a moment, but shakes off %s fear.",
		  victim, victim->His_Her( ) );
    }
    return false;
  }
  
  if( ( victim->pcdata || is_set( victim->species->act_flags, ACT_WIMPY ) )
      && number_range( 0, 3 ) == 0
      && attempt_flee( victim ) ) {
    return true;
  }

  const char *const verbs [] = {
    "cringe", "cower", "shudder", "shriek", "scream"
  };

  // Can't shriek or scream if silenced.
  const int max = is_silenced( victim ) ? 2 : 4;

  const char *const verb = verbs[ number_range( 0, max ) ];

  interpret( victim, verb );
  
  record_damage( victim, ch );    // Provides an experience share.

  disrupt_spell( victim );
  set_min_delay( victim, 40+level );

  return true;
}


/*
 *   PARALYZE
 */

bool spell_paralyze( char_data* ch, char_data* victim, void*,
		     int level, int duration )
{
  if( duration == -4 || duration == -3 )
    return false;

  if( victim->is_affected( AFF_PARALYSIS ) ) {
    return false;
  }

  if( victim->shdata->race == RACE_ELEMENTAL
      || makes_save( victim, ch, RES_POISON, SPELL_PARALYZE, level ) ) {
    send( victim, "Nothing happens.\n\r" );
    send_seen( victim, "Nothing happens.\n\r" );
    return false;
  }

  return spell_affect( ch, victim, level, duration,
		       SPELL_PARALYZE, AFF_PARALYSIS );
}
