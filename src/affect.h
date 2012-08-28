#ifndef tfe_affect_h
#define tfe_affect_h


// For Aff.Char.
#define APPLY_NONE                  0
#define APPLY_STR                   1
#define APPLY_DEX                   2
#define APPLY_INT                   3
#define APPLY_WIS                   4
#define APPLY_CON                   5
#define APPLY_MAGIC                 6
#define APPLY_FIRE                  7
#define APPLY_COLD                  8
#define APPLY_SHOCK                 9
#define APPLY_MIND                 10
#define APPLY_AGE                  11 
#define APPLY_MANA                 12
#define APPLY_HIT                  13
#define APPLY_MOVE                 14
#define APPLY_AC                   17
#define APPLY_HITROLL              18
#define APPLY_DAMROLL              19
#define APPLY_MANA_REGEN           20
#define APPLY_HIT_REGEN            21
#define APPLY_MOVE_REGEN           22
#define APPLY_ACID                 23
#define APPLY_POISON               24
#define MAX_AFF_LOCATION           25

#define AFFECT_INTS                 3


// For Aff.Room.
#define APPLY_BOTH_SIDES            1


class affect_data
{
public:
  char_data*    leech;
  visible_data *target;
  int           type;
  int           duration;
  int           level;
  int           location;
  int           modifier;
  int           leech_regen;
  int           leech_max;

  affect_data( );
  affect_data( const affect_data& );
  ~affect_data( );
};


bool   add_affect         ( char_data*, affect_data* );
void   add_affect         ( obj_data*&, affect_data* );
void   add_affect         ( room_data*, affect_data* );
int    affect_level       ( const char_data*, int, affect_data* = 0 ); 
int    affect_level       ( const room_data*, int, const exit_data* = 0 ); 
int    affect_duration    ( const char_data*, int ); 
void   init_affects       ( char_data*, int = 0, int = -1 );
void   modify_affect      ( char_data*, affect_data*, bool, bool = true );
bool   strip_affect       ( char_data*, int, bool = true );
bool   strip_affect       ( obj_data*&, int, bool = true );
int    shorten_affect     ( char_data*, int, int, bool = true );
void   remove_affect      ( char_data* );
void   remove_affect      ( char_data*, affect_data*, bool = true );
void   remove_affect      ( obj_data* );
void   remove_affect      ( obj_data*&, affect_data*, bool = true );
void   remove_affect      ( room_data* );
void   remove_affect      ( room_data*, affect_data*, bool = true );
void   read_affects       ( FILE*, obj_clss_data* );
void   read_affects       ( FILE*, obj_data* );
void   read_affects       ( FILE*, char_data* );
void   write_affects      ( FILE*, obj_clss_data* );
void   write_affects      ( FILE*, obj_data* );
void   write_affects      ( FILE*, char_data* );


inline int affect_delay( )
{
  return number_range( PULSE_TICK/2, 3*PULSE_TICK/2 );
}


#endif // tfe_affect_h
