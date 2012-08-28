#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include "define.h"
#include "struct.h"


/*
 *   DO_PUT ROUTINE
 */


static thing_data *trigger( thing_data *thing, char_data *ch, thing_data *container )
{
  obj_data *cont = (obj_data*) container;
  obj_data *obj = (obj_data*) thing;

  if( !obj->Is_Valid( )
      || !cont->Is_Valid( ) )
    return 0;

  for( oprog_data *oprog = cont->pIndexData->oprog; oprog; oprog = oprog->next ) {
    if( oprog->trigger == OPROG_TRIGGER_PUT ) {
      push( );
      clear_variables( );
      var_ch = ch;
      var_obj = (obj_data*)thing;
      var_container = cont;
      const int result = oprog->execute( );
      pop( );
      return( result && obj->Is_Valid( ) ? thing : 0 );
    }
  }

  return thing;
}


static thing_data *put( thing_data* thing, char_data* ch, thing_data* container )
{
  thing = thing->From( thing->Selected( ) );

  if( container->array->where != ch )
    set_owner( (obj_data*) thing, 0, ch );

  thing->To( container );

  return thing;
}


static void put_in( char_data* ch, thing_array* array, obj_data* container )
{
  thing_array    subset  [ 5 ];
  thing_func*      func  [ 5 ]  = { same, cursed, wont_fit, trigger, put };

  sort_objects( ch, *array, container, 5, subset, func );
  
  char where [ THREE_LINES ];
  *where = '\0';

  if( container->array == ch->array ) {
    const char *pos = ch->in_room->position( );
    if( *pos ) {
      snprintf( where, THREE_LINES, " %s", pos );
    } else {
      snprintf( where, THREE_LINES, " here" );
    }
  }

  container->Select( 1 );

  msg_type = MSG_INVENTORY;

  page_priv( ch, 0, empty_string );
  page_priv( ch, &subset[0], "can't fold", 0, "into itself" );
  page_priv( ch, &subset[1], "can't let go of" );
  page_priv( ch, &subset[2], "doesn't have room for", container );
  page_publ( ch, &subset[4], "put", container, "into", where );

  delete array;

  standard_delay( ch, subset[4], container );
}


static void put_on( char_data* ch, thing_array* array, obj_data* container )
{
  thing_array    subset  [ 5 ];
  thing_func*      func  [ 5 ]  = { same, cursed, wont_fit, trigger, put };

  sort_objects( ch, *array, container, 5, subset, func );
  
  container->Select( 1 );

  msg_type = MSG_INVENTORY;

  page_priv( ch, 0, empty_string );
  page_priv( ch, &subset[0], "can't fold", 0, "into itself" );
  page_priv( ch, &subset[1], "can't let go of" );
  page_priv( ch, &subset[2], "doesn't have space for", container );
  page_publ( ch, &subset[4], "put", container, "onto" );

  delete array;

  standard_delay( ch, subset[4], container );
}


void do_put( char_data* ch, const char *argument )
{
  char               arg  [ MAX_STRING_LENGTH ];
  thing_array*     array;
  obj_data*    container = 0;
  thing_data *target = 0;
  char_data *victim = 0;
  int i;

  bool spec_put_in = false,
    spec_put_on = false;

  int sel;

  if( contains_word( argument, "in", arg ) ) {
    spec_put_in = true;
    if( !( container = one_object( ch, argument, "put things in",
				   &ch->contents,
				   &ch->wearing,
				   ch->array ) ) )
      return;
    sel = container->temp;
  } else if( contains_word( argument, "on", arg ) ) {
    spec_put_on = true;
    if( !( target = one_thing( ch, argument, "put things on",
			       ch->array ) ) )
      return;
  } else if ( ( i = word_count( argument ) ) > 2 || i == 0 ) {
    send( ch, "Syntax: put <obj> [in] <container>\n\r\
        put <obj> [on] <surface>\n\r\
        put <obj> [on] <pet>\n\r\
        put <fuel> [on] <fire>\n\r" );
    return;
  } else {
    argument = one_argument( argument, arg );
    if( !( target = (thing_data *) one_visible( ch, argument, "put things in or on",
						(visible_array *) &ch->contents, OBJ_DATA,
						(visible_array *) &ch->wearing, OBJ_DATA,
						(visible_array *) ch->array, THING_DATA ) ) )
      return;
  }
  
  if( target ) {
    container = object( target );
    victim = character( target );
    sel = target->temp;
  }

  if( !container && !victim ) {
    // Not sure what this might be...
    fsend( ch,
	   "You cannot put items in or on %s.",
	   target );
    return;
  }

  // Note: this could change container's select count from 1.
  if( !( array = several_things( ch, arg, "put", &ch->contents ) ) )
    return;

  // Put armor on pets.
  if( victim ) {
    if( is_confused_pet( ch ) ) 
      return;
    if( victim == ch ) {
      send( ch, "You can't put things on yourself... try wear, wield, or hold instead.\n\r" );
      delete array;
      return;
    }
    if( !is_set( victim->status, STAT_PET )
	|| victim->leader != ch ) {
      fsend( ch, "%s is not your pet.", victim );
      delete array;
      return;
    }
    put_on( ch, array, victim );
    return;
  }

  if( container->pIndexData->item_type == ITEM_FIRE ) {
    if( container->array->where != ch->in_room ) {
      send( ch, "You can only add fuel to fires in the room.\n\r" );
      return;
    }
    thing_array fuel, non_fuel, nodrop;
    int weight = 0;
    for( i = array->size-1; i >= 0; --i ) {
      if( obj_data *obj = object( array->list[i] ) ) {
	int flammable = obj->materials
	  & ( ( 1 << MAT_WOOD ) | ( 1 << MAT_PAPER ) | ( 1 << MAT_CLOTH ) );
	if( flammable && obj->materials == flammable ) {
	  fuel += obj;
	  weight += obj->Weight( obj->Selected( ) );
	} else if( !cursed( obj, ch ) ) {
	  nodrop += obj;
	} else {
	  non_fuel += obj;
	}
      }
    }

    page_priv( ch, &nodrop, "can't let go of" );
    page_priv( ch, &non_fuel, "can't burn", container, "in" );

    if( !fuel.is_empty( ) ) {
      container->Select( 1 );
      fsend( ch, "You fuel %s with %s.\n\r", container, &fuel );
      fsend_seen( ch, "%s fuels %s with %s.\n\r", ch, container, &fuel );
      
      if( weight < 20 ) {
	fsend( ch, "The small amount of fuel burns up immediately, with little effect." );
	fsend_seen( ch, "The small amount of fuel burns up immediately, with little effect.", ch );
      } else if( container->timer > 0 && weight > 0 ) {
	container->value[0] += number_range( weight/40 + 1, weight/20 );
      }
      
      for( int i = 0; i < fuel; ++i ) {
	obj_data *obj = (obj_data*) fuel[i];
	obj->Extract( obj->Selected( ) );
      }
    }

    return;
  }

  if( !spec_put_on ) {

    /* PIPES */

    if( container->pIndexData->item_type == ITEM_PIPE ) {
      if( container->contents != 0 ) {
	container->Select( 1 );
	select( container->contents );
	fsend( ch, "%s already contains %s.",
	       container, &container->contents );
      } else if( obj_data *obj = object( array->list[0] ) ) {
	if( *array != 1 || obj->Selected( ) != 1 ) {
	  send( ch, "You can only put one item in a pipe at a time.\n\r" );
	} else if( obj->pIndexData->item_type != ITEM_TOBACCO ) {
	  fsend( ch, "%s is not something you wish to smoke.", obj );
	} else {
	  obj = (obj_data*) put( obj, ch, container );
	  container->Select( 1 );
	  fsend( ch, "You put %s into %s.", obj, container );
	  fsend_seen( ch, "%s puts %s into %s.", ch, obj, container );
	  set_delay( ch, 5 );
	}
      }
      
      delete array;
      return;
    }    
    
    
    /* KEYRINGS */
    
    if( container->pIndexData->item_type == ITEM_KEYRING ) {
      thing_array keys, non_keys;
      for( i = array->size-1; i >= 0; --i ) {
	if( obj_data *obj = object( array->list[i] ) ) {
	  if( obj->pIndexData->item_type != ITEM_KEY ) {
	    if( obj == container ) {
	      container->Select( 1 );
	      fpage( ch, "You can't put %s into itself.", container );
	    } else {
	      non_keys += array->list[i];
	    }
	    array->remove(i);
	  }
	}
      }
      
      page_priv( ch, &non_keys, "can't put", container, "into" );

      put_in( ch, array, container );      
      return;
    }
    
    /* BOWS */
    
    if( container->pIndexData->item_type == ITEM_WEAPON
	&& container->pIndexData->value[3] == WEAPON_BOW-WEAPON_UNARMED ) {
      if( container->contents != 0 ) {
	container->Select( 1 );
	select( container->contents );
	fsend( ch, "%s already contains %s.",
	       container, &container->contents );
      } else if( obj_data *obj = object( array->list[0] ) ) {
	if( *array != 1 || obj->Selected( ) != 1 ) {
	  send( ch, "You can only put one arrow in a bow at a time.\n\r" );
	} else if( obj->pIndexData->item_type != ITEM_ARROW
		   || obj->pIndexData->value[3] != WEAPON_BOW-WEAPON_UNARMED ) {
	  fsend( ch, "%s is not an arrow.", obj );
	} else {
	  obj = (obj_data*) put( obj, ch, container );
	  container->Select( 1 );
	  fsend( ch, "You put %s into %s.", obj, container );
	  fsend_seen( ch, "%s puts %s into %s.", ch, obj, container );
	  set_delay( ch, 5 );
	}
      }
      
      delete array;
      return;
    }

  }

  /* BAGS AND TABLES */

  if( spec_put_in && container->pIndexData->item_type != ITEM_CONTAINER ) {
    container->Select( 1 );
    fsend( ch,
	   "You cannot put items in %s.",
	   container );
    delete array;
    return;
  }

  if( spec_put_on && container->pIndexData->item_type != ITEM_TABLE ) {
    container->Select( 1 );
    fsend( ch,
	   "You cannot put items on %s.",
	   container );
    delete array;
    return;
  }

  if( container->pIndexData->item_type != ITEM_CONTAINER
      && container->pIndexData->item_type != ITEM_TABLE ) {
    container->Select( 1 );
    fsend( ch,
	   "You cannot put items in or on %s.",
	   container );
    delete array;
    return;
  }

  if( container->pIndexData->item_type == ITEM_CONTAINER ) {
    if( is_set( container->value[1], CONT_CLOSED) ) {
      container->Select( 1 );
      include_closed = false;
      fsend( ch, "%s is closed.", container );
      include_closed = true;
      delete array;
      return;
    }
    
    put_in( ch, array, container );

  } else {
    if( container->array != ch->array ) {
      container->Select( 1 );
      fsend( ch, "You can't put things on %s unless it is on the ground.", container );
      delete array;
      return;
    }

    put_on( ch, array, container );
  }
}


void setup_cache( char_data *ch, obj_data *cache )
{
  // Set max capacity.
  int level = ch->Level( );

  cache->value[0] = 10*level;

  // Set an adjective.
  if( level < 21 ) {
    // Small hole, 1-20.
    if( cache->after != cache->pIndexData->after ) {
      free_string( cache->after, MEM_OBJECT );
    }
    cache->after = alloc_string( "small", MEM_OBJECT );
  } else if( level >= 50 && level < 70 ) {
    // Large hole, 50-69.
    if( cache->after != cache->pIndexData->after ) {
      free_string( cache->after, MEM_OBJECT );
    }
    cache->after = alloc_string( "large", MEM_OBJECT );
  } else if( level > 70 ) {
    // Huge hole, 70-99.
    if( cache->after != cache->pIndexData->after ) {
      free_string( cache->after, MEM_OBJECT );
    }
    cache->after = alloc_string( "huge", MEM_OBJECT );
  }
}


void do_bury( char_data* ch, const char *argument )
{
  /* More cache ideas:
     - no player/pet corpses (own maybe?)
     - dig: some chance of finding someone else's cache, or part of it
       by searching?
  */

  if( is_mob( ch )
      || is_mounted( ch, "bury anything" )
      || is_ridden( ch, "bury anything" )
      || is_entangled( ch, "bury anything" )
      || is_fighting( ch, "bury anything" )
      || is_drowning( ch, "bury anything" ) ) {
    return;
  }

  thing_array *array;
  if( !( array = several_things( ch, argument, "bury", &ch->contents ) ) )
    return;

  obj_data *tool = find_oflag( ch->wearing, OFLAG_TOOL_DIG );

  if( !tool ) {
    send( ch, "You must be holding a digging tool to bury anything.\n\r" );
    return;
  }

  room_data *room = ch->in_room;
  int terrain = room->sector_type;
  int move = 2*terrain_table[ terrain ].mv_cost;

  if( ch->move < move ) {
    send( ch, "You are too exhausted to dig a hole.\n\r" );
    return;
  }

  ch->move -= move;

  if( !room->Seen( ch ) ) {
    send( ch, "You can't bury things when you can't even see the ground!\n\r" );
    return;
  }

  if( !is_set( terrain_table[ terrain ].flags, TFLAG_BURY )
      || is_set( room->room_flags, RFLAG_SAVE_ITEMS ) ) {
    send( ch, "You can't bury anything here.\n\r" );
    return;
  }

  // Don't allow burying where an acode might prevent digging.
  for( action_data *action = room->action; action; action = action->next ) {
    if( member( "dig", action->command, true ) ) {
      send( ch, "You can't bury anything here.\n\r" );
      return;
    }
  }


  obj_data *cache = 0;

  // See if the player already has a cache here.
  for( int i = 0; i < room->contents; ++i ) {
    if( obj_data *obj = object( room->contents[i] ) ) {
      if( obj->pIndexData->vnum == OBJ_CACHE
	  && obj->owner == ch->pcdata->pfile ) {
	cache = obj;
	fpage( ch, "After digging around for a while with %s, you locate your existing cache.",
	       tool );
	fsend_seen( ch, "%s digs around for a while with %s, and uncovers a cache of items.",
		    ch, tool );
	// Re-start the cache timer.
	cache = (obj_data*) cache->From( 1, true );
	cache->To( );
	break;
      }
    }
  }

  const char *surface = room->surface( );

  if( !*surface ) {
    surface = "ground";
  }

  if( !cache ) {
    if( ch->pcdata->pfile->caches.size >= MAX_CACHE( ch ) ) {
      send( ch, "You can't have more than %d caches.\n\r",  MAX_CACHE( ch ) );
      return;
    }
    fpage( ch, "Using %s, you dig a hole in the %s to conceal your stash.",
	   tool, surface );
    fsend_seen( ch, "Using %s, %s digs a hole in the %s.",
		tool, ch, surface );
    cache = create( get_obj_index( OBJ_CACHE ) );
    cache->To( room );
  }

  // Set cache size.
  // Do this even if it's an old cache, to allow player level increase to take effect.
  setup_cache( ch, cache );

  // Put things into the cache.
  thing_array    subset  [ 5 ];
  thing_func*      func  [ 5 ]  = { same, cursed, wont_fit, trigger, put };

  sort_objects( ch, *array, cache, 5, subset, func );
  
  // So you can see it when you put things into it.
  remove_bit( cache->extra_flags, OFLAG_SECRET );

  msg_type = MSG_INVENTORY;
  include_closed = false;

  page_priv( ch, 0, empty_string );
  page_priv( ch, &subset[0], "can't fold", 0, "into itself" );
  page_priv( ch, &subset[1], "can't let go of" );
  page_priv( ch, &subset[2], "doesn't have room for", cache );
  page_publ( ch, &subset[4], "put", cache, "into" );

  include_closed = true;
  set_bit( cache->extra_flags, OFLAG_SECRET );

  delete array;

  if( cache->contents.is_empty( ) ) {
    fpage( ch, "You fill in the empty hole you dug for no reason." );
    fsend_seen( ch, "%s fills in the empty hole %s dug for no reason.",
		ch, ch->He_She( ) );
    cache->Extract( );
  } else {
    fpage( ch, "Looking around furtively, you quickly fill in the hole." );
    fsend_seen( ch, "Looking around furtively, %s quickly fills in the hole.",
		ch );
    cache->owner = ch->pcdata->pfile;
    ch->pcdata->pfile->caches += cache;
  }

  damage_weapon( ch, tool );

  set_delay( ch, 32 );

  /*
  obj_data *obj;

  if( !( obj = one_object( ch, argument, "bury",
			   &ch->contents,
			   ch->array ) ) ) 
    return;

  if( obj->pIndexData->item_type != ITEM_CORPSE ) {
    send( ch, "You can only bury corpses.\n\r" );
    return;
  }

  if( !is_set( terrain_table[ ch->in_room->sector_type ].flags, TFLAG_BURY ) ) {
    send( ch, "You can't bury anything here.\n\r" );
    return;
  }

  if( !forbidden( obj, ch ) ) {
    fsend( ch, "You are forbidden from burying %s.", obj );
    return;
  }

  fsend( ch, "Looking around furtively, you quickly bury %s.", obj );
  fsend_seen( ch, "Looking around furtively, %s quickly buries %s.", ch, obj );

  obj->Extract( );
  */
}
