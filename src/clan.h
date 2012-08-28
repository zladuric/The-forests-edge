#ifndef tfe_clan_h
#define tfe_clan_h

/*
 *   TITLES
 */


#define TITLE_SET_FLAGS       0
#define TITLE_EDIT_CHARTER    1
#define TITLE_RECRUIT         2
#define TITLE_REMOVE_NOTES    3
#define MAX_TITLE             4


class Title_Data
{
 public:
  const char*          name;
  int           flags  [ 2 ];
  pfile_data*   pfile;

  Title_Data( const char *s1, pfile_data* p1 )
    : pfile(p1)
  {
    record_new( sizeof( Title_Data ), -MEM_CLAN );
    name = alloc_string( s1, MEM_CLAN );
    vzero( flags, 2 );
  }
  
  Title_Data( )
    : name(empty_string), pfile(0)
  {
    record_new( sizeof( Title_Data ), -MEM_CLAN );
    vzero( flags, 2 );
  }
  
  ~Title_Data( ) {
    record_delete( sizeof( Title_Data ), -MEM_CLAN );
    free_string( name, MEM_CLAN );
  }
};


title_data*    get_title         ( pfile_data* );


class Title_Array
{
 public:
  int           size;
  title_data**  list;
  
  Title_Array( )
    : size(0), list(0)
  {
    record_new( sizeof( Title_Array ), -MEM_CLAN );
  }
  
  ~Title_Array( )
  {
    record_delete( sizeof( Title_Array ), -MEM_CLAN );
    if( size > 0 ) 
      delete [] list;
  }
};


/*
 *   CLAN CLASS
 */


#define CLAN_APPROVED         0
#define CLAN_KNOWN            1
#define CLAN_PUBLIC           2
#define CLAN_IMMORTAL         3
#define MAX_CLAN_FLAGS        4


extern clan_data**    clan_list;
extern int             max_clan; 


class Clan_Data
{
 public:
  const char        *name;
  char            *abbrev;
  const char     *charter;
  int               flags  [ 2 ];
  int           min_level;
  int             classes;
  int               races;
  int          alignments;
  int               sexes;
  int                date;
  bool           modified;
  title_array      titles;
  pfile_array     members;
  note_data**   note_list;
  int            max_note;

  Clan_Data( const char* );
  ~Clan_Data( );

  const char* Name( ) const {
    return name == empty_string ? abbrev : name;
  }
};


// For macros.h:pntr_search().
inline const char *name( const clan_data *clan )
{
  return clan->abbrev;
}


inline bool same_clan( const char_data* c1, const char_data* c2 )
{
  return( c1->pcdata->pfile->clan
    && c1->pcdata->pfile->clan == c2->pcdata->pfile->clan );
}


inline bool knows_members( char_data* ch, const clan_data* clan )
{
  /*
    ch->pcdata->pfile could be zero if it's a socket doing a who-list lookup.
  */

  return( ( ch
	    && ch->pcdata->pfile
	    && ch->pcdata->pfile->clan == clan )
	  || ( is_set( clan->flags, CLAN_APPROVED )
	       && is_set( clan->flags, CLAN_PUBLIC ) ) 
	  || has_permission( ch, PERM_CLANS ) );
}


void        add_member      ( clan_data*, pfile_data* );
void        remove_member   ( pfile_data* );
void        remove_member   ( player_data* );
void        save_clans      ( clan_data* = 0 );
void        load_clans      ( void );
clan_data*  find_clan       ( char_data*, const char * );


#endif // tfe_clan_h
