#include <ctype.h>
#include <sys/types.h>
#include <stdio.h>
#include "define.h"
#include "struct.h"


const char *const skill_level_name[] = {
  "unlearnt",		// 0
  "novice",		// 1
  "amateur",		// 2
  "initiate",		// 3
  "practiced",		// 4
  "competent",		// 5
  "proficient",		// 6
  "adept",		// 7
  "expert",		// 8
  "master",		// 9
  "grand master"	// 10
};


// Find the shortest skill name that completely matches.
int find_skill( const char *name, int cat )
{
  int n = -1;

  if( int length = strlen( name ) ) {
    int x = 0;
    int t = ( cat < 0 ) ? 0 : cat;
    for( ; t < MAX_SKILL_CAT; ++t ) {
      const int m = table_max[ skill_table_number[ t ] ];
      for( int e = 0; e < m; ++e ) {
	if( !strncasecmp( name, skill_entry( t, e )->name, length ) ) {
	  int y = strlen( skill_entry( t, e )->name );
	  if( n < 0 || y < x ) {
	    n = skill_ident( t, e );
	    x = y;
	  }
	}
      }
      if( cat >= 0 )
	break;
    }
  }

  return n;
}


// Find exact name match.
int skill_index( const char* name, int cat )
{
  if( *name ) {
    int j = ( cat < 0 ) ? 0 : cat;
    for( ; j < MAX_SKILL_CAT; ++j ) {
      const int m = table_max[ skill_table_number[ j ] ];
      for( int i = 0; i < m; ++i ) {
	if( !strcasecmp( name, skill_entry( j, i )->name ) )
	  return skill_ident( j, i );
      }
      if( cat >= 0 )
	break;
    }
  }
  
  return -1;
}


void display_skills( char_data *ch, int cat )
{
  int j = ( cat < 0 ) ? 0 : cat;
  for( ; j < MAX_SKILL_CAT; ++j ) {
    display_array( ch, skill_cat_name[j],
		   &skill_entry( j, 0 )->name, &skill_entry( j, 1 )->name,
		   table_max[ skill_table_number[ j ] ] );
    if( cat >= 0 )
      break;
    if( j != MAX_SKILL_CAT-1 ) {
      page( ch, "\n\r" );
    }
  }
}


void init_skills( char_data *ch )
{
  for( int j = 0; j < MAX_SKILL_CAT; ++j ) {
    vzero( ch->shdata->skills[j], table_max[ skill_table_number[ j ] ] );
  }

  ch->shdata->skills[ SKILL_CAT_LANGUAGE ][ skill_number( LANG_PRIMAL ) ] = 10;

  const int race = ch->shdata->race;

  if( race >= 0 && race < table_max[ TABLE_PLYR_RACE ] ) {
    for( int i = 0; i < table_max[ TABLE_SKILL_LANGUAGE ]; ++i ) {
      if( is_set( plyr_race_table[ race ].language, i ) ) {
	ch->shdata->skills[ SKILL_CAT_LANGUAGE ][ i ] = 10;
	ch->pcdata->speaking = i;
      }
    }

    //    ch->shdata->skills[ SKILL_CAT_LANGUAGE ][ skill_number( LANG_HUMANIC+race ) ] = 10;
    //    ch->pcdata->speaking = skill_number( LANG_HUMANIC+race );
  }
}


int char_data :: get_skill( int i ) const
{
  if( i < 0 )
    return UNLEARNT;

  if( species ) {
    if( i >= SPELL_FIRST && i < SPELL_FIRST+table_max[ TABLE_SKILL_SPELL ] ) {
      return 8;
    }
    switch( i ) {
      // These mskills are implemented so far:
    case SKILL_RIPOSTE:
    case SKILL_CLIMB:
    case SKILL_SWIMMING:
    case SKILL_SHIELD_BLOCK:
    case SKILL_SHIELD_STRIKE:
    case SKILL_TUMBLE:
    case SKILL_DODGE:
    case SKILL_BLIND_FIGHTING:
    case SKILL_PARRY:
    case SKILL_GUARD:
    case SKILL_SHADOW_DANCE:
    case SKILL_CHARGE:
    case SKILL_STUN:
    case SKILL_POWER_STRIKE:
    case SKILL_CRITICAL_HIT:
    case SKILL_DEATH_STRIKE:
    case SKILL_BACKSTAB:
    case SKILL_ASSASSINATE:
    case SKILL_HIDE:
    case SKILL_SNEAK:
    case SKILL_CAMOUFLAGE:
    case SKILL_TRIP:
    case SKILL_INSPECT:
    case SKILL_UNTRAP:
    case SKILL_APPRAISE:
    case SKILL_BANDAGE:
    case SKILL_RIDING:
    case SKILL_MEDITATE:
    case SKILL_TRANCE:
    case SKILL_BERSERK:
    case SKILL_FOCUS:
    case SKILL_FRENZY:
    case SKILL_EYE_GOUGE:
    case SKILL_GARROTE:
    case SKILL_OFFHAND_PARRY:
    case SKILL_PICK_LOCK:
    case SKILL_MELEE:
    case SKILL_RESCUE:
    case SKILL_MOUNTED_FIGHTING:
    case SKILL_ESCAPE:
    case WEAPON_UNARMED:
    case WEAPON_DAGGER:
    case WEAPON_SWORD:
    case WEAPON_CLUB:
    case WEAPON_STAFF:
    case WEAPON_POLEARM:
    case WEAPON_MACE:
    case WEAPON_WHIP:
    case WEAPON_AXE:
    case WEAPON_BOW:
    case WEAPON_SPEAR:
      return (int)shdata->skills[ skill_table( i ) ][ skill_number( i ) ];
    default:
      return 10;
    }
  }

  const int level = Level();

  if( level < LEVEL_APPRENTICE ) {
    const Skill_Type *skill = skill_entry(i);

    const int class_level = skill->level[pcdata->clss];
    
    if( class_level < 0 || class_level > level ) {
      return UNLEARNT;
    }

    if( !skill->religion( pcdata->religion ) ) {
      return UNLEARNT;
    }
  }
  
  return (int)shdata->skills[ skill_table( i ) ][ skill_number( i ) ];
}
   
   
bool char_data :: check_skill( int i, int base )
{
  // Base success rate.
  if( base > 0 ) {
    if ( number_range( 1, 100 ) <= base ) {
      return true;
    }
  }

  const int level = get_skill( i );

  if( level == UNLEARNT )
    return false;

  // Base failure rate.
  if( base < 0 ) {
    if ( number_range( 1, 100 ) <= -base ) {
      return false;
    }
  }

  // Failure rate is 95% at level 1, 5% at level 10.
  if( number_range( 1, 20 ) > 2 * level - 1 )
    return false;

  return true;
}


/*
 *   BATTLE SKILLS
 */


static char *consider_string( double ratio )
{
  if( ratio > 16 )  return "Attacking and suicide are equivalent.\n\r";
  if( ratio > 8 )   return "You would require divine intervention.\n\r";
  if( ratio > 4 )   return "You don't have much of a hope.\n\r";
  if( ratio > 2 )   return "You'll need a lot of help.\n\r";
  if( ratio > 1.5 ) return "With luck on your side you might prevail.\n\r";
  if( ratio > 1 )   return "It will be a tough fight.\n\r";
  
  if( 1./ratio > 5 ) return "An unworthy opponent.\n\r";
  if( 1./ratio > 2 ) return "Should be an easy kill.\n\r";
  if( 1./ratio > 1.5 ) return "You shouldn't have much difficulty.\n\r";
  
  return "It looks like a fair fight.\n\r";
}
 

void do_consider( char_data* ch, const char *argument )
{
  if( is_mob( ch ) )
    return;
  
  char_data*  victim;

  if( !( victim = one_character( ch, argument, "consider", ch->array ) ) )
    return;

  set_delay( ch, 10 );

  if( ch->is_affected( AFF_HALLUCINATE ) ) {
    send( ch, "Hah! Death to all grid bugs!\n\r" );
    return;
  }
  
  if( victim->species && victim->shdata->deaths == 0 ) {
    fsend( ch,
	   "You look at %s but are unable to determine how difficult %s is.",
	   victim, victim->He_She( ) );
  } else {
    send( ch, consider_string( (double) xp_compute( victim ) / (double) xp_compute( ch ) ) );
  }
  
  if( victim->species )
    send( ch, "[ Nation: %s ]\n\r",
	  nation_table[ victim->species->nation ].name );
}


void do_inspect( char_data* ch, const char *argument )
{
  if( ch->get_skill( SKILL_INSPECT ) == UNLEARNT ) {
    if( !not_player( ch ) )
      send( ch, "You are not trained at inspecting containers for traps.\n\r" );
    return;
  }

  obj_data *obj;

  if( !( obj = one_object( ch, argument, "inspect", &ch->contents, ch->array) ) ) {
    return;
  }

  if( is_fighting( ch, "inspect for traps" )
      || is_entangled( ch, "inspect for traps" ) ) {
    return;
  }

  if( obj->pIndexData->item_type != ITEM_CONTAINER ) {
    fsend( ch, "%s is not a container.", obj );
    return;
  }

  if( !is_set( obj->value[1], CONT_CLOSED ) ) {
    fsend( ch, "%s is already open - just look inside it.", obj );
    return;
  }

  if( is_set( obj->value[1], CONT_LOCKED ) ) {
    fsend( ch, "%s is locked.", obj );
    return;
  }

  fsend( ch, "You inspect %s for traps.", obj );
  fsend_seen( ch, "%s carefully inspects %s.", ch, obj );

  set_delay( ch, 20 );

  obj_data *trap;
  bool found = false;
  bool exist = false;
  for( int i = 0; i < obj->contents ; ++i ) {
    if( ( trap = object( obj->contents[i] ) )
	&& trap->pIndexData->item_type == ITEM_TRAP ) {
      exist = true;
      if ( trap->Seen( ch )
	   && levellimit( trap, ch )
	   //	   && trap->pIndexData->level <= ch->Level( )
	   && ch->check_skill( SKILL_INSPECT, 10 ) ) {
	if( !found ) {
	  fsend_color( ch, COLOR_SKILL, "YIKES! You find %s.", trap );
	  fsend_color_seen( ch, COLOR_SKILL, "%s seems to have found something.", ch->He_She() );
	  found = true;
	} else {
	  fsend_color( ch, COLOR_SKILL, "You also find %s.", trap );
	}
      }
    }
  }
  
  if( !found ) {
    send( ch, "You find nothing unusual.\n\r" );
    fsend_seen( ch, "%s doesn't seem to have found anything.", ch->He_She() );
    return;
  }

  if( found || !exist ) {
    ch->improve_skill( SKILL_INSPECT );
  }
}


void do_untrap( char_data *ch, const char *argument )
{
  if( ch->get_skill( SKILL_UNTRAP ) == UNLEARNT ) {
    if( !not_player( ch ) )
      send( ch, "You are not trained at removing traps.\n\r" );
    return;
  }

  obj_data *obj;
  if( !( obj = one_object( ch, argument, "untrap",
			   &ch->contents, ch->array) ) ) {
    return;
  }
  
  if( is_fighting( ch, "disarm traps" )
      || is_entangled( ch, "disarm traps" ) ) {
    return;
  }

  for( oprog_data *oprog = obj->pIndexData->oprog; oprog; oprog = oprog->next ) {
    if( oprog->trigger == OPROG_TRIGGER_UNTRAP ) {
      clear_variables( );
      var_obj = obj;
      var_ch = ch;    
      var_room = ch->in_room;
      oprog->execute( ); 
      return;
    }
  }

  if( obj->pIndexData->item_type != ITEM_CONTAINER ) {
    fsend( ch, "%s is not a container.", obj );
    return;
  }

  if( !is_set( obj->value[1], CONT_CLOSED ) ) {
    fsend( ch, "%s is already open - just look inside it.", obj );
    return;
  }
  
  if( is_set( obj->value[1], CONT_LOCKED ) ) {
    fsend( ch, "%s is locked.", obj );
    return;
  }

  fsend( ch, "You attempt to disarm traps on %s.", obj );
  fsend_seen( ch, "%s fiddles with %s.", ch, obj );

  set_delay( ch, 32 );

  for( int i = obj->contents.size - 1; i >= 0; i-- ) {
    obj_data *trap;
    if( ( trap = object( obj->contents[i] ) )
	&& trap->pIndexData->item_type == ITEM_TRAP ) {
      if( trap->Seen( ch )
	   && levellimit( trap, ch )
	   && ch->check_skill( SKILL_INSPECT ) ) {
	trap->Select( 1 );
	oprog_data *oprog;
	for( oprog = trap->pIndexData->oprog; oprog; oprog = oprog->next ) {
	  if( oprog->trigger == OPROG_TRIGGER_UNTRAP ) {
	    clear_variables( );
	    var_obj = trap;
	    var_ch = ch;
	    var_room = ch->in_room;
	    var_container = obj;
	    oprog->execute( ); 
	    return;
	  }
        }
	if( !oprog ) {
	  if ( ch->check_skill( SKILL_UNTRAP, 5 ) ) {
	    fsend_color( ch, COLOR_SKILL, "You disarm %s.", trap );
	    fsend_color_seen( ch, COLOR_SKILL, "%s disarms %s.", ch, trap );
	    trap->Extract();
	    ch->improve_skill( SKILL_UNTRAP );
	    return;
	  } else {
	    for( oprog = trap->pIndexData->oprog; oprog; oprog = oprog->next ) {
	      if( oprog->trigger == OPROG_TRIGGER_USE ) {
		clear_variables( );
		var_obj = trap;
		var_ch = ch;
		var_room = ch->in_room;
		var_container = obj;
		oprog->execute( ); 
		return;
	      }
	    }
	    if( !oprog ) {
	      bug( "Inside trap object %d has no UNTRAP or USE trigger.",
		  trap->pIndexData->vnum );
	      trap->Extract();
	    }
	  }
	}
      }
    }
  }
  
  send( ch, "You find no traps.\n\r", obj );
  send_seen( ch, "Nothing happens.", ch );
}


void do_compare( char_data* ch, const char *argument )
{
  if( is_mob( ch ) )
    return;
  
  int flags;
  if( !get_flags( ch, argument, &flags, "sS", "compare" ) )
    return;

  char arg  [ MAX_INPUT_LENGTH ];
  if( !two_argument( argument, "with", arg ) ) {
    send( ch, "Syntax: compare <obj1> [with] <obj2>.\n\r" );
    return;
  }  
  
  mob_data *keeper = 0;  // For compiler warning.

  if( flags != 0 ) {
    if( !( keeper = find_keeper( ch ) ) )
      return;
    if( is_set( ch->in_room->room_flags, RFLAG_PET_SHOP ) ) {
      send( ch, "You can't compare pets yet.\n\r" );
      return;
    }
  }
  
  obj_data *obj1;
  if( is_set( flags, 0 ) ){
    if( !( obj1 = one_object( ch, arg, "compare", 
			      &keeper->contents ) ) )
      return;
  } else
    if( !( obj1 = one_object( ch, arg, "compare",
			      &ch->contents ) ) )
      return;
  
  const int sel1 = obj1->temp;

  obj_data *obj2;
  if( is_set( flags, 1 ) ){
    if( !( obj2 = one_object( ch, argument, "compare", 
			      &keeper->contents ) ) )
      return;
  } else
    if( !( obj2 = one_object( ch, argument, "compare",
			      &ch->contents ) ) ) 
      return;
  
  if( !ch->Can_See( true )
      || is_fighting( ch, "compare" )
      || is_entangled( ch, "compare" )
      || is_drowning( ch, "compare" ) ) {
    return;
  }
    
  if( obj1 == obj2 && sel1 == obj2->temp ) {
    fsend( ch,
	   "You compare %s with itself coming to no useful conclusion.",
	   obj1 );
    return;
  }
  
  if( obj1->pIndexData->item_type != obj2->pIndexData->item_type ) {
    send( ch, "Objects must be of same type.\n\r" );
    return;
  }
  
  set_delay( ch, 10 );

  if( obj1->pIndexData->item_type == ITEM_ARMOR ) {
    const int armor1 = armor_class( obj1, is_set( obj1->extra_flags, OFLAG_IDENTIFIED ) );
    const int armor2 = armor_class( obj2, is_set( obj2->extra_flags, OFLAG_IDENTIFIED ) );
  
    if( ( obj1->pIndexData->wear_flags & obj2->pIndexData->wear_flags
	  & ~( 1 << ITEM_TAKE ) ) == 0 ) {
      fsend( ch,
	     "Those items do not get worn on the same location, so comparing them is not a useful exercise." );
      return;
    }

    if( armor1 == armor2 ) {
      fsend( ch, "You think %s and %s would offer similar protection.",
	     obj1, obj2 );
      return;
    }

    if( armor1 < armor2 )
      swap( obj1, obj2 );
    
    fsend( ch, "You think %s would offer better protection than %s.",
	   obj1, obj2 );
    return;
  }
  
  if( obj1->pIndexData->item_type == ITEM_WEAPON ) {
    int dam1 = dice_data( obj1->value[1] ).twice_average( );
    int dam2 = dice_data( obj2->value[1] ).twice_average( );

    if( is_set( obj1->extra_flags, OFLAG_IDENTIFIED ) )
      dam1 += 2*obj1->value[0];

    if( is_set( obj2->extra_flags, OFLAG_IDENTIFIED ) )
      dam2 += 2*obj2->value[0];
    
    if( dam1 == dam2 ) {
      fsend( ch, "You think %s and %s would do similar damage.",
	     obj1, obj2 );
      return;
    }
    
    if( dam1 < dam2 ) 
      swap( obj1, obj2 );
    
    fsend( ch, "You think %s would do more damage than %s.",
	   obj1, obj2 );
    return;
  }
  
  send( ch, "You can only compare weapons and armor.\n\r" );
}


void do_appraise( char_data *ch, const char *argument )
{
  obj_data* obj;
  if( !( obj = one_object( ch, argument, "appraise", &ch->contents ) ) ) {
    return;
  }

  if( !ch->Can_See( true )
      || is_fighting( ch, "appraise" )
      || is_entangled( ch, "appraise" )
      || is_drowning( ch, "appraise" ) ) {
    return;
  }
  
  if( int val = monetary_value( obj ) ) {
    fsend( ch,
	   "Your keen numismatic skills tell you that %s is worth exactly its face value of %s copper piece%s.",
	   obj, number_word( val, ch ), val == 1 ? "" : "s" );
    return;
  }

  if( ch->get_skill( SKILL_APPRAISE ) == UNLEARNT ) {
    if( !not_player( ch ) )
      fsend( ch,
	     "You know no more about an object's value than what you have learned from looking in the shops." );
    return;
  }

  fsend_seen( ch, "%s takes an appraising look at %s %s is carrying.",
	      ch, obj, ch->He_She( ) );

  set_delay( ch, 10 );

  int cost = obj->Cost( );

  if( cost >
      ch->get_skill(SKILL_APPRAISE)*ch->get_skill(SKILL_APPRAISE)
      *ch->Level( ) ) {
    fsend( ch, "You think %s is valuable but are unsure how valuable.",
	   obj );
    fsend_seen( ch, "%s scowls in frustration.", ch );
    return;
  }
  
  /* Cost equation from shop.cc:get_cost() */

  if( obj->pIndexData->item_type == ITEM_ARMOR
      || obj->pIndexData->item_type == ITEM_WEAPON )
    cost /= 3;

  cost = cost * obj->condition / obj->Durability() * sqr(4-obj->rust) / 16;

  switch( obj->pIndexData->item_type ) {
  case ITEM_WAND :
    if( obj->value[0] != 0 ) {
      cost *= obj->value[3] + obj->pIndexData->value[3];
      cost /= 2*obj->pIndexData->value[3];
    }
    break;
    
  case ITEM_GEM :
    if( obj->value[1] > GEM_UNCUT
	&& obj->value[1] < GEM_FLAWLESS ) {
      cost /= 2;
    } else if( obj->value[1] == GEM_FLAWLESS ) {
      cost *= 2;
    }
    break;
  }    

  if( !is_set( obj->extra_flags, OFLAG_IDENTIFIED )
      && ( obj->pIndexData->fakes != 0 && get_obj_index( obj->pIndexData->fakes )
	   || obj->pIndexData->item_type == ITEM_GEM ) ) {

    Content_Array *array = obj->array;
    
    if( array ) {
      obj = (obj_data *) obj->From( 1, true );
    }
  
    set_bit( obj->extra_flags, OFLAG_IDENTIFIED );

    if( array )
      obj->To( );
  }

  if( cost <= 0 ) {
    fsend( ch, "%s looks worthless to you.", obj );
    fsend_seen( ch, "%s sighs in resignation.", ch );
    return;
  }

  fsend( ch, "You think %s would be worth about %d cp.",
	 obj, cost );
  fsend_seen( ch, "%s nods in comprehension.", ch );
  
  ch->improve_skill( SKILL_APPRAISE );
}


void do_bandage( char_data *ch, const char *argument )
{
  if( ch->get_skill( SKILL_BANDAGE ) == UNLEARNT ) {
    if( !not_player( ch ) )
      send( ch, "You really wouldn't know how to bandage wounds.\n\r" );
    return;
  }

  char_data *victim;
  if( !( victim = one_character( ch, argument, "bandage", ch->array ) ) )
    return;
  
  if( ch == victim ) {
    send( ch, "If you are able to bandage you don't need to be bandaged.\n\r");
    return;
  }
  
  if( victim->hit > 0 ) {
    fsend( ch, "%s doesn't need bandaging.", victim );
    return;
  }
  
  if( is_fighting( ch, "bandage someone" )
      || is_entangled( ch, "bandage someone" )
      || is_drowning( ch, "bandage someone" ) ) {
    return;
  }
  
  obj_data *obj = 0;  // For compiler warning.

  bool found = false;
  for( int i = 0; i < ch->contents; i++ )
    if( (obj = object( ch->contents[i] )) &&
	obj->pIndexData->item_type == ITEM_BANDAGE &&
	obj->position == WEAR_NONE ) {
      found = true;
      break;
    }
  
  if( !found ) {
    fsend( ch, "You don't have anything to bandage %s with!", victim );
    return;
  }
  
  obj->Select( 1 );

  if( ch->check_skill( SKILL_BANDAGE ) ) {
    fsend_color( ch, COLOR_SKILL,
	   "You use %s to bandage %s's wounds, stabilizing %s injuries.",
	   obj, victim, victim->His_Her( ) );
    fsend_color_seen( ch, COLOR_SKILL, "%s uses %s to bandage %s's wounds, stabilizing %s injuries.",
		ch, obj, victim, victim->His_Her() );
    send_color( victim, COLOR_SKILL, "Someone bandages your wounds, stabilizing your injuries.");
    send( victim, "\n\r" );
    victim->hit = 1;
    update_pos( victim );
    ch->improve_skill( SKILL_BANDAGE );

  } else {
    fsend( ch,
	   "You try to bandage %s's wounds but just make the situation worse.",
	   victim );
    fsend_seen( ch,
		"%s tries to bandage %s's wounds but does more harm than good.",
		ch, victim );
  }
  
  set_delay( ch, 20 );

  obj->Extract( 1 );
}
