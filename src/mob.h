#ifndef tfe_mob_h
#define tfe_mob_h

#define ACT_NO_SCAN                 0
#define ACT_SENTINEL                1
#define ACT_SCAVENGER               2
#define ACT_CAN_TAME                3
#define ACT_AGGR_ALL                4
#define ACT_STAY_AREA               5
#define ACT_WIMPY                   6
#define ACT_HUMANOID                7
#define ACT_WARM_BLOODED            8
#define ACT_SUMMONABLE              9
#define ACT_ASSIST_GROUP           10
#define ACT_CAN_FLY                11
#define ACT_REST_REGEN             12
#define ACT_MOUNT                  13 
#define ACT_OPEN_DOORS             14
#define ACT_CAN_CARRY              15
#define ACT_HAS_EYES               16
#define ACT_HAS_SKELETON           17
#define ACT_GHOST                  18
#define ACT_ZERO_REGEN             19
#define ACT_AGGR_LAWFUL            20
#define ACT_AGGR_CHAOTIC           21
#define ACT_NO_BASH                22
#define ACT_MIMIC                  23
#define ACT_STAY_TERRAIN           24
#define ACT_NO_TRIP                25
#define ACT_AGGR_EVIL              26
#define ACT_AGGR_GOOD              27
#define ACT_CARNIVORE              28
#define ACT_ELEMENTAL              29
#define ACT_USE_THE                30
#define ACT_PREDATOR               31
#define ACT_FOLLOW_FLEE            32
#define MAX_ACT                    33

#define ACT_INTS                    2


#include "skill.h"


class Species_Data
{
public:
  descr_data*         descr;
  extra_array   extra_descr;
  mprog_data*         mprog;
  reset_data*         reset;
  share_data*        shdata;
  Attack_Data       *attack;
  const char*       creator;
  const char      *last_mod;
  int                  date;
  int                  vnum;
  int                serial;
  int               hitdice;
  int              movedice;
  int                   sex;	// unsigned char?
  int                  hand;	// unsigned char?
  int                 adult;
  int              maturity;
  int              language;
  int                nation;
  int                 group;
  int           affected_by  [ AFFECT_INTS ];
  int                  gold;
  int                 color;
  int                  size;	// unsigned char?
  int                weight;
  int                chance  [ MAX_ARMOR ];
  int                 armor  [ MAX_ARMOR ];
  char*           part_name  [ MAX_ARMOR ];
  int             act_flags  [ ACT_INTS ];
  int             wear_part;
  int              skeleton;
  int                zombie;
  int                corpse;
  int                 price;
  int              movement;

  int                damage;
  int                rounds;
  int               special;
  int          damage_taken;
  int                   exp;      

  int                wander;
  int                 light;

  const char            *comments;

  bool                 used;

  static int modified;

  Species_Data( int );
  ~Species_Data( );

  const char *Name ( bool = true, bool = false, bool = true ) const;
  //  const char *Long_Name( ) const;

  //  int Weight( ) const;

  bool is_affected( int ) const;

  void set_modified( char_data *ch );

  bool dies_at_zero( ) const;

  int get_skill ( int skill ) const
  { return shdata->skills[ skill_table( skill ) ][ skill_number( skill ) ]; }
};


bool            can_extract    ( species_data*, char_data* );


/*
 *   GET_SPECIES ROUTINE
 */


extern species_data *species_list [ MAX_SPECIES ];
extern int species_max;


inline species_data *get_species( int vnum )
{
  if( vnum <= 0 || vnum > species_max )
    return 0;

  return species_list[ vnum ];
}


#endif // tfe_mob_h
