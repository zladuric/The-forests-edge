#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "define.h"
#include "struct.h"


/*
 *   SUPPORT FUNCTIONS
 */


const char *one_line( const char* argument, char* line )
{
  if( !*argument ) {
    *line = '\0';
  } else {
    for( ; *argument && *argument != '\n'; argument++, line++ )
      *line = *argument;
    
    *line = '\n';
    *(line+1) = '\r';
    *(line+2) = '\0';
    
    if( *argument == '\n' ) {
      ++argument;
      if( *argument == '\r' )
        ++argument;
    }
  }
  
  return argument;
}


static void display_text( char_data* ch, const char *input )
{
  if( !*input ) {
    page( ch, "(No text has been entered.)\n\r" );
    return;
  }

  char line [ MAX_STRING_LENGTH ];

  for( int i = 1; ; i += 2 ) {
    input = one_line( input, line );
    if( !*line )
      return;
    // *** FIX ME: result can be longer than MAX_STRING_LENGTH
    // But page() can't handle this.
    page( ch, "[%2d]  %s", i, line );
  }
}


const char *break_line( const char* argument, char* line, int length )
{
  const char*   input  = argument;
  char*          word  = 0;
  bool        newword  = true;

  if( !*argument ) {
    *line = '\0';
    return argument;
  }

  for( ; *input && *input != '\n' && input-argument < length; ) {
    if( *input == ' ' ) {
      if( newword ) {
        word = line;
        newword = false;
      }
    } else {
      newword = true;
    }
    *line++ = *input++;
  }

  if( !*input || *input == '\n' || !word ) 
    word = line;

  *word = '\0';

  argument = input+(word-line);
  skip_spaces( argument );

  return argument;
}


/*
 *   SUB_PROCEDURES
 */


int format_tell( char *output, const char *input, const char_data *ch )
{
  const unsigned width = ( ch && ch->pcdata ) ? ch->pcdata->columns : 80;
  const unsigned limit = width-2;

  char *start = output;

  skip_spaces( input );

  strcpy( output, " \"" );
  output += 2;

  char *word = 0;
  unsigned next = 0;
  unsigned count = 3;	// 3 is correct... accounts for extra space after return.
  int total = 3;

  while( *input ) {
    if( word && count > limit ) {
      const int need = 3;
      for( char *line = output-1; line > word; --line )
	*(line+need) = *line;
      output += need;
      memcpy( word, "\n\r  ", 4 );  
      word = 0;
      count = next+2;
      next = 0;
      total += 2;
    }
    /*
    if( ansi ) {
      while( *input == '@' ) {
	*output++ = *input++;
	if( char c = *input ) {
	  *output++ = *input++;
	  if( c == '@' ) {
	    ++count;
	    ++next;
	    ++total;
	  } else if( c == 'I' ) {
	    count += 2;
	    next += 2;
	    total += 2;
	  }
	}
      }
    } else {
    */
    while( *input == '\x1b' ) {
      while( char c = *input ) {
	*output++ = c;
	++input;
	if( isalpha( c ) ) {
	  break;
	}
      }
    }
    //    }
    if( !*input )
      break;
    if( isspace( *input ) ) {
      if( *input == '\n' && *(input+1) == '\r' )
        ++input;
      word = output;
      next = 0;
      *output++ = ' ';
    } else {
      *output++ = *input;
    }
    ++input;
    ++count;
    ++next;
    ++total;
  }

  for( ; isspace( *(output-1) ) && output > start; --output, --total );

  strcpy( output, "\"" );
  ++total;

  // Return amount of space left on first line of output.
  return limit+2-total;
}


unsigned format( char* output, const char* input,
		 bool ansi, int indent, const char_data *ch,
		 bool crlf )
{
  const unsigned width = ( ch && ch->pcdata ) ? ch->pcdata->columns : 80;
  const unsigned limit = width-2;

  skip_spaces( input );
 
  const char *letter;
  const char *start = output;
  char *word = 0;
  unsigned next = 0;
  unsigned count = indent;
  bool skip = false;	// 2 spaces after period can be overwritten with \n\r.
  unsigned delim_size = 0;

  memset( output, ' ', indent );
  output += indent;

  while( *input ) {
    if( word && count > limit ) {
      //      if( !word ) 
      //        word = output;
      const int need = skip ? indent : indent+1+delim_size;
      if( need > 0 ) {
	// Make room to insert crlf and indent.
	for( char *line = output-1; line > word; --line ) {
	  *(line+need) = *line;
	}
	output += need;
      }
      // Catch-22: can't insert default color here, because if the string had a color
      // code in it, the next line won't be re-started with the desired color.
      *(word+delim_size) = '\n';
      *(word+1+delim_size) = '\r';
      memset( word+2+delim_size, ' ', indent );
      word = 0;
      count = indent+next;
      next = 0;
      skip = false;
      delim_size = 0;
    }
    if( ansi ) {
      while( *input == '@' ) {
	*output++ = *input++;
	if( char c = *input ) {
	  *output++ = *input++;
	  if( c == '@' ) {
	    ++count;
	    ++next;
	  } else if( c == 'I' ) {
	    count += 2;
	    next += 2;
	  }
	}
      }
    } else {
      while( *input == '\x1b' ) {
	while( char c = *input ) {
	  *output++ = c;
	  ++input;
	  if( isalpha( c ) ) {
	    break;
	  }
	}
      }
    }
    if( !*input )
      break;
    if( isperiod( *input ) ) {
      do {
        *output++ = *input++;
	++count;
	++next;
      } while( isperiod( *input ) );
      for( letter = input; *letter == ' '; ++letter );
      if( *letter == '\n' || !*letter )
        input = letter;
      if( !*input )
        break;
      // Add a space after period.
      if( *input == ' ' ) {
	do {
	  ++input;
	} while( isspace( *input ) );
	if( count > limit ) {
	  // Word plus period(s) need to go on next line.
	  *output++ = ' ';
	  *output++ = ' ';
	  //	  count += 2;
	  next += 2;
	  continue;
	}
	word = output;
        *output++ = ' ';
        *output++ = ' ';
	count += 2;
	next = 0;
	skip = true;
	delim_size = 0;
      }
      continue;
    }
    if( isspace( *input ) ) {
      word = output;
      *output++ = ' ';
      ++count;
      next = 0;
      skip = false;
      delim_size = 0;
      do {
        ++input;
      } while( isspace( *input ) );
      continue;
    }
    if( *input == '-' ) {
      while( *input == '-' ) {
	*output++ = *input++;
	++count;
	++next;
      }
      if( count > limit )
	continue;
      word = output-1;
      next = 0;
      skip = false;
      delim_size = 1;
      continue;
    }
    *output++ = *input++;
    ++count;
    ++next;
  }
  
  for( ; isspace( *(output-1) ) && output > start; --output ) {
    --count;
  }

  if( count > 0 ) {
    if( ch && !ansi ) {
      output += sprintf( output, color_code( ch, COLOR_DEFAULT ) );
    }
    if( crlf ) {
      *output++ = '\n';
      *output++ = '\r';
    }
  }
  *output = '\0';

  return count;
}


/*
 *   FORMATS CODE 
 */


static void indent( const char *input, char *output, bool partial = false )
{
  char line [ MAX_STRING_LENGTH ];
  int level = 0;
  int spaces = 0;

  *output = '\0';
  
  int over = 0;
  if( partial ) {
    for( ; *input == ' '; ++input, ++over );
  }

  while( true ) {
    for( ; *input == ' '; ++input );
    input = one_line( input, line );
    
    if( !*line )
      break;
    
    const char *letter = line;

    if( *letter == '}' ) {
      --level;
      while( *++letter == ' ' );
    }

    for( int i = 0; i < 2*level+spaces+over; ++i ) {
      *output++ = ' ';
    }
    
    spaces = 0;
    strcpy( output, line );
    output += strlen( line );   

    if( *letter == '{' ) {
      ++level;
    } else if( !strncmp( letter, "if(", 3 )
	       || !strncmp( letter, "if ", 3 )
	       || !strncmp( letter, "loop(", 5 )
	       || !strncmp( letter, "loop ", 5 )
	       || !strncmp( letter, "else", 4 )
	       || !strncmp( letter, "&&", 2 )
	       || !strncmp( letter, "||", 2 ) ) {
      for( ; *letter && *letter != '{'; ++letter );
      if( *letter == '{' ) {
        ++level;	// block
      } else {
        spaces = 2;     // single statement
      }
    }
  }

  *output = '\0';
}


/*
 *   MAIN ROUTINE
 */


const char *edit_string( char_data* ch, const char *argument, const char *input,
			 int mem_type, bool ansi )
{
  if( !*argument ) {
    // Prevent empty command from changing string.
    // This is necessary to prevent unnecessary softcode compiles.
    free_string( ch->pcdata->buffer, MEM_PLAYER );
    ch->pcdata->buffer = alloc_string( input, MEM_PLAYER );
    
    display_text( ch, input );

    return input;
  }

  const char *const too_long = "Due to internal limits, files can be no longer then 8k characters.\n\r";
  const char *const line_long = "Due to internal limits, lines can be no longer then %d characters.\n\r";

  static char paragraph [ 3*MAX_STRING_LENGTH ];

  char buf  [ 3*MAX_STRING_LENGTH ];
  char line [ MAX_STRING_LENGTH ];
  const char *pString = input;
  char *word;
  int i, j, k;
  int repl = -1;
  bool display = true;

  *buf       = '\0';
  *paragraph = '\0';

  const int in_len = strlen( input )+1;
  const int buf_len = strlen( ch->pcdata->buffer )+1;

  if( !strcasecmp( argument, "undo" ) ) {
    if( ch->pcdata->buffer == empty_string ) {
      page( ch, "You haven't edited anything since logging in.\n\r" );
      return pString;
    }
    if( in_len > 1 ) {
      record_delete( in_len, -mem_type);
      record_new( in_len, -MEM_PLAYER);
    }
    if( buf_len > 1 ) {
      record_new( buf_len, -mem_type);
      record_delete( buf_len, -MEM_PLAYER);
    }
    pString = ch->pcdata->buffer;
    ch->pcdata->buffer = input;
    display_text( ch, pString );
    return pString;
  } 

  if( !strcmp( argument, "indent" ) ) {
    indent( input, paragraph );
    input = paragraph;

  } else if( !strcmp( argument, "spell" ) ) {
    if( const char *s = spell( ch, input ) )
      page( ch, s );
    else
      page( ch, "No spelling errors found.\n\r" );
    return pString;

  } else if( *argument == '?' ) {
    argument = one_argument( ++argument, buf );
    i = strlen( buf );		// String being replaced.
    word = paragraph;
    j = in_len;
    k = strlen( argument );	// Replacement.
    repl = 0;
    const char *l = 0;

    while( *input ) {
      if( isprint( *input ) ) {
	if( !l )
	  l = input;
      } else if( *input == '\n' ) {
	l = 0;
      }
      if( i > 0 && !strncmp( buf, input, i ) ) {
	if( k > i ) {
	  if( j + k - i >= 2*MAX_STRING_LENGTH ) {
	    page( ch, too_long );
	    return pString;
	  }
	  if( l && ( input - l ) + k - i > MAX_INPUT_LENGTH ) {
	    page( ch, line_long, MAX_INPUT_LENGTH );
	    return pString;
	  }
	}
        strcpy( word, argument );   
        word += k;
	++repl;
        input += i;
        j += k - i;
      } else {
        *word++ = *input++;
      }
    }
    *word = '\0';
    input = paragraph;

  } else if( *argument ) {
    if( isdigit( *argument ) ) {
      for( i = 0; isdigit( *argument ); argument++ ) 
        i = 10*i + *argument - '0';
    } else {
      i = -1;
    }
    
    if( *argument == '-' && i != -1 ) {
      for( j = 0, argument++; isdigit( *argument ); argument++ )  
        j = 10*j + *argument - '0';
      
      if( j < i ) {
        page( ch, "Line number range incorrect.\n\r" );
	return pString;
      }
      
      skip_spaces( argument );

      for( k = 1; k < i; k += 2 ) {
        input = one_line( input, line );
	if( !*line ) {
	  page( ch, "Line number range incorrect.\n\r" );
	  return pString;
	}
        strcat( paragraph, line );
      }

      if( matches( argument, "cut", 3 ) ) {
	for( ; k <= j; k += 2 ) {
	  input = one_line( input, line ); 
	  if( !*line ) {
	    if( k <= i+1 ) {
	      page( ch, "Line number range incorrect.\n\r" );
	      return pString;
	    }
	    break;
	  }
	  strcat( buf, line );
	}

	free_string( ch->pcdata->paste, MEM_PLAYER );
	ch->pcdata->paste = alloc_string( buf, MEM_PLAYER );

      } else if( matches( argument, "copy", 3 ) ) {
	unsigned n = 0;
	for( ; k <= j; k += 2 ) {
	  input = one_line( input, line ); 
	  if( !*line ) {
	    if( k <= i+1 ) {
	      page( ch, "Line number range incorrect.\n\r" );
	      return pString;
	    }
	    break;
	  }
	  strcat( buf, line );
	  ++n;
	}

	free_string( ch->pcdata->paste, MEM_PLAYER );
	ch->pcdata->paste = alloc_string( buf, MEM_PLAYER );

	page( ch, "Copied %u lines to paste buffer.\n\r", n );
	return pString;

      } else if( matches( argument, "indent", 3 ) ) {
	for( ; k <= j; k += 2 ) {
	  input = one_line( input, line ); 
	  if( !*line ) {
	    if( k <= i+1 ) {
	      page( ch, "Line number range incorrect.\n\r" );
	      return pString;
	    }
	    break;
	  }
	  strcat( buf, line );
	}
	indent( buf, paragraph + strlen( paragraph ), true );

      } else if( matches( argument, "delete", 3 ) ) {
	for( ; k <= j; k += 2 ) {
	  input = one_line( input, line ); 
	  if( !*line ) {
	    if( k <= i+1 ) {
	      page( ch, "Line number range incorrect.\n\r" );
	      return pString;
	    }
	    break;
	  }
	}

      } else if( *argument ) {
	page( ch, "Bad syntax after line range.\n\r" );
	return pString;

      } else {
	for( ; k <= j; k += 2 ) {
	  input = one_line( input, line ); 
	  if( !*line ) {
	    if( k <= i+1 ) {
	      page( ch, "Line number range incorrect.\n\r" );
	      return pString;
	    }
	    break;
	  }
	  int l = strlen( line );
	  if( line[ l - 3 ] == '-' ) {
	    line[ l - 2 ] = '\0';
	  } else {
	    line[ l - 2 ] = ' ';
	    line[ l - 1 ] = '\0';
	  }
	  
	  strcat( buf, line );
	}
	
	// Parameter "ansi" should be set if color codes will be interpreted
	// when the string is actually printed outside the editor.
	format( line, buf, ansi );
	strcat( paragraph, line );
      }

      while( true ) {
        input = one_line( input, line );
        if( !*line )
          break;
        strcat( paragraph, line );
      }

    } else if( *argument == '?' && i != -1 ) {
      for( k = 1; k < i; k += 2 ) {
        input = one_line( input, line );
	if( !*line ) {
	  break;
	}
        strcat( paragraph, line );
      }
      input = one_line( input, line );
      if( k != i || !*line ) {
	page( ch, "No such line.\n\r" );
	return pString;
      }
      argument = one_argument( ++argument, buf );
      i = strlen( buf );
      word = paragraph + strlen( paragraph );
      j = in_len;
      k = strlen( argument );	// Replacement.
      repl = 0;
      const char *str = line;

      while( *str ) {
	if( i > 0 && !strncmp( buf, str, i ) ) {
	  if( k > i ) {
	    // Replacement is longer than original text.
	    if( j + k - i >= 2*MAX_STRING_LENGTH ) {
	      page( ch, too_long );
	      return pString;
	    }
	    if( ( str - line ) + k - i > MAX_INPUT_LENGTH ) {
	      page( ch, line_long, MAX_INPUT_LENGTH );
	      return pString;
	    }
	  }
	  strcpy( word, argument );
	  word += k;
	  ++repl;
	  str += i;
	  j += k - i;
	} else {
	  *word++ = *str++;
	}
      }
      *word = '\0';

      while( true ) {
        input = one_line( input, line );
        if( !*line )
          break;
        strcat( paragraph, line );
      }

    } else if( *argument == '+' && i != -1 ) {
      // Paste.
      if( !*ch->pcdata->paste ) {
	page( ch, "Paste buffer is empty.\n\r" );
	return pString;
      }

      const int paste_len = strlen( ch->pcdata->paste );

      if( in_len + paste_len >= 2*MAX_STRING_LENGTH ) {
        page( ch, too_long );
        return pString;
      }

      for( k = 1; k < i; k += 2 ) {
        input = one_line( input, line );
	if( !*line ) {
	  page( ch, "Line number %d too large.\n\r", i );
	  return pString;
	}
        strcat( paragraph, line );
      }

      strcat( paragraph, ch->pcdata->paste );

      while( true ) {
        input = one_line( input, line );
        if( !*line )
          break;
	if( k != i )
	  strcat( paragraph, line );
	k += 2;
      }

    } else {
      if( in_len >= 2*MAX_STRING_LENGTH && *argument ) {
        page( ch, too_long );
        return pString;
      }

      display = !is_set( ch->pcdata->pfile->flags, PLR_EDITOR_QUIET );

      if( *argument == ' ' && i != -1 ) {
        ++argument;
      }
      
      for( k = 1; ; k += 2 ) {
        input = one_line( input, line );

	if( !display ) {
	  if( !*line ) {
	    if( i == -1 || ( k == i && *argument ) || k == i+1 ) {
	      page( ch, "Appended line %d.\n\r", k );
	    }
	  } else if( k == i ) {
	    if( *argument ) {
	      page( ch, "Replaced line %d.\n\r", k );
	    } else {
	      page( ch, "Deleted line %d.\n\r", k );
	    }
	  } else if( k == i+1 ) {
	    page( ch, "Inserted line %d.\n\r", k );
	  }
	}

        if( !*line ) {
          if( i == -1 || ( k == i && *argument ) || k == i+1 ) {
            strcat( paragraph, argument );
            strcat( paragraph, "\n\r" );
	  } else if( k <= i ) {
	    page( ch, "Line number %d too large.\n\r", i );
	    return pString;
	  }
          break;
	}

	if( ( k == i && *argument ) || k == i+1 ) {
          strcat( paragraph, argument );
          strcat( paragraph, "\n\r" );
	}

        if( k != i ) {
          strcat( paragraph, line );
	}
      }
    }
    input = paragraph;
  }
  
  if( repl == 0 ) {
    page( ch, "No occurrences found.\n\r" );
    return pString;
  }

  if( in_len > 1 ) {
    record_delete( in_len, -mem_type);
    record_new( in_len, -MEM_PLAYER);
  }
  
  free_string( ch->pcdata->buffer, MEM_PLAYER );
  ch->pcdata->buffer = pString;

  if( repl > 0 ) {
    page( ch, "\n\rReplaced %d occurrences.\n\r\n\r", repl );
  }

  if( display ) {
    display_text( ch, input );
  }

  return alloc_string( input, mem_type );
}
