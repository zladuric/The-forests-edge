#ifndef tfe_table_h
#define tfe_table_h


#include "align.h"


/*
 *   LOCAL DEFINITIONS
 */


#define MAX_SOCIAL                500
#define MAX_SPELL_ACT             500
#define MAX_LIQUID                100
#define MAX_TOWN                   50
#define MAX_MATERIAL               50
#define MAX_GROUP                 200
#define MAX_RACE                  200
#define MAX_AFF_CHAR              150
#define MAX_AFF_OBJ                50
#define MAX_AFF_ROOM               50
#define MAX_COMMAND               500
#define MAX_CMD_CAT                30
#define MAX_CLSS                    9 
#define MAX_BUILD                 100
#define MAX_HELP_CAT               30
#define MAX_NATION                 25
#define MAX_ASTRAL                 25
#define MAX_RELIGION               50
#define MAX_TERRAIN               100
#define MAX_CLIMATE                50
#define MAX_MONTH                  20
#define MAX_DAY                    10
#define MAX_MOVEMENT              200
#define MAX_HALLUCINATE           100
#define MAX_FUNCTION             1000
#define ABS_PHY_SKILL             200
#define ABS_LANG_SKILL             25
#define ABS_SPELL_SKILL           300
#define ABS_TRADE_SKILL            20
#define ABS_WEAP_SKILL             25

#define TABLE_SOC_DEFAULT           0
#define TABLE_SOC_HUMAN             1
#define TABLE_SOC_ELF               2
#define TABLE_SOC_GNOME             3
#define TABLE_SOC_DWARF             4
#define TABLE_SOC_HALFLING          5
#define TABLE_SOC_ENT               6
#define TABLE_SOC_CENTAUR           7
#define TABLE_SOC_LIZARD            8
#define TABLE_SOC_OGRE              9
#define TABLE_SOC_TROLL            10
#define TABLE_SOC_ORC              11
#define TABLE_SOC_GOBLIN           12
#define TABLE_SOC_VYAN             13
#define TABLE_SPELL_ACT            14
#define TABLE_LIQUID               15
#define TABLE_TOWN                 16
#define TABLE_RELIGION             17
#define TABLE_SKILL_PHYSICAL       18
#define TABLE_SKILL_LANGUAGE       19
#define TABLE_SKILL_SPELL          20
#define TABLE_SKILL_TRADE          21
#define TABLE_SKILL_WEAPON         22
#define TABLE_MATERIAL             23
#define TABLE_NATION               24
#define TABLE_GROUP                25
#define TABLE_RACE                 26
#define TABLE_PLYR_RACE            27
#define TABLE_AFF_CHAR             28
#define TABLE_AFF_OBJ              29
#define TABLE_AFF_ROOM             30
#define TABLE_COMMAND              31
#define TABLE_CMD_CAT              32
#define TABLE_CLSS                 33
#define TABLE_STARTING             34
#define TABLE_TEDIT                35
#define TABLE_BUILD                36
#define TABLE_HELP_CAT             37
#define TABLE_ASTRAL               38
#define TABLE_ALIGNMENT            39
#define TABLE_TERRAIN              40
#define TABLE_CLIMATE              41
#define TABLE_MONTHS               42
#define TABLE_DAYS                 43
#define TABLE_MOVEMENT             44
#define TABLE_HALLUCINATE          45
#define TABLE_FUNCTION             46
#define MAX_TABLE                  47


/*
 *   STRUCTURES 
 */


class time_stats
{
public:
  time_stats( )
    : calls(0)
  { }
  unsigned         calls;
  time_data   total_time;
  time_data     max_time;
};


class Table_Data
{
public:
  Table_Data( ) : name( empty_string )
  { }
  virtual ~Table_Data( )
  { free_string( name, MEM_TABLE ); }

  const char *name;

  virtual bool single( ) const = 0;
  virtual void init( )
  { }

  virtual Tprog_Data **program( )
  { return 0; }
};


class Alignment_Data: public Table_Data
{
public:
  Alignment_Data( ) : abbrev( 0 ) { }
  virtual ~Alignment_Data( ) {
    free_string( abbrev, MEM_TABLE );
  }

  virtual bool single( ) const
  { return true; }

  const char *abbrev;
};


class Aff_Char_Type: public Table_Data
{
public:
  Aff_Char_Type( )
    : id_line(empty_string), score_name(empty_string),
      msg_on(empty_string), msg_on_room(empty_string),
      msg_fade(empty_string), msg_fade_room(empty_string),
      msg_off(empty_string), msg_off_room(empty_string)
  { }
  virtual ~Aff_Char_Type( ) {
    free_string( id_line, MEM_TABLE );
    free_string( score_name, MEM_TABLE );
    free_string( msg_on, MEM_TABLE );
    free_string( msg_on_room, MEM_TABLE );
    free_string( msg_fade, MEM_TABLE );
    free_string( msg_fade_room, MEM_TABLE );
    free_string( msg_off, MEM_TABLE );
    free_string( msg_off_room, MEM_TABLE );
    free_string( modifier, MEM_TABLE );
  }

  virtual bool single( ) const
  { return true; }

  const char*          id_line;
  const char*       score_name;
  const char*           msg_on;
  const char*      msg_on_room;
  const char*         msg_fade;
  const char*    msg_fade_room;
  const char*          msg_off;
  const char*     msg_off_room;
  int                 location;
  const char*         modifier;
};


class Aff_Obj_Type: public Table_Data
{
public:
  Aff_Obj_Type( )
    : msg_on(empty_string), msg_off(empty_string), location(0), prog(0)
  { }
  virtual ~Aff_Obj_Type( ) {
    free_string( msg_on, MEM_TABLE );
    free_string( msg_off, MEM_TABLE );
    delete prog;
  }

  virtual bool single( ) const
  { return true; }

  virtual void init( )
  {
    location = -1;
  }

  virtual Tprog_Data **program( )
  { return &prog; }

  const char*           msg_on;
  const char*          msg_off;
  int                 location;
  Tprog_Data       *prog;
};


class Aff_Room_Type: public Table_Data
{
public:
  Aff_Room_Type( )
    : msg_on(0), msg_fade(0), msg_off(0)
  { }
  virtual ~Aff_Room_Type( ) {
    free_string( msg_on, MEM_TABLE );
    free_string( msg_fade, MEM_TABLE );
    free_string( msg_off, MEM_TABLE );
  }

  virtual bool single( ) const
  { return true; }

  char *msg_on;
  char *msg_fade; 
  char *msg_off;
};


class Nation_Data: public Table_Data
{
public:
  Nation_Data( ) : abbrev(0)
  { }
  virtual ~Nation_Data( ) {
    free_string( abbrev, MEM_TABLE );
  }

  virtual bool single( ) const
  { return false; }

  char*          abbrev;
  int              room  [ 2 ];
  int            temple;
  int              race  [ MAX_PLYR_RACE ];
  int         alignment  [ MAX_ALIGNMENT ];
};


class Command_Type: public Table_Data, public time_stats
{
public:
  Command_Type( )
    : help(0), func_name(0),
      function(0), position(0), reqlen(0),
      disrupt(false), reveal(false), queue(false),
      category(0), prog(0)
  {
    vzero( level, 2 );
  }
  virtual ~Command_Type( ) {
    free_string( help, MEM_TABLE );
    free_string( func_name, MEM_TABLE );
    delete prog;
  }

  virtual bool single( ) const
  { return true; }

  virtual void init( )
  {
    set_bit( level, PERM_COMMANDS );
  }

  virtual Tprog_Data **program( )
  { return &prog; }

  char*             help;
  char*        func_name;
  do_func*      function;
  int           position;
  int              level  [ 2 ];
  int             reqlen;
  bool           disrupt;
  bool            reveal;
  bool             queue;
  int           category;
  Tprog_Data       *prog;
};


class Plyr_Race_Data: public Table_Data
{
public:
  Plyr_Race_Data( )
  { }
  virtual ~Plyr_Race_Data( )
  { }

  virtual bool single( ) const
  { return false; }

  virtual void init( )
  {
    tolerance = 10;
  }

  int             size;
  int        tolerance;
  int         weight_m;
  int         weight_f;
  int         height_m;
  int         height_f;
  int           resist  [ MAX_RESIST ];
  int           affect  [ AFFECT_INTS ];  
  int       stat_bonus  [ 5 ];
  int         hp_bonus;
  int       mana_bonus;
  int       move_bonus;
  int       start_room  [ 3 ];
  int           portal;
  int        start_age;
  int        life_span;
  int       alignments;
  int         language;
  bool            open;
  //  int      hunger_time;
  //  int      thirst_time;
  //  int       drunk_time;
};


class Clss_Type: public Table_Data
{
public:
  Clss_Type( ) : abbrev(0)
  { }
  virtual ~Clss_Type( ) {
    free_string( abbrev, MEM_TABLE );
  }

  virtual bool single( ) const
  { return false; }

  const char*      abbrev;    
  int             hit_min;    
  int             hit_max;    
  int            mana_min;
  int            mana_max;
  int            move_min;
  int            move_max;
  int           hit_bonus;
  int          mana_bonus;
  int          move_bonus;
  int              resist  [ MAX_RESIST ];
  int          alignments;
  bool               open;
};


class Group_Data: public Table_Data
{
public:
  Group_Data( )
  { }
  virtual ~Group_Data( )
  { }

  virtual bool single( ) const
  { return true; }

};


class Liquid_Type: public Table_Data
{
public:
  Liquid_Type( ) : color(0)
  { }
  virtual ~Liquid_Type( )
  {
    free_string( color, MEM_TABLE );
  }

  virtual bool single( ) const
  { return true; }

  char*          color;
  int           hunger;
  int           thirst;
  int          alcohol;
  int             cost;
  bool          create;
  int            spell;
};


class Material_Type: public Table_Data
{
public:
  Material_Type( )
    : msg_fire(0), msg_cold(0), msg_acid(0),
      rust_name(0), rust_verb(0)
  {
    vzero( rust, 4 );
    vzero( ingot, 2 );
  }
  virtual ~Material_Type( ) {
    free_string( msg_fire, MEM_TABLE );
    free_string( msg_cold, MEM_TABLE );
    free_string( msg_acid, MEM_TABLE );
    free_string( rust_name, MEM_TABLE );
    free_string( rust_verb, MEM_TABLE );
    for( int i = 0; i < 4; ++i ) {
      free_string( rust[i], MEM_TABLE );
    }
  }

  virtual bool single( ) const
  { return true; }

  int             cost;
  int           weight;
  int             mana;
  int            armor;
  int          enchant;
  int        save_fire;
  int        save_cold;
  int        save_acid;
  char*       msg_fire;
  char*       msg_cold;
  char*       msg_acid;
  char*      rust_name;
  char*      rust_verb;
  char*           rust  [4];  
  int            ingot  [2];
};


class Race_Data: public Table_Data
{
public:
  Race_Data( ) : plural(0), abbrev(0), track(0), family(0)
  { }
  virtual ~Race_Data( ) {
    free_string( plural, MEM_TABLE );
    free_string( abbrev, MEM_TABLE );
    free_string( track, MEM_TABLE );
  }

  virtual bool single( ) const
  { return true; }

  const char*    plural;
  char*          abbrev;
  char*           track;
  int            family;
};

  
class Religion_Data: public Table_Data
{
public:
  Religion_Data( )
  { }
  virtual ~Religion_Data( )
  { }

  virtual bool single( ) const
  { return true; }

  virtual void init( )
  {
    alignments = ( 1 << MAX_ALIGNMENT ) - 1;
    classes = ( 1 << MAX_CLSS ) - 1;
    sexes = ( 1 << ( MAX_SEX - 1 ) ) - 1;
    races = ( 1 << MAX_PLYR_RACE ) - 1;
  }

  int             sex;
  int      alignments;
  int         classes;
  int           sexes;
  int           races;
};


class Skill_Type: public Table_Data
{
public:
  Skill_Type( )
  { }
  virtual ~Skill_Type( )
  { }

  virtual bool single( ) const
  { return false; }

  virtual void init( )
  {
    pre_skill[0] = -1;
    pre_skill[1] = -1;
    pre_level[0] = 10;
    pre_level[1] = 10;
    for( int i = 0; i < MAX_CLSS; ++i ) {
      prac_cost[i] = -1;
      level[i] = -2;
    }
  }

  bool religion( int i ) const;

  int          pre_skill  [ 2 ];
  int          pre_level  [ 2 ];
  int          prac_cost  [ MAX_CLSS ];
  int              level  [ MAX_CLSS ];
  Array<int> religions;
};


class Spell_Skill_Type: public Skill_Type, public time_stats
{
public:
  Spell_Skill_Type( )
    : damage(0), regen(0), leech_mana(0), cast_mana(0), duration(0),
      type(0), location(APPLY_NONE), function(0), prog(0)
  { }
  virtual ~Spell_Skill_Type( )
  {
    free_string( damage, MEM_TABLE );
    free_string( regen, MEM_TABLE );
    free_string( leech_mana, MEM_TABLE );
    free_string( cast_mana, MEM_TABLE );
    free_string( duration, MEM_TABLE );
    delete prog;
  }

  virtual bool single( ) const
  { return false; }

  virtual Tprog_Data **program( )
  { return &prog; }

  int            prepare;
  int               wait;
  char*           damage;
  char*            regen;
  char*       leech_mana;
  char*        cast_mana;
  char*         duration;
  int               type;
  int           location;
  spell_func*   function;
  int             action  [ MAX_SPELL_WAIT ];
  int            reagent  [ MAX_SPELL_WAIT ];
  Tprog_Data       *prog;
};


class Weapon_Skill_Type: public Skill_Type
{
public:
  Weapon_Skill_Type( )
  {
    for( int i = 0; i < MAX_ATTACK; ++i ) {
      noun[i] = verb[i] = empty_string;
    }
  }
  virtual ~Weapon_Skill_Type( )
  {
    for( int i = 0; i < MAX_ATTACK; ++i ) {
      free_string( noun[i], MEM_TABLE );
      free_string( verb[i], MEM_TABLE );
    }
 }

  virtual bool single( ) const
  { return false; }

  const char *noun[ MAX_ATTACK ];
  const char *verb[ MAX_ATTACK ];
};


class Social_Type: public Table_Data
{
public:
  Social_Type( )
    : char_no_arg(0), others_no_arg(0),
      char_found(0), others_found(0), vict_found(0),
      vict_sleep(0), char_auto(0), others_auto(0),
      dir_self(0), dir_others(0),
      obj_self(0), obj_others(0),
      ch_obj_self(0), ch_obj_others(0), ch_obj_victim(0), ch_obj_sleep(0),
      self_obj_self(0), self_obj_others(0), prog(0)
  { }
  virtual ~Social_Type( ) {
    free_string( char_no_arg, MEM_TABLE );
    free_string( others_no_arg, MEM_TABLE );
    free_string( char_found, MEM_TABLE );
    free_string( others_found, MEM_TABLE );
    free_string( vict_found, MEM_TABLE );
    free_string( vict_sleep, MEM_TABLE );
    free_string( char_auto, MEM_TABLE );
    free_string( others_auto, MEM_TABLE );
    free_string( dir_self, MEM_TABLE );
    free_string( dir_others, MEM_TABLE );
    free_string( obj_self, MEM_TABLE );
    free_string( obj_others, MEM_TABLE );
    free_string( ch_obj_self, MEM_TABLE );
    free_string( ch_obj_others, MEM_TABLE );
    free_string( ch_obj_victim, MEM_TABLE );
    free_string( ch_obj_sleep, MEM_TABLE );
    free_string( self_obj_self, MEM_TABLE );
    free_string( self_obj_others, MEM_TABLE );
    delete prog;
  }

  virtual bool single( ) const
  { return true; }

  virtual Tprog_Data **program( )
  { return &prog; }

  int            position;
  bool         aggressive;
  bool            disrupt;
  bool             reveal;
  char*       char_no_arg;
  char*     others_no_arg;
  char*        char_found;
  char*      others_found;
  char*        vict_found;
  char*        vict_sleep;
  char*         char_auto;
  char*       others_auto;
  char*          dir_self;
  char*        dir_others;
  char*          obj_self;
  char*        obj_others;
  char*       ch_obj_self;
  char*     ch_obj_others;
  char*     ch_obj_victim;
  char*      ch_obj_sleep;
  char*     self_obj_self;
  char*   self_obj_others;
  Tprog_Data       *prog;
};


class Spell_Act_Type: public Table_Data
{
public:
  Spell_Act_Type( )
    : self_other(0), victim_other(0), others_other(0),
      self_self(0), others_self(0)
  { }
  virtual ~Spell_Act_Type( ) {
    free_string( self_other, MEM_TABLE );
    free_string( victim_other, MEM_TABLE );
    free_string( others_other, MEM_TABLE );
    free_string( self_self, MEM_TABLE );
    free_string( others_self, MEM_TABLE );
  }

  virtual bool single( ) const
  { return true; }

  char*      self_other;
  char*    victim_other;
  char*    others_other;
  char*       self_self;
  char*     others_self;
};


class Spell_Data: public Table_Data
{
public:
  Spell_Data( )
    : damage(0), regen(0), leech_mana(0), cast_mana(0), duration(0),
      type(0), location(APPLY_NONE), function(0)
  { }
  virtual ~Spell_Data( ) {
    free_string( damage, MEM_TABLE );
    free_string( regen, MEM_TABLE );
    free_string( leech_mana, MEM_TABLE );
    free_string( cast_mana, MEM_TABLE );
    free_string( duration, MEM_TABLE );
  }

  virtual bool single( ) const
  { return false; }

  int            prepare;
  int               wait;
  char*           damage;
  char*            regen;
  char*       leech_mana;
  char*        cast_mana;
  char*         duration;
  int               type;
  int           location;
  spell_func*   function;
  int             action  [ 5 ];
  int            reagent  [ 5 ];
};


class Tedit_Data: public Table_Data
{
public:
  Tedit_Data( )
    : edit(99), new_delete(99), sort(true), lock(false)
  { }
  virtual ~Tedit_Data( )
  { }

  virtual bool single( ) const
  { return true; }

  int      edit;
  int      new_delete;
  bool     sort;
  bool     lock;
};


class Town_Type: public Table_Data
{ 
public:
  Town_Type( )
    : recall(0)
  { }
  virtual ~Town_Type( )
  { }

  virtual bool single( ) const
  { return true; }

  int     recall;
};


class Category_Data: public Table_Data
{
public:
  Category_Data( )
    : level(0)
  { }
  virtual ~Category_Data( )
  { }

  virtual bool single( ) const
  { return true; }

  int           level;
};


class Starting_Data: public Table_Data
{
public:
  Starting_Data( )
  {
    vzero( skill, 5 );
    vzero( level, 5 );
    vzero( object, 10 );
  }
  virtual ~Starting_Data( )
  { }

  virtual bool single( ) const
  { return true; }

  int       skill  [ 5 ];
  int       level  [ 5 ];
  int      object  [ 10 ]; 
}; 


class Recipe_Data: public Table_Data
{
public:
  Recipe_Data( )
  {
    vzero( result, 2 );
    vzero( ingredient, 20 );
    vzero( skill, 3 );
    vzero( tool, 4 );
  }
  virtual ~Recipe_Data( )
  { }

  virtual bool single( ) const
  { return true; }

  int          result  [ 2 ];
  int      ingredient  [ 20 ];
  int           skill  [ 3 ];
  int            tool  [ 4 ];
};


class Terrain_Data: public Table_Data
{
public:
  Terrain_Data( );
  virtual ~Terrain_Data( );

  virtual bool single( ) const
  { return true; }

  virtual void init( )
  {
    mv_cost = 1;
    light = 100;
  }

  int mv_cost;
  int color;
  int light;
  int wind;
  int flags;
  char *surface;
  char *position;
  char *drop;
  int forage[20];
};


class Climate_Data: public Table_Data
{
public:
  Climate_Data( )
    : forage(0) { }
  virtual ~Climate_Data( ) { }

  virtual bool single( ) const
  { return true; }

  int temp_summer;
  int temp_winter;
  int humid_summer;
  int humid_winter;

  int forage;
};


class Month_Data: public Table_Data
{
public:
  Month_Data( ) { }
  virtual ~Month_Data( ) { }

  virtual bool single( ) const
  { return true; }

  int days;
};


class Day_Data: public Table_Data
{
public:
  Day_Data( ) { }
  virtual ~Day_Data( ) { }

  virtual bool single( ) const
  { return true; }
};


class Movement_Data: public Table_Data
{
public:
  Movement_Data( )
    : leave(empty_string), arrive(empty_string), position(empty_string), player(false)
  { }
  virtual ~Movement_Data( )
  {
    free_string( leave, MEM_TABLE );
    free_string( arrive, MEM_TABLE );
    free_string( position, MEM_TABLE );
  }

  virtual bool single( ) const
  { return true; }

  const char *leave;
  const char *arrive;
  const char *position;
  bool player;
};


class Weapon_Data: public Table_Data
{
public:
  Weapon_Data( )
  {
    for( int i = 0; i < MAX_ATTACK; ++i ) {
      noun[i] = verb[i] = empty_string;
    }
  }
  virtual ~Weapon_Data( )
  {
    for( int i = 0; i < MAX_ATTACK; ++i ) {
      free_string( noun[i], MEM_TABLE );
      free_string( verb[i], MEM_TABLE );
    }
  }

  virtual bool single( ) const
  { return true; }

  const char *noun[ MAX_ATTACK ];
  const char *verb[ MAX_ATTACK ];
};


class Hallucinate_Data: public Table_Data
{
public:
  Hallucinate_Data( )
    : plural(empty_string)
  {
  }
  virtual ~Hallucinate_Data( )
  {
    free_string( plural, MEM_TABLE );
  }

  virtual bool single( ) const
  { return true; }

  const char *plural;
};


class Function_Data: public Table_Data, public time_stats
{
public:
  Function_Data( )
    : func_name(0),function(0),
      prog(0),
      return_type(NONE)
  {
    vfill( arg_type, MAX_CFUNC_ARG, NONE );
    vfill( var, MAX_CFUNC_ARG, (const var_data*)0 );
  }
  virtual ~Function_Data( )
  {
    free_string( func_name, MEM_TABLE );
    delete prog;
  }

  virtual bool single( ) const
  { return false; }

  virtual Tprog_Data **program( )
  { return &prog; }

  char*        func_name;
  cfunc*        function;
  Tprog_Data       *prog;

  arg_enum return_type;
  arg_enum arg_type [ MAX_CFUNC_ARG ];
  const var_data *var [ MAX_CFUNC_ARG ];
};


/*
class Define_Entry
{
public:
  Define_Entry( )
    : singular(empty_string), plural(empty_string)
  { }
  ~Define_Entry( )
  {
    free_string( singular, MEM_TABLE );
    free_string( plural, MEM_TABLE );
  }

  const char *singular;
  const char *plural;
  int upper_limit;
};


class Define_Data: public Table_Data
{
public:
  Define_Data( )
  { }
  virtual ~Define_Data( )
  { }
  
  Array<Define_Entry> entries;
};
*/


class Entry_Data
{
public:
  void     *offset;
  int      type;
  bool     load;
  bool     save;
};


/*
 *   DEFINITIONS
 */

// Aff.Char
#define AFF_NONE                   -1 
#define AFF_ARMOR                   0
#define AFF_BLESS                   1
#define AFF_BLIND                   2
#define AFF_DISPLACE                3
#define AFF_WEAKEN                  4
#define AFF_CURSE                   5
#define AFF_DETECT_EVIL             6
#define AFF_DETECT_HIDDEN           7
#define AFF_SEE_INVIS               8 
#define AFF_DETECT_MAGIC            9
#define AFF_FAERIE_FIRE            10
#define AFF_FIRE_SHIELD            11
#define AFF_HIDE                   12
#define AFF_INFRARED               13 
#define AFF_INVISIBLE              14
#define AFF_WRATH                  15
#define AFF_POISON                 16
#define AFF_PROTECT                17
#define AFF_SANCTUARY              18
#define AFF_SLEEP                  19
#define AFF_SNEAK                  20
#define AFF_REGENERATION           21
#define AFF_ICE_SHIELD             22
#define AFF_WATER_WALKING          23
#define AFF_WATER_BREATHING        24
#define AFF_INVULNERABILITY        25
#define AFF_ENTANGLED              26
#define AFF_CONFUSED               27
#define AFF_HALLUCINATE            28
#define AFF_SLOW                   29
#define AFF_PROT_PLANTS            30
#define AFF_VITALITY               31 
#define AFF_DETECT_GOOD            32
#define AFF_LIFE_SAVING            33
#define AFF_SLEEP_RESIST           34
#define AFF_RESIST_POISON          35
#define AFF_OGRE_STRENGTH          36
#define AFF_SILENCE                37
#define AFF_TONGUES                38
#define AFF_CONTINUAL_LIGHT        39
#define AFF_PLAGUE                 40
#define AFF_DETECT_LAW             41
#define AFF_DETECT_CHAOS           42 
#define AFF_PARALYSIS              43
#define AFF_FLOAT                  44
#define AFF_BARKSKIN               45
#define AFF_PASS_DOOR              46
#define AFF_AXE_PROF               47
#define AFF_SWORD_PROF             48
#define AFF_BOW_PROF               49
#define AFF_LIGHT_SENSITIVE        50
#define AFF_DEATH                  51
#define AFF_SENSE_DANGER           52
#define AFF_RESIST_FIRE            53
#define AFF_RESIST_COLD            54
#define AFF_HASTE                  55
#define AFF_PROTECT_EVIL           56
#define AFF_PROTECT_GOOD           57
#define AFF_FLY                    58
#define AFF_SENSE_LIFE             59
#define AFF_TRUE_SIGHT             60
#define AFF_RESIST_ACID            61
#define AFF_RESIST_SHOCK           62
#define AFF_THORN_SHIELD           63
#define AFF_CHOKING                64
#define AFF_ION_SHIELD             65
#define AFF_SEE_CAMOUFLAGE         66
#define AFF_CAMOUFLAGE             67
#define AFF_SUSTENANCE             68
#define AFF_DAGGER_PROF            69
#define AFF_CLUB_PROF              70
#define AFF_STAFF_PROF             71
#define AFF_POLEARM_PROF           72
#define AFF_MACE_PROF              73
#define AFF_SPEAR_PROF             74
#define AFF_WHIP_PROF              75
#define AFF_UNARMED_PROF           76
#define AFF_MORALE                 77
#define AFF_HEROISM                78
#define AFF_ZEAL                   79
#define AFF_VALOR                  80
#define AFF_GRACE                  81
#define AFF_FORTITUDE              82
#define AFF_SENTINEL               83
#define AFF_LEGENDS                84
#define AFF_MYSTICS                85
#define AFF_DEAFNESS               86
#define AFF_WANDERER               87
#define AFF_PROTECT_LAW            88
#define AFF_PROTECT_CHAOS          89
#define AFF_DARKVISION             90

// Aff.Obj
#define AFF_BURNING                0
#define AFF_FLAMING                1
#define AFF_POISONED               2


// Aff.Room
#define AFF_WIZLOCK                0
#define AFF_WARD                   1


#define CLSS_MAGE                  0
#define CLSS_CLERIC                1
#define CLSS_THIEF                 2
#define CLSS_WARRIOR               3
#define CLSS_PALADIN               4
#define CLSS_RANGER                5
#define CLSS_DRUID                 6
#define CLSS_MONK                  7
#define CLSS_BARD                  8


#define GROUP_NONE                  0


#define LIQ_WATER                   0
#define LIQ_SLIME                   9
#define LIQ_ACID                   22
#define LIQ_POISON                 23
#define LIQ_HOLY_WATER             24


#define NATION_NONE                 0

#define REL_NONE                    0


/*
 *   VARIABLES
 */

extern Social_Type      social_table         [ MAX_PLYR_RACE+1 ][ MAX_SOCIAL ];
extern Spell_Act_Type   spell_act_table      [ MAX_SPELL_ACT ];
extern Liquid_Type      liquid_table         [ MAX_LIQUID ];
extern Town_Type        town_table           [ MAX_TOWN ]; 
extern Skill_Type       skill_physical_table [ ABS_PHY_SKILL ];
extern Skill_Type       skill_language_table [ ABS_LANG_SKILL ];
extern Spell_Skill_Type skill_spell_table    [ ABS_SPELL_SKILL ];
extern Skill_Type       skill_trade_table    [ ABS_TRADE_SKILL ];
extern Weapon_Skill_Type skill_weapon_table  [ ABS_WEAP_SKILL ];
extern Material_Type    material_table       [ MAX_MATERIAL ];
extern Nation_Data      nation_table         [ MAX_NATION ];
extern Group_Data       group_table          [ MAX_GROUP ];
extern Race_Data        race_table           [ MAX_RACE ];
extern Plyr_Race_Data   plyr_race_table      [ MAX_PLYR_RACE ];
extern Aff_Char_Type    aff_char_table       [ MAX_AFF_CHAR ];
extern Aff_Obj_Type     aff_obj_table        [ MAX_AFF_OBJ ];
extern Aff_Room_Type    aff_room_table       [ MAX_AFF_ROOM ];
extern Command_Type     command_table        [ MAX_COMMAND ];
extern Category_Data    cmd_cat_table        [ MAX_CMD_CAT ];
extern Clss_Type        clss_table           [ MAX_CLSS ];
extern Starting_Data    starting_table       [ MAX_CLSS+MAX_PLYR_RACE+1 ];
extern Tedit_Data       tedit_table          [ MAX_TABLE ];
extern Recipe_Data      build_table          [ MAX_BUILD ];
extern Category_Data    help_cat_table       [ MAX_HELP_CAT ];
extern Town_Type        astral_table         [ MAX_ASTRAL ]; 
extern Religion_Data    religion_table       [ MAX_RELIGION ]; 
extern Alignment_Data   alignment_table      [ MAX_ALIGNMENT ];
extern Terrain_Data     terrain_table        [ MAX_TERRAIN ];
extern Climate_Data     climate_table        [ MAX_CLIMATE ];
extern Month_Data       month_table          [ MAX_MONTH ];
extern Day_Data         day_table            [ MAX_DAY ];
extern Movement_Data    movement_table       [ MAX_MOVEMENT ];
extern Hallucinate_Data hallucinate_table    [ MAX_HALLUCINATE ];
extern Function_Data    function_table       [ MAX_FUNCTION ];

//extern const Entry_Data *table_entry [ MAX_TABLE-MAX_PLYR_RACE];
//extern Entry_Data       table_entry          [ MAX_TABLE-MAX_PLYR_RACE ][ MAX_FIELD ];

//extern const char**     table_field          [ MAX_TABLE ];
extern int              table_max            [ MAX_TABLE ];


/*
 *   FUNCTIONS
 */


void  init_command       ( int );
void  init_commands      ( void );
void  display_commands   ( char_data*, const char* );
void  init_spells        ( void );
void  sort_socials       ( void );
bool  edit_table         ( char_data*, int );
void  init_function      ( int );
void  init_functions     ( void );
void  display_functions  ( char_data*, const char* );

bool table_rowhere( char_data*, int );

const char *table_name( int );
const char *entry_name( int, int );
Table_Data *table_addr( int, int );


#endif // tfe_table_h
