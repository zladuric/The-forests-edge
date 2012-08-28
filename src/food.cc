#include <ctype.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include "define.h"
#include "struct.h"


const char*  cook_word [] = { "none", "raw", "cooked", "burnt" };


const default_data consume_msg [] =
{
  { "to_char",  "You eat $q.", -1 },
  { "to_char",  "You eat part of $q.", -1 },
  { "to_char",  "You rip $q into shreds and happily munch on it. Who said real trolls don't eat salad?", ITEM_SCROLL },
  { "to_char",  "You gleefully pull apart $q and eat it piece by piece.", ITEM_CORPSE },
  { "to_char",  "You pull a tender piece off $q and eat it.", ITEM_CORPSE },
  { "to_room",  "$n eats $q.", -1 },
  { "to_room",  "$n eats part of $q.", -1 },
  { "to_room",  "$n rips $q into shreds and happily munches on it.", ITEM_SCROLL },
  { "to_room",  "$n pulls apart $q and eats it piece by piece, making a disgusting sight.", ITEM_CORPSE },
  { "to_room",  "$n pulls a tender piece off $q and eats it.", ITEM_CORPSE },
  { "", "", -1 }
};


/*
 *   EAT ROUTINES
 */


static bool eats_corpses( char_data* ch )
{
  if( ch->species
      && is_set( ch->species->act_flags, ACT_CARNIVORE ) )
    return true;
	     
  return( ch->shdata->race == RACE_TROLL
	  || ch->shdata->race == RACE_ORC );
}


bool vegetarian( char_data* ch )
{
  if( ch->species
      && !is_set( ch->species->act_flags, ACT_CARNIVORE ) )
    return true;

  return( ch->shdata->race == RACE_ENT );
}


bool carnivore( char_data* ch )
{
  if( ch->species )
    return ch->shdata->race == RACE_UNDEAD;

  return( ch->shdata->race == RACE_ORC );
}
 

bool can_eat( char_data* ch, obj_data* obj, bool msg )
{
  if( obj->array == ch->array
      && ch->mount ) {
    if( msg ) {
      send( ch, "Unless you dismount, you can only eat things you are carrying.\n\r" );
    }
    return false;
  }

  if( !can_wear( obj, ITEM_TAKE ) ) {
    if( msg )
      fsend( ch, "You cannot eat %s.", obj );
    return false;
  }

  if( obj->pIndexData->item_type == ITEM_CORPSE ) {
    if( !eats_corpses( ch ) ) {
      if( msg )
	send( ch, "What a disgusting suggestion!\n\r" );
      return false;
    }
    if( obj != forbidden( obj, ch ) ) {
      if( msg )
	fsend( ch,
	       "You feel the gods would be unduly displeased if you ate %s.",
	       obj );
      return false;
    }
  } else if( obj->pIndexData->item_type != ITEM_FOOD ) {
    if( ch->shdata->race == RACE_TROLL ) {
      if( obj->any_metal( ) || obj->stone( ) ) {
	if( msg )
	  send( ch, "That is a bit too crunchy even for your taste.\n\r" );
        return false;
      }
    } else {
      if( msg )
	send( ch, "That's not edible.\n\r" );
      return false;
    }
  }
  
  if( is_set( obj->extra_flags, OFLAG_FLAMING )
      || is_set( obj->extra_flags, OFLAG_BURNING ) ) {
    if( msg )
      fsend( ch, "If you feel the need for a good burn, eat a habanero pepper instead of %s.", obj );
    return false;
  }

  if( is_set( obj->extra_flags, OFLAG_DIVINE ) ) {
    if( msg )
      fsend( ch,
	     "You feel the gods would be unduly displeased if you ate %s.",
	     obj );
    return false;
  }

  if( vegetarian( ch )
      && is_set( obj->materials, MAT_FLESH ) ) {
    if( msg )
      send( ch, "You do not find meat edible.\n\r" );
    return false;
  }

  if( carnivore( ch )
      && !is_set( obj->materials, MAT_FLESH )
      && obj->pIndexData->item_type != ITEM_CORPSE ) {
    if( msg )
      send( ch, "You only eat flesh.\n\r" );
    return false;
  }

  if( ch->condition[COND_FULL] > 40
      && !privileged( ch, LEVEL_DEMIGOD ) ) {
    if( msg ) {
      send( ch, "You are too full to eat more.\n\r" );
      if( is_set( ch->status, STAT_ORDERED ) ) {
	interpret( ch, "shake" );
      }
    }
    return false;
  }  
  
  if( obj->pIndexData->item_type != ITEM_CORPSE
      && !obj->contents.is_empty() ) {
    if( msg )
      send( ch, "That object is not empty, remove its contents first.\n\r" );
    return false;
  }

  return true;
}


bool eat( char_data* ch, obj_data* obj )
{
  ch->Select( 1 );

  if( !can_eat( ch, obj, true ) )
    return false;

  int to_char;
  int to_room;

  if( obj->pIndexData->item_type == ITEM_SCROLL ) {
    to_char = 2;
    to_room = 7;
  } else if( obj->pIndexData->item_type == ITEM_CORPSE ) {
    to_char = 3;
    to_room = 8;
  } else {
    to_char = 0;
    to_room = 5;
  }

  int food_value = obj->pIndexData->item_type != ITEM_FOOD
    ? obj->weight/100
    : obj->value[0];
  
  oprog_data *oprog;

  for( oprog = obj->pIndexData->oprog; oprog; oprog = oprog->next ) {
    if( oprog->trigger == OPROG_TRIGGER_CONSUME ) {
      obj->Select( 1 );
      push( );
      clear_variables( );
      var_ch = ch;
      var_obj = obj; 
      var_room = ch->in_room;
      var_def = consume_msg;
      var_def_type = -1;
      const int result = oprog->execute( );
      pop( );
      if( !result )
	return true;
      //      if( !obj->Is_Valid( ) )
      //	return false;
      break;
    }
  }

  bool limited = false;
  int limit = food_value;
  if( obj->Is_Valid( ) ) {
    
    if( !oprog
	&& !privileged( ch, LEVEL_DEMIGOD )
	&& ( obj->pIndexData->item_type == ITEM_FOOD
	     || obj->pIndexData->item_type == ITEM_CORPSE ) ) {
      limit = min( 50-ch->condition[COND_FULL], food_value );
      if( limit < food_value ) {
	// Can't eat it all. Take home leftovers.
	++to_char;
	++to_room;
	limited = true;
      }
    }

    act( ch, prog_msg( oprog, consume_msg[to_char] ), 0, 0, obj );
    act_seen( prog_msg( oprog, consume_msg[to_room] ), ch, 0, obj );
  }
  
  if( obj->pIndexData->item_type == ITEM_FOOD ) {
    if( is_set( obj->value[3], CONSUME_POISON )
	&& poison( ch, 5, 2 * obj->value[0], true ) ) {
      food_value /= 2;
    }
    
    if( is_set( obj->value[3], CONSUME_PLAGUE )
	&& plague( ch, number_range( 5, 12 ), true ) ) {
      food_value /= 3;
    }
    
    if ( obj->value[1] == COOK_RAW
	 || obj->value[1] == COOK_BURNT ) {
      food_value /= 2;
    }

    limit = min( food_value, limit );
  }
  
  gain_condition( ch, COND_FULL, food_value );
  
  if( obj->Is_Valid( ) ) {
    if( limited ) {
      Content_Array *where = obj->array;
      // Eat only part.
      obj = (obj_data*)obj->From(1);
      if( obj->pIndexData->item_type == ITEM_FOOD ) {
	obj->value[0] -= limit;
	if( obj->pIndexData->vnum == OBJ_BODY_PART ) {
	  obj->weight = obj->weight * ( obj->value[0] - limit ) / obj->value[0];
	  obj->weight = max( obj->weight, 1 );
	} else {
	  obj->weight = obj->value[0]*obj->pIndexData->weight/obj->pIndexData->value[0];
	}
      } else {
	obj->weight -= 100*limit;
      }
      if( obj->pIndexData->weight > 0 ) {
	obj->weight = max( 1, obj->weight );
      }
      obj->To( *where );
    } else if( obj->pIndexData->item_type == ITEM_CORPSE ) {
      extract_corpse( obj );
    } else {
      obj->Extract( 1 );
    }
  }
  
  return true;
}


void do_eat( char_data* ch, const char *argument )
{
  if( ch->fighting ) {
    send( ch, "Eating during combat would be hazardous to your health.\n\r" );
    return;
  }

  obj_data* obj;
  if( !( obj = one_object( ch, argument, "eat",
			   &ch->contents,
			   ch->array ) ) )
    return;
  
  if( eat( ch, obj ) )
    set_delay( ch, 10 );
}


/*
 *   COOK ROUTINES
 */


void do_cook( char_data* ch, const char *argument )
{
  if( is_fighting( ch, "cook" ) )
    return;

  if( !*argument ) {
    send( ch, "What do you wish to cook?\n\r" );
    return;
  }
  
  obj_data *obj;

  if( !( obj = one_object( ch, argument, "cook", &ch->contents ) ) ) {
    return;
  }
  
  obj_data *fire;

  if( !( fire = find_type( ch, *ch->array, ITEM_FIRE ) ) ) {
    send( ch, "You see nothing here on which to cook.\n\r", obj );
    return;
  }

  if( obj->pIndexData->item_type != ITEM_FOOD 
      || obj->value[1] < COOK_RAW
      || obj->value[1] > COOK_BURNT ) {
    send( ch, "You don't see what good cooking %s will do.\n\r", obj );
    return;
  }

  fsend( ch, "You cook %s over %s.", obj, fire );
  fsend_seen( ch, "%s cooks %s over %s.", ch, obj, fire );

  set_delay( ch, 15 );

  if ( obj->value[1] == COOK_BURNT ) {
    fsend( ch, "%s is reduced to inedible ashes.", obj );
    fsend_seen( ch, "%s is reduced to inedible ashes.", obj );
    obj->Extract(1);
    return;
  }

  int new_val = obj->value[1]+1;
  if ( !ch->check_skill( TRADE_COOKING ) ) {
    if ( new_val == COOK_COOKED ) {
      if ( number_range( 1, 4 ) > 1 ) {
	new_val = COOK_BURNT;
      } else {
	send( ch, "Despite your best efforts, your food remains raw.\n\r" );
	fsend_seen( ch, "Oddly, %s efforts don't make %s any more palatable.",
		    ch->His_Her(), obj );
	return;
      }
    }
  }

  if ( new_val == COOK_BURNT ) {
    fsend( ch, "%s is transformed into a blackened, smoking lump.", obj );
    fsend_seen( ch, "%s is transformed into a blackened, smoking lump.", obj );
  }

  obj = (obj_data *)obj->From( 1, true );
  obj->value[1] = new_val;
  // Cooked food is less likely to be poisonous or diseased.
  if( is_set( obj->value[3], CONSUME_POISON )
      && number_range( 1, 4 ) == 1 ) {
    remove_bit( obj->value[3], CONSUME_POISON );
  }
  if( is_set( obj->value[3], CONSUME_PLAGUE )
      && number_range( 1, 4 ) == 1 ) {
    remove_bit( obj->value[3], CONSUME_PLAGUE );
  }
  obj->To( 0 );

  if ( new_val == COOK_COOKED ) {
    ch->improve_skill( TRADE_COOKING );
  }
}
