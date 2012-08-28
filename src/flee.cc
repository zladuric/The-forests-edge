#include <ctype.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include "define.h"
#include "struct.h"


static const char* wimpy_info [] = {
  "\
  ------------------------------\n\r\
  \\                             \\\n\r\
  /      -- Wimpy Values --     /\n\r\
  \\                             \\\n\r",
  "  %c   %6s : %3d%%  %-9s  %c\n\r",
  "  /                             /\n\r\
  ------------------------------\n\r"
};


static const char *wimpy_word [] = { "flee", "blink", "pray", "recall" };


void do_wimpy( char_data* ch, const char *argument )
{
  if( is_mob( ch ) )
    return;

  int *value = unpack_int( ch->pcdata->wimpy );

  /*
  // Fix up old wimpy settings, from when it was an absolute number of hp
  // instead of a percentage.
  for( int i = 0; i < 4; ++i ) {
    if( value[i] > 100 ) {
      value[i] = 100;
      ch->pcdata->wimpy = pack_int( value );
    }
  }
  */

  if( !*argument ) {
    send( ch, wimpy_info[0] );
    char buf [11];
    for( int i = 0; i < 4; ++i ) {
      if( value[i] > 0 ) {
	snprintf( buf, 11, "(%d hp)", value[i]*ch->max_hit/100 );
      } else {
	snprintf( buf, 11, "(off)" );
      }
      const char c = (i%2) ? '\\' : '/';
      send( ch, wimpy_info[1], c, wimpy_word[i], value[i], buf, c );
    }
    send( ch, wimpy_info[2] );
    return;
  }

  int num;

  for( int i = 0; i < 4; ++i ) {
    if( matches( argument, wimpy_word[i] ) ) {
      if( !*argument ) {
	send( ch, "Your wimpy setting for %s is %d%%.\n\r", 
	      wimpy_word[i], value[i] );
	send( ch, "This currently equals %d hit points.\n\r",
	      value[i]*ch->max_hit/100 );
	return;
      }
      if( ( num = atoi( argument ) ) < 0 || num > 100 ) {
        send( ch, "You can only set wimpy values from 0 to 100 percent.\n\r" );
        return;
      }
      value[i] = num;
      ch->pcdata->wimpy = pack_int( value );
      send( ch, "Your wimpy setting for %s set to %d%%.\n\r", 
	    wimpy_word[i], num );
      if( num == 0 ) {
	send( ch, "This disables wimpy %s.\n\r", wimpy_word[i] );
      } else {
	send( ch, "This currently equals %d hit points.\n\r",
	      num*ch->max_hit/100 );
      }
      /*
      if( i == 2 && num > 25 ) {
	fsend( ch,
	       "Warning! Wimpy pray settings greater than 25% will not help you in combat!" );
      }
      */
      return;
    }
  }

  send( ch, "Syntax: wimpy <field> <value>\n\r" );
}


/*
 *   AUTO-FLEE HANDLER
 */


static obj_data *has_recall( char_data* ch )
{
  const int snum = skill_number( SPELL_RECALL );

  for( int i = 0; i < ch->contents; ++i ) {
    obj_data *obj;
    if( ( obj = object( ch->contents[i] ) )
	//	&& obj->pIndexData->vnum == OBJ_RECALL
	&& obj->pIndexData->item_type == ITEM_SCROLL
	&& obj->value[0] == snum
	&& is_set( obj->extra_flags, OFLAG_IDENTIFIED ) )
      return obj;
  }
  
  return 0;
}


void check_wimpy( char_data* ch )
{
  if( !opponent2( ch ) ) 
    return;
  
  if( ch->species ) {
    if( ch->hit < ch->max_hit/4
	&& is_set( ch->species->act_flags, ACT_WIMPY )
	&& number_range( 0,5 ) == 0 ) 
      attempt_flee( ch );
    return;
  }
  
  if( is_berserk( ch )
      || !is_set( ch->status, STAT_WIMPY ) ) 
    return;
  
  const int *const value = unpack_int( ch->pcdata->wimpy );
  cast_data *prepare;
  obj_data *obj;

  if( value[3] > 0
      && ch->hit <= value[3]*ch->max_hit/100
      && !is_set( ch->in_room->room_flags, RFLAG_NO_RECALL )
      && !is_set( ch->in_room->room_flags, RFLAG_NO_MAGIC )
      && ( obj = has_recall( ch ) ) ) {
    recite( ch, empty_string, obj ); 
  } else if( value[2] > 0
	     && ch->hit <= value[2]*ch->max_hit/100
	     && ch->hit < ( ch->fighting ? ch->max_hit/4 : ch->max_hit/3 )
	     && !is_set( ch->in_room->room_flags, RFLAG_NO_PRAY ) ) {
    do_pray( ch, "" );
    // Don't pray again until you lose hp.
    remove_bit( ch->status, STAT_WIMPY );
  } else if( value[1] > 0
	     && ch->hit <= value[1]*ch->max_hit/100
	     && ch->get_skill( SPELL_BLINK ) != UNLEARNT
	     && ( prepare = has_prepared( ch, SPELL_BLINK-SPELL_FIRST ) )
	     && !is_set( ch->in_room->room_flags, RFLAG_NO_MAGIC ) ) {
    do_cast( ch, "blink" );
  } else if( value[0] > 0
	     && ch->hit <= value[0]*ch->max_hit/100 ) {
    attempt_flee( ch );
  }
}


/*
 *   FLEE ROUTINE
 */


void do_flee( char_data* ch, const char *argument )
{
  room_data *room;
  if( !( room = Room( ch->array->where ) ) ) {
    send( ch, "You aren't in a room, which is confusing.\n\r" );
    return;
  }
  
  if( ch->rider
      && is_set( ch->status, STAT_ORDERED ) ) {
    return;
  }

  if( !opponent2( ch ) ) {
    send( ch, "You cannot flee unless you are involved in a battle.\n\r" );
    return;
  }

  if( is_berserk( ch ) ) {
    send( ch,
	  "You can never flee while the battle lust rages within you!\n\r" );
    return;
  }
  
  if( is_entangled( ch, "flee" ) ) {
    return;
  }
  
  exit_data *exit = 0;
  if( *argument
      && ch->in_room->Seen( ch ) ) {
    if( !( exit = (exit_data*) one_visible( ch,
					    argument,
					    "flee",
					    (visible_array*) &room->exits ) ) )
      return;
    
    if( is_set( exit->exit_info, EX_NO_FLEE ) ) {
      fsend( ch, "You cannot flee %s.",
	     dir_table[exit->direction].name );
      return;
    }

    int move, type;
    if( !ch->Can_Move( 0, ch->in_room, exit, exit->direction, move, type, true ) ) {
      //      fsend( ch, "You can't flee %s.", dir_table[ exit->direction ].name );
      return;
    }
  }
  
  attempt_flee( ch, exit );
}


bool attempt_flee( char_data* ch, exit_data* exit )
{
  disrupt_spell( ch );
  set_delay( ch, 32 );

  if( !exit
      && !( exit = random_movable_exit( ch, 0, false, false, true ) ) ) {
    fsend( ch, "You attempt to flee, but find no suitable exit." );
    return false;
  }
  
  char_data *rch = opponent3( ch );
  const int skill = ch->get_skill( SKILL_ESCAPE );

  if( rch && number_range( 0, 100 ) < 15+20*ch->get_burden()-skill ) {
    send( ch, "You attempt to flee, but fail to escape the battle!\n\r" );
    fsend_seen( ch, "%s attempts to flee, but %s fails to escape the battle!",
		ch, ch->He_She() );
    return true;
  }

  Content_Array *was_in = ch->array;
  move_char( ch, exit->direction, true );

  if( !ch->Is_Valid( ) || ch->hit <= 0 )
    return true;
  
  if( ch->array == was_in ) {
    send( ch,
	  "You flee but for some reason end up where you started!\n\r" );
    return true;
  }

  if( !ch->species ) {
    int exp = (ch->Level()*ch->hit)/2;
    if( ch->Level() == 1 )
      exp = min( ch->exp, exp );
    if( exp > 0 ) 
      add_exp( ch, -exp, "You lose %d exp for fleeing.\n\r" );
  }

  /*
  if( ch->species ) {
    stop_events( ch, execute_path );
    stop_events( ch, execute_wander );
    delay_wander( new event_data( execute_wander, ch ) );
  }
  */
  
  const int skill1 = ch->get_skill( SKILL_HIDE );
  
  if( skill != UNLEARNT
      && skill1 != UNLEARNT
      && number_range( 1, 100 ) < 4*skill ) {
    const int skill2 = ch->get_skill( SKILL_CAMOUFLAGE );
    if( skill2 != UNLEARNT && !is_set( ch->status, STAT_CAMOUFLAGED ) && number_range( 0, 1 ) == 1 ) {
      do_camouflage( ch, "" );
    } else if( skill1 != UNLEARNT && !is_set( ch->status, STAT_HIDING ) ) {
      do_hide( ch, "" );
    }
  }

  ch->improve_skill( SKILL_ESCAPE );

  if( rch
      && !rch->pcdata
      && !is_set( rch->status, STAT_PET )
      && is_set( rch->species->act_flags, ACT_FOLLOW_FLEE )
      && ch->Seen( rch )
      && rch->hit > rch->max_hit / 2
      && number_range( 0, 1 ) == 0
      && ( !skill || number_range( 1, 100 ) > 4*skill ) ) {
    // Try to follow fled opponent.
    move_char( rch, exit->direction, false );
  }
  
  if( ch->species
      && ( !rch || rch->array == was_in ) ) {
    if( event_data *event = find_event( ch, execute_path ) ) {
      if( exit_data *rev = reverse( exit ) ) {
	// Try to find a path that doesn't include our enemy's room.
	retry_path( event, rev );
      }
    }
  }

  //  ch->cmd_queue.clear();

  return true;
}
