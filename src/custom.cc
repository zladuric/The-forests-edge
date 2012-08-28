#include <ctype.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include "define.h"
#include "struct.h"


/*
 *   SUPPORT ROUTINES
 */


static int get_metal( const custom_data *custom )
{
  for( int i = 0; i < MAX_INGRED; i++ ) {
    if( !custom->ingred[i] )
      continue;
    for( int j = MAT_BRONZE; j <= MAT_ADAMANTINE; j++ )
      if( is_set( custom->ingred[i]->materials, j ) )
        return j;
  }

  return MAT_BRONZE;
}


static bool will_repair( const shop_data *shop, const custom_data *custom )
{
  if( custom->item->repair > shop->repair )
    return false;

  if( shop->materials != 0 ) {
    int materials = custom->item->materials;
    if( is_set( custom->item->extra_flags, OFLAG_RANDOM_METAL ) ) {
      set_bit( materials, get_metal( custom ) );
    }
    if( materials != 0
	&& ( materials & shop->materials ) == 0 )
      return false;
  }

  return true;
}


static int custom_level( const custom_data *custom )
{
  const obj_clss_data *obj_clss = custom->item;
  
  if( ( obj_clss->item_type != ITEM_WEAPON
	&& obj_clss->item_type != ITEM_ARMOR )
      || !is_set( obj_clss->extra_flags, OFLAG_RANDOM_METAL ) )
    return obj_clss->level;

  const int metal = get_metal( custom ) - MAT_BRONZE;

  return ( metal > 0 )
    ? ( 5*metal + obj_clss->level )
    : obj_clss->level;
}


static void append_liquid( char *tmp, const obj_clss_data *clss, bool ident )
{
  if( !include_liquid )
    return;
  
  if( clss->item_type != ITEM_DRINK_CON
      || ( clss->value[1] != -1 && clss->value[1] <= 0 )
      || clss->value[2] < 0 || clss->value[2] >= table_max[ TABLE_LIQUID ] )
    return;
  
  if( ident ) {
    sprintf( tmp+strlen( tmp ), " of %s",
	     liquid_table[clss->value[2]].name );
  } else {
    sprintf( tmp+strlen( tmp ), " containing %s",
	     liquid_table[clss->value[2]].color );
  }
}


const char *item_name( const custom_data *custom, bool brief )
{
  const obj_clss_data *obj_clss = custom->item;
  const char *name = obj_clss->Name( 1, brief );
  char *tmp = static_string( );

  if( !is_set( obj_clss->extra_flags, OFLAG_RANDOM_METAL ) ) {
    strcpy( tmp, name );
  } else { 
    snprintf( tmp, THREE_LINES, "%s %s",
	      material_table[ get_metal( custom ) ].name,
	      name );
  }

  append_liquid( tmp, obj_clss, true );
  return tmp;
}


// Custom-fit items can only be made if they will fit the buyer.
static bool can_custom( char_data *ch, obj_clss_data *obj_clss )
{
  if( !is_set( obj_clss->size_flags, SFLAG_CUSTOM ) )
    return true;

  // RACE SIZE
  if( is_set( obj_clss->size_flags, SFLAG_RACE )
      && ( ch->shdata->race >= MAX_PLYR_RACE
	   || !is_set( obj_clss->size_flags, SFLAG_HUMAN+ch->shdata->race ) ) )
    return false;
  
  // SIZE
  if( is_set( obj_clss->size_flags, SFLAG_SIZE )
      && !( is_set( obj_clss->size_flags, SFLAG_RANDOM )
	    || is_set( obj_clss->size_flags, wear_size( ch ) ) ) )
    return false;

  return true;
}


static custom_data *find_custom( char_data *ch, mob_data *keeper,
				 const char *argument, custom_data* custom )
{
  char arg [ MAX_INPUT_LENGTH ];

  const bool numbered = isdigit( *argument );
  const int n = smash_argument( arg, argument );

  if( n == 0 ) {
    send( ch, find_zero );
    return 0;
  }

  if( n < 0 || *argument ) {
    fsend( ch, find_one, "custom" );
    return 0;
  }

  if( !*arg ) {
    send( ch, find_keyword );
    return 0;
  }

  const bool scored = !numbered && ( n == 1 );
  int value = 0;
  int count = 0;
  int i;
  custom_data *found  = 0;

  for( ; custom; custom = custom->next ) {
    if( ( i = is_name( arg, item_name( custom ), scored ) ) ) {
      if( !scored ) {
	if( ++count >= n ) {
	  return custom;
	}
      } else if( i > value ) {
	value = i;
	found = custom;
      }
    }
  }

  if( found )
    return found;

  if( count > 0 ) {
    fsend( ch, find_few,
	   number_word( count ),
	   count == 1 ? "" : "s", arg ); 
    return 0;
  }

  process_tell( keeper, ch, "I can make nothing of that name." );
  return 0;
}


/*
 *   BUILDER CUSTOM COMMANDS
 */


void do_shcustom( char_data* ch, const char *argument )
{
  wizard_data *imm;

  if( !( imm = wizard( ch ) ) )
    return;

  mob_data *keeper;

  if( !( keeper = find_keeper( ch ) ) )
    return;

  room_data *room;

  if( !( room = Room( ch->array->where ) ) ) {
    send( ch, "You aren't in a room.\n\r" );
    return;
  }
 
  if( is_set( room->room_flags, RFLAG_PET_SHOP ) ) {
    send( ch, "Pet shops cannot have customs.\n\r" );
    return;
  }
  
  char              buf  [ MAX_STRING_LENGTH ];
  obj_data*         obj;
  int                 i;
  thing_array*    array;

  shop_data *shop = keeper->pShop;
  custom_data *custom = locate( shop->custom, imm->custom_edit );

  if( matches( argument, "exit" ) ) {
    if( !custom )
      send( ch, "You are already not editing a custom.\n\r" );
    else {
      send( ch, "You no longer edit a custom.\n\r" );
      imm->custom_edit = 0;
    }
    return;
  }

  if( !custom ) {
    if( !*argument ) {
      if( !( custom = shop->custom ) ) {
        send( ch, "This shop is has no customs.\n\r" );
        return;
      }
      send_title( ch, "Customs - %s", keeper->descr->name );
      send_underlined( ch, "Index  Vnum   Obj\n\r" );
      bool repair_msg = false;
      for( i = 1; custom; custom = custom->next, i++ ) {
	if( !will_repair( shop, custom ) )
	  repair_msg = true;
        send( ch, "%4d%7d   %s%s\n\r", i,
	      custom->item->vnum,
	      item_name( custom ),
	      will_repair( shop, custom ) ? "" : "*" );
      }
      if( repair_msg ) {
	send( ch, "\n\r" );
	send_centered( ch, "[ Items marked with '*' cannot be repaired by this shop. ]\n\r" );
      }
      return;
    }
    
    if( number_arg( argument, i ) ) {
      if( !( custom = locate( shop->custom, i ) ) ) {
        send( ch, "No custom exists with that index.\n\r" );
        return;
      }
      if( !*argument ) {
        send( ch, "You now edit custom #%d.\n\r", i );
        imm->custom_edit = i;
        return;
      }
      if( !( obj = one_object( ch, argument, 
			       "custom", &ch->contents ) ) )
        return;
      custom->item = obj->pIndexData;
      fsend( ch, "Item created by custom set to %s.",
	     item_name( custom ) );
      return;
    }
    
    if( matches( argument, "delete" ) ) {
      if( !( custom = locate( shop->custom, atoi( argument ) ) ) ) 
        send( ch, "No custom exists with that index.\n\r" );
      else {
        fsend( ch, "Custom for %s deleted.", item_name( custom ) );
        remove( shop->custom, custom );
        delete custom;
      }
      return;
    }
    
    if( !( obj = one_object( ch, argument,
			     "custom", &ch->contents ) ) ) 
      return;

    custom = new custom_data;
    custom->item = obj->pIndexData;
    custom->next = shop->custom;
    shop->custom = custom;
  
    fsend( ch, "%s added to shop custom list.", item_name( custom ) );
    return;
  }
  
  if( !*argument ) {
    send( ch, "Item: %s\n\r", item_name( custom ) );
    send( ch, "Cost: %d\n\r\n\r", custom->cost );
    *buf = '\0';
    for( i = 0; i < MAX_INGRED; i++ ) {
      if( custom->ingred[i] ) {
        sprintf( buf + strlen( buf ), "[%2d] %s (%d)\n\r",
		 i+1,
		 custom->ingred[i]->Name( custom->number[i], true ),
		 custom->ingred[i]->vnum );
      }
    }
    if( !*buf )
      sprintf( buf, "No ingredients.\n\r" );
    send( ch, buf );
    return;
  }

  if( matches( argument, "cost" ) ) {
    custom->cost = atoi( argument );
    fsend( ch, "Cost for labor on %s set to %d cp.",
	   item_name( custom ), custom->cost );
    return;
  } 

  if( matches( argument, "delete" ) ) {
    i = atoi( argument )-1;
    if( i >= 0 && i < MAX_INGRED && custom->ingred[i] ) {
      custom->ingred[i] = 0;
      send( ch, "Ingredient removed.\n\r" );
      return;
    }
    send( ch, "Ingredient not found to remove.\n\r" ); 
    return;
  }

  if( !( array = several_things( ch, argument, "add as an ingredient", &ch->contents ) ) ) 
    return;

  if( array->size != 1 ) {
    send( ch, "You may only add one type of ingredient at a time.\n\r" );
    delete array;
    return;
  }

  if( !( obj = object( array->list[0] ) ) ) {
    send( ch, "Only objects may be ingredients.\n\r" );
    delete array;
    return;
  }

  for( i = 0; i < MAX_INGRED; i++ ) 
    if( !custom->ingred[i] ) {
      custom->ingred[i] = obj->pIndexData;
      custom->number[i] = obj->Selected( );
      fsend( ch, "%s is added as an ingredient for %s.",
	     obj, item_name( custom ) );
      delete array;
      return;
    }

  fsend( ch, "%s has the maximum number of ingredients (%d) allowed.",
	 item_name( custom ), MAX_INGRED );

  delete array;
} 


/*
 *   PLAYER CUSTOM COMMANDS
 */


static void display_custom( char_data* ch, mob_data* keeper )
{
  if( !ch->pcdata )
    return;

  shop_data *shop = keeper->pShop;

  if( !shop->custom ) {
    process_tell( keeper, ch, "There is nothing I can custom for you." );
    return;
  }

  char             tmp  [ FIVE_LINES ];
  char           level  [ 5 ];

  send( ch, "Copper Pieces: %d\n\r\n\r", get_money( ch ) );
  send_underlined( ch,
    "Item                        Price  Weight  Lvl  Ingredients\n\r" );

  for( const custom_data *custom = shop->custom; custom; custom = custom->next ) {
    int lvl = custom_level( custom );
    if( !can_use( ch, custom->item, 0 )
	|| lvl > ch->Level( ) )
      strcpy( level, "***" );
    else
      snprintf( level, 5, "%d", lvl );
    
    const char *name = item_name( custom );
    const int len = strlen( name );
    
    if( len <= 27 ) {
      snprintf( tmp, FIVE_LINES, "%-27s%6d%8.2f%5s  ",
		name,
		haggle( ch, custom->cost ),
		double( custom->item->weight ) / 100.0,
		level );
      capitalize( tmp );
    } else {
      page( ch, "%s\n\r", name );
      snprintf( tmp, FIVE_LINES, "%-27s%6d%8.2f%5s  ",
		"",
		haggle( ch, custom->cost ),
		double( custom->item->weight ) / 100.0,
		level );
    }

    bool flag = false;
    for( int i = 0; i < MAX_INGRED; ++i ) {
      if( !custom->ingred[i] )
        continue;
      const char *name = custom->ingred[i]->Name( custom->number[i], true );
      if( strlen( tmp )+strlen( name ) > 77 && i != 0 ) {
        strcat( tmp, "\n\r" );
        page( ch, tmp );
        snprintf( tmp, FIVE_LINES, "%48s", "" );
        flag = false;
      }  
      sprintf( tmp+strlen( tmp ), "%s%s",
	       ( flag ? ", " : "" ),
	       name );
      flag = true;
    }
    
    strcat( tmp, "\n\r" );
    page( ch, tmp );
  }
}


void do_custom( char_data* ch, const char *argument )
{
  mob_data *keeper;

  if( !( keeper = find_keeper( ch ) ) )
    return;

  room_data *room;

  if( ( room = Room( ch->array->where ) )
      && is_set( room->room_flags, RFLAG_PET_SHOP ) ) {
    process_tell( keeper, ch, "I cannot manufacture a pet for you.\
 Perhaps you'd like to buy one from my @elist@n?" );
    return;
  }

  if( !*argument ) {
    display_custom( ch, keeper );
    return;
  }

  char                tmp  [ TWO_LINES ];
  custom_data*     custom;
  obj_data*           obj;

  shop_data *shop = keeper->pShop;

  if( !( custom = find_custom( ch, keeper, argument, shop->custom ) ) ) {
    return;
  }

  if( !can_custom( ch, custom->item ) ) {
    snprintf( tmp, TWO_LINES, "I cannot make %s in your size.",
	      item_name( custom, false ) );
    process_tell( keeper, ch, tmp );
    return;
  }

  obj_array list;
  Content_Array money;
  bool fence = false;
  
  for( int i = 0; i < MAX_INGRED; ++i ) {
    if( !custom->ingred[i] )
      continue;
  
    int k = 0;
    int l = 0;
    int m = 0;
    int f = 0;
    int g = 0;
    int b = 0;
    int c = 0;

    for( int j = ch->contents.size - 1; ; --j ) {
      if( j == -1 ) {
	const char *name = item_name( custom, false );
	const int n = l + m + f + g + b + c;
	if( k + n >= custom->number[i] ) {
	  if( f != 0 ) {
	    const int x = max( f, custom->number[i]-k );
	    snprintf( tmp, TWO_LINES, "Someone has been gnawing on %s, so I cannot use %s to make %s.",
		      custom->ingred[i]->Name( x ),
		      x == 1 ? "it" : "them",
		      name );
	  } else if( g != 0 ) {
	    const int x = max( g, custom->number[i]-k );
	    snprintf( tmp, TWO_LINES, "%s %s not of sufficient quality to make %s.",
		      custom->ingred[i]->Name( x ),
		      x == 1 ? "is" : "are",
		      name );
	  } else if( b != 0 ) {
	    const int x = max( b, custom->number[i]-k );
	    snprintf( tmp, TWO_LINES, "%s %s a bit over-cooked for making %s.",
		      custom->ingred[i]->Name( x ),
		      x == 1 ? "is" : "are",
		      name );
	  } else if( c != 0 ) {
	    const int x = max( c, custom->number[i]-k );
	    snprintf( tmp, TWO_LINES, "%s %s in too poor condition for making %s.",
		      custom->ingred[i]->Name( x ),
		      x == 1 ? "is" : "are",
		      name );
	  } else if( l != 0 ) {
	    const int x = max( l, custom->number[i]-k );
	    snprintf( tmp, TWO_LINES, "I will not make %s with stolen materials, and %s %s stolen!",
		      name,
		      custom->ingred[i]->Name( x ),
		      x == 1 ? "is" : "are" );
	  } else if( m != 0 ) {
	    //	    int x = max( m, custom->number[i]-k );
	    snprintf( tmp, TWO_LINES, "I cannot use %s to make %s unless it is empty.",
		      custom->ingred[i]->Name( 1 ),
		      name );
	  }
	} else {
	  snprintf( tmp, TWO_LINES, "Customing %s requires %s, which you do not have.",
		    name,
		    custom->ingred[i]->Name( custom->number[i] ) );
	}
	*tmp = toupper( *tmp );
	process_tell( keeper, ch, tmp );
	money.To( ch->contents );
        return;
      }
      if( ( obj = object( ch->contents[j] ) )
	  && obj->pIndexData == custom->ingred[i] ) {
	if( ( obj->pIndexData->item_type == ITEM_FOOD
	      && obj->value[0] < obj->pIndexData->value[0] )
	    || ( obj->pIndexData->item_type == ITEM_CORPSE
		 && obj->weight < obj->value[2] ) ) {
	  // Partially-eaten food.
	  f += obj->Number( );
	  continue;
	}
	if( obj->pIndexData->item_type == ITEM_GEM
	    && obj->pIndexData->value[1] == GEM_RANDOM
	    && obj->value[1] >= GEM_FRACTURED
	    && obj->value[1] <= GEM_SCRATCHED ) {
	  // Low-quality gems.
	  g += obj->Number( );
	  continue;
	}
	if( obj->pIndexData->item_type == ITEM_FOOD
	    && obj->pIndexData->value[1] != COOK_BURNT
	    && obj->value[1] == COOK_BURNT ) {
	  // Burned food.
	  b += obj->Number( );
	  continue;
	}
	if( custom->item != obj->pIndexData
	    && 2*obj->condition < obj->Durability( ) ) {
	  // Poor condition.
	  // Does not apply to items that are ingredients to repair themselves.
	  c += obj->Number( );
	  continue;
	}
	if( !obj->contents.is_empty( ) ) {
	  m += obj->Number( );
	  continue;
	}
	if( !obj->Belongs( ch ) ) {
	  if( !is_set( keeper->pShop->flags, SHOP_STOLEN ) ) {
	    l += obj->Number( );
	    continue;
	  } else {
	    fence = true;
	  }
	}
        if( custom->number[i]-k <= obj->Number( ) ) {
	  obj->Select( custom->number[i]-k );
	  if( monetary_value( obj ) ) {
	    obj = (obj_data*) obj->From( obj->Selected( ) );
	    obj->To( money );
	  }
	  list += obj;
          break;
	}
        obj->Select_All( );
        k += obj->Number( ); 
	if( monetary_value( obj ) ) {
	  obj = (obj_data*) obj->From( obj->Selected( ) );
	  obj->To( money );
	}
	list += obj;
      }
    }
  }

  int cost = haggle( ch, custom->cost );

  if( cost > 0 ) {
    snprintf( tmp, TWO_LINES, "You hand %s", keeper->Name( ch ) );
    if( !remove_coins( ch, cost, tmp, false ) ) {
      fsend( ch, "You can't afford to custom %s.",
	     item_name( custom, false ) );
      money.To( ch->contents );
      return;
    }
  }

  money.To( ch->contents );

  fsend( ch, "You give %s to %s.", (thing_array*)&list, keeper );
  fsend_seen( ch, "%s gives %s to %s.", ch, (thing_array*)&list, keeper );

  for( int i = 0; i < list; ++i ) {
    obj = list[i];
    obj->Extract( obj->Selected( ) );
  }

  obj = create( custom->item );

  if( is_set( obj->pIndexData->extra_flags, OFLAG_RANDOM_METAL ) ) {
    set_bit( obj->materials, get_metal( custom ) );
    set_alloy( obj, 0 );
  }

  set_quality( obj );
  set_size( obj, ch );

  set_bit( obj->extra_flags, OFLAG_IDENTIFIED );
  set_bit( obj->extra_flags, OFLAG_KNOWN_LIQUID );
  remove_bit( obj->extra_flags, OFLAG_REPLICATE );

  if( fence ) {
    fsend_seen( keeper, "%s winks conspiratorially at %s.", keeper, ch );
    fsend( ch, "%s winks conspiratorially at you.", keeper );
  }
  fsend_seen( keeper, "%s creates %s.", keeper, obj );
  fsend_seen( keeper, "%s gives %s to %s.", keeper, obj, ch );
  fsend( ch, "%s gives you %s.", keeper, obj );

  process_tell( keeper, ch, "Good luck" );
  
  obj->To( ch );
}
