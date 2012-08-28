#ifndef tfe_reset_h
#define tfe_reset_h


/*
 *   CLASSES
 */


class Reset_Data 
{
public:
  reset_data*        next;
  int                vnum;
  int                flags;
  int                chances;
  int                value;
  int                count;
  int                liquid;

  Reset_Data( int );
  Reset_Data( const Reset_Data& );
  ~Reset_Data( );
};


class Rtable_Data
{
public:
  char*         name;
  reset_data*   reset;  

  Rtable_Data( );
  ~Rtable_Data( );
};


/*
 *   VARIABLES
 */


extern  int              num_rtable;
extern  rtable_data**   rtable_list;


/*
 *   DEFINITIONS
 */


#define RST_ROOM              0
#define RST_MOB               1
#define RST_TABLE             2

#define RSFLAG_REROLL         0
#define RSFLAG_MOB            3
#define RSFLAG_OBJECT         4

#define RSFLAG_LEADER         5
#define RSFLAG_FOLLOWER       6
#define RSFLAG_SENTINEL       7
#define RSFLAG_NIGHT          8
#define RSFLAG_DAY            9
#define RSFLAG_AGGRESSIVE    10
#define RSFLAG_MOUNT         11
#define RSFLAG_CHAIR         12
#define RSFLAG_PEACEFUL      13

#define RSFLAG_CONTAINER      5
#define RSFLAG_INSIDE         6
#define RSFLAG_OVERLOAD       7
#define RSFLAG_HIDDEN         8
#define MAX_RSFLAG           14


#define RSPOS_SLEEPING        0
#define RSPOS_MEDITATING      1
#define RSPOS_RESTING         2
#define RSPOS_STANDING        3
#define MAX_RESET_POS         4

#define RSPOS_GROUND         -2
#define RSPOS_SKIN           -2
#define RSPOS_INV            -1


/*
 *   FUNCTIONS
 */


obj_array *get_skin_list   ( species_data* );

void   load_rtables    ();
void   save_rtables    ();

void   reset_room      ( room_data* );
void   reset_shop      ( mob_data* );
void   mreset_mob      ( mob_data* );
void   mob_setup       ( mob_data*, room_data* );

bool   has_reset       ( obj_clss_data* );
bool   has_reset       ( species_data* );

const char *name       ( const reset_data* );

void   edit_reset      ( char_data*, const char *,
			 int, reset_data*&, int,
                         const char* = empty_string );

void   unregister_reset   ( mob_data* );
void   unregister_reset   ( obj_data* );


#endif // tfe_reset_h
