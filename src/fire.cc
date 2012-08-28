#include <ctype.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include "define.h"
#include "struct.h"


/*
 *   SMOKE COMMAND
 */


void do_smoke( char_data* ch, const char *argument )
{
  if( !*argument ) {
    send( ch, "What do you want to smoke?\n\r" );
    return;
  }

  obj_data *pipe;

  if( !( pipe = one_object( ch, argument, "smoke", &ch->contents ) ) )
    return;
  
  if( is_entangled( ch, "smoke" )
      || is_drowning( ch, "smoke" )
      || is_fighting( ch, "smoke" ) ) {
    return;
  }

  if( pipe->pIndexData->item_type == ITEM_TOBACCO ) {
    fsend( ch, "You need to put %s in a pipe to smoke it.", pipe );
    return;
  }
  
  if( pipe->pIndexData->item_type != ITEM_PIPE ) {
    fsend( ch, "%s is not something you can smoke.", pipe );
    return;
  }
  
  if( pipe->contents.is_empty() ) {
    fsend( ch, "%s contains nothing to smoke.", pipe );
    return;
  }

  obj_data *tobacco = object( pipe->contents[0] );
  
  oprog_data *oprog;

  for( oprog = tobacco->pIndexData->oprog; oprog; oprog = oprog->next ) {
    if( oprog->trigger == OPROG_TRIGGER_USE ) {
      tobacco->Select( 1 );
      clear_variables( );
      var_ch = ch;
      var_room = ch->in_room;
      var_obj = tobacco;
      var_container = pipe;
      var_def = use_msg;
      var_def_type = ITEM_TOBACCO;
      if( !oprog->execute( )
	  || !tobacco->Is_Valid( ) )
        return;
      break;
    }
  }
  
  act( ch, prog_msg( oprog, use_msg[0] ), 0, 0, pipe, tobacco );
  act_notchar( prog_msg( oprog, use_msg[1] ), ch, 0, pipe, tobacco );

  tobacco->Extract( );
}


/*
 *   IGNITE
 */


void do_ignite( char_data* ch, const char *argument )
{
  obj_data*       obj;
  obj_data*    source  = 0;
  bool          found  = false;
  
  if( ch->shdata->race == RACE_TROLL ) {
    send( ch, "Due to the natural attraction of flame and troll flesh and\
 the associated\n\rchildhood nightmares burning things is not one of your\
 allowed hobbies.\n\r" );
    return;
  }

  if( !( obj = one_object( ch, argument, "ignite",
			   &ch->wearing,
			   &ch->contents,
			   ch->array ) ) )
    return;

  if( is_entangled( ch, "ignite something" )
      || is_drowning( ch, "ignite something" ) ) {
    return;
  }

  if( is_submerged( ch ) ) {
    send( ch, "You can't ignite something underwater.\n\r" );
    return;
  }

  if( is_set( obj->extra_flags, OFLAG_BURNING ) ) {
    fsend( ch, "%s is already burning.", obj );
    return;
  }

  for( int i = 0; !found && i < *ch->array; i++ )
    if( ( source = object( ch->array->list[i] ) )
	&& is_set( source->extra_flags, OFLAG_BURNING ) ) 
      found = true;
  
  for( int i = 0; !found && i < ch->contents; i++ )
    if( ( source = object( ch->contents[i] ) )
	&& is_set( source->extra_flags, OFLAG_BURNING ) ) 
      found = true;

  if( !found ) {
    fsend( ch, "You have nothing with which to ignite %s.", obj );
    return;
  }
  
  if( obj->vs_fire( ) > 90 ) {
    fsend( ch, "Depressingly, %s doesn't seem inclined to burn.", obj );
    return;
  }
  
  fsend( ch, "You ignite %s using %s.", obj, source );
  fsend( *ch->array, "%s ignites %s using %s.", ch, obj, source );
  
  affect_data affect;
  affect.type = AFF_BURNING;
  affect.duration = number_range( 2, 4 );
  affect.level = 1;

  add_affect( obj, &affect );
}


/*
 *   FIRE DAMAGE ROUTINES
 */


const index_data fire_index [] = 
{
  { "has no effect on",      "have no effect on",      0 },
  { "singes",                "singe",                  3 },
  { "scorches",              "scorch",                 7 },
  { "toasts",                "toast",                 15 },
  { "cooks",                 "cook",                  30 },
  { "fries",                 "fry",                   50 },
  { "SEARS",                 "SEAR",                  75 },
  { "CHARS",                 "CHAR",                 100 },
  { "* IMMOLATES *",         "* IMMOLATE *",         140 },
  { "* VAPORIZES *",         "* VAPORIZE *",         200 },
  { "** INCINERATES **",     "** INCINERATE **",     300 },  
  { "** CREMATES **",        "** CREMATE **",        400 },
  { "*** DISINTEGRATES ***", "*** DISINTEGRATE ***",  -1 }
};


bool damage_fire( char_data* victim, char_data* ch, int damage,
		  const char* string, bool plural,
		  const char *die )
{
  if( victim->is_affected( AFF_SANCTUARY ) )
    damage = 0;
  else
    add_percent_average( damage, -victim->Save_Fire( ) );

  dam_message( victim, ch, damage, string,
	       lookup( fire_index, damage, plural ) );
  
  return inflict( victim, ch, damage, die );
}


int Obj_Data :: vs_fire( )
{
  int save  = 100;

  for( int i = 0; i < table_max[TABLE_MATERIAL]; i++ ) 
    if( is_set( pIndexData->materials, i ) )
      save = min( save, material_table[i].save_fire );
  
  if( pIndexData->item_type != ITEM_ARMOR 
      || pIndexData->item_type != ITEM_WEAPON ) 
    return save;
  
  return save+value[0]*(100-save)/(value[0]+2);
}


/* 
 *   FIRE BASED SPELLS
 */


void burn_web( char_data *victim, const char *spell )
{
  // Spell may have killed victim.
  if( !victim )
    return;

  if( strip_affect( victim, AFF_ENTANGLED, false ) ) {
    if( spell && *spell ) {
      fsend( victim, "The %s burns away the web that entangled you.", spell );
      fsend_seen( victim, "The %s burns away the web that entangled %s.", spell, victim );
    } else {
      fsend( victim, "The web that entangled you burns away." );
      fsend_seen( victim, "The web that entangled %s burns away.", victim );
    }
  }
}


/*
bool spell_resist_fire( char_data* ch, char_data* victim, void*,
			int level, int duration )
{
  return spell_affect( ch, victim, level, duration,
		       SPELL_RESIST_FIRE, AFF_RESIST_FIRE );
}
*/


bool spell_fire_shield( char_data* ch, char_data* victim, void*,
			int level, int duration )
{
  //  strip_affect( victim, AFF_INVISIBLE );
  leave_shadows( victim );

  if( is_submerged( victim ) ) {
    send( victim, "The water around you bubbles briefly.\n\r" );
    fsend_seen( victim, "The water around %s bubbles briefly.", victim );
    return false;
  }
  
  if( victim->is_affected( AFF_ICE_SHIELD ) ) {
    send( victim, "A cloud of steam surrounds you, but quickly dissipates.\n\r" );
    fsend_seen( victim, "A cloud of steam surrounds %s, but quickly dissipates.", victim );
    return false;
  }

  if( spell_affect( ch, victim, level, duration,
		    SPELL_FIRE_SHIELD, AFF_FIRE_SHIELD ) ) {
    burn_web( victim, "fire shield" );
    return true;
  }

  return false;
}


bool spell_ignite_weapon( char_data* ch, char_data*, void* vo,
			  int level, int duration )
{
  if( null_caster( ch, SPELL_IGNITE_WEAPON ) )
    return false;

  obj_data *obj = (obj_data*) vo;

  if( is_set( obj->pIndexData->extra_flags, OFLAG_FLAMING ) ) {
    send( ch, "Nothing happens.\n\r" );
    return false;
  }

  if( is_submerged( ch ) && ch->array ) {
    fsend( *ch->array, "The water around %s bubbles briefly.", obj );
    return false;
  }

  if( is_set( obj->pIndexData->materials, MAT_WOOD ) ) {
    fsend( ch,
	   "%s you are carrying bursts into flames which quickly consume it.",
	   obj );
    fsend( *ch->array,
	   "%s %s is carrying bursts into flames which quickly consume it.",
	   obj, ch );
    obj->Extract( 1 );
    return true;
  }

  return spell_affect( ch, obj, level, duration,
		       SPELL_IGNITE_WEAPON, AFF_FLAMING );
}

 
/*
 *   DAMAGE SPELLS
 */


/*
bool spell_burning_hands( char_data* ch, char_data* victim, void*,
			  int level, int )
{
  damage_fire( victim, ch, spell_damage( SPELL_BURNING_HANDS, level ),
	       "*the burst of flame" );
  
  if( number_range( 1, 20 ) <= level ) {
    burn_web( victim, "flame" );
  }

  return true; 
}


bool spell_flame_strike( char_data* ch, char_data* victim, void*,
			 int level, int )
{
  damage_fire( victim, ch, spell_damage( SPELL_FLAME_STRIKE, level ),
	       "*An incandescent spear of flame" );

  burn_web( victim, "flame" );

  return true;
}



bool spell_conflagration( char_data* ch, char_data* victim, void*,
			  int level, int )
{
  damage_fire( victim, ch, spell_damage( SPELL_CONFLAGRATION, level ),
	       "*A raging inferno" );

  burn_web( victim, "inferno" );

  return true;
}
*/


/*
 *   FIREBALL
 */


static bool fireball_effect( char_data *ch, char_data *victim, int level )
{
  if( damage_fire( victim, ch, spell_damage( SPELL_FIREBALL, level ),
		   "*The raging fireball" ) )
    return true;

  burn_web( victim, "fireball" );

  if( victim->mount ) {
    if( number_range( 1, 20 ) + level > victim->get_skill( SKILL_RIDING ) + 10 ) {
      send( victim, "The blast throws you from your mount!\n\r" );
      fsend_seen( victim, "%s is thrown from %s mount by the blast.",
		  victim, victim->His_Her( ) );
      dismount( victim, POS_RESTING );
      set_delay( victim, 20 );
      return false;
    }
  }

  /*
  room_data*     dest;
  int               i;

  i = number_range( 0, 20 );

  if( number_range( 0, SIZE_HORSE ) > victim->Size( )
    && i < 6 && victim->Can_Move( i ) ) {
    send( victim, "The blast throws you from the room!\n\r" );
    fsend_seen( victim, "The blast throws %s from the room!", victim );
    dest = victim->in_room->exit[i]->to_room;
    char_from_room( victim );
    char_to_room( victim, dest );
    send( "\n\r", victim );
    do_look( victim, "");
    }
  */

  return false;
}


bool spell_fireball( char_data* ch, char_data* victim, void*,
		     int level, int )
{
  //  if( null_caster( ch, SPELL_FIREBALL ) )
  //    return true; 

  if( fireball_effect( ch, victim, level ) )
    return true;

  // *** FIX ME: this all needs rework.

  if( ch
      && victim->in_room != ch->in_room )
    return true;

  if( victim->mount )
    fireball_effect( ch, victim->mount, level );

  if( victim->rider )
    fireball_effect( ch, victim->rider, level );

  return true;
}
