#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "define.h"
#include "struct.h"


  /*
void do_probe( char_data *ch, const char *argument )
{
  char buf [ MAX_STRING_LENGTH ];
  char_data *victim;

  if( ch->pcdata == NULL || ch->shdata->skill[SKILL_PROBE] == UNLEARNT ) {
    send( "Only psionics can probe the minds of others.\n\r", ch );
    return;
    }

  if( ( victim = get_char_room( ch, argument ) ) == NULL ) {
    send( "They aren't here.\n\r", ch );
    return;
    }

  sprintf( buf, "Hits: %d/%d\n\r",
    victim->hit, victim->max_hit );
  sprintf( buf+strlen( buf ), "Energy: %d/%d\n\r",
    victim->mana, victim->max_mana );
  sprintf( buf+strlen( buf ), "Exp Value: %d\n\r\n\r",
    xp_compute( victim ) );
  send( buf, ch );

  sprintf( buf, "Alignment: %s\n\r",
    alignment_name[victim->shdata->alignment] );
  send( buf, ch );
}
  */ 


  /*
void do_sweep( char_data* ch, const char * )
{
  char buf [ MAX_STRING_LENGTH ];
  char tmp [ MAX_STRING_LENGTH ];
  char_data *rch;
  room_data *room;
  int number[ MAX_SPECIES ];
  bool found = false;
  int i, col = 0;

  if( !ch->pcdata || ch->shdata->skill[SKILL_SWEEP] == UNLEARNT ) {
    send( "Only psionics can sweep an area.\n\r", ch );
    return;
    }

  for( i = 1; i <= species_max; ++i )
    number[i] = 0;

  for( room = ch->in_room->area->room_first; room;
    room = room->next ) {
    for( rch = room->people; rch; rch = rch->next_in_room ) {
    if( rch == ch || !rch->Seen( ch ) )
        continue;
      if( !found ) {
        send_title( ch, "Creatures in Area" );
        found = true;
        }
      if( rch->species ) {
        number[rch->species->vnum]++;    
        }
      else {
        sprintf( buf, "%28s%s", rch->descr->name,
          ++col%2 == 0 ? "\n\r" : "" );
        send( buf, ch );
        }
      }
    }

  for( i = 1; i <= species_max; ++i ) {
    if( number[i] == 0 || species_list[i]->shdata->intelligence < 5 )
      continue;

    if( number[i] == 1 ) 
      sprintf( buf, "%s", species_list[i]->descr->name );
    else  
      sprintf( buf, "%s (x%d)",
        species_list[i]->descr->name, number[i] );

    sprintf( tmp, "%28s%s", buf, ++col%2 == 0 ? "\n\r" : ""  );
    send( tmp, ch );
    }

  if( col%2 == 1 )
    send( "\n\r", ch );

  if( !found ) 
    send( "You detect nothing in the area!?\n\r", ch );
}
  */


/*
 *   SPELLS
 */


bool spell_mind_blade( char_data* ch, char_data* victim, void*,
  int level, int )
{
  if( !ch ) {
    bug( "Mind_Blade: NULL caster." );
    return true;
  }
 
  int dam = ch->Level() > 20 ? 20 : ch->Level();

  damage_mind( victim, ch, roll_dice( 2+level/2, dam ),
	       "*The mind blade" );

  return true;
}
