#include <sys/types.h>
#include <stdio.h>
#include <syslog.h>
#include "define.h"
#include "struct.h"


static void char_bash   ( char_data*, char_data* );
static void door_bash   ( char_data*, exit_data* );


void do_bash( char_data* ch, const char *argument )
{
  char            arg  [ MAX_INPUT_LENGTH ];
  char_data*   victim;
  
  if( is_confused_pet( ch )
      || is_mounted( ch, "bash" )
      || is_entangled( ch, "bash" )
      || is_drowning( ch, "bash" ) )
    return;
  
  if( !*argument ) {
    if( !( victim = ch->fighting ) ) {	
      send( ch, "Whom do you want to bash?\n\r" );
      return;
    }
    victim->Select( 1 );
    char_bash( ch, victim );
    return;
  }
  
  argument = one_argument( argument, arg ); 
  
  visible_data *vis = one_visible(ch, arg, "bash",
				  (visible_array *) ch->array, CHAR_DATA,
				  (visible_array *) &ch->in_room->exits, EXIT_DATA );
  
  if( ( victim = character( vis ) ) ) {
    //    if( ch->fighting && victim != ch->fighting ) {
    //      send( ch, "You are already attacking someone else!\n\r" );
    //      return;
    //    }
    char_bash( ch, victim );
    return;
  }
  
  door_bash( ch, exit( vis ) );

  /*  
  argument = one_argument( argument, arg );
  
  if( !strncasecmp( arg, "with", 4 ) )
    strcpy( arg, argument );
  
  if( ( obj = object( vis ) ) )
    if( ( bludgeon = one_object( ch, arg, "bash with", &ch->contents,
				 &ch->wearing ) ) ) {
      obj_bash( ch, obj, bludgeon );
      return;
    } else
      return;
  
  if( vis )
    send( ch, "You can't bash that.\n\r");
  */
}


/*
 *   CHARACTER BASH FUNCTION
 */ 


bool can_bash( char_data *ch, char_data *victim, bool msg )
{
  if( ch ) {
    if( victim == ch ) {
      if( msg )
	send( ch, "Bashing yourself is not very productive.\n\r" );
      return false;
    }
  
    if( !can_kill( ch, victim, msg ) )
      return false;
    
    if( !ch->Can_See( ) ) {
      if( msg )
	send( ch, "You can't bash unless you can see!\n\r" );
      return false;
    }
    
    if( water_logged( ch->in_room ) ) {
      if( msg )
	send( ch, "You can only bash on dry land.\n\r" );
      return false;
    }
    
    if( midair( ch ) ) {
      if( msg )
	send( ch, "You cannot bash in midair.\n\r" );
      return false;
    }
  }

  if( victim->species ) { 
    if( is_set( victim->species->act_flags, ACT_NO_BASH ) ) {
      if( msg )
	fsend( ch, "%s cannot be bashed.", victim );
      return false;
    }
    if( is_set( victim->species->act_flags, ACT_GHOST ) ) {
      if( msg )
	send( ch, "Bashing a ghost is a completely futile exercise.\n\r" );
      return false;
    }
  }
  
  const int mpos = victim->mod_position( );

  if( mpos == POS_FLYING ) {
    if( ch && msg ) {
      fsend( ch, "%s is flying and cannot be bashed.", victim );
    }
    return false; 
  }

  //  if( !ch )
  //    return true;

  if( victim->position < POS_FIGHTING ) {
    if( ch && msg ) {
      const char *pos = ch->in_room->position( );
      if( *pos ) {
	fsend( ch, "Your victim is already down %s!", pos );
      } else {
	fsend( ch, "Your victim is already down!" );
      }
    }
    return false;
  }
  
  if( ch && victim->Size( ) > ch->Size( ) + 1 ) {
    if( msg )
      fsend( ch, "%s is way too large for you to successfully bash %s.",
	     victim, victim->Him_Her( ) );
    return false;
  }

  return true;
}


static int bash_attack( char_data* ch, char_data* victim )
{
  int roll = number_range( 0, 20 )
    - 2*(victim->Size( ) - ch->Size( )) 
    - (victim->Dexterity() - ch->Dexterity())/2
    - (victim->Level() - ch->Level())/5;
  
  if( !ch->species )
    roll += ch->get_skill( SKILL_BASH )/2;
  else
    roll += 3;

  const int mpos = ch->mod_position( );

  if( roll < 4 ) {  
    if( mpos != POS_FLYING && number_range( 0, 26 ) < ch->Dexterity() ) {
      if( !ch->rider ) {
	fsend( ch, "You attempt to bash %s, but miss and fall down!",
	       victim );
	fsend( victim,
	       "%s attempts to bash you, but %s misses and falls down.",
	       ch, ch->He_She( victim ) );
	fsend_seen( ch,
		    "%s attempts to bash %s, but %s misses and falls down.",
		    ch, victim, ch->He_She( ) );
      } else {
	const char *pos = ch->in_room->position( );
	if( *pos ) {
	  fsend( ch, "You attempt to bash %s, but miss and fall down, tumbling %s %s!",
		 victim, ch->rider, pos );
	  fsend( victim,
		 "%s attempts to bash you, but %s misses and falls down, tumbling %s %s.",
		 ch, ch->He_She( victim ), ch->rider, pos );
	  fsend_seen( ch,
		      "%s attempts to bash %s, but %s misses and falls down, tumbling %s %s.",
		      ch, victim, ch->He_She( ), ch->rider, pos );
	  fsend( ch->rider, "%s attempts to bash %s, but %s misses and falls down, tumbling you %s!",
		 ch, victim, ch->He_She( ), pos );
	} else {
	  fsend( ch, "You attempt to bash %s, but miss and fall down, sending %s tumbling!",
		 victim, ch->rider );
	  fsend( victim,
		 "%s attempts to bash you, but %s misses and falls down, sending %s tumbling.",
		 ch, ch->He_She( victim ), ch->rider );
	  fsend_seen( ch,
		      "%s attempts to bash %s, but %s misses and falls down, sending %s tumbling.",
		      ch, victim, ch->He_She( ), ch->rider );
	  fsend( ch->rider, "%s attempts to bash %s, but %s misses and falls down, sending you tumbling!",
		 ch, victim, ch->He_She( ) );
	}
	dismount( ch->rider, POS_RESTING );
      }
      ch->position = POS_RESTING;
    } else {
      fsend( ch, "You attempt to bash %s but are unsuccessful.",
	    victim );
      fsend( victim, "%s attempts to bash you but is unsuccessful.",
	    ch );
      fsend_seen( ch,
		 "%s attempts to bash %s but is unsuccessful.",
		 ch, victim );
    }
    return 32;
  }
  
  if( roll > 20 ) {
    const char *pos = ch->in_room->position( );
    if( !victim->rider ) {
      if( *pos ) {
	fsend_color( ch, COLOR_SKILL, "You send %s sprawling %s!",
		     victim, pos );
	fsend_color( victim, COLOR_SKILL, "%s sends you sprawling %s!",
		     ch, pos );
	fsend_color_seen( ch, COLOR_SKILL, "%s sends %s sprawling %s!",
			  ch, victim, pos );
      } else {
	fsend_color( ch, COLOR_SKILL, "You send %s sprawling!",
		     victim );
	fsend_color( victim, COLOR_SKILL, "%s sends you sprawling!",
		     ch );
	fsend_color_seen( ch, COLOR_SKILL, "%s sends %s sprawling!",
			  ch, victim );
      }
    } else {
      if( *pos ) {
	fsend_color( ch, COLOR_SKILL, "You send %s sprawling %s, taking %s with %s!",
		     victim, pos, victim->rider, victim->Him_Her( ) );
	fsend_color( victim, COLOR_SKILL, "%s sends you sprawling %s, taking %s with you!",
		     ch, pos, victim->rider );
	fsend_color_seen( ch, COLOR_SKILL, "%s sends %s sprawling %s, taking %s with %s!",
			  ch, victim, pos, victim->rider, victim->Him_Her( ) );
	fsend_color( victim->rider, COLOR_SKILL, "%s sends %s sprawling %s, taking you with %s!",
		     ch, victim, pos, victim->Him_Her( ) );
      } else {
	fsend_color( ch, COLOR_SKILL, "You send %s sprawling, taking %s with %s!",
		     victim, victim->rider, victim->Him_Her( ) );
	fsend_color( victim, COLOR_SKILL, "%s sends you sprawling, taking %s with you!",
		     ch, victim->rider );
	fsend_color_seen( ch, COLOR_SKILL, "%s sends %s sprawling, taking %s with %s!",
			  ch, victim, victim->rider, victim->Him_Her( ) );
	fsend_color( victim->rider, COLOR_SKILL, "%s sends %s sprawling, taking you with %s!",
		     ch, victim, victim->Him_Her( ) );
      }
      dismount( victim->rider, POS_RESTING );
      record_damage( victim->rider, ch );
    }
    record_damage( victim, ch );
    disrupt_spell( victim );
    set_min_delay( victim, 32 );
    victim->position = POS_RESTING;
    ch->improve_skill( SKILL_BASH );
    return 20;
  }
  
  if( mpos == POS_FLYING || roll < 15 ) {
    fsend( ch, "You attempt to bash %s but fail.", victim );
    fsend( victim, "%s attempts to bash you but fails.", ch );
    fsend_seen( ch, 
		"%s attempts to bash %s but fails.", ch, victim );
    return 20;
  }

  if( !ch->rider ) {
    fsend( ch, "You attempt to bash %s, but are knocked down yourself!",
	   victim );
    fsend( victim, "%s attempts to bash you, but you knock %s down instead.",
	   ch, ch->Him_Her( ) );
    fsend_seen( ch,
		"Attempting to bash %s, %s is knocked down.",
		victim, ch ); 
  } else {
    const char *pos = ch->in_room->position( );
    if( *pos ) {
      fsend( ch, "You attempt to bash %s, but are knocked down yourself, tumbling %s %s!",
	     victim, ch->rider, pos );
      fsend( victim, "%s attempts to bash you, but you knock %s down instead, tumbling %s %s.",
	     ch, ch->Him_Her( ), ch->rider, pos );
      fsend_seen( ch,
		  "Attempting to bash %s, %s is knocked down, tumbling %s %s.",
		  victim, ch, ch->rider, pos ); 
      fsend( ch->rider,
	     "Attempting to bash %s, %s is knocked down, tumbling you %s!",
	     victim, ch, pos ); 
    } else {
      fsend( ch, "You attempt to bash %s, but are knocked down yourself, sending %s tumbling!",
	     victim, ch->rider );
      fsend( victim, "%s attempts to bash you, but you knock %s down instead, sending %s tumbling.",
	     ch, ch->Him_Her( ), ch->rider );
      fsend_seen( ch,
		  "Attempting to bash %s, %s is knocked down, sending %s tumbling.",
		  victim, ch, ch->rider ); 
      fsend( ch->rider,
	     "Attempting to bash %s, %s is knocked down, sending you tumbling!",
	     victim, ch ); 
    }
    dismount( ch->rider, POS_RESTING );
  }

  ch->position = POS_RESTING;
  
  return 32;
}


void char_bash( char_data* ch, char_data* victim )
{
  char_data *orig = victim;

  if( victim->mount ) {
    victim = victim->mount;
  }

  if( !can_bash( ch, victim, true ) )
    return;

  if( !set_fighting( ch, orig ) )
    return;

  if( victim != orig ) {
    react_attack( ch, victim );
  }

  remove_bit( ch->status, STAT_WIMPY );
  strip_affect( ch, AFF_INVISIBLE );
  leave_shadows( ch );

  set_delay( ch, bash_attack( ch, victim ) );
}


/*
 *   OBJECT BASH ROUTINE
 */


/*
void obj_bash( char_data* ch, obj_data*, obj_data* )
{
  send( ch, "Bashing objects does nothing useful yet.\n\r" );
}
*/


void door_bash( char_data* ch, exit_data* door )
{
  send( ch, "Bashing exits does nothing useful yet.\n\r" );
}
