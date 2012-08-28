#include <sys/types.h>
#include <stdio.h>
#include "define.h"
#include "struct.h"


/*
 *   CONST TABLES
 */


const char* affect_location[] = {
  "None", "Strength", "Dexterity",
  "Intelligence", "Wisdom", "Constitution", "Magic", "Fire", "Cold",
  "Electricity", "Mind", "Age", "Mana_points", "Hit_points", "Move_points",
  "?Unused", "?Unused",
  "Armor", "Hitroll", "Damroll", "Mana_regen", "Hit_regen", "Move_regen",
  "Acid", "Poison"
};


static const char *const modify_amount[] = {
  "slightly", "somewhat", "quite a bit",
  "much", "**MUCH**"
};


static const char* const affect_name [ 2*MAX_AFF_LOCATION ] = {
  "stronger", "weaker",
  "more agile", "less agile",
  "more intelligent", "less intelligent",
  "wiser", "less wise",
  "tougher", "less tough",
  "", "",
  "", "",
  "", "",
  "", "",
  "", "",
  "older", "younger",
  "", "",
  "", "",
  "", "",
  "", "",
  "", "",
  "", "",
  "", "",
  "", "",
  "", "",
  "", "",
  "", "",
  "", "",
  "", ""
};


/*
 *   AFFECT CLASS
 */


affect_data :: affect_data( )
  : leech(0), target(0),
    type(AFF_NONE), duration(-1), level(1),
    location(APPLY_NONE), modifier(0),
    leech_regen(0), leech_max(0)
{
  record_new( sizeof( affect_data ), MEM_AFFECT );
}


affect_data::affect_data(const affect_data& aff)
  : leech(aff.leech), target(aff.target),
    type(aff.type), duration(aff.duration), level(aff.level),
    location(aff.location), modifier(aff.modifier),
    leech_regen(aff.leech_regen), leech_max(aff.leech_max)
{
  record_new( sizeof( affect_data ), MEM_AFFECT );
}


affect_data :: ~affect_data( )
{
  record_delete( sizeof( affect_data ), MEM_AFFECT );
}


static event_data *find_event( thing_data *owner, const affect_data *affect )
{
  for( int i = 0; i < owner->events; ++i ) {
    event_data *event = owner->events[i];
    if( event->func == update_affect
	&& event->pointer == affect ) {
      return event;
    }
  }

  bug( "Find_event: affect event not found." );
  bug( "-- Owner = %s.", owner );

  return 0;
}


static int innate_affect( const char_data *ch, int sn, int level = 0, int race = -1 )
{
  if( sn == AFF_SNEAK
      || sn == AFF_CAMOUFLAGE
      || sn == AFF_HIDE ) {
    return 0;
  }

  if( level <= 0 ) {
    level = ch->Level( );
  }

  if( race < 0 ) {
    race = ch->shdata->race;
  }

  if( ( race < MAX_PLYR_RACE
	&& is_set( plyr_race_table[race].affect, sn ) )
      || ( ch->species
	   && ch->species->is_affected( sn ) ) ) {
    if( ch->species ) {
      return min( 10, 3 + level/10 );
    } else {
      return min( 7, 3 + level/10 );
    }
  }

  return 0;
}


void init_affects( char_data *ch, int old_level, int old_race )
{
  affect_data aff;

  if( old_race >= 0 && old_race != ch->shdata->race && old_race < MAX_PLYR_RACE ) {
    // Race change, need to remove old innate affects first.
    for( int i = 0; i < table_max[ TABLE_AFF_CHAR ]; ++i ) {
      if( is_set( plyr_race_table[ old_race ].affect, i ) ) {
	if( i != AFF_SNEAK
	    && i != AFF_CAMOUFLAGE
	    && i != AFF_HIDE ) {
	  aff.type = i;
	  aff.level = innate_affect( ch, i, 0, old_race );
	  modify_affect( ch, &aff, false, false );
	}
      }
    }
  }

  if( ch->shdata->race < MAX_PLYR_RACE ) {
    for( int i = 0; i < table_max[ TABLE_AFF_CHAR ]; ++i ) {
      if( is_set( plyr_race_table[ ch->shdata->race ].affect, i ) ) {
	if( i != AFF_SNEAK
	    && i != AFF_CAMOUFLAGE
	    && i != AFF_HIDE ) {
	  if( old_level > 0 ) {
	    if( old_level == ch->Level( )
		|| aff_char_table[ i ].location == APPLY_NONE
		|| !ch->is_affected( i ) )
	      continue;
	    // Up/down-grade the affect modifier if ch's experience level changed.
	    int new_cast = innate_affect( ch, i );
	    int old_cast = innate_affect( ch, i, old_level );
	    if( old_cast == new_cast )
	      continue;
	    bool error1;
	    int new_mod = evaluate( aff_char_table[ i ].modifier, error1, new_cast, 0 );
	    bool error2;
	    int old_mod = evaluate( aff_char_table[ i ].modifier, error2, old_cast, 0 );
	    if( error1 || error2 ) {
	      bug( "Init_Affects: Failed evaluate." );
	      bug( "-- Aff = %s", aff_char_table[ i ].name ); 
	      continue;
	    }
	    if( old_mod == new_mod )
	      continue;
	    aff.type = AFF_NONE;
	    aff.location = aff_char_table[ i ].location;
	    aff.modifier = new_mod - old_mod;
	    modify_affect( ch, &aff, true, false );
	  } else {
	    aff.type = i;
	    aff.level = innate_affect( ch, i );
	    modify_affect( ch, &aff, true, false );
	  }
	}
      }
    }
  }

  if( ch->species ) {
    for( int i = 0; i < table_max[ TABLE_AFF_CHAR ]; ++i ) {
      if( ch->species->is_affected( i ) ) {
	if( i != AFF_SNEAK
	    && i != AFF_CAMOUFLAGE
	    && i != AFF_HIDE ) {
	  aff.type = i;
	  aff.level = innate_affect( ch, i );
	  modify_affect( ch, &aff, true, false );
	}
      }
    }
  }
}


/*
 *   DISK ROUTINES
 */


void read_affects( FILE* fp, char_data* ch )
{
  int           i, j;

  fread( &i, sizeof( int ), 1, fp );

  for( ; i > 0; i-- ) {
    affect_data *affect = new affect_data;
    fread( &affect->type,     sizeof( int ), 1, fp );
    fread( &affect->duration, sizeof( int ), 1, fp );
    fread( &affect->level,    sizeof( int ), 1, fp );
    fread( &affect->modifier, sizeof( int ), 1, fp );
    fread( &affect->location, sizeof( int ), 1, fp ); 
    fread( &j, sizeof( int ), 1, fp );
    if( j != -1 ) {
      affect->leech_regen = j;
      affect->leech       = ch;
      ch->leech_list     += affect;
      fread( &affect->leech_max, sizeof( int ), 1, fp );
    }
    affect->target = ch;
    modify_affect( ch, affect, true );
    ch->affected += affect;

    event_data *event = new event_data( update_affect, ch );
    event->pointer = (void*) affect;
    add_queue( event, affect_delay( ) );
  }
}


void write_affects( FILE* fp, char_data* ch )
{
  affect_data*  affect;
  int             i, j;

  if( ch->affected.is_empty() )
    return;

  fwrite( "Afft", 4, 1, fp );
  fwrite( &ch->affected.size, sizeof( int ), 1, fp );

  for( i = 0; i < ch->affected; ++i ) {
    affect = ch->affected[i];
    fwrite( &affect->type,     sizeof( int ), 1, fp );
    fwrite( &affect->duration, sizeof( int ), 1, fp );
    fwrite( &affect->level,    sizeof( int ), 1, fp );
    fwrite( &affect->modifier, sizeof( int ), 1, fp );
    fwrite( &affect->location, sizeof( int ), 1, fp );
    if( affect->leech == ch ) {
      if( affect->leech_regen < 0 ) {
	bug( "Attempted to save leech with regen < 0!" );
	bug( "-- char = %s", ch );
	bug( "-- type = %d", affect->type );
	j = -1;
	fwrite( &j, sizeof( int ), 1, fp );
      } else {
	fwrite( &affect->leech_regen, sizeof( int ), 1, fp );
	fwrite( &affect->leech_max,   sizeof( int ), 1, fp );        
      }
    } else {
      j = -1;
      fwrite( &j, sizeof( int ), 1, fp );
    }
  } 
}


/*
 *   CHARACTER SUBROUTINES
 */


int prep_max( char_data* ch )
{
  int i = 0;

  for( cast_data *cast = ch->prepare; cast; cast = cast->next )
    i += cast->mana*cast->times;
  
  return i;
}


int affect_level( const char_data *ch, int sn, affect_data *af )
{
  if( !ch->is_affected( sn ) ) {
    return 0;
  }

  int level = innate_affect( ch, sn );

  for( int i = 0; i < ch->affected; ++i ) {
    affect_data *paf = ch->affected[i];
    if( paf != af
	&& paf->type == sn ) {
      level = max( level, paf->level );
      break;
    }
  }

  if( level < 7 ) {	// *** FIX ME!
    for( int i = 0; i < ch->wearing; ++i ) {
      obj_data *obj = (obj_data*) ch->wearing[i];
      if( is_set( obj->pIndexData->affect_flags, sn ) ) {
	level = 7;	// *** FIX ME!
	break;		// *** FIX ME!
      }
    }
  }

  // Make "unknown" affects work at level 1,
  // Unless we're just checking if there ARE any others besides *af.
  if( !af ) {
    level = max( level, 1 );
  }

  return level;
}


/*
 *   DO_AFFECT ROUTINE
 */


static void page_sources( char_data *ch, char_data *victim, bool all )
{
  bool found = false;

  for( int sn = 0; sn < table_max[ TABLE_AFF_CHAR ]; ++sn ) {
    if( victim->is_affected( sn )
	&& *aff_char_table[sn].score_name ) {
      if( !found ) {
        found = true;
	if( ch != victim ) {
	  page_title( ch, "Affects for %s", victim );
	}
        page_underlined( ch, "%-50s%s\n\r", "Affect", "Source" );
      }
      bool first = true;
      const char *name = aff_char_table[sn].score_name;
      bool nl = false;
      
      if( strlen( name ) > 49 ) {
	nl = true;
	page( ch, "%s\n\r", trunc( name, 79 ) );
	name = empty_string;
      }
      
      if( innate_affect( victim, sn ) ) {
	first = false;
	page( ch, "%-49s innate\n\r", name );
      }
      
      for( int i = 0; i < victim->affected; ++i ) {
	affect_data *affect = victim->affected[i];
	if( sn == affect->type ) {
	  if( !affect->leech ) {
	    if( first ) {
	      first = false;
	      if( is_builder( ch ) ) {
		page( ch, "%-49s %d turn%s\n\r", name, affect->duration,
		      affect->duration == 1 ? "" : "s" );
	      } else {
		page( ch, "%-49s temporary\n\r", name );
	      }
	    } else {
	      if( is_builder( ch ) ) {
		page( ch, "%-49s +%d turn%s\n\r", "", affect->duration,
		      affect->duration == 1 ? "" : "s" );
	      } else {
		page( ch, "%-49s +temporary\n\r", "" );
	      }
	    }
	  } else {
	    if( first ) {
	      first = false;
	      page( ch, "%-49s leech: %s\n\r", name,
		    affect->leech == victim ? "self"
		    : trunc( affect->leech->Seen_Name( ch ), 22 ) );
	    } else {
	      page( ch, "%-49s +leech: %s\n\r", "",
		    affect->leech == victim ? "self"
		    : trunc( affect->leech->Seen_Name( ch ), 21 ) );
	    }
	  }
	}
      }
      
      for( int i = 0; i < victim->wearing; ++i ) {
	obj_data *obj = (obj_data*) victim->wearing[i];
	if( is_set( obj->pIndexData->affect_flags, sn ) ) {
	  if( first ) {
	    first = false;
	    page( ch, "%-49s %s\n\r", name,
		  trunc( obj->Seen_Name( victim, 1, true ), 29 ) );
	  } else {
	    page( ch, "%-49s +%s\n\r", "",
		  trunc( obj->Seen_Name( victim, 1, true ), 28 ) );
	  }
	}
      }
      
      if( first ) {
	page( ch, "%-49s unknown\n\r", name );
      }
    }
  }

  if( all ) {
    char *tmp = static_string( );
    
    for( int i = 1; i < MAX_AFF_LOCATION; ++i ) {
      for( int k = 0; k < 2; ++k ) {
	bool first = true;
	for( int j = 0; j < victim->affected; ++j ) {
	  affect_data *affect = victim->affected[j];
	  if( affect->type == AFF_NONE
	      && affect->location == i
	      && affect->modifier != 0
	      && ( affect->modifier > 0 ) == ( k == 0 )
	      && *affect_name[ 2*(i-1)+k ] ) {
	    if( !found ) {
	      found = true;
	      if( ch != victim ) {
		page_title( ch, "Affects for %s", victim );
	      }
	      page_underlined( ch, "%-50s%s\n\r", "Affect", "Source" );
	    }
	    if( first ) {
	      first = false;
	      snprintf( tmp, THREE_LINES, "You feel %s.", affect_name[ 2*(i-1)+k ] );
	      if( is_builder( ch ) ) {
		page( ch, "%-49s %d turn%s\n\r", tmp, affect->duration,
		      affect->duration == 1 ? "" : "s" );
	      } else {
		page( ch, "%-49s temporary\n\r", tmp );
	      }
	    } else {
	      if( is_builder( ch ) ) {
		page( ch, "%-49s +%d turn%s\n\r", "", affect->duration,
		      affect->duration == 1 ? "" : "s" );
	      } else {
		page( ch, "%-49s +temporary\n\r", "" );
	      }
	    }
	  }
	}
	for( int j = 0; j < victim->wearing; ++j ) {
	  obj_data *obj = (obj_data*) victim->wearing[j];
	  for( int n = 0; n < obj->pIndexData->affected; ++n ) {
	    affect_data *affect = obj->pIndexData->affected[n];
	    if( affect->type == AFF_NONE
		&& affect->location == i
		&& affect->modifier != 0
		&& ( affect->modifier > 0 ) == ( k == 0 )
		&& *affect_name[ 2*(i-1)+k ] ) {
	      if( !found ) {
		found = true;
		if( ch != victim ) {
		  page_title( ch, "Affects for %s", victim );
		}
		page_underlined( ch, "%-50s%s\n\r", "Affect", "Source" );
	      }
	      if( first ) {
		first = false;
		snprintf( tmp, THREE_LINES, "You feel %s.", affect_name[ 2*(i-1)+k ] );
		page( ch, "%-49s %s\n\r", tmp,
		      trunc( obj->Seen_Name( victim, 1, true ), 29 ) );
	      } else {
		page( ch, "%-49s +%s\n\r", "",
		      trunc( obj->Seen_Name( victim, 1, true ), 28 ) );
	      }
	    }
	  }
	}
      }
    }
  }

  if( !found ) {
    if( ch == victim ) {
      send( ch, "You have not noticed any affects to yourself.\n\r" );
    } else {
      fsend( ch, "No affects found for %s.", victim );
    }
  }
}


void do_affects( char_data* ch, const char *argument )
{
  if( is_confused_pet( ch ) )
    return;
  
  int flags;

  if( is_builder( ch ) ) {
    if( !get_flags( ch, argument, &flags, "al", "Affect" ) )
      return;
    if( is_set( flags, 1 ) ) {
      if( ch->affected == 0 ) {
        send( ch, "You have no non-default affects.\n\r" );
        return;
      }
      page_underlined( ch, "Dur.   Level   Affect\n\r" );
      for( int i = 0; i < ch->affected; i++ ) {
        affect_data *affect = ch->affected[i];
        page( ch, "%3d%8d    %s\n\r", affect->duration, affect->level,
	      affect->type == AFF_NONE
	      ? "Unknown?"
	      : aff_char_table[ affect->type ].name );
      }
      return;
    }
  } else {
    if( !get_flags( ch, argument, &flags, "a", "Affect" ) )
      return;
  }

  char_data *victim = ch;

  if( is_builder( ch ) && *argument ) {
    if( has_permission( ch, PERM_PLAYERS )
	&& ( victim = one_player( ch, argument, empty_string,
				  (thing_array*) &player_list ) ) ) {
      if( victim != ch
	  && get_trust( victim ) >= get_trust( ch ) ) {
	fsend( ch, "You cannot view the affects of %s.", victim );
	return;
      }
    } else if( !( victim = one_mob( ch, argument, "affects", (thing_array*) ch->array ) ) ) {
      return;
    }
  }

  page_sources( ch, victim, is_set( flags, 0 ) );
}


/*
 *   AFFECT ROUTINES
 */


bool add_affect( char_data* ch, affect_data* paf )
{
  //  if( innate_affect( ch, paf->type ) )
  //    return;

  // Return true even if they have the affect innately, to allow spell improves.

  bool add = true;

  affect_data *paf_new = 0;

  if( paf->type != AFF_NONE ) {
    for( int i = 0; i < ch->affected; ++i ) {
      if( paf->type == ch->affected[i]->type ) {
	add = false;
        paf_new = ch->affected[i];
	if( paf->duration >= paf_new->duration ) {
	  if( event_data *event = find_event( ch, paf_new ) ) {
	    unlink( event );
	    paf_new->duration = paf->duration;
	    add_queue( event, affect_delay( ) );
	  } else {
	    ch->affected -= paf_new;
	    delete paf_new;
	    paf_new = 0;
	  }
	}
	break;
      }
    }
  }
 
  if( !paf_new ) {
    paf_new = new affect_data( *paf );

    modify_affect( ch, paf_new, true );

    paf_new->leech = 0;
    paf_new->target = ch;

    ch->affected += paf_new;

    event_data *event = new event_data( update_affect, ch );
    event->pointer = paf_new;
    add_queue( event, affect_delay( ) );
  }
  
  if( !paf_new->leech && paf->leech ) {
    paf->leech->leech_list   += paf_new;
    paf->leech->max_mana     -= paf->leech_max;
    paf_new->leech            = paf->leech;
    paf_new->leech_regen      = paf->leech_regen;
    paf_new->leech_max        = paf->leech_max;
  }

  return add;
}


void remove_affect( char_data* ch )
{
  for( int i = ch->affected.size-1; i >= 0; i-- ) {
    affect_data *affect = ch->affected[i];
    remove_affect( ch, affect );
  }
}


void remove_affect( char_data* ch, affect_data* affect, bool msg )
{
  if( affect->leech ) {
    if( affect->leech != ch ) {
      fsend( affect->leech, "Leech for %s on %s dissipated.",
	     aff_char_table[affect->type].name, affect->target );
    }
    remove_leech( affect );
  }

  //  if( innate_affect( ch, affect->type ) )
  //    return;

  ch->affected -= affect;
  
  modify_affect( ch, affect, false, msg );
  
  if( affect->type != AFF_NONE
     && !ch->is_affected( affect->type ) ) {
    if( affect->type == AFF_SLEEP ) {
      if( !ch->pcdata
	  && ch->in_room
	  && !is_set( ch->status, STAT_PET )
	  && ch->position == POS_SLEEPING ) {
	mob_data *npc = ((mob_data*) ch );
	if( !npc->reset
	    || npc->reset->value != RSPOS_SLEEPING
	    || is_set( npc->status, STAT_STOOD ) ) {
	  do_stand( ch, "" );
	}
      }
    } else if( affect->type == AFF_INVISIBLE
	       || affect->type == AFF_PROT_PLANTS ) {
      // Factors that might have been preventing is_aggressive( *, ch ) == true.
      update_aggression( ch );
    }
  }
  
  if( event_data *event = find_event( ch, affect ) ) {
    extract( event );
  }
  
  delete affect;
}


int shorten_affect( char_data* ch, int sn, int amount, bool msg )
{
  if( !ch->is_affected( sn ) )
    return 0;

  for( int i = ch->affected.size-1; i >= 0; --i ) {
    affect_data *affect = ch->affected[i];
    if( affect->type == sn ) {
      if( affect->duration <= amount ) {
	remove_affect( ch, affect, msg );
	return 0;
      } else {
	affect->duration -= amount;
	if( affect->duration == 1
	    && affect_level( ch, affect->type, affect ) == 0 ) {
	  act( ch, aff_char_table[ affect->type ].msg_fade, ch );
	  act_social_room( aff_char_table[ affect->type ].msg_fade_room, ch );
	}
	return affect->duration;
      }
    }
  }

  return -1;
}


bool strip_affect( char_data* ch, int sn, bool msg )
{
  if( !ch->is_affected( sn ) )
    return false;

  //  if( innate_affect( ch, sn ) )
  //    return false;

  const bool before = ch->is_affected( sn );

  for( int i = ch->affected.size-1; i >= 0; --i ) {
    affect_data *affect = ch->affected[i];
    if( affect->type == sn ) {
      remove_affect( ch, affect, msg );
      break;
    }
  }

  const bool after = ch->is_affected( sn );

  return before && !after;
}


/*
static bool has_affect( char_data* ch, int sn )
{
  if( sn == AFF_NONE ) 
    return false;

  if( innate_affect( ch, sn ) )
    return true;
  
  for( int i = 0; i < ch->affected; ++i )
    if( ch->affected[i]->type == sn )
      return true;

  obj_data* obj;
  for( int i = 0; i < ch->wearing; ++i )
    if( ( obj = object( ch->wearing[i] ) )
	&& is_set( obj->pIndexData->affect_flags, sn ) )
      return true;

  return false;
}
*/


void modify_affect( char_data* ch, affect_data* paf, bool fAdd, bool msg )
{
  int mod;

  if( paf->type != AFF_NONE ) {
    const int max_lev = affect_level( ch, paf->type, paf );

    if( paf->level <= max_lev ) {
      return;
    }

    /*
    if( fAdd
	? ch->is_affected( paf->type )
	: has_affect( ch, paf->type ) )
      return;
    */
    
    paf->location = aff_char_table[ paf->type ].location;

    // If modifier == "code", the code has specified its value.
    if( strcasecmp( aff_char_table[ paf->type ].modifier, "code" ) ) {
      bool error;
      paf->modifier = evaluate( aff_char_table[ paf->type ].modifier,
				error, paf->level, 0 );
      if( error ) {
	bug( "Modify_Affect: Failed evaluate." );
	bug( "-- Aff = %s", aff_char_table[ paf->type ].name ); 
      }
    }

    mod = paf->modifier;

    if( max_lev == 0 ) {
      if( fAdd ) {
	if( ch->array && msg ) { 

	  act( ch, aff_char_table[ paf->type ].msg_on, ch );
	  act_social_room( aff_char_table[ paf->type ].msg_on_room, ch );
	}
	set_bit( ch->affected_by, paf->type );
	
      } else {
	remove_bit( ch->affected_by, paf->type );
	if( ch->array && msg ) {
	  act( ch, aff_char_table[ paf->type ].msg_off, ch );
	  act_social_room( aff_char_table[ paf->type ].msg_off_room, ch );
	}
      }
    } else {
      bool error;
      int mod2 = evaluate( aff_char_table[ paf->type ].modifier,
			   error, max_lev, 0 );
      if( error ) {
	bug( "Modify_Affect: Failed evaluate." );
	bug( "-- Aff = %s", aff_char_table[ paf->type ].name ); 
      }
      mod -= mod2;
    }

    mod = fAdd ? mod : -mod;

  } else {
    mod = paf->modifier*( fAdd ? 1 : -1 );
  }
  
  if( mod == 0 )
    return;

  switch( paf->location ) {
    default:
      bug( "Modify_affect: unknown location %d.", paf->location );
      return;
 
    case APPLY_AGE:
      if( !ch->species && ch->pcdata )
        ch->pcdata->mod_age += mod;
      break;

    case APPLY_NONE:                                                break;
    case APPLY_INT:         ch->mod_int                   += mod;   break;
    case APPLY_STR:         ch->mod_str                   += mod;   break;
    case APPLY_DEX:         ch->mod_dex                   += mod;   break;
    case APPLY_WIS:         ch->mod_wis                   += mod;   break;
    case APPLY_CON:         ch->mod_con                   += mod;   break;
    case APPLY_MAGIC:       ch->mod_resist[RES_MAGIC]     += mod;   break;
    case APPLY_FIRE:        ch->mod_resist[RES_FIRE]      += mod;   break;
    case APPLY_COLD:        ch->mod_resist[RES_COLD]      += mod;   break;
    case APPLY_SHOCK   :    ch->mod_resist[RES_SHOCK]     += mod;   break;
    case APPLY_MIND:        ch->mod_resist[RES_MIND]      += mod;   break;
    case APPLY_ACID:        ch->mod_resist[RES_ACID]      += mod;   break;
    case APPLY_POISON:      ch->mod_resist[RES_POISON]    += mod;   break;
    case APPLY_MOVE:        ch->mod_move                  += mod;   break;
    case APPLY_MANA:        ch->mod_mana                  += mod;   break;
    case APPLY_HIT:         ch->mod_hit                   += mod;   break;
    case APPLY_AC:          ch->mod_armor                 += mod;   break;
    case APPLY_HITROLL:     ch->hitroll                   += mod;   break;
    case APPLY_DAMROLL:     ch->damroll                   += mod;   break;
    case APPLY_MANA_REGEN:  ch->mana_regen                += mod;   break;
    case APPLY_HIT_REGEN:   ch->hit_regen                 += mod;   break;
    case APPLY_MOVE_REGEN:  ch->move_regen                += mod;   break;
  }
 
  int i = 2*( paf->location-1 )+( mod < 0 );

  if( ch->link
      && ch->link->connected == CON_PLAYING
      && *affect_name[i] ) {
    fsend( ch, "[You feel %s %s.]",
	   modify_amount[min( max( mod, -mod ), 5 )-1],
	   affect_name[i] );
  }
  
  if( ch->array )
    update_maxes( ch );
}


/*
 *   OBJECT AFFECT ROUTINES
 */


void read_affects( FILE* fp, obj_clss_data* obj_clss )
{
  char letter;
  
  while ( ( letter = fread_letter( fp ) ) == 'A' ) {
    affect_data *affect = new affect_data;    
    affect->location = fread_number( fp );
    affect->modifier = fread_number( fp );
    obj_clss->affected += affect;
  }

  ungetc( letter, fp );
}


void write_affects( FILE* fp, obj_clss_data* obj_clss )
{
  for( int i = 0; i < obj_clss->affected; ++i ) {
    const affect_data *affect = obj_clss->affected[i]; 
    fprintf( fp, "A %d %d\n", affect->location, affect->modifier );
  }
}


void read_affects( FILE* fp, obj_data* obj )
{
  affect_data*  affect;
  event_data*    event;
  int                i;

  fread( &i, sizeof( int ), 1, fp );

  for( ; i > 0; --i ) {
    affect = new affect_data;
    fread( &affect->type,     sizeof( int ), 1, fp );
    fread( &affect->duration, sizeof( int ), 1, fp );
    fread( &affect->level,    sizeof( int ), 1, fp );
    obj->affected += affect;

    event = new event_data( update_affect, obj );
    event->pointer = (void*) affect;
    add_queue( event, affect_delay( ) );
  }
}


void write_affects( FILE* fp, obj_data* obj )
{
  affect_data*  affect;

  if( obj->affected.is_empty() )
    return;

  fwrite( "Afft", 4, 1, fp );
  fwrite( &obj->affected.size, sizeof( int ), 1, fp );

  for( int i = 0; i < obj->affected; i++ ) {
    affect = obj->affected[i];
    fwrite( &affect->type,     sizeof( int ), 1, fp );
    fwrite( &affect->duration, sizeof( int ), 1, fp );
    fwrite( &affect->level,    sizeof( int ), 1, fp );
  }
}


void add_affect( obj_data*& obj, affect_data* paf )
{
  const int flag = aff_obj_table[ paf->type ].location;
  if( flag >= 0
      && is_set( obj->pIndexData->extra_flags, flag ) ) {
    return;
  }
  
  for( int i = 0; i < obj->affected; ++i ) {
    if( paf->type == obj->affected[i]->type ) {
      affect_data *paf_new = obj->affected[i];
      if( paf->duration >= paf_new->duration ) {
	if( event_data *event = find_event( obj, paf_new ) ) {
	  unlink( event );
	  paf_new->duration = paf->duration;
	  add_queue( event, affect_delay( ) );
	} else {
	  obj->affected -= paf_new;
	  delete paf_new;
	  paf_new = 0;
	}
      }
      return;
    }
  }
  
  obj->Select( 1 );

  if( Tprog_Data *tprog = aff_obj_table[ paf->type ].prog ) {
    push( );
    clear_variables( );
    var_obj = obj;
    var_i = paf->duration;
    const int result = tprog->execute( );
    pop( );
    if( !result
	|| !obj->Is_Valid( ) ) {
      return;
    }
  }

  Content_Array *array = obj->array;
  
  if( array ) {
    if( room_data *where = Room( array->where ) ) {
      act_room( where, aff_obj_table[ paf->type ].msg_on, 0, 0, obj );
    } else if( char_data *ch = character( array->where ) ) {
      act( ch, aff_obj_table[ paf->type ].msg_on, ch, 0, obj );
    }
  }

  affect_data *paf_new = new affect_data( *paf );

  paf_new->leech  = 0;
  paf_new->target = 0;
  
  // in_place because aff.obj affects don't make much difference
  // (except light) to equip() and unequip().
  if( array )
    obj = (obj_data *) obj->From( 1, true );

  obj->affected += paf_new;

  event_data *event = new event_data( update_affect, obj );
  event->pointer = paf_new;
  add_queue( event, affect_delay( ) );

  if( flag >= 0 ) {
    set_bit( obj->extra_flags, flag );
    
    switch( flag ) {
    case OFLAG_FLAMING:
      ++obj->light;
      break;
    }
  }

  if( array )
    obj->To( );
}


static affect_data *is_affected( obj_data* obj, int type )
{
  for( int i = 0; i < obj->affected; i++ )
    if( obj->affected[i]->type == type )
      return obj->affected[i];

  return 0;
}


void remove_affect( obj_data *obj )
{
  for( int i = obj->affected.size-1; i >= 0; i-- ) {
    remove_affect( obj, obj->affected[i] );
  }
}


void remove_affect( obj_data*& obj, affect_data* affect, bool msg )
{
  Content_Array *array = obj->array;

  // in_place because aff.obj affects don't make much difference
  // (except light) to equip() and unequip().

  if( array )
    obj = (obj_data *) obj->From( 1, true );

  //  obj->Select( 1 );
  obj->affected -= affect;

  const int flag = aff_obj_table[ affect->type ].location;
  if( ( flag < 0 || !is_set( obj->pIndexData->extra_flags, flag ) )
      && !is_affected( obj, affect->type ) ) {
    if( flag >= 0 )
      remove_bit( obj->extra_flags, flag );
    
    if( msg && array ) {
      if( room_data *where = Room( array->where ) ) {
	act_room( where, aff_obj_table[ affect->type ].msg_off, 0, 0, obj );
      } else if( char_data *ch = character( array->where ) ) {
	act( ch, aff_obj_table[ affect->type ].msg_off, 0, 0, obj );
      }
    }
    
    if( flag >= 0 ) {
      switch( flag ) {
      case OFLAG_FLAMING:
	--obj->light;
	break;
      }
    }
  }

  if( event_data *event = find_event( obj, affect ) ) {
    extract( event );
  }
  
  delete affect;

  if( array ) {
    obj->To( );
  }
}


bool strip_affect( obj_data*& obj, int sn, bool msg )
{
  const int flag = aff_obj_table[ sn ].location;

  if( flag >= 0
      && ( !is_set( obj->extra_flags, flag )
	   || is_set( obj->pIndexData->extra_flags, flag ) ) )
    return false;
  
  for( int i = obj->affected.size-1; i >= 0; --i ) {
    affect_data *affect = obj->affected[i];
    if( affect->type == sn ) {
      remove_affect( obj, affect, msg );
      return true;
    }
  }
  
  Content_Array *array = obj->array;

  // in_place because aff.obj affects don't make much difference
  // (except light) to equip() and unequip().

  if( array )
    obj = (obj_data *) obj->From( 1, true );

  if( flag >= 0 )
    remove_bit( obj->extra_flags, flag );

  if( array )
    obj->To( );

  roach( "Strip_Affect: Affect on object not found." );
  roach( "-- Obj = %s", obj );

  return true;
}


int affect_duration( const char_data* ch, int bit ) 
{
  if( ch->is_affected( bit ) ) 
    for( int i = 0; i < ch->affected; i++ )
      if( const_cast<char_data *>( ch )->affected[i]->type == bit ) 
        return const_cast<char_data *>( ch )->affected[i]->duration;

  return 0;
}


/*
 *   ROOM AFFECTS
 */

void add_affect( room_data *room, affect_data *paf )
{
  for( int i = 0; i < room->affected; ++i ) {
    if( paf->type == room->affected[i]->type
	&& paf->target == room->affected[i]->target ) {
      affect_data *paf_new = room->affected[i];
      if( paf->duration >= paf_new->duration ) {
	if( event_data *event = find_event( room, paf_new ) ) {
	  unlink( event );
	  paf_new->duration = paf->duration;
	  add_queue( event, affect_delay( ) );
	} else {
	  room->affected -= paf_new;
	  delete paf_new;
	  paf_new = 0;
	}
      }
      return;
    }
  }

  affect_data *paf_new = new affect_data( *paf );
  
  paf_new->leech = 0;
  
  room->affected += paf_new;
  
  if( exit_data *door = exit( paf->target ) ) {
    act_room( room, aff_room_table[ paf_new->type ].msg_on, 0, 0, 0, 0, door );
    exit_data *back = reverse( door );
    if( back ) {
      act_room( door->to_room, aff_room_table[ paf_new->type ].msg_on, 0, 0, 0, 0, back );
      if( paf_new->location == APPLY_BOTH_SIDES ) {
	set_bit( back->affected_by, paf_new->type );
      }
    }
    set_bit( door->affected_by, paf_new->type );
  } else {
    act_room( room, aff_room_table[ paf_new->type ].msg_on, 0 );
  }

  event_data *event = new event_data( update_affect, room );
  event->pointer = paf_new;
  add_queue( event, affect_delay( ) );
}


void remove_affect( room_data *room )
{
  for( int i = room->affected.size-1; i >= 0; i-- ) {
    remove_affect( room, room->affected[i] );
  }
}


void remove_affect( room_data *room, affect_data *affect, bool msg )
{
  room->affected -= affect;

  if( exit_data *door = exit( affect->target ) ) {
    act_room( room, aff_room_table[ affect->type ].msg_off, 0, 0, 0, 0, door );
    exit_data *back = reverse( door );
    if( back ) {
      act_room( door->to_room, aff_room_table[ affect->type ].msg_off, 0, 0, 0, 0, back );
      if( affect->location  == APPLY_BOTH_SIDES ) {
	remove_bit( back->affected_by, affect->type );
      }
    }
    remove_bit( door->affected_by, affect->type );
  } else if( msg ) {
    act_room( room, aff_room_table[ affect->type ].msg_off, 0 );
  }

  if( event_data *event = find_event( room, affect ) ) {
    extract( event );
  }
  
  delete affect;
}


int affect_level( const room_data *room, int sn, const exit_data *exit )
{
  for( int i = 0; i < room->affected; ++i ) {
    const affect_data *paf = room->affected[i];
    if( paf->type == sn ) {
      if( !exit || ((exit_data*)paf->target) == exit ) {
	return paf->level;
      }
    }
  }

  return 0;
}
