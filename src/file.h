#ifndef tfe_file_h
#define tfe_file_h

/*
 *   FILE LOCATIONS
 */


#define ACTION_SWAP_DIR    "../var/swap/action/"
#define AREA_DIR           "../area/"
#define AREA_PREV_DIR      "../prev/area/"
#define BACKUP_DIR         "../reimb/"
#define CLAN_DIR           "../clans/"
#define CLAN_NOTE_DIR      "../notes/clans/"
#define FILES_DIR          "../files/"
#define FILES_PREV_DIR     "../prev/files/"
#define IMMORTAL_LOG_DIR   "../logs/immortal/"
#define LOST_FOUND_DIR     "../lost+found/"
#define MOB_LOG_DIR        "../logs/mob/"
#define MAIL_DIR           "../mail/"
#define NOTE_DIR           "../notes/"
#define OBJ_LOG_DIR        "../logs/object/"
#define PLAYER_DIR         "../player/"  
#define PLAYER_LOG_DIR     "../logs/player/" 
#define PLAYER_PREV_DIR    "../prev/player/"
#define HTML_CLAN_DIR      "../public_html/clans/"
#define HTML_DIR           "../public_html/"
#define HTML_HELP_DIR      "../public_html/help/"
#define ROOM_DIR           "../rooms/"
#define ROOM_PREV_DIR      "../prev/rooms/" 
#define ROOM_LOG_DIR       "../logs/room/"
#define ROOM_SWAP_DIR      "../var/swap/room/"
#define TABLE_DIR          "../tables/"
#define TABLE_PREV_DIR     "../prev/tables/"
#define TEMP_DIR           "../var/tmp"

#define ACCOUNT_FILE     "accounts"
#define AREA_LIST        "area.lst"
#define BADNAME_FILE     "badname.txt"
#define BANNED_FILE      "banned.txt"
#define BUG_FILE         "../files/bugs.txt"
#define CUSTOM_FILE      "custom.txt"
#define DICTIONARY_FILE  "dictionary.txt"
#define GODLOCK_FILE     "../var/control/godlock"
#define HELP_FILE        "help.are"
#define HTML_INDEX_FILE  "index.html"
#define HTML_PLAYER_FILE "players.html"
#define LIST_FILE        "Lists"
#define MOB_FILE         "mob.mob"
#define OBJECT_FILE      "obj.obj"
#define PANIC_FILE       "../var/control/panic"
#define QUEST_FILE       "quest"
#define REBOOT_FILE      "../var/control/reboot"
#define REMORT_FILE      "remort.txt"
#define RTABLE_FILE      "rtables"
#define SHOP_FILE        "shop.dat"
#define SHUTDOWN_FILE    "../var/control/shutdown"
#define TRAINER_FILE     "trainer.dat"
#define WIZLOCK_FILE     "../var/control/wizlock"
#define WORLD_FILE       "world"


/*
 *   BUG FUNCTIONS
 */


void   panic       ( const char* );
void   bug         ( int, const char* );


#define BUG_APHID   3
#define BUG_BEETLE  2
#define BUG_ROACH   1


inline void aphid  ( const char* text )   {  bug( BUG_APHID, text ); } 
inline void beetle ( const char* text )   {  bug( BUG_BEETLE, text ); } 
inline void roach  ( const char* text )   {  bug( BUG_ROACH, text ); }
inline void bug    ( const char* text )   {  bug( BUG_BEETLE, text ); }


template < class T >
void aphid( const char* text, T item )
{
  char   tmp  [ TWO_LINES ];
 
  snprintf( tmp, TWO_LINES, text, tostring( item, 0 ) );
  
  if( *text == '%' )
    *tmp = toupper( *tmp );

  bug( BUG_APHID, tmp );
}


template < class T >
void roach( const char* text, T item )
{
  char   tmp  [ TWO_LINES ];
 
  snprintf( tmp, TWO_LINES, text, tostring( item, 0 ) );

  if( *text == '%' )
    *tmp = toupper( *tmp );

  bug( BUG_ROACH, tmp );
}


template < class S, class T >
void roach( const char* text, S item1, T item2 )
{
  char   tmp  [ TWO_LINES ];
 
  snprintf( tmp, TWO_LINES, text, tostring( item1, 0 ), tostring( item2, 0 ) );
  
  if( *text == '%' )
    *tmp = toupper( *tmp );

  bug( BUG_ROACH, tmp );
}


template < class S, class T, class U >
void roach( const char* text, S item1, T item2, U item3 )
{
  char tmp [ TWO_LINES ];
 
  snprintf( tmp, TWO_LINES, text,
	    tostring( item1, 0 ),
	    tostring( item2, 0 ),
	    tostring( item3, 0 ) );
  
  if( *text == '%' )
    *tmp = toupper( *tmp );

  bug( BUG_ROACH, tmp );
}


template < class S, class T, class U, class V >
void roach( const char* text, S item1, T item2, U item3, V item4 )
{
  char tmp [ TWO_LINES ];
 
  snprintf( tmp, TWO_LINES, text,
	    tostring( item1, 0 ),
	    tostring( item2, 0 ),
	    tostring( item3, 0 ),
	    tostring( item4, 0 ) );
  
  if( *text == '%' )
    *tmp = toupper( *tmp );

  bug( BUG_ROACH, tmp );
}


template < class T >
void bug( const char* text, T item )
{
  char   tmp  [ TWO_LINES ];
 
  snprintf( tmp, TWO_LINES, text, tostring( item, 0 ) );

  if( *text == '%' )
    *tmp = toupper( *tmp );

  bug( tmp );
}


template < class S, class T >
void bug( const char* text, S item1, T item2 )
{
  char   tmp  [ TWO_LINES ];
 
  snprintf( tmp, TWO_LINES, text, tostring( item1, 0 ), tostring( item2, 0 ) );

  if( *text == '%' )
    *tmp = toupper( *tmp );

  bug( tmp );
}


template < class S, class T, class U >
void bug( const char* text, S item1, T item2, U item3 )
{
  char   tmp  [ TWO_LINES ];
 
  snprintf( tmp, TWO_LINES, text,
	    tostring( item1, 0 ), tostring( item2, 0 ), tostring( item3, 0 ) );

  if( *text == '%' )
    *tmp = toupper( *tmp );

  bug( tmp );
}


template < class S, class T, class U, class V >
void bug( const char* text, S item1, T item2, U item3, V item4 )
{
  char   tmp  [ TWO_LINES ];
 
  snprintf( tmp, TWO_LINES, text,
	    tostring( item1, 0 ), tostring( item2, 0 ),
	    tostring( item3, 0 ), tostring( item4, 0 ) );

  if( *text == '%' )
    *tmp = toupper( *tmp );

  bug( tmp );
}


template < class T >
void panic( const char* text, T item )
{
  char   tmp  [ TWO_LINES ];
 
  snprintf( tmp, TWO_LINES, text, item );
  
  if( *text == '%' )
    *tmp = toupper( *tmp );

  panic( tmp );
}


template < class S, class T >
void panic( const char* text, S item1, T item2 )
{
  char   tmp  [ TWO_LINES ];
 
  snprintf( tmp, TWO_LINES, text, item1, item2 );
  
  if( *text == '%' )
    *tmp = toupper( *tmp );

  panic( tmp );
}


/*
 *   BOOT CONTROL
 */

void quit ( int status );

void shutdown( const char *, const char * = 0 );
void reboot( const char *, const char * = 0 );

bool create_control_file( const char *, const char *, const char * = 0 );
bool exist_control_file( const char * );
void delete_control_file( const char * );

void check_panic( );


/*
 *   FILE ROUTINES
 */


FILE*   open_file         ( const char*, const char*, const char*,
                            bool = false );
FILE*   open_file         ( const char*, const char*,
                            bool = false ); 
bool    delete_file       ( const char*, const char*, bool = true );
bool    rename_file       ( const char*, const char*,
                            const char*, const char* );

char    fread_letter      ( FILE *fp );
int     fread_number      ( FILE *fp );
unsigned fread_unsigned   ( FILE *fp );
void    fread_to_eol      ( FILE *fp );
//char*   fread_block       ( FILE* );
char*   fread_string      ( FILE*, int );
char*   fread_word        ( FILE* );

void    fwrite_string     ( FILE*, const char* );

void write_all  ( bool = false );


/*
 *   HELP ROUTINES
 */


bool   save_help          ( );
  

/*
 *   WORLD ROUTINES
 */


bool   save_world         ( );
void   load_world         ( );  

/*
 *   QUITTING ROUTINES
 */


void   forced_quit        ( player_data*, bool = false );


/*
 *   WEBPAGE ROUTINES
 */


//void w3_help  ( void );
//void w3_who   ( void );


#endif // tfe_file_h
