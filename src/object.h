#ifndef tfe_object_h
#define tfe_object_h

/* 
 *   OBJ_DATA CLASS
 */


#define ANTI_HUMAN                  0
#define ANTI_MAGE                  14
#define ANTI_MALE                  24
#define ANTI_FEMALE                25
#define ANTI_GOOD                  26
#define ANTI_LAWFUL                29
#define MAX_ANTI                   32


#define ITEM_TAKE                   0
#define ITEM_WEAR_FINGER            1
#define ITEM_WEAR_NECK              2
#define ITEM_WEAR_BODY              3
#define ITEM_WEAR_HEAD              4
#define ITEM_WEAR_LEGS              5
#define ITEM_WEAR_FEET              6
#define ITEM_WEAR_HANDS             7 
#define ITEM_WEAR_ARMS              8
#define ITEM_WEAR_UNUSED3           9
#define ITEM_WEAR_UNUSED4          10 
#define ITEM_WEAR_WAIST            11
#define ITEM_WEAR_WRIST            12
#define ITEM_HELD_R                13
#define ITEM_HELD_L                14
#define ITEM_WEAR_UNUSED0          15 
#define ITEM_WEAR_UNUSED1          16
#define ITEM_WEAR_UNUSED2          17
#define ITEM_WEAR_FLOATING         18
#define ITEM_WEAR_HORSE_BODY       19
#define ITEM_WEAR_HORSE_BACK       20
#define ITEM_WEAR_HORSE_FEET       21      
#define MAX_ITEM_WEAR              22

#define ITEM_OTHER                  0
#define ITEM_LIGHT                  1
#define ITEM_SCROLL                 2
#define ITEM_WAND                   3
#define ITEM_STAFF                  4
#define ITEM_WEAPON                 5
#define ITEM_GEM                    6
#define ITEM_SPELLBOOK              7
#define ITEM_TREASURE               8
#define ITEM_ARMOR                  9
#define ITEM_POTION                10
#define ITEM_REAGENT               11
#define ITEM_FURNITURE             12
#define ITEM_TRASH                 13
#define ITEM_VEHICLE               14 
#define ITEM_CONTAINER             15
#define ITEM_LOCK_PICK             16 
#define ITEM_DRINK_CON             17
#define ITEM_KEY                   18
#define ITEM_FOOD                  19
#define ITEM_MONEY                 20
#define ITEM_KEYRING               21
#define ITEM_BOAT                  22
#define ITEM_CORPSE                23
#define ITEM_INSTRUMENT            24
#define ITEM_FOUNTAIN              25
#define ITEM_WHISTLE               26
#define ITEM_TRAP                  27
#define ITEM_LIGHT_PERM            28
#define ITEM_BANDAGE               29
#define ITEM_BOUNTY                30
#define ITEM_GATE                  31
#define ITEM_ARROW                 32 
#define ITEM_SKIN                  33
#define ITEM_BODY_PART             34
#define ITEM_CHAIR                 35
#define ITEM_TABLE                 36
#define ITEM_BOOK                  37
#define ITEM_PIPE                  38
#define ITEM_TOBACCO               39
#define ITEM_DECK_CARDS            40
#define ITEM_FIRE                  41
#define ITEM_GARROTE               42
#define ITEM_TOY                   43
#define MAX_ITEM                   44


#define MAT_PAPER                   0
#define MAT_WOOD                    1
#define MAT_LEATHER                 2
#define MAT_UNUSED1                 3
#define MAT_CLOTH                   4
#define MAT_GLASS                   5
#define MAT_STONE                   6
#define MAT_BONE                    7
#define MAT_FLESH                   8
#define MAT_ORGANIC                 9
#define MAT_UNUSED2                10
#define MAT_BRONZE                 11
#define MAT_IRON                   12
#define MAT_STEEL                  13
#define MAT_MITHRIL                14
#define MAT_ADAMANTINE             15
#define MAT_ELECTRUM               16
#define MAT_SILVER                 17
#define MAT_GOLD                   18
#define MAT_COPPER                 19
#define MAT_PLATINUM               20
#define MAT_KRYNITE                21
#define MAT_TIN                    22
#define MAT_BRASS                  23
#define MAT_TITANIUM               24

 
#define OBJ_GOLD                    2
#define OBJ_CORPSE_NPC             10
#define OBJ_CORPSE_PC              11
#define OBJ_COPPER                 12
#define OBJ_SILVER                 14
#define OBJ_PLATINUM               16
#define OBJ_BALL_OF_LIGHT          21
#define OBJ_ASTRAL_GATE           270
#define OBJ_LOCUST_SWARM          271
#define OBJ_POISON_CLOUD          279
#define OBJ_CAMPFIRE             1112
#define OBJ_CORPSE_PET           3639
#define OBJ_BODY_PART            3646
#define OBJ_CACHE                4715


#define OFLAG_GLOW                  0
#define OFLAG_HUM                   1
#define OFLAG_LAWFUL                2
#define OFLAG_CHAOTIC               3
#define OFLAG_EVIL                  4
#define OFLAG_IS_INVIS              5
#define OFLAG_MAGIC                 6
#define OFLAG_NODROP                7
#define OFLAG_SANCT                 8
#define OFLAG_FLAMING               9
#define OFLAG_BACKSTAB             10
#define OFLAG_NO_DISARM            11
#define OFLAG_NOREMOVE             12
#define OFLAG_AIR_RISE             13
#define OFLAG_DIVINE               14
#define OFLAG_NO_MAJOR             15
#define OFLAG_NOSHOW               16
#define OFLAG_NOSACRIFICE          17
#define OFLAG_WATER_PROOF          18
#define OFLAG_AIR_FALL             19
#define OFLAG_NO_SELL              20
#define OFLAG_NO_JUNK              21
#define OFLAG_IDENTIFIED           22
#define OFLAG_RUST_PROOF           23
#define OFLAG_WATER_FLOAT          24 
#define OFLAG_WATER_SINK           25
#define OFLAG_NOSAVE               26
#define OFLAG_BURNING              27
#define OFLAG_ADDITIVE             28
#define OFLAG_GOOD                 29
#define OFLAG_THE_BEFORE           30
#define OFLAG_REPLICATE            31
#define OFLAG_KNOWN_LIQUID         32
#define OFLAG_POISON_COATED        33
#define OFLAG_NO_AUCTION           34
#define OFLAG_NO_ENCHANT           35
#define OFLAG_COPIED               36
#define OFLAG_RANDOM_METAL         37
#define OFLAG_COVER                38
#define OFLAG_TWO_HAND             39
#define OFLAG_ONE_OWNER            40
#define OFLAG_NO_STEAL             41
#define OFLAG_PASS_THROUGH         42
#define OFLAG_NO_RESET             43
#define OFLAG_SECRET               44
#define OFLAG_TOOL_DIG             45
#define OFLAG_TOOL_MINE            46
#define OFLAG_THE_AFTER            47
#define MAX_OFLAG                  48

#define RESTR_BLADED                0
#define RESTR_NO_HIDE               1
#define RESTR_NO_SNEAK              2
#define RESTR_DISHONORABLE          3
#define MAX_RESTRICTION             4


/*
 *   OBJ_CLSS_DATA 
 */


extern obj_clss_data* obj_index_list [ MAX_OBJ_INDEX ];
extern int obj_clss_max;


class Obj_Clss_Data
{
public:
  affect_array        affected;
  extra_array      extra_descr;
  oprog_data*            oprog;
  char*               singular;
  char*                 plural;
  char*                 before;
  char*                  after;
  char*        prefix_singular;
  char*          prefix_plural;
  char*                 long_s;  
  char*                 long_p;
  char*                creator;
  const char         *last_mod;
  time_t                  date;
  int                     vnum;
  int                   serial;
  int                    fakes;
  int                item_type;
  int              extra_flags  [ 2 ];
  int               size_flags;
  int             restrictions;
  int               anti_flags;
  int                materials;
  int               wear_flags;
  int              layer_flags;
  int             affect_flags  [ AFFECT_INTS ];
  int                    count;
  int                    limit;
  int                   weight;
  int                     cost;
  int                    level;
  int                   remort;
  int                    value  [ 4 ];
  int                   repair;
  int               durability;   
  int                   blocks;
  int                    light;
  const char         *comments;
  bool                    used;

  static int modified;

  Obj_Clss_Data   ( int );
  ~Obj_Clss_Data  ( );

  const char*  Name      ( int = 1, bool = false, bool = true ) const;
  const char*  Keywords  ( );

  int          metal     ( ) const;
  int      any_metal     ( ) const;

  void set_modified( char_data *ch );

  bool is_wearable( ) const;
  bool is_container( ) const;
};


bool   can_extract    ( obj_clss_data*, char_data* );
void   load_objects   ( void );
void   junk_obj       ( char_data*, obj_data* );

const char*  after_descr   ( obj_clss_data* );
const char*  before_descr  ( obj_clss_data* );


/*
 *   OBJ_DATA 
 */


extern obj_array obj_list;


class Obj_Data : public thing_data
{
public:
  obj_clss_data*     pIndexData;
  Save_Data*               save;
  char*                  source;
  char*                   label;
  char*                singular;
  char*                  plural;
  char*                  before;
  char*                   after;
  pfile_data*             owner;
  int               extra_flags  [ 2 ];
  int                size_flags;
  int                 materials;
  int                    weight;
  int                      cost;
  int                     value  [ 4 ];
  int                     timer;
  int                 condition;
  int                      rust;	// unsigned char?
  int                       age;  
  int                     layer;
  int                  position;
  int                     light;
  reset_data             *reset;
  bool                 for_sale;
  bool                     sold;

  Obj_Data( obj_clss_data* );
  virtual ~Obj_Data( );

  /* BASIC */

  virtual int Number( ) const;
  virtual int Selected( ) const;
  virtual int Shown( ) const;

  virtual void Set_Number( int num );
  virtual void Select( int num );
  virtual void Select_All( );
  virtual void Show( int num );

  virtual int   Type     ( ) const
    { return OBJ_DATA; }

  virtual void  Extract  ( );

  void  Extract  ( int );

  /* NAME/KEYWORDS */

  virtual const char*  Keywords        ( char_data* );
  virtual const char*  Name            ( const char_data* = 0, int = 1, bool = false ) const;
  virtual const char*  Seen_Name       ( const char_data* = 0, int = 1, bool = false ) const;
  virtual const char*  Show_To         ( char_data* );
  virtual const char*  Location        ( Content_Array* = 0 );
  virtual void         Look_At         ( char_data* ); 
  virtual bool         Seen            ( const char_data* ) const;

  const char*  condition_name  ( char_data*, bool = false, int cond = -1 );
 
  /* TO/FROM */

  virtual void         To              ( Content_Array& );
  virtual void         To              ( thing_data* = 0 );
  virtual thing_data*  From            ( int = 1, bool = false );

  void         transfer_object ( char_data*, char_data*, Content_Array&, int = 1 ); 

  /* PROPERTIES */

  virtual int          Count           ( int = -1 ) const;
  virtual int          Light           ( int = -1 ) const;
  virtual int          Weight          ( int = -1 );
  virtual int          Empty_Weight    ( int = -1 );
  virtual int          Capacity        ( );
  virtual int          Empty_Capacity  ( ) const;

  int          Level           ( ) const;
  int          Durability      ( ) const;
  bool         Damaged         ( ) const;
  int          Enchantment     ( ) const;
  int          Cost            ( ) const;
  bool         Belongs         ( pfile_data* );
  bool         Belongs         ( char_data* );

  bool droppable( const char_data *ch ) const;
  bool removable( const char_data *ch ) const;

  bool paper( ) const     { return  is_set( materials, MAT_PAPER ); }
  bool glass( ) const     { return  is_set( materials, MAT_GLASS ); }
  bool wood( ) const      { return  is_set( materials, MAT_WOOD ); }
  bool stone( ) const     { return  is_set( materials, MAT_STONE ); }

  int  metal           ( ) const;
  int  any_metal       ( ) const;
  //  bool is_cloth        ( );
  //  bool is_leather      ( );

  /* SAVING THROWS */

  int vs_fire ( );
  int vs_acid ( );
  int vs_cold ( );

private:
  int               number;
  int             selected;
  int                shown;
};


/*
 *   FUNCTIONS
 */

void browse ( char_data*, const char*, Content_Array*, const char* );

void         consolidate        ( obj_data*, bool = true );

obj_data*    create             ( obj_clss_data*, int = 1 );
obj_data*    duplicate          ( obj_data*, int = 1 );

void         set_owner          ( obj_data*, pfile_data*, pfile_data* );
void         set_owner          ( thing_array&, pfile_data*, pfile_data* );
void         set_owner          ( obj_data*, char_data*, char_data* );
void         set_owner          ( thing_array&, char_data*, char_data* );
void         set_owner          ( pfile_data*, thing_array& );
  
bool         read_object        ( FILE*, Content_Array&, Save_Data*, char* );
void         write_object       ( FILE*, Content_Array&, Save_Data*, bool );

bool         is_same            ( const obj_data*, const obj_data*, bool = true );

int          armor_class        ( obj_data*, bool ident = true );
void         rust_object        ( obj_data*, int, bool = false );
void         set_quality        ( obj_data* );
void         set_size           ( obj_data*, char_data* = 0 );
void         condition_abbrev   ( char*, obj_data*, char_data* );
void         age_abbrev         ( char*, obj_data*, char_data* );
bool         newbie_abuse       ( char_data* );
const char*  light_name         ( int );
bool         valid_container    ( char_data*, obj_data*, bool );
void         get_obj            ( char_data*, thing_array&, obj_data* = 0, bool = true );
void         drop               ( char_data*, thing_array&, bool pager = true );
void         junk               ( char_data*, thing_array&, bool pager = true );

void transfer_objects ( char_data*, Content_Array&, char_data*, thing_array&  );

void setup_cache                ( char_data*, obj_data* );


/*
 *   OBJECT ARGUMENTS
 */


obj_data*    find_type         ( char_data*, thing_array&, int ); 
obj_data*    find_vnum         ( thing_array&, int ); 
obj_data*    find_oflag        ( thing_array&, int ); 


/*
 *   ARRAY ROUTINES
 */


void         rehash             ( char_data*, thing_array&, bool = false );
void         rehash_weight      ( char_data*, thing_array&, bool = false );
void         sort_objects       ( char_data*, thing_array&, thing_data*, int,
                                  thing_array*, thing_func** );

void         page_publ          ( char_data*, thing_array*, const char*,
                                  thing_data* = 0,
                                  const char* = empty_string,
                                  const char* = empty_string,
				  const char* = 0,
				  bool = true );
void         page_priv          ( char_data*, thing_array*, const char*,
                                  thing_data* = 0,
                                  const char* = empty_string,
                                  const char* = empty_string,
				  bool = true );

void         send_priv          ( char_data*, thing_array*, const char*,
                                  thing_data* );
void         send_publ          ( char_data*, thing_array*, const char*,
                                  const char* );

thing_func      stolen;
thing_func stolen_contents;
thing_func      corpse;
thing_func      cursed;
thing_func        same;
thing_func    wont_fit;
thing_func   cant_take;
thing_func      on_top;
thing_func      sat_on;
thing_func        many;
thing_func       heavy;
thing_func   forbidden;
thing_func        drop;
thing_func     no_room;
thing_func     to_char;
thing_func  levellimit;
thing_func     antiobj;


extern const default_data use_msg [];
extern const default_data unlock_msg [];
extern const default_data lock_msg [];
extern const default_data consume_msg [];
extern const default_data timer_msg [];


/*
 *   INLINE UTILITIES
 */


inline obj_clss_data* get_obj_index( int vnum )
{
  if( vnum <= 0 || vnum > obj_clss_max ) 
    return 0;

  return obj_index_list[vnum];
}


inline int repair_condition( const obj_data* obj )
{
  return obj->Durability() - 5*obj->age;
}


/*
 *   FOOD ROUTINES
 */


extern const char* cook_word [];


bool can_eat   ( char_data*, obj_data*, bool );
bool eat       ( char_data*, obj_data* );
bool can_drink ( char_data*, obj_data*, bool );
bool would_drink ( char_data*, obj_data* );
bool drink     ( char_data*, obj_data* );
void fill      ( char_data*, obj_data*, obj_data* );
void empty     ( char_data*, thing_array& );


/*
 *   NAME ROUTINES
 */


extern bool include_empty;
extern bool include_liquid;
extern bool include_closed;


/*
 *   MONEY ROUTINES
 */


int   monetary_value    ( obj_data* );
bool  remove_silver     ( char_data* );
void  split_money       ( char_data*, int, bool );
void  add_coins         ( char_data*, int, const char* = 0, bool = false );
bool  remove_coins      ( char_data*, int, const char* = 0, bool = false ); 


/*
 *   WEIGHT FUNCTIONS
 */


//bool   can_carry         ( char_data*, obj_data*, bool = true );


#endif // tfe_object_h
