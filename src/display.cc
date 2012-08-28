#include <ctype.h>
#include <sys/types.h>
#include <stdio.h>
#include <syslog.h>
#include "define.h"
#include "struct.h"


/*
 *   TITLE FUNCTIONS
 */


void send_title( char_data* ch, const char* text )
{
  char     tmp1  [ FOUR_LINES ];
  char     tmp2  [ FOUR_LINES ];
  char*  letter;
  int    length  = strlen( text );
 
  if( !ch->link )
    return;

  const int width = /*ch->pcdata ? ch->pcdata->columns :*/ 80;

  if( length > width ) {
    roach( "Send_Title: Length of text > screen width (%d).", width );
    roach( "-- Char = %s", ch );
    roach( "-- Text = %s", text );
    return;
  }

  snprintf( tmp2, FOUR_LINES, "%*s", (width-length)/2, "" );
  snprintf( tmp1, FOUR_LINES, "%s", text );

  *tmp1 = toupper( *tmp1 );
  for( letter = tmp1; *letter; )
    if( *letter++ == ' ' )
      *letter = toupper( *letter );

  send( ch, tmp2 );
  send_color( ch, COLOR_TITLES, tmp1 );
  send( ch, "\n\r" );
  
  for( letter = tmp1; *letter; ++letter )
     if( *letter != ' ' )
       *letter = '-';

  send( ch, tmp2 );
  send_color( ch, COLOR_TITLES, tmp1 );
  send( ch, "\n\r" );
}


void page_title( char_data* ch, const char* text )
{
  char     tmp1  [ FOUR_LINES ];
  char     tmp2  [ FOUR_LINES ];
  char*  letter;
  int    length  = strlen( text );

  if( !ch->link )
    return;

  const int width = /*ch->pcdata ? ch->pcdata->columns :*/ 80;

  if( length > width ) {
    roach( "Page_Title: Length of text > screen width (%d).", width );
    roach( "-- Char = %s", ch );
    roach( "-- Text = %s", text );
    return;
  }

  snprintf( tmp2, FOUR_LINES, "%*s", (width-length)/2, "" );
  snprintf( tmp1, FOUR_LINES, "%s", text );

  *tmp1 = toupper( *tmp1 );
  for( letter = tmp1; *letter; )
    if( *letter++ == ' ' )
      *letter = toupper( *letter );

  page( ch, tmp2 );
  page_color( ch, COLOR_TITLES, tmp1 );
  page( ch, "\n\r" );
  
  for( letter = tmp1; *letter; ++letter )
    if( *letter != ' ' )
      *letter = '-';

  page( ch, tmp2 );
  page_color( ch, COLOR_TITLES, tmp1 );
  page( ch, "\n\r" );
}


void display_array( char_data *ch, const char *title,
		    const char *const *entry1,
		    const char *const *entry2,
		    int max, bool sort, bool (*func)(const char_data*, int) )
{
  if( !ch->pcdata )
    return;

  const unsigned columns = ch->pcdata->columns / 19;

  if( max < 0 ) {
    // Count entries.
    for( max = 0; **(entry1+max*(entry2-entry1)); ++max );
    if( max == 0 )
      return;
  }

  int sorted[ max ];
  max = sort_names( entry1, entry2, sorted, max, sort );

  //  char tmp [ TWO_LINES ];

  if( title && *title ) {
    if( *title == '+' ) {
      page_title( ch, title+1 );
    } else {
      //      snprintf( tmp, TWO_LINES, "%s:\n\r", title );
      page( ch, "%s:\n\r", title );
    }
  }

  int j = 0;

  for( int i = 0; i < max; ++i ) {
    if( !func || func( ch, sorted[i] ) ) {
      char **string = (char**) (int)( entry1+sorted[i]*(entry2-entry1) );
      /*
      snprintf( tmp, TWO_LINES, "%19s%s",
		*string,
		j%columns == columns-1 ? "\n\r" : "" );
      */
      page( ch, "%19s%s",
	    *string,
	    j%columns == columns-1 ? "\n\r" : "" );
      ++j;
    }
  }

  if( j%columns != 0 )
    page( ch, "\n\r" );
}


/*
 *   WORD LIST
 */


const char* word_list( const char** list, int max, bool use_and )
{
  if( max == 0 )
    return empty_string;

  char *tmp = static_string( list[0] );

  if( max > 1 ) {
    for( int i = 1; i < max; i++ ) {
      sprintf( tmp+strlen( tmp ), ", %s%s",
	       use_and && i+1 == max ? "and " : "", list[i] );
    }
  }

  return tmp;
}
