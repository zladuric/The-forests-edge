#include <ctype.h>
#include <sys/types.h>
#include <stdio.h>
#include "define.h"
#include "struct.h"


/*
 *   CONSTANTS
 */


const direction_type dir_table [] =
{ 
  { "north", 2, "the south",  "to the north"  },
  { "east",  3, "the west",   "to the east"   },
  { "south", 0, "the north",  "to the south"  },
  { "west",  1, "the east",   "to the west"   },
  { "up",    5, "below",      "above you"     },
  { "down",  4, "above",      "below you"     }
  //  { "extra", 6, "??",         "??"            }
};


/*
 *   LOCAL FUNCTIONS
 */


static bool        passes_drunk      ( char_data* );
static bool        is_exhausted      ( char_data*, char_data*, int&, int );
static void        act_leader        ( char_data*, const char*, char_data* );

#define rd  room_data
#define cd  char_data
#define ed  exit_data
#define ad  action_data

//static void   arrival_message   ( cd*, cd*, rd*, rd*, ed*, int, ad*&, bool );
//static void   leaving_message   ( cd*, cd*, rd*, ed*, int, ad*, bool );

#undef rd
#undef cd
#undef ed
#undef ad


/*
void trigger_to_room( char_data *ch, room_data *room, exit_data *exit )
{
  species_data *species = ch->species;

  if( !species )
    return;

  for( mprog_data *mprog = species->mprog; mprog; mprog = mprog->next ) {
    if( mprog->trigger == MPROG_TRIGGER_TO_ROOM ) {
      clear_variables( );
      var_mob = ch;
      var_room = room;
      var_exit = exit;
      mprog->execute( );
      break;
    }
  }
}
*/


static const char *const leaving_verb [] = {
  "sneaks", "leaves", "flies",
  "swims", "glides", "wades",
  "falls", "climbs" }; 

static const char *const leaving_action [] = {
  "sneak", "leave", "fly",
  "swim", "glide", "wade",
  "fall", "climb" };

static const char *const arriving_verb [] = {
  "sneaks in", "arrives", "flies in",
  "swims in", "glides in", "wades in",
  "falls in", "climbs in" };


const char *move_verb( char_data *ch, const char *Movement_Data::*ptr, int type )
{
  const int movement = ( ch->species )
    ? ch->species->movement
    : ((player_data*)ch)->movement;
  
  if( type != MOVE_WALK
      || movement < 0
      || !*( movement_table[ movement ].*ptr ) ) {
    if( ptr == &Movement_Data::name ) {
      return leaving_action[type];
    } else if( ptr == &Movement_Data::leave ) {
      return leaving_verb[type];
    } else {
      return arriving_verb[type];
    }
  } else {
    return movement_table[ movement ].*ptr;
  }
}


/*
 *   MOVEMENT ABILITIES
 */


bool char_data :: can_float( ) const
{
  return is_affected( AFF_FLOAT );
}


bool char_data :: can_swim( ) const
{
  if( position <= POS_RESTING
      || is_set( status, STAT_STUNNED )
      || is_affected( AFF_PARALYSIS )
      || is_affected( AFF_ENTANGLED ) ) {
    return false;
  }

  return get_skill( SKILL_SWIMMING ) != UNLEARNT;
}


bool char_data :: can_fly( ) const
{
  if( is_affected( AFF_FLY ) )
    return true;

  if( position <= POS_RESTING
      || is_set( status, STAT_STUNNED )
      || is_affected( AFF_PARALYSIS )
      || is_affected( AFF_ENTANGLED ) ) {
    return false;
  }

  return species && is_set( species->act_flags, ACT_CAN_FLY );
}


bool char_data :: can_breathe_underwater( ) const
{
  return( is_affected( AFF_WATER_BREATHING )
	  || shdata->race == RACE_UNDEAD );
}


bool char_data :: can_climb( ) const
{
  return get_skill( SKILL_CLIMB ) != UNLEARNT;
}


bool char_data :: can_hold_light( ) const
{
  if( !in_room ) 
    return false;

  // Level 7+ swimmers don't lost their light in water_surface rooms.
  // Level 10 swimmers don't lose their light in river rooms.
  int tflags = terrain_table[ in_room->sector_type ].flags;

  if( !is_set( tflags, TFLAG_WATER ) )
    return true;

  int swim = get_skill( SKILL_SWIMMING );

  return !( is_set( tflags, TFLAG_SUBMERGED )
	    || swim < 10 && is_set( tflags, TFLAG_DEEP ) && is_set( tflags, TFLAG_FLOWING )
	    || swim < 7 && is_set( tflags, TFLAG_DEEP )
	    || swim < 7 && Size() < SIZE_DOG );
}


static char_data *following( char_data *ch )
{
  if( is_set( ch->status, STAT_FOLLOWER ) ) {
    if( ch->mount
	&& ch->mount->leader != ch
	&& is_set( ch->mount->status, STAT_FOLLOWER ) ) {
      return ch->mount->leader;
    } else {
      return ch->leader;
    }
  }

  return 0;
}


/*
 *   CAN MOVE ROUTINE
 */ 


const default_data blocking_msg [] =
{
  { "to_char",  "$N is blocking $d.", -1 },
  { "to_room",  "", -1 },
  { "", "", -1 }
};


bool char_data :: Can_Move( char_data *lead,
			    room_data *from_room, exit_data *exit, int dir,
			    int& move, int& type, bool msg,
			    bool open, bool fall, bool remote )
{
  if( rider ) {
    return rider->Can_Move( lead, from_room, exit, dir, move, type, msg, open, fall, remote );
  }

  Show( 1 );

  if( mount ) {
    mount->Show( 1 );
  }

  if( !exit
      || ( !exit->Seen( this ) && !remote ) ) {
    if( msg ) {
      if( from_room->Seen( this ) ) {
	fsend( this, "You see no exit %s.", 
	       dir_table[dir].where );
      } else {
	fsend( this, "You attempt to move %s but you run into something.",
	       dir_table[dir].name );
	fsend_seen( this,
		    "%s blindly attempts to move %s but fails to find an exit.",
		    this, dir_table[dir].name );
      }
      act_leader( lead, "** %s seems confused and does not follow you. **", this );
    }
    return false;
  }
  
  if( is_set( exit->exit_info, EX_CLOSED )
      && !remote
      && !( open
	    && species
	    && is_set( species->act_flags, ACT_OPEN_DOORS )
	    && is_set( exit->exit_info, EX_ISDOOR )
	    && !is_set( exit->exit_info, EX_NO_OPEN )
	    && !is_set( exit->affected_by, AFF_WIZLOCK )
	    && ( !is_set( exit->exit_info, EX_LOCKED )
		 || has_key( this, exit->key ) ) ) ) {
    if( !is_affected( AFF_PASS_DOOR ) ) {
      if( msg ) {
	if( from_room->Seen( this ) ) {
	  fsend( this, "%s %s closed.",
		 exit, exit_verb( this, exit ) );
	} else {
	  fsend( this, "You attempt to move %s but you run into something.",
		 dir_table[dir].name );
	  fsend_seen( this,
		      "%s blindly attempts to move %s but fails to find an exit.",
		      this, dir_table[dir].name );
	}
	act_leader( lead, "** %s cannot pass through the exit. **", this );
      }
      return false;

    } else if( is_set( exit->exit_info, EX_NO_PASS )
	       && !privileged( this, LEVEL_BUILDER ) ) {
      if( msg ) {
	if( from_room->Seen( this ) ) {
	  fsend( this, "You cannot pass through %s.",
		 exit );
	} else {
	  fsend( this, "You attempt to move %s but you run into something.",
		 dir_table[dir].name );
	  fsend_seen( this,
		      "%s blindly attempts to move %s but fails to find an exit.",
		      this, dir_table[dir].name );
	}
	act_leader( lead, "** %s cannot pass through the exit. **", this );
      }
      return false;

    } else if( !privileged( this, LEVEL_BUILDER )
	       && affect_level( this, AFF_PASS_DOOR ) <= affect_level( from_room, AFF_WIZLOCK, exit ) ) {
      if( msg ) {
	if( from_room->Seen( this ) ) {
	  fsend( this, "Some magical force blocks passage through %s.",
		 exit );
	} else {
	  fsend( this, "You attempt to move %s but you run into something.",
		 dir_table[dir].name );
	  fsend_seen( this,
		      "%s blindly attempts to move %s but fails to find an exit.",
		      this, dir_table[dir].name );
	}
	act_leader( lead, "** %s cannot pass through the exit. **", this );
      }
      return false;
    }

    if( mount ) {
      if( !mount->is_affected( AFF_PASS_DOOR ) ) {
	if( msg ) {
	  if( from_room->Seen( this ) ) {
	    fsend( this, "%s %s closed.",
		   exit, exit_verb( this, exit ) );
	  } else {
	    fsend( this, "You attempt to ride %s but %s runs into something.",
		   dir_table[dir].name, mount );
	    fsend_seen( this,
			"%s blindly attempts to ride %s but %s fails to find an exit.",
			this, dir_table[dir].name, mount );
	  }
	  act_leader( lead, "** %s's mount cannot pass through the exit. **", this );
	}
	return false;

      } else if( is_set( exit->exit_info, EX_NO_PASS )
		 && !privileged( mount, LEVEL_BUILDER ) ) {
	if( msg ) {
	  if( from_room->Seen( this ) ) {
	    fsend( this, "You cannot pass through %s.",
		   exit );
	  } else {
	    fsend( this, "You attempt to ride %s but %s runs into something.",
		   dir_table[dir].name, mount );
	    fsend_seen( this,
			"%s blindly attempts to ride %s but %s fails to find an exit.",
			this, dir_table[dir].name, mount );
	  }
	  act_leader( lead, "** %s's mount cannot pass through the exit. **", this );
	}
	return false;

      } else if( !privileged( mount, LEVEL_BUILDER )
		 && affect_level( mount, AFF_PASS_DOOR ) <= affect_level( from_room, AFF_WIZLOCK, exit ) ) {
	if( msg ) {
	  if( from_room->Seen( this ) ) {
	    fsend( this, "Some magical force blocks %s's passage through %s.",
		   mount, exit );
	  } else {
	    fsend( this, "You attempt to ride %s but %s runs into something.",
		   dir_table[dir].name, mount );
	    fsend_seen( this,
			"%s blindly attempts to ride %s but %s fails to find an exit.",
			this, dir_table[dir].name, mount );
	  }
	  act_leader( lead, "** %s's mount cannot pass through the exit. **", this );
	}
	return false;
	
      }
    }
  }

  char_data *mover = mount ? mount : this;

  if( is_set( exit->exit_info, EX_REQUIRES_CLIMB ) 
      && !mover->can_climb( )
      && !mover->can_fly( ) ) {
    if( msg ) {
      if( mount ) {
	fsend( this, "Leaving %s requires climbing, of which %s is incapable.",
	       dir_table[dir].name, mount );
	act_leader( lead, "** %s's mount is unable to climb so can't follow you. **", this );
      } else {
	fsend( this, "Leaving %s requires climbing, of which you are incapable.",
	       dir_table[dir].name );
	act_leader( lead, "** %s is unable to climb so can't follow you. **", this );
      }
    }
    return false;
  }


  if( is_set( exit->affected_by, AFF_WARD )
      && !remote
      && !is_set( status, STAT_RESPOND ) ) {
    if( shdata->race == RACE_UNDEAD ) {
      if( msg ) {
	fsend( this, "You are thrown back by an invisible barrier on %s.", exit );
	fsend_seen( this,
		    "%s attempts to move %s but is thrown back by an invisible barrier.",
		    this, dir_table[dir].name );
	act_leader( lead, "** %s is thrown back by an invisible barrier. **", this );
      }
      return false;
    }
    if( mount
	&& mount->shdata->race == RACE_UNDEAD
	&& !is_set( mount->status, STAT_RESPOND ) ) {
      if( msg ) {
	fsend( this, "%s is thrown back by an invisible barrier on %s.", mount, exit );
	fsend_seen( this,
		    "%s attempts to ride %s but %s is thrown back by an invisible barrier.",
		    this, dir_table[dir].name, mount );
	act_leader( lead, "** %s's mount is thrown back by an invisible barrier. **", this );
      }
      return false;
    }
  }

  if( Size( ) > exit->size ) {
    if( msg ) {
      fsend( this, "You are too large to fit through %s.", exit );
      act_leader( lead, "** %s is too large to follow you through the exit. **", this );
    }
    return false;
  }

  if( mount && mount->Size( ) > exit->size ) {
    if( msg ) {
      fsend( this, "%s is too large to fit through %s.", mount, exit );
      act_leader( lead, "** %s's mount is too large to follow you through the exit. **", this );
    }
    return false;
  }
    
  room_data *to_room = exit->to_room;

  if( Size( ) > to_room->size ) {
    if( msg ) {
      send( this, "You are too large to fit in there.\n\r" );
      act_leader( lead, "** %s is too large to follow you there. **", this );
    }
    return false;
  }

  if( mount ) {
    if( mount->Size( ) > to_room->size ) {
      if( msg ) {
	fsend( this, "%s is too large to fit in there.", mount );
	act_leader( lead, "** %s's mount is too large to follow you there. **", this );
      }
      return false;
    }
    if( is_set( to_room->room_flags, RFLAG_NO_MOUNT ) ) {
      if( msg ) {
	send( this, "You cannot go there while mounted.\n\r" );
	act_leader( lead, "** %s cannot follow you there while mounted. **", this );
      }
      return false;
    }
  }

  if( fall ) {
    type = MOVE_FALL;
    move = 0;
    return true;
  }

  // Blocking.
  if( msg ) {
    bool after = false;
    for( int i = 0; i < *array; ++i ) { 
      if( char_data *rch = character( array->list[i] ) ) {
	if( rch == mover ) {
	  after = true; 
	  continue;
	}
	if( rch->Size( ) > max( SIZE_GNOME, mover->Size( )-2 )
	    && rch->position >= POS_STANDING
	    && !rch->fighting
	    && !is_entangled( rch )
	    && mover->Seen( rch )
	    && ( ( after && rch->room_position == dir )
		 || ( !after && mover->room_position != -1 && mover->room_position != dir ) ) ) {
	  rch->Select( 1 );
	  bool result = true;
	  mprog_data *mprog = 0;
	  if( rch->species ) {
	    for( mprog = rch->species->mprog; mprog; mprog = mprog->next ) {
	      if( mprog->trigger == MPROG_TRIGGER_BLOCK ) {
		clear_variables( );
		var_mob = rch;
		var_ch = this;
		var_room = from_room;
		var_exit = exit;
		result = mprog->execute( );
		if( !Is_Valid( ) ) {
		  return false;
		}
		break;
	      }
	    }
	  }
	  if( !result
	      || !mprog && ( rch->aggressive.includes( mover ) || rch->aggressive.includes( this ) ) ) {
	    act( this, prog_msg( mprog, blocking_msg[0] ),
		 this, rch, 0, 0, exit );
	    act_notchar( prog_msg( mprog, blocking_msg[1] ),
			 this, rch, 0, 0, exit );
	    act_leader( lead, "** %s is blocked from following you. **", this );
	    return false;
	  }
	}
      }
    }
  }

  const bool forced = is_set( status, STAT_FORCED )
                      || mount && is_set( mount->status, STAT_FORCED );

  // Can't move to/from unopen areas.
  if( from_room->area != to_room->area
      && ( from_room->area->status != AREA_OPEN || to_room->area->status != AREA_OPEN )
      && !forced
      && !is_apprentice( this )
      && !( is_set( status, STAT_PET ) && lead == leader && is_apprentice( leader ) ) ) {
    if( msg ) {
      send( this, "That area is not open to players yet.\n\r" );
      act_leader( lead, "** %s cannot follow you into an unopen area. **", this );
    }
    return false;
  }

  const int sect_from = from_room->sector_type;
  const int sect_to = to_room->sector_type;
  int move_from = terrain_table[ sect_from ].mv_cost;
  int move_to = terrain_table[ sect_to ].mv_cost;
  const bool deep_from = deep_water( mover, from_room );
  const bool deep_to = deep_water( mover, to_room );
  const bool sub_from = is_submerged( 0, from_room );
  const bool sub_to = is_submerged( 0, to_room );
  const bool air_from = midair( 0, from_room );
  const bool air_to = midair( 0, to_room );

  // Moves forbidden to wandering mobs.
  if( !pcdata
      && !is_set( status, STAT_PET )
      && !is_set( status, STAT_RESPOND )
      && !forced ) {
    // No.Mob
    if( is_set( exit->exit_info, EX_NO_MOB )
	|| is_set( to_room->room_flags, RFLAG_NO_MOB ) ) {
      return false;
    }
    // Sentinel.
    if( is_set( status, STAT_SENTINEL ) ) {
      return false;
    }
    // Stay_Area.
    if( is_set( species->act_flags, ACT_STAY_AREA )
	&& to_room->area != from_room->area ) {
      return false;
    }
    // Stay_Terrain.
    if( is_set( species->act_flags, ACT_STAY_TERRAIN )
	&& sect_to != sect_from ) {
      return false;
    }
    // Submerging.
    if( !sub_from
	&& sub_to
	&& ( !can_breathe_underwater( )
	     || mount && !mount->can_breathe_underwater( ) ) ) {
      return false;
    }
    // Entering/leaving deep water.
    if( ( deep_from && !deep_to || !deep_from && deep_to )
	&& ( !is_affected( AFF_WATER_WALKING )
	     || mount && !mount->is_affected( AFF_WATER_WALKING ) ) ) {
      return false;
    }
  }

  if( ( air_to
	|| air_from )
      && !mover->can_fly() ) {
    if( msg ) {
      if( mount ) {
	fsend( this, "%s does not know how to fly.", mount );
	act_leader( lead, "** %s's mount is unable to fly so does not follow you. **", this );
      } else {
	send( this, "You can't fly.\n\r" );
	act_leader( lead, "** %s is unable to fly so does not follow you. **", this );
      }
      return false;
    }
  }
  
  if( sub_to
      || sub_from ) {
    if( !mover->can_swim() ) {
      if( msg ) {
	if( mount ) {
	  fsend( this, "%s does not know how to swim.", mount );
	  act_leader( lead, "** %s's mount cannot swim so fails to follow you. **", this ); 
	} else {
	  send( this, "You don't how to swim.\n\r" );
	  act_leader( lead, "** %s cannot swim so fails to follow you. **", this ); 
	}
      }
      return false;
    } else if( mount && !mount->can_breathe_underwater( ) ) {
      if( msg ) {
	fsend( this, "%s cannot be ridden underwater.", mount );
	act_leader( lead, "** %s cannot ride underwater so fails to follow you. **", this ); 
      }
      return false;
    }

  } else if( ( deep_from || deep_to )
	     && !mover->can_swim()
	     && !mover->can_float()
	     && !( !mover->species && mover->is_affected( AFF_WATER_WALKING ) )
	     && !mover->can_fly() ) {
    if( msg ) {
      if( mount ) {
	fsend( this, "%s does not know how to swim or fly.", mount );
	act_leader( lead, "** %s's mount cannot swim or fly so fails to follow you. **", this ); 
      } else {
	send( this, "You don't how to swim or fly.\n\r" );
	act_leader( lead, "** %s cannot swim or fly so fails to follow you. **", this ); 
      }
    }
    return false;
  }

  type = MOVE_WALK;

  const bool flies = mover->species && is_set( mover->species->act_flags, ACT_CAN_FLY );

  const int from_flags = terrain_table[ sect_from ].flags;

  if( air_from ) {
    if( flies ) {
      move_from = ( mount ? 2 : 1 );
    }
    type = MOVE_FLY;

  } else if( is_set( from_flags, TFLAG_WATER ) ) {
    if( is_set( from_flags, TFLAG_SUBMERGED ) ) {
      // Underwater.
      type = MOVE_SWIM;

    } else if( is_set( from_flags, TFLAG_DEEP ) ) {
      // River or surface.
      if( mover->can_float()
	  || !mover->species && mover->is_affected( AFF_WATER_WALKING )
	  || flies ) {
	move_from = ( mount ? 2 : 1 );
      } else if( !mover->can_swim() ) {
	move_from *= 2;
      }
      
    } else {
      // Shallows.
      if( mover->can_float()
	  || !mover->species && mover->is_affected( AFF_WATER_WALKING )
	  || flies ) {
	move_from = ( mount ? 2 : 1 );
      } else if( mover->Size() < SIZE_DOG ) {
	if( !mover->can_swim() ) {
	  move_from *= 2;
	}
      }
    }

  } else {
    if( flies || mover->can_float() ) {
      move_from = ( mount ? 2 : 1 );
    }
  }

  const int to_flags = terrain_table[ sect_to ].flags;

  if( air_to ) {
    if( flies ) {
      move_to = ( mount ? 2 : 1 );
    }
    if( type == MOVE_WALK )
      type = MOVE_FLY;

  } else if( is_set( to_flags, TFLAG_WATER ) ) {
    if( is_set( to_flags, TFLAG_SUBMERGED ) ) {
      // Underwater.
      if( !species
	  && is_affected( AFF_WATER_WALKING )
	  && !sub_from ) {
	if( msg ) {
	  send( this, "You cannot submerge.\n\r" );
	  act_leader( lead, "** %s cannot submerge so fails to follow you. **", this );
	}
	return false;
      }
      type = MOVE_SWIM;
      
    } else if( is_set( to_flags, TFLAG_DEEP ) ) {
      // River or surface.
      if( flies ) {
	move_to = ( mount ? 2 : 1 );
	if( type == MOVE_WALK )
	  type = MOVE_FLY;
      } else if( mover->can_float()
		 || !mover->species && mover->is_affected( AFF_WATER_WALKING ) ) {
	move_to = ( mount ? 2 : 1 );
	if( type == MOVE_WALK )
	  type = MOVE_GLIDE;
      } else if( !mover->can_swim() ) {
	move_to *= 2;
	if( type == MOVE_WALK )
	  type = MOVE_SWIM;
      } else {
	if( type == MOVE_WALK )
	  type = MOVE_SWIM;
      }

    } else {
      // Shallows.
      if( flies ) {
	move_to = ( mount ? 2 : 1 );
	if( type == MOVE_WALK )
	  type = MOVE_FLY;
      } else if( mover->can_float()
		 || !mover->species && mover->is_affected( AFF_WATER_WALKING ) ) {
	move_to = ( mount ? 2 : 1 );
	if( type == MOVE_WALK )
	  type = MOVE_GLIDE;
      } else if( mover->Size() < SIZE_DOG ) {
	if( !mover->can_swim() ) {
	  move_to *= 2;
	  if( type == MOVE_WALK )
	    type = MOVE_SWIM;
	} else {
	  if( type == MOVE_WALK )
	    type = MOVE_SWIM;
	}
      } else if( mover->is_humanoid() ) {
	if( type == MOVE_WALK )
	  type = MOVE_WADE;
      }
    }

  } else {
    if( flies || mover->can_float() ) {
      move_to = ( mount ? 2 : 1 );
    }
  }

  if( type == MOVE_WALK ) {
    if( flies ) {
      type = MOVE_FLY;
    } else if( is_set( exit->exit_info, EX_REQUIRES_CLIMB ) ) {
      if( mover->can_fly( ) ) {
	type = MOVE_FLY;
      } else {
	move_to *= 2;
	type = MOVE_CLIMB;
      }
    } else if( mover->can_float() ) {
      type = MOVE_GLIDE;
    }
  }

  move = int( double( move_from + move_to )
	      * ( 2.0 - 5.0 * double( mover->Capacity() ) / double( mover->Empty_Capacity() ) / 3.0 ) );

  return true;
}


/*
 *   MAIN MOVEMENT ROUTINE
 */


static bool move_part1( char_data *ch, char_data *leader,
			room_data *from_room, exit_data *exit, int door,
			int& type, bool flee, bool fall,
			action_data*& action )
{
  // A leaving trigger might have done something odd, moving people.
  if( ch->in_room != from_room ) {
    return false;
  }

  // No one follows if the original mover fled or fell.
  // Or if leader didn't move.
  if( leader ) {
    if( flee
	|| fall
	|| is_set( leader->status, STAT_NOFOLLOW ) ) {
      return false;
    }
  }

  ch->Show( 1 );

  char_data *mount = ch->mount;

  if( mount ) {
    ch->mount->Show( 1 );
  }

  char_data *mover = mount ? mount : ch;

  const bool remount = mount && mount->leader != ch;

  if( !fall ) {
    // Sleeping/dying/etc.--whether leader is seen is meaningless.
    if( ch->position <= POS_SLEEPING ) {
      if( leader ) {
	char buf [ THREE_LINES ];
	snprintf( buf, THREE_LINES, "** %%s is %s and so does not follow you. **",
		  ch->position_name( ) );
	act_leader( leader, buf, ch );
      }
      return false;
    }
    
    if( leader ) {
      char_data *seer = remount ? mount : ch;
      if( ( seer->pcdata || !seer->Can_See( ) )
	  && !leader->Seen( seer ) ) {
	act_leader( leader, "** %s does not follow you. **", seer );
	return false;
      }
    }
    
    // Resting--after checking for whether leader is seen.
    if( ch->position <= POS_RESTING ) {
      fsend( ch, "You cannot move while %s.", ch->position_name( ) );
      if( leader ) {
	char buf [ THREE_LINES ];
	snprintf( buf, THREE_LINES, "** %%s is %s and so does not follow you. **",
		  ch->position_name( ) );
	act_leader( leader, buf, ch );
      }
      return false;
    }
    
    if( remount
	&& !is_set( mount->status, STAT_FOLLOWER )
	&& !is_set( mount->status, STAT_ORDERED )
	&& !is_set( mount->status, STAT_FORCED ) ) {
      fsend( ch, "%s refuses to move.", mount );
      act_leader( leader, "** %s's mount refuses to follow you. **", ch );
      return false;
    }
    
    if( mover->is_affected( AFF_PARALYSIS ) ) {
      if( mount ) {
	fsend( ch, "%s is paralyzed and unable to move.", mount );
	act_leader( leader, "** %s's mount is paralyzed and unable to follow you. **", ch );
      } else {
	send( ch, "You are paralyzed and unable to move.\n\r" );
	act_leader( leader, "** %s is paralyzed and unable to follow you. **", ch );
      }
      return false;
    }
    
    if( mover->is_affected( AFF_ENTANGLED ) ) {
      if( mount ) {
	fsend( ch, "%s is entangled in a web and quite stuck.", mount );
	act_leader( leader, "** %s's mount is entangled in a web and unable to follow you. **", ch );
      } else {
	send( ch, "You are entangled in a web and quite stuck.\n\r" );
	act_leader( leader, "** %s is entangled in a web and unable to follow you. **", ch );
      }
      return false;
    }
    
    if( mover->is_affected( AFF_CHOKING ) ) {
      if( mount ) {
	fsend( ch, "%s is choking to death and unable to move.", mount );
	act_leader( leader, "** %s's mount is choking to death and unable to follow you. **", ch );
      } else {
	send( ch, "You are choking to death, unable to move.\n\r" );
	act_leader( leader, "** %s is choking to death and unable to follow you. **", ch );
      }
      return false;
    }
    
    if( !flee ) {    
      if( opponent( ch ) ) {
	send( ch, "You can't walk away from a battle - use flee.\n\r" );
	act_leader( leader, "** %s is fighting and unable to follow you. **", ch );
	return false;
      }
      
      if( mount && opponent( mount ) ) {
	fsend( ch, "%s can't walk away from a battle - use flee.", mount );
	act_leader( leader, "** %s's mount is fighting and unable to follow you. **", ch );
	return false;
      }
      
      if( ch->pcdata
	  && !remount
	  && is_set( ch->pcdata->pfile->flags, PLR_JOIN_FIGHT ) ) {
	for( int i = 0; i < from_room->contents; ++i ) {
	  if( char_data *rch = character( from_room->contents[i] ) ) {
	    char_data *victim = rch->fighting;
	    if( victim && join_fight( victim, rch, ch ) ) {
	      fsend( ch, "You decide to stay and join the fight." );
	      act_leader( leader, "** %s does not follow you. **", ch );
	      return false;
	    }
	  }
	}
      }
    }
  }

  int move;
  if( !ch->Can_Move( leader, from_room, exit, door, move, type, !fall, false, fall ) ) {
    return false;
  }

  if( flee ) {
    // Fleeing stops searching, tracking, sneaking, and covering tracks.
    remove_bit( mover->status, STAT_SNEAKING );
    remove_bit( mover->status, STAT_COVER_TRACKS );
    if( !mover->species ) {
      remove_bit( mover->pcdata->pfile->flags, PLR_SEARCHING );
      remove_bit( mover->pcdata->pfile->flags, PLR_TRACK );
    }

  } else if( !fall ) {
    if( type == MOVE_WALK
	&& is_set( mover->status, STAT_SNEAKING ) ) {
      move += 2;
    }
    
    if( type == MOVE_WALK
	&& is_set( mover->status, STAT_COVER_TRACKS )
	&& can_track( from_room ) ) {
      move += 2;
    }
    
    if( !mover->species ) {
      if( is_set( mover->pcdata->pfile->flags, PLR_TRACK ) ) {
	move += 2;
      }
      if( is_set( mover->pcdata->pfile->flags, PLR_SEARCHING ) ) {
	// Note: 1 more mv assessed by show_secret() in show_room().
	move += 2;
      }
    }
  }

  if( !fall ) {
    if( is_exhausted( ch, leader, move, type )
	|| !passes_drunk( ch ) ) {
      return false;
    }
  }

  if( !trigger_leaving( ch, from_room, exit, door, action ) ) {
    return false;
  }

  if( !flee
      && ( type == MOVE_WALK || type == MOVE_GLIDE )
      && is_set( mover->status, STAT_SNEAKING )
      && mover->check_skill( SKILL_SNEAK, 30 ) ) {
    type = MOVE_SNEAK;
    set_bit( mover->status, STAT_SNUCK );
  } else if( flee ) {
    type = MOVE_FLEE;
  }

  mover->move -= move;

  return true;
}


static void list_followers( char_data *leader, char_data *ch, char_array& array, room_data *from_room )
{
  if( ch->in_room != from_room
      || array.includes( ch ) )
    return;

  // Make sure the mount is before its non-leader rider.
  if( ch->mount
      && ch->mount->leader != ch
      && !array.includes( ch->mount ) ) {
    return;
  }

  array.append( ch );
  
  if( !is_set( ch->status, STAT_LEADER ) ) {
    set_bit( ch->status, STAT_FOLLOWER );
    if( ( ch->pcdata || !ch->Can_See( ) )
	&& leader
	&& !leader->Seen( ch ) ) {
      set_bit( ch->status, STAT_NOFOLLOW );
      if( ch->rider
	  && ch->leader != ch->rider ) {
	array.append( ch->rider );
	set_bit( ch->rider->status, STAT_NOFOLLOW );
      }
      return;
    }
  }

  if( ch->rider
      && ch->leader != ch->rider ) {
    list_followers( 0, ch->rider, array, from_room );
  }

  for( int i = 0; i < ch->followers; ++i ) {
    char_data *follower = ch->followers[i];
    list_followers( ch, follower, array, from_room );
  }
}


/* 
 *   MESSAGE ROUTINES
 */


// Victim's movement is either seen or heard by ch.
static bool detect_move( char_data *ch, char_data *victim )
{
  if( ch == victim || !is_set( victim->status, STAT_SNUCK ) )
    return true;

  // Note: switched imms' holylight works.
  if( has_holylight( ch ) )
    return true;
  
  // Can see own pets sneak.
  if( victim->Seen( ch )
      && ( victim->leader == ch )
      && is_set( victim->status, STAT_PET ) ) {
    return true;
  }

  return false;
}


static void follow_message( char_data *ch, char_data *actor, char_data *leader,
			    bool leave, int type )
{
  // Leader may have been combined as a follower.
  leader->Show( 1 );

  const bool plural = ( actor->Shown( ) > 1 );
  
  if( ch == leader ) {
    if( !actor->mount ) {
      const int movement = ( actor->species )
	? actor->species->movement
	: ((player_data*)actor)->movement;
      if( type == MOVE_WALK
	  && movement < 0 ) {
	fsend( ch, "%s follow%s you.", actor,
	       plural ? "" : "s" );
      } else {
	const char *verb;
	if( plural ) {
	  verb = ( type != MOVE_WALK )
	    ? leaving_action[type] : movement_table[ movement ].name;
	} else {
	  verb = ( type != MOVE_WALK )
	    ? leaving_verb[type] : movement_table[ movement ].leave;
	}
	fsend( ch, "%s %s after you.", actor, verb );
      }
    } else {
      fsend( ch, "%s ride%s after you.", actor,
	     plural ? "" : "s" );
    }
    return;
  }

  if( !actor->mount ) {
    const int movement = ( actor->species )
      ? actor->species->movement
      : ((player_data*)actor)->movement;
    if( type == MOVE_WALK
	  && movement < 0 ) {
      fsend( ch, "%s follow%s %s.", actor,
	     plural ? "" : "s",
	     leader );
    } else if( leave ) {
      const char *verb;
      if( plural ) {
	verb = ( type != MOVE_WALK )
	  ? leaving_action[type] : movement_table[ movement ].name;
      } else {
	verb = ( type != MOVE_WALK )
	  ? leaving_verb[type] : movement_table[ movement ].leave;
      }
      fsend( ch, "%s %s after %s.",
	     actor, verb, leader );
    } else if( plural ) {
      // Yes, we use leaving verbs for arriving plurals...
      const char *verb = ( type != MOVE_WALK )
	? leaving_action[type] : movement_table[ movement ].name;
      fsend( ch, "%s %s in after %s.",
	     actor, verb, leader );
    } else {
      const char *verb = ( type != MOVE_WALK )
	? arriving_verb[type] : movement_table[ movement ].arrive;
      fsend( ch, "%s %s after %s.",
	     actor, verb, leader );
    }
    return;
  }

  fsend( ch, "%s ride%s after %s.", actor,
	 plural ? "" : "s",
	 leader );

  /*
  if( !leader && leave
      || !actor->Seen( ch ) ) {
    return false;
  }

  if( ( is_set( ch->status, STAT_LEADER ) || is_set( ch->status, STAT_FOLLOWER ) )
      && ( !leave || !is_set( ch->pcdata->message, MSG_GROUP_MOVE ) ) ) {
    return true;
  }
  
  if( !leader ) {
    return false;
  }

  if( ch == leader ) {
    if( !actor->mount ) {
      const int movement = ( actor->species )
	? actor->species->movement
	: ((player_data*)actor)->movement;
      if( type == MOVE_WALK
	  && movement < 0 ) {
	fsend( ch, "%s follows you.", actor );
      } else {
	fsend( ch, "%s %s after you.",
	       actor,
	       ( type != MOVE_WALK )
	       ? leaving_verb[type] : movement_table[ movement ].leave );
      }
    } else {
      fsend( ch, "%s rides after you.", actor );
    }
    return true;
  }
  
  if( snuck && !detect_move( ch, leader )
      || !leader->Seen( ch ) )
    return false;
  
  if( !actor->mount ) {
    const int movement = ( actor->species )
      ? actor->species->movement
      : ((player_data*)actor)->movement;
      if( type == MOVE_WALK
	  && movement < 0 ) {
	fsend( ch, "%s follows %s.", actor, leader );
      } else if( leave ) {
	//		 || is_set( ch->status, STAT_LEADER )
	//		 || is_set( ch->status, STAT_FOLLOWER ) ) {
	fsend( ch, "%s %s after %s.",
	       actor,
	       ( type != MOVE_WALK )
	       ? leaving_verb[type] : movement_table[ movement ].leave,
	       leader );
      } else {
	fsend( ch, "%s %s after %s.",
	       actor,
	       ( type != MOVE_WALK )
	       ? arriving_verb[type] : movement_table[ movement ].arrive,
	       leader );
      }

  } else {
    fsend( ch, "%s rides after %s.", actor, leader );
  }
  */

    //  return true;
}


const default_data entering_msg [] =
{
  { "to_room",  "$n $t from $T.", -1 },
  { "", "", -1 }
};


static action_data *arrival_action( room_data *room, int dir )
{
  for( action_data *action = room->action; action; action = action->next ) {
    if( action->trigger == TRIGGER_ENTERING
	&& ( ( dir < MAX_DIR_COMPASS && is_set( action->flags, DIR_ANY ) )
	     || is_set( action->flags, dir ) ) ) {
      return action;
    }
  }

  return 0;
}


static void arriving_other( char_data* rch, char_data* ch, room_data *from,
			    exit_data *exit, int type, action_data *action )
{
  if( !ch->Seen( rch )
      && ch->mount
      && ch->mount->Seen( rch ) ) {
    ch = ch->mount;
  }

  if( !ch->Seen( rch ) ) {
    if( rch->Can_Hear()
	&& get_trust( rch ) >= invis_level( ch ) )
      send( rch, "You hear someone or something arrive.\n\r" );
    return;
  }

  if( type == MOVE_FALL ) {
    if( ch->mount ) {
      fsend( rch,
	     "%s, riding %s, falls in uncontrollably from %s.",
	     ch, ch->mount, dir_table[exit->direction].arrival_msg );
    } else {
      fsend( rch,
	     "%s falls in uncontrollably from %s.",
	     ch, dir_table[exit->direction].arrival_msg );
    }
    return;
    
  } else if( type == MOVE_FLEE ) {
    int back = dir_table[ exit->direction ].reverse;
    if( ch->mount ) {
      fsend( rch,
	     "%s, riding %s, charges blindly in, fleeing something %s.",
	     ch, ch->mount, dir_table[back].where );
    } else {
      fsend( rch,
	     "%s arrives, obviously fleeing something %s.",
	     ch, dir_table[back].where );
    }
    return;
  }

  const char *verb = 0;

  if( !ch->mount ) {
    verb = move_verb( ch, &Movement_Data::arrive, type );
  }

  exit_data *rev = reverse( exit );
  if( rev
      && rev->to_room == from
      && is_set( exit->exit_info, EX_CLOSED ) ) {
    if( ch->mount ) {
      fsend( rch, "%s riding %s arrives right through %s!",
	     ch, ch->mount, rev );
    } else {
      fsend( rch, "%s %s right through %s!",
	     ch, verb, rev );
    }

  } else {
    if( ch->mount ) {
      fsend( rch, "%s riding %s arrives from %s.",
	     ch, ch->mount, dir_table[ exit->direction ].arrival_msg );
    } else {
      act( rch, prog_msg( action, entering_msg[0] ),
	   ch, 0, verb,
	   dir_table[ exit->direction ].arrival_msg );
    }
  }
}


/*
static void arrival_message( char_data *ch, char_data *leader,
			     room_data *room, room_data *from, exit_data* exit,
			     int type, action_data *action )
{
  //  const int dir = dir_table[ exit->direction ].reverse;

  //  action = arrival_action( room, dir );

  ch->Show( 1 );

  char_data *rch;
  for( int i = 0; i < room->contents; ++i ) {
    if( ( rch = character( room->contents[i] ) )
	&& rch != ch
	&& rch->link
	&& rch->position > POS_SLEEPING
	&& detect_move( rch, ch )
	&& !follow_message( rch, ch, leader, false, type ) ) {
      arriving_other( rch, ch, from, exit, type, action );
    }
  }

  // Sense Danger affect.
  for( int i = 0; i < room->exits; ++i ) {
    room_data *to_room = room->exits[i]->to_room;
    if( to_room != from ) {
      for( int j = 0; j < to_room->contents; ++j )  
	if( ( rch = character( to_room->contents[j] ) )
	    && rch->is_affected( AFF_SENSE_DANGER ) ) {
	  fsend( rch, "You sense %s %s.", ch,
		 dir_table[ dir_table[ room->exits[i]->direction ].reverse ].where );
	}
    }
  }
}
*/


/* 
 *   LEAVING MESSAGES
 */


const default_data leaving_msg [] =
{
  { "to_char",  "You $t $T.", -1 },
  { "to_room",  "$n $t $T.", -1 },
  { "", "", -1 }
};


static void leaving_self( char_data *ch, char_data *leader, exit_data *exit,
			  int type, action_data *action )
{
  if( type == MOVE_FALL ) {
    if( ch->mount ) {
      fsend( ch, "Falling uncontrollably, you ride %s %s.",
	     ch->mount, dir_table[ exit->direction ].name );
    } else {
      fsend( ch, "The inexorable force of gravity pulls you %s.",
	     dir_table[ exit->direction ].name );
    }
    return;
  }
    
  if( type == MOVE_FLEE ) {
    if( ch->mount ) {
      fsend( ch, "Fleeing the battle, you ride %s %s.",
	     ch->mount, dir_table[ exit->direction ].name );
    } else {
      fsend( ch, "You flee %s.", dir_table[ exit->direction ].name );
    }
    return;
  }

  if( leader ) {
    if( !ch->mount ) {
      const int movement = ( ch->species )
	? ch->species->movement
	: ((player_data*)ch)->movement;
      if( type == MOVE_WALK
	  && movement < 0 ) {
	fsend( ch, "You follow %s.", leader->Seen_Name( ch ) );
      } else {
	fsend( ch, "You %s after %s.",
	       ( type != MOVE_WALK )
	       ? leaving_action[type] : movement_table[ movement ].name,
	       leader->Seen_Name( ch ) );
      }

    } else {
      fsend( ch, "You ride after %s.", leader->Seen_Name( ch ) );
    }

  } else if( is_set( exit->exit_info, EX_CLOSED ) ) {
    if( !ch->mount ) {
      fsend( ch, "You %s through %s!",
	     ( exit->direction == DIR_UP || exit->direction == DIR_DOWN )
	     ? "climb" : "step",
	     exit );
    } else {
      fsend( ch, "You ride through %s!",
	     exit );
    }

  } else if( ch->mount ) {
    // *** FIX ME: what if it's water?
    fsend( ch, "You ride %s %s.", ch->mount,
	   dir_table[ exit->direction ].name );

  } else {
    const char *verb = move_verb( ch, &Movement_Data::name, type );
    act( ch, prog_msg( action, leaving_msg[0] ),
	 ch, 0, verb,
	 dir_table[ exit->direction ].name );
  }
}


static void leaving_other( char_data* rch, char_data* ch, room_data* room,
			   exit_data* exit, int type, action_data* action )
{
  if( !ch->Seen( rch )
      && ch->mount
      && ch->mount->Seen( rch ) )
    ch = ch->mount;

  if( !ch->Seen( rch ) ) {
    if( rch->Can_Hear()
	&& get_trust( rch ) >= invis_level( ch ) )
      send( rch, "You hear someone or something leave.\n\r" );
    return;
  }

  if( type == MOVE_FALL ) {
    if( ch->mount ) {
      fsend( rch, "Falling uncontrollably, %s rides %s %s.",
	     ch, ch->mount, dir_table[ exit->direction ].name );
    } else {
      fsend( rch, "The inexorable force of gravity pulls %s %s.", ch,
	     dir_table[ exit->direction ].name );
    }
    return;
  }

  if( type == MOVE_FLEE ) {
    if( ch->mount ) {
      fsend( rch, "Fleeing the battle, %s rides %s %s.",
	     ch, ch->mount, dir_table[ exit->direction ].name );
    } else {
      fsend( rch, "%s blindly flees %s.", ch,
	     dir_table[ exit->direction ].name );
    }
    return;
  }
  
  const char *verb = 0;

  if( !ch->mount ) {
    verb = move_verb( ch, &Movement_Data::leave, type );
  }
  
  if( is_set( exit->exit_info, EX_CLOSED ) ) {
    if( ch->mount ) {
      fsend( rch, "%s rides %s right through %s!",
	     ch, ch->mount, exit );
    } else {
      fsend( rch, "%s %s right through %s!",
	     ch, verb, exit );
    }

  } else {
    if( ch->mount ) {
      fsend( rch, "%s rides %s %s.", ch, ch->mount,
	     dir_table[ exit->direction ].name );
    } else {
      act( rch, prog_msg( action, leaving_msg[1] ),
	   ch, 0, verb,
	   dir_table[ exit->direction ].name );
    }
  }
}


/*
static void leaving_message( char_data *ch, char_data *leader,
			     room_data *room, exit_data* exit,
			     int type, action_data* action, bool snuck )
{
  leaving_self( ch, leader, exit, type, action );
  
  for( int i = 0; i < room->contents; ++i ) {
    char_data *rch = character( room->contents[i] );
    if( rch
	&& ch != rch
	&& rch->link
	&& rch->position > POS_SLEEPING
	&& ( type != MOVE_SNEAK || detect_move( rch, ch ) )
	&& !follow_message( rch, ch, leader, true, snuck, type ) ) {
      leaving_other( rch, ch, room, exit, type, action );
    }
  }
}
*/


bool move_char( char_data *ch, int door, bool flee, bool fall )
{
  if( ch->rider ) {
    return move_char( ch->rider, door, flee, fall );
  }

  set_bit( ch->status, STAT_LEADER );

  room_data *from_room = ch->in_room;

  char_array followers;

  // Make a list of the movers.
  if( !flee && !fall ) {
    list_followers( 0, ch, followers, from_room );
  } else {
    followers.append( ch );
    if( ch->mount ) {
      followers.append( ch->mount );
    }
  }

  exit_data *exit = exit_direction( from_room, door );

  // Determine whether each member can move.
  Array<int> types;
  action_array actions;
  int j = 0;
  bool cover = false;
  for( int i = 0; i < followers; ++i ) {
    char_data *follower = followers[i];

    if( follower->rider ) {
      set_bit( follower->status, STAT_NOFOLLOW );
      continue;
    }

    char_data *leader = following( follower );

    int type;
    action_data *action;
    if( !move_part1( follower, leader, from_room, exit, door, type, flee, fall, action ) ) {
      set_bit( follower->status, STAT_NOFOLLOW );
      if( follower->mount ) {
	set_bit( follower->mount->status, STAT_NOFOLLOW );
      }
      continue;
    }
    
    if( is_set( follower->status, STAT_COVER_TRACKS )
	&& can_track( from_room ) )
      cover = true;

    ++j;
    types.append( type );
    actions.append( action );
  }

  // Did anyone move?
  if( const int count = j ) {

    // Print leaving messages.
    for( int k = 0; k < from_room->contents; ++k ) {
      char_data *rch = character( from_room->contents[k] );
      if( rch
	  && rch->link
	  && rch->position > POS_SLEEPING ) {
	select( (thing_array&)followers );	// Do not use ch-specific select, may hear move.
	j = 0;
	for( int i = 0; i < followers; ++i ) {
	  char_data *follower = followers[i];
	  
	  if( is_set( follower->status, STAT_NOFOLLOW ) )
	    continue;
	  
	  action_data *action = actions[j];
	  const int type = types[j++];
	  
	  char_data *leader = following( follower );
	  
	  if( follower == rch ) {
	    leaving_self( follower, leader, exit, type, action );

	  } else if( detect_move( rch, follower ) ) {
	    if( !leader
		|| !follower->Seen( rch ) ) {
	      leaving_other( rch, follower, from_room, exit, type, action );

	    } else if( ( is_set( rch->status, STAT_LEADER )
			 || is_set( rch->status, STAT_FOLLOWER ) )
		       && !is_set( rch->pcdata->message, MSG_GROUP_MOVE ) ) {
	      // No message at all.

	    } else if( !detect_move( rch, leader )
		       || !leader->Seen( rch ) ) {
	      leaving_other( rch, follower, from_room, exit, type, action );

	    } else {
	      // Combine indistinguishable following mobs if possible.
	      if( follower->species ) {
		int g = j;
		for( int n = i+1; n < followers; ++n ) {
		  char_data *other = followers[n];
		  if( is_set( other->status, STAT_NOFOLLOW ) )
		    continue;
		  
		  const int otype = types[g++];
		  
		  // If rch sees other follow follower, can't combine follower any further.
		  
		  char_data *olead = following( other );

		  // Does rch see other follow anyone?
		  if( other == rch
		      || !detect_move( rch, other )
		      || !olead
		      || !other->Seen( rch )
		      || !detect_move( rch, olead )
		      || !olead->Seen( rch ) )
		    continue;

		  // If it's follower, we're through.
		  if( olead == follower ) {
		    break;
		  }

		  // Is other's follow message identical?
		  if( olead != leader
		      || other->species != follower->species
		      || !follower->mount && !other->mount && otype != type
		      || follower->mount && !other->mount
		      || !follower->mount && other->mount
		      || !look_same( rch, follower, other, true ) )
		    continue;

		  other->Show( other->Shown( ) + follower->Shown( ) );
		  follower->Show( 0 );
		  break;
		}
		if( !follower->Shown( ) )
		  continue;
	      }
	      follow_message( rch, follower, leader, true, type );
	    }
	  }
	}
      }
    }

    room_data *to_room = exit->to_room;
    bool covered = false;

    // Make tracks.
    // Improve movement skills.
    // Move everyone.
    const int back = dir_table[ door ].reverse;
    j = 0;
    for( int i = 0; i < followers; ++i ) {
      char_data *follower = followers[i];
      
      if( is_set( follower->status, STAT_NOFOLLOW ) ) {
	continue;
      }
    
      if( !cover )
	make_tracks( follower, from_room, door );
      
      const int type = types[j++];

      char_data *mover = follower->mount ? follower->mount : follower;
      
      if( is_set( mover->status, STAT_SNUCK ) ) {
	mover->improve_skill( SKILL_SNEAK );
      } else if( type == MOVE_SWIM ) {
	mover->improve_skill( SKILL_SWIMMING );
      }

      if( is_set( exit->exit_info, EX_REQUIRES_CLIMB ) ) {
	mover->improve_skill( SKILL_CLIMB );
      }

      if( cover && !covered && is_set( follower->status, STAT_COVER_TRACKS ) ) {
	if( count == 1 ) {
	  send( follower, "You cover your tracks.\n\r" );
	  fsend_seen( follower, "%s covers %s tracks.", follower, follower->His_Her( ) );
	} else {
	  send( follower, "You cover your group's tracks.\n\r" );
	  fsend_seen( follower, "%s covers %s group's tracks.", follower, follower->His_Her( ) );
	}
	covered = true;
	follower->improve_skill( SKILL_COVER_TRACKS );
      }

      remove_bit( follower->status, STAT_SENTINEL );
      follower->From( );
      follower->To( to_room );
      follower->room_position = back;
      
      if( follower->mount ) {
	remove_bit( follower->mount->status, STAT_SENTINEL );
	follower->mount->From( );
	follower->mount->To( to_room );
	follower->mount->room_position = back;
      }
    }

    // Arrival messages.
    action_data *action = arrival_action( to_room, back );
    //    actions.clear( count );
    // Print leaving messages.
    for( int k = 0; k < to_room->contents; ++k ) {
      char_data *rch = character( to_room->contents[k] );
      if( rch
	  && rch->link
	  && rch->position > POS_SLEEPING ) {
	select( (thing_array&)followers );	// Do not use ch-specific select, may hear move.
	j = 0;
	for( int i = 0; i < followers; ++i ) {
	  char_data *follower = followers[i];
	  
	  if( is_set( follower->status, STAT_NOFOLLOW ) )
	    continue;

	  const int type = types[j++];

	  if( follower == rch
	      || !detect_move( rch, follower ) )
	    continue;

	  char_data *leader = following( follower );

	  if( !follower->Seen( rch ) ) {
	    arriving_other( rch, follower, from_room, exit, type, action );

	  } else if( is_set( rch->status, STAT_LEADER )
		     || is_set( rch->status, STAT_FOLLOWER ) ) {
	    // No message at all.

	  } else if( !leader
		     || !detect_move( rch, leader )
		     || !leader->Seen( rch ) ) {
	    arriving_other( rch, follower, from_room, exit, type, action );

	  } else {
	    // Combine indistinguishable following mobs if possible.
	    if( follower->species ) {
	      int g = j;
	      for( int n = i+1; n < followers; ++n ) {
		char_data *other = followers[n];
		if( is_set( other->status, STAT_NOFOLLOW ) )
		  continue;
		
		const int otype = types[g++];
		
		// If rch sees other follow follower, can't combine follower any further.
		
		char_data *olead = following( other );
		
		// Does rch see other follow anyone?
		if( other == rch
		    || !detect_move( rch, other )
		    || !other->Seen( rch )
		    || !olead
		    || !detect_move( rch, olead )
		    || !olead->Seen( rch ) )
		  continue;
		
		// If it's follower, we're through.
		if( olead == follower ) {
		  break;
		}
		
		// Is other's follow message identical?
		if( olead != leader
		    || other->species != follower->species
		    || !follower->mount && !other->mount && otype != type
		    || follower->mount && !other->mount
		    || !follower->mount && other->mount
		    || !look_same( rch, follower, other, true ) )
		  continue;
		
		other->Show( other->Shown( ) + follower->Shown( ) );
		follower->Show( 0 );
		break;
	      }
	      if( !follower->Shown( ) )
		continue;
	    }
	    follow_message( rch, follower, leader, false, type );
	  }
	}
      }
    }

    // Sense Danger affect.
    // Show room.
    // Update combat.
    for( int i = 0; i < followers; ++i ) {
      char_data *follower = followers[i];
      
      if( is_set( follower->status, STAT_NOFOLLOW ) ) {
	continue;
      }
      
      for( int j = 0; j < to_room->exits; ++j ) {
	room_data *room = to_room->exits[j]->to_room;
	if( room != from_room ) {
	  for( int k = 0; k < room->contents; ++k ) {
	    char_data *rch = character( room->contents[k] );
	    if( rch
		&& rch->is_affected( AFF_SENSE_DANGER )
		&& get_trust( rch ) >= invis_level( follower ) ) {
	      fsend( rch, "You sense %s %s.", follower,
		     dir_table[ dir_table[ to_room->exits[j]->direction ].reverse ].where );
	    }
	  }
	}
      }

      send( follower, "\n\r" );
      show_room( follower, to_room, true, true );

      renter_combat( follower );
      update_aggression( follower );
    }
    
    // Entering triggers.
    //    j = 0;
    for( int i = 0; i < followers; ++i ) {
      char_data *follower = followers[i];
      
      if( is_set( follower->status, STAT_NOFOLLOW ) ) {
	continue;
      }
      
      trigger_entering( follower, to_room, back, action );	

      if( follower->Is_Valid( )
	  && follower->active.time == -1 ) {
	set_min_delay( follower, 5 );
      }
    }
  }

  // Clean up.
  for( int i = 0; i < followers; ++i ) {
    char_data *follower = followers[i];
    remove_bit( follower->status, STAT_FOLLOWER );
    remove_bit( follower->status, STAT_NOFOLLOW );
    remove_bit( follower->status, STAT_SNUCK );
  }

  remove_bit( ch->status, STAT_LEADER );

  return ( j != 0 );
}


/*
 *   DRUNK ROUTINE
 */


static const char *const drunk_message[] = {
  "You stumble and barely stay on your feet.",
  "$n stumbles and barely stays on $s feet.",

  "The ground moves quickly, sending you reeling.",
  "$n staggers, obviously intoxicated.",

  "Your legs give way to gravity.",
  "$n suddenly sits down and looks quite surprised.",

  "You stumble and fall to the ground.",
  "$n has had too much to drink and falls to the ground.",

  "You trip over your left foot and fall to the ground.",
  "The feet of $n walk in opposite directions and down $e falls."
};


static bool passes_drunk( char_data* ch )
{
  if( !ch->is_drunk( )
      || ch->mount
      || is_submerged( ch )
      || number_range( 1, 20 ) < 18 )
    return true;

  const int i = number_range( 0, 4 );

  act( ch, drunk_message[2*i], ch );
  act_notchar( drunk_message[2*i+1], ch );

  if( i == 0 )
    return true;
  
  rest( ch, false );
  //  ch->position = POS_RESTING;

  inflict( ch, 0, i-1, "falling down drunk", false );

  return false;
}


/*
 *   EXHAUSTION
 */


static bool is_exhausted( char_data *ch, char_data *leader, int& move, int type )
{
  if( type == MOVE_SWIM && !ch->species )
    move *= 12-ch->get_skill( SKILL_SWIMMING );
  
  if( ch->mount ) {
    if( ch->mount->move < move ) {
      send( ch, "Your mount is exhausted.\n\r" );
      act_leader( leader, "** %s's mount is too exhausted to follow you. **", ch );
      return true;
    }
  } else {
    if( ch->move < move ) {
      send( ch, "You are too exhausted.\n\r" );
      act_leader( leader, "** %s is too exhausted to follow you. **", ch );
      return true;
    }
  }

  return false;
}


/*
 *   TERRAIN FUNCTIONS
 */


/*
 *   ENTERING/LEAVING TRIGGERS
 */

bool trigger_leaving( char_data *ch, room_data *room,
		      exit_data *exit, int door,
		      action_data*& action )
{
  // Copy the array in case triggers alter room contents.
  thing_array stuff = room->contents;
  
  // Mob leave triggers.
  for( int i = 0; i < stuff; ++i ) {
    char_data *rch = mob( stuff[i] );
    if( !rch
	|| !rch->Is_Valid()
	|| rch == ch
	|| rch->position < POS_RESTING
	|| rch->pcdata
	|| rch->in_room != room )
      continue; 
    for( mprog_data *mprog = rch->species->mprog; mprog; mprog = mprog->next ) {
      if( mprog->trigger == MPROG_TRIGGER_LEAVING
	  && ( mprog->value == door || ( exit && mprog->value == -1 ) ) ) {
	clear_variables( );
        var_ch = ch;
        var_mob = rch;
        var_room = room;
	var_exit = exit;
        if( !mprog->execute( )
	    || !ch->Is_Valid()
	    || ch->in_room != room )
          return false;
      }
    }
  }
  
  // Object leave triggers.
  for( int i = 0; i < stuff; ++i ) {
    obj_data *obj = object( stuff[i] );
    if ( !obj
	 || !obj->Is_Valid()
	 || obj->array->where != room )
      continue;
    for( oprog_data *oprog = obj->pIndexData->oprog; oprog; oprog = oprog->next ) {
      if( oprog->trigger == OPROG_TRIGGER_LEAVING ) {
	clear_variables( );
	var_ch = ch;
	var_obj = obj;
	var_room = room;
	var_exit = exit;
	if( !oprog->execute( )
	    || !ch->Is_Valid()
	    || ch->in_room != room )
          return false;
	break;
      }
    }
  }

  bool result = true;

  for( action = room->action; action; action = action->next ) {
    if( action->trigger == TRIGGER_LEAVING 
	&& ( ( exit && is_set( action->flags, DIR_ANY ) )
	     || is_set( action->flags, door ) ) ) {
      clear_variables( );
      var_ch = ch;
      var_room = room;
      var_exit = exit;
      var_def = leaving_msg;
      var_def_type = -1;
      result = action->execute( );
      if( !ch->Is_Valid()
	  || ch->in_room != room ) 
        return false;
      if( result )
        return true;
    }
  }

  // If any acode didn't continue, prevent movement.
  return result;
}


bool trigger_entering( char_data *ch, room_data *room, int door,
		       action_data *action )
{
  exit_data *reverse = exit_direction( room, door );

  if( !action && !reverse ) {
    // For transfer() (DIR_TRANSFER), etc.
    action = arrival_action( room, door );
  }

  if( action ) {
    clear_variables( );
    var_ch = ch; 
    var_room = room;
    var_exit = reverse;
    var_def = entering_msg;
    var_def_type = -1;
    action->execute( );
    if( !ch->Is_Valid()
	|| ch->in_room != room )
      return false;
  }

  // Copy the array in case triggers alter room contents.
  thing_array stuff = room->contents;

  // Object enter triggers.
  for( int i = 0; i < stuff; ++i ) {
    obj_data *obj;
    if ( !( obj = object( stuff[i] ) )
	 || !obj->Is_Valid()
	 || obj->array->where != room )
      continue;
    for( oprog_data *oprog = obj->pIndexData->oprog; oprog; oprog = oprog->next ) {
      if( oprog->trigger == OPROG_TRIGGER_ENTERING ) {
	clear_variables( );
	var_ch = ch;
	var_obj = obj;
	var_room = room;
	var_exit = reverse;
	oprog->execute( );
	if( !ch->Is_Valid()
	    || ch->in_room != room )
          return false;
	break;
      }
    }
  }
  
  // Mob enter triggers.
  for( int i = 0; i < stuff; ++i ) {
    char_data *npc;
    if( !( npc = mob( stuff[i] ) )
	|| !npc->Is_Valid()
	|| npc == ch
	|| npc->position < POS_RESTING
	|| npc->pcdata
	|| npc->in_room != room ) 
      continue;
    for( mprog_data *mprog = npc->species->mprog; mprog; mprog = mprog->next ) {
      if( mprog->trigger == MPROG_TRIGGER_ENTRY
	  && ( mprog->value == door || mprog->value == -1 ) )  {
	clear_variables( );
        var_ch = ch;
        var_mob = npc;
        var_room = room;
	var_exit = reverse;
        mprog->execute( );
        if( !ch->Is_Valid()
	    || ch->in_room != room )
          return false;
      }
    }
  }

  // Mob to_room triggers.
  if( ch->species ) {
    for( mprog_data *mprog = ch->species->mprog; mprog; mprog = mprog->next ) {
      if( mprog->trigger == MPROG_TRIGGER_TO_ROOM ) {
	clear_variables( );
	var_mob = ch;
	var_room = room;
	var_exit = reverse;
	mprog->execute( );
	break;
      }
    }
  }

  return true;
}


/*
 *   LEADER MESSAGE ROUTINES
 */


static void act_leader( char_data *leader, const char *text, char_data *follower )
{
  //  if( !leader || is_set( follower->status, STAT_UNSEEN ) )
  if( !leader || !follower->Seen( leader ) )
    return;

  char buf [ MAX_INPUT_LENGTH ];
  snprintf( buf, MAX_INPUT_LENGTH, text, follower->Name( leader ) );
  buf[3] = toupper( buf[3] );
  fsend( leader, buf );
}


/*
 *   DO MOVE FUNCTIONS
 */


/*
static int move_dir( char_data *ch, int def )
{
  if( ch->pcdata
      && is_confused( ch )
      && number_range( 0, 5 ) == 0 ) {
    
    if( exit_data *exit = random_movable_exit( ch ) ) {
      return exit->direction;
    }
  }

  return def;
}
*/


void do_north( char_data* ch, const char * )
{
  move_char( ch, DIR_NORTH, false );
}


void do_east( char_data *ch, const char * )
{
  move_char( ch, DIR_EAST, false );
}


void do_south( char_data *ch, const char * )
{
  move_char( ch, DIR_SOUTH, false );
}


void do_west( char_data* ch, const char * )
{
  move_char( ch, DIR_WEST, false );
}


void do_up( char_data *ch, const char * )
{
  move_char( ch, DIR_UP, false );
}


void do_down( char_data *ch, const char * )
{
  move_char( ch, DIR_DOWN, false );
}


void do_search( char_data* ch, const char *argument )
{
  if( not_player( ch )
      || is_mounted( ch, "search" ) )
    return;
  
  const bool bit = is_set( ch->pcdata->pfile->flags, PLR_SEARCHING );

  if( toggle( ch, argument, "Searching", ch->pcdata->pfile->flags, PLR_SEARCHING ) ) {
    if( !bit && is_set( ch->pcdata->pfile->flags, PLR_SEARCHING ) )
      send( ch, "[Searching increases movement point costs.]\n\r" );
    return;
  }

  room_data *room = ch->in_room;

  if( !room->Seen( ch ) ) {
    send( ch, "You can't see a thing!\n\r" );
    return;
  }

  for( action_data *action = room->action; action; action = action->next )
    if( action->trigger == TRIGGER_SEARCHING 
	&& ( ( !*action->target && !*argument )
	     || ( *argument && is_name( argument, action->target ) ) ) ) {
      clear_variables( );
      var_ch = ch;
      var_room = room;
      if( !action->execute( )
	  || ch->in_room != room ) 
        return;
      break;
    }

  if( !*argument ) {
    if( ch->move < 2 ) {
      send( ch, "You are too tired to search.\n\r" );
      return;
    }
    ch->move -= 2;
    if( !search( ch ) ) {
      send( ch, "You rummage around but find nothing interesting.\n\r" );
      fsend_seen( ch, "%s rummages around but finds nothing interesting.", ch );
    }
  } else {
    send( ch, "Whatever that is, searching it results in nothing interesting.\n\r" ); 
  }
}

static const char *movement_name( int val )
{
  if( val < 0 )
    return "none";

  return movement_table[ val ].name;
}


static bool available( const char_data *ch, int i )
{
  return ch->Level() >= LEVEL_BUILDER
    || movement_table[i].player;
}


void do_movement( char_data *ch, const char *argument )
{
  if( is_mob( ch ) ) 
    return;

  player_data *pl = (player_data*)ch;

  if( !*argument ) {
    display_array( ch, "Movement Types",
		   &movement_table[0].name, &movement_table[1].name,
		   table_max[ TABLE_MOVEMENT ], true, available );
    page( ch, "\n\r[ Current setting: %s ]\n\r", movement_name( pl->movement ) );
    return;
  }

  if( !is_set( pl->pcdata->pfile->flags, PLR_APPROVED ) ) {
    send( ch, "You cannot set your movement type, since you have not been approved.\n\r" );
    return;
  }

  int i = -1;

  if( strcasecmp( argument, "none" ) ) {
    for( i = 0; i < table_max[ TABLE_MOVEMENT ]; ++i ) {
      if( available( ch, i )
	  && matches( argument, movement_table[i].name ) )
	break;
    }
    
    if( i == table_max[ TABLE_MOVEMENT ] ) {
      send( ch, "Unknown movement type.\n\r" );
      return;
    }
    
    /*
    if( ch->Level() < LEVEL_BUILDER
	&& !movement_table[i].player ) {
      fsend( ch, "Movement \"%s\" is not available to players.", movement_table[i].name );
      return;
    }
    */
  }

  pl->movement = i;

  fsend( ch, "Movement set to \"%s\".", movement_name( i ) );
}


/*
 *   SPEED WALKING
 */

void do_speed( char_data *ch, const char *argument )
{
  if( is_mob( ch ) ) 
    return;

  if( !*argument ) {
    send( ch, "Syntax: speed <directions>\n\r" );
    return;
  }

  const char *const dirs = "neswud 0123456789";
  const char *const cmds [] = {
    "north", "east", "south", "west", "up", "down"
  };

  int n = 1;

  if( isdigit( *argument ) ) {
    n = 0;
    while( isdigit( *argument ) ) {
      n = 10*n+(*argument++)-'0';
    }
    if( n <= 0 || n > 20 ) {
      send( ch, "Repeat count can be from 1 to 20.\n\r" );
      return;
    }
  }

  if( !isalpha( *argument ) || argument[ strspn( argument, dirs ) ] ) {
    send( ch, "Valid speed-walking directions are n, e, s, w, u, and d.\n\r" );
    return;
  }
  
  if( cmd_room && cmd_room == ch->in_room ) {
    send( ch, "Speed walking aborted.\n\r" );
    return;
  }

  const char *const cmd = cmds[ strchr( dirs, *argument ) - dirs ];

  if( --n == 0 ) {
    ++argument;
    skip_spaces( argument );
  }

  // Put a command to do the rest of the speed-walking back into the queue.
  if( *argument ) {
    char tmp [ MAX_INPUT_LENGTH ];
    if( n > 1 ) {
      snprintf( tmp, MAX_INPUT_LENGTH, "speed %d%s", n, argument );
    } else {
      snprintf( tmp, MAX_INPUT_LENGTH, "speed %s", argument );
    }
    command_data *command = ch->cmd_queue.shift( tmp, false );
    if( n == 0 )
      command->room = ch->in_room;
  }

  // Put the direction command into the queue.
  ch->cmd_queue.shift( cmd, false );

  set_delay( ch, 0 );
}
