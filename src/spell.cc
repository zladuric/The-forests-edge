#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include "define.h"
#include "struct.h"


bool spell_amnesia( char_data *ch, char_data *victim, void*, int level, int duration )
{
  if( duration == -4 || duration == -3 )
    return false;

  send( victim, "Your memory seems to have gone blank!\n\r" );

  if( victim->species )
    return false;

  victim->pcdata->practice = 9*total_pracs( victim )/10;

  init_skills( victim );

  /*
  for( int j = 0; j < MAX_SKILL_CAT; ++j ) {
    vzero( victim->shdata->skills[j], table_max[ skill_table_number[ j ] ] );
  }

  victim->pcdata->speaking = LANG_HUMANIC+victim->shdata->race-LANG_FIRST;
  victim->shdata->skills[ SKILL_CAT_LANGUAGE ][ LANG_HUMANIC+victim->shdata->race-LANG_FIRST ] = 10;
  victim->shdata->skills[ SKILL_CAT_LANGUAGE ][ LANG_PRIMAL-LANG_FIRST ] = 10; 
  */

  while( cast_data *cast = victim->prepare ) {
    victim->prepare = cast->next;
    delete cast;
  }
  victim->prepare = 0;

  return true;
}


/*
bool spell_magic_mapping( char_data* ch, char_data* victim, void*, int level, int duration )
{
  if( duration == -4 || duration == -3 )
    return false;

  send( ch, "An image of the surrounding area forms in your mind.\n\r\n\r" );

  show_map( victim, 0, 13+level, 49+level );

  return true;
}
*/


bool spell_youth( char_data* ch, char_data* victim, void* vo, int,
		  int duration )
{
  if( duration == -4 )
    return false;

  obj_data *obj = (obj_data*) vo;

  if( duration == -3 ) {
    if( obj->age > 0 ) {
      fsend( ch, "%s appears less deteriorated by age.", obj );
      fsend_seen( ch, "%s appears less deteriorated by age.", obj );
      obj = (obj_data *) obj->From( 1, true );
      obj->age = max( 0, obj->age-10 );
      obj->To( );
    }
    return true;
  }

  player_data *pc = player( victim );

  if( !pc )
    return false;

  if( pc->Age( ) > plyr_race_table[
    victim->shdata->race ].start_age ) {
    --pc->base_age;
    send( victim, "You feel younger!\n\r" );
    fsend_seen( victim, "%s looks younger!", victim );
  }

  return true;
}
