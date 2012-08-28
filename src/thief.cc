#include <sys/types.h>
#include <stdio.h>
#include "define.h"
#include "struct.h"


static bool can_hide( char_data* ch, const char *msg )
{
  if( is_mounted( ch, msg )
      || is_ridden( ch, msg )
      || is_entangled( ch, msg )
      || is_fighting( ch, msg )
      || is_drowning( ch, msg ) ) {
    return false;
  }
  
  if( ch->is_affected( AFF_FIRE_SHIELD )
      || ch->is_affected( AFF_FAERIE_FIRE ) ) {
    if( msg ) 
      fsend( ch, "Your fiery glow makes it impossible to %s.", msg );
    return false;
  }
  
  if( ch->is_affected( AFF_ION_SHIELD ) ) {
    if( msg ) 
      fsend( ch, "Your shield of sparks makes it impossible to %s.", msg );
    return false;
  }
  
  if( ch->is_affected( AFF_ICE_SHIELD ) ) {
    if( msg ) 
      fsend( ch, "Your shield of whirling ice crystals makes it impossible to %s.", msg );
    return false;
  }
  
  /*
  if( ch->is_affected( AFF_THORN_SHIELD ) ) {
    if( msg ) 
      fsend( ch, "Your shield of thorns makes it impossible to %s.", msg );
    return false;
  }
  */
  
  if( midair( ch ) ) {
    if( msg ) {
      fsend( ch, "You cannot %s in midair.", msg );
    }
    return false;
  }

  if( water_logged( ch->in_room ) ) {
    if( msg ) {
      int pos = ch->mod_position( );
      if( pos == POS_HOVERING ) {
	fsend( ch, "You cannot %s while hovering over water.", msg );
      } else if( pos == POS_FLYING ) {
	fsend( ch, "You cannot %s while flying over water.", msg );
      } else {
	fsend( ch, "You cannot %s in the water.", msg );
      }
    }
    return false;
  }
  
  for( int i = 0; i < ch->wearing; ++i ) {
    obj_data *obj = (obj_data*) ch->wearing[i];
    obj->Select( 1 );
    const char *me_loc, *them_loc;
    obj_loc_spam( ch, obj, 0, me_loc, them_loc );
    if( is_set( obj->extra_flags, OFLAG_HUM ) ) {
      if( msg )
	fsend( ch, "%s%s is making too much noise for you to %s.", obj, me_loc, msg );
      return false;
    }
    if( is_set( obj->extra_flags, OFLAG_FLAMING ) ) {
      if( msg )
	fsend( ch, "%s%s is putting off too many flames for you to %s.", obj, me_loc, msg );
      return false;
    }
    if( is_set( obj->extra_flags, OFLAG_BURNING ) ) {
      if( msg )
	fsend( ch, "%s%s is putting off too much smoke for you to %s.", obj, me_loc, msg );
      return false;
    }
  }
  return true;
}


/*
static bool gouge_attack( char_data* ch, char_data* victim )
{
  int skill = ch->get_skill( SKILL_EYE_GOUGE );

  int roll = number_range( 0, 20 )
    + skill/2
    + (ch->Dexterity() - victim->Dexterity())/2
    + (ch->Level() - victim->Level())/4;


  if( roll < 6 ) {  
    fsend( ch, "You attempt to gouge %s but are unsuccessful.", victim );
    fsend( victim, "%s attempts to gouge you but is unsuccessful.", ch );
    fsend_seen( ch,
		"%s attempts to gouge %s but is unsuccessful.",
		ch, victim );
    set_min_delay( victim, 32 );
    return false;
  }
  
  if( roll < 21 ) {
    fsend( ch, "You attempt to gouge %s but fail.", victim );
    fsend( victim, "%s attempts to gouge you but fails.", ch );
    fsend_seen( ch,
		"%s attempts to gouge %s but fails.", ch, victim );
    
    set_min_delay( victim, 20 );
    return false;
  }

  fsend( ch, "You gouge %s in the eye!!", victim );
  fsend( victim, "%s gouges you in the eye!!", ch );
  fsend_seen( ch, "%s gouges %s in the eye!!", ch, victim );
  
  affect_data affect;
  affect.type = AFF_BLIND;
  affect.duration = skill;
  affect.level = skill;
  affect.leech = 0;
  add_affect( victim, &affect );
  
  disrupt_spell( victim ); 
  record_damage( victim, ch );    // Provides an experience share.
  
  ch->improve_skill( SKILL_EYE_GOUGE );
  
  set_min_delay( victim, 20 );
  return true;
}


void do_gouge( char_data* ch, const char *argument )
{
  if( is_confused_pet( ch )
      || is_mob( ch ) )
    return;
  
  if( is_set( ch->pcdata->pfile->flags, PLR_PARRY ) ) {
    send( ch, "You cannot gouge while parrying.\n\r" );
    return;
  }
  
  if( ch->get_skill( SKILL_EYE_GOUGE ) == UNLEARNT ) {
    send( ch, "You do not know how to gouge eyes.\n\r" );
    return;
  }

  char_data *victim;

  if( !( victim = get_victim( ch, argument, "Gouge the eyes of" ) ) )
    return;
  
  if( victim == ch ) {
    send( ch, "Gouging your own eyes is not very productive.\n\r" );
    return;
  }

  if( !can_kill( ch, victim ) )
    return;

  if( victim->species
      && !is_set( victim->species->act_flags, ACT_HAS_EYES ) ) {
    fsend( ch, "%s has no eyes and cannot be blinded.", victim );
    return;
  }
  
  if( !set_fighting( ch, victim ) ) {
    return;
  }

  remove_bit( ch->status, STAT_WIMPY );
  strip_affect( ch, AFF_INVISIBLE );
  leave_shadows( ch );
  
  gouge_attack( ch, victim );
}
*/


void do_disguise( char_data* ch, const char *argument)
{
  if( is_mob( ch ) )
    return;
  
}


void do_garrote( char_data* ch, const char *argument )
{
  if( is_mob( ch ) )
    return;
  
  if( *argument == '\0' ) {
    send( ch, "Garrote whom?\n\r" );
    return;
  }

  char_data *victim;

  if( !( victim = get_victim( ch, argument, "garrote" ) ) )
    return;
 
  garrote( ch, victim );
}


bool garrote( char_data *ch, char_data *victim )
{
  int skill = ch->get_skill( SKILL_GARROTE );

  if( skill == UNLEARNT ) {
    send( ch, "Garrote is not part of your repertoire.\n\r" );
    return false;
  }

  if( is_mounted( ch, "garrote" )
      || is_entangled( ch, "garrote" )
      || is_fighting( ch, "garrote" )
      || is_drowning( ch, "garrote" ) ) {
    return false;
  }
  
  if( victim == ch ) {
    send( ch, "How can you garrote yourself?\n\r" );
    return false;
  }

  obj_data *garrote = ch->Wearing( WEAR_HELD_R, LAYER_BASE );
  
  if( !garrote
      || garrote->pIndexData->item_type != ITEM_GARROTE ) {
    send( ch, "You are not wielding a garrote.\n\r" );
    return false;
  }
  
  if( ch->Seen( victim ) ||
      victim->position > POS_SLEEPING && victim->aggressive.includes( ch ) ) {
    fsend( ch, "%s is too wary of you for garrote to succeed.", victim );
    return false;
  }

  if( victim->is_affected( AFF_CHOKING ) ) {
    fsend( ch, "%s is already choking.", victim );
    return false;
  }

  species_data *species = victim->species;
  share_data *shdata = victim->shdata;

  // Can't garrote the undead.
  if( shdata->race == RACE_UNDEAD ) {
    send( ch, "The undead don't breathe much, so garroting them is useless.\n\r" );
    return false;
  }

  // Size restriction.
  if( victim->Size( ) > ch->Size( ) + 1 ) {
    fsend( ch, "%s is way too large for you to successfully garrote %s.",
	   victim, victim->Him_Her( ) );
    return false;
  }

  // Humanoids or anything with a neck only.
  if( ( !victim->is_humanoid( )
	&& !is_set( species->wear_part, WEAR_NECK ) )
      || shdata->race == RACE_PLANT 
      || shdata->race == RACE_ELEMENTAL
      || shdata->race == MAX_PLYR_RACE ) {
    fsend( ch, "You can't garrote %s.", victim );
    return false;
  }

  // Other questionables.
  if( species
      && ( is_set( species->act_flags, ACT_GHOST )
	   || is_set( species->act_flags, ACT_MIMIC ) ) ) {
    fsend( ch, "You can't garrote %s.", victim );
  }

  if( !can_kill( ch, victim ) )
    return false;
  
  fsend( victim, "You feel %s tighten around your neck.", garrote );
  //  fsend_seen( victim, "%s tightens around %s's neck.", garrote, victim ); 
  
  if( !set_fighting( ch, victim ) )
    return false;

  remove_bit( ch->status, STAT_WIMPY );
  strip_affect( ch, AFF_INVISIBLE );
  leave_shadows( ch );

  if( !attack( ch, victim, "garrote", 0, 0,
	       dice_data( garrote->value[1] ).roll( ),
	       5*(skill-5), ATT_GARROTE ) ) {
    send( ch, "Your attempted garroting fails.\n\r" );
    fsend( victim, "%s attempts to garrote you, but fails.",
	   ch );
    fsend_seen( ch, "%s attempts to garrote %s, but fails.",
		ch, victim );
    set_delay( ch, 20 );
  } else {
    ch->improve_skill( SKILL_GARROTE );
    affect_data af;
    af.type = AFF_CHOKING;
    modify_affect( victim, &af, true );
    set_bit( ch->status, STAT_GARROTING );
    set_delay( ch, 10 );
  }

  return true;
}


/* 
 *   BACKSTAB FUNCTIONS
 */


void do_backstab( char_data* ch, const char *argument )
{
  if( is_mob( ch ) )
    return;
  
  if( !*argument ) {
    send( ch, "Backstab whom?\n\r" );
    return;
  }

  char_data*  victim;
  if( !( victim = get_victim( ch, argument, "backstab" ) ) ) 
    return;

  backstab( ch, victim );
}


bool backstab( char_data *ch, char_data *victim )
{
  int skill1 = ch->get_skill( SKILL_BACKSTAB );

  if( skill1 == UNLEARNT ) {
    send( ch, "Backstabbing is not part of your repertoire.\n\r" );
    return false;
  }

  if( is_mounted( ch, "backstab" )
      || is_entangled( ch, "backstab" )
      || is_fighting( ch, "backstab" )
      || is_drowning( ch, "backstab" ) ) {
    return false;
  }
  
  if( victim == ch ) {
    send( ch, "How can you backstab yourself?\n\r" );
    return false;
  }
  
  obj_data *wield, *secondary, *shield;
  get_wield( ch, wield, secondary, shield );

  if( !wield ) {
    wield = secondary;
    if( !wield ) {
      send( ch, "You need to be wielding a weapon to backstab.\n\r" );
      return false;
    }
  }

  if( !is_set( wield->pIndexData->extra_flags, OFLAG_BACKSTAB ) ) {
    fsend( ch, "It isn't possible to use %s to backstab.", wield );
    return false;
  }

  if( ch->Seen( victim ) ||
      victim->position > POS_SLEEPING && victim->aggressive.includes( ch ) ) {
    fsend( ch, "%s is too wary of you for backstab to succeed.", victim );
    return false;
  }
  
  if( !can_kill( ch, victim ) )
    return false;
  
  if( !set_fighting( ch, victim ) )
    return false;

  remove_bit( ch->status, STAT_WIMPY );
  strip_affect( ch, AFF_INVISIBLE );
  leave_shadows( ch );

  int skill2 = ch->get_skill( SKILL_ASSASSINATE );
  int skill = max( skill1, skill2 );

  const char *string = ( skill2 != UNLEARNT )
    ? "assassinate"
    : "backstab";

  if( !attack( ch, victim, string, 0, wield, -1,
		5*(skill-5), ATT_BACKSTAB ) ) {
    send( ch, "Your attempted %s misses the mark.\n\r", string );
    fsend( victim, "%s attempts to %s you, but misses the mark.",
	   ch, string );
    fsend_seen( ch, "%s attempts to %s %s, but misses the mark.",
		ch, string, victim );
    set_delay( ch, 20 );
  } else {
    // Backstab at 10 is a prereq for assassinate.
    if( skill2 == UNLEARNT
	/*|| number_range( 1,2 ) == 1*/ ) {
      ch->improve_skill( SKILL_BACKSTAB );
    } else {
      ch->improve_skill( SKILL_ASSASSINATE );
    }
    set_delay( ch, 10 );
  }

  return true;
}


/*
 *   STEAL ROUTINES
 */


void do_steal( char_data* ch, const char *argument )
{
  if( is_confused_pet( ch )
      || is_mob( ch ) )
    return;
  
  int skill = ch->get_skill( SKILL_STEAL );
  if( skill == UNLEARNT ) {
    send( ch, "You don't know how to steal.\n\r" );
    return;
  }

  if( is_fighting( ch, "steal" ) )
    return;

  if( is_set( ch->pcdata->pfile->flags, PLR_PARRY ) ) {
    send( ch, "You can not steal with parry on.\n\r" );
    return;
  }
  
  char arg  [ MAX_INPUT_LENGTH ];

  if( !two_argument( argument, "from", arg ) ) {
    send( ch, "Syntax: steal <object> [from] <character>\n\r");
    return;
  }

  char_data*  victim;

  if( !( victim = one_character( ch, argument, "steal from",
				 ch->array ) ) ) {
    return;
  }
  
  if( victim == ch ) {
    send( ch, "Stealing from yourself is pointless.\n\r");
    return;
  }
  
  if( !can_kill( ch, victim ) ) {
    fsend( ch, "You can't steal from %s.", victim );
    return; 
  }
  
  if( is_set( ch->in_room->room_flags, RFLAG_ARENA ) ) {
    fsend( ch, "You can't steal in the arena." );
    return; 
  }

  obj_data *obj = one_object( ch, arg, empty_string,
			      &victim->contents,
			      &victim->wearing );

  /*
  if( ch->get_skill( SKILL_STEAL ) < 8
      && obj
      && obj->array == &victim->wearing ) {
    fsend( ch, "You aren't a good enough thief to steal something %s is wearing.", victim );
    return;
  }
  */

  int chance = 2*ch->Dexterity() - victim->Intelligence( ) - victim->Wisdom( )
             + ( ch->Level() - victim->Level() )/2;

  // Try to limit abuse.
  if( player( victim ) ) {
    chance -= 10;
  }

  // Harder to steal from someone who can see you.
  if( ch->Seen( victim ) ) {
    chance -= 10;
  }

  obj_data *container = 0;

  if( obj ) {
    if( obj->pIndexData->item_type != ITEM_CONTAINER
	|| !is_set( obj->value[1], CONT_CLOSED )
	   && obj->contents.is_empty( ) ) {
      if( obj->array == &victim->wearing ) {
	if( victim->position > POS_SLEEPING ) {
	  fsend( ch, "You can't steal something %s is wearing!", victim );
	  return;
	}
	chance -= 10;
      }
    } else {
      container = obj;
      if( !container->contents.is_empty( ) ) {
	int i = number_range( 0, container->contents.size-1 );
	obj = (obj_data *) container->contents[i];
      } else {
	obj = 0;
      }
    }
  }

  if( obj ) {
    // Closed containers are harder to steal from.
    if( container && is_set( container->value[1], CONT_CLOSED ) ) {
      chance -= 10;
    }
    // Higher level objects are harder to steal.
    if( obj->pIndexData->level > 0 ) {
      chance -= obj->pIndexData->level/2;
    }
    // Higher-value money is harder to steal.
    if( monetary_value( obj ) > 0 )
      chance -= (int) ( log10( (double) monetary_value( obj ) ) * 5.0 );

  } else {
    // Less chance of getting caught if no such item found.
    chance += 10;
  }

  // There's always some chance of failure.
  chance = min( 90, chance );

  if( !ch->check_skill( SKILL_STEAL, chance ) ) {
    fsend( ch, "As you try to steal from %s, your hand slips. Oops.", victim );
    
    remove_bit( ch->status, STAT_WIMPY );
    strip_affect( ch, AFF_INVISIBLE );
    leave_shadows( ch );	// Maybe should be spoil_hide()?
    
    do_stand( victim, "" );
    react_attack( ch, victim );
    set_delay( ch, 32 );
    
    fsend( victim, "%s tried to steal from you!", ch );
    fsend_seen( victim, "%s tried to steal from %s!", ch, victim );
    modify_reputation( ch, victim, REP_STOLE_FROM );
    if( victim == active_shop( ch ) ) {
      snprintf( arg, MAX_INPUT_LENGTH, "Guards! %s is a thief!", ch->Name( victim ) );
      do_yell( victim, arg );
      summon_help( victim, ch );
    }
    return;
  }
  
  set_delay( ch, 20 );

  if( !obj ) {
    if( container ) {
      include_closed = false;
      fsend( ch, "You rummage around inside %s's %s, but come up empty-handed.", victim, container->Name( ch, 1, true ) );
      include_closed = true;
    } else {
      fsend( ch, "%s doesn't have anything like that to steal.", victim );
    }
    return;
  }
  
  if( obj->Weight( 1 ) > ch->Capacity( ) ) {
    fsend( ch, "You can't carry %s.", obj );
    return;
  }
  
  if( obj->Count( 1 ) > ( ch->can_carry_n( ) - ch->contents.number ) ) {
    fsend( ch, "You can't handle %s.", obj );
    return;
  }
  
  if( !obj->droppable( 0 )
      || obj->array == &victim->wearing && !obj->removable( 0 )
      || is_set( obj->extra_flags, OFLAG_NO_STEAL ) ) {
    fsend( ch, "You can't pry %s away from %s.", obj, victim );
    return;
  }
  
  obj = (obj_data*) obj->From( 1 );
  obj->To( ch );
  
  if( container ) {
    include_closed = false;
    fsend( ch, "You succeed in stealing %s from %s's %s.",
	   obj, victim, container->Name( ch, 1, true ) );
    include_closed = true;
  } else {
    fsend( ch, "You succeed in stealing %s from %s.", obj, victim );
  }

  ch->improve_skill( SKILL_STEAL );
}


void do_heist( char_data* ch, const char *argument )
{
  /*
  char           buf  [ MAX_INPUT_LENGTH ];
  char           arg  [ MAX_INPUT_LENGTH ];
  char_data*  victim;
  obj_data*      obj;

  if( is_confused_pet( ch ) )
    return;
 
  if( is_mob( ch ) )
    return;
 
  if( is_set( ch->pcdata->pfile->flags, PLR_PARRY ) ) {
    send( ch, "You can not steal with parry on.\n\r" );
    return;
    }

  argument = one_argument( argument, arg );

  for( ; ; ) {
    argument = one_argument( argument, buf );
    if( buf[ 0 ] == '\0' || !strcasecmp( buf, "from" ) )
      break;
    sprintf( arg+strlen( arg ), " %s", buf );
    }

  if( arg[0] == '\0' || argument[0] == '\0' ) {
    send( ch, "Syntax: steal <object> from <character>\n\r" );
    return;
    }

  if( !( victim = get_char_room( ch, argument, true ) ) ) 
    return;

  if( victim == ch ) {
    send( ch, "That's pointless.\n\r" );
    return;
    }

  if( !can_kill( ch, victim ) ) {
    send( ch, "You can't steal from them.\n\r" );
    return; 
    } 

  remove_bit( ch->pcdata->pfile->flags, PLR_PARRY );

  if( !ch->check_skill( SKILL_STEAL )
    || number_range( 3, 35 ) < victim->Intelligence( ) ) {
      strip_affect( ch, AFF_INVISIBLE );
    leave_shadows( ch );
    start_fight( ch, victim );
    set_attack( ch, victim, number_range( 5, 20 ) );
    set_delay( ch, 32 );
 
    remove_bit( ch->status, STAT_LEAPING );
    remove_bit( ch->status, STAT_WIMPY );
    send( victim, "%s tried to steal from you.\n\r", ch );
    send( *ch->array, "%s tried to steal from %s.\n\r", ch, victim );
    modify_reputation( ch, victim, REP_STOLE_FROM );
    if( victim->pShop ) {
      sprintf( buf, "Guards! %s is a thief.", ch->Name( victim ) );
      do_yell( victim, buf );
      summon_help( victim, ch );
      }
    return;
    }

  if( !( obj = get_obj_inv( victim, arg ) ) ) {
    send( ch, "You can't find it.\n\r" );
    return;
    }
    
  if( !obj->droppable( 0 ) ) {
    send( ch, "You can't pry it away.\n\r" );
    return;
    }

  if( ch->num_ins >= ch->can_carry_n( ) ) {
    send( ch, "You have your hands full.\n\r" );
    return;
    }

  if( ch->wght_ins+weight( obj ) > ch->can_carry_w( ) ) {
    send( ch, "You can't carry that much weight.\n\r" );
    return;
    }

  obj = remove( obj, 1 );
  put_obj( obj, ch );
  send( ch, "You succeeded in stealing %s.\n\r", obj );
  ch->improve_skill( SKILL_STEAL );
  */
}

 
/*
 *   SNEAK FUNCTIONS
 */


void do_sneak( char_data* ch, const char *argument )
{
  if( is_set( ch->status, STAT_SNEAKING ) ) {
    remove_bit( ch->status, STAT_SNEAKING );
    send( ch, "You stop sneaking.\n\r" );
    return;
  }

  if( ch->get_skill( SKILL_SNEAK ) == UNLEARNT ) {
    send( ch, "Sneaking is not something you are adept at.\n\r" );
    return;
  }

  if( !can_hide( ch, "sneak" ) )
    return;

  for( int i = 0; i < ch->wearing; ++i ) {
    obj_data *obj = (obj_data*) ch->wearing[i];
    if( is_set( obj->pIndexData->restrictions, RESTR_NO_SNEAK ) ) {
      fsend( ch, "You cannot sneak while wearing %s.", obj );
      return;
    }
  }

  set_bit( ch->status, STAT_SNEAKING );

  send( ch, "You start sneaking.\n\r" );
  send( ch, "[Sneaking increases movement cost by 2 points.]\n\r" );
}


/*
 *   HIDE ROUTINES
 */


void do_camouflage( char_data* ch, const char *)
{
  if( leave_camouflage( ch ) )
    return;
  
  if( ch->get_skill( SKILL_CAMOUFLAGE ) == UNLEARNT ) {
    send( ch, "Camouflage is not something you are adept at.\n\r" );
    return;
  }
  
  if( !can_hide( ch, "camouflage yourself" ) )
    return;
  
  for( int i = 0; i < ch->wearing; ++i ) {
    obj_data *obj = (obj_data*) ch->wearing[i];
    if( is_set( obj->pIndexData->restrictions, RESTR_NO_HIDE ) ) {
      fsend( ch, "You cannot camouflage yourself while wearing %s.", obj );
      return;
    }
  }

  for( int i = 0; i < *ch->array; ++i ) {
    char_data* rch;
    if( ( rch = character( ch->array->list[i] ) ) &&
	rch != ch && 
	ch->Seen( rch ) )
      ch->seen_by += rch;
  }

  send( ch, "You camouflage yourself and disappear from plain view.\n\r" );
  fsend_seen( ch, "%s tries to blend in with %s surroundings.", ch, ch->His_Her( )  );
  
  remove_bit( ch->status, STAT_HIDING );
  set_bit( ch->status, STAT_CAMOUFLAGED );

  ch->improve_skill( SKILL_CAMOUFLAGE );

  set_delay( ch, 10 );
}


void do_hide( char_data* ch, const char *)
{
  if( leave_shadows( ch ) )
    return;

  if( ch->get_skill( SKILL_HIDE ) == 0 ) {
    send( ch, "Hiding is not something you are adept at.\n\r" );
    return;
  }

  if( !can_hide( ch, "hide" ) )
    return;
  
  for( int i = 0; i < ch->wearing; ++i ) {
    obj_data *obj = (obj_data*) ch->wearing[i];
    if( is_set( obj->pIndexData->restrictions, RESTR_NO_HIDE ) ) {
      fsend( ch, "You cannot hide while wearing %s.", obj );
      return;
    }
  }

  for( int i = 0; i < *ch->array; ++i ) {
    char_data* rch;
    if( ( rch = character( ch->array->list[i] ) )
	&& rch != ch
	&& ch->Seen( rch ) )
      ch->seen_by += rch;
  }

  send( ch, "You step into the shadows.\n\r" );
  fsend_seen( ch, "%s steps into the shadows attempting to hide.", ch );
  
  set_bit( ch->status, STAT_HIDING );

  ch->improve_skill( SKILL_HIDE );

  set_delay( ch, 10 );
}


bool leave_camouflage( char_data* ch )
{
  if( !is_set( ch->status, STAT_CAMOUFLAGED ) )
    return false;
  
  remove_bit( ch->status, STAT_CAMOUFLAGED );
  remove_bit( ch->status, STAT_HIDING );
  
  ch->seen_by.clear( );
  
  send( ch, "You stop camouflaging yourself.\n\r" );
  fsend_seen( ch, "%s suddenly appears from nowhere.", ch );

  update_aggression( ch );

  return true;
}


bool leave_shadows( char_data* ch )
{
  if( leave_camouflage( ch ) ) {
    return true;
  }

  if( !is_set( ch->status, STAT_HIDING ) )
    return false;
  
  remove_bit( ch->status, STAT_HIDING );

  ch->seen_by.clear( );

  send( ch, "You stop hiding.\n\r" );
  fsend_seen( ch, "%s steps from the shadows.", ch );

  update_aggression( ch );

  return true;
}


/* 
 *   DIP ROUTINE
 */


void do_dip( char_data* ch, const char *argument )
{
  char              arg  [ MAX_INPUT_LENGTH ];
  obj_data*   container;
  obj_data*         obj;
  affect_data    affect;
  int             spell;

  if( !two_argument( argument, "into", arg ) ) {
    send( ch, "Syntax: Dip <object> [into] <object>\n\r" );
    return;
  }

  if( !( obj = one_object( ch, arg, "dip",
			   &ch->contents ) ) ) 
    return;

  if( !( container = one_object( ch, argument, "dip into",
				 &ch->contents,
				 ch->array ) ) ) 
    return;

  if( container->pIndexData->item_type != ITEM_DRINK_CON
      && container->pIndexData->item_type != ITEM_FOUNTAIN ) {
    fsend( ch, "%s isn't something you can dip things into.", container );
    return;
  }
  
  if( container == obj ) {
    fsend( ch, "You can't dip %s into itself.", obj );
    return;
  }

  if( container->pIndexData->item_type == ITEM_DRINK_CON
      && container->value[1] == 0 ) {
    include_empty = false;
    fsend( ch, "%s is empty.", container );
    include_empty = true;
    return;
  }
  
  obj = (obj_data *) obj->From( 1, true );

  if( strip_affect( obj, AFF_BURNING ) ) {
    obj->To( );
    fsend( ch, "You extinguish %s by quickly dipping it into %s.",
	   obj, container );
    fsend( *ch->array,
	   "%s extinguishes %s by quickly dipping it into %s.",
	   ch, obj, container );
    return;
  } 
  
  obj->To( );

  fsend( ch, "You dip %s into %s.", obj, container );
  fsend_seen( ch, "%s dips %s into %s.", ch, obj, container );

  if( container->pIndexData->item_type == ITEM_DRINK_CON
      && container->value[1] > 0 ) {
    container = (obj_data*) container->From( 1, true );
    container->value[1] = max( 0, container->value[1] - 5 );
    container->To( );
  }

  if( ( spell = liquid_table[container->value[2]].spell ) == -1 )
    return;
  
  if( spell < SPELL_FIRST || spell >= SPELL_FIRST+table_max[ TABLE_SKILL_SPELL ] ) {
    bug( "Do_dip: Liquid with non-spell skill." );
    return;
  }

  //  char_data *victim;
  //  if( !check_target( spell, ch, obj, victim, obj ) )
  //    return;
  spell -= SPELL_FIRST;		// Normally done by check_target.

  if( cast_triggers( spell, 0, 0, ch, obj ) )
    return;

  time_data start;
  gettimeofday( &start, 0 );
  
  if( Tprog_Data *tprog = skill_spell_table[spell].prog ) {
    clear_variables( );
    var_i = 10;
    var_j = -3;
    var_ch = ch;
    var_room = ch->in_room;
    var_obj = obj;
    const bool result = tprog->execute( );
    if( !result
	|| !ch->Is_Valid( ) ) {
      finish_spell( start, spell );
      return;
    }
  }

  if( skill_spell_table[spell].function )
    ( *skill_spell_table[spell].function )( ch, 0, obj, 10, -3 ); 

  finish_spell( start, spell );
}
