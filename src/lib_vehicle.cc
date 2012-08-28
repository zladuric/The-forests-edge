#include <ctype.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include "define.h"
#include "struct.h"


const void *code_embark( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];
  obj_data *obj = (obj_data*)(thing_data*) argument[1];

  if( !ch ) {
    code_bug( "Embark: character is null." );
    return 0;
  }

  if( !obj ) {
    code_bug( "Embark: vehicle is null." );
    return 0;
  }

  return 0;
}


const void *code_disembark( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];

  if( !ch ) {
    code_bug( "Disembark: character is null." );
    return 0;
  }

  return 0;
}
