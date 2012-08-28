#include <ctype.h>
#include <stdlib.h>
#include <sys/types.h>
#include <stdio.h>
#include <syslog.h>
#include "define.h"
#include "struct.h"


char_data*  list_followers   ( char_data* );


/*
 *   FOLLOWER ROUTINES
 */


char_data *group_leader( char_data* ch )
{
  if( !is_set( ch->status, STAT_IN_GROUP ) )
    return 0;
  
  for( ; ch->leader; ch = ch->leader );
  
  return ch;
}


static void degroup( char_data *ch )
{
  if( !is_set( ch->status, STAT_IN_GROUP ) )
    return;

  remove_bit( ch->status, STAT_IN_GROUP );

  for( int i = 0; i < ch->followers; ++i ) {
    degroup( ch->followers[i] );
  }
}


void do_follow( char_data *ch, const char *argument )
{
  if( !*argument ) {
    if( !ch->leader ) {
      send( ch, "You are not currently following anyone.\n\r" );
    } else {
      fsend( ch, "You are currently following %s.", ch->leader->Seen_Name( ch ) );
    }
    return;
  }

  char_data *victim;

  if( !( victim = one_character( ch, argument, "follow", ch->array ) ) )
    return;

  if( is_set( ch->status, STAT_PET ) ) {
    interpret( ch, "sulk" );
    interpret( ch, "shake" );
    return;
  }

  if( victim == ch ) {
    if( !ch->leader ) {
      send( ch, "You already follow yourself.\n\r" );
      return;
    }
    stop_follower( ch );
    return;
  }
  
  if( victim == ch->fighting || victim->fighting == ch ) {
    send( ch, "You cannot follow your opponent!\n\r" );
    return;
  }

  const char *msg = associate( ch, victim, "follow" );

  if( !msg )
    return;
  
  if( ch->leader )
    stop_follower( ch );
  
  add_follower( ch, victim, msg );
}


void add_follower( char_data* ch, char_data* victim, const char *msg )
{
  if( ch->leader ) {
    bug( "Add_follower: non-null leader." );
    return;
  }
  
  for( char_data *leader = victim->leader; leader; leader = leader->leader )
    if( leader == ch ) {
      send( ch, "No loops in follow allowed.\n\r" );
      return;
    }

  if( is_set( ch->status, STAT_PET ) ) {
    set_owner( ch->contents, victim, ch );
    set_owner( ch->wearing, victim, ch );
  }

  ch->leader = victim;
  victim->followers += ch;

  degroup( ch );

  if( msg ) {
    if( ch->Seen( victim ) )
      fsend( victim, "%s now follows you.", ch );
    if( !*msg )
      fsend( ch, "You now follow %s.", victim->Seen_Name( ch ) );
    else
      fsend( ch, "You %s follow %s.", msg, victim->Seen_Name( ch ) );
    fsend_seen( ch, "%s starts following %s.", ch, victim );
  }
}


void stop_follower( char_data *ch )
{
  if( !ch->leader ) {
    bug( "Stop_follower: null leader." );
    bug( "-- Ch = %s", ch->descr->name );
    return;
  }
  
  if( ch->Seen( ch->leader ) )
    fsend( ch->leader, "%s stops following you.", ch );
  
  fsend( ch, "You stop following %s.", ch->leader );
  
  if( is_set( ch->status, STAT_PET ) ) {
    remove_bit( ch->status, STAT_PET );
    remove_bit( ch->status, STAT_TAMED );
    remove_bit( ch->status, STAT_FAMILIAR );
    free_string( ((mob_data*)ch)->pet_name, MEM_MOBS );
    ((mob_data*)ch)->pet_name = empty_string; 
    if( player_data *pc = player( ch->leader ) ) {
      if( pc->familiar == ch ) 
	pc->familiar = 0;
      set_owner( ch->contents, 0, pc );
      set_owner( ch->wearing, 0, pc );
      player_log( pc, "%s [PET] stopped following.", ch->Name() );
    }
  }
  
  remove_bit( ch->status, STAT_IN_GROUP );
  ch->leader->followers -= ch; 
  ch->leader = 0;
}


/*
 *   GROUP ROUTINES
 */


void add_group( char_data *ch, char_data *victim )
{
  if( victim == ch ) {
    send( ch, "You add yourself to your group.\n\r" );
    fsend_seen( ch, "%s joins %s own group.",
    		ch, ch->His_Her( ) );
    set_bit( ch->status, STAT_IN_GROUP );
    remove_bit( ch->status, STAT_GROUP_LOOTER );
    return;
  }

  if( !is_set( ch->status, STAT_IN_GROUP ) ) {
    send( ch, "You need to group yourself first.\n\r" );
    return;
  }

  victim->Show( 1 );

  fsend( victim, "You join %s's group.", ch->Seen_Name( victim ) );
  fsend( ch, "%s joins your group.", victim->Seen_Name( ch ) );
  fsend_seen( victim, "%s joins %s's group.", victim, ch );

  set_bit( victim->status, STAT_IN_GROUP ); 
  remove_bit( victim->status, STAT_GROUP_LOOTER );
}


static void group_all( char_data *ch )
{
  char_data *rch;
  bool found  = false;

  if( !is_set( ch->status, STAT_IN_GROUP ) ) {
    add_group( ch, ch );
    found = true;
  }
  
  for( int i = 0; i < *ch->array; i++ ) 
    if( ( rch = character( ch->array->list[i] ) )
	&& rch->leader == ch
	&& !is_set( rch->status, STAT_IN_GROUP )
	&& rch->Seen( ch ) ) {
      add_group( ch, rch );
      found = true;
    }

  if( !found ) 
    send( ch, "You and anyone following you are already grouped.\n\r" );
}   


static void group_disband( char_data *ch )
{
  if( !is_set( ch->status, STAT_IN_GROUP ) ) {
    send( ch, "You are not in a group.\n\r" );
    return;
  }

  if( ch->leader && is_set( ch->leader->status, STAT_IN_GROUP ) ) {
    send( ch, "You are not the group leader.\n\r" );
    return;
  }

  bool found = false;
  for( int i = 0; i < ch->followers; ) {
    char_data *rch = ch->followers[i];
    if( is_set( rch->status, STAT_IN_GROUP ) ) {
      found = true;
      fsend( rch, "%s disbands the group.", ch->Seen_Name( rch ) );
      if( !is_set( rch->status, STAT_PET ) ) {
	fsend( rch, "You stop following %s.", ch->Seen_Name( rch ) );
	ch->followers -= rch; 
	rch->leader = 0;
      } else {
	++i;
      }
      remove_bit( rch->status, STAT_IN_GROUP );
    } else {
      ++i;
    }
  }

  if( found ) {
    send( ch, "You disband your group.\n\r" );
  } else {
    send( ch, "You try to disband yourself, but fail miserably.\n\r" );
  }
}


static void add_followers( char_data *ch, char_array& array )
{
  if( array.includes( ch ) )
    return;

  array.append( ch );
  
  for( int i = 0; i < ch->followers; ++i ) {
    char_data *follower = ch->followers[i];
    add_followers( follower, array );
  }
}


static void display_group( char_data* ch )
{
  if( !is_set( ch->status, STAT_IN_GROUP ) ) {
    send( ch, "You are not in any group.\n\r" );
    return;
  }

  char               tmp  [ TWO_LINES ];
  char                hp  [ 15 ];
  char              move  [ 15 ];
  char            energy  [ 15 ];
  char_data*         gch;
  char_data*      leader;
  char_array   incognito;

  for( leader = ch; leader->leader; leader = leader->leader );

  send( ch, "Leader: %s\n\r", leader->Seen_Name( ch ) );
  send_underlined( ch,
    "                                          Hits    Energy     Moves\
          Exp\n\r" );

  char_array array;
  add_followers( leader, array );

  for( int i = 0; i < array; ++i ) {
    gch = array[i];

    if( !is_set( gch->status, STAT_IN_GROUP ) )
      continue; 

    if( gch != ch
	&& gch->pcdata
	&& is_set( gch->pcdata->pfile->flags, PLR_GROUP_INCOG ) ) {
      incognito += gch;
      continue;
    }

    snprintf( hp, 15, "%d/%d", gch->hit,  gch->max_hit );
    snprintf( energy, 15, "%d/%d", gch->mana, gch->max_mana );
    snprintf( move, 15, "%d/%d", gch->move, gch->max_move );

    snprintf( tmp, TWO_LINES,
	     "[ %2d %3s %3s %s ] %-19s %9s %9s %9s %12d\n\r",
	      gch->Level(), gch->species
	      ? "Mob" : clss_table[ gch->pcdata->clss ].abbrev,
	      race_table[ gch->shdata->race ].abbrev,
	      is_set( gch->status, STAT_GROUP_LOOTER ) ? "L" : " ",
	      trunc( gch->Seen_Name( ch ), 19 ).c_str( ),
	      hp, energy, move,
	      gch->pcdata ? exp_for_level( gch )-gch->exp : 0 );
    tmp[15] = toupper( tmp[15] );
    send( ch, tmp );
  }
  
  //  bool             found  = false;

  for( int i = 0; i < incognito; ++i ) {
    /*
    if( !found ) {
      found = true;
      send_centered( ch, "-*-" );
    }
    */
    send( ch, "[  Incognito %s ] %-31s %s\n\r",
	  is_set( gch->status, STAT_GROUP_LOOTER ) ? "L" : " ",
	  trunc( incognito[i]->Seen_Name( ch ), 31 ).c_str( ),
	  condition_word( incognito[i] ) );
  }
}


void do_group( char_data* ch, const char *argument )
{
  if( !*argument ) {
    display_group( ch );
    return;
  }

  int flags;
  if( !get_flags( ch, argument, &flags, "l", "group" ) ) {
    return;
  }

  if( is_set( flags, 0 ) ) {
    if( !is_set( ch->status, STAT_IN_GROUP ) ) {
      send( ch, "You are not in any group.\n\r" );
      return;
    }
    if( !*argument ) {
      if( is_set( ch->status, STAT_GROUP_LOOTER ) ) {
	send( ch, "You are the group looter.\n\r" );
      } else {
	char_data *leader = group_leader( ch );
	if( is_set( leader->status, STAT_IN_GROUP )
	    && is_set( leader->status, STAT_GROUP_LOOTER ) ) {
	  fsend( ch, "%s is the group looter.",
		 leader->Seen_Name( ch ) );
	  return;
	} else {
	  for( int i = 0; i < leader->followers; ++i ) {
	    char_data *rch = leader->followers[i];
	    if( is_set( rch->status, STAT_IN_GROUP )
		&& is_set( rch->status, STAT_GROUP_LOOTER ) ) {
	      fsend( ch, "%s is the group looter.",
		     rch->Seen_Name( ch ) );
	      return;
	    }
	  }
	}
	send( ch, "No group looter has been set.\n\r" );
      }
      return;
    }

    if( ch->leader ) {
      send( ch, "Only the group leader can set a group looter.\n\r" );
      return;
    }

    char_data *looter;

    if( exact_match( argument, "none" ) ) {
      looter = 0;
    } else {
      looter = one_player( ch, argument, "set as group looter", ch->array );
      if( !looter )
	return;
      if( group_leader( looter ) != ch ) {
	fsend( ch, "%s is not in your group.", looter->Seen_Name( ch ) );
	return;
      }
    }

    if( is_set( ch->status, STAT_IN_GROUP )
	&& is_set( ch->status, STAT_GROUP_LOOTER ) ) {
      if( ch == looter ) {
	send( ch, "You are already the group looter.\n\r" );
	return;
      }
      remove_bit( ch->status, STAT_GROUP_LOOTER );
      send( ch, "You remove yourself as group looter.\n\r" );
      if( !looter )
	return;

    } else {
      for( int i = 0; i < ch->followers; ++i ) {
	char_data *rch = ch->followers[i];
	if( is_set( rch->status, STAT_IN_GROUP )
	    && is_set( rch->status, STAT_GROUP_LOOTER ) ) {
	  if( rch == looter ) {
	    fsend( ch, "%s is slready the group looter.", rch->Seen_Name( ch ) );
	    return;
	  }
	  remove_bit( rch->status, STAT_GROUP_LOOTER );
	  fsend( ch, "You remove %s as group looter.", rch->Seen_Name( ch ) );
	  fsend( rch, "%s has removed you as group looter.", ch->Seen_Name( rch ) );
	  if( !looter )
	    return;
	  break;
	}
      }
    }

    if( !looter ) {
      send( ch, "No group looter has been set.\n\r" );
      return;
    }

    set_bit( looter->status, STAT_GROUP_LOOTER );

    if( looter == ch ) {
      send( ch, "You set yourself as group looter.\n\r" );
    } else {
      fsend( ch, "You set %s as group looter.", looter->Seen_Name( ch ) );
      fsend( looter, "%s has set you as group looter.", ch->Seen_Name( looter ) );
    }

    return;
  }

  if( ch->leader ) {
    send( ch, "But you are following someone else!\n\r" );
    return;
  }
  
  if( !strcasecmp( argument, "all" ) ) {
    group_all( ch );
    return;
  }

  if( !strcasecmp( argument, "disband" ) ) {
    group_disband( ch );
    return;
  }

  char_data *victim = one_character( ch, argument, empty_string, ch->array );
  
  if( victim
      && !is_set( victim->status, STAT_IN_GROUP ) 
      && ( victim == ch || victim->leader == ch ) ) {
    add_group( ch, victim );  
    return;
  }
  
  if( victim != ch ) 
    victim = one_character( ch, argument, empty_string,
			    (thing_array*)&ch->followers, 0, 0, true );
  
  if( victim
      && is_set( victim->status, STAT_IN_GROUP ) ) {
    if( victim != ch ) {
      fsend( ch, "You remove %s from your group.", victim->Seen_Name( ch ) );
      fsend( victim, "%s removes you from %s group.",
	     ch->Seen_Name( victim ), ch->His_Her( ) );
      for( int i = 0; i < ch->followers; ++i ) {
	char_data *follower = ch->followers[i];
	if( follower != victim
	    && is_set( follower->status, STAT_IN_GROUP ) ) {
	  fsend( follower, "%s removes %s from the group.",
		 ch->Seen_Name( follower ), victim->Seen_Name( follower ) );
	}
      }
      if( !is_set( victim->status, STAT_PET ) )
        stop_follower( victim );
      remove_bit( victim->status, STAT_IN_GROUP );
      
    } else {
      send( ch, "You remove yourself from your group.\n\r" );
      fsend_seen( ch, "%s removes %sself from %s group.",
		  ch, ch->Him_Her( ), ch->His_Her( ) );
      degroup( ch );
    }
    return;
  }

  if( !( victim = one_character( ch, argument, "group", ch->array ) ) )
    return;

  fsend( ch, "%s isn't following you.", victim );
}


/*
 *   UTILITY ROUTINES
 */


int min_group_move( char_data *ch )
{
  char_data*   rch;
  int         move  = ch->move;

  for( int i = 0; i < *ch->array; i++ ) 
    if( ( rch = character( ch->array->list[i] ) )
	&& is_same_group( ch, rch )
	&& rch != ch
	&& ( !rch->pcdata || !is_set( rch->pcdata->pfile->flags, PLR_GROUP_INCOG ) ) )
      move = min( move, rch->move );

  return move;
}


int min_group_hit( char_data *ch )
{
  char_data*   rch;
  int         hit  = ch->hit;

  for( int i = 0; i < *ch->array; i++ ) 
    if( ( rch = character( ch->array->list[i] ) )
	&& is_same_group( ch, rch )
	&& rch != ch
	&& ( !rch->pcdata || !is_set( rch->pcdata->pfile->flags, PLR_GROUP_INCOG ) ) )
      hit = min( hit, rch->hit );

  return hit;
}


bool is_same_group( const char_data *ach, const char_data *bch )
{
  if( !ach || !bch )
    return false;

  if( ach == bch )
    return true;

  if( !is_set( ach->status, STAT_IN_GROUP )
      || !is_set( bch->status, STAT_IN_GROUP ) )
    return false;

  for( ; ach->leader; ach = ach->leader );
  for( ; bch->leader; bch = bch->leader );

  return( ach == bch );
}


/*
 *   ORDER COMMAND
 */


void do_order( char_data *ch, const char *argument )
{
  char          arg  [ MAX_INPUT_LENGTH ];
  char_data*    rch;
  room_data*   room  = ch->in_room;
  bool        found  = false;

  if( !two_argument( argument, "to", arg ) ) {
    argument = one_argument( argument, arg );
    if( !*argument ) {
      send( ch, "Syntax: Order <pet|all> [to] <command>\n\r" );
      return;
    }
  }
  
  if( strcasecmp( arg, "all" ) ) {
    if( !( rch = one_character( ch, arg, "order", ch->array ) ) )
      return;
    
    if( rch == ch ) {
      send( ch, "Ordering yourself makes no sense.\n\r" );
      return;
    }
    
    fsend( ch, "You order %s to '%s'.", rch, argument );
    if( rch->position > POS_SLEEPING )
      fsend( rch, "%s orders you to '%s'.", ch, argument );
    fsend( *ch->array, "%s orders %s to '%s'.",
	   ch, rch, argument );
    
    if( !is_set( rch->status, STAT_PET )
	|| rch->leader != ch ) {
      fsend( ch, "%s ignores you.", rch );
      return;
    }
    
    for( mprog_data *mprog = rch->species->mprog; mprog; mprog = mprog->next ) {
      if( mprog->trigger == MPROG_TRIGGER_ORDER
	  && ( !*mprog->string
	       || is_name( argument, mprog->string ) ) ) {
	//push( );
	clear_variables( );
	var_ch = ch;
	var_mob = rch;
	var_arg = argument;
	var_room = Room( ch->array->where );
	if( !mprog->execute( )
	    || !rch->Is_Valid( ) )
	  return;
      }
    }

    set_bit( rch->status, STAT_ORDERED );
    interpret( rch, argument );
    remove_bit( rch->status, STAT_ORDERED );
    return;
  }
  
  // Copy the array since orders (e.g. movement) could change it.
  thing_array stuff = *ch->array;

  for( int i = 0; i < stuff; ++i ) {
    if( !( rch = character( stuff[i] ) )
	|| !rch->Is_Valid()
	|| rch->array != &room->contents
	|| !is_set( rch->status, STAT_PET )
	|| rch->leader != ch )
      continue;
    
    if( !found ) {
      fsend( ch, "You order all your followers to '%s'.",
	     argument );
      fsend( *ch->array, "%s orders all %s followers to '%s'.",
	     ch, ch->His_Her( ), argument );
      found = true;
    }
    
    for( mprog_data *mprog = rch->species->mprog; mprog; mprog = mprog->next ) {
      if( mprog->trigger == MPROG_TRIGGER_ORDER
	  && ( !*mprog->string
	       || is_name( argument, mprog->string ) ) ) {
	//push( );
	clear_variables( );
	var_ch = ch;
	var_mob = rch;
	var_arg = argument;
	var_room = Room( ch->array->where );
	if( !mprog->execute( )
	    || !rch->Is_Valid( ) ) {
	  rch = 0;
	  break;
	}
      }
    }

    if( rch ) {
      set_bit( rch->status, STAT_ORDERED );
      interpret( rch, argument );
      remove_bit( rch->status, STAT_ORDERED );
    }
  }
  
  if( !found ) 
    send( ch, "You have no followers here.\n\r"  );
}
