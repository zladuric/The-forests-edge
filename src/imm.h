#ifndef tfe_imm_h
#define tfe_imm_h

#define LEVEL_HERO                 90
#define LEVEL_AVATAR               91
#define LEVEL_APPRENTICE           92
#define LEVEL_BUILDER              93
#define LEVEL_ARCHITECT            94
#define LEVEL_IMMORTAL             95
#define LEVEL_SPIRIT               96
#define LEVEL_ANGEL                97
#define LEVEL_DEMIGOD              98
#define LEVEL_GOD                  99
#define MAX_LEVEL                  99


#define LEVEL_HELP                 94
#define LEVEL_MOB                  92
#define LEVEL_MOB_ALL              94
#define LEVEL_OBJECT               93
#define LEVEL_OBJECT_ALL           95
#define LEVEL_PLAYER               95
#define LEVEL_EDIT_PLAYER          96
#define LEVEL_QUEST                94
#define LEVEL_ROOM                 92
#define LEVEL_ROOM_ALL             94
#define LEVEL_SHOP                 94 
#define LEVEL_TABLE                95


/*
 *   PERMISSION HEADER
 */


#define PERM_ALL_MOBS              0 
#define PERM_ALL_OBJECTS           1 
#define PERM_ALL_ROOMS             2 
#define PERM_APPROVE               3
#define PERM_BASIC                 4
#define PERM_BUILD_CHAN            5
#define PERM_COMMANDS              6
#define PERM_ECHO                  7
#define PERM_GOD_CHAN              8
#define PERM_GOTO                  9
#define PERM_HELP_FILES           10
#define PERM_IMM_CHAN             11
#define PERM_LISTS                12
#define PERM_LOAD_OBJECTS         13
#define PERM_MISC_TABLES          14
#define PERM_MOBS                 15
#define PERM_NOTEBOARD            16
#define PERM_OBJECTS              17
#define PERM_PLAYERS              18
#define PERM_QUESTS               19
#define PERM_REIMB_EXP            20
#define PERM_REIMB_EQUIP          21
#define PERM_ROOMS                22
#define PERM_SHUTDOWN             23
#define PERM_ACCOUNTS             24
#define PERM_SNOOP                25
#define PERM_SPELLS               26
#define PERM_SOCIALS              27
#define PERM_TRANSFER             28
#define PERM_UNFINISHED           29
#define PERM_WRITE_ALL            30
#define PERM_WRITE_AREAS          31
#define PERM_AVATAR_CHAN          32
#define PERM_CLANS                33
#define PERM_RTABLES              34 
#define PERM_DISABLED             35
#define PERM_FORCE_PLAYERS        36
#define PERM_ADMIN_CHAN           37
#define MAX_PERMISSION            38


extern char const*  permission_name  [ MAX_PERMISSION ];
extern char const*  imm_title        [ ];


bool   has_permission  ( const char_data*, int*, bool = false );
bool   mortal          ( char_data*, char* );
int    invis_level     ( const char_data * );
bool   privileged      ( const char_data *, int = LEVEL_APPRENTICE );
bool   has_holylight   ( const char_data* );


inline bool has_permission( const char_data* ch, int flag, bool msg = false )
{
  int array  [ 2 ];

  if( flag >= 32 ) {
    array[0] = 0;
    array[1] = ( 1 << ( flag-32 ) );
  } else {
    array[0] = 1 << flag;
    array[1] = 0;
  }
  
  return has_permission( ch, array, msg );
}


/*
 *   LEVEL FUNCTIONS
 */


inline bool is_avatar     ( const char_data* ch )  { return get_trust( ch ) >= LEVEL_AVATAR; }
inline bool is_apprentice ( const char_data* ch )  { return get_trust( ch ) >= LEVEL_APPRENTICE; }
inline bool is_builder    ( const char_data* ch )  { return get_trust( ch ) >= LEVEL_BUILDER; }
inline bool is_architect  ( const char_data* ch )  { return get_trust( ch ) >= LEVEL_ARCHITECT; }
inline bool is_immortal   ( const char_data* ch )  { return get_trust( ch ) >= LEVEL_IMMORTAL; }
inline bool is_spirit     ( const char_data* ch )  { return get_trust( ch ) >= LEVEL_SPIRIT; }
inline bool is_angel      ( const char_data* ch )  { return get_trust( ch ) >= LEVEL_ANGEL; }
inline bool is_demigod    ( const char_data* ch )  { return get_trust( ch ) >= LEVEL_DEMIGOD; }
inline bool is_god        ( const char_data* ch )  { return get_trust( ch ) >= LEVEL_GOD; }


#endif // tfe_imm_h
