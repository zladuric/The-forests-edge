#include <ctype.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include "define.h"
#include "struct.h"


static const char *const admin_email = "terminus@genesismuds.com";
static const char *const admin_name = "TFE";
static const char *const system_name = "Digital Genesis";


account_data**  account_list = 0;
int             max_account;


/*
 *   LOCAL FUNCTIONS
 */


void            save_accounts       ( void );


/*
 *   ACCOUNT_DATA 
 */


account_data :: account_data( )
  : next(0),
    name(empty_string), email(empty_string), pwd(empty_string),
    confirm(empty_string), new_email(empty_string), notes(empty_string),
    timezone(empty_string),
    last_login(-1), balance(0), banned(-1), no_players(-1)
{
  record_new( sizeof( account_data ), MEM_ACCOUNT );
} 


account_data :: ~account_data( )
{
  record_delete( sizeof( account_data ), MEM_ACCOUNT );

  free_string( name,      MEM_ACCOUNT );
  free_string( email,     MEM_ACCOUNT );
  free_string( new_email, MEM_ACCOUNT );
  free_string( pwd,       MEM_ACCOUNT );
  free_string( confirm,   MEM_ACCOUNT );
  free_string( notes,     MEM_ACCOUNT );
  free_string( timezone,  MEM_ACCOUNT );
}


bool account_data :: Seen( char_data *ch ) const
{
  if( !ch->pcdata )
    return false;

  if( is_demigod( ch ) )
    return true;

  if( this == ch->pcdata->pfile->account )
    return true;

  return has_permission( ch, PERM_ACCOUNTS );
}


/*
 *   ACCOUNT FUNCTIONS
 */


void add_list( account_data* account )
{
  int pos = pntr_search( account_list, max_account, account->name );

  if( pos < 0 )
    pos = -pos-1;

  insert( account_list, max_account, account, pos );
}


void extract( account_data* account )
{
  int pos = pntr_search( account_list, max_account, account->name );

  if( pos >= 0 ) {
    if( account_list[pos] != account )
      bug( "Extract_Account: Wrong position!" );
    else
      remove( account_list, max_account, pos );
    }

  for( int i = 0; i < player_list; ++i ) {
    player_data *pl = player_list[i];
    if( !pl->Is_Valid( )
	|| !wizard( pl ) )
      continue;
    if( wizard( pl )->account_edit == account ) {
      send( pl, "** The account you were editing has been deleted. **\n\r" );
      wizard( pl )->account_edit = 0;
    }
  }

  delete account;
}


/*
 *   FIND ACCOUNT ROUTINES
 */


account_data* find_account( const char *name, bool all )
{
  int pos = pntr_search( account_list, max_account, name );

  if( pos < 0 ) 
    pos = -pos-1; 

  if( !all ) 
    for( ; pos < max_account
      && account_list[pos]->last_login == -1; pos++ );

  return( pos != max_account
	  && !strncasecmp( name, account_list[pos]->name, strlen( name ) ) 
	  ? account_list[pos] : 0 );
}


account_data* account_arg( const char *& argument )
{
  for( int i = strlen( argument ); i > 0; ) {
    int pos = pntr_search( account_list, max_account, argument, i );
    if( pos < 0 )
      pos = -pos-1;
    for( ; pos < max_account; ++pos )
      if( account_list[pos]->last_login != -1 ) {
        if( !strncasecmp( argument, account_list[pos]->name, i ) ) {
          argument += i;
          skip_spaces( argument );
          return account_list[pos];
	}
        break;
      }
    for( ; --i > 0 && isgraph( argument[i] ); );   
  }
  
  return 0;
}


/*
 *   VALID ACCOUNT NAME
 */


static bool valid_acnt_name( const char *argument )
{
  int l = strlen( argument );

  if( l < 5 || l >= 20 )
    return false;
  
  for( int i = 0; i < l; ++i ) {
    if( !isalnum( *( argument+i ) ) ) {
      return false;
    }
  }

  return true;
}


/*
 *   EMAIL ROUTINES
 */


static account_data *existing_email( const char *argument, const account_data *skip = 0 )
{
  for( int i = 0; i < max_account; i++ )
    if( !strcasecmp( account_list[i]->email, argument ) 
	|| account_list[i] != skip && !strcasecmp( account_list[i]->new_email, argument ) )
      return account_list[i];

  return 0;
}  


static bool valid_email( const char *addr )
{
  size_t i;
  
  if( strlen( addr ) < 5 ) 
    return false;
  
  for( i = 1; i < strlen( addr )-1; i++ )
    if( addr[i] == (char) '@' )
      break;
  
  if( i == strlen( addr )-1 )
    return false;
  
  for( i = 0; i < strlen( addr ); i++ ) 
    if( !isalnum( addr[i] ) && addr[i] != (char) '@'
	&& addr[i] != (char) '.' && addr[i] != (char) '_'
	&& addr[i] != (char) '-' && addr[i] != (char) '!'
	&& addr[i] != (char) '+' ) 
      return false;
  
  return true;
}


static void gen_confirm( account_data *account )
{
  char confirm [ 9 ];

  for( int i = 0; i < 8; ++i )
    confirm[i] = 'a' + number_range( 0, 25 );
  confirm[8] = '\0';

  account->confirm = alloc_string( confirm, MEM_ACCOUNT );
}


static void send_email( link_data* link, bool change )
{  
  char                 tmp1  [ MAX_INPUT_LENGTH ];
  char                 tmp2  [ MAX_INPUT_LENGTH ];
  FILE*                 fp;

  account_data *account = link->account;
  account->last_login = current_time;

  snprintf( tmp1, MAX_INPUT_LENGTH, "%s/account.%s.msg", TEMP_DIR, account->name );

  if( !( fp = open_file( tmp1, "w" ) ) ) 
    return;

  if( change ) {
    fprintf( fp, "Subject: Email Address Change Request\n" );
    fprintf( fp, "A request was entered at %s to change\n", system_name );
    fprintf( fp, "the email address of an account to this address.  The\n" );
    fprintf( fp, "confirmation code needed to complete this is given\n" );
    fprintf( fp, "below.\n" );
    fprintf( fp, "\n" );
    fprintf( fp, "            Account: %s\n", account->name );   
    fprintf( fp, "         Prev Email: %s\n", account->email );   
    fprintf( fp, "          New Email: %s\n", account->new_email );   
    fprintf( fp, "  Confirmation Code: %s\n", account->confirm );   
    fprintf( fp, "    Site of Request: %s\n", link->host );
  } else {
    fprintf( fp, "Subject: New Account Request\n" );
    fprintf( fp, "A request was entered at %s to open an\n", system_name );
    fprintf( fp, "account for this email address.  If this is indeed\n" );
    fprintf( fp, "the case your confirmation code is '%s'.\n", account->confirm );
    fprintf( fp, "The request was from a user connected from %s.\n", 
	     link->host );
  }

  fclose( fp );

  snprintf( tmp2, MAX_INPUT_LENGTH,
	    "(cat \"%s\" | /usr/lib/sendmail -i -F\"%s\" -r\"%s\" \"%s\"; rm -f \"%s\") &",
	    tmp1, admin_name, admin_email, account->new_email, tmp1 );
  system( tmp2 );
}


/*
 *   NANNY ROUTINES
 */


static void resend_confirm( link_data *link )
{
  if( link->account->new_email == empty_string ) {
    help_link( link, "Acnt_NoResend" );
    press_return( link );
    link->connected = CON_PAGE;
    return;
  }

  send_email( link, false );
  help_link( link, "Acnt_Done" );
  link->connected = CON_PAGE;
}


void nanny_acnt_enter( link_data* link, const char *argument )
{
  account_data*  account;

  if( !*argument ) {
    write_greeting( link );
    link->connected = CON_INTRO;
    return;
  }
  
  if( !( account = find_account( argument ) ) ) {
    help_link( link, "Unfound_Acnt" );
    send( link, "Account: " );
    return;
  }

  if( is_banned( account, link ) )
    return;

  link->account = account;

  if( link->connected == CON_ACNT_RESEND_REQ ) {
    resend_confirm( link );
    return;
  }

  if( account->email == empty_string ) {
    help_link( link, "Confirm_Acnt" );
    send( link, "Confirmation Code: " );
    link->connected = CON_ACNT_CONFIRM;
    return;
  }
  
  help_link( link, "Acnt_Check_Pwd" );
  send( link, "Password: " );

  switch( link->connected ) {
  case CON_VE_ACCOUNT:
    link->connected = CON_VE_VALIDATE;
    break;
  case CON_CE_ACCOUNT:
    link->connected = CON_CE_PASSWORD;
    break;
  default:
    link->connected = CON_ACNT_CHECK_PWD;
    break;
  }
}


void nanny_acnt_check_pwd( link_data* link, const char *argument )
{
  if( strcmp( link->account->pwd, argument ) ) {
    help_link( link, "Acnt_Bad_Pwd" );
    link->connected = CON_PAGE;
    return;
  }
  
  if( link->connected == CON_CE_PASSWORD ) {
    help_link( link, "New_Email" );
    send( link, "Current: %s\n\r\n\r", link->account->email );
    send( link, "New Email: " );
    link->connected = CON_CE_EMAIL;
  } else {
    help_link( link, "Login_new_name" );
    send( link, "Name: " );
    link->connected = CON_GET_NEW_NAME;
  }
}


void nanny_acnt_confirm( link_data* link, const char *argument ) 
{
  account_data* account = link->account;

  if( no_input( link, argument ) )
    return;

  if( strcmp( argument, link->account->confirm ) ) {
    help_link( link, "Bad_Confirm" );
    send( link, "Confirmation Code: " );
    return;
  }

  if( link->connected == CON_VE_CONFIRM ) {
    help_link( link, "Email_Changed" );
    press_return( link );
    link->connected = CON_PAGE;
  } else {
    send( link, "\n\r>> Account Validated. <<\n\r" );
    help_link( link, "Login_new_name" );
    send( link, "Name: " );
    link->connected = CON_GET_NEW_NAME;
  }
  
  free_string( account->email, MEM_ACCOUNT );
  account->email = account->new_email;
  account->new_email = empty_string;

  free_string( account->confirm, MEM_ACCOUNT );
  account->confirm = empty_string;

  save_accounts( );
}


void nanny_acnt_name( link_data* link, const char *argument )
{
  if( no_input( link, argument ) )
    return;

  if( !valid_acnt_name( argument ) ) {
    help_link( link, "Invalid_Acnt_Name" );
    send( link, "Account: " );
    return;
  }

  if( find_account( argument, true ) ) {
    help_link( link, "Existing_Acnt" );
    send( link, "Account: " );
    return;
  }
   
  link->account = new account_data;
  link->account->name = alloc_string( argument, MEM_ACCOUNT ); 

  add_list( link->account );

  help_link( link, "Acnt_Pwd" );
  send( link, "Password: " );

  link->connected = CON_ACNT_PWD;
}


void nanny_acnt_password( link_data* link, const char *argument )
{
  if( strlen( argument ) < 5 ) {
    help_link( link, "Short_Acnt_Pwd" );
    send( link, "Password: " );
    return;
  }

  if( strlen( argument ) > 16 ) {
    help_link( link, "Long_Acnt_Pwd" );
    send( link, "Password: " );
    return;
  }

  if ( !strcmp( argument, link->account->name ) ) {
    help_link( link, "Bad_Acnt_Pwd" );
    send( link, "Password: " );
    return;
  }

  link->account->pwd = alloc_string( argument, MEM_ACCOUNT );

  help_link( link, "Acnt_Email" );
  send( link, "Email: " );

  link->connected = CON_ACNT_EMAIL;
}


void nanny_acnt_email( link_data* link, const char *argument )
{
  account_data *account = link->account;

  if( no_input( link, argument ) )
    return;
 
  if( !valid_email( argument ) ) {
    help_link( link, "Invalid_Email" );
    send( link, "Email: " );
    return;
  }
  
  if( existing_email( argument, account ) ) {
    help_link( link, "Existing_Email" );
    link->connected = CON_PAGE;
    if( link->account->last_login == -1 ) {
      extract( link->account );       
      link->account = 0;
    }
    return;
  }
  
  if( is_banned( argument ) ) {
    send( link,
	  "New accounts with email from that site are banned.\n\r" );
    close_socket( link, true );
    return;        
  }

  free_string( account->new_email,  MEM_ACCOUNT );
  account->new_email = alloc_string( argument, MEM_ACCOUNT );

  if( link->connected == CON_CE_EMAIL ) {
    help_link( link, "ChangeEmail_Sent" );
    press_return( link );
    link->connected = CON_PAGE;
    gen_confirm( account );
    send_email( link, true );
    return;
  }
  
  link->connected = CON_ACNT_CHECK;

  help_link( link, "Acnt_Check" );
 
  send( link,
	"   Account: %s\n\r  Password: %s\n\r     Email: %s\n\r\n\r", 
	account->name, account->pwd, account->new_email );
  send( link, "Is this correct? " );
}


void nanny_acnt_check( link_data* link, const char *argument )
{
  if( toupper( *argument ) != 'Y'  ) {
    help_link( link, "Acnt_Cancel" );
    extract( link->account );
    link->account = 0;
    link->connected = CON_PAGE;
    return;
  }

  link->connected = CON_PAGE;

  gen_confirm( link->account );
  send_email( link, false );
  save_accounts( );

  help_link( link, "Acnt_Done" );
}


void nanny_acnt_menu( link_data* link, const char *argument )
{
  switch( atoi( argument ) ) {
  case 1:
    if( is_banned( link->host ) ) {
      send( link, "The site you are connected from is banned.\n\r" );
      close_socket( link, true );
      return;        
    }
    help_link( link, "Create_Account" );
    link->connected = CON_ACNT_NAME;
    break;
    
  case 2:
    help_link( link, "CE_AccountName" );
    link->connected = CON_CE_ACCOUNT;
    break;
    
  case 3:
    help_link( link, "CE_AccountName" );
    link->connected = CON_VE_VALIDATE;
    break;
    
  case 4:
    help_link( link, "Acnt_Request" );
    link->connected = CON_ACNT_REQUEST;
    send( link, "Email: " );
    return;
    
  case 5:
    help_link( link, "Acnt_Resend" );
    link->connected = CON_ACNT_RESEND_REQ;
    break;
    
  default:
    write_greeting( link );
    link->connected = CON_INTRO;
    return;
  }

  send( link, "Account: " );
}


void nanny_acnt_request( link_data* link, const char *argument )
{
  char               tmp1  [ MAX_INPUT_LENGTH ];
  char               tmp2  [ THREE_LINES ];
  account_data*  account;
  bool             found  = false;
  FILE*               fp;

  if( !( account = existing_email( argument ) )
      || account->email == empty_string ) {
    help_link( link, "Acnt_No_Email" );
    link->connected = CON_PAGE;
    return;
  }

  snprintf( tmp1, MAX_INPUT_LENGTH, "%s/request.%s.msg", TEMP_DIR, account->name );

  if( !( fp = open_file( tmp1, "w" ) ) ) 
    return;

  fprintf( fp, "Subject: Information Request\n" );
  fprintf( fp, "A user connected from %s requested this\n", link->host );
  fprintf( fp, "information be emailed to this address.\n\n" );
  
  fprintf( fp, "       Account Name: %s\n", account->name );
  fprintf( fp, "   Account Password: %s\n", account->pwd );

  if( account->confirm != empty_string )
    fprintf( fp, "  Confirmation Code: %s\n", account->confirm );

  fprintf( fp, "\n" );

  for( int i = 0; i < max_pfile; i++ ) 
    if( pfile_list[i]->account == account ) {
      if( !found ) {
        fprintf( fp, "%-20s%-15s%s\n", "Player", "Password", "Last Login" );
        fprintf( fp, "%-20s%-15s%s\n", "------", "--------", "----------" );
        found = true;
      }
      fprintf( fp, "%-20s%-15s%s\n", pfile_list[i]->name,
	       pfile_list[i]->pwd, pfile_list[i]->last_host );
    }

  if( !found ) 
    fprintf( fp, "No existing players on this account.\n" );

  fclose( fp );

  snprintf( tmp2, THREE_LINES,
	    "(cat \"%s\" | /usr/lib/sendmail -i -F\"%s\" -r\"%s\" \"%s\"; rm -f \"%s\") &",
	    tmp1, admin_name, admin_email, account->email, tmp1 );
  system( tmp2 );

  help_link( link, "Acnt_Sent" );
  link->connected = CON_PAGE;
}


void nanny_ve_validate( link_data* link, const char *argument )
{
  if( !*argument ) {
    write_greeting( link );
    link->connected = CON_INTRO;
    return;
  }

  account_data *account;

  if( !( account = find_account( argument ) ) ) {
    help_link( link, "Unfound_Acnt" );
    send( link, "Account: " );
    return;
  }

  if( account->new_email == empty_string ) {
    help_link( link, "CE_norequest" );
    link->connected = CON_PAGE;
    return;
  }

  help_link( link, "CE_Code" );
  send( link, "Confirmation Code: " );

  link->account = account;
  link->connected = CON_VE_CONFIRM;
}


/*
 *   DISK ROUTINES
 */


void save_accounts( )
{
  rename_file( FILES_DIR, ACCOUNT_FILE,
	       FILES_PREV_DIR, ACCOUNT_FILE );
  
  FILE *fp;

  if( !( fp = open_file( FILES_DIR, ACCOUNT_FILE, "w" ) ) ) 
    return;

  fprintf( fp, "%d\n\n", max_account );

  for( int i = 0; i < max_account; i++ ) {
    account_data *account = account_list[i];
    fwrite_string( fp, account->name );
    fwrite_string( fp, account->email );
    fwrite_string( fp, account->pwd );
    fwrite_string( fp, account->new_email );
    fwrite_string( fp, account->confirm );
    fwrite_string( fp, account->notes );
    fwrite_string( fp, account->timezone );
    fprintf( fp, "%d %d %d %d\n\n",
	     account->last_login,
	     account->balance,
	     account->banned,
	     account->no_players );
    }

  fclose( fp );
}


void load_accounts( )
{
  FILE *fp;

  echo( "Loading Accounts...\n\r" );

  if( !( fp = open_file( FILES_DIR, ACCOUNT_FILE, "r" ) ) ) {
    max_account  = 0;
    account_list  = 0;
    return;
  }

  max_account = fread_number( fp );
  account_list = new account_data* [ max_account ];

  for( int i = 0; i < max_account; i++ ) {
    account_data *account = new account_data;

    account->name      = fread_string( fp, MEM_ACCOUNT );
    account->email     = fread_string( fp, MEM_ACCOUNT );
    account->pwd       = fread_string( fp, MEM_ACCOUNT );
    account->new_email = fread_string( fp, MEM_ACCOUNT );
    account->confirm   = fread_string( fp, MEM_ACCOUNT );
    account->notes     = fread_string( fp, MEM_ACCOUNT );
    account->timezone  = fread_string( fp, MEM_ACCOUNT );
 
    account->last_login = fread_number( fp );
    account->balance    = fread_number( fp );
    account->banned     = fread_number( fp );
    account->no_players = fread_number( fp );

    account_list[i] = account;
  }

  fclose( fp );
}


/*
 *   LIST ACCOUNTS ROUTINE
 */


static void display_pfile( char_data* ch, pfile_data* pfile, bool& first, bool& deny )
{
  if( first ) {
    first = false;
    page_underlined( ch, "%-15s%-32s%-16s%-10sLevel\n\r",
		     "Character", "Site", "Last On", "Class" );
  }

  const char *flag = empty_string;
  if( is_set( pfile->flags, PLR_DENY ) ) {
    deny = true;
    flag = "*";
  }

  page( ch, "%-15s%-32s%-16s%-10s%5d%s\n\r",
	pfile->name,
	trunc( pfile->last_host, 31 ),
	ltime( pfile->last_on, false, ch )+4,
	clss_table[pfile->clss].name,
	pfile->level,
	flag );
}


void display_account( char_data* ch, account_data* account, bool& first )
{
  if( first ) {
    first = false;
    page_underlined( ch, "%-20sEmail\n\r", "Account" );
  }

  page( ch, "%-20s%s\n\r", account->name, account->email );
}


void do_accounts( char_data* ch, const char *argument )
{
  account_data*   account = 0;
  pfile_data*       pfile = 0;
  wizard_data*        imm = (wizard_data*) ch;
  bool              first = true;
  bool deny = false;
  int                   i;
  int flags = 0;

  if( *argument || !( account = imm->account_edit ) ) {

    if( !get_flags( ch, argument, &flags, "spPeD", "Accounts" ) )
      return;
    
    if( is_set( flags, 0 ) ) {
      if( !has_permission( ch, PERM_ACCOUNTS ) ) {
	send( ch, "You do not have permission to search accounts.\n\r" );
	return;
      }
      if( !*argument ) {
	send( ch, "For what site do you wish to list the players?\n\r" );
	return;
      }
      if( ( i = site_search( argument ) ) < 0 )
	i = -i-1;
      for( ; i < site_entries && !rstrncasecmp( site_list[i]->last_host,
						argument, strlen( argument ) ); i++ ) 
	if( site_list[i]->account->Seen( ch ) )
	  display_pfile( ch, site_list[i], first, deny );
      if( first ) {
	send( ch, "No players from that site.\n\r" );
      } else if( deny ) {
	page( ch, "\n\r" );
	page_centered( ch, "[ Players flagged with * are denied access. ]" );
      }
      return;
    }
      
    if( is_set( flags, 2 ) ) {
      if( !has_permission( ch, PERM_ACCOUNTS ) ) {
	send( ch, "You do not have permission to search passwords.\n\r" );
	return;
      }
      if( !*argument ) {
	send( ch,
	      "For which character to you wish to run a password match?\n\r" );
	return;
      }
      if( !( pfile = find_pfile( argument, ch ) ) ) 
	return;
      for( i = 0; i < max_pfile; i++ ) {
	if( !strcasecmp( pfile_list[i]->pwd, pfile->pwd )
	    && pfile_list[i]->account->Seen( ch ) ) {
	  display_pfile( ch, pfile_list[i], first, deny );
	}
      }
      if( deny ) {
 	page( ch, "\n\r" );
	page_centered( ch, "[ Players flagged with * are denied access. ]" );
      }
      return;
    }
  
    if( !*argument ) {
      if( is_set( flags, 3 ) ) {
	if( imm->account_edit ) {
	  fsend( ch, "You stop editing account \"%s\".", imm->account_edit->name );
	  imm->account_edit = 0;
	} else {
	  send( ch, "Which account do you wish to edit?\n\r" );
	}
      } else {
	send( ch,
	      "For what player %sdo you wish to list an account summary?\n\r",
	      ( !has_permission( ch, PERM_ACCOUNTS ) || is_set( flags, 1 ) )
	      ? ""
	      : "or account " );
      }
      return;
    }
    
    if( is_set( flags, 1 )
	|| !has_permission( ch, PERM_ACCOUNTS )
	|| !( account = find_account( argument ) ) ) {
      if( !( pfile = find_pfile( argument ) ) ) {
	send( ch, "No matching player %sfound.\n\r",
	      ( !has_permission( ch, PERM_ACCOUNTS ) || is_set( flags, 1 ) )
	      ? ""
	      : "or account " );
	return;
      }
      if( pfile
	  && pfile != ch->pcdata->pfile
	  && pfile->trust >= get_trust( ch ) ) {
	fsend( ch, "You cannot view the account of %s.", pfile->name );
	return;
      }
      if( !( account = pfile->account ) ) {
	fsend( ch, "%s has no account.", pfile->name );
	return;
      }
    }    
  
    if( is_set( flags, 3 ) ) {
      imm->account_edit = account;
      imm->player_edit = 0;
      if( pfile ) {
	fsend( ch, "You now edit %s's account.", pfile->name );
      } else {
	fsend( ch, "You now edit account \"%s\".", account->name );
      }
      return;
    }

    if( is_set( flags, 4 ) ) {
      if( !has_permission( ch, PERM_ACCOUNTS ) ) {
	send( ch, "You do not have permission delete accounts.\n\r" );
	return;
      }
      for( int i = 0; i < max_pfile; ++i ) {
	if( pfile_list[i]->account == account ) {
	  fsend( ch, "Account \"%s\" cannot be deleted since it has characters.",
		 account->name );
	  return;
	}
      }
      fsend( ch, "Account \"%s\" has been deleted.", account->name );
      extract( account );
      return;
    }
  }

  if( account->Seen( ch ) ) {
    page( ch, "  Account: %s\n\r", account->name );
    page( ch, "    Email: %s\n\r", account->email );
    
    if( is_god( ch ) ) 
      page( ch, " Password: %s\n\r", account->pwd );

    if( *account->timezone )
      page( ch, "Time Zone: %s\n\r", account->timezone );

    if( account->banned != -1 ) 
      page( ch, "   Banned: %s\n\r",
	    account->banned == 0 ? "forever" :
	    ltime( account->banned, false, ch ) ); 
    
    page( ch, "\n\r" );
    
    for( i = 0; i < max_pfile; i++ ) {
      if( pfile_list[i]->account == account
	  //	  && pfile_list[i]->account->Seen( ch )
	  && ( pfile_list[i] == ch->pcdata->pfile || pfile_list[i]->trust < get_trust( ch ) ) ) {
	display_pfile( ch, pfile_list[i], first, deny );
      }
    }
    if( first ) {
      page( ch, "No players on account.\n\r" );
    } else if( deny ) {
      page( ch, "\n\r" );
      page_centered( ch, "[ Players flagged with * are denied access. ]" );
    }
  }
  
  if( account->notes != empty_string ) {
    page( ch, "\n\r" );
    page_underlined( ch, "Pbug\n\r" );
    page( ch, account->notes );
  } else if( !account->Seen( ch ) ) {
    page( ch, "\n\rNo pbugs.\n\r" );
  }
}


/*
 *   PURCHASE COMMAND
 */


bool lower_balance( char_data* ch, int amount )
{
  account_data* account;

  if( !( account = ch->pcdata->pfile->account ) ) {
    send( ch, "You don't have an account.\n\r" );
    return false;
  }

  if( account->balance < amount ) {
    send( ch, "Your account balance is $%.2f - requested purchase\
 is $%.2f.\n\r", account->balance, amount );
    return false;
  } 

  account->balance -= amount;

  return true;
}


void do_purchase( char_data* ch, const char *argument )
{
  if( is_mob( ch ) )
    return;

  player_data*        pc  = (player_data*) ch;
  account_data*  account;

  if( !( account = ch->pcdata->pfile->account ) ) {
    send( ch, "You lack an account.\n\r" );
    return;
  }

  if( !*argument ) {
    send( ch, "Balance: $%.2f\n\r", (double)account->balance / 100.0 );
    return;
  }

  int     i;
  int  cost;

  if( matches( argument, "gsp" ) ) {
    if( !number_arg( argument, i ) ) {
      send( pc, "How many gossip points do you wish to purchase at 500 per dollar?\n\r" );
      return;
    }
    if( i <= 0 ) {
      send( pc, "The resale value of gsps is nothing.\n\r" );
      return;
    }
    if( pc->gossip_pts+i > 1000 ) {
      send( pc, "You may have at maximum 1000 gossip points.\n\r" );
      return;
    }
    cost = (4+i)/5;
    if( lower_balance( ch, cost ) ) { 
      i = min( 5*cost, 1000-pc->gossip_pts );
      pc->gossip_pts += i;
      send( ch, "%s gossip points purchased for $%.2f.\n\r",
	    number_word( i ), (double)cost / 100.0 );
    }
    return;
  }
}


void do_pbug( char_data* ch, const char *argument )
{
  wizard_data *imm;

  if( !( imm = wizard( ch ) ) ) {
    return;
  }

  account_data *account = imm->account_edit;

  if( !account ) {
    send( ch, "You are not currently editing any account.\n" );
    return;
  }
  
  if( *argument && !account->Seen( ch ) ) {
    fsend( ch, "You cannot edit the account." );
    return;
  }

  account->notes = edit_string( ch, argument, account->notes, MEM_DESCR, false );
}
