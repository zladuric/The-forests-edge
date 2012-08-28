#include <ctype.h>
#include <sys/types.h>
#include <stdio.h>
#include <syslog.h>
#include "define.h"
#include "struct.h"


room_data *cmd_room = 0;


static bool struggle_paralysis( char_data* ch )
{
  affect_data *affect;
  
  for( int i = 0; ; i++ ) {
    if( i >= ch->affected ) {
      bug( "Struggle_Paralysis: Paralyzed character %s with no affect.", ch );
      remove_bit( ch->affected_by, AFF_PARALYSIS );
      return false;
    }
    if( ch->affected[i]->type == AFF_PARALYSIS ) {
      affect = ch->affected[i];
      break;
    }
  }
  
  if( roll_dice( 2,8 )+number_range( 0, affect->level ) < ch->Constitution( ) ) {
    strip_affect( ch, AFF_PARALYSIS );
    return false;
  } else {
    send( ch, "You struggle, but remain paralyzed.\n\r" );
    fsend_seen( ch, "%s twitches slightly.", ch );
    set_delay( ch, 32 );
    return true;
  }
}


static bool struggle_web( char_data* ch )
{
  affect_data *affect;
  
  for( int i = 0; ; i++ ) {
    if( i >= ch->affected ) {
      bug( "Struggle_Web: Entangled character %s with no affect.", ch );
      remove_bit( ch->affected_by, AFF_ENTANGLED );
      return false;
    }
    if( ch->affected[i]->type == AFF_ENTANGLED ) {
      affect = ch->affected[i];
      break;
    }
  }
  
  if( roll_dice( 2,8 )+number_range( 0, affect->level ) < ch->Strength( ) ) {
    strip_affect( ch, AFF_ENTANGLED );
    return false;
  } else {
    send( ch, "You struggle, but fail to escape the web.\n\r" );
    fsend_seen( ch, "%s struggles, but fails to escape the web holding %s.",
		ch, ch->Him_Her( ) );
    set_delay( ch, 32 );
    return true;
  }
}


static void remove_garrote( char_data *ch, char_data *garroter )
{
  affect_data af;
  af.type = AFF_CHOKING;
  modify_affect( ch, &af, false );
  if( garroter )
    remove_bit( garroter->status, STAT_GARROTING );
}


static bool struggle_garrote( char_data* ch )
{
  char_data *garroter = 0;

  for( int i = 0; i < *ch->array; ++i ) {
    char_data *rch = character( ch->array->list[i] );
    if( rch
	&& rch->fighting == ch
	&& is_set( rch->status, STAT_GARROTING ) ) {
      garroter = rch;
      break;
    }
  }

  if( !garroter ) {
    // Should never happen.
    bug( "Struggle_garrote: no garroter for %s.", ch );
    remove_garrote( ch, 0 );
    return false;
  }

  int skill = garroter->get_skill( SKILL_GARROTE );

  if( roll_dice( 2,8 )+number_range( 0, 2*skill ) + garroter->Strength( )/2
      < ch->Strength( ) + ch->Dexterity( )/2 ) {
    remove_garrote( ch, garroter );
    fsend( ch, "You escape %s's garrote!", garroter );
    fsend( garroter, "%s escapes your garrote!", ch );
    fsend_seen( ch, "%s escapes %s's garrote!", ch, garroter );
    return false;
  } else {
    send( ch, "You struggle, but fail to escape the garrote.\n\r" );
    fsend_seen( ch, "%s struggles, but fails to escape the garrote choking %s.",
		ch, ch->Him_Her( ) );
    set_delay( ch, 32 );
    return true;
  }
}


static void passive_action( char_data* ch )
{
  if( ch->cast ) {
    spell_update( ch );
    return;
  }

  // Interpret as many zero-delay commands as are queued.
  // This avoids executing one command, struggling for a while, one more command, ...
  // Would it be better to set a delay of 1 and return?
  // This would more closely mimic how non-queued zero-delay commands work from network.cc
  // (i.e. one command per player per 1/16 second tick)
  while( command_data *cmd = ch->cmd_queue.pop( ) ) {
    assign_bit( ch->status, STAT_ORDERED, cmd->ordered );
    cmd_room = cmd->room;
    interpret( ch, cmd->string );
    cmd_room = 0;
    remove_bit( ch->status, STAT_ORDERED );
    delete cmd;
    
    if( ch->link )
      ch->link->idle = -1;  // Will be zero after increment in update_links().

    if( ch->active.time != -1 ) {
      return;
    }
  }

  if( ch->is_affected( AFF_PARALYSIS )
      && struggle_paralysis( ch ) ) {
    return;
  }
  
  if( ch->is_affected( AFF_ENTANGLED )
      && struggle_web( ch ) ) {
    return;
  }
  
  if( ch->is_affected( AFF_CHOKING )
      && struggle_garrote( ch ) ) {
    return;
  }

  if( ch->position < POS_STANDING
      && ( ch->fighting
	   || !ch->aggressive.is_empty() ) ) {
    jump_feet( ch );
    set_delay( ch, 40-ch->Dexterity( ) );
  }
}


void execute_leap( event_data* event )
{
  char_data* ch      = (char_data*) event->owner;
  char_data* victim  = (char_data*) event->pointer;

  remove_bit( ch->status, STAT_STUNNED );

  extract( event );

  if( ch->position <= POS_SLEEPING ) 
    return;

  if( ch->fighting
      || ch->active.time != -1 )
    return;

  ch->Show( 1 );

  passive_action( ch );

  if( ch->active.time != -1 )
    return; 

  if( !set_fighting( ch, victim ) )
    return;

  strip_affect( ch, AFF_INVISIBLE );
  leave_shadows( ch );
  leap_message( ch, victim );

  fight_round( ch );
}


void fight_round( char_data* ch )
{
  char_data *victim = ch->fighting;

  victim->Show( 1 );

  int time = -1;

  if( is_set( ch->status, STAT_GARROTING ) ) {
    obj_data *garrote = ch->Wearing( WEAR_HELD_R, LAYER_BASE );
    if( !garrote
	|| garrote->pIndexData->item_type != ITEM_GARROTE ) {
      bug( "Fight_Round: garroter %s removed garrote on %s.", ch, victim );
      affect_data af;
      af.type = AFF_CHOKING;
      modify_affect( victim, &af, false );
      remove_bit( ch->status, STAT_GARROTING );
    } else {
      damage_physical( victim, ch,
		       dice_data( garrote->value[1] ).roll( ) + (int)ch->Damroll( 0 ),
		       "garrote" );
    }

    time = 32;
  }

  if( time < 0 ) {
    time = ( ch->species
	     ? mob_round( ch, victim )
	     : player_round( ch, victim ) );
  }
  
  if( !ch->Is_Valid( ) )
    return;

  if( ch->species
      && !is_set( ch->status, STAT_PET ) ) {
    // Update mob experience values.
    add_round( ch->species, time );
  }
  
  if( ch->active.time == -1 )
    set_delay( ch, time );

  check_wimpy( ch );
}


void next_action( event_data* event )
{
  char_data *ch  = (char_data*) event->owner;

  remove_bit( ch->status, STAT_STUNNED );

  if( ch->position <= POS_SLEEPING )
    return;

  ch->Show( 1 );

  passive_action( ch );

  if( !ch->Is_Valid( )
      || ch->active.time != -1 
      || ( !ch->fighting && ch->aggressive.is_empty() ) )
    return;

  // Confused when fighting.
  if( is_confused( ch )
      && number_range( 0, 3 ) == 0 ) {
    confused_char( ch );
    set_delay( ch, 32 );
    return;
  }

  if( ch->fighting ) 
    fight_round( ch );
  else
    init_attack( ch, ch->aggressive[0] );
}


/*
 *   WANDER EVENT
 */


static bool scavenge( char_data *ch,  obj_data *obj, int m, int n, obj_array& stuff )
{
  if( n >= 0 && m >= 0
      && can_wear( obj, ITEM_TAKE )
      && obj->Seen( ch ) ) {
    if( n < obj->Count( 1 ) )
      return false;
    const int x = obj->Weight( 1 );
    if( m < x )
      return false;
    // Can take at least one of the object...
    if( obj->pIndexData->item_type == ITEM_MONEY
	|| obj->pIndexData->item_type == ITEM_GEM
	|| obj->pIndexData->item_type == ITEM_TREASURE
	|| can_eat( ch, obj, false ) ) {
      int l = m / x;
      l = min( l, obj->Number( ) );
      if( obj->Count( 1 ) > 0 ) {
	l = min( l, n );
      }
      obj->Select( number_range( 1, l ) );
      stuff += obj;
      return true;
    } else if( could_wear( ch, obj, false ) ) {
      obj->Select( 1 );
      stuff += obj;
      return true;
    } else if( obj->pIndexData->item_type == ITEM_DRINK_CON ) {
      obj->Select( 1 );
      stuff += obj;
      return true;
    }
  }

  return false;
}


bool behave( char_data *ch, exit_data *exit, bool urgent )
{
  if( ch->pcdata
      || is_entangled( ch )
      || ch->leader && !ch->leader->species
      || opponent( ch ) )
    return false;

  ch->Show( 1 );

  // Re-camo, re-hide, re-sneak.
  if( !urgent
      && ch->position >= POS_STANDING
      && !ch->rider
      && !ch->mount
      && number_range( 0, 1 ) == 1 ) {
    bool done = false;

    if( ch->species->is_affected( AFF_CAMOUFLAGE )
	&& !is_set( ch->status, STAT_CAMOUFLAGED ) ) {
      do_camouflage( ch, "" );
      done = is_set( ch->status, STAT_CAMOUFLAGED );

    } else if( ch->species->is_affected( AFF_HIDE )
	       && !ch->species->is_affected( AFF_CAMOUFLAGE )
	       && !is_set( ch->status, STAT_HIDING ) ) {
      do_hide( ch, "" );
      done = is_set( ch->status, STAT_HIDING );
    }

    if( ch->species->is_affected( AFF_SNEAK )
	&& !is_set( ch->status, STAT_SNEAKING ) ) {
      do_sneak( ch, "" );
      done |= is_set( ch->status, STAT_SNEAKING );
    }

    if( done )
      return true;
  }

  // Scavenge.
  if( !urgent
      && number_range( 0, 1 ) == 1 ) {
    obj_array food, stuff, trash, drinks, fountains;

    if( is_set( ch->species->act_flags, ACT_SCAVENGER ) ) {
      const bool carry = ch->can_carry( );
      int n = 0, m = 0;
      if( carry ) {
	n = ch->can_carry_n( ) - ch->contents.number;
	// This will allow no more than lightly burdened (1/4 capacity).
	m = ( ch->Empty_Capacity( ) - 4*ch->contents.weight - 2*ch->wearing.weight ) / 4;
      }
      
      for( int i = 0; i < ch->in_room->contents; ++i ) {
	if( obj_data *obj = object( ch->in_room->contents[i] ) ) {
	  if( obj->reset
	      || !obj->Seen( ch ) )
	    continue;
	  if( can_eat( ch, obj, false ) ) {
	    obj->Select( 1 );
	    food += obj;
	  } else if( carry ) {
	    bool anything = false;
	    if( valid_container( ch, obj, false ) ) {
	      for( int j = 0; j < obj->contents; ++j ) {
		if( obj_data *obj2 = object( obj->contents[j] ) ) {
		  anything |= scavenge( ch, obj2, m, n, stuff );
		}
	      }
	    }
	    if( !anything ) {
	      scavenge( ch, obj, m, n, stuff );
	    }
	  }
	}
      }
    }
    
    for( int i = 0; i < ch->in_room->contents; ++i ) {
      if( obj_data *obj = object( ch->in_room->contents[i] ) ) {
	if( stuff.includes( obj )
	    || !obj->Seen( ch ) )
	  continue;
	if( ( obj->pIndexData->item_type == ITEM_FOUNTAIN
	      || ( obj->pIndexData->item_type == ITEM_DRINK_CON
		   && !obj->reset
		   && obj->value[1] != 0 ) )
	    && would_drink( ch, obj ) ) {
	    /*
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
	    */
	  obj->Select( 1 );
	  fountains += obj;
	  continue;
	}
	if( obj->reset )
	  continue;
	if( can_eat( ch, obj, false ) ) {
	  obj->Select( 1 );
	  food += obj;
	}
      }
    }

    const bool drinky = !is_submerged( ch ) && ch->mod_position( ) != POS_FLYING;

    for( int i = 0; i < ch->contents; ++i ) {
      if( obj_data *obj = object( ch->contents[i] ) ) {
	if( obj->reset || obj->for_sale )
	  continue;
	if( obj->pIndexData->item_type == ITEM_TRASH
	    && obj == cursed( obj, ch ) ) {
	  obj->Select_All( );
	  trash += obj;
	} else if( could_wear( ch, obj, true ) ) {
	  obj->Select( 1 );
	  stuff += obj;
	} else if( can_eat( ch, obj, false ) ) {
	  obj->Select( 1 );
	  food += obj;
	} else if( obj->pIndexData->item_type == ITEM_DRINK_CON ) {
	  if( obj->value[1] != 0 ) {
	    if( would_drink( ch, obj ) ) {
	    /*
	    if( !is_set( obj->value[3], CONSUME_POISON )
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
	    */
	      if( drinky
		  && can_drink( ch, obj, false ) ) {
		// To drink.
		obj->Select( 1 );
		drinks += obj;
	      }
	    } else {
	      // To empty.
	      obj->Select_All( );
	      drinks += obj;
	    }
	  } else if( !fountains.is_empty() ) {
	    // To fill.
	    obj->Select( 1 );
	    drinks += obj;
	  }
	}
      }
    }
    
    for( int i = 0; i < ch->wearing; ++i ) {
      if( obj_data *obj = object( ch->wearing[i] ) ) {
	if( obj->reset || obj->for_sale )
	  continue;
	if( can_eat( ch, obj, false ) ) {
	  obj->Select( 1 );
	  food += obj;
	} else if( obj->pIndexData->item_type == ITEM_DRINK_CON ) {
	  if( obj->value[1] != 0 ) {
	    if( would_drink( ch, obj ) ) {
	    /*
	    if( !is_set( obj->value[3], CONSUME_POISON )
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
	    */
	      if( drinky
		  && can_drink( ch, obj, false ) ) {
		// To drink.
		obj->Select( 1 );
		drinks += obj;
	      }
	    } else {
	      // To empty.
	      obj->Select_All( );
	      drinks += obj;
	    }
	  } else if( !fountains.is_empty() ) {
	    // To fill.
	    obj->Select( 1 );
	    drinks += obj;
	  }
	}
      }
    }

    const int k = food.size + stuff.size + trash.size + drinks.size;
    if( k != 0 ) {
      int j = number_range( 0, k-1 );
      if( j < food.size ) {
	obj_data *obj = food[j];
	eat( ch, obj );
	return true;
      }
      if( ( j -= food.size ) < stuff.size ) {
	obj_data *obj = stuff[j];
	thing_array array;
	array.append( obj );
	if( obj->array != &ch->contents ) {
	  obj_data *container = object( obj->array->where );
	  if( container )
	    look_in( ch, container );
	  get_obj( ch, array, container, false );
	}
	// Get trigger could have killed ch.
	if( ch->Is_Valid( ) ) {
	  if( obj_data *obj = object( array[0] ) ) {
	    if( obj->label != empty_string ) {
	      // Remove any label and re-consolidate.
	      obj = (obj_data*) obj->From( 1 );
	      free_string( obj->label, MEM_OBJECT );
	      obj->label = empty_string;
	      obj->To( ch );
	      array[0] = obj;
	    }
	    wear( ch, array );
	  }
	}
	return true;
      }
      if( ( j -= stuff.size ) < trash.size ) {
	obj_data *obj = trash[j];
	thing_array array;
	array.append( obj );
	drop( ch, array, false );
	return true;
      }
      j -= trash.size;
      obj_data *obj = drinks[j];
      if( obj->value[1] != 0 ) {
	if( would_drink( ch, obj ) ) {
	/*
	if( !is_set( obj->value[3], CONSUME_POISON )
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
	*/
	  drink( ch, obj );
	} else {
	  thing_array array;
	  array.append( obj );
	  empty( ch, array );
	}
      } else {
	fill( ch, obj, fountains[ number_range( 0, fountains.size-1 ) ] ) ;
      }
      return true;
    }
    if( drinky
	&& !fountains.is_empty( ) ) {
      obj_data *fount = fountains[ number_range( 0, fountains.size-1 ) ];
      if( drink( ch, fount ) ) {
	return true;
      }
    }
  }

  if( ch->Is_Valid( )
      && !ch->leader
      && !ch->rider ) {
    // Rest, sleep for regen.
    // Get up when recovered.
    if( !ch->mount
	&& number_range( 0, 1 ) == 1 )
      if( ( !urgent
	    || ch->position < POS_STANDING )
	  && mob_pos( (mob_data*)ch ) )
	return true;
    
    if( !is_set( ch->status, STAT_SENTINEL )
	&& ch->position >= POS_STANDING ) {
      if( exit_data *door = exit ? exit : random_movable_exit( ch, 0, true ) ) {
	int move, type;
	if( is_set( ch->species->act_flags, ACT_OPEN_DOORS )
	    && is_set( door->exit_info, EX_CLOSED )
	    && !ch->Can_Move( 0, ch->in_room, door, door->direction, move, type, false, false ) ) {
	  unlock_door( ch, door );
	  open_door( ch, door );
	  if( !exit && is_set( door->exit_info, EX_CLOSED ) ) {
	    door = random_movable_exit( ch );
	  }
	}
	if( !exit && door ) {
	  move_char( ch, door->direction, false );
	}
      }
    }
  }

  return false;
}


void execute_wander( event_data* event )
{
  char_data *ch = (char_data*) event->owner;

  const bool done = behave( ch );

  // Wander.
  if( ch->Is_Valid( ) ) {
    if( done ) {
      // Faster wander.
      add_queue( event, number_range( ch->species->wander/10, ch->species->wander/2 ) );
    } else {
      delay_wander( event );
    }
  }
}


void delay_wander( event_data* event )
{
  char_data*      ch  = (char_data*) event->owner;
  const int         wander  = ch->species->wander;
 
  add_queue( event, number_range( wander/10, wander ) );
}


/*
 *   DROWN EVENT
 */


void execute_drown( event_data* event )
{
  char_data *ch = (char_data*) event->owner;

  if( ch->mod_position( ) == POS_DROWNING ) {
    send( ch, "You try to breathe but just swallow water.\n\r" );
    fsend_seen( ch, "%s is drowning!", ch );
    if( inflict( ch, 0, 5, "drowning" ) ) {
      return;
    }
  }

  if( event->Is_Valid( ) ) {
    add_queue( event, number_range( 50, 75 ) );
  }
}


/*
 *   FALL EVENT
 */


void execute_fall( event_data* event )
{
  char_data* ch = (char_data*) event->owner;

  if( ch->rider ) {
    add_queue( event, number_range( 25, 50 ) );
    return;
  }

  if( ch->mount ? !ch->mount->can_fly( ) : !ch->can_fly( ) ) {
    if( move_char( ch, DIR_DOWN, false, true ) )
      return;
  }

  if( event->Is_Valid( ) ) {
    add_queue( event, number_range( 25, 50 ) );
  }
}
