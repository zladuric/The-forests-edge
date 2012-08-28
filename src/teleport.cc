#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include "define.h"
#include "struct.h"


/*      
 *   ASTRAL GATE
 */


static void make_gate( char_data *ch, room_data* from, room_data* to, int life )
{
  obj_data *gate = create( get_obj_index( OBJ_ASTRAL_GATE ) );
  gate->value[0] = life;
  gate->value[1] = to->vnum;
  gate->value[3] = (int) ch;	// *** FIX ME!
  gate->To( from );

  const char *surface = from->surface( );

  if( *surface ) {
    fsend( from->contents, "%s slowly rises from the %s.", gate,
	   surface );
  } else {
    fsend( from->contents, "%s slowly rises.", gate );
  }
}


bool spell_astral_gate( char_data* ch, char_data*, void* vo, int level, int )
{
  if( null_caster( ch, SPELL_ASTRAL_GATE ) )
    return false;

  room_data *room = (room_data*) vo;

  if( !room ) {
    send( ch, "Nothing happens.\n\r" );
    send_seen( ch, "Nothing happens.\n\r" );
    return false;
  }

  if( room == ch->in_room
      || is_set( ch->in_room->room_flags, RFLAG_NO_RECALL ) ) {
    send( ch, "Nothing happens.\n\r" );
    send_seen( ch, "Nothing happens.\n\r" );
    return false;
  }

  //  int life = 50 + ( ch->get_skill( SPELL_ASTRAL_GATE ) * number_range( 10, 20 ) );
  int life = number_range( 1+level/2, level );

  make_gate( ch, room, ch->in_room, life );
  make_gate( ch, ch->in_room, room, life );

  return true;
}


/*      
 *   BLINK
 */


bool spell_blink( char_data* ch, char_data* victim, void*, int level, int )
{
  room_data *room;

  if( !( room = Room( victim->array->where ) ) ) {
    if( ch )
      send( ch, "Blink only works in a room.\n\r" );
    return false;
  } 
  
  if( is_set( room->room_flags, RFLAG_NO_RECALL ) ) {
    send( victim, "Nothing happens.\n\r" );
    return false;
  }

  for( int i = 0; i < level; ++i ) {
    exit_data *exit = random_movable_exit( victim, room, false, true );
    if( !exit )
      break;
    if( victim->array->where == exit->to_room )
      continue;
    room = exit->to_room;
  }

  if( room == victim->array->where ) {
    send( victim, "You are trapped!\n\r" );
    if( ch )
      send( ch, "Nothing happens.\n\r" );
    return false;
  }

  if( victim->mount ) {
    send( victim->mount, "\n\rYou disappear and suddenly find yourself elsewhere.\n\r\n\r" );
    fsend_seen( victim, "%s, riding %s,  vanishes in the blink of an eye!", victim, victim->mount );
    victim->mount->From( );
    victim->mount->To( room ); 
  } else {
    fsend_seen( victim, "%s vanishes in the blink of an eye!", victim );
  }

  send( victim, "\n\rYou disappear and suddenly find yourself elsewhere.\n\r\n\r" );

  victim->From( );
  victim->To( room );

  if( !victim->was_in_room ) {
    if( victim->mount ) {
      fsend( *victim->array, "%s, riding %s, suddenly appears in a flash of white light!",
	     victim, victim->mount );
      show_room( victim->mount, room, false, false );
    } else {
      fsend( *victim->array, "%s suddenly appears in a flash of white light!", victim );
    }
    show_room( victim, room, false, false );
  }

  return true;
}


/*
 *   PASSDOOR
 */


/*
bool spell_pass_door( char_data* ch, char_data* victim, void*,
		      int level, int duration )
{
  return spell_affect( ch, victim, level, duration,
		       SPELL_PASS_DOOR, AFF_PASS_DOOR );
}
*/


/*
 *   RECALL
 */


bool spell_recall( char_data* ch, char_data* victim, void* vo, int, int )
{
  if( !victim )
    victim = ch;

  if( is_set( victim->in_room->room_flags, RFLAG_NO_RECALL ) ) {
    send( ch, "Nothing happens.\n\r" );
    return false;
  }
  
  if( !consenting( victim, ch, "recalling" ) )
    return false;

  room_data *room = Room( (thing_data*) vo );

  if( !room ) {
    if( victim->species ) {
      if( !is_set( victim->status, STAT_PET )
	  || !victim->leader ) {
	send( ch, "Nothing happens.\n\r" );
	return true;
      }
      room = get_temple( victim->leader );
    } else
      room = get_temple( victim );
  }
  
  if( victim->mount ) {
    send( victim->mount, "\n\r** You feel yourself pulled to another location. **\n\r\n\r" );
    fsend_seen( victim->mount, "%s, riding %s, disappears in a flash of light.", victim, victim->mount );
    victim->mount->From( );
    victim->mount->To( room ); 
  } else {
    fsend( *victim->array, "%s disappears in a flash of light.", victim );
  }

  send( victim,
	"\n\r** You feel yourself pulled to another location. **\n\r\n\r" );

  victim->From( );
  victim->To( room );

  if( !victim->was_in_room ) {
    if( victim->mount ) {
      fsend( *victim->array, "%s, riding %s, appears in a flash of light.",
	     victim, victim->mount );
      show_room( victim->mount, room, false, false );
    } else {
      fsend( *victim->array, "%s appears in a flash of light.", victim );
    }
    show_room( victim, room, false, false );
  }

  return true;
}


/*
 *   SUMMON
 */


bool spell_summon( char_data* ch, char_data* victim, void*, int, int )
{
  if( null_caster( ch, SPELL_SUMMON ) )
    return false;
 
  if( !victim
      || victim->in_room == ch->in_room ) {
    send( ch, "Nothing happens.\n\r" );
    send_seen( ch, "Nothing happens.\n\r" );
    return false;
  }

  if( privileged( victim, LEVEL_BUILDER )
      || victim->species && ( !is_set( victim->status, STAT_PET ) || victim->leader != ch )
      || is_set( victim->in_room->room_flags, RFLAG_NO_RECALL )
      || victim->Size( ) > ch->in_room->size
      || ( victim->mount
	   && victim->mount->in_room == victim->in_room
	   && victim->mount->Size( ) > ch->in_room->size ) ) {
    // Rider has to be smaller than than mount.
    //      || ( victim->rider
    //	   && victim->rider->in_room == victim->in_room
    //	   && victim->rider->Size( ) > ch->in_room->size ) ) {
    send( ch, "You fail to summon them.\n\r" );
    send( victim,
	  "The world comes back to focus as the summoning fails.\n\r" );
    return false;
  }
  
  if( victim->pcdata
      && is_set( victim->pcdata->pfile->flags, PLR_NO_SUMMON ) ) {
    fsend( ch, "%s does not wish to be summoned.", victim );
    send( victim,
	  "The world comes back to focus as the summoning fails.\n\r" );
    return false;
  }

  /*
  if( victim->rider
      && victim->rider->pcdata
      && is_set( victim->rider->pcdata->pfile->flags, PLR_NO_SUMMON ) ) {
    fsend( ch, "%s's rider does not wish to be summoned.", victim );
    send( victim,
	  "The world comes back to focus as the summoning fails.\n\r" );
    return false;
  }
  */

  if( !consenting( victim, ch, "summoning" ) ) 
    return false;

  /* MAKE LIST */

  char_array   list;
  char_data*     rch;

  list += victim;

  // Deal with someone mounted on your pet.
  if( victim->rider )
    list += victim->rider;
  if( victim->mount )
    list += victim->mount;

  for( int i = 0; i < victim->in_room->contents; ++i ) {
    if( ( rch = character( victim->in_room->contents[i] ) )
	&& rch->leader == victim
	&& rch->species
	&& rch->Size( ) <= ch->in_room->size ) {
      list += rch;
    }
  }
  
  /* TRANSFER CHARACTERS */

  if( !victim->was_in_room ) {
    thing_array *things = (thing_array *)&list;
    for( int i = 0; i < victim->in_room->contents; ++i ) {
      if( ( rch = character( victim->in_room->contents[i] ) )
	  && rch->pcdata
	  && !list.includes( rch ) ) {
	select( *things, rch );
	rehash( rch, *things );
	if( !none_shown( *things ) ) {
	  fsend( rch, "%s slowly fade%s out of existence.",
		 (thing_array*)&list, one_shown( *things ) ? "s" : "" );
	}
      }
    }
  }

  for( int i = 0; i < list; ++i ) {
    list[i]->From( );
    list[i]->To( ch->in_room );
  }
  
  for( int i = 0; i < list; ++i ) {
    if( list[i]->pcdata ) {
      send( list[i],
	    "\n\r** You feel yourself pulled to another location. **\n\r\n\r" );
      show_room( list[i], list[i]->in_room, false, false );
    }
  }

  if( !victim->was_in_room ) {
    thing_array *things = (thing_array *)&list;
    for( int i = 0; i < victim->in_room->contents; ++i ) {
      if( ( rch = character( victim->in_room->contents[i] ) )
	  && rch->pcdata
	  && !list.includes( rch ) ) {
	select( *things, rch );
	rehash( rch, *things );
	if( !none_shown( *things ) ) {
	  fsend( rch, "%s slowly materialize%s.",
		 (thing_array*)&list, one_shown( *things ) ? "s" : "" );
	}
      }
    }
  }
  
  return true;
}


/*
 *   TRANSFER
 */


bool spell_transfer( char_data* ch, char_data *victim, void*, int, int )
{
  if( null_caster( ch, SPELL_TRANSFER ) )
    return false;
 
  if( !victim ) {
    send( ch, "Nothing happens.\n\r" );
    send_seen( ch, "Nothing happens.\n\r" );
    return false;
  }

  return true;
}
