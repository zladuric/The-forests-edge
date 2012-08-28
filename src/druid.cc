#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "define.h"
#include "struct.h"


bool spell_polymorph( char_data*, char_data*, void*, int, int )
{
  return false;
}


void do_polymorph( char_data*, const char * )
{
  /*
  char_data*        mob;
  obj_data*         obj;
  player_data*   player;

  return;
  if( !ch->link )
    return;

  if( ch->link->original ) {
    send_to_char( "You can only polymorph in human form.\n\r", ch );
    return;
    }

  send_to_char( "You transform into a tiger.\n\r", ch );

  mob = new Mob_Data( get_species( 690 ) );
  char_to_room( mob, ch->in_room );

  act( "The air around $n fades into a strange array of swirling colors.",
    ch, 0, 0, TO_ROOM );
  act( "$e is enveloped by the colors and when they recede you see $N.",
    ch, 0, mob, TO_ROOM );
  if( ch->contents ) {
    send_to_char( "You equipment clatters to the floor.\n\r", ch );
    while( ch->contents ) {
      obj = remove( ch->contents, ch->contents->number );
      put_obj( obj, ch->in_room );
      }
    act( "$n's equipment lies scattered %s.",
      ch, 0, 0, TO_ROOM, ***room position );
    } 
 
  char_from_room( ch ); 

  ch->link->character = mob;
  ch->link->original  = player;
  mob->link           = ch->link;
  ch->link            = 0;
  mob->pcdata         = ch->pcdata;
  mob->timer          = 0;

  set_bit( ch->status, STAT_POLYMORPH );
  */
}


void monk_return( char_data* )
{
  /*
  remove_bit( ch->link->original->status, STAT_POLYMORPH );

  send_to_char( "You transform back to human form.\n\r", ch );

  ch->link->character       = ch->link->original;
  ch->link->original        = 0;
  ch->link->character->link = ch->link; 

  char_to_room( ch->link->character, ch->in_room );
  ch->link                  = 0;
  ch->pcdata                = 0;
  ch->Extract( );
  */
}
