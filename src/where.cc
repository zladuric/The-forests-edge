#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "define.h"
#include "struct.h"


const char* Room_Data :: Location( Content_Array* )
{
  char *tmp = static_string( );
  snprintf( tmp, THREE_LINES, "Room %d, %s", vnum, name );
  return tmp;
}


const char* Obj_Data :: Location( Content_Array *where )
{
  if( !where ) {
    if( !array || !array->where ) {
      return "nowhere";
    }
    return array->where->Location( array );
  }

  char* tmp = static_string( );
  snprintf( tmp, THREE_LINES, "Container, %s", Seen_Name( ) );
  return tmp;
}


const char *char_data :: Location( Content_Array *where )
{
  const char *word;

  if( !where ) {
    if( !array || !array->where ) {
      return "nowhere";
    } else {
      return array->where->Location( array );
    }
  } else if( where == &contents ) {
    word = "Inventory";
  } else if( where == &wearing ) {
    word = "Worn";
  } else {
    word = "** BUG **";
  }

  char *tmp = static_string( );
  snprintf( tmp, THREE_LINES, "%s, %s", Seen_Name( ), word );  
  return tmp;
}


const char *player_data :: Location( Content_Array *where )
{
  const char *word;

  if( !where ) {
    if( !array || !array->where ) {
      return "nowhere";
    } else {
      return array->where->Location( );
    }
  } else if( where == &junked ) {
    word = "Junked";
  } else if( where == &locker ) {
    word = "Locker";
  } else if( where == &contents ) {
    word = "Inventory";
  } else if( where == &wearing ) {
    word = "Worn";
  } else {
    word = "** BUG **";
  }

  char *tmp = static_string( );
  snprintf( tmp, THREE_LINES, "%s, %s", Seen_Name( ), word );  
  return tmp;
}


const char *thing_data :: Location( Content_Array* ) 
{
  return "** BUG **";
}
