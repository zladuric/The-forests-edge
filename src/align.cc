#include <ctype.h>
#include <sys/types.h>
#include <stdio.h>
#include "define.h"
#include "struct.h"


flag_data align_flags = { "Alignments",
			  &alignment_table[0].name,
			  &alignment_table[1].name,
			  &table_max[ TABLE_ALIGNMENT ], false };

flag_data abv_align_flags = { "Alignments",
			      &alignment_table[0].abbrev,
			      &alignment_table[1].abbrev,
			      &table_max[ TABLE_ALIGNMENT ], false };

/*
void do_alignment( char_data* ch, const char *argument )
{
  int i;

  if( is_mob( ch ) )
    return;
 
  i = ch->shdata->alignment;

  class type_field type = 
    { "alignment", MAX_ALIGNMENT, &alignment_name[0], &alignment_name[1],
      &i, true };

  type.set( ch, "you", argument );

  if( !is_set( &clss_table[ ch->pcdata->clss ].alignments, i ) ) {
    send( ch, "Your class does not allow that alignment.\n\r" );
    return;
    }

  if( ch->shdata->race < MAX_PLYR_RACE 
    && !is_set( &plyr_race_table[ ch->shdata->race ].alignments, i ) ) {
    send( ch, "Your race does not allow that alignment.\n\r" );
    return;
    }

  ch->shdata->alignment = i;

  return;
}
*/
