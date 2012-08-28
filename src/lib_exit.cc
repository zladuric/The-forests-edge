#include <ctype.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include "define.h"
#include "struct.h"


static bool open_exit( exit_data *exit )
{
  const bool result = is_set( exit->exit_info, EX_CLOSED );
  remove_bit( exit->exit_info, EX_CLOSED );

  if( exit_data *back = reverse( exit ) ) {
    if( is_set( back->affected_by, AFF_WIZLOCK ) ) {
      room_data *to = exit->to_room;
      for( int i = 0; i < to->affected; ++i ) {
	affect_data *aff = to->affected[i];
	if( aff->type == AFF_WIZLOCK
	    && aff->target == back ) {
	  remove_affect( to, aff );
	  break;
	}
      }
    }
    remove_bit( back->exit_info, EX_CLOSED );
  }

  return result;
}


const void *code_open( const void **argument )
{
  thing_data *thing = (thing_data*) argument[0];

  if( room_data *room = Room( thing ) ) {
    const int dir = (int) argument[1];
    if( dir < 0 || dir >= MAX_DIR ) {
      code_bug( "Open: bad direction." );
      return 0;
    }
    
    if( exit_data *exit = exit_direction( room, dir ) ) {
      return (void*) open_exit( exit );
    }

    return 0;

  } else if( obj_data *obj = object( thing ) ) {
    if( obj->pIndexData->item_type != ITEM_CONTAINER ) {
      code_bug( "Open: object is not a container." );
      return 0;
    }

    const bool result = is_set( obj->value[1], CONT_CLOSED );
    remove_bit( obj->value[1], CONT_CLOSED );
    remove_bit( obj->value[1], CONT_LOCKED );

    return (void*) result;
  }

  code_bug( "Open: bad target." );
  return 0;
}


const void *code2_open( const void **argument )
{
  exit_data *exit = (exit_data*) argument[0];

  if( !exit ) {
    code_bug( "Open: null exit." );
    return 0;
  }

  return (void*) open_exit( exit );
}


static bool close_exit( exit_data *exit )
{
  const bool result = !is_set( exit->exit_info, EX_CLOSED );
  set_bit( exit->exit_info, EX_CLOSED );

  if( ( exit = reverse( exit ) ) )
    set_bit( exit->exit_info, EX_CLOSED );
  
  return result;
}


const void *code_close( const void **argument )
{
  thing_data *thing = (thing_data*) argument[0];

  if( room_data *room = Room( thing ) ) {
    const int dir = (int) argument[1];
    
    if( dir < 0 || dir >= MAX_DIR ) {
      code_bug( "Close: bad direction." );
      return 0;
    }
    
    if( exit_data *exit = exit_direction( room, dir ) ) {
      return (void*) close_exit( exit );
    }

    return 0;
    
  } else if( obj_data *obj = object( thing ) ) {
    if( obj->pIndexData->item_type != ITEM_CONTAINER ) {
      code_bug( "Close: object is not a container." );
      return 0;
    }

    const bool result = !is_set( obj->value[1], CONT_CLOSED );
    set_bit( obj->value[1], CONT_CLOSED );

    return (void*) result;
  }
  
  code_bug( "Close: bad target." );
  return 0;
}


const void *code2_close( const void **argument )
{
  exit_data *exit = (exit_data*) argument[0];

  if( !exit ) {
    code_bug( "Close: null exit." );
    return 0;
  }

  return (void*) close_exit( exit );
}


static bool lock_exit( exit_data *exit )
{
  const bool result = !is_set( exit->exit_info, EX_LOCKED );
  set_bit( exit->exit_info, EX_LOCKED );

  if( ( exit = reverse( exit ) ) )
    set_bit( exit->exit_info, EX_LOCKED );

  return result;
}


const void *code_lock( const void **argument )
{
  thing_data *thing = (thing_data*) argument[0];
  
  if( room_data *room = Room( thing ) ) {
    const int dir = (int) argument[1];
    
    if( dir < 0 || dir >= MAX_DIR ) {
      code_bug( "Lock: bad direction." );
      return 0;
    }
    
    exit_data *exit;
    
    if( room
	&& ( exit = exit_direction( room, dir ) ) ) {
      return (void*) lock_exit( exit );
    }

    return 0;
    
  } else if( obj_data *obj = object( thing ) ) {
    if( obj->pIndexData->item_type != ITEM_CONTAINER ) {
      code_bug( "Lock: object is not a container." );
      return 0;
    }

    if( !is_set( obj->value[1], CONT_CLOSED ) ) {
      code_bug( "Lock: container is not closed." );
      return 0;
    }

    const bool result = !is_set( obj->value[1], CONT_LOCKED );
    set_bit( obj->value[1], CONT_LOCKED );

    return (void*) result;
  }

  code_bug( "Lock: bad target." );
  return 0;
}


const void *code2_lock( const void **argument )
{
  exit_data *exit = (exit_data*) argument[0];

  if( !exit ) {
    code_bug( "Lock: null exit." );
    return 0;
  }
  
  return (void*) lock_exit( exit );
}


static bool unlock_exit( exit_data *exit )
{
  const bool result = is_set( exit->exit_info, EX_LOCKED );
  remove_bit( exit->exit_info, EX_LOCKED );

  if( ( exit = reverse( exit ) ) )
    remove_bit( exit->exit_info, EX_LOCKED );

  return result;
}


const void *code_unlock( const void **argument )
{
  thing_data *thing = (thing_data*) argument[0];
  
  if( room_data *room = Room( thing ) ) {
    const int dir = (int) argument[1];
    
    if( dir < 0 || dir >= MAX_DIR ) {
      code_bug( "Unlock: bad direction." );
      return 0;
    }
    
    exit_data *exit;
    
    if( room
	&& ( exit = exit_direction( room, dir ) ) ) {
      return (void*) unlock_exit( exit );
    }

    return 0;
    
  } else if( obj_data *obj = object( thing ) ) {
    if( obj->pIndexData->item_type != ITEM_CONTAINER ) {
      code_bug( "Unlock: object is not a container." );
      return 0;
    }
    
    if( !is_set( obj->value[1], CONT_CLOSED ) ) {
      code_bug( "Unlock: container is not closed." );
      return 0;
    }
    
    const bool result = is_set( obj->value[1], CONT_LOCKED );
    remove_bit( obj->value[1], CONT_LOCKED );
    
    return (void*) result;
  }
  
  code_bug( "Unlock: bad target." );
  return 0;
}


const void *code2_unlock( const void **argument )
{
  exit_data *exit = (exit_data*) argument[0];

  if( !exit ) {
    code_bug( "Unlock: null exit." );
  }

  return (void*) unlock_exit( exit );
}


const void *code_is_open( const void **argument )
{
  thing_data *thing = (thing_data*) argument[0];
  
  if( room_data *room = Room( thing ) ) {
    const int dir = (int) argument[1];
    
    if( dir < 0 || dir >= MAX_DIR ) {
      code_bug( "Is_Open: bad direction." );
      return 0;
    }
    
    exit_data *exit;  
    
    return (void*) ( room
		     && ( exit = exit_direction( room, dir ) )
		     && !is_set( exit->exit_info, EX_CLOSED ) );

  } else if( obj_data *obj = object( thing ) ) {
    if( obj->pIndexData->item_type != ITEM_CONTAINER ) {
      code_bug( "Is_Open: object is not a container." );
      return 0;
    }
    
    return (void*) !is_set( obj->value[1], CONT_CLOSED );
  }
  
  code_bug( "Is_Open: bad target." );
  return 0;
}


const void *code2_is_open( const void **argument )
{
  exit_data *exit = (exit_data*) argument[0];

  if( !exit ) {
    code_bug( "Is_Open: null exit." );
    return 0;
  }

  return (void*) !is_set( exit->exit_info, EX_CLOSED );
}


const void *code_is_locked( const void **argument )
{
  thing_data *thing = (thing_data*) argument[0];
  
  if( room_data *room = Room( thing ) ) {
    const int dir = (int) argument[1];
    
    if( dir < 0 || dir >= MAX_DIR ) {
      code_bug( "Is_Locked: bad direction." );
      return 0;
    }
    
    exit_data *exit;  
    
    return (void*) ( room
		     && ( exit = exit_direction( room, dir ) )
		     && is_set( exit->exit_info, EX_LOCKED ) );

  } else if( obj_data *obj = object( thing ) ) {
    if( obj->pIndexData->item_type != ITEM_CONTAINER ) {
      code_bug( "Is_Open: object is not a container." );
      return 0;
    }
    
    return (void*) is_set( obj->value[1], CONT_LOCKED );
  }
  
  code_bug( "Is_Locked: bad target." );
  return 0;
}


const void *code2_is_locked( const void **argument )
{
  exit_data *exit = (exit_data*) argument[0];

  if( !exit ) {
    code_bug( "Is_Locked: null exit." );
  }

  return (void*) is_set( exit->exit_info, EX_LOCKED );
}


const void *code_show( const void **argument )
{
  char_data*    ch  = (char_data*)(thing_data*) argument[0];
  room_data*  room  = (room_data*)(thing_data*) argument[1];
  const int          dir  = (int)        argument[2];

  if( dir < 0 || dir >= MAX_DIR ) {
    code_bug( "Show: bad direction." );
    return 0;
  }

  exit_data *exit;  

  if( room
      && ch
      && ( exit = exit_direction( room, dir ) )
      && is_set( exit->exit_info, EX_SECRET ) )
    ch->seen_exits += exit;
  
  return 0;
}


const void *code2_show( const void **argument )
{
  char_data*    ch  = (char_data*)(thing_data*) argument[0];
  exit_data *exit = (exit_data*) argument[0];

  if( !ch ) {
    code_bug( "Show: null character." );
    return 0;
  }

  if( !exit ) {
    code_bug( "Show: null exit." );
    return 0;
  }

  if( is_set( exit->exit_info, EX_SECRET ) )
    ch->seen_exits += exit;
  
  return 0;
}


/*
 *   DFLAG ROUTINES
 */


const void *code_door( const void **argument )
{
  room_data*  room  = (room_data*)(thing_data*) argument[0];
  const int          dir  = (int)        argument[1];

  if( !room ) {
    code_bug( "Dflag: NULL room." );
    return 0;
  }

  if( dir < 0 || dir >= MAX_DIR ) {
    code_bug( "Dflag: bad direction." );
    return 0;
  }

  return exit_direction( room, dir );  
}


const void *code_dflag( const void **argument )
{
  const int         flag  = (int)        argument[0];
  exit_data*  exit  = (exit_data*) argument[1]; 

  if( !exit ) {
    code_bug( "Dflag: NULL exit." );
    return 0;
  }

  return (void*) is_set( exit->exit_info, flag );
}


const void *code_set_dflag( const void **argument )
{
  const int         flag  = (int)        argument[0];
  exit_data*  exit  = (exit_data*) argument[1]; 

  if( !exit ) {
    code_bug( "Set_Dflag: NULL exit." );
    return 0;
  }

  set_bit( exit->exit_info, flag );

  return 0;
}


const void *code_remove_dflag( const void **argument )
{
  const int         flag  = (int)        argument[0];
  exit_data*  exit  = (exit_data*) argument[1]; 

  if( !exit ) {
    code_bug( "Remove_Dflag: NULL exit." );
    return 0;
  }

  remove_bit( exit->exit_info, flag );

  return 0;
}


const void *code_direction( const void **argument )
{
  exit_data *exit = (exit_data*) argument[0]; 

  if( !exit ) {
    code_bug( "Direction: NULL exit." );
    return 0;
  }

  return (void*) (int)exit->direction;
}


const void *code_leads_to( const void **argument )
{
  exit_data *exit = (exit_data*) argument[0]; 

  if( !exit ) {
    code_bug( "Leads_To: NULL exit." );
    return 0;
  }

  return (void*) exit->to_room;
}


const void *code_reverse( const void **argument )
{
  exit_data *exit = (exit_data*) argument[0]; 

  if( !exit ) {
    code_bug( "Leads_To: NULL exit." );
    return 0;
  }

  return (void*) reverse( exit );
}
