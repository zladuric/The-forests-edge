#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <unistd.h>
#include "define.h"
#include "struct.h"
#include "html.h"


typedef int pfcomp_func   ( pfile_data*, pfile_data* );


pfile_data*       ident_list  [ MAX_PFILE ];
pfile_data**      pfile_list  = 0;
pfile_data**      high_score  = 0;
pfile_data**      death_list  = 0;
pfile_data**      high_quest  = 0;
pfile_data**       site_list  = 0;
int                max_pfile  = 0;
int                 max_high  = 0;
int                max_death  = 0;
int                max_quest  = 0;
int             site_entries  = 0;


int          search          ( pfile_data**, int, pfile_data*, pfcomp_func );
pfcomp_func  compare_exp;
pfcomp_func  compare_death;
pfcomp_func  compare_quest;


/*
 *   PFILE_DATA CLASS
 */ 


Pfile_Data :: Pfile_Data( const char* string )
  : pwd(empty_string), account(0), last_host(empty_string),
    settings(0), trust(0), deaths(0), quest(0), level(0), remort(0),
    race(0), clss(0), sex(SEX_RANDOM), alignment(ALGN_PURE_NEUTRAL),
    bounty(0),
    last_on(current_time), last_note(last_on), created(last_on),
    ident(-1), serial(-1), exp(0), rank(-1), guesses(0), clan(0),
    homepage(empty_string), home_modified(false)
{
  record_new( sizeof( pfile_data ), MEM_PFILE );

  name = alloc_string( string, MEM_PFILE );

  flags[0] = ( 1 << PLR_GOSSIP );
  flags[1] = 0;

  vzero( vote, MAX_VOTE );
  vzero( permission, 2 );
}


Pfile_Data :: ~Pfile_Data( )
{
  record_delete( sizeof( pfile_data ), MEM_PFILE );

  if( ident > 0 ) {
    remove_list( pfile_list, max_pfile, this );
    remove_list( site_list, site_entries, this );
    ident_list[ident] = 0; 
  }
  
  if( rank >= 0 ) {
    remove_list( high_score, max_high, this );
    remove_list( death_list, max_death, this );
    remove_list( high_quest, max_quest, this );
  }

  clear_auction( this );

  if( clan ) 
    remove_member( this );

  for( int i = 0; i < max_pfile; ++i ) 
    for( int j = 0; j < MAX_VOTE; ++j ) 
      if( pfile_list[i]->vote[j] == this )
        pfile_list[i]->vote[j] = 0;
  
  for( int i = 0; i < MAX_IFLAG+1; ++i ) {
    for( info_data *info = info_history[i]; info; info = info->next ) {
      if( info->pfile == this ) {
	info->pfile = 0;
      }
    }
  }

  free_string( name,      MEM_PFILE );
  free_string( pwd,       MEM_PFILE );
  free_string( last_host, MEM_PFILE );
  free_string( homepage,  MEM_PFILE );
}


Pc_Data::Pc_Data ()
  : title(empty_string),
    tmp_keywords(empty_string), tmp_short(empty_string),
    buffer(empty_string), paste(empty_string), prompt(empty_string),
    message(( 1 << MAX_MESSAGE )-1), mess_settings(0),
    trust(0), clss(0), religion(REL_NONE),
    terminal(0), lines(24), columns(80),
    speaking(-1), piety(0), practice(-1), prac_timer(5),
    quest_pts(0),
    max_level(-1), mod_age(0), wimpy(0),
    mail(0), mail_edit(0), pfile(0), recognize(0),
    burden(-1),
    dictionary(empty_string), journal(empty_string)
{
  vzero( cflags, MAX_CFLAG );
  vzero( color, SAVED_COLORS );
  vzero( quest_flags, MAX_QUEST );
  vzero( iflag, 2 );
}


Pc_Data::~Pc_Data ( )
{
  free_string( title,        MEM_PLAYER );
  free_string( buffer,       MEM_PLAYER ); 
  free_string( paste,        MEM_PLAYER );
  free_string( tmp_short,    MEM_PLAYER );
  free_string( tmp_keywords, MEM_PLAYER );
  free_string( prompt,       MEM_PLAYER );
  free_string( dictionary,   MEM_PLAYER );
  free_string( journal,      MEM_PLAYER );
  
  delete recognize;

  delete mail_edit;
  delete_list( mail );
}


void extract( pfile_data *pfile, link_data *prev )
{
  link_data*       next;
  int               pos  = 0;

  for( link_data *link = link_list; link; link = next ) {
    next = link->next; 
    if( link->pfile == pfile && link != prev ) {
      send( link,
	    "\n\r\n\r+++ Character was deleted - Closing link +++\n\r" );
      close_socket( link, true );
    }
  }
  
  for( int i = 0; i < player_list; ++i ) {
    player_data *victim = player_list[i];
    if( victim->Is_Valid( )
	&& ( pos = search( victim->pcdata->recognize, pfile->ident ) ) >= 0 )
      remove( victim->pcdata->recognize, pos );
  }
  
  for( int i = 0; i < obj_list; ++i ) {
    obj_data *obj = obj_list[i];
    if( obj->Is_Valid() && obj->owner == pfile ) {
      if( obj->pIndexData->vnum == OBJ_CACHE ) {
	obj->Extract( );
	continue;
      }
      remove_bit( obj->extra_flags, OFLAG_ONE_OWNER );
      obj->owner = 0;
      if( obj->pIndexData->item_type != ITEM_MONEY ) {
	Content_Array *array = obj->array;
	while( array ) {
	  if( thing_data *where = array->where ) {
	    if( char_data *ch = character( where ) ) {
	      if( player_data *pl = player( ch ) ) {
		obj->owner = pl->pcdata->pfile;
	      } else if( is_set( ch->status, STAT_PET ) ) {
		if( player_data *pl = player( ch->leader ) ) {
		  obj->owner = pl->pcdata->pfile;
		}
	      }
	      break;
	    } else if( Room( where ) ) {
	      break;
	    } else if( Auction( where ) ) {
	      break;
	    }
	    array = where->array;
	  } else {
	    array = 0;
	  }
	}
      }
      consolidate( obj );
    }
  }
  
  for( int i = 0; i < MAX_IFLAG+1; ++i ) {
    for( info_data *info = info_history[i]; info; info = info->next ) {
      if( info->pfile == pfile ) {
	info->pfile = 0;
      }
    }
  }

  delete pfile; 
}


static int assign_ident( )
{
  for( int i = 1; i < MAX_PFILE; i++ )
    if( !ident_list[i] )
      return i;

  roach( "Assign_ident: ident_list full." );
  shutdown( "ident_list full" );
  return 0;	// To avoid compiler warning.
}


static int assign_serial( )
{
  return max_serial++;
}


void modify_pfile( char_data* ch )
{
  pfile_data *pfile = ch->pcdata->pfile;
  player_data *pc = (player_data*) ch;

  if( pfile->ident < 1 || pfile->ident > MAX_PFILE ) {
    pfile->ident = assign_ident( );
    pfile->serial = assign_serial( );
  }

  pfile->race      = ch->shdata->race;
  pfile->deaths    = ch->shdata->deaths;
  pfile->alignment = ch->shdata->alignment;

  pfile->clss      = ch->pcdata->clss; 
  pfile->religion  = ch->pcdata->religion;
  pfile->quest     = ch->pcdata->quest_pts;

  pfile->sex       = ch->sex;
  pfile->level     = ch->Level();
  pfile->remort    = pc->remort;
  pfile->exp       = ch->exp;
  
  if( !ident_list[pfile->ident] ) {
    add_list( pfile_list, max_pfile, pfile );
    add_list( site_list, site_entries, pfile ); 
    ident_list[pfile->ident] = pfile;
    
    if( ch->pcdata->trust < LEVEL_APPRENTICE ) {
      add_list( high_score, max_high, pfile );
      add_list( death_list, max_death, pfile );
      add_list( high_quest, max_quest, pfile );
    }

  } else {
    if( ident_list[pfile->ident] != pfile ) {
      roach( "Modify_Pfile: non-unique identity." );
      roach( "-- Ch = { %s, %s }",
	     pfile->name, ident_list[pfile->ident]->name );
      panic( "-- Id = %d", pfile->ident );
    }

    if( pfile->trust < LEVEL_APPRENTICE
	&& ch->pcdata->trust >= LEVEL_APPRENTICE ) {
      remove_list( high_score, max_high, pfile );
      remove_list( death_list, max_death, pfile );
      remove_list( high_quest, max_quest, pfile );
    } else if( pfile->trust >= LEVEL_APPRENTICE
	       && ch->pcdata->trust < LEVEL_APPRENTICE ) {
      add_list( high_score, max_high, pfile );
      add_list( death_list, max_death, pfile );
      add_list( high_quest, max_quest, pfile );
    }
  }

  pfile->trust = ch->pcdata->trust;
}


/*
 *   SITE LIST ROUTINES
 */


int site_search( const char* word )
{
  int      min  = 0;
  int    value;
  int      mid;
  int      max;

  if( !site_list )
    return -1;

  max = site_entries-1;

  while( true ) {
    if( max < min )
      return -min-1;

    mid    = (max+min)/2;
    value  = rstrcasecmp( site_list[mid]->last_host, word );

    if( value == 0 ) 
      break;
    if( value < 0 )
      min = mid+1;
    else 
      max = mid-1;
    }

  for( ; mid > 0; mid-- )
    if( strcasecmp( site_list[mid-1]->last_host, word ) )
      break;

  return mid;
}


/*
 *   PLAYER NAME LIST
 */


void add_list( pfile_data**& list, int& size, pfile_data* pfile )
{
  int pos;
  int   i;

  if( list == site_list ) {
    pos = site_search( pfile->last_host );
  } else if( list == high_score ) {
    pos = search( high_score, max_high, pfile, compare_exp );
    if( pos < 0 )
      pos = -pos-1;
    pfile->rank = pos; 
    for( i = pos; i < max_high; i++ )
      ++high_score[i]->rank;
  } else if( list == death_list ) {
    if( ( pos = search( death_list, max_death, pfile, compare_death ) ) >= 0 )  {
      roach( "Add_list: Repeated name %s (pos %d) in high death list.", pfile->name, pos );
      return;
    }
  } else if( list == high_quest ) {
    if( ( pos = search( high_quest, max_quest, pfile, compare_quest ) ) >= 0 )  {
      roach( "Add_list: Repeated name %s (pos %d) in high quest list.", pfile->name, pos );
      return;
    }
  } else {
    if( ( pos = pntr_search( pfile_list, max_pfile, pfile->name ) ) >= 0 ) {
      roach( "Add_list: Repeated name %s.", pfile->name );
    }
  }
  
  insert( list, size, pfile, pos < 0 ? -pos-1 : pos );
}


void remove_list( pfile_data**& list, int& size, pfile_data* pfile )
{
  int  ident  = pfile->ident;
  int    pos;

  if( ident < 0 )
    return;

  if( list == site_list ) {
    for( pos = site_search( pfile->last_host );
      pos < site_entries && ( pos < 0 || site_list[pos] != pfile ); pos++ ) 
      if( pos < 0
        || strcasecmp( site_list[pos]->last_host, pfile->last_host ) ) {
        roach( "Remove_Site: Pfile not found!?" );
        roach( "-- Pfile = %s", pfile->name );
        roach( "--  Host = %s", pfile->last_host );
        for( pos = 0; pos < size; pos++ )
          if( site_list[pos] == pfile ) {
            remove( list, size, pos );
            roach( "Remove_Site: Pfile found at wrong position!" );
            break;
	  }          
        return;
      }
  } else if( list == high_score ) {
    for( int i = pfile->rank+1; i < size; ++i )
      list[i]->rank--;
    pos = pfile->rank;
    pfile->rank = -1;
  } else if( list == death_list ) {
    pos = search( list, size, pfile, compare_death );
  } else if( list == high_quest ) {
    pos = search( list, size, pfile, compare_quest );
  } else {
    pos = pntr_search( list, size, pfile->name );
  }
  
  if( pos < 0 ) {
    roach( "Remove_list: Pfile not found in list!?" );
    return;
  }
  
  remove( list, size, pos );
}


/*
 *   SEARCH ROUTINES
 */


pfile_data* player_arg( const char *& argument )
{
  for( int i = strlen( argument ); i > 0; ) {
    int pos = pntr_search( pfile_list, max_pfile, argument, i );
    if( pos < 0 )
      pos = -pos-1;
    for( ; pos < max_pfile; ++pos ) {
      if( !strncasecmp( argument, pfile_list[pos]->name, i ) ) {
          argument += i;
          skip_spaces( argument );
          return pfile_list[pos];
      }
      break;
    }
    for( ; --i > 0 && isgraph( argument[i] ); );   
  }

  return 0;
}
 

pfile_data* find_pfile( const char *name, char_data* ch )
{
  int i = pntr_search( pfile_list, max_pfile, name );

  if( i >= 0 )
    return pfile_list[i];

  if( -i-1 < max_pfile
      && !strncasecmp( pfile_list[-i-1]->name, name, strlen( name ) ) )
    return pfile_list[-i-1];

  if( ch )
    send( ch, "No such player exists.\n\r" );

  return 0;
} 


pfile_data* find_pfile_exact( const char *name )
{
  const int i = pntr_search( pfile_list, max_pfile, name );

  return( i < 0 ? 0 : pfile_list[i] );
}


pfile_data* find_pfile_substring( const char* name )
{
  char      tmp  [ 4 ];
  int         i;

  memcpy( tmp, name, 3 );
  tmp[3] = '\0'; 
 
  if( ( i = pntr_search( pfile_list, max_pfile, tmp ) ) < 0 )
    i = -i-1;

  for( ; i < max_pfile; i++ ) {
    char *search = pfile_list[i]->name;
    if( strncasecmp( search, tmp, 3 ) )
      break;
    if( !strncasecmp( search, name, strlen( search ) ) )
      return pfile_list[i];
  }

  return 0;
}


player_data *find_player( const pfile_data *pfile )
{
  if( !pfile )
    return 0;

  for( int i = 0; i < player_list; ++i ) {
    player_data *player = player_list[i];
    // Note: must check Is_Valid( ) here...
    // Invalid players stay in player_list after Extract( ).
    if( player->pcdata->pfile == pfile && player->Is_Valid( ) ) {
      return player;
    }
  }

  return 0;
}


/*
 *   PLAYER FILE COMMANDS
 */


void forced_quit( player_data* ch, bool crash )
{
  link_data *link  = ch->link;

  if( ch->switched )
    do_return( ch->switched, "" );

  if( link ) {
    if( link->connected != CON_PLAYING ) {
      close_socket( link, true );
      return;
    }
    reset_screen( ch );
    send( link, "[0m\n\r" );
    send( link, "Thank you for visiting The Forest's Edge.\n\r" );
  }
  
  fsend_seen( ch, "%s is gone, although you didn't see %s leave.",
	      ch, ch->Him_Her() );

  // Do not use static_strings here.
  char tmp1 [ THREE_LINES ];
  char tmp2 [ THREE_LINES ];

  snprintf( tmp1, THREE_LINES, "%s has quit.", ch->descr->name );
  snprintf( tmp2, THREE_LINES, "%s has quit at %s.",
	    ch->descr->name,
	    ch->Location( ) );
  const int inv = invis_level( ch );

  if( !crash )
    remove_bit( ch->pcdata->pfile->flags, PLR_CRASH_QUIT ); 

  ch->pcdata->pfile->last_on = ch->pcdata->pfile->last_note = current_time;
  ch->Save( false );	// Prevent last_on memory/disk mismatch.

  link_data *link_next;
  for( link_data *link_new = link_list; link_new; link_new = link_next ) {
    link_next = link_new->next; 
    if( link_new->character && link_new != link
	&& !strcmp( link_new->character->descr->name, ch->descr->name ) ) {
      send( link_new, "\n\r\n\rFor security reasons closing link : please reconnect.\n\r" );
      close_socket( link_new, true );
    }
  }
  
  //  if( !strcasecmp( ch->descr->name, "Guest" ) ) {
  //    delete_file( PLAYER_DIR, ch->descr->name );
  //    delete ch->pcdata->pfile;
  //  }

  ch->Extract( );

  // Print this after the extract, so we see other messages first.
  info( inv, tmp1, LEVEL_BUILDER, tmp2, IFLAG_LOGINS, 1, ch );

  if( link ) 
    close_socket( link, true );
}


void do_quit( char_data* ch, const char *)
{
  if( is_mob( ch )
      || is_fighting( ch, "quit" ) ) {
    return;
  }

  for( int i = 0; i < ch->followers; ++i ) {
    char_data *fol = ch->followers[i];
    if( is_set( fol->status, STAT_PET )
	&& is_fighting( fol, 0 ) ) {
      fsend( ch, "You can't quit while %s, your pet, is fighting.",
	     fol );
      return;
    }
  }

  if( ch->position < POS_STUNNED  ) {
    send( ch, "You're not dead, yet.\n\r" );
    return;
  }

  if( ch->Level() < LEVEL_APPRENTICE
      && can_pkill( ch, 0, false ) ) {
    send( ch, "You cannot quit in rooms which allow player killing.\n\r" );
    return;
  }

  player_data *pc = player( ch );

  forced_quit( pc, is_set( pc->status, STAT_FORCED ) );
}


void do_delete( char_data* ch, const char *argument )
{
  if( is_mob( ch ) ) 
    return;

  if( is_demigod( ch ) ) {
    if( !*argument ) {
      send( ch, "Syntax: delete <player> <password>\n\r" );
      return;
    }
    char arg [ MAX_INPUT_LENGTH ];
    argument = one_argument( argument, arg );
    pfile_data *pfile = find_pfile_exact( arg );
    if( !pfile ) {
      fsend( ch, "Player \"%s\" not found.", arg );
      return;
    }
    if( pfile == ch->pcdata->pfile ) {
      send( ch, "You can't delete yourself.\n\r" );
      return;
    }
    
    player_data *pl = find_player( pfile );

    if( pl && pl->Level( ) == 0 ) {
      // Player is being created.
      fsend( ch, "Player \"%s\" not found.", arg );
      return;
    }

    /*
    player_data *pl = 0;
    for( int i = 0; i < player_list; ++i ) {
      if( player_list[i]->pcdata->pfile == pfile ) {
	if( player_list[i]->Level( ) == 0 ) {
	  // Player is being created.
	  fsend( ch, "Player \"%s\" not found.", arg );
	  return;
	}
	pl = player_list[i];
	break;
      }
    }
    */

    if( !*argument ) {
      send( ch, "Syntax: delete <player> <password>\n\r" );
      return;
    }

    if( strcmp( argument, pfile->pwd ) ) {
      send( ch, "That password is not correct.\n\r" );
      return;
    }

    if( !pl ) {
     link_data link;
     if( !load_char( &link, pfile->name, PLAYER_DIR ) ) {
	bug( "Load_players: error reading player file. (%s)", pfile->name );
	return;
      }
      pl = link.player;
      pl->link = 0;
      link.player = 0;
      link.character = 0;

    } else {
      fsend_seen( pl, "A heavenly sword impales %s.", pl );
      fsend_seen( pl, "Death gratefully takes %s's spirit.", pl );
      
      //      clear_screen( pl );
      reset_screen( pl );
      
      send( pl, "Your character has been deleted by the gods.\n\r" );
      send( pl, "Death is surprisingly peaceful.\n\r" );
      send( pl, "Good night.\n\r" );
    }
    
    char *tmp1 = static_string( );
    char *tmp2 = static_string( );
    snprintf( tmp1, THREE_LINES, "%s deleted by %s.",
	      pl->descr->name,
	      ch->descr->name );
    snprintf( tmp2, THREE_LINES, "%s (account %s) deleted by %s.",
	      pl->descr->name,
	      pl->pcdata->pfile->account->name,
	      ch->descr->name );
    info( invis_level( ch ), tmp1, LEVEL_BUILDER, tmp2, IFLAG_ADMIN, 1 );
    
    player_log( pl, "Deleted from account \"%s\" as \"%s\" by %s.",
		pl->pcdata->pfile->account->name,
		pl->real_name( ),
		ch->real_name( ) );

    purge( pl );
    return;
  }

  if( ( ch->Level( ) > 5 || ch->pcdata->pfile->remort > 0 )
      && ch->pcdata->trust < LEVEL_BUILDER ) {
    fsend( ch, "Characters over level five may not delete.  Time has\
 shown that players often delete and then regret the action.  It also\
 serves no purpose.  If you no longer wish to play just quit and do not\
 return." );
    return;
  }

  if( strcmp( argument, ch->pcdata->pfile->pwd ) ) {
    send( ch,
	  "You must type \"delete <password>\" to delete your character.\n\r" );
    return;
  }
  
  fsend_seen( ch, "%s throws %sself on %s sword.", ch,
	      ch->Him_Her( ), ch->His_Her( ) );
  fsend_seen( ch, "Death gratefully takes %s's spirit.", ch );
  
  //  clear_screen( ch );
  reset_screen( ch );
  
  send( ch, "Your character has been vaporized.\n\r" );
  send( ch, "Death is surprisingly peaceful.\n\r" );
  send( ch, "Good night.\n\r" );

  char *tmp1 = static_string( );
  char *tmp2 = static_string( );
  snprintf( tmp1, THREE_LINES, "%s has deleted %sself.",
	    ch->descr->name,
	    ch->Him_Her( ) );
  snprintf( tmp2, THREE_LINES, "%s (account %s) has deleted %sself at %s.",
	    ch->descr->name,
	    ch->pcdata->pfile->account->name,
	    ch->Him_Her( ),
	    ch->Location( ) );
  info( invis_level( ch ), tmp1, LEVEL_BUILDER, tmp2, IFLAG_ADMIN, 1, ch );
  
  player_log( ch, "Deleted from account \"%s\" as \"%s\".",
	      ch->pcdata->pfile->account->name,
	      ch->real_name( ) );

  purge( (player_data*) ch );
}


void purge( player_data* ch )
{
  delete_file( PLAYER_DIR,      ch->descr->name, false );
  delete_file( PLAYER_PREV_DIR, ch->descr->name, false );
  delete_file( BACKUP_DIR,      ch->descr->name, false );
  delete_file( MAIL_DIR,        ch->descr->name, false );
  delete_file( LOST_FOUND_DIR,  ch->descr->name, false );

  if( ch->link && boot_stage == 2 ) {
    ch->link->player = 0;
    ch->link->character = 0;
    close_socket( ch->link, true );
    ch->link = 0;
  }

  // Objects are extracted.
  ch->char_data::Extract( );

  if( pfile_data *pfile = ch->pcdata->pfile )
    extract( pfile );

  save_html_players( );
}


/*
 *  SAVE
 */


void do_save( char_data* ch, const char *)
{
  if( is_mob( ch ) )
    return;

  player_data *pc = (player_data*) ch;

  if( !is_demigod( ch )
      && pc->save_time > current_time + 180
      && !is_set( pc->status, STAT_FORCED ) ) {
    send( ch, "You are saving your character too frequently - request denied.\n\r" );
  } else {
    pc->Save( );
    send( ch, "Your character has been saved.\n\r" );
  }
}


/*
 *   INFORMATIONAL COMMANDS 
 */


void do_title( char_data* ch, const char *argument )
{
  if( is_mob( ch ) )
    return;

  if( !is_set( ch->pcdata->pfile->flags, PLR_APPROVED ) ) {
    send( ch, "You cannot set your title until your appearance has been approved.\n\r" );
    return;
  }

  player_data *pc = player( ch );

  if( ch->Level( ) < LEVEL_APPRENTICE ) {
    if( pc->gossip_pts < 0 ) {
      send( ch, "You cannot set your title when your gossip points are negative.\n\r" );
      return;
    }
    
    if( ch->in_room->vnum == ROOM_PRISON ) {
      send( ch, "You cannot set your title while in prison.\n\r" );
      return;
    }
  }

  int flags;

  if( !get_flags( ch, argument, &flags, "lx", "Title" ) )
    return;

  if( is_set( flags, 0 )
      && is_immortal( ch ) ) {

    char arg1 [MAX_INPUT_LENGTH];
    argument = one_argument( argument, arg1 );

    char_data *victim = ch;

    if( *argument ) {
      if( !( victim = one_player( ch, argument, "title", 
				  (thing_array*) &player_list ) ) )
	return;
    }

    wizard_data *imm = wizard( victim );

    if( !imm ) {
      send( ch, "Only immortals may have level titles.\n\r" );
      return;
    }

    if( strlen( arg1 ) > 13 ) {
      send( ch, "Your level title must be less than 13 characters.\n\r" );
      return;
    }

    free_string( imm->level_title, MEM_WIZARD );
    imm->level_title = alloc_string( arg1, MEM_WIZARD );

    if( imm == ch )
      fsend( ch, "Your level title has been set to \"%s\".\n\r", 
	     arg1 );
    else {
      fsend( imm, "Your level title has been set to \"%s\".\n\r", 
	     arg1 );
      fsend( ch, "%s's level title has been set to \"%s\".\n\r", 
	     imm, arg1 );
    }

    return;
  }

  if( !*argument ) {
    if( *ch->pcdata->title ) {
      fsend( ch, "Your title is \"%s\".", ch->pcdata->title );
    }
    send( ch, "To what do you want to set your title?\n\r" );
    return;
  }

  free_string( ch->pcdata->title, MEM_PLAYER );

  char tmp [ MAX_STRING_LENGTH ];

  const bool chomp = !is_set( flags, 1 )
    && ( *argument == ','
	 || *argument == '.'
	 || *argument == '\''
	 || *argument == ':' );

  snprintf( tmp, MAX_STRING_LENGTH, "%s%s",
	    chomp ? "" : " ", argument );
  
  tmp[ MAX_INPUT_LENGTH ] = '\0';

  char buf [ MAX_STRING_LENGTH ];
  unsigned titlen = convert_to_ansi( ch, MAX_STRING_LENGTH, tmp, buf );

  if( titlen > 53 ) {
    int save_term = ch->pcdata->terminal;
    ch->pcdata->terminal = TERM_DUMB;
    titlen = convert_to_ansi( ch, MAX_STRING_LENGTH, tmp, buf );
    ch->pcdata->terminal = save_term;
    ch->pcdata->title = alloc_string( trunc( buf, 53 ).c_str(), MEM_PLAYER );
  } else {
    ch->pcdata->title = alloc_string( tmp, MEM_PLAYER );
  }

  send( ch, "Your title has been changed.\n\r" );
}


void do_password( char_data* ch, const char *argument )
{
  char       arg  [ MAX_INPUT_LENGTH ];
  const char**     pwd;
  int      flags;

  if( is_mob( ch ) 
      || !get_flags( ch, argument, &flags, "a", "Password" ) )
    return;
  
  argument = one_argument( argument, arg );

  if( !*arg || !*argument ) {
    send( ch, "Syntax: password <old> <new>.\n\r" );
    return;
  }

  int type;
  if( is_set( flags, 0 ) ) {
    if( !ch->pcdata->pfile->account ) {
      send( ch, "You don't have an account to change the password of.\n\r" );
      return;
    }
    pwd = &ch->pcdata->pfile->account->pwd;
    type = MEM_ACCOUNT;
  } else {
    pwd = &ch->pcdata->pfile->pwd;
    type = MEM_PFILE;
  }
  
  if( strcmp( arg, *pwd ) ) {
    send( ch, "Wrong password.\n\r" );
    bug( "Changing Password: Incorrect guess - %s.", ch->descr->name );
    return;
  }

  if( strlen( argument ) < 5 ) {
    send( ch, "New password must be at least five characters long.\n\r" );
    return;
  }
  
  free_string( *pwd, type );
  *pwd = alloc_string( argument, type );
  
  if( is_set( flags, 0 ) ) {
    send( ch, "Account password changed.\n\r" );
    save_accounts( );
  } else {
    send( ch, "Password changed.\n\r" );
    player_data *pc = (player_data*) ch;
    pc->Save( );
  }
}


/*
 *   RENAME
 */


void do_rename( char_data* ch, const char *argument )
{
  char           arg  [ MAX_INPUT_LENGTH ];
  char           tmp  [ THREE_LINES ];
  char           tmp1  [ THREE_LINES ];
  player_data*  victim;

  in_character = false;

  argument = one_argument( argument, arg );
  
  if( !( victim = one_player( ch, arg, "rename",
			      (thing_array*) &player_list ) ) )
    return;

  if( victim != ch && get_trust( victim ) >= get_trust( ch ) ) {
    send( ch, "You can only rename those of lower trust level.\n\r" );
    return;
  }
  
  if( victim == ch && get_trust( ch ) < LEVEL_SPIRIT ) {
    send( ch, "To avoid temptation you are unable to rename yourself.\n\r" );
    return;
  }
  
  if( *argument == '\0' ) {
    send( ch, "What do you want to rename them?\n\r" );
    return;
  }  
  
  char arg1 [ MAX_STRING_LENGTH ];
  strcpy( arg1, argument );
  *arg1 = toupper( *arg1 );

  if( !check_parse_name( ch->link, arg1 ) )
    return;

  const char *name = victim->descr->name;

  player_log( victim, "Name changed from %s to %s by %s.",
	      name, arg1, ch->real_name( ) ); 
  player_log( ch, "Renamed player %s to %s.",
	      name, arg1 );

  rename_file( PLAYER_DIR,      name, PLAYER_DIR,      arg1 );
  rename_file( PLAYER_PREV_DIR, name, PLAYER_PREV_DIR, arg1 );
  rename_file( PLAYER_LOG_DIR,  name, PLAYER_LOG_DIR,  arg1 );
  rename_file( BACKUP_DIR,      name, BACKUP_DIR,      arg1 );
  rename_file( MAIL_DIR,        name, MAIL_DIR,        arg1 );
  rename_file( LOST_FOUND_DIR,  name, LOST_FOUND_DIR,  arg1 );

  pfile_data *pfile = victim->pcdata->pfile;

  remove_list( pfile_list, max_pfile, pfile );

  if( pfile->trust < LEVEL_APPRENTICE ) {
    remove_list( high_score, max_high, pfile );
    remove_list( death_list, max_death, pfile );
    remove_list( high_quest, max_quest, pfile );
  }

  for( int i = 0; i < MAX_NOTEBOARD; ++i ) {
    if( i != NOTE_CLAN ) {
      bool save = false;
      for( int j = 0; j < max_note[i]; ++j ) {
	note_data *note = note_list[i][j];
	if( !strcmp( name, note->from ) ) {
	  free_string( note->from, MEM_NOTE );
	  note->from = alloc_string( arg1, MEM_NOTE );
	  save = true;
	}
      }
      if( save ) {
	save_notes( i );
      }
    }
  }

  for( int i = 0; i < max_clan; ++i ) {
    bool save = false;
    for( int j = 0; j < clan_list[i]->max_note; ++j ) {
      note_data *note = clan_list[i]->note_list[j];
      if( !strcmp( name, note->from ) ) {
	free_string( note->from, MEM_NOTE );
	note->from = alloc_string( arg1, MEM_NOTE );
	save = true;
      }
    }
    if( save ) {
      save_notes( clan_list[i] );
    }
  }

  if( is_apprentice( victim ) ) {
    bool save = false;
    
    for( int i = 1; i <= species_max; ++i ) {
      if( species_data *species = species_list[i] ) {
	if( !strcmp( name, species->creator ) ) {
	  free_string( species->creator, MEM_SPECIES );
	  species->creator = alloc_string( arg1, MEM_SPECIES );
	}
	if( !strcmp( name, species->last_mod ) ) {
	  free_string( species->last_mod, MEM_SPECIES );
	  species->last_mod = alloc_string( arg1, MEM_SPECIES );
	}
      }
    }
    
    if( save )
      save_mobs( );
    
    save = false;
    
    for( int i = 1; i <= obj_clss_max; ++i ) {
      if( obj_clss_data *obj_clss = obj_index_list[i] ) {
	if( !strcmp( name, obj_clss->creator ) ) {
	  free_string( obj_clss->creator, MEM_OBJ_CLSS );
	  obj_clss->creator = alloc_string( arg1, MEM_OBJ_CLSS );
	}
	if( !strcmp( name, obj_clss->last_mod ) ) {
	  free_string( obj_clss->last_mod, MEM_OBJ_CLSS );
	  obj_clss->last_mod = alloc_string( arg1, MEM_OBJ_CLSS );
	}
      }
    }

    if( save )
      save_objects( );
  }

  if( ch == victim ) {
    fsend( ch, "You rename yourself %s.", arg1 );
  } else {
    fsend( ch, "%s renamed %s.", name, arg1 );
    fsend( victim, "You have been renamed %s by %s.", arg1,
	  ch->descr->name );
  }
  
  snprintf( tmp, THREE_LINES, "%s renamed %s by %s.",
	    name, arg1, ch->descr->name );
  snprintf( tmp1, THREE_LINES, "%s renamed %s.",
	    name, arg1 );
  info( LEVEL_AVATAR, tmp1, invis_level( ch ), tmp, IFLAG_ADMIN, 2, ch );

  free_string( pfile->name, MEM_PFILE );
  pfile->name = alloc_string( arg1, MEM_PFILE );

  free_string( victim->descr->name, MEM_DESCR );
  victim->descr->name = alloc_string( arg1, MEM_DESCR );
 
  add_list( pfile_list, max_pfile, pfile );

  if( pfile->trust < LEVEL_APPRENTICE ) {
    add_list( high_score, max_high, pfile );
    add_list( death_list, max_death, pfile );
    add_list( high_quest, max_quest, pfile );
  }

  if( victim->link && victim->link->connected == CON_PLAYING ) {
    // Re-insert into link_list to preserve alpha order.
    victim->link->set_playing();
  }

  victim->Save( );
}


/*
 *   HIGH SCORE ROUTINES
 */


static const char* score_name( pfile_data **list, int limit,
			       int i, char_data* ch )
{
  if( i < 0 || i >= limit )
    return "---";

  if( is_incognito( list[i], ch ) ) {
    return "???";
  }

  int score;
  if( list == high_score ) {
    score = list[i]->level;
  } else if( list == death_list ) {
    score = list[i]->deaths;
  } else {
    score = list[i]->quest;
  }

  char *tmp = static_string();
  snprintf( tmp, THREE_LINES, "%s (%d)",
	    list[i]->name, score );
  return tmp;
}


void do_high( char_data* ch, const char *argument )
{
  char       tmp  [ TWO_LINES ];
  char        r1  [ 8 ];
  char        r2  [ 8 ];
  int        who  [ 20 ];
  int     c1 = 0, c2;
  int          i;
  int       clss  = MAX_CLSS;
  int       race  = MAX_PLYR_RACE;

  if( is_confused_pet( ch ) )
    return;
  
  if( matches( argument, "mob" ) ) {
    high_mob( ch, argument );
    return;
  }

  pfile_data **list = high_score;
  int limit = max_high;
  if( !matches( argument, "score" ) ) {
    if( matches( argument, "death" ) ) {
      list = death_list;
      limit = max_death;
    } else if( matches( argument, "quest" ) ) {
      list = high_quest;
      limit = max_quest;
    }
  }

  if( isalpha( *argument ) ) {
    for( race = 0; race < MAX_PLYR_RACE; race++ )
      if( matches( argument, race_table[race].name ) )
        break;
  }

  if( isalpha( *argument ) ) {
    for( clss = 0; clss < MAX_CLSS; clss++ )
      if( matches( argument, clss_table[clss].name ) )
        break;
  }

  if( number_arg( argument, c1 ) ) {
    if( c1 <= 0 ) {
      send( ch, "Error - number is out of range.\n\r" );
      return;
    }
  }

  if( *argument ) {
    send( ch, "Syntax: high [score|death|quest] [<race>] [<class>] [#]\n\r" );
    return;
  }

  if( clss != MAX_CLSS
      || race != MAX_PLYR_RACE ) {
    if ( limit > 19 && c1 != 0 ) {
      c1 = min( c1, limit-19 );
    } else {
      c1 = 1;
    }
    
    for( i = c2 = 0; c2-c1 < 19 && i < limit; ++i ) {
      if( ( clss == MAX_CLSS || list[i]->clss == clss )
	  && ( race == MAX_PLYR_RACE || list[i]->race == race )
	  && ++c2 >= c1 ) {
	who[c2-c1] = i;
      }
    }

    if( c1 > 1 && c2 < c1+19 ) {
      c1 = max( c2-19, 1 );
      for( i = c2 = 0; c2-c1 < 19 && i < limit; ++i ) {
	if( ( clss == MAX_CLSS || list[i]->clss == clss )
	    && ( race == MAX_PLYR_RACE || list[i]->race == race )
	    && ++c2 >= c1 ) {
	  who[c2-c1] = i;
	}
      }
    }

    for( i = c2-c1+1; i < 20; ++i )
      who[i] = -1;
    
    c2 = c1+10;

  } else {
    if( c1 == 0 ) {
      c1 = 1;
      if( list == high_score ) {
	c2 = max( 11, min( ch->pcdata->pfile->rank-4, max_high-9 ) );
      } else if( list == death_list ) {
	c2 = max( 11, min( search( list, limit, ch->pcdata->pfile, compare_death )-4, max_death-9 ) );
      } else {
	c2 = max( 11, min( search( list, limit, ch->pcdata->pfile, compare_quest )-4, max_quest-9 ) );
      }
    } else {
      if( limit > 19 ) {
	c1 = min( c1, limit-19 );
      } else {
	c1 = 1;
      }
      c2 = c1+10;
    }
    
    for( i = 0; i < 10; ++i ) {
      who[i] = c1+i-1;
      who[10+i] = c2+i-1;
    }
  }

  if( list == high_score ) {
    send( ch, "              _         _   _  _   _   _         _  ___ \n\r" );
    send( ch, "       |_| | | _ |_|   (_  |  | | |_) |_   |  | (_   |  \n\r" );
    send( ch, "       | | | |_| | |    _) |_ |_| | \\ |_   |_ |  _)  |  \n\r" );
  } else if( list == death_list ) {
    send( ch, "              _             _  _  ___             _  ___ \n\r" );
    send( ch, "       |_| | | _ |_|   |\\  |_ |_|  |  |_|   |  | (_   |  \n\r" );
    send( ch, "       | | | |_| | |   |_| |_ | |  |  | |   |_ |  _)  |  \n\r" );
  } else {
    send( ch, "              _         _       _  _  ___         _  ___ \n\r" );
    send( ch, "       |_| | | _ |_|   | | | | |_ (_   |    |  | (_   |  \n\r" );
    send( ch, "       | | | |_| | |   |_\\ |_| |_  _)  |    |_ |  _)  |  \n\r" );
  }
  send( ch, "\n\r\n\r" );
  
  for( i = 0; i < 10; ++i ) {
    snprintf( r1, 8, "[%d]", c1+i );
    snprintf( r2, 8, "[%d]", c2+i );
    snprintf( tmp, TWO_LINES, "%12s  %-20s %5s  %s\n\r",
	      r1, score_name( list, limit, who[i], ch ),
	      r2, score_name( list, limit, who[10+i], ch ) );
    send( ch, tmp );
  }
}


void update_score( char_data* ch )
{
  if( !ch->pcdata )
    return;

  pfile_data *pfile = ch->pcdata->pfile;

  if( pfile->rank < 0 ) {
    pfile->level = ch->Level();
    pfile->exp = ch->exp;
    pfile->deaths = ch->shdata->deaths;
    pfile->quest = ch->pcdata->quest_pts;
    return;
  }

  if( pfile->exp != ch->exp
      || pfile->level != ch->Level() ) {
    pfile->level = ch->Level();
    pfile->exp = ch->exp;
    bool found = true;
    while( found ) {
      found = false;
      int pos;
      for( pos = pfile->rank; pos > 0; --pos ) {
	if( compare_exp( pfile, high_score[pos-1] ) <= 0 )
	  break;
	++high_score[pos-1]->rank;
	swap( high_score[pos], high_score[pos-1] );
	found = true;
      }
      for( ; pos < max_high-2; ++pos ) {
	if( compare_exp( pfile, high_score[pos+1] ) >= 0 )
	  break;
	--high_score[pos+1]->rank;
	swap( high_score[pos], high_score[pos+1] );
	found = true;
      }
      pfile->rank = pos;
    }
  }

  // Update deaths list.
  if( pfile->deaths != ch->shdata->deaths ) {
    remove_list( death_list, max_death, pfile );
    pfile->deaths = ch->shdata->deaths;
    add_list( death_list, max_death, pfile );
  }

  // Update quest point list.
  if( pfile->quest != ch->pcdata->quest_pts ) {
    remove_list( high_quest, max_quest, pfile );
    pfile->quest = ch->pcdata->quest_pts;
    add_list( high_quest, max_quest, pfile );
  }
}


/*
 *   SEARCH HIGH & DEATH LISTS
 */
 

int compare_exp( pfile_data *p1, pfile_data* p2 )
{
  if( p1->level != p2->level )
    return( p1->level > p2->level ? 1 : -1 );
  
  if( p1->exp != p2->exp )
    return( p1->exp > p2->exp ? 1 : -1 );

  return strcasecmp( p2->name, p1->name );
}


int compare_death( pfile_data* p1, pfile_data* p2 )
{
  if( p1->deaths != p2->deaths )
    return( p1->deaths > p2->deaths ? 1 : -1 );
  
  return strcasecmp( p2->name, p1->name );
}


int compare_quest( pfile_data* p1, pfile_data* p2 )
{
  if( p1->quest != p2->quest )
    return( p1->quest > p2->quest ? 1 : -1 );
  
  return strcasecmp( p2->name, p1->name );
}


int search( pfile_data** list, int max, pfile_data* pfile,
	    pfcomp_func* compare )
{
  int     min  = 0;
  int     mid;
  int   value;

  if( !list )
    return -1;

  max--;

  while (true) {
    if( max < min )
      return -min-1;

    mid    = (max+min)/2;
    value  = ( *compare )( pfile, list[mid] );

    if( value == 0 ) {
      /*
      if( unique )
	break;

      do {
	if( !strcmp( pfile->name, list[mid]->name ) )
	  break;
	++mid;
	if( mid > max || ( *compare )( pfile, list[mid] ) ) {
	  mid = -1;
	}
      } while( mid > 0 );
      */

      break;
    }

    if( value < 0 )
      min = mid+1;
    else 
      max = mid-1;
  }

  return mid;
}


static int select( pfile_data *pfile, char_data *ch, const char *argument )
{
  while( true ) {
    char hyphen = *argument;

    if( !hyphen )
      return 1;

    char letter;

    if( hyphen != '-' ) {
      letter = 'n';
    } else {
      ++argument;
      if( !isalpha( letter = *argument++ ) ) {
        send( ch, "Illegal character for flag - See help pfind.\n\r" );
        return -1;
      }
    }

    bool negative = false;
    skip_spaces( argument );
    
    if( *argument == '!' ) {
      negative = true;
      ++argument;
    }
    
    if( !*argument || *argument == '-' || isspace( *argument ) ) {
      send( ch, "All flags require an argument - See help pfind.\n\r" );
      return -1;
    }
  
    int length = 0;
    char tmp [ MAX_INPUT_LENGTH ];

    while( *argument && strncmp( argument-1, " -", 2 ) ) {
      if( length > ONE_LINE-2 ) {
        send( ch, "Flag arguments must be less than one line.\n\r" );
        return -1;
      }
      tmp[length++] = *argument++;
    }

    for( ; isspace( tmp[length-1] ); --length );

    tmp[length] = '\0';
    
    if( letter == 'n' ) {
      if( !is_name( tmp, pfile->name ) )
        return 0;
      continue;
    }

    if( letter == 'p' ) {
      int i = 0;
      for( ; i < MAX_PERMISSION; ++i ) {
	if( !strncasecmp( tmp, permission_name[i], length ) ) {
          if( is_set( pfile->permission, i ) == negative ) {
            return 0;
	  }
          break;
	}
      }
      if( i == MAX_PERMISSION ) {
	send( ch, "Unknown permission flag \"%s\".\n\r", tmp );
	return -1;
      }
      continue;
    }

    if( letter == 'l' ) {
      int min, max;
      atorange( tmp, min, max );
      if( max < 0 )
	max = 100;
      if( pfile->level < min
	  || pfile->level > max )
        return 0;
      continue;
    }

    if( letter == 't' ) {
      int min, max;
      atorange( tmp, min, max );
      if( max < 0 )
	max = 100;
      if( pfile->trust < min
	  || pfile->trust > max )
        return 0;
      continue;
    }

    send( ch, "Unknown flag - See help pfind.\n\r" );
    return -1;
  }
}


void do_pfind( char_data *ch, const char *argument )
{
  unsigned count = 0;

  for( int i = 0; i < max_pfile; ++i ) {
    pfile_data *pfile = pfile_list[i];
    switch( select( pfile, ch, argument ) ) {
    case -1 : return;
    case  1 :
      if( count == 0 ) {
	page( ch, "\n\r" );
	page_underlined( ch,
			 "ID#     %-14s %-20s %-16s Trs Lvl Algn Sex Race Clss Clan\n\r",
			 "Name", "Account", "Last On" );
      }
      ++count;
      page( ch,
	    "[%5d] %-14s %-20s %-16s",
	    pfile->ident,
	    pfile->name,
	    pfile->account->name,
	    ltime( pfile->last_on, false, ch ) );
      page( ch, " %3s %3d   %2s   %c  %3s  %3s %4s\n\r",
	    pfile->trust > 0 ? int3( pfile->trust ) : "",
	    pfile->level,
	    alignment_table[ pfile->alignment ].abbrev,
	    toupper( *sex_name[ pfile->sex ] ),
	    race_table[ pfile->race ].abbrev,
	    clss_table[ pfile->clss ].abbrev,
	    pfile->clan ? pfile->clan->abbrev : "" );
    }
  }

  if( count == 0 ) {
    send( ch, "No player matching search was found.\n\r" );
  } else {
    page( ch, "\n\rFound %d match%s.\n\r",
	  count,
	  count == 1 ? "" : "es" );
  }
}


bool save_html_players( )
{
  /*
  char orig_cwd [ MAXPATHLEN+1 ];
  if( !getcwd( orig_cwd, MAXPATHLEN+1 ) )
    return false;

  mkdir( HTML_DIR, 0755 );

  if( chdir( HTML_DIR ) < 0 )
    return false;

  char save_cwd [ MAXPATHLEN+1 ];
  if( !getcwd( save_cwd, MAXPATHLEN+1 ) ) {
    return false;
  }
  */
  
  FILE *index = open_file( HTML_DIR, HTML_PLAYER_FILE, "w" );

  if( !index )
    return false;

  html_start( index, "TFE Players", "List of Players" );

  fprintf( index, "<table width=\"100%%\">" );

  for( char a = 'A'; a <= 'Z'; ++a ) {
    fprintf( index, "<td><a href=\"#%c\">%c</a></td>\n", a, a );
  }

  fprintf( index, "</table>\n" );

  char c = 0;

  for( int i = 0; i < max_pfile; ++i ) {
    pfile_data *pfile = pfile_list[i];
    if( is_set( pfile->flags, PLR_DENY ) )
      continue;
    const char *name = pfile->name;
    if( toupper( *name ) != c ) {
      c = toupper( *name );
      fprintf( index, "<hr><h3><u><a name=\"%c\">%c</a></h3></u>\n", c, c );
    }
    fprintf( index, "<p><div style=\"margin-left: 10px;\"><b>%s</b> the ", name );
    if( pfile->level >= LEVEL_APPRENTICE ) {
      fprintf( index, "%s",
	       imm_title[pfile->level-91] );
    } else {
      if( pfile->race < MAX_PLYR_RACE ) {
	fprintf( index, "<a href=\"help/Races/%c%s.html\">%s</a>",
		 toupper( *race_table[pfile->race].plural ),
		 race_table[pfile->race].plural+1,
		 race_table[pfile->race].name );
      } else {
	fprintf( index, race_table[pfile->race].name );
      }
      if( level_setting( &pfile->settings, SET_INCOGNITO ) == 0 ) {
	fprintf( index, " <a href=\"help/Classes/%c%s.html\">%s</a>",
		 toupper( *clss_table[pfile->clss].name ),
		 clss_table[pfile->clss].name+1,
		 clss_table[pfile->clss].name );
      }
    }
    /*
    } else if( level_setting( &pfile->settings, SET_INCOGNITO ) == 0 ) {
      fprintf( index, "%s %s",
	       race_table[pfile->race].name,
	       clss_table[pfile->clss].name );
    } else {
      fprintf( index, "%s",
	       race_table[pfile->race].name );
    }
    */
    fprintf( index, "<div style=\"margin-left: 30px;\">Last on: %s</div>\n",
	     ltime( pfile->last_on, false ) );
    /*
    if( is_set( pfile->flags, PLR_EMAIL_PUBLIC ) ) {
      fprintf( index, "<div style=\"margin-left: 30px;\">Email: <a href=\"mailto:%s\">%s</a></div>\n",
	       pfile->account->email, pfile->account->email );
    }
    */
    if( *pfile->homepage ) {
      const char *h = pfile->homepage;
      while( isalnum( *h ) ) {
	++h;
      }
      if( strncmp( h, "://", 3 ) ) {
	fprintf( index, "<div style=\"margin-left: 30px;\">Homepage: <a href=\"http://%s\">%s</a></div>\n",
		 pfile->homepage, pfile->homepage );
      } else {
	fprintf( index, "<div style=\"margin-left: 30px;\">Homepage: <a href=\"%s\">%s</a></div>\n",
		 pfile->homepage, pfile->homepage );
      }
    }

    pfile->home_modified = false;

    fprintf( index, "</div></p>\n" );
  }

  html_stop( index );
  
  /*
  if( chdir( orig_cwd ) < 0 )
    panic( "Save_HTML_PLayers: can't chdir back to run directory" );
  */

  return true;
}
