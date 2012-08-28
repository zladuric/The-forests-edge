#include <ctype.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <syslog.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <sys/param.h>
#include <netinet/in.h>
#include <arpa/telnet.h>
#include "define.h"
#include "struct.h"


int         mud_socket;
int         who_socket;

unsigned who_calls = 0;

bool            wizlock  = false;
bool            godlock  = false;
link_data*    link_list  = 0; 
static link_data *link_next;

static link_data *who_stack = 0;

static void   extract              ( link_data* );

static bool   process_output       ( link_data* );
bool   read_link            ( link_data* );
static void   stop_idling          ( player_data* );

const char go_ahead_str  [] = { IAC, GA, '\0' };
const char echo_off_str  [] = { IAC, WILL, TELOPT_ECHO, '\0' };
const char echo_on_str   [] = { IAC, WONT, TELOPT_ECHO, '\0' };
const char refuse_tm_str [] = { IAC, WONT, TELOPT_TM, '\0' };
const char accept_tm_str [] = { IAC, WILL, TELOPT_TM, '\0' };


bool link_data::past_password() const
{
  return connected != CON_PASSWORD_ECHO
    && connected != CON_PASSWORD_NOECHO
    && connected != CON_INTRO;
}


void link_data::set_playing()
{
  connected = CON_PLAYING;

  remove( link_list, this );

  if( !link_list
      || ( link_list->connected == CON_PLAYING 
	   && strcmp( link_list->pfile->name, pfile->name ) > 0 ) ) {
    next = link_list;
    link_list = this;
    return;
  }

  link_data* prev;
  for( prev = link_list; prev->next; prev = prev->next ) 
    if( prev->next->connected == CON_PLAYING 
	&& strcmp( prev->next->pfile->name, pfile->name ) > 0 )
      break;
  
  next = prev->next;
  prev->next = this;
}



/*
 *   PREVIOUS PROCESS SOCKET ROUTINES
 */


/* Function checks how many files are currently opened on the system.  It opens
   a number of links equal to that number */
// This is badly flawed.
//void recover_links( )
//{
//  rlimit       lim;
//  link_data*  link;
//  
//  /* Check for the limits on how many files may be opened, and how many are
//     currently opened, store the value in lim */
//  if( getrlimit( RLIMIT_NOFILE, &lim ) != 0 )
//    panic( "Init_Network: error getting file number." );
//
//  /* Create a number of links, from 3 to the current number of open files. */
//  int count = 0;
//  for( int i = 3; i < (int)( lim.rlim_cur ); i++ )
//    if( (int)( write( i, "\n\r", 2 ) ) != - 1 ) { 
//      link             = new link_data;
//      link->channel    = i;
//      link->connected  = CON_INTRO;
//      link->next       = link_list;
//      link_list        = link;
//      ++count;
//    }
//
//  if (count) {
//    fprintf ( stdout, "Recovered %d links.\n", count);
//  }
//
//  echo( "-- New process started. --\n\r" );
//}


/* Step through the 'link' list, getting the host and opening the link
 if it needs it. */
/*
void restart_links( )
{
  link_data*                  link;
  struct sockaddr_in   net_addr_in;
  socklen_t                      addrlen;
 
  while ( (link = link_list) ) {
    link_list = link_list->next;
    
    addrlen = sizeof( net_addr_in  );
    
    // Determine the peername of the connecting link.  Then write the hostname
    if( getpeername( link->channel, (struct sockaddr*) &net_addr_in, &addrlen ) == -1 )
      panic( "Restart_Links: Error returned by getpeername." );
    
    // handles connecting the link and determining the host name
    write_host( link, (char*) &net_addr_in.sin_addr ); 
  }
}
*/


/*
 *   SOCKET ROUTINES
 */

static void open_who( int port )
{
  /* accepts a new socket into the port, from the connect que */
  struct sockaddr net_addr;
  socklen_t addrlen = sizeof( net_addr );
  const int fd_conn = accept( port, &net_addr, &addrlen );

  /* if no new link was opened, return */
  if( fd_conn < 0 )
    return;

  fcntl( fd_conn, F_SETFL, O_NDELAY );

  link_data *link = new link_data;
  link->channel = fd_conn;
  link->character = link->player = new player_data( empty_string );
  link->player->link = link;
  link->player->pcdata->terminal = TERM_ANSI;
  for( int i = 0; i < MAX_COLOR; ++i )
    link->player->pcdata->color[i] = term_table[ TERM_ANSI ].defaults[i];

  char *tmp1 = static_string( );
  sprintf_minutes( tmp1, current_time-boot_time );
  send( link->player, "System started %s ago.\n\r\n\r", tmp1 );

  show_who( link->player, false );

  unlink( &link->player->active );
  unlink( &link->player->update );
  unlink( &link->player->regen );
  link->player->active.time = -1;
  stop_events( link->player );
  delete link->player;
  link->character = link->player = 0;

  append( who_stack, link );

  ++who_calls;
}


/* This great function takes an open port as input, and accepts a connection
   through that port.  It assigns the net_addr, and sets some basic
   flags on the new link.  It then passes the good stuff to write_host. */
static void open_link( int port )
{
  /* accepts a new socket into the port, from the connect que */
  struct sockaddr net_addr;
  socklen_t addrlen = sizeof( net_addr );
  const int fd_conn = accept( port, &net_addr, &addrlen );

  /* if no new link was opened, return */
  if( fd_conn < 0 )
    return;

  fcntl( fd_conn, F_SETFL, O_NDELAY );
  
  struct sockaddr_in net_addr_in;

  /* don't delay our MUD, just cause the guy isn't responding */
  addrlen = sizeof( net_addr_in );

  /* get the peername of the guy trying to connect. */
  if( getpeername( fd_conn, (struct sockaddr*) &net_addr_in, &addrlen ) == -1 ) {
    bug( "Open_Link: Error returned by getpeername." );
    close( fd_conn );
    return;
  }

  /* create a new link data, attatching this guy to the new fd */
  link_data *link = new link_data;
  link->channel = fd_conn;
  link->connected = CON_INTRO;

  /* get his host name, and finish creating the link data */
  write_host( link, net_addr_in.sin_addr ); 
}


/* This function creates, opens, and accept connections via a socket through
   the inputed port number.  Sets the flags of this socket to SO_LINGER and
   SO_REUSEADDR.  Also sets the socket to exit on EXEC family funct.  Returns
   the connected socket. */
int open_port( int portnum )
{
  struct sockaddr_in         server;
  struct linger         sock_linger;
  struct hostent*              host;
  char*                    hostname  = static_string( );
  int                          sock;
  int                             i  = 1;
  const int                            sz  = sizeof( int ); 

  sock_linger.l_onoff  = 0;  //The linger data is reset
  sock_linger.l_linger = 0;  //Linger for 0 second before closing socket.

  /* Get the host name of the current processor, then check and
     see if you can get the host. */
  if( gethostname( hostname, THREE_LINES ) != 0 ) 
    panic( "Open_Port: Gethostname failed." );

  if( !( host = gethostbyname(hostname) ) )
    panic( "Open_Port: Error in gethostbyname %s.", hostname );

  /* Open up a socket, using the ARPA internet protocols, and socket 
     bit streaming format */
  if( ( sock = socket( AF_INET, SOCK_STREAM, 0 ) ) < 0 ) 
    panic( "Open_port: error in socket call" );

  
  /* Set the properties of the socket.  SO_LINGER makes it so that if the 
     socket closes it will attempt to pass any lingering information on 
     to the process for the time set in linger above.  SO_REUSEADDR allows 
     for the reuse of local addresses. */
  if( setsockopt( sock, SOL_SOCKET, SO_REUSEADDR, (char*) &i, sz ) < 0 ) 
    panic( "Open_port: error in setsockopt (SO_REUSEADDR)" );
  
  if( setsockopt( sock, SOL_SOCKET, SO_LINGER, (char*) &sock_linger,
		  sizeof( sock_linger ) ) < 0 ) 
    panic( "Open_port: error in setsockopt (SO_LINGER)" );

  /* Set socket so that it will close if an EXEC family function is called. */
  if( fcntl( sock, F_SETFD, 1 ) == -1 )
    panic( "Open_port: Error setting close on Exec." );

  // Zero out the memory address of the server.
  memset( &server, 0, sizeof( struct sockaddr_in ) );

  // Fill in the appriate information 
  server.sin_family      = AF_INET;
  server.sin_port        = htons( portnum );

  //copy the address of the server into the appropriate place.
  memcpy( &server.sin_addr, host->h_addr_list[0], host->h_length );

  //Name the socket via bind.
  if( bind( sock, (struct sockaddr*) &server, sizeof( server ) ) ) 
    panic( "Open_port: Error binding port %d at %s.",
	   portnum, host->h_name );

  //Listen for connections comming in through the socket, que max is 3
  if( listen( sock, 5 ) ) 
    panic( "Open_port: error in listen" );

  //Let interested parties know about the binding.
  printf( "Binding port %d at %s.\n\r", portnum, host->h_name );

  //Return the activated socket, with connection.
  return sock;
}


static void ampersand( text_data *receive )
{
  if( !strncasecmp( receive->ptr, "ali", 3 ) )
    return; 

  if( char *letter = strstr( receive->ptr, " & " ) ) {
    *letter = '\0';
    letter += 3;
    skip_spaces( letter );
    receive->cont = letter;
  }

  /*
  for( char *letter = receive->message.text; *letter; ++letter ) {
    if( !strncmp( letter, " & ", 3 ) ) {
      *letter = '\0';
      letter += 3;
      skip_spaces( letter );
      text_data *next = new text_data( letter );
      next->next = receive->next;
      receive->next = next;      
      break;
    }
  }
  */
}


/* This function takes the input pipes, checks each of the links in the
   'link' list, and handles the I/O.  If the link is dead (or has idled too
   long) the player is saved, and the link is cut.  If there is a command 
   to execute, it gets set to the interpreter, if there is something to 
   write, it gets written.  Also handles ampersand expansion on incomming
   commands. --YEGGO */
void update_links( )
{
  time_data netstart;
  gettimeofday( &netstart, 0 );

  time_data start = netstart;

  link_data*          link;
  text_data*       receive;
  fd_set          read_set;
  fd_set         write_set;
  fd_set          exec_set;

  //  struct timeval   timeout;
  //  timeout.tv_sec  = 1;
  //  timeout.tv_usec = 0;

  FD_ZERO( &read_set );      //clear the read set
  FD_ZERO( &write_set );     //clear the write set
  FD_ZERO( &exec_set );      //clear the exception set

  FD_SET( mud_socket, &read_set );  //add the read sets to the descriptors
  FD_SET( who_socket, &read_set );  //add the read sets to the descriptors

  // scroll through 'link' list and add the sets to the descriptor values
  int maxfd = max( mud_socket, who_socket );
  for( link = link_list; link; link = link->next ) {
    if( link->channel >= 0 ) {
      if( link->channel > maxfd ) {
	maxfd = link->channel;
      }
      FD_SET( link->channel, &read_set  );
      FD_SET( link->channel, &write_set );
      FD_SET( link->channel, &exec_set );
    }
  }

  // poll the sets to see if any new info is available
  static time_data timeout;
  const int num = select( maxfd+1, &read_set, &write_set, &exec_set, &timeout );

  if( num < 0 ) 
    panic( "Update links: select" );
  
  // if there is anything to read from socket one, read it :)
  if( FD_ISSET( mud_socket, &read_set ) ) {
    open_link( mud_socket );
  }
  
  if( FD_ISSET( who_socket, &read_set ) ) {
    open_who( who_socket );
  }

  // For each link in the 'link' list, do some stuff to it
  for( link = link_list; link; link = link_next ) {
    link_next = link->next;
  
    if( link->channel >= 0 ) {
      /* if there are available exceptions, save the character, and close the
	 link */
      if( FD_ISSET( link->channel, &exec_set ) ) {
	if( link->player ) {
	  link->player->Save( );
	}
	close_socket( link );
	continue;
      }
      
      /* if there is something to read, read it.  First set the idle to zero
	 then check and see if there is a player there .. if there is reset
	 his time.  Then read the stuff from the link.  If you can't, save 
	 the player and close the socket. */
      if( FD_ISSET( link->channel, &read_set ) ) {
	link->idle = 0;            
	if( link->player ) {
	  link->player->timer = current_time;
	}
	if( !read_link( link ) ) { 
	  if( link->player ) {
	    link->player->Save( );   
	  }
	  close_socket( link );
	  continue;
	}
      }
    }

    // if the link has idled for too long, and the player hasn't connected
    // then close the socket
    if( link->idle++ > 10000 && link->connected != CON_PLAYING ) {
      send( link, "\n\r\n\r-- CONNECTION TIMEOUT --\n\r" );
      close_socket( link, true );
      continue;
    }
  }
  
  pulse_time[ TIME_READ_INPUT ] = stop_clock( start );

  gettimeofday( &start, 0 );

  /* Check each link in the 'link' list.  If there is a command in the 
     command recieved buffer, set receive to the command, and set the 
     link->command variable to true.  Call ampersand to do the ampersand
     expansion in the command. Move the command received buffer to the next
     command, and reset the idle.  If the the player is playing, stop their
     idling and interpret the command.  Otherwise get the nanny on his ass. */
  for( link = link_list; link; link = link_next ) {
    link_next = link->next;
    if( link->command = ( receive = link->receive ) ) {
      ampersand( receive );
      if( !receive->cont ) {
	link->receive = receive->next;
      }
      link->idle = 0;
      if( link->connected == CON_PLAYING ) {
        stop_idling( link->player );
        if( !interpret( link->character, receive->ptr ) ) {
	  // Command queue overflow.
	  // Delete rest of catenated commands.
	  link->receive = receive->next;
	  receive->cont = 0;
	}
      } else {
        nanny( link, receive->ptr );
      }
      if( receive->cont ) {
	receive->ptr = receive->cont;
	receive->cont = 0;
      } else {
	//	link->receive = receive->next;
	delete receive; 
      }
    }
  }

  update_queue( now_list );

  pulse_time[ TIME_COMMANDS ] = stop_clock( start );

  gettimeofday( &start, 0 );

  /* Check all of the links.  If they have idled for long enough, and 
     there is something to write, process the output.  If you can't
     process it, save the character and close the link. */
  for( link = link_list; link; link = link_next ) {
    link_next = link->next;
    // Delay is matched to max spell prep rate.
    int delay = modify_delay( link->character, 25 );
    if( ( link->idle%delay == 0 || link->again )
	&& FD_ISSET( link->channel, &write_set )
	&& !process_output( link ) ) {
      if( link->player )
	link->player->Save( );
      close_socket( link );
    }
  }
  
  for( link = who_stack; link; link = link_next ) {
    link_next = link->next;
    process_output( link );
    if( !link->send || !link->again ) {
      close( link->channel );
      remove( who_stack, link );
      delete link;
    }
  }

  pulse_time[ TIME_WRITE_OUTPUT ] = stop_clock( start );
  pulse_time[ TIME_NETWORK ] = stop_clock( netstart );

  // Timed out?
  //  return num == 0;
}


/*
 *   CLOSING OF SOCKETS
 */


void extract( link_data* prev )
{
  for( link_data *link = link_list; link; link = link->next ) {
    if( link->snoop_by == prev )
      link->snoop_by = 0;
  }

  send( prev->snoop_by, "Your victim has left the game.\n\r" );

  if( prev->account && prev->account->last_login == -1  ) 
    extract( prev->account );

  if( link_next == prev )
    link_next = prev->next;

  remove( link_list, prev );
  delete prev;
}


void close_socket( link_data* link, bool process )
{
  char_data*         ch;
  const int         connected  = link->connected;

  if( link->channel < 0 ) {
    bug( "Close_Socket: Closing a dead socket??" );
    return;
  }

  if( ( ch = link->player ) && ch != link->character )
    do_return( link->character, "" );

  if( process ) {
    link->connected = CON_CLOSING_LINK;
    process_output( link );
  }

  if( ch ) {
    if( connected == CON_PLAYING ) {
      char *buf1 = static_string();
      char *buf2 = static_string();
      snprintf( buf1, THREE_LINES, "%s has lost link.", ch->descr->name );
      snprintf( buf2, THREE_LINES, "%s has lost link at %s.",
		ch->descr->name,
		ch->Location( ) );
      info( invis_level( ch ), buf1, LEVEL_APPRENTICE, buf2, IFLAG_LOGINS, 2, ch );
      wizard_data *imm = wizard( ch );
      room_data *room;
      if( imm
	  && is_builder( imm )
	  && ( room = get_room_index( imm->recall ) )
	  && room != imm->in_room
	  && ( !is_set( room->room_flags, RFLAG_OFFICE )
	       || imm->office == room->vnum
	       || is_demigod( imm ) ) ) {
	raw_goto( imm, imm->in_room, room );
      } else {
	fsend_seen( ch, "%s stops paying attention.", ch );
      }
      ch->link = 0;
    } else {
      // Character creating, not yet saved.
      if( ch->Level() == 0 && ch->pcdata->pfile ) 
        extract( ch->pcdata->pfile, link );
      ch->Extract( );
    }
  }

  close( link->channel );
  extract( link );
}


/*
 *   INPUT ROUTINES
 */

/* I think this function erases the inputed line from the characters
   terminal.  Returns false if it couldn't write to the link.  Returns true
   if it did what it should, or if it didn't need to */
bool erase_command_line( char_data* ch )
{
  char *tmp  = static_string( );

  /* If there is no character, the characters terminal is set to dumb, 
     the character isn't playing, or the player is using the status bar ..
     return TRUE */
  if( !ch
      || ch->pcdata->terminal == TERM_DUMB  
      || ch->link->connected != CON_PLAYING
      || !is_set( ch->pcdata->pfile->flags, PLR_STATUS_BAR ) )
    return true;

  // Move cursor to input line, then erase to bottom of screen.
  // In other words, erase command input by user.
  const ssize_t len = snprintf( tmp, THREE_LINES, "[%hd;1H[J", ch->pcdata->lines );

  /* Write the above cryptic command sequence to the link.  If you can't
     return false */
  if( write( ch->link->channel, tmp, len ) == -1 )
    return false; 
  
  /* Correctly worked, return TRUE */
  return true;
}


/* This function reads the input from the link, adding it to the end of
   any pending commands.
   Returns true if the inputted command was correctly stored in the 
   link->receive string.  Returns false if there was an error writing to 
   or reading from the link, or if too much input was recieved from
   someone who isn't playing yet */
bool read_link( link_data* link )
{
  char            buf  [ 2*MAX_INPUT_LENGTH+100 ];
  text_data*  receive;
  char*         input;
  char*        output;

  // Move any pending input into the buffer
  strcpy( buf, link->rec_pending );

  //get the length of the buffer, then read in 100 character from the 
  //link, putting them after the pending input in the buffer.
  const size_t length = strlen( buf );
  const ssize_t nRead = read( link->channel, buf+length, 100 );

  //if nothing new was read in, return FALSE
  if( nRead <= 0 )
    return false;

  //free up the pending command string.
  free_string( link->rec_pending, MEM_LINK );
  link->rec_pending = empty_string;

  //put the null termination at the end of the input
  buf[ length+nRead ] = '\0';

  // Handle telnet sequences.
  // Refuse DO TIMING-MARK.
  // Ignore all other sequences.
  for( size_t i = length; i < length+nRead; ++i ) {
    const unsigned char c = buf[ i ];
    if( !link->in_command && !link->in_option && link->after_iac ) {
      link->cmd = c;
    } else if( link->in_option ) {
      if( link->cmd == DO && c == TELOPT_TM ) {
	text_data *text = new text_data( refuse_tm_str );
	append( link->send_telnet, text );
      }
    }

    // Update telnet command state.
    link->in_option = !link->in_command && link->after_iac && ( c >= WILL && c <= DONT );
    link->in_command = !link->in_command && link->after_iac && ( c == SB )
                       || link->in_command && !(link->after_iac && ( c == SE ) );
    link->after_iac = ( c == IAC && !link->after_iac );
  }

  /* If too much stuff is in the buffer, tell the player that the input
     is being truncated.  If there is no one playing, return
     FALSE.  Put the newline at the end of the buffer
     to truncate. */
  if( length+nRead > MAX_INPUT_LENGTH-2 ) {
    if( link->connected != CON_PLAYING )
      return false;
    send( link->character, "!! Truncating input !!\n\r" );
    sprintf( buf+MAX_INPUT_LENGTH-3, "\n\r" );
  }

  for( input = output = buf; *input; ++input ) {
 
    /* Check if input is a newline.  If it isn't, check and see if it is a 
       printable character.  If it isn't, skip it.
       Increment the output (if the character is
       printable), and continue the for loop */
    if( *input != '\n' ) {
      if( isprint( *input ) )
        *output++ = *input;
      continue;
    }

 
    /* Hit a newline, so back up to the last non-space character*/
    while( --output >= buf && *output == ' ' );
    
    /* Terminate the string after the last non-space :) */
    *(++output) = '\0';

    /* If the character isn't playing, point the receive pointer to 
       the buffer */
    if( link->connected != CON_PLAYING ) { 
      receive = new text_data( buf );
    /* Else if ithe first buffer character is a !, execute the previous
       command */
    } else if( *buf == '!' ) {
      receive = new text_data( link->rec_prev );
    /* Otherwise, parse the input for aliases, set the recieve value
       to the inputted command, and reset the previous command. */
    } else {
      receive = new text_data( subst_alias( link, buf ) );
      // Blank input doesn't replace history.
      if( *receive->message.text ) {
	free_string( link->rec_prev, MEM_LINK );
	link->rec_prev = alloc_string( receive->message.text, MEM_LINK );
      }
    }
    /* Put the newly received commands on the end of the command queue,
       and reset the output pointer */
    append( link->receive, receive );
    output = buf;
    
    // Erase input command line.
    if( !erase_command_line( link->character ) )
      return false;
  }

  /* terminate the output string (points to the front of buf).  Then
     set the pending command to point to the buf. */
  *output = '\0';
  link->rec_pending = alloc_string( buf, MEM_LINK ); 

  return true;
}


/*
 *   OUTPUT ROUTINES
 */


static bool process_output( link_data *link )
{
  char_data *ch = link->character;

  if( link->connected == CON_PLAYING ) {
    if( !ch ) {
      /* If the character is playing, but there is no character, report the
	 bug and return false */
      bug( "Process_Output: Link playing with null character." );
      bug( "--     Host = '%s'", link->host );
      bug( "-- Rec_Prev = '%s'", link->rec_prev );
      return false;
    }
    show_burden( ch );
  }

  /* If there is nothing to send, and there is no command pending,
     return TRUE */
  if( !link->send && !link->command && !link->send_telnet )
    return true;

  while( text_data *output = link->send_telnet ) {
    errno = 0;
    const size_t len = output->remain();
    const ssize_t bytes = write( link->channel, output->ptr, len );
    if( bytes < 0 ) {
      if( errno == EAGAIN ) {
	link->again = true;
	return true;
      }
      return false;
    } else if( (const size_t)bytes < len ) {
      output->ptr += bytes;
      link->again = true;
      return true;
    }
    link->send_telnet = output->next;
    delete output;
  }

  if( !link->again ) {
    /* If the character is playing, has the status bar turned on, and isn't
       using terminal type DUMB, set status bar to TRUE. */
    const bool status_bar = ( link->connected == CON_PLAYING
			      && is_set( ch->pcdata->pfile->flags, PLR_STATUS_BAR )
			      && ch->pcdata->terminal != TERM_DUMB );
    
    /* SAVE CURSOR, SHOW PROMPT */
    
    if( status_bar ) {
      text_data *next = link->send;
      link->send = 0;
      scroll_window( ch );
      if( next ) {
	set_bit( ch->status, STAT_NO_SNOOP );
	send( ch, "\n\r" );
	remove_bit( ch->status, STAT_NO_SNOOP );
      }
      cat( link->send, next );
      set_window_title( ch );
      prompt_ansi( link );
      command_line( ch );

    } else {
      if( !link->command ) {
	text_data *next = link->send;
	link->send = 0;
	if( ch ) {
	  set_bit( ch->status, STAT_NO_SNOOP );
	  send( ch, "\n\r" );
	  remove_bit( ch->status, STAT_NO_SNOOP );
	}
	cat( link->send, next );
      }  
      if( link->connected == CON_PLAYING && !link->receive ) {
	set_window_title( ch );
	prompt_nml( link );
      }
    }
  
    link->newline = true;
  }

  /* SEND OUTPUT */

  while( text_data *output = link->send ) {
    errno = 0;
    const size_t len = output->remain();
    const ssize_t bytes = write( link->channel, output->ptr, len );
    if( bytes < 0 ) {
      if( errno == EAGAIN ) {
	link->again = true;
	return true;
      }
      return false;
    } else if( (const size_t)bytes < len ) {
      output->ptr += bytes;
      link->again = true;
      return true;
    }
    link->send = output->next;
    delete output;
  }

  link->again = false;
  return true;
}


/*
 *   LOGIN ROUTINES
 */


typedef void login_func( link_data*, const char * );


struct login_handle
{
  login_func*  function;
  int          state;
};


void nanny( link_data* link, const char *argument )
{
  const login_handle nanny_list [] = {
    { nanny_intro,              CON_INTRO              },
    { nanny_acnt_name,          CON_ACNT_NAME          },
    { nanny_acnt_password,      CON_ACNT_PWD           },
    { nanny_acnt_email,         CON_ACNT_EMAIL         },
    { nanny_acnt_enter,         CON_ACNT_ENTER         },
    { nanny_acnt_confirm,       CON_ACNT_CONFIRM       },
    { nanny_acnt_check,         CON_ACNT_CHECK         },
    { nanny_acnt_check_pwd,     CON_ACNT_CHECK_PWD     },
    { nanny_old_password,       CON_PASSWORD_ECHO      },
    { nanny_old_password,       CON_PASSWORD_NOECHO    },
    { nanny_motd,               CON_READ_MOTD          },
    { nanny_imotd,              CON_READ_IMOTD         },
    { nanny_new_name,           CON_GET_NEW_NAME       },
    { nanny_acnt_request,       CON_ACNT_REQUEST       },
    { nanny_acnt_menu,          CON_ACNT_MENU          },
    { nanny_confirm_password,   CON_CONFIRM_PASSWORD   },
    { nanny_set_term,           CON_SET_TERM           },
    { nanny_show_rules,         CON_READ_GAME_RULES    },
    { nanny_agree_rules,        CON_AGREE_GAME_RULES   },
    { nanny_alignment,          CON_GET_NEW_ALIGNMENT  },
    { nanny_help_alignment,     CON_HELP_ALIGNMENT     },
    { nanny_disc_old,           CON_DISC_OLD           },
    { nanny_help_class,         CON_HELP_CLSS          },
    { nanny_class,              CON_GET_NEW_CLSS       },
    { nanny_help_race,          CON_HELP_RACE          },
    { nanny_race,               CON_GET_NEW_RACE       },
    { nanny_stats,              CON_DECIDE_STATS       },  
    { nanny_help_sex,           CON_HELP_SEX           },
    { nanny_sex,                CON_GET_NEW_SEX        },   
    { nanny_new_password,       CON_GET_NEW_PASSWORD   },
    { nanny_acnt_enter,         CON_CE_ACCOUNT         },
    { nanny_acnt_check_pwd,     CON_CE_PASSWORD        },
    { nanny_acnt_email,         CON_CE_EMAIL           },
    { nanny_acnt_enter,         CON_VE_ACCOUNT         },
    { nanny_ve_validate,        CON_VE_VALIDATE        },
    { nanny_acnt_confirm,       CON_VE_CONFIRM         },
    { nanny_acnt_enter,         CON_ACNT_RESEND_REQ    },
    { 0,                        -1                     }
  };

  skip_spaces( argument );

  for( int i = 0; nanny_list[i].function; i++ ) 
    if( link->connected == nanny_list[i].state ) {
      nanny_list[i].function( link, argument );
      return;
    }

  if( link->connected == CON_PAGE ) {
    write_greeting( link );
    link->connected = CON_INTRO;
    return;
  }

  /*
  if( link->connected == CON_FEATURES ) {
    help_link( link, "Features_2" );
    link->connected = CON_PAGE;
    return;
  }
  */

  if( link->connected == CON_POLICIES ) {
    help_link( link, "Policy_2" );
    link->connected = CON_PAGE;
    return;
  }

  bug( "Nanny: bad link->connected %d.", link->connected );
  close_socket( link );
}


/* Function pulls a player out of the void, and puts him back where he was */
void stop_idling( player_data *pl )
{
  /* If there is no player, there is no link, the player isn't playing,
     or the character has never been in a room, return */
  if( !pl
      || !pl->link
      || pl->link->connected != CON_PLAYING
      || !pl->was_in_room )
    return;

  /* Otherwise, set the player's timer, and return the player to the room
     that he was in */
  pl->timer = current_time;

  if( pl->array )
    pl->From( );
  pl->To( pl->was_in_room );
  
  char *buf = static_string();
  snprintf( buf, THREE_LINES, "%s has returned from the void at %s.",
	    pl->descr->name,
	    pl->Location( ) );
  info( LEVEL_IMMORTAL, empty_string, invis_level( pl ), buf, IFLAG_LOGINS, 3, pl );

  fsend_seen( pl, "%s is here, although you didn't see %s arrive.",
	      pl, pl->Him_Her() );
}


void write_greeting( link_data* link )
{
  help_link( link, "greeting" );
  send( link, "                   Choice: " );
}
