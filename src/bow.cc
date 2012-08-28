#include <sys/types.h>
#include <stdio.h>
#include <syslog.h>
#include "define.h"
#include "struct.h"


void do_shoot( char_data *ch, const char *argument )
{















/*
  char              arg  [ MAX_STRING_LENGTH ];
  char              buf  [ MAX_STRING_LENGTH ];
  char_data*     victim;
  exit_data*       exit;
  obj_data*       arrow;
  room_data*       room  = ch->in_room;
  int                 i;
  int            length;
  int               dex;
  int              roll;
  int            chance;
  int            damage;
  const char*      word;

  obj_data *wield, *secondary, *shield;
  get_wield( ch, wield, secondary, shield );

  if( !wield 
  || wield->value[3] != WEAPON_BOW-WEAPON_UNARMED ) {
  send( ch, "You aren't wielding a ranged weapon.\n\r" );
  return;
    }

  for( int i = 0; i < ch->contents; i++ )  
    if( arrow->pIndexData->item_type == ITEM_ARROW )
      break;

  if( arrow == NULL ) {
    send( "You don't have any ammunition for your weapon.\n\r", ch );
    return;
    }

  argument = one_argument( argument, arg );
  length = strlen( arg );

  if( arg[0] == '\0' ) {
    send( "Which direction do you want to shoot?\n\r", ch );
    return;
    }

  for( i = 0; i < MAX_DOOR; i++ )
    if( !strncasecmp( arg, dir_table[i].name, length ) )
      break;
  
  if( i == MAX_DOOR ) {
    send( "That direction makes no sense!\n\r", ch );
    return;
    }

  if( !( exit = room->exit[i] )
    || is_set( exit->exit_info, EX_CLOSED ) ) {
    send( "That would be shooting into a wall!?.\n\r", ch );
    return;
    }

  if( argument[0] == '\0' ) {
    send( "Who do you want to shoot at?\n\r", ch );
    return;
    }
  
  ch->in_room = room->exit[i]->to_room;
  victim = get_char_room( ch, argument );
  ch->in_room = room;
 
  if( victim == NULL ) {
    send( "You don't see them there ...\n\r", ch );
    return;
    }  

  roll   = number_range( -150, 150 ); 
  dex    = victim->Dexterity( ); 
  chance = -victim->mod_armor+ch->Level()
    +5*get_hitroll( ch, wield );

  if( chance < roll ) {
    sprintf( buf, "$p comes flying in from the %s, barely missing $n.",
      dir_table[i].name );
    act( buf, victim, arrow, NULL, TO_ROOM );
    sprintf( buf, "$p comes flying in from the %s, barely missing you.",
      dir_table[i].name );
    act_to( buf, victim, arrow, NULL, victim );
    sprintf( buf, "$n shoots $p %s barely missing $N.", dir_table[i].name );
    act( buf, ch, arrow, victim, TO_ROOM );
    sprintf( buf, "You shoot $p %s barely missing $N.", dir_table[i].name );
    act_to( buf, ch, arrow, victim, ch );
    return;
    }

  damage  = dice_data( arrow->value[1] ).roll( );
  word    = lookup( physical_index, damage );

  sprintf( buf, "You shoot $p %s.", dir_table[i].name );
  act_to( buf, ch, arrow, NULL, ch );

  sprintf( buf, "$n shoots $p %s.", dir_table[i].name );
  act( buf, ch, arrow, NULL, TO_ROOM );

  sprintf( buf, "$p comes flying in from the %s.", dir_table[i].name );
  act( buf, victim, arrow, NULL, TO_ALL );

  sprintf( buf, "$p %s $n.", word );
  act( buf, victim, arrow, NULL, TO_ROOM );

  room = victim->in_room;
  victim->in_room = ch->in_room;
  act( buf, victim, arrow, NULL, TO_ALL );
  victim->in_room = room;
 
  send( victim, "%s %s you.", arrow, word );

  extract( arrow, 1 );
*/
}

