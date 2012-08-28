#include <ctype.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include "define.h"
#include "struct.h"


void press_return( link_data* link )
{
  send( link, "\n\rPress <return> to continue." );     
}


bool no_input( link_data* link, const char *argument )
{
  if( *argument )
    return false;

  send( link, "\n\r>> No input received - closing link <<\n\r" );
  close_socket( link, true );

  return true;
}


bool game_wizlocked( link_data* link )
{
  if( !wizlock ) 
    return false;

  help_link( link, "Game_Wizlocked" );
  close_socket( link, true );

  return true;
} 


bool game_godlocked( link_data* link )
{
  if( !godlock ) 
    return false;

  help_link( link, "Game_Godlocked" );
  close_socket( link, true );

  return true;
} 


static void remort_contents( thing_array& array, player_data *pl, int old_race );
static void remort_item( obj_data *obj, player_data *pl, int old_race );


static void remort_contents( thing_array& array, player_data *pl, int old_race )
{
  for( int i = array.size-1; i >= 0; --i )
    if( obj_data *content = object( array[i] ) )
      remort_item( content, pl, old_race );
}


static void remort_item( obj_data *obj, player_data *pl, int old_race )
{
  if( !obj->owner
      && obj->pIndexData->item_type != ITEM_MONEY ) {
    obj->owner = pl->pcdata->pfile;
    if( old_race != pl->shdata->race
	&& old_race < MAX_PLYR_RACE
	&& pl->shdata->race < MAX_PLYR_RACE
	&& is_set( obj->size_flags, SFLAG_CUSTOM )
	&& is_set( obj->size_flags, SFLAG_RACE )
	&& is_set( obj->size_flags, SFLAG_HUMAN + old_race )
	&& !is_set( obj->size_flags, SFLAG_HUMAN + pl->shdata->race ) ) {
      remove_bit( obj->size_flags, SFLAG_HUMAN + old_race );
      set_bit( obj->size_flags, SFLAG_HUMAN + pl->shdata->race );
    }
    consolidate( obj );
  }

  remort_contents( obj->contents, pl, old_race );
}


static void remort_items( int old_race, player_data *pl, Content_Array& array )
{
  while( !array.is_empty( ) ) {
    obj_data *obj = (obj_data*) array[0]->From( array[0]->Number( ) );
    obj->Select_All( );
    const int m = monetary_value( obj );
    if( m > 0 ) {
      pl->bank += m;
      obj->Extract( );
    } else {
      remort_item( obj, pl, old_race );
      obj->To( pl->locker );
    }
  }
}


static void finish_remort( link_data *link )
{
  player_data *remort = link->player;

  if( !remort->remort_name )
    return;

  const int old_race = remort->pcdata->pfile->race;

  // Prevent problems inserting pfile into array.
  char *pwd = alloc_string( remort->pcdata->pfile->pwd, MEM_PFILE );
  link->account = remort->pcdata->pfile->account;
  link->pfile = 0;
  pfile_data *pfile = remort->pcdata->pfile;
  remort->pcdata->pfile = 0;
  extract( pfile );

  player_data *pl = new player_data( remort->remort_name );
  pfile = new pfile_data( remort->remort_name );

  pl->pcdata->pfile = pfile;
  pl->link = link;
  
  link->player = pl;
  link->character = pl;
  link->pfile = pfile;
  
  remort->link = 0;

  //  modify_pfile( pl );

  // Copy old password.
  pfile->pwd = pwd;

  // Copy terminal type and colors.
  pl->pcdata->terminal = remort->pcdata->terminal;
  vcopy( pl->pcdata->color, remort->pcdata->color, MAX_COLOR );

  // Grab old strings.
  pl->pcdata->prompt = remort->pcdata->prompt;
  remort->pcdata->prompt = 0;
  pl->pcdata->dictionary = remort->pcdata->dictionary;
  remort->pcdata->dictionary = 0;
  pl->pcdata->journal = remort->pcdata->journal;
  remort->pcdata->journal = 0;

  // Copy creation variables.
  pl->sex = remort->sex;
  pl->pcdata->clss = remort->pcdata->clss;
  pl->shdata->race = remort->shdata->race;
  pl->pcdata->speaking = remort->pcdata->speaking;
  pl->shdata->strength = remort->shdata->strength;
  pl->shdata->intelligence = remort->shdata->intelligence;
  pl->shdata->wisdom = remort->shdata->wisdom;
  pl->shdata->dexterity = remort->shdata->dexterity;
  pl->shdata->constitution = remort->shdata->constitution;
  pl->shdata->alignment = remort->shdata->alignment;

  // Probably just a language skill... from creation process.
  for( int i = 0; i < MAX_SKILL_CAT; ++i ) {
    vcopy( pl->shdata->skills[i], remort->shdata->skills[i], table_max[ skill_table_number[ i ] ] );
  }

  // Preserve mail.
  pl->pcdata->mail = remort->pcdata->mail;
  remort->pcdata->mail = 0;

  // Retain completed quests.
  /*
  pl->pcdata->quest_pts = remort->pcdata->quest_pts;
  for( int i = 0; i < MAX_QUEST; ++i ) {
    if( remort->pcdata->quest_flags[i] == QUEST_DONE ) {
      pl->pcdata->quest_flags[i] = QUEST_DONE;
    }
  }
  */

  // Move items and money to remort's bank.
  remort_items( old_race, pl, remort->contents );
  remort_items( old_race, pl, remort->wearing );
  remort_items( old_race, pl, remort->locker );

  pl->bank += remort->bank;

  pl->remort = remort->remort;

  const bool name_diff = strcasecmp( remort->descr->name, pl->descr->name );

  if( name_diff ) {
    int i = pntr_search( remort_array, max_remort, remort->descr->name );
    if( i <= 0 ) {
      i = -i-1;
      record_new( sizeof( char* ), MEM_BADNAME );
      const char *name = alloc_string( remort->descr->name, MEM_BADNAME );
      insert( remort_array, max_remort, name, i );
      save_remort( );
    }
  }

  char *tmp = static_string( );
  snprintf( tmp, THREE_LINES, "Character %s remorted as %s on account %s.",
	    remort->descr->name,
	    pl->descr->name,
	    link->account->name );
  info( LEVEL_IMMORTAL, empty_string, 0, tmp, IFLAG_LOGINS, 2, pl );

  sprintf_minutes( tmp, remort->time_played( ) );

  player_log( pl, "Remort from %s.", remort->descr->name );
  player_log( pl, "Time played: %s.", tmp );

  if( name_diff ) {
    player_log( remort, "Remort as %s.", pl->descr->name );
    player_log( remort, "Time played: %s.", tmp );
  }

  purge( remort );

  // Save mail, since purge() above may have deleted same-name mail file.
  save_mail( pl->pcdata->pfile, pl->pcdata->mail );
}


/*
 *   ENTERING GAME ROUTINE
 */


room_data* get_temple( char_data* ch )
{
  room_data *room  = 0;
  
  if( ch->shdata->race < MAX_PLYR_RACE ) 
    room = get_room_index( plyr_race_table[ ch->shdata->race ].start_room[ ch->Align_Good_Evil() ] );

  if( !room )
    room = get_room_index( ROOM_CHIIRON_TEMPLE );

  return room;
}


room_data* get_portal( char_data* ch )
{
  room_data *room  = 0;
  
  if( ch->shdata->race < MAX_PLYR_RACE ) 
    room = get_room_index( plyr_race_table[ ch->shdata->race ].portal );
  
  if( !room )
    room = get_temple( ch );

  return room;
}


void enter_room( char_data* ch )
{
  room_data *prison = get_room_index( ROOM_PRISON, false );

  //  if( ( !is_set( ch->pcdata->pfile->flags, PLR_CRASH_QUIT )
  //	&& current_time-boot_time > 30*60 )
  if( !is_set( ch->pcdata->pfile->flags, PLR_CRASH_QUIT )
      || ch->pcdata->pfile->last_on > boot_time		// logged on since reboot
      || ch->Level() >= LEVEL_APPRENTICE
      || !is_set( ch->pcdata->pfile->flags, PLR_PORTAL )
      || prison && ch->was_in_room == prison ) {
    ch->To( ch->was_in_room ? ch->was_in_room : get_temple( ch ) );
    return;
  }
  
  room_data *room = get_portal( ch );

  for( int i = 0; i < ch->followers.size; i++ ) {
    char_data *pet = ch->followers[i]; 
    if( pet->was_in_room == ch->was_in_room )
      pet->was_in_room = room;
  }

  ch->was_in_room = 0;
  ch->To( room );
}


static void enter_game( link_data *link )
{
  char          tmp1  [ TWO_LINES ];
  char          tmp2  [ TWO_LINES ];
  player_data *pl = link->player;
  pfile_data *pfile  = pl->pcdata->pfile;
  int              i;

  if( pl->Level() == 0 ) 
    new_player( player( pl ) );
  else
    enter_room( pl );

  if( pl->position == POS_FIGHTING )
    pl->position = POS_STANDING;
  
  pl->pcdata->burden = pl->get_burden( );

  init_affects( pl );

  snprintf( tmp1, TWO_LINES, "%s@%s has connected.", pl->descr->name, pl->link->host );
  snprintf( tmp2, TWO_LINES, "%s has connected.", pl->descr->name );

  info( invis_level( pl ), tmp2, LEVEL_DEMIGOD, tmp1, IFLAG_LOGINS, 1, pl );

  fsend_seen( pl, "%s is here, although you didn't see %s arrive.",
	      pl, pl->Him_Her() );

  clear_screen( pl );
  show_room( pl, pl->in_room, false, false );

  for( i = 0; i < pl->followers.size; i++ ) {
    char_data *pet = pl->followers[i];
    if( !pet->was_in_room )
      pet->was_in_room = get_temple( pl );
    pet->To( pet->was_in_room );
    fsend_seen( pet, "%s appears, as if from nowhere.", pet );
  }      

  send( pl, "\n\r" );
  send_centered( pl, "---==|==---" );
  send( pl, "\n\r" );

  if( *pfile->last_host ) {
    send_centered( pl, "Last login was %s from %s.",
		   ltime( pfile->last_on, false, pl ), pfile->last_host );
  } else {
    send_centered( pl, "Connection is from %s.", pl->link->host );
  }
  
  if( pfile->guesses > 0 ) {
    send( pl, "\n\r" );
    send_centered( pl, "INCORRECT PASSWORD ATTEMPTS: %d",
		   pfile->guesses );
    pfile->guesses = 0;
  }
  
  if( pfile->account )
    pfile->account->last_login = current_time;
  
  if( strcasecmp( pl->link->host, pfile->last_host ) ) {
    remove_list( site_list, site_entries, pfile );
    free_string( pfile->last_host, MEM_PFILE );
    pfile->last_host = alloc_string( pl->link->host, MEM_PFILE );
    add_list( site_list, site_entries, pfile );
  }
  
  mail_message( pl );
  auction_message( pl );
  recent_notes( pl );
  request_message( pl );

  set_bit( pfile->flags, PLR_CRASH_QUIT );
  set_bit( pl->status, STAT_IN_GROUP );

  // Copy updates into save.cc:read_pet().
  remove_bit( pl->status, STAT_BERSERK );
  remove_bit( pl->status, STAT_FOCUS );
  remove_bit( pl->status, STAT_REPLY_LOCK );
  remove_bit( pl->status, STAT_FORCED );
  remove_bit( pl->status, STAT_NO_SNOOP );
  remove_bit( pl->status, STAT_GARROTING );
  remove_bit( pl->status, STAT_RESPOND );
  remove_bit( pl->status, STAT_LEADER );
  remove_bit( pl->status, STAT_FOLLOWER );
  remove_bit( pl->status, STAT_NOFOLLOW );
  remove_bit( pl->status, STAT_SNUCK );
  remove_bit( pl->status, STAT_WAITING );
  remove_bit( pl->status, STAT_HOLD_POS );
  remove_bit( pl->status, STAT_STOOD );
  remove_bit( pl->status, STAT_FLEE_FROM );
  remove_bit( pl->status, STAT_GROUP_LOOTER );

  pl->timer = current_time;

  reconcile_recognize( pl );
}


/*
 *   NANNY ROUTINES
 */


void nanny_intro( link_data* link, const char *argument )
{
  link->account = 0;

  for( ; !isalnum( *argument )
	 && *argument != '+'
	 && *argument != '-'
	 && *argument;
       ++argument );
  
  switch( atoi( argument ) ) {
  case 1:
    help_link( link, "Acnt_Menu" );
    send( link, "                   Choice: " );
    link->connected = CON_ACNT_MENU;
    return;
  case 2:
    if( !game_wizlocked( link )
	&& !game_godlocked( link ) ) {
      help_link( link, "Enter_Acnt" );
      send( link, "Account: " );
      link->connected = CON_ACNT_ENTER;
    }
    return;
  case 3:
    help_link( link, "Features_1" );      
    link->connected = CON_PAGE;
    return;
  case 4:
    help_link( link, "Policy_1" );
    link->connected = CON_POLICIES;
    return;
  case 5:
    help_link( link, "Problems" );
    link->connected = CON_PAGE;
    return;
  }
 
  bool suppress_echo = ( *argument == '-' );

  if( suppress_echo )
    ++argument;

  if( !*argument ) {
    send( link, "\n\r>> No choice made - closing link. <<\n\r" ); 
    close_socket( link, true );
    return;
  }

  pfile_data *pfile = find_pfile_exact( argument );  
  link->pfile  = pfile;

  if( !pfile ) {
    help_link( link, "Unfound_Char" );
    link->connected = CON_PAGE;
    return;
  }

  if( ( pfile->trust < LEVEL_APPRENTICE
	&& game_wizlocked( link ) )
      || ( pfile->trust < LEVEL_GOD
	   && game_godlocked( link ) ) )
    return;

  send( link, "                 Password: " );

  if( !suppress_echo ) {
    send( link, echo_off_str ); 
    link->connected = CON_PASSWORD_ECHO;
  } else {
    link->connected = CON_PASSWORD_NOECHO;
  }
}


/*
 *   NAME/PASSWORD
 */


void nanny_old_password( link_data* link, const char *argument )
{
  char              buf  [ TWO_LINES ];
  char              buf1 [ ONE_LINE ];
  player_data*       pl  = link->player;
  link_data*   link_old;

  send( link, "\n\r" );

  if( link->connected == CON_PASSWORD_ECHO )
    send( link, echo_on_str ); 

  if( strcmp( argument, link->pfile->pwd ) ) {
    if( *argument && link->pfile->guesses++ > 25 ) {
      bug( "Attempting to crack password for %s?", link->pfile->name );
      bug( "--     Site: %s", link->host );
      bug( "-- Attempts: %d", link->pfile->guesses );
      bug( "--    Guess: %s", argument );
    }
    help_link( link, "Wrong_Password" );
    close_socket( link, true );
    return;
  }

  if( is_banned( link->pfile->account, link ) ) {
    snprintf( buf, TWO_LINES, "Login attempt on banned account \"%s\", player \"%s\".",
	      link->pfile->account->name, link->pfile->name );
    info( LEVEL_DEMIGOD, empty_string, 0, buf, IFLAG_LOGINS, 3 );
    return;
  }

  if( is_set( link->pfile->flags, PLR_DENY ) ) {
    help_link( link, "Player_Denied" );
    snprintf( buf, TWO_LINES, "Login attempt by player \"%s\" denied.", link->pfile->name );
    info( LEVEL_DEMIGOD, empty_string, 0, buf, IFLAG_LOGINS, 3 );
    close_socket( link, true );
    return;
  }

  for( link_old = link_list; link_old; link_old = link_old->next ) 
    if( link_old != link
	&& link_old->player
	&& link_old->pfile == link->pfile
	&& link_old->past_password() ) {
      send( link, "Already playing!\n\rDisconnect previous link? " );
      link->connected = CON_DISC_OLD;
      return;
    }

  if( player_data *pl_old = find_player( link->pfile ) ) {
    link->character = pl_old;
    link->player = pl_old;
    pl_old->link = link;
    pl_old->timer = current_time;
    if( strcasecmp( link->host, link->pfile->last_host ) ) {
      remove_list( site_list, site_entries, link->pfile );
      free_string( link->pfile->last_host, MEM_PFILE );
      link->pfile->last_host = alloc_string( link->host, MEM_PFILE );
      add_list( site_list, site_entries, link->pfile );
    }
    link->set_playing();
    snprintf( buf, TWO_LINES, "%s@%s has reconnected.", pl_old->descr->name, link->host );
    snprintf( buf1, ONE_LINE, "%s has reconnected.", pl_old->descr->name );
    info( invis_level( pl_old ), buf1, LEVEL_DEMIGOD, buf, IFLAG_LOGINS, 1, pl_old );
    setup_screen( pl_old );
    send( pl_old, "Reconnecting.\n\r" );
    fsend_seen( pl_old, "%s suddenly starts paying attention.", pl_old );
    return;
  }

  if( !load_char( link, link->pfile->name, PLAYER_DIR ) ) {
    send( link, "\n\r+++ Error getting player file +++\n\r" );
    bug( "Error finding player file for" );
    bug( "-- Player = '%s'", link->pfile->name );
    close_socket( link );
    return;
  }

  pl = link->player;

  // Remort.
  if( is_set( pl->pcdata->pfile->flags, PLR_REMORT ) ) {
    if( pl->Level() == 90 ) {
      send( link, "\n\r" );
      clear_screen( pl );
      help_link( link, "login_remort" );
      send( link, "\n\r\n\rName: " );
      
      link->connected = CON_GET_NEW_NAME;
      return;
    }
    bug( "Remort: player %s is level %d.", pl->descr->name, pl->Level() );
    remove_bit( pl->pcdata->pfile->flags, PLR_REMORT );
    pl->Save( false );
  }

  // IMOTD help file may have permissions required.
  if( get_trust( pl ) < LEVEL_APPRENTICE
      || !find_help( pl, "imotd" ) ) {
    nanny_imotd( link, "" );
    return;
  }

  send( link, "\n\r" );
  clear_screen( pl );
  help_link( link, "imotd" );
  press_return( link );

  link->connected = CON_READ_IMOTD;
}
    

/*
 *   MESSAGE OF THE DAY ROUTINES
 */


void nanny_imotd( link_data* link, const char * )
{
  send( link, "\n\r" );
  clear_screen( link->character );

  help_link( link, "motd" );
  press_return( link );

  link->connected = CON_READ_MOTD;
}


void nanny_motd( link_data* link, const char * )
{
  send( link, "\n\r" );
  finish_remort( link );
  setup_screen( link->player );
  link->set_playing();
  enter_game( link );
}


void nanny_disc_old( link_data* link, const char *argument )
{
  link_data* link_prev;

  if( toupper( *argument ) != 'Y' ) {
    send( link, "Ok.  Good Bye then.\n\r" );
    close_socket( link, true );
    return;
  }

  for( link_prev = link_list; link_prev; link_prev = link_prev->next ) 
    if( link_prev != link
	&& link_prev->player
	&& link_prev->pfile == link->pfile
	&& link_prev->past_password() ) 
      break;

  if( !link_prev ) {
    link->connected = CON_PASSWORD_NOECHO;
    nanny( link, link->pfile->pwd );
    return;
  }
  
  swap( link_prev->channel, link->channel );
  swap( link_prev->host,    link->host );

  send( link, "\n\r\n\r+++ Link closed by new login +++\n\r" );
  close_socket( link, true );

  switch( link_prev->connected ) {
    case CON_PLAYING    :
      show_room( link_prev->character, link_prev->character->in_room, false, false );
      break;
    case CON_READ_MOTD  : 
    case CON_READ_IMOTD :
      nanny_motd( link_prev, "" );
      break;
  } 
}
