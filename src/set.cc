#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "define.h"
#include "struct.h"


/*
 *   SET STRINGS
 */


void set_string( char_data* ch, const char *argument, const char*& string,
  const char* subject, int mem_type )
{
  if( !*argument ) {
    send( ch, "Set %s to what?\n\r[ Current Value: %s ]\n\r",
	  subject, string );
    return;
  }  

  send( ch, "%s set.\n\r[ New:  %s ]\n\r[ Prev: %s ]\n\r",
	subject, argument, string );
  free_string( string, mem_type );
  string = alloc_string( argument, mem_type );
}


const char* string_field :: set( char_data* ch, const char* subject,
				 const char *argument )
{
  static char buf [ FOUR_LINES ];

  if( !*argument ) {
    send( ch, "Set %s of %s to?\n\r[ Current value: %s ]\n\r",
	  name, subject, *value );
    return empty_string;
  }

  char tmp [ MAX_STRING_LENGTH ];
  snprintf( tmp, MAX_STRING_LENGTH, "%s", *value );

  if( !func ) {
    if( !strcasecmp( argument, "blank" ) ) {
      send( ch, "%s on %s set blank.\n\r[ Prev: %s ]\n\r",
	    name, subject, *value );
      free_string( *value, mem_type );
      *value = empty_string;
      snprintf( buf, FOUR_LINES, "%s set blank.", name );
      return buf;
    }

  } else if( !( *func )( ch, argument, *value ) ) {
    return empty_string;
  }
  
  send( ch, "%s on %s set.\n\r[ New:  %s ]\n\r[ Prev: %s ]\n\r",
	name, subject, argument, tmp );
  free_string( *value, mem_type );
  *value = alloc_string( argument, mem_type );
  snprintf( buf, FOUR_LINES, "%s set to %s.", name, argument );

  return buf;
}


/*
 *   SET TYPES
 */


/*
void set_type( char_data* ch, const char *argument, int& value,
	       const char* field, const char* subject, int max,
	       const char** word1, const char** word2 )
{
  int i;

  if( !word2 )
     word2 = word1+1;

  if( !*argument ) {
    if( !ch->pcdata )
      return;

    const unsigned width = ch->pcdata->columns;

    int val = value;
    if( val < 0 || val >= max )
      val = 0;

    page( ch, "%s Options:\n\r", field );

    for( i = 0; i < max; ++i ) {
      page( ch, "%20s%s", element( i, word1, word2 ),
	    i%3 == 2 ? "\n\r" : "" );
    }

    page( ch, "\n\r%s[ Current Value: %s ]\n\r", i%3 != 0 ? "\n\r" : "",
	  element( val, word1, word2 ) );
    return;
  }

  for( i = 0; i < max; i++ ) {
    if( fmatches( element( i, word1, word2 ), argument ) ) {
      value = i;
      send( ch, "%s on %s set to %s.\n\r", field, subject,
	    element( i, word1, word2 ) );
      return;
    }
  }

  send( ch, "Unknown %s.\n\r", subject );
}
*/


const char* type_field :: set( char_data* ch, const char* subject,
			       const char *argument )
{
  static char  buf  [ TWO_LINES ];
  //  char         tmp  [ TWO_LINES ];
  int            i;

  if( !*argument ) {
    if( !ch->pcdata )
      return empty_string;

    display_array( ch, name,
		   first, second,
		   max, sort );

    page( ch, "\n\r[ Current Value: %s ]\n\r", element( *value ) );

    return empty_string;
  }
  
  int length = strlen( argument );
  
  for( i = 0; i < max; ++i ) {
    if( !strncasecmp( element( i ), argument, length ) ) {
      if( *value == i ) {
	fsend( ch, "%s on %s is already set to %s.",
	       name, subject, element( i ) );
	return empty_string;
      }
      *value = i;
      //      snprintf( tmp, TWO_LINES, "%s on %s set to %s.\n\r", name, subject, element( i ) );
      //      tmp[0] = toupper( tmp[0] );
      send( ch, "%s on %s set to %s.\n\r", name, subject, element( i ) );
      snprintf( buf, TWO_LINES, "%s set to %s.", name, element( i ) );
      return buf;
    }
  }
  
  //  snprintf( tmp, TWO_LINES, "Unknown %s.\n\r", name );
  send( ch, "Unknown %s.\n\r", name );

  return empty_string;
}


/*
 *   INTEGERS
 */


const char* int_field :: set( char_data *ch, const char* subject,
			      const char *argument )
{
  char*  tmp  = static_string( );
  int    num;

  if( !*argument ) {
    send( ch, "Set %s of %s to?\n\r[ Allowed range: %d to %d - current\
 value: %d ]\n\r", name, subject, min, max, *value );
    return empty_string;
  }
  
  if( ( num = strtoul( argument, 0, 0 ) ) < min || num > max ) {
    send( ch, "The allowed range for %s is from %d to %d.\n\r",
	  name, min, max );
    return empty_string;
  }
  
  if( num == *value ) {
    fsend( ch, "%s on %s is already set to %d.", 
	   name,subject, num );
    return empty_string;
  }

  send( ch, "%s on %s set to %d.\n\r[ Prev. Value: %d ]\n\r",
	name, subject, num, *value );
  snprintf( tmp, THREE_LINES, "%s set to %d from %d.", name, num, *value );

  *value = num;

  return tmp;
}


/*
 *   BYTES
 */


const char* byte_field :: set( char_data *ch, const char* subject,
			       const char *argument )
{
  char*  tmp  = static_string( );
  int    num;
  
  if( !*argument ) {
    send( ch, "Set %s of %s to?\n\r[ Allowed range: %d to %d - current\
 value: %d ]\n\r", name, subject, min, max, *value );
    return empty_string;
  }
  
  if( ( num = atoi( argument ) ) < min || num > max ) {
    send( ch, "The allowed range for %s is from %d to %d.\n\r",
	  name, min, max );
    return empty_string;
  }
  
  if( num == *value ) {
    fsend( ch, "%s on %s is already set to %d.", 
	   name, subject, num );
    return empty_string;
  }
  
  send( ch, "%s on %s set to %d.\n\r[ Prev. Value: %d ]\n\r",
	name, subject, num, *value );
  snprintf( tmp, THREE_LINES, "%s set to %d from %d.", name, num, *value );
  
  *value = num;
  
  return tmp;
}


/*
 *   CENTS
 */


const char* cent_field :: set( char_data *ch, const char* subject,
			       const char *argument )
{
  char*  tmp  = static_string( );
  int    num;

  if( !*argument ) {
    send( ch, "Set %s of %s to?\n\r[ Allowed range: %.2f to %.2f - current\
 value: %.2f ]\n\r", name, subject,
	  (double)min / 100.0, (double)max / 100.0, (double)*value / 100.0 );
    return empty_string;
  }

  // The 0.5 is added to handle cases where the double representation is slightly
  // less than the desired number, so (int) won't round down.
  if( ( num = (int) (100.0*atof( argument )+0.5) ) < min || num > max ) {
    send( ch, "The allowed range for %s is from %.2f to %.2f.\n\r",
	  name, (double) min/100.0, (double) max/100.0 );
    return empty_string;
  }
  
  if( num == *value ) {
    fsend( ch, "%s on %s is already set to %.2f.", 
	   name, subject, (double)num / 100.0 );
    return empty_string;
  }
  
  send( ch, "%s on %s set to %.2f.\n\r[ Prev. Value: %.2f ]\n\r",
	name, subject, (double)num / 100.0, (double)*value / 100.0 );
  snprintf( tmp, THREE_LINES, "%s set to %.2f from %.2f.",
	    name, (double)num / 100.0, (double)*value / 100.0 );
  
  *value = num;

  return tmp;
}


/*
 *   DICE
 */


const char* dice_field :: set( char_data* ch, const char* subject,
			       const char *argument )
{
  static char    buf  [ TWO_LINES ];
  //  char           tmp  [ TWO_LINES ];
  dice_data    dice1 = *value;

  if( !*argument ) {
    //    snprintf( tmp, TWO_LINES, "Set %s of %s to?\n\r[ Current value: %dd%d+%d ]\n\r", 
    //	      name, subject, dice1.number, dice1.side, dice1.plus );
    send( ch, "Set %s of %s to?\n\r[ Current value: %dd%d+%d ]\n\r", 
	  name, subject, dice1.number, dice1.side, dice1.plus );
    return empty_string;
  }

  dice_data dice2;
  dice2.number = atoi( argument );

  for( ; isdigit( *argument ); argument++ );
  if( toupper( *argument++ ) != 'D' ) {
    send( ch, "Incorrect format: #d#+#.\n\r" );
    return empty_string;
  }

  dice2.side = atoi( argument );

  for( ; isdigit( *argument ); argument++ );

  dice2.plus = ( *argument++ == '+' ? atoi( argument ) : 0 );
  
  if( dice2.number > 0x3f
      || dice2.side > 0xfff
      || dice2.plus > 0x3fff ) {
    send( ch, "Max dice value: %dd%d+%d.\n\r",
	  0x3f, 0xfff, 0x3fff );
    return empty_string;
  }

  //  snprintf( tmp, TWO_LINES, "%s on %s set to %dd%d+%d.\n\r[ Prev. Value: %dd%d+%d ]\n\r",
  //	    name, subject, dice2.number, dice2.side, dice2.plus,
  //	    dice1.number, dice1.side, dice1.plus );
  send( ch, "%s on %s set to %dd%d+%d.\n\r[ Prev. Value: %dd%d+%d ]\n\r",
	name, subject, dice2.number, dice2.side, dice2.plus,
	dice1.number, dice1.side, dice1.plus );
  
  *value = (int) dice2;
  
  snprintf( buf, TWO_LINES, "%s set to %dd%d+%d.",
	    name, dice2.number, dice2.side, dice2.plus );

  return buf; 
} 
