#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "define.h"
#include "struct.h"


const char *burden_name [] = { "unburdened", "lightly burdened",
			       "encumbered", "heavily burdened",
			       "strained", "stressed", "over-taxed" };

const int default_weight [] =
  { 10, 200, 2000, 5000, 15000, 35000,
    100000, 150000, 400000, 1000000 }; 


int char_data :: can_carry_n( ) const
{
  return MAX_WEAR+2*Dexterity( )+20;
}


int char_data :: get_burden( ) const
{
  return range( 0, ( 6*contents.weight+3*wearing.weight )/Empty_Capacity( ), 6 );
}


void show_burden( char_data *ch )
{
  char& old = ch->pcdata->burden;
  int current = ch->get_burden( );
  
  if( old < 0 || old != current ) {
    fsend( ch, "You are now %s%s%s.",
	   color_scale( ch, current ), burden_name[ current ], normal( ch ) );
    old = current;
  }
}


/*
bool can_carry( char_data* ch, obj_data *obj, bool msg )
{
  if ( obj->number > ch->can_carry_n() - ch->contents.number ) {
    if( msg )
      send( ch, "[ %s: too many items. ]\n\r", obj );
    return false;
  }
*/
  /*
  int wght = obj->Weight( );
  int num  = obj->Count( );
  if( ch->num_ins+num > ch->can_carry_n( ) ) {
    if( msg )
      send( ch, "[ %s: too many items. ]\n\r", obj );
    return false;
  }

  if( ch->wght_ins+wght > ch->can_carry_w( ) ) {
    if( msg )
      send( ch, "[ %s: too heavy. ]\n\r", obj );
    return false;
    }
  */
/*
  return true;
}
*/


/*
 *   NUMBER FUNCTIONS
 */


int thing_data :: Count( int i ) const
{
  if( i == -1 )
    return 1;

  return i;
}


int Obj_Data :: Count( int i ) const
{
  if( pIndexData->item_type == ITEM_MONEY )
    return 0;

  if( i == -1 )
    return Number( );

  return i;
}


/*
 *   BASE WEIGHT FUNCTIONS
 */


int thing_data :: Capacity( )          { return 0; }
int thing_data :: Empty_Capacity( ) const    { return 0; }
int thing_data :: Empty_Weight( int )  { return 0; }


int thing_data :: Weight( int )
{
  return contents.weight;
}


/*
 *   CHARACTER FUNCTIONS
 */


int char_data :: Empty_Capacity( ) const
{
  return 3000*Strength( );
}


int char_data :: Capacity( )
{
  return Empty_Capacity( )-contents.weight-wearing.weight/2;
}


int char_data :: Empty_Weight( int )
{
  if( species ) {
    if( species->weight != 0 )
      return species->weight;

    if( shdata->race >= MAX_PLYR_RACE )
      return default_weight[ species->size ];

  } else if( shdata->race >= MAX_PLYR_RACE )
    return default_weight[ SIZE_HUMAN ];

  if( sex == SEX_MALE )
    return plyr_race_table[ shdata->race ].weight_m;

  if( sex == SEX_FEMALE )
    return plyr_race_table[ shdata->race ].weight_f;

  return ( plyr_race_table[ shdata->race ].weight_m + plyr_race_table[ shdata->race ].weight_f ) / 2;
}


int char_data :: Weight( int )
{
  return contents.weight
         + wearing.weight
         + Empty_Weight( );
}


/*
 *   OBJECT FUNCTIONS
 */


int Obj_Data :: Empty_Capacity( ) const
{
  return 100*value[0];
}


int Obj_Data :: Capacity( )
{
  return Empty_Capacity( )-contents.weight;
}


int Obj_Data :: Weight( int i )
{
  int sum = contents.weight;
  
  if( pIndexData->item_type == ITEM_CONTAINER ) {
    if( is_set( value[1], CONT_HOLDING ) ) {
      sum /= 2;
    }
  } else if( pIndexData->item_type == ITEM_DRINK_CON ) {
    if( value[1] > 0 ) {
      sum += (int) ( 2.2*value[1] );
    }
  }
  
  sum += Empty_Weight( );
  sum *= ( i < 0 ? Number( ) : i );
  
  return sum; 
}


static int metal_weight( obj_data* obj )
{
  for( int i = MAT_BRONZE; i <= MAT_ADAMANTINE; ++i )
    if( is_set( obj->materials, i ) ) 
      return obj->weight*material_table[i].weight/10;

  return obj->weight;
}


int Obj_Data :: Empty_Weight( int )
{
  if( ( pIndexData->item_type != ITEM_WEAPON
	&& pIndexData->item_type != ITEM_ARMOR )
      || !is_set( pIndexData->extra_flags, OFLAG_RANDOM_METAL ) )
    return weight;
  
  return metal_weight( this );
}


int player_data :: Height( ) const
{
  if( shdata->race < MAX_PLYR_RACE ) {
    if( sex == SEX_MALE )
      return plyr_race_table[ shdata->race ].height_m;
    else if( sex == SEX_FEMALE )
      return plyr_race_table[ shdata->race ].height_f;
    else
      return ( plyr_race_table[ shdata->race ].height_m + plyr_race_table[ shdata->race ].height_f ) / 2;
  }

  int default_height [] = {
    1, 6, 24, 48, 66,
    84, 96, 120, 144, 288
  };
  
  return default_height[ SIZE_HUMAN ];
}
