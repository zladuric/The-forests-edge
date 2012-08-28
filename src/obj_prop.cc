#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "define.h"
#include "struct.h"


int obj_clss_data :: metal( ) const
{
  if( is_set( extra_flags, OFLAG_RANDOM_METAL ) )
    return -1;

  for( int i = MAT_BRONZE; i <= MAT_ADAMANTINE; ++i )
    if( is_set( materials, i ) )
      return i;

  return 0;
}


int obj_clss_data :: any_metal( ) const
{
  if( is_set( extra_flags, OFLAG_RANDOM_METAL ) )
    return -1;

  for( int i = MAT_BRONZE; i <= MAT_BRASS; ++i )
    if( is_set( materials, i ) )
      return i;

  return 0;
}


bool obj_clss_data :: is_wearable( ) const
{
  if( item_type == ITEM_WEAPON || item_type == ITEM_ARMOR )
    return true;

  int i = LAYER_BOTTOM;
  for( ; i < MAX_LAYER; ++i ) {
    if( is_set( layer_flags, i ) )
      break;
  }

  if( i != MAX_LAYER ) {
    for( i = 0; i < MAX_ITEM_WEAR; ++i ) {
      if( i != ITEM_TAKE
	  && i != ITEM_HELD_L
	  && i != ITEM_HELD_R
	  && i != ITEM_WEAR_FLOATING
	  && i != ITEM_WEAR_UNUSED0
	  && i != ITEM_WEAR_UNUSED1
	  && i != ITEM_WEAR_UNUSED2
	  && i != ITEM_WEAR_UNUSED3
	  && i != ITEM_WEAR_UNUSED4
	  && is_set( wear_flags, i ) )
	break;
    }
    if( i != MAX_ITEM_WEAR ) 
      return true;
  }

  return false;
}


bool obj_clss_data :: is_container( ) const
{
  return ( item_type == ITEM_CONTAINER
	   || item_type == ITEM_CORPSE
	   || item_type == ITEM_TABLE 
	   || item_type == ITEM_KEYRING
	   || item_type == ITEM_PIPE
	   || ( item_type == ITEM_WEAPON
		&& value[3] == WEAPON_BOW-WEAPON_UNARMED ) );
}


bool Obj_Data :: droppable( const char_data *ch ) const
{
  return privileged( ch, LEVEL_BUILDER )
    || !is_set( extra_flags, OFLAG_NODROP );
}


bool Obj_Data :: removable( const char_data *ch ) const
{
  return privileged( ch, LEVEL_BUILDER )
    || !is_set( extra_flags, OFLAG_NOREMOVE );
}


int Obj_Data :: metal( ) const
{
  for( int i = MAT_BRONZE; i <= MAT_ADAMANTINE; ++i )
    if( is_set( materials, i ) )
      return i;

  return 0;
}


int Obj_Data :: any_metal( ) const
{
  for( int i = MAT_BRONZE; i <= MAT_BRASS; ++i )
    if( is_set( materials, i ) )
      return i;

  return 0;
}


static int metal_factor( const obj_data* obj )
{
  for( int i = MAT_BRONZE; i <= MAT_ADAMANTINE; ++i )
    if( is_set( obj->materials, i ) ) 
      return ( i - MAT_BRONZE );

  return 0;
}


int Obj_Data :: Durability( ) const
{
  if( ( pIndexData->item_type != ITEM_WEAPON
	&& pIndexData->item_type != ITEM_ARMOR )
      || !is_set( pIndexData->extra_flags, OFLAG_RANDOM_METAL ) )
    return pIndexData->durability;
  
  return pIndexData->durability + 100*metal_factor( this );
}


int Obj_Data :: Level( ) const
{
  if( ( pIndexData->item_type != ITEM_WEAPON
	&& pIndexData->item_type != ITEM_ARMOR )
      || !is_set( pIndexData->extra_flags, OFLAG_RANDOM_METAL ) )
    return pIndexData->level;

  return pIndexData->level + 5*metal_factor( this );
}


int Obj_Data :: Enchantment( ) const
{
  if( ( pIndexData->item_type == ITEM_WEAPON
	|| pIndexData->item_type == ITEM_ARMOR
	|| pIndexData->item_type == ITEM_ARROW )
      && is_set( extra_flags, OFLAG_IDENTIFIED ) ) {
    return value[0];
  }

  return 0;
}


int Obj_Data :: Cost( ) const
{
  int value = cost;
  const int ench = Enchantment( );

  if( ench < 0 ) {
    return 0;
  } else if( ench > 0 ) {
    value += value*sqr( ench )/2;
  }
  
  if( ( pIndexData->item_type != ITEM_WEAPON
	&& pIndexData->item_type != ITEM_ARMOR )
      || !is_set( pIndexData->extra_flags, OFLAG_RANDOM_METAL ) )
    return value;
  
  for( int i = MAT_BRONZE; i <= MAT_ADAMANTINE; i++ )
    if( is_set( materials, i ) ) 
      return value*material_table[i].cost;

  return value;
}
