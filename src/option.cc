#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include "define.h"
#include "struct.h"


const char* message_flags [ MAX_MESSAGE ] = {
  "Autosave", "Bleeding", "Damage.Mobs",
  "Damage.Players", "Followers", "Hungry", "Max.Hit", "Max.Energy",
  "Max.Move", "Misses", "Queue", "Spell.Counter", "Thirsty", "Weather", 
  "Item.Lists", "Long.Names", "Door.Dir" };

const char* mess_settings [ MAX_MESS_SETTING ] = {
  "Look", "??", "Banks",
  "Prepare", "Equipment", "Inventory", "??" };

const char *iflag_name [ MAX_IFLAG+1 ] = {
  "auction", "info.clans", "deaths", "levels", "logins",
  "info.bugs", "requests", "writes", "admin", "maint",
  "notes" };

const char* plyr_settings [ MAX_SETTING ] = {
  "Autoloot", "Autoscan",
  "Ignore", "Incognito", "Room.Info" };


int msg_type  = MSG_STANDARD;



Info_Data :: Info_Data( const char *s1, const char *s2,
			int n, int l, int t,
			clan_data *c, pfile_data *p )
  : next(0), none(n), level(l), type(t), clan(c), pfile(p),
    when(current_time)
{
  record_new( sizeof( Info_Data ), MEM_INFO );
  string1 = alloc_string( s1, MEM_INFO );
  string2 = alloc_string( s2, MEM_INFO );
}


Info_Data :: ~Info_Data( )
{
  record_delete( sizeof( Info_Data ), MEM_INFO );
  free_string( string1, MEM_INFO );
  free_string( string2, MEM_INFO );
}


/*
 *   INFO ROUTINES
 */


info_data *info_history [ MAX_IFLAG+1 ];


static bool can_see_info( char_data* ch, pfile_data* pfile, int type )
{
  if( type == IFLAG_LOGINS
      || type == IFLAG_DEATHS
      || type == IFLAG_LEVELS ) 
    return( !pfile || !is_incognito( pfile, ch ) );

  return true;
}


static const char *info_msg( char_data* ch, info_data* info )
{
  if( ( info->clan && info->clan != ch->pcdata->pfile->clan )
      || !can_see_info( ch, info->pfile, info->type )
      || info->none > get_trust( ch ) )
    return empty_string;
  
  return( info->level > get_trust( ch )
	  ? info->string1
	  : info->string2 );
}


void do_info( char_data* ch, const char *argument )
{
  info_data*     info;
  const char*  string;
  bool          found;
  bool          first  = true;
  int          length  = strlen( argument );
  int            j;

  if( not_player( ch ) )
    return;

  const unsigned width = ch->pcdata->columns;

  for( int i = 0; i < MAX_IFLAG+1; ++i ) {
    if( !*argument
	|| !strncasecmp( argument, iflag_name[i], length ) ) {
      found = false;
      for( j = 0, info = info_history[i]; info; info = info->next )
        if( info_msg( ch, info ) != empty_string )
          j++;
      for( info = info_history[i]; info; info = info->next ) {
        if( ( string = info_msg( ch, info ) ) != empty_string  
	    && ( *argument || --j < 5  ) ) {
          if( !found ) {
            if( !first )
              page( ch, "\n\r" );
	    page_centered( ch, "--- %s ---", iflag_name[i] );
            found = true;
            first = false;
	  }
	  const bool timestamp = is_set( ch->pcdata->pfile->flags, PLR_INFO_TIME );
	  const unsigned indent = timestamp ? 19 : 0;
	  if( strlen( string )+indent < width ) {
	    if( timestamp ) {
	      page( ch, "[%s] %s\n\r",
		    ltime( info->when, false, ch ),
		    string );
	    } else {
	      page( ch, "%s\n\r", string );
	    }
	  } else {
	    const char *s = string;
	    const char *w = s;
	    const char *t = s;
	    while( true ) {
	      if( t-s+indent == width ) {
		if( w == s ) {
		  // No word-break found.
		  w = t-1;
		  t = 0;
		}
		if( s == string ) {
		  if( timestamp ) {
		    page( ch, "[%s] %.*s\n\r",
			  ltime( info->when, false, ch ),
			  w-s, s );
		  } else {
		    page( ch, "%.*s\n\r", w-s, s );
		  }
		} else {
		  if( timestamp ) {
		    page( ch, "%19s%.*s\n\r", "", w-s, s );
		  } else {
		    page( ch, "%.*s\n\r", w-s, s );
		  }
		}
		s = t ? w+1 : w;
		skip_spaces(s);
		t = w = s;
	      }
	      if( !*t )
		break;
	      if( *t == ' ' )
		w = t;
	      ++t;
	    }
	    if( t > s ) {
	      if( timestamp ) {
		page( ch, "%19s%.*s\n\r", "", t-s, s );
	      } else {
		page( ch, "%.*s\n\r", t-s, s );
	      }
	    }
	  }
	}
      }
      if( *argument ) {
        if( first ) 
          send( ch, "The %s info history is blank.\n\r", iflag_name[i] );
        return;
      }
    }
  }
  
  if( !*argument ) { 
    if( first )
      send( ch, "The info history is blank.\n\r" );
    return;
  }
  
  if( toggle( ch, argument, "Info channel",
	      ch->pcdata->pfile->flags, PLR_INFO ) ) 
    return;

  send( ch, "Illegal syntax - see help info.\n\r" );
}


void do_iflag( char_data* ch, const char *argument )
{
  if( not_player( ch ) )
    return;

  if( !*argument ) {
    display_levels( "Info",
		    &iflag_name[0],
		    &iflag_name[1],
		    &ch->pcdata->iflag[0], MAX_IFLAG, ch );
    page( ch, "\n\r" );
    display_levels( "Noteboard",
		    &noteboard_name[0],
		    &noteboard_name[1],
		    &ch->pcdata->iflag[1], MAX_NOTEBOARD, ch );
    return;
  }

  if( set_levels( &iflag_name[0], &iflag_name[1], &ch->pcdata->iflag[0],
		  MAX_IFLAG, ch, argument, false ) ) 
    return;
  
  if( set_levels( &noteboard_name[0], &noteboard_name[1], &ch->pcdata->iflag[1],
		  MAX_NOTEBOARD, ch, argument, false ) ) 
    return;

  send( ch, "Unknown option.\n\r" );
}


// Priority: 1 == highest, 3 == lowest.
void info( int none, const char* arg1, int level, const char* arg2, int flag,
	   int priority, char_data *ch, clan_data *clan,
	   pfile_data *pfile, thing_array *array )
{
  const char *const bug_name  [] = {
    "## Roach ##",
    "## Beetle ##",
    "## Aphid ##" };
  
  char             tmp1  [ FOUR_LINES ];
  char             tmp2  [ FOUR_LINES ];
  char             tmp3  [ MAX_STRING_LENGTH ];
  char             tmp4  [ MAX_STRING_LENGTH ];
  player_data*   victim  = 0;

  info_data *info;

  if( array ) {
    snprintf( tmp3, MAX_STRING_LENGTH, arg1, list_name( 0, array ).c_str() );
    snprintf( tmp4, MAX_STRING_LENGTH, arg2, list_name( 0, array ).c_str() );
    info = new info_data( tmp3, tmp4, none, level, flag,
			  clan, ch ? ch->pcdata->pfile : 0 );
  } else {
    info = new info_data( arg1, arg2, none, level, flag,
			  clan, ch ? ch->pcdata->pfile : 0 );
  }

  info_data **history = &info_history[ min( flag, MAX_IFLAG ) ];
  
  append( *history, info );

  if( count( *history ) > 50 ) {
    info = *history;
    *history = info->next;
    delete info;
  }

  snprintf( tmp1, FOUR_LINES, "%s || %%s", flag == IFLAG_AUCTION ? "Auction"
	    : ( flag == IFLAG_BUGS ? bug_name[priority-1] : "Info" ) );

  for( link_data *link = link_list; link; link = link->next ) {
    if( link->connected != CON_PLAYING
	|| !( victim = link->player )
	|| victim == ch
	|| !is_set( victim->pcdata->pfile->flags, PLR_INFO ) 
	|| ( clan && victim->pcdata->pfile->clan != clan ) 
	|| ( ch && !can_see_info( victim, ch->pcdata->pfile, flag ) ) )
      continue;
    
    int setting;
    if( flag < MAX_IFLAG )  
      setting = level_setting( &victim->pcdata->iflag[0], flag );
    else
      setting = level_setting( &victim->pcdata->iflag[1], flag-MAX_IFLAG );

    if( ch && ( flag == IFLAG_LOGINS || flag == IFLAG_DEATHS
		|| flag == IFLAG_LEVELS || flag == IFLAG_ADMIN
		|| flag == IFLAG_MAINT ) ) {
      if( setting == 0
	  || ( setting == 1 && !victim->Befriended( ch ) )
	  || ( setting == 2 && !victim->Recognizes( ch ) ) 
	  || !can_see_who( victim, ch ) )
        continue;
    } else if( setting < priority )
      continue;
    
    int trust = get_trust( victim );
    const char *string = empty_string;
    if( trust >= none ) {
      if( trust < level
	  || pfile && pfile == victim->pcdata->pfile ) {
	if( array ) {
	  snprintf( tmp3, MAX_STRING_LENGTH, arg1, list_name( link->character, array ).c_str() );
	  string = tmp3;
	} else {
	  string = arg1;
	}
      } else {
	if( array ) {
	  snprintf( tmp3, MAX_STRING_LENGTH, arg2, list_name( link->character, array ).c_str() );
	  string = tmp3;
	} else {
	  string = arg2;
	}
      }
    }

    const unsigned width = victim->pcdata->columns;
    const unsigned len1 = strlen( tmp1 );
    while( true ) {
      string = break_line( string, tmp2, width+1-len1 );
      if( !*tmp2 )
        break;
      send_color( link->character,
		  flag == IFLAG_AUCTION ? COLOR_AUCTION : COLOR_INFO,
		  tmp1, tmp2 );
      send( link->character, "\n\r" );
    }
  }
}


/*
 *   MESSAGE ROUTINE
 */


void do_message( char_data* ch, const char *argument )
{
  if( not_player( ch ) )
    return;

  if( !*argument ) {
    display_flags( "*Message", &message_flags[0],
		   &message_flags[1], &ch->pcdata->message, MAX_MESSAGE, ch );
    page( ch, "\n\r" );
    display_levels( "Message", &mess_settings[0],
		    &mess_settings[1], &ch->pcdata->mess_settings, MAX_MESS_SETTING, ch );
    page( ch, "\n\r" );
    page_centered( ch, "[ Also see the iflag and option commands ]" );
    return;
  }
  
  if( set_flags( &message_flags[0], &message_flags[1],
		 &ch->pcdata->message, 0,
		 MAX_MESSAGE, 0,
		 ch, argument, 0,
		 false, false ) ) 
    return;
  
  if( !set_levels( &mess_settings[0], &mess_settings[1],
		   &ch->pcdata->mess_settings, MAX_MESS_SETTING, ch, argument, false ) )
    send( ch, "Unknown option.\n\r" );
}


/*
 *   OPTION ROUTINE
 */


void do_options( char_data* ch, const char *argument )
{
  if( not_player( ch ) )
    return;

  int max_plr = is_demigod( ch ) ? MAX_PLR : MAX_PLR_OPTION;

  if( !*argument ) {
    display_flags( "*Option Flags",
		   &plr_name[0], &plr_name[1],
		   ch->pcdata->pfile->flags, max_plr, ch );
    page( ch, "\n\r" );
    display_levels( "Option", &plyr_settings[0],
		    &plyr_settings[1],
		    &ch->pcdata->pfile->settings,
		    MAX_SETTING, ch );
    page( ch, "\n\r" );
    page_centered( ch, "[ Also see the iflag and message commands ]" );
    return;
  }
  
  int sorted[ max_plr ];
  max_plr = sort_names( &plr_name[0], &plr_name[1],
			sorted, max_plr, true );

  for( int i = 0; i < max_plr; ++i ) {
    if( matches( argument, plr_name[sorted[i]] ) ) {

      switch( sorted[i] ) {
      case PLR_TRACK:
	send( ch, "Please use the \"track\" command instead.\n\r" );
	return;

	/*
      case PLR_SEARCHING:
	send( ch, "Please use the \"search\" command instead.\n\r" );
	return;
	*/

      case PLR_NO_PRIVILEGES:
	send( ch, "Please use the \"privileges\" command instead.\n\r" );
	return;
      
      case PLR_PARRY:
	if( is_berserk( ch, "parry" ) )
	  return;
	break;

      case PLR_STATUS_BAR:
        reset_screen( ch );
	break;

      default:
	break;
      }

      switch_bit( ch->pcdata->pfile->flags, sorted[i] );
      
      if( sorted[i] == PLR_STATUS_BAR ) 
        setup_screen( ch );
      
      fsend( ch, "%s set to %s.", plr_name[sorted[i]],
	     true_false( ch->pcdata->pfile->flags, sorted[i] ) );
      
      if( is_set( ch->pcdata->pfile->flags, sorted[i] ) ) {
        switch( sorted[i] ) {
	case PLR_PARRY :
	  send( ch, "[This option prevents you from fighting back.]\n\r" );
	  break;
	}
      }
      
      return;
    }
  }
  
  if( !set_levels( &plyr_settings[0], &plyr_settings[1],
		   &ch->pcdata->pfile->settings, MAX_SETTING, ch, argument, false ) )
    send( ch, "Unknown flag or setting.\n\r" );
}
