#ifndef tfe_program_h
#define tfe_program_h

/*
 *   BASE PROGRAM CLASS
 */


class arg_type;


class program_data
{
public:
  arg_type*      binary;
  mem_block*     memory;
  int            active;
  bool          corrupt;
  
  program_data( );
  virtual ~program_data( );

  virtual void compile( );
  virtual void decompile( );
  virtual int execute( thing_data* = 0 ) = 0;
  virtual void display( char_data* ) const = 0;

  virtual bool abort( thing_data* ) const
  { return false; }

  virtual void read( FILE* );
  virtual void write( FILE* );

  virtual void Set_Code( const char * );
  virtual void Read_Code( FILE* );
  virtual void Edit_Code( char_data *, const char * );
  virtual const char *Code( ) const;

  virtual extra_array& Extra_Descr( );

  extra_data *find_extra( const char * );

private:
  const char *code;
  extra_array data;
};


extern program_data *curr_prog;


class default_data
{
public:
  const char *name;
  const char *msg;
  int type;
};


const char*   prog_msg       ( program_data*, const default_data& );
void          show_defaults  ( char_data*, int, const default_data**, int );


/*
 *   ACTION HEADER
 */


#define TRIGGER_NONE                0
#define TRIGGER_ENTERING            1
#define TRIGGER_RANDOM              2
#define TRIGGER_LEAVING             3
#define TRIGGER_RANDOM_ALWAYS       4
#define TRIGGER_SACRIFICE           5
#define TRIGGER_TIME                6
#define TRIGGER_ATTACK              7
#define TRIGGER_OPEN_DOOR           8
#define TRIGGER_SEARCHING           9
#define TRIGGER_CLOSE_DOOR         10
#define TRIGGER_LOCK_DOOR          11
#define TRIGGER_UNLOCK_DOOR        12
#define TRIGGER_KNOCK_DOOR         13
#define TRIGGER_SUN                14
#define TRIGGER_MOON               15
#define TRIGGER_CAST               16
#define TRIGGER_DESCRIBE           17
#define TRIGGER_RESET              18
#define TRIGGER_PATH               19
#define TRIGGER_INIT               20
#define MAX_ATN_TRIGGER            21


#define AFLAG_LOOP_FOLLOWER   0
#define AFLAG_MOUNTED         1
#define AFLAG_RESTING         2
#define AFLAG_SLEEPING        3
#define AFLAG_FIGHTING        4
#define AFLAG_INVENTORY       5
#define AFLAG_WORN            6
#define AFLAG_ROOM            7


void   action_update       ( );
void   random_update       ( );
bool   check_actions       ( char_data*, const char *, const char *, int );


class action_data : public program_data
{
public:
  action_data*      next;
  int            trigger;
  int              value;
  int              flags;
  const char*    command;
  const char*     target;
  room_data*        room;
  
  action_data( room_data *room );
  virtual ~action_data( );

  void validate( );
  void invalidate( );

  virtual void compile( );
  virtual void decompile( );
  virtual int execute( thing_data* = 0 );
  virtual void display( char_data* ) const;

  virtual void Set_Code( const char * );
  virtual void Edit_Code( char_data *, const char * );
  virtual const char *Code( ) const;

  virtual extra_array& Extra_Descr( );
};


bool action_target( const action_data*, int, int );
const char *Keywords( const action_data* );


/* 
 *   MPROG HEADER
 */


#define MPROG_TRIGGER_ENTRY         0
#define MPROG_TRIGGER_LEAVING       1
#define MPROG_TRIGGER_ASKING        2
#define MPROG_TRIGGER_BLOCK         3
#define MPROG_TRIGGER_DEATH         4
#define MPROG_TRIGGER_KILL          5
#define MPROG_TRIGGER_GIVE          6
#define MPROG_TRIGGER_RESET         7
#define MPROG_TRIGGER_TELL          8
#define MPROG_TRIGGER_SKIN          9
#define MPROG_TRIGGER_TIMER        10
#define MPROG_TRIGGER_ATTACK       11
#define MPROG_TRIGGER_ORDER        12
#define MPROG_TRIGGER_NONE         13
#define MPROG_TRIGGER_CAST         14
#define MPROG_TRIGGER_DESCRIBE     15
#define MPROG_TRIGGER_TO_ROOM      16
#define MPROG_TRIGGER_PATH         17
#define MPROG_TRIGGER_SUMMON       18
#define MPROG_TRIGGER_PRACTICE     19
#define MAX_MPROG_TRIGGER          20


//void extract   ( mprog_data*, wizard_data* );


class Attack_Data: public program_data
{
public:
  species_data *species;

  Attack_Data( species_data *species );
  virtual ~Attack_Data( );

  virtual int execute( thing_data* = 0 );
  virtual void display( char_data* ) const;
  virtual bool abort( thing_data* ) const;
};


class Mprog_Data : public program_data
{
public:
  mprog_data*        next;
  int             trigger;
  int               value;
  const char      *string;
  species_data*   species;

  Mprog_Data( species_data *species );
  virtual ~Mprog_Data( );

  virtual int execute( thing_data* = 0 );
  virtual void display( char_data* ) const;
};


/*
 *   OBJECT PROGRAM HEADER
 */


#define OPROG_TRIGGER_PUT           0
#define OPROG_TRIGGER_GET           1
#define OPROG_TRIGGER_TIMER         2
#define OPROG_TRIGGER_HIT           3
#define OPROG_TRIGGER_NONE          4
#define OPROG_TRIGGER_TO_ROOM       5
#define OPROG_TRIGGER_ENTERING      6
#define OPROG_TRIGGER_WEAR          7
#define OPROG_TRIGGER_CONSUME       8
#define OPROG_TRIGGER_SIT           9
#define OPROG_TRIGGER_RANDOM       10
#define OPROG_TRIGGER_UNTRAP       11
#define OPROG_TRIGGER_USE          12
#define OPROG_TRIGGER_UNLOCK       13
#define OPROG_TRIGGER_LOCK         14
#define OPROG_TRIGGER_THROW        15
#define OPROG_TRIGGER_LEAVING      16
#define OPROG_TRIGGER_REMOVE       17
#define OPROG_TRIGGER_LOAD         18
#define OPROG_TRIGGER_CAST         19
#define OPROG_TRIGGER_RESET        20
#define OPROG_TRIGGER_DESCRIBE     21
#define OPROG_TRIGGER_GET_FROM     22
#define MAX_OPROG_TRIGGER          23


#define OPFLAG_INVENTORY  0
#define OPFLAG_WORN       1
#define OPFLAG_ROOM       2
#define MAX_OPFLAG        3


class Oprog_Data : public program_data
{
public:
  oprog_data*          next;
  obj_clss_data*    obj_act;
  int              obj_vnum;
  int               trigger;
  const char        *target;
  const char       *command;
  int                 value;
  int                 flags;
  obj_clss_data*   obj_clss;

  Oprog_Data( obj_clss_data *clss );
  virtual ~Oprog_Data( );

  virtual int execute( thing_data* = 0 );
  virtual void display( char_data* ) const;
  virtual bool abort( thing_data* ) const;
};


/*
 *   OBJECT PROGRAM HEADER
 */


class Table_Data;

class Tprog_Data : public program_data
{
public:
  int table;
  int entry;

  Tprog_Data( int, int );
  virtual ~Tprog_Data( );

  virtual int execute( thing_data* = 0 );
  virtual void display( char_data* ) const;
  //  virtual bool abort( thing_data* ) const;
};


/*
 *   SUPPORT FUNCTIONS
 */


enum {
  search_acode,
  search_attack,
  search_mprog,
  search_oprog,
  search_tprog
};
  

typedef bool search_type ( char_data*, const arg_type*, void*,
			   int, int, const char*, int, const char* );

search_type search_func;
search_type search_room;
search_type search_mload;
search_type search_oload;
search_type search_quest;
search_type search_cflag;


#endif // tfe_program_h
