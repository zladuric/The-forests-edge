#include <ctype.h>
#include <sys/types.h>
#include <stdio.h>
#include <syslog.h>
#include "define.h"
#include "struct.h"


/*
 *   ACID DAMAGE ROUTINES
 */


const index_data acid_index [] = 
{
  { "has no effect on",     "have no effect on",     0 },
  { "blemishes",            "blemish",               3 },
  { "irritates",            "irritate",              7 },
  { "burns",                "burn",                 15 },
  { "erodes",               "erode",                30 },
  { "blisters",             "blister",              45 },
  { "MARS",                 "MAR",                  60 },
  { "WITHERS",              "WITHER",               90 },
  { "* CORRODES *",         "* CORRODE *",         120 },
  { "* SCARS *",            "* SCAR *",            150 }, 
  { "* DISFIGURES *",       "* DISFIGURE *",       190 }, 
  { "** MELTS **",          "** MELT **",          240 },
  { "** LIQUIFIES **",      "** LIQUIFY **",       290 },
  { "*** DISSOLVES ***",    "*** DISSOLVE ***",    350 },
  { "*** DELIQUESCES ***",  "*** DELIQUESCE ***",   -1 }
};


bool damage_acid( char_data* victim, char_data* ch, int damage,
		  const char* string, bool plural,
		  const char *die )
{
  if( victim->is_affected( AFF_SANCTUARY ) )
    damage = 0;
  else
    add_percent_average( damage, -victim->Save_Acid( ) );

  dam_message( victim, ch, damage, string,
	       lookup( acid_index, damage, plural ) );
  
  return inflict( victim, ch, damage, die );
}


int Obj_Data :: vs_acid( )
{
  int save = 100;

  for( int i = 0; i < table_max[TABLE_MATERIAL]; i++ ) 
    if( is_set( pIndexData->materials, i ) )
      save = min( save, material_table[i].save_acid );

  if( pIndexData->item_type != ITEM_ARMOR 
      || pIndexData->item_type != ITEM_WEAPON ) 
    return save;
  
  return save+value[0]*(100-save)/(value[0]+2);
}


/* 
 *   ACID BASED SPELLS
 */


/*
bool spell_resist_acid( char_data* ch, char_data* victim, void*,
			int level, int duration )
{
  return spell_affect( ch, victim, level, duration,
		       SPELL_RESIST_ACID, AFF_RESIST_ACID );
}
*/


bool spell_acid_blast( char_data* ch, char_data* victim, void* vo,
		       int level, int duration )
{
  obj_data*  obj  = (obj_data*) vo;

  /* Drink */
  if( duration == -1 ) {
    fsend( victim, "You feel incredible pain as the acid eats away at your\
 stomach and throat.  Luckily you don't feel it for long." );
    fsend_seen( victim, "%s grasps %s throat and spasms in pain - %s does\
 not survive long.", victim, victim->His_Her( ),
      victim->He_She( ) );
    death_message( victim );
    death( victim, 0, "drinking acid" );
    return true;
  }  

  /* Fill */
  if( duration == -4 ) {
    if( is_set( obj->materials, MAT_STONE ) 
	|| is_set( obj->materials, MAT_GLASS ) )
      return false;
    
    fsend( victim, "The acid bubbles and boils, eating its way through %s,\
 which you quickly drop and watch disappear into nothing.", obj );
    fsend_seen( victim, "%s quickly drops %s\
 as %s dissolved by the liquid.", victim, obj, 
		obj->Selected( ) > 1 ? "they are" : "it is" );
    
    obj->Extract( obj->Selected( ) );
    return true;
  }
  
  /* Dip */
  if( duration == -3 ) {
    const int save = obj->vs_acid( );
    if( save == 0 || number_range( 1,100 ) > save ) {
      int points = number_range( 50, 500 );
      if( is_set( obj->extra_flags, OFLAG_SANCT ) ) {
	points -= 25;
      }
      if( points >= obj->condition
	  || save == 0 || number_range( 1,100 ) > save ) {
        fsend( *ch->array, "%s is devoured.", obj );
        obj->Extract( 1 );
        return true;
      }
      fsend( ch, "%s is partially destroyed.", obj );
      obj = (obj_data *) obj->From( 1, true );
      obj->condition -= points;
      obj->To( );
      return true;
    }
    int metal = obj->metal( );
    if( metal != 0 && obj->rust > 0 ) {
      fsend( ch, "%sthe %s on %s is removed.", 
	     obj->rust > 1 ? "Some of " : "",
	     material_table[ metal ].rust_name,
	     obj->pIndexData->Name( 1, false, is_set( obj->extra_flags, OFLAG_IDENTIFIED ) ) );
      fsend_seen( ch, "%sthe %s on %s is removed.", 
		  obj->rust > 1 ? "Some of " : "",
		  material_table[ metal ].rust_name,
		  obj->pIndexData->Name( 1, false, is_set( obj->extra_flags, OFLAG_IDENTIFIED ) ) );
      obj = (obj_data *)obj->From( obj->Selected( ), true );
      --obj->rust;
      obj->To( );
    }
    return true;
  }
  
  /* Throw-Cast */
  
  damage_acid( victim, ch, spell_damage( SPELL_ACID_BLAST, level ),
	       "*the splatter of acid" );
  
  return true;
}


/*
bool spell_acid_storm( char_data* ch, char_data* victim, void*,
		       int level, int )
{
  damage_acid( victim, ch, spell_damage( SPELL_ACID_STORM, level ),
	       "*the blast of acid" );

  return true;
}
*/
