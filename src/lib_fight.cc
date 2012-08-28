#include <ctype.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include "define.h"
#include "struct.h"


/*
 *   FIGHT ROUTINES
 */


const void *code_can_attack( const void **argument )
{
  char_data*      ch  = (char_data*)(thing_data*) argument[0];
  char_data*  victim  = (char_data*)(thing_data*) argument[1];

  if( !ch ) {
    code_bug( "Can_Attack: NULL character." );
    return (void*) false;
  }

  if( !victim ) {
    code_bug( "Can_Attack: NULL victim." );
    return (void*) false;
  }

  if( !ch->in_room
      || ch->in_room != victim->in_room )
    return (void*) false;

  return (void *) can_kill( ch, victim, false );
}


const void *code_set_victim( const void **argument )
{
  char_data*      ch  = (char_data*)(thing_data*) argument[0];
  char_data*  victim  = (char_data*)(thing_data*) argument[1];

  if( !ch ) {
    code_bug( "Set_Victim: NULL character." );
    return (void*) false;
  }

  if( !victim ) {
    stop_fight( ch );
    //  code_bug( "Set_Victim: NULL victim." );
    return (void*) false;
  }

  if( !ch->in_room
      || ch->in_room != victim->in_room
      || !can_kill( ch, victim, false ) ) {
    //    code_bug( "Set_Victim: can't attack specified victim." );
    return (void*) false;
  }

  if( !ch->fighting ) {
    if( event_data *event = find_event( ch, execute_leap ) ) {
      event->pointer = (void*) victim;
      return (void*) true;
    }
    /*
    for( int i = 0; i < ch->events.size; ++i ) {
      if( ch->events[i]->func == execute_leap ) {
	ch->events[i]->pointer = (void*) victim;
	return (void*) true;
      }
    }
    */
    init_attack( ch, victim );
    return (void*) true;
  }

  push( );

  if( !set_fighting( ch, victim ) ) {
    pop( );
    return (void*) false;
  }

  ch->aggressive += victim;

  pop( );
  return (void*) true;
}


const void *code_attack_weapon( const void **argument )
{
  char_data*      ch  = (char_data*)(thing_data*) argument[0];
  char_data*  victim  = (char_data*)(thing_data*) argument[1];
  int            dam  = (int)        argument[2];
  const char *string  = (char*)      argument[3];

  if( !string ) {
    code_bug( "Attack_Weapon: NULL attack string." );
    return 0;
  }

  if( !ch
      || !victim
      || ch->in_room != victim->in_room
      || ch->position < POS_FIGHTING
      || !can_kill( ch, victim, false ) ) 
    return 0;

  obj_data *wield, *secondary, *shield;
  get_wield( ch, wield, secondary, shield );

  push( );

  if( !ch->fighting ) {
    if( !set_fighting( ch, victim ) ) {
      pop( );
      return 0;
    }
    remove_bit( ch->status, STAT_WIMPY );
    strip_affect( ch, AFF_INVISIBLE );
    leave_shadows( ch );
    set_delay( ch, 32 );
  } else if( ch->fighting != victim
	     && !trigger_attack( ch, victim ) ) {
    pop( );
    return 0;
  } else {
    react_attack( ch, victim );
  }

  const int i = attack( ch,
			victim,
			wield ? attack_noun( wield->value[3] ) : string,
			wield ? attack_verb( wield->value[3] ) : string,
			wield,
			wield ? -1 : dam,
			0 );

  pop( );

  return (void*) i;
}


const void *code_attack_offhand( const void **argument )
{
  char_data*      ch  = (char_data*)(thing_data*) argument[0];
  char_data*  victim  = (char_data*)(thing_data*) argument[1];
  int            dam  = (int)        argument[2];
  const char *string  = (char*)      argument[3];

  if( !string ) {
    code_bug( "Attack_Offhand: NULL attack string." );
    return 0;
  }

  if( !ch
      || !victim
      || ch->in_room != victim->in_room
      || ch->position < POS_FIGHTING
      || !can_kill( ch, victim, false )
      || is_set( ch->status, STAT_TWO_HAND ) ) 
    return 0;

  obj_data *wield, *secondary, *shield;
  get_wield( ch, wield, secondary, shield );

  push( );

  if( !ch->fighting ) {
    if( !set_fighting( ch, victim ) ) {
      pop( );
      return 0;
    }
    remove_bit( ch->status, STAT_WIMPY );
    strip_affect( ch, AFF_INVISIBLE );
    leave_shadows( ch );
    set_delay( ch, 32 );
  } else if( ch->fighting != victim
	     && !trigger_attack( ch, victim ) ) {
    pop( );
    return 0;
  } else {
    react_attack( ch, victim );
  }

  const int i = attack( ch,
			victim,
			secondary ? attack_noun( secondary->value[3] ) : string,
			secondary ? attack_verb( secondary->value[3] ) : string,
			secondary,
			secondary ? -1 : dam,
			0 );

  pop( );

  return (void*) i;
}


const void *code_attack_backstab( const void **argument )
{
  char_data*      ch  = (char_data*)(thing_data*) argument[0];
  char_data*  victim  = (char_data*)(thing_data*) argument[1];

  if( !ch
      || !victim
      || ch->in_room != victim->in_room
      || ch->position < POS_FIGHTING )
    return 0;

  push( );

  const int i = backstab( ch, victim );

  pop( );

  return (void*) i;
}


const void *code_attack_charge( const void **argument )
{
  char_data*      ch  = (char_data*)(thing_data*) argument[0];
  char_data*  victim  = (char_data*)(thing_data*) argument[1];

  if( !ch
      || !victim
      || ch->in_room != victim->in_room
      || ch->position < POS_FIGHTING ) 
    return 0;

  push( );

  const int i = charge( ch, victim );

  pop( );

  return (void*) i;
}


const void *code_attack_garrote( const void **argument )
{
  char_data*      ch  = (char_data*)(thing_data*) argument[0];
  char_data*  victim  = (char_data*)(thing_data*) argument[1];

  if( !ch
      || !victim
      || ch->in_room != victim->in_room
      || ch->position < POS_FIGHTING )
    return 0;

  push( );

  const int i = garrote( ch, victim );

  pop( );

  return (void*) i;
}


const void *code_attack_room( const void **argument )
{
  char_data*        ch  = (char_data*)(thing_data*) argument[0];
  int              dam  = (int)        argument[1];
  const char   *string  = (char*)      argument[2];

  if( !string ) {
    code_bug( "Attack_Room: NULL attack string." );
    return 0;
  }

  bool plural = false;

  if( *string == '+' ) {
    ++string;
    plural = true;
  }

  if( !ch || ch->position < POS_SLEEPING ) 
    return 0;

  char_data *rch;
  bool first = true;
  room_data *save_room = ch->in_room;
  Content_Array content = save_room->contents;
  for( int i = 0; i < content; ++i ) {
  //  for( int i = *ch->array-1; i >= 0; i-- ) {
    if( ( rch = character( content[i] ) )
	&& rch != ch
	&& ch->Is_Valid( )
	&& ch->array == &save_room->contents
	&& rch->Is_Valid( )
	&& rch->array == &save_room->contents
	&& invis_level( rch ) < LEVEL_BUILDER
	&& ( rch->pcdata
	     || ( is_set( rch->status, STAT_PET ) && rch->leader->pcdata ) )
	&& can_kill( ch, rch, false ) ) {
      if( first ) {
	push( );
	first = false;
	if( !ch->fighting ) {
	  if( !set_fighting( ch, rch ) )
	    continue;
	  remove_bit( ch->status, STAT_WIMPY );
	  strip_affect( ch, AFF_INVISIBLE );
	  leave_shadows( ch );
	  set_delay( ch, 32 );
	} else if( ch->fighting != rch
		   && !trigger_attack( ch, rch ) ) {
	  continue;
	} else {
	  react_attack( ch, rch );
	}
      }

      //      attack( ch, rch, string, 0, dam, 0 );
      damage_physical( rch, ch, dam, string, plural );
    }
  }

  if( !first ) {
    pop( );
  }

  return 0;
}


const void *code_attack( const void **argument )
{
  char_data*      ch  = (char_data*)(thing_data*) argument[0];
  char_data*  victim  = (char_data*)(thing_data*) argument[1];
  int            dam  = (int)        argument[2];
  const char *string  = (char*)      argument[3];

  if( !string ) {
    code_bug( "Attack: NULL attack string." );
    return 0;
  }

  if( !ch
      || !victim
      || ch->in_room != victim->in_room
      || ch->position < POS_FIGHTING
      || !can_kill( ch, victim, false ) )
    return 0;

  push( );

  if( !ch->fighting ) {
    if( !set_fighting( ch, victim ) ) {
      pop( );
      return 0;
    }
    remove_bit( ch->status, STAT_WIMPY );
    strip_affect( ch, AFF_INVISIBLE );
    leave_shadows( ch );
    set_delay( ch, 32 );
  } else if( ch->fighting != victim
	     && !trigger_attack( ch, victim ) ) {
    pop( );
    return 0;
  } else {
    react_attack( ch, victim );
  }

  const int i = attack( ch, victim, string, 0, 0, dam, 0 );

  pop( );

  return (void*) i;
}


/*
 *   ELEMENTAL ATTACKS
 */


static const void *element_attack( const void **argument, int type )
{
  char_data*      ch  = (char_data*)(thing_data*) argument[0];
  char_data*  victim  = (char_data*)(thing_data*) argument[1];
  int            dam  = (int)        argument[2];
  const char *string  = (char*)      argument[3];

  if( !string ) {
    code_bug( "Attack: Null string." );
    return 0;
  }
  
  if( !ch
      || !victim
      || ch->in_room != victim->in_room
      || ch->position < POS_FIGHTING
      || !can_kill( ch, victim, false ) ) 
    return 0;

  push( );

  if( !ch->fighting ) {
    if( !set_fighting( ch, victim ) ) {
      pop( );
      return 0;
    }
    remove_bit( ch->status, STAT_WIMPY );
    strip_affect( ch, AFF_INVISIBLE );
    leave_shadows( ch );
    set_delay( ch, 32 );
  } else if( ch->fighting != victim
	     && !trigger_attack( ch, victim ) ) {
    pop( );
    return 0;
  } else {
    react_attack( ch, victim );
  }

  const int i = attack( ch, victim, string, 0, 0, dam, 0, type );

  pop( );

  return (void*) i;
}


const void *code_attack_acid  ( const void **arg ) { return element_attack( arg, ATT_ACID ); }
const void *code_attack_cold  ( const void **arg ) { return element_attack( arg, ATT_COLD ); }
const void *code_attack_shock ( const void **arg ) { return element_attack( arg, ATT_SHOCK ); }
const void *code_attack_fire  ( const void **arg ) { return element_attack( arg, ATT_FIRE ); }
const void *code_attack_magic ( const void **arg ) { return element_attack( arg, ATT_MAGIC ); }
const void *code_attack_mind  ( const void **arg ) { return element_attack( arg, ATT_MIND ); }
const void *code_attack_sound ( const void **arg ) { return element_attack( arg, ATT_SOUND ); }


/*
 *   DAMAGE ROUTINES
 */ 


const void *code_inflict( const void **argument )
{
  char_data *victim = (char_data*)(thing_data*) argument[0];
  char_data *ch = (char_data*)(thing_data*) argument[1];
  int dam = (int) argument[2];
  const char *string = (char*) argument[3];
  
  if( victim ) {
    push();
    if( victim->is_affected( AFF_SANCTUARY ) ) {
      dam = 0;
    } else {
      // This should be Save_Physical( ch ), but then dam_message() wouldn't match up.
      dam *= 100-victim->Save_Physical( 0 );
      dam += 50;
      dam /= 100;
    }
    inflict( victim, ch, dam, string );
    pop();
  }

  return 0;
}


static const void *element_inflict( const void **argument,
				    int (char_data::*save)() const )
{
  char_data *victim = (char_data*)(thing_data*) argument[0];
  char_data *ch = (char_data*)(thing_data*) argument[1];
  int dam = (int) argument[2];
  const char *string = (char*) argument[3];

  if( !string ) {
    code_bug( "Inflict: Null string." );
    return 0;
  }

  if( victim ) {
    push();
    if( victim->is_affected( AFF_SANCTUARY ) ) {
      dam = 0;
    } else {
      dam *= 100-(victim->*save)( );
      dam += 50;
      dam /= 100;
    }
    inflict( victim, ch, dam, string );
    pop();
  }

  return 0;
}


const void *code_inflict_acid  ( const void **arg )
{ return element_inflict( arg, &char_data::Save_Acid ); }

const void *code_inflict_cold  ( const void **arg )
{ return element_inflict( arg, &char_data::Save_Cold ); }

const void *code_inflict_shock ( const void **arg )
{ return element_inflict( arg, &char_data::Save_Shock ); }

const void *code_inflict_fire  ( const void **arg )
{ return element_inflict( arg, &char_data::Save_Fire ); }

const void *code_inflict_magic ( const void **arg )
{ return element_inflict( arg, &char_data::Save_Magic ); }

const void *code_inflict_mind  ( const void **arg )
{ return element_inflict( arg, &char_data::Save_Mind ); }

const void *code_inflict_sound ( const void **arg )
{ return element_inflict( arg, &char_data::Save_Sound ); }


const void *code_dam_message( const void **argument )
{
  char_data *victim = (char_data*)(thing_data*) argument[0];
  int dam = (int) argument[1];
  const char *string = (char*) argument[2];
  char_data *ch = (char_data*)(thing_data*) argument[3];
  
  if( !victim )
    return 0;

  bool plural = false;

  if( *string == '+' ) {
    ++string;
    plural = true;
  }

  if( victim->is_affected( AFF_SANCTUARY ) ) {
    dam = 0;
  } else {
    // Do *not* use add_percent_average() here.
    // Result has to match corresponding inflict().
    dam *= 100-victim->Save_Physical( 0 );
    dam += 50;
    dam /= 100;
  }

  dam_message( victim, ch, dam, string, lookup( physical_index, dam, plural ) );

  return 0;
}


static const void *element_message( const void **argument, 
				    int (char_data::*save)() const,
				    const index_data *index )
{
  char_data *victim = (char_data*)(thing_data*) argument[0];
  int dam = (int) argument[1];
  const char *string = (char*) argument[2];
  char_data *ch = (char_data*)(thing_data*) argument[3];

  if( !victim )
    return 0;

  bool plural = false;

  if( *string == '+' ) {
    ++string;
    plural = true;
  }

  if( victim->is_affected( AFF_SANCTUARY ) ) {
    dam = 0;
  } else {
    // Do *not* use add_percent_average() here.
    // Result has to match corresponding inflict().
    dam *= 100-(victim->*save)( );
    dam += 50;
    dam /= 100;
  }

  dam_message( victim, ch, dam, string, lookup( index, dam, plural ) );

  return 0;
}


const void *code_fire_message( const void **arg )
{ return element_message( arg, &char_data::Save_Fire, fire_index ); }

const void *code_shock_message( const void **arg )
{ return element_message( arg, &char_data::Save_Shock, shock_index ); }

const void *code_cold_message( const void **arg )
{ return element_message( arg, &char_data::Save_Cold, cold_index ); }

const void *code_acid_message( const void **arg )
{ return element_message( arg, &char_data::Save_Acid, acid_index ); }

const void *code_magic_message( const void **arg )
{ return element_message( arg, &char_data::Save_Magic, physical_index ); }

const void *code_mind_message( const void **arg )
{ return element_message( arg, &char_data::Save_Mind, physical_index ); }

const void *code_sound_message( const void **arg )
{ return element_message( arg, &char_data::Save_Sound, sound_index ); }


static void *damage( const void **arg,
		     const char *name,
		     const char *die,
		     bool (*func) ( char_data*, char_data*,
				    int, const char*, bool,
				    const char* )
		     )
{
  char_data *victim = (char_data*)(thing_data*) arg[0];
  char_data *ch = (char_data*)(thing_data*) arg[1];
  const int dam = (int) arg[2];
  const char *dam_string = (char*) arg[3];
  const char *die_string = (char*) arg[4];

  if( !victim )
    return 0;

  if( dam < 0 ) {
    code_bug( "%s: negative damage.", name );
    return 0;
  }

  bool plural = false;

  if( *dam_string == '+' ) {
    ++dam_string;
    plural = true;
  }

  if( !die_string || !*die_string )
    die_string = die;

  if( !dam_string && !ch ) {
    // ch may have died.
    return 0;
  }

  return (void*) func( victim, ch, dam, dam_string, plural, die_string );
}


const void *code_dam_acid( const void **arg )
{ return damage( arg, "Dam_Acid", "acid", &damage_acid ); }

const void *code_dam_cold( const void **arg )
{ return damage( arg, "Dam_Cold", "cold", &damage_cold ); }

const void *code_dam_fire( const void **arg )
{ return damage( arg, "Dam_Fire", "fire", &damage_fire ); }

const void *code_dam_magic( const void **arg )
{ return damage( arg, "Dam_Magic", "magic", &damage_magic ); }

const void *code_dam_mind( const void **arg )
{ return damage( arg, "Dam_Mind", "a mind attack", &damage_mind ); }

const void *code_dam_physical( const void **arg )
{ return damage( arg, "Dam_Physical", "physical damage", &damage_physical ); }

const void *code_dam_shock( const void **arg )
{ return damage( arg, "Dam_Shock", "electricity", &damage_shock ); }

const void *code_dam_sound( const void **arg )
{ return damage( arg, "Dam_Sound", "sound", &damage_sound ); }
