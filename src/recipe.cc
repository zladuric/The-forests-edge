
#include <ctype.h>
#include <sys/types.h>
#include <stdio.h>
#include <syslog.h>
#include "define.h"
#include "struct.h"


void do_build( char_data* ch, const char *argument ) 
{
  int                  i, j;
  int                 skill;
  obj_clss_data*     result = 0;  // For compiler warning.
  
  if( *argument == '\0' ) {
    send( ch, "What do you wish to build?\n\r" );
    return;
  }
  
  for( i = 0; i < table_max[ TABLE_BUILD ]; i++ ) 
    if( ( result = get_obj_index( build_table[i].result[0] ) )
	&& is_name( argument, result->Keywords( ) ) )
      break;

  if( i == table_max[ TABLE_BUILD ] ) {
    send( ch, "Whatever that is it isn't something you can build.\n\r" );
    return;
  }

  recipe_data *recipe = &build_table[i];
 
  for( j = 0; j < 3; j++ )  
    if( ( skill = recipe->skill[j] ) >= 0 
	&& ch->get_skill( skill ) == UNLEARNT ) {
      fsend( ch, "To build %s you need to know %s.",       
	     result->Name( ), skill_entry( skill )->name );
      return;
    }

  /*
  obj_data*             obj;
  obj_clss_data*     ingred;
  for( i = 0; i < 10; i++ ) {
    ingred = get_obj_index( recipe->ingredient[2*i] );
    if( ingred == NULL )
      continue;
    for( j = 0, obj = ch->contents; obj; obj = obj->next_content ) 
      if( obj->pIndexData == ingred && obj->wear_loc == -1 )
        j += obj->number;
    if( j == 0 ) {
      send( ch, "Building %s requires %s.\n\r", result->Name( ),
        ingred->Name( ) ); 
      return;
      }
    }
  */
}
