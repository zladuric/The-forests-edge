#ifndef tfe_char_h
#define tfe_char_h

#include "define.h"
#include "abil.h"
#include "thing.h"
#include "network.h"


/*
 *   SHARE_DATA
 *
 *   Data needed by both species_data and char_data.
 */


class Share_Data
{
public:
  unsigned char       strength;
  unsigned char   intelligence;
  unsigned char         wisdom;
  unsigned char      dexterity;
  unsigned char   constitution;
  int          level;	// unsigned char?
  int      alignment;	// unsigned char?
  int           race;	// unsigned char?
  int          kills;
  int         deaths;
  int           fame;
  int         resist  [ MAX_RESIST ];
  unsigned char *skills [ MAX_SKILL_CAT ];

  Share_Data( );
  ~Share_Data( );
}; 


/*
 *   CREATURE CLASS AND DERIVED SUB-CLASSES
 */


class char_data : public thing_data
{
public: 
  char_data*           mount;
  char_data*           rider;	// Move to mob_data?
  char_data*        fighting;
  descr_data*          descr;
  species_data*      species;	// Move to mob_data?

  event_data          active;
  event_data          update;
  event_data           regen;
  command_queue    cmd_queue;

  char_data*          leader;
  char_array       followers;  
  char_array         seen_by;
  char_array        known_by;	// Move to mob_data? Change to player_array?
  char_array      aggressive;	// Move to mob_data?
  exit_array      seen_exits;
  affect_array    leech_list;	// Move to player_data?
  Content_Array      wearing;

  cast_data*            cast;
  link_data*            link;
  pc_data*            pcdata;
  share_data*         shdata;
  enemy_data*          enemy;	// Move to mob_data?
  cast_data*         prepare;	// Move to player_data?
  obj_data*          pos_obj;

  room_data*         in_room;
  room_data*     was_in_room;	// Move to player_data. Major problems in output.h.

  // Keep these together; saved/restored as a block of ints in save.cc::file_pet( ).
  int                    hit;
  int               base_hit;
  int                   mana;
  int              base_mana;
  int                   move;
  int              base_move;

  int                mod_hit;
  int               mod_move;
  int               mod_mana;
  int                max_hit;
  int               max_move;
  int               max_mana;

  int                mod_str;	// signed char?
  int                mod_int;	// signed char?
  int                mod_wis;	// signed char?
  int                mod_dex;	// signed char?
  int                mod_con;	// signed char?

  int              mod_resist  [ MAX_RESIST ];
  int               mod_armor;
  int             affected_by  [ AFFECT_INTS ];
  int                  status  [ STATUS_INTS ];
  int                     exp;
  int                 hitroll;
  int                 damroll;
  int              move_regen;
  int              mana_regen;
  int               hit_regen;
  int           room_position;	// unsigned char? - no, none == -1
  int                     sex;	// unsigned char?
  int                    hand;	// unsigned char?
  int                   color;
  int                position;	// unsigned char? - no, extracted == -1

  int              condition [ MAX_COND ];

  char_data( );
  virtual ~char_data( );

  /* BASE */

  virtual int Selected( ) const;
  virtual int Shown( ) const;

  virtual void Select( int num );
  virtual void Select_All( );
  virtual void Show( int num );

  virtual int   Type     ( ) const = 0;
  virtual void  Extract  ( );

  /* NAME/KEYWORDS */

  virtual const char*   Keywords        ( char_data* );
  virtual const char*   Name            ( const char_data* = 0, int = 1, bool = false ) const;
  virtual const char*   Seen_Name       ( const char_data* = 0, int = 1, bool = false ) const;
  const char *Long_Name       ( const char_data*, int = 1, bool colored = true ) const;
  const char *The_Name       ( const char_data*, int = 1 ) const;
  virtual const char*   Show_To         ( char_data* );
  const char*   real_name       ( ); 
  virtual const char*   Location        ( Content_Array* = 0 );
  virtual void          Look_At         ( char_data* );

  void make_known ( char_data* );

  const char*   Him_Her         ( char_data* = 0 ) const;
  const char*   His_Her         ( char_data* = 0 ) const;
  const char*   He_She          ( char_data* = 0 ) const;

  /* PROPERTIES */

  bool Is_Awake   ( bool = false ) const;
  bool Can_Hear   ( bool = false ) const;
  bool Can_See    ( bool = false ) const;

  virtual bool Seen       ( const char_data* ) const;


  virtual bool Is_Valid        ( ) const = 0;

  virtual bool In_Game         ( ) const;
  virtual void To              ( Content_Array& );
  virtual void To              ( thing_data* = 0 );
  virtual thing_data*  From    ( int = 1, bool = false );

  virtual int  Size             ( ) const = 0;
  virtual int  Weight           ( int = -1 );
  virtual int  Empty_Weight     ( int = -1 );
  virtual int  Capacity         ( );
  virtual int  Empty_Capacity   ( ) const;

  /* AFFECTS */

  bool is_affected ( int ) const;
  bool Sees_Invis ( ) const;
  bool Sees_Hidden ( ) const;
  bool Sees_Camo ( ) const;
  bool detects_good ( ) const;
  bool detects_evil ( ) const;
  bool detects_law ( ) const;
  bool detects_chaos ( ) const;

  /* STATS */

  int            Strength        ( ) const;
  int            Intelligence    ( ) const;
  int            Wisdom          ( ) const;
  int            Dexterity       ( ) const;
  int            Constitution    ( ) const;

  int            Level           ( ) const
  { return shdata->level; }

  virtual int    Remort          ( ) const
  { return 0; }

  double         Hitroll         ( obj_data* );
  double         Damroll         ( obj_data* );
  int            Hit_Regen       ( );
  int            Move_Regen      ( );
  int            Mana_Regen      ( );

  virtual double  Mean_Hp         ( ) const = 0;
  virtual double  Mean_Mana       ( ) const = 0; 
  virtual double  Mean_Move       ( ) const = 0;

  /* OTHER */

  bool           can_climb              ( ) const;
  bool           can_float              ( ) const;
  bool           can_swim               ( ) const;
  bool           can_breathe_underwater ( ) const;
  bool           can_fly                ( ) const;
  bool           can_hold_light         ( ) const;
  bool           can_carry              ( ) const;

  bool           Can_Move               ( char_data*, room_data*, exit_data*, int, int&, int&, bool,
					  bool = false, bool = false, bool = false );
  bool           can_buy                ( obj_data*, char_data* );
  bool           can_edit               ( obj_clss_data*, bool = true ) const;
  bool           can_edit               ( species_data*, bool = true ) const;
  bool           can_edit               ( room_data*, bool = true ) const;

  bool           check_skill            ( int, int = 0 );
  bool           is_humanoid            ( ) const;

  bool           Recognizes      ( const char_data* ) const;
  bool           Befriended      ( char_data* ) const;
  bool           Befriended      ( pfile_data* ) const;
  bool           Ignoring        ( char_data* ) const;
  bool           Filtering       ( char_data* ) const;
  bool           Filtering       ( pfile_data* ) const;
  virtual bool           Accept_Msg      ( char_data* ) const = 0;

  
  obj_data*      Wearing         ( int, int = -1 );
  virtual int            Light           ( int = -1 ) const;

  int            Save_Physical   ( char_data * ) const;
  int            Save_Magic      ( ) const;
  int            Save_Fire       ( ) const;
  int            Save_Cold       ( ) const;
  int            Save_Mind       ( ) const;
  int            Save_Shock      ( ) const;
  int            Save_Acid       ( ) const;
  int            Save_Poison     ( ) const;
  int            Save_Sound      ( ) const;

  int            can_carry_n     ( ) const;
  //  int            can_carry_w     ( ) const;
  int            get_burden      ( ) const;
  int            get_skill       ( int ) const;
  
  void           improve_skill    ( int );

  int            Align_Good_Evil () const
  { return shdata->alignment % 3; }
  int            Align_Law_Chaos () const
  { return shdata->alignment / 3; }
  int		 Race_Distance   (const char_data *) const;
  int            Align_Distance  (const char_data *) const;

  int mod_position( ) const;
  const char *position_name( char_data * = 0 ) const;

  void set_default_title( );

  int tolerance ( ) const;
  virtual bool   is_drunk ( ) const;

  bool knows ( const char_data* ) const;

private:
  bool selected;
  int shown;
};


inline bool is_good( const char_data *ch )
{ return ch->Align_Good_Evil() == 0; }

inline bool is_evil( const char_data *ch )
{ return ch->Align_Good_Evil() == 2; }

inline bool is_lawful( const char_data *ch )
{ return ch->Align_Law_Chaos() == 0; }

inline bool is_chaotic( const char_data *ch )
{ return ch->Align_Law_Chaos() == 2; }


/*
 *   PLAYER DATA
 */


class Enemy_Data
{
public:
  enemy_data *next;
  pfile_data *pfile;
  int           damage;
  //  bool          attacker;

  Enemy_Data( );
  ~Enemy_Data( );
}; 


class Reput_Data
{
public:
  int      nation  [ MAX_NATION ];  
  int   alignment  [ MAX_ALIGNMENT ]; 
  int       blood;
  int        gold; 
  int       magic; 

  Reput_Data( )
    : blood(0), gold(0), magic(0)
  {
    vzero( nation,    MAX_NATION );
    vzero( alignment, MAX_ALIGNMENT ); 
  }
};

 
class player_data : public char_data, public Save_Data
{
public:
  char_data*     switched;
  char_data*     familiar;
  char_data*        reply;
  int            base_age;
  int                bank;
  int              prayer;
  int          gossip_pts;
  int             whistle;
  int           noteboard;
  note_data*    note_edit;
  player_data     *docker;
  int              remort;
  const char *remort_name;
  int            movement;
  time_t            logon;
  time_t        save_time; 
  int              played;
  int               timer;

  alias_array       alias;
  Content_Array    locker;
  Content_Array    junked;

  reput_data   reputation;

  //  event_data       hunger;
  //  event_data       thirst;
  //  event_data        drunk;

  tell_data*        chant;
  tell_data*          say;
  tell_data*         yell;
  tell_data*        shout;
  tell_data*         tell;
  tell_data*        gtell;
  tell_data*         chat;
  tell_data*       gossip;
  tell_data*        ctell;
  tell_data*      whisper;
  tell_data*        atalk;
  tell_data*           to;

  player_data( const char* );
  virtual ~player_data( );

  virtual int           Type            ( ) const
  { return PLAYER_DATA; }
  virtual void  Extract  ( );
  virtual bool          Is_Valid        ( ) const
  { return valid == PLAYER_DATA; }
  
  int           Age             ( );
  virtual const char*   Location        ( Content_Array* = 0 );
  virtual int           Size            ( ) const;

  virtual int   Remort          ( ) const
  { return remort; }

  virtual double          Mean_Hp         ( ) const;
  virtual double          Mean_Mana       ( ) const;
  virtual double          Mean_Move       ( ) const;

  virtual bool           Accept_Msg      ( char_data* ) const;

  virtual void Save ( bool = true );

  int Height( ) const;

  time_t time_played ( )
  { return current_time-logon+played; }

  //  virtual bool   is_drunk( ) const;
};


class player_array: public Array<player_data*> {};

extern player_array   player_list;


/*
 *   WIZARD CLASS
 */


class Wizard_Data : public player_data
{
public:
  action_data*     action_edit;
  player_data*     player_edit;
  exit_data*         exit_edit;
  extra_data*       adata_edit;
  extra_data*      mpdata_edit;
  extra_data*      opdata_edit;
  extra_data*      oextra_edit;
  extra_data*      mextra_edit;
  extra_data*      textra_edit;
  extra_data*        room_edit;
  mprog_data*       mprog_edit;
  obj_clss_data* obj_clss_edit;
  oprog_data*       oprog_edit;
  quest_data*       quest_edit;
  species_data*       mob_edit;
  help_data*         help_edit;
  account_data*   account_edit;
  
  int              custom_edit;
  int              rtable_edit;
  int                list_edit;
  int               table_edit  [ 2 ];

  //  int               permission  [ 2 ];
  int                   office;
  int                 wizinvis;		// unsigned char?

  int                   recall;

  const char*                 bamfin;
  const char*                bamfout;
  const char*            level_title;

  // Channels that require permissions.
  tell_data*        build_chan;
  tell_data*        admin_chan;
  tell_data*          imm_talk;
  tell_data*          god_talk;
  //  tell_data*            avatar;

  player_data         *docking;

  Wizard_Data( const char* );
  virtual ~Wizard_Data( );

  virtual int   Type            ( ) const
  { return WIZARD_DATA; }
  virtual bool          Is_Valid        ( ) const
  { return valid == WIZARD_DATA; }
};


/*
 *   MOB CLASS
 */


class Mob_Data : public char_data
{
public:
  mob_data*          prev;
  reset_data*       reset;
  trainer_data*  pTrainer;
  shop_data*        pShop;
  int            maturity;
  const char    *pet_name;

  Mob_Data( species_data * );
  virtual ~Mob_Data( );

  virtual int    Type          ( ) const
  { return MOB_DATA; }
  virtual bool   Is_Valid      ( ) const
  { return valid == MOB_DATA; }
  virtual bool   In_Game       ( ) const
  { return Is_Valid(); }
  virtual int    Size          ( ) const;

  virtual double          Mean_Hp         ( ) const;
  virtual double          Mean_Mana       ( ) const;
  virtual double          Mean_Move       ( ) const;

  virtual bool           Accept_Msg      ( char_data* ) const;

  //  virtual bool   is_drunk( ) const;
};


class mob_array: public Array<Mob_Data*> {};

extern mob_array      mob_list;


/*
 *   DESCRIPTION DATA
 */


class Descr_Data
{
public:
  const char*           name;
  const char*       keywords;
  const char*       singular;
  const char*         long_s;
  const char*          adj_s;
  const char*       prefix_s;
  const char*         plural;
  const char*         long_p;
  const char*          adj_p;
  const char*       prefix_p;
  const char*       complete;

  Descr_Data( );
  ~Descr_Data( );

  Descr_Data( const Descr_Data& old ) {
    record_new( sizeof( descr_data ), MEM_DESCR );
    name = alloc_string( old.name, MEM_DESCR );
    keywords = alloc_string( old.name, MEM_DESCR );
    singular = alloc_string( old.singular, MEM_DESCR );
    long_s = alloc_string( old.long_s, MEM_DESCR );
    adj_s = alloc_string( old.adj_s, MEM_DESCR );
    prefix_s = alloc_string( old.prefix_s, MEM_DESCR );
    plural = alloc_string( old.plural, MEM_DESCR );
    long_p = alloc_string( old.long_p, MEM_DESCR );
    adj_p = alloc_string( old.adj_p, MEM_DESCR );
    prefix_p = alloc_string( old.prefix_p, MEM_DESCR );
    complete = alloc_string( old.complete, MEM_DESCR );
  }
};


#endif // tfe_char_h
