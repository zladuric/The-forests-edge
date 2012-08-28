#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <unistd.h>
#include "define.h"
#include "struct.h"


const char**  badname_array  = 0;
int             max_badname  = 0;

const char**  remort_array  = 0;
int             max_remort  = 0;

class ban_data;
ban_data* ban_list = 0;


class ban_data
{
public:
  ban_data*    next;
  const char*        name;

  ban_data( )
    : next(0), name(empty_string)
  {
    record_new( sizeof( ban_data ), MEM_BAN );
    append( ban_list, this );
  }
  
  ~ban_data( )
  {
    free_string( name, MEM_BAN );
    record_delete( sizeof( ban_data ), MEM_BAN );
    remove( ban_list, this );
  }
};


/*
 *   BAN/ALLOW ROUTINES
 */


void do_ban( char_data* ch, const char *argument )
{
  account_data*   account;
  ban_data*           ban;
  pfile_data*       pfile  = 0;
  int               flags;
  bool              first  = true;
  const char*        name;

  if( !get_flags( ch, argument, &flags, "srnR", "Ban" ) )
    return;
  
  if( is_set( flags, 2 ) ) {
    if( !*argument ) {
      if( !badname_array ) {
        send( ch, "The badname array is empty.\n\r" );
      } else {
        display_array( ch, "Bad Name Array", &badname_array[0],
		       &badname_array[1], max_badname );
      }
      return;
    }
    int i = pntr_search( badname_array, max_badname, argument );
    if( is_set( flags, 1 ) ) {
      if( i < 0 ) {
        fsend( ch, "The name \"%s\" isn't in the badname array.", argument );
      } else {
        record_delete( sizeof( char* ), MEM_BADNAME );
        remove( badname_array, max_badname, i );
        send( ch, "Name removed from badname array.\n\r" );
        save_badname( );
      }
    } else {
      if( i > 0 ) {
        fsend( ch, "The name \"%s\" is already in the badname array.", argument );
      } else if( check_parse_name( ch->link, argument ) ) {
        i = -i-1;
        record_new( sizeof( char* ), MEM_BADNAME );
        name = alloc_string( argument, MEM_BADNAME );
        insert( badname_array, max_badname, name, i );
        fsend( ch, "The name \"%s\" has been added to the badname array.", name );
        save_badname( );
      }
    }
    return;
  }

  if( is_set( flags, 3 ) ) {
    if( !*argument ) {
      if( !remort_array ) {
        send( ch, "The remort array is empty.\n\r" );
      } else {
        display_array( ch, "Remort Name Array", &remort_array[0],
		       &remort_array[1], max_remort );
      }
      return;
    }
    int i = pntr_search( remort_array, max_remort, argument );
    if( is_set( flags, 1 ) ) {
      if( i < 0 ) {
        fsend( ch, "The name \"%s\" isn't in the remort name array.", argument );
      } else {
        record_delete( sizeof( char* ), MEM_BADNAME );
        remove( remort_array, max_remort, i );
        send( ch, "Name removed from remort name array.\n\r" );
        save_remort( );
      }
    } else {
      if( i > 0 ) {
        fsend( ch, "The name \"%s\" is already in the remort name array.", argument );
      } else if( check_parse_name( ch->link, argument ) ) {
        i = -i-1;
        record_new( sizeof( char* ), MEM_BADNAME );
        name = alloc_string( argument, MEM_BADNAME );
        insert( remort_array, max_remort, name, i );
        fsend( ch, "The name \"%s\" has been added to the remort name array.", name );
        save_remort( );
      }
    }
    return;
  }

  if( is_set( flags, 0 ) ) {
    if( !*argument ) {
      page_title( ch, "Banned Sites" );
      for( ban = ban_list; ban; ban = ban->next )
        page( ch, "%s\n\r", ban->name );
      return;
    }
    if( !is_set( &flags, 1 ) ) {
      for( ban = ban_list; ban; ban = ban->next ) {
        if( !strcasecmp( argument, ban->name ) ) {
          send( ch, "That site is already banned!\n\r" );
          return;
	}
      }
      fsend( ch, "Site \"%s\" banned.", argument );
      ban = new ban_data;
      ban->name = alloc_string( argument, MEM_BAN );
      save_banned( );
    } else {
      for( ban = ban_list; ban; ban = ban->next ) {
        if( !strcasecmp( argument, ban->name ) ) {
          fsend( ch, "Site \"%s\" ban lifted.", ban->name );
          delete ban;
          save_banned();
          return;
	}
      }
      fsend( ch, "Site \"%s\" is not banned.", argument );
    }
    return;
  }
  
  if( !*argument ) {
    page_title( ch, "Banned Accounts" );
    for( int i = 0; i < max_account; i++ )
      if( account_list[i]->banned != -1 ) 
        display_account( ch, account_list[i], first );
    return;
  }

  if( !( account = account_arg( argument ) ) ) {
    if( !( pfile = player_arg( argument ) ) ) {
      send( ch, "No such account or player.\n\r" );
      return;
    }
    if( !( account = pfile->account ) ) {
      send( ch, "Player %s doesn't have an account which makes banning it difficult.\n\r",
	    pfile->name );
      return;
    }
  }
  
  if( is_set( flags, 1 ) ) {
    if( account->banned == -1 ) {
      send( ch, "Account %s was not banned.\n\r", account->name );
    } else {
      send( ch, "Ban lifted from account %s.\n\r", account->name );
      account->banned = -1;
      save_accounts( );
    }
    return;
  }

  if( ch->pcdata->pfile->account == account ) {
    send( ch, "Banning yourself would be rather foolish.\n\r" );
    return;
  }

  for( int i = 0; i < max_pfile; i++ )
    if( pfile_list[i]->account == account 
	&& pfile_list[i]->trust >= ch->pcdata->trust ) {
      send( ch, "You do not have permission to ban account %s.\n\r", account->name );
      return;
    }
  
  if( !*argument ) {
    send( ch, "For what time period?\n\r" );
    return;
  }

  int t = time_arg( argument, ch );
  if( t == -1 ) 
    return;
  
  if( t == 0 ) {
    account->banned = 0;
    fsend( ch, "Account %s banned forever.",
	   account->name );
  } else {
    account->banned = current_time+t;
    fsend( ch, "Account %s banned until %s.",
	   account->name, ltime( account->banned, false, ch )+4 );
  }
  
  save_accounts( );

  link_data *link = new link_data;
  link->connected = CON_PLAYING;

  for( int j = 0; j < max_pfile; ++j ) {
    bool extract = false;
    player_data *pl = 0;
    pfile_data *pfile = pfile_list[j];
    for( int i = 0; i < MAX_VOTE; ++i ) {
      if( pfile->account == account
	  || ( pfile->vote[i]
	       && pfile->vote[i]->account == account ) ) {

	if( !pl ) {
	  for( int k = 0; k < player_list; ++k ) {
	    if( player_list[k]->pcdata->pfile == pfile
		&& player_list[k]->In_Game( ) ) {
	      pl = player_list[k];
	      break;
	    }
	  }
	  
	  if( !pl ) {
	    if( !load_char( link, pfile->name, PLAYER_DIR ) ) {
	      bug( "Load_players: error reading player file. (%s)", pfile->name );
	      break;
	    }
	    
	    pl = link->player;
	    extract = true;
	  }
	}

	pfile->vote[i] = 0;
      }
    }
    if( pl ) {
      pl->Save( false );
      if( extract ) {
	pl->Extract();
	extracted.delete_list();
      }
    }
  }

  delete link;

  for( int i = 0; i < player_list; i++ ) {
    player_data *victim = player_list[i];
    if( victim->Is_Valid( )
	&& victim->pcdata->pfile->account == account ) {
      send( victim, "-- You have been banned. --\n\r" );
      forced_quit( victim );
    }
  }
}


/*
 *   IS_BANNED ROUTINE
 */


bool is_banned( const char* host )
{
  ban_data*       ban;
  int        host_len  = strlen( host );
  int         ban_len;
  
  for( ban = ban_list; ban; ban = ban->next ) { 
    ban_len = strlen( ban->name );
    if( ban_len <= host_len
	&& !strcasecmp( &host[host_len-ban_len], ban->name ) ) 
      break;
  }
  
  return ban;
}


bool is_banned( account_data* account, link_data* link )
{
  if( !account || account->banned == -1 )
    return false;

  if( account->banned != 0 && account->banned < current_time ) {
    account->banned = -1;
    return false;
  }
  
  if( !link )
    return true;

  help_link( link, "Banned_Acnt" );

  if( account->banned != 0 ) 
    send( link, "Ban Expires: %s\n\r",
	  ltime( account->banned, false, account ) );

  close_socket( link, true );
  return true;
}


/*
 *   READ/WRITE BADNAME
 */


void load_badname( void )
{
  echo( "Loading Bad Names...\n\r" );

  FILE *fp = open_file( FILES_DIR, BADNAME_FILE, "r", true );
  
  if( strcmp( fread_word( fp ), "#BADNAME" ) )
    panic( "Load_badname: missing header" );

  while( true ) {
    const char *name = fread_string( fp, MEM_BADNAME );
    if( *name == '$' ) {
      free_string( name, MEM_BADNAME );
      break;
    }
    record_new( sizeof( char* ), MEM_BADNAME );
    insert( badname_array, max_badname, name, max_badname );
  }

  fclose( fp );
}


void save_badname( void )
{
  rename_file( FILES_DIR, BADNAME_FILE,
	       FILES_PREV_DIR, BADNAME_FILE );
  
  FILE *fp;

  if( !( fp = open_file( FILES_DIR, BADNAME_FILE, "w" ) ) ) {
    return;
  }
  
  fprintf( fp, "#BADNAME\n\n" );

  for( int i = 0; i < max_badname; i++ )
    fwrite_string( fp, badname_array[i] );

  fwrite_string( fp, "$" );
  fclose( fp );
}


void load_remort( void )
{
  echo( "Loading Remort Names...\n\r" );

  FILE *fp = open_file( FILES_DIR, REMORT_FILE, "r", true );
  
  if( strcmp( fread_word( fp ), "#REMORT" ) )
    panic( "Load_remort: missing header" );

  while( true ) {
    const char *name = fread_string( fp, MEM_BADNAME );
    if( *name == '$' ) {
      free_string( name, MEM_BADNAME );
      break;
    }
    record_new( sizeof( char* ), MEM_BADNAME );
    insert( remort_array, max_remort, name, max_remort );
  }

  fclose( fp );
}


void save_remort( void )
{
  rename_file( FILES_DIR, REMORT_FILE,
	       FILES_PREV_DIR, REMORT_FILE );
  
  FILE *fp;

  if( !( fp = open_file( FILES_DIR, REMORT_FILE, "w" ) ) ) {
    return;
  }
  
  fprintf( fp, "#REMORT\n\n" );

  for( int i = 0; i < max_remort; i++ )
    fwrite_string( fp, remort_array[i] );

  fwrite_string( fp, "$" );
  fclose( fp );
}


/*
 *   READ/WRITE BANNED
 */


void load_banned( void )
{
  echo( "Loading Banned Sites...\n\r" );

  FILE *fp = open_file( FILES_DIR, BANNED_FILE, "r", true );

  if( strcmp( fread_word( fp ), "#BANNED" ) )
    panic( "Load_banned: missing header" );

  while (true) {
    const char *name = fread_string( fp, MEM_BAN );
    if( *name == '$' ) {
      free_string( name, MEM_BAN );
      break;
    }
    ban_data *ban = new ban_data;
    ban->name = name;
  }
  
  fclose( fp );
}


void save_banned( )
{
  rename_file( FILES_DIR, BANNED_FILE,
	       FILES_PREV_DIR, BANNED_FILE );
  
  FILE *fp = open_file( FILES_DIR, BANNED_FILE, "w" ); 
  
  fprintf( fp, "#BANNED\n\n" );
  
  for( ban_data *ban = ban_list; ban; ban = ban->next ) 
    fwrite_string( fp, ban->name );

  fwrite_string( fp, "$" );
  fclose( fp );
}
