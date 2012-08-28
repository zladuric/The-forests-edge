#include <math.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include "define.h"
#include "struct.h"


/*
 *   SUNLIGHT
 */


const char* light_name( int i )
{
  const char *const adj [] = { "Pitch Dark", "Extremely Dark", "Dark",
    "Dimly Lit", "Lit", "Well Lit", "Brightly Lit", "Blinding" };
  
  if( i >= 45 ) { i = 7; }
  else if( i >   30 ) { i = 6; }
  else if( i >   20 ) { i = 5; }
  else if( i >    4 ) { i = 4; }
  else if( i >    0 ) { i = 3; }
  else if( i >= -10 ) { i = 2; }
  else if( i >= -20 ) { i = 1; }
  else i = 0;
  
  return adj[i];
}


/*
 *   ILLUMINATION
 */


int thing_data  :: Light( int ) const
{
  return 0;
}


int char_data   :: Light( int ) const
{
  if( species ) {
    if( abs( wearing.light ) > abs( species->light ) ) {
      return wearing.light;
    } else {
      return species->light;
    }
  }
  
  return wearing.light;
}


int Obj_Data :: Light( int n ) const
{
  return light;

  //  int i  = pIndexData->light;  
  //  i = ( ( i > 0 ) ? 1 : ( (i < 0) ? -1 : 0 ) );
  //i *= ( n == -1 ? number : n );
  //  return i; 
}


void Room_Data:: recalc_light ()
{
  int old_light = contents.light;
  int char_light = 0;
  int obj_light = 0;

  for( int i = 0; i < contents; ++i ) {
    if( char_data *ch = character( contents.list[i] ) ) {
      char_light += ch->Light();
    } else if( obj_data *obj = object( contents.list[i] ) ) {
      int ol = obj->Light();
      if( abs( ol ) > abs( obj_light ) ) {
	obj_light = ol;
      }
    }
  }

  contents.light = ( abs( char_light ) > abs( obj_light ) ) ? char_light : obj_light;

  if( old_light <= 0
      && contents.light > 0
      && Light( ) > 0 ) {
    update_aggression( this );
    for( int i = 0; i < exits; ++i ) {
      exit_data *exit = exits.list[i];
      room_data *room = exit->to_room;
      if( room->Light( 1 ) <= 0
	  && room->Light( ) > 0 ) {
	update_aggression( room );
      }
    }
  }
}


// n < 0 : normal calculation, include light from adjacent rooms.
// n == 0 : exclude adjacent rooms (internal calculation).
// n > 0 : exclude sun/moon light and adjacent rooms.
int Room_Data :: Light( int n ) const
{
  int light = contents.light;

  if( size > SIZE_GIANT ) {
    if( light > 0 ) {
      light = ( light - 1 ) / ( size - SIZE_GIANT + 1 ) + 1;
    } else if( light < 0 ) {
      light = ( light + 1 ) / ( size - SIZE_GIANT + 1 ) - 1;
    }
  }

  if( abs( light ) < 10
      && is_set( room_flags, RFLAG_LIT ) ) {
    light = 10;
  }

  if( n <= 0 ) {
    unsigned val = sunlight( );
    if( val > 0 ) {
      const unsigned l = val * terrain_table[ sector_type ].light / 100;
      if( l > unsigned( abs( light ) ) ) {
	light = l;
      }
    }
    val = moonlight( );
    if( val > 0 ) {
      const unsigned l = val * terrain_table[ sector_type ].light / 100;
      if( l > unsigned( abs( light ) ) ) {
	light = l;
      }
    }
  }

  if( n < 0 ) {
    for( int i = 0; i < exits; ++i ) {
      exit_data *exit = exits.list[i];
      room_data *room = exit->to_room;
      int rl = room->Light( 0 ) * ( range( 0, exit->size, 9 ) * 5 + 5 ) / 100;
      if( is_set( exit->exit_info, EX_CLOSED ) ) {
	rl = rl * range( 0, exit->light, 100 ) / 100;
      }
      if( abs( rl ) > abs( light ) ) {
	light = rl;
      }
    }
  }

  return light;
} 


void extinguish_light( char_data* ch )
{
  obj_data *obj;
  thing_array lights, ruined, flames;
  for( int i = 0; i < ch->wearing; ++i ) {
    if( ( obj = object( ch->wearing[i] ) ) ) {
      if( obj->pIndexData->item_type == ITEM_LIGHT
	  || obj->pIndexData->item_type == ITEM_LIGHT_PERM ) {
	if( !is_set( obj->extra_flags, OFLAG_WATER_PROOF ) ) {
	  obj = (obj_data *) obj->From();
	  obj->Show( obj->Number( ) );
	  if ( obj->pIndexData->item_type == ITEM_LIGHT_PERM
	       || obj->value[0] < 0
	       || (obj->value[0] /= 2) != 0 ) {
	    lights += obj;
	  } else {
	    ruined += obj;
	  }
	}
      } else if( strip_affect( obj, AFF_FLAMING ) ) {
	flames += obj;
      }
    }
  }
  
  if ( !flames.is_empty() ) {
    fsend( ch, "As you enter the water, the flames are extinguished around %s you are carrying.",
	  &flames );
    fsend_seen( ch, "As %s enters the water, the flames are extinguished around %s %s is carrying.",
		ch, &flames, ch->He_She() );
  }

  if ( !lights.is_empty() ) {
    fsend( ch, "As you enter the water, %s you are carrying %s extinguished.",
	   &lights,
	   lights > 1 ? "are" : "is" );
    fsend_seen( ch, "As %s enters the water, %s %s is carrying %s extinguished.",
		ch, &lights, ch->He_She(),
		lights > 1 ? "are" : "is" );
    
    for ( int i = 0; i < lights; ++i ) {
      lights[i]->To( ch );
    }
  }
  
  if ( !ruined.is_empty() ) {
    fsend( ch, "%s you are carrying %s ruined by the water.",
	  &ruined,
	  ruined > 1 ? "are" : "is" );
    fsend_seen( ch, "%s %s is carrying %s ruined by the water.",
		&ruined, ch,
		ruined > 1 ? "are" : "is" );
    
    for ( int i = 0; i < ruined; ++i ) {
      ruined[i]->Extract();
    }
  }
}
  

/*
 *   SPELLS
 */


bool spell_create_light( char_data* ch, char_data*, void*,
			 int level, int )
{
  if( null_caster( ch, SPELL_CREATE_LIGHT ) )
    return false;
  
  obj_data *light = create( get_obj_index( OBJ_BALL_OF_LIGHT ) );

  //  if( ch->pcdata ) {
  //    set_owner( light, ch->pcdata->pfile );
  //  }

  // Light ball color depends on skill level.
  light->value[0] = (level - 1) * 3 + number_range(1, 5);	// Lifetime and color.
  light->light = light->value[0]/6 + 1;

  set_bit( light->extra_flags, OFLAG_NO_AUCTION );
  set_bit( light->extra_flags, OFLAG_NO_SELL );
  set_bit( light->extra_flags, OFLAG_NOSACRIFICE );

  if( ch->can_carry_n( ) < ch->contents.number + 1
      || ch->Capacity( ) < light->Weight( ) ) {
    if( ch->in_room ) {
      light->To( ch->in_room );
      const char *drop = ch->in_room->drop( );
      if( *drop ) {
	fsend( ch, "%s appears nearby, and falls %s.", light, drop );
	fsend_seen( ch, "%s appears near %s, and falls %s.", light, ch, drop );
      } else {
	fsend( ch, "%s appears here.", light );
	fsend_seen( ch, "%s appears here.", light );
      }
    }
  } else {
    light->To( ch );    
    fsend( ch, "%s appears in your hand.", light );
    fsend( *ch->array, "%s appears in %s's hand.", light, ch );
  }

  // Slow down improves.
  return number_range( 0, 1 ) == 1;
}

/*
bool spell_continual_light( char_data *ch, char_data *victim, void*,
			    int level, int duration )
{
  return spell_affect( ch, victim, level, duration,
		       SPELL_CONTINUAL_LIGHT, AFF_CONTINUAL_LIGHT );
}
*/
