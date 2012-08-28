#include <sys/types.h>
#include <stdio.h>
#include "define.h"
#include "struct.h"


bool is_mounted( char_data* ch, const char *msg )
{
  if( !ch->mount )
    return false;

  cant_message( ch, msg, "mounted" );

  return true;
}


bool is_ridden( char_data* ch, const char *msg )
{
  if( !ch->rider )
    return false;

  cant_message( ch, msg, "being ridden" );
  
  return true;
}


bool dismount( char_data* ch, int pos )
{
  if( !ch || !ch->mount )
    return false;

  ch->mount->rider = 0;
  ch->mount = 0;

  if( pos == POS_RESTING
      && ch->in_room
      && deep_water( ch ) ) {
    pos = POS_STANDING;
  }

  ch->position = pos;
  disrupt_spell( ch );

  return true;
}


bool mount( char_data *ch, char_data *victim )
{
  if( victim == ch ) {
    send( ch, "You can't ride yourself.\n\r" );
    return false;
  }

  if( ch->get_skill( SKILL_RIDING ) == UNLEARNT ) {
    send( ch, "You don't know how to ride.\n\r" );
    return false;
  }
  
  if( is_entangled( ch, "mount" ) ) {
    return false;
  }

  if( player( victim ) ) {
    send( ch, "You can't ride players.\n\r" );
    return false;
  }

  if( ch->mount ) {
    fsend( ch, "You are already riding %s.", ch->mount );
    return false;
  }
  
  if( is_set( ch->in_room->room_flags, RFLAG_NO_MOUNT ) ) {
    send( ch, "You can't mount here.\n\r" );
    return false;
  }
  
  if( victim->rider ) {
    send( ch, "Someone else is already riding them.\n\r" );
    return false;
  } 
  
  if( !is_set( victim->status, STAT_PET )
      || !is_set( victim->species->act_flags, ACT_MOUNT ) ) {
    fsend( ch, "%s refuses to let you mount %s.",
	   victim, victim->Him_Her( ) );
    return false;
  }
  
  if( victim->leader != ch
      && !victim->leader->Befriended( ch ) ) {
    fsend( ch, "%s refuses to let you mount %s.",
	   victim, victim->Him_Her( ) );
    return false;
  }

  if( victim->fighting ) {
    send( ch, "It is impossible to mount a fighting beast.\n\r" );
    return false;
  }
  
  if( victim->Size( ) <= ch->Size( ) ) {
    fsend( ch, "%s is too small to carry your weight.", victim );
    return false;
  }
  
  if( victim->position != POS_STANDING ) {
    send( ch, "You can only mount a beast while it is standing.\n\r" );
    return false;
  }

  //  strip_affect( ch, AFF_INVISIBLE );
  leave_shadows( ch );
  remove_bit( ch->status, STAT_SNEAKING );
  remove_bit( ch->status, STAT_COVER_TRACKS );

  //  strip_affect( victim, AFF_INVISIBLE );
  leave_shadows( victim );
  remove_bit( victim->status, STAT_SNEAKING );
  remove_bit( victim->status, STAT_COVER_TRACKS );

  if( !ch->species ) {
    remove_bit( ch->pcdata->pfile->flags, PLR_TRACK );
    remove_bit( ch->pcdata->pfile->flags, PLR_SEARCHING );
  }

  fsend( ch, "You mount %s.", victim );
  fsend_seen( ch, "%s mounts %s.", ch, victim );

  ch->mount = victim;
  victim->rider = ch;

  ch->improve_skill( SKILL_RIDING );

  return true;
}


void do_mount( char_data* ch, const char *argument )
{
  char_data *victim;
  
  if( !*argument
      && ( victim = has_mount( ch, false ) )
      && victim->array == ch->array
      || ( victim = one_character( ch, argument, "mount",
				   ch->array ) ) ) {
    victim->Select( 1 );
    mount( ch, victim );
  }
}


void do_dismount( char_data *ch, const char * )
{
  if( !ch->mount ) {
    send( ch, "You aren't mounted.\n\r" );
    return;
  }  

  if( is_entangled( ch, "dismount" ) ) {
    return;
  }

  ch->mount->Show( 1 );

  fsend( ch, "You dismount from %s.", ch->mount );
  fsend_seen( ch, "%s dismounts from %s.", ch, ch->mount );

  dismount( ch );
}


void check_mount( char_data* ch )
{
  if( ch->rider )
    ch = ch->rider;

  if( !ch->mount )
    return;
  
  if( number_range( -100, 9 ) < ch->get_skill( SKILL_RIDING ) )
    return;
  
  ch->mount->Show( 1 );
  
  send( ch, "You are thrown from your mount.\n\r" );
  fsend( ch->mount, "%s is thrown from your back.", ch );
  fsend_seen( ch, "%s is thrown from %s which %s was riding.",
	      ch, ch->mount, ch->He_She( ) );
  
  dismount( ch, POS_RESTING );

  if( !ch->is_affected( AFF_SANCTUARY ) ) {
    const int dam = number_range( 1, 4 );
    dam_message( ch, 0, dam, "falling off a mount", lookup( physical_index, dam ) );
    inflict( ch, 0, dam, "falling off a mount" );
  }
}
