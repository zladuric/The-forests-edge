#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include "define.h"
#include "struct.h"


void do_zap( char_data* ch, const char *argument )
{
  if( is_confused_pet( ch )
      || is_familiar( ch ) )
    return;
  
  char arg [ MAX_STRING_LENGTH ];
  
  argument = one_argument( argument, arg );

  obj_data *wand;
  if( !( wand = one_object( ch, arg, "zap",
			    &ch->wearing,
			    &ch->contents ) ) )
    return;

  if( wand->pIndexData->item_type != ITEM_WAND ) {
    fsend( ch, "%s isn't something you can zap.", wand );
    return;
  }
  
  if( is_entangled( ch, "zap" ) ) {
    return;
  }

  if( ( wand != ch->Wearing( WEAR_HELD_R, LAYER_BASE ) ) &&
      ( wand != ch->Wearing( WEAR_HELD_L, LAYER_BASE ) ) ) {
    fsend( ch, "You need to be holding %s to zap it.", wand );
    return;
  }

  int spell = wand->value[0];

  if( spell < 0
      || spell >= table_max[TABLE_SKILL_SPELL] ) {
    fsend( ch, "You zap %s, but nothing seems to happen.", wand );
    fsend_seen( ch, "%s zaps %s, but nothing seems to happen.", ch, wand );
    set_delay( ch, 20 );
    bug( "Zap: Spell out of range." );
    bug( "-- Wand = %s.", wand->Seen_Name( ) );
    return;
  }

  cast_data *cast = new cast_data;
  cast->spell = spell;

  if( !get_target( ch, cast, argument, "zap", "zapping", wand ) ) {
    delete cast;
    return;
  }

  visible_data *target = cast->target;
  char_data *victim = character( target );  
  obj_data *obj = object( target );

  set_delay( ch, 20 );

  if( wand->value[3] == 0
      || ( spell == SPELL_RECALL-SPELL_FIRST
	   && is_set( ch->in_room->room_flags, RFLAG_NO_RECALL ) )
      || ( spell == SPELL_SUMMON-SPELL_FIRST
	   && victim && is_set( victim->in_room->room_flags, RFLAG_NO_RECALL ) )
      || is_set( ch->in_room->room_flags, RFLAG_NO_MAGIC ) 
      || ( victim && is_set( victim->in_room->room_flags, RFLAG_NO_MAGIC ) ) ) {
    fsend( ch, "You zap %s, but nothing seems to happen.", wand );
    fsend_seen( ch, "%s zaps %s, but nothing seems to happen.", ch, wand );
    delete cast;
    return;
  }

  if( wand->value[3] > 0 ) {
    --wand->value[3];
  }

  int level = wand->value[1];

  if( level < 1 || level > MAX_SPELL_LEVEL ) {
    bug( "Zap: Level out of range." );
    bug( "-- Wand = %s", wand->Seen_Name( ) );
    level = 1;
  }
  
  int dur = wand->value[2];

  if( dur < 1 ) {
    if( dur == 0 ) {
      dur = duration( spell+SPELL_FIRST, level );
    } else {
      bug( "Zap: Duration out of range." );
      bug( "-- Wand = %s", wand->Seen_Name( ) );
      dur = 1;
    }
  }

  /*
  if( victim
      && skill_spell_table[ spell ].type == STYPE_OFFENSIVE
      && !set_fighting( ch, victim ) ) {
    delete cast;
    return;
  }
  */

  disrupt_spell( ch );
  remove_bit( ch->status, STAT_WIMPY );
  strip_affect( ch, AFF_INVISIBLE );
  leave_shadows( ch );

  if( victim
      && skill_spell_table[ spell ].type != STYPE_WORLD
      && skill_spell_table[ spell ].type != STYPE_SUMMON ) {
    if( victim != ch ) {
      fsend( ch, "You zap %s at %s.", wand, victim );
      fsend_seen( ch, "%s zaps %s at %s.", ch, wand, victim );
      if( victim->in_room == ch->in_room
	  && ch->Seen( victim ) ) {
	fsend( victim, "%s zaps %s at you!", ch, wand ); 
      }
    } else {
      fsend( ch, "You zap %s at yourself.", wand );
      fsend_seen( ch, "%s zaps %s at %sself.", ch, wand, ch->Him_Her( ) );
    }
  } else if( obj ) {
    fsend( ch, "You zap %s at %s.", wand, obj );
    fsend_seen( ch, "%s zaps %s at %s.", ch, wand, obj );
  } else {
    fsend( ch, "You zap %s.", wand );
    fsend_seen( ch, "%s zaps %s.", ch, wand );
  }

  if( cast_triggers( spell, ch, target, victim, obj ) ) {
    delete cast;
    return;
  }

  if( !react_spell( ch, victim, spell ) ) {
    delete cast;
    return;
  }

  time_data start;
  gettimeofday( &start, 0 );
  
  if( Tprog_Data *tprog = skill_spell_table[spell].prog ) {
    clear_variables( );
    var_i = level;
    var_j = dur;
    var_ch = ch;
    var_room = ch->in_room;
    var_victim = victim;
    var_obj = obj;
    var_exit = exit( target );
    var_list = (thing_array&)cast->audience;
    const int result = tprog->execute( );
    if( !result
	|| !ch->Is_Valid( ) ) {
      delete cast;
      finish_spell( start, spell );
      return;
    }
  }

  void *vo = ( skill_spell_table[spell].type != STYPE_SONG )
    ? (void*)target : (void*)&cast->audience;

  if( skill_spell_table[ spell ].function )
    ( *skill_spell_table[ spell ].function )( ch, victim, vo, level, dur );

  delete cast;
  finish_spell( start, spell );
}


/*
 *   POTION FUNCTIONS
 */


void do_quaff( char_data* ch, const char *argument )
{
  obj_data *obj;
  if( !( obj = one_object( ch, argument, "quaff", &ch->contents ) ) )
    return;

  if( obj->pIndexData->item_type != ITEM_POTION ) {
    fsend( ch, "%s isn't something you can quaff.", obj );
    return;
  }

  if( is_choking( ch, "quaff" )
      || is_paralyzed( ch, "quaff" ) ) {
    return;
  }
  
  if( !antiobj( obj, ch ) ) {
    fsend( ch, "You attempt to quaff %s, but the taste is so abhorrent you cannot swallow even a tiny sip.", obj );
    return;
  }

  set_delay( ch, 25 );

  fsend( ch, "You quaff %s.", obj );
  fsend_seen( ch, "%s quaffs %s.", ch, obj );

  int spell = obj->value[0];

  if( !levellimit( obj, ch )
      || spell < 0
      || spell >= table_max[TABLE_SKILL_SPELL] ) {
    send( ch, "Nothing seems to happen.\n\r" );

    if( spell < 0
	|| spell >= table_max[TABLE_SKILL_SPELL] ) {
      bug( "Quaff: Spell out of range." );
      bug( "-- Potion = %s.", obj->Seen_Name( ) );
    }

    obj->Extract( 1 );
    
    if( obj_data *vial = potion_container( obj ) ) {
      vial->To( ch );
    }
    return;
  }

  int level = obj->value[1];

  if( level < 1 || level > MAX_SPELL_LEVEL ) {
    bug( "Quaff: Level out of range." );
    bug( "-- Potion = %s", obj->Seen_Name( ) );
    level = 1;
  }

  int dur = obj->value[2];

  if( dur < 1 ) {
    if( dur == 0 ) {
      dur = duration( spell+SPELL_FIRST, level );
    } else {
      bug( "Quaff: Duration out of range." );
      bug( "-- Potion = %s", obj->Seen_Name( ) );
      dur = 1;
    }
  }  

  obj->Extract( 1 );

  if( obj_data *vial = potion_container( obj ) ) {
    vial->To( ch );
  }
  
  // Potions work OK in no-magic rooms.
  if( spell == SPELL_RECALL-SPELL_FIRST
      && is_set( ch->in_room->room_flags, RFLAG_NO_RECALL ) ) {
    send( ch, "Nothing seems to happen.\n\r" );
    return;
  }
  
  disrupt_spell( ch );
  remove_bit( ch->status, STAT_WIMPY );

  /*
  if( spell == SPELL_FIRE_SHIELD-SPELL_FIRST
      || spell == SPELL_ICE_SHIELD-SPELL_FIRST
      || spell == SPELL_ION_SHIELD-SPELL_FIRST
      || spell == SPELL_FAERIE_FIRE-SPELL_FIRST ) {
    leave_shadows( ch );
  }
  */

  if( cast_triggers( spell, 0, 0, ch, 0 ) )
    return;

  time_data start;
  gettimeofday( &start, 0 );
  
  if( Tprog_Data *tprog = skill_spell_table[spell].prog ) {
    clear_variables( );
    var_i = level;
    var_j = dur;
    var_room = ch->in_room;
    var_victim = ch;
    const int result = tprog->execute( );
    if( !result
	|| !ch->Is_Valid( ) ) {
      finish_spell( start, spell );
      return;
    }
  }

  if( skill_spell_table[ spell ].function )
    ( *skill_spell_table[ spell ].function )( 0, ch, 0, level, dur );

  finish_spell( start, spell );
}


/*
 *   SCROLL ROUTINES
 */


bool recite( char_data *ch, const char *argument, obj_data *scroll )
{
  scroll->Select( 1 );

  if( is_set( ch->status, STAT_BERSERK ) ) {
    send( ch, "Your mind is on killing, not reading scrolls.\n\r" );
    return false;
  }

  if( scroll->pIndexData->item_type != ITEM_SCROLL ) {
    fsend( ch, "%s isn't something you can recite.", scroll );
    return false;
  }
  
  if( is_silenced( ch, "recite" )
      || is_paralyzed( ch, "recite" ) ) {
    return false;
  }

  if( !ch->in_room->Seen( ch ) ) {
    fsend( ch, "You try to recite %s, but it is too dark to read the writing.",
	   scroll );
    return false;
  }

  if( !levellimit( scroll, ch ) ) {
    fsend( ch, "You can't understand the words written on %s.", scroll );
    return false;
  }

  if( !antiobj( scroll, ch ) ) {
    fsend( ch, "You begin to recite %s, but the words are so abhorrent you cannot continue.", scroll );
    return false;
  }

  int spell = scroll->value[0];

  if( spell < 0
      || spell >= table_max[TABLE_SKILL_SPELL] ) {
    fsend( ch, "You attempt to pronounce the words on %s but absolutely nothing happens.",
	   scroll );
    set_delay( ch, 30 );
    bug( "Recite: Spell out of range." );
    bug( "-- Scroll = %s", scroll->Seen_Name( ) );
    return false;
  }

  cast_data *cast = new cast_data;
  cast->spell = spell;

  if( !get_target( ch, cast, argument, "recite", "reciting", scroll ) ) {
    delete cast;
    return false;
  }

  visible_data *target  = cast->target;
  char_data *victim = character( target );

  set_delay( ch, 30 );
  
  if( ( spell == SPELL_RECALL-SPELL_FIRST
	&& is_set( ch->in_room->room_flags, RFLAG_NO_RECALL ) )
      || ( spell == SPELL_SUMMON-SPELL_FIRST
	   && victim && is_set( victim->in_room->room_flags, RFLAG_NO_RECALL ) )
      || is_set( ch->in_room->room_flags, RFLAG_NO_MAGIC )
      || ( victim && is_set( victim->in_room->room_flags, RFLAG_NO_MAGIC ) ) ) {
    fsend( ch, "You attempt to pronounce the words on %s but absolutely nothing happens.",
	   scroll );
    delete cast;
    return false;
  }
  
  if( skill_spell_table[ spell ].type == STYPE_OFFENSIVE ) {
    if( !set_fighting( ch, victim ) ) {
      delete cast;
      return false;
    }
  }

  // For wimpy recall.
  disrupt_spell( ch );

  remove_bit( ch->status, STAT_WIMPY );
  strip_affect( ch, AFF_INVISIBLE );
  leave_shadows( ch );

  fsend( ch, "You recite %s.", scroll );
  fsend( *ch->array, "%s recites %s.", ch, scroll );
  
  int level = scroll->value[1];

  if( level < 1 || level > MAX_SPELL_LEVEL ) {
    bug( "Recite: Level out of range." );
    bug( "-- Scroll = %s", scroll->Seen_Name( ) );
    level = 1;
  }

  int dur = scroll->value[2];

  if( dur < 1 ) {
    if( dur == 0 ) {
      dur = duration( spell+SPELL_FIRST, level );
    } else {
      bug( "Recite: Duration out of range." );
      bug( "-- Scroll = %s", scroll->Seen_Name( ) );
      dur = 1;
    }
  }
  
  scroll->Extract( 1 );

  if( cast_triggers( spell, ch, target, victim, object( target ) ) ) {
    delete cast;
    return true;
  }

  if( !react_spell( ch, victim, spell ) ) {
    delete cast;
    return true;
  }

  time_data start;
  gettimeofday( &start, 0 );
  
  if( Tprog_Data *tprog = skill_spell_table[spell].prog ) {
    clear_variables( );
    var_i = level;
    var_j = dur;
    var_ch = ch;
    var_room = ch->in_room;
    var_victim = victim;
    var_obj = object( target );
    var_exit = exit( target );
    var_list = (thing_array&) cast->audience;
    const int result = tprog->execute( );
    if( !result
	|| !ch->Is_Valid( ) ) {
      delete cast;
      finish_spell( start, spell );
      return true;
    }
  }

  void *vo = ( skill_spell_table[spell].type != STYPE_SONG )
    ? (void*)target : (void*)&cast->audience;
  
  if( skill_spell_table[ spell ].function )
    ( *skill_spell_table[ spell ].function )( ch, victim, vo, level, dur );

  delete cast;
  finish_spell( start, spell );
  return true;
}


void do_recite( char_data* ch, const char *argument )
{
  if( is_confused_pet( ch )
      || is_familiar( ch ) )
    return;
  
  char arg [ MAX_STRING_LENGTH ];

  argument = one_argument( argument, arg );

  obj_data *scroll;
  if( !( scroll = one_object( ch, arg, "recite", &ch->contents ) )  )
    return;
  
  recite( ch, argument, scroll );
}
