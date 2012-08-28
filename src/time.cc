#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include "define.h"
#include "struct.h"


time_t      startup_time;
unsigned    time_history   [ 10 ];
long long   time_total     [ 10 ];

time_data   pulse_time     [ MAX_TIME ];
time_data   total_time     [ MAX_TIME ];
time_data   critical_time  [ MAX_TIME ];


const char *const SWeekday[7] = {
  "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};

const char *const SMonth[12] = {
   "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct",
   "Nov", "Dec" };


/*
 *   STRING PRINT FUNCTIONS
 */


void sprintf_minutes( char* tmp, const time_t time )
{
  const int days    = time/(60*60*24);
  const int hours   = (time/3600)%24;
  const int minutes = (time/60)%60;
  const int seconds = time%60;
 
  unsigned count = 0;

  *tmp = '\0';
  char *t = tmp;

  if( days > 0 ) {
    t += sprintf( t, "%d day%s", days, days == 1 ? "" : "s" );
    ++count;
  }

  if( hours > 0 ) {
    t += sprintf( t, "%s%d hour%s",
		  ( count == 1 ) ? " and " : "",
		  hours, hours == 1 ? "" : "s" );
    ++count;
  }

  if( minutes > 0 && count < 2 ) {
    t += sprintf( t, "%s%d minute%s",
		  ( count == 1 ) ? " and " : "",
		  minutes, minutes == 1 ? "" : "s" );
    ++count;
  }

  if( seconds > 0 && count < 2 ) {
    t += sprintf( t, "%s%d second%s",
		  ( count == 1 ) ? " and " : "",
		  seconds, seconds == 1 ? "" : "s" );
    ++count;
  }

  if( count == 0 ) {
    t += sprintf( t, "less than one second" );
  }
}


void sprintf_time( char* tmp, const time_t time, const char *zone )
{
  /*
  int hours   = (time/3600+letter-'0')%12+1;
  int minutes = (time/60)%60;
 
  sprintf( tmp, "%d:%02d", hours, minutes );
  */

  if( zone && *zone ) {
    setenv( "TZ", zone, 1 );
  }

  struct tm *local = localtime( &time );
  char buf [6];
  strftime( buf, 6, "%I:%M", local );

  if( !*buf ) {
    strcpy( tmp, buf+1 );
  } else {
    strcpy( tmp, buf );
  }

  if( zone && *zone ) {
    if( tz ) {
      setenv( "TZ", tz, 1 );
    } else {
      unsetenv( "TZ" );
    }
  }
}


char *ltime( const time_t& time, bool all, const char_data *ch )
{
  account_data *account = ( ch && ch->pcdata ) ? ch->pcdata->pfile->account : 0;

  return ltime( time, all, account );
}


char *ltime( const time_t& time, bool all, const account_data *account )
{
  const bool zone = account && *account->timezone;

  if( zone ) {
    setenv( "TZ", account->timezone, 1 );
  }

  struct tm *tmPtr  = localtime( &time );
  char *tmp = static_string( );

  if( all ) {
    snprintf( tmp, THREE_LINES, "%02d:%02d %02d %3s %04d",
	      tmPtr->tm_hour,
	      tmPtr->tm_min,
	      tmPtr->tm_mday,
	      SMonth[tmPtr->tm_mon],
	      tmPtr->tm_year + 1900 );
  } else if( abs((int)(time-current_time)) > 300*24*60*60 ) {
    snprintf( tmp, THREE_LINES, "%s %s %02d %04d",
	      SWeekday[tmPtr->tm_wday],
	      SMonth[tmPtr->tm_mon],
	      tmPtr->tm_mday,
	      tmPtr->tm_year + 1900 );
  } else {
    snprintf( tmp, THREE_LINES, "%s %s %02d %02d:%02d",
	      SWeekday[tmPtr->tm_wday],
	      SMonth[tmPtr->tm_mon],
	      tmPtr->tm_mday,
	      tmPtr->tm_hour,
	      tmPtr->tm_min );
  }

  if( zone ) {
    if( tz ) {
      setenv( "TZ", tz, 1 );
    } else {
      unsetenv( "TZ" );
    }
  }

  return tmp;
}


void sprintf_date( char* tmp, const time_t time )
{
  struct tm *tmPtr = localtime( &time );
 
  sprintf( tmp, "%s %d%s %d",
	   SMonth[ tmPtr->tm_mon ], tmPtr->tm_mday,
	   number_suffix( tmPtr->tm_mday ), tmPtr->tm_year + 1900);
}


/*
 *   LAG ROUTINES
 */


const time_data& stop_clock( const struct timeval& start )
{
  static time_data time;
  gettimeofday( &time, 0 );

  time -= start;

  /*
  time.tv_sec  = stop.tv_sec-start.tv_sec;
  time.tv_usec = stop.tv_usec-start.tv_usec;

  if( time.tv_usec < 0 ) {
    time.tv_usec += 1000000;
    time.tv_sec  -= 1;
  }
  */

  return time;
}


static void display_lag( char_data* ch, const char *name, const time_stats& t )
{
  const unsigned calls = t.calls;
  const long long total = t.total_time.time( );
  const long long max = t.max_time.time( );

  page( ch, "%-22s %8u %10.3f %11.3f %9.1f\n\r", 
	name, calls,
	calls == 0 ? 0.0 : (double) total/(1000*calls),
	(double) max/1000, (double) total/1000 );
}


#define LAG_SORT 50


static void order( int *list, double *value, int label, double num )
{
  if( value[ LAG_SORT-1 ] > num )
    return;

  int i;

  for( i = LAG_SORT-1; i > 0; i-- ) {
    if( value[i-1] > num )
      break;
    list[i] = list[i-1];
    value[i] = value[i-1];
  }

  list[i] = label;
  value[i] = num;
}


void do_lag( char_data* ch, const char *argument )
{
  char      tmp  [ TWO_LINES ];
  int     flags;
  int      i, j;

  if( !get_flags( ch, argument, &flags, "hrRsf", "lag" ) )
    return;;

  const int length = strlen( argument );

  if( is_set( flags, 0 ) ) {
    if( is_set( flags, 2 ) ) {
      for( i = 0; i < 10; ++i ) {
	time_history[i] = 0;
	time_total[i] = 0;
      }
      send( ch, "Lag history cleared.\n\r" );
      return;
    }
    long long sum = 0;
    long long total = 0;
    long long total_time = 0;
    long long total_lag = 0;
    long long time_time[10];
    long long lag_time[10];
    for( i = 0; i < 10; ++i ) {
      sum += time_history[i];
      total += time_total[i];
      if( i != 0 ) {
	total_time += time_total[i];
	time_time[i] = time_total[i];
	lag_time[i] = time_total[i] - ((long long)time_history[i])*(1000000/PULSE_PER_SECOND);
	total_lag += time_total[i] - ((long long)time_history[i])*(1000000/PULSE_PER_SECOND);
      } else {
	total_time += time_history[0]*(1000000/PULSE_PER_SECOND);
	time_time[i] = time_history[0]*(1000000/PULSE_PER_SECOND);
	lag_time[i] = 0;
      }
    }
    if( total == 0 )
      total = 1;
    if( total_time == 0 )
      total_time = 1;
    if( total_lag == 0 )
      total_lag = 1;
    send_underlined( ch,
      "Sec. Delay     Cycles     % Cycles        % CPU       % Time        % Lag\n\r" );
    for( j = 0, i = 0; i < 10; ++i, j = ( j == 0 ? 1 : 2*j ) ) {
      snprintf( tmp, TWO_LINES, "%.3f", (double) j/PULSE_PER_SECOND );
      snprintf( tmp+10, TWO_LINES-10, "%8s %12d %12.2f %12.2f %12.2f %12.2f\n\r",
		i == 0 ? "none" :
		( i == 9 ? "more" : tmp ),
		time_history[i],
		100.0 * ( (double) time_history[i] ) / sum,
		100.0 * ( (double) time_total[i] ) / total,
		100.0 * ( (double) time_time[i] ) / total_time,
		100.0 * ( (double) lag_time[i] ) / total_lag );
      send( ch, tmp+10 );
    }
    return;
  }

  if( is_set( flags, 3 ) ) {
    if( is_set( flags, 2 ) ) {
      for( i = 0; i < table_max[ TABLE_SKILL_SPELL ]; ++i ) {
	skill_spell_table[i].calls = 0;
	skill_spell_table[i].total_time.zero( );
	skill_spell_table[i].max_time.zero( );
      }
      send( ch, "Spell execution time statistics cleared.\n\r" );
      return;
    }

    page_underlined( ch,
		     "%-22s    Calls    Average    Max Time     Total\n\r",
		     "Spell" );
    
    if( is_set( flags, 1 ) ) {
      int list [ LAG_SORT ];
      double value [ LAG_SORT ];
      vzero( list, LAG_SORT );
      vzero( value, LAG_SORT );
      
      for( i = 0; i < table_max[ TABLE_SKILL_SPELL ]; ++i )
	order( list, value, i, skill_spell_table[i].total_time.time() );
      for( i = 0; i < LAG_SORT; ++i )
	display_lag( ch,
		     skill_spell_table[ list[i] ].name,
		     skill_spell_table[ list[i] ] );
      
      return;
    }

  
    for( i = 0; i < table_max[ TABLE_SKILL_SPELL ]; ++i ) 
      if( !strncasecmp( skill_spell_table[i].name, argument, length ) )
	display_lag( ch,
		     skill_spell_table[i].name,
		     skill_spell_table[i] );

    return;
  }

  if( is_set( flags, 4 ) ) {
    if( is_set( flags, 2 ) ) {
      for( i = 0; i < table_max[ TABLE_FUNCTION ]; ++i ) {
	function_table[i].calls = 0;
	function_table[i].total_time.zero( );
	function_table[i].max_time.zero( );
      }
      send( ch, "Function execution time statistics cleared.\n\r" );
      return;
    }

    page_underlined( ch,
		     "%-22s    Calls    Average    Max Time     Total\n\r",
		     "Spell" );
    
    if( is_set( flags, 1 ) ) {
      int list [ LAG_SORT ];
      double value [ LAG_SORT ];
      vzero( list, LAG_SORT );
      vzero( value, LAG_SORT );
      
      for( i = 0; i < table_max[ TABLE_FUNCTION ]; ++i )
	order( list, value, i, function_table[i].total_time.time() );
      for( i = 0; i < LAG_SORT; ++i )
	display_lag( ch,
		     function_table[ list[i] ].name,
		     function_table[ list[i] ] );
      
      return;
    }

  
    for( i = 0; i < table_max[ TABLE_FUNCTION ]; ++i ) 
      if( !strncasecmp( function_table[i].name, argument, length ) )
	display_lag( ch,
		     function_table[i].name,
		     function_table[i] );

    return;
  }

  if( is_set( flags, 2 ) ) {
    for( i = 0; i < table_max[ TABLE_COMMAND ]; ++i ) {
      command_table[i].calls = 0;
      command_table[i].total_time.zero( );
      command_table[i].max_time.zero( );
    }
    send( ch, "Command execution time statistics cleared.\n\r" );
    return;
  }

  page_underlined( ch,
		   "%-22s    Calls    Average    Max Time     Total\n\r",
		   "Command" );

  if( is_set( flags, 1 ) ) {
    int list [ LAG_SORT ];
    double value [ LAG_SORT ];
    vzero( list, LAG_SORT );
    vzero( value, LAG_SORT );

    for( i = 0; i < table_max[ TABLE_COMMAND ]; ++i )
      order( list, value, i, command_table[i].total_time.time() );
    for( i = 0; i < LAG_SORT; ++i )
      display_lag( ch,
		   command_table[ list[i] ].name,
		   command_table[ list[i] ] );
    
    return;
  }
  
  for( i = 0; i < table_max[ TABLE_COMMAND ]; ++i ) 
    if( !strncasecmp( command_table[i].name, argument, length ) )
      display_lag( ch,
		   command_table[i].name,
		   command_table[i] );
}


/*
 *   SYSTEM COMMAND
 */


static void display_time( char_data *ch, const char *text, int num, char *c2 = empty_string )
{
  const int total = total_time[ TIME_ACTIVE ].hundred( );

  const double i = (double) 100.0*total_time[num].hundred( )/total;

  const int critical = critical_time[ TIME_ACTIVE ].hundred( );

  double j = 0.0;

  if( critical > 0 ) {
    j = (double) 100.0*critical_time[num].hundred( )/critical;
  }

  send( ch, "%-21s%6.1f%14.1f       %s\n\r", text, i, j, c2 );
}


void do_system( char_data* ch, const char *)
{
  send_centered( ch, "--| System Info |--" );
  send( ch, "\n\r" );

  send( ch, " System Time: %s", (char*) ctime( &current_time ) );
  send( ch, "\r" );
  send( ch, "  Started at: %s", ctime( &boot_time ) );
  send( ch, "\r" );
  send( ch, "Startup took: %d seconds\n\r\n\r", (int)( startup_time ) );

  send_underlined( ch, "Function            Percent      Critical\n\r" );

  char *tmp  = static_string( );
  
  double i = total_time[ TIME_ACTIVE ].hundred( )
           +total_time[ TIME_WAITING ].hundred( );
  i = 100.0*critical_time[ TIME_WAITING ].hundred( )/i;

  snprintf( tmp, THREE_LINES, "        Lag: %5.1f%%", i );

  display_time( ch, "Player Links",     TIME_NETWORK, tmp );

  i = total_time[ TIME_ACTIVE ].hundred( )
           +total_time[ TIME_WAITING ].hundred( );
  i = 100.0*total_time[ TIME_WAITING ].hundred( )/i;

  snprintf( tmp, THREE_LINES, "       Lead: %5.1f%%", i );

  display_time( ch, "  Read Input",     TIME_READ_INPUT, tmp );

  i = total_time[ TIME_ACTIVE ].hundred( )
    +total_time[ TIME_WAITING ].hundred( );

  i = 100.0*total_time[ TIME_ACTIVE ].hundred( )/i;

  snprintf( tmp, THREE_LINES, "  CPU Usage: %5.1f%%", i );

  display_time( ch, "  Command Interp", TIME_COMMANDS, tmp );
  display_time( ch, "  Write Output",   TIME_WRITE_OUTPUT );

  display_time( ch, "Host Daemon",      TIME_DAEMON );

  display_time( ch, "Update Handler",  	TIME_UPDATE );
  display_time( ch, "  Events",        	TIME_EVENT );
  display_time( ch, "  Wait Queue",    	TIME_QUEUE );
  display_time( ch, "  Area Resets",   	TIME_RESET );  
  display_time( ch, "  Rndm Acodes",   	TIME_RNDM_ACODE );
  display_time( ch, "  Time Acodes",   	TIME_TIME_ACODE );
  display_time( ch, "  Auction",       	TIME_AUCTION );
  display_time( ch, "  Weather",       	TIME_WEATHER );
  display_time( ch, "  Cleanup",       	TIME_CLEANUP );
  display_time( ch, "  Maintenance",   	TIME_MAINT );
}


/* 
 *   TIME FUNCTION   
 */


void do_time( char_data* ch, const char *argument )
{
  if( is_mob( ch ) )
    return;

  if( !*argument ) {
    char buf [ TWO_LINES ];
    sprintf_time( buf, current_time, ch->pcdata->pfile->account->timezone );
    send( ch, "\n\rCurrent real-world time: %s.\n\r", buf );
    if( ch->pcdata->pfile->account->timezone != empty_string ) {
      send( ch, "Time zone setting: %s.\n\r", ch->pcdata->pfile->account->timezone );
    } else {
      send( ch, "[ Use \"time -z\" to set your time zone. ]\n\r" );
    }

    if( is_apprentice( ch ) ) {
      weather.set_tick( weather.tick );
      send( ch, "\n\rMUD time: %u:%02u, Day: %s, Date: %s %u, Year: %u.\n\r",
	    weather.hour, weather.minute,
	    day_table[weather.day_of_week].name,
	    month_table[weather.month].name,
	    weather.day_of_month,
	    weather.year );
    }
    return;
  }

  int flags;

  if( !get_flags( ch, argument, &flags, "z", "Time" ) )
    return;

  if( flags == 0
      && !is_apprentice( ch ) ) {
    send( ch, "Usage: time [-z <zone>]\n\r" );
    send( ch, "   See the web site for a list of time zones recognized by TFE.\n\r" );
    return;
  }

  if( !ch->pcdata )
    return;

  account_data *account = ch->pcdata->pfile->account;

  if( !*argument ) {
    send( ch, "Time zone set to system default.\n\r" );
    if( *account->timezone ) {
      send( ch, "[ Prev. Value: %s ]\n\r", account->timezone );
      free_string( account->timezone, MEM_ACCOUNT );
    }
    account->timezone = empty_string;
    return;
  }

  char buf [ MAX_INPUT_LENGTH ];
  argument = one_argument( argument, buf );

  if( strlen( buf ) > 50 ) {
    send( ch, "Time zone must be 50 characters or less.\n\r" );
    return;
  }

  fsend( ch, "Time zone set to %s.", buf );

  if( *account->timezone ) {
    send( ch, "[ Prev. Value: %s ]\n\r", account->timezone );
    free_string( account->timezone, MEM_ACCOUNT );
  }

  account->timezone = alloc_string( buf, MEM_ACCOUNT );
}


/*
 *   TIME ARGUMENTS
 */


int time_arg( const char *& argument, char_data* ch )
{
  if( !strcasecmp( argument, "forever" ) ) 
    return 0;
  
  int i;
  
  if( !number_arg( argument, i ) ) {
    send( ch, "Length of time must be of format <number> <units>.\n\r" );
    return -1;
  }

  if( i < 1 ) {
    send( ch, "Only positive definite time periods are acceptable.\n\r" );
    return -1;
  }
  
  if( !*argument ) {
    send( ch, "Please specify a unit of time.\n\r" );
    return -1;
  }
  
  if( matches( argument, "seconds" ) )  return i;
  if( matches( argument, "minutes" ) )  return 60*i;
  if( matches( argument, "hours" ) )    return 60*60*i;
  if( matches( argument, "days" ) )     return 24*60*60*i;
  if( matches( argument, "years" ) )    return 365*24*60*60*i;
  
  send( ch, "Unknown unit of time.\n\rKnown units are seconds, minutes, hours,\
 days, and years.\n\r" ); 
  
  return -1;
}
