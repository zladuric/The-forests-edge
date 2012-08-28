#include <stdio.h>
#include "define.h"
#include "struct.h"
#include "vehicle.h"


bool embark( char_data *ch, obj_data *obj )
{
  if( obj->pIndexData->item_type != ITEM_VEHICLE ) { 
    return false;
  }
  
  if( is_fighting( ch, 0 ) ) {
    return false;
  }

  /*
  if( is_mounted( ch, 0 ) ) {
    return false;
  }
  */

  if( ch->pos_obj ) {
    return false;
  }

  obj->contents += ch;
  ch->pos_obj = obj;

  disrupt_spell( ch );

  return true;
}


void disembark( char_data *ch )
{
  if( !ch->pos_obj
      || ch->pos_obj->pIndexData->item_type != ITEM_VEHICLE )
    return;

  ch->pos_obj->contents -= ch;
  ch->pos_obj = 0;
}
