#ifndef tfe_player_h
#define tfe_player_h

/*
 *   ALIAS HEADERS
 */


char *subst_alias     ( link_data*, char* );


class alias_data
{
public:
  char *abbrev;
  char *command;
  char length;

  alias_data       ( const char *, const char * );
  ~alias_data      ( );

  friend char* name( alias_data* alias ) {
    return alias->abbrev;
  }
};


/*
 *   ATTRIBUTES
 */


extern index_data        fame_index  [];
extern index_data       piety_index  [];
extern const index_data  reputation_index  [];


/*
 *   PFILE ROUTINES
 */


class Vote_Data
{
public:
  Vote_Data( )
    : text( empty_string ), serial( 0 )
  {
    record_new( sizeof( Vote_Data ), MEM_VOTE );
  }
  ~Vote_Data( )
  {
    record_delete( sizeof( Vote_Data ), MEM_VOTE );
    free_string( text, MEM_VOTE );
  }

  const char *text;
  int serial;
  static int max_serial;
};

#define MAX_VOTE 10

extern Vote_Data votes [ MAX_VOTE ];


/*
    Data needed even when player is offline
*/

class Pfile_Data: public Save_Data
{
public:
  char*              name;
  const char*         pwd;
  account_data*   account;
  char*         last_host;
  pfile_data*        vote  [ MAX_VOTE ];
  int               flags  [ 2 ];
  int            settings;
  int               trust; 	// unsigned char?
  int              deaths;
  int               quest;
  int               level;	// unsigned char?
  int              remort;	// unsigned char?
  int                race;	// unsigned char?
  int                clss;	// unsigned char?
  int                 sex;	// unsigned char?
  int           alignment;	// unsigned char?
  int            religion;
  int              bounty;
  int             last_on;
  int           last_note;
  int             created;
  int               ident;
  int              serial;
  int                 exp;
  int                rank;
  int             guesses;   	// unsigned char?
  clan_data*         clan;
  const char*    homepage;
  bool      home_modified;
  int          permission  [ 2 ];
  obj_array       corpses;
  obj_array        caches;
  auction_array   auction;

  Pfile_Data   ( const char* );
  ~Pfile_Data  ( );

  void Save ( bool = true );

  const char *Him_Her( ) const;

  bool Filtering( char_data* ) const;
  bool Befriended( char_data* ) const;

  // Used by search macros.
  friend char *name( pfile_data *pfile ) {
    return pfile->name;
  }
};


class Pfile_Array
{
public:
  int           size;
  pfile_data**  list;

  Pfile_Array( ) {
    size = 0;
    list = 0;
  }
  
  ~Pfile_Array( ) {
    if( size > 0 )
      delete [] list;
  }
};


extern pfile_data*     ident_list  [ MAX_PFILE ];
extern pfile_data**    pfile_list;
extern pfile_data**     site_list;
extern int              max_pfile;
extern int           site_entries;
extern int         record_players;
extern int             max_serial;


inline pfile_data* get_pfile( int i )
{
  if( i < 0 || i >= MAX_PFILE )
    return 0;

  return ident_list[i];
}


void          purge                  ( player_data* );
player_data*  find_player            ( const pfile_data* );
pfile_data*   find_pfile             ( const char*, char_data* = 0 );
pfile_data*   find_pfile_exact       ( const char* );
pfile_data*   find_pfile_substring   ( const char* );
pfile_data*   player_arg             ( const char *& );
int           site_search            ( const char* );
void          extract                ( pfile_data*, link_data* = 0 );
void          add_list               ( pfile_data**&, int&, pfile_data* );
void          remove_list            ( pfile_data**&, int&, pfile_data* );
void   modify_pfile       ( char_data *ch );


/* 
 *   RECOGNIZE
 */


class Recognize_Data
{
public:
  int    size;
  int*   list;

  Recognize_Data    ( int );
  ~Recognize_Data   ( );
};


void   remove                ( Recognize_Data*&, int );
void   reconcile_recognize   ( char_data* );
int    search                ( const Recognize_Data*, int );
bool   consenting            ( char_data*, char_data*,
                               const char* = empty_string );

/*
 *   PCDATA

     Data needed by players and switched mobs
 */


class Pc_Data
{
public:
  char*            title;
  const char*      tmp_keywords;
  const char*      tmp_short;
  const char*      buffer;
  const char*      paste;
  char*            prompt;
  int              message;
  int              mess_settings; 
  int              iflag  [ 2 ];
  int              trust;	// unsigned char?
  int              clss;	// unsigned char?
  int              religion;
  int              color [ SAVED_COLORS ];
  int              terminal;	// unsigned char?
  unsigned short   lines;
  unsigned short   columns;
  int              cflags [ MAX_CFLAG ];
  int              speaking;
  int              piety;
  int              practice;
  int              prac_timer;
  int              quest_pts;
  int              quest_flags [ MAX_QUEST ];
  int              level_hit;
  int              level_mana;
  int              level_move;
  int              max_level; 	// unsigned char?
  int              mod_age;
  int              wimpy;
  note_data        *mail;
  note_data        *mail_edit;
  pfile_data*      pfile;
  Recognize_Data*  recognize;
  char             burden;
  const char  *dictionary;
  const char     *journal;
  
  Pc_Data ();
  ~Pc_Data ();
};


/*
 *   REQUESTS
 */


bool  remove      ( request_array&, char_data* );


extern request_array    request_imm;
extern request_array    request_app;


#endif // tfe_player_h
