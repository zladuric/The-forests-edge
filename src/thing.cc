#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include "define.h"
#include "struct.h"


thing_array extracted;


int visible_data :: Number( ) const
{ return 1; }


int visible_data :: Selected( ) const
{ return 1; }


int visible_data :: Shown( ) const
{ return 1; }


void visible_data :: Select( int num )
{
  if( num != 1 ) {
    bug( "visible_data::Select( num ): num != 1" );
    bug( "-- Vis = %s", this );
    bug( "-- Num = %d", num );
  }
}


void visible_data :: Select_All( )
{
}


void visible_data :: Show( int num )
{
  if( num != 1 ) {
    bug( "visible_data::Show( num ): num != 1" );
    bug( "-- Vis = %s", this );
    bug( "-- Num = %d", num );
  }
  /*
  if( num < 0 ) {
    bug( "visible_data::Show( num ): num < 0" );
    bug( "-- Vis = %s", this );
    bug( "-- Num = %d", num );
  } else if( num > 1 ) {
    bug( "visible_data::Show( num ): num > 1" );
    bug( "-- Vis = %s", this );
    bug( "-- Num = %d", num );
  }
  */
}


bool standard_delay( char_data *ch, const thing_array& array, const obj_data *container )
{
  if( array.is_empty( )
      || container && ( !container->Is_Valid( ) 
			|| ch == container->array->where ) )
    return false;

  set_delay( ch, min( 15, 3 + 2*array.size ) );

  return true;
}


/*
 *   APPEARANCE
 */


bool look_same( char_data *ch, thing_data* t1, thing_data* t2, bool auction )
{
  obj_data*    obj1;
  obj_data*    obj2;

  if( ( obj1 = object( t1 ) ) && ( obj2 = object( t2 ) ) ) {
    return look_same( ch, obj1, obj2, auction );
  }

  char_data*    ch1;
  char_data*    ch2;

  if( ( ch1 = character( t1 ) ) && ( ch2 = character( t2 ) ) ) {
    return look_same( ch, ch1, ch2, auction );
  }
 
  return false;
}


bool none_shown( thing_array& array )
{
  for( int i = 0; i < array; i++ )
    if( array[i]->Shown( ) > 0 )
      return false;
  
  return true;
}  


thing_data* one_shown( thing_array& array )
{
  int j = -1;

  for( int i = 0; i < array; i++ )
    if( array[i]->Shown( ) > 0 )
      if( j == -1 ) 
        j = i;
      else
        return 0;

  return j == -1 ? 0 : array[j];
}  


void show_contents( char_data *ch, thing_data *thing )
{
  select( thing->contents );
  rehash_weight( ch, thing->contents );

  if( none_shown( thing->contents ) ) {
    page( ch, "%s is empty.\n\r", thing );
    return;
  }

  page( ch, "Contents of %s:\n\r\n\r", thing );

  page_underlined( ch,
		   "Item                                                                    Weight\n\r" );

  for( int i = 0; i < thing->contents; ++i ) {
    if( obj_data *obj = object( thing->contents[i] ) ) {
      if( obj->Shown( ) > 0 ) {
	page( ch, "%-70s %7.2f\n\r",
	      obj->Seen_Name( ch, obj->Shown( ), true ),
	      0.01*(double)obj->temp );
      }
    }
  }
}


text& list_name( char_data* ch, thing_array *array, const char *conj, bool hash ) 
{
  thing_data *last = 0;
  thing_data *first = 0;
  thing_data *thing;

  if( hash )
    rehash( ch, *array );

  for( int i = 0; i < *array; i++ ) {
    if( array->list[i]->Shown( ) > 0 ) {
      first = array->list[i];
      break;
    }
  }
  
  text& list_text = static_text();

  if( !first ) {
    list_text = "nothing";
    return list_text;
  }

  for( int i = *array-1; i >= 0; i-- )
    if( array->list[i]->Shown( ) > 0 ) {
      last = array->list[i];
      break;
    }

  for( int i = 0; i < *array; i++ ) {
    thing = array->list[i];
    if( thing->Shown( ) > 0 ) {
      if( thing != first ) {
	if( thing == last && conj && *conj ) {
	  list_text += ' ';
	  list_text += conj;
	  list_text += ' ';
	} else {
	  list_text += ", ";
	}
      }
      list_text += thing->Name( ch, thing->Shown( ) );
    }
  }

  return list_text;
}
         

const char *visible_data :: Keywords( char_data* )
{
  return empty_string;
}


const char *visible_data :: Name( const char_data*, int, bool ) const
{
  return "** BUG **";
}


void thing_data :: Extract( )
{
  bug( "thing_data::Extract( ) called" );
  bug( "-- Thing = %s", this );
}


const char *thing_data :: Seen_Name( const char_data*, int, bool ) const
{
  return "** BUG **";
}

const char *thing_data :: Show_To( char_data* )
{
  return "** BUG **";
}


/*
 *   EXTRACTION
 */


void extract( thing_array& array )
{
  for( int i = array-1; i >= 0; --i )
    array[i]->Extract( );
}
