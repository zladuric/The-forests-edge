#ifndef tfe_macros_h
#define tfe_macros_h


extern void aphid  ( const char* text );
extern void beetle ( const char* text );
extern void roach  ( const char* text );
extern void bug    ( const char* text );


/*
 *   GENERAL
 */


template < class T >
void swap( T& a, T& b )
{
  T temp;

  memcpy( &temp, &a,    sizeof( T ) );
  memcpy( &a,    &b,    sizeof( T ) ); 
  memcpy( &b,    &temp, sizeof( T ) );
}


template < class T >
bool exchange( T& value, T a, T b )
{ 
  if( value == a ) 
    value = b;
  else if( value == b )
    value = a;
  else
    return false;

  return true;
}


template < class T >
T element( int i, T* first, T* second )
{
  return *(first+i*(second-first));
}


/*
 *   LINKED LIST
 */


template < class T >
void add( T*& list, T* item )
{
  item->next = list;
  list       = item;
}


template < class T >
void append( T*& list, T* item )
{
  item->next = 0;
  cat( list, item );
}


template < class T >
void cat( T*& list, T* item )
{
  T* prev;

  if( !list ) {
    list = item;
  } else {
    for( prev = list; prev->next; prev = prev->next );
    prev->next = item;
  }
}


/*
template < class T >
void remove( T*& list, T* item )
{
  if( list == item ) {
    list = item->next;
  } else { 
    T *prev;
    for( prev = list; prev && prev->next != item; prev = prev->next );
    if( prev ) {
      prev->next = item->next;
    } else {
      bug( "Remove: Not in list." );
    }
  }
}  
*/


template < class T >
void remove( T*& list, T* item, const char* msg = 0 )
{
  if( list == item ) {
    list = item->next;
  } else {
    T *prev;
    for( prev = list; prev && prev->next != item; prev = prev->next );
    if( prev ) {
      prev->next = item->next;
    } else {
      if( msg ) {
	bug( msg );
      } else {
	bug( "Remove: Not in list." );
      }
    }
  }
}  


template < class T >
int count( T* list )
{
  int i;

  for( i = 0; list; i++ )
    list = list->next;

  return i;
}


template < class T >
T* locate( T* list, int i )
{
  for( ; list; list = list->next )
    if( --i == 0 )
      return list;

  return 0;
}


template < class T >
void delete_list( T*& list )
{
  while( T *item = list ) {
    list = list->next;
    delete item;
  }
}


template < class T >
void extract_list( T*& list )
{
  T *item_next;

  for( T *item = list; item; item = item_next ) {
    item_next = item->next;
    extract( item );
  }

  list = 0;
}


template < class T >
bool is_listed( T* list, T* element )
{
  for( ; list; list = list->next )
    if( list == element )
      return true;

  return false;
} 


/*
 *    ARRAY
 */


template < class T >
void vzero( T *item, int max )
{
  for( int i = 0; i < max; ++i )
    item[i] = (T)0;
}


template < class T >
void vfill( T *item, int max, T val )
{
  for( int i = 0; i < max; ++i )
    item[i] = val;
}


template < class T >
void vcopy( T *item, const T *src, int max )
{
  for( int i = 0; i < max; ++i )
    item[i] = src[i];
}


/*
 *   SEARCH/SORT TEMPLATES
 */


template < class T >
void sort( T *list, int size )
{
  for( int i = 0; i < size-1; ++i ) {
    bool done = true;

    for( int j = 0; j < size-1-i; ++j ) {
      if( strcasecmp( list[j].name, list[j+1].name ) > 0 ) {
        swap( list[j], list[j+1] );
        done = false;
      }
    }

    if( done )
      break;
  }
}


template < class T >
int pntr_search( T** list, int max, const char* word )
{
  int      min  = 0;
  int    value;
  int      mid;

  if( !list )
    return -1;

  --max;

  while( true ) {
    if( max < min )
      return -min-1;

    mid = (max+min)/2;
    value = strcasecmp( name( list[mid] ), word );

    if( value == 0 )
      break;
    if( value < 0 )
      min = mid+1;
    else 
      max = mid-1;
  }

  return mid;
}


template < class T >
int pntr_search( T** list, int max, const char* word, int n )
{
  int      min  = 0;
  int    value;
  int      mid;

  if( !list )
    return -1;

  --max;

  while( true ) {
    if( max < min )
      return -min-1;

    mid = (max+min)/2;
    value = strncasecmp( name( list[mid] ), word, n );

    if( value >= 0 )
      max = mid-1;
    else
      min = mid+1;
  }

  //  return mid;
}


template < class T >
int search( const T* list, int max, const char* word )
{
  int      min  = 0;
  int    value;
  int      mid;

  if( !list )
    return -1;

  --max;

  while( true ) {
    if( max < min )
      return -min-1;
    
    mid    = (max+min)/2;
    value  = strcasecmp( list[mid].name, word );

    if( value == 0 )
      break;

    if( value < 0 )
      min = mid+1;
    else 
      max = mid-1;
  }

  return mid;
}


template < class T >
int search( const T* list, int max, const char* word, size_t n )
{
  int      min  = 0;
  int    value;
  int      mid;

  if( !list )
    return -1;

  --max;

  while( true ) {
    if( max < min )
      return -min-1;
    
    mid    = (max+min)/2;
    value  = strncasecmp( list[mid].name, word, n );

    if( value == 0 ) {
      if( list[mid].name[n] ) {
	max = mid-1;
      } else {
	break;
      }
    } else if( value < 0 ) {
      min = mid+1;
    } else {
      max = mid-1;
    }
  }

  return mid;
}


/*
 *   STATIC LIST ROUTINES
 */


template < class T >
void insert( T*& prev, int& size, T element, int pos )
{
  if( !prev ) {
    prev     = new T[1];
    prev[0]  = element;
    size     = 1;
    return;
  }

  T *next = new T[size+1];

  memcpy( next, prev, pos*sizeof( T ) );
  memcpy( &next[pos+1], &prev[pos], (size-pos)*sizeof( T ) );

  delete [] prev;

  next[pos] = element;
  prev = next;
  ++size;
}  


template < class T >
void remove( T*& prev, int& size, int pos )
{
  T *next;

  if( pos < 0 || pos >= size ) 
    return;

  if( size == 1 ) {
    delete [] prev;
    prev  = 0;
    size  = 0;
    return;
  }

  next = new T[size-1];

  memcpy( next, prev, pos*sizeof( T ) );
  memcpy( &next[pos], &prev[pos+1], (size-pos-1)*sizeof( T ) );

  delete [] prev;

  prev = next;
  size--;
}  


#endif // tfe_macros_h
