#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include "define.h"
#include "struct.h"


int armor_class( obj_data *obj, bool ident )
{ 
  const int ench = ident ? obj->value[0] : 0;

  if( obj->value[1] < 0 ) {
    return obj->value[1] + ench;
  } else {
    return obj->value[1]*(5-obj->rust)/5 + ench;
  }
}
