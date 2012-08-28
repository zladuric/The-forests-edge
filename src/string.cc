#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <limits.h>
#include <unistd.h>
#include "define.h"
#include "struct.h"


char *const empty_string = "";


#define STATIC_STRINGS 30

static char static_storage [ STATIC_STRINGS*THREE_LINES ];
static int store_pntr = 0;


static text static_text_store [ STATIC_STRINGS ];
static unsigned text_pntr = 0;


char *static_string( const char* msg )
{
  char *tmp = &static_storage[store_pntr*THREE_LINES];
  store_pntr = ( store_pntr+1 )%STATIC_STRINGS;

  if( !msg ) {
    *tmp = '\0';
  } else {
    strcpy( tmp, msg );
  }

  return tmp;
}


text& static_text( const char *msg )
{
  text& str = static_text_store [ text_pntr ];
  text_pntr = (text_pntr+1)%STATIC_STRINGS;

  if( !msg ) {
    str = empty_string;
  } else {
    str = msg;
  }

  return str;
}


/*
 *   STRING COMPARISON
 */


static bool null_strings( const char* astr, const char* bstr )
{
  if( astr && bstr )
    return false;

  if( !astr && !bstr )
    bug( "Str*cmp: Null string (x2)." );
  else
    bug( "Str*cmp: Comparing '%s' to null.",
	 astr ? astr : bstr );

  return true;
}


/* Takes two strings as input.  If astr is greater returns 1.  If they
   are equal, returns 0.  If bstr is greater, returns -1.  Does not take
   case into accounted (compares tolower'd strings).  If one string is 
   less then the other, truncates the longer string and compares the two
   shortened strings */
int strcasecmp( const char* astr, const char* bstr )
{
  /* If the two pointers point to the same thing, return equal */
  if( astr == bstr )
    return 0;

  /* If the strings are null, return greater then */
  if( null_strings( astr, bstr ) )
    return 1;

  /* Parse through the strings while neither one has ended.  If
     the tolower of the strings is not equal at some point, return
     1 if astr is greater, and -1 if bstr is greater */
  for( ; *astr || *bstr; astr++, bstr++ )
    if( tolower( *astr ) != tolower( *bstr ) )
      return( tolower( *astr ) > tolower( *bstr ) ? 1 : -1 );

  /* One of the strings ended without hitting and inequality. Return
     equal */
  return 0;
}


int strncasecmp( const char* astr, const char* bstr, int n )
{
  if( null_strings( astr, bstr ) )
    return 1;

  const char *end = astr+n;

  for( ; astr < end && ( *astr || *bstr ); astr++, bstr++ )
    if( tolower( *astr ) != tolower( *bstr ) )
      return( tolower( *astr ) > tolower( *bstr ) ? 1 : -1 );

  return 0;
}


int strncmp( const char* astr, const char* bstr, int n )
{
  if( null_strings( astr, bstr ) )
    return 1;

  const char *end = astr+n;

  for( ; astr < end && ( *astr || *bstr ); astr++, bstr++ )
    if( *astr != *bstr )
      return( *astr > *bstr ? 1 : -1 );

  return 0;
}


char* strcat( char* dst, const char* src )
{
  char *s;

  for( s = dst; *s; s++ );
  while( *src )
    *s++ = *src++;

  *s = '\0';

  return dst;
}


int rstrcasecmp( const char* astr, const char* bstr )
{
  if( null_strings( astr, bstr ) )
    return 1;

  int a = strlen( astr );
  int b = strlen( bstr );
  int c = min( a, b ); 

  for( int i = 1; i <= c; ++i )
    if( tolower( astr[a-i] ) != tolower( bstr[b-i] ) ) 
      return( tolower( astr[a-i] ) > tolower( bstr[b-i] ) ? 1 : -1 );

  return( a < b ? -1 : a > b );
}


int rstrncasecmp( const char* astr, const char* bstr, int n )
{
  if( null_strings( astr, bstr ) )
    return 1;

  int a = strlen( astr );
  int b = strlen( bstr );
  int c = min( min( a, b ), n ); 

  for( int i = 1; i <= c; ++i ) {
    if( tolower( astr[a-i] ) != tolower( bstr[b-i] ) ) {
      return( tolower( astr[a-i] ) > tolower( bstr[b-i] ) ? 1 : -1 );
    }
  }

  return( c == n ? 0 : a < b );
}


/*
 *   GENERIC UTILITY ROUTINES
 */

/* This function takes an argument, a word, and a length.  If the argument
   is the same as the word, and the length of the argument is greater then
   length, return TRUE.  Otherwise return FALSE. */
// Matches through end of line.
bool fmatches( const char *argument, const char* word, size_t len )
{
  const char *str = argument;

  char quote = 0;

  if( *str == '\'' || *str == '\"' ) {
    quote = *str++;
  }

  if( !*str )
    return !*word && len == 0;

  size_t count = 0;

  for( ; *str && *str != quote; ++word, ++str, ++count ) {
    if( tolower( *word ) != tolower( *str ) )
      return false;
  }

  if( *str ) {
    ++str;
    skip_spaces( str );
    if( *str ) {
      // Not EOL.
      return false;
    }
  }

  return count >= len;

  /*
  int i;

  for( i = 0; str[i] && str[i] != quote; ++i ) 
    if( tolower( word[i] ) != tolower( str[i] ) )
      return false;

  if( quote
      && str[i]
      && str[i+1] ) {
    return false;
  }

  return i >= length;
  */
}


// Similar to fmatches(), but word doesn't have to be at end of line.
bool matches( const char*& argument, const char *word, size_t len )
{
  const char *str = argument;

  char quote = ' ';

  if( *str == '\'' || *str == '\"' ) {
    quote = *str++;
  }

  if( !*str )
    return !*word && len == 0;

  size_t count = 0;

  for( ; *str && *str != quote; ++word, ++str, ++count ) {
    if( tolower( *word ) != tolower( *str ) )
      return false;
  }

  if( *str ) {
    if( quote != ' ' ) {
      ++str;
      if( *str && !isspace( *str ) )
	return false;
    }
    skip_spaces( str );
  }

  if( count >= len ) {
    argument = str;
    return true;
  }

  return false;
}


bool exact_match( const char*& argument, const char *word )
{
  const char *str = argument;

  char quote = 0;

  if( *str == '\'' || *str == '\"' ) {
    quote = *str++;
  }

  for( ; *word; word++, str++ ) {
    if( tolower( *word ) != tolower( *str ) )
      return false;
  }

  if( quote ) {
    if( *str == quote ) {
      ++str;
    } else if( *str ) {
      return false;
    }
  }
  
  if( *str
      && !isspace( *str )
      && *str != ','
      && *str != ';' )
    return false;
  
  skip_spaces( str );
  argument = str;

  return true;
}


bool number_arg( const char*& argument, int& i, bool unsign )
{
  const char* str  = argument;
  int           j  = 0;
  bool      minus  = false;
  
  if( !unsign ) {
    if( *str == '-' ) {
      minus = true;
      ++str;
    } else if( *str == '+' ) {
      ++str;
    }
  }
  
  while(true) {
    if( !isdigit( *str ) )
      return false;
    j = 10*j+(*str++)-'0';
    if( !*str || *str == ' ' )
      break;
  }
  
  argument = str;
  i = minus ? -j : j;
  
  skip_spaces( argument );
  
  return true;
}


const char *separate( const char *string, bool identified )
{
  if( *string != '{' )
    return string;
  
  int i;
  
  for( i = 1; string[i] != '}' && string[i]; ++i );
  
  if( !identified ) {
    if( i == 1 )
      return empty_string;
    char *tmp = static_string( );
    memcpy( tmp, string+1, i-1 );
    tmp[i-1] = '\0';
    return tmp;
  }

  if( !string[i] )
    return empty_string;

  for( i++; string[i] == ' '; i++ );

  return string+i;
}


/*
char* one_condition( char *argument, char *cond )
{
  if( !*argument )
    *cond = '\0';
  else {
    for( ; *argument != '\n' && *argument
	   && *argument != '&'; argument++, cond++ )
      *cond = *argument;

    *cond = '\0';

    if( *argument == '&' )
      argument += 2;

    if( *argument == '\n' ) {
      argument++;
      if( *argument == '\r' )
        argument++;
      }
    }

  return argument;
}
*/


// Remove all words in "words" from "phrase".
// Returns 0 or newly-allocated string.
char *subtract( const char *phrase, const char *words )
{
  char buf [ MAX_STRING_LENGTH ];
  char word [ MAX_STRING_LENGTH ];
  bool omit = false;

  *buf = '\0';

  while( true ) {
    phrase = one_argument( phrase, word );

    if( !*word ) {
      if( *buf && omit ) {
	size_t l = strlen( buf );
	char *str = new char [ l+1 ];
	strcpy( str, buf );
	return str;
      } else {
	return 0;
      }
    }

    if( !is_name( word, words )
	&& !is_name( word, phrase ) ) {
      if( *buf ) {
	strcat( buf, " " );
      }
      strcat( buf, word );
    } else {
      omit = true;
    }
  }
}


/*
 *  IS_NAME ROUTINE
 */


// Name/keyword matching.
// All words in str must be found in namelist.
int is_name( const char *str, const char* namelist, bool scored )
{
  char name  [ MAX_STRING_LENGTH ];
  char word  [ MAX_STRING_LENGTH ];

  bool trivial = true;

  int score = 0;

  while( true ) {
    str = one_argument( str, word );

    if( !*word )
      return trivial ? 0 : score;

    const char *list = namelist;
    int length = 3;
    bool partial = false;

    while( true ) {
      list = one_argument( list, name );

      if( !*name ) {
	if( partial ) {
	  break;
	}
        return 0;
      }

      if( isdigit( *name ) ) {
        length = atoi( name );
        continue;
      }
      
      int wordlen = strlen( word );

      if( strncasecmp( word, name, wordlen ) ) {
	// Mismatch.
        length = 3;
        continue;
      }
      
      /*
	if( command ) {
	int pos = search( command_table, table_max[ TABLE_COMMAND ], name );
        if( pos >= 0 && command_table[pos].reqlen <= (int)strlen( word ) )
	break;
	}
      */
      
      int namelen = strlen( name );

      if( wordlen >= min( length, namelen ) ) {
	if( !scored || namelen == wordlen ) {
	  // Complete word match.
	  partial = !scored;	// 1 point per word if !scored.
	  break;
	}
	// Partial word match.
	partial = true;
      }
    }

    if( strcasecmp( "the", name )
    	&& strcasecmp( "a", name )
    	&& strcasecmp( "an", name ) ) {
      trivial = false;
      score += ( partial ? 1 : 2 );
    }
  }
}


// Single word compare.
// 0: no match
// 1: partial match
// 2: full match
static const char *compare( const char *s1, const char *s2, int cmd, int len )
{
  static char buf [ MAX_INPUT_LENGTH ];
  const char *s3 = s2;

  for( const char *s = s1; *s && *s != ' '; ++s, ++s3 )
    if( toupper( *s ) != toupper( *s3 ) )
      return 0;

  if( cmd >= 0 && len < 0 ) {
    // Matching of built-in commands.
    one_word( s2, buf );
    int pos = find_command( 0, buf, true );
    if( pos >= 0 ) {
      return ( pos == cmd ) ? s2 : 0;
    }
  }

  if( len < 0 )
    len = 3;

  if( s3-s2 < len && *s3 && *s3 != ' ' ) {
    return 0;
  }
  
  return( !*s3 || *s3 == ' ' ? s2 : s3 );
}


// Single word (s1) vs. compound list (s2) match.
// 0: no match
// 1: partial match
// 2: full match
const char *member( const char* s1, const char* s2, int cmd )
{
  skip_spaces( s2 );

  if( !*s1 )
    return *s2 ? 0 : s2;
  
  int len = -1;
  const char *i = 0;
  
  while( *s2 ) {
    if( isdigit( *s2 ) ) {
      len = atoi( s2 );
    } else {
      if( const char *s3 = compare( s1, s2, cmd, len ) ) {
	if( s3 == s2 ) {
	  // Exact match.
	  return s2;
	}
	// Partial match.
	i = s2;
      }
      len = -1;
    }
    for( ; *s2 && *s2 != ' '; ++s2 );
    for( ; *s2 == ' '; ++s2 );
  }
  
  return i ? i : 0;
}
 

/*
// Single word compare.
// 0: no match
// 1: partial match
// 2: full match
static int subset_compare( const char* s1, const char* s2, int len )
{
  const char *s3  = s2;
  //  int         pos;

  for( ; *s1 && *s1 != ' '; ++s1, ++s2 )
    if( toupper( *s1 ) != toupper( *s2 ) )
      return 0;

  if( s2-s3 < len && *s2 && *s2 != ' ' )
    return 0;
  
  return( !*s2 || *s2 == ' ' ? 2 : 1 );
}


// Single word (s1) vs. compound list (s2) match.
// 0: no match
// 1: partial match
// 2: full match
static int subset_member( const char* s1, const char* s2 )
{
  int i = 0;
  
  skip_spaces( s2 );

  if( !*s1 )
    return( !*s2 );
  
  while( *s2 ) {
    switch( subset_compare( s1, s2, 3 ) ) {
    case 2 : return 2;
    case 1 : i = 1;
    }
    for( ; *s2 && *s2 != ' '; ++s2 );
    for( ; *s2 == ' '; ++s2 );
  }
  
  return i;
}
 

// Only used by custom.cc.
// 100 - #words in item name => full match all words
// 50 - #words in item name => partial match (some incomplete words)
int subset( const char *s1, const char *s2 )
{
  int i = 100;

  while( *s1 ) {
    switch( subset_member( s1, s2 ) ) {
    case 0 : return 0;
    case 1 : i = 50;
    }
    for( ; *s1 && *s1 != ' '; ++s1 );
    for( ; *s1 == ' '; ++s1 );
  }

  return i - word_count( s2 );
}
*/


bool word_match( const char*& argument, const char *word )
{
  const char *str = argument;

  while( *word ) {
    if( tolower( *word++ ) != tolower( *str++ ) ) {
      return false;
    }
  }

  if( isalnum( *str )
      || *str == '_' )
    return false;

  skip_spaces( str );
  argument = str;

  return true;
}


/*
 *   STRING TYPES
 */


bool is_number( const char *arg )
{
  if( !*arg )
    return false;

  if( ( *arg == '+' ) || ( *arg == '-' ) )
    ++arg;
  
  for( ; *arg && *arg != ' '; ++arg )
    if( !isdigit( *arg ) )
      return false;

  return true;
}


bool isperiod( char letter )
{
  switch( letter ) {
  case '.' :
  case '?' :
  case '!' :
    return true;
  }
  
  return false;
}


bool isvowel( char letter )
{
  switch( toupper( letter ) ) {
    case 'A' :
    case 'E' :
    case 'I' :
    case 'O' :
    case 'U' :
      return true;
    }

  return false;
}


/*
 *   PARSE INPUT
 */


bool contains_word( const char*& argument, const char* word, char* arg )
{
  const char*  s1  = argument;
  const char*  s2;

  while ( *s1 ) {
    if( exact_match( s2 = s1, word ) ) {
      for( ; s1 != argument && *(s1-1) == ' '; s1-- );
      memcpy( arg, argument, s1-argument );
      arg[s1-argument] = '\0';
      argument = s2;
      return true;
    } 
    for( ; *s1 && *s1 != ' '; s1++ );
    for( ; *s1 == ' '; s1++ );
  }

  return false;
}


bool two_argument( const char*& argument, const char* word, char* arg )
{
  int i;

  if( ( i = word_count( argument ) ) < 2 )
    return false;

  if( i == 2 ) {
    argument = one_argument( argument, arg );
    return true;
  }
  
  return contains_word( argument, word, arg )
    && *argument
    && *arg;
}
 

const char *one_argument( const char *argument, char *arg_first )
{
  skip_spaces( argument );
  
  char cEnd = ' ';
  if( *argument == '\'' || *argument == '"' ) {
    cEnd = *argument++;
    if( *argument == ' ' || !*argument ) {
      --argument;
      cEnd = ' ';
    }
  }
  
  while( *argument ) {
    if( *argument == cEnd ) {
      ++argument;
      break;
    }
    *arg_first++ = *argument++;
  }
  
  *arg_first = '\0';
 
  skip_spaces( argument );
  
  return argument;
}


// obj -> 1st obj (1)
// n*obj -> n objs (-N)
// n.obj -> nth obj (N)
// all.obj, all*obj -> all objs (INT_MIN)
int smash_argument( char *tmp, const char *& argument )
{
  int number = 0;

  skip_spaces( argument );
  
  if( !strncasecmp( argument, "all", 3 ) ) {
    if( !argument[3] ) {
      *tmp = '\0';
      argument += 3;
      return INT_MIN;
    }
    if( argument[3] == '*' || argument[3] == '.' ) {
      argument += 4;
      number = INT_MIN;
    }
  }
  
  if( number == 0 ) {
    if( !isdigit( *argument ) ) {
      number = 1;
    } else { 
      for( ; *argument && isdigit( *argument ); ++argument ) {
        number = 10*number+*argument-'0';
      }
      if( *argument == '*' && number != 1 ) {
        number *= -1;
        ++argument;
      } else if( *argument == '.' ) {
        ++argument;
      }
    }
  }
  
  for( ; *argument; ++argument ) {
    if( *argument == ',' ) {
      ++argument;
      skip_spaces( argument );
      break;
    }
    *tmp++ = ( *argument == '.' ? ' ' : *argument );
  }
  
  *tmp = '\0';

  return number;
}


/*
 *   FORMATTING STRINGS
 */


char *capitalize( char* arg )
{
  char *letter;
  
  if( *arg != '' ) {
    *arg = toupper( *arg );
    return arg;
  }
  
  for( letter = arg; *letter; ) 
    if( *letter++ == 'm' ) {
      *letter = toupper( *letter );
      return arg;
    }
  
  bug( "Capitalize: Missing end of escape code?" );
  bug( "-- arg = %s", arg );
  
  return arg;
}


char* capitalize_words( const char* argument )
{
  char*  tmp  = static_string( );
  int      i; 

  *tmp = toupper( *argument );

  for( i = 1; argument[i]; i++ ) 
    tmp[i] = ( argument[i-1] == ' ' ? toupper( argument[i] )
      : argument[i] );

  tmp[i] = '\0';

  return tmp;
}


void smash_spaces( char *tmp )
{
  while( *tmp ) {
    if( *tmp == ' ' )
      *tmp = '_';
    ++tmp;
  }
  /*
  for( size_t i = 0; i < strlen( tmp ); i++ )
    if( tmp[i] == ' ' )
      tmp[i] = '_';
  */
}


void add_spaces( char* tmp, int i )
{
  if( i < 0 ) {
    roach( "Add_Spaces: Number to add negative." );
    return;
  }

  tmp += strlen( tmp );

  for( ; i > 0; i-- ) 
    *tmp++ = ' ';

  *tmp = '\0';
} 


int word_count( const char* s )
{
  int i;
  
  for( ; *s == ' '; s++ );
  
  for( i = 0; *s; i++ ) {
    for( ; *s && *s != ' '; s++ );
    for( ; *s == ' '; s++ );
  }
  
  return i;
}


text trunc( const char *string, size_t length )
{
  const size_t l = strlen( string );

  if( l > length ) {
    text t( string, length );
    t.replace( length-3, 3, 3, '.' );
    return t;
  }

  return text( string, l );
}


static void skip_punct( const char*& arg, const char *word )
{
  while( isspace( *arg ) || ispunct( *arg ) ) {
    skip_spaces( arg );
    if( word && ispunct( *word ) )
      return;
    while( ispunct( *arg ) )
      ++arg;
  }
}


bool search_text( const char *text, const char *phrase )
{
  const char *word = phrase;

  while( true ) {
    const char *list = text;
    skip_punct( list, word );
    size_t match = 0;
    while( *list ) {
      if( !*word ) {
	if( match < 3
	    && *list
	    && !isspace( *list ) ) {
	  // Short substring.
	  // Treat like a mismatch.
	  while( *list && !isspace( *list ) )
	    ++list;
	  word = phrase;
	  match = 0;
	  skip_punct( list, word );
	  continue;
	}
	return true;
      }
      if( isspace( *word ) ) {
	// Matched word.
	if( match < 3
	    && *list
	    && !isspace( *list ) ) {
	  // Short substring.
	  // Treat like a mismatch.
	  while( *list && !isspace( *list ) )
	    ++list;
	  word = phrase;
	  match = 0;
	  skip_punct( list, word );
	  continue;
	}
	skip_spaces( word );
	phrase = word;
	break;
      }
      if( tolower( *word ) != tolower( *list ) ) {
	// Mismatched word.
 	while( *list && !isspace( *list ) )
	  ++list;
	word = phrase;
	match = 0;
	skip_punct( list, word );
	continue;
      }
      ++word;
      ++list;
      ++match;
    }
    skip_spaces( word );
    if( !*list )
      return !*word;
  }
}


const char *one_word( const char *phrase, char *word )
{
  while( *phrase && !isspace( *phrase ) ) {
    *word++ = *phrase++;
  }

  *word = '\0';

  skip_spaces( phrase );
  return phrase;
}
