#include <limits.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include "define.h"
#include "struct.h"


/*
 *   TYPE CONVERSIONS
 */


const void *code_conv_int_dir( const void **argument )
{
  const int x = (int) argument[0];

  if( x < 0 || x >= MAX_DIR_COMPASS ) {
    code_bug( "Convert int->dir: bad direction %d.", x );
    return 0;
  }

  return (void*)x;
}


const void *code_conv_dir_int( const void **argument )
{
  const int x = (int) argument[0];

  return (void*)x;
}


const void *code_conv_int_str( const void **argument )
{
  const int x = (int) argument[0];

  snprintf( lib_buf, LIB_BUF_LEN, "%d", x );

  const char *tmp = alloc_string( lib_buf, MEM_QUEUE );
  extract_strings += tmp;
  return tmp;
}


const void *code_conv_exit_str( const void **argument )
{
  const exit_data *exit = (exit_data*) argument[0];

  if( !exit ) {
    return "[BUG]";
  }

  return exit->Name( );
}


/*
 *   SET EQUAL
 */


const void *code_set_equal( const void **argument )
{
  arg_type *const arg  = (arg_type*) argument[0];
  void*    value  = (void*)     argument[1];

  void **pntr  = (void**) arg->value;
  *pntr = value;

  return *pntr;
}


/*
 *   BITWISE OPERATORS
 */


const void *code_bit_and( const void **argument )
{
  const int i = (int) argument[0];
  const int j = (int) argument[1];

  return (void*) ( i & j );
}


const void *code_bit_or( const void **argument )
{
  const int i = (int) argument[0];
  const int j = (int) argument[1];

  return (void*) ( i | j );
}


const void *code_bit_xor( const void **argument )
{
  const int i = (int) argument[0];
  const int j = (int) argument[1];

  return (void*) ( i ^ j );
}


const void *code_lshift( const void **argument )
{
  const int i = (int) argument[0];
  const int j = (int) argument[1];

  return (void*) ( i << j );
}


const void *code_rshift( const void **argument )
{
  const int i = (int) argument[0];
  const int j = (int) argument[1];

  return (void*) ( i >> j );
}


const void *code_bit_and_equal( const void **argument )
{
  const arg_type *const arg = (arg_type*) argument[0];
  int i = (int) argument[1];

  void **pntr  = (void**) arg->value;
  *pntr = (void*)((int)*pntr & i);

  return *pntr;
}


const void *code_bit_or_equal( const void **argument )
{
  const arg_type *const arg = (arg_type*) argument[0];
  const int i = (int) argument[1];

  void **pntr  = (void**) arg->value;
  *pntr = (void*)((int)*pntr | i);

  return *pntr;
}


const void *code_bit_xor_equal( const void **argument )
{
  const arg_type *const arg = (arg_type*) argument[0];
  const int i = (int) argument[1];

  void **pntr  = (void**) arg->value;
  *pntr = (void*)((int)*pntr ^ i);

  return *pntr;
}


const void *code_lshift_equal( const void **argument )
{
  const arg_type *const arg = (arg_type*) argument[0];
  const int i = (int) argument[1];

  void **pntr  = (void**) arg->value;
  *pntr = (void*)((int)*pntr << i);

  return *pntr;
}


const void *code_rshift_equal( const void **argument )
{
  const arg_type *const arg = (arg_type*) argument[0];
  const int i = (int) argument[1];

  void **pntr  = (void**) arg->value;
  *pntr = (void*)((int)*pntr >> i);

  return *pntr;
}


/*
 *   MATH OPERATORS
 */


const void *code_plus_equal( const void **argument )
{
  const arg_type *const   arg  = (arg_type*) argument[0];
  int          i  = (int)       argument[1];

  void **pntr  = (void**) arg->value;
  *pntr = (void*)((int)*pntr + i);

  return *pntr;
}


const void *code_minus_equal( const void **argument )
{
  const arg_type *const   arg  = (arg_type*) argument[0];
  int          i  = (int)       argument[1];

  void **pntr  = (void**) arg->value;
  *pntr = (void*)((int)*pntr - i);

  return *pntr;
}


const void *code_mult_equal( const void **argument )
{
  const arg_type *const   arg  = (arg_type*) argument[0];
  int          i  = (int)       argument[1];

  void **pntr  = (void**) arg->value;
  *pntr = (void*)((int)*pntr * i);

  return *pntr;
}


const void *code_div_equal( const void **argument )
{
  const arg_type *const   arg  = (arg_type*) argument[0];
  int          i  = (int)       argument[1];

  void **pntr  = (void**) arg->value;

  if( i == 0 ) {
    code_bug( "Divide Assign: divide by zero." );
    return (void*) ( (int)*pntr < 0 ? INT_MIN : INT_MAX );
  }

  *pntr = (void*)((int)*pntr / i);

  return *pntr;
}


const void *code_rem_equal( const void **argument )
{
  const arg_type *const   arg  = (arg_type*) argument[0];
  int          i  = (int)       argument[1];

  if( i == 0 ) {
    code_bug( "Remainder Assign: divide by zero." );
    return 0;
  }

  void **pntr  = (void**) arg->value;
  *pntr = (void*)((int)*pntr % i);

  return *pntr;
}


const void *code_pre_incr( const void **argument )
{
  const arg_type *const arg = (arg_type*) argument[0];

  void **pntr  = (void**) arg->value;
  int old = (int)*pntr;
  *pntr = (void*)(old + 1);

  return *pntr;
}


const void *code_pre_decr( const void **argument )
{
  const arg_type *const arg = (arg_type*) argument[0];

  void **pntr  = (void**) arg->value;
  int old = (int)*pntr;
  *pntr = (void*)(old - 1);

  return *pntr;
}


const void *code_post_incr( const void **argument )
{
  const arg_type *const arg = (arg_type*) argument[0];

  void **pntr  = (void**) arg->value;
  int old = (int)*pntr;
  *pntr = (void*)(old + 1);

  return (void*)old;
}


const void *code_post_decr( const void **argument )
{
  const arg_type *const arg = (arg_type*) argument[0];

  void **pntr  = (void**) arg->value;
  int old = (int)*pntr;
  *pntr = (void*)(old - 1);

  return (void*)old;
}


const void *code_is_equal( const void **argument )
{
  const int i  = (int) argument[0];
  const int j  = (int) argument[1];

  return (void*) ( i == j );
}


const void *code_not_equal( const void **argument )
{
  const int i  = (int) argument[0];
  const int j  = (int) argument[1];

  return (void*) ( i != j );
}


const void *code_add( const void **argument )
{
  const int          i  = (int)       argument[0];
  const int          j  = (int)       argument[1];

  return (void*) ( i + j );
}


const void *code_subtract( const void **argument )
{
  const int          i  = (int)       argument[0];
  const int          j  = (int)       argument[1];

  return (void*) ( i - j );
}


const void *code_multiply( const void **argument )
{
  const int          i  = (int)       argument[0];
  const int          j  = (int)       argument[1];

  return (void*) ( i * j );
}


const void *code_divide( const void **argument )
{
  const int          i  = (int)       argument[0];
  const int          j  = (int)       argument[1];

  if( j == 0 ) {
    code_bug( "Divide: divide by zero." );
    return (void*) ( i < 0 ? INT_MIN : INT_MAX );
  }

  return (void*) ( i / j );
}


const void *code_remainder( const void **argument )
{
  const int          i  = (int)       argument[0];
  const int          j  = (int)       argument[1];

  if( j == 0 ) {
    code_bug( "Remainder: divide by zero." );
    return 0;
  }

  return (void*) ( i % j );
}


/*
 *   COMPARISIONS
 */


const void *code_gt( const void **argument )
{
  const int i  = (int) argument[0];
  const int j  = (int) argument[1];

  return (void*) ( i > j );
}


const void *code_lt( const void **argument )
{
  const int  i  = (int) argument[0];
  const int  j  = (int) argument[1];

  return (void*) ( i < j );
}


const void *code_ge( const void **argument )
{
  const int  i  = (int) argument[0];
  const int  j  = (int) argument[1];

  return (void*) ( i >= j );
}


const void *code_le( const void **argument )
{
  const int  i  = (int) argument[0];
  const int  j  = (int) argument[1];

  return (void*) ( i <= j );
}


/*
 *   LOGIC GATES
 */


const void *code_and( const void **argument )
{
  const int i  = (int) argument[0];
  const int j  = (int) argument[1];

  return (void*) ( i && j );
}


const void *code_or( const void **argument )
{
  const int i  = (int) argument[0];
  const int j  = (int) argument[1];

  return (void*) ( i || j );
}


const void *code_eor( const void **argument )
{
  const int i  = (int) argument[0];
  const int j  = (int) argument[1];

  return (void*) ( ( i && !j ) || ( j && !i ) );
}


/*
 *   STRING COMPARE
 */


const void *code_streq( const void **argument )
{
  const char *const s = (const char*) argument[0];
  const char *const t = (const char*) argument[1];

  return (void*) !strcasecmp( s, t );
}


const void *code_strneq( const void **argument )
{
  const char *const s = (const char*) argument[0];
  const char *const t = (const char*) argument[1];

  return (void*) strcasecmp( s, t );
}
