#ifndef tfe_move_h
#define tfe_move_h

#define DIR_NORTH                  0
#define DIR_EAST                   1
#define DIR_SOUTH                  2
#define DIR_WEST                   3
#define DIR_UP                     4
#define DIR_DOWN                   5
#define MAX_DIR_COMPASS            6

#define DIR_EXTRA                  6
#define DIR_ANY                    7
#define DIR_TRANSFER               8
#define MAX_DIR                    9


class direction_type
{
 public:
  const char *const name;
  int            reverse;
  char*      arrival_msg;  
  char*            where;
};


extern const direction_type  dir_table    [ ];


/*
 *   MOVEMENT
 */


#define MOVE_NONE        -2
#define MOVE_FLEE        -1
#define MOVE_SNEAK        0
#define MOVE_WALK         1
#define MOVE_FLY          2
#define MOVE_SWIM         3
#define MOVE_GLIDE        4
#define MOVE_WADE         5
#define MOVE_FALL         6
#define MOVE_CLIMB        7
        

bool   move_char          ( char_data*, int, bool, bool = false );
void   enter_water        ( char_data* );
const char *move_verb     ( char_data*, const char *Movement_Data::*, int = MOVE_WALK );


bool trigger_leaving      ( char_data*, room_data*, exit_data*, int, action_data*& );
bool trigger_entering     ( char_data*, room_data*, int, action_data * );

extern const default_data leaving_msg  [ ];
extern const default_data entering_msg  [ ];
extern const default_data blocking_msg [ ];


/*
 *   PATHS
 */


class Path_Data
{
public:
  char_data  *summoner;
  room_data      *goal;
  exit_array    ignore;
  unsigned        step;
  unsigned      length;
  int*      directions;   // unsigned  char *?
  int            range;
  int            delay;
  bool       interrupt;
  bool           valid;
  bool           retry;
  char_data    *notify;

  Path_Data( );
  ~Path_Data( );
};


#define MAX_PATH_RANGE 200


path_func mark_respond;


void mark_range        ( thing_data*, int, path_func*,
                         const char* = empty_string, exit_array* = 0,
			 int = 50 );

void clear_range       ( thing_data* );

void exec_range        ( thing_data*, int, path_func*,
                         const char* = empty_string, exit_array* = 0,
			 int = 50 );

event_data *add_path   ( char_data*, thing_data*, int = 100, int = 50, bool = true );

bool retry_path        ( event_data*, exit_data* );


#endif // tfe_move_h
