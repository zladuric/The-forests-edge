#include <sys/types.h>
#include <stdio.h>
#include "define.h"
#include "struct.h"


Weather_Data  weather;

action_array weather_actions;


const char *const wind_dirs [ 16 ] = {
  "E", "ENE", "NE", "NNE", "N", "NNW", "NW", "WNW",
  "W", "WSW", "SW", "SSW", "S", "SSE", "SE", "ESE"
};


const index_data beaufort_index [ 13 ] = {
  { "calm",		"",	 0 },
  { "light air", 	"",	 3 },
  { "light breeze", 	"",	 7 },
  { "gentle breeze", 	"",	12 },
  { "moderate breeze",	"",	18 },
  { "fresh breeze",	"",	24 },
  { "strong breeze",	"",	31 },
  { "moderate gale",	"",	38 },
  { "gale",		"",	46 },
  { "strong gale",	"",	54 },
  { "storm",		"",	63 },
  { "violent storm",	"",	73 },	// Adjusted per US statute, beaufort has 72.
  { "hurricane",	"",	-1 }
};


#define OVERCAST 80


const index_data cloud_index [ 5 ] = {
  { "clear",		"",	20 },
  { "scattered clouds",	"",	40 },
  { "partly cloudy",	"",	60 },
  { "mostly cloudy",	"",	OVERCAST },
  { "overcast",		"", 	-1 }
};


class weather_name
{
public:
  const char *const name[ 5 ];
  const int index;
};


//  Wet ->
//
// Warm -|
//       v
const weather_name weather_names[] = {
  { {     "arctic",       "frigid",     "freezing",           "icy",       "gelid" },  30 },
  { {      "bleak",        "nippy",         "cold",        "frosty",        "dank" },  50 },
  { {      "crisp",        "brisk",         "cool",        "clammy",       "soggy" },  67 },
  { {   "very dry",          "dry",         "mild",         "humid",  "very humid" },  77 },
  { {       "arid",        "balmy",         "warm",         "muggy",      "sultry" },  85 },
  { {  "scorching",     "roasting",          "hot",    "sweltering",     "boiling" }, 105 },
  { {    "parched",     "sizzling",     "very hot",        "steamy",  "oppressive" },  -1 }
};

const int humid_level[] = {
  20,
  40,
  60,
  80,
  -1
};

static const int moon_slip = 49;

const char *const moon_name [ 8 ] = {
  "full", "waning gibbous", "waning half", "waning crescent",
  "new", "waxing crescent", "waxing half", "waxing gibbous"
};


static const int moon_light [ 8 ] = {
 3, 2, 1, 0, 0, 0, 1, 2
};


static const int hottest_day = 200;
static const int humidest_day = 200;


static double avg_daily( int day, int high_point )
{
  const int diff = ( day % weather.days_per_year ) - high_point;

  return cos( 2.0*M_PI*diff/weather.days_per_year );
}


unsigned Weather_Data::sun( ) const
{
  if( hour < 5 || hour >= 20 )
    return 0;
  if( hour <= 11 )
    return 4*hour - 16;
  if( hour == 12 )
    return 30;
  return 80 - 4*hour;
}


void Weather_Data::set_moon( )
{
  moon_var = 6*60 + int( 2.0*60.0*sin( 2.0*M_PI*double(moon_mid)/39343.0 ) );
  moon_phase = ( ( moon_mid + 90 ) % ( 24*60 ) ) / ( 3*60 );
}


void Weather_Data::set_tick( unsigned long t )
{
  tick = t;

  /*
    	Where's the moon?
  */
  moon_mid = t - t % ( 24*60 + moon_slip ) + 8*60;
  set_moon( );

  /*
    	Set time
  */
  minute = t % 60;
  t /= 60;
  hour = t % 24;
  t /= 24;

  /*
    	Set calendar
  */
  day = t;
  day_of_week = t % table_max[TABLE_DAYS];

  days_per_year = 0;
  for( month = 0; (int)month < table_max[TABLE_MONTHS]; ++month ) {
    days_per_year += month_table[month].days;
  }

  day_of_month = t % days_per_year + 1;
  t /= days_per_year;

  for( month = 0; (int)month < table_max[TABLE_MONTHS]; ++month ) {
    if( (int)day_of_month > month_table[month].days ) {
      day_of_month -= month_table[month].days;
    } else {
      break;
    }
  }

  year = t + START_YEAR;

  sunlight = sun( );

  if( tick + moon_var >= moon_mid
      && tick < moon_mid + moon_var ) {
    moonlight = moon_light[ moon_phase ];
  } else {
    moonlight = 0;
  }
}


void Weather_Data::init( )
{
  avg_day_temp = avg_daily( day, hottest_day );
  avg_day_temp_delta = ( avg_daily( day+1, hottest_day ) - avg_day_temp ) / ( 24*60 );
  avg_day_temp += ( minute + hour*60 ) * avg_day_temp_delta;
  avg_day_humid = avg_daily( day, humidest_day );
  avg_day_humid_delta = ( avg_daily( day+1, humidest_day ) - avg_day_humid ) / ( 24*60 );
  avg_day_humid += ( minute + hour*60 ) * avg_day_humid_delta;
  intraday_temp = 0.25 * ( 1 - cos( 2.0*M_PI*( tick-14*60 )/(24.0*60.0) ) );

  for( area_data *area = area_list; area; area = area->next ) {
    area->set_weather( );
  }

  for( int i = 0; i < WEATHER_FRONTS; ++i ) {
    front[i].init( weather.tick, i );
  }
}


static int value_check( int val )
{
  return ( val == 0 )
    ? 0
    : ( ( val < 0 )
	? -1
	: 1 );
}

void Weather_Data::update( )
{
  struct timeval start;
  gettimeofday( &start, 0 );

  const int old_light = sunlight + moonlight;

  char tmp [ THREE_LINES ];
  
  ++tick;

  bool check = false;
  int trig = 0;
  int val = 0;

  const char *w1 = 0;
  const char *w2 = 0;
  const char *w3 = 0;
  const char *w4 = 0;

  if( tick + moon_var == moon_mid ) {
    moonlight = moon_light[ moon_phase ];
    if( moon_phase != 4
	&& moonlight >= sunlight ) {
      snprintf( tmp, THREE_LINES, "The %s moon rises above the horizon.", moon_name[ moon_phase ] );
      w1 = tmp;
      w2 = "A huge, pale sugar cookie peeks over the horizon and winks at you.";
    }
    check = true;
    trig = TRIGGER_MOON;
    val = 1;
  } else if ( tick == moon_mid + moon_var ) {
    if( moon_phase != 4
	&& sunlight == 0 ) {
      w1 = "The moon disappears below the horizon.";
      w2 = "A huge, pale sugar cookie disappears below the horizon.";
    }
    moonlight = 0;
    check = true;
    trig = TRIGGER_MOON;
    val = -1;
    moon_mid += 24*60 + moon_slip;
    set_moon( );
  } else if( tick == moon_mid ) {
    check = true;
    trig = TRIGGER_MOON;
    val = 0;
  }

  avg_day_temp += avg_day_temp_delta;
  avg_day_humid += avg_day_humid_delta;
  intraday_temp =  0.25 * ( 1 - cos( 2.0*M_PI*( tick-14*60 )/(24.0*60.0) ) );

  if( ++minute > 59 ) {
    ++hour;
    minute = 0;
    
    save_world( );

    switch ( hour ) {
    case 5:
      w3 = "The day has begun.";
      w4 = "The colors around you suddenly intensify.";
      break;

    case 6:
      w3 = "The sun rises in the east.";
      w4 = "A giant, glowing fruit loop ascends the eastern sky.";
      check = true;
      trig = TRIGGER_SUN;
      val = 1;
      break;

    case 12:
      w3 = "The midday sun shines overhead.";
      w4 = "A giant, glowing fruit loop dances directly overhead.";
      check = true;
      trig = TRIGGER_SUN;
      val = 0;
      break;

    case 19:
      w3 = "The sun slowly disappears in the west.";
      w4 = "A giant, glowing fruit loop disappears in the west.";
      check = true;
      trig = TRIGGER_SUN;
      val = -1;
      break;

    case 20:
      w3 = "The night has begun.";
      w4 = "The colors around you suddenly fade a bit.";
      break;

    case 24:
      hour = 0;
      ++day;
      ++day_of_month;
      day_of_week = ( day_of_week + 1 ) % table_max[TABLE_DAYS];
      avg_day_temp = avg_daily( day, hottest_day );
      avg_day_temp_delta = ( avg_daily( day+1, hottest_day ) - avg_day_temp ) / ( 24*60 );
      avg_day_humid = avg_daily( day, humidest_day );
      avg_day_humid_delta = ( avg_daily( day+1, humidest_day ) - avg_day_humid ) / ( 24*60 );
      break;
    }
  }

  if( (int)day_of_month > month_table[month].days ) {
    day_of_month = 1;
    if( (int)++month >= table_max[TABLE_MONTHS] ) {
      month = 0;
      ++year;
    }
  }
  
  sunlight = sun( );
  
  for( area_data *area = area_list; area; area = area->next ) {
    area->set_weather( );
    if( old_light == 0 && sunlight+moonlight > 0 ) {
      // This sucks... go through ALL rooms, restarting combat.
      for( room_data *room = area->room_first; room; room = room->next ) {
	if( room->Light( 1 ) <= 0
	    && room->Light( ) > 0 ) {
	  update_aggression( room );
	}
      }
    }
  }

  for( int i = 0; i < WEATHER_FRONTS; ++i ) {
    front[i].advance( tick, i );
  }

  const double s_grad = front[DIR_SOUTH].gradient - front[DIR_NORTH].gradient;
  const double w_grad = front[DIR_WEST].gradient - front[DIR_EAST].gradient;

  const double magnitude = sqrt( s_grad * s_grad + w_grad * w_grad );  // Max is sqrt( 8 ) = 2 * sqrt( 2 ).
  wind_speed = magnitude * 85.0 / ( 2.0 * M_SQRT2 );
  //  wind_cat = (int) wind_speed;	// Do not round.

  wind_angle = atan( s_grad / w_grad );

  if( w_grad > 0 ) {
    wind_angle += M_PI;
  } else if( wind_angle < 0.0 ) {
    wind_angle += 2*M_PI;
  }

  //  wind_dir = ((int)( ( wind_angle * 8.0 / M_PI ) + 0.5 ) ) % 16;

  if( w1 || w3 ) {
    for( const link_data *link = link_list; link; link = link->next ) {
      char_data *rch = link->character;
      if( link->connected == CON_PLAYING
	  && !rch->in_room->is_indoors( )
	  && !is_set( rch->in_room->room_flags, RFLAG_UNDERGROUND )
	  && !is_submerged( 0, rch->in_room )
	  && rch->Can_See( )
	  && is_set( rch->pcdata->message, MSG_WEATHER ) ) {
	if( rch->is_affected( AFF_HALLUCINATE ) ) {
	  if( w2 )
	    send_color( rch, COLOR_WEATHER, w2 );
	  if( w4 )
	    send_color( rch, COLOR_WEATHER, w4 );
	} else {
	  if( w1 )
	    send_color( rch, COLOR_WEATHER, w1 );
	  if( w3 )
	    send_color( rch, COLOR_WEATHER, w3 );
	}
	send( rch, "\n\r" );
      }
    }
  }

  if( check ) {
    for( int i = 0; i < weather_actions; ++i ) {
      action_data *action = weather_actions[i];
      room_data *room = action->room;
      if( action->trigger == trig
	  && value_check( action->value ) == val ) {
	clear_variables( );
	var_room = room;
	action->execute( );
      }
    }
  }

  pulse_time[ TIME_WEATHER ] = stop_clock( start );
}


const char *weather_word( double temperature, double humidity )
{
  int h;
  for( h = 0; humid_level[h] < humidity && humid_level[h] != -1; ++h );

  int i;
  for( i = 0; weather_names[i].index < temperature && weather_names[i].index != -1; ++i );

  return weather_names[i].name[h];
}


const char *cloud_word( double clouds )
{
  return lookup( cloud_index, (int)clouds );
}


const char *wind_word( double speed, double angle )
{
  const int cat = (int) speed;	// Do not round.

  const char *beau = lookup( beaufort_index, cat );

  // Angle doesn't matter if it's calm.
  if( cat == 0 )
    return beau;

  const int dir = ((int)( ( angle * 8.0 / M_PI ) + 0.5 ) ) % 16;

  char *tmp = static_string( );

  snprintf( tmp, THREE_LINES, "%s from %s", beau, wind_dirs[dir] );

  return tmp;
}


void weather_front::init( unsigned long tick, unsigned number )
{
  if( progress < 0.0 ) {
    // Just loaded from disk; init areas.
    for( area_data *area = area_list; area; area = area->next ) {
      // Humidity.
      // Approximates that area->humidity was the same at start.
      int left = int( 10.0*( 50.0 - area->humidity ) + 0.5 ) - 500;
      int search = norm_search( humidity_prev+WEATHER_H_DIST, 0, left, 2*WEATHER_H_DIST+1, WEATHER_H_STDDEV );
      //      double search = ((double)norm2000_search( humidity_prev+WEATHER_H_DIST, left, 2*WEATHER_H_DIST ));
      area->h_prev[number] = ((double)search) / 10.0;
      search = norm_search( humidity+WEATHER_H_DIST, 0, left, 2*WEATHER_H_DIST+1, WEATHER_H_STDDEV );
      //      search = ((double)norm2000_search( humidity+WEATHER_H_DIST, left, 2*WEATHER_H_DIST ));
      area->h_goal[number] = ((double)search) / 10.0;

      // Clouds.
      search = norm_search( clouds_prev+WEATHER_H_DIST, 0, left, 2*WEATHER_H_DIST+1, WEATHER_H_STDDEV );
      //      search = ((double)norm2000_search( clouds_prev+WEATHER_H_DIST, left, 2*WEATHER_H_DIST ));
      area->c_prev[number] = ((double)search) / 10.0;
      search = norm_search( clouds+WEATHER_H_DIST, 0, left, 2*WEATHER_H_DIST+1, WEATHER_H_STDDEV );
      //      search = ((double)norm2000_search( clouds+WEATHER_H_DIST, left, 2*WEATHER_H_DIST ));
      area->c_goal[number] = ((double)search) / 10.0;

      // Temperature.
      search = norm_search( temperature_prev+WEATHER_T_DIST, 0, -WEATHER_T_DIST, 2*WEATHER_T_DIST+1, WEATHER_T_STDDEV );
      //      search = ((double)norm2000_search( temperature_prev+WEATHER_T_DIST, left, 2*WEATHER_T_DIST ));
      area->t_prev[number] = ((double)search - WEATHER_T_DIST) / 10.0;
      search = norm_search( temperature+WEATHER_T_DIST, 0, -WEATHER_T_DIST, 2*WEATHER_T_DIST+1, WEATHER_T_STDDEV );
      //      search = ((double)norm2000_search( temperature+WEATHER_T_DIST, left, 2*WEATHER_T_DIST ));
      area->t_goal[number] = ((double)search - WEATHER_T_DIST) / 10.0;
    }
    progress = 0.0;
    advance( tick, number );
    return;
  }

  humidity_prev = humidity;
  humidity = number_range( 0, 2*WEATHER_H_DIST ) - WEATHER_H_DIST;

  temperature_prev = temperature;
  temperature = number_range( 0, 2*WEATHER_T_DIST ) - WEATHER_T_DIST;
  
  clouds_prev = clouds;
  clouds = number_range( 0, 2*WEATHER_H_DIST ) - WEATHER_H_DIST;

  for( area_data *area = area_list; area; area = area->next ) {
    area->h_prev[number] = area->h_goal[number];
    area->c_prev[number] = area->c_goal[number];
    area->t_prev[number] = area->t_goal[number];
    // Humidity.
    int left = int( 10.0*( 50.0 - area->humidity ) + 0.5 ) - 500;;
    int search = norm_search( humidity+WEATHER_H_DIST, 0, left, 2*WEATHER_H_DIST+1, WEATHER_H_STDDEV );
    //    double search = ((double)norm2000_search( humidity+WEATHER_H_DIST, left, 2*WEATHER_H_DIST ));
    area->h_goal[number] = ((double)search) / 10.0;
    // Clouds.
    search = norm_search( clouds+WEATHER_H_DIST, 0, left, 2*WEATHER_H_DIST+1, WEATHER_H_STDDEV );
    //    search = ((double)norm2000_search( clouds+WEATHER_H_DIST, left, 2*WEATHER_H_DIST ));
    area->c_goal[number] = ((double)search) / 10.0;
    // Temperature.
    //    left = 1000-WEATHER_T_DIST;
    search = norm_search( temperature+WEATHER_T_DIST, 0, -WEATHER_T_DIST, 2*WEATHER_T_DIST+1, WEATHER_T_STDDEV );
    //    search = ((double)norm2000_search( temperature+WEATHER_T_DIST, left, 2*WEATHER_T_DIST ));
    area->t_goal[number] = ((double)search - WEATHER_T_DIST) / 10.0;
  }
  
  // 12 hours to 84 hours.
  duration = 60*12 + number_range( 0, 60*72 );
  start = tick;
  progress = 0.0;
  gradient = 0.0;
}


void weather_front::advance( unsigned long tick, unsigned number )
{
  if( tick >= start+duration )
    init( tick, number );
  
  progress = ( 1 - cos( M_PI * ( tick - start ) / duration ) ) / 2;
  gradient = sin( M_PI * ( tick - start ) / duration );
}


void do_weather( char_data* ch, const char *argument )
{
  room_data *room = ch->in_room;
  area_data *area = room->area;

  send( ch, "\n\r" );

  send( ch, "         Time: %u:%02u\n\r",
	weather.hour, weather.minute );

  send( ch, "         Date: %s %u\n\r",
	month_table[weather.month].name,
	weather.day_of_month );

  send( ch, "\n\r" );

  send( ch, "     Sunlight: %u\n\r", room->sunlight( ) );
  send( ch, "    Moonlight: %u\n\r", room->moonlight( ) );

  send( ch, "\n\r" );

  double temp = area->temperature;
  if( !is_set( room->room_flags, RFLAG_INDOORS )
      && !is_submerged( 0, room ) ) {
    temp -= ( 100.0 - area->humidity ) * weather.intraday_temp;
  }

  send( ch, "  Temperature: %.1fF  (area = %.1fF, intraday = %.1F)\n\r",
	room->temperature( ),
	area->temperature,
	temp );
  send( ch, "     Humidity: %.1f%%  (area = %.1f%%)\n\r",
	room->humidity( ),
	room->area->humidity );
  send( ch, "         Wind: %.1f MPH, %.1f degrees\n\r",
	room->wind_speed( ), room->wind_angle( ) * 180 / M_PI );
  send( ch, "       Clouds: %.1f%%\n\r", room->clouds( ) );

  send( ch, "\n\r" );

  send( ch, "     Gust test = %.3f/%.3f\n\r", 
	fabs( weather.front[DIR_SOUTH].gradient + weather.front[DIR_NORTH].gradient ),
	fabs( weather.front[DIR_WEST].gradient + weather.front[DIR_EAST].gradient )
	);

  send( ch, "\n\r" );
  send_underlined( ch, "Fronts\n\r" );

  for( int i = 0; i < WEATHER_FRONTS; ++i ) {
    send( ch, "%3d   t = %+4d (%6.1fF)   h = %+4d (%6.1f%%)   c = %+4d (%6.1f%%)",
	  i,
	  weather.front[i].temperature,
	  area->temperature + area->t_goal[i],
	  weather.front[i].humidity,
	  area->h_goal[i],
	  weather.front[i].clouds,
	  area->c_goal[i] );
    send( ch, "   p = %d/%d   g = %.3f\n\r",
	  weather.tick - weather.front[i].start,
	  weather.front[i].duration,
	  weather.front[i].gradient );
  }
}


void area_data :: set_weather( )
{
  const Climate_Data& clime = climate_table[ climate ];

  humidity = weather.avg_day_humid
    * abs( clime.humid_summer - clime.humid_winter ) / 2.0
    + ( clime.humid_summer + clime.humid_winter ) / 2.0;

  temperature = weather.avg_day_temp
    * abs( clime.temp_summer - clime.temp_winter ) / 2.0
    + ( clime.temp_summer + clime.temp_winter ) / 2.0;
}


const char *room_data :: sky_state( ) const
{
  if( is_indoors( ) )
    return "???";

  const unsigned hour = weather.hour;

  if( hour < 5 || hour >= 20 ) 
    return "Night";

  if( hour < 6 )
    return "Dawn";		// 5:00 AM - 5:59 AM (1 hour)

  if( hour < 19 ) {
    if( clouds( ) > OVERCAST ) {
      return "Day";
    }
    if( hour < 8 )
      return "Early Morning";	// 6:00 AM - 7:59 AM (2 hours)
    if( hour < 10 )
      return "Mid-Morning";	// 8:00 AM - 9:59 AM (2 hours)
    if( hour < 11 )
      return "Late Morning";	// 10:00 AM - 10:59 AM (1 hour)
    if( hour < 13 )
      return "Near Noon";	// 11:00 AM - 12:59 PM (2 hours)
    if( hour < 16 )
      return "Afternoon";	// 1:00 PM - 3:59 PM (3 hours)
    if( hour < 17 )
      return "Late Afternoon";	// 4:00 PM - 4:59 PM (1 hour)
    return "Evening";		// 5:00 PM - 6:59 PM (2 hours)
  }

  return "Dusk";		// 7:00 PM - 7:59 PM (1 hour)
}


const char *room_data :: moon_state( ) const
{
  if( is_indoors( ) || clouds( ) > OVERCAST )
    return "???";

  if( weather.tick + weather.moon_var < weather.moon_mid
      || weather.tick >= weather.moon_mid + weather.moon_var
      || weather.moon_phase == 4 ) {
    // Moon is new or not up.
    return "None";
  }

  return moon_name[ weather.moon_phase ];
}


unsigned room_data :: sunlight( ) const
{
  if( is_indoors( )
      || is_set( room_flags, RFLAG_UNDERGROUND ) )
    return 0;

  unsigned val = weather.sunlight;

  // Clouds can remove up to 50% of sunlight.
  const double c = (int) clouds( );
  if( val > 0 && (int)c > 0 ) {
    val = (unsigned)( val * ( 1.0 - c / 200.0 ) );
  }

  return val;
}


unsigned room_data :: moonlight( ) const
{
  if( is_indoors( )
      || is_set( room_flags, RFLAG_UNDERGROUND ) )
    return 0;

  unsigned val = weather.moonlight;

  // Clouds can remove all of moonlight.
  const double c = (int) clouds( );
  if( val > 0 && (int)c > 0 ) {
    val = (unsigned)( val * ( 1.0 - c / 100.0 ) );
  }

  return val;
}


double room_data :: humidity( int n ) const
{
  double hum = 0.0;
  for( int i = 0; i < WEATHER_FRONTS; ++i ) {
    hum += ( area->h_prev[i]
	     + ( area->h_goal[i] - area->h_prev[i] ) * weather.front[i].progress );
  }
  hum /= WEATHER_FRONTS;

  unsigned count = 1;

  if( n < 0 ) {
    for( int i = 0; i < exits; ++i ) {
      exit_data *exit = exits[i];
      hum += exit->to_room->humidity( 0 );
      ++count;
    }
  }

  hum /= count;

  return hum;
}


double room_data :: clouds( int n ) const
{
  double c = 0.0;
  for( int i = 0; i < WEATHER_FRONTS; ++i ) {
    c += ( area->c_prev[i]
	   + ( area->c_goal[i] - area->c_prev[i] ) * weather.front[i].progress );
  }
  c /= WEATHER_FRONTS;

  unsigned count = 1;

  if( n < 0 ) {
    for( int i = 0; i < exits; ++i ) {
      exit_data *exit = exits[i];
      c += exit->to_room->clouds( 0 );
      ++count;
    }
  }

  c /= count;

  return c;
}


double room_data :: temperature( int n ) const
{
  double temp = area->temperature;
  for( int i = 0; i < WEATHER_FRONTS; ++i ) {
    temp += ( area->t_prev[i]
	     + ( area->t_goal[i] - area->t_prev[i] ) * weather.front[i].progress ) / WEATHER_FRONTS;
  }

  unsigned count = 1;

  if( !is_set( room_flags, RFLAG_INDOORS )
      && !is_submerged( 0, this ) ) {
    temp -= ( 100.0 - humidity( ) ) * weather.intraday_temp;
  }

  if( n < 0 ) {
    for( int i = 0; i < exits; ++i ) {
      exit_data *exit = exits[i];
      temp += exit->to_room->temperature( 0 );
      ++count;
    }
  }

  temp /= count;

  /*
  const int c = (int) clouds( );

  if( c > 0 ) {
    // Cloud cover.
  }
  */

  /*
  const int wind = (int) wind_speed( );

  if( wind > 0 ) {
    // Wind chill factor.

  }
  */

  return temp;
}


double room_data :: wind_speed( ) const
{
  return weather.wind_speed * terrain_table[ sector_type ].wind / 100.0;
}


double room_data :: wind_angle( ) const
{
  return weather.wind_angle;
}
