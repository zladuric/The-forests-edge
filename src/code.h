#ifndef tfe_code_h
#define tfe_code_h

/*
 *   TYPE DECLARATIONS
 */


// *** Warning! Do not modify this list without considering
//     the effect on the Function table.
enum arg_enum {
  NONE, ANY, STRING, INTEGER, NULL_POINTER,
  CHARACTER, OBJECT, ROOM, DIRECTION, NATION,
  SKILL, RFLAG, STAT, CLASS, RELIGION,
  RACE, THING, AFFECT, OFLAG, SEX,
  COLOR, WEAR_PART, GROUP, DFLAG, EXIT,
  WEAR_LAYER, OBJ_TYPE, THING_LIST, ACT_FLAG,
  OPROG_TRIG, MPROG_TRIG, ACODE_TRIG
};

#define MAX_ARG (ACODE_TRIG+1)


enum fam_enum {
  null, variable, constant, function, if_clause,
  end, cont, loop, assign, op, conv, ret//, decl
};


enum loop_enum { loop_all_in_room,
		 loop_followers,
		 loop_all_on_chair,
		 loop_rooms_in_area,
		 loop_unknown };


typedef const void* cfunc     ( const void** );


#define MAX_CFUNC_ARG 6


/*
 *   VARIABLES
 */


class Var_Data
{
public:
  char*        name;
  void*        pointer;
  arg_enum     type;  
};


extern const var_data variable_list [];


const var_data *find_variable( const char* );


/*
 *   QUEUE CLASS
 */


class queue_data
{
public:
  queue_data*        next;
  arg_type*           arg;

  char_data*           ch;
  char_data*          rch;
  char_data*       victim;
  char_data*          mob;
  obj_data*           obj;
  room_data*         room;
  int                   i;
  int                   j;
  int                   k;
  int                   l;
  int                   m;
  int                   n;
  obj_data       *act_obj;
  obj_data*     container;
  const char        *vcmd;
  const char        *varg;
  exit_data*         exit;
  const default_data *def;
  int            def_type;	// char?
  thing_data       *thing;
  thing_array        list;
  char_data     *o_victim;
  char_data         *o_ch;

  program_data   *program;
  thing_data       *owner;
  int                time;

  queue_data( ) {
    record_new( sizeof( queue_data ), MEM_QUEUE );
  }

  ~queue_data( ) {
    record_delete( sizeof( queue_data ), MEM_QUEUE );
  }
};


/*
 *   Function calls
 */


class cfunc_type
{
public:
  const char *name;
  cfunc*    func_call;
  arg_enum  type;
  arg_enum  arg [ MAX_CFUNC_ARG ];
};


/*
 *   ARGUMENT TYPES
 */


class arg_type
{
public:
  arg_enum      type;	// unsigned char?
  fam_enum    family;	// unsigned char?
  const void  *value;
  arg_type*     next;
  bool     logic_neg;
  bool     arith_neg;
  bool bit_neg;

  arg_type( fam_enum fam )
    : type(NONE), family(fam), value(0), next(0),
    logic_neg(false), arith_neg(false), bit_neg( false )
  {
    record_new( sizeof( arg_type ), MEM_PROGRAM );
  }
  
  virtual ~arg_type( )
  {
    record_delete( sizeof( arg_type ), MEM_PROGRAM );
    delete next;
  }
};


class loop_type : public arg_type
{
public:
  loop_enum       fruit;
  arg_type*       aloop;
  arg_type*   condition;

  loop_type( )
    : arg_type(loop),
      fruit(loop_unknown), aloop(0), condition(0)
  {
    record_new( sizeof( loop_type ), MEM_PROGRAM );
    record_delete( sizeof( arg_type ), MEM_PROGRAM );
  }
  
  virtual ~loop_type( )
  {
    record_new( sizeof( arg_type ), MEM_PROGRAM );
    record_delete( sizeof( loop_type ), MEM_PROGRAM );
    delete aloop;
    delete condition;
  }
};


class aif_type : public arg_type
{
public:
  arg_type*  condition;
  arg_type*        yes;
  arg_type*         no;        

  aif_type( )
    : arg_type(if_clause),
      condition(0), yes(0), no(0)
  {
    record_new( sizeof( aif_type ), MEM_PROGRAM );
    record_delete( sizeof( arg_type ), MEM_PROGRAM );
  }

  virtual ~aif_type( )
  {
    record_new( sizeof( arg_type ), MEM_PROGRAM );
    record_delete( sizeof( aif_type ), MEM_PROGRAM );
    delete condition;
    delete yes;
    delete no;
  }
};


class assign_type : public arg_type
{
public:
  const cfunc_type*  func;
  arg_type*           arg  [2];
  
  assign_type( )
    : arg_type(assign), func(0)
  {
    record_new( sizeof( assign_type ), MEM_PROGRAM );
    record_delete( sizeof( arg_type ), MEM_PROGRAM );
    vzero( arg, 2 ); 
  }
    
  virtual ~assign_type( )
  {
    record_new( sizeof( arg_type ), MEM_PROGRAM );
    record_delete( sizeof( assign_type ), MEM_PROGRAM );
    for( int i = 0; i < 2; ++i )
      delete arg[i];
  }
};


class op_type : public arg_type
{
public:
  const cfunc_type*  func;
  arg_type*           arg  [2];
  
  op_type( )
    : arg_type(op), func(0)
  {
    record_new( sizeof( op_type ), MEM_PROGRAM );
    record_delete( sizeof( arg_type ), MEM_PROGRAM );
    vzero( arg, 2 ); 
  }
    
  virtual ~op_type( )
  {
    record_new( sizeof( arg_type ), MEM_PROGRAM );
    record_delete( sizeof( op_type ), MEM_PROGRAM );
    for( int i = 0; i < 2; ++i )
      delete arg[i];
  }
};


class afunc_type : public arg_type
{
public:
  const cfunc_type*  func;
  arg_type*           arg  [ MAX_CFUNC_ARG ];
  
  afunc_type( )
    : arg_type(function), func(0)
  {
    record_new( sizeof( afunc_type ), MEM_PROGRAM );
    record_delete( sizeof( arg_type ), MEM_PROGRAM );
    vzero( arg, MAX_CFUNC_ARG ); 
  }
    
  virtual ~afunc_type( )
  {
    record_new( sizeof( arg_type ), MEM_PROGRAM );
    record_delete( sizeof( afunc_type ), MEM_PROGRAM );
    for( int i = 0; i < MAX_CFUNC_ARG; ++i )
      delete arg[i];
  }
};


class conv_type : public arg_type
{
public:
  const cfunc_type *func;
  arg_type *arg;
  
  conv_type( )
    : arg_type(conv), func(0), arg(0)
  {
    record_new( sizeof( conv_type ), MEM_PROGRAM );
    record_delete( sizeof( arg_type ), MEM_PROGRAM );
  }
    
  virtual ~conv_type( )
  {
    record_new( sizeof( arg_type ), MEM_PROGRAM );
    record_delete( sizeof( conv_type ), MEM_PROGRAM );
    delete arg;
  }
};


/*
class decl_type : public arg_type
{
public:
  arg_enum var_type;
  const char *var_name;
  void *value;

  decl_type( )
    : arg_type( decl ), var_type( NONE ), var_name( 0 )
  {
    record_new( sizeof( decl_type ), MEM_PROGRAM );
    record_delete( sizeof( arg_type ), MEM_PROGRAM );
  }

  ~decl_type( )
  {
    record_new( sizeof( decl_type ), MEM_PROGRAM );
    record_delete( sizeof( afunc_type ), MEM_PROGRAM );
    free_string( var_name, MEM_PROGRAM );
    if( var_type == STRING ) {
      free_string( (const char*)value, MEM_PROGRAM );
    }
  }
};
*/


class stack_data
{
public:
  stack_data*    next;

  char_data*       ch;
  char_data*   victim;
  obj_data*       obj;
  room_data*     room;
  char_data*      mob;
  char_data*      rch;
  int               i;
  int               j;
  int               k;
  int               l;
  int               m;
  int               n;
  obj_data   *act_obj;
  obj_data *container;
  const char    *vcmd;
  const char    *varg;
  exit_data     *exit;
  const default_data *def;
  int def_type;		// char?
  thing_data   *thing;
  thing_array    list;
  char_data     *o_victim;
  char_data         *o_ch;

  stack_data( ) {
    record_new( sizeof( stack_data ), MEM_STACK );
  }

  ~stack_data( ) {
    record_delete( sizeof( stack_data ), MEM_STACK );
  }
};


/*
 *   CODE RELATED FUNCTIONS
 */

void         code_bug        ( const char* );
void         pop             ( bool discard = false );
void         push            ( );
void         clear_queue     ( program_data* );
void         clear_variables ( );
int          find_cfunc      ( const char *& );

extern bool               end_prog;
extern bool              cont_prog;
extern bool            return_prog;
extern bool             queue_prog;
extern bool               now_prog;
extern queue_data*      queue_list;
extern queue_data*        now_list;
extern char              error_buf  [ MAX_INPUT_LENGTH ];
extern mem_block*       block_list;
extern const char*   arg_type_name  [];

extern const cfunc_type cfunc_list [];
extern unsigned max_cfunc;


template < class T >
void code_bug( const char* text, T item )
{
  char tmp [ TWO_LINES ];
 
  snprintf( tmp, TWO_LINES, text, tostring( item, 0 ) );

  if( *text == '%' )
    *tmp = toupper( *tmp );

  code_bug( tmp );
}


template < class S, class T >
void code_bug( const char* text, S item1, T item2 )
{
  char tmp [ TWO_LINES ];
 
  snprintf( tmp, TWO_LINES, text, tostring( item1, 0 ), tostring( item2, 0 ) );

  code_bug( tmp );
}


#endif // tfe_code_h
