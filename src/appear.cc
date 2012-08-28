#include <ctype.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include "define.h"
#include "struct.h"


/*
 *   APPEARANCE FUNCTIONS
 */


static const char *const appearance_string = "Your %s has now been set.  When you have\
 completed it, your %s and your description please type request avatar.\
 See help appearance and the identity command for more information.";

static const char *const unchangable_msg = "You may not edit your description, keywords,\
 or appearance once approved. You may request an avatar to be unapproved,\
 after which you must be reapproved to gain levels.";


void do_appearance( char_data* ch, const char *argument )
{
  if( is_mob( ch ) )
    return;

  if( !has_permission( ch, PERM_APPROVE ) 
      && is_set( ch->pcdata->pfile->flags, PLR_APPROVED ) ) {
    fsend( ch, unchangable_msg );
    return;
  }
 
  if( !*argument ) {
    send( ch, "What do you want your appearance set to?\n\r" );
    return;
  }
  
  if( !strncasecmp( argument, "a ", 2 )
      || !strncasecmp( argument, "an ", 3 ) ) {
    send( ch, "Your appearance should not start with a or an.\n\r" );
    return;
  }
  
  if( ispunct( argument[ strlen( argument )-1 ] ) ) {
    send( ch, "Appearances should not end with punctuation.\n\r" );
    return;
  }
  
  if( strlen( argument ) >= 50 ) {
    send( ch, "Appearances must be less than 50 characters.\n\r" );
    return;
  }
  
  free_string( ch->pcdata->tmp_short, MEM_PLAYER );
  char *new_short = alloc_string( argument, MEM_PLAYER );
  *new_short = tolower( *new_short );
  ch->pcdata->tmp_short = new_short;

  fsend( ch, appearance_string, "appearance", "keywords" );
}


void do_keywords( char_data* ch, const char *argument )
{
  if( is_mob( ch ) )
    return;
  
  if( !has_permission( ch, PERM_APPROVE ) 
      && is_set( ch->pcdata->pfile->flags, PLR_APPROVED ) ) {
    fsend( ch, unchangable_msg );
    return;
  }
  
  if( !*argument ) {
    send( ch, "What do you want your keywords set to?\n\r" );
    return;
  }
  
  if( is_name( ch->pcdata->pfile->name, argument ) ) {
    send( ch, "Your keywords should NOT include your name.\n\r" );
    return;
  }
  
  free_string( ch->pcdata->tmp_keywords, MEM_PLAYER );
  ch->pcdata->tmp_keywords = alloc_string( argument, MEM_PLAYER );
  
  fsend( ch, appearance_string, "keywords", "appearance" );
}


void do_descript( char_data* ch, const char *argument )
{
  char_data*     victim  = ch;
  wizard_data*   imm;

  in_character = false;

  if( is_mob( ch ) )
    return;

  if( ( imm = wizard( ch ) ) && imm->player_edit )
    victim = imm->player_edit;

  if( !has_permission( ch, PERM_APPROVE ) 
      && is_set( ch->pcdata->pfile->flags, PLR_APPROVED ) ) {
    fsend( ch, unchangable_msg );
    return;
  }
  
  if( victim != ch ) {
    page( ch, "Player: %s\n\r\n\r", victim->Seen_Name( ch ) );
  }

  if( exact_match( argument, "preview" ) ) {
    char tmp [ 3*MAX_STRING_LENGTH ];
    convert_to_ansi( ch, 3*MAX_STRING_LENGTH, victim->descr->complete, tmp );
    page( ch, tmp );
    return;
  }

  victim->descr->complete = edit_string( ch, argument,
					 victim->descr->complete, MEM_DESCR, true );
}


/*
 *   REQUEST COMMAND
 */


request_array request_imm;
request_array request_app;


class Request_Data
{
public: 
  pfile_data*   pfile;
  char*        reason;
  time_t         when;
  
  Request_Data( char_data* ch, const char *argument )
    : pfile(ch->pcdata->pfile), when(current_time)
  {
    record_new( sizeof( request_data ), MEM_REQUEST );
    reason = alloc_string( argument, MEM_REQUEST );
  }
  
  ~Request_Data( )
  {
    record_delete( sizeof( request_data ), MEM_REQUEST );
    free_string( reason, MEM_REQUEST );
  }
};


static void display( player_data *pl, request_array& array, const char* word,
		     bool& first, int level )
{
  if( array.is_empty() ) 
    return;

  bool second = true;

  for( int i = 0; i < array; ++i ) {
    Request_Data *req = array[i];
    if( get_trust( pl ) >= level
	|| pl->pcdata->pfile == req->pfile ) {
      if( first ) {
	first = false;
	//	page_underlined( pl, "%-15s Reason\n\r", "Name" );
	page_underlined( pl, "%-18s Who\n\r", "When" );
      }
      if( second ) {
	second = false;
	page( pl, "\n\r" );
	page( pl, "--- %s ---\n\r", word );
      }
      page( pl, "\n\r[%s] %s\n\r",
	    ltime( req->when, false, pl ),
	    req->pfile->name );
      ipage( pl, req->reason, 3 );
    }
  }
}


bool includes( request_array& array, char_data* ch )
{
  for( int i = 0; i < array; ++i )
    if( array[i]->pfile == ch->pcdata->pfile )
      return true;

  return false;
}


bool remove( request_array& array, char_data* ch )
{
  for( int i = 0; i < array; ++i )
    if( array[i]->pfile == ch->pcdata->pfile ) {
      delete array[i];
      array.remove( i );
      return true;
    }
  
  return false;
}


void request_message( player_data *pl )
{
  int trust = get_trust( pl );

  if( trust < LEVEL_AVATAR
      || trust == LEVEL_AVATAR && !has_permission( pl, PERM_APPROVE ) ) {
    return;
  }

  int count = request_app.size;

  if( trust > LEVEL_AVATAR ) {
    count += request_imm.size;
  }

  if( count == 0 )
    return;

  send_centered( pl, "There %s %s request%s pending.",
		 count == 1 ? "is" : "are",
		 number_word( count ),
		 count == 1 ? "" : "s" ); 
}


void do_request( char_data* ch, const char *argument )
{
  if( is_mob( ch ) )
    return;

  if( !*argument ) {
    bool first = true;
    display( (player_data*)ch, request_app, "Avatar", first, LEVEL_AVATAR );
    display( (player_data*)ch, request_imm, "Immortal", first, LEVEL_APPRENTICE ); 
    if( first )
      send( ch, "Request queues are empty.\n\r" );
    return;
  }

  char *tmp;

  if( matches( argument, "cancel" ) ) {
    const bool av = remove( request_app, ch );
    const bool imm = remove( request_imm, ch );
    if( av || imm ) {
      send( ch, "Requests cancelled.\n\r" );
      snprintf( tmp = static_string( ), THREE_LINES,
		"%s cancelled all %s requests.",
		ch->descr->name, ch->His_Her( ) );
      const int level = av ? LEVEL_AVATAR : LEVEL_APPRENTICE;
      info( level, empty_string, 0, tmp, IFLAG_REQUESTS, 3, ch );
    } else
      send( ch, "You have no requests pending.\n\r" ); 
    return;
  }

  if( matches( argument, "avatar" ) ) {
    if( includes( request_app, ch ) ) {
      send( ch, "You have already requested an avatar.\n\r" );
      return;
    }
    if( !*argument
	&& is_set( ch->pcdata->pfile->flags, PLR_APPROVED ) ) {
      send( ch,
	    "Since you are already approved you must specify a reason.\n\r" );
      return;
    }
    request_app += new request_data( ch, argument );
    send( ch, "Request for an avatar registered.\n\r" );
    if( !*argument ) 
      snprintf( tmp = static_string( ), THREE_LINES,
	       "%s requesting an avatar for approval.",
	       ch->descr->name );
    else
      snprintf( tmp = static_string( ), THREE_LINES,
		"%s requesting an avatar with reason '%s'.",
		ch->descr->name, argument );
    info( LEVEL_AVATAR, empty_string, 0, tmp, IFLAG_REQUESTS, 2, ch );
    return;
  }

  if( matches( argument, "immortal" ) ) {
    if( includes( request_imm, ch ) ) {
      send( ch, "You have already requested immortal attention.\n\r" );
      return;  
    }
    if( !*argument ) {
      send( ch, "You must include a reason when requesting an immortal.\n\r" );
      return;
    }
    request_imm += new request_data( ch, argument );
    send( ch, "Request for an immortal registered.\n\r" );
    snprintf( tmp = static_string( ), THREE_LINES,
	      "%s requesting an immortal with reason '%s'.",
	      ch->descr->name, argument );
    info( LEVEL_APPRENTICE, empty_string, 0, tmp, IFLAG_REQUESTS, 1, ch );
    return;
  }
  
  send( ch, "Unknown syntax - see help request.\n\r" );
}
 

/*
 *   IMMORTAL COMMANDS
 */


void do_approve( char_data* ch, const char *argument )
{
  char            arg  [ MAX_INPUT_LENGTH ];
  char             tmp  [ THREE_LINES ];
  player_data*  player;
  int            flags;
  bool           found  = false;
  wizard_data*     imm  = wizard( ch );

  in_character = false;

  if( !get_flags( ch, argument, &flags, "slpfrka", "Approve" ) )
    return;
  
  if( is_set( flags, 1 ) ) {
    for( int i = 0; i < player_list; ++i ) {
      player = player_list[i];
      if( !player->In_Game( )
	  || ( player->pcdata->tmp_short == empty_string
	       && player->pcdata->tmp_keywords == empty_string )
	  || !can_see_who( ch, player ) )
        continue;
      
      if( !found ) {
        found = true;
        snprintf( tmp, THREE_LINES, "%-15s   %s\n\r", "Name", "Appearance" );
        page_underlined( ch, tmp );
      }
      page( ch, "%-15s   %s\n\r",
	    player->descr->name, player->descr->singular );          
    }
    if( !found ) 
      send( ch, "There are no players with unapproved appearances.\n\r" );
    return;
  }

  if( is_set( flags, 2 ) ) {
    if( imm->player_edit ) {
      fsend( ch, "You stop editing %s.", imm->player_edit->descr->name );
      imm->player_edit = 0;
      imm->account_edit = 0;
      if( !*argument )
	return;
    }
    if( !( player = one_player( ch, argument, "approve",
				(thing_array*) &player_list ) ) ) {
      return;
    }
    if( player == ch ) {
      send( ch, "You are now editing yourself.\n\r" );
      imm->player_edit = 0;
      imm->account_edit = 0;
      return;
    }
    if( !is_demigod( ch )
	&& ( is_avatar( player )
	     || ( player->Level() > ch->Level() + 20
		  && ch->pcdata->trust == LEVEL_AVATAR ) ) ) {
      fsend( ch, "You don't have permission to edit %s's description.",
	     player->descr->name );
      return;
    }
    for( int i = 0; i < player_list; ++i ) {
      if( wizard_data *wiz = wizard( player_list[i] ) ) {
	if( wiz->Is_Valid( ) && wiz->player_edit == player ) {
	  fsend( ch, "%s is currently editing %s.",
		 wiz->descr->name, player->descr->name );
	  return;
	}
      }
    }
    fsend( ch, "You are now editing %s.", player->descr->name );
    fsend( player, "%s is now editing your appearance.",
	   ch->descr->name );
    imm->player_edit = player;
    imm->account_edit = player->pcdata->pfile->account;
    return;
  }
  
  if( !( player = imm->player_edit )
       || *argument && flags == 0 ) {
    argument = one_argument( argument, arg );
    if( !( player = one_player( ch, arg, "approve",
				(thing_array*) &player_list ) ) ) {
      return;
    }
  }

  if( is_set( flags, 3 ) ) {
    if( !*argument
	|| ( player = one_player( ch, argument, "approve",
				  (thing_array*) &player_list ) ) ) {
      if( is_set( player->pcdata->pfile->flags, PLR_APPROVED ) ) {
	fsend( ch, "%s is already marked as approved.",
	       player->descr->name );
      } else {
	set_bit( player->pcdata->pfile->flags, PLR_APPROVED );
	fsend( ch, "You mark %s as approved.",
	       player->descr->name );
	remove( request_app, player );
      }
    }
    if( imm->player_edit && player != imm->player_edit ) {
      fsend( ch, "You are still editing %s.", imm->player_edit->descr->name );
    }
    return;
  }

  if( is_set( flags, 4 ) ) {
    if( !*argument
	|| ( player = one_player( ch, argument, "approve",
				   (thing_array*) &player_list ) ) ) {
      if( !is_set( player->pcdata->pfile->flags, PLR_APPROVED ) ) {
	fsend( ch, "%s isn't approved.", player->descr->name );
      } else {
	remove_bit( player->pcdata->pfile->flags, PLR_APPROVED );
	player->set_default_title( );
	free_string( player->pcdata->pfile->homepage, MEM_PFILE );
	player->pcdata->pfile->homepage = empty_string;
	fsend( ch, "You unapprove %s.", player->descr->name );
	fsend_color( player, COLOR_WIZARD, "%s has unapproved you.", ch );
	player_log( player, "Unapproved by %s.", ch->real_name( ) );
	player_log( ch, "Unapproved %s.", player->real_name( ) );
	snprintf( tmp, THREE_LINES, "%s unapproved.", player->Name() );
	snprintf( arg, MAX_INPUT_LENGTH, "%s unapproved by %s.", player->Name(), ch->Name() );
	info( LEVEL_AVATAR, tmp, invis_level( ch ), arg, IFLAG_ADMIN, 1, ch );
      }
    }
    if( imm->player_edit && player != imm->player_edit ) {
      fsend( ch, "You are still editing %s.", imm->player_edit->descr->name );
    }
    return;
  }

  if( is_set( flags, 5 ) ) {
    if( is_set( player->pcdata->pfile->flags, PLR_APPROVED ) ) {
      fsend( ch, "%s is approved, and must be unapproved before keywords can be changed.",
	    player->descr->name );
      return;
    }

    if( !*argument ) {
      fsend( ch, "What do you want %s's keywords set to?", player->descr->name );
      return;
    }
    
    if( is_name( player->pcdata->pfile->name, argument ) ) {
      fsend( ch, "%s's keywords should NOT include %s name.",
	    player->descr->name, player->His_Her() );
      return;
    }
    
    free_string( player->pcdata->tmp_keywords, MEM_PLAYER );
    player->pcdata->tmp_keywords = alloc_string( argument, MEM_PLAYER );

    fsend_color( player, COLOR_WIZARD, "%s has set your keywords to '%s'.",
		 ch, player->pcdata->tmp_keywords );
    fsend( ch, "You have set %s's keywords to '%s'.",
	   player->descr->name, player->pcdata->tmp_keywords );
    return;
  }

  if( is_set( flags, 6 ) ) {
    if( is_set( player->pcdata->pfile->flags, PLR_APPROVED ) ) {
      fsend( ch, "%s is approved, and must be unapproved before appearance can be changed.",
	     player->descr->name );
      return;
    }
    if( !*argument ) {
      send( ch, "What do you want %s's appearance set to?\n\r", player->descr->name );
      return;
    }

    if( !strncasecmp( argument, "a ", 2 )
	|| !strncasecmp( argument, "an ", 3 ) ) {
      send( ch, "Appearances must not start with a or an.\n\r" );
      return;
    }
    
    if( ispunct( argument[ strlen( argument )-1 ] ) ) {
      send( ch, "Appearances must not end with punctuation.\n\r" );
      return;
    }
    
    if( strlen( argument ) >= 50 ) {
      send( ch, "Appearances must be less than 50 characters.\n\r" );
      return;
    }
  
    free_string( player->pcdata->tmp_short, MEM_PLAYER );
    char *new_short = alloc_string( argument, MEM_PLAYER );
    *new_short = tolower( *new_short );
    player->pcdata->tmp_short = new_short;
    
    fsend_color( player, COLOR_WIZARD, "%s has set your appearance to '%s'.",
		 ch, player->pcdata->tmp_short );
    fsend( ch, "You have set %s's appearance to '%s'.",
	   player->descr->name, player->pcdata->tmp_short );
    return;
  }

  if( !is_set( flags, 0 ) ) {
    if( imm->player_edit && player != imm->player_edit ) {
      page( ch, "[Note: you are still editing %s]\n\r\n\r", imm->player_edit->descr->name );
    }
    page( ch, "   Name: %s\n\r", player->descr->name );
    page( ch, "   Race: %s\n\r", race_table[ player->shdata->race ].name );
    page( ch, "  Class: %s\n\r", clss_table[ player->pcdata->clss ].name );
    page( ch, "    Sex: %s\n\r", sex_name[ player->sex ] );

    page( ch, "\n\r[Current]\n\r" );
    page( ch, "  Appearance: %s\n\r", player->descr->singular );
    page( ch, "    Keywords: %s\n\r", player->descr->keywords );
    page( ch, "\n\r[New]\n\r" );
    page( ch, "  Appearance: %s\n\r", player->pcdata->tmp_short );
    page( ch, "    Keywords: %s\n\r\n\r", player->pcdata->tmp_keywords );
    page( ch, "[Description]\n\r" );
    page( ch, player->descr->complete );
    return;
  }
  
  if( !*argument
      || ( player = one_player( ch, argument, "approve",
				(thing_array*) &player_list ) ) ) {
    
    if( player->pcdata->tmp_short != empty_string ) {
      free_string( player->descr->singular, MEM_DESCR );
      player->descr->singular = player->pcdata->tmp_short;
      player->pcdata->tmp_short = empty_string;
    }
    
    if( player->pcdata->tmp_keywords != empty_string ) {
      free_string( player->descr->keywords, MEM_DESCR );
      player->descr->keywords = player->pcdata->tmp_keywords;
      player->pcdata->tmp_keywords = empty_string;
    }
    
    fsend( ch, "You have approved %s.", player );
    fsend_color( player, COLOR_WIZARD, "%s has approved you.", ch );
    
    remove( request_app, player );
    
    player_log( player, "Approved by %s.", ch->real_name( ) );
    player_log( ch, "Approved %s.", player->real_name( ) );
    
    snprintf( tmp, THREE_LINES, "%s approved.", player->Name() );
    snprintf( arg, MAX_INPUT_LENGTH, "%s approved by %s.", player->Name(), ch->Name() );
    info( LEVEL_AVATAR, tmp, invis_level( ch ), arg, IFLAG_ADMIN, 1, ch );
    
    set_bit( player->pcdata->pfile->flags, PLR_APPROVED );
  }

  if( imm->player_edit ) {
    if( player == imm->player_edit ) {
      fsend( ch, "You stop editing %s.", player->descr->name );
      imm->player_edit = 0;
      imm->account_edit = 0;
    } else {
      fsend( ch, "You are still editing %s.", imm->player_edit->descr->name );
    }
  }
}
