#include <sys/types.h>
#include <stdio.h>
#include "define.h"
#include "struct.h"


bool char_data :: Is_Awake ( bool msg ) const
{
  if( position <= POS_SLEEPING ) {
    if( msg ) {
      send( this, "You cannot do that while sleeping.\n\r" );
    }
    return false;
  }
  
  return true;
}


/*
 *   LOCAL FUNCTIONS
 */


static bool  can_wake     ( char_data*, bool msg = true );


/*
 *   CONSTANTS
 */


const char *pos_name [MAX_MOD_POSITION] =
{ "dead", "mortally wounded",
  "incapacitated", "stunned", "sleeping", "meditating", "resting",
  "fighting", "standing",
  "flying", "falling", "hovering",
  "wading", "swimming", "drowning"
};


static obj_data *find_chair( char_data *ch )
{
  room_data *room = ch->in_room;

  if( !room )
    return 0;

  for( int i = 0; i < room->contents; ++i ) {
    obj_data *obj = object( room->contents[i] );
    if( obj
	&& obj->pIndexData->item_type == ITEM_CHAIR
	&& obj->contents.size < obj->pIndexData->value[0] ) {
      return obj;
    }
  }

  return 0;
}


static int drown_pos( const char_data *ch )
{
  if( ch->can_breathe_underwater( ) ) {
    return ( ch->position < POS_STANDING ) ? ch->position : POS_SWIMMING;
  }

  return POS_DROWNING;
}


int char_data :: mod_position( ) const
{
  if( !in_room ) {
    return position;
  }

  const int tflags = terrain_table[ in_room->sector_type ].flags;

  if( is_set( tflags, TFLAG_FALL ) ) {
    return can_fly( ) ? POS_FLYING : POS_FALLING;
  }

  if( is_set( tflags, TFLAG_WATER ) ) {
    if( is_set( tflags, TFLAG_SUBMERGED ) ) {
      // Underwater.
      return drown_pos( this );
    }

    if( is_set( tflags, TFLAG_DEEP ) ) {
      // River or surface.
      if( species
	  && is_set( species->act_flags, ACT_CAN_FLY )
	  && is_affected( AFF_FLY ) ) {
	return ( position < POS_STANDING ) ? position : POS_FLYING;
      }
      if( can_float()
	  || !species && is_affected( AFF_WATER_WALKING ) ) {
	return ( position < POS_STANDING ) ? position : POS_HOVERING;
      }
      if( !can_swim( ) ) {
	if( can_fly( ) ) {
	  return POS_FLYING;
	}
	return drown_pos( this );
      }
      return POS_SWIMMING;
    }
      
    // Shallows.
    if( species
	&& is_set( species->act_flags, ACT_CAN_FLY )
	&& is_affected( AFF_FLY ) ) {
      return ( position < POS_STANDING ) ? position : POS_FLYING;
    }
    if( can_float()
	|| !species && is_affected( AFF_WATER_WALKING ) ) {
      return ( position < POS_STANDING ) ? position : POS_HOVERING;
    }
    if( Size() < SIZE_DOG ) {
      if( !can_swim( ) ) {
	if( can_fly( ) ) {
	  return POS_FLYING;
	}
	return drown_pos( this );
      }
      return POS_SWIMMING;
    }
    if( is_humanoid() ) {
      return ( position < POS_STANDING ) ? position : POS_WADING;
    }
  }

  if( position < POS_STANDING ) {
    return position;
  }

  if( species
      && is_set( species->act_flags, ACT_CAN_FLY )
      && is_affected( AFF_FLY ) ) {
    return POS_FLYING;
  }

  if( can_float() ) {
    return POS_HOVERING;
  }

  return POS_STANDING;
}


const char *char_data :: position_name( char_data *ch ) const
{
  const int pos = mod_position( );

  const int movement = species
    ? species->movement
    : ((player_data*)this)->movement;

  if( pos == POS_STANDING
      && mount )
    return "mounted";

  if( pos != POS_STANDING
      || movement < 0
      || !*movement_table[ movement ].position
      || ch && ch->is_affected( AFF_HALLUCINATE ) )
    return pos_name[ pos ];

  return movement_table[ movement ].position;
}


bool mob_pos( mob_data *npc )
{
  switch( npc->position ) {
  case POS_STANDING:
    if( is_set( npc->species->act_flags, ACT_REST_REGEN )
	&& npc->Hit_Regen( ) > 0
	&& !npc->fighting
	&& npc->aggressive.is_empty( )
	&& ( npc->hit <= npc->max_hit / 2
	     || npc->move < ( npc->base_move + npc->mod_move ) / 8 )
	&& ( !npc->reset
	     || npc->reset->value == RSPOS_STANDING
	     || is_set( npc->status, STAT_STOOD ) ) ) {
      const int skill = npc->get_skill( SKILL_MEDITATE );
      if( skill != UNLEARNT ) {
	  //	  && number_range( 1, 10 ) <= skill ) {
	do_meditate( npc, "" );
	remove_bit( npc->status, STAT_HOLD_POS );
	if( !npc->Is_Valid( ) || npc->position != POS_STANDING )
	  return true;
      }
      // There are some places (shallows) where you can sit, but not meditate.
      obj_data *chair;
      if( is_set( npc->species->act_flags, ACT_HUMANOID )
	  && ( chair = find_chair( npc ) ) ) {
	sit( npc, chair, true );
	if( npc->Is_Valid( ) && npc->hit <= npc->max_hit / 4 ) {
	  do_sleep( npc, "" );
	  remove_bit( npc->status, STAT_HOLD_POS );
	}
      } else {
	if( npc->hit <= npc->max_hit / 4 ) {
	  do_sleep( npc, "" );
	  remove_bit( npc->status, STAT_HOLD_POS );
	  if( !npc->Is_Valid( ) || npc->position != POS_STANDING )
	    return true;
	}
	// There are some places (shallows) where you can sit, but not sleep.
	do_rest( npc, "" );
	remove_bit( npc->status, STAT_HOLD_POS );
      }
      return ( !npc->Is_Valid( ) || npc->position != POS_STANDING );
    }
    break;
    
  case POS_RESTING:
    {
      if( is_set( npc->status, STAT_HOLD_POS ) )
	return false;
      if( ( !is_set( npc->species->act_flags, ACT_REST_REGEN )
	    || npc->Hit_Regen( ) <= 0 )
	  && ( !npc->reset
	       || npc->reset->value == RSPOS_STANDING
	       || is_set( npc->status, STAT_STOOD ) ) ) {
	// Mob doesn't rest for regen; just stand up.
	do_stand( npc, "" );
	return ( !npc->Is_Valid( ) || npc->position != POS_RESTING );
      }
      const int skill = npc->get_skill( SKILL_MEDITATE );
      if( skill
	  && !npc->pos_obj
	  && npc->reset
	  && npc->reset->value == RSPOS_MEDITATING
	  && !is_set( npc->status, STAT_STOOD ) ) {
	// Return to meditate reset pos.
	do_meditate( npc, "" );
	remove_bit( npc->status, STAT_HOLD_POS );
	if( !npc->Is_Valid( ) || npc->position != POS_RESTING )
	  return true;
      }
      if( is_set( npc->species->act_flags, ACT_REST_REGEN )
	  && npc->Hit_Regen( ) > 0
	  && !npc->fighting
	  && npc->aggressive.is_empty( )
	  && ( npc->hit <= npc->max_hit / 2
	       || npc->move < ( npc->base_move + npc->mod_move ) / 2 ) ) {
	// Wants to at least keep resting.
	if( skill
	    && ( !npc->reset
		 || npc->reset->value == RSPOS_STANDING
		 || npc->reset->value == RSPOS_MEDITATING
		 || is_set( npc->status, STAT_STOOD ) ) ) {
	  // Meditate if possible, instead.
	  do_meditate( npc, "" );
	  remove_bit( npc->status, STAT_HOLD_POS );
	  if( !npc->Is_Valid( ) || npc->position != POS_RESTING )
	    return true;
	}
	if( npc->hit <= npc->max_hit / 4
	    && ( !npc->reset
		 || npc->reset->value == RSPOS_STANDING
		 || npc->reset->value == RSPOS_RESTING
		 || is_set( npc->status, STAT_STOOD ) ) ) {
	  // Sleep if the hp situation is really bad and can't meditate.
	  do_sleep( npc, "" );
	  remove_bit( npc->status, STAT_HOLD_POS );
	}
      } else if( npc->hit > npc->max_hit / 2
		 && npc->move >= ( npc->base_move + npc->mod_move ) / 2
		 && ( !npc->reset
		      || npc->reset->value != RSPOS_RESTING
		      || is_set( npc->status, STAT_STOOD ) ) ) {
	// Regenned enough.
	do_stand( npc, "" );
      }
      /*
      if( is_set( npc->species->act_flags, ACT_REST_REGEN )
	  && npc->Hit_Regen( ) > 0
	  && !npc->fighting
	  && npc->aggressive.is_empty( )
	  && npc->hit <= npc->max_hit / 4
	  && ( !npc->reset
	       || npc->reset->value == RSPOS_STANDING
	       || npc->reset->value == RSPOS_RESTING
	       || is_set( npc->status, STAT_STOOD ) ) ) {
	do_sleep( npc, "" );
	remove_bit( npc->status, STAT_HOLD_POS );
      } else if( npc->hit > npc->max_hit / 2
		 && npc->move >= ( npc->base_move + npc->mod_move ) / 2
		 && ( !npc->reset
		      || npc->reset->value != RSPOS_RESTING
		      || is_set( npc->status, STAT_STOOD ) ) ) {
	do_stand( npc, "" );
      }
      */
      return ( !npc->Is_Valid( ) || npc->position != POS_RESTING );
    }

  case POS_MEDITATING:
    if( is_set( npc->status, STAT_HOLD_POS ) )
      return false;
    if( ( !is_set( npc->species->act_flags, ACT_REST_REGEN )
	  || npc->Hit_Regen( ) <= 0 )
	&& ( !npc->reset
	     || npc->reset->value == RSPOS_STANDING
	     || is_set( npc->status, STAT_STOOD ) ) ) {
      // Mob doesn't rest for regen; just stand up.
      do_stand( npc, "" );
      return ( !npc->Is_Valid( ) || npc->position != POS_RESTING );
    }
    if( npc->hit > npc->max_hit / 2
	&& npc->move >= ( npc->base_move + npc->mod_move ) / 2
	&& ( !npc->reset
	     || npc->reset->value != RSPOS_MEDITATING
	     || is_set( npc->status, STAT_STOOD ) ) ) {
      do_stand( npc, "" );
      return ( !npc->Is_Valid( ) || npc->position != POS_MEDITATING );
    }
    break;

  case POS_SLEEPING:
    if( is_set( npc->status, STAT_HOLD_POS ) )
      return false;
    if( ( !is_set( npc->species->act_flags, ACT_REST_REGEN )
	  || npc->Hit_Regen( ) <= 0 )
	&& ( !npc->reset
	     || npc->reset->value == RSPOS_STANDING
	     || is_set( npc->status, STAT_STOOD ) ) ) {
      // Mob doesn't rest for regen; just stand up.
      do_stand( npc, "" );
      return ( !npc->Is_Valid( ) || npc->position != POS_RESTING );
    }
    if( npc->hit > npc->max_hit / 4
	&& ( !npc->reset
	     || npc->reset->value != RSPOS_SLEEPING
	     || is_set( npc->status, STAT_STOOD ) ) ) {
      if( npc->hit > npc->max_hit / 2
	  && npc->move >= ( npc->base_move + npc->mod_move ) / 2
	  && ( !npc->reset
	       || npc->reset->value != RSPOS_RESTING
	       || is_set( npc->status, STAT_STOOD ) ) ) {
	do_stand( npc, "" );
      } else {
	do_rest( npc, "" );
	remove_bit( npc->status, STAT_HOLD_POS );
      }
      return ( !npc->Is_Valid( ) || npc->position != POS_SLEEPING );
    }
    break;
  }

  return false;
}


void update_pos( char_data* ch )
{
  if( ch->hit > 0 ) {
    if( ch->position < POS_SLEEPING ) {
      if( deep_water( ch ) ) {
	ch->position = POS_STANDING;
      } else {
	ch->position = POS_RESTING;
      }
      remove_bit( ch->status, STAT_HOLD_POS );
      renter_combat( ch );
    }
    return;
  }

  if( ch->hit <= -11
      || ch->species && ch->species->dies_at_zero( ) && ch->hit <= 0 )
    ch->position = POS_DEAD;
  else if( ch->hit <= -6 )
    ch->position = POS_MORTAL;
  else if( ch->hit <= -3 )
    ch->position = POS_INCAP;
  else
    ch->position = POS_STUNNED;

  remove_bit( ch->status, STAT_HOLD_POS );
  set_fighting( ch, 0 );
  disrupt_spell( ch );
}


void pos_message( const char_data *ch )
{
  switch( ch->position ) {
  case POS_DEAD:
    send( ch, "You have died, and are unable to do anything.\n\r" );
    return;

  case POS_MORTAL:
  case POS_INCAP:
    send( ch, "The bright white light has you distracted.\n\r" );
    return;
    
  case POS_STUNNED:
    send( ch, "You are stunned and cannot move.\n\r" );
    return;
   
  case POS_SLEEPING:
    send( ch, "You cannot do that while sleeping.\n\r" );
    return;
    
  case POS_MEDITATING:
    send( ch, "You are deep in meditation.\n\r" );
    return;
    
  case POS_RESTING:
    send( ch, "Perhaps you should stand first.\n\r" );
    return;
  }
}


/*
 *   CHAIR ROUTINES
 */


static bool sit_trigger( char_data *ch, obj_data *obj )
{
  for( oprog_data *oprog = obj->pIndexData->oprog; oprog; oprog = oprog->next )
    if( oprog->trigger == OPROG_TRIGGER_SIT ) {
      obj->Select( 1 );
      push( );
      clear_variables( );
      var_ch = ch;
      var_obj = obj; 
      var_room = ch->in_room;
      oprog->execute( );
      pop( );
      return true;
    }
  
  return false;
}


bool sit( char_data* ch, obj_data* obj, bool msg )
{
  // Note: ch->position may already be sitting or sleeping from reset.cc.
  // So, don't check that here.

  if( obj->pIndexData->item_type != ITEM_CHAIR ) { 
    if( msg )
      fsend( ch, "%s is not made to be sat on.", obj );
    return false;
  }
  
  if( is_fighting( ch, msg ? "sit down" : 0 )
      || is_mounted( ch, msg ? "sit down" : 0 ) ) {
    return false;
  }

  if( obj->contents.size >= obj->pIndexData->value[0] ) {
    if( msg )
      fsend( ch, "There are too many others on %s.", obj->Name( ch ) );
    return false;
  }

  if( ch->pos_obj ) {
    ch->pos_obj->contents -= ch;
  }

  obj->contents += ch;
  ch->pos_obj = obj;
  obj->Show( 1 );
  
  if( ch->rider ) {
    if( msg ) {
      fsend( ch, "You sit down on %s, forcing %s to dismount.", obj, ch->rider );
      fsend( ch->rider, "%s sits down on %s, forcing you to dismount.", ch, obj );
      fsend_seen( ch, "%s sits down on %s, forcing %s to dismount.", ch, obj, ch->rider );
    }
    dismount( ch->rider );
  } else if( msg ) {
    fsend( ch, "You rest your weary bones and sit on %s.",
	   obj );
    fsend_seen( ch, "%s sits down on %s.",
		ch, obj );
  }

  disrupt_spell( ch );
  ch->position = POS_RESTING;

  sit_trigger( ch, obj );

  return true;
}


void unseat( char_data* ch )
{
  if( !ch->pos_obj
      || ch->pos_obj->pIndexData->item_type != ITEM_CHAIR )
    return;

  ch->pos_obj->contents -= ch;
  ch->pos_obj = 0;
}


/*
 *   MEDITATE
 */ 


void do_meditate( char_data* ch, const char* )
{
  if( ch->get_skill( SKILL_MEDITATE ) == UNLEARNT ) {
    send( ch, "You do not have the knowledge required to bring yourself into a state of\n\rmeditation.\n\r" );
    return;
  }
  
  if( is_mounted( ch, "meditate" )
      //      || is_ridden( ch, "meditate" )
      || is_fighting( ch, "meditate" ) ) {
    return;
  }
  
  if( water_logged( ch->in_room ) ) {
    send( ch, "You can't meditate in the water.\n\r" );
    return;
  }
  
  if( midair( ch ) ) {
    send( ch, "You can't meditate while off the ground.\n\r" );
    return;
  }

  if( is_set( ch->in_room->room_flags, RFLAG_NO_REST ) ) {
    send( ch, "You can't meditate here.\n\r" );
    return;
  }

  if( ch->position == POS_MEDITATING ) {
    send( ch, "You are already meditating.\n\r" );
    return;
  }

  if( ch->pos_obj ) {
    ch->pos_obj->Show( 1 );
    fsend( ch, "%s is too uncomfortable for meditating.",
	   ch->pos_obj->Name( ch ) );
    return;
  }

  if( ch->rider ) {
    fsend( ch, "You kneel down and start to meditate, forcing %s to dismount.", ch->rider );
    fsend( ch->rider, "%s kneels down and starts to meditate, forcing you to dismount.", ch );
    fsend_seen( ch, "%s kneels down and starts to meditate, forcing %s to dismount.", ch, ch->rider );
    dismount( ch->rider );
  } else {
    send( ch, "You kneel down and start to meditate.\n\r" );
    fsend_seen( ch, "%s kneels down and starts to meditate.", ch );
  }

  disrupt_spell( ch );
  ch->position = POS_MEDITATING;
  //  remove_bit( ch->status, STAT_BERSERK );
  //  remove_bit( ch->status, STAT_FOCUS );
  set_bit( ch->status, STAT_HOLD_POS );
}


/*
 *   SIT
 */


void do_sit( char_data* ch, const char *argument )
{
  if( is_fighting( ch, "sit down" )
      || is_mounted( ch, "sit down" ) ) {
    return;
  }
  
  if( !*argument ) {
    do_rest( ch, "" );
    return;
  }
  
  if( !strncasecmp( argument, "on ", 3 ) ) {
    argument += 3;
  }

  if( ch->position != POS_STANDING ) {
    send( ch,
	  "You need to be standing before you can sit down.\n\r" );
    return;
  }
  
  thing_data *thing;  
  if( !( thing = one_thing( ch, argument, "sit on", ch->array ) ) ) 
    return;
  
  obj_data *obj;
  if( !( obj = object( thing ) )  ) {
    if( ch == thing ) {
      fsend( ch, "You can't sit on yourself." );
    } else {
      fsend( ch, "You can't sit on %s.", thing );
    }
    return;
  }
  
  sit( ch, obj, true );
}


bool rest( char_data *ch, bool msg )
{
  const char *const rest = msg ? "rest" : 0;

  if( is_mounted( ch, rest ) ) {
    return false;
  }

  if( deep_water( ch ) ) {
    if( msg )
      send( ch, "You can't rest while swimming.\n\r" );
    return false;
  }
  
  if( midair( ch ) ) {
    if( msg )
      send( ch, "You can't rest while off the ground.\n\r" );
    return false;
  }

  if( is_set( ch->in_room->room_flags, RFLAG_NO_REST ) ) {
    if( msg )
      send( ch, "You can't rest here.\n\r" );
    return false;
  }

  switch( ch->position ) {
  case POS_SLEEPING:
    if( can_wake( ch, msg ) ) {
      ch->position = POS_RESTING;
      if( ch->pos_obj ) {
	ch->pos_obj->Show( 1 );
	if( msg ) {
	  fsend( ch, "You wake and sit up on %s.", ch->pos_obj );
	  fsend_seen( ch, "%s wakes and sits up on %s.", ch, ch->pos_obj );
	}
      } else if( msg ) {
	send( ch, "You wake and sit up.\n\r" );
	fsend_seen( ch, "%s wakes and sits up.", ch );
      }
      renter_combat( ch );
      return true;
    }
    break;
      
  case POS_RESTING:
    if( msg )
      send( ch,"You are already resting.\n\r" );
    break;
    
  case POS_MEDITATING:
    if( msg ) {
      send( ch, "You stop meditating and begin resting.\n\r" );
      fsend_seen( ch, "%s stops meditating and begins resting.", ch );
    }
    ch->position = POS_RESTING;
    return true;
    //    break;
    
  case POS_STANDING:
    if( ch->rider ) {
      if( msg ) {
	const char *pos = ch->in_room->position( );
	if( *pos ) {
	  fsend( ch, "You lie down %s, forcing %s to dismount.", pos, ch->rider );
	  fsend( ch->rider, "%s lies down %s, forcing you to dismount.", ch, pos );
	  fsend_seen( ch, "%s lies down %s, forcing %s to dismount.", ch, pos, ch->rider );
	} else {
	  fsend( ch, "You lie down, forcing %s to dismount.", ch->rider );
	  fsend( ch->rider, "%s lies down, forcing you to dismount.", ch );
	  fsend_seen( ch, "%s lies down, forcing %s to dismount.", ch, ch->rider );
	}
      }
      dismount( ch->rider );
    } else if( msg ) {
      const char *pos = ch->in_room->position( );
      if( *pos ) {
	fsend( ch, "You sit down %s.", pos );
	fsend_seen( ch, "%s sits down %s.", ch, pos );
      } else {
	fsend( ch, "You sit down.", pos );
	fsend_seen( ch, "%s sits down.", ch, pos );
      }
    }
    disrupt_spell( ch );
    ch->position = POS_RESTING;
    return true;
    //    break;
  }

  return false;
}


void do_rest( char_data* ch, const char * )
{
  if( is_fighting( ch, "rest" ) )
    return;

  if( rest( ch, true ) )
    set_bit( ch->status, STAT_HOLD_POS );
}


/*
 *   SLEEP 
 */


static bool can_wake( char_data* ch, bool msg  )
{
  if( !ch->is_affected( AFF_SLEEP ) )
    return true;

  if( is_apprentice( ch ) ) {
    strip_affect( ch, AFF_SLEEP, true );
    return true;
  }
  
  if( msg )
    send( ch, "You are magically slept and unable to wake.\n\r" );

  return false;
} 


void do_sleep( char_data* ch, const char *)
{
  if( is_fighting( ch, "sleep" )
      || is_mounted( ch, "sleep" ) ) {
    return;
  }

  if( water_logged( ch->in_room ) ) { 
    send( ch, "You can't sleep in the water.\n\r" );
    return;
  }

  if( midair( ch ) ) {
    send( ch, "You can't sleep while off the ground.\n\r" );
    return;
  }

  if( is_set( ch->in_room->room_flags, RFLAG_NO_REST ) ) {
    send( ch, "You can't sleep here.\n\r" );
    return;
  }

  if( mob_data *keeper = active_shop( ch ) ) {
    fsend( ch, "%s won't allow vagrants to sleep in %s shop.", 
	   keeper, keeper->His_Her( ) );
    return;
  }

  const char *const pos = ch->in_room->position( );

  switch( ch->position ) {
  case POS_SLEEPING:
    send( ch, "You are already asleep.\n\r" );
    break;
  case POS_MEDITATING:
    if( *pos ) {
      fsend( ch, "You stop meditating and go to sleep %s.", pos );
      fsend_seen( ch, "%s lies down and goes to sleep %s.", ch, pos );
    } else {
      fsend( ch, "You stop meditating and go to sleep." );
      fsend_seen( ch, "%s lies down and goes to sleep.", ch );
    }
    sleep( ch );
    break;
  case POS_RESTING:
    if( ch->pos_obj ) {
      ch->pos_obj->Show( 1 );
      fsend( ch, "You go to sleep on %s.", ch->pos_obj );
      fsend_seen( ch, "%s lies down and goes to sleep on %s.", ch, ch->pos_obj );
    } else {
      if( *pos ) {
	fsend( ch, "You go to sleep %s.", pos );
	fsend_seen( ch, "%s lies down and goes to sleep %s.", ch, pos );
      } else {
	fsend( ch, "You go to sleep." );
	fsend_seen( ch, "%s lies down and goes to sleep.", ch );
      }
    }
    sleep( ch );
    break;
  case POS_STANDING:
    if( *pos ) {
      fsend( ch, "You lie down and go to sleep %s.", pos );
      fsend_seen( ch, "%s lies down and goes to sleep %s.", ch, pos );
    } else {
      fsend( ch, "You lie down and go to sleep." );
      fsend_seen( ch, "%s lies down and goes to sleep.", ch );
    }
    sleep( ch );
    break;
  }

  set_bit( ch->status, STAT_HOLD_POS );
}


void sleep( char_data *ch )
{
  if( ch->mount ) {
    fsend( ch->mount,
	   "%s tumbles off your back.",
	   ch );
    fsend_seen( ch,
		"%s tumbles off %s %s is riding.",
		ch, ch->mount, ch->He_She( ) );
    dismount( ch );
  }

  if( ch->rider ) {
    fsend( ch->rider,
	   "%s lies down, dismounting you.",
	   ch );
    fsend_seen( ch,
		"%s lies down, dismounting %s.",
		ch, ch->rider );
    dismount( ch->rider );
  }

  disrupt_spell( ch );
  ch->position = POS_SLEEPING;  
}


/*
 *   STAND 
 */


bool stand( char_data *ch, bool msg )
{
  if( ch->mount ) {
    if( msg ) {
      send( ch, "Use dismount to stand on the ground.\n\r" );
    }
    return false;
  }
  
  switch( ch->position ) {
  case POS_SLEEPING:
    if( can_wake( ch ) ) {
      ch->position = POS_STANDING;
      remove_bit( ch->status, STAT_HOLD_POS );
      set_bit( ch->status, STAT_STOOD );
      if( ch->pos_obj ) {
	if( msg ) {
	  ch->pos_obj->Show( 1 );
	  fsend( ch, "You wake, get off %s and stand up.", ch->pos_obj );
	  fsend_seen( ch, "%s wakes, gets off %s and stands up.",
		      ch, ch->pos_obj );
	}
	unseat( ch );
      } else {
	if( msg ) {
	  send( ch, "You wake and stand up.\n\r" );
	  fsend_seen( ch, "%s wakes and stands up.", ch );
	}
      }
      renter_combat( ch );
      return true;
    }
    break;
    
  case POS_MEDITATING:
    if( msg ) {
      send( ch, "You stop meditating and stand up.\n\r" );
      fsend_seen( ch, "%s stops meditating and stands up.", ch );
    }
    ch->position = POS_STANDING;
    remove_bit( ch->status, STAT_HOLD_POS );
    set_bit( ch->status, STAT_STOOD );
    return true;
    //    break;
    
  case POS_RESTING:
    if( ch->pos_obj ) {
      if( msg ) {
	ch->pos_obj->Show( 1 );
	fsend( ch, "You get off %s and stand up.", ch->pos_obj );
	fsend_seen( ch, "%s gets off %s and stands up.",
		    ch, ch->pos_obj );
      }
      unseat( ch );
    } else {
      if( msg ) {
	send( ch, "You stand up.\n\r" );
	fsend_seen( ch, "%s stands up.", ch );
      }
    }
    ch->position = POS_STANDING;
    remove_bit( ch->status, STAT_HOLD_POS );
    set_bit( ch->status, STAT_STOOD );
    return true;
    //    break;
    
  case POS_STANDING:
    send( ch, "You are already standing.\n\r" );
    break;
  }

  return false;
}


void do_stand( char_data* ch, const char *)
{
  if( is_fighting( ch, 0 )
      && ch->position == POS_RESTING ) {
    send( ch, "You're trying, dern it.\n\r" );
    return;
  }

  stand( ch, true );
}


/*
 *   WAKE
 */


void do_wake( char_data* ch, const char *argument )
{
  char_data* victim;
  
  if( !*argument ) {
    if( ch->position > POS_SLEEPING ) {
      send( ch, "You aren't sleeping.\n\r" );
      return;
    }
    do_stand( ch, "" );
    return;
  }
  
  if( ch->position <= POS_SLEEPING ) {
    send( ch, "You are asleep yourself!\n\r" );
    return;
  }
  
  if( !( victim = one_character( ch, argument, "wake",  ch->array ) ) )
    return;
  
  if( victim->position > POS_SLEEPING ) {
    fsend( ch, "%s is already awake.", victim );
    return;
  }
  
  if( victim->position < POS_SLEEPING
      || victim->is_affected( AFF_SLEEP ) ) {
    fsend( ch, "You try to wake %s, but %s doesn't react.",
	   victim, victim->He_She( ) );
    fsend_seen( ch, "%s tries to wake %s, but %s doesn't react.",
		ch, victim, victim->He_She( ) );
    return;
  }

  if( !privileged( ch, LEVEL_BUILDER )
      && !victim->species
      && is_set( victim->pcdata->pfile->flags, PLR_NO_WAKE ) ) {
    send( victim, "Someone tries to wake you, but you ignore them.\n\r" );
    fsend( ch, "You try to wake %s, but %s rolls over and ignores you.",
	   victim, victim->He_She( ) );
    fsend_seen( ch, "%s tries to wake %s, who rolls over and ignores %s.",
		ch, victim, ch->Him_Her( ) );
    return;
  }

  fsend( ch, "You nudge %s.", victim );
  fsend( victim, "%s nudges you.", ch );
  fsend_seen( ch, "%s nudges %s.", ch, victim );

  remove_bit( victim->status, STAT_HOLD_POS );
  victim->position = POS_RESTING;

  if ( victim->pos_obj ) {
    victim->pos_obj->Show( 1 );
    fsend( victim, "You wake and sit up on %s.", victim->pos_obj );
    fsend_seen( victim, "%s wakes and sits up on %s.", victim->He_She(), victim->pos_obj );
  } else {
    send( victim, "You wake and sit up.\n\r" );
    fsend_seen( victim, "%s wakes and sits up.", victim->He_She() );
  }

  renter_combat( victim );
}
