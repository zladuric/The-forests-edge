#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include "define.h"
#include "struct.h"


const void *code_list_assign( const void **argument )
{
  arg_type *arg = (arg_type*) argument[0];
  thing_array& value = *(thing_array*) argument[1];

  thing_array& list = **(thing_array**) arg->value;
  list = value;

  return (void*) list.size;
}


const void *code_prepend( const void **argument )
{
  thing_array *list = (thing_array*) argument[0];
  thing_data *thing = (thing_data*) argument[1];

  if( !thing )
    return 0;

  list->insert( thing, 0 );

  return 0;
}


const void *code_append( const void **argument )
{
  thing_array *list = (thing_array*) argument[0];
  thing_data *thing = (thing_data*) argument[1];

  if( !thing )
    return 0;

  list->append( thing );

  return 0;
}


const void *code_remove( const void **argument )
{
  thing_array *list = (thing_array*) argument[0];
  const int n = (int) argument[1];

  if( n < 0 ) {
    code_bug( "Item: index < 0." );
    return 0;
  }

  if( n >= list->size ) {
    code_bug( "Item: index >= list length." );
    return 0;
  }

  thing_data *thing = list->list[n];

  list->remove( n );

  return thing;
}


const void *code_insert( const void **argument )
{
  thing_array *list = (thing_array*) argument[0];
  thing_data *thing = (thing_data*) argument[1];
  const int n = (int) argument[2];

  if( !thing )
    return 0;

  if( n < 0 ) {
    code_bug( "Item: index < 0." );
    return 0;
  }

  if( n > list->size ) {
    code_bug( "Item: index > list length." );
    return 0;
  }

  list->insert( thing, n );

  return 0;
}


const void *code_contents( const void **argument )
{
  thing_data *thing = (thing_data*) argument[0];

  static thing_array result;

  result = thing->contents;

  for( int i = 0; i < result; ) {
    if( char_data *rch = character( result[i] ) ) {
      if( invis_level( rch ) >= LEVEL_BUILDER ) {
	result.remove( i );
	continue;
      }
    }
    ++i;
  }

  select( result );

  return &result;
}


const void *code_wearing( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];

  static thing_array result;

  result = ch->wearing;

  select( result );

  return &result;
}


const void *code_followers( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];

  static thing_array result;

  result = *(thing_array*) &ch->followers;

  select( result );
  
  return &result;
}


const void *code_item( const void **argument )
{
  thing_array *list = (thing_array*) argument[0];
  const int n = (int) argument[1];

  if( n < 0 ) {
    code_bug( "Item: index < 0." );
    return 0;
  }

  if( n >= list->size ) {
    return 0;
  }

  return list->list[n];
}


const void *code_length( const void **argument )
{
  thing_array *list = (thing_array*) argument[0];

  return (void*) list->size;
}


const void *code_is_empty( const void **argument )
{
  thing_array *list = (thing_array*) argument[0];

  return (void*) list->is_empty( );
}


const void *code_where( const void **argument )
{
  thing_data *thing = (thing_data*) argument[0];

  if( !thing ) {
    code_bug( "Where: NULL thing." );
    return 0;
  }

  if( !thing->array )
    return 0;

  return (void*) thing->array->where;
}


const void *code_list_name( const void **argument )
{
  thing_array *list = (thing_array*) argument[0];
  char_data *ch = (char_data*)(thing_data*) argument[1];
  const char *conj = (const char*) argument[2];

  return list_name( ch, list, conj ).c_str();
}


const void *code_take( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];
  thing_array *list = (thing_array*) argument[1];
  obj_data *container = (obj_data*)(thing_data*) argument[2];

  if( !ch ) {
    code_bug( "Take: NULL character." );
    return 0;
  }

  for( int i = 0; i < *list; ) {
    obj_data *obj = object( list->list[i] );
    if( !obj
	|| !obj->array
	|| !obj->array->where
	|| !ch->in_room ) {
      list->remove(i);
      continue;
    }
    if( container ) {
      if( obj->array->where != container
	  || !container->array
	  || ( container->array != &ch->in_room->contents
	       && container->array != &ch->contents
	       && container->array != &ch->wearing ) ) {
	list->remove(i);
	continue;
      }
    } else {
      if( obj->array != &ch->in_room->contents ) {
	list->remove(i);
	continue;
      }
    }

    ++i;
  }

  get_obj( ch, *list, container, false );

  return 0;
}


const void *code_drop( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];
  thing_array *list = (thing_array*) argument[1];

  if( !ch ) {
    code_bug( "Drop: NULL character." );
    return 0;
  }

  for( int i = 0; i < *list; ) {
    obj_data *obj = object( list->list[i] );
    if( !obj
	|| obj->array != &ch->contents ) {
      list->remove(i);
      continue;
    }
    ++i;
  }

  drop( ch, *list, false );

  return 0;
}


const void *code_junk( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];
  thing_array *list = (thing_array*) argument[1];

  if( !ch ) {
    code_bug( "Junk: NULL character." );
    return 0;
  }

  for( int i = 0; i < *list; ) {
    obj_data *obj = object( list->list[i] );
    if( !obj
	|| obj->array != &ch->contents ) {
      list->remove(i);
      continue;
    }
    ++i;
  }

  junk( ch, *list, false );

  return 0;
}
