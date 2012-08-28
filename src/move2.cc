#include <ctype.h>
#include <sys/types.h>
#include <stdio.h>
#include "define.h"
#include "struct.h"


void extinguish_light( char_data* );


void consolidate( obj_data *obj, bool keep )
{
  if( !obj->Is_Valid( ) )
    return;

  Content_Array *array = obj->array;

  if( !array )
    return;

  if( char_data *ch = character( array->where ) ) {
    bool consol = ( array == &ch->contents );
    if( player_data *pl = player( ch ) ) {
      consol = consol || ( array == &pl->locker );
    }
    if( !consol )
      return;
  } else if( Room( array->where ) ) {
    if( obj->pIndexData->item_type == ITEM_CHAIR )
      return;
  }

  // ***TEMP***
  keep = true;

  bool after = false;
  for( int j = 0; j < *array; ++j ) {
    if( obj_data *obj2 = object( array->list[j] ) ) {
      if( obj2 == obj ) {
	after = true;
      } else if( is_same( obj2, obj, keep ) ) {
	if( !keep && !obj->save && obj2->save ) {
	  swap( obj, obj2 );
	  after = !after;
	}
	if( !after ) {
	  *array -= obj;
	  array->list[j] = obj;
	} else {
	  *array -= obj2;
	}
	clear_queue( obj2, obj );
	// Replace obj2 with more of obj.
        // Total number and weight unchanged.
	obj2->array = 0;
	obj->Set_Number( obj->Number( ) + obj2->Number( ) );
	obj2->Extract( );
	break;
      }
    }
  }
}


void remove_weight( thing_data* thing, int i )
{
  Content_Array *where = thing->array;

  if( !where )
    return;

  player_data*       pc; 

  int w1 = thing->Weight( i );
 
  where->number -= thing->Count( i );

  while( true ) {
    if( !( thing = where->where )
	|| !thing->array
	|| ( ( pc = player( thing ) )
	     && ( where == &pc->locker
		  || where == &pc->junked ) ) ) {
      where->weight -= w1;
      break;
    }
    
    // Take into account that thing->Weight() may not decrease by w1,
    // Because of holding, etc.
    const int w2   = thing->Weight( );
    where->weight -= w1;
    w1             = w2-thing->Weight( );
    where          = thing->array;
  }
}


void add_weight( thing_data* thing, int i )
{
  Content_Array *where = thing->array;
  
  if( !where )
    return;

  player_data*        pc;

  int w1 = thing->Weight( i );
 
  where->number += thing->Count( i );

  while( true ) {
    if( !( thing = where->where )
	|| !thing->array
	|| ( ( pc = player( thing ) )
	     && ( where == &pc->locker
		  || where == &pc->junked ) ) ) {
      where->weight += w1;
      break;
    }
    
    // Take into account that thing->Weight() may not increase by w1,
    // Because of holding, etc.
    const int w2   = thing->Weight( );
    where->weight += w1;
    w1             = thing->Weight( )-w2;
    where          = thing->array;
  }
}
  

/*
 *   TRANSFER FROM
 */


thing_data *thing_data :: From( int, bool in_place )
{
  remove_weight( this, Number( ) );
  
  *array -= this;

  if( !in_place )
    array = 0;

  return this;
}


thing_data *char_data :: From( int, bool )
{
  if( !array ) {
    roach( "Removing character from somewhere that isn't nowhere." );
    roach( "-- Ch = %s", this );
    return this;
  }

  if( was_in_room ) {
    return this;
  }

  room_data *room = Room( array->where );

  in_room = 0;

  if( room ) {
    if( !species )
      --room->area->nplayer;
    
    stop_fight( this );		// Also disrupts spell with this as target.
    stop_events( this, execute_drown );
    stop_events( this, execute_fall );
    
    if( event_data *event = find_event( this, execute_path ) ) {
      ((path_data*) event->pointer)->valid = false;
    }

    for( int i = 0; i < room->contents; ++i ) {
      if( obj_data *obj = object( room->contents[i] ) ) {
	if( !is_set( obj->pIndexData->wear_flags, ITEM_TAKE ) ) {
	  for( int j = 0; j < obj->pIndexData->affected; ++j ) {
	    modify_affect( this, obj->pIndexData->affected[j], false );
	  }
	}
      }
    }
  }
  
  unseat( this );
  disrupt_spell( this );

  //  remove_bit( status, STAT_BERSERK );
  //  remove_bit( status, STAT_FOCUS );

  thing_data :: From( );

  if( room && Light() ) {
    room->recalc_light();
  }

  return this;
}


thing_data *Obj_Data :: From( int i, bool in_place )
{
  Content_Array *where = array;

  if( !where ) {
    roach( "From( Obj ): object is nowhere." );
    roach( "-- Obj = %s", this );
    return this;
  }

  if( i > number ) {
    roach( "From( Obj ): number > amount." );
    roach( "-- Obj = %s", this );
    roach( "-- Number = %d", i ); 
    roach( "-- Amount = %d", number );
    roach( "-- Where = %s", Location( ) );
    i = number;
  }
  
  if( pIndexData->item_type == ITEM_CHAIR ) {
    for( int i = contents.size-1; i >= 0; --i ) {
      if( char_data *ch = character( contents[i] ) ) {
	unseat( ch );
      }
    }
  }

  char_data *ch = 0;
  room_data *room = 0;

  if( ( ch = character( where->where ) ) ) {
    if( !in_place
	&& i == number
	&& ch->cast ) {
      if( this == object( ch->cast->target ) ) {
	disrupt_spell( ch );
      } else if( where == &ch->contents
	  || where == &ch->wearing ) {
	for( int j = 0; j < MAX_SPELL_WAIT; ++j ) {
	  if( ch->cast->reagent[j] == this ) {
	    disrupt_spell( ch );
	    break;
	  }
	}
      }
    }

  } else if( ( room = Room( where->where ) ) ) {
    if( !in_place
	&& i == number ) {
      for( int j = 0; j < *where; j++ ) {
	char_data *rch;
	if( ( rch = character( where->list[j] ) )
	    && rch->cast
	    && this == rch->cast->target ) {
	  disrupt_spell( rch );
	}
      }

      if( !is_set( pIndexData->wear_flags, ITEM_TAKE ) ) {
	for( int j = 0; j < pIndexData->affected; ++j ) {
	  for( int k = 0; k < room->contents; ++k ) {
	    if( char_data *rch = character( room->contents[k] ) ) {
	      modify_affect( rch, pIndexData->affected[j], false );
	    }
	  }
	}
      }
    }
  }
  
  if( boot_stage == 2 || is_set( extra_flags, OFLAG_NOSAVE ) )
    pIndexData->count -= i;

  obj_data *obj;

  if( number > i ) {
    remove_weight( this, i );
    number -= i;
    selected = shown = max( 0, selected - i );
    obj = duplicate( this, i );
    if( in_place ) {
      obj->for_sale = for_sale;
      obj->sold = sold;
      obj->array = array;
    }
    return obj;
  }
  
  selected = shown = number;

  thing_data :: From( 0, in_place );
  
  if( !in_place ) {
    for_sale = false;
    sold = false;
  }

  stop_events( this, execute_decay );
  
  if( ch ) {
    // Unequip desn't work properly before thing_data::From().
    if( where == &ch->wearing ) {
      unequip( ch, this, in_place );
      if( !in_place ) {
	reset = 0;
	if( timer < 0 )
	  timer = -timer;
      }
    } else if( where == &ch->contents ) {
      if( !in_place ) {
	reset = 0;
      }
    }
  } else if( room ) {
    if( !in_place ) {
      unregister_reset( this );
      if( timer < 0 )
	timer = -timer;
    }
    if( Light() ) {
      room->recalc_light();
    }
  } else if( ( obj = object( where->where ) )
	     //	     && where == &obj->contents
	     && obj->contents.is_empty() ) {
    // If we just emptied a container, consolidate the container itself.
    consolidate( obj );
  }

  return this;
}


/*
 *   TRANSFER TO
 */

  
void thing_data :: To( thing_data *thing )
{
  To( thing->contents );
}


void thing_data :: To( Content_Array& where )
{
  if( !array ) {
    where += this;
    array = &where;
  }

  add_weight( this, Number( ) );
}


void char_data :: To( thing_data* thing )
{
  To( thing->contents );
}


void char_data :: To( Content_Array& where )
{
  room_data *room = Room( where.where );

  if( !room ) {
    roach( "Attempted transfer of character %s to non-room object.", this );
    if( was_in_room ) {
      To( was_in_room );
    } else {
      Extract( );
    }
    return;
  }
  
  if( was_in_room ) {
    if( was_in_room != room ) {
      was_in_room = room;
      return;
    }
    was_in_room = 0;
    if( array ) {
      From( );
    }
  }
  
  if( array ) {
    roach( "Adding character from somewhere which isn't nowhere." );
    roach( "-- Ch = %s", this );
    From( )->To( where );
    return;
  }
  
  /* CHARACTER TO ROOM */
  
  const bool to_water = water_logged( room );

  if( to_water ) {
    // Prevent "<follower> steps from the shadows." message in room because of water.
    leave_shadows( this );
    if( is_set( status, STAT_SNEAKING ) ) {
      send( this, "You stop sneaking.\n\r" );
      remove_bit( status, STAT_SNEAKING );
    }
  }

  thing_data :: To( where );
  
  room_position = -1;
  in_room = room;
  
  if( !species ) 
    ++room->area->nplayer;
  
  wizard_data *imm;
  
  if( ( imm = wizard( this ) ) ) {
    imm->custom_edit  = 0;
    imm->room_edit    = 0;
    imm->action_edit  = 0;
    imm->exit_edit    = 0;
  }
  
  if( Light() != 0 ) {
    room->recalc_light();
  }

  if( midair( 0, room ) ) {
    add_queue( new event_data( execute_fall, this ),
	       number_range( 25, 50 ) );

  } else if( to_water ) {
    add_queue( new event_data( execute_drown, this ),
	       number_range( 50, 75 ) );
    
    if( is_submerged( this ) ) {
      enter_water( this );
      if( !can_hold_light( ) ) {
	extinguish_light( this );
      }
    }
  }

  for( int i = 0; i < room->contents; ++i ) {
    if( obj_data *obj = object( room->contents[i] ) ) {
      if( !is_set( obj->pIndexData->wear_flags, ITEM_TAKE ) ) {
	for( int j = 0; j < obj->pIndexData->affected; ++j ) {
	  modify_affect( this, obj->pIndexData->affected[j], true );
	}
      }
    }
  }

  if( is_set( status, STAT_FOLLOWER ) )
    return;

  // Walk-in aggression
  renter_combat( this );

  // Response to walkin.  
  update_aggression( this );

  /*
  if( species ) {
    for( mprog_data *mprog = species->mprog; mprog; mprog = mprog->next ) {
      if( mprog->trigger == MPROG_TRIGGER_TO_ROOM ) {
      clear_variables( );
	var_mob = this;
	var_room = room;
	mprog->execute( );
	break;
      }
    }
  }
  */
}


static bool in_place = false;


void Obj_Data :: To( thing_data *thing )
{
  if( !thing ) {
    if( !array ) {
      roach( "Adding in-place object to noplace." );
      roach( "-- Obj = %s", this );
      Extract( );
    } else {
      Content_Array& where = *array;
      array = 0;
      in_place = true;
      To( where );
      in_place = false;
    }
    return;
  }

  To( thing->contents );
}


void Obj_Data :: To( Content_Array& where )
{
  if( array ) {
    roach( "Adding object from somewhere which isn't nowhere." );
    roach( "-- Obj = %s", this );
    roach( "-- Where = %s", Location( ) );
    thing_data *xyz = array->where;
    if( room_data *room = Room( xyz ) ) {
      roach( "-- Room = %d (%s)", room->vnum, room->name );
    } else if( char_data *rch = character( xyz ) ) {
      roach( "-- Char = %s (%s)", rch, rch->descr->name );
    }
    From( number )->To( where );
    return;
  }

  if( !Is_Valid( ) )
    return;

  room_data*   room = 0;
  char_data*     ch = 0;
  obj_data*     obj = 0;
  int             i;

  if( boot_stage == 2 )
    pIndexData->count += number;

  if( ( ch = character( where.where ) ) ) {
    if( where == ch->wearing ) {
      if( timer > 0 ) {
	event_data *event = new event_data( execute_decay, this );
	add_queue( event, timer );
      }
      equip( ch, this, in_place );
      if( !Is_Valid( ) || array )
	return;
      // Determine array position *after* equip() in case wear trigger
      // changes the wearing array.
      for( i = 0; i < ch->wearing; ++i ) {
	obj = (obj_data*) ch->wearing[i];
	if( obj->position > position 
	    || ( obj->position == position
		 && obj->layer > layer ) ) 
	  break;
      }
      where.insert( this, i );
      array = &where;
      thing_data :: To( where );
      return;

    } else if( where == ch->contents ) {
      if( !in_place )
	if( player( ch ) )
	  set_owner( this, ch, 0 );
	else if( is_set( ch->status, STAT_PET )
		 && player( ch->leader ) )
	  set_owner( this, ch->leader, 0 );
    }
    
  } else if( ( room = Room( where.where ) ) ) {

    if( !in_place ) {
      for( oprog_data *oprog = pIndexData->oprog; oprog; oprog = oprog->next ) {
	if( oprog->trigger == OPROG_TRIGGER_TO_ROOM ) {
	  clear_variables( );
	  var_room = room;
	  var_obj = this;
	  oprog->execute( );
	  if( !Is_Valid() ) {
	    return;
	  }
	}
      }
      
      if( !is_submerged( 0, room )
	  //room->sector_type != SECT_UNDERWATER
	  && is_set( extra_flags, OFLAG_AIR_RISE )
	  && exit_direction( room, DIR_UP ) ) {
	event_data *event = new event_data( execute_obj_rise, this );
	add_queue( event, number_range( 25, 50 ) );
	
      } else if( midair( 0, room ) ) {
	//room->sector_type == SECT_AIR ) {
	if( is_set( extra_flags, OFLAG_AIR_FALL ) ) {
	  if( exit_direction( room, DIR_DOWN ) ) {
	    event_data *event = new event_data( execute_obj_fall, this );
	    add_queue( event, number_range( 10, 25 ) );
	  }
	}
	
      } else if( is_submerged( 0, room )
		 //room->sector_type == SECT_UNDERWATER
		 && is_set( extra_flags, OFLAG_WATER_FLOAT ) ) {
	if( exit_direction( room, DIR_UP ) ) {
	  event_data *event = new event_data( execute_obj_float, this );
	  add_queue( event, number_range( 25, 50 ) );
	}
	
      } else if( water_logged( room )
		 && is_set( extra_flags, OFLAG_WATER_SINK ) ) {
	if( exit_direction( room, DIR_DOWN ) ) {
	  event_data *event = new event_data( execute_obj_sink, this );
	  add_queue( event, number_range( 10, 25 ) );
	}
      }

      if( !is_set( pIndexData->wear_flags, ITEM_TAKE ) ) {
	for( int j = 0; j < pIndexData->affected; ++j ) {
	  for( int k = 0; k < room->contents; ++k ) {
	    if( char_data *rch = character( room->contents[k] ) ) {
	      modify_affect( rch, pIndexData->affected[j], true );
	    }
	  }
	}
      }
    }
    
    if( timer > 0 ) {
      event_data *event = new event_data( execute_decay, this );
      add_queue( event, timer );
    }
    
    if ( pIndexData->item_type == ITEM_CHAIR ) {
      // Never consolidate chairs in a room.
      // Their content array is manipulated uniquely.
      thing_data::To( where );
      if( Light() ) {
	room->recalc_light();
      }
      return;
    }
    
  } else if ( ( obj = object( where.where ) ) ) {
    if( /*where == &obj->contents
	  &&*/ obj->number > 1 ) {
      // Are we putting something into a consolidated (empty) container?
      // Can't just create the container we're putting object into;
      // calling code might freak out if we did.
      // So move all the other containers instead.
      obj_data *containers = duplicate( obj, obj->number - 1 );
      *obj->array += containers;
      containers->array = obj->array;
      obj->number = 1;
      /*
      thing_data *containers = obj->From(obj->number-1);
      // No consolidation needed.
      // Put the thing in the container first, or else
      // the containers would get re-consolidated.
      */
      thing_data :: To( where );
      return;
    }
  }

  // Auto-consolidate.
  for( i = 0; i < where; ++i ) {
    if( ( obj = object( where[i] ) )
	&& is_same( obj, this ) ) {
      clear_queue( obj, this );
      obj->array = 0;
      where[i] = this;
      array = &where;
      thing_data::To( where );
      Set_Number( number + obj->number );
      //      selected += obj->selected;
      //      shown += obj->shown;
      obj->Extract();
      return;
    }
  }

  thing_data :: To( where );

  if( room && Light() ) {
    room->recalc_light();
  }
}


void Obj_Data :: transfer_object( char_data* to_ch, char_data* from_ch, 
				  Content_Array& where, int number )
{
  if( obj_data *obj = object( From( number ) ) ) {
    set_owner( obj, to_ch, from_ch );
    obj->To( where );
  }
}


void transfer_objects( char_data *to_ch, Content_Array& to_where,
		       char_data *from_ch, thing_array& from_array )
{
  // Copy the array, in case From( ) changes from_array.
  thing_array stuff = from_array;

  for( int i = 0; i < stuff; ++i ) {
    if( obj_data *obj = object( stuff[i] ) ) {
      obj->transfer_object( to_ch, from_ch, to_where, obj->Selected( ) );
    }
  }
}


/*
 *  DECAY
 */


const default_data timer_msg [] =
{
  { "to_char", "$r extinguish$z2.", ITEM_LIGHT },
  { "to_room", "$r extinguish$z2.", ITEM_LIGHT },
  { "to_char", "$r extinguish$z2.", ITEM_LIGHT_PERM },
  { "to_room", "$r extinguish$z2.", ITEM_LIGHT_PERM },
  { "fade_char", "$r flicker$z1 briefly.", ITEM_LIGHT },
  { "fade_room", "$r flicker$z1 briefly.", ITEM_LIGHT },
  { "fade_char", "$r flicker$z1 briefly.", ITEM_LIGHT_PERM },
  { "fade_room", "$r flicker$z1 briefly.", ITEM_LIGHT_PERM },
  { "", "", -1 }
};


void execute_decay( event_data* event )
{
  obj_data* obj = (obj_data*) event->owner;
  Content_Array *where = obj->array;

  obj->Select_All( );
  
  char_data *ch = where ? character( where->where ) : 0;

  if( ch )
    ch->Show( 1 );

  room_data *room = where ? Room( where->where ) : 0;

  oprog_data *oprog = 0;

  for( oprog = obj->pIndexData->oprog; oprog; oprog = oprog->next ) {
    if( oprog->trigger == OPROG_TRIGGER_TIMER ) {
      break;
    }
  }

  switch( obj->pIndexData->item_type ) {
  case ITEM_LIGHT:
  case ITEM_LIGHT_PERM:
    {
      if( ch
	  && obj->position == WEAR_FLOATING
	  && ch->is_affected( AFF_CONTINUAL_LIGHT ) ) {
	add_queue( event, number_range( (obj->timer+1)/2, 3*obj->timer/2 ) );
	return;
      }
      if( obj->value[0] > 0
	  && --obj->value[0] == 1 ) {
	// Fade messages.
	if( ch ) {
	  if( ch->Can_See() ) {
	    act( ch, prog_msg( oprog, timer_msg[4] ), ch, 0, obj );
	  }
	  act_seen( prog_msg( oprog, timer_msg[5] ), ch, 0, obj );
	} else if( room ) {
	  act_room( room, prog_msg( oprog, timer_msg[4] ), ch, 0, obj );
	}
      }
      if( obj->value[0] != 0 ) {
	if( obj->pIndexData->vnum == OBJ_BALL_OF_LIGHT ) {
	  if( obj->value[0] % 6 == 5
	      && obj->light > 0 ) {
	    // Ball just changed light value.
	    // Obj_Data::To() restarts the event.
	    obj = (obj_data *) obj->From( obj->Number( ), true );
	    obj->light = obj->value[0]/6 + 1;
	    obj->To( );
	    return;
	  }
	} else if( obj->pIndexData->item_type == ITEM_LIGHT
		   && obj->value[0] > 0 ) {
	  // Lights' condition degrades.
	  obj->condition = min( obj->condition, obj->Durability() * obj->value[0] / obj->value[2] );
	}
	add_queue( event, number_range( (obj->timer+1)/2, 3*obj->timer/2 ) );
	return;
      }
      if( ch ) {
	if( ch->Can_See() ) {
	  act( ch, prog_msg( oprog, timer_msg[0] ), ch, 0, obj );
	}
	act_seen( prog_msg( oprog, timer_msg[1] ), ch, 0, obj );
      } else if( room ) {
	act_room( room, prog_msg( oprog, timer_msg[1] ), ch, 0, obj );
      }
    }
    break;
  case ITEM_GATE:
    // Gate timings must be exact, since they may have 2 sides.
    if( obj->value[0] > 0 && --obj->value[0] > 0 ) {
      add_queue( event, obj->timer );
      return;
    }
    break;
  case ITEM_CORPSE:
    //  case ITEM_FIRE:
    if( obj->value[0] > 0 && --obj->value[0] > 0 ) {
      add_queue( event, number_range( (obj->timer+1)/2, 3*obj->timer/2 ) );
      return;
    }
    break;
  }

  if( oprog ) {
    // Set_obj_timer( obj, 0 ) in softcode could delete the event,
    // causing extract( event ) to crash the MUD. So, unlink it first.
    //      obj->events -= event;
    obj->Select_All( );
    clear_variables( );
    var_room = room;
    var_ch = ch;
    var_obj = obj;
    var_def = timer_msg;
    var_def_type = obj->pIndexData->item_type;
    const int result = oprog->execute( );
    if( !event->Is_Valid( )
	|| !obj->Is_Valid( ) ) {
      return;
    }
    if( !result ) {
      extract( event );
      return;
    }
    //      obj->events += event;
  }

  switch( obj->pIndexData->item_type ) {
  case ITEM_FIRE:
    if( obj->value[0] > 0 && --obj->value[0] > 0 ) {
      add_queue( event, number_range( (obj->timer+1)/2, 3*obj->timer/2 ) );
      return;
    }
    break;
  }

  if( room ) {
    for( int i = 0; i < *where; i++ ) {
      const char *const plural = ( obj->Number( ) == 1 ) ? "s" : "";
      char_data *rch;
      if( ( rch = character( where->list[i] ) )
	  && rch->link
	  && obj->Seen( rch ) ) {
	switch( obj->pIndexData->item_type ) {
	case ITEM_CORPSE:
	  fsend( rch, "%s rot%s away.", obj, plural );
	  if( !obj->contents.is_empty() ) {
	    select( obj->contents, rch );
	    rehash( rch, obj->contents );
	    
	    if( !none_shown( obj->contents ) ) {
	      const char *drop = rch->in_room->drop( );
	      if( thing_data *thing = one_shown( obj->contents ) ) {
		if( *drop ) {
		  fsend( rch, "%s fall%s %s.",
			 thing,
			 thing->Shown( ) == 1 ? "s" : "",
			 drop );
		} else {
		  fsend( rch, "%s fall%s out.",
			 thing,
			 thing->Shown( ) == 1 ? "s" : "" );
		}
	      } else {
		if( *drop ) {
		  fsend( rch, "Some items fall %s.", drop );
		} else {
		  fsend( rch, "Some items fall out." );
		}
	      }
	    }
	  }
	  break;
	case ITEM_GATE:
	  fsend( rch, "%s disappear%s.", obj, plural );
	  break;
	case ITEM_FIRE:
	  fsend( rch, "%s burn%s out.", obj, plural );
	  break;
	case ITEM_FOOD:
	  fsend( rch, "%s decompose%s.", obj, plural );
	  break;
	case ITEM_FOUNTAIN:
	  fsend( rch, "%s dr%s up.", obj,
		 ( obj->Number( ) == 1 ) ? "ies" : "y" );
	  break;
	}
      }
    }
    switch( obj->pIndexData->item_type ) {
    case ITEM_CORPSE:
      if( obj->owner
	  && ( obj->pIndexData->vnum == OBJ_CORPSE_PC
	       || obj->pIndexData->vnum == OBJ_CORPSE_PET ) ) {
	set_owner( obj->contents, 0, obj->owner );
      }
      obj->contents.To( *where );
      break;
    }
  }
    
  obj->Extract( );
}


void Content_Array :: To( Content_Array& where )
{
  while( size > 0 ) {
    thing_data *thing = list[0]->From( list[0]->Number( ) );
    thing->To( where );
  }
}


void execute_obj_fall( event_data* event )
{
  obj_data *obj = (obj_data*) event->owner;
  Content_Array *where = obj->array;
  room_data *room = where ? Room( where->where ) : 0;

  extract( event );

  if( !midair( 0, room ) )
    //  room->sector_type != SECT_AIR )
    return;

  obj->Show( obj->Number( ) );

  exit_data *exit = exit_direction( room, DIR_DOWN );

  if( !exit )
    return;

  fsend_all( room, "%s fall%s downward.",
	     obj,
	     ( obj->Shown( ) == 1 ) ? "s" : "" );

  obj = (obj_data*) obj->From( obj->Number( ) );
  obj->To( exit->to_room );
  
  fsend_all( exit->to_room, "%s fall%s in from above.",
	     obj,
	     ( obj->Shown( ) == 1 ) ? "s" : "" );
}


void execute_obj_rise( event_data* event )
{
  obj_data *obj = (obj_data*) event->owner;
  Content_Array *where = obj->array;
  room_data *room = where ? Room( where->where ) : 0;

  extract( event );

  if( !room || is_submerged( 0, room ) )
      //!room  || room->sector_type == SECT_UNDERWATER )
    return;

  obj->Show( obj->Number( ) );

  exit_data *exit = exit_direction( room, DIR_UP );

  if( !exit )
    return;

  fsend_all( room, "%s rise%s upward.",
	     obj,
	     ( obj->Shown( ) == 1 ) ? "s" : "" );

  obj = (obj_data*) obj->From( obj->Number( ) );
  obj->To( exit->to_room );
  
  fsend_all( exit->to_room, "%s rise%s in from below.",
	     obj,
	     ( obj->Shown( ) == 1 ) ? "s" : "" );
}


void execute_obj_float( event_data* event )
{
  obj_data *obj = (obj_data*) event->owner;
  Content_Array *where = obj->array;
  room_data *room = where ? Room( where->where ) : 0;

  extract( event );

  if( !is_submerged( 0, room ) )
      //!room || room->sector_type != SECT_UNDERWATER )
    return;

  obj->Show( obj->Number( ) );

  exit_data *exit = exit_direction( room, DIR_UP );

  if( !exit )
    return;

  fsend_all( room, "%s float%s upward.",
	     obj,
	     ( obj->Shown( ) == 1 ) ? "s" : "" );

  obj = (obj_data*) obj->From( obj->Number( ) );
  obj->To( exit->to_room );
  
  fsend_all( exit->to_room, "%s float%s in from below.",
	     obj,
	     ( obj->Shown( ) == 1 ) ? "s" : "" );
}


void execute_obj_sink( event_data* event )
{
  obj_data *obj = (obj_data*) event->owner;
  Content_Array *where = obj->array;
  room_data *room = where ? Room( where->where ) : 0;

  extract( event );

  if( !water_logged( room ) )
    return;

  obj->Show( obj->Number( ) );

  exit_data *exit = exit_direction( room, DIR_DOWN );

  if( !exit )
    return;

  fsend_all( room, "%s sink%s downward.",
	     obj,
	     ( obj->Shown( ) == 1 ) ? "s" : "" );

  obj = (obj_data*) obj->From( obj->Number( ) );
  obj->To( exit->to_room );
  
  fsend_all( exit->to_room, "%s sink%s in from above.",
	     obj,
	     ( obj->Shown( ) == 1 ) ? "s" : "" );
}
