#ifndef tfe_string_h
#define tfe_string_h

#include <string>
typedef std::basic_string <char> text;


extern char *const empty_string;


int           strcasecmp     ( const char*, const char* );
int           strncasecmp    ( const char*, const char*, int );
int           strncmp        ( const char*, const char*, int );
int           rstrcasecmp    ( const char*, const char* );
int           rstrncasecmp   ( const char*, const char*, int );
char*         strcat         ( char*, const char* );

bool          matches        ( const char*&, const char*, size_t = 1 );
bool          exact_match    ( const char*&, const char* );
bool          fmatches       ( const char*, const char*, size_t = 1 );
bool          number_arg     ( const char*&, int&, bool = false );

bool          contains_word  ( const char*&, const char*, char* );
bool          two_argument   ( const char*&, const char*, char* );
const char*   one_argument   ( const char*, char* );

int           smash_argument ( char*, const char*& );

const char    *member        ( const char*, const char*, int = -1 );

bool          isvowel           ( char letter );
int           is_name           ( const char *, const char*, bool = false );
const char*   break_line        ( const char*, char*, int );
const char*   word_list         ( const char**, int, bool = true );
void          smash_spaces      ( char* );
char*         capitalize        ( char* );
char*         capitalize_words  ( const char* );
void          add_spaces        ( char*, int );
const char   *separate          ( const char*, bool );

bool          search_text       ( const char*, const char* );

char         *subtract        ( const char*, const char* );

bool          word_match      ( const char*&, const char* );

const char   *one_word        ( const char*, char* );


/*
 *  INLINE UTILITY ROUTINES
 */


int word_count( const char* s );


text trunc( const char* string, size_t length );


inline void skip_spaces( char*& arg )
{
  while( isspace( *arg ) )
    ++arg;

  // Do NOT do this, causes compile() to crash:
  //  if( !*arg )
  //    arg = empty_string;
}


inline void skip_spaces( const char*& arg )
{
  while( isspace( *arg ) )
    ++arg;

  // Do NOT do this, causes compile() to crash:
  //  if( !*arg )
  //    arg = empty_string;
}


/*
inline bool matches( const char*& argument, const char* word, bool exact )
{
  return( exact ? exact_match( argument, word )
	  : matches( argument, word ) );
}
*/



/*
 *   STATIC STRINGS
 */

char *static_string ( const char *msg = 0 );
text& static_text ( const char *msg = 0 );


#endif // tfe_string_h
