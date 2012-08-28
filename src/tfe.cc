#include <errno.h>
#include <sys/types.h>
#include <pwd.h>
#include <signal.h>
#include <stdio.h>
#include <syslog.h>
#include <stdlib.h>
#include <unistd.h>
#include "define.h"
#include "struct.h"


char            str_boot_time  [ 26 ];
int             port = DEFAULT_PORT;
char *tz = 0;

bool                 tfe_down  = false;
time_t           current_time; 
time_t              boot_time;
static time_data last_time;
static time_data tick_time;

static void  init_variables      ( );
static void  set_time            ( );
static void  wait_pulse          ( );
static void  record_time         ( time_data& );


/* Definition of the signal handler. */
void sighandler( int sig )
{
  char* name;

  switch( sig ) {
  case SIGPIPE :   name = "broken pipe";  break;
  case SIGBUS  :   name = "bus error";    break;
  default      :   name = "unknown";      break; 
  }

  roach( "Mud received signal %d, %s.", sig, name );
  roach( "Error: %s", strerror( errno ) );
}  


/*
 *   MAIN PROCEDURE
 */      


int main( int argc, char **argv )
{
  delete_control_file( REBOOT_FILE );
  delete_control_file( PANIC_FILE );
  create_control_file( SHUTDOWN_FILE, "failed boot sequence" );

  int x = 1;

  if( argc > x
      && !strcmp( argv[x], "-w" ) ) {
    create_control_file( WIZLOCK_FILE, "boot flag" );
    wizlock = true;
    printf( "Starting wizlocked.\n" );
    ++x;
  } else if( exist_control_file( WIZLOCK_FILE ) ) {
    wizlock = true;
    printf( "Starting wizlocked.\n" );
  }

  if( argc > x
      && !strcmp( argv[x], "-g" ) ) {
    create_control_file( GODLOCK_FILE, "boot flag" );
    godlock = true;
    printf( "Starting godlocked.\n" );
    ++x;
  } else if( exist_control_file( GODLOCK_FILE ) ) {
    godlock = true;
    printf( "Starting godlocked.\n" );
  }

  if( argc > x ) {
    port = atoi( argv[x] );
    ++x;
  }

  const struct passwd *const pw = getpwuid( getuid( ) );
  if( pw ) {
    printf( "Process Owner: %s\n", pw->pw_name );
  }

  // Get the TZ environment variable.
  if( const char *env = getenv( "TZ" ) ) {
    tz = new char [ strlen( env ) + 1 ];
    strcpy( tz, env );
  }

  signal( SIGPIPE, sighandler );
  signal( SIGBUS, sighandler );

  time_data start;
  gettimeofday( &start, 0 );

  set_time( );
  init_memory( );
  init_variables( );

  bug( -1, "** STARTING MUD **" );

  //  recover_links( );
  init_daemon( );
  boot_db( );

  /*
  write_all( true );
  save_badname( );
  save_banned( );
  save_remort( );
  save_accounts( );
  save_notes( -1 );
  save_mail( 0 );
  exit( 0 );
  */

  mud_socket = open_port( port );
  who_socket = open_port( port+1 );

  echo( "TFE done booting\n\r" );
  bug( -1, "** MUD BOOTED **" );

  //  restart_links( );

  startup_time = stop_clock( start ).tv_sec;

  delete_control_file( SHUTDOWN_FILE );

  // Init timings.
  tick_time.tv_sec = 0;
  tick_time.tv_usec = 1000000/PULSE_PER_SECOND;
  gettimeofday( &last_time, 0 );
  current_time = last_time.tv_sec;

  // Main loop.
  while( !tfe_down ) {
    update_handler( );
    read_host( );
    update_links( );
    wait_pulse( );
  }

  kill_daemon();

  return 0;
}


/*
 *   TIME HANDLING ROUTINES
 */


void set_time( )
{
  time_data now_time;
  gettimeofday( &now_time, 0 );

  current_time = now_time.tv_sec;
  boot_time = current_time;

  strcpy( str_boot_time, ctime( &current_time ) );
  srand( current_time );
}


void wait_pulse( )
{
  time_data cycle_time;
  gettimeofday( &cycle_time, 0 );

  cycle_time -= last_time;

  record_time( cycle_time );
  
  total_time[ TIME_ACTIVE ] += cycle_time;
  
  if( cycle_time < tick_time ) {
    // Lead.
    time_data lead_time = tick_time;
    lead_time -= cycle_time;
    total_time[ TIME_WAITING ] += lead_time;
    if( select( 0, 0, 0, 0, &lead_time ) < 0 ) 
      bug( "Wait_Pulse: error in select" );
    
  } else {
    // Lag.
    critical_time[ TIME_ACTIVE ] += cycle_time;
    cycle_time -= tick_time;
    critical_time[ TIME_WAITING ] += cycle_time;
    
    for( int i = TIME_WAITING+1; i < MAX_TIME; ++i ) {
      critical_time[i] += pulse_time[i];
    }
  }
  
  for( int i = TIME_WAITING+1; i < MAX_TIME; ++i ) {
    total_time[i] += pulse_time[i];
    pulse_time[i] -= pulse_time[i];
  }

  gettimeofday( &last_time, 0 );
  current_time = last_time.tv_sec;
}


void record_time( time_data& time )
{
  long long lag = time.tv_sec*PULSE_PER_SECOND + time.tv_usec*PULSE_PER_SECOND/1000000;
  long i = 0;
  for( i = 0; lag > 0 && i < 9; lag >>= 1, ++i ); 
  ++time_history[i];
  time_total[i] += time.time( );
}
 

/*
 *   ROUTINE TO INIT CONSTANTS
 */


void init_variables( )
{
  vzero( ident_list, MAX_PFILE );
  vzero( event_queue, QUEUE_LENGTH );
  vzero( info_history, MAX_IFLAG );
  vzero( quest_list, MAX_QUEST );
  vzero( time_history, 10 );
  vzero( time_total, 10 );
}
