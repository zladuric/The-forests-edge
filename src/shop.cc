#include <ctype.h>
#include <math.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include "define.h"
#include "struct.h"


shop_data*  shop_list = 0;
const char* shop_flags [MAX_SHOP] = { "buys.stolen", "no.melt" };


/*
 *   SHOP OBJECT PROPERTIES
 */


bool Obj_Data :: Damaged( ) const
{
  return( 10*condition < 9*repair_condition( this ) );
}


/*
 *   UPDATE ROUTINE
 */


void shop_update( void )
{
  for( int i = 0; i < mob_list; ++i ) {
    mob_data *keeper = mob_list[i];

    if( !keeper->Is_Valid( )
	|| !keeper->pShop
	|| !keeper->reset
	|| player_in_room( keeper->in_room ) )
      continue;

    reset_shop( keeper );

    for( int j = keeper->contents-1; j >= 0; j-- ) {
      obj_data *obj = (obj_data*) keeper->contents[j];
      if( obj->for_sale ) {
	int remove = 0;
	const int chance = obj->sold ? 75 : 10;
	for( int k = obj->Number( ); k > 0; --k ) {
	  if( number_range( 1, chance ) == 1 )
	    ++remove;
	}
	if( remove != 0 ) {
	  obj->Extract( remove );
	}
      }
    }
  }
}


/*
 *   COST ROUTINES
 */


static bool will_trade( mob_data* keeper, obj_data* obj, bool buy )
{
  if( !keeper->pShop )
    return false;

  if( buy ) {
    return obj->for_sale;
  }

  if( obj->materials != 0 
      && keeper->pShop->materials != 0
      && ( obj->materials & keeper->pShop->materials ) == 0 )
    return false;

  if( obj->pIndexData->item_type == ITEM_FOOD ) {
    if( obj->value[0] < obj->pIndexData->value[0] ) {
      // Partially-eaten food.
      return false;
    }
    if( obj->pIndexData->value[1] != COOK_BURNT
	&& obj->value[1] == COOK_BURNT ) {
      // Burnt food.
      return false;
    }
  }

  if( obj->pIndexData->item_type == ITEM_CORPSE
      && obj->weight < obj->value[2] ) {
    // Partially-eaten corpse.
    return false;
  }

  return is_set( keeper->pShop->buy_type, obj->pIndexData->item_type );
}


int haggle( char_data *ch, int cost, bool buy )
{
  if( !ch || ch->species || cost <= 0 )
    return cost;

  return cost + (1-2*buy)*ch->get_skill( SKILL_HAGGLE )*cost/100;
}


static int get_cost( mob_data* keeper, char_data* ch, obj_data* obj, bool buy, int num = 0 )
{
  /* NOTE: also see calculation in skill.cc:do_appraise() */

  if( !obj
      || !buy && is_set( obj->extra_flags, OFLAG_NO_SELL )
      || !will_trade( keeper, obj, buy ) )
    return 0;

  if( num == 0 )
    num = obj->Selected( );

  int number = 0;
  //  int start = 0;
  for( int i = 0; i < keeper->contents; ++i ) {
    obj_data *inv = object( keeper->contents[i] );
    if( inv->pIndexData == obj->pIndexData
	&& inv->for_sale ) {
      /*
      if( inv == obj ) {
	start = number;
      }
      */
      number += inv->Number( );
    }
  }
  
  int cost = obj->Cost( );

  /*
  if( !is_set( obj->extra_flags, OFLAG_IDENTIFIED )
      && obj->pIndexData->fakes != 0 ) {
    if( obj_clss_data *fake = get_obj_index( obj->pIndexData->fakes ) ) {
      if( fake->cost <= 0 ) {
	cost /= 2;
      } else {
	cost = fake->cost;
      }
    }
  }
  */

  cost = haggle( ch, cost, buy );
  
  // Keeper buys for less than he sells the same item.
  if( !buy ) {
    if( obj->pIndexData->item_type == ITEM_ARMOR
	|| obj->pIndexData->item_type == ITEM_WEAPON ) {
      cost /= 3;
    }
    // Fencing fee.
    if( ch && !obj->Belongs( ch ) )
      cost = 2 * cost / 3;
  } else {
    cost *= 4;
  }
  
  cost = cost * obj->condition / obj->Durability() * sqr(4-obj->rust) / 16;
  
  switch( obj->pIndexData->item_type ) {
  case ITEM_WAND :
    if( obj->value[0] >= 0 ) {
      if( obj->pIndexData->value[3] > 0 ) {
	// Un-charged wand is worth 10% of full value.
	cost *= 9*obj->value[3] + obj->pIndexData->value[3];
	cost /= 10*obj->pIndexData->value[3];
      }
    } else {
      cost /= 2;
    }
    break;
    
  case ITEM_REAGENT:
    if( obj->pIndexData->value[0] > 0 ) {
      cost *= obj->value[0] + obj->pIndexData->value[0];
      cost /= 2*obj->pIndexData->value[0];
    }
    break;

  case ITEM_DRINK_CON :
    if( buy )
      cost += obj->value[1]*liquid_table[ obj->value[2] ].cost/25;
    break;

  case ITEM_GEM :
    if( obj->value[1] > GEM_UNCUT
	&& obj->value[1] < GEM_FLAWLESS ) {
      cost /= 2;
    } else if( obj->value[1] == GEM_FLAWLESS ) {
      cost *= 2;
    }
    break;
  }    
  
  int total = 0;
  for( int i = 0; i < num; ++i )
    total += cost*3/(3+number+( buy ? -i : i ) );
    //    total += cost*3/(3+number+( buy ? -(i+start) : i ) );

  if( buy && total == 0 ) {
    total = 1;
  }

  return total;
}


static int repair_cost( mob_data* keeper, obj_data* obj )
{
  if( !will_trade( keeper, obj, false )
      || obj->pIndexData->repair > keeper->pShop->repair )
    return 0;
  
  obj_clss_data *obj_clss = obj->pIndexData;
  
  int cost = 50*obj_clss->repair*keeper->pShop->repair
    +obj->cost*( obj->Durability() - obj->condition )/( 2*obj->Durability() );
  
  cost = cost*(20+obj->age)/20;  
  
  return cost;
}


/*
 *   BUY FUNCTION 
 */


static bool buyable_pet( mob_data *pet )
{
  if( pet->leader
      || !is_set( pet->species->act_flags, ACT_CAN_TAME ) )
    return false;

  return true;
}


static thing_data *cantafford( thing_data* t1, char_data* ch, thing_data* )
{
  obj_data *obj = (obj_data*) t1;

  if( obj->temp <= ch->temp ) {
    ch->temp -= obj->temp;
    return obj;
  }
      
  return 0;
}


void do_buy( char_data *ch, const char *argument )
{
  mob_data *keeper = find_keeper( ch );

  if( !keeper
      || ch == keeper )
    return;

  char             buf  [ MAX_INPUT_LENGTH ];
  room_data*      room;

  /* PET SHOP */

  if( is_set( ch->in_room->room_flags, RFLAG_PET_SHOP ) ) {
    if( ch->species ) {
      process_tell( keeper, ch, "I don't sell pets to the likes of you." );
      return;
    }

    if( !( room = get_room_index( ch->in_room->vnum+1 ) ) ) {
      send( ch, "The pet shop is still under construction.\n\r" );
      return;
    }

    if( !*argument ) {
      process_tell( keeper, ch, "Would you like to buy a pet from my @elist@n?" );
      return;
    }

    thing_array list;
    mob_data *pet;

    for( int i = 0; i < room->contents; i++ ) 
      if( ( pet = mob( room->contents[i] ) )
	  && buyable_pet( pet ) )
        list += pet;

    if( !( pet = one_mob( ch, argument, empty_string, &list ) ) ) {
      process_tell( keeper, ch, "That isn't on my @elist@n of pets for sale." );
      return;
    }

    if( pet->Level() > ch->Level() ) {
      snprintf( buf, MAX_INPUT_LENGTH, "You don't look experienced enough to own %s.", pet->Name() );
      process_tell( keeper, ch, buf );
      return;
    }

    const int cost = haggle( ch, pet->species->price, true );

    if( cost == 0 ) {
      snprintf( buf, MAX_INPUT_LENGTH, "I haven't set a price on %s yet, so it isn't for sale.", pet->Name() );
      process_tell( keeper, ch, buf );
      return;
    }

    if( is_set( pet->species->act_flags, ACT_MOUNT ) ) {
      if( has_mount( ch ) )
        return;
    } else {
      if( number_of_pets( ch ) >= 2 ) {
        process_tell( keeper, ch, "You already have two pets and may not buy another." );
        return;
      }
    }

    snprintf( buf, MAX_INPUT_LENGTH, "You hand %s", keeper->Name( ch ) ); 
    if( !remove_coins( ch, cost, buf ) ) {
      if( !privileged( ch, LEVEL_APPRENTICE ) ) {
	snprintf( buf, MAX_INPUT_LENGTH, "You cannot afford %s.", pet->Name() );
	process_tell( keeper, ch, buf );
        return;
      }
      send( ch, "You don't have enough gold, but it doesn't seem to matter.\n\r" );
    }

    pet->From( );
    pet->To( *ch->array );

    fsend( ch, "You buy %s as a pet.", pet);
    fsend_seen( ch, "%s buys %s as a pet.", ch, pet );

    ch->improve_skill( SKILL_HAGGLE );

    set_bit( pet->status, STAT_PET );

    add_follower( pet, ch );

    process_tell( keeper, ch, "Good luck.");

    unregister_reset( pet );

    return;
  }

  /* OBJECT SHOP */

  if( !*argument ) {
    process_tell( keeper, ch,
		  "Would you like to buy items from my @elist@n?" );
    return;
  }

  thing_array list;

  for( int i = 0; i < keeper->contents; ++i ) {
    obj_data *obj = (obj_data*) keeper->contents[i];
    if( will_trade( keeper, obj, true ) )
      list += obj;
  }
  
  thing_array *array;

  if( !( array = several_things( ch, argument, empty_string, &list ) ) ) {
    process_tell( keeper, ch, "That isn't on my @elist@n of items for sale." );
    return;
  }

  int money = get_money( ch );
  ch->temp = money;

  for( int i = 0; i < *array; ++i ) {
    obj_data *obj = (obj_data*) array->list[i];
    obj->temp = get_cost( keeper, ch, obj, true );
  }

  thing_array   subset  [ 5 ];
  thing_func*     func  [ 5 ]  = { heavy, many, 0, cantafford, to_char };

  sort_objects( ch, *array, 0, 3, subset, func );
  
  // Recalc costs based on how many you can hold.
  for( int i = 0; i < subset[2]; ++i ) {
    obj_data *obj = ((obj_data*)subset[2][i]);
    obj->temp = get_cost( keeper, ch, obj, true );
  }

  sort_objects( ch, subset[2], 0, 2, subset+3, func+3 );

  page_priv( ch, 0, empty_string );
  page_priv( ch, &subset[0], "can't lift" );
  page_priv( ch, &subset[1], "can't handle" );
  page_priv( ch, &subset[3], "can't afford" );

  rehash( ch, subset[4] );

  if( none_shown( subset[4] ) ) {
    delete array;
    return;
  }

  if( !one_shown( subset[4] ) ) {
    page_publ( ch, &subset[4], "buy", keeper, "from", "for" );
  } else {
    page_publ( ch, &subset[4], "buy", keeper, "from" );
  }

  if( ( money -= ch->temp ) > 0 ) {
    char msg [ TWO_LINES ];
    snprintf( msg, TWO_LINES, "You hand %s", keeper->Seen_Name( ch ) );
    remove_coins( ch, money, msg, true );
    ch->improve_skill( SKILL_HAGGLE );
  }

  delete array;
}


void do_list( char_data* ch, const char *argument )
{
  mob_data *keeper = find_keeper( ch );

  if( !keeper
      || ch == keeper )
    return;

  thing_array*  array;
  obj_data*       obj;
  room_data*     room;

  if( !( keeper = find_keeper( ch ) ) )
    return;

  if( !*argument )
    argument = "all";

  /* PET SHOP */

  if( is_set( ch->in_room->room_flags, RFLAG_PET_SHOP ) ) {
    if( !( room = get_room_index( ch->in_room->vnum+1 ) ) ) {
      send( ch, "The pet shop is still under construction.\n\r" );
      return;
    }

    mob_data *pet;
    thing_array list;

    for( int i = 0; i < room->contents; i++ ) 
      if( ( pet = mob( room->contents[i] ) )
	  && buyable_pet( pet ) )
        list += pet;

    if( list.is_empty() ) {
      process_tell( keeper, ch, "Sorry, I'm out of pets right now." );
      return;
    }

    if( !( array = several_things( ch, argument, "list", &list ) ) )
      return;

    send( ch, "Copper Pieces: %d\n\r\n\r", get_money( ch ) );
    send_underlined( ch,
      "Pet                                    Price    Level\n\r" );

    for( int i = 0; i < *array; i++ ) {
      pet = (mob_data*) array->list[i];
      send( ch, "%-35s %8d %8d\n\r",
	    pet->Seen_Name( ch, 1 ),
	    haggle( ch, pet->species->price, true ),
	    pet->Level() );
    }

    delete array;
    return;
  }

  /* OBJECT SHOP */

  thing_array list;

  for( int i = 0; i < keeper->contents; i++ ) {
    obj = (obj_data*) keeper->contents[i];
    if( get_cost( keeper, ch, obj, true, 1 ) > 0 )
      list += obj;
    }

  if( list.is_empty() ) {
    process_tell( keeper, ch, "Sorry, I have nothing to sell right now." );
    return;
  }
  
  if( !( array = several_things( ch, argument, "list", &list ) ) )
    return;

  rehash( ch, list, true );

  /*
  // Rehash without changing temp values.
  for( int i = 0; i < *array; ++i ) {
    array->list[i]->Show( array->list[i]->Selected( ) );
  }

  for( int i = 0; i < array->size-1; ++i ) {
    obj_data *obj1 = (obj_data*) array->list[i];
    if( obj1->Shown( ) > 0 ) {
      for( int j = i+1; j < *array; ++j ) {
	obj_data *obj2 = (obj_data*) array->list[j];
	if( look_same( ch, obj1, obj2, true ) ) {
	  obj1->Show( obj1->Shown( ) + obj2->Shown( ) );
	  obj2->Show( 0 );
	}
      }
    }
  }
  */

  page( ch, "Copper Pieces: %d\n\r\n\r", get_money( ch ) );
  page_underlined( ch, "Item                               Price  Weight  Level  Number  Condition\n\r" );

  char level [ 5 ];

  include_closed = false;

  for( int i = 0; i < *array; ++i ) {
    obj = (obj_data*) array->list[i];

    if( obj->Shown( ) == 0 )
      continue;

    if( !can_use( ch, 0, obj ) )
      snprintf( level, 5, "***" );
    else
      snprintf( level, 5, "%d", obj->pIndexData->level );

    const char *name = obj->Seen_Name( ch, 1 );
    int len = strlen( name );

    if( len <= 32 ) {
      page( ch, "%-32s%8d%8.2f%7s%8d  %-s\n\r",
	    name,
	    get_cost( keeper, ch, obj, true, 1 ),
	    double( obj->Weight( 1 ) ) / 100.0,
	    level,
	    obj->Shown( ),
	    obj->condition_name( ch, true ) );
    } else {
      page( ch, "%s\n\r", name );
      page( ch, "%-32s%8d%8.2f%7s%8d  %-s\n\r",
	    "",
	    get_cost( keeper, ch, obj, true, 1 ),
	    double( obj->Weight( 1 ) ) / 100.0,
	    level,
	    obj->Shown( ),
	    obj->condition_name( ch, true ) );
    }
  }

  include_closed = true;

  delete array;
}


/*
 *   SELL
 */


static thing_data *uninterested( thing_data* thing, char_data* ch,
			  thing_data* keeper )
{
  obj_data* obj = (obj_data*) thing;

  if( ( obj->temp = get_cost( (mob_data*) keeper, ch, obj, false ) ) <= 0 ) 
    return 0;

  return obj;
}


static thing_data *not_empty( thing_data* obj, char_data*, thing_data* )
{
  return( obj->contents == 0 ? obj : 0 );
}


static thing_data *sell_stolen( thing_data* thing, char_data* ch, thing_data *thing2 )
{
  //  obj_data *obj = (obj_data*) thing;
  mob_data *keeper = (mob_data*) thing2;

  if( is_set( keeper->pShop->flags, SHOP_STOLEN ) )
    return thing;

  return stolen( thing, ch );

  //  if( !obj->Belongs( ch ) )
  //    return 0;

  // Can't sell non-empty, so why bother...
  //  for( int i = 0; i < obj->contents; i++ )
  //    if( !stolen( obj->contents[i], ch ) )
  //      return 0;

  //  return obj;
}


static bool happy;


static thing_data *sold( thing_data *thing, char_data *ch, thing_data *keeper )
{
  obj_data *obj = (obj_data*) thing->From( thing->Selected( ) );

  const bool id = is_set( obj->extra_flags, OFLAG_IDENTIFIED );

  set_bit( obj->extra_flags, OFLAG_IDENTIFIED );
  set_bit( obj->extra_flags, OFLAG_KNOWN_LIQUID );
  
  // Re-charge sold wands.
  //  if( obj->pIndexData->item_type == ITEM_WAND )
  //    obj->value[3] = obj->pIndexData->value[3];

  free_string( obj->label, MEM_OBJECT );
  obj->label = empty_string;
  
  obj->for_sale = true;
  obj->sold = true;

  ch->temp += obj->temp;

  if( obj->Enchantment( ) < 0
      || ( obj->pIndexData->item_type == ITEM_WAND
	   && obj->value[3] == 0 ) ) {
    // Junk cursed items.
    // Junk empty wands.
    obj->Extract( );
  } else {
    if( obj->Enchantment( ) > 0 && !id ) {
      happy = true;
    }
    obj->To( keeper );
  }

  // Note: this may return extracted obj to prevent returning consolidated items
  // in keeper inventory.
  return obj;
}


void do_sell( char_data* ch, const char *argument )
{
  mob_data *keeper = find_keeper( ch );

  if( !keeper
      || ch == keeper )
    return;

  if( is_set( ch->in_room->room_flags, RFLAG_PET_SHOP ) ) {
    process_tell( keeper, ch, "I don't buy pets." );
    return;
  }

  if( !*argument ) {
    process_tell( keeper, ch, "What would you like to sell?" );
    return;
  }

  thing_array *array;

  if( !( array = several_things( ch, argument, "sell", &ch->contents ) ) ) 
    return;

  ch->temp = 0;
  happy = false;

  thing_array subset [5];
  thing_func *func [5] = { cursed, sell_stolen, uninterested, not_empty, sold };

  sort_objects( ch, *array, keeper, 5, subset, func );

  page_priv( ch, 0, empty_string );
  page_priv( ch, &subset[0], "can't let go of" );
  page_priv( ch, &subset[1], "don't own" );
  page_priv( ch, &subset[2], "isn't interested in", keeper );
  page_priv( ch, &subset[3], 0, 0, "isn't empty", "aren't empty" );

  rehash( ch, subset[4] );

  if( none_shown( subset[4] ) ) {
    delete array;
    return;
  }

  if ( !one_shown( subset[4] ) ) {
    page_publ( ch, &subset[4], "sell", keeper, "to", "for" );
  } else {
    page_publ( ch, &subset[4], "sell", keeper, "to" );
  }

  bool fence = false;
  bool curse = false;

  for( int i = 0; i < subset[4]; ++i ) {
    obj_data *obj = (obj_data*) subset[4][i];
    if( !obj->Belongs( ch ) )
      fence = true;

    if( obj->Enchantment( ) < 0 ) {
      curse = true;
    }

    if( obj->owner ) {
      obj->owner = 0;
      consolidate( obj );
    }
  }

  if( curse && !happy ) {
    fsend_seen( keeper, "%s scowls furiously at %s.", keeper, ch );
    fpage( ch, "%s scowls furiously at you.", keeper );
  } else if( happy && !curse ) {
    fsend_seen( keeper, "%s smirks at %s.", keeper, ch );
    fpage( ch, "%s smirks at you.", keeper );
  }

  if( fence && !curse) {
    fsend_seen( keeper, "%s winks conspiratorially at %s.", keeper, ch );
    fpage( ch, "%s winks conspiratorially at you.", keeper );
  }

  if( ch->temp > 0 ) {
    char msg [ TWO_LINES ];
    snprintf( msg, TWO_LINES, "%s hands you", keeper->Seen_Name( ch ) );
    add_coins( ch, ch->temp, msg, true );
  }

  delete array;
}


void do_value( char_data* ch, const char *argument )
{
  mob_data *keeper = find_keeper( ch );

  if( !keeper
      || ch == keeper )
    return;

  if( is_set( ch->in_room->room_flags, RFLAG_PET_SHOP ) ) {
    process_tell( keeper, ch, "I don't buy pets" );
    return;
  }

  obj_data *obj;

  if( !( obj = one_object( ch, argument, "value", &ch->contents ) ) ) 
    return;

  if( !obj->droppable( ch ) ) {
    send( ch, "You can't let go of %s.\n\r", obj );
    return;
  }
  
  char buf [ MAX_STRING_LENGTH ];

  if( !obj->Belongs( ch )
      && !is_set( keeper->pShop->flags, SHOP_STOLEN ) ) {
    snprintf( buf, MAX_STRING_LENGTH, "%s is stolen so I would never buy it.",
	      obj->Seen_Name( ch ) );
    *buf = toupper( *buf );
    process_tell( keeper, ch, buf );
    return;
  }

  const int cost = get_cost( keeper, ch, obj, false );

  if( cost > 0
      && !is_set( obj->extra_flags, OFLAG_IDENTIFIED )
      && obj->pIndexData->fakes != 0 ) {
    snprintf( buf, MAX_STRING_LENGTH, "%s could be a fake, so I will not estimate its value until you are ready to sell.",
	      obj->Seen_Name( ch ) );
    *buf = toupper( *buf );
    process_tell( keeper, ch, buf );
    return;    
  }

  const int rcost = repair_cost( keeper, obj );
  
  if( obj->Damaged( ) ) {
    if( rcost > 0 ) {
      int repair = repair_condition( obj );
      if( obj->condition >= repair ) {
	snprintf( buf, MAX_STRING_LENGTH,
		  "I see %s is damaged.  I could repair it slightly for %d cp %s ",
		  obj->Seen_Name( ch ), rcost, cost > 0 ? "or" : "but" ); 
      } else {
	snprintf( buf, MAX_STRING_LENGTH,
		  "I see %s is damaged.  I could repair it to %s for %d cp %s ",
		  obj->Seen_Name( ch ), obj->condition_name( ch, true, repair ), rcost, cost > 0 ? "or" : "but" ); 
      }
    } else {
      snprintf( buf, MAX_STRING_LENGTH, "I see %s is damaged.  I am unable to repair it %s ",
		obj->Seen_Name( ch ), cost > 0 ? "but" : "and" );
    }
    if( cost > 0 ) 
      sprintf( buf+strlen( buf ), "would give you %d cp for it.", cost );
    else
      strcat( buf, "am uninterested in buying it." );
  } else {
    if( cost > 0 )
      snprintf( buf, MAX_STRING_LENGTH, "I would pay you %d cp for %s.",
		cost, obj->Seen_Name( ch ) ); 
    else
      snprintf( buf, MAX_STRING_LENGTH, "I am uninterested in buying %s.",
		obj->Seen_Name( ch ) );
  }
  
  const int blocks = obj->pIndexData->blocks;

  if( blocks > 0
      && !is_set( keeper->pShop->flags, SHOP_NOMELT ) ) {
    
    obj_clss_data *clss;
    
    for( int metal = MAT_BRONZE; metal < table_max[ TABLE_MATERIAL ]; ++metal )
      if( is_set( obj->materials, metal )
	  && ( clss = get_obj_index( material_table[metal].ingot[0] ) )
	  && is_set( keeper->pShop->materials, metal ) ) {
	sprintf( buf+strlen( buf ),
		 "  I could melt it down to produce %s for %d cp.",
		 clss->Name( blocks ),
		 blocks*100 );
	break;
      }
  }
  
  process_tell( keeper, ch, buf );
}


/*
 *   REPAIR ROUTINES
 */


void do_repair( char_data* ch, const char *argument )
{
  mob_data *keeper = find_keeper( ch );

  if( !keeper
      || ch == keeper )
    return;
  
  if( keeper->pShop->repair == 0 ) {
    process_tell( keeper, ch, "Sorry, I do not repair items." );
    return;
  }
  
  char           buf  [ MAX_INPUT_LENGTH ];
  obj_data*      obj;
  int           cost;
  int           cond;

  if( !( obj = one_object( ch, argument, "repair", &ch->contents ) ) ) 
    return;

  if( !obj->droppable( ch ) ) {
    send( ch, "You can't let go of %s.\n\r", obj );
    return;
  }
  
  if( ( cost = repair_cost( keeper, obj ) ) <= 0 ) {
    snprintf( buf, MAX_INPUT_LENGTH, "%s isn't something I can repair.", obj->Seen_Name( ch ) );
    *buf = toupper( *buf );
    process_tell( keeper, ch, buf );
    return;
  }
  
  if( ( cond = repair_condition( obj ) ) < 0 ) {
    snprintf( buf, MAX_INPUT_LENGTH, "That %s is too old to be worth repairing.", obj->Seen_Name( ch, 1, true ) );
    process_tell( keeper, ch, buf );
    return;
  }

  if( !obj->Damaged( ) ) {
    snprintf( buf, MAX_INPUT_LENGTH, "That %s isn't damaged enough to be worth repairing.", obj->Seen_Name( ch, 1, true ) );
    process_tell( keeper, ch, buf );
    return;
  }

  snprintf( buf, MAX_INPUT_LENGTH, "%s repairs %s for you at a cost of", keeper->Name( ch ), obj->Seen_Name( ch ) );
  *buf = toupper( *buf );
  if( !remove_coins( ch, cost, buf ) ) {
    snprintf( buf, MAX_INPUT_LENGTH, "You can't afford the cost of repairing %s.", 
	      obj->Seen_Name( ch ) );
    process_tell( keeper, ch, buf );   
    return;
  }
  
  fsend_seen( ch, "%s repairs %s for %s.", keeper, obj, ch );

  // You actually hand it to him, so it's not no-move.
  //  obj->no_move = true;
  obj = (obj_data*) obj->From( 1 );

  obj->age += (int)( 1+obj->rust*pow( cond-obj->condition,1.5 )/100.0 );
  obj->condition = cond;

  obj->To( ch );
}


/*
 *   MELT
 */


void do_melt( char_data *ch, const char *argument )
{
  mob_data *keeper = find_keeper( ch );

  if( !keeper
      || ch == keeper )
    return;

  obj_data *obj;

  if( !( obj = one_object( ch, argument, "melt", &ch->contents ) ) ) 
    return;
  
  obj_clss_data *clss = 0;  // Init for compiler warning.
  int metal;

  for( metal = MAT_BRONZE; metal < table_max[ TABLE_MATERIAL ]; ++metal )
    if( is_set( obj->materials, metal )
	&& ( clss = get_obj_index( material_table[metal].ingot[0] ) ) )
      break;
  
  if( !clss ) {
    fsend( ch, "%s is not made out of metal.", obj );
    return;
  }

  if( is_set( keeper->pShop->flags, SHOP_NOMELT ) ) {
    process_tell( keeper, ch,
		  "Sorry, I am unable to melt metals." );
    return;
  }

  char buf  [ MAX_INPUT_LENGTH ];

  int blocks;
  if( ( blocks = obj->pIndexData->blocks ) == 0 ) {
    snprintf( buf, MAX_INPUT_LENGTH, "%s does not contain enough metal to be worth melting.",
	      obj->Seen_Name( ch ) );
    buf[0] = toupper( buf[0] );
    process_tell( keeper, ch, buf );
    //    fsend( ch, "%s does not contain enough metal to be worth melting.", obj );
    return;
  }
  
  if( !is_set( keeper->pShop->materials, metal ) ) {
    snprintf( buf, MAX_INPUT_LENGTH, "Sorry, %s is not made out of a metal I can melt.",
	      obj->Seen_Name( ch ) );
    process_tell( keeper, ch, buf );
    //    process_tell( keeper, ch,
    //		  "Sorry, that is not made out of a metal I can melt." );
    return;
  }
  
  if( blocks == -1 ) {
    snprintf( buf, MAX_INPUT_LENGTH, "I cannot melt %s.", obj->Seen_Name( ch ) );
    process_tell( keeper, ch, buf );
    return;
  }

  int cost = blocks*100;

  if( !obj->droppable( ch ) ) {
    fsend( ch, "You can't let go of %s.", obj );
    return;
  }
  
  snprintf( buf, MAX_INPUT_LENGTH, "You hand %s to %s along with",
	    obj->Seen_Name( ch ), keeper->Seen_Name( ch ) );
  if( !remove_coins( ch, cost, buf ) ) { 
    snprintf( buf, MAX_INPUT_LENGTH, "You can't afford my fee of %d cp to melt %s.",
	      cost, obj->Seen_Name( ch ) );
    process_tell( keeper, ch, buf );
    return;
  }
    
  fsend_seen( ch, "%s has %s melted down.", ch, obj );

  snprintf( buf, MAX_INPUT_LENGTH, "%s takes %s and places it in the furnace.  ",
	    keeper->Seen_Name( ch ), obj->Seen_Name( ch ) );
  buf[0] = toupper( buf[0] );
  
  obj->Extract( 1 );
  
  if( !( obj = create( clss, blocks ) ) ) {
    bug( "Repair: Ingot for %s does not exist.", material_table[metal].name );
    return;
  }
  
  obj->Show( blocks );
  set_bit( obj->materials, metal );
  
  int length = strlen( buf );
  snprintf( buf+length, MAX_INPUT_LENGTH-length,
	    "%s then pulls it out and after much hammering and reheating hands you %s.",
	    keeper->He_She( ), obj->Seen_Name( ch, blocks ) );
  buf[length] = toupper( buf[length] );
  fsend( ch, buf );
  
  obj->To( ch );
}


/*
 *  ONLINE EDITING OF SHOPS
 */


void do_shedit( char_data* ch, const char *argument )
{
  char                  buf  [MAX_STRING_LENGTH ];
  mob_data*          keeper;
  shop_data*           shop;
  char_data*         victim;
  species_data*     species;
  int                number;
  
  for( shop = shop_list; shop; shop = shop->next )
    if( ch->in_room == shop->room ) 
      break;  

  if( *argument && !ch->can_edit( ch->in_room ) )
    return;

  if( exact_match( argument, "new" ) ) {
    if( shop ) {
      send( ch, "There is already a shop here.\n\r" );
      return;
    }
    shop = new shop_data;
    shop->room = ch->in_room;
    shop->custom = 0;
    shop->keeper = -1;
    shop->repair = 0;
    shop->materials = 0;
    shop->buy_type[0] = 0;
    shop->buy_type[1] = 0;
    shop->next = shop_list;
    shop_list = shop;
    send( ch, "New shop created here.\n\r" );
    return;
  }

  if( !shop ) {
    send( ch, "There is no shop associated with this room.\n\r" );
    return;
  }
  
  if( !*argument ) {
    species = get_species( shop->keeper ); 
    sprintf( buf, "Shop Keeper: %s  [ Vnum: %d ]\n\r\n\r",
	     ( species ? species->Name() : "none" ), shop->keeper );
    sprintf( buf+strlen( buf ), "Repair: %d\n\r\n\r", shop->repair );
    send( ch, buf );
    return;
  }
  
  if( exact_match( argument, "delete" ) ) {
    remove( shop_list, shop );
    for( int i = 0; i < mob_list; ++i )
      if( mob_list[i]->pShop == shop )
        mob_list[i]->pShop = 0;
    for( custom_data *custom = shop->custom; custom; custom = custom->next ) {
      fsend( ch, "Custom deleted: %s.", custom->item );
    }
    delete shop;
    send( ch, "Shop deleted.\n\r" );
    return;
  }
   
  if( matches( argument, "keeper" ) ) {
    if( !( victim = one_character( ch, argument, "set keeper",
				   ch->array ) ) )
      return;

    if( !( keeper = mob( victim ) ) ) {
      send( ch, "Players cannot be shop keepers.\n\r" );
      return;
    }
    shop->keeper = keeper->species->vnum;
    keeper->pShop = shop;
    fsend( ch, "Shop keeper set to %s.", keeper->descr->name );
    return;
  }
  
  if( matches( argument, "repair" ) ) {
    if( ( number = atoi( argument ) ) < 0 || number > 10 ) {
      send( ch,
	    "A shop's repair level must be between 0 and 10.\n\r" ); 
      return;
    }
    shop->repair = number;
    send( ch, "The shop's repair level is set to %d.\n\r", number );
  }
}


void do_shflag( char_data* ch, const char *argument )
{
  shop_data *shop;

  for( shop = shop_list; shop; shop = shop->next )
    if( ch->in_room == shop->room ) 
      break;  

  if( !shop ) {
    send( ch, "This room has no shop entry.\n\r" );
    return;
  }

#define types 3

  const char*  title [types] = { "Basic", "Obj_Types", "Materials" };
  int            max [types] = { MAX_SHOP, MAX_ITEM, table_max[ TABLE_MATERIAL ] };

  const char** name1 [types] = { &shop_flags[0], &item_type_name[0],
				 &material_table[0].name };
  const char** name2 [types] = { &shop_flags[1], &item_type_name[1],
				 &material_table[1].name };
  
  int*    flag_value [types] = { &shop->flags, shop->buy_type,
				 &shop->materials };
  const int uses_flag [types] = { 1, 1, 1 };
  const bool sort [types] = { true, true, true };

  if( const char *response = flag_handler( title, name1, name2,
					   flag_value, 0, max,
					   uses_flag, sort, 0,
					   ch, argument, "shop",
					   types ) ) {
    if( *response ) {
      room_log( ch, ch->in_room->vnum, response );
    }
  }

#undef types
}


/*
 *   MISC SHOP ROUTINES
 */


mob_data *active_shop( char_data* ch )
{
  room_data *room = ch->in_room;
  
  if( !is_set( room->room_flags, RFLAG_PET_SHOP )
      && !is_set( room->room_flags, RFLAG_SHOP ) ) 
    return 0;
  
  for( int i = 0; i < room->contents; i++ ) {
    mob_data *keeper;
    if( ( keeper = mob( room->contents[i] ) )
	&& keeper->pShop
	&& keeper->pShop->room == room
	&& keeper->reset	// Keeper must be reset, not mloaded.
	&& IS_AWAKE( keeper )
	&& ( privileged( ch, LEVEL_APPRENTICE ) || ch->Seen( keeper ) ) )
      return keeper;
  }
  
  return 0;
}


mob_data *find_keeper( char_data* ch )
{
  mob_data *keeper = 0;

  for( int i = 0; ; i++ ) {
    if( i >= *ch->array ) {
      if( is_set( ch->in_room->room_flags, RFLAG_PET_SHOP )
	  || is_set( ch->in_room->room_flags, RFLAG_SHOP ) ) {
        send( ch, "The shop keeper is not around right now.\n\r" );
        return 0;
      }
      send( ch, "You are not in a shop.\n\r" );
      return 0;
    }
    if( ( keeper = mob( ch->array->list[i] ) )
	&& keeper->pShop
	&& keeper->Seen( ch ) )
      break;
  }

  if( !IS_AWAKE( keeper ) ) {
    send( ch, "The shopkeeper seems to be asleep.\n\r" );
    return 0;
  }
   
  if( !ch->Seen( keeper )
      && !privileged( ch, LEVEL_APPRENTICE ) ) {
    do_say( keeper, "I don't trade with folks I can't see." );
    return 0;
  }
  
  if( !ch->can_carry() ) {
    send( ch, "You can't carry anything so shopping is rather pointless.\n\r" );
    return 0;
  }

  return keeper;
}


/*
 *   DISK ROUTINES
 */


void load_shops( void )
{
  shop_data *shop = 0;
  int i, j;

  echo( "Loading Shops...\n\r" );

  FILE *fp = open_file( AREA_DIR, SHOP_FILE, "r" );
  
  if( strcmp( fread_word( fp ), "#SHOPS" ) ) 
    panic( "Load_shops: header not found" );

  while( ( i = fread_number( fp ) ) != -1 ) {

    if( i == 0 ) {
      custom_data *custom = new custom_data;
      custom->item = get_obj_index( fread_number( fp ) );
      custom->cost = fread_number( fp );
  
      for( i = 0; i < MAX_INGRED; ++i ) {
        if( ( j = fread_number( fp ) ) == 0 ) {
          fread_number( fp );
          continue;
	}
        custom->ingred[i] = get_obj_index( j );
        custom->number[i] = fread_number( fp );
      }
      
      if( !custom->item ) {
        roach( "Load_Shops: Removing null custom item." );
        delete custom;
      } else if( !shop ) {
        roach( "Load_Shops: Custom in null shop?" );
        delete custom;
      } else {
	// Prevent list reversal with each reboot.
	append( shop->custom, custom );
      }
   
      fread_to_eol( fp );
      continue;
    }

    shop = new shop_data;

    shop->keeper = fread_number( fp );
  
    shop->flags       = fread_number( fp );
    shop->buy_type[0] = fread_number( fp );
    shop->buy_type[1] = fread_number( fp );

    shop->repair      = fread_number( fp );
    shop->materials   = fread_number( fp );
    shop->open_hour   = fread_number( fp );
    shop->close_hour  = fread_number( fp );
    
    fread_to_eol( fp );
    
    if( !( shop->room = get_room_index( i ) ) ) {
      roach( "Load_Shops: Deleting shop in non-existent room %d.", i ); 
      delete shop;
      shop = 0;
    } else {
      shop->next = shop_list;
      shop_list = shop; 
    }
  }

  fclose( fp );
}


void save_shops( )
{
  rename_file( AREA_DIR, SHOP_FILE,
	       AREA_PREV_DIR, SHOP_FILE );
  
  FILE *fp;
  
  if( !( fp = open_file( AREA_DIR, SHOP_FILE, "w" ) ) )
    return;
  
  fprintf( fp, "#SHOPS\n" );
  
  for( shop_data *shop = shop_list; shop; shop = shop->next ) {
    fprintf( fp, "%5d %5d ", shop->room->vnum, shop->keeper );
    fprintf( fp, "%5d %5d %5d ", shop->flags, shop->buy_type[0],
	     shop->buy_type[1] );
    fprintf( fp, "%2d %5d %5d %5d\n", shop->repair, shop->materials,
	     shop->open_hour, shop->close_hour );
    for( custom_data *custom = shop->custom; custom; custom = custom->next ) {
      fprintf( fp, "    0 %5d %5d ", custom->item->vnum, custom->cost );
      for( int i = 0; i < MAX_INGRED; i++ )
        fprintf( fp, "%5d %2d ",
		 custom->ingred[i] ? custom->ingred[i]->vnum : 0,
		 custom->number[i] );
      fprintf( fp, "\n" );
    }
  }
  fprintf( fp, "-1\n\n#$\n\n" );
  fclose( fp );
}
