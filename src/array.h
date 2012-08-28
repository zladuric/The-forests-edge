#ifndef __ARRAY_H__
#define __ARRAY_H__


/*
 *   BASE ARRAY CLASS
 */


#include "memory.h"


template <class something>
class Array
{
public:
  int           size;
  something    *list;
  int         memory;
  
  Array( )
    : size(0), list(0), memory(0)
  {
    record_new( sizeof( Array ), MEM_ARRAY );
  } 

  Array( const Array& from )
    : size(from.size), memory(from.size)
  {
    record_new( sizeof( Array ), MEM_ARRAY );
    if( size != 0 ) {
      record_new( memory*sizeof( something ), -MEM_ARRAY );
      list = new something [ memory ];
      memcpy( list, from.list, size*sizeof( something ) );
    } else {
      list = 0;
    }
  }

  virtual ~Array( )
  {
    record_delete( sizeof( Array ), MEM_ARRAY );
    clear();
  }

  void clear( int x = 0 )
  {
    if( memory != x ) {
      if( memory != 0 ) {
	record_delete( memory*sizeof( something ), -MEM_ARRAY );
	delete [] list;
      }
      if( x != 0 ) {
	record_new( x*sizeof( something ), -MEM_ARRAY );
	list = new something [ x ];
      } else {
	list = 0;
      }
      memory = x;
    }
    size = 0;
  }

  void collect()
  {
    if( memory > size ) {
      record_delete( memory*sizeof( something ), -MEM_ARRAY );
      memory = size;
      record_new( memory*sizeof( something ), -MEM_ARRAY );
      something *next = new something [ memory ];
      memcpy( next, list, size*sizeof( something ) );
      delete [] list;
      list = next;
    }
  }


  // x can be const_cast.
  bool includes( const something& x ) const
  {
    for( int i = 0; i < size; ++i )
      if( list[i] == x )
        return true;
    return false;
  }

  // x can be const_cast.
  int find( const something& x )
  {
    for( int i = 0; i < size; ++i )
      if( list[i] == x )
        return i;
    return -1;
  }
  
  void remove( int i )
  {
    if( --size == 0 ) {
      clear();
    } else if( size < ( memory / 2 ) ) {
      // Decrease memory allocation if 2*new_mem+1 < old_mem.
      record_delete( memory*sizeof( something ), -MEM_ARRAY );
      memory = size;
      record_new( memory*sizeof( something ), -MEM_ARRAY );
      something *next = new something [ memory ];
      memcpy( next, list, i*sizeof( something ) );
      memcpy( &next[i], &list[i+1], (size-i)*sizeof( something ) );
      delete [] list;
      list = next;
    } else {
      memcpy( &list[i], &list[i+1],
	      (size-i)*sizeof( something ) );
    }
  }

  void remove_all( const something& x ) {
    for( int i = 0; i < size; ++i ) {
      if( list[i] == x ) {
	remove( i-- );
      }
    }
  }
  
  void replace_all( const something& x, const something& y ) {
    for( int i = 0; i < size; ++i ) {
      if( list[i] == x ) {
	list[i] = y;
      }
    }
  }

  void swap( Array& other )
  {
    ::swap( size, other.size);
    ::swap( list, other.list );
    ::swap( memory, other.memory );
  }

  void insert( const something& x, int i )
  {
    if( size == memory ) {
      if( memory != 0 ) {
	record_delete( memory*sizeof( something ), -MEM_ARRAY );
      }
      memory = 2*memory+1;
      record_new( memory*sizeof( something ), -MEM_ARRAY );
      something *next = new something [ memory ];
      memcpy( next, list, i*sizeof( something ) );
      memcpy( &next[i+1], &list[i], (size-i)*sizeof( something ) );
      delete [] list;
      list = next;
    } else {
      memmove( &list[i+1], &list[i],
	       (size-i)*sizeof( something ) );
    }
    list[i] = x;
    ++size;
  }

  Array& operator = ( const Array& from )
  {
    if( memory != 0 ) {
      record_delete( memory*sizeof( something ), -MEM_ARRAY);
      delete [] list;
    }
    size = from.size;
    memory = from.size;
    if( memory != 0 ) {
      record_new( memory*sizeof( something ), -MEM_ARRAY );
      list = new something [ memory ];
      memcpy( list, from.list, memory*sizeof( something ) ); 
    } else {
      list = 0;
    }
    return *this;
  }
 
  bool is_empty( ) const
  { return size == 0; }

  /* OPERATORS */

  something& operator [] ( int i )
  { return list[i]; }

  const something& operator [] ( int i ) const
  { return list[i]; }

  const bool operator == ( const Array& a ) const
  { return this == &a; };

  friend bool operator <  ( int i, const Array& a ) { return( i < a.size ); }
  friend bool operator >  ( int i, const Array& a ) { return( i > a.size ); }
  friend bool operator >  ( const Array& a, int i ) { return( a.size > i ); }
  friend bool operator >= ( const Array& a, int i ) { return( a.size >= i ); }
  friend bool operator >= ( int i, const Array& a ) { return( i >= a.size ); }
  friend bool operator <= ( const Array& a, int i ) { return( a.size <= i ); }
  friend bool operator <= ( int i, const Array& a ) { return( i <= a.size ); }
  friend bool operator == ( const Array& a, int i ) { return( a.size == i ); }
  friend bool operator == ( int i, const Array& a ) { return( i == a.size ); }
  friend bool operator != ( const Array& a, int i ) { return( a.size != i ); }
  friend bool operator != ( int i, const Array& a ) { return( i != a.size ); }
  friend int  operator -  ( const Array& a, int i ) { return a.size-i; }
  friend int  operator -  ( int i, const Array& a ) { return i-a.size; }

  friend int  operator+  ( const Array& a, const Array& b ) { return a.size+b.size; }

  void operator -= ( const something& x )
  {
    for( int i = 0; i < size; i++ ) {
      if( list[i] == x ) {
        remove( i );
        break;
      }
    }
  }

  something& append ( const something& x )
  {
    if( size == memory ) {
      if( memory != 0 ) {
	record_delete( memory*sizeof( something ), -MEM_ARRAY );
      }
      memory = 2*memory+1;
      record_new( memory*sizeof( something ), -MEM_ARRAY );
      something *next = new something [ memory ];
      memcpy( next, list, size*sizeof( something ) );
      delete [] list;
      list = next;
    }
    return list[size++] = x;
  }
  
  void operator += ( const something& x )
  {
    if( !includes( x ) ) {
      append( x );
      /*
      if( size == memory ) {
	if( memory != 0 ) {
	  record_delete( memory*sizeof( something ), -MEM_ARRAY );
	}
        memory = 2*memory+1;
        record_new( memory*sizeof( something ), -MEM_ARRAY );
        something *next = new something [ memory ];
        memcpy( next, list, size*sizeof( something ) );
        delete [] list;
        list = next;
      }
      list[size++] = x;
      */
    }
  }
  
  void delete_list ( )
  {
    for( int i = 0; i < size; ++i ) 
      delete list[i]; 
    clear();
  }
  
  
  /*
   *   BINARY SEARCH
   */
  
  template < class T >
  int binary_search( T item, int func( something, T ) )
  {
    int      min  = 0;
    int      max  = size-1;
    int      mid;
    int    value;

    while( true ) {
      if( max < min )
	return min;
      
      mid    = (max+min)/2;
      value  = ( *func )( list[mid], item );
      
      if( value == 0 )
	break;
      if( value < 0 )
	min = mid+1;
      else 
	max = mid-1;
    }
    
    return mid;
  }
  
};


#endif
