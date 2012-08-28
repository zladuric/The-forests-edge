#include <ctype.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include "define.h"
#include "struct.h"


obj_data *potion_container( obj_data *obj, int num )
{
  if( obj->pIndexData->item_type != ITEM_POTION )
    return 0;

  obj_clss_data *clss = get_obj_index( obj->value[3] );
  
  if( !clss )
    return 0;

  obj_data *container = create( clss, num );
  container->value[1] = 0;

  return container;
}


int char_data :: tolerance( ) const
{
  if( shdata->race >= MAX_PLYR_RACE )
    return 10;

  const int tol  = plyr_race_table[ shdata->race ].tolerance;

  if( tol <= 0 )
    return 10;

  return tol;
}


bool char_data :: is_drunk( ) const
{
  return condition[ COND_DRUNK ] >= tolerance( );
}


const char* liquid_name( int i, bool known )
{
  if( i < 0 || i >= MAX_LIQUID ) {
    roach( "Liquid_Name: Impossible liquid." );   
    return "non-existent liquid";
  }
  
  return( known ? liquid_table[i].name : liquid_table[i].color );
}


const char* liquid_name( const obj_data* obj )
{
  const int i = obj->value[2];

  if( i < 0 || i >= MAX_LIQUID ) {
    roach( "Liquid_Name: Impossible liquid." );   
    return "non-existent liquid";
  }

  return( is_set( obj->extra_flags, OFLAG_KNOWN_LIQUID ) 
	  ? liquid_table[i].name : liquid_table[i].color );
}


bool plague( char_data *ch, int duration, bool consume )
{
  // Lizzies are immune to plague.
  if( ch->shdata->race == RACE_LIZARD
      || ch->shdata->race == RACE_UNDEAD
      || ch->shdata->race == RACE_GOLEM
      || ch->shdata->race == RACE_PLANT
      || ch->Level( ) < 6 ) {
    return false;
  }

  const bool was_plagued = ch->is_affected( AFF_PLAGUE );

  if( makes_save( ch, 0, RES_POISON, 0, 0 ) ) {
    if( !was_plagued ) {
      send( ch, "You are ill for a moment, but the feeling passes.\n\r" );
    }
    return false;
  }

  if( consume && !was_plagued ) {
    send( ch, "You shiver suddenly and throw up.\n\r" );
    fsend_seen( ch, "%s shivers suddenly and throws up.", ch );
  }

  affect_data affect;
  affect.type = AFF_PLAGUE;
  affect.duration = duration;
  affect.level = 10;

  add_affect( ch, &affect );
  return !was_plagued;
}


bool poison( char_data *ch, int level, int duration, bool consume )
{
  if( ch->shdata->race == RACE_UNDEAD ) {
    return false;
  }

  const bool was_poisoned = ch->is_affected( AFF_POISON );

  if( makes_save( ch, 0, RES_POISON, 0, level ) ) {
    if( !was_poisoned ) {
      send( ch, "You are nauseous for a moment, but the feeling passes.\n\r" );
    }
    return false;
  }

  if( consume && !was_poisoned ) {
    send( ch, "You choke and gag.\n\r" );
    fsend_seen( ch, "%s chokes and gags.", ch );
  }

  affect_data affect;
  affect.type = AFF_POISON;
  affect.duration = duration;
  affect.level = level;

  add_affect( ch, &affect );
  return !was_poisoned;
}


/*
 *   DRINK ROUTINES
 */


void do_drink( char_data* ch, const char *argument )
{
  if( ch->fighting ) {
    send( ch, "Drinking during combat would be hazardous to your health.\n\r" );
    return;
  }

  obj_data* obj;

  if( !strncasecmp( argument, "from ", 5 ) )
    argument += 5;
  
  if( !*argument ) {
    if( !( obj = find_type( ch, *ch->array, ITEM_FOUNTAIN ) ) ) {
      send( ch, "Drink what?\n\r" );
      return;
    }
  } else {
    if( !( obj = one_object( ch, argument, "drink",
			     &ch->contents,
			     &ch->wearing,
			     ch->array ) ) )
      return;
  }
  
  if( drink( ch, obj ) )
    set_delay( ch, 10 );
}


bool would_drink( char_data *ch, obj_data *obj )
{
  int liquid, spell;

  if( ( obj->pIndexData->item_type == ITEM_FOUNTAIN
	|| ( obj->pIndexData->item_type == ITEM_DRINK_CON
	     && obj->value[1] != 0 ) )
      && !is_set( obj->value[3], CONSUME_POISON )
      && !is_set( obj->value[3], CONSUME_PLAGUE )
      && ( liquid = obj->value[2] ) >= 0
      && liquid < table_max[ TABLE_LIQUID ]
      && ( ( spell = liquid_table[obj->value[2]].spell ) == -1
	   || ( ( spell -= SPELL_FIRST ) >= 0
		&& spell < table_max[ TABLE_SKILL_SPELL ]
		&& skill_spell_table[spell].type != STYPE_OFFENSIVE ) )
      && ( liquid_table[liquid].thirst > 0
	   || liquid_table[liquid].hunger > 0
	   || liquid_table[liquid].alcohol > 0
	   || liquid_table[liquid].spell != -1 ) ) {
    return true;
  }

  return false;
}


bool can_drink( char_data *ch, obj_data *obj, bool msg )
{
  if( obj->pIndexData->item_type != ITEM_FOUNTAIN
      && obj->pIndexData->item_type != ITEM_DRINK_CON ) {
    if( msg )
      fsend( ch, "You can't drink from %s.", obj );
    return false;
  }
  
  const char *me_loc, *them_loc;
  if( msg ) {
    obj_loc_spam( ch, obj, 0, me_loc, them_loc );
  }

  if( obj->pIndexData->item_type != ITEM_FOUNTAIN
      && obj->value[1] == 0 ) {
    if( msg ) {
      include_empty  = false;
      fsend( ch, "%s%s is already empty.", obj, me_loc );
      include_empty  = true;
    }
    return false;
  }

  int liquid = obj->value[2];

  if( liquid >= table_max[ TABLE_LIQUID ]
      || liquid < 0 ) {
    bug( "Do_drink: bad liquid number. (Obj# %d, Liq# %d)",
	 obj->pIndexData->vnum, liquid );
    liquid = obj->value[2] = LIQ_WATER;
  }

  if( ch->condition[ COND_THIRST ] > 40
      && !privileged( ch, LEVEL_DEMIGOD ) ) {
    if( liquid_table[liquid].hunger > 0
	&& liquid_table[liquid].hunger > liquid_table[liquid].thirst ) {
      // This liquid is really food.
      // Soup, honey, etc.
      if( ch->condition[COND_FULL] > 40 ) {
	if( msg ) {
	  send( ch, "You do not feel hungry or thirsty.\n\r" );
	  if( is_set( ch->status, STAT_ORDERED ) ) {
	    interpret( ch, "shake" );
	  }
	}
	return false;
      }
    } else {
      if( msg ) {
	send( ch, "You do not feel thirsty.\n\r" );
	if( is_set( ch->status, STAT_ORDERED ) ) {
	  interpret( ch, "shake" );
	}
      }
      return false;
    }
  }

  const int alcohol = liquid_table[liquid].alcohol;
  if( ch->condition[ COND_DRUNK ] + alcohol >= 6*ch->tolerance( ) ) {
    if( msg ) {
      send( ch, "Your stomach revolts at the idea of drinking more.\n\r" );
    }
    return false;
  }

  return true;
}


bool drink( char_data* ch, obj_data* obj )
{
  if( !can_drink( ch, obj, true ) )
    return false;

  const bool endless = obj->pIndexData->item_type == ITEM_FOUNTAIN
    || obj->value[1] < 0;

  int amount = number_range( 20, 40 );

  if( !endless )
    amount = min( amount, obj->value[1] );

  const int liquid = obj->value[2];
  const int alcohol = amount*liquid_table[liquid].alcohol/10;
  
  const char *me_loc, *them_loc;
  obj_loc_spam( ch, obj, 0, me_loc, them_loc );

  include_liquid = false;
  
  fsend( ch, "You drink %s%s from %s%s.",
	 ( !endless && obj->value[1] == amount ) ? "the last drops of " : "",
	 liquid_name( obj ), obj, me_loc );
  
  fsend_seen( ch, "%s drinks %s from %s%s.",
	     ch, liquid_name( obj ), obj, them_loc );
    
  include_liquid = true;

  if( is_set( obj->value[3], CONSUME_POISON )
      || liquid == LIQ_POISON ) {
    poison( ch, 5, 1+2*amount/10, true );
  }

  if( is_set( obj->value[3], CONSUME_PLAGUE ) ) {
    plague( ch, number_range( 5, 12 ), true );
  }

  if( !privileged( ch, LEVEL_DEMIGOD ) ) {
    ch->condition[ COND_ALCOHOL ] += (alcohol+1)/2;
    gain_drunk( ch, alcohol/2 );
  }
  
  gain_condition( ch, COND_FULL, amount*liquid_table[liquid].hunger/10 );
  gain_condition( ch, COND_THIRST, amount*liquid_table[liquid].thirst/10 );
  
  if( !endless ) {
    obj = (obj_data*) obj->From( 1, true );
    if( ( obj->value[1] -= amount ) == 0 ) {
	//	&& obj->pIndexData->item_type != ITEM_FOUNTAIN ) {
      remove_bit( obj->extra_flags, OFLAG_KNOWN_LIQUID );
      remove_bit( obj->value[3], CONSUME_POISON );
      if( number_range( 0, 1 ) == 1 )
	remove_bit( obj->value[3], CONSUME_PLAGUE );
    }
    obj->To( );
  }

  int spell = liquid_table[liquid].spell;

  if( spell == -1 ) 
    return true;

  if( spell < SPELL_FIRST || spell >= SPELL_FIRST+table_max[ TABLE_SKILL_SPELL ] ) {
    bug( "Do_drink: Liquid with non-spell skill." );
    return true;
  }

  char_data *caster = 0;
  obj_data *dummy;
  char_array dumaud;

  if( !check_target( spell, caster, ch, ch, dummy, dumaud ) )
    return true;

  // ( victim && !obj && !ch ) -> victim drank liquid.
  if( cast_triggers( spell, 0, 0, ch, 0 ) )
    return true;

  time_data start;
  gettimeofday( &start, 0 );
  
  if( Tprog_Data *tprog = skill_spell_table[spell].prog ) {
    push( );
    clear_variables( );
    var_i = 10;
    var_j = -1;
    //    var_ch = 0;
    var_room = ch->in_room;
    var_victim = ch;
    const int result = tprog->execute( );
    pop( );
    if( !result
	|| !ch->Is_Valid( ) ) {
      finish_spell( start, spell );
      return true;
    }
  }

  if( skill_spell_table[spell].function )
    ( *skill_spell_table[spell].function )( 0, ch, 0, 10, -1 ); 

  finish_spell( start, spell );
  return true;
}


/*
 *   FILL 
 */


void fill( char_data *ch, obj_data *obj, obj_data *fountain )
{
  if( fountain->pIndexData->item_type != ITEM_DRINK_CON
      && fountain->pIndexData->item_type != ITEM_FOUNTAIN ) {
    fsend( ch, "%s isn't something you can fill from.", fountain );
    return;
  }
  if( fountain->pIndexData->item_type == ITEM_DRINK_CON
      && fountain->value[1] == 0 ) {
    include_empty = false; 
    fsend( ch, "%s is empty.", fountain );
    include_empty = true; 
    return;
  }

  int liquid = fountain->value[2];

  if( liquid >= table_max[ TABLE_LIQUID ]
      || liquid < 0 ) {
    bug( "Do_Fill: bad liquid number. (Obj# %d, Liq# %d)",
	 fountain->pIndexData->vnum, liquid );
    liquid = fountain->value[2] = LIQ_WATER;
  }

  if( obj->pIndexData->item_type != ITEM_DRINK_CON ) {
    fsend( ch, "You can't fill %s with liquid.", obj );
    return;
  }

  if( obj == fountain ) {
    fsend( ch, "You can't fill %s from itself.", obj );
    return;
  }

  if( obj->value[1] >= obj->value[0] ) {
    fsend( ch, "%s is already full.", obj );
    return;
  }

  obj = (obj_data *) obj->From( 1, true );

  if( fountain->value[2] == LIQ_POISON
      || is_set( fountain->value[3], CONSUME_POISON ) ) {
    set_bit( obj->value[3], CONSUME_POISON );
  }

  if( is_set( fountain->value[3], CONSUME_PLAGUE ) ) {
    set_bit( obj->value[3], CONSUME_PLAGUE );
  }

  fsend( ch, "You fill %s from %s.",
	 obj, fountain );
  fsend_seen( ch, "%s fills %s from %s.",
	      ch, obj, fountain );
  
  if( obj->value[1] == 0 ) { 
    obj->value[2] = fountain->value[2];
  } else if( obj->value[2] != fountain->value[2] ) {
    if( obj->value[2] != LIQ_SLIME && fountain->value[2] != LIQ_SLIME ) {
      fsend( ch,
	     "The mixture of the two liquids seems to have created something rather unpalatable." );
    }
    obj->value[2] = LIQ_SLIME;
  }
  
  if( fountain->pIndexData->item_type == ITEM_FOUNTAIN
      || fountain->value[1] < 0 ) {
    obj->value[1] = obj->value[0];
  } else {
    fountain = (obj_data *) fountain->From( 1, true );
    
    if( fountain->value[1] < obj->value[0] - obj->value[1] ) {
      obj->value[1] += fountain->value[1];
      fountain->value[1] = 0;
    } else {
      fountain->value[1] -= obj->value[0] - obj->value[1];
      obj->value[1] = obj->value[0];
    }
    
    fountain->To( );
  }
  
  assign_bit( obj->extra_flags, OFLAG_KNOWN_LIQUID,
	      is_set( fountain->extra_flags, OFLAG_KNOWN_LIQUID) );

  obj->To( );
  react_filled( ch, obj, obj->value[2] );
}


void do_fill( char_data* ch, const char *argument )
{
  char               arg  [ MAX_INPUT_LENGTH ];
  obj_data*     fountain;

  if( word_count( argument ) == 1 ) {
    if( !( fountain = find_type( ch, *ch->array, ITEM_FOUNTAIN ) ) ) {
      send( ch, "You see nothing obvious to fill from.\n\r" );
      return;
    }
  } else {  
    if( !two_argument( argument, "from", arg ) ) {
      send( ch, "Syntax: Fill <object> [from] <container>.\n\r" );
      return;
    }
    if( !( fountain = one_object( ch, argument,
				  "fill",
				  &ch->contents,
				  &ch->wearing,
				  ch->array ) ) ) 
      return;
    argument = arg;
  }
  
  obj_data *obj;
  if( !( obj = one_object( ch, argument, "fill",
			   &ch->contents,
			   &ch->wearing ) ) ) 
    return;

  fill( ch, obj, fountain );
}


bool react_filled( char_data* ch, obj_data* obj, int liquid )
{
  int spell;

  if( ( spell = liquid_table[liquid].spell ) < 0 )
    return false;

  if( spell < SPELL_FIRST || spell >= SPELL_FIRST+table_max[ TABLE_SKILL_SPELL ] ) {
    bug( "Do_fill: Liquid with non-spell skill." );
    return false;
  }

  spell -= SPELL_FIRST;

  // ( victim && obj && !ch ) -> ch filled obj if duration == -4.

  if( cast_triggers( spell, 0, 0, ch, obj ) )
    return true;

  time_data start;
  gettimeofday( &start, 0 );
  
  if( Tprog_Data *tprog = skill_spell_table[spell].prog ) {
    push( );
    clear_variables( );
    var_i = 10;
    var_j = -4;
    //    var_ch = 0;
    var_room = ch->in_room;
    var_victim = ch;
    var_obj = obj;
    const int result = tprog->execute( );
    pop( );
    if( !result
	|| !ch->Is_Valid( ) ) {
      finish_spell( start, spell );
      return true;
    }
  }

  if( skill_spell_table[spell].function )
    ( *skill_spell_table[spell].function )( 0, ch, obj, 10, -4 );

  finish_spell( start, spell );
  return true;
}


/*
 *   EMPTY ROUTINES
 */


static thing_data *not_container( thing_data* thing, char_data*, thing_data* )
{
  obj_data *obj = object( thing );

  if( obj->pIndexData->item_type == ITEM_DRINK_CON
      || obj->pIndexData->item_type == ITEM_POTION )
    return obj;

  return 0;
}


static thing_data *already_empty( thing_data* thing, char_data*, thing_data* )
{
  obj_data *obj = object( thing );

  if( obj->pIndexData->item_type == ITEM_POTION
      || obj->value[1] != 0 )
    return obj;

  return 0;
}


static thing_data *fountain( thing_data* thing, char_data*, thing_data* )
{
  obj_data *obj = object( thing );

  if( obj->pIndexData->item_type == ITEM_POTION
      || obj->value[0] >= 0 )
    return obj;

  return 0;
}


static thing_data *empty_con( thing_data* thing, char_data*, thing_data* )
{
  obj_data* obj         = object( thing );
  Content_Array* where  = obj->array;

  if( obj->pIndexData->item_type == ITEM_POTION ) {
    obj->Extract( obj->Selected( ) );
    if( obj_data *vial = potion_container( obj, obj->Selected( ) ) ) {
      vial->To( *where );
    }

  } else {
    obj = (obj_data*) obj->From( obj->Selected( ), true );
    obj->value[1] = 0;
    remove_bit( obj->value[3], CONSUME_POISON );
    if( number_range( 0, 1 ) == 1 )
      remove_bit( obj->value[3], CONSUME_PLAGUE );
    obj->To( );
  }
    
  return obj;
}


void empty( char_data *ch, thing_array& stuff )
{
  thing_array   subset  [ 4 ];
  thing_func*     func  [ 4 ]  = { not_container, already_empty,
				   fountain, empty_con };

  sort_objects( ch, stuff, 0, 4, subset, func );
  
  include_empty = false;
  
  page_priv( ch, 0, empty_string );
  page_priv( ch, &subset[0], "emptying", 0, "doesn't make sense" );
  page_priv( ch, &subset[1], 0, 0, "is already empty", "are already empty" );
  page_priv( ch, &subset[2], "can't empty" );
  page_publ( ch, &subset[3], "empty", 0, empty_string, empty_string, "emptie" );
  
  include_empty = true;
}


void do_empty( char_data* ch, const char *argument )
{
  thing_array*  array;

  if( !( array = several_things( ch, argument, "empty",
				 &ch->contents,
				 &ch->wearing ) ) ) 
    return;
 
  empty( ch, *array );

  delete array;
}
