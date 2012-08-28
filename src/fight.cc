#include <ctype.h>
#include <sys/types.h>
#include <stdio.h>
#include <syslog.h>
#include "define.h"
#include "struct.h"


/*
 *   LOCAL FUNCTIONS
 */


static bool    absorb_armor      ( char_data*, char_data*, const char*, bool, const char*, int&, const char*&, bool );
static bool    absorb_magic      ( char_data*, char_data*, const char*, bool, int& );
static bool    block_shield      ( char_data*, char_data*, obj_data*, obj_data*, const char*, bool );
static void    damage_modifiers  ( char_data*, char_data*, obj_data*, int& );
bool    damage_weapon     ( char_data*, obj_data*& );
static bool    dodge             ( char_data*, char_data*, int&, bool );
static bool    tumble            ( char_data*, char_data*, int&, bool );
static bool    fire_shield       ( char_data*, char_data* );
static bool    ice_shield        ( char_data*, char_data* );
static bool    thorn_shield      ( char_data*, char_data* );
static bool    ion_shield        ( char_data*, char_data* );
static bool    flaming_weapon    ( char_data*, char_data*, obj_data* );
static bool    misses_blindly    ( char_data*, char_data*, int&, const char*, bool );
static bool    parry             ( char_data*, char_data*, obj_data*, obj_data*, obj_data*, int& );
static bool    guard             ( char_data*, char_data*, obj_data*, int& );
static bool    shadow_dance      ( char_data*, char_data*, const char*, bool );
static bool    switch_victim     ( char_data*, char_data* );
static bool    trip              ( char_data*, char_data*, obj_data* );
static int     damage_armor      ( char_data*, char_data*, int, int&, obj_data* = 0 );
static int     backstab_damage   ( char_data* );
static int     charge_damage     ( char_data* );
static int     garrote_damage    ( char_data* );
static void    critical_hit      ( char_data*, char_data*, obj_data*, int& );
static void    modify_roll       ( char_data*, char_data*, obj_data*, int& );
static void    power_strike      ( char_data*, char_data*, obj_data*, int& );
static void    stun              ( char_data*, char_data*, const char*, bool );
static void    trigger_hit       ( char_data*, char_data*, obj_data* );
static int     get_damage        ( char_data*, obj_data* );
static bool    gouge_attack      ( char_data*, char_data*, int& );


/*
 *   LOCAL CONSTANTS
 */


static const char *const dam_loc_name [MAX_WEAR_HUMANOID] = {
  "light", "finger", "finger", "neck", "[BUG1]",
  "body", "head", "leg", "foot", "hand", "arm", "[BUG2]", "[BUG3]",
  "waist", "wrist", "wrist", "[BUG4]", "[BUG5]" };


/*
 *   EXTERNAL SUPPORT ROUTINES
 */


const char* weapon_class( const obj_clss_data *clss )
{
  if( clss->value[3] >= 0 && clss->value[3] < table_max[ TABLE_SKILL_WEAPON ] )
    return skill_weapon_table[ clss->value[3] ].name;

  return "none";
}


const char* weapon_class( const obj_data *obj )
{
  if( obj->value[3] >= 0 && obj->value[3] < table_max[ TABLE_SKILL_WEAPON ] )
    return skill_weapon_table[ obj->value[3] ].name;

  return "none";
}


const char *attack_noun( int type )
{
  if( number_range( 0, 3 ) == 0 )
    return skill_weapon_table[type].noun[0];

  int i = 0;
  for( ; i < MAX_ATTACK; ++i ) {
    if( !*skill_weapon_table[type].noun[i] ) {
      break;
    }
  }

  return skill_weapon_table[type].noun[ number_range( 0, i-1 ) ];
}


const char *attack_verb( int type )
{
  if( number_range( 0, 3 ) == 0 )
    return skill_weapon_table[type].verb[0];

  int i = 0;
  for( ; i < MAX_ATTACK; ++i ) {
    if( !*skill_weapon_table[type].verb[i] ) {
      break;
    }
  }

  return skill_weapon_table[type].verb[ number_range( 0, i-1 ) ];
}


bool can_defend( char_data* victim, char_data* ch )
{
  if( victim->position < POS_FIGHTING
      || is_entangled( victim )
      || !ch->Seen( victim )
      || is_set( victim->status, STAT_STUNNED ) )
    return false;

  return true;
}


bool is_fighting( char_data* ch, const char *msg )
{
  if( ch->position <= POS_SLEEPING || !opponent( ch ) )
    return false;

  if( !ch->fighting ) {
    cant_message( ch, msg, "being attacked" );
  } else {
    cant_message( ch, msg, "fighting" );
  }
  
  return true;
}


bool not_fighting( char_data *ch, const char *msg )
{
  if( ch->fighting )
    return false;

  if( msg ) {
    fsend( ch, "You must be attacking to %s.",
	   *msg ? msg : "do that" );
  }

  return true;
}


char_data *opponent( char_data* ch )
{
  if( !ch->array )
    return 0;

  char_data *rch;

  if( ( rch = ch->fighting ) ) {
    rch->Select( 1 );
    return rch;
  }

  for( int i = 0; i < *ch->array; ++i ) {
    if( ( rch = character( ch->array->list[i] ) )
	&& rch->fighting == ch ) {
      rch->Select( 1 );
      return rch;
    }
  }
  
  return 0;
} 


// Someone is attacking you, or whomever you are attacking is fighting or incapacitated.
// Allows you to flee.
char_data *opponent2( char_data *ch )
{
  char_data *rch = ch->fighting;

  if( rch
      && ( rch->fighting
	   || rch->position <= POS_SLEEPING
	   || is_set( rch->status, STAT_FLEE_FROM ) ) ) {
    rch->Select( 1 );
    return rch;
  }
  
  for( int i = 0; i < *ch->array; ++i ) {
    if( ( rch = character( ch->array->list[i] ) ) 
	&& rch->position > POS_SLEEPING 
	&& rch->fighting == ch ) {
      rch->Select( 1 );
      return rch;
    }
  }

  return 0;
}


// Someone is attacking you, or whomever you are attacking is fighting.
// i.e., someone can try to prevent you from fleeing.
char_data *opponent3( char_data *ch )
{
  char_data *rch = ch->fighting;

  if( rch
      && rch->fighting
      && rch->position > POS_SLEEPING
      && !is_set( rch->status, STAT_STUNNED ) ) {
    rch->Select( 1 );
    return rch;
  }
  
  for( int i = 0; i < *ch->array; ++i ) {
    if( ( rch = character( ch->array->list[i] ) ) 
	&& rch->position > POS_SLEEPING 
	&& rch->fighting == ch
	&& !is_set( rch->status, STAT_STUNNED ) ) {
      rch->Select( 1 );
      return rch;
    }
  }

  return 0;
}


bool set_fighting( char_data *ch, char_data *victim )
{
  if( ch == victim )
    return false;

  if( ch->fighting == victim )
    return true;

  if( victim && !trigger_attack( ch, victim ) ) {
    return false;
  }

  if( is_set( ch->status, STAT_GARROTING ) ) {
    if( ch->fighting
	&& ch->fighting->is_affected( AFF_CHOKING ) ) {
      if( ch->fighting->position > POS_DEAD ) {
	fsend( ch, "You stop garroting %s.", ch->fighting );
	fsend( ch->fighting, "%s stops garroting you.", ch );
	fsend_seen( ch, "%s stops garroting %s.", ch, ch->fighting );
      }
      affect_data af;
      af.type = AFF_CHOKING;
      modify_affect( ch->fighting, &af, false );
    }
    remove_bit( ch->status, STAT_GARROTING );
  }

  ch->fighting = victim;

  if( victim ) {
    react_attack( ch, victim );
  }

  return true;
}


/*
char_data* has_enemy( char_data* ch )
{
  if( ch->fighting && ch->fighting->fighting )
    return ch->fighting;

  char_data *rch;

  for( int i = 0; i < *ch->array; i++ ) 
    if( ( rch = character( ch->array->list[i] ) ) 
	&& rch->position > POS_SLEEPING 
	&& rch->aggressive.includes( ch ) )
      return rch;
  
  if( ch->cast
      && spell_table[ ch->cast->spell ].type == STYPE_OFFENSIVE )
    return (char_data*) ch->cast->target;
  
  return 0;
}
*/


/*
 *   TOP LEVEL ROUND HANDLER
 */


void leap_message( char_data* ch, char_data* victim )
{
  char_data *rch;

  victim->Show( 1 );

  if( victim->fighting == ch ) {
    fsend( ch, "You counterattack %s!", victim );

    if( ch->Seen( victim ) )
      fsend( victim, "%s%s counterattacks you!%s",
	     bold_v( victim ), ch, normal( victim ) );
    
    for( int i = 0; i < *victim->array; i++ ) 
      if( ( rch = character( victim->array->list[i] ) ) 
	  && rch != victim
	  && rch != ch
	  && ch->Seen( rch )
	  && rch->link ) 
        fsend( rch, "%s%s counterattacks %s!%s",
	       damage_color( rch, ch, victim ), ch, victim, normal( rch ) );
  } else {
    fsend( ch, "You leap to attack %s!", victim );
    
    if( ch->Seen( victim ) ) {
      fsend( victim, "%s%s leaps to attack you!%s",
	     bold_v( victim ), ch, normal( victim ) );
    }

    for( int i = 0; i < *victim->array; i++ ) 
      if( ( rch = character( victim->array->list[i] ) ) 
	  && rch != victim
	  && rch != ch
	  && ch->Seen( rch )
	  && rch->link ) 
        fsend( rch, "%s%s leaps to attack %s!%s",
	      damage_color( rch, ch, victim ), ch, victim, normal( rch ) );
  }
}


bool jump_feet( char_data* ch )
{
  if( ch->position != POS_RESTING
      && ch->position != POS_MEDITATING ) 
    return false;

  if( ch->pos_obj ) {
    ch->pos_obj->Show( 1 );
    fsend( ch, "You quickly jump off %s to your feet.", ch->pos_obj );
    fsend_seen( ch, "%s jumps off %s to %s feet.", ch, ch->pos_obj, ch->His_Her( ) );
    unseat( ch );
  } else {
    fsend( ch, "You quickly jump to your feet." );
    fsend_seen( ch, "%s jumps to %s feet.", ch, ch->His_Her( ) );
  }

  ch->position = POS_STANDING;
  remove_bit( ch->status, STAT_HOLD_POS );
  set_bit( ch->status, STAT_STOOD );

  return true;
} 


/*
 *   PLAYER FIGHT ROUTINES
 */


void get_wield( char_data *ch, obj_data *& wield, obj_data *& secondary, obj_data *& shield )
{
  wield = 0;
  secondary = 0;
  shield = 0;

  obj_data *obj;

  //  if( !ch->species ) {
  if( ( obj = ch->Wearing( WEAR_HELD_L, LAYER_OVER ) )
      && obj->pIndexData->item_type == ITEM_ARMOR ) {
    shield = obj;
  }
  
  if( !shield
      && ( obj = ch->Wearing( WEAR_HELD_L, LAYER_BASE ) )
      && obj->pIndexData->item_type == ITEM_WEAPON ) {
    secondary = obj;
  }
  
  if( ( obj = ch->Wearing( WEAR_HELD_R, LAYER_BASE ) )
      && obj->pIndexData->item_type == ITEM_WEAPON ) {
    wield = obj;
  }
  
  /*
  } else {
    // Until all mobs have wear_part set...
    for( int i = 0; i < ch->wearing; ++i ) {
      obj = (obj_data*) ch->wearing[i];
      if( obj->position == WEAR_HELD_L ) {
	if( obj->pIndexData->item_type == ITEM_ARMOR ) {
	  shield = obj;
	} else if( obj->pIndexData->item_type == ITEM_WEAPON ) {
	  secondary = obj;
	}
      } else if( obj->position == WEAR_HELD_R ) {
	wield = obj;
      }
    }

    if( shield )
      secondary = 0;
      }
  */
}


bool uses_claws( char_data *ch )
{
  return ( ch->shdata->race == RACE_LIZARD
	   && !ch->Wearing( WEAR_HANDS ) );
}


int player_round( char_data* ch, char_data* victim )
{
  if( is_set( ch->pcdata->pfile->flags, PLR_PARRY ) )
    return 25;

  obj_data *wield, *secondary, *shield;
  get_wield( ch, wield, secondary, shield );

  const int attack_skills [] = {
    SKILL_SECOND, SKILL_THIRD,
    SKILL_FOURTH, SKILL_FIFTH,
    SKILL_OFFHAND_ATTACK, SKILL_SECOND_OFFHAND
  };

  int attacks = 20;  // Implied first attack at level 10 (counts 2x second, third, ...)

  for( int i = 0; i < 4; ++i )
    attacks += ch->get_skill( attack_skills[i] ); 

  if( secondary ) {
    attacks += ( ch->get_skill( SKILL_OFFHAND_ATTACK )
		 + ch->get_skill( SKILL_SECOND_OFFHAND ) );
    if( number_range( 0, 3 ) == 0 ) {
      wield = secondary;
    }
  }

  int skill, weight;

  if( wield ) {
    int i = ( wield == secondary ) ? 2 : 3;
    int j = is_set( ch->status, STAT_TWO_HAND ) ? 15 : 10;
    weight = max( 0, wield->Weight( 1 )-j*i*ch->Strength( ) ) ;
    skill = WEAPON_FIRST+wield->value[3];
  } else {
    weight = 0;
    skill = WEAPON_UNARMED;
  }

  {
    const int i = number_range( -1, 7 );
    
    if( i == -1 ) {
      ch->improve_skill( skill );
    } else if( i < 4 ||
	       ( i < 6 && wield && wield == secondary ) ) {
      ch->improve_skill( attack_skills[i] );
    }
  }
  
  const char *dt = 0;
  const char *dv = 0;
  int damage = -1;
  int mod = 0;

  if( wield && wield->value[3] != 0 ) {
    dt = attack_noun( wield->value[3] );
    dv = attack_verb( wield->value[3] );
  } else {
    if( ch->get_skill( SKILL_TIGER_PAW ) != UNLEARNT ) {
      const int skill [] = { SKILL_DEMON_SLASH, SKILL_DRAGON_STRIKE,
			     SKILL_EAGLE_CLAW, SKILL_BEAR_GRASP,
			     SKILL_LOCUST_KICK, SKILL_TIGER_PAW };
      const int sides [] = { 16, 14,
			     12, 10,
			     8,  6 };
      for( int i = 0; i < 6; ++i ) {  
	if( number_range( 0, 63-9*i ) < ch->get_skill( skill[i] ) ) {
	  if( number_range( 1, 5 ) != 1 )
	    ch->improve_skill( (int)skill[i] );
	  damage = roll_dice( 2, sides[i] );
	  dt = skill_physical_table[ skill[i] - SKILL_PHY_FIRST ].name;
	  break;
	}
      }
    }
    if( !dt ) {
      int skill = ch->get_skill( SKILL_PUNCH );
      mod = 4*skill - 20;
      if( !wield && uses_claws( ch ) ) {
	dt = "claw";
      } else {
	dt = attack_noun( 0 );
	dv = attack_verb( 0 );
      }
    }
  }
  
  attack( ch, victim, dt, dv, wield, damage, mod, ATT_PHYSICAL );

  skill = ch->get_skill( skill );

  int rate = 5*320 + weight - 25*skill - 25*ch->Dexterity( );
  rate = number_range( rate, 3*rate );
  rate /= 5*attacks;
  
  return rate;
}


/*
 *   SPECIES FIGHT ROUTINES
 */


int mob_round( char_data* ch, char_data* victim )
{
  ch->Show( 1 );

  if( switch_victim( ch, victim ) )
    return 16;

  clear_variables( );
  var_mob = ch;
  var_room = ch->in_room;
  var_ch = victim;
  var_victim = victim;

  ch->species->attack->execute( );

  return 32;
}


bool switch_victim( char_data* ch, char_data* victim )
{
  if( ch->is_affected( AFF_HALLUCINATE )
      && number_range( 0, 9 ) == 0 ) {
    char_data *rch;
    char_array list;
    for( int i = 0; i < *ch->array; ++i ) { 
      if( ( rch = character( ch->array->list[i] ) )
	  && rch->Seen( ch )
	  && can_kill( ch, rch, false ) ) {
	list += rch;
      }
    }

    if( list.is_empty() )
      return false;

    rch = list[ number_range( 0, list.size-1 ) ];

    if( rch == victim )
      return false;

    fsend( victim, "%s, in a daze, stops attacking you and leaps to attack %s.", ch, rch );
    fsend( rch, "%s, in a daze, stops attacking %s and leaps to attack you!", ch, victim );
    fsend_seen( ch, "%s, in a daze, stops attacking %s and leaps to attack %s.",
		ch, victim, rch ); 

    if( !set_fighting( ch, rch ) )
      return false;
    
    return true;
  }

  if( !( victim->pcdata && is_set( victim->pcdata->pfile->flags, PLR_PARRY )
	 || victim->active.time != -1 && is_set( victim->status, STAT_WAITING ) )
      || number_range( 0, 4 ) != 0 )
    return false;
  
  char_data *rch;
  char_array list;
  for( int i = 0; i < *ch->array; ++i ) { 
    if( ( rch = character( ch->array->list[i] ) )
	&& rch != victim
	&& rch->fighting == ch ) {
      list += rch;
    }
  }

  if( list.is_empty() )
    return false;

  rch = list[ number_range( 0, list.size-1 ) ];
  
  fsend( victim, "%s stops attacking you and leaps to attack %s.", ch, rch );
  fsend( rch, "%s stops attacking %s and leaps to attack you!", ch, victim );
  fsend_seen( ch, "%s stops attacking %s and leaps to attack %s.",
	      ch, victim, rch );
  
  if( !set_fighting( ch, rch ) )
    return false;
  
  return true;
}


void add_round( species_data *species, int delay )
{
  if( number_range( 0,32 ) > delay )
    return;
  
  if( species->exp > 1e9
      || species->rounds++ > 1e9
      || species->damage > 1e9
      || species->damage_taken > 1e9 ) {
    species->rounds          /= 2;
    species->damage          /= 2;
    species->damage_taken    /= 2;
    species->shdata->deaths  /= 2;
    species->shdata->kills   /= 2;
    species->exp             /= 2;
    species->special         /= 2;
  }
}


/*
 *   DAMAGE AND ATTACK NAME ROUTINES
 */


int get_damage( char_data* ch, obj_data* wield )
{
  int damage;
  const int skill = ch->species ? 5 : ch->get_skill( SKILL_PUNCH );

  if( wield ) {
    damage = dice_data( wield->value[1] ).roll( );
    if( wield->value[3] != 0 ) {
      // Everything but unarmed weapon class.
      return damage;
    }
  } else if( uses_claws( ch ) ) {
    damage = roll_dice( 1, 6 );
  } else {
    damage = roll_dice( 1, 4 );
  }

  damage += skill/3;

  return damage;
}


void damage_modifiers( char_data* ch, char_data* victim, obj_data* wield,
		       int& damage )
{
  if( is_focusing( ch ) ) {
    if( ch->mana > 0 ) {
      --ch->mana;
    }
    if( ch->mana <= 0 ) {
      remove_bit( ch->status, STAT_FOCUS );
      send_color( ch, COLOR_SKILL,
		  "Your senses return to normal as you lose your battle focus." );
      send( ch, "\n\r" );
    }
  }

  damage += (int) ch->Damroll( wield );
  
  if( wield ) {
    if( is_set( wield->extra_flags, OFLAG_GOOD ) && is_evil( victim )
	|| is_set( wield->extra_flags, OFLAG_EVIL ) && is_good( victim ) ) {
      damage += 2;
    } else if( is_set( wield->extra_flags, OFLAG_GOOD ) && is_good( victim )
	|| is_set( wield->extra_flags, OFLAG_EVIL ) && is_evil( victim )) {
      damage -= 2;
    }
    if( is_set( wield->extra_flags, OFLAG_LAWFUL ) && is_chaotic( victim )
	|| is_set( wield->extra_flags, OFLAG_CHAOTIC ) && is_lawful( victim ) ) {
      damage += 2;
    } else if( is_set( wield->extra_flags, OFLAG_LAWFUL ) && is_lawful( victim )
	|| is_set( wield->extra_flags, OFLAG_CHAOTIC ) && is_chaotic( victim ) ) {
      damage -= 2;
    }
  }

  if( ch->move > 0 && is_berserk( ch ) ) {
    --ch->move;
    damage += ( ch->get_skill( SKILL_BERSERK ) + 2 ) / 2;
    const int skill = ch->get_skill( SKILL_FRENZY );
    if( skill != UNLEARNT
	&& number_range( 1, 4 ) == 4 ) {
      fsend_color( ch, COLOR_SKILL, "You attack with frenzy!" );
      fsend_color_seen( ch, COLOR_SKILL, "%s attacks with frenzy!", ch );
      ch->improve_skill( SKILL_FRENZY );
      damage += ( skill + 2 ) / 2;
    }
  }
  
  if( victim->position == POS_RESTING )
    damage = 3*damage/2;
  else if( victim->position <= POS_SLEEPING )
    damage *= 4;
}


static int proficiency( char_data *ch, obj_data *obj )
{
#define WEAPON_AFFS 11

  const int weapon_affs [WEAPON_AFFS] = {
    AFF_UNARMED_PROF,
    AFF_DAGGER_PROF,
    AFF_SWORD_PROF,
    AFF_CLUB_PROF,
    AFF_STAFF_PROF,
    AFF_POLEARM_PROF,
    AFF_MACE_PROF,
    AFF_WHIP_PROF,
    AFF_AXE_PROF,
    AFF_BOW_PROF,
    AFF_SPEAR_PROF
  };

  if( obj && obj->pIndexData->item_type != ITEM_WEAPON ) {
    return 0;
  }

  int x, y;
  if( !obj ) {
    x = 0;
    y = 0;
  } else {
    x = obj->value[0];
    y = obj->value[3];
  }
  
  if ( y >= 0 && y < WEAPON_AFFS ) {
    if ( ch->is_affected( weapon_affs[y] ) ) {
      ++x;
    }
  }

  return x;

#undef WEAPON_AFFS
}


double char_data :: Hitroll( obj_data* obj )
{
  int x = 3*hitroll + Dexterity( ) - 9;

  x += 3*proficiency( this, obj );

  if( mount ) {
    const int smf = get_skill( SKILL_MOUNTED_FIGHTING );
    if( smf != UNLEARNT ) {
      // +0.0 at level 1, +3.0 at level 10.
      x += ( smf - 1 );
    }
  }

  if( is_focusing( this ) ) {
    // +0.33 at level 1, +3.33 at level 10.
    x += get_skill( SKILL_FOCUS );
  }

  return (double)x / 3.0;
}


double char_data :: Damroll( obj_data* obj )
{
  int x = 3*damroll + Strength( ) - 12;

  x += 3*proficiency( this, obj );

  if( mount ) {
    const int smf = get_skill( SKILL_MOUNTED_FIGHTING );
    if( smf != UNLEARNT ) {
      // +0.33 at level 1, +2.0 at level 10.
      x += ( smf + 2 ) / 2;
    }
  }

  if( is_focusing( this ) ) {
    // +0.33 at level 1, +2.0 at level 10.
    x += ( get_skill( SKILL_FOCUS ) + 2 ) / 2;
  }

  return (double)x / 3.0;
}


/*
 *   MAIN BATTLE ROUTINES
 */


// Similar to damage.cc:inflict()... wake a sleeping victim even if no damage.
static void wakey( char_data *victim )
{
  if( victim->position == POS_SLEEPING ) {
    const bool slept = victim->is_affected( AFF_SLEEP );
    if( slept ) {
      // Prevent immediate standing up when whacked.
      remove_bit( victim->status, STAT_HOLD_POS );
      victim->position = POS_RESTING;
      strip_affect( victim, AFF_SLEEP );
      victim->position = POS_SLEEPING;
    }
    if( victim->position == POS_SLEEPING ) {
      send( victim, "You wake up.\n\r" );
      fsend_seen( victim, "%s wakes up.", victim );
      if( deep_water( victim ) ) {
	victim->position = POS_STANDING;
      } else {
	victim->position = POS_RESTING;
      }
      remove_bit( victim->status, STAT_HOLD_POS );
      renter_combat( victim );
    }
  }
}


bool attack( char_data* ch, char_data* victim, const char* dt, const char *dv,
	     obj_data* wield, int damage, int modifier, int type )
{
  if( ch->position < POS_FIGHTING )
    return false;

  bool backstab = false;
  bool charge = false;
  bool garrote = false;
  bool no_miss = false;

  if( type == ATT_BACKSTAB ) {
    backstab = no_miss = true;
    type = ATT_PHYSICAL;
  } else if( type == ATT_CHARGE ) {
    charge = no_miss = true;
    type = ATT_PHYSICAL;
  } else if( type == ATT_GARROTE ) {
    garrote = no_miss = true;
    type = ATT_PHYSICAL;
  }

  bool plural = false;

  if( *dt == '+' ) {
    plural = true;
    ++dt;
  }

  if( !dv )
    dv = dt;

  int roll = number_range( -250, 250 ) + modifier;

  modify_roll( ch, victim, wield, roll );

  const bool active = can_defend( victim, ch );

  if( !no_miss
      && type == ATT_PHYSICAL
      && misses_blindly( ch, victim, roll, dt, plural ) )
    return false;

  if( active
      && ( dodge( ch, victim, roll, no_miss )
	   || tumble( ch, victim, roll, no_miss ) ) )
    return false;
  
  if( number_range( 1, 10 ) == 1 ) {
    ch->improve_skill( SKILL_BLIND_FIGHTING );
    if( ch->mount ) {
      ch->improve_skill( SKILL_MOUNTED_FIGHTING );
    }
  }

  obj_data *v_wield, *v_secondary, *v_shield;
  get_wield( victim, v_wield, v_secondary, v_shield );

  if( type == ATT_PHYSICAL ) {
    if( fire_shield( ch, victim )
	|| ice_shield( ch, victim )
	|| ion_shield( ch, victim )
	|| thorn_shield( ch, victim )
	|| damage_weapon( ch, wield ) )
      return false;
    
    if( active
	&& !no_miss
	&& ( parry( ch, victim, wield, v_wield, v_secondary, roll )
	     || guard( ch, victim, v_wield, roll )
	     || block_shield( ch, victim, wield, v_shield, dt, plural ) ) )
      return false;

    if( !no_miss
	&& trip( ch, victim, wield ) )
      return false;
  }
  
  if( active
      && !no_miss
      && shadow_dance( ch, victim, dt, plural ) )
    return false;

  if( damage < 0 ) {
    damage = get_damage( ch, wield );
  }

  if( backstab )
    damage *= backstab_damage( ch );
  else if( charge )
    damage *= charge_damage( ch );
  else if( garrote )
    damage *= garrote_damage( ch );

  damage_modifiers( ch, victim, wield, damage );
  damage = max( 1, damage );

  if( victim->species
      && victim->hit > 0
      && !is_set( victim->status, STAT_PET ) )
    victim->species->damage_taken += min( victim->hit, damage );

  if( ch->species
      && !is_set( ch->status, STAT_PET ) ) {
    ch->species->damage += damage/2;
  }

  switch( type ) {
  case ATT_PHYSICAL:
    {
      const char *loc_name = empty_string;
      critical_hit( ch, victim, wield, damage );
      power_strike( ch, victim, wield, damage );
      add_percent_average( damage, -victim->Save_Physical( ch ) );
      if( damage > 0 &&
	  ( absorb_magic( victim, ch, dt, plural, damage )
	    || absorb_armor( victim, ch, dt, plural, dv, damage, loc_name, garrote ) ) )
	return false;
      if( damage <= 0
	  || no_miss
	  || !gouge_attack( ch, victim, damage ) ) {
	dam_local( victim, ch, damage, dt, plural, loc_name );
      }
      if( !inflict( victim, ch, damage, "" ) 
	  && !flaming_weapon( ch, victim, wield ) ) { 
	stun( ch, victim, dt, plural ); 
	trigger_hit( ch, victim, wield );
      }
    }
    break;
    
  case ATT_ACID:
    damage_acid( victim, ch, damage, dt, plural );
    break;
    
  case ATT_COLD:
    damage_cold( victim, ch, damage, dt, plural );
    break;
    
  case ATT_SHOCK:
    damage_shock( victim, ch, damage, dt, plural );
    break;
    
  case ATT_FIRE:
    damage_fire( victim, ch, damage, dt, plural );
    break;
    
  case ATT_MAGIC:
    damage_magic( victim, ch, damage, dt, plural );
    break;
    
  case ATT_MIND:
    damage_mind( victim, ch, damage, dt, plural );
    break;
    
  case ATT_SOUND:
    damage_sound( victim, ch, damage, dt, plural );
    break;
    
  default:
    bug( "Attack: Impossible attack type." );
    break;
  }
  
  if( ch->species
      && !is_set( ch->status, STAT_PET ) )
    ch->species->damage -= damage/2;

  return( damage > 0 );
}


void modify_roll( char_data* ch, char_data* victim, obj_data* wield,
		  int& roll )
{
  const int weapon = ( wield ? wield->value[3] : 0 );

  if( is_focusing( victim ) )
    roll -= ( victim->get_skill( SKILL_FOCUS ) + 1 ) / 2;

  roll -= victim->mod_armor;
  roll += ( ch->Level( )
	    + (int) ( 5.0*ch->Hitroll( wield ) )
	    + 8*ch->get_skill( WEAPON_UNARMED+weapon )
	    + 3*ch->get_skill( SKILL_BLIND_FIGHTING ) );
  
  if( victim->species 
      && victim->shdata->race == RACE_UNDEAD 
      && victim->in_room
      && victim->in_room->Light() > 30 )
    roll += 20;
  
  if( !victim->fighting ) {
    roll += 50;
  }
  
  if( victim->position <= POS_SLEEPING ) {
    roll += 100;
  }
}


/*
 *   WEAPON ROUTINES
 */


bool damage_weapon( char_data* ch, obj_data*& wield )
{
  if( !wield
      || number_range( 0, 15-4*wield->rust ) > !is_set( wield->extra_flags, OFLAG_SANCT )
      || number_range( 1, 4+wield->value[0] ) > 4
      || --wield->condition > 0 )
    return false;
  
  fsend( ch, "%s you are wielding shatters into pieces.", wield );
  fsend_seen( ch, "%s which %s is wielding shatters into pieces.",
	      wield, ch );

  wield->Extract( );
  wield = 0;

  return true;
}


bool gouge_attack( char_data *ch, char_data *victim, int& damage )
{
  const int skill = ch->get_skill( SKILL_EYE_GOUGE );

  if( skill == UNLEARNT
      || ch->is_affected( AFF_HALLUCINATE )
      || !victim->Seen( ch )
      || victim->species && !is_set( victim->species->act_flags, ACT_HAS_EYES )
      || victim->is_affected( AFF_BLIND )
      || skill + ( ch->Dexterity( ) - victim->Dexterity( ) )/ 2 < number_range( 1, 250 ) ) {
    return false;
  }

  fsend_color( ch, COLOR_SKILL, "You gouge %s in the eye!", victim );
  fsend_color( victim, COLOR_SKILL, "%s gouges you in the eye!", ch );
  fsend_color_seen( ch, COLOR_SKILL, "%s gouges %s in the eye!", ch, victim );
  
  ch->improve_skill( SKILL_EYE_GOUGE );

  dam_local( victim, ch, damage, "gouge", false, "eyes" );

  affect_data affect;
  affect.type = AFF_BLIND;
  affect.duration = 1 + skill/2;
  affect.level = skill;
  add_affect( victim, &affect );
  
  // damage *= ???

  disrupt_spell( victim ); 
  set_min_delay( victim, 10 );

  return true;
}


void trigger_hit( char_data* ch, char_data* victim, obj_data* wield )
{
  if( !wield || ch->fighting != victim )
    return;

  for( oprog_data *oprog = wield->pIndexData->oprog; oprog; oprog = oprog->next ) {
    if( oprog->trigger == OPROG_TRIGGER_HIT ) { 
      wield->Select( 1 );
      push( );
      clear_variables( );
      var_ch     = ch;
      var_victim = victim;
      var_obj    = wield;    
      var_room   = ch->in_room;
      oprog->execute( );
      pop( );
    }
  }
}


bool flaming_weapon( char_data *ch, char_data *victim, obj_data *wield )
{
  if( !wield
      || !is_set( wield->extra_flags, OFLAG_FLAMING )
      || is_submerged( victim )
      || is_submerged( ch ) )
    return false;

  const int dam = roll_dice( 2, 4 );

  return damage_fire( victim, ch, dam, "flames", true );
}


/* 
 *   ARMOR ROUTINES
 */


bool absorb_armor( char_data* victim, char_data* ch,
		   const char* dt, bool plural, const char *dv,
		   int& damage, const char*& loc_name, bool garrote )
{
  int  absorbed;
  int         i;
  int       loc;

  const int roll = number_range( 0, 999 );

  if( victim->is_humanoid( )
      || garrote ) {

    if( garrote ) {
      loc = WEAR_NECK;
    } else {
      // Note: no left_hand or right_hand, otherwise weapons/shields
      // would get damaged when hit.
      if( roll < 400 )  loc = WEAR_BODY;		// 40%
      else if( roll < 450 )  loc = WEAR_NECK;		//  5%
      else if( roll < 600 )  loc = WEAR_HEAD;		// 15%
      else if( roll < 700 )  loc = WEAR_LEGS;		// 10%
      else if( roll < 750 )  loc = WEAR_FEET;		//  5%
      else if( roll < 830 )  loc = WEAR_HANDS;		//  8%
      else if( roll < 930 )  loc = WEAR_ARMS;		// 10%
      else if( roll < 940 )  loc = WEAR_WRIST_L;	//  1%
      else if( roll < 950 )  loc = WEAR_WRIST_R;	//  1%
      else if( roll < 990 )  loc = WEAR_WAIST;		//  4%
      else if( roll < 995 )  loc = WEAR_FINGER_L;	//  0.5%
      else                   loc = WEAR_FINGER_R;	//  0.5%
    }

    int global;
    absorbed = damage_armor( ch, victim, loc, global );
    damage -= absorbed;

    if( damage <= 0 ) {
      spam_char( victim, "Your %s armor absorbs %s's %s.",
		 dam_loc_name[loc], ch, dt );
      spam_char( ch, "%s's %s armor absorbs your %s.",
		 victim, dam_loc_name[loc], dt );
      spam_room( ch, "%s's %s armor absorbs %s's %s.",
		 victim, dam_loc_name[loc], ch, dt );
      wakey( victim );
      return true;
    }

    // Global AC.
    if( global > 0 && global > absorbed ) {
      if( ( damage -= number_range( 0, global - absorbed ) ) <= 0 ) {
	const char *const s = ( plural ? "" : "s" );
	const char *const es = ( plural ? "" : "es" );
	spam_char( victim,
		   "%s's %s seem%s to hit you, but do%s no damage.", 
		   ch, dt, s, es );
	spam_char( ch,
		   "Your %s seem%s to hit %s, but do%s no damage.", 
		   dt, s, victim, es );
	spam_room( ch, "%s's %s seem%s to hit %s, but do%s no damage.",
		   ch, dt, s, victim, es );
	wakey( victim );
	return true;
      }
    }

    //    if( absorb_bracers( victim, ch, damage, absorbed, dt, plural ) )
    //      return true;
    
    loc_name = dam_loc_name[loc];
    
    return false;
  }
  
  for( i = 0; i < MAX_ARMOR-1; i++ ) 
    if( roll < victim->species->chance[i] )
       break;

  if( victim->species->armor[i] < 0 ) {
    damage += number_range( 0, -victim->species->armor[i] );
  } else {
    damage -= number_range( 0, victim->species->armor[i] );
  }
  
  if( damage > 0 ) {
    loc_name = victim->species->part_name[i];
    return false;
  }

  if( *victim->species->part_name[i] ) {
    spam_char( victim, "Your %s absorbs %s's attack.",
	       victim->species->part_name[i], ch );
    spam_char( ch, "You %s %s's %s inflicting no damage.",
	       dv, victim, victim->species->part_name[i] );
    spam_room( ch, "%s's %s absorbs %s's %s.",
	       victim, victim->species->part_name[i], ch, dt );
  } else {
    spam_char( victim, "Your armor absorbs %s's attack.", ch );
    spam_char( ch, "You %s %s inflicting no damage.",
	       dv, victim );
    spam_room( ch, "%s's armor absorbs %s's attack.", victim, ch );
  }

  wakey( victim );
  return true;
}


int damage_armor( char_data *ch, char_data* victim, int loc, int& global, obj_data *obj )
{
  int absorbed = 0;
  global = 0;

  for( int i = victim->wearing-1; i >= 0; i-- ) {
    obj_data *armor = object( victim->wearing[i] );
    bool is_armor = ( armor->pIndexData->item_type == ITEM_ARMOR );
    if( is_armor ) {
      global += armor->value[2];
    }
    if( obj && armor != obj ) {
      // Prevent other left-hand items from damage if holding a shield.
      continue;
    }
    if( armor->position == loc ) {
      int enchant = 0;
      if( is_armor ) {
	enchant = armor->value[0];
	int val = armor_class( armor );
	if( val < 0 ) {
	  absorbed -= number_range( 0, -val );
	} else {
	  absorbed += number_range( 0, val );
	}
	if( is_set( armor->extra_flags, OFLAG_GOOD ) && is_evil( ch )
	    || is_set( armor->extra_flags, OFLAG_EVIL ) && is_good( ch ) ) {
	  absorbed += 2;
	} else if( is_set( armor->extra_flags, OFLAG_GOOD ) && is_good( ch )
		   || is_set( armor->extra_flags, OFLAG_EVIL ) && is_evil( ch )) {
	  absorbed -= 2;
	}
	if( is_set( armor->extra_flags, OFLAG_LAWFUL ) && is_chaotic( ch )
	    || is_set( armor->extra_flags, OFLAG_CHAOTIC ) && is_lawful( ch ) ) {
	  absorbed += 2;
	} else if( is_set( armor->extra_flags, OFLAG_LAWFUL ) && is_lawful( ch )
		   || is_set( armor->extra_flags, OFLAG_CHAOTIC ) && is_chaotic( ch ) ) {
	  absorbed -= 2;
	}
      }
      if( number_range( 0, 15-4*armor->rust ) <= (int) !is_set( armor->extra_flags, OFLAG_SANCT )
	  && number_range( 1,4+enchant ) <= 4
	  && --armor->condition < 0 ) {
        fsend_color( victim, COLOR_MILD, "%s you are wearing shatters into pieces.", armor );
        fsend_seen( victim, "%s which %s is wearing shatters into pieces.",
		    armor, victim );
	if( !armor->contents.is_empty() ) {
	  for( int i = 0; i < *victim->array; ++i ) {
	    char_data *rch;
	    if( !( rch = character( victim->array->list[i] ) )
		|| !rch->link
		|| !victim->Seen( rch ) )
	      continue;
	    
	    select( armor->contents, rch );
	    rehash( rch, armor->contents );
	    
	    if( !none_shown( armor->contents ) ) {
	      const char *drop = rch->in_room->drop( );
	      if( thing_data *thing = one_shown( armor->contents ) ) {
		if( *drop ) {
		  fsend( rch, "%s fall%s %s.",
			 thing,
			 thing->Shown( ) == 1 ? "s" : "",
			 drop );
		} else {
		  fsend( rch, "%s fall%s out.",
			 thing,
			 thing->Shown( ) == 1 ? "s" : "" );
		}
	      } else {
		if( *drop ) {
		  fsend( rch, "Some items fall %s.", drop );
		} else {
		  fsend( rch, "Some items fall out." );
		}
	      }
	    }
	  }
	  armor->contents.To( *victim->array );
	}
        armor->Extract( );
      }
    }
  }
  
  return absorbed;
}


/*
 *   ATTACK ROUTINES
 */


void critical_hit( char_data* ch, char_data* victim, obj_data* wield,
		   int& damage )
{
  const int skill1 = ch->get_skill( SKILL_CRITICAL_HIT );

  if( skill1 < 1 )
    return;

  const int weapon = ( wield ? wield->value[3] : 0);

  if( number_range( 1,50 ) > ch->get_skill( WEAPON_UNARMED+weapon ) )
    return; 

  const int skill2 = ch->get_skill( SKILL_DEATH_STRIKE );

  if( skill2 > 0
      && number_range( 0, 30 ) < skill2 ) {
    fsend_color( ch, COLOR_SKILL, "You call upon the shadows to destroy %s!", victim );
    fsend_color( victim, COLOR_SKILL, "%s calls upon the shadows to destroy you!", ch );
    fsend_color_seen( ch, COLOR_SKILL, "%s calls upon the shadows to destroy %s!",
		      ch, victim );
    ch->improve_skill( SKILL_DEATH_STRIKE );
    damage *= 8;
  } else if( number_range( 0, 20 ) < skill1 ) {
    fsend_color( ch, COLOR_SKILL, "You critically hit %s!", victim );
    fsend_color( victim, COLOR_SKILL, "%s critically hits you!", ch );
    fsend_color_seen( ch, COLOR_SKILL, "%s critically hits %s!", ch, victim );
    ch->improve_skill( SKILL_CRITICAL_HIT );
    damage *= 5;
  }
}


void power_strike( char_data* ch, char_data* victim, obj_data* wield,
		   int& damage )
{
  const int skill = ch->get_skill( SKILL_POWER_STRIKE );

  if( skill < 1 )
    return;

  const int weapon = ( wield ? wield->value[3] : 0 );
  
  if( number_range( 1,75 ) > ch->get_skill( WEAPON_UNARMED+weapon ) )
    return;
  
  if( number_range( 0, 25 ) < skill ) {
    fsend_color( ch, COLOR_SKILL, "Your power strike hits %s!", victim );
    fsend_color( victim, COLOR_SKILL, "%s hits you with a power strike!", ch );
    fsend_color_seen( ch, COLOR_SKILL, "%s hits %s with a power strike!", ch, victim );
    ch->improve_skill( SKILL_POWER_STRIKE );
    damage *= 3;
  }
}


/*
 *   OFFENSIVE ROUTINES
 */


static bool can_stun( char_data *ch, char_data *victim )
{
  if( victim->position <= POS_STUNNED
      || victim->shdata->race == RACE_PLANT )
    return false;

  return true;
}


static void stun_effect( char_data *ch, char_data *victim, const char *dt, bool plural )
{
  const char *const s = ( plural ? "" : "s" );

  if( victim->Seen( ch ) ) 
    fsend_color( ch, COLOR_SKILL, "-* Your %s momentarily stun%s %s! *-",
		 dt, s, victim );
  
  fsend_color( victim, COLOR_SKILL, "%s's %s momentarily stun%s you!",
	       ch, dt, s );

  fsend_color_seen( victim, COLOR_SKILL, "%s's %s momentarily stun%s %s.",
		    ch, dt, s, victim );

  disrupt_spell( victim );
  set_bit( victim->status, STAT_STUNNED );
  set_min_delay( victim, 40 );
}


void stun( char_data* ch, char_data* victim, const char* dt, bool plural )
{
  const int skill = ch->get_skill( SKILL_STUN );
  
  if( skill == UNLEARNT )
    return;

  if( !can_stun( ch, victim ) )
    return;
  
  if( skill <= number_range( 0, 150+6*victim->Level() ) ) 
    return;
  
  stun_effect( ch, victim, dt, plural );

  ch->improve_skill( SKILL_STUN );  
}


bool trip( char_data* ch, char_data* victim, obj_data* wield )
{
  if( ch->mount || ch->rider )
    return false;

  if( victim->mount ) {
    victim = victim->mount;
  }

  if( victim->position < POS_FIGHTING )
    return false;

  // If these were allowed, the victim would leap to his feet after trip,
  // which he couldn't.
  if( is_entangled( victim )
      || is_set( victim->status, STAT_STUNNED ) )
    return false;

  if( deep_water( ch )
      || deep_water( victim ) )
    return false;

  if( victim->species &&
      ( is_set( victim->species->act_flags, ACT_GHOST )
  	|| is_set( victim->species->act_flags, ACT_CAN_FLY )
	|| is_set( victim->species->act_flags, ACT_NO_TRIP ) ) )
    return false;
  
  const int skill1 = ch->get_skill( SKILL_TRIP );

  if( !ch->species ) {
    const int skill2 = ch->get_skill( SKILL_KICK );
    if( skill2 != 10 &&
	skill1 == UNLEARNT )
      return false;
    if( number_range( 0, 500 ) > 2*skill1 + skill2/2 )
      return false; 
  } else {
    //    if( !ch->is_humanoid( )
    //	|| is_set( ch->species->act_flags, ACT_GHOST )  )
    //      return false;
    if( skill1 < 1
	|| number_range( 0, 200 ) > skill1 )
      return false;
  }
  
  const char *drop = ch->in_room->drop( );

  if( !victim->rider ) {
    if( *drop ) {
      fsend_color( ch, COLOR_SKILL,
		   "With a well timed kick you knock %s down %s!",
		   victim, drop );
      fsend_color( victim, COLOR_SKILL,
		   "%s delivers a quick kick, knocking you down %s!",
		   ch, drop );
      fsend_color_seen( victim, COLOR_SKILL,
			"%s kicks %s, knocking %s down %s.",
			ch, victim, victim->Him_Her( ), drop );
    } else {
      fsend_color( ch, COLOR_SKILL,
		   "With a well timed kick you knock %s down!",
		   victim );
      fsend_color( victim, COLOR_SKILL,
		   "%s delivers a quick kick, knocking you down!",
		   ch );
      fsend_color_seen( victim, COLOR_SKILL,
			"%s kicks %s, knocking %s down.",
			ch, victim, victim->Him_Her( ) );
    }
  } else {
    if( *drop ) {
      fsend_color( ch, COLOR_SKILL,
		   "With a well timed kick you knock %s down %s, taking %s with %s!",
		   victim, drop, victim->rider, victim->Him_Her( ) );
      fsend_color( victim, COLOR_SKILL,
		   "%s delivers a quick kick, knocking you down %s and taking %s with you!",
		   ch, drop, victim->rider );
      fsend_color_seen( victim, COLOR_SKILL,
			"%s kicks %s, knocking %s down %s and taking %s with %s.",
			ch, victim, victim->Him_Her( ), drop,
			victim->rider, victim->Him_Her( ) );
      fsend_color( victim->rider, COLOR_SKILL,
		   "%s kicks %s, knocking %s down %s and taking you with %s!",
		   ch, victim, victim->Him_Her( ), drop,
		   victim->Him_Her( ) );
    } else {
      fsend_color( ch, COLOR_SKILL,
		   "With a well timed kick you knock %s down, taking %s with %s!",
		   victim, victim->rider, victim->Him_Her( ) );
      fsend_color( victim, COLOR_SKILL,
		   "%s delivers a quick kick, knocking you down and taking %s with you!",
		   ch, victim->rider );
      fsend_color_seen( victim, COLOR_SKILL,
			"%s kicks %s, knocking %s down and taking %s with %s.",
			ch, victim, victim->Him_Her( ),
			victim->rider, victim->Him_Her( ) );
      fsend_color( victim->rider, COLOR_SKILL,
		   "%s kicks %s, knocking %s down and taking you with %s!",
		   ch, victim, victim->Him_Her( ),
		   victim->Him_Her( ) );
    }
    dismount( victim->rider, POS_RESTING );
    record_damage( victim->rider, ch );
  }

  ch->improve_skill( SKILL_TRIP );

  record_damage( victim, ch );
  disrupt_spell( victim );
  
  victim->position = POS_RESTING;
  
  return true;
} 


int garrote_damage( char_data* ch )
{
  const int skill = ch->get_skill( SKILL_GARROTE );

  const int i = 10*skill
    + ch->Level( )/2
    + ch->Strength( );

  return 1+i/25;
}


int charge_damage( char_data* ch )
{
  const int skill = ch->get_skill( SKILL_CHARGE );

  const int i = 10*skill
    + ch->Level( )/2
    + ch->Strength( );

  return 1+i/25;
}

int backstab_damage( char_data* ch )
{
  const int skill1 = ch->get_skill( SKILL_BACKSTAB );
  const int skill2 = ch->get_skill( SKILL_ASSASSINATE );

  const int i = 10*skill1
    + 10*skill2
    + ch->Level( );
  
  return 1+i/25;
}


/*
 *   DEFENSIVE ROUTINES
 */

/*
bool absorb_bracers( char_data* victim, char_data* ch, int& damage,
		     int armor, const char* dt, bool plural )
{
  char           tmp  [ MAX_INPUT_LENGTH ];
  obj_data*  bracers;
  
  if( ( bracers = victim->Wearing( WEAR_WRIST_L, LAYER_BASE ) )
      && bracers->pIndexData->item_type == ITEM_ARMOR ) {
    armor -= bracers->value[2];
  }
  
  if( ( bracers = victim->Wearing( WEAR_WRIST_R, LAYER_BASE ) )
      && bracers->pIndexData->item_type == ITEM_ARMOR ) {
    armor -= bracers->value[2];
  }
  
  if( armor >= 0
      || ( damage -= number_range( 0, -armor ) ) > 0 )
    return false;

  const char *const s = ( plural ? "are" : "is" );

  spam_char( victim,
	     "Your bracers glow briefly and %s's %s %s deflected.", 
	     ch, dt, s );
  
  spam_char( ch,
	     "%s's bracers glow briefly and your %s %s magically deflected.",
	     victim, dt, s );
  
  sprintf( tmp, "%%s's bracers glow briefly and %%s's %s %s deflected.", dt, s );
  spam_room( tmp, victim, ch );
  
  return true;
}
*/


bool absorb_magic( char_data* victim, char_data* ch, const char* dt, bool plural,
		   int& damage )
{
  if( !victim->is_affected( AFF_PROTECT ) )
    return false;

  damage -= number_range( 0, 3 );
  
  if( damage > 0 ) 
    return false;
  
  const char *const s = ( plural ? "are" : "is" );

  spam_char( victim,
	     "The air crackles as %s's %s %s mysteriously blocked.",
	     ch, dt, s );

  spam_char( ch,
	     "The air around %s crackles as your %s %s mysteriously blocked.",
	     victim, dt, s );
  
  spam_room( ch, "The air crackles as %s's %s %s mysteriously blocked.",
	     ch, dt, s );

  
  wakey( victim );
  return true;
}


bool ion_shield( char_data* ch, char_data* victim )
{
  if( victim->is_affected( AFF_ION_SHIELD ) ) {
    int d = spell_damage( SPELL_ION_SHIELD, affect_level( victim, AFF_ION_SHIELD ) );
    damage_shock( ch, victim, d, "electric spark" );
    if( ch->hit <= 0 )
      return true;
  }

  return false;
}


bool thorn_shield( char_data* ch, char_data* victim )
{
  if( victim->is_affected( AFF_THORN_SHIELD ) ) {
    int d = spell_damage( SPELL_THORN_SHIELD, affect_level( victim, AFF_THORN_SHIELD ) );
    damage_acid( ch, victim, d, "thorn shield" );
    if( ch->hit <= 0 )
      return true;
    }

  return false;
}


bool fire_shield( char_data* ch, char_data* victim ) 
{
  if( victim->is_affected( AFF_FIRE_SHIELD ) ) {
    int d = spell_damage( SPELL_FIRE_SHIELD, affect_level( victim, AFF_FIRE_SHIELD ) );
    damage_fire( ch, victim, d, "fire shield" );
    if( ch->hit <= 0 )
      return true;
    }

  return false;
}


bool ice_shield( char_data* ch, char_data* victim ) 
{
  if( victim->is_affected( AFF_ICE_SHIELD ) ) {
    int d = spell_damage( SPELL_ICE_SHIELD, affect_level( victim, AFF_ICE_SHIELD ) );
    damage_cold( ch, victim, d, "ice shield" );
    if( ch->hit <= 0 )
      return true;
    }

  return false;
}


// wield and shield are victim's.
bool block_shield( char_data* ch, char_data* victim,
		   obj_data *wield, obj_data *shield, const char* dt, bool plural )
{
  if( !shield )
    return false;
  
  int roll = number_range( 1, 100 );

  int mount = 0;
  if( victim->mount ) {
    const int smf = victim->get_skill( SKILL_MOUNTED_FIGHTING );
    if( smf != UNLEARNT ) {
      mount = ( smf + 4 )/5;	// +1 at level 1, +2 at level 6.
    }
  }

  if( roll > 25
      || roll > ( shield->value[1] + shield->value[0] + 1 )/2
         + victim->get_skill( SKILL_SHIELD_BLOCK )
         + victim->get_skill( SKILL_SHIELD_STRIKE )/2
         + mount )
    return false;
  
  const char *name = shield->pIndexData->Name( 1, true,
					       is_set( shield->extra_flags, OFLAG_IDENTIFIED ) );
  spam_char( victim, "You block %s's %s with your %s.", ch, dt, name );
  spam_char( ch, "%s blocks your %s with %s %s.", victim, dt,
	     victim->His_Her( ch ), name );
  spam_room( ch, "%s blocks %s's %s with %s %s.",
	     victim, ch, dt, victim->His_Her( ), name );

  int global;
  damage_armor( ch, victim, WEAR_HELD_L, global, shield );

  victim->improve_skill( SKILL_SHIELD_BLOCK ); 

  const int skill = victim->get_skill( SKILL_SHIELD_STRIKE );
  const int dex = victim->Dexterity( );
  const int str = victim->Strength( );
  const int check = min( 60, 4*skill + dex + str );

  if( skill == UNLEARNT
      || ( !victim->species && is_set( victim->pcdata->pfile->flags, PLR_PARRY ) )
      || ( victim->active.time != -1 && is_set( victim->status, STAT_WAITING ) )
      || ( roll = number_range( 1, 100 ) ) > check )
    return true;

  const int skill_bash = victim->get_skill( SKILL_BASH );
  const int skill_disarm = victim->get_skill( SKILL_DISARM );

  const bool ok_bash = ( roll < check/3 )
    && skill_bash != UNLEARNT
    && can_bash( victim, ch, false );
  const bool ok_disarm = ( roll < check/2 )
    && skill_disarm != UNLEARNT
    && wield
    && can_disarm( victim, ch, wield, false );

  if( ok_bash ) {
    roll = number_range( 0, 22 )
      - 2*( ch->Size( ) - victim->Size( ) ) 
      - ( ch->Dexterity() - dex )/2
      - ( ch->Strength() - str )/2
      - ( ch->Level() - victim->Level() )/10
      + skill/2 + victim->get_skill( SKILL_BASH )/2;
    
    if( roll > 20 ) {
      if( ch->mount ) {
	// Dismount.
	fsend_color( victim, COLOR_SKILL, "You ram your %s into %s, sending %s tumbling from %s mount!",
		     name, ch, ch->Him_Her( victim ), ch->His_Her( victim ) );
	fsend_color( ch, COLOR_SKILL, "%s rams %s %s into you, sending you tumbling from your mount!",
		     victim, victim->His_Her( ch ), name );
	fsend_color_seen( victim, COLOR_SKILL, "%s rams %s %s into %s, sending %s tumbling from %s mount!",
		     victim, victim->His_Her( ), name, ch, ch->Him_Her( ), ch->His_Her( ) );
	dismount( ch, POS_RESTING );
      } else {
	// Bash.
	fsend_color( victim, COLOR_SKILL, "You ram your %s into %s, sending %s sprawling!",
		     name, ch, ch->Him_Her( victim ) );
	fsend_color( ch, COLOR_SKILL, "%s rams %s %s into you, sending you sprawling!",
		     victim, victim->His_Her( ch ), name );
	fsend_color_seen( victim, COLOR_SKILL, "%s rams %s %s into %s, sending %s sprawling!",
			  victim, victim->His_Her( ), name, ch, ch->Him_Her( ) );
	ch->position = POS_RESTING;
      }
      disrupt_spell( ch ); 
      set_delay( ch, 32 );
      victim->improve_skill( SKILL_SHIELD_STRIKE ); 
      return true;
    }
  }

  if( ok_disarm ) {
    const int fail = is_set( ch->status, STAT_TWO_HAND ) ? -65 : -55;
    roll = fail + 4*( dex - ch->Dexterity() );
    const bool easy = roll > 0 && number_range( 1, 100 ) <= roll;
    const bool hard = roll < 0 && number_range( 1, 100 ) <= -roll;

    if( easy
	|| ( !hard
	     && number_range( 1, 20 ) <= 2 * ( skill + victim->get_skill( SKILL_DISARM ) )/2 - 1 ) ) {
      name = wield->pIndexData->Name( 1, true,
				       is_set( wield->extra_flags, OFLAG_IDENTIFIED ) );
      fsend_color( victim, COLOR_SKILL,
		   "You thrust your shield against %s's %s, sending it flying!",
		   ch, name );
      fsend_color( ch, COLOR_SKILL, "%s thrusts %s shield against your %s, sending it flying!",
		   victim, victim->His_Her( ), name );
      fsend_color_seen( victim, COLOR_SKILL, "%s thrusts %s shield against %s's %s, sending it flying!",
			victim, victim->His_Her( ), ch, name );
      if( ( wield = object( wield->From( 1 ) ) ) ) {
	wield->To( ch );
      }
      victim->improve_skill( SKILL_SHIELD_STRIKE ); 
      return true;
    }
  }
  
  fsend_color( victim, COLOR_SKILL, "You deflect %s's %s and smash your shield into %s!",
	       ch, dt, ch->Him_Her( ) );
  fsend_color( ch, COLOR_SKILL, "%s deflects your %s and smashes you with %s shield!",
	       victim, dt, victim->His_Her( ) );
  fsend_color_seen( victim, COLOR_SKILL, "%s deflects %s's %s and smashes %s with %s shield!",
		    victim, ch, dt, ch->Him_Her( ), victim->His_Her( ) );

  const int damage = int( victim->Damroll( 0 ) )
    + roll_dice( 2, shield->value[1] )
    + shield->value[0];

  damage_physical( ch, victim, damage, "shield strike" );

  /*
  if( can_stun( victim, ch ) ) {
    if( skill > number_range( 0, 100+5*victim->Level() ) ) {
      stun_effect( victim, ch, "shield strike" );
    }
  }
  */

  victim->improve_skill( SKILL_SHIELD_STRIKE ); 

  return true;
}


bool shadow_dance( char_data* ch, char_data* victim, const char* dt, bool plural )
{
  const int skill = victim->get_skill( SKILL_SHADOW_DANCE );

  if( skill < 1 || number_range( 0, 35 ) >= skill )
    return false;
  
  spam_char( victim, "You meld with the shadows, avoiding %s's %s.",
	     ch, dt );
  spam_char( ch, "%s melds with the shadows, avoiding your %s.",
	     victim, dt );
  spam_room( ch, "%s melds with the shadows, avoiding %s's %s.",
	     victim, ch, dt );
  
  victim->improve_skill( SKILL_SHADOW_DANCE );
  
  return true;
}


/*
bool counter_attack( char_data* ch, char_data* victim, obj_data *wield, int& roll )
{
  if( victim->get_skill( SKILL_COUNTER_ATTACK ) < 1 )
    return false;

  if( victim->species ) {
    if( wield )
      roll -= 25;
    }
  else
    roll -= 4*victim->get_skill( SKILL_COUNTER_ATTACK );

  if( roll >= 0 )
    return false;
  
  spam_char( victim, "You counter-attack %s's attack!", ch );
  spam_char( ch, "%s counters your attack!", victim );
  spam_room( "%s counters %s's attack!", victim, ch );
  
  one_round( ch, victim );
 
  victim->improve_skill( SKILL_COUNTER_ATTACK );
  return true;
}
*/


bool parry( char_data* ch, char_data* victim,
	    obj_data *wield, obj_data *vwield, obj_data *vsecondary, int& roll )
{
  if( vwield ) {
    // Cannot parry with unarmed, whip, or bow.
    const int vweapon = vwield->value[3] + WEAPON_FIRST;
    if( vweapon <= WEAPON_UNARMED
	|| vweapon == WEAPON_WHIP
	|| vweapon == WEAPON_BOW
	|| vweapon > WEAPON_SPEAR ) {
      vwield = 0;
    }
  }

  const int skill1 = victim->get_skill( SKILL_PARRY );
  const int dex = victim->Dexterity( );

  int mount = 0;
  if( victim->mount ) {
    const int smf = victim->get_skill( SKILL_MOUNTED_FIGHTING );
    if( smf != UNLEARNT ) {
      mount = ( smf + 3 )/3;	// +1 at level 1, +2 at level 3, +3 at level 6, +4 at level 9.
    }
  }

  const bool water = is_submerged( victim );
  const int skill3 = ( water || victim->mount && mount == 0 ) ? UNLEARNT : victim->get_skill( SKILL_RIPOSTE );

  if( vwield ) {
    roll -= 3*skill1 + 2*skill3 + dex/2 + mount;
  } else if( !victim->species ) {
    roll -= skill1 + dex/3 + mount/2;
  }

  if( roll >= 0 ) {
    if( !vsecondary )
      return false;
    const int weapon = vsecondary->value[3] + WEAPON_FIRST;
    if( weapon <= WEAPON_UNARMED
	|| weapon == WEAPON_WHIP
	|| weapon == WEAPON_BOW
	|| weapon > WEAPON_SPEAR )
      return false;
    const int skill2 = victim->get_skill( SKILL_OFFHAND_PARRY );
    roll -= skill2 + dex/4 + mount/2;
    if( roll >= 0 )
      return false;
    vwield = vsecondary;
  }
  
  if( vwield ) {
    if( skill3 != UNLEARNT
	&& vwield != vsecondary
	&& ( victim->species || !is_set( victim->pcdata->pfile->flags, PLR_PARRY ) )
	&& number_range( 1, 100 ) <= 3*skill3/2 + 4 ) {
      // Riposte!
      if( wield
	  && can_disarm( victim, ch, wield, false )
	  && number_range( 1, 7 ) == 1 ) {
	// Attempt disarm.
	const int fail = is_set( ch->status, STAT_TWO_HAND ) ? -65 : -55;
	const int val = fail + 4*( dex - ch->Dexterity() );
	const bool easy = val > 0 && number_range( 1, 100 ) <= val;
	const bool hard = val < 0 && number_range( 1, 100 ) <= -val;
	const char *name = wield->pIndexData->Name( 1, true, 
						    is_set( wield->extra_flags, OFLAG_IDENTIFIED ) );
	if( easy
	    || ( !hard
		 && number_range( 1, 20 ) <= 2 * ( skill3 + victim->get_skill( SKILL_DISARM ) )/2 - 1 ) ) {
	  fsend_color( victim, COLOR_SKILL,
		       "You parry %s's attack and quickly move to disarm %s, sending %s %s flying!",
		       ch, ch->Him_Her( victim ), ch->His_Her( ), name );
	  fsend_color( ch, COLOR_SKILL,
		       "%s parries your attack and quickly moves to disarm you, sending your %s flying!",
		       victim, name );
	  fsend_color_seen( victim, COLOR_SKILL,
			    "%s parries %s's attack and quickly moves to disarm %s, sending %s %s flying!",
			    victim, ch, ch->Him_Her( ), victim->His_Her( ), name );
	  if( ( wield = object( wield->From( 1 ) ) ) ) {
	    wield->To( ch );
	  }
	  victim->improve_skill( SKILL_RIPOSTE );
	} else {
	  fsend_color( victim, COLOR_SKILL,
		       "You parry %s's attack and quickly move to disarm %s, but %s manages to hold onto %s %s.",
		       ch, ch->Him_Her( victim ), ch->He_She( victim ), ch->His_Her( ), name );
	  fsend_color( ch, COLOR_SKILL,
		       "%s parries your attack and quickly moves to disarm you, but you manage to hold onto your %s.",
		       victim, name );
	  fsend_color_seen( victim, COLOR_SKILL,
			    "%s parries %s's attack and quickly moves to disarm %s, but %s manages to hold onto %s %s.",
			    victim, ch, ch->Him_Her( ), ch->He_She( ), ch->His_Her( ), name );
	}

      } else {
	// Counter attack.
	fsend_color( victim, COLOR_SKILL,
		     "You parry %s's attack, leaving %s vulnerable to a counterattack.",
		     ch, ch->Him_Her( victim ) );
	fsend_color( ch, COLOR_SKILL,
		     "%s parries your attack, leaving you vulnerable to a counterattack!",
		     victim );
	fsend_color_seen( victim, COLOR_SKILL,
			  "%s parries %s's attack, leaving %s vulnerable to a counterattack!",
			  victim, ch, ch->Him_Her( ) );
	
	const char *dt = attack_noun( vwield->value[3] );
	const char *dv = attack_verb( vwield->value[3] );
	attack( victim, ch, dt, dv, vwield, -1, 0, ATT_PHYSICAL );
	victim->improve_skill( SKILL_RIPOSTE );
      }
      return true;
    }

    const char *name = vwield->pIndexData->Name( 1, true, 
						 is_set( vwield->extra_flags, OFLAG_IDENTIFIED ) );
    spam_char( victim, "You parry %s's attack with your %s.", ch, name );
    if( victim->Seen( ch ) && vwield->Seen( ch ) ) {
      spam_char( ch, "%s parries your attack with %s %s.",
		 victim, victim->His_Her( ch ), name );
    } else {
      spam_char( ch, "%s parries your attack.", victim );
    }
    spam_room( ch, "%s parries %s's attack with %s %s.",
	       victim, ch, victim->His_Her( ), name );

  } else {
    spam_char( victim, "You parry %s's attack.", ch );
    spam_char( ch, "%s parries your attack.", victim );
    spam_room( ch, "%s parries %s's attack.", victim, ch );
  }

  if( vwield && vwield == vsecondary ) {
    victim->improve_skill( SKILL_OFFHAND_PARRY );
  } else {
    victim->improve_skill( SKILL_PARRY );
  }
  
  return true;
}


bool guard( char_data* ch, char_data* victim, obj_data *vwield, int& roll )
{
  const int skill = victim->get_skill( SKILL_GUARD );

  if( skill < 1 )
    return false;

  const int dex = victim->Dexterity( );

  int mount = 0;
  if( victim->mount ) {
    const int smf = victim->get_skill( SKILL_MOUNTED_FIGHTING );
    if( smf != UNLEARNT ) {
      mount = ( smf + 3 )/3;	// +1 at level 1, +2 at level 3, +3 at level 6, +4 at level 9.
    }
  }

  if( vwield ) {
    roll -= 4*skill + dex/2 + mount;
  } else if( !victim->species ) {
    roll -= skill + dex/3 + mount/2;
  }

  if( roll >= 0 )
    return false;

  spam_char( victim, "You guard yourself from %s's attack.", ch );
  spam_char( ch, "%s guards from your attack.", victim );
  spam_room( ch, "%s guards from %s's attack.", victim, ch );

  victim->improve_skill( SKILL_GUARD );

  return true;
}


bool tumble( char_data* ch, char_data* victim, int& roll, bool no_miss )
{
  if( victim->mount || victim->rider )
    return false;

  const int skill = victim->get_skill( SKILL_TUMBLE );

  if( skill < 1 )
    return false;

  const int dex = victim->Dexterity( );

  roll -= skill + 3*dex - 15;

  if( roll >= 0 )
    return false;

  if( !no_miss ) {
    spam_char( ch, "%s tumbles away, avoiding your attack.", victim );
    spam_char( victim, "You tumble away, avoiding %s's attack.", ch );
    spam_room( ch, "%s tumbles away, avoiding %s's attack.", victim, ch );
    victim->improve_skill( SKILL_TUMBLE );
  }

  return true;
}


bool dodge( char_data* ch, char_data* victim, int& roll, bool no_miss )
{
  if( victim->mount && !can_defend( victim->mount, ch ) )
    return false;

  char_data *dodger = victim->mount ? victim->mount : victim;

  const int skill = dodger->get_skill( SKILL_DODGE );

  // Players with dodge == 0 still sometimes dodge anyway.
  if( dodger->species && skill == UNLEARNT ) {
    return false;
  }

  const int dex = dodger->Dexterity( ); 

  int mount = 0;
  if( victim->mount ) {
    const int smf = victim->get_skill( SKILL_MOUNTED_FIGHTING );
    if( smf != UNLEARNT ) {
      mount = ( smf + 4 )/5;	// +1 at level 1, +2 at level 6.
    }
  }

  int minus;

  if( dodger->rider ) {
    minus = ( skill + 2*dex - 15 + mount );
  } else {
    minus = ( 2*skill + 3*dex - 15 );
  }

  if( !( dodger->species
	 && dodger->is_affected( AFF_WATER_BREATHING ) )
      && is_submerged( dodger ) ) {
    minus = minus * ( 20 + 3 * dodger->get_skill( SKILL_SWIMMING ) ) / 100;
  }

  roll -= minus;

  if( roll >= 0 )
    return false;

  if( !no_miss ) {
    if( victim->mount ) {
      spam_char( ch, "%s dodges aside, protecting %s from your attack.", dodger, victim );
      spam_char( dodger, "You dodge aside, protecting %s from %s's attack.", victim, ch );
      spam_char( victim, "%s dodges aside, protecting you from %s's attack.", dodger, ch );
      spam_room( dodger, "%s dodges aside, protecting %s from %s's attack.", dodger, victim, ch );
    } else {
      spam_char( ch, "%s dodges your attack.", victim );
      spam_char( victim, "You dodge %s's attack.", ch );
      spam_room( victim, "%s dodges %s's attack.", victim, ch );
    }
    dodger->improve_skill( SKILL_DODGE );
  }

  return true;
}


bool misses_blindly( char_data* ch, char_data* victim, int& chance,
		     const char* dt, bool plural )
{
  if( chance < 0 ) {
    if( victim->Can_See() ) {
      spam_char( victim, "%s's attack misses you.", ch );
    }
    spam_char( ch, "You miss %s.", victim );
    spam_room( ch, "%s's attack misses %s.", ch, victim );
    return true;
  }
  
  if( !victim->Seen( ch ) ) {
    if( !ch->in_room->Seen( ch )
	&& ( chance -= 30 ) < 0 ) {
      spam_char( ch,
		 "You swing wildly in the dark, missing everything." );
      spam_room( ch, "%s swings wildly, missing everything.", ch );
      return true;
    }
    
    if( ( chance -= 30 ) < 0 ) {
      spam_char( ch,
		 "You swing at your unseen victim, but hit nothing." );
      if( victim->Can_See() ) {
	spam_char( victim,
		   "%s tries to hit you, but swings in the wrong direction.", ch );
      }
      spam_room( ch, "%s tries to hit %s, but swings in the wrong direction.",
		 ch, victim );
      return true;
    }

  } else {
    if( victim->is_affected( AFF_DISPLACE )
	&& ( chance -= 10 ) < 0 ) {
      const char *const s = ( plural ? "" : "s" );
      spam_char( ch, "Your %s seem%s to strike %s, but passes through %s.",
		 dt, s, victim, victim->Him_Her( ch ) );
      if( victim->Can_See() ) {
	spam_char( victim, "%s's %s strike%s your displaced image, doing you no harm.",
		   ch, dt, s );
      }
      spam_room( ch, "%s strikes the displaced image of %s.",
		 ch, victim );
      return true;
    }
  }
  
  return false;
}
