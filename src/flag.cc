#include <ctype.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include "define.h"
#include "struct.h"


/*
 *   GLOBAL CONSTANTS
 */


int max_permission  = MAX_PERMISSION;

flag_data affect_flags = { "Affect",
			   &aff_char_table[0].name,
			   &aff_char_table[1].name,
			   &table_max[ TABLE_AFF_CHAR ], true };

flag_data permission_flags = { "Permission",
			       &permission_name[0],
			       &permission_name[1],
			       &max_permission, true };

flag_data material_flags = { "Materials", 
			     (const char**) &material_table[0].name,
			     (const char**) &material_table[1].name,
			     &table_max[ TABLE_MATERIAL ], true };


int last_bit;
int last_flag;


const char *flag_handler( const char** title, const char*** name1,
			  const char*** name2, int **bit, int **perm, int* max,
			  const int* uses_flag, const bool *sort,
			  const char* can_edit,
			  char_data* ch, const char *argument, const char *what,
			  int types, flag_op op )
{
  const char*  string;
  int            i, j;
  
  if( !*argument ) {
    for( i = j = 0; i < types; i++ ) {
      if( uses_flag[i] ) {
        if( j++ != 0 )
          page( ch, "\n\r" );
        display_flags( title[i], name1[i], name2[i], bit[i], max[i], ch, sort[i] );
      }
    }
    return empty_string;
  }
  
  int last = -1;

  for( i = 0; i < types; ++i ) {
    if( uses_flag[i] ) {
      if( matches( argument, title[i] ) ) {
	display_flags( title[i], name1[i], name2[i], bit[i], max[i], ch, sort[i] );
	return empty_string;
      }
      last = i;
    }
  }
  
  for( bool exact = true; ; exact = false ) {
    for( i = 0; i < types; ++i ) {
      if( uses_flag[i]
	  && ( string = set_flags( name1[i], name2[i],
				   bit[i], perm ? perm[i] : 0,
				   max[i], uses_flag[i] == 1 ? can_edit : 0,
				   ch, argument, what,
				   exact, !exact && i == last,
				   sort[i], op ) ) ) {
	if( *string ) {
	  last_bit = i;
	}
        return string;
      }
    }
    if( !exact )
      break;
  }
  
  return 0;
}


/*
 *   DISPLAY ROUTINES
 */


void sort_ints( const int *int1,
		const int *int2,
		int *sorted,
		int max,
		bool reverse )
{
  for( int i = 0; i < max; ++i ) {
    sorted[i] = i;
  }
  
  for( int n = 0; n < max; ++n ) {
    bool done = true;
    for( int k = 0; k < max-1-n; ++k ) {
      const int i1 = *(int1+sorted[k]*(int2-int1));
      const int i2 = *(int1+sorted[k+1]*(int2-int1));
      if( reverse ? ( i1 < i2 ) : ( i2 < i1 ) ) {
	swap( sorted[k], sorted[k+1] );
	done = false;
      }
    }
    if( done )
      break;
  }
}


int sort_names( const char *const *name1,
		const char *const *name2,
		int *sorted,
		int max,
		bool sort )
{
  int j = 0;
  for( int i = 0; i < max; ++i ) {
    const char *const name = *(name1+i*(name2-name1));
    if( name && *name != '?' ) {
      sorted[j++] = i;
    }
  }

  if( !sort )
    return j;

  for( int n = 0; n < j; ++n ) {
    bool done = true;
    for( int k = 0; k < j-1-n; ++k ) {
      const char *const s1 = *(name1+sorted[k]*(name2-name1));
      const char *const s2 = *(name1+sorted[k+1]*(name2-name1));
      if( strcasecmp( s1, s2 ) > 0 ) {
	swap( sorted[k], sorted[k+1] );
	done = false;
      }
    }
    if( done )
      break;
  }

  return j;
}


void display_flags( const char* text,
		    const char *const *name1, const char *const *name2,
		    int* bit, int max, char_data* ch,
		    bool sort )
{
  if( !ch->pcdata )
    return;

  const unsigned columns = ch->pcdata->columns / 19;

  int sorted[ max ];
  max = sort_names( name1, name2, sorted, max, sort );

  if( *text == '*' ) 
    page_title( ch, "%s ", text+1 );
  else 
    page( ch, "%s Flags:\n\r", text );

  int i = 0;

  for( ; i < max; ++i ) {
    page( ch, "%15s (%1c)%s",
	      *(name1+sorted[i]*(name2-name1)),
	      is_set( bit, sorted[i] ) ? '*' : ' ',
	      i%columns == columns-1 ? "\n\r" : "" );
  }
  
  if( i%columns != 0 )
    page( ch, "\n\r" );     
}


void flag_data :: sprint( char *tmp, int* bit )
{
  int m = *max;

  int sorted[ m ];
  m = sort_names( name1, name2, sorted, m, sort );

  *tmp = '\0';
  
  if( *max < 16 && *bit+1 == ( 1 << *max ) ) {
    strcpy( tmp, "all" );
    return;
  }

  bool any = false;

  for( int i = 0; i < m; ++i ) {
    if( is_set( bit, sorted[i] ) ) {
      tmp += sprintf( tmp, "%s%s", any ? ", " : "",
		      *(name1+sorted[i]*(name2-name1)) );
      any = true;
    }
  }

  if( !any ) 
    strcpy( tmp, "none" );
}
  

void flag_data :: display( char_data* ch, int* bit )
{
  if( !ch->pcdata )
    return;

  const unsigned columns = ch->pcdata->columns / 19;

  int m = *max;

  int sorted[ m ];
  m = sort_names( name1, name2, sorted, m, sort );

  int      i;

  page( ch, "%s Flags:\n\r", title );

  for( i = 0; i < m; ++i ) {
    page( ch, "%15s (%1c)%s",
	      *(name1+sorted[i]*(name2-name1)),
	      is_set( bit, sorted[i] ) ? '*' : ' ',
	      i%columns == columns-1 ? "\n\r" : "" );
  }
  
  if( i%columns != 0 )
    page( ch, "\n\r" );     
}


/*
 *   SET ROUTINE
 */


const char *flag_data :: set( char_data* ch, const char *argument, const char *what,
			      int *bit, int *perm,
			      bool exact, bool fail_msg )
{
  if( !*argument ) {
    display( ch, bit );
    return 0;
  }

  return set_flags( name1, name2, bit, perm, *max, 0,
		    ch, argument, what,
		    exact, fail_msg, sort );
}


const char *set_flags( const char *const *name1, const char *const *name2,
		       int *bit, int *perm,
		       int max, const char* can_edit,
		       char_data* ch, const char *argument, const char *what,
		       bool exact, bool fail_msg, bool sort, flag_op op )
{
  int sorted[ max ];
  max = sort_names( name1, name2, sorted, max, sort );

  char *tmp = static_string( );

  for( int i = 0; i < max; i++ ) {
    const char *name = *(name1+sorted[i]*(name2-name1));
    if( matches( argument, name, exact ) ) {
      if( can_edit ) {
        send( ch, can_edit );
        return empty_string;
      }
      if( perm && !is_set( perm, sorted[i] ) ) {
        fsend( ch, "You do not have permission to alter the \"%s\" flag.", name );
	return empty_string;
      }
      switch( op ) {
      case flag_remove:
	remove_bit( bit, sorted[i] );
	break;
      case flag_set:
	set_bit( bit, sorted[i] );
	break;
      case flag_toggle:
	switch_bit( bit, sorted[i] );
	break;
      }
      last_flag = sorted[i];
      if( what ) {
	fsend( ch, "%s on %s set to %s.",
	       name,
	       what,
	       true_false( bit, sorted[i] ) );
      } else {
	fsend( ch, "%s set to %s.",
	       name,
	       true_false( bit, sorted[i] ) );
      }
      snprintf( tmp, THREE_LINES, "%s set to %s.", name,
		true_false( bit, sorted[i] ) );
      *tmp = toupper( *tmp );
      return tmp;
    }
  }
  
  if( fail_msg )
    send( ch, "Unknown flag - use no argument for list.\n\r" );
  
  return 0;
}


bool set_flags( char_data* ch, const char *& argument, int *bit, const char* flags )
{
  bool set;
  int    i;

  if( *argument != '+' && *argument != '-' )
    return false;

  set = ( *argument++ == '+' ); 
  
  for( ; *argument && *argument != ' '; argument++ ) { 
    for( i = 0; ; i++ ) {
      if( !flags[i] ) {
        send( ch, "Unknown flag: %c.\n\r", *argument );
        break;
      } else if( flags[i] == *argument ) {
        send( ch, "%c bit %s.\n\r", *argument,
	      set ? "set" : "removed" );
        assign_bit( bit, i, set );
        break;
      }
    }
  }

  return true;
}


/* 
 *   TOGGLE FUNCTION
 */


void set_bool( char_data* ch, const char *argument, const char* text,
	       bool& flag )
{
  if( !strncasecmp( "true", argument, strlen( argument ) ) ) {
    flag = true;
  }
  else if( !strncasecmp( "false", argument, strlen( argument ) ) ) {
    flag = false;
  }
  else {
    fsend( ch, "%s can be set either true or false.", text );
    return;
  }
  
  fsend( ch, "%s set %s.", text, flag ? "true" : "false" );
}


bool toggle( char_data* ch, const char *arg, const char* text,
	     int* bit, int flag )
{
  bool already;
  
  if( !strcasecmp( arg, "on" ) ) {
    if( !( already = is_set( bit, flag ) ) ) 
      set_bit( bit, flag );
  } else if( !strcasecmp( arg, "off" )  ) {
    if( !( already = !is_set( bit, flag ) ) )
      remove_bit( bit, flag );
  } else 
    return false;
  
  if( text )
    if( already )
      fsend( ch, "%s already %s.", text, arg );
    else
      fsend( ch, "%s turned %s.", text, arg );

  return true;
}


/*
 *   GET_FLAGS ROUTINE
 */


/* This function takes a character, an argument, a bit, a list of flags, and
   a function name.  The character is the character whose bits are being
   set.  The argument is a pointer to a string, starting with -.  The 
   function parses till the end of string or until the first white space.  
   It sets bits, in the order that they appear in the flags argument.  The 
   function name is used to report that invalid arguments were passed to
   a certain function. */
bool get_flags( char_data* ch, const char *& argument, int* bit, const char* flags,
		const char* function )
{
  //initialize the bits to 0
  *bit = 0;
  
  /* If the argument doesn't start with -, return TRUE */
  if( *argument != '-' )
    return true;

  /* Move to the next character in the argument */
  ++argument;

  /* Move through the arguement, until a space or end of string. 
     If hit the end of available flags, then tell the character illegal
     argument.  Otherwise set the appropriate bit.  */
  
  for( ; *argument && *argument != ' '; ++argument ) {
    for( int i = 0; ; i++ ) {
      if( !flags[i] ) {
	if( function )
	  send( ch, "%s: illegal option %c.\n\r", function, *argument );
	--argument;
	return false;
      }
      if( *argument == flags[i] ) {
        set_bit( bit, i );
        break;
      }
    }
  }
  
  /* skip the spaces in the argument */
  skip_spaces( argument );
  
  return true;
}


/* 
 *   ALTER ROUTINE
 */


void alter_flags( int* modify, int* next, int* prev, int max )
{
  for( int i = 0; i < max; i++ ) {
    if( is_set( prev, i ) != is_set( next, i ) 
	&& is_set( modify, i ) != is_set( next, i ) ) {
      switch_bit( modify, i );
    }
  }
}


/*
 *  LEVEL ROUTINES
 */


void display_levels( const char* text, const char** name1,
		     const char** name2, int* bit, int max, char_data* ch,
		     bool sort)
{
  if( !ch->pcdata )
    return;

  const unsigned columns = ch->pcdata->columns / 19;

  int      i;
  int  level;

  int sorted[ max ];
  max = sort_names( name1, name2, sorted, max, sort );

  page_title( ch, "%s Levels", text );

  for( i = 0; i < max; ++i ) {
    level = level_setting( bit, sorted[i] );
    page( ch, "%15s (%c)%s",
	  *(name1+sorted[i]*(name2-name1)),
	  level == 0 ? '-' : '0'+level,
	  i%columns == columns-1 ? "\n\r" : "" );
  }
  
  if( i%columns != 0 )
    page( ch, "\n\r" );     
}


bool set_levels( const char** name1, const char** name2, int* bit,
		 int max, char_data* ch, const char *argument, bool exact,
		 bool sort )
{
  int        i;
  int    level;

  int sorted[ max ];
  max = sort_names( name1, name2, sorted, max, sort );

  for( i = 0; i < max; i++ ) {
    const char *name = *(name1+sorted[i]*(name2-name1));
    if( matches( argument, name, exact ) ) {
      if( *argument == '\0'
	  || ( !is_number( argument )
	       && *argument != '-'
	       && !strcasecmp( argument, "off" ) ) ) {
        send( ch,
	      "You must specify to what level you wish to set '%s'.\n\r\n\r",
	      name );
        send( ch,
	      "[ Current setting: %s ]\n\r",
	      number_word( level_setting( bit, sorted[i] ) ) );
        return true;
      }

      if( ( level = atoi( argument ) ) < 0 || level > 3 ) {
        send( ch,
	      "The allowed level range for '%s' is from zero to three.\n\r",
	      name );
        return true;
      }

      set_level( bit, sorted[i], level );

      if( level == 0 ) 
        send( ch,
	      "%s turned off.\n\r",
	      name );
      else
        send( ch,
	      "%s set to level %s.\n\r",
	      name,
	      number_word( level ) );
      return true;
    }
  }
  
  return false;
}
