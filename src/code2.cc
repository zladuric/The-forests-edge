#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include "define.h"
#include "struct.h"


mem_block*  block_list  = 0;

char error_buf  [ MAX_INPUT_LENGTH ];
static const char *this_code;
static extra_array *this_data;


static arg_type *read_statement( const char*& code );
static arg_type *read_assign( const char*& code );
static arg_type *read_const( const char*& code, const arg_enum type );
static arg_type *read_digit( const char*& code );
static arg_type *read_expression( const char*& code, op_type *up, const arg_enum type = ANY );
static arg_type *read_variable( const char*& code );
static arg_type *read_function( const char*& code );
static arg_type *read_string( const char*& code, extra_array& data );
static arg_type *read_term( const char*& code, const arg_enum type = ANY );


static bool skip_comments( const char *& code )
{
  if( *error_buf )
    return true;

  do {
    skip_spaces( code );
    
    // End-of-line comments.
    if( *code == '/' ) {
      if( *(code+1) == '/' ) {
	code += 2;
	while( *code && *code++ != '\n' );
      } else if( *(code+1) == '*' ) {
	code += 2;
	while( *code && ( *code != '*' || *(code+1) != '/' ) ) {
	  if( *code == '/' && *(code+1) == '*' ) {
	    strcpy( error_buf, "Nested multi-line comments." );
	    return true;
	  }
	  ++code;
	}
	if( !*code ) {
	  strcpy( error_buf, "End of program inside multi-line comment." );
	  return true;
	}
	code += 2;
      }
    }
    
    if( !*code ) 
      return false;

  } while( isspace( *code ) );

  return false;
}


static const cfunc_type convert_list [ ] =
{
  { "",    &code_conv_int_str,           STRING,    { INTEGER, NONE, NONE, NONE, NONE, NONE } },
  { "",    &code_conv_exit_str,          STRING,    { EXIT, NONE, NONE, NONE, NONE, NONE } },
  { "",    &code_conv_int_dir,           DIRECTION, { INTEGER, NONE, NONE, NONE, NONE, NONE } },
  { "",    &code_conv_dir_int,           INTEGER,   { DIRECTION, NONE, NONE, NONE, NONE, NONE } },
  { 0, 0 }
};


static bool can_assign( arg_enum t1, arg_enum t2, const cfunc_type *& conv )
{
  conv = 0;

  if( t1 == INTEGER )
    return t2 != THING_LIST;

  if( t1 == THING )
    return( t2 == CHARACTER || t2 == OBJECT || t2 == ROOM || t2 == NULL_POINTER || t2 == THING );

  if( ( t1 == CHARACTER || t1 == OBJECT || t1 == ROOM || t1 == STRING )
      && t2 == NULL_POINTER )
    return true;

  for( int i = 0; convert_list[i].name; ++i ) {
    if( t1 == convert_list[i].type && t2 == convert_list[i].arg[0] ) {
      conv = &convert_list[i];
      return true;
    }
  }

  return( t1 == t2 );  
}


static const cfunc_type op_list [ ] =
{
  { "||",    &code_or,           INTEGER, { NONE,    NONE,    NONE,  NONE, NONE,  NONE }  },
  { "&&",    &code_and,          INTEGER, { NONE,    NONE,    NONE,  NONE, NONE,  NONE }  },
  { "\\|",   &code_bit_or,       INTEGER, { INTEGER, INTEGER, NONE,  NONE, NONE,  NONE }  },
  { "\\^",   &code_bit_xor,      INTEGER, { INTEGER, INTEGER, NONE,  NONE, NONE,  NONE }  },
  { "\\&",   &code_bit_and,      INTEGER, { INTEGER, INTEGER, NONE,  NONE, NONE,  NONE }  },
  { "==",    &code_streq,        INTEGER, { STRING,  STRING,  NONE,  NONE, NONE,  NONE }  },
  { "!=",    &code_strneq,       INTEGER, { STRING,  STRING,  NONE,  NONE, NONE,  NONE }  },
  { "==",    &code_is_equal,     INTEGER, { ANY,     ANY,     NONE,  NONE, NONE,  NONE }  },
  { "!=",    &code_not_equal,    INTEGER, { ANY,     ANY,     NONE,  NONE, NONE,  NONE }  },
  { ">=",    &code_ge,           INTEGER, { INTEGER, INTEGER, NONE,  NONE, NONE,  NONE }  },
  { "<=",    &code_le,           INTEGER, { INTEGER, INTEGER, NONE,  NONE, NONE,  NONE }  },
  { "<<",    &code_lshift,       INTEGER, { INTEGER, INTEGER, NONE,  NONE, NONE,  NONE }  },
  { ">>",    &code_rshift,       INTEGER, { INTEGER, INTEGER, NONE,  NONE, NONE,  NONE }  },
  { ">",     &code_gt,           INTEGER, { INTEGER, INTEGER, NONE,  NONE, NONE,  NONE }  },
  { "<",     &code_lt,           INTEGER, { INTEGER, INTEGER, NONE,  NONE, NONE,  NONE }  },
  { "+",     &code_add,          INTEGER, { INTEGER, INTEGER, NONE,  NONE, NONE,  NONE }  },
  { "-" ,    &code_subtract,     INTEGER, { INTEGER, INTEGER, NONE,  NONE, NONE,  NONE }  },
  { "*" ,    &code_multiply,     INTEGER, { INTEGER, INTEGER, NONE,  NONE, NONE,  NONE }  },
  { "/" ,    &code_divide,       INTEGER, { INTEGER, INTEGER, NONE,  NONE, NONE,  NONE }  },
  { "%" ,    &code_remainder,    INTEGER, { INTEGER, INTEGER, NONE,  NONE, NONE,  NONE }  },
  { "",      0,                  NONE,    { NONE,    NONE,    NONE,  NONE, NONE,  NONE }  }
};


static int op_prec_list [ ] =
{
  12,
  11,
  10,
  9,
  8,
  7,
  7,
  7,
  7,
  6,
  6,
  5,
  5,
  6,
  6,
  4,
  4,
  3,
  3,
  3
};


arg_type *read_term( const char*& code, const arg_enum type )
{
  if( skip_comments( code ) ) {
    return 0;
  }

  bool arith_neg = false;
  bool logic_neg = false;
  bool bit_neg = false;

  if( *code == '-' ) {
    arith_neg = true;
    ++code;
    if( skip_comments( code ) ) {
      return 0;
    }
  } else if( *code == '+' ) {
    ++code;
    if( skip_comments( code ) ) {
      return 0;
    }
  } else if( *code == '!' ) {
    logic_neg = true;
    ++code;
    if( skip_comments( code ) ) {
      return 0;
    }
  } else if( *code == '~' ) {
    bit_neg = true;
    ++code;
    if( skip_comments( code ) ) {
      return 0;
    }
  }

  arg_type *arg = 0;

  if( *code == '(' ) {
    ++code;
    skip_comments( code );
    if( !( arg = read_assign( code ) )
	&& !*error_buf ) {
      arg = read_expression( code, 0, type );
    }
    if( *error_buf ) {
      delete arg;
      return 0;
    }
    if( skip_comments( code ) ) {
      delete arg;
      return 0;
    }
    if( *code != ')' ) {
      strcpy( error_buf, "Missing close parenthesis in expression." ); 
      delete arg;
      return 0;
    }
    ++code;
    if( skip_comments( code ) ) {
      delete arg;
      return 0;
    }
  }
  
  if( !arg ) {
    // Note: read_function() must be done before read_const().
    arg = read_function( code );
    if( *error_buf ) {
      delete arg;
      return 0;
    }
  }
  
  if( !arg ) {
    arg = read_const( code, type );
  }
  
  if( !arg ) {
    arg = read_variable( code );
  }
  
  if( !arg ) {
    arg = read_digit( code );
    if( *error_buf ) {
      delete arg;
      return 0;
    }
  }
  
  if( !arg ) {
    arg = read_string( code, *this_data );
    if( *error_buf ) {
      delete arg;
      return 0;
    }
  }
    
  if( !arg ) {
    strcpy( error_buf, "Syntax error in expression." ); 
    delete arg;
    return 0;
  }

  if( arith_neg ) {
    if( arg->type != INTEGER ) {
      strcpy( error_buf, "Unary '-' requires integer operand." );
      delete arg;
      return 0;
    }
    if( arg->family == constant )
      arg->value = (void*) -(int)(arg->value);
    else
      arg->arith_neg = true;
  } else if( logic_neg ) {
    arg->logic_neg = true;
  } else if( bit_neg ) {
    if( arg->type != INTEGER ) {
      strcpy( error_buf, "Unary '~' requires integer operand." );
      delete arg;
      return 0;
    }
    if( arg->family == constant )
      arg->value = (void*) ~(int)(arg->value);
    else
      arg->bit_neg = true;
  }

  if( skip_comments( code ) ) {
    delete arg;
    return 0;
  }

  return arg;
}


arg_type *read_expression( const char*& code, op_type *up, const arg_enum type )
{
  // read lhs.

  arg_type *term = read_term( code, type );

  if( !term )
    return 0;

  int i;
  for( i = 0; ; ++i ) {
    if( !*op_list[i].name ) {
      return term;
    }
    const int l = strlen( op_list[i].name );
    if( !strncmp( code, op_list[i].name, l )
	&& ( op_list[i].arg[0] == NONE
	     || op_list[i].arg[0] == ANY
	     || op_list[i].arg[0] == term->type ) ) {
      code += l;
      break;
    }
  }
  
  op_type *oper = new op_type;
  oper->type = INTEGER;
  oper->func = &op_list[i];
  oper->arg[0] = term;

  if( up
      && op_prec_list[i] >= op_prec_list[ up->func - op_list ] ) {
    // New op is lower or equal prec than up.
    return oper;
  }

  // read rhs.

  while( true ) {
    arg_type *arg;
    if( oper->func->arg[0] == ANY ) {
      arg = read_expression( code, oper, term->type );
    } else {
      arg = read_expression( code, oper );
    }
    
    if( !arg ) {
      delete oper;
      return 0;
    }
    
    op_type *right = ( arg->family == op ) ? (op_type*) arg : 0 ;
    
    if( !right
	|| right->arg[1] ) {
      oper->arg[1] = arg;
      if( oper->func->arg[0] != NONE
	  && oper->func->arg[0] == oper->func->arg[1]
	  && oper->arg[0]->type != oper->arg[1]->type ) {
	snprintf( error_buf, MAX_INPUT_LENGTH,
		  "Both sides of operator '%s' must be of identical type.",
		  op_list[oper->func - op_list].name );
	delete oper;
	return 0;
      }
      if( !oper->func->func_call
	  || ( oper->func->arg[0] != NONE
	       && ( oper->func->arg[0] != ANY && oper->func->arg[0] != oper->arg[0]->type
		    || oper->func->arg[1] != ANY && oper->func->arg[1] != oper->arg[1]->type ) ) ) {
	snprintf( error_buf, MAX_INPUT_LENGTH,
		  "Bad argument types for operator '%s'.",
		  op_list[oper->func - op_list].name );
	delete oper;
	return 0;
      }
      return oper;
    }
    
    // rotate the tree for operator precedence.
    oper->arg[1] = right->arg[0];
    right->arg[0] = oper;

    if( oper->func->arg[0] != NONE
	&& oper->func->arg[0] == oper->func->arg[1]
	&& oper->arg[0]->type != oper->arg[1]->type ) {
      snprintf( error_buf, MAX_INPUT_LENGTH,
		"Both sides of operator '%s' must be of identical type.",
		op_list[oper->func - op_list].name );
      delete oper;
      return 0;
    }
    if( !oper->func->func_call
	|| ( oper->func->arg[0] != NONE
	     && ( oper->func->arg[0] != ANY && oper->func->arg[0] != oper->arg[0]->type
		  || oper->func->arg[1] != ANY && oper->func->arg[1] != oper->arg[1]->type ) )  ){
      snprintf( error_buf, MAX_INPUT_LENGTH,
		"Bad argument types for operator '%s'.",
		op_list[oper->func - op_list].name );
      delete oper;
      return 0;
    }

    if( up
	&& op_prec_list[ right->func - op_list] >= op_prec_list[ up->func - op_list ] ) {
      // rhs lower or equal prec than up.
      return right;
    }

    oper = right;
  }
}


// Can have multiple, overloaded cfuncs (same name, different arg types)
// Find the first one.
int find_cfunc( const char *& code )
{
  int min = 0;
  int max = max_cfunc-1;
  int last = -1;
  const char *next = 0;

  while( true ) {
    if( max < min ) {
      if( last != -1 ) {
	code = next;
	skip_spaces( code );
      }
      return last;
    }

    const int mid = (max+min)/2;

    int value;
    const char *s = code;
    const char *n = cfunc_list[mid].name;
    while( true ) {
      // tolower( char ) seems to match what strcasecmp() does,
      // on this machine... not very portable.
      const char sc = tolower( *s );
      const char nc = tolower( *n++ );
      if( !isalpha( sc ) && sc != '_' ) {
	if( nc ) {
	  value = 1;
	} else {
	  value = 0;
	  last = mid;
	  next = s;
	}
	break;
      }
      ++s;
      if( !nc ) {
	value = -1;
	break;
      }
      if( ( value = (int) nc - (int) sc ) )
	break;
    }

    if( value >= 0 ) {
      max = mid-1;
    } else {
      min = mid+1;
    }
  }
}


static arg_type *read_function( const char*& code )
{
  const char *str = code;

  const int k = find_cfunc( code );

  if( k < 0 )
    return 0;

  if( *code != '(' ) {
    code = str;
    return 0;
  }

  ++code;
  int i = k;

  const char *args = code;

  while( true ) {
    afunc_type *afunc = new afunc_type;     
    afunc->type = cfunc_list[i].type;
    afunc->func = &cfunc_list[i];

    int j = 0;
    while( true ) {
      if( skip_comments( code ) ) {
	delete afunc;
	return 0;
      }

      if( *code == ')' ) {
	++code;
	return afunc;
      }

      if( j == MAX_CFUNC_ARG ) {
	snprintf( error_buf, MAX_INPUT_LENGTH,
		  "Too many arguments for function call %s.", cfunc_list[i].name );
	delete afunc;
	return 0;
      }

      if( cfunc_list[i].arg[j] == NONE ) {
	// Too many args for this proto.
	break;
      }

      if( !( afunc->arg[j] = read_expression( code, 0, cfunc_list[i].arg[j] ) ) ) {
	if( !*error_buf )
	  strcpy( error_buf, "Function call missing closing ')'." );
	delete afunc;
	return 0;
      }

      const cfunc_type *conv;
      if( !can_assign( cfunc_list[i].arg[j], afunc->arg[j]->type, conv ) ) {
	// Wrong arg type for this proto.
	break;
      }

      if( conv ) {
	// Argument requires automatic conversion function.
	conv_type *conv_func = new conv_type;
	conv_func->type = conv->type;
	conv_func->func = conv;
	conv_func->arg = afunc->arg[j];
	afunc->arg[j] = conv_func;
      }

      if( *code == ',' ) 
	++code;

      ++j;
    }

    if( !*cfunc_list[i+1].name
	|| !word_match( str, cfunc_list[i+1].name ) ) {
      if( i == k ) {
	// Non-overloaded func, we can be specific.
	if( cfunc_list[i].arg[j] == NONE ) {
	  snprintf( error_buf, MAX_INPUT_LENGTH,
		    "Too many arguments for function call %s.", cfunc_list[i].name );
	} else {
	  snprintf( error_buf, MAX_INPUT_LENGTH,
		    "Passing %s to function %s for argument %d, requires %s.",
		    arg_type_name[ afunc->arg[j]->type ],
		    cfunc_list[i].name, j+1,
		    arg_type_name[ cfunc_list[i].arg[j] ] );
	}

      } else {
	// Overloaded func.
	snprintf( error_buf, MAX_INPUT_LENGTH,
		  "No matching prototype for overloaded function %s.", cfunc_list[i].name );
      }

      delete afunc;
      return 0;
    }

    ++i;
    code = args;
  }
}


static aif_type *read_if( const char*& code )
{
  if( *code != '(' ) {
    strcpy( error_buf, "Missing '(' after if." ); 
    return 0;
  }

  ++code;

  aif_type *aif = new aif_type;

  if( !( aif->condition = read_expression( code, 0 ) ) ) {
    if( !*error_buf ) 
      strcpy( error_buf, "If statement with null condition??" );
    delete aif;
    return 0;  
  }

  if( skip_comments( code ) ) {
    delete aif;
    return 0;  
  }

  if( *code != ')' ) {
    strcpy( error_buf, "If statement missing closing )." );
    delete aif;
    return 0;
  }

  ++code;

  if( !( aif->yes = read_statement( code ) ) ) {
    if( !*error_buf )
      strcpy( error_buf, "If statement with no effect." );
    return 0;
  }
  
  if( skip_comments( code ) ) {
    delete aif;
    return 0;  
  }

  if( word_match( code, "else" ) ) {
    if( !( aif->no = read_statement( code ) ) ) {
      if( !*error_buf )
        strcpy( error_buf, "Else clause with no effect." );
      delete aif;
      return 0;
    }
  }

  return aif;
}


#define LOOP_KINDS 4
static const char* loop_name [ LOOP_KINDS ] = {
  "all_in_room",
  "followers",
  "all_on_chair",
  "rooms_in_area" };


static loop_type *read_loop( const char*& code )
{
  if( *code != '(' ) {
    strcpy( error_buf, "Missing '(' after loop." ); 
    return 0;
  }

  ++code;

  if( skip_comments( code ) ) {
    return 0;
  }

  loop_type *aloop = new loop_type;
  
  for( int i = 0; i < LOOP_KINDS; i++ ) {
    if( word_match( code, loop_name[i] ) ) {
      if( *code != ')' ) {
        strcpy( error_buf, "Missing ')' after loop." ); 
        delete aloop;
        return 0;
      }
      ++code; 
      aloop->fruit = (loop_enum) i;
      if( ( aloop->aloop = read_statement( code ) ) )
        return aloop;
      if( !*error_buf ) 
        strcpy( error_buf, "Error in loop." );
      delete aloop;
      return 0;
    }
  }
  
  aloop->fruit = loop_unknown;
  
  if( !( aloop->condition = read_expression( code, 0 ) ) ) {
    if( !*error_buf ) 
      strcpy( error_buf, "Loop statement with null condition??" );
    delete aloop;
    return 0;  
  }
  
  if( skip_comments( code ) ) {
    delete aloop;
    return 0;
  }

  if( *code != ')' ) {
    strcpy( error_buf, "Loop statement missing closing )." );
    delete aloop;
    return 0;
  }

  ++code;

  if( !( aloop->aloop = read_statement( code ) ) ) {
    if( !*error_buf ) 
      strcpy( error_buf, "Loop statement with null loop." );
    delete aloop;
    return 0;  
  }   

  return aloop;
}  


static bool semicolon( const char *& code, const char *msg )
{
  if( skip_comments( code ) ) {
    return false;
  }

  if( *code != ';' ) {
    snprintf( error_buf, MAX_INPUT_LENGTH, "Missing semicolon after %s statement.", msg );
    return false;
  }

  ++code;
  return true;
}


static arg_type *read_statement( const char *& code )
{
  arg_type*    arg;
  arg_type*   arg1;
  arg_type*   arg2;

  if( skip_comments( code ) || !*code ) {
    return 0;
  }

  if( *code == '{' ) {
    ++code;
    if( skip_comments( code ) ) {
      return 0;
    }
    if( *code == '}' ) {
      ++code;
      return new arg_type( null );
    }
    if( !( arg1 = read_statement( code ) ) ) {
      if( !*error_buf ) {
	strcpy( error_buf, "End of statement block without }." );
      }
      return 0;
    }
    for( arg2 = arg1; ; arg2 = arg2->next ) {
      if( skip_comments( code ) ) {
	delete arg1;
	return 0;
      }
      if( *code == '}' ) {
        ++code;
        return arg1;
      }
      if( !( arg2->next = read_statement( code ) ) ) {
        if( !*error_buf ) {
          strcpy( error_buf, "End of statement block without }." );
	}
        delete arg1;
        return 0;
      }
    }
  }
  
  if( word_match( code, "end" ) ) {
    if( !semicolon( code, "end" ) )
      return 0;
    return new arg_type( end );
  }

  if( word_match( code, "continue" ) ) {
    if( !semicolon( code, "continue" ) )
      return 0;
    return new arg_type( cont );
  }

  if( word_match( code, "return" ) ) {
    if( !semicolon( code, "return" ) )
      return 0;
    return new arg_type( ret );
  }

  if( word_match( code, "loop" ) ) {
    return read_loop( code );
  }

  if( word_match( code, "if" ) ) {
    return read_if( code );
  }

  if( ( arg = read_function( code ) ) ) {
    semicolon( code, "function call" );
    return arg;
  }
  
  if( *error_buf )
    return 0;

  if( ( arg = read_assign( code ) ) ) {
    semicolon( code, "assignment" );
    return arg;
  }

  if( !*error_buf ) 
    strcpy( error_buf, "Syntax error." );

  return 0;
}


/*
 *   CONSTANTS
 */


class Const_Data
{
public:
  const char* const * entry1;
  const char* const * entry2;
  int              *size;
  arg_enum         type;

  const char* entry( int j ) const
  {
    return *(entry1+j*(entry2-entry1));
  }
};


static int max_dir = 6;
static int max_dflag = MAX_DFLAG;
static int max_rflag = MAX_RFLAG;
static int max_stat = 22;
static int max_oflag = MAX_OFLAG;
static int max_sex = MAX_SEX - 1;
static int max_color = MAX_COLOR;
static int max_wear = MAX_WEAR;
static int max_layer = MAX_LAYER+1;
static int max_item_type = MAX_ITEM;
static int max_act = MAX_ACT;
static int max_oprog_trig = MAX_OPROG_TRIGGER;
static int max_mprog_trig = MAX_MPROG_TRIGGER;
static int max_acode_trig = MAX_ATN_TRIGGER;


static const char *stat_name[] = {
  "str", "int", "wis", "dex", "con",
  "level", "piety", "class", "align", "prayer", "quest", "remort",
  "hit", "mana", "move",
  "max_hit", "max_mana", "max_move",
  "hunger", "thirst", "alcohol", "drunk"
};


static const const_data const_list [ MAX_ARG ] = 
{
  {  0,                       0,                       0              , NONE },
  {  0,                       0,                       0              , NONE },
  {  0,                       0,                       0              , NONE },
  {  0,                       0,                       0              , NONE },
  {  0,                       0,                       0              , NONE },
  {  0,                       0,                       0              , NONE },
  {  0,                       0,                       0              , NONE },
  {  0,                       0,                       0              , NONE },
  {  &dir_table[0].name,      &dir_table[1].name,      &max_dir        ,DIRECTION },
  {  &nation_table[0].name,   &nation_table[1].name,   &table_max[ TABLE_NATION ], NATION },
  {  &empty_string,           &empty_string,           0              , SKILL },
  {  &rflag_name[0],          &rflag_name[1],          &max_rflag     , RFLAG },
  {  &stat_name[0],           &stat_name[1],           &max_stat      , STAT },
  {  &clss_table[0].name,     &clss_table[1].name,     &table_max[ TABLE_CLSS ], CLASS },
  {  &religion_table[0].name, &religion_table[1].name, &table_max[ TABLE_RELIGION ], RELIGION },
  {  &race_table[0].name,     &race_table[1].name,     &table_max[ TABLE_RACE ], RACE },
  {  0,                       0,                       0              , NONE },
  {  &aff_char_table[0].name, &aff_char_table[1].name, &table_max[ TABLE_AFF_CHAR ], AFFECT },
  {  &oflag_name[0],          &oflag_name[1],          &max_oflag     , OFLAG },
  {  &sex_name[0],            &sex_name[1],            &max_sex,        SEX   },
  {  &color_fields[0],        &color_fields[1],        &max_color,      COLOR },
  {  &wear_part_name[0],      &wear_part_name[1],      &max_wear,       WEAR_PART },
  {  &group_table[0].name,    &group_table[1].name,    &table_max[ TABLE_GROUP ], GROUP },
  {  &dflag_name[0],          &dflag_name[1],          &max_dflag     , DFLAG },
  {  0,                       0,                       0              , NONE  },
  {  &layer_name[0],          &layer_name[1],          &max_layer,      WEAR_LAYER },
  {  &item_type_name[0],      &item_type_name[1],      &max_item_type,  OBJ_TYPE },
  {  0,                       0,                       0              , NONE  },
  {  &act_name[0],            &act_name[1],            &max_act,        ACT_FLAG },
  {  &oprog_trigger[0],       &oprog_trigger[1],       &max_oprog_trig, OPROG_TRIG },
  {  &mprog_trigger[0],       &mprog_trigger[1],       &max_mprog_trig, MPROG_TRIG },
  {  &action_trigger[0],      &action_trigger[1],      &max_acode_trig, ACODE_TRIG }
};


static bool const_valid( const char_data *ch, int i )
{
  return const_list[i].entry1;
}


void do_constants( char_data* ch, const char *argument )
{
  if( is_mob( ch ) ) 
    return;

  int max = MAX_ARG;
  int sorted[ max ];
  max = sort_names( &arg_type_name[0], &arg_type_name[1], sorted, max );

  if( !*argument ) {
    display_array( ch, "+Constant types",
		   &arg_type_name[0], &arg_type_name[1],
		   MAX_ARG, true, const_valid );

    /*
    // Can't use display_array() here, or we'd get "integer", etc.
    page_title( ch, "Constant types" );

    const unsigned columns = ch->pcdata->columns / 19;
    int j = 0;

    for( int i = 0; i < max; ++i ) {
     int k = sorted[i];
     char tmp [ TWO_LINES ];
      if( const_list[k].entry1 ) {
	snprintf( tmp, TWO_LINES, "%19s%s",
		  arg_type_name[k],
		  j%columns == columns-1 ? "\n\r" : "" );
	page( ch, tmp );
	++j;
      }
    }
    
    if( j%columns != 0 )
      page( ch, "\n\r" );
    */

    return;
  }
  
  for( int i = 0; i < max; ++i ) {
    int k = sorted[i];
    if( const_list[k].entry1 ) {
      if( matches( argument, arg_type_name[k] ) ) {
	if( const_list[k].type == SKILL ) {
	  // Multiple skill lists.
	  display_skills( ch );
	  /*
	  for( int j = 0; j < MAX_SKILL_CAT; ++j ) {
	    display_array( ch, skill_cat_name[j],
			   &skill_entry( j, 0 )->name, &skill_entry( j, 1 )->name,
			   table_max[ skill_table_number[ j ] ] );
	    if( j != MAX_SKILL_CAT-1 ) {
	      page( ch, "\n\r" );
	    }
	  }
	  */
	} else {
	  display_array( ch, arg_type_name[k],
			 const_list[k].entry1, const_list[k].entry2,
			 *const_list[k].size );
	}
	return;
      }
    }
  }

  fsend( ch, "Constant type \"%s\" not found.", argument );
}


static arg_type *read_const( const char*& code, const arg_enum type )
{
  // "null" is a special case.
  if( ( type == ANY
	|| type == NULL_POINTER
	|| type == CHARACTER
	|| type == OBJECT
	|| type == STRING
	|| type == ROOM
	|| type == THING )
      && word_match( code, "null" ) ) {
    arg_type *arg = new arg_type( constant );
    arg->type = NULL_POINTER;
    arg->value = 0;
    return arg;
  }

  if( type == ANY ) {
    // It's now an error if we match something here...
    // All enum constants must have context.
    return 0;
    /*
    for( int i = 0; i < MAX_ARG; i++ ) {
      if( !const_list[i].entry1 )
	continue;
      if( const_list[i].type == SKILL ) {
	for( int k = 0; k < MAX_SKILL_CAT; ++k ) {
	  int m = table_max[ skill_table_number[ k ] ];
	  for( int j = 0; j < m; ++j ) {
	    const char *name = skill_entry( k, j )->name;
	    if( word_match( code, name ) ) {
	      arg_type *arg = new arg_type(constant);
	      arg->type = type;
	      arg->value = (void*) skill_ident( k, j );
	      return arg;
	    }
	  }
	}
      } else {
	for( int j = 0; j < *const_list[i].size; ++j ) {
	  const char *name = const_list[i].entry( j );
	  if( word_match( code, name ) ) {
	    arg_type *arg = new arg_type( constant );
	    arg->type = const_list[i].type;
	    arg->value = (void*) j;
	    return arg;
	  }
	}
      }
    }
    */
  }

  if( !const_list[type].entry1 )
    return 0;

  // Multiple skill tables.
  if( type == SKILL ) {
    for( int k = 0; k < MAX_SKILL_CAT; ++k ) {
      int m = table_max[ skill_table_number[ k ] ];
      for( int j = 0; j < m; ++j ) {
	const char *name = skill_entry( k, j )->name;
	if( word_match( code, name ) ) {
	  arg_type *arg = new arg_type(constant);
	  arg->type = type;
	  arg->value = (void*) skill_ident( k, j );
	  return arg;
	}
      }
    }
    return 0;
  }

  for( int j = 0; j < *const_list[type].size; ++j ) {
    const char *name = const_list[type].entry( j );
    if( word_match( code, name ) ) {
      arg_type *arg = new arg_type(constant);
      arg->type = type;
      arg->value = (void*) j;
      return arg;
    }
  }
  
  return 0;
}


/*
 *   VARIABLES
 */


static thing_array *var_list_p = &var_list;


const var_data variable_list [] =
{
  { "mob",           (thing_data*)&var_mob,            CHARACTER  }, 
  { "rch",           (thing_data*)&var_rch,            CHARACTER  }, 
  { "victim",        (thing_data*)&var_victim,         CHARACTER  }, 
  { "cmd",           &var_cmd,            STRING     },
  { "arg",           &var_arg,            STRING     },
  { "room",          (thing_data*)&var_room,           ROOM       },
  { "obj",           (thing_data*)&var_obj,            OBJECT     },
  { "act_obj",       (thing_data*)&var_act_obj,        OBJECT     },
  { "container",     (thing_data*)&var_container,      OBJECT     },
  { "ch",            (thing_data*)&var_ch,             CHARACTER  },
  { "i",             &var_i,              INTEGER    },
  { "j",             &var_j,              INTEGER    },
  { "k",             &var_k,              INTEGER    },
  { "l",             &var_l,              INTEGER    },
  { "m",             &var_m,              INTEGER    },
  { "n",             &var_n,              INTEGER    },
  { "exit",          &var_exit,           EXIT       },
  { "thing",         &var_thing,          THING      },
  { "list",          &var_list_p,           THING_LIST },
  { "",              0,                   NONE       }
};


const var_data *find_variable( const char *name )
{
  for( int i = 0; ; ++i ) {
    if( !*variable_list[i].name )
      return 0;
    if( !strcmp( name, variable_list[i].name ) ) {
      return &variable_list[i];
    }
  }
}


static arg_type *read_variable( const char*& code )
{
  int i;

  for( i = 0; ; i++ ) {
    if( !*variable_list[i].name ) {
      return 0;
    }
    if( word_match( code, variable_list[i].name ) ) {
      break;
    }
  }

  arg_type *arg = new arg_type(variable);
  arg->type = variable_list[i].type;
  arg->value = variable_list[i].pointer;

  return arg;
}


static const cfunc_type pre_assign_list [ ] =
{
  { "++",    &code_pre_incr,     ANY,        { INTEGER, NONE, NONE,  NONE, NONE, NONE }  },
  { "--",    &code_pre_decr,     ANY,        { INTEGER, NONE, NONE,  NONE, NONE, NONE }  },
  { "",      0,                  NONE,       { NONE,  NONE,  NONE,  NONE, NONE, NONE }  }
};


static const cfunc_type assign_list [ ] =
{
  { "==",    0,                   NONE,      { ANY, ANY, NONE,  NONE, NONE, NONE }  },
  { "=",     &code_list_assign,   INTEGER,   { THING_LIST, THING_LIST, NONE, NONE, NONE, NONE } },
  { "=",     &code_set_equal,     ANY,       { ANY, ANY, NONE,  NONE, NONE, NONE }  },
  { "+=",    &code_plus_equal,    ANY,       { INTEGER, INTEGER, NONE,  NONE, NONE, NONE }  },
  { "-=",    &code_minus_equal,   ANY,       { INTEGER, INTEGER, NONE,  NONE, NONE, NONE }  },
  { "*=",    &code_mult_equal,    ANY,       { INTEGER, INTEGER, NONE,  NONE, NONE, NONE }  },
  { "/=",    &code_div_equal,     ANY,       { INTEGER, INTEGER, NONE,  NONE, NONE, NONE }  },
  { "%=",    &code_rem_equal,     ANY,       { INTEGER, INTEGER, NONE,  NONE, NONE, NONE }  },
  { "<<=",   &code_lshift_equal,  ANY,       { INTEGER, INTEGER, NONE,  NONE, NONE, NONE }  },
  { ">>=",   &code_rshift_equal,  ANY,       { INTEGER, INTEGER, NONE,  NONE, NONE, NONE }  },
  { "\\&=",  &code_bit_and_equal, ANY,       { INTEGER, INTEGER, NONE,  NONE, NONE, NONE }  },
  { "\\|=",  &code_bit_or_equal,  ANY,       { INTEGER, INTEGER, NONE,  NONE, NONE, NONE }  },
  { "\\^=",  &code_bit_xor_equal, ANY,       { INTEGER, INTEGER, NONE,  NONE, NONE, NONE }  },
  { "+=",    0,                   ANY,       { ANY, ANY, NONE,  NONE, NONE, NONE }  },
  { "-=",    0,                   ANY,       { ANY, ANY, NONE,  NONE, NONE, NONE }  },
  { "*=",    0,                   ANY,       { ANY, ANY, NONE,  NONE, NONE, NONE }  },
  { "/=",    0,                   ANY,       { ANY, ANY, NONE,  NONE, NONE, NONE }  },
  { "%=",    0,                   ANY,       { ANY, ANY, NONE,  NONE, NONE, NONE }  },
  { "<<=",   0,                   ANY,       { ANY, ANY, NONE,  NONE, NONE, NONE }  },
  { ">>=",   0,                   ANY,       { ANY, ANY, NONE,  NONE, NONE, NONE }  },
  { "\\&=",  0,                   ANY,       { ANY, ANY, NONE,  NONE, NONE, NONE }  },
  { "\\|=",  0,                   ANY,       { ANY, ANY, NONE,  NONE, NONE, NONE }  },
  { "\\^=",  0,                   ANY,       { ANY, ANY, NONE,  NONE, NONE, NONE }  },
  { "++",    &code_post_incr,     ANY,       { INTEGER, NONE, NONE,  NONE, NONE, NONE }  },
  { "--",    &code_post_decr,     ANY,       { INTEGER, NONE, NONE,  NONE, NONE, NONE }  },
  { "++",    0,                   ANY,       { ANY, NONE, NONE,  NONE, NONE, NONE }  },
  { "--",    0,                   ANY,       { ANY, NONE, NONE,  NONE, NONE, NONE }  },
  { "",      0,                   NONE,      { NONE,  NONE,  NONE,  NONE, NONE, NONE }  }
};


static arg_type *read_assign( const char*& code )
{
  const char *save = code;
  const cfunc_type *cf = 0;

  for( int i = 0; ; ++i ) {
    if( !*pre_assign_list[i].name ) {
      break;
    }
    int l = strlen( pre_assign_list[i].name );
    if( !strncasecmp( code, pre_assign_list[i].name, l ) ) {
      code += l;
      cf = &pre_assign_list[i];
      break;
    }
  }

  arg_type *arg0 = read_variable( code );

  if( !arg0 ) {
    if( cf ) {
      snprintf( error_buf, MAX_INPUT_LENGTH,
		"Unknown variable for assignment operator '%s'.", cf->name );
    }
    return 0;
  }

  if( skip_comments( code ) ) {
    delete arg0;
    return 0;
  }

  if( cf ) {
    if( cf->arg[0] != ANY
	&& cf->arg[0] != arg0->type ) {
      snprintf( error_buf, MAX_INPUT_LENGTH,
		"Bad variable type for assignment operator '%s'.", cf->name );
      delete arg0;
      return 0;
    }

  } else {
    for( int i = 0; ; ++i ) {
      if( !*assign_list[i].name ) {
	code = save;
	//      strcpy( error_buf, "Syntax error after variable name." );
	delete arg0;
	return 0;
      }
      int l = strlen( assign_list[i].name );
      if( !strncasecmp( code, assign_list[i].name, l )
	  && ( assign_list[i].arg[0] == ANY
	       || assign_list[i].arg[0] == arg0->type ) ) {
	code += l;
	cf = &assign_list[i];
	break;
      }
    }

    // Prevent "=" from matching "==".
    if( cf->type == NONE ) {
      code = save;
      delete arg0;
      return 0;
    }

    if( !cf->func_call ) {
      snprintf( error_buf, MAX_INPUT_LENGTH,
		"Bad variable type for assignment operator '%s'.", cf->name );
      delete arg0;
      return 0;
    }
  }

  // Single-argument assignment.
  if( cf->arg[1] == NONE ) {
    assign_type *ass = new assign_type;
    if( cf->type == ANY ) {
      ass->type = arg0->type;
    } else {
      ass->type = cf->type;
    }
    ass->func = cf;
    ass->arg[0] = arg0;
    return ass;
  }
  
  arg_type *arg1 = read_expression( code, 0 );

  if( !arg1 ) {
    delete arg0;
    return 0;
  }

  if( cf->arg[1] != ANY
      && cf->arg[1] != arg1->type ) {
    snprintf( error_buf, MAX_INPUT_LENGTH,
	      "%s required on right side of assignment operator '%s'.",
	      arg_type_name[ cf->arg[1] ],
	      cf->name );
    delete arg0;
    delete arg1;
    return 0;
  }

  const cfunc_type *conv;
  if( !can_assign( arg0->type, arg1->type, conv ) ) {
    snprintf( error_buf, MAX_INPUT_LENGTH, "Assignment to %s from %s.",
	      arg_type_name[ arg0->type ], arg_type_name[ arg1->type ] );
    delete arg0;
    delete arg1;
    return 0;
  }

  assign_type *ass = new assign_type;
  if( cf->type == ANY ) {
    ass->type = arg0->type;
  } else {
    ass->type = cf->type;
  }
  ass->func = cf;
  ass->arg[0] = arg0;
  ass->arg[1] = arg1;

  if( conv ) {
    // Assignment requires automatic conversion function.
    conv_type *conv_func = new conv_type;
    conv_func->type = conv->type;
    conv_func->func = conv;
    conv_func->arg = ass->arg[1];
    ass->arg[1] = conv_func;
  }

  return ass;
}


/*
 *   STRINGS
 */


static const char *get_string( const char* name, extra_array& list )
{
  for( int i = 0; i < list; ++i ) {
    if( !strcasecmp( name, list[i]->keyword ) )
      return list[i]->text;
  }
  
  return 0;
}


static const char *code_alloc( const char* argument )
{
  const int size = strlen( argument );

  if( size == 0 )
    return empty_string;

  mem_block *block = new mem_block( size+1 );
  block->next = block_list;
  block_list  = block;

  memcpy( block->pntr, argument, size+1 );

  return (char*) block->pntr;
}


static arg_type *read_string( const char*& code, extra_array& data )
{
  if( *code != '"' && *code != '#' )
    return 0;

  arg_type *arg = new arg_type( constant );
  arg->type = STRING;

  const char *string = code+1;

  if( *code == '"' ) {
    for( ++code; *code != '"'; ++code ) {
      if( !*code ) {
        strcpy( error_buf, "Unexpected end of string." );
        delete arg;
        return 0;
      }
    }
    char buf[ code-string+1 ];
    strncpy( buf, string, code-string );
    buf[code-string] = '\0';
    arg->value = code_alloc( buf );
    ++code;
    
    return arg;
  }
  
  for( ++code; *code != ',' && !isspace( *code ); ++code ) 
    if( !*code ) {
      strcpy( error_buf, "Unexpected end of string." );
      delete arg;
      return 0;
    }
  
  char buf[ code-string+1 ];
  strncpy( buf, string, code-string );
  buf[code-string] = '\0';

  arg->value = get_string( buf, data );

  if( !arg->value ) {
    snprintf( error_buf, MAX_INPUT_LENGTH, "String not found. (%s)", buf );
    delete arg;
    arg = 0;
  }

  return arg;
}


/*
 *   NUMBERS
 */


static arg_type* itoarg( int i )
{
  /*
  // Save a little memory.
  if( i == 0 )
    return 0;
  */

  arg_type *arg = new arg_type(constant);

  arg->type   = INTEGER;
  arg->value  = (void*) i;

  return arg;
}


static arg_type *read_digit( const char*& code )
{
  if( !isdigit( *code ) )
    return 0;

  if( *code == '0'
      && *(code+1)
      && tolower( *(code+1) ) == 'x'
      && *(code+2)
      && isxdigit( *(code+2) ) ) {
    code += 2;
    int val = 0;
    for( ; isxdigit( *code ); ++code ) {
      if( val & 0xf0000000 ) {
	strcpy( error_buf, "Numeric overflow in hex literal." );
	return 0;
      }
      val *= 16;
      if( isdigit( *code ) ) {
	val += *code - '0';
      } else {
	val += tolower( *code ) - 'a' + 10;
      }
    }
    return itoarg( val );
  }

  int dice = 0;

  for( ; isdigit( *code ); ++code ) {
    dice = *code+10*dice-'0';
    if( dice < 0 ) {
      strcpy( error_buf, "Numeric overflow in decimal literal." );
      return 0;
    }
  }

  if( *code != 'd' )
    return itoarg( dice );

  //int plus = 0;
  int side = 0;

  for( ++code; isdigit( *code ); ++code ) {
     side = *code+10*side-'0';
     if( side < 0 ) {
       strcpy( error_buf, "Numeric overflow in decimal literal." );
       return 0;
     }
  }

  /*
  if( *code == '+' ) {
    for( ++code; isdigit( *code ); ++code ) {
      plus = *code+10*plus-'0';
      if( plus < 0 ) {
	strcpy( error_buf, "Numeric overflow in decimal literal." );
	return 0;
      }
    }
  }
  */

  /*
  if( dice == 0 || side == 0 )
    return 0;
  */

  afunc_type *afunc = new afunc_type;
  afunc->type = INTEGER;
  afunc->family = function;

  afunc->arg[0] = itoarg( dice );
  afunc->arg[1] = itoarg( side );
  afunc->arg[2] = 0;

  for( int i = 0; ; i++ ) {
    if( !strcasecmp( cfunc_list[i].name, "dice" ) ) {
      afunc->func = &cfunc_list[i];
      break;
    }
  }

  return afunc;
}


/*
 *   COMPILER ROUTINES
 */

void program_data :: compile( )
{
  arg_type*         arg;
  arg_type*    arg_list  = 0;
  arg_type*    arg_last  = 0;
  const char    *letter;

  if( active != 0 ) {
    bug( "Compile: compiling active program." );
    bug( "-- Active count = %d", active );
  }

  clear_queue( this );
  
  if( memory ) {
    delete_list( memory );
    memory = 0;
  }

  if( binary ) {
    delete binary;
    binary = 0;
  }

  mem_block *tmp_list = block_list;
  block_list = 0;

  this_code = Code( );
  this_data = &Extra_Descr( );

  *error_buf = '\0';

  while( !*error_buf ) {
    if( !( arg = read_statement( this_code ) ) )
      break;

    if( !arg_list )
      arg_list = arg;
    else
      arg_last->next = arg;
    
    arg_last = arg;
  }

  mem_block *list = block_list;
  block_list = tmp_list;

  if( *error_buf ) { 
    int line = 1;
    for( letter = Code( ); letter != this_code; ++letter )
      if( *letter == '\n' )
        line += 2;
    corrupt = true;
    if( var_ch ) {
      page_header( var_ch, "*** %s\n\r\n\r", error_buf );
      page_header( var_ch, "*** Error on line %d\n\r", line );
      page_header( var_ch, "*** FAILS TO COMPILE\n\r" );
      next_page( var_ch->link );
    }
    delete_list( list );
    if( arg_list )
      delete arg_list;
    return;
  }

  binary = arg_list;
  memory = list;
  active = 0;
  corrupt = false;
}


void program_data :: decompile( )
{
  if( !binary )
    return;

  if( active != 0 ) {
    bug( "Decompile: decompiling active program." );
    bug( "-- Active count = %d", active );
  }

  clear_queue( this );
  
  delete binary;
  binary = 0;
  delete_list( memory );
  memory = 0;
}


void do_compile( char_data* ch, const char *argument )
{
  wizard_data *imm = wizard( ch );

  if( !imm )
    return;

  int flags;
  if( !get_flags( ch, argument, &flags, "aAMO", "testing" ) )
    return;

  if( is_set( flags, 0 ) ) {
    if( !ch->can_edit( ch->in_room, false ) ) {
      send( ch, "You don't have permission to use the -a option here.\n\r" );
      return;
    }
    bool tmp_load = false;
    unsigned count = 0;
    for( room_data *room = ch->in_room->area->room_first; room; room = room->next ) {
      unsigned num = 1;
      for( action_data *action = room->action; action; action = action->next ) {
	if( !action->binary ) {
	  if( !ch->in_room->area->act_loaded ) {
	    tmp_load = true;
	  }
	  action->compile( );
	  if( action->corrupt ) {
	    send( ch, "Error in room #%d action %u.\n\r", room->vnum, num );
	  }
	  ++count;
	  action->decompile( );
	}
      }
      ++num;
    }
    if( tmp_load )
      ch->in_room->area->clear_actions( );
    send( ch, "*** Compiled %u acodes.\n\r", count );

  } else if( is_set( flags, 1 ) ) {
    if( !has_permission( ch, PERM_ALL_ROOMS ) ) {
      send( ch, "You don't have permission to use the -A option.\n\r" );
      return;
    }
    unsigned count = 0;
    for( area_data *area = area_list; area; area = area->next ) {
      bool tmp_load = false;
      for( room_data *room = area->room_first; room; room = room->next ) {
	unsigned num = 1;
	for( action_data *action = room->action; action; action = action->next ) {
	  if( !action->binary ) {
	    if( !area->act_loaded ) {
	      tmp_load = true;
	    }
	    action->compile( );
	    if( action->corrupt ) {
	      send( ch, "Error in room #%d action %u.\n\r", room->vnum, num );
	    }
	    ++count;
	    action->decompile( );
	  }
	  ++num;
	}
      }
      if( tmp_load )
	area->clear_actions( );
    }
    send( ch, "*** Compiled %u acodes.\n\r", count );
  }
  
  if( is_set( flags, 2 ) ) {
    if( !has_permission( ch, PERM_ALL_MOBS ) ) {
      send( ch, "You don't have permission to use the -M option.\n\r" );
      return;
    }
    unsigned attack = 0;
    unsigned other = 0;
    for( int i = 1; i <= species_max; ++i ) {
      if( species_data *species = species_list[i] ) {
	if( !species->attack->binary ) {
	  species->attack->compile( );
	  if( species->attack->corrupt ) {
	    send( ch, "Error in mob #%d attack mprog.\n\r", species->vnum );
	  }
	  ++attack;
	  species->attack->decompile( );
	}
	unsigned num = 1;
	for( mprog_data *mprog = species->mprog; mprog; mprog = mprog->next ) {
	  if( !mprog->binary ) {
	    mprog->compile( );
	    if( mprog->corrupt ) {
	      send( ch, "Error in mob #%d mprog %u.\n\r", species->vnum, num );
	    }
	    ++other;
	    mprog->decompile( );
	  }
	  ++num;
	}
      }
    }
    send( ch, "*** Compiled %u attack + %u other mprogs.\n\r", attack, other );
  }

  if( is_set( flags, 3 ) ) {
    if( !has_permission( ch, PERM_ALL_OBJECTS ) ) {
      send( ch, "You don't have permission to use the -O option.\n\r" );
      return;
    }
    unsigned count = 0;
    for( int i = 1; i <= obj_clss_max; ++i ) {
      if( obj_clss_data *obj_clss = obj_index_list[i] ) {
	unsigned num = 1;
	for( oprog_data *oprog = obj_clss->oprog; oprog; oprog = oprog->next ) {
	  if( !oprog->binary ) {
	    oprog->compile( );
	    if( oprog->corrupt ) {
	      send( ch, "Error in object #%d oprog %u.\n\r", obj_clss->vnum, num );
	    }
	    ++count;
	    oprog->decompile( );
	  }
	  ++num;
	}
      }
    }
    send( ch, "*** Compiled %u oprogs.\n\r", count );
  }
}
