#include <sys/types.h>
#include <stdio.h>
#include <syslog.h>
#include "define.h"
#include "struct.h"


static void throw_message( char_data* ch, char_data* victim, obj_data* obj,
			     const char* text, const char* word, bool seen )
{
  char         tmp  [ THREE_LINES ];
  char_data*   rch;

  for( int i = 0; i < *ch->array; i++ ) {
    if( ( rch = character( ch->array->list[i] ) )
	&& ( rch == victim && ( !seen || victim->Can_See( ) )
	     || rch == ch
	     || ch->Seen( rch )
	     || rch != victim && victim->Seen( rch ) ) ) {
      snprintf( tmp, THREE_LINES, text,
		rch == ch ? "You" : ch->Name( rch ),
		rch == ch ? "" : "s", obj->Name( rch ),
		rch == victim ? "you" : victim->Name( rch ),
		rch == victim ? "you" : word,
		rch == victim ? "" : "s" );
      *tmp = toupper( *tmp );
      fsend( rch, tmp );
    }
  }
}


static bool throw_liquid( char_data* ch, char_data* victim, obj_data* obj )
{
  int roll = number_range( -50,50 )+ch->Dexterity( );
  
  if( roll < 0 ) {
    throw_message( ch, victim, obj,
		   "%s throw%s %s at %s, but it misses %s completely!",
		   victim->Him_Her( ), true );
    return false;
  }
  
  roll -= victim->Dexterity( );
  
  if( roll < 0 && can_defend( victim, ch ) ) {
    throw_message( ch, victim, obj,
		   "%s throw%s %s at %s, but %s adroitly dodge%s it!",
		   victim->He_She( ), true );
    return false;
  }
  
  throw_message( ch, victim, obj,
		 "%s throw%s %s at %s, splashing the contents all over %s!",
		 victim->Him_Her( ), false );

  return true;
}


/*
 *   ITEM TYPE THROW ROUTINES
 */


static void throw_potion( char_data *ch, char_data *victim, obj_data *obj, bool bad_spell )
{
  int spell = obj->value[0];

  if( spell < 0
      || spell >= table_max[TABLE_SKILL_SPELL] ) {
    bug( "Throw_Potion: spell out of range." );
    bug( "-- Potion = %s", obj->Seen_Name( ) );
    bad_spell = true;
  }
  
  int level = obj->value[1];

  if( level < 1 || level > MAX_SPELL_LEVEL ) {
    bug( "Throw_Potion: level out of range." );
    bug( "-- Potion = %s", obj->Seen_Name( ) );
    level = 1;
  }
  
  int dur = obj->value[2];

  if( dur < 1 ) {
    if( dur == 0 ) {
      dur = duration( spell+SPELL_FIRST, level );
    } else {
      bug( "Throw_Potion: duration out of range." );
      bug( "-- Potion = %s", obj->Seen_Name( ) );
      dur = 1;
    }
  }  

  if( !bad_spell
      && !react_spell( ch, victim, spell ) )
    return;

  if( throw_liquid( ch, victim, obj )
      && !bad_spell ) {
    
    dur = 1+dur/3;
    level = 1+level/2;
    
    if( cast_triggers( spell, ch, 0, victim, 0 ) )
      return;
    
    time_data start;
    gettimeofday( &start, 0 );
  
    if( Tprog_Data *tprog = skill_spell_table[spell].prog ) {
      clear_variables( );
      var_i = level;
      var_j = dur;
      var_ch = ch;
      var_room = ch->in_room;
      var_victim = victim;
      const bool result = tprog->execute( );
      if( !result
	  || !ch->Is_Valid( ) ) {
	finish_spell( start, spell );
	return;
      }
    }

    if( skill_spell_table[ spell ].function )
      ( *skill_spell_table[ spell ].function )( ch, victim, 0, level, dur );

    finish_spell( start, spell );
  }
  
  obj->Extract( 1 );

  if( obj_data *vial  = potion_container( obj ) ) {
    vial->To( *ch->array );
  }
}


static void throw_drink_con( char_data* ch, char_data* victim, obj_data* obj, bool bad_spell )
{
  int liquid = obj->value[2];  
  if( liquid >= table_max[ TABLE_LIQUID ] || liquid < 0 ) {
    bug( "Throw_Drink_Con: bad liquid number. (Obj# %d, Liq# %d)",
	 obj->pIndexData->vnum, liquid );
    liquid = obj->value[2] = LIQ_WATER;
  }
  
  // Note: spell can be < 0, for none.
  int spell = liquid_table[liquid].spell;

  if( spell >= SPELL_FIRST+table_max[ TABLE_SKILL_SPELL ]
      || spell >= 0 && spell < SPELL_FIRST ) {
    bug( "Throw_Drink_Con: spell out of range." );
    bad_spell = true;
  } else if( spell < 0 ) {
    bad_spell = true;
  }

  obj = (obj_data *) obj->From(1);
  spell -= SPELL_FIRST;

  if( !bad_spell
      && !react_spell( ch, victim, spell ) )
    return;

  if( throw_liquid( ch, victim, obj )
      && !bad_spell ) {

    if( cast_triggers( spell, ch, 0, victim, 0 ) )
      return;
    
    time_data start;
    gettimeofday( &start, 0 );
  
    if( Tprog_Data *tprog = skill_spell_table[spell].prog ) {
      clear_variables( );
      var_i = 10;
      var_j = 5;
      var_ch = ch;
      var_room = ch->in_room;
      var_victim = victim;
      const bool result = tprog->execute( );
      if( !result
	  || !ch->Is_Valid( ) ) {
	finish_spell( start, spell );
	return;
      }
    }

    if( skill_spell_table[spell].function )
      ( *skill_spell_table[spell].function )( ch, victim, 0, 10, 5 );

    finish_spell( start, spell );
  }

  obj->value[1] = 0;
  set_owner( obj, 0, ch );
  obj->To( *ch->array );
}


void do_throw( char_data* ch, const char *argument )
{
  if( newbie_abuse( ch ) )
    return; 

  char            arg  [ MAX_INPUT_LENGTH ];

  argument = one_argument( argument, arg );

  if( !strncasecmp( argument, "at ", 3 ) )
    argument += 3;

  if( !*arg ) {
    send( ch, "Throw what?\n\r" );
    return;
  }

  char_data*   victim;
  obj_data*       obj;

  if( !( obj = one_object( ch, arg, "throw", &ch->contents ) ) ) {
    return;
  }

  if ( !obj->droppable( ch ) ) {
    fsend( ch, "You can't let go of %s.", obj );
    return;
  }

  if( !*argument ) {
    fsend( ch, "Throw %s at whom?", obj );
    return;
  }
  
  if( !( victim = one_character( ch, argument, "throw", ch->array ) ) ) {
    return;
  }

  if( victim == ch ) {
    send( ch, "You can't throw things at yourself!\n\r" );
    return;
  } 
  
  // Can throw (strength * 2) lbs.
  if( obj->weight > 200*ch->Strength( ) ) {
    fsend( ch, "%s is too heavy for you to throw any distance.", obj );
    return;
  }

  int spell = -1;
  bool bad_spell = false;

  if( obj->pIndexData->item_type == ITEM_DRINK_CON ) {
    if( obj->value[1] != 0 ) {
      int liquid = obj->value[2];
      if( liquid < table_max[ TABLE_LIQUID ] && liquid >= 0 ) {
	spell = liquid_table[liquid].spell;
      }
    }
  } else if( obj->pIndexData->item_type == ITEM_POTION ) {
    spell = obj->value[0] + SPELL_FIRST;
  }

  obj_data *dummy = 0;
  char_array dumaud;

  bad_spell = !check_target( spell, ch, victim, victim, dummy, dumaud );

  if( !bad_spell
      && obj->pIndexData->item_type == ITEM_POTION
      && !levellimit( obj, victim ) ) {
    bad_spell = true;
  }

  int roll = number_range( 1, 100 ) + 4*ch->Dexterity();
  bool miss = ( roll <= 88 );

  bool dodge = false,
       catches = false,
       hit = false;

  if( !miss ) {
    if( can_defend( victim, ch ) ) {
      roll = number_range( 1, 100 );
      if( obj->pIndexData->item_type != ITEM_WEAPON
	  || !is_set( &obj->pIndexData->restrictions, RESTR_BLADED ) ) {
	catches = ( roll <= ( victim->Dexterity() / 2 ) );
      }
      dodge = !catches && ( roll <= ( victim->Dexterity() * 2 ) );
    }
    hit = !dodge && !catches;
  }

  remove_bit( ch->status, STAT_WIMPY );
  strip_affect( ch, AFF_INVISIBLE );
  leave_shadows( ch );

  if( ( obj->pIndexData->item_type != ITEM_DRINK_CON
	|| obj->value[1] == 0 )
      && obj->pIndexData->item_type != ITEM_POTION ) {
    const char *const stuff = obj->pIndexData->item_type == ITEM_FOOD
                              ? " - food fight!!"
                              : ".";
    const char *const type = obj->pIndexData->item_type == ITEM_FOOD
                             ? "flying victual"
                             : "projectile";
    fsend( ch, "You throw %s at %s%s", obj, victim, stuff );
    if( victim->Can_See( ) )
      fsend( victim, "%s throws %s at you%s", ch, obj, stuff );
    fsend_seen( ch, "%s throws %s at %s%s", ch, obj, victim, stuff );

    if ( miss ) {
      const char *pos = ch->in_room->position( );
      if( *pos ) {
	fsend( *ch->array, "The %s misses %s and lands %s.",
	       type, victim, pos );
	if( victim->Can_See( ) ) {
	  fsend( victim, "The %s misses you and lands %s.",
		 type, pos );
	}
      } else {
	fsend( *ch->array, "The %s misses %s.",
	       type, victim );
	if( victim->Can_See( ) ) {
	  fsend( victim, "The %s misses you.",
		 type );
	}
      }
    } else if ( dodge ) {
      const char *pos = ch->in_room->position( );
      if( *pos ) {
	fsend( *ch->array, "%s dodges the %s and it lands %s.",
	       victim, type, pos );
	fsend( victim, "You dodge the %s and it lands %s.",
	       type, pos );
      } else {
	fsend( *ch->array, "%s dodges the %s.",
	       victim, type );
	fsend( victim, "You dodge the %s.",
	       type );
      }
    } else {
      fsend( *ch->array, "The %s hits %s.",
	     type, victim );
      if( victim->Can_See( ) )
	fsend( victim, "The %s hits you.",
	       type );
      else
	fsend( victim, "A thrown object hits you.",
	       type );
      for( oprog_data *oprog = obj->pIndexData->oprog; oprog; oprog = oprog->next ) {
	if( oprog->trigger == OPROG_TRIGGER_THROW ) {
	  clear_variables( );
	  var_ch = ch;
	  var_victim = victim;
	  var_room = ch->in_room;
	  var_obj = obj;
	  if( !oprog->execute( )
	      || !obj->Is_Valid() )
	    return;
	}
      }
    }
    obj->transfer_object( 0, ch, *ch->array );
    return;
  }

  if( obj->pIndexData->item_type == ITEM_DRINK_CON ) {
    throw_drink_con( ch, victim, obj, bad_spell );
  } else {
    throw_potion( ch, victim, obj, bad_spell );
  }

  set_delay( ch, 32 );
}
