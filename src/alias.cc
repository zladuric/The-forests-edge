#include <sys/types.h>
#include <stdio.h>
#include <syslog.h>
#include "define.h"
#include "struct.h"


/*
 *   ALIAS NEW AND DELETE
 */


alias_data :: alias_data( const char *arg, const char *argument )
  : length(0)
{
  record_new( sizeof( alias_data ), MEM_ALIAS );

  abbrev = alloc_string( arg, MEM_ALIAS );
  command = alloc_string( argument, MEM_ALIAS );
}


alias_data :: ~alias_data( )
{
  record_delete( sizeof( alias_data ), MEM_ALIAS );
  
  free_string( abbrev, MEM_ALIAS );
  free_string( command, MEM_ALIAS );
}
 
 
/*
 *   ALIAS ROUTINES
 */


void do_alias( char_data* ch, const char *argument )
{
  if( is_mob( ch ) )
    return;

  player_data *pc = player( ch );
  int length = -1;

  if( *argument == '-'
      && isdigit( *(argument+1) )
      && *(argument+2) == ' ' ) {
    length = *(argument+1) - '0';
    argument += 3;
    skip_spaces( argument );
  }

  char arg [ MAX_INPUT_LENGTH ];

  argument = one_argument( argument, arg );

  if( !*arg ) {
    if( pc->alias.is_empty() ) {
      send( ch, "You have no aliases.\n\r" );
      return;
    }
    page_underlined( ch, "%-12s Min  Substitution\n\r", "Alias" );
    for( int i = 0; i < pc->alias; ++i ) {
      alias_data *alias = pc->alias[i];
      const unsigned indent = 18;
      if( strlen( alias->command )+indent < ch->pcdata->columns ) {
	page( ch, "%-12s  %c   %s\n\r",
	      alias->abbrev,
	      alias->length ? alias->length + '0' : ' ',
	      alias->command );
      } else {
	const char *s = alias->command;
	const char *w = s;
	const char *t = s;
	while( true ) {
	  if( t-s+indent == ch->pcdata->columns ) {
	    if( w == s ) {
	      // No word-break found.
	      w = t-1;
	      t = 0;
	    }
	    if( s == alias->command ) {
	      page( ch, "%-12s  %c   %.*s\n\r",
		    alias->abbrev,
		    alias->length ? alias->length + '0' : ' ',
		    w-s, s );
	    } else {
	      page( ch, "%18s%.*s\n\r", "", w-s, s );
	    }
	    s = t ? w+1 : w;
	    skip_spaces(s);
	    t = w = s;
	  }
	  if( !*t )
	    break;
	  if( *t == ' ' )
	    w = t;
	  ++t;
	}
	if( t > s ) {
	  page( ch, "%18s%.*s\n\r", "", t-s, s );
	}
      }
    }
    return;
  }

  const char *a = arg;
  if( *a == '*' )
    ++a;

  if( strlen( a ) > 8 ) {
    send( ch, "Alias must be less than 9 letters.\n\r" );
    return;
  }

  skip_spaces( a );
  if( !*a ) {
    send( ch, "Aliasing nothing doesn't make much sense.\n\r" );
    return;
  }

  int pos = pntr_search( pc->alias.list, pc->alias.size, arg );

  if( !*argument ) {
    if( pos >= 0 ) {
      delete pc->alias[pos];
      pc->alias.remove( pos );
      send( ch, "Alias removed.\n\r" );
      return;
    }
    send( ch, "Alias not found.\n\r" );
    return;
  }

  if( strlen( argument ) > 300 ) {
    send( ch, "Substitution must be less than 301 letters.\n\r" );
    return;
  }

  if( is_number( arg ) ) {
    fsend( ch, "Due to the prevalent usage of numbers as arguments in\
 commands, it is ill-advised to alias numbers without the use of an\
 asterisk.  See \"help alias\" for more information." );
    return;
  }
  
  if( pos >= 0 ) {
    alias_data *alias = pc->alias[pos];
    free_string( alias->command, MEM_ALIAS );
    alias->command = alloc_string( argument, MEM_ALIAS );
    if( length >= 0 ) {
      alias->length = length;
    }
    fsend( ch, "Alias replaced %s -> \"%s\".",
	   alias->abbrev, alias->command );
    if( *alias->abbrev != '*' ) {
      fsend( ch, "Consider using %s*%s%s instead to anchor this alias; see \"help alias\".",
	     color_code( ch, COLOR_MILD ),
	     alias->abbrev,
	     color_code( ch, COLOR_DEFAULT ) );
    }
  } else {
    if( !is_builder( ch )
	&& pc->alias.size >= 100 ) {
      send( ch, "You are limited to one hundred aliases.\n\r" );
      return;
    }
    alias_data *alias = new alias_data( arg, argument );
    if( length > 0 ) {
      alias->length = length;
    }
    pc->alias.insert( alias, -pos-1 );
    fsend( ch, "Alias added %s -> \"%s\".",
	   alias->abbrev, alias->command );
    if( *alias->abbrev != '*' ) {
      fsend( ch, "Consider using %s*%s%s instead to anchor this alias; see \"help alias\".",
	     color_code( ch, COLOR_MILD ),
	     alias->abbrev,
	     color_code( ch, COLOR_DEFAULT ) );
    }
  }
}


char *subst_alias( link_data* link, char *message )
{
  static char         buf  [ MAX_STRING_LENGTH ];
  player_data*     player;
  alias_data*       alias;
  const char*      abbrev;
  int             i, j, k;
  int              length;
  bool            newline  = true;

  if( !( player = link->player )
      || link->connected != CON_PLAYING 
      || player->alias.is_empty()
      || !strncasecmp( message, "ali", 3 ) )
    return message;

  skip_spaces( message );

  for( i = j = 0; message[i] && j < MAX_INPUT_LENGTH; ) {
    for( k = 0; k < player->alias; ++k ) {
      alias = player->alias[k];
      if( *alias->abbrev == '*' ) {
        if( !newline )
          continue;
        abbrev = alias->abbrev+1;
      } else {
        abbrev = alias->abbrev;
      }
      if( alias->length ) {
	length = alias->length;
      } else {
	length = strlen( abbrev );
      }
      if( !strncasecmp( message+i, abbrev, length ) ) {
	bool fail = true;
	while( true ) {
	  if( message[ length+i ] == ' '
	      || message[ length+i ] == '\0' ) {
	    strcpy( buf+j, alias->command );
	    i += length;
	    j += strlen( alias->command );
	    fail = false;
	    break;
	  }
	  if( !*( abbrev+length )
	      || toupper( *(message+i+length) ) != toupper( *(abbrev+length) ) ) {
	    break;
	  }
	  ++length;
	}
	if( !fail ) {
	  break;
	}
      }
    }
    if( k == player->alias.size ) {
      while( message[i] != ' ' && message[i] )
        buf[j++] = message[i++];
    }
    for( ; message[i] == ' '; ++i ) {
      buf[j++] = ' ';
    }
    if( message[i] == '&' && message[i+1] == ' ' ) {
      newline = true;
      strcpy( buf+j, "& " );
      j += 2;
      i += 2;
    } else {
      newline = false;
    }
  }

  if( j >= MAX_INPUT_LENGTH ) {
    send( player, "!! Truncating input !!\n\r" );
    buf[ MAX_INPUT_LENGTH-1 ] = '\0';
  } else {
    buf[j] = '\0';
  }

  return buf;
}
