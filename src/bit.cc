#include "bit.h"


const char *true_false( const int *array, int bit )
{
  return is_set( array, bit ) ? "true" : "false";
}


const char *yes_no( const int *array, int bit )
{
  return is_set( array, bit ) ? "yes" : "no";
}


const char *on_off( const int *array, int bit )
{
  return is_set( array, bit ) ? "on" : "off";
}


/*
 *   BIT VECTOR FUNCTIONS
 */


bool is_set( int vector, int bit )
{
  return( ( vector & ( 1 << bit ) ) != 0 );
}


void switch_bit( int& vector, int bit )
{
  vector ^= ( 1 << bit );
}


void set_bit( int& vector, int bit )
{
  vector |= ( 1 << bit );
}


void remove_bit( int& vector, int bit )
{
  vector &= ~( 1 << bit ); 
}


void assign_bit( int& vector, int bit, bool value )
{
  if( value )
    set_bit( vector, bit );
  else
    remove_bit( vector, bit );
}
  

/*
 *   BIT ARRAY FUNCTIONS
 */


bool is_set( const int *array, int bit )
{
  int i = bit/32;
  int j = bit-32*i;

  return( ( array[i] & ( 1 << j ) ) != 0 );
}


void switch_bit( int *array, int bit )
{
  int i = bit/32;
  int j = bit-32*i;

  array[i] ^= ( 1 << j );
}


void set_bit( int *array, int bit )
{
  int i = bit/32;
  int j = bit-32*i;

  array[i] |= ( 1 << j );
}


void remove_bit( int *array, int bit )
{
  int i = bit/32;
  int j = bit-32*i;

  array[i] &= ~( 1 << j ); 
}


void assign_bit( int *array, int bit, bool value )
{
  if( value )
    set_bit( array, bit );
  else
    remove_bit( array, bit );
}
  

/*
 *   LEVEL FUNCTIONS
 */


void set_level( int* array, int bit, int level )
{
  assign_bit( array, 2*bit, is_set( level, 0 ) );
  assign_bit( array, 2*bit+1, is_set( level, 1 ) );
}


int level_setting( const int *array, int bit )
{
  int      i  = bit/16;
  int      j  = 2*(bit-16*i);

  return ( array[i] >> j ) & 3;
}


void set_level( int& vector, int bit, int level )
{
  assign_bit( vector, 2*bit, is_set( level, 0 ) );
  assign_bit( vector, 2*bit+1, is_set( level, 1 ) );
}


int level_setting( int vector, int bit )
{
  int j = 2*bit;

  return ( vector >> j ) & 3;
}
