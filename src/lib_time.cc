#include <ctype.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include "define.h"
#include "struct.h"


const void *code_time_hour( const void **argument )
{
  return (void*) weather.hour;
}


const void *code_time_minute( const void **argument )
{
  return (void*) weather.minute;
}


const void *code_date_year( const void **argument )
{
  return (void*) weather.year;
}


const void *code_date_first_year( const void **argument )
{
  return (void*) int( START_YEAR );
}


const void *code_date_month( const void **argument )
{
  return (void*) weather.month;
}


const void *code_date_day( const void **argument )
{
  return (void*) weather.day_of_month;
}


const void *code_days_in_month( const void **argument )
{
  const int month = (unsigned) argument[0];

  if( month < 0 || month >= table_max[TABLE_MONTHS] )
    return 0;

  return (void*) month_table[month].days;
}


const void *code_date_weekday( const void **argument )
{
  return (void*) weather.day_of_week;
}


const void *code_moon_phase( const void **argument )
{
  if( weather.tick + weather.moon_var < weather.moon_mid
      || weather.tick >= weather.moon_mid + weather.moon_var )
    return (void*) -1;

  return (void*) weather.moon_phase;
}
