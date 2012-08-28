#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include "define.h"
#include "struct.h"


/*
 *   GIVE FUNCTIONS
 */


static bool give_pet( char_data* ch, char_data* victim, obj_data* obj )
{
  fsend( ch, "You offer %s to %s.", obj, victim );
  fsend_seen( ch, "%s offers %s to %s.", ch, obj, victim );
  
  if( obj->pIndexData->item_type == ITEM_FOOD 
      && eat( victim, obj ) ) {
    return true;
  }
  
  if( obj->pIndexData->item_type == ITEM_DRINK_CON
      && would_drink( victim, obj )
      && drink( victim, obj ) ) {
    return true;
  }
  
  fsend( ch, "%s inspects it but doesn't seem interested.",
	 victim->He_She( ch ) );
  fsend_seen( victim, "%s inspects it but doesn't seem interested.",
	      victim->He_She( ), ch );	// Unused ch on purpose.

  return true;
}


static bool give_npc( char_data* ch, char_data* victim, thing_array* array )
{
  mprog_data*  mprog  = 0;
  obj_data*      obj;

  if( victim->pcdata )
    return false;

  if( array->size != 1 || array->list[0]->Selected( ) != 1 ) {
    fsend( ch,
	   "%s would be much happier if items were offered one at a time.",
	   victim );
    return true;
  }
  
  if( ( obj = object( array->list[0] ) ) ) {
    if( is_set( victim->status, STAT_PET ) ) 
      return give_pet( ch, victim, obj );
    
    for( mprog = victim->species->mprog; mprog; mprog = mprog->next )
      if( mprog->trigger == MPROG_TRIGGER_GIVE
	  && ( mprog->value == obj->pIndexData->vnum || mprog->value == 0 ) )
        break;
  }

  if( !mprog ) {
    fsend( ch, "%s doesn't seem interested in %s.", victim, array->list[0] );
    return true;
  } 
  
  fsend( ch, "You give %s to %s.", obj, victim );
  fsend_seen( ch, "%s gives %s to %s.", ch, obj, victim );
  
  obj = (obj_data*) obj->From( obj->Selected( ) );
  set_owner( obj, 0, ch );
  obj->To( victim );

  //push( );
  clear_variables( );
  var_ch   = ch;
  var_mob  = victim;  
  var_obj  = obj; 
  var_room = ch->in_room;
  mprog->execute( );
  //pop( );

  return true;
}


thing_data* given( thing_data* obj, char_data* receiver, thing_data* giver )
{
  obj = obj->From( obj->Selected( ) );
  if( !is_set( receiver->status, STAT_PET ) ) {
    // Prevent set_owner to 0, then back to leader on obj_data::To().
    set_owner( (obj_data*) obj, receiver, (char_data*) giver );
  }
  obj->To( receiver );
  return obj;
}


void do_give( char_data* ch, const char *argument )
{
  char arg [ MAX_INPUT_LENGTH ];
  char_data*   victim;
  thing_array*  array;
  
  if( is_confused_pet( ch ) || newbie_abuse( ch ) ) 
    return;

  if( !two_argument( argument, "to", arg ) ) {
    send( ch, "Give what to whom?\n\r" );
    return;
  }
  
  if( !( victim = one_character( ch, argument, "give to",
				 ch->array ) ) )
    return;
  
  if( !( array = several_things( ch, arg, "give", &ch->contents ) ) )
    return;
  
  if( victim == ch ) {
    send( ch, "You can't give things to yourself.\n\r" );
    delete array;
    return;
  }
  
  if( victim->position <= POS_SLEEPING ) {
    fsend( ch, "%s isn't in a position to be handed items.", victim );
    delete array;
    return;
  }

  if( ( !is_set( victim->status, STAT_PET )
	|| victim->leader != ch
	|| !victim->can_carry( ) )
      && give_npc( ch, victim, array ) ) {
    delete array;
    return;
  }

  if( victim->pcdata
      && ( ch->pcdata && ch->pcdata != victim->pcdata
	   || is_set( ch->status, STAT_PET ) && ch->leader->pcdata && ch->leader->pcdata != victim->pcdata )
      && get_trust( ch ) < LEVEL_APPRENTICE
      && is_set( victim->pcdata->pfile->flags, PLR_NO_GIVE ) ) {
    fsend( ch, "%s politely refuses your gift.", victim );
    fsend( victim, "%s tries to give you something, but you politely refuse.", ch );
    fsend_seen( ch, "%s tries to give something to %s, who politely refuses.",
		ch, victim );
    return;
  }

  thing_array   subset  [ 4 ];
  thing_func*     func  [ 4 ]  = { cursed, heavy, many, given };

  sort_objects( victim, *array, ch, 4, subset, func );

  page_priv( ch, 0, empty_string );
  page_priv( ch, &subset[0], "can't let go of" );
  page_priv( ch, &subset[1], "can't lift", victim );
  page_priv( ch, &subset[2], "can't handle", victim );
  page_publ( ch, &subset[3], "give", victim, "to" );

  delete array;
}
