#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include "define.h"
#include "struct.h"


#define EXTRACT_BLOOD     0
#define MAX_EXTRACTION    1


const char* extract_name [] = { "blood" };


void extract_corpse( obj_data *corpse )
{
  if( !corpse->contents.is_empty() ) {
    for( int i = 0; i < *corpse->array; ++i ) {
      char_data *rch;
      if( !( rch = character( corpse->array->list[i] ) )
	  || !rch->link
	  || !corpse->Seen( rch ) )
	continue;
      
      select( corpse->contents, rch );
      rehash( rch, corpse->contents );
      
      if( !none_shown( corpse->contents ) ) {
	const char *drop = rch->in_room->drop( );
	if( thing_data *thing = one_shown( corpse->contents ) ) {
	  if( *drop ) {
	    fsend( rch, "%s fall%s %s.",
		   thing,
		   thing->Shown( ) == 1 ? "s" : "",
		   drop );
	  } else {
	    fsend( rch, "%s fall%s out.",
		   thing,
		   thing->Shown( ) == 1 ? "s" : "" );
	  }
	} else {
	  if( *drop ) {
	    fsend( rch, "Some items fall %s.", drop );
	  } else {
	    fsend( rch, "Some items fall out." );
	  }
	}
      }
    }
    
    if( corpse->owner
	&& ( corpse->pIndexData->vnum == OBJ_CORPSE_PC
	     || corpse->pIndexData->vnum == OBJ_CORPSE_PET ) ) {
      set_owner( corpse->contents, 0, corpse->owner );
    }

    corpse->contents.To( *corpse->array );
  }
  
  corpse->Extract( 1 );
}


/*
void return_corpses( char_data *ch )
{
  obj_array corpses;

  for( int i = 0; i < obj_list; ++i ) {
    obj_data *obj = obj_list[i];
    pfile_data *pfile;
    if( obj->Is_Valid( )
	&& ( pfile = obj->owner )
	&& ( obj->pIndexData->vnum == OBJ_CORPSE_PC
	     || obj->pIndexData->vnum == OBJ_CORPSE_PET )
	&& Room( obj->array->where )
	&& !obj->contents.is_empty( ) ) {
      corpses += obj;
    }
  }

  for( int i = 0; i < corpses; ++i ) {
    obj_data *obj = corpses[i];
    pfile_data *pfile = obj->owner;
    send( ch, "-- Returned items from %s's corpse --\n\r", pfile->name );
    select( obj->contents );
    if( player_data *pl = find_player( pfile ) ) {
      send( pl, "-- Items from your corpse have been retrieved --\n\r" );
      transfer_objects( pl, pl->locker, 0, obj->contents );
    } else {
      transfer_file( pfile, &player_data::locker, &obj->contents );
    }
  }
}
*/


void do_extract( char_data* ch, const char *argument )
{
  char             arg  [ MAX_INPUT_LENGTH ];
  int                i;

  argument = one_argument( argument, arg );

  for( i = 0; i < MAX_EXTRACTION; i++ )
    if( !strcasecmp( arg, extract_name[i] ) ) 
      break;

  if( i == MAX_EXTRACTION ) {
    send( ch, "That isn't something you can extract from anything.\n\r" );
    return;
  }
  
  if( !*argument ) {
    send( ch, "From what do you wish to extract %s from?\n\r",
	  extract_name[i] );
    return;
  }
  /*
  obj_data*        obj;
  obj_data*  container;
  if( !( obj = one_object( ch, argument, ch ) ) ) {
  send( ch, "The object you wish to extract %s from is not here.\n\r",
  extract_name[i] );
  return;
  }

  if( i == EXTRACT_BLOOD ) {
    if( obj->pIndexData->item_type != ITEM_CORPSE_NPC ) {
      send( "You can only extract blood from corpses.\n\r", ch );
      return;
      }
    if( ( container = get_eq_char( ch, WEAR_HOLD ) ) == NULL
      || container->pIndexData->item_type != ITEM_DRINK_CON ) {
      send( "You need to be holding a liquid container to extract\
 blood.\n\r", ch );
      return;
      }
    fsend( ch,
      "You extract into %s as much blood as possible from %s.\n\r",
      container, obj );
    }
  */
}
