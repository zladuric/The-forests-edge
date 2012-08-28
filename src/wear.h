#ifndef tfe_wear_h
#define tfe_wear_h

#define SFLAG_CUSTOM                0
#define SFLAG_SIZE                  1
#define SFLAG_RACE                  2
#define SFLAG_RANDOM                3
#define SFLAG_TINY                  4
#define SFLAG_SMALL                 5
#define SFLAG_MEDIUM                6
#define SFLAG_LARGE                 7
#define SFLAG_GIANT                 8
#define SFLAG_HUMAN                 9
#define SFLAG_ELF                  10
#define SFLAG_GNOME                11
#define SFLAG_DWARF                12    
#define SFLAG_HALFLING             13
#define SFLAG_ENT                  14
#define SFLAG_CENTAUR              15
#define SFLAG_LIZARD               16
#define SFLAG_OGRE                 17 
#define SFLAG_TROLL                18 
#define SFLAG_ORC                  19 
#define SFLAG_GOBLIN               20 
#define SFLAG_VYAN                 21  
#define MAX_SFLAG                  22


#define WEAR_NONE                  -1
#define WEAR_FLOATING               0
#define WEAR_FINGER_R               1
#define WEAR_FINGER_L               2
#define WEAR_NECK                   3
#define WEAR_UNUSED0                4
#define WEAR_BODY                   5
#define WEAR_HEAD                   6
#define WEAR_LEGS                   7
#define WEAR_FEET                   8
#define WEAR_HANDS                  9
#define WEAR_ARMS                  10
#define WEAR_UNUSED1               11
#define WEAR_UNUSED2               12
#define WEAR_WAIST                 13
#define WEAR_WRIST_R               14
#define WEAR_WRIST_L               15
#define WEAR_HELD_R                16
#define WEAR_HELD_L                17
#define WEAR_UNKNOWN0              18
#define WEAR_UNKNOWN1              19
#define WEAR_UNKNOWN2              20
#define WEAR_UNKNOWN3              21
#define WEAR_HORSE_BODY            22
#define WEAR_HORSE_BACK            23
#define WEAR_HORSE_FEET            24
#define MAX_WEAR_HUMANOID          18
#define MAX_WEAR                   25

#define LAYER_ANY                  -1
#define LAYER_BOTTOM                0
#define LAYER_UNDER                 1
#define LAYER_BASE                  2
#define LAYER_OVER                  3
#define LAYER_TOP                   4
#define MAX_LAYER                   5


extern const int         wear_index  [ MAX_WEAR ];
extern const char*  reset_wear_name  [ ];
extern const char*      wear_abbrev  [ ];
extern const char*  reset_skin_name  [ ];
extern const char*      skin_abbrev  [ ];
extern const char*        wear_name  [ ];
extern const char**  wear_part_name;
extern const char*       layer_name  [ MAX_LAYER+1 ];

extern const default_data wear_msg [];
extern const default_data remove_msg [];


/*
 *   WEAR ROUTINES
 */


bool         can_use     ( char_data*, obj_clss_data*, obj_data* );
bool         could_wear  ( char_data *, obj_data*, bool );
void         equip       ( char_data*, obj_data*, bool );
void         unequip     ( char_data*, obj_data*, bool );
void         wear        ( char_data *, thing_array& );
void         put_on      ( char_data*, thing_array*, char_data* );


/*
 *   INLINE ROUTINES
 */


inline int wear_size( char_data* ch )
{
  switch( ch->Size( ) ) {
  case SIZE_ANT:
  case SIZE_RAT:
  case SIZE_DOG:
    return SFLAG_TINY;
  case SIZE_GNOME:
    return SFLAG_SMALL;
  case SIZE_HUMAN:
    return SFLAG_MEDIUM;
  case SIZE_OGRE:
  case SIZE_HORSE:
    return SFLAG_LARGE;
  default:
    return SFLAG_GIANT;
  }
  //  return SFLAG_TINY-SIZE_DOG+range( SIZE_DOG, ch->Size( ), SIZE_GIANT );
}


inline bool can_wear( obj_data* obj, int part )
{
  return is_set( obj->pIndexData->wear_flags, part );
}


#endif // tfe_wear_h
