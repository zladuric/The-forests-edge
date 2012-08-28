#ifndef tfe_weather_h
#define tfe_weather_h

#include <sys/time.h>


/*
 *   TIME ROUTINES
 */


class Time_Data : public timeval
{
public:
  Time_Data( )
  {
    zero( );
  }

  Time_Data( const Time_Data &t )
  {
    tv_sec = t.tv_sec;
    tv_usec = t.tv_usec;
  }
  
  Time_Data( const struct timeval &t )
  {
    tv_sec = t.tv_sec;
    tv_usec = t.tv_usec;
  }
  
  operator bool( ) const
  {
    return tv_sec != 0 || tv_usec != 0;
  }

  void zero( )
  {
    tv_sec = 0;
    tv_usec = 0;
  }

  void operator+=( const time_data& t )
  {
    tv_sec += t.tv_sec;
    tv_usec += t.tv_usec;
    
    tv_sec += tv_usec/1000000;
    tv_usec %= 1000000;
  }
  
  void operator-=( const time_data& t )
  {
    if( t.tv_sec > tv_sec ) {
      tv_sec = tv_usec = 0;
    } else if( t.tv_sec == tv_sec ) {
      if( t.tv_usec > tv_usec ) {
	tv_sec = tv_usec = 0;
      } else {
	tv_sec = 0;
	tv_usec -= t.tv_usec;
      }
    } else {
      tv_sec -= t.tv_sec;
      if( t.tv_usec > tv_usec ) {
	tv_usec += 1000000 - t.tv_usec;
	--tv_sec;
      } else {
	tv_usec -= t.tv_usec;
      }
    }
  }

  void operator=( const time_data& t )
  {
    tv_sec = t.tv_sec;
    tv_usec = t.tv_usec;
  }
  
  bool operator<( const time_data& t ) const
  {
    return( tv_sec == t.tv_sec
	    ? tv_usec < t.tv_usec
	    : tv_sec < t.tv_sec );
  }    
  
  bool operator<=( const time_data& t ) const
  {
    return( tv_sec == t.tv_sec
	    ? tv_usec <= t.tv_usec
	    : tv_sec <= t.tv_sec );
  }
  
  bool operator>( const time_data& t ) const
  {
    return( tv_sec == t.tv_sec
	    ? tv_usec > t.tv_usec
	    : tv_sec > t.tv_sec );
  }    
  
  bool operator>=( const time_data& t ) const
  {
    return( tv_sec == t.tv_sec
	    ? tv_usec >= t.tv_usec
	    : tv_sec >= t.tv_sec );
  }
  
  long long time( ) const {
    return (long long) tv_sec * 1000000 + tv_usec;
  } 
  
  int hundred( ) const {
    return 100*tv_sec+tv_usec/10000;
  }
};


#define TIME_ACTIVE         0
#define TIME_WAITING        1
#define TIME_COMMANDS       2
#define TIME_NETWORK        3
#define TIME_READ_INPUT     4
#define TIME_WRITE_OUTPUT   5
#define TIME_DAEMON         6
#define TIME_UPDATE         7
#define TIME_EVENT          8
#define TIME_RESET          9
#define TIME_RNDM_ACODE    10
#define TIME_WEATHER       11
#define TIME_TIME_ACODE    12
#define TIME_AUCTION       13
#define TIME_QUEUE         14
#define TIME_CLEANUP       15
#define TIME_MAINT         16
#define MAX_TIME           17


extern time_t      boot_time;
extern time_t      current_time;
extern time_t      startup_time;
extern unsigned    time_history   [ 10 ];
extern long long   time_total     [ 10 ];
extern time_data   pulse_time     [ MAX_TIME ];
extern time_data   total_time     [ MAX_TIME ];
extern time_data   critical_time  [ MAX_TIME ];


void               sprintf_minutes  ( char*, const time_t );
void               sprintf_date     ( char*, const time_t );
void               sprintf_time     ( char*, const time_t, const char* );
char              *ltime            ( const time_t&, bool, const char_data *ch );
char              *ltime            ( const time_t&, bool = false, const account_data* = 0 );
const time_data&   stop_clock       ( const struct timeval& );
int                time_arg         ( const char *&, char_data* );

inline int weeks ( int sec ) { return 7*24*60*60*sec; }
inline int days  ( int sec ) { return 24*60*60*sec; }

const char *weather_word( double, double );
const char *wind_word( double, double );
const char *cloud_word( double );


/* 
 *   WEATHER ROUTINES
 */


#define WEATHER_FRONTS		  4

#define WEATHER_H_DIST		500
#define WEATHER_H_STDDEV	320

#define WEATHER_T_DIST		200
#define WEATHER_T_STDDEV	320


class weather_front
{
public:
  weather_front( )
    : progress( -1.0 )
  { }

  void init( unsigned long tick, unsigned number );
  void advance( unsigned long tick, unsigned number );

  unsigned long start;
  unsigned long duration;
  double progress;
  double gradient;
  int humidity_prev;
  int humidity;
  int temperature_prev;
  int temperature;
  int clouds_prev;
  int clouds;
};


class Weather_Data
{
public:
  unsigned long tick;

  unsigned days_per_year;

  unsigned minute;
  unsigned hour;
  unsigned day_of_week;
  unsigned day_of_month;
  unsigned day;
  unsigned month;
  unsigned year;

  unsigned sunlight;
  unsigned moonlight;

  unsigned moon_mid;
  unsigned moon_var;
  int moon_phase;

  double avg_day_temp;
  double avg_day_temp_delta;
  double avg_day_humid;
  double avg_day_humid_delta;
  double intraday_temp;

  weather_front front [ WEATHER_FRONTS ];

  double wind_speed;
  double wind_angle;

  void set_tick( unsigned long t );
  void init( );
  void update( );

  bool is_day ( ) const
  { return( hour >= 5 && hour < 20 ); }

private:
  unsigned sun( ) const;
  void set_moon( );
};


extern Weather_Data weather;
extern action_array weather_actions;


/*
 *    CLIMATE
 */

#define CLIMATE_NONE             0


#endif // tfe_weather_h
