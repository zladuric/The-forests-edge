#include <sys/types.h>
#include <stdio.h>
#include "define.h"
#include "struct.h"


void spoil_hide( char_data* ch )
{
  if( !is_set( ch->status, STAT_HIDING ) )
    return;

  for( int i = 0; i < ch->array->size; i++ ) {
    char_data* rch; 
    if( ( rch = character( ch->array->list[i] ) )
	&& rch != ch
	&& !ch->seen_by.includes( rch ) ) {
      if( !ch->Seen( rch ) ) {
	remove_bit( ch->status, STAT_HIDING );
	if( ch->Seen( rch ) ) {
	  send( rch, "++ You notice %s hiding in the shadows! ++\n\r", ch );
	  ch->seen_by += rch;
	}
	set_bit( ch->status, STAT_HIDING );
      } else {
	ch->seen_by += rch;
      }
    }
  }
}
