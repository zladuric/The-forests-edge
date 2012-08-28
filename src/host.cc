#include <ctype.h>
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
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/telnet.h>
#include <signal.h>
#include "define.h"
#include "struct.h"


pid_t      daemon_pid   = -1;
char       read_buffer  [ FOUR_LINES ];
link_data*  host_stack  = 0;
int          read_pntr  = 0;
int              input  = -1;
int             output  = -1;

void  connect_link   ( link_data*, const char* );


/* If the daemon isn't working, try to respawn him */
void broken_pipe( )
{
  roach( "Write_Host: Pipe to host daemon is broken." );
  roach( "Write_Host: Attempting to revive daemon." );
  init_daemon( );
}


/* This function spawns a daemon to do the host name checking.  It returns
   true if it could spawn the daemon, false if it couldn't */
bool init_daemon( )
{
  char       tmp  [ ONE_LINE ];
  int     pipe_a  [ 2 ];  //TFE to daemon communication
  int     pipe_b  [ 2 ];  //daemon to TFE communication

  kill_daemon();

  printf( "Waking daemon...\n" );

  /* Open new pipes, pipe_a and pipe_b */
  pipe( pipe_a );
  pipe( pipe_b );  

  /* split the process.  If we are in the child, execute if statment.  If
     not, skip it. */
  if( ( daemon_pid = fork( ) ) == (pid_t) 0 ) { 
    /* CHILD PROCESS */
    
    /*dup the pipes, and close off the ends */
    dup( pipe_a[0] );
    dup( pipe_b[1] );
    close( pipe_a[1] );
    close( pipe_b[0] );

    /* Tell everyone which pipes the dameon is using */
    printf( "Init_Daemon: Pipes are %d and %d.\n",
	    pipe_a[0], pipe_b[1] );

    /* Start the daemon and exit the child */
    snprintf( tmp, ONE_LINE, "%d", pipe_a[0] );
    snprintf( tmp+9, ONE_LINE-9, "%d", pipe_b[1] );

    if( execlp( "./daemon", "./daemon", &tmp[0], &tmp[9], (char*) 0 ) == -1 )
      printf( "Init Daemon: Error in Execlp.\n" ); 
     
    exit( 1 );

  } else if( daemon_pid == -1 ) {
    /* If couldn't split the process, return FALSE */
    return false;
  }

  /* PARENT PROCESS */

  /* close read on pipe a, and write on pipe b */
  close( pipe_a[0] );
  close( pipe_b[1] );
  
  /* set input to the pipe coming in from the daemon, and output to the 
     pipe going to the daemon */
  input  = pipe_b[0];
  output = pipe_a[1];
  
  /* Make it so the process doesn't wait for new information everytime
     it checks the pipes */
  //  fcntl( input,  F_SETFL, O_NDELAY );
  //  fcntl( output, F_SETFL, O_NDELAY );
   
   /* CLOSE PIPE ON EXEC */
   
  /*If an EXEC family call is made, close the pipes */
  fcntl( input, F_SETFD, 1 );
  fcntl( output, F_SETFD, 1 );
  
  return true;
}


/* function walks through the links, checking who is playing. returns that number */
int players_on( )
{
  int num = 0;

  for( link_data *link = link_list; link; link = link->next ) {
    if( link->connected == CON_PLAYING
	&& !is_set( link->character->pcdata->pfile->flags, PLR_WIZINVIS ) ) {
      ++num;
    }
  }

  return num;
}


/* Function that takes a new link, tells it that it connected, and tries
   to determine the host name.  It does this by calling a daemon program,
   which is supposed to determine it and return it :) */
void write_host( link_data *link, const struct in_addr& inaddr )
{
  //  char*  tmp2  = static_string( );
  /* If there are too many links attempting to connect, tell the host deamon
     to knock it off and get back to work */
  if( count( host_stack ) > 20 ) {
    aphid( "Write_Host: Host daemon is swamped." );
  } else if( write( output, &inaddr, sizeof( struct in_addr ) ) == -1 ) {
    /* Otherwise, try to tell the daemon the hostname. */
    broken_pipe( );
  } else {
    /* If you wrote to the daemon, go ahead and append the link to the
       host stack */
    append( host_stack, link );
    return;
  }
  
  /* If the daemon was swamped, or if you couldn't write to him, go ahead 
     and let the poor guy in, just don't bother getting his host address. */

  /*
  int    addr;
  memcpy( &addr, name, sizeof( int ) ); 
  addr = ntohl( addr );
  */
  const u_long addr = ntohl( inaddr.s_addr );

  // Make IP address.
  char *tmp = static_string( );
  snprintf( tmp, THREE_LINES, "%lu.%lu.%lu.%lu",
	    ( addr >> 24 ) & 0xFF,
	    ( addr >> 16 ) & 0xFF,
	    ( addr >>  8 ) & 0xFF,
	    ( addr       ) & 0xFF );
  
  // Connect the link
  connect_link( link, tmp );
}
 

void read_host( )
{ 
  time_data start;  
  gettimeofday( &start, 0 );
 
  struct timeval timeout;
  timeout.tv_sec = 0;
  timeout.tv_usec = 0;

  fd_set read_fds;
  FD_ZERO( &read_fds );
  FD_SET( input, &read_fds );

  const int selectStatus = select( input+1, &read_fds, 0, 0, &timeout );

  if( selectStatus == -1 ) {
    fprintf( stderr, "TFE: Error reading from daemon.\n" );
    perror( "read" );
    pulse_time[ TIME_DAEMON ] = stop_clock( start );  
    return;
  }

  if( selectStatus == 0 ) {
    pulse_time[ TIME_DAEMON ] = stop_clock( start );  
    return;
  }
    
  // input received
  const ssize_t nRead = read( input, &read_buffer[read_pntr], FOUR_LINES-read_pntr );

  if( nRead == -1 ) {
    fprintf(stderr, "TFE: Error reading from daemon.\n");
    perror("read");
    broken_pipe( );
    pulse_time[ TIME_DAEMON ] = stop_clock( start );  
    return;
  } 

  read_pntr += nRead;

  int i;

  do {
    for( i = 0; i < read_pntr; ++i )
      if( !read_buffer[i] ) // search for a null
	break;
    
    // If there isn't a complete hostname string in the buffer, give up.
    if( i == read_pntr ) {
      pulse_time[ TIME_DAEMON ] = stop_clock( start );  
      return;
    }
    
    // i is pointing to the end of a string now
    
    if( strcmp( read_buffer, "Alive?" ) && host_stack ) {
      // read_buffer is a valid hostname string or IP address string
      // assumes that IP address in host_stack matches what we just read
      link_data *link = host_stack;
      host_stack = host_stack->next;
      connect_link( link, read_buffer );
    }
    
    // copy remainder of the buffer i characters backwards
    for( int j = i+1; j < read_pntr; ++j )
      read_buffer[j-i-1] = read_buffer[j];
    
    read_pntr -= i+1;
  } while( read_pntr > 0 );
  

  /*If the read pointer is less then one line in length, read in some
    from the daemon.  If you couldn't access the daemon, try to wake
    him up, and stop reading. If you accessed him, increase the read
    pointer by the amount read in */
  /*
  if( read_pntr < ONE_LINE ) {
    if( ( nRead = read( input, &read_buffer[read_pntr], 50 ) ) == -1 ) {
      if( errno != 11 )
	{ 
	  broken_pipe( );
	  return;
	}
      else
	nRead = 0;
    } 
      
    read_pntr += nRead;
  }
 
   
  // Step through the read buffer till the end of the string
  for( i = 0; i < read_pntr; i++ )
    if( read_buffer[i] == '\0' )
      break;
  
  // If we hit the read pointer before hitting the end of string, then
  // we haven't gotten a full host name from the daemon, so return.
  if( i == read_pntr )
    return;

  
  // If the daemon wants to know wether we are still alive, try to
  // connect the next host in the host_stack as a responce
  if( strcmp( read_buffer, "Alive?" ) ) {
    link       = host_stack;
    host_stack = host_stack->next;
    connect_link( link, read_buffer );
  }
  
  // The daemon has given us a host address.  i points to the end of
 //  the string in the read buffer.
  for( j = i+1; j < read_pntr; j++ )
    read_buffer[j-i-1] = read_buffer[j];
  
  read_pntr -= i+1;
  */


  //Determine how much time it took the daemon to do its work
  pulse_time[ TIME_DAEMON ] = stop_clock( start );  
}


/* Takes a link and a host name.  Sets the link->host string to the host
   name, and adds the link to the 'link' list.  Write the greeting to
   the new link.  Send info to interested parties about the new link */
void connect_link( link_data* link, const char* host )
{  
  link->host = alloc_string( host, MEM_LINK );
  link->next = link_list;
  link_list  = link;

  char  tmp  [ TWO_LINES ];

  if( is_banned( host ) ) {
    snprintf( tmp, TWO_LINES, "Connection from banned site %s refused.", link->host );
    info( LEVEL_DEMIGOD, empty_string, 0, tmp, IFLAG_LOGINS, 3 );
    close_socket(link);
    return;
  }

  /* Tell incoming players that they have connected */
  char *tmp1 = static_string( );
  char *tmp2 = static_string( );
  sprintf_minutes( tmp1, current_time-boot_time );
  const int players = players_on( );
  snprintf( tmp2, THREE_LINES, "\n\r%d player%s on.\n\rSystem started %s ago.\n\r\
Getting site info ...\n\r",
	    players,
	    players == 1 ? "" : "s",
	    tmp1 );
  write( link->channel, tmp2, strlen( tmp2 ) );  

  write_greeting( link );

  snprintf( tmp, TWO_LINES, "Connection from %s.", link->host );
  info( LEVEL_DEMIGOD, empty_string, 0, tmp, IFLAG_LOGINS );
}


void kill_daemon()
{
  if( daemon_pid > 0 ) {
    printf( "Kill_Daemon: Killing hostname daemon...\n");
    if( input != -1 ) {
      close( input );
      close( output );
    }
    kill( daemon_pid, SIGTERM );
    daemon_pid = -1;
    
    // Purge the host_stack.
    link_data* tmp_link;
    while( host_stack ) {
      aphid( "Kill_Daemon: Closing pending connection." );
      if( host_stack->channel > 0 ) {
	close( host_stack->channel );
      }
      tmp_link = host_stack;
      host_stack = host_stack->next;
      delete tmp_link;
    }
  }
}
