#ifndef tfe_room_h
#define tfe_room_h

/*
 *   AREA HEADER
 */


#define AREA_OPEN           0
#define AREA_WORTHLESS      1
#define AREA_ABANDONED      2
#define AREA_PROGRESSING    3
#define AREA_PENDING        4
#define AREA_BLANK          5
#define AREA_IMMORTAL       6
#define AREA_DELETED        7
#define MAX_AREA_STATUS     8


extern  area_data*      area_list;
extern  const char*   area_status  [ MAX_AREA_STATUS ];

area_data *load_area      ( const char* );

set_func set_area_file;


class area_data
{
public:
  area_data*         next;
  room_data*   room_first;
  const char*        file;
  const char*     creator;
  const char*        name;
  const char*        help;
  bool           modified;
  bool               used;
  bool             loaded;
  bool              dirty;
  bool               seen;
  bool         act_loaded;
  bool          act_dirty;
  bool           act_used;
  int             nplayer;
  int               level;	// unsigned char?
  int          reset_time;
  int                 age;	// unsigned char?
  int              status;	// unsigned char?
  int             climate;	// unsigned char?
  int              forage;	// unsigned char?

  double temperature;
  double t_prev [ WEATHER_FRONTS ];
  double t_goal [ WEATHER_FRONTS ];

  double humidity;
  double h_prev [ WEATHER_FRONTS ];
  double h_goal [ WEATHER_FRONTS ];

  double c_prev [ WEATHER_FRONTS ];
  double c_goal [ WEATHER_FRONTS ];

  area_data( const char * );
  ~area_data( );

  void load_rooms( FILE *fp );

  bool Save( bool = false );

  void update_forage( );

  bool save_text( );
  void clear_text( );
  bool load_text( );

  bool save_actions( );
  void clear_actions( );
  bool load_actions( );

  void set_weather( );
};


room_data*   get_room_index    ( int, bool = false );


/*
 *   EXIT HEADER
 */


#define MAX_DOOR                    6


#define EX_ISDOOR                   0
#define EX_CLOSED                   1
#define EX_LOCKED                   2
#define EX_SECRET                   3
#define EX_PICKPROOF                4
#define EX_NO_SHOW                  5
#define EX_NO_OPEN                  6
#define EX_RESET_CLOSED             7      
#define EX_RESET_LOCKED             8
#define EX_RESET_OPEN               9
#define EX_REQUIRES_CLIMB          10
#define EX_SEARCHABLE              11
#define EX_PLURAL                  12
#define EX_NO_SCAN                 13
#define EX_NO_MOB                  14
#define EX_NO_PASS                 15
#define EX_NO_BLINK                16
#define EX_NO_FLEE                 17
#define EX_NO_PATH                 18
#define MAX_DFLAG                  19


extern const default_data open_msg [];
extern const default_data close_msg [];
extern const default_data lock_door_msg [];
extern const default_data unlock_door_msg [];
extern const default_data knock_door_msg [];


class Exit_Data : public visible_data
{
 public:
  room_data*    to_room;
  int           exit_info;
  int           save_info;
  int           key;
  unsigned char size;
  unsigned char direction;
  unsigned char light;
  int           affected_by;
  const char*   name;
  const char*   keywords;

  Exit_Data     ( );
  virtual ~Exit_Data    ( );

  virtual int Type( ) const
    { return EXIT_DATA; }

  virtual const char*   Name       ( const char_data* = 0, int = 1, bool = false ) const;
  virtual const char*   Keywords   ( char_data* );
  virtual bool          Seen       ( const char_data* ) const;
  virtual void          Look_At    ( char_data* );
};


int         direction_arg      ( const char *& argument );
int         exits_prompt       ( char*, char_data*, int color = COLOR_DEFAULT );
void        read_exits         ( FILE*, room_data*, int );
void        autoexit           ( char_data* );
const char *exit_verb          ( char_data*, exit_data*);
obj_data*   has_key            ( char_data*, int );

exit_data*  exit_direction     ( room_data*, int );
exit_data*  reverse            ( exit_data* );

//exit_data*  random_exit          ( room_data* );
//exit_data*  random_open_exit     ( char_data*, room_data* );
exit_data*  random_movable_exit  ( char_data*, room_data* = 0,
				   bool = false, bool = false, bool = false );

bool        open_door          ( char_data*, exit_data* );
bool        close_door         ( char_data*, exit_data* );
bool        lock_door          ( char_data*, exit_data* );
bool        unlock_door        ( char_data*, exit_data* );
bool        pick_door          ( char_data*, exit_data* );
void        knock_door         ( char_data*, exit_data* );


/*
 *   LOCATIONS TYPES
 */


#define LOC_NOT_INDOORS     0
#define LOC_NOT_OUTDOORS    1
#define LOC_SUNLIGHT        2
#define LOC_FULLMOON        3
#define LOC_FOREST          4
#define LOC_NOT_UNDERWATER  5
#define LOC_NOT_MOUNTED     6
#define LOC_NOT_FIGHTING    7
#define LOC_UNDERGROUND     8
#define LOC_ABOVEGROUND     9
#define LOC_PERFORM        10
#define MAX_LOCATION       11


bool allowed_location   ( char_data*, int*, const char*, const char* );


extern flag_data location_flags;


/*
 *   ROOM HEADER
 */


#define ROOM_LIMBO                  2
#define ROOM_CHAT                   9
#define ROOM_CHIIRON_TEMPLE       904
#define ROOM_PRISON                13
#define ROOM_DEATH              33751


#define RFLAG_LIT                   0
#define RFLAG_SAFE                  1
#define RFLAG_INDOORS               2
#define RFLAG_NO_MOB                3
#define RFLAG_NO_RECALL             4
#define RFLAG_NO_MAGIC              5
#define RFLAG_NO_AUTOSCAN           6
#define RFLAG_ALTAR                 7
#define RFLAG_ALTAR_GOOD            8
#define RFLAG_ALTAR_NEUTRAL         9
#define RFLAG_ALTAR_EVIL           10
#define RFLAG_BANK                 11
#define RFLAG_SHOP                 12
#define RFLAG_PET_SHOP             13
#define RFLAG_OFFICE               14
#define RFLAG_NO_PRAY              15
#define RFLAG_SAVE_ITEMS           16
#define RFLAG_UNDERGROUND          17
#define RFLAG_AUCTION_HOUSE        18
#define RFLAG_RESET0               19
#define RFLAG_RESET1               20
#define RFLAG_RESET2               21
#define RFLAG_STATUS0              22
#define RFLAG_STATUS1              23
#define RFLAG_STATUS2              24
#define RFLAG_NO_MOUNT             25
#define RFLAG_ARENA                26
#define RFLAG_NO_REST              27
#define RFLAG_NO_PKILL             28
#define RFLAG_ALTAR_LAW            29
#define RFLAG_ALTAR_NEUTRAL2       30
#define RFLAG_ALTAR_CHAOS          31
#define MAX_RFLAG                  32


#include "track.h"


class Room_Data : public thing_data, public Save_Data
{
  friend class area_data;
public:
  exit_array           exits; 
  room_data*            next;
  area_data*            area;
  action_data*        action;
  reset_data*          reset;
  track_data*          track;
  int                   vnum;
  //  int             room_flags;
  //  int             temp_flags;
  int             room_flags [2];
  int            sector_type;	// unsigned char?
  int               distance;	// char?
  int                   size;	// unsigned char?
  const char*           name;

  Room_Data( area_data *, int );
  virtual ~Room_Data( );

  virtual int Type    ( ) const
  { return ROOM_DATA; }
  virtual int Light   ( int = -1 ) const;
  
  virtual const char*  Location   ( Content_Array* = 0 );
  virtual const char*  Name       ( const char_data* = 0, int = 1, bool = false ) const;
  virtual bool         Seen       ( const char_data* ) const;
  virtual void         Look_At    ( char_data* );

  bool         is_dark    ( const char_data*) const;
  bool         is_indoors ( ) const;
  void         recalc_light ( );

  const char *surface ( ) const;
  const char *position ( ) const;
  const char *drop ( ) const;

  const char *sky_state( ) const;
  const char *moon_state( ) const;
  unsigned sunlight( ) const;
  unsigned moonlight( ) const;

  double temperature( int = -1 ) const;
  double humidity( int = -1 ) const;
  double clouds( int = -1 ) const;
  double wind_speed( ) const;
  double wind_angle( ) const;

  virtual void Save ( bool = true );

  void Set_Description( char_data *, const char * );
  void Edit_Description( char_data *, const char * );
  const char *Description( ) const;

  void Set_Comments( char_data *, const char * );
  void Edit_Comments( char_data *, const char * );
  const char *Comments( ) const;

  extra_array& Extra_Descr( );

  void read_action( FILE * );
  void write_actions( FILE * ) const;

private:
  extra_array    extra_descr;
  const char          *description;
  const char             *comments;
};


void  load_room_items  ( );


/*
 *   TERRAIN HEADER
 */


// Terrain flags for tables.
#define TFLAG_WATER                  0
#define TFLAG_DEEP                   1
#define TFLAG_FLOWING                2
#define TFLAG_SUBMERGED              3
#define TFLAG_ARID                   4
#define TFLAG_FALL                   5
#define TFLAG_PLANT                  6
#define TFLAG_BURY                   7
#define TFLAG_ROUGH                  8
#define TFLAG_FOREST                 9
#define TFLAG_TRACKS                10
#define MAX_TFLAG                   11


bool   water_logged   ( const room_data* );
bool   deep_water     ( const char_data*, const room_data* = 0 );
bool   is_submerged   ( const char_data*, const room_data* = 0 );
bool   midair         ( const char_data*, const room_data* = 0 );
bool   arid           ( const char_data*, const room_data* = 0 );
bool   forest         ( const char_data*, const room_data* = 0 );


#endif // tfe_room_h
