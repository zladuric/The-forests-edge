#include <ctype.h>
#include <dirent.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <unistd.h>
#include "define.h"
#include "struct.h"


const char *imm_title [] = { "Avatar", "Apprentice", "Builder",
  "Architect", "Immortal", "Spirit", "Angel", "Demigod", "God" };

const char *permission_name [ MAX_PERMISSION ] = {
  "all_mobs", "all_objects", "all_rooms", "approve",
  "basic", "build_chan", "commands", "echo", "god_chan",
  "goto", "help_files", "imm_chan",
  "lists", "load_objects", "misc_tables", "mobs", "noteboard",
  "objects", "players", "quests",
  "reimb_exp", "reimb_equip", "rooms", "shutdown", "accounts",
  "snoop", "spells",
  "socials", "transfer", "unfinished", "write_all", "write_areas",
  "avatar_chan", "clans", "rtables", "disabled", "force_players",
  "admin_chan" };


/*
 *   PERMISSION ROUTINES
 */


int invis_level( const char_data *ch )
{
  const wizard_data *imm = wizard( ch );

  if( !imm
      || imm->species
      || imm->pcdata->pfile->trust < LEVEL_APPRENTICE
      || !is_set( imm->pcdata->pfile->flags, PLR_WIZINVIS ) )
    return 0;

  return imm->wizinvis;
}


bool has_holylight( const char_data *ch )
{
  //  if( !wizard( ch ) )
  if( !ch || !ch->pcdata )
    return false;

  return is_set( ch->pcdata->pfile->flags, PLR_HOLYLIGHT );
}


bool mortal( char_data* ch, char* command )
{
  if( !ch->species && ch->Level() >= LEVEL_APPRENTICE ) 
    return false;

  fsend( ch,
	 "To prevent abuse you are unable to use %s in mortal form.\n\r",
	 command );
  
  return true;
}


bool has_permission( const char_data* ch, int* flag, bool msg )
{
  if( flag[0] == 0 && flag[1] == 0 )
    return true;

  const wizard_data *imm;

  if( !ch || !( imm = wizard( ch ) ) )
    return false;

  if( is_god( ch ) )
    return true;
  
  for( int i = 0; i < 2; ++i )
    if( imm->pcdata->pfile->permission[i] & flag[i] )
      return true;
  
  if( msg ) 
    send( ch, "Permission denied.\n\r." );

  return false;
}


bool privileged( const char_data *ch, int level )
{
  if( !ch || ch->species || ch->Level( ) < level )
    return false;

  return !is_set( ch->pcdata->pfile->flags, PLR_NO_PRIVILEGES );
}


/*
 *   FIND LOCATION
 */


static room_data *find_location( char_data* ch, const char *argument )
{
  int i;

  if( number_arg( argument, i ) )
    return get_room_index( i );

  char_data *victim;

  if( ( victim = one_character( ch, argument, empty_string,
				(thing_array*) &player_list,
				(thing_array*) &mob_list ) ) )
    return victim->in_room;
  
  return 0;
}


/*
 *   TRUST
 */

 
void do_trust( char_data *ch, const char *argument )
{
  char           arg  [ MAX_INPUT_LENGTH ];
  player_data*  victim;
  int          level;

  in_character = false;

  argument = one_argument( argument, arg );

  if( !*arg
      || !*argument
      || !number_arg( argument, level ) ) {
    send( ch, "Syntax: trust <char> <level>.\n\r" );
    return;
  }

  if( !( victim = one_player( ch, arg, "trust",
			      (thing_array*) &player_list ) ) )
    return;

  if( level < 0 || level > MAX_LEVEL ) {
    send( ch, "Level must be 0 (reset) or 1 to %d.\n\r", MAX_LEVEL );
    return;
  }
  
  if( level > get_trust( ch ) ) {
    fsend( ch, "You can't trust %s beyond your trust level %d.",
	   victim, get_trust( ch ) );
    return;
  }

  /*
  if( level != 0 && level < victim->Level() ) {
    send( ch, "You can't trust %s less than %s experience level %d.\n\r",
	  victim, victim->His_Her(), ch->Level() );
    return;
  }
  */

  if( wizard_data *wiz = wizard( victim ) ) {
    wiz->wizinvis = max( wiz->wizinvis, level );
  }

  fsend( victim, "You are now trusted at level %d.", level );

  if( ch != victim ) 
    fsend( ch, "%s is now trusted at level %d.",
	   victim->real_name( ), level );
  
  player_log( victim, "Trust set from level %d to level %d by %s.",	    
	      victim->pcdata->trust,
	      level,
	      ch->real_name( ) );
  player_log( ch, "Trusted %s from level %d to level %d.",
	      victim->real_name( ),
	      victim->pcdata->trust,
	      level );

  victim->pcdata->trust = level;

  modify_pfile( victim );
  victim->Save( );
}


/*
 *   REIMB FUNCTION
 */


void do_reimburse( char_data* ch, const char *argument )
{
  char_data*     victim;
  int             flags;

  in_character = false;

  if( mortal( ch, "reimburse" ) )
    return;

  if( !get_flags( ch, argument, &flags, "delDrRwEW", "reimburse" ) )
    return;

  if( !( victim = one_player( ch, argument, "reimburse",
			      ch->array ) ) )
    return;
  
  if( victim == ch ) {
    send( ch, "You can't reimburse yourself.\n\r" );
    return;
  }

  if( flags == 0 ) {
    send( ch, "You must specify what to reimburse, see help.\n\r" );
    return;
  }

  if( is_set( flags, 4 )
      || is_set( flags, 5 ) ) {
    int level = is_set( flags, 5 ) ? ( victim->Level() ) : ( victim->Level( ) - 1 );
    level = max( 1, level );
    int exp = exp_for_level( level )/5;
    fsend( ch, "You charge %s for %d exp and one death.",
	   victim, exp );
    fsend_color( victim, COLOR_WIZARD, "%s charges you for %d exp and one death.",
		 ch, exp );
    
    player_log( victim, "Charged 1 death, %d exp by %s.",
		exp, ch->real_name( ) );
    
    ++victim->shdata->deaths;
    add_exp( victim, -exp );
    return;
  }
  
  if( is_set( flags, 0 )
      || is_set( flags, 3 ) ) {
    int level = is_set( flags, 3 ) ? ( victim->Level() + 1 ) : ( victim->Level( ) );
    int exp = exp_for_level( level )/5;
    fsend( ch, "You reimburse %s for %d exp and one death.",
	   victim, exp );
    fsend_color( victim, COLOR_WIZARD, "%s reimburses you for %d exp and one death.",
		 ch, exp );
    
    player_log( victim, "Reimbursed 1 death, %d exp by %s.",
		exp, ch->real_name( ) );
    
    --victim->shdata->deaths;
    add_exp( victim, exp );
    return;
  }
  
  link_data link;

  if( !load_char( &link, victim->descr->name, BACKUP_DIR ) ) {
    send( ch, "No backup of that player found.\n\r" );
    return;
  }

  if( is_set( flags, 7 )
      || is_set( flags, 8 ) ) {

    bool found = false;
    for( int i = 0; i < link.player->followers; ++i ) {
      thing_array *array =  is_set( flags, 7 )
	                    ? &link.player->followers[i]->contents
                       	    : &link.player->followers[i]->wearing;
      if( !array->is_empty() ) {
	found = true;
	for( int i = array->size - 1; i >= 0; --i ) {
	  thing_data *thing = array->list[i];
	  thing = thing->From( thing->Number( ) );
	  thing->To( *ch->array );
	}
      }
    }

    if( found ) {
      const char *drop = ch->in_room->drop( );
      send( ch, "You scan the annals of history ...\n\r" );
      if( *drop ) {
	fsend( ch, "Finding the status of %s from earlier times you\
 duplicate %s equipment, dropping it %s.",
	       victim, victim->His_Her( ), drop );
      } else {
	fsend( ch, "Finding the status of %s from earlier times you\
 duplicate %s equipment, and drop it.",
	       victim, victim->His_Her( ) );
      }
	
      fsend_color( victim, COLOR_WIZARD, "You feel %s probing your history ...", ch );
      fsend_color( victim, COLOR_WIZARD,
		   "With a sweep of %s arms %s produces your old equipment.",
		   ch->His_Her( victim ), ch->He_She( victim ) );
    } else {
      fsend( ch, "The backup file shows no equipment for %s.", victim );
    }

  } else {
    
    thing_array *array = ( is_set( flags, 1 ) 
			   ? &link.player->contents 
			   : ( is_set( flags, 6 )
			       ? &link.player->wearing
			       : &link.player->locker ) );
    
    if( array->is_empty() ) {
      fsend( ch, "The backup file shows no equipment for %s.", victim );
    } else {   
      for( int i = array->size - 1; i >= 0; --i ) {
	thing_data *thing = array->list[i];
	thing = thing->From( thing->Number( ) );
	thing->To( *ch->array );
      }
      
      const char *drop = ch->in_room->drop( );

      send( ch, "You scan the annals of history ...\n\r" );

      if( *drop ) {
	fsend( ch, "Finding the status of %s from earlier times you\
 duplicate %s equipment, dropping it %s.",
	       victim, victim->His_Her( ), drop );
      } else {
	fsend( ch, "Finding the status of %s from earlier times you\
 duplicate %s equipment, and drop it.",
	       victim, victim->His_Her( ) );
      }
      
      fsend_color( victim, COLOR_WIZARD, "You feel %s probing your history ...", ch );
      fsend_color( victim, COLOR_WIZARD,
		   "With a sweep of %s arms %s produces your old equipment.",
		   ch->His_Her( victim ), ch->He_She( victim ) );
    }
  }
    
  link.player->Extract( );
  extracted.delete_list();
}


/*
 *   ROUTINES TO DEAL WITH MALCONTENTS
 */


static bool deny_loop( char_data *ch, int flag, const char *title )
{
  bool found = false;
  int trust = get_trust( ch );
  for( int i = 0; i < max_pfile; ++i ) {
    pfile_data *pfile = pfile_list[i];
    if( pfile->trust < trust
	&& is_set( pfile->flags, flag ) ) {
      if( !found ) {
	page_title( ch, title );
	page_underlined( ch, "%-14s  %-20s  %s\n\r", "Player", "Account", "Last On" );
	found = true;
      }
      page( ch, "%-14s  %-20s  %s\n\r",
	    pfile->name, pfile->account->name, ltime( pfile->last_on, false, ch )+4 );
    }
  }

  return found;
}


void do_deny( char_data *ch, const char *argument )
{
  in_character = false;

  int flags;
  if( !get_flags( ch, argument, &flags, "ramn", "deny" ) ) {
    return;
  }

  if( !*argument ) {
    bool found = deny_loop( ch, PLR_DENY, "Players Denied Connection" );
    found = deny_loop( ch, PLR_NO_AUCTION, "Players Denied Auction" ) || found;
    found = deny_loop( ch, PLR_NO_MAIL, "Players Denied Mail" ) || found;
    found = deny_loop( ch, PLR_NO_NOTES, "Players Denied Noteboards" ) || found;

    if( !found )
      send( ch, "No players are currently denied.\n\r" );

    return;
  }

  pfile_data *pfile = find_pfile( argument, ch );
  
  if( !pfile )
    return;

  if( pfile == ch->pcdata->pfile ) {
    send( ch, "You cannot deny yourself.\n\r" );
    return;
  }

  if( pfile->trust >= get_trust( ch ) ) {
    fsend( ch, "You cannot deny %s.", pfile->name );
    return;
  }

  const char *name;
  int flag;

  if( is_set( flags, 1 ) ) {
    name = "auction";
    flag = PLR_NO_AUCTION;
  } else if( is_set( flags, 2 ) ) {
    name = "mail";
    flag = PLR_NO_MAIL;
  } else if( is_set( flags, 3 ) ) {
    name = "noteboards";
    flag = PLR_NO_NOTES;
  } else {
    name = "connection";
    flag = PLR_DENY;
  }

  if( is_set( flags, 0 ) && !is_set( pfile->flags, flag ) ) {
    fsend( ch, "Player \"%s\" is not currently denied %s.", pfile->name, name );
    return;
  } else if( !is_set( flags, 0 ) && is_set( pfile->flags, flag ) ) {
    fsend( ch, "Player \"%s\" is already denied %s.", pfile->name, name );
    return;
  }

  bool loaded = false;
  player_data *pc = find_player( pfile );

  if( !pc ) {
    link_data link;
    link.connected = CON_PLAYING;
    if( !load_char( &link, pfile->name, PLAYER_DIR ) ) {
      bug( "Load_Char: error reading player file. (%s)", pfile->name );
      return;
    }
    pc = link.player;
    loaded = true;
  }

  char *tmp = static_string( );

  if( is_set( flags, 0 ) ) {
    remove_bit( pfile->flags, flag );
    fsend( ch, "Player \"%s\" is no longer denied %s.", pfile->name, name );
    char *tmp = static_string( );
    snprintf( tmp, THREE_LINES, "%s un-denied %s by %s.",
	      pc->descr->name, name, ch->Name() );
    if( !loaded ) {
      send( pc, "\n\r" );
      send_color( pc, COLOR_WIZARD, "Your %s privileges have been restored.", name );
      send( pc, "\n\r" );
      pc->Save( );
    }

  } else {
    set_bit( pfile->flags, flag );
    fsend( ch, "Player \"%s\" is now denied %s.", pfile->name, name );
    snprintf( tmp, THREE_LINES, "%s denied %s by %s.",
	      pc->descr->name, name, ch->Name() );
    if( !loaded ) {
      if( flag == PLR_DENY ) {
	send( pc, "\n\r" );
	send_color( pc, COLOR_WIZARD, "You have earned the displeasure of the gods." );
	send( pc, "\n\r" );
	send_color( pc, COLOR_WIZARD, "Your character is now denied access." );
	send( pc, "\n\r" );
	forced_quit( pc );
      } else {
	send( pc, "\n\r" );
	send_color( pc, COLOR_WIZARD, "Your %s privileges have been revoked.", name );
	send( pc, "\n\r" );
	pc->Save( );
      }
    }
  }
  
  
  if( loaded ) {
    send( ch, "\n\r" );
    send_centered( ch, "[ Player file was loaded from disk. ]" );
    pc->Save( false );
    pc->Extract();
    extracted.delete_list();
  }

  info( get_trust( ch ), empty_string, 0, tmp, IFLAG_ADMIN, 1, ch );
}


void disconnect( char_data *victim, char_data *ch )
{
  if( ch && get_trust( victim ) >= get_trust( ch ) ) {
    send( ch, "You can only disconnect those of lower trust.\n\r" );
    return;
  }
  
  if( !victim->link ) {
    if( ch )
      fsend( ch, "%s doesn't have a descriptor.", victim );
    return;
  }

  close_socket( victim->link, true );

  if( ch )
    fsend( ch, "Closed %s's socket.", victim );

  char *tmp = static_string( );
  if( ch ) {
    snprintf( tmp, THREE_LINES, "%s disconnected by %s.",
	      victim->descr->name, ch->Name() );
  } else {
    snprintf( tmp, THREE_LINES, "%s disconnected.",
	      victim->descr->name );
  }
  info( get_trust( ch ), empty_string, 0, tmp, IFLAG_ADMIN, 1, ch );
}


void do_disconnect( char_data *ch, const char *argument )
{
  in_character = false;

  char_data *victim;

  if( !( victim = one_player( ch, argument, "disconnect",
			      (thing_array*) &player_list ) ) )
    return;

  disconnect( victim, ch );
}


void do_imprison( char_data *ch, const char *argument )
{
  //  in_character = false;

  if( !*argument ) {
    send( ch, "Syntax: imprison <player>\n\r" );
    return;
  }

  pfile_data *pfile = find_pfile_exact( argument );

  if( !pfile ) {
    send( ch, "No such player found.\n\r" );
    send( ch, "The name must be spelled exactly and completely.\n\r" );
    return;
  }

  if( pfile == ch->pcdata->pfile ) {
    send( ch, "You cannot imprison yourself.\n\r" );
    return;
  }

   if( pfile->trust >= get_trust( ch ) ) {
    fsend( ch, "You cannot imprison %s.", pfile->name );
    return;
  }

  bool loaded = false;
  player_data *pc = find_player( pfile );
   
  if( !pc ) {
    link_data link;
    link.connected = CON_PLAYING;
    if( !load_char( &link, pfile->name, PLAYER_DIR ) ) {
      bug( "Load_Char: error reading player file. (%s)", pfile->name );
      return;
    }
    pc = link.player;
    loaded = true;
  }

  room_data *from = pc->in_room;

  if( !from
      && !( from = pc->was_in_room ) ) {
    fsend( ch, "%s cannot be imprisoned, since they are not yet in the game.", pfile->name );


  } else if( from->vnum == ROOM_PRISON ) {
    fsend( ch, "%s is already imprisoned.  Maybe you should apply thumbscrews?", pfile->name );

  } else {
    
    dismount( pc );
    
    if( pc->leader )
      stop_follower( pc );
    
    fsend_color_seen( pc, COLOR_WIZARD, "%s is mercilessly vaporized.", pc );
    
    if( pc->array )
      pc->From( );
    pc->To( get_room_index( ROOM_PRISON ) );
    //  set_bit( pc->pcdata->pfile->flags, PLR_FREEZE );
    
    fsend_color_seen( pc, COLOR_WIZARD, "%s joins you in your penance.", pc );
    send_color( pc, COLOR_WIZARD, "You have been imprisoned!" );
    send( pc, "\n\r" );
    
    leave_shadows( pc );
    pc->set_default_title( );
    
    if( pc->in_room ) {
      send( pc, "\n\r" );
      show_room( pc, pc->in_room, false, false );
    }
    fsend( ch, "%s imprisoned from %s (%d).",
	   pc->descr->name, from->name, from->vnum );
    
    char *tmp = static_string( );
    snprintf( tmp, THREE_LINES, "%s imprisoned by %s from %s (%d).",
	      pc->descr->name, ch->Name( ), from->name, from->vnum );
    info( get_trust( ch ), empty_string, 0, tmp, IFLAG_ADMIN, 1, ch );
    
    player_log( pc, "Imprisoned from %s (%d) by %s.",
		from->name, from->vnum,
		ch->descr->name );
    player_log( ch, "Imprisoned %s from %s (%d).",
		pc->descr->name,
		from->name, from->vnum );
    
    pc->Save( false );
  }

  if( loaded ) {
    send( ch, "\n\r" );
    send_centered( ch, "[ Player file was loaded from disk. ]" );
    pc->Extract();
    extracted.delete_list();
  }

  /*
  player_data *victim;
     
  if( !( victim = one_player( ch, argument, "imprison",
			      (thing_array*) &player_list ) ) )
    return;
  
  if( victim == ch ) {
    send( ch, "You can't imprison yourself!\n\r" );
    return;
  }

  if( get_trust( victim ) >= get_trust( ch ) ) {
    send( ch,
	  "You can't imprison someone of equal or higher trust level.\n\r" );
    return;
  }

  room_data *from = victim->in_room;

  if( !from ) {
    send( ch, "They are in limbo.\n\r" );
    return;
  }  
  
  dismount( victim );

  if( victim->leader )
    stop_follower( victim );

  fsend_color_seen( victim, COLOR_WIZARD, "%s is mercilessly vaporized.", victim );

  victim->From( );
  victim->To( get_room_index( ROOM_PRISON ) );
  //  set_bit( victim->pcdata->pfile->flags, PLR_FREEZE );

  fsend_color_seen( victim, COLOR_WIZARD, "%s joins you in your penance.", victim );
  send_color( victim, COLOR_WIZARD, "You have been imprisoned!" );

  leave_shadows( victim );
  victim->set_default_title( );
  victim->Save( );

  send( victim, "\n\r" );
  show_room( victim, victim->in_room, false, false );
  fsend( ch, "%s imprisoned from %s (%d).",
	 victim, from->name, from->vnum );

  char *tmp = static_string( );
  snprintf( tmp, THREE_LINES, "%s imprisoned by %s from %s (%d).",
	    victim->Name(), ch->Name(), from->name, from->vnum );
  info( get_trust( ch ), empty_string, 0, tmp, IFLAG_ADMIN, 1, ch );

  player_log( victim, "Imprisoned from %s (%d) by %s.",
	      from->name, from->vnum,
	      ch->descr->name );
  player_log( ch, "Imprisoned %s from %s (%d).",
	      victim->descr->name,
	      from->name, from->vnum );
  */
}


void do_pardon( char_data* ch, const char *argument )
{
  /*
  char_data *victim;
  int i;

  in_character = false;

  if( !( victim = one_player( ch, argument, "pardon",
    (thing_array*) &player_list ) ) )
    return;

  if( get_trust( victim ) > get_trust( ch ) ) {
    send( ch,
      "You can only pardon those of lower or equal level.\n\r" );
    return;
    }
    
  for( i = 0; i < table_max[ TABLE_NATION ]; i++ )
    victim->pcdata->pfile->reputation[i] = 500;

  if( ch != victim ) {
  send_color( victim, COLOR_WIZARD, "%s has pardoned you.", ch );
  send( victim, "\n\r" );
    }
  
  send( ch, "Ok.\n\r" );

  char *tmp = static_string( );
  snprintf( tmp, THREE_LINES, "%s pardoned by %s.",
	    victim->Name(), ch->Name() );
  info( get_trust( ch ), empty_string, 0, tmp, IFLAG_ADMIN, 1, ch );
  */
}


void do_freeze( char_data *ch, const char *argument )
{
  player_data*  victim;

  in_character = false;

  if( !( victim = one_player( ch, argument, "pardon",
			      (thing_array*) &player_list ) ) )
    return;

  if( get_trust( ch ) <= get_trust( victim ) ) {
    send( ch, "You failed.\n\r" );
    return;
  }

  switch_bit( victim->pcdata->pfile->flags, PLR_FREEZE );

  char *tmp = static_string( );
  if( is_set( victim->pcdata->pfile->flags, PLR_FREEZE ) ) {
    send_color( victim, COLOR_WIZARD, "You can't do anything!" );
    send( victim, "\n\r" );
    send( ch, "FREEZE set.\n\r" );
    snprintf( tmp, THREE_LINES, "%s frozen by %s.",
	      victim->Name(), ch->Name() );
  } else {
    send_color( victim, COLOR_WIZARD, "You can play again." );
    send( victim, "\n\r" );
    send( ch, "FREEZE removed.\n\r" );
    snprintf( tmp, THREE_LINES, "%s unfrozen by %s.",
	      victim->Name(), ch->Name() );
  }
  info( get_trust( ch ), empty_string, 0, tmp, IFLAG_ADMIN, 1, ch );

  victim->Save( );
}


void do_peace( char_data* ch, const char *)
{
  if( mortal( ch, "peace" ) )
    return;

  send( ch, "You wave your hand in a gesture for silence.\n\r" );
  fsend_color_seen( ch, COLOR_WIZARD, "%s waves %s hand in a gesture for silence.",
		    ch, ch->His_Her( ) );
  
  for( int i = 0; i < *ch->array; i++ ) {
    if( char_data *rch = character( ch->array->list[i] ) ) {
      remove_bit( rch->status, STAT_BERSERK );
      remove_bit( rch->status, STAT_FOCUS );
      set_fighting( rch, 0 );
      rch->aggressive.clear( );
      stop_events( rch, execute_leap );
      disrupt_spell( rch );
    }
  }
}


/*
 *   BAMFIN/BAMFOUT
 */


static const char* bamfin( wizard_data* wizard )
{
  return wizard->bamfin != empty_string
    ? wizard->bamfin : "materializes from the ether.";
}


static const char* bamfout( wizard_data* imm )
{
  return imm->bamfout != empty_string
    ? imm->bamfout : "disappears into the ether.";
}


static const char *bamfspace( const char *bamf )
{
  if( !bamf
      || !*bamf )
    return " ";

  const char c = *bamf;

  if( c == ','
      || c == '.'
      || c == '\''
      || c == ':' ) {
    return "";
  }

  return " ";
}


void do_bamfin( char_data* ch, const char *argument )
{
  wizard_data* imm  = wizard( ch );

  if( !imm )
    return;

  free_string( imm->bamfin, MEM_WIZARD );
  imm->bamfin = alloc_string( argument, MEM_WIZARD );

  send( ch, "Bamfin message set to:\n\r" );

  const char *bamf = bamfin( imm );

  fsend( ch, "%s%s%s",
	 ch->descr->name,
	 bamfspace( bamf ),
	 bamf );
}


void do_bamfout( char_data *ch, const char *argument )
{
  wizard_data* imm  = wizard( ch );

  if( !imm )
    return;

  free_string( imm->bamfout, MEM_WIZARD );
  imm->bamfout = alloc_string( argument, MEM_WIZARD );

  send( ch, "Bamfout message set to:\n\r" );

  const char *bamf = bamfout( imm );

  fsend( ch, "%s%s%s", ch->descr->name,
	 bamfspace( bamf ),
	 bamf );
}


/*
 *   GOTO
 */


static void exec_goto( char_data* victim, room_data* room, wizard_data* imm )
{
  victim->Select( 1 );

  if( imm ) {
    const char *bamf = bamfout( imm );
    fsend_color_seen( victim, COLOR_WIZARD, "%s%s%s",
		      victim,
		      bamfspace( bamf ),
		      bamf );
  } else {
    fsend_color_seen( victim, COLOR_WIZARD, "%s suddenly disappears!", victim );
  }

  victim->From( );
  victim->To( room );

  if( imm ) {
    const char *bamf = bamfin( imm );
    fsend_color_seen( victim, COLOR_WIZARD, "%s%s%s",
		      victim,
		      bamfspace( bamf ),
		      bamf );
  } else {
    fsend_color_seen( victim, COLOR_WIZARD, "%s suddenly appears!", victim );
  }

  show_room( victim, room, false, false );
}


void raw_goto( wizard_data *imm, room_data *from, room_data *to )
{
  if( from == to )
    return;

  exec_goto( imm, to, imm );
  
  char_data *rch;
  for( int i = from->contents-1; i >= 0; i-- ) 
    if( ( rch = character( from->contents[i] ) )
	&& rch->leader == imm
	&& is_set( rch->status, STAT_PET ) )
      exec_goto( rch, to, 0 );
}


void do_goto( char_data* ch, const char *argument )
{
  wizard_data *imm;

  if( !ch
      || !( imm = wizard( ch ) ) )
    return;

  if( !*argument ) {
    send( ch, "Usage: goto <location>\n\r" );
    return;
  }
 
  in_character = false;

  room_data *to;
  if( !( to = find_location( ch, argument ) ) ) {
    send( ch, "No such location.\n\r" );
    return;
  }
  
  in_character = true;

  room_data *from;
  if( ( from = ch->in_room ) == to ) {
    send( ch, "You are already there!\n\r" );
    return;
  }
  
  if( is_set( to->room_flags, RFLAG_OFFICE )
      && !is_demigod( ch )
      && to->vnum != imm->office ) {
    send( ch, "That location belongs to another immortal.\n\r" );
    return;
  } 

  raw_goto( imm, from, to );
}


/*
 *   TRANSFER
 */


void transfer( char_data* ch, char_data* victim, room_data* room )
{
  char              tmp  [ TWO_LINES ];
  Content_Array*  where  = victim->array;
  char_data*        rch;

  fsend_color_seen( victim, COLOR_WIZARD, "%s disappears in a mushroom cloud.", victim );
  victim->From( );
  victim->To( room );
  fsend_color_seen( victim, COLOR_WIZARD, "%s arrives from a puff of smoke.", victim );

  snprintf( tmp, TWO_LINES, "** %s has transferred you. **", ch->Name( victim ) );
  tmp[3] = toupper( tmp[3] );
  send_color( victim, COLOR_WIZARD, tmp );
  send( victim, "\n\r\n\r" );

  if( !victim->was_in_room ) {
    show_room( victim, room, false, false );
  }

  for( int i = *where-1; i >= 0; i-- ) 
    if( ( rch = character( where->list[i] ) )
	&& rch->leader == victim
	&& is_set( rch->status, STAT_PET ) )
      transfer( ch, rch, room );
}


void do_transfer( char_data* ch, const char *argument )
{
  wizard_data *imm;

  if( !ch
      || !( imm = wizard( ch ) ) )
    return;

  char              arg  [ MAX_INPUT_LENGTH ];
  char_data*     victim;
  player_data*   player;
  room_data*       room;

  argument = one_argument( argument, arg );

  if( !*arg ) {
    send( ch, "Syntax: transfer <char> [room]\n\r" );
    return;
  }
  
  in_character = false;

  if( !*argument ) {
    room = ch->in_room;
  } else {
    if( !( room = find_location( ch, argument ) ) ) {
      send( ch, "No such location.\n\r" );
      return;
    }
  }
  
  if( is_set( room->room_flags, RFLAG_OFFICE )
      && !is_god( ch )
      && room->vnum != imm->office ) {
    send( ch, "That location belongs to another immortal.\n\r" );
    return;
  } 

  if( !strcasecmp( arg, "all" ) ) {
    in_character = true;
    for( int i = 0; i < player_list; i++ ) {
      player = player_list[i];
      if( player->In_Game( )
	  && player->Level() < LEVEL_APPRENTICE
	  && get_trust( player ) < get_trust( ch )
	  && player->in_room != room ) {
	transfer( ch, player, room );
      }
    }
    fsend( ch, "All players transferred to %s (%d).",
	   room->name, room->vnum );
    char *tmp = static_string( );
    snprintf( tmp, THREE_LINES, "All players transferred to %s (%d) by %s.",
	      room->name, room->vnum, ch->Name() );
    info( get_trust( ch ), empty_string, 0, tmp, IFLAG_ADMIN, 1, ch );
    return;
  }
  
  if( !( victim = one_character( ch, arg, "transfer",
				 ch->array,
				 (thing_array*) &player_list,
				 (thing_array*) &mob_list ) ) )
    return;
  
  if( victim == ch ) {
    send( ch, "You can't transfer yourself!\n\r" );
    return;
  }
  
  /*
  if( get_trust( victim ) >= get_trust( ch ) ) {
    send( ch,
	  "You can't transfer someone of equal or higher trust level.\n\r" );
    return;
  }
  */

  if( get_trust( victim ) >= get_trust( ch )
      || ( is_pet( victim )
	   && victim->leader != ch
	   && get_trust( victim->leader ) >= get_trust( ch ) ) ) {
    fsend( ch, "You can't transfer %s.", victim );
    return;
  }

  room_data *from = victim->in_room;

  if( !from ) {
    fsend( ch, "%s is in limbo.", victim );
    return;
  }
  
  if( from == room ) {
    fsend( ch, "%s is already at %s (%d).",
	   victim, room->name, room->vnum );
    return;
  }

  in_character = true;

  if( victim->rider )
    dismount( victim->rider );

  if( victim->mount
      && victim->mount->leader != victim ) {
    dismount( victim );
  }

  transfer( ch, victim, room );
 
  fsend( ch, "Transferred %s from %s (%d) to %s (%d).",
	 victim->Seen_Name( ch ),
	 from->name, from->vnum,
	 room->name, room->vnum );

  char *tmp = static_string( );
  snprintf( tmp, THREE_LINES, "%s transferred from %s (%d) to %s (%d) by %s.",
	    victim->Name(),
	    from->name, from->vnum,
	    room->name, room->vnum,
	    ch->Name() );
  info( get_trust( ch ), empty_string, 0, tmp, IFLAG_ADMIN, 1, ch );
}


/*
 *   ECHO COMMANDS
 */


void echo( const char* string )
{
  int n = strlen( string );

  fprintf( stdout, string );

  for( const link_data *link = link_list; link; link = link->next ) {
    write( link->channel, string, n );
  }
}


void do_echo( char_data *ch, const char *argument )
{
  if( mortal( ch, "echo" ) )
    return;

  if( !*argument  ) {
    send( ch, "Usage: echo <text>\n\r" );
    return;
  }

  for( const link_data *link = link_list; link; link = link->next ) {
    if( link->connected == CON_PLAYING ) {
      write( link->channel, "\7", 1 );
      fsend_color( link->character, COLOR_WIZARD, argument );
    }
  }
}


void do_recho( char_data* ch, const char *argument )
{
  if( mortal( ch, "echo" ) )
    return;

  if( !*argument ) {
    send( ch, "Syntax: recho <text>\n\r" );
    return;
  }

  for( int i = 0; i < *ch->array; i++ )
    if( char_data *rch = character( ch->array->list[i] ) )
      fsend_color( rch, COLOR_WIZARD, argument );
}


void do_beep( char_data* ch, const char *argument )
{
  if( mortal( ch, "beep" ) )
    return;

  if( !strcasecmp( argument, "all" ) ) {
    unsigned count = 0;
    for( int i = 0; i < player_list; ++i ) {
      player_data *pl = player_list[i];
      if( pl->In_Game( )
	  && pl != ch
	  && get_trust( pl ) < get_trust( ch ) ) {
	send( pl, "\7" );
	++count;
      }
    }
    send( ch, "Beeped %u player%s.\n\r", count, count == 1 ? "" : "s" );
    return;
  }

  player_data *victim;
  
  if( !( victim = one_player( ch, argument, "beep", 
			      (thing_array*) &player_list ) ) )
    return;
  
  if( victim == ch ) {
    send( ch, "\7You beep yourself.\7\n\r" );
  } else {
    fsend_color( victim, COLOR_WIZARD, "\7%s has beeped you.\7", ch );
    fsend( ch, "You beep %s.", victim );
  }
}


void do_where( char_data* ch, const char *argument)
{
  char_data*   victim;
  bool          found  = false;

  in_character = false;

  for( link_data *link = link_list; link; link = link->next ) {
    if( link->connected == CON_PLAYING
	&& ( victim = link->character )
	&& victim != ch
	&& victim->pcdata
	&& victim->in_room
	&& victim->in_room->area == ch->in_room->area
	&& victim->Seen( ch ) ) {
      //	&& ch->Recognizes( victim ) )       {
      if( !found ) {
	page_title( ch, "Players near you" );
	found = true;
      }
      if( victim->species ) {
	page( ch, "%s (switched %s) %s (%d)\n\r",
	      victim->Seen_Name( ),
	      victim->pcdata->pfile->name,
	      victim->in_room->name,
	      victim->in_room->vnum );
      } else {
	page( ch, "%-14s %s (%d)\n\r",
	      victim->Seen_Name( ),
	      victim->in_room->name,
	      victim->in_room->vnum );
      }
    }
  }
  
  if( !found )
    page( ch, "No visible players in the area.\n\r" );
}


void do_purge( char_data* ch, const char *argument )
{
  if( mortal( ch, "purge" ) )
    return;

  if( !strcasecmp( argument, "self" ) ) {
    player_data *pc = (player_data*) ch;
    if( !pc->junked.is_empty( ) ) {
      send( ch, "Junk list cleared.\n\r" );
      stop_events( ch, execute_junk );
      extract( pc->junked );
    }
    if( ch->contents.is_empty( )
	 && ch->wearing.is_empty( ) ) {
      send( ch, "You aren't wearing or carrying anything.\n\r" );
      return;
    }
    fsend_color_seen( ch, COLOR_WIZARD, "%s utters an ancient rune.", ch );
    if( !ch->contents.is_empty( ) ) {
      send( ch, "Everything you are carrying turns to dust.\n\r" );
      fsend_color_seen( ch, COLOR_WIZARD, "Everything %s is carrying turns to dust.\n\r", ch );
      extract( ch->contents );
    }
    if( !ch->wearing.is_empty( ) ) {
      send( ch, "Everything you are wearing turns to dust.\n\r" );
      fsend_color_seen( ch, COLOR_WIZARD, "Everything %s is wearing turns to dust.\n\r", ch );
      extract( ch->wearing );
    }
    return;
  }

  if( !*argument ) {
    fsend_color( *ch->array, COLOR_WIZARD,
		 "%s utters an ancient rune, engulfing the room in a blinding light.",
		 ch );
    send( ch, "Room purged.\n\r" );
    
    for( int i = *ch->array-1; i >= 0; i-- ) {
      thing_data *thing = ch->array->list[i];
      if( char_data *victim = character( thing ) ) {
	if( victim->pcdata
	    || is_set( victim->status, STAT_PET ) && !victim->leader->species )
	  continue;

      } else if( obj_data *obj = object( thing ) ) {
	if( !obj->reset
	    && is_set( obj->extra_flags, OFLAG_NO_RESET ) )
	  continue;
	room_data *room = Room( ch->array->where );
	if( room
	    && !obj->reset
	    && is_set( room->room_flags, RFLAG_SAVE_ITEMS ) )
	  continue;
      }

      thing->Extract( );
 
      /*
      if( !( victim = character( ch->array->list[i] ) )
	  || ( !victim->pcdata
	       && ( !is_set( victim->status, STAT_PET )
		    || victim->leader->species ) ) ) {
        ch->array->list[i]->Extract( );
      }
      */
    }
    return;
  }

  int flags;
  if( !get_flags( ch, argument, &flags, "MO", "purge" ) ) {
    return;
  }

  if( is_set( flags, 0 ) ) {
    species_data *species;
    int vnum;
    unsigned count = 0;
    if( !number_arg( argument, vnum ) ) {
      send( ch, "Syntax: purge -M <mob_vnum>\n\r" );
    } else if( !( species = get_species( vnum ) ) ) {
      send( ch, "No such mob: %d.\n\r", vnum );
    } else if( !ch->can_edit( species, false ) ) {
      send( ch, "You don't have permission to purge mob #%d.\n\r", vnum );
    } else {
      // Need to rehash them all first to get the fsend_seen() messages right.
      for( int i = 0; i < mob_list; ++i ) {
	char_data *mob = mob_list[i];
	if( mob->Is_Valid( )
	    && mob->species->vnum == vnum
	    && ( !is_set( mob->status, STAT_PET )
		 || mob->leader && mob->leader->species ) ) {
	  if( mob->array ) {
	    rehash( 0, *mob->array );
	  }
	}
      }
      for( int i = 0; i < mob_list; ++i ) {
	char_data *mob = mob_list[i];
	if( mob->Is_Valid( )
	    && mob->species->vnum == vnum
	    && ( !is_set( mob->status, STAT_PET )
		 || mob->leader && mob->leader->species ) ) {
	  if( mob->Shown() != 0 ) {
	    fsend_seen( mob, "%s disappear%s in a cloud of greasy, black smoke.",
			mob,
			mob->Shown() == 1 ? "s" : "" );
	  }
	  ++count;
	  mob->Extract( );
	}
      }
      fsend( ch, "Purged %u instance%s of %s.",
	     count,
	     count == 1 ? "" : "s",
	     species );
    }
    return;
  }

  if( is_set( flags, 1 ) ) {
    obj_clss_data *clss;
    int vnum;
    unsigned count = 0;
    if( !number_arg( argument, vnum ) ) {
      send( ch, "Syntax: purge -O <object_vnum>\n\r" );
    } else if( !( clss = get_obj_index( vnum ) ) ) {
      send( ch, "No such object: %d.\n\r", vnum );
    } else if( !ch->can_edit( clss, false ) ) {
      send( ch, "You don't have permission to purge obj #%d.\n\r", vnum );
    } else {
      for( int i = 0; i < obj_list; ++i ) {
	obj_data *obj = object( obj_list[i] );
	if( obj
	    && obj->Is_Valid( )
	    && obj->pIndexData->vnum == vnum ) {
	  //	  if( room_data *room = Room( obj->array->where ) ) {
	  //	  }
	  count += obj->Number( );
	  obj->Extract( );
	}
      }
      fsend( ch, "Purged %u instance%s of %s.",
	     count,
	     count == 1 ? "" : "s",
	     clss );
    }
    return;
  }

  thing_data *thing;

  if( !( thing = one_thing( ch, argument, "purge", ch->array ) ) )
    return;
  
  if( player( thing ) ) {
    fsend( ch, "%s is a player and you can't purge players.",
	   thing );
    return;
  }

  char_data *rch = character( thing );
  if( rch
      && is_pet( rch )
      && rch->leader != ch
      && get_trust( rch->leader ) >= get_trust( ch ) ) {
    fsend( ch, "You can't purge %s.", rch );
    return;
  }

  fsend( ch, "You purge %s.", thing );
  fsend_color( *ch->array, COLOR_WIZARD, "%s purges %s.", ch, thing );

  thing->From(1)->Extract( );
}


void do_advance( char_data* ch, const char *argument )
{
  char           arg  [ MAX_INPUT_LENGTH ];
  player_data*  victim;
  int          level;
  int              i;

  in_character = false;

  argument = one_argument( argument, arg );
  
  if( !*arg || !*argument || !is_number( argument ) ) {
    send( ch, "Usage: advance <char> <level>.\n\r" );
    return;
  }
  
  if( !( victim = one_player( ch, arg, "advance", 
			     (thing_array*) &player_list ) ) ) 
    return;
  
  if( ( level = atoi( argument ) ) <= 0 ) {
    send( ch,
	  "Advancing them to a non-positive level does not make sense.\n\r" );
    return;
  }
  
  if( ch != victim ) {
    if( !is_demigod( ch ) ) {
      send( ch, "You cannot advance others.\n\r" );
      return;
    } 
    if( !is_god( ch ) && level > 97 ) {
      send( ch, "You can only advance others to 97.\n\r" );
      return;
    }
  }
  
  if( level > get_trust( ch ) ) {
    fsend( ch, "You can't advance %s beyond your trust level %d.",
	  victim, get_trust(ch) );
    return;
  }

  int old_level = victim->Level( );

  player_log( victim, "Advanced from level %d to level %d by %s.",	    
	      old_level,
	      level,
	      ch->real_name( ) );
  player_log( ch, "Advanced %s from level %d to level %d.",
	      victim->real_name( ),
	      old_level,
	      level );
  
  if( level < old_level ) {
    for( i = old_level; i > level; --i ) {
      lose_level( victim, false );
    }
    
  } else if( level > old_level ) {
    for( i = old_level; i < level; ++i ) {
      advance_level( victim, false );
    }
  } else {
    if( ch != victim ) {
      fsend( ch, "%s is already level %d.\n\r", victim, old_level );
    } else {
      send( ch, "You are already level %d.\n\r", old_level );
    }
    return;
  }
  
  if( victim->Level( ) != old_level ) {
    init_affects( victim, old_level );
  }

  victim->exp = 0;
  update_score( victim );
  victim->Save( );

  if( ch != victim ) {
    fsend( ch, "You have set %s to level %d.", victim->real_name( ), level );
    fsend_color( victim, COLOR_WIZARD, "%s has set you to level %d.", ch->real_name( ), level );
  } else {
    fsend( ch, "You are now level %d.", level );
  }
}


/*
 *   RESTORE
 */
 

static void restore( char_data* victim, char_data* ch )
{
  victim->hit  = victim->max_hit;
  victim->mana = victim->max_mana;

  update_max_move( victim );

  victim->move = victim->max_move;

  strip_affect( victim, AFF_POISON );
  strip_affect( victim, AFF_HALLUCINATE );
  strip_affect( victim, AFF_CONFUSED );
  strip_affect( victim, AFF_BLIND );
  strip_affect( victim, AFF_SILENCE );
  strip_affect( victim, AFF_PLAGUE );
  strip_affect( victim, AFF_DEATH );
  strip_affect( victim, AFF_DEAFNESS );
  strip_affect( victim, AFF_PARALYSIS );

  gain_condition( victim, COND_FULL, 999 );
  gain_condition( victim, COND_THIRST, 999 );

  update_pos( victim );

  if( ch != victim ) 
    fsend_color( victim, COLOR_WIZARD, "%s has restored you.", ch );
}


void do_restore( char_data* ch, const char *argument )
{
  if( mortal( ch, "restore" ) )
    return;

  if( !*argument ) {
    send( ch, "Usage: restore <char>\n\r" );
    return;
  }

  char_data *victim;

  if( !strcasecmp( argument, "all" ) ) {
    if( !is_spirit( ch ) ) {
      send( ch, "You must be level %d to restore all.\n\r", LEVEL_SPIRIT );
      return;
    }
    int count = 0;
    for( int i = 0; i < player_list; ++i ) {
      victim = player_list[i];
      if( victim->In_Game( )
	  && victim->Level() < LEVEL_APPRENTICE ) {
        restore( victim, ch );
	++count;
      }
    }
    if( count == 0 ) {
      send( ch, "There are no players to restore.\n\r" );
    } else {
      send( ch, "%s player%s restored.\n\r",
	    number_word( count, ch ),
	    count == 1 ? "" : "s" );
      char *tmp = static_string( );
      snprintf( tmp, THREE_LINES, "All players (%d) restored by %s.",
		count, ch->Name() );
      info( get_trust( ch ), empty_string, 0, tmp, IFLAG_ADMIN, 1, ch );
    }
    return;
  }
  
  if( !strcasecmp( argument, "room" ) ) {
    if( !is_immortal( ch ) ) {
      send( ch, "You must be level %d to restore room.\n\r", LEVEL_IMMORTAL );
      return;
    }
    int count = 0;
    for( int i = 0; i < ch->in_room->contents; ++i ) {
      if( player_data *pl = player( ch->in_room->contents[i] ) ) {
	if( pl->In_Game( )
	    && pl->Level( ) < LEVEL_APPRENTICE ) {
	  restore( pl, ch );
	  ++count;
	}
      }
    }
    if( count == 0 ) {
      send( ch, "There are no players here to restore.\n\r" );
    } else {
      send( ch, "%s player%s in room restored.\n\r",
	    number_word( count, ch ),
	    count == 1 ? "" : "s" );
      char *tmp = static_string( );
      snprintf( tmp, THREE_LINES, "All players (%d) in room %d restored by %s.",
		count, ch->in_room->vnum, ch->Name() );
      info( get_trust( ch ), empty_string, 0, tmp, IFLAG_ADMIN, 1, ch );
    }
    return;
  }

  if( !( victim = one_character( ch, argument, "restore",
				 ch->array, (thing_array*) &player_list ) ) )
    return;
  
  if( !is_immortal( ch ) && ch != victim ) {
    send( ch, "You can only restore yourself.\n\r" );
    return;
  }
  
  if( ch == victim ) {
    send( ch, "You restore yourself.\n\r" );
  } else {
    fsend( ch, "%s restored.", victim );
    
    char *tmp = static_string( );
    snprintf( tmp, THREE_LINES, "%s restored by %s.",
	      victim->Name(), ch->Name() );
    info( get_trust( ch ), empty_string, 0, tmp, IFLAG_ADMIN, 1, ch );
  }

  restore( victim, ch );  
}


/*
 *   WIZLOCK
 */


void do_wizlock( char_data* ch, const char *argument )
{
  if( *argument == '\0' ) {
    send( ch, "Wizlock is %s.\n\r", wizlock ? "on" : "off" );
    return;
  }
  
  bool flag = wizlock;
  
  if( !toggle( ch, argument, "wizlock", ( int* ) &wizlock, 0 ) ) {
    send( ch, "Syntax: wizlock [on|off]\n\r" );
    return;
  }

  if( wizlock != flag ) {
    char *tmp = static_string( );
    snprintf( tmp, THREE_LINES, "** WIZLOCK %s by %s **",
	      wizlock ? "set" : "removed", ch->Name( ) ); 
    info( 0, empty_string, 0, tmp, IFLAG_LOGINS, 1, ch );
    
    if( wizlock ) {
      create_control_file( WIZLOCK_FILE, ch->descr->name );
    } else {
      delete_control_file( WIZLOCK_FILE );
    }
  }
}


void do_godlock( char_data* ch, const char *argument )
{
  if( *argument == '\0' ) {
    send( ch, "Godlock is %s.\n\r", godlock ? "on" : "off" );
    return;
  }

  bool flag = godlock;

  if( !toggle( ch, argument, "godlock", ( int* ) &godlock, 0 ) ) {
    send( ch, "Syntax: godlock <on|off>\n\r" );
    return;
  }

  if( godlock != flag ) {
    char *tmp = static_string( );
    snprintf( tmp, THREE_LINES, "** GODLOCK %s by %s **",
	      godlock ? "set" : "removed", ch->Name( ) ); 
    info( 0, empty_string, 0, tmp, IFLAG_LOGINS, 1, ch );

    if( godlock ) {
      create_control_file( GODLOCK_FILE, ch->descr->name );
    } else {
      delete_control_file( GODLOCK_FILE );
    }
  }
}


void do_sset( char_data* ch, const char *argument )
{
  in_character = false;

  char arg1 [MAX_INPUT_LENGTH];
  char arg2 [MAX_INPUT_LENGTH];

  player_data *pl;
  int col = 0;

  argument = one_argument( argument, arg1 );
  argument = one_argument( argument, arg2 );

  if( !*arg1 ) {
    send( ch, "Syntax: sset <char> [<skill>] [<value>]\n\r" );
    return;
  }
 
  if( !( pl = one_player( ch, arg1, "sset", 
			      (thing_array*) &player_list ) ) )
    return;

  if( ch != pl ) {
    if( !has_permission( ch, PERM_PLAYERS ) ) {
      send( ch, "You can only sset yourself.\n\r" );
      return;
    }
    
    if( is_builder( pl ) && !is_demigod( ch ) ) {
      send( ch, "You can't sset immortals.\n\r" );
      return;
    }
    
    if( get_trust( pl ) >= get_trust( ch ) ) {
      send( ch, "You can't sset them.\n\r" );
      return;
    }
  }
  
  if( !*arg2 ) {
    page_title( ch, pl->descr->name );
    for( int j = 0; j < MAX_SKILL_CAT; ++j ) {
      int m = table_max[ skill_table_number[ j ] ];
      for( int i = 0; i < m; ++i ) {
	int sn = skill_ident( j, i );
	int level = pl->get_skill( sn );
	if( level == UNLEARNT )
	  continue;
	page( ch, "%21s (%2d)%s", skill_entry(sn)->name,
	      level, ++col%3 == 0 ? "\n\r" : "" );
      }
    }
    if( col % 3 != 0 )
      page( ch, "\n\r" );
    return;
  }

  bool fAll = !strcasecmp( arg2, "all" );

  int value = -1;

  if( fAll || *argument ) {
    if( !number_arg( argument, value )
	|| value > 10
	|| value < 0 ) {
      send( ch, "Skill proficiency can range from 0 to 10.\n\r" );
      return;
    }
  }

  if( fAll ) {
    for( int j = 0; j < MAX_SKILL_CAT; ++j ) {
      int m = table_max[ skill_table_number[ j ] ];
      for( int i = 0; i < m; ++i ) {
	pl->shdata->skills[j][i] = (unsigned char)value;
      }
    }
    fsend( ch, "All skills on %s set to %d.", pl, value );
    player_log( pl, "All skills set to %d by %s.",
		value, ch->descr->name );
    return;
  }

  const int s = find_skill( arg2 );
  
  if( s == -1 ) {
    send( ch, "Unknown skill.\n\r" );
    return;
  }

  const int sn = skill_number( s );
  const int st = skill_table( s );
  Skill_Type *entry = skill_entry( st, sn );

  if( value == -1 ) {
    fsend( ch, "Skill \"%s\" on %s is at %d.",
	   entry->name, pl->descr->name, (int)pl->shdata->skills[st][sn] );
    return;
  }

  pl->shdata->skills[st][sn] = (unsigned char)value;

  fsend( ch, "Skill \"%s\" on %s set to %d.",
	 entry->name, pl->descr->name, value );
  player_log( pl, "Skill '%s' set to %d by %s.",
	      entry->name, value, ch->descr->name );
  
  /*
  for( int j = 0; j < MAX_SKILL_CAT; ++j ) {
    int m = table_max[ skill_table_number[ j ] ];
    for( int i = 0; i < m; ++i ) {
      if( fAll ) {
	pl->shdata->skills[j][i] = (unsigned char)value;
	continue;
      }
      Skill_Type *entry = skill_entry( j, i );
      if( !strncasecmp( entry->name, arg2, strlen( arg2 ) ) ) {
	if( value == -1 ) {
	  fsend( ch, "Skill \"%s\" on %s is at %d.",
		 entry->name, pl->descr->name, (int)pl->shdata->skills[j][i] );
	} else {
	  pl->shdata->skills[j][i] = (unsigned char)value;
	  fsend( ch, "Skill \"%s\" on %s set to %d.",
		 entry->name, pl->descr->name, value );
	}
	return;
      }
    }
  }

  if( fAll ) {
    fsend( ch, "All skills on %s set to %d.", pl, value );
    return;
  } 

  send( ch, "Unknown skill.\n\r" );
  */
}


void do_force( char_data* ch, const char *argument )
{
  char              arg  [ MAX_STRING_LENGTH ];
  char_data*     victim;
  player_data*   player;

  argument = one_argument( argument, arg );

  if( !*arg || !*argument ) {
    send( ch, "Syntax: force <char> <command>\n\r" );
    return;
  }
  
  if( !strcasecmp( arg, "all" ) ) {
    if( !is_demigod( ch ) ) {
      send( ch, "Quite definitely not.\n\r" );
      return;
    }
    fsend( ch, "You force all players to '%s'.", argument );
    for( int i = 0; i < player_list; ++i ) {
      player = player_list[i];
      if( player->In_Game( )
	  && get_trust( player ) < get_trust( ch ) ) {
        fsend_color( player, COLOR_WIZARD, "%s forces you to '%s'.", ch, argument );
        set_bit( player->status, STAT_FORCED );
        interpret( player, argument );
        remove_bit( player->status, STAT_FORCED );
      }
    }
    return;
  }
  
  if( !( victim = one_character( ch, arg, "force",
				 ch->array, (thing_array*) &player_list ) ) )
    return;
  
  if( victim == ch ) {
    send( ch, "Forcing yourself doesn't make sense.\n\r" );
    return;
  }

  if( victim->pcdata
      && !has_permission( ch, PERM_FORCE_PLAYERS ) ) {
    send( ch, "You do not have permission to force players.\n\r" );
    return;
  }
  
  if( get_trust( victim ) >= get_trust( ch )
      || ( is_pet( victim )
	   && victim->leader != ch
	   && get_trust( victim->leader ) >= get_trust( ch ) ) ) {
    send( ch, "You are unable to force them.\n\r" );
    return;
  }
  
  fsend( ch, "You force %s to '%s'.", victim, argument );
  fsend_color( victim, COLOR_WIZARD, "%s forces you to '%s'.", ch, argument );

  set_bit( victim->status, STAT_FORCED );
  interpret( victim, argument );
  remove_bit( victim->status, STAT_FORCED );
}


void do_invis( char_data* ch, const char *argument )
{
  if( not_player( ch ) )
    return;
  
  wizard_data *imm;
  if( !( imm = wizard( ch ) ) )
    return;

  if( *argument ==  '\0' ) {
    if( !is_set( ch->pcdata->pfile->flags, PLR_WIZINVIS ) ) { 
      fsend_color_seen( ch, COLOR_WIZARD, "%s slowly fades into thin air.", ch );
      send( ch, "You slowly vanish into thin air.\n\r" );
      set_bit( ch->pcdata->pfile->flags, PLR_WIZINVIS );
    } else {
      remove_bit( ch->pcdata->pfile->flags, PLR_WIZINVIS );
      fsend_color_seen( ch, COLOR_WIZARD, "%s slowly fades into existence.", ch );
      send( ch, "You slowly fade back into existence.\n\r" );
    }
    update_aggression( ch );
    return;
  }

  int num = atoi( argument );
  if( num > ch->Level() || num < 1 ) {
    send( ch, "Choose a number between 1 and your level.\n\r" );
    return;
  }

  send( ch, "Your wizinvis level has been set to %d.\n\r", num );
  imm->wizinvis = num;
}


void do_holylight( char_data* ch, const char *)
{
  if( !ch->pcdata )
    return;
    
  switch_bit( ch->pcdata->pfile->flags, PLR_HOLYLIGHT );
  
  send( ch, "Holy light mode %s.\n\r",
	on_off( ch->pcdata->pfile->flags, PLR_HOLYLIGHT ) );
}


/*
 *   SNOOP COMMANDS
 */


void do_snoop( char_data* ch, const char *argument )
{
  if( is_mob( ch ) )
    return;
  
  in_character = false;

  char buf [ MAX_INPUT_LENGTH ];
  
  link_data *link;
  player_data *victim;
  
  if( !*argument ) {
    bool fFound = false;
    
    buf[0] = '\0';
    for( link = link_list; link; link = link->next ) {
      if( link->snoop_by
	  && get_trust( ch ) >= get_trust( link->snoop_by->character ) ) {
	fFound = true;
	int l = strlen(buf);
	snprintf( buf + l, MAX_INPUT_LENGTH-l, "%15s : %s\n\r",
		  link->character->real_name( ),
		  link->snoop_by->character->real_name( ) );
      }
    }
    
    if( fFound )
      send( ch, buf );
    else
      send( ch, "No active snoops.\n\r" );
    return;
  }
  
  if( !( victim = one_player( ch, argument, "snoop", 
			     (thing_array*) &player_list ) ) ) {
    return;
  }
  
  if( !victim->link ) {
    send( ch, "No descriptor to snoop.\n\r" );
    return;
  }

  if( victim == player( ch ) ) {
    send( ch, "Canceling all snoops.\n\r" );
    for( link = link_list; link; link = link->next ) 
      if( link->snoop_by == ch->link )
	link->snoop_by = 0;
    return;
  }
  
  if( victim->link->snoop_by ) {
    if( ch->link == victim->link->snoop_by ) {
      victim->link->snoop_by = 0;
      fsend( ch, "Stopped snooping %s.", victim );
    } else {
      fsend( ch, "%s is already being investigated.", victim );
    }
    return;
  }
  
  if( get_trust( victim ) >= get_trust( ch ) ) {
    fsend( ch, "%s is little out of your league.", victim );
    return;
  }
  
  if( ch->link )
    for( link = ch->link->snoop_by; link; link = link->snoop_by ) 
      if( link->character == victim || link->player == victim ) {
        send( ch, "No snoop loops.\n\r" );
        return;
      }
  
  victim->link->snoop_by = ch->link;
  fsend( ch, "Now snooping %s.", victim );
}


/*
 *   SHUTDOWN/REBOOT
 */


static void shutdown_sequence( char_data *ch )
{
  clear_auction();
  //  return_corpses( ch );

  for( int i = 0; i < player_list; ++i ) {
    player_data *pc = player_list[i];
    if( pc->Is_Valid() ) {
      pc->Save( );
    }
  }

  for( area_data *area = area_list; area; area = area->next ) {
    for( room_data *room = area->room_first; room; room = room->next ) {
      room->Save();
    }
  }

  write_all( );
}


void do_shutdown( char_data* ch, const char *argument )
{
  int flags;

  if( !get_flags( ch, argument, &flags, "n", "shutdown" ) )
    return;

  if( is_set( ch->pcdata->pfile->flags, PLR_WIZINVIS ) ) {
    echo( "** System shutdown **\n\r" );
  } else {
    echo( "** Shutdown by %s **\n\r", ch->descr->name );
  }

  immortal_log( ch, "shutdown", argument );

  if( !is_set( flags, 0 ) )
    shutdown_sequence( ch );

  delete_control_file( WIZLOCK_FILE );
  delete_control_file( GODLOCK_FILE );

  shutdown( ch->descr->name );
}


void do_reboot( char_data* ch, const char *argument )
{
  int flags;

  if( !get_flags( ch, argument, &flags, "n", "reboot" ) )
    return;

  if( is_set( ch->pcdata->pfile->flags, PLR_WIZINVIS ) ) {
    echo( "** System reboot **\n\r" );
  } else {
    echo( "** Reboot by %s **\n\r", ch->descr->name );
  }

  immortal_log( ch, "reboot", argument );

  if( !is_set( flags, 0 ) )
    shutdown_sequence( ch );

  reboot( ch->descr->name );
}


/*
 *   AT
 */

 
void do_at( char_data *ch, const char *argument )
{
  wizard_data *imm;

  if( !ch
      || !( imm = wizard( ch ) ) )
    return;

  const char *const usage = "Usage: at <location> <command>\n\r";

  char arg [MAX_INPUT_LENGTH];

  argument = one_argument( argument, arg );

  if( !*argument ) {
    send( ch, usage );
    return;
  }

  in_character = false;

  room_data *to;
  if( !( to = find_location( ch, arg ) ) ) {
    send( ch, "No such location.\n\r" );
    return;
  }

  room_data *from;
  if( ( from = ch->in_room ) == to ) {
    send( ch, "You are already there!\n\r" );
    return;
  }
  
  if( is_set( to->room_flags, RFLAG_OFFICE )
      && !is_god( ch )
      && to->vnum != imm->office ) {
    send( ch, "That location belongs to another immortal.\n\r" );
    return;
  } 

  ch->From( );
  ch->To( to );

  send( ch, "*** At %s (%d) ***\n\r\n\r", to->name, to->vnum );

  interpret( ch, argument );

  ch->From();
  ch->To( from );
}


/*
 *   DOCK
 */

 
void do_dock( char_data *ch, const char *argument )
{
  const char *const usage = "Usage: dock <player> [<value> [<reason>]]\n\r";

  wizard_data *imm;

  if( !( imm = wizard( ch ) ) )
    return;

  char arg [MAX_INPUT_LENGTH];

  argument = one_argument( argument, arg );

  in_character = false;

  if( !*arg ) {
    if( imm->docking ) {
      fsend( ch, "You are currently docking %s.", imm->docking );
    } else {
      send( ch, usage );
    }
    return;
  }

  player_data *victim;

  if( !( victim = one_player( ch, arg, "dock", 
			      (thing_array*) &player_list ) ) )
    return;
  
  if( victim == ch ) {
    if( imm->docking ) {
      fsend( ch, "You stop docking %s.", imm->docking );
      imm->docking->docker = 0;
      imm->docking = 0;
    } else {
      send( ch, "You were not docking anyone.\n\r" );
    }
    return;
  }

  if( get_trust( victim ) >= get_trust( ch ) ) {
    send( ch, "You failed to dock %s.\n\r", victim );
    return;
  }

  if( victim->docker
      && victim->docker != (player_data *) imm ) {
    fsend( ch, "%s is being docked by %s.\n\r", victim, victim->docker );
  }

  if( !*argument ) {
    fsend( ch, "%s has %d gossip points.", victim, victim->gossip_pts );
    return;
  }

  if( victim->docker
      && victim->docker != imm ) {
    return;
  }

  int val;

  if( !number_arg( argument, val ) ) {
    send( ch, usage );
    return;
  }

  if( val <= 0 ) {
    send( ch, "You cannot dock zero or negative gossip points!\n\r" );
    return;
  }

  if( val > 1000 ) {
    send( ch, "You cannot dock more than 1,000 gossip points!\n\r" );
    return;
  }

  if( victim->gossip_pts <= 0 ) {
    send( ch, "You cannot dock someone with zero or fewer gossip points!\n\r" );
    return;
  }

  if( imm->docking
      && victim != imm->docking ) {
    fsend( ch, "You stop docking %s.", imm->docking );
    imm->docking->docker = 0;
  }

  imm->docking = victim;
  victim->docker = imm;

  victim->gossip_pts -= val;

  if( victim->gossip_pts < 0 ) {
    victim->set_default_title( );
    free_string( victim->pcdata->pfile->homepage, MEM_PFILE );
    victim->pcdata->pfile->homepage = empty_string;
  }

  victim->Save( );

  char tmp [THREE_LINES];
  const char *p = ( val == 1 ) ? "point" : "points";

  if( *argument ) {
    fsend_color( victim, COLOR_WIZARD, "You have been docked %d gossip %s for: %s.",
		 val, p, argument );
    fsend( ch, "You have docked %s %d gossip %s for: %s.",
	   victim, val, p, argument );
    snprintf( tmp, THREE_LINES, "%s docked %d gossip %s for: %s.",
	      victim->Name(), val, p, argument );
    snprintf( arg, MAX_INPUT_LENGTH, "%s docked %d gossip %s by %s for: %s.",
	      victim->Name(), val, p, ch->Name(), argument );
    info( LEVEL_AVATAR, tmp, invis_level( ch ), arg, IFLAG_ADMIN, 1, ch );
    player_log( victim, "Docked %d gossip %s by %s for: %s.",
		val, p, ch->real_name( ), argument );
    player_log( ch, "Docked %s %d gossip %s for: %s.",
		victim->real_name( ), val, p, argument );
    
  } else {
    fsend_color( victim, COLOR_WIZARD, "You have been docked %d gossip %s.",
		 val, p );
    fsend( ch, "You have docked %s %d gossip %s for no specified reason.",
	   victim, val, p );
    snprintf( tmp, THREE_LINES, "%s docked %d gossip %s.",
	      victim->Name(), val, p );
    snprintf( arg, MAX_INPUT_LENGTH, "%s docked %d gossip %s by %s.",
	      victim->Name(), val, p, ch->Name() );
    info( LEVEL_AVATAR, tmp, invis_level( ch ), arg, IFLAG_ADMIN, 1, ch );
    player_log( victim, "Docked %d gossip %s by %s.",
		val, p, ch->real_name( ) );
    player_log( ch, "Docked %s %d gossip %s.",
		victim->real_name( ), val, p );
  }

  p = ( victim->gossip_pts == 1 ) ? "point" : "points";

  fsend_color( victim, COLOR_WIZARD, "You now have %d gossip %s - please use them more wisely.",
	       victim->gossip_pts, p );
  fsend( ch, "%s now has %d gossip %s.", victim, victim->gossip_pts, p );
}


void do_status( char_data *ch, const char *argument )
{
  /*
  int flags;
  if( !get_flags( ch, argument, &flags, "p", "status" ) )
    return;
  */

  //  if( flags == 0 ) {
  send( ch, "         time: %d\n\r", weather.tick );
  send( ch, "     sunlight: %d\n\r", weather.sunlight );
  send( ch, "    moonlight: %d\n\r", weather.moonlight );
  send( ch, "     moon_mid: %d\n\r", weather.moon_mid );
  send( ch, "     moon_var: %d\n\r", weather.moon_var );
  send( ch, "   moon_phase: %d\n\r", weather.moon_phase );
  
  send( ch, "   max_serial: %d\n\r", max_serial );
  send( ch, "  species_max: %d\n\r", species_max );
  send( ch, " obj_clss_max: %d\n\r", obj_clss_max );

  send( ch, "    who_calls: %u\n\r", who_calls );
  //  }
}


static void show_extra( char_data *ch, const extra_array& extras, const extra_data *extra, const char *title  )
{
  if( !extra )
    return;

  for( int i = 0; i < extras; ++i ) {
    if( extras[i] == extra ) {
      page( ch, "%s: [%d] %s\n\r", title, i+1, extra->keyword );
      return;
    }
  }
}


void do_edit( char_data *ch, const char *argument )
{
  wizard_data *imm = wizard( ch );

  if( !imm )
    return;

  page_title( ch, "Your Current Edits" );

  page( ch, "Room: [%d] %s\n\r",
	imm->in_room->vnum, imm->in_room->name );

  show_extra( ch, ch->in_room->Extra_Descr( ), imm->room_edit, "  Extra" );

  if( imm->action_edit ) {
    int i = 1;
    for( action_data *action = ch->in_room->action; action; action = action->next ) {
      if( action == imm->action_edit ) {
	if( *action->command ) {
	  page( ch, "  Action: [%d] Command %s - %s\n\r", i,
		action->command, action->target );
	} else {
	  page( ch, "  Action: [%d] %s - %s\n\r", i,
		action_trigger[action->trigger], Keywords( action ) );
	}

	show_extra( ch, action->Extra_Descr( ), imm->adata_edit, "    Adata" );
	break;
      }
      ++i;
    }
  }

  if( exit_data *exit = imm->exit_edit ) {
    page( ch, "  Exit: [%s] %s - %s [%d]\n\r",
	  dir_table[ exit->direction ].name,
	  exit->Name( ), exit->to_room->name, exit->to_room->vnum );
  }

  if( int i = imm->custom_edit ) {
    for( int j = 0; j < ch->in_room->contents; ++j ) {
      if( mob_data *keeper = mob( ch->in_room->contents[j] ) ) {
	if( shop_data *shop = keeper->pShop ) {
	  if( custom_data *custom = locate( shop->custom, imm->custom_edit ) ) {
	    page( ch, "  Custom: [%d] %s [%d]\n\r",
		  i, item_name( custom, false ), custom->item->vnum );
	  }
	  break;
	}
      }
    }
  }

  if( obj_clss_data *clss = imm->obj_clss_edit ) {
    page( ch, "\n\rObject: [%d] %s\n\r", clss->vnum, clss->Name( ) );
    show_extra( ch, clss->extra_descr, imm->oextra_edit, "  Extra" );
    if( imm->oprog_edit ) {
      int i = 1;
      for( oprog_data *oprog = clss->oprog; oprog; oprog = oprog->next ) {
	if( oprog == imm->oprog_edit ) {
	  if( oprog->trigger == OPROG_TRIGGER_NONE ) {
	    page( ch, "  Oprog: [%d] Command %s - %s\n\r", i,
		  oprog->command, oprog->target );
	  } else {
	    page( ch, "  Oprog: [%d] %s - %s\n\r", i,
		  oprog_trigger[oprog->trigger], oprog->target );
	  }
	  show_extra( ch, oprog->Extra_Descr( ), imm->opdata_edit, "    Opdata" );
	  break;
	}
	++i;
      }
    }
  }

  if( species_data *species = imm->mob_edit ) {
    page( ch, "\n\rMob: [%d] %s\n\r",
	  species->vnum, species->Name( true, false, false ) );
    show_extra( ch, species->extra_descr, imm->mextra_edit, "  Extra" );
    if( imm->mprog_edit ) {
      int i = 1;
      for( mprog_data *mprog = species->mprog; mprog; mprog = mprog->next ) {
	if( mprog == imm->mprog_edit ) {
	  if( mprog->trigger == MPROG_TRIGGER_NONE ) {
	    page( ch, "  Mprog: [%d] Command %s\n\r", i, mprog->string );
	  } else if( mprog->trigger == MPROG_TRIGGER_GIVE ) {
	    const char *name = "anything";
	    if( mprog->value != 0 ) {
	      obj_clss_data *clss = get_obj_index( mprog->value );
	      name = clss ? clss->Name() : "??";
	    }
	    page( ch, "  Mprog: [%d] %s %s [%d]\n\r", i,
		  mprog_trigger[mprog->trigger], name, mprog->value );

	  } else {
	    page( ch, "  Mprog: [%d] %s %s\n\r", i,
		  mprog_trigger[mprog->trigger], mprog->string );
	  }
	  show_extra( ch, mprog->Extra_Descr( ), imm->mpdata_edit, "    Mpdata" );
	  break;
	}
	++i;
      }
    }
  }

  if( imm->table_edit[0] != -1 ) {
    page( ch, "\n\rTable: %s, Entry: %s\n\r",
	  table_name(imm->table_edit[0]), entry_name( imm->table_edit[0], imm->table_edit[1] ) );
  }

  if( imm->rtable_edit != -1 ) {
    page( ch, "\n\rRtable: [%d] %s\n\r", imm->rtable_edit+1, rtable_list[imm->rtable_edit]->name );
  }

  if( imm->list_edit != -1 ) {
    page( ch, "\n\rList: %s\n\r", list_entry[0][imm->list_edit] );
  }

  if( help_data *help = imm->help_edit ) {
    for( int i = 0; i < max_help; ++i ) {
      if( help_list[i] == help ) {
	page( ch, "\n\rHelp: [%d] %s\n\r", i, help->name );
	break;
      }
    }
  }

  if( quest_data *quest = imm->quest_edit ) {
    page( ch, "\n\rQuest: [%d] %s\n\r", quest->vnum, quest->message );
  }

  if( player_data *player = imm->player_edit ) {
    page( ch, "\n\rPlayer: %s\n\r", player->descr->name );
  }

  if( account_data *account = imm->account_edit ) {
    page( ch, "\n\rAccount: %s\n\r", account->name );
  }

  bool found = false;
  for( int i = 0; i < table_max[ TABLE_TEDIT ]; ++i ) {
    if( tedit_table[i].lock ) {
      if( !found ) {
	page( ch, "\n\r" );
	page_underlined( ch, "Locked Tables\n\r" );
	found = true;
      }
      page( ch, "  %s\n\r", tedit_table[i].name );
    }
  }
}


void do_privileges( char_data *ch, const char *argument )
{
  wizard_data *imm = wizard( ch );

  if( !imm )
    return;

  switch_bit( ch->pcdata->pfile->flags, PLR_NO_PRIVILEGES );

  fsend( ch, "%s set to %s.", plr_name[PLR_NO_PRIVILEGES],
	 true_false( ch->pcdata->pfile->flags, PLR_NO_PRIVILEGES ) );

  if( is_set( ch->pcdata->pfile->flags, PLR_NO_PRIVILEGES ) ) {
    send( ch, "[This option eliminates certain immortal privileges.]\n\r" );
  }
}


/*
static void extra_mem_usage( const extra_array& extras,
			     unsigned& valid_ptr_num,
			     unsigned& wasted_ptr_num,
			     unsigned& array_mem,
			     unsigned& key_mem,
			      unsigned& text_mem )
{
  valid_ptr_num += extras.size;
  wasted_ptr_num += ( extras.memory - extras.size );
  array_mem += sizeof( extra_array ) + extras.memory * sizeof( extra_data * );
  for( int i = 0; i < extras.size; ++i ) {
    if( extras.list[i]->keyword && extras.list[i]->keyword != empty_string )
      key_mem += strlen( extras.list[i]->keyword ) + 1;
    if( extras.list[i]->text && extras.list[i]->text != empty_string )
      text_mem += strlen( extras.list[i]->text ) + 1;
  }
}
*/



static void testing_support( char_data *ch, char_data *pl, Content_Array& stuff, const char *msg )
{
  if( stuff.is_empty() )
    return;

  for( int i = 0; i < stuff; ++i ) {
    if( obj_data *obj = object( stuff[i] ) ) {
      if( obj->pIndexData->item_type == ITEM_LIGHT
	  || obj->pIndexData->item_type == ITEM_LIGHT_PERM ) {
	obj->value[2] = obj->value[0];
      }
      testing_support( ch, pl, obj->contents, "sub" );
    }
  }
}


static bool edit_object( obj_data *obj, obj_clss_data *clss = 0 )
{
  // Edit classes.
  if( clss ) {
    assign_bit( clss->extra_flags, OFLAG_THE_AFTER, is_set( clss->extra_flags, OFLAG_THE_BEFORE ) );
    return false;
  }

  if( !obj->Is_Valid( ) )
    return false;

  // Edit instances.
  assign_bit( obj->extra_flags, OFLAG_THE_AFTER, is_set( obj->extra_flags, OFLAG_THE_BEFORE ) );

  return false;
}


static unsigned edit_objects( Content_Array& stuff )
{
  if( stuff.is_empty() )
    return 0;

  unsigned count = 0;

  for( int i = 0; i < stuff; ++i ) {
    if( obj_data *obj = object( stuff[i] ) ) {
      if( edit_object( obj ) )
	++count;
      count += edit_objects( obj->contents );
    }
  }

  return count;
}


void do_testing( char_data* ch, const char *argument )
{
  wizard_data *imm;

  if( !ch || !( imm = wizard( ch ) ) ) {
    return;
  }

  if( !is_god( ch ) ) {
    send( ch, "Beware: Severus uses this command for all kinds of nasty things.\n\r" );
    return;
  }

  in_character = false;

  int flags;

  if( !get_flags( ch, argument, &flags, "O", "test" ) )
    return;

  if( is_set( flags, 0 ) ) {
    // Edit ALL objects.

    unsigned count = 0;

    // Object classes.
    for( int i = 1; i <= obj_clss_max; ++i ) {
      if( obj_clss_data *obj_clss = obj_index_list[i] ) {
	count += edit_object( 0, obj_clss );
      }
    }

    if( count != 0 )
      save_objects( );

    count = 0;

    // All existing objects.
    for( int i = 0; i < obj_list; ++i ) {
      count += edit_object( obj_list[i] );
    }

    // Save all rooms.
    if( count != 0 ) {
      for( area_data *area = area_list; area; area = area->next ) {
	for( room_data *room = area->room_first; room; room = room->next ) {
	  room->Save( );
	}
      }
    }

    // All player files.
    link_data link;
    link.connected = CON_PLAYING;
    
    for( int i = 0; i < max_pfile; ++i ) {
      pfile_data *pfile = pfile_list[i];
      bool save = true;
      bool extract = false;
      player_data *pl = 0;
      
      for( int j = 0; j < player_list; ++j ) {
	if( player_list[j]->pcdata->pfile == pfile
	    && player_list[j]->In_Game( ) ) {
	  pl = player_list[j];
	  break;
	}
      }
      
      if( !pl ) {
	if( !load_char( &link, pfile->name, PLAYER_DIR ) ) {
	  bug( "Load_players: error reading player file. (%s)", pfile->name );
	  continue;
	}
	
	pl = link.player;
	extract = true;

	count = 0;
	count += edit_objects( pl->contents );
	count += edit_objects( pl->wearing );
	count += edit_objects( pl->locker );

	for( int i = 0; i < pl->followers; ++i ) {
	  char_data *pet = pl->followers[i];
	  count += edit_objects( pet->contents );
	  count += edit_objects( pet->wearing );
	}

	if( count == 0 )
	  save = false;
      }
      
      if( save ) {
	pl->Save( false );
      }
      
      if( extract ) {
	pl->Extract();
	extracted.delete_list();
      }
    }

    return;
  }


  for( int i = 0; i < max_help; ) {
    help_data *help = help_list[i];
    if( help->category == 19 ) {
      delete help;
    } else {
      ++i;
    }
  }
}
