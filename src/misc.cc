#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <syslog.h>
#include "define.h"
#include "struct.h"


bool is_berserk( char_data* ch, const char *msg )
{
  if( is_set( ch->status, STAT_BERSERK ) ) {
    cant_message( ch, msg, "berserk" );
    return true;
  }

  return false;
}


bool is_focusing( char_data* ch, const char *msg )
{
  if( is_set( ch->status, STAT_FOCUS ) ) {
    cant_message( ch, msg, "focusing" );
    return true;
  }

  return false;
}


bool is_parrying( char_data* ch, const char *msg )
{
  if( ch->species )
    return false;

  if( is_set( ch->pcdata->pfile->flags, PLR_PARRY ) ) {
    cant_message( ch, msg, "parrying" );
    return true;
  }

  return false;
}


bool is_confused( char_data* ch, const char *msg )
{
  if( ch->is_affected( AFF_CONFUSED ) ) {
    cant_message( ch, msg, "confused" );
    return true;
  }

  return false;
}


bool is_choking( char_data* ch, const char *msg )
{
  if( ch->is_affected( AFF_CHOKING ) ) {
    cant_message( ch, msg, "choking to death" );
    return true;
  }

  return false;
}


bool is_paralyzed( char_data* ch, const char *msg )
{
  if( ch->is_affected( AFF_PARALYSIS ) ) {
    cant_message( ch, msg, "paralyzed" );
    return true;
  }

  return false;
}


bool is_silenced( char_data* ch, const char *msg )
{
  if( ch->is_affected( AFF_SILENCE ) ) {
    cant_message( ch, msg, "silenced" );
    return true;
  }

  if( ch->is_affected( AFF_CHOKING ) ) {
    cant_message( ch, msg, "choking to death" );
    return true;
  }

  return false;
}


bool is_entangled( char_data* ch, const char *msg )
{
  if( ch->is_affected( AFF_PARALYSIS ) ) {
    cant_message( ch, msg, "paralyzed" );
    return true;
  }

  if( ch->is_affected( AFF_ENTANGLED ) ) {
    cant_message( ch, msg, "entangled" );
    return true;
  }

  if( ch->is_affected( AFF_CHOKING ) ) {
    cant_message( ch, msg, "choking to death" );
    return true;
  }

  return false;
}


bool is_drowning( char_data *ch, const char *msg )
{
  if( ch->mod_position( ) != POS_DROWNING )
    return false;

  /*
  room_data *room = ch->in_room;

  if( !room
      || room->sector_type != SECT_UNDERWATER )
    return false;
  
  if( ch->can_breathe_underwater( ) ) {
    return false;
  }
  */

  cant_message( ch, msg, "drowning" );
  return true;
}


void cant_message( char_data* ch, const char *act, const char *cond )
{
  if( !act )
    return;

  fsend( ch, "You can't %s while %s.",
	 *act  ? act : "do that",
	 cond );
}


/*
 *   BUG ROUTINES
 */


void panic( const char* text )
{
  bug( text );
  bug( "** MUD KILLED **" );

  quit( 1 );
}


void bug( int level, const char* str )
{
  if( !str || str == empty_string )
    return;

  if( level != BUG_APHID ) {
    if( FILE *fp = fopen( BUG_FILE, "a" ) ) {
      fprintf( fp, "[%s] %s\n", ltime( current_time, true ), str );
      fclose( fp );
    } else {
      info( LEVEL_APPRENTICE, empty_string, 0, "Error opening bug file.", IFLAG_BUGS, 1 );
      fprintf( stderr, "[BUG] Error opening bug file.\n" );
    }
  }
  
  if( level != -1 ) {
    fprintf( stderr, "[BUG] %s\n\r", str );
    info( LEVEL_APPRENTICE, empty_string, 0, str, IFLAG_BUGS, level );
  }
}


/*
 *   DEFINE COMMAND
 */


const char *lookup( const index_data *index, int number, bool plural )
{
  int i;

  for( i = 0; index[i].value < number && index[i].value != -1; ++i );

  return( plural ? index[i].plural : index[i].singular );
}


void do_define( char_data* ch, const char *argument )
{
#define count 9

  static const char *const name [count+1] = {
    "Acid Damage", "Fire Damage", "Cold Damage",
    "Electrical Damage", "Physical Damage", "Sonic Damage",
    "Fame", "Piety", "Reputation", "" };
  
  static const index_data *const index [count] = {
    acid_index, fire_index, cold_index,
    shock_index, physical_index, sound_index,
    fame_index, piety_index, reputation_index };

  if( !*argument ) {
    display_array( ch, "+Defined Terms",
		   &name[0], &name[1],
		   count );
    /*
    send_title( ch, "Defined Terms" );
    for( int i = 0; *name[i] != '\0'; i++ ) 
      send( ch, "%20s%s", name[i], i%3 == 2 || *name[i+1] == '\0'
	    ? "\n\r" : "" );
    */
    return;
  }

#undef count  

  for( int i = 0; *name[i]; ++i ) {
    if( fmatches( argument, name[i] ) ) {
      page_title( ch, name[i] );
      int prev = ( index[i][0].value < 0 ? -1000 : 0 );
      for( int j = 0; ; j++ ) {
        if( j == 0 && index[i][0].value == 0 ) {
	  prev = 1;
          continue;
	}
        if( index[i][j].value == -1 ) {
          page( ch, "%39s   %d+\n\r", index[i][j].singular, prev );
          return;
          }
        if( index[i][j].value == prev )
          page( ch, "%39s   %d\n\r", index[i][j].singular, prev );
        else
          page( ch, "%39s   %d to %d\n\r",
            index[i][j].singular, prev, index[i][j].value );
        prev = index[i][j].value+1;
      }
    }
  }

  send( ch, "Unknown field - see help define.\n\r" );
}


/*
 *   TYPO COMMAND
 */


void do_typo( char_data* ch, const char *argument )
{
  if( not_player( ch ) )
    return;

  if( *argument == '\0' ) {
    send( ch, "Room #%d\n\r\n\r", ch->in_room->vnum );
    send( ch, "What typo do you wish to report?\n\r" );
    return;
  } 
  
  if( strlen( ch->in_room->Comments( ) )
      > MAX_STRING_LENGTH-MAX_INPUT_LENGTH-30 ) {
    send( ch, "Comment field at max length - typo ignored.\n\r" );
    return;
  }
  
  char tmp [ MAX_STRING_LENGTH ];

  char_data *rch = ch->species ? ch->leader : ch;

  snprintf( tmp, MAX_STRING_LENGTH, "%s[%s] %s\n\r", ch->in_room->Comments( ),
	    rch->real_name( ), argument );
  
  ch->in_room->Set_Comments( ch, tmp );

  snprintf( tmp, MAX_STRING_LENGTH, "Typo posted by %s at %s.",
	    rch->real_name( ), ch->Location( ) );
  info( invis_level( rch ), empty_string, LEVEL_APPRENTICE, tmp, IFLAG_BUGS, 1, rch );

  send( ch, "Typo noted - thanks.\n\r" );
}    


/*
 *   RANDOM SUPPORT ROUTINES
 */


bool player_in_room( room_data* room )
{
  // Note that familiars will return true.
  if( room ) {
    for( int i = 0; i < room->contents; ++i ) {
      char_data *ch;
      if( ( ch = character( room->contents[i] ) )
	  && invis_level( ch ) < LEVEL_BUILDER
	  && ch->pcdata ) {
        return true;
      }
    }
  }

  return false;
}


void do_wait( char_data* ch, const char *argument )
{
  const char *const usage = "Syntax: wait <seconds>\n\r";

  int i;

  if( !number_arg( argument, i ) ) {
    send( ch, usage );
    return;
  }

  if( *argument ) {
    send( ch, usage );
    return;
  }

  if( ( i > 30 && !is_demigod( ch ) ) || i < 0 ) {
    send( ch, "You can wait for 0 to 30 seconds.\n\r" );
    return;
  }

  set_delay( ch, i * PULSE_PER_SECOND, false );

  set_bit( ch->status, STAT_WAITING );
}


void do_queue( char_data* ch, const char *argument )
{
  if( !*argument )
    return;

  interpret( ch, argument );
}


void confused_char( char_data* ch )
{
  char_data *rch = 0;
  int j = number_range( 0, 4 );

  if( j < 2 ) {
    rch = random_pers( ch->in_room, ch );
  } else if( j == 2 ) {
    rch = ch;
  }

  /*  
  if( rch
      && j == 1
      && !opponent( ch )
      && can_kill( ch, rch, false ) ) {
    init_attack( ch, rch );
    return;
  }
  */
  
  const char *const word[] = {
    "hop", "pout", "smile", "growl", "sulk",
    "drool", "clap", "fume", "whimper", "weep",
    "giggle", "stare", "sob", "shiver", "jump",
    "cry", "dance", "cluck", "bark", "howl",
    "pant", "cackle", "frolic", "moan", "groan",
    "gulp", "whine", "snicker", "snort", "bstare",
    "raspberry", "snarl", "mutter", "tongue", "admire",
    "salute", "strut", "tickle", "embrace", "flex"
  };
  
  int i = number_range( 0, 39 );

  if( !rch ) {
    check_social( ch, word[i], 0 );
  } else if( rch == ch ) {
    check_social( ch, word[i], 0, ch );
  } else {
    check_social( ch, word[i], 0, rch );
  }
}


void do_journal( char_data *ch, const char *argument )
{
  if( not_player( ch ) )
    return;

  char tmp [ 3*MAX_STRING_LENGTH ];

  if( is_builder( ch )
      && has_permission( ch, PERM_SNOOP ) ) {
    int flags;
    if( !get_flags( ch, argument, &flags, "p", "journal" ) ) {
      return;
    }

    if( is_set( flags, 0 ) ) {
      in_character = false;
      
      pfile_data *pfile = find_pfile( argument, ch );
      
      if( !pfile )
	return;
      
      if( pfile != ch->pcdata->pfile
	  && pfile->trust >= get_trust( ch ) ) {
	fsend( ch, "You cannot view the journal of %s.", pfile->name );
	return;
      }
      
      player_data *pc = find_player( pfile );
      bool loaded = false;
      
      if( !pc ) {
	link_data link;
	link.connected = CON_PLAYING;
	if( !load_char( &link, pfile->name, PLAYER_DIR ) ) {
	  bug( "Load_players: error reading player file. (%s)", pfile->name );
	  return;
	}
	pc = link.player;
	loaded = true;
      }
      
      if( !*pc->pcdata->journal ) {
	if( ch == pc ) {
	  page( ch, "Your journal is empty.\n\r" );
	} else {
	  page( ch, "%s's journal is empty.\n\r", pc );
	}
	
      } else {
	
	if( ch == pc ) {
	  page_title( ch, "Your Journal" );
	} else {
	  page_title( ch, "%s's journal", pc );
	}
	
	convert_to_ansi( ch, 3*MAX_STRING_LENGTH, pc->pcdata->journal, tmp );
	page( ch, tmp );
      }
      
      if( loaded ) {
	page( ch, "\n\r" );
	page_centered( ch, "[ Player file was loaded from disk. ]" );
	pc->Extract();
	extracted.delete_list();
      }
      return;
    }
  }

  if( !*argument ) {
    if( !*ch->pcdata->journal ) {
      send( ch, "Your journal is empty.\n\r" );
      send( ch, "Type \"journal <text>\" to add text.\n\r" );
      return;
    }
    page_title( ch, "Your Journal" );

  } else if( exact_match( argument, "preview" ) ) {
    if( !*ch->pcdata->journal ) {
      send( ch, "Your journal is empty.\n\r" );
      send( ch, "Type \"journal <text>\" to add text.\n\r" );
      return;
    }
    page_title( ch, "Your Journal" );
    convert_to_ansi( ch, 3*MAX_STRING_LENGTH, ch->pcdata->journal, tmp );
    page( ch, tmp );
    return;
  }

  ch->pcdata->journal = edit_string( ch, argument, ch->pcdata->journal, MEM_PLAYER, false );
}
