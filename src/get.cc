#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include "define.h"
#include "struct.h"


/*
 *   DO_GET ROUTINE
 */


bool valid_container( char_data *ch, obj_data *obj, bool msg )
{
  if( obj != forbidden( obj, ch ) ) {
    if( msg )
      send( ch, "You can't steal from that corpse.\n\r" );
    return false;
  }
  
  /*
  if( obj->pIndexData->item_type != ITEM_CONTAINER
      && obj->pIndexData->item_type != ITEM_CORPSE
      && obj->pIndexData->item_type != ITEM_TABLE 
      && obj->pIndexData->item_type != ITEM_KEYRING
      && obj->pIndexData->item_type != ITEM_PIPE
      && ( obj->pIndexData->item_type != ITEM_WEAPON
	   || obj->pIndexData->value[3] != WEAPON_BOW-WEAPON_UNARMED ) ) {
  */
  if( !obj->pIndexData->is_container( ) ) {
    if( msg )
      fsend( ch, "%s isn't a container.", obj );
    return false;
  }
  
  if( obj->pIndexData->item_type == ITEM_CONTAINER
      && is_set( &obj->value[1], CONT_CLOSED ) ) {
    if( msg ) {
      const char *me_loc, *them_loc;
      obj_loc_spam( ch, obj, 0, me_loc, them_loc );
      include_closed = false;
      fsend( ch, "%s%s is closed.", obj, me_loc );
      include_closed = true;
    }
    return false;
  }
  
  if( obj->contents.is_empty() ) {
    if( msg ) {
      const char *me_loc, *them_loc;
      obj_loc_spam( ch, obj, 0, me_loc, them_loc );
      fsend( ch, "%s%s contains nothing.", obj, me_loc );
    }
    return false;
  }
  
  return true;
}


void do_get( char_data* ch, const char *argument )
{
  char                 arg  [ MAX_INPUT_LENGTH ];
  thing_array*       array;

  if( !ch->can_carry() ) {
    if( !is_confused_pet( ch ) )
      send( ch, "You are unable to carry items.\n\r" );
    return; 
  }
  
  if( !*argument ) {
    send( ch, "What do you wish to take?\n\r" );
    return;
  }
  
  if( !two_argument( argument, "from", arg ) ) {
    // Don't bother trying to get non-mimic chars.
    thing_array stuff;
    for( int i = 0; i < *ch->array; ++i ) {
      thing_data *thing = ch->array->list[i];
      if( object( thing ) ) {
	stuff += thing;
      } else if( char_data *rch = character( thing ) ) {
	if( rch->species
	    && is_set( rch->species->act_flags, ACT_MIMIC ) ) {
	  stuff += thing;
	}
      }
    }
    if( !( array = several_things( ch, argument, "get", &stuff ) ) ) 
      return;

    get_obj( ch, *array );
    delete array;
    return;
  }
  
  obj_data *obj;
  if( !( obj = one_object( ch, argument, "get from",
			     &ch->contents,
			     &ch->wearing,
			     ch->array ) ) ) 
    return;
  
  if( !valid_container( ch, obj, true ) )
    return;
  
  if( !( array = several_things( ch, arg, "get", &obj->contents ) ) )
    return;

  get_obj( ch, *array, obj );

  delete array;
}


/*
 *   CONTAINERS
 */


thing_data* forbidden( thing_data* thing, char_data* ch, thing_data* )
{
  obj_data *obj;

  if( !( obj = object( thing ) ) )
    return thing;
 
  //  pfile_data *pfile;

  if( obj->pIndexData->vnum != OBJ_CORPSE_PC
      && obj->pIndexData->vnum != OBJ_CORPSE_PET
      || !obj->owner
      )
    return obj;

  // !pcdata seems correct here, allows familiar to loot corpse.
  if( !ch->pcdata )
    return 0;

  if( ch->Level() >= LEVEL_APPRENTICE 
      || ch->pcdata->pfile == obj->owner )
    return obj;
  
  if( player_data *player = find_player( obj->owner ) ) {
    if( player->In_Game( ) ) {
      return( can_kill( ch, player, false )
	      || player->Befriended( ch ) ? obj : 0 ); 
    }
  }

  /*
  for( int i = 0; i < player_list; ++i ) {
    player_data *player = player_list[i];
    if( player->pcdata->pfile == obj->owner ) {
      if( player->In_Game( ) )
	return( can_kill( ch, player, false )
		|| player->Befriended( ch ) ? obj : 0 ); 
      break;
    }
  }
  */
  
  return obj;
}


static thing_data *trigger( thing_data *thing, char_data *ch, thing_data *container )
{
  obj_data *cont = (obj_data*) container;
  obj_data *obj = (obj_data*) thing;

  if( !obj->Is_Valid( )
      || cont && !cont->Is_Valid( ) )
    return 0;

  if( cont ) {
    for( oprog_data *oprog = cont->pIndexData->oprog; oprog; oprog = oprog->next ) {
      if( oprog->trigger == OPROG_TRIGGER_GET_FROM ) {
	push( );
	clear_variables( );
	var_ch = ch;
	var_obj = obj;
	var_container = cont;
	const int result = oprog->execute( );
	pop( );
	if( !result
	    || !obj->Is_Valid( ) )
	  return 0;
	break;
      }
    }
  }

  for( oprog_data *oprog = obj->pIndexData->oprog; oprog; oprog = oprog->next ) {
    if( oprog->trigger == OPROG_TRIGGER_GET ) {
      push( );
      clear_variables( );
      var_ch = ch;
      var_obj = obj;
      if( cont ) {
	var_container = cont;
      } else {
	var_room = ch->in_room;
      }
      const int result = oprog->execute( );
      pop( );
      if( !result
	  || !obj->Is_Valid( ) )
	return 0;
      break;
    }
  }

  return thing;
}


void get_obj( char_data *ch, thing_array& array, obj_data *container, bool pager )
{
  if( array.is_empty( ) )
    return;

  ch->Select( 1 );

  thing_array subset [ 8 ];
  thing_func *func [ 8 ] = { cant_take, forbidden, heavy,
			     sat_on, on_top, many, trigger, to_char };

  // If someone gets all the light from the room, we still need to see the get.
  int light = 0;
  bool fix_light = !container && ch->in_room;
  if( fix_light ) {
    light = ch->in_room->contents.light;
  }

  sort_objects( ch, array, container, 8, subset, func );

  msg_type = MSG_STANDARD;

  if( container ) {
    // Prevent "You get <blah> from an empty <blah>."
    // but still allow "You get an empty <blah> from <blah>."
    if( container->contents.size == 0 )
      container->contents.size = -1;  
    if( container->array != ch->array )   
      msg_type = MSG_INVENTORY;
  }

  if( fix_light && ch->in_room->contents.light <= 0 && light > 0 ) {
    swap( light, ch->in_room->contents.light );
  }

  page_priv( ch, 0, empty_string, 0, empty_string, empty_string, pager );
  page_priv( ch, &subset[0], "can't take", 0, empty_string, empty_string, pager );
  page_priv( ch, &subset[1], "are forbidden from taking", 0, empty_string, empty_string, pager );
  page_priv( ch, &subset[2], "can't lift", 0, empty_string, empty_string, pager );
  page_priv( ch, &subset[3], "You can't take", 0, "while someone is seated on it", empty_string, pager );
  page_priv( ch, &subset[4], "You can't take", 0, "while something is on it", empty_string, pager );
  page_priv( ch, &subset[5], "can't handle", 0, empty_string, empty_string, pager );
  page_publ( ch, &subset[7], "get", container, "from", empty_string, 0, pager );

  if( fix_light && light <= 0 && ch->in_room->contents.light > 0 ) {
    swap( light, ch->in_room->contents.light );
  }

  if( container && container->contents.size == -1 )
    container->contents.size = 0; 

  array = subset[7];

  standard_delay( ch, subset[7], container );
}


void do_dig( char_data* ch, const char *argument )
{
  if( is_mob( ch )
      || is_mounted( ch, "dig" )
      || is_ridden( ch, "dig" )
      || is_entangled( ch, "dig" )
      || is_fighting( ch, "dig" )
      || is_drowning( ch, "dig" ) ) {
    return;
  }

  obj_data *tool = find_oflag( ch->wearing, OFLAG_TOOL_DIG );

  if( !tool ) {
    send( ch, "You are not holding a digging tool.\n\r" );
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
    send( ch, "You can't dig when you can't even see the ground!\n\r" );
    return;
  }

  obj_data *cache = 0;

  // See if the player already has a cache here.
  for( int i = 0; i < room->contents; ++i ) {
    if( obj_data *obj = object( room->contents[i] ) ) {
      if( obj->pIndexData->vnum == OBJ_CACHE
	  && is_set( obj->extra_flags, OFLAG_SECRET ) ) {
	if( obj->owner == ch->pcdata->pfile ) {
	  cache = obj;
	  fpage( ch, "After digging around for a while with %s, you locate your existing cache.",
		 tool );
	  fsend_seen( ch, "%s digs around for a while with %s, and uncovers a cache of items.",
		      ch, tool );
	  break;
	} else if( !obj->owner ) {
	  cache = obj;
	  fpage( ch, "After digging around for a while with %s, you locate a hidden cache!",
		 tool );
	  fsend_seen( ch, "%s digs around for a while with %s, and uncovers a cache of items!",
		      ch, tool );
	  break;	
	}
      }
    }
  }

  if( !cache ) {
    if( !is_set( terrain_table[ room->sector_type ].flags, TFLAG_BURY )
	|| is_set( room->room_flags, RFLAG_SAVE_ITEMS ) ) {
      send( ch, "You can't dig here.\n\r" );
    } else {
      fpage( ch, "You dig around for a while with %s, but don't find anything valuable.",
	     tool);
      fsend_seen( ch, "%s digs around for a while with %s, but doesn't find anything valuable.",
		  ch, tool );
      set_delay( ch, 32 );
    }
    return;
  }

  damage_weapon( ch, tool );

  cache = (obj_data*) cache->From( 1 );

  transfer_objects( 0, room->contents, ch, cache->contents );

  remove_bit( cache->extra_flags, OFLAG_NO_RESET );
  remove_bit( cache->extra_flags, OFLAG_SECRET );
  remove_bit( cache->extra_flags, OFLAG_ONE_OWNER );

  remove_bit( cache->value[1], CONT_CLOSED );

  if( cache->owner ) {
    cache->owner->caches -= cache;
    cache->owner = 0;
  }

  cache->timer = 0;

  cache->To( room );

  set_delay( ch, 32 );
}
