#ifndef tfe_general_h
#define tfe_general_h


extern int      boot_stage;
extern bool       tfe_down;
extern bool   in_character;


/*
 *   INDEX ROUTINES
 */


class index_data
{
 public:
   const char*  singular;
   const char*    plural;
   int             value;
};


const char*  lookup     ( const index_data*, int, bool = false );


/*
 *   INFO ROUTINES
 */


void   info   ( int, const char*, int, const char*, int,
		int = 3, char_data* = 0, clan_data* = 0,
		pfile_data* = 0, thing_array* = 0 );


/*
 *   NOTE/MAIL ROUTINES
 */


void   recent_notes     ( char_data* );
void   mail_message     ( char_data* );


/*
 *   STRING FUNCTIONS
 */


class String
{
public:
  char*      text;
  size_t     length;
  
  String( const char *msg = 0, bool split = false ) {
    if ( !msg ) {
      length = 0;
      text = new char[1];
      *text = '\0';
    } else {
      if( split ) {
	length = 0;
	const char *s = msg;
	while( *s != '\n' && *s != '\r' && *s ) {
	  ++s;
	  ++length;
	}
	while( *s == '\n' || *s == '\r' ) {
	  ++s;
	  ++length;
	}
	text = new char [ length+1 ];
	memcpy( text, msg, length );
	*(text+length) = '\0';
      } else {
	length = strlen( msg );
	text = new char [ length+1 ];
	memcpy( text, msg, length+1 );
      }
    }
  }
  
  ~String( ) {
    delete [] text;
  }

  bool newline( ) const {
    return length > 0 && ( text[length-1] == '\n' || text[length-1] == '\r' );
  }

  /*
  const String& operator = ( const char *msg ) {
    if( !msg ) {
      delete [] text;
      length = 0;
      text = new char[1];
      *text = '\0';
    } else {
      char *old_text = text;
      length = strlen( msg );
      text = new char [ length+1 ];
      memcpy( text, msg, length+1 );
      delete [] old_text;
    }
  }
  */
};


inline const char* name( char* word )
{
  return word;
} 


inline const char* name( const char* word )
{
  return word;
}



/*
 *   TRUST
 */


int    get_trust      ( const char_data* );


#endif // tfe_general_h
