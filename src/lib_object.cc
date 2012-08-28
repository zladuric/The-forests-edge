#include <ctype.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include "define.h"
#include "struct.h"


const void *code_number( const void **argument )
{
  obj_data *obj = (obj_data*)(thing_data*) argument[0];

  if( !obj ) {
    code_bug( "Number: Null object." );
    return 0;
  }

  return (void*) obj->Number( );
}


const void *code_selected( const void **argument )
{
  obj_data *obj = (obj_data*)(thing_data*) argument[0];

  if( !obj ) {
    code_bug( "Selected: Null object." );
    return 0;
  }

  return (void*) obj->Selected( );
}


const void *code_oload( const void **argument )
{
  int vnum = (int) argument[0];
  int i = (int) argument[1];
  char_data *ch = (char_data*)(thing_data*) argument[2];
 
  obj_clss_data *clss = get_obj_index( vnum );

  if( !clss ) {
    code_bug( "Oload: bad object vnum." );
    return 0;
  }

  if( clss->level > LEVEL_HERO ) {
    code_bug( "Oload: bad object level." );
    return 0;
  }

  if( i == 0 ) {
    i = 1;
  } else if( i < 0 || i * clss->level > 100 ) {
    code_bug( "Oload: bad object count." );
    return 0;
  }

  if( clss->limit >= 0
      && clss->count >= clss->limit ) {
    return 0;
  }

  obj_data *obj;
  if( ( obj = create( clss, i ) ) ) {
    set_alloy( obj, 10 );
    set_quality( obj );
    set_size( obj, ch );
  } else {
    code_bug( "Oload: bad object." );
    return 0;
  }
  
  return obj;
}


const void *code_obj_value( const void **argument )
{
  obj_data *obj = (obj_data*)(thing_data*) argument[0];
  int i  = (int) argument[1];

  if( !obj ) {
    code_bug( "Obj_Value: Null object." );
    return 0;
  }

  if( i < 0 || i > 3 ) {
    code_bug( "Obj_Value: Value number out of range." );
    return 0;
  }

  return (void*) obj->value[i];
}


const void *code_set_obj_timer( const void **argument )
{
  obj_data*  obj  = (obj_data*)(thing_data*) argument[0];
  int          i  = (int)       argument[1];

  if( !obj ) {
    code_bug( "Set_Obj_Timer: Null object." );
    return 0;
  }

  if( i < 0 ) {
    code_bug( "Set_Obj_Timer: Negative time." );
    return obj;
  }

  // Must do from/to because of consolidation.

  Content_Array *where = obj->array;

  if( where ) {
    obj = (obj_data*) obj->From( 1, true );
  }

  obj->timer = i * PULSE_PER_SECOND;

  if( where ) {
    obj->To( );
  }

  return obj;
}


const void *code_set_obj_value( const void **argument )
{
  obj_data *obj  = (obj_data*)(thing_data*) argument[0];
  int i = (int) argument[1];
  int val = (int) argument[2];
  int n = (int) argument[3];

  if( !obj ) {
    code_bug( "Set_Obj_Value: Null object." );
    return 0;
  }

  if( i < 0 || i > 3 ) {
    code_bug( "Set_Obj_Value: Value number out of range." );
    return obj;
  }

  if( n == -1 ) {
    n = obj->Number( );
  } else if( n < 0 || n > obj->Number( ) ) {
    code_bug( "Set_Obj_Value: Count out of range." );
    return obj;
  }

  if( n == 0 ) {
    n = obj->Selected( );
    if( n == 0 )
      return obj;
  }

  if( obj->array ) {
    push( );
    obj = (obj_data *) obj->From( n, true );
    obj->value[i] = val;
    obj->To( );
    pop( );
  } else {
    obj->value[i] = val;
  }

  return obj;
}


const void *code_junk_obj( const void **argument )
{
  obj_data*     obj  = (obj_data*)(thing_data*) argument[0];
  int             i  = (int)       argument[1];  

  if( !obj ) {
    code_bug( "Junk_Obj: Null object." );
    return 0;
  }

  push( );

  if( i == -1 ) {
    i = obj->Number( );
  } else if( i < 0 || i > obj->Number( ) ) {
    code_bug( "Junk_Obj: Count out of range." );
    return obj;
  }

  if( i == 0 ) {
    obj->Extract( obj->Selected( ) );
  } else {
    obj->Extract( i );
  }

  pop( );

  return 0;
}


const void *code_replace_obj( const void **argument )
{
  obj_data *obj = (obj_data*)(thing_data*) argument[0];
  int          vnum  = (int)       argument[1];
  int             i  = (int)       argument[2];  
  char_data *ch = (char_data*)(thing_data*) argument[3];

  if( !obj ) {
    code_bug( "Replace_Obj: Null object." );
    return 0;
  }

  Content_Array *array = obj->array;

  if( !array ) {
    code_bug( "Replace_Obj: Object is nowhere." );
    return 0;
  }

  const int position = obj->position;
  const int layer = obj->layer;

  push( );

  int num;

  if( i == 0 ) {
    num = obj->Selected( );
    obj->Extract( num );
  } else {
    num = min( i, obj->Number( ) );
    if( num != i ) {
      code_bug( "Replace_Obj: Number too high." );
    }
    obj->Extract( i );
  }

  obj_clss_data *clss = get_obj_index( vnum );

  if( clss ) {
    if( clss->level > LEVEL_HERO ) {
      code_bug( "Replace_Obj: bad object level." );
      pop( );
      return 0;
    }
    
    if( clss->limit >= 0
	&& clss->count >= clss->limit ) {
      pop( );
      return 0;
    }

    if( ( obj = create( clss, num ) ) ) {
      set_alloy( obj, 10 );
      set_quality( obj );
      set_size( obj, ch );
      obj->position = position;
      obj->layer = layer;
      obj->To( *array );
      pop( );
      return obj;
    }
  }

  pop( );

  code_bug( "Replace_Obj: Bad replacement." );
  return 0;
}


const void *code_obj_condition( const void **argument )
{
  obj_data *obj = (obj_data*)(thing_data*) argument[0];

  if( !obj ) {
    code_bug( "Obj_Condition: NULL object." );
    return 0;
  }

  return (void *) obj->condition;
}


const void *code_obj_durability( const void **argument )
{
  obj_data *obj = (obj_data*)(thing_data*) argument[0];

  if( !obj ) {
    code_bug( "Obj_Condition: NULL object." );
    return 0;
  }

  return (void *) obj->Durability( );
}


const void *code_set_after( const void **argument )
{
  obj_data *obj  = (obj_data*)(thing_data*) argument[0];
  char  *string  = (char*)      argument[1];
  
  if( !obj ) {
    code_bug( "Code_set_after: NULL obj." );
    return 0;
  }

  if( obj->after != obj->pIndexData->after ) {
    free_string( obj->after, MEM_OBJECT );
  }
  obj->after = alloc_string( string, MEM_OBJECT );

  return 0;
}


const void *code_set_before( const void **argument )
{
  obj_data *obj  = (obj_data*)(thing_data*) argument[0];
  char  *string  = (char*)      argument[1];
  
  if( !obj ) {
    code_bug( "Code_set_before: NULL obj." );
    return 0;
  }

  if( obj->before != obj->pIndexData->before ) {
    free_string( obj->before, MEM_OBJECT );
  }
  obj->before = alloc_string( string, MEM_OBJECT );

  return 0;
}


const void *code_owns( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];
  obj_data *obj = (obj_data*)(thing_data*) argument[1];

  if( !obj ) {
    code_bug( "Owns: NULL object." );
    return 0;
  }

  if( !ch )
    return (void*)( !obj->owner );

  player_data *pl = player( ch );

  if( !pl )
    return (void*) false;

  return (void*) ( obj->owner == pl->pcdata->pfile );
}


const void *code_set_owner( const void **argument )
{
  obj_data *obj = (obj_data*)(thing_data*) argument[0];
  char_data *ch = (char_data*)(thing_data*) argument[1];

  if( !obj ) {
    code_bug( "Set_Owner: NULL object." );
    return 0;
  }

  pfile_data *pfile = ( !ch || ch->species ) ? 0 : ch->pcdata->pfile;
  Content_Array *where = obj->array;

  if( where ) {
    obj = (obj_data*) obj->From( 1, true );
  }

  set_owner( obj, pfile, obj->owner );
  
  if( where ) {
    obj->To( );
  }

  return obj;
}


const void *code_has_obj( const void **argument )
{
  const int vnum = (int) argument[0];
  thing_data *thing = (thing_data*) argument[1];

  if( !thing ) {
    code_bug( "Has_Obj: NULL location." );
    return 0;
  }
 
  return find_vnum( thing->contents, vnum );
}


const void *code_has_obj_flag( const void **argument )
{
  const int flag = (int) argument[0];
  thing_data *thing = (thing_data*) argument[1];

  if( !thing ) {
    code_bug( "Has_Obj_Flag: NULL location." );
    return 0;
  }
 
  return find_oflag( thing->contents, flag );
}


const void *code_wearing_obj( const void **argument )
{
  const int vnum  = (int) argument[0];
  char_data *ch  = (char_data*)(thing_data*) argument[1];

  if( !ch ) {
    code_bug( "Wearing_Obj: NULL character." );
    return 0;
  }
 
  return find_vnum( ch->wearing, vnum );
}


const void *code_wearing_obj_flag( const void **argument )
{
  const int flag = (int) argument[0];
  char_data *ch = (char_data*)(thing_data*) argument[1];

  if( !ch ) {
    code_bug( "Wearing_Obj_Flag: NULL character." );
    return 0;
  }

  return find_oflag( ch->wearing, flag );
}


const void *code_worn( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];
  int slot = (int)argument[1];
  int layer = (int)argument[2];

  if( !ch ) {
    code_bug( "Worn: NULL character." );
    return 0;
  }
 
  if( slot < 0 || slot >= MAX_WEAR ) {
    code_bug( "Worn: Bad slot." );
    return 0;
  }

  --layer;

  if( layer < -1 || layer >= MAX_LAYER ) {
    code_bug( "Worn: Bad layer." );
    return 0;
  }

  return (void*) ch->Wearing( slot, layer );
}


const void *code_obj_in_room( const void **argument )
{
  int         vnum  = (int)       argument[0];
  room_data*  room  = (room_data*)(thing_data*) argument[1];

  if( !room ) {
    code_bug( "Obj_in_room: NULL room." );
    return 0;
  }

  return find_vnum( room->contents, vnum );
}


const void *code_obj_to_room( const void **argument )
{
  obj_data *obj = (obj_data*)(thing_data*) argument[0];
  room_data *room = (room_data*)(thing_data*) argument[1];
  int n = (int) argument[2];

  if( !obj ) {
    code_bug( "Obj_to_room: NULL object." );
    return 0;
  }

  if( !room ) {
    code_bug( "Obj_to_room: NULL room." );
    return 0;
  }

  if( n < 0 ) {
    code_bug( "Obj_to_room: negative count." );
    return 0;
  }

  if( n == 0 )
    n = 1;

  if( n > obj->Number( ) ) {
    code_bug( "Obj_to_room: count > number of objects." );
    return 0;
  }

  push();

  if( obj->array ) {
    obj = (obj_data *) obj->From( n );
  }

  obj->To( room );

  pop();

  return obj;
}


const void *code_obj_to_char( const void **argument )
{
  obj_data*   obj  = (obj_data*)(thing_data*)  argument[0];
  char_data*   ch  = (char_data*)(thing_data*) argument[1];
  int n = (int) argument[2];

  if( !obj ) {
    code_bug( "Obj_to_char: NULL object." );
    return 0;
  }

  if( !ch ) {
    code_bug( "Obj_to_char: NULL character." );
    return 0;
  }

  if( n < 0 ) {
    code_bug( "Obj_to_char: negative count." );
    return 0;
  }

  if( n == 0 )
    n = 1;

  if( n > obj->Number( ) ) {
    code_bug( "Obj_to_char: count > number of objects." );
    return 0;
  }

  push();

  if( obj->array ) {
    obj = (obj_data *) obj->From( n );
  }

  obj->Select( n );

  obj->To( ch );

  pop();

  return obj;
}


const void *code_obj_to_container( const void **argument )
{
  obj_data *obj = (obj_data*)(thing_data*) argument[0];
  obj_data *container = (obj_data*)(thing_data*) argument[1];
  int n = (int) argument[2];

  if( !obj ) {
    code_bug( "Obj_to_container: NULL object." );
    return 0;
  }

  if( !container ) {
    code_bug( "Obj_to_container: NULL container." );
    return 0;
  }

  if( obj == container ) {
    code_bug( "Obj_to_container: object == container." );
    return 0;
  }

  //  int type = container->pIndexData->item_type;

  /*
  if( type != ITEM_CONTAINER
      && type != ITEM_TABLE ) {
    code_bug( "Obj_to_container: container is not type \"container\" or \"table\"." );
    return 0;
  }
  */

  if( n < 0 ) {
    code_bug( "Obj_to_container: negative count." );
    return 0;
  }

  if( n == 0 )
    n = 1;

  if( n > obj->Number( ) ) {
    code_bug( "Obj_to_container: count > number of objects." );
    return 0;
  }

  push();

  if( obj->array ) {
    obj = (obj_data *) obj->From( n );
  }

  obj->To( container );

  pop();

  return obj;
}


const void *code_obj_affected( const void **argument )
{
  obj_data  *obj  = (obj_data*)(thing_data*)  argument[0];
  int        aff  = (int)        argument[1];
  
  if( !obj ) {
    code_bug( "Obj_affected: NULL obj." );
    return 0;
  }

  if( aff < 0
      || aff >= MAX_AFF_CHAR ) {
    code_bug( "Obj_affected: affect value out of range." );
    return 0;
  }

  return (void*) is_set( obj->pIndexData->affect_flags, aff );
}


const void *code_obj_flag( const void **argument )
{
  obj_data  *obj  = (obj_data*)(thing_data*)  argument[0];
  int       flag  = (int)        argument[1];
  
  if( !obj ) {
    code_bug( "Obj_flag: NULL obj." );
    return 0;
  }

  if( flag < 0
      || flag >= MAX_OFLAG ) {
    code_bug( "Obj_flag: flag number out of range." );
    return 0;
  }

  return (void*) is_set( obj->extra_flags, flag );
}


const void *code_set_obj_flag( const void **argument )
{
  obj_data  *obj  = (obj_data*)(thing_data*) argument[0];
  int       flag  = (int) argument[1];
  int          n  = (int) argument[2];

  if( !obj ) {
    code_bug( "Set_obj_flag: NULL obj." );
    return 0;
  }

  if( flag < 0
      || flag >= MAX_OFLAG ) {
    code_bug( "Set_obj_flag: flag number out of range." );
    return 0;
  }

  if( n == -1 ) {
    n = obj->Number( );
  } else if( n < 0 || n > obj->Number( ) ) {
    code_bug( "Set_Obj_Flag: Count out of range." );
    return obj;
  }

  if( n == 0 ) {
    n = obj->Selected( );
    if( n == 0 )
      return obj;
  }

  if( obj->array ) {
    push( );
    obj = (obj_data *) obj->From( n, true );
    set_bit( obj->extra_flags, flag );
    obj->To( );
    pop( );
  } else {
    set_bit( obj->extra_flags, flag );
  }

  return obj;
}


const void *code_remove_obj_flag( const void **argument )
{
  obj_data  *obj  = (obj_data*)(thing_data*) argument[0];
  int       flag  = (int) argument[1];
  int          n  = (int) argument[2];

  if( !obj ) {
    code_bug( "Set_obj_flag: NULL obj." );
    return 0;
  }

  if( flag < 0
      || flag >= MAX_OFLAG ) {
    code_bug( "Set_obj_flag: flag number out of range." );
    return 0;
  }

  if( n == -1 ) {
    n = obj->Number( );
  } else if( n < 0 || n > obj->Number( ) ) {
    code_bug( "Set_Obj_Flag: Count out of range." );
    return obj;
  }

  if( n == 0 ) {
    n = obj->Selected( );
    if( n == 0 )
      return obj;
  }

  if( obj->array ) {
    push( );
    obj = (obj_data *) obj->From( n, true );
    remove_bit( obj->extra_flags, flag );
    obj->To( );
    pop( );
  } else {
    remove_bit( obj->extra_flags, flag );
  }

  return obj;
}


const void *code_set_weight( const void **argument )
{
  obj_data *obj = (obj_data*)(thing_data*) argument[0];
  int weight = (int) argument[1];
  int n = (int) argument[2];

  if( !obj ) {
    code_bug( "Set_Weight: NULL object." );
    return 0;
  }

  if( weight < 0 ) {
    code_bug( "Set_Weight: negative weight." );
    return 0;
  }

  if( n == -1 ) {
    n = obj->Number( );
  } else if( n < 0 || n > obj->Number( ) ) {
    code_bug( "Set_Weight: Count out of range." );
    return obj;
  }

  if( n == 0 ) {
    n = obj->Selected( );
    if( n == 0 )
      return obj;
  }

  Content_Array *where = obj->array;

  if( where ) {
    obj = (obj_data*) obj->From( n, true );
  }

  obj->weight = weight;
  
  if( where ) {
    obj->To( );
  }

  return obj;
}


const void *code_obj_cost( const void **argument )
{
  obj_data *obj = (obj_data*)(thing_data*) argument[0];

  if( !obj ) {
    code_bug( "Obj_Cost: NULL object." );
    return 0;
  }

  return (void*) obj->Cost( );
}


const void *code_set_cost( const void **argument )
{
  obj_data *obj = (obj_data*)(thing_data*) argument[0];
  const int cost = (int) argument[1];
  int n = (int) argument[2];

  if( !obj ) {
    code_bug( "Set_Cost: NULL object." );
    return 0;
  }

  if( cost < 0 ) {
    code_bug( "Set_Cost: negative cost." );
    return 0;
  }

  if( n == -1 ) {
    n = obj->Number( );
  } else if( n < 0 || n > obj->Number( ) ) {
    code_bug( "Set_Cost: Count out of range." );
    return obj;
  }

  if( n == 0 ) {
    n = obj->Selected( );
    if( n == 0 )
      return obj;
  }

  Content_Array *where = obj->array;

  if( where ) {
    obj = (obj_data*) obj->From( n, true );
  }

  obj->cost = cost;
  
  if( where ) {
    obj->To( );
  }

  return obj;
}


const void *code_set_light( const void **argument )
{
  obj_data *obj = (obj_data*)(thing_data*) argument[0];
  int light = (int) argument[1];
  int n = (int) argument[2];

  if( !obj ) {
    code_bug( "Set_Light: NULL object." );
    return 0;
  }

  /*
  if( light < 0 ) {
    code_bug( "Set_Light: negative light." );
    return 0;
  }
  */

  if( n == -1 ) {
    n = obj->Number( );
  } else if( n < 0 || n > obj->Number( ) ) {
    code_bug( "Set_Light: Count out of range." );
    return obj;
  }

  if( n == 0 ) {
    n = obj->Selected( );
    if( n == 0 )
      return obj;
  }

  Content_Array *where = obj->array;

  if( where ) {
    obj = (obj_data*) obj->From( n, true );
  }

  obj->light = light;
  
  if( where ) {
    obj->To( );
  }

  return obj;
}

const void *code_obj_type( const void **argument )
{
  obj_data *obj = (obj_data*)(thing_data*) argument[0];

  if( !obj ) {
    code_bug( "Obj_Type: NULL object." );
    return (void*) -1;
  }

  return (void*) obj->pIndexData->item_type;
}


const void *code_has_key( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];
  int vnum = (int) argument[1];

  if( !ch ) {
    code_bug( "Has_Key: character is null." );
    return 0;
  }

  if( vnum <= 0 ) {
    code_bug( "Has_Key: bad vnum." );
    return 0;
  }

  return has_key( ch, vnum );
}


const void *code_obj_count( const void **argument )
{
  const int vnum = (int) argument[0];

  obj_clss_data *clss = get_obj_index( vnum );

  if( !clss ) {
    code_bug( "Obj_Count: bad object vnum." );
    return 0;
  }

  return (void*) clss->count;
}


const void *code_obj_limit( const void **argument )
{
  const int vnum = (int) argument[0];

  obj_clss_data *clss = get_obj_index( vnum );

  if( !clss ) {
    code_bug( "Obj_Limit: bad object vnum." );
    return 0;
  }

  return (void*) clss->limit;
}


const void *code_obj_level( const void **argument )
{
  obj_data *obj = (obj_data*)(thing_data*) argument[0];

  if( !obj ) {
    code_bug( "Obj_Level: NULL object." );
    return (void*) -1;
  }

  return (void*) obj->Level( );
}
