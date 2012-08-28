#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include "define.h"
#include "struct.h"


bool check_target( int& spell, char_data *& ch,
		   thing_data *target, char_data *& victim, obj_data *& obj,
		   char_array& audience,
		   const char *func )
{
  char buf [ THREE_LINES ];

  if( spell < SPELL_FIRST
      || spell >= SPELL_FIRST+table_max[ TABLE_SKILL_SPELL ] ) {
    if( func ) {
      snprintf( buf, THREE_LINES, "%s: Invalid spell.", func );
      code_bug( buf );
    }
    return false;
  }

  spell -= SPELL_FIRST;
  
  if( ch )
    ch->Show( 1 );

  if( target )
    target->Show( 1 );
 
  victim = character( target );
  obj = object( target );
  room_data *room = Room( target );

  if( target && !victim && !obj && !room ) {
    if( func ) {
      snprintf( buf, THREE_LINES, "%s: Bad target type for spell \"%s\"",
		func, skill_spell_table[spell].name );
      code_bug( buf );
    }
    return false;
  }

  switch( skill_spell_table[ spell ].type ) {
  case STYPE_OFFENSIVE :
    if( !victim ) {
      if( func ) {
	if( target ) {
	  snprintf( buf, THREE_LINES, "%s: Non-character target for offensive spell \"%s\"",
		    func, skill_spell_table[spell].name );
	} else {
	  snprintf( buf, THREE_LINES, "%s: NULL target for offensive spell \"%s\"",
		    func, skill_spell_table[spell].name );
	}
	code_bug( buf );
      }
      return false;
    }
    if( victim == ch ) {
      if( func ) {
	snprintf( buf, THREE_LINES, "%s: Target is caster for offensive spell \"%s\"",
		  func, skill_spell_table[spell].name );
	code_bug( buf );
      }
      return false;
    }
    if( !can_kill( ch, victim, false ) ) {
      return false;
    }
    break;
    
  case STYPE_SELF_ONLY :
    if( !ch ) {
      if( !target ) {
	if( func ) {
	  snprintf( buf, THREE_LINES, "%s: No target for self-only spell \"%s\"",
		    func, skill_spell_table[spell].name );
	  code_bug( buf );
	}
	return false;
      }
      if( !victim ) {
	if( func ) {
	  snprintf( buf, THREE_LINES, "%s: Non-character target for self-only spell \"%s\"",
		    func, skill_spell_table[spell].name );
	  code_bug( buf );
	}
	return false;
      }
    } else if( target && target != ch ) {
      if( func ) {
	snprintf( buf, THREE_LINES, "%s: Non-NULL target for self-only spell \"%s\"",
		  func, skill_spell_table[spell].name );
	code_bug( buf );
      }
      return false;
    } else {
      victim = ch;
    }
    break;

  case STYPE_NO_TARGET :
    if( target ) {
      if( func ) {
	snprintf( buf, THREE_LINES, "%s: Non-NULL target for no-target spell \"%s\"",
		  func, skill_spell_table[spell].name );
	code_bug( buf );
      }
      return false;
    }
    break;

  case STYPE_PEACEFUL :
    if( !victim ) {
      if( target ) {
	if( func ) {
	  snprintf( buf, THREE_LINES, "%s: Non-character target for peaceful spell \"%s\"",
		    func, skill_spell_table[spell].name );
	  code_bug( buf );
	}
	return false;
      }
      if( !ch ) {
	if( func ) {
	  snprintf( buf, THREE_LINES, "%s: No target for peaceful spell \"%s\"",
		    func, skill_spell_table[spell].name );
	  code_bug( buf );
	}
	return false;
      }
      // spell( peaceful, ch ) -> spell( peaceful, null, ch )
      victim = ch;
      ch = 0;
    }
    break;
    
  case STYPE_PEACEFUL_OTHER :
    if( !victim ) {
      if( target ) {
	if( func ) {
	  snprintf( buf, THREE_LINES, "%s: Non-character target for peaceful-other spell \"%s\"",
		    func, skill_spell_table[spell].name );
	  code_bug( buf );
	}
	return false;
      }
      if( !( victim = opponent( ch ) ) ) {
	if( func ) {
	  snprintf( buf, THREE_LINES, "%s: No target for peaceful-other spell \"%s\"",
		    func, skill_spell_table[spell].name );
	  code_bug( buf );
	}
	return false;
      }
    }
    if( victim == ch ) {
      if( func ) {
	snprintf( buf, THREE_LINES, "%s: Spell target is caster for peaceful-other spell \"%s\"",
		  func, skill_spell_table[spell].name );
	code_bug( buf );
      }
      return false;
    }
    break;

  case STYPE_MOB_ONLY :
    if( !victim ) {
      if( target ) {
	if( func ) {
	  snprintf( buf, THREE_LINES, "%s: Non-character target for mob-only spell \"%s\"",
		    func, skill_spell_table[spell].name );
	  code_bug( buf );
	}
	return false;
      }
      if( !( victim = opponent( ch ) ) ) {
	if( func ) {
	  snprintf( buf, THREE_LINES, "%s: No target for mob-only spell \"%s\"",
		    func, skill_spell_table[spell].name );
	  code_bug( buf );
	}
	return false;
      }
    }
    if( victim == ch ) {
      if( func ) {
	snprintf( buf, THREE_LINES, "%s: Target is caster for mob-only spell \"%s\"",
		  func, skill_spell_table[spell].name );
	code_bug( buf );
      }
      return false;
    }
    if( victim->Type( ) == PLAYER_DATA ) {
      if( func ) {
	snprintf( buf, THREE_LINES, "%s: Target is not a mob for mob-only spell \"%s\"",
		  func, skill_spell_table[spell].name );
	code_bug( buf );
      }
      return false;
    }
    break;

  case STYPE_WORLD :
    if( target && !victim ) {
      if( func ) {
	snprintf( buf, THREE_LINES, "%s: Non-character target for world spell \"%s\"",
		  func, skill_spell_table[spell].name );
	code_bug( buf );
      }
      return false;
    }
    if( victim && victim == ch ) {
      if( func ) {
	snprintf( buf, THREE_LINES, "%s: Target is caster for world spell \"%s\"",
		  func, skill_spell_table[spell].name );
	code_bug( buf );
      }
      return false;
    }
    break;

  case STYPE_SUMMON :
    if( !target ) {
      if( func ) {
	snprintf( buf, THREE_LINES, "%s: NULL target for summon spell \"%s\"",
		  func, skill_spell_table[spell].name );
	code_bug( buf );
      }
      return false;
    }
    if( !victim ) {
      if( func ) {
	snprintf( buf, THREE_LINES, "%s: Non-character target for summon spell \"%s\"",
		  func, skill_spell_table[spell].name );
	code_bug( buf );
      }
      return false;
    }
    if( victim == ch ) {
      if( func ) {
	snprintf( buf, THREE_LINES, "%s: Target is caster for summon spell \"%s\"",
		  func, skill_spell_table[spell].name );
	code_bug( buf );
      }
      return false;
    }
    if( victim->species
	&& ( !is_set( victim->status, STAT_PET ) || victim->leader != ch ) ) {
      if( func ) {
	snprintf( buf, THREE_LINES, "%s: Target is not a player or caster's pet for summon spell \"%s\"",
		  func, skill_spell_table[spell].name );
	code_bug( buf );
      }
      return false;
    }
    break;

  case STYPE_ANNOYING :
    if( !ch && !victim && !room ) {
      if( func ) {
	snprintf( buf, THREE_LINES, "%s: cannot determine target for annoying spell \"%s\"",
		  func, skill_spell_table[spell].name );
	code_bug( buf );
      }
      return false;
    }
    if( ch && in_sanctuary( ch, false )
	|| victim && in_sanctuary( victim, false )
	|| room && is_set( room->room_flags, RFLAG_SAFE ) ) {
      return false;
    }
    break;

  case STYPE_REPLICATE :
    if( !obj ) {
      if( func ) {
	snprintf( buf, THREE_LINES, "%s: Bad target for replicate spell \"%s\"",
		  func, skill_spell_table[spell].name );
	code_bug( buf );
      }
      return false;
    }
    if( !obj->contents.is_empty()
	|| ( obj->pIndexData->item_type == ITEM_DRINK_CON
	     && obj->value[1] != 0 ) ) {
      if( func ) {
	snprintf( buf, THREE_LINES, "%s: Non-empty target for replicate spell \"%s\"",
		  func, skill_spell_table[spell].name );
	code_bug( buf );
      }
      return false;
    }
    if( is_set( obj->extra_flags, OFLAG_MAGIC ) ) {
      if( func ) {
	snprintf( buf, THREE_LINES, "%s: Magic target for replicate spell \"%s\"",
		  func, skill_spell_table[spell].name );
	code_bug( buf );
      }
      return false;
    }
    
    if( !is_set( obj->extra_flags, OFLAG_REPLICATE ) ) {
      if( func ) {
	snprintf( buf, THREE_LINES, "%s: No-replicate target for replicate spell \"%s\"",
		  func, skill_spell_table[spell].name );
	code_bug( buf );
      }
      return false;
    }
    break;

  case STYPE_WEAPON_ARMOR :
    if( !obj ) {
      if( func ) {
	snprintf( buf, THREE_LINES, "%s: Bad target for weapon-armor spell \"%s\"",
		  func, skill_spell_table[spell].name );
	code_bug( buf );
      }
      return false;
    }
    if( obj->pIndexData->item_type != ITEM_WEAPON 
	&& obj->pIndexData->item_type != ITEM_ARMOR ) {
      if( func ) {
	snprintf( buf, THREE_LINES, "%s: Non-weapon/armor target for weapon-armor spell \"%s\"",
		  func, skill_spell_table[spell].name );
	code_bug( buf );
      }
      return false;
    }
    break;

  case STYPE_WEARABLE :
    if( !obj ) {
      if( func ) {
	snprintf( buf, THREE_LINES, "%s: Bad target for wearable spell \"%s\"",
		  func, skill_spell_table[spell].name );
	code_bug( buf );
      }
      return false;
    }
    if( !obj->pIndexData->is_wearable( ) ) {
      if( func ) {
	snprintf( buf, THREE_LINES, "%s: Non-wearable target for wearable spell \"%s\"",
		  func, skill_spell_table[spell].name );
	code_bug( buf );
      }
      return false;
    }
    break;

  case STYPE_OBJECT :
    if( !obj ) {
      if( func ) {
	snprintf( buf, THREE_LINES, "%s: Bad target for object spell \"%s\"",
		  func, skill_spell_table[spell].name );
	code_bug( buf );
      }
      return false;
    }
    break;

  case STYPE_WEAPON :
    if( !obj ) {
      if( func ) {
	snprintf( buf, THREE_LINES, "%s: Bad target for weapon spell \"%s\"",
		  func, skill_spell_table[spell].name );
	code_bug( buf );
      }
      return false;
    }
    if( obj->pIndexData->item_type != ITEM_WEAPON ) {
      if( func ) {
	snprintf( buf, THREE_LINES, "%s: Non-weapon target for weapon spell \"%s\"",
		  func, skill_spell_table[spell].name );
	code_bug( buf );
      }
      return false;
    }
    break;

  case STYPE_CORPSE :
    if( !obj ) {
      if( func ) {
	snprintf( buf, THREE_LINES, "%s: Bad target for corpse spell \"%s\"",
		  func, skill_spell_table[spell].name );
	code_bug( buf );
      }
      return false;
    }
    if( obj->pIndexData->item_type != ITEM_CORPSE ) {
      if( func ) {
	snprintf( buf, THREE_LINES, "%s: Non-corpse target for corpse spell \"%s\"",
		  func, skill_spell_table[spell].name );
	code_bug( buf );
      }
      return false;
    }
    if( obj->pIndexData->vnum == OBJ_CORPSE_PC
	|| obj->pIndexData->vnum == OBJ_CORPSE_PET ) {
      if( func ) {
	snprintf( buf, THREE_LINES, "%s: Player/pet corpse target for corpse spell \"%s\"",
		  func, skill_spell_table[spell].name );
	code_bug( buf );
      }
      return false;
    }
    break;

  case STYPE_DRINK_CON :
    if( !obj ) {
      if( func ) {
	snprintf( buf, THREE_LINES, "%s: Bad target for drink-container spell \"%s\"",
		  func, skill_spell_table[spell].name );
	code_bug( buf );
      }
      return false;
    }
    if( obj->pIndexData->item_type != ITEM_DRINK_CON ) {
      if( func ) {
	snprintf( buf, THREE_LINES, "%s: Non-drink container target for drink-container spell \"%s\"",
		  func, skill_spell_table[spell].name );
	code_bug( buf );
      }
      return false;
    }
    break;

  case STYPE_PEACEFUL_ANY :
    if( !obj && !victim ) {
      if( !ch ) {
	if( func ) {
	  snprintf( buf, THREE_LINES, "%s: Bad target for peaceful-any spell \"%s\"",
		    func, skill_spell_table[spell].name );
	  code_bug( buf );
	}
	return false;
      }
      victim = ch;
      ch = 0;
    }
    break;
    
  case STYPE_RECALL :
    if( target ) {
      if( func ) {
	snprintf( buf, THREE_LINES, "%s: Non-NULL target for recall spell \"%s\"",
		  func, skill_spell_table[spell].name );
	code_bug( buf );
      }
      return false;
    }
    break;

  case STYPE_SONG :
    if( !ch && !victim && !room ) {
      if( func ) {
	snprintf( buf, THREE_LINES, "%s: cannot determine target for song spell \"%s\"",
		  func, skill_spell_table[spell].name );
	code_bug( buf );
      }
      return false;
    }
    if( !room )
      room = ch ? ch->in_room : victim->in_room;
    for( int i = 0; i < room->contents; ++i ) {
      if( char_data *rch = character( room->contents[i] ) ) {
	if( rch->Can_Hear( )
	    && invis_level( rch ) == 0 )
	  audience += rch;
      }
    }
    break;

  case STYPE_ASTRAL :
  case STYPE_EXIT :
  default :
    if( func ) {
      snprintf( buf, THREE_LINES, "%s: Unsupported type for spell \"%s\"",
		func, skill_spell_table[spell].name );
      code_bug( buf );
    }
    return false;
  }

  if( ch
      && victim
      && skill_spell_table[ spell ].type != STYPE_WORLD
      && skill_spell_table[ spell ].type != STYPE_SUMMON
      && ch->in_room != victim->in_room ) {
    if( func ) {
      snprintf( buf, THREE_LINES, "%s: Caster and target not in same room for non-world spell \"%s\"",
		func, skill_spell_table[spell].name );
      code_bug( buf );
    }
    return false;
  }
  
  return true;
}


static const char *cast_on( char_data *ch, int spell, const char *cmd, obj_data *item )
{
  char *tmp = static_string( );

  if( cmd ) {
    snprintf( tmp, THREE_LINES, "%s %s at", cmd, item->Name( ch ) );
  } else {
    snprintf( tmp, THREE_LINES, "cast %s on", skill_spell_table[ spell ].name );
  }

  return tmp;
}


static bool target_offensive( char_data* ch, cast_data* cast, const char *argument,
			      const char *cmd, const char *casting, obj_data *item )
{
  if( !*argument
      && ( cast->target = opponent( ch ) ) )
    return true;

  char_data *victim;

  if( !( victim = one_character( ch, argument,
				 cast_on( ch, cast->spell, cmd, item ),
				 ch->array ) ) )
    return false;

  if( victim == ch ) {
    fsend( ch,
	   "Your anima speaks up nervously and argues persuasively that %s %s at yourself would be fatuous.",
	   casting ? casting : "casting",
	   item ? item->Name( ch ) : skill_spell_table[ cast->spell ].name );
    return false;
  }
  
  if( !can_kill( ch, victim ) )
    return false;

  cast->target = victim;
  return true;
}


static bool target_self_only( char_data* ch, cast_data* cast, const char *argument,
			      const char *cmd, const char *casting, obj_data *item )
{
  if( *argument ) {
    fsend( ch,
	   "%s %s requires no target as the only possible victim is yourself.",
	   casting ? casting : "casting",
	   item ? item->Name( ch ) : skill_spell_table[ cast->spell ].name );
    return false;
  }

  cast->target = ch;
  return true;
}


static bool target_no_target( char_data* ch, cast_data* cast, const char *argument,
			      const char *cmd, const char *casting, obj_data *item )
{
  if( *argument ) {
    fsend( ch,
	   "%s %s requires no target.",
	   casting ? casting : "casting",
	   item ? item->Name( ch ) : skill_spell_table[ cast->spell ].name );
    return false;
  }

  cast->target = 0;
  return true;
}


static bool target_peaceful( char_data* ch, cast_data* cast, const char *argument,
			     const char *cmd, const char *casting, obj_data *item )
{
  if( !*argument ) {
    cast->target = ch;
    return true;
  }

  return( ( cast->target = one_character( ch, argument,
					  cast_on( ch, cast->spell, cmd, item ),
					  ch->array ) ) );
}


static bool target_peaceful_other( char_data* ch, cast_data* cast, const char *argument,
				   const char *cmd, const char *casting, obj_data *item )
{
  if( !*argument
      && ( cast->target = opponent( ch ) ) )
    return true;

  char_data*  victim;

  if( !( victim = one_character( ch, argument,
				 cast_on( ch, cast->spell, cmd, item ),
				 ch->array ) ) )
    return false;
  
  if( ch == victim ) {
    fsend( ch, "You can't %s %s %s yourself.",
	   cmd ? cmd: "cast",
	   cmd ? item->Name( ch ) : skill_spell_table[ cast->spell ].name,
	   cmd ? "at" : "on");
    return false;
  }

  cast->target = victim;

  return true;
}


static bool target_mob_only( char_data* ch, cast_data* cast, const char *argument,
			     const char *cmd, const char *casting, obj_data *item )
{
  if( !*argument 
      && ( cast->target = opponent( ch ) ) ) 
    return true;

  char_data*  victim;

  if( !( victim = one_character( ch, argument,
				 cast_on( ch, cast->spell, cmd, item ),
				 ch->array ) ) )
    return false;

  if( ch == victim ) {
    fsend( ch, "You can't %s %s %s yourself.",
	   cmd ? cmd: "cast",
	   cmd ? item->Name( ch ) : skill_spell_table[ cast->spell ].name,
	   cmd ? "at" : "on");
    return false;
  }

  if( victim->Type( ) == PLAYER_DATA ) {
    fsend( ch, "You can't %s %s %s players.",
	   cmd ? cmd: "cast",
	   cmd ? item->Name( ch ) : skill_spell_table[ cast->spell ].name,
	   cmd ? "at" : "on");
    return false;
  }
  
  cast->target = victim;
  return true;
}


static bool target_world( char_data* ch, cast_data* cast, const char *argument,
			  const char *cmd, const char *casting, obj_data *item )
{
  char_data *victim;

  if( !( victim = one_character( ch, argument, empty_string,
				 (thing_array*) &ch->followers,
				 (thing_array*) &player_list,
				 (thing_array*) &mob_list ) ) ) {
    cast->target = 0;
    return true;
  }

  if( ch == victim ) {
    fsend( ch, "You can't %s %s %s yourself.",
	   cmd ? cmd: "cast",
	   cmd ? item->Name( ch ) : skill_spell_table[ cast->spell ].name,
	   cmd ? "at" : "on");
    return false;
  }

  /*
  if( victim->in_room
      && is_set( victim->in_room->room_flags, RFLAG_NO_MAGIC ) ) {
    send( ch, "Your spell's target cannot be reached.\n\r" );
    return false;
  }
  */

  cast->target = victim;
  return true;
}


static bool target_summon( char_data* ch, cast_data* cast, const char *argument,
			   const char *cmd, const char *casting, obj_data *item )
{
  char_data *victim;

  if( !( victim = one_character( ch, argument,
				 cast_on( ch, cast->spell, cmd, item ),
				 (thing_array*) &ch->followers,
				 (thing_array*) &player_list ) ) ) {
    return false;
  }

  if( ch == victim ) {
    fsend( ch, "You can't %s %s %s yourself.",
	   cmd ? cmd: "cast",
	   cmd ? item->Name( ch ) : skill_spell_table[ cast->spell ].name,
	   cmd ? "at" : "on" );
    return false;
  }

  if( victim->species
      && ( !is_set( victim->status, STAT_PET ) || victim->leader != ch ) ) {
    fsend( ch, "You can't %s %s %s %s.",
	   cmd ? cmd: "cast",
	   cmd ? item->Name( ch ) : skill_spell_table[ cast->spell ].name,
	   cmd ? "at" : "on",
	   victim );
    return false;
  }

  /*
  if( victim->in_room
      && is_set( victim->in_room->room_flags, RFLAG_NO_MAGIC ) ) {
    send( ch, "Your spell's target cannot be reached.\n\r" );
    return false;
  }
  */

  cast->target = victim;
  return true;
}


static bool target_replicate( char_data* ch, cast_data* cast, const char *argument,
			      const char *cmd, const char *casting, obj_data *item )
{
  obj_data* obj;

  if( !( obj = one_object( ch, argument,
			   cast_on( ch, cast->spell, cmd, item ),
			   &ch->contents ) ) )
    return false;

  if( !obj->contents.is_empty()
      || ( obj->pIndexData->item_type == ITEM_DRINK_CON
	   && obj->value[1] != 0 ) ) {
    if( cmd ) {
      send( ch, "Nothing happens.\n\r" );
    } else {
      include_empty = false;
      fsend( ch, "The contents of %s would disrupt the spell.", obj );
      include_empty = true;
    }
    return false;
  }
  
  if( is_set( obj->extra_flags, OFLAG_MAGIC ) ) {
    if( cmd ) {
      send( ch, "Nothing happens.\n\r" );
    } else {
      send( ch, "You are unable to copy magical items.\n\r" ); 
    }
    return false;
  }

  if( !is_set( obj->extra_flags, OFLAG_REPLICATE ) ) {
    if( cmd ) {
      send( ch, "Nothing happens.\n\r" );
    } else {
      fsend( ch, "%s cannot be replicated.", obj );
    }
    return false;
  }

  cast->target = obj;

  return true;
}


static bool target_weapon_armor( char_data* ch, cast_data* cast, const char *argument,
				 const char *cmd, const char *casting, obj_data *item )
{
  obj_data*    obj;

  if( !( obj = one_object( ch, argument,
			   cast_on( ch, cast->spell, cmd, item ),
			   &ch->contents,
			   ch->array ) ) )
    return false;

  if( obj->pIndexData->item_type != ITEM_WEAPON 
      && obj->pIndexData->item_type != ITEM_ARMOR ) {
    fsend( ch, "You can only %s %s %s weapons and armor.",
	   cmd ? cmd: "cast",
	   cmd ? item->Name( ch ) : skill_spell_table[ cast->spell ].name,
	   cmd ? "at" : "on" );
    return false;
  }
  
  cast->target = obj;

  return true;
}


static bool target_wearable( char_data* ch, cast_data* cast, const char *argument,
			     const char *cmd, const char *casting, obj_data *item ) 
{
  obj_data*    obj;

  if( !( obj = one_object( ch, argument,
			   cast_on( ch, cast->spell, cmd, item ),
			   &ch->contents,
			   ch->array ) ) )
    return false;

  if( !obj->pIndexData->is_wearable( ) ) {
    fsend( ch, "You can only %s %s %s wearable equipment.",
	   cmd ? cmd: "cast",
	   cmd ? item->Name( ch ) : skill_spell_table[ cast->spell ].name,
	   cmd ? "at" : "on" );
    return false;
  }
  
  cast->target = obj;

  return true;
}


static bool target_object( char_data* ch, cast_data* cast, const char *argument,
			   const char *cmd, const char *casting, obj_data *item )
{
  obj_data*    obj;

  if( !( obj = one_object( ch, argument,
			   cast_on( ch, cast->spell, cmd, item ),
			   &ch->contents,
			   ch->array ) ) )
    return false;

  cast->target = obj;

  return true;
}


static bool target_exit( char_data* ch, cast_data* cast, const char *argument,
			 const char *cmd, const char *casting, obj_data *item )
{
  exit_data *exit = one_exit( ch, argument,
			      cast_on( ch, cast->spell, cmd, item ),
			      ch->in_room );

  if( !exit )
    return false;

  cast->target = exit;

  return true;
}


static bool target_weapon( char_data* ch, cast_data* cast, const char *argument,
			   const char *cmd, const char *casting, obj_data *item )
{
  obj_data*    obj;

  if( !( obj = one_object( ch, argument,
			   cast_on( ch, cast->spell, cmd, item ),
			   &ch->contents,
			   ch->array ) ) )
    return false;

  if( obj->pIndexData->item_type != ITEM_WEAPON ) {
    fsend( ch, "You can only %s %s %s weapons.",
	   cmd ? cmd: "cast",
	   cmd ? item->Name( ch ) : skill_spell_table[ cast->spell ].name,
	   cmd ? "at" : "on" );
    return false;
  }

  cast->target = obj;

  return true;
}


static bool target_corpse( char_data* ch, cast_data* cast, const char *argument,
			   const char *cmd, const char *casting, obj_data *item )
{
  obj_data*    obj;

  if( !( obj = one_object( ch, argument,
			   cast_on( ch, cast->spell, cmd, item ),
			   ch->array ) ) )
    return false;
  
  if( obj->pIndexData->item_type != ITEM_CORPSE ) {
    fsend( ch, "You can only %s %s %s corpses.",
	   cmd ? cmd: "cast",
	   cmd ? item->Name( ch ) : skill_spell_table[ cast->spell ].name,
	   cmd ? "at" : "on" );
    return false;
  }
  
  if( obj->pIndexData->vnum == OBJ_CORPSE_PC
      || obj->pIndexData->vnum == OBJ_CORPSE_PET ) {
    fsend( ch, "You cannot %s %s %s that corpse.",
	   cmd ? cmd: "cast",
	   cmd ? item->Name( ch ) : skill_spell_table[ cast->spell ].name,
	   cmd ? "at" : "on" );
    return false;
  }
  
  cast->target = obj;

  return true;
}


static bool target_drink_con( char_data* ch, cast_data* cast, const char *argument,
			      const char *cmd, const char *casting, obj_data *item )
{
  obj_data*    obj;

  if( !( obj = one_object( ch, argument,
			   cast_on( ch, cast->spell, cmd, item ),
			   &ch->contents,
			   ch->array ) ) )
    return false;

  if( obj->pIndexData->item_type != ITEM_DRINK_CON ) {
    fsend( ch, "You can only %s %s %s a drink container.",
	   cmd ? cmd: "cast",
	   cmd ? item->Name( ch ) : skill_spell_table[ cast->spell ].name,
	   cmd ? "at" : "on" );
    return false;
  }

  cast->target = obj;

  return true;
}


static bool target_astral( char_data* ch, cast_data* cast, const char *argument,
			   const char *cmd, const char *casting, obj_data *item )
{
  if( !*argument ) {
    send( ch, "Where do you wish to go?\n\r" );
    return false;
  }

  skip_spaces( argument );

  for( int i = 0; i < table_max[ TABLE_ASTRAL ]; ++i ) {
    if( exact_match( argument, astral_table[i].name ) ) {
      cast->target = get_room_index( astral_table[i].recall, true );
      return true;
    }
  }

  cast->target = 0;
  return true;

  /*
  if( cmd ) {
    send( ch, "Nothing happens.\n\r" );
  } else {
    fsend( ch, "Unknown astral marker: '%s'.", argument );
  }

  return false;
  */
}


static bool target_recall( char_data* ch, cast_data* cast, const char *argument,
			   const char *cmd, const char *casting, obj_data *item )
{
  if( !*argument ) {
    //    cast->target = ch;
    return true;
  }
  
  skip_spaces( argument );

  for( int i = 0; i < table_max[ TABLE_TOWN ]; ++i ) {     
    if( matches( argument, town_table[i].name ) ) {
      cast->target = get_room_index( town_table[i].recall );
      return true;
    }
  }
  
  return( ( cast->target = one_character( ch, argument, "cast on",
					  ch->array ) ) );
}


static bool target_annoying( char_data* ch, cast_data* cast, const char *argument,
			     const char *cmd, const char *casting, obj_data *item )
{
  if( *argument ) {
    fsend( ch,
	   "%s %s requires no target.",
	   casting ? casting : "casting",
	   item ? item->Name( ch ) : skill_spell_table[ cast->spell ].name );
    return false;
  }

  return !in_sanctuary( ch );
}


static bool target_peaceful_any( char_data* ch, cast_data* cast, const char *argument,
				 const char *cmd, const char *casting, obj_data *item )
{
  if( !*argument ) {
    cast->target = ch;
    return true;
  }

  return( ( cast->target = one_thing( ch, argument,
				      cast_on( ch, cast->spell, cmd, item ),
				      &ch->contents,
				      ch->array ) ) );
}


static bool target_song( char_data* ch, cast_data* cast, const char *argument,
			 const char *cmd, const char *casting, obj_data *item )
{
  if( *argument ) {
    fsend( ch,
	   "%s %s requires no target.",
	   casting ? casting : "casting",
	   item ? item->Name( ch ) : skill_spell_table[ cast->spell ].name );
    return false;
  }

  // Make a list of all potential targets in the room.
  room_data *room = ch->in_room;
  for( int i = 0; i < room->contents; ++i ) {
    if( char_data *rch = character( room->contents[i] ) ) {
      if( rch->Can_Hear( )
	  && invis_level( rch ) == 0 )
	cast->audience += rch;
    }
  }

  return true;
}


/*
 *   MAIN ROUTINE
 */


bool get_target( char_data* ch, cast_data* cast, const char *argument,
		 const char *cmd, const char *casting, obj_data *item )
{
  const int spell = cast->spell;
  const int type = skill_spell_table[spell].type;

#define parm ch, cast, argument, cmd, casting, item

  switch( type ) {
  case STYPE_OFFENSIVE      : return target_offensive      ( parm ); 
  case STYPE_PEACEFUL       : return target_peaceful       ( parm ); 
  case STYPE_SELF_ONLY      : return target_self_only      ( parm ); 
  case STYPE_OBJECT         : return target_object         ( parm );
  case STYPE_EXIT           : return target_exit           ( parm );
  case STYPE_WORLD          : return target_world          ( parm );
  case STYPE_PEACEFUL_OTHER : return target_peaceful_other ( parm );
  case STYPE_WEAPON         : return target_weapon         ( parm ); 
  case STYPE_DRINK_CON      : return target_drink_con      ( parm ); 
  case STYPE_MOB_ONLY       : return target_mob_only       ( parm );
  case STYPE_ANNOYING       : return target_annoying       ( parm );
  case STYPE_CORPSE         : return target_corpse         ( parm ); 
  case STYPE_RECALL         : return target_recall         ( parm );
  case STYPE_WEAPON_ARMOR   : return target_weapon_armor   ( parm );
  case STYPE_REPLICATE      : return target_replicate      ( parm );
  case STYPE_ASTRAL         : return target_astral         ( parm );
  case STYPE_NO_TARGET      : return target_no_target      ( parm ); 
  case STYPE_WEARABLE       : return target_wearable       ( parm ); 
  case STYPE_PEACEFUL_ANY   : return target_peaceful_any   ( parm ); 
  case STYPE_SUMMON         : return target_summon         ( parm );
  case STYPE_SONG           : return target_song           ( parm );
  }
  
#undef parm

  bug( "Get_Target: Unknown spell type %d for spell %s (%d).",
       type, skill_spell_table[spell].name, spell );

  if( cmd ) {
    bug( "Get_Target: Command was %s - item %s.",
	 cmd, item );
  }

  return false;
}
