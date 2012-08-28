#include <sys/types.h>
#include <stdio.h>
#include "define.h"
#include "struct.h"


string_array extract_strings;
event_array extract_events;


static void cleanup( );
static void maint( );

/*
void    drunk_update     ( char_data* );
bool    plague_update    ( char_data* );
void    poison_update    ( char_data* );
*/

/*
 *   TIMED ACTIONS
 */


static void room_update( )
{
  time_data start;
  gettimeofday( &start, 0 );
  
  random_update( );
  
  pulse_time[ TIME_RNDM_ACODE ] = stop_clock( start );  
}


// Player-related timings.
static void player_update( )
{
  for( int i = 0; i < player_list; ++i ) {
    player_data *pc = player_list[i];
    if( !pc->In_Game( ) )
      continue;
    // Average 10 minutes per autosave.
    if( pc->save_time+600 < current_time
	&& !wizlock
	&& !godlock ) {
      if( is_set( pc->pcdata->message, MSG_AUTOSAVE ) )
	send( pc, "Autosaving...\n\r" );
      pc->Save( );
    }
    
    const int idle = current_time - pc->timer;
    link_data *link =  pc->switched  ? pc->switched->link : pc->link;
    
    // 5 mins to void if link alive, 30 secs if link dead.
    if( pc->Level() < LEVEL_APPRENTICE 
	&& idle > ( link ? 300 : 30 ) ) {
      if( !pc->was_in_room && pc->in_room ) {
	if( pc->switched ) 
	  do_return( pc->switched, "" );
	char *buf = static_string();
	snprintf( buf, THREE_LINES, "%s has voided at %s.",
		  pc->descr->name,
		  pc->Location( ) );
	info( LEVEL_IMMORTAL, empty_string, invis_level( pc ), buf, IFLAG_LOGINS, 3, pc );
	send( pc, "Your mind wanders, and you lose awareness of your surroundings.\n\r" );
	fsend_seen( pc, "%s is gone, although you didn't see %s leave.", pc, pc->Him_Her() );
	pc->Save( );
	room_data *room = pc->in_room;
	pc->From( );
	pc->To( get_room_index( ROOM_LIMBO ) );
	pc->was_in_room = room;
      } else if( idle > 900 ) {
	// 15 mins idle to forced quit.
	forced_quit( pc );
	return;
      }
    }
  }
}


/*
 *   MAIN UPDATE FUNCTION
 */


void update_handler( void )
{
  static unsigned pulse_area = 1;
  static unsigned pulse_mobile = 1;
  static unsigned pulse_violence = 1;
  static unsigned pulse_room = 1;
  static unsigned pulse_cleanup = PULSE_CLEANUP;
  static unsigned pulse_maint = 1;

  time_data start;
  gettimeofday( &start, 0 );

  event_update( );
  extracted.delete_list();

  for( int i = 0; i < extract_strings; ++i ) {
    free_string( extract_strings[i], MEM_QUEUE );
  }
  extract_strings.clear( );

  extract_events.delete_list();

  if( --pulse_area == 0 ) {
    pulse_area = number_range( PULSE_AREA/2, 3*PULSE_AREA/2 );
    area_update( );
  }

  // Every 6 real time seconds == 1 MUD minute.
  if( --pulse_mobile == 0 ) {
    pulse_mobile = PULSE_MOBILE;
    weather.update( );
    player_update( );
    action_update( );
    auction_update( );
  }

  if( --pulse_room == 0 ) {
    pulse_room = number_range( PULSE_ROOM/2, 3*PULSE_ROOM/2 );
    room_update( );
  }

  // Every 2 seconds real time.
  if( --pulse_violence == 0 ) {
    pulse_violence = PULSE_VIOLENCE;
    update_queue( queue_list );
  }

  if( --pulse_cleanup == 0 ) {
    cleanup( );
    pulse_cleanup = PULSE_CLEANUP;
  }

  if( --pulse_maint == 0 ) {
    maint( );
    pulse_maint = PULSE_MAINT;
  }

  update_queue( now_list );

  pulse_time[ TIME_UPDATE ] = stop_clock( start );
}


/*
 *   UPDATE CHARACTERS
 */


static bool plague_update( char_data* ch )
{
  if( !ch->is_affected( AFF_PLAGUE ) )
    return false;

  affect_data *aff = 0;

  for( int i = 0; i < ch->affected; ++i ) {
    if( ch->affected[i]->type == AFF_PLAGUE ) {
      aff = ch->affected[i];
      break;
    }
  }

  if( !aff && !ch->species ) {
    bug( "Plague_update: plagued player with no affect data." );
    bug( "-- Char = %s", ch );
    remove_bit( ch->affected_by, AFF_PLAGUE );
    return false;
  }

  if( aff && aff->modifier > -2 ) {
    fsend( *ch->array, "%s coughs violently.", ch );
    if( aff->modifier == 0 ) 
      send( ch, "You cough violently.\n\r" );
    else
      send( ch, "You cough, your throat burning in pain.\n\rThis is much worse than any cold, you have the plague!\n\r" );
  } else {
    fsend( ch, "You cough violently as the plague continues to destroy your body." );
    fsend( *ch->array,
	   "%s coughs violently, looking very ill.",
	   ch );
  }

  if( aff ) {
    if( ch->Constitution( ) == 3 ) {
      if( !ch->species ) {
	send( ch, "You succumb to the plague!\n\r" );
	fsend_seen( ch, "%s succumbs to the plague!", ch );
	death( ch, 0, "the plague" );
	return true;
      }
    } else {
      --aff->modifier;
      affect_data aff_tmp;
      aff_tmp.location = APPLY_CON;
      aff_tmp.modifier = -1;
      modify_affect( ch, &aff_tmp, true );
    }
  }

  // Contagion.
  room_data *room = ch->in_room; 
  if( room
      && !is_set( room->room_flags, RFLAG_SAFE ) ) {
    char_data *victim;
    for( int i = 0; i < room->contents; ++i ) {
      if( ( victim = character( room->contents[i] ) )
	  && victim != ch
	  && victim->is_humanoid( )
	  && ( !victim->is_affected( AFF_PLAGUE ) || number_range( 1, 8 ) == 1 )
	  && number_range( 1, 2 ) == 1 ) {
	plague( victim, number_range( 5, 12 ), false );
      }
    }
  }

  return false;
}


static bool poison_update( char_data* ch )
{
  if( ch->is_affected( AFF_POISON ) ) {
    send( ch, "Your condition deteriorates from a poison affliction.\n\r" );
    fsend_seen( ch,
		"%s's condition deteriorates from a poison affliction.", ch );
    return inflict( ch, 0, number_range( 1, 4 ), "poison", true );
  }

  return false;
}


static void update( char_data* ch )
{
  if( !plague_update( ch )
      && !poison_update( ch ) ) {
    
    if( ch->position == POS_INCAP ) {
      inflict( ch, 0, 1, "" );
    } else if( ch->position == POS_MORTAL ) {
      if( ch->array && ch->hit <= -9 ) {
	fsend( *ch->array, "%s succumbs to %s wounds.", ch, ch->His_Her( ) );
      }
      bool in_death = ch->in_room ? ( ch->in_room->vnum == ROOM_DEATH ) : false;
      if( inflict( ch, 0, 2, "bleeding to death" )
	  && !ch->species
	  && in_death ) {
	send( ch, "Closing your link for your own protection.\n\r" );
	disconnect( ch );
      }
    }
  }
}


static void update( mob_data *mob )
{
  if( mob->position == POS_EXTRACTED || !mob->in_room )
    return;

  /*
  if( mob->timer > 0 && --mob->timer == 0 ) 
    for( mprog_data *mprog = mob->species->mprog; mprog; mprog = mprog->next ) 
      if( mprog->trigger == MPROG_TRIGGER_TIMER ) {
        clear_variables( );
        var_mob  = mob;
        var_room = mob->in_room;
        mprog->execute( );
	return;
      }
  */

  if( mob->maturity > 0 && --mob->maturity == 0 ) {
    species_data *species = get_species( mob->species->adult );
    if( species ) {
      if( species->size > mob->in_room->size ) {
	// If the new mob doesn't fit in the room, it doesn't happen. Yet.
	mob->maturity = 1;
      } else {
	/*
	Content_Array *where = mob->array;
	mob_data *mature = new Mob_Data( species );
	mature->To( where );
	share_enemies( mob, mature );
	if( mob->fighting ) {
	  init_attack( mature, mob->fighting );
	}
	mature->hit = mature->max_hit * mob->hit / mob->max_hit;
	mature->move = mature->max_move * mob->move / mob->max_move;
	mature->mana = mature->max_mana * mob->mana / mob->max_mana;
	update_maxes( mature );
	fsend( *mob->array, "%s becomes %s.", mob, mature );
	*/
	// *** See also: char.cc:mob_data().
	Content_Array &where = *mob->array;
	for( int i = 0; i < where; ++i ) {
	  if( char_data *rch = character( where[i] ) ) {
	    if( rch != mob
		&& mob->Seen( rch ) ) {
	      fsend( rch, "%s becomes %s.",
		     mob->Long_Name( rch, 1, false ),
		     species->Name( mob->known_by.includes( rch ), false, false ) );
	    }
	  }
	}
	//      fsend_seen( mob, "%s becomes %s.", mob, species->Name( false, false, false ) );
	//      mob->known_by.clear( );
	mob->thing_data::From( );
	if( mob->shdata != mob->species->shdata ) {
	  delete mob->shdata;
	}
	if( mob->descr != mob->species->descr ) {
	  delete mob->descr;
	}
	mob->shdata = species->shdata;
	mob->descr = species->descr;
	mob->color = species->color;
	mob->species = species;
	mob->base_hit  = max( 1, dice_data( species->hitdice ).roll( ) );
	mob->base_move = max( 0, dice_data( species->movedice ).roll( ) );
	mob->base_mana = 100;
	update_maxes( mob );
	mob->maturity = species->maturity;
	mob->thing_data::To( where );
      }
    } else {
      for( int i = 0; i < *mob->array; ++i ) {
	if( char_data *rch = character( mob->array->list[i] ) ) {
	  if( rch != mob
	      && mob->Seen( rch ) ) {
	    fsend( rch, "%s suddenly vanishes.",
		   mob->Long_Name( rch, 1, false ) );
	  }
	}
      }
      mob->Extract( );
    }
    return;
  }

  if( condition_update( mob ) )
    return;

  update( (char_data*) mob );
}


static void update( player_data *player )
{
  if( !player->In_Game( )
      || player->was_in_room )
    return;
  
  if( player->gossip_pts < 1000
      && number_range( 0, player->pcdata->trust >= LEVEL_AVATAR ? 3 : 5 ) == 0 )
    ++player->gossip_pts;
  
  player->shdata->fame  = max( --player->shdata->fame, 0 );
  player->whistle       = max( --player->whistle, 0 );
  player->prayer        = min( ++player->prayer, 1000 );

  if( condition_update( player ) )
    return;

  update( (char_data*) player );
}


/*
 *   CHARACTER UPDATE SUBROUTINES
 */


void execute_update( event_data *event )
{
  char_data *ch = (char_data*) event->owner;

  ch->Show( 1 );

  if( ch->species ) {
    update( (mob_data*) ch );
  } else {
    update( (player_data*) ch );
  }

  set_update( ch );
}


/*
 *   AFFECT UPDATES
 */


static void room_message( obj_data *obj, const char *text )
{
  if( !obj->array || !obj->array->where )
    return;

  thing_data *th = obj->array->where;

  char_data *ch = character( th );
  room_data *room = Room( th );

  if( ch && !room ) {
    if( obj->array != &ch->contents
	&& obj->array != &ch->wearing )
      return;
    if( !ch->array || !ch->array->where )
      return;
    room = Room( ch->array->where );
  }

  if( !room )
    return;


  if( ch ) {
    act( ch, text, ch, 0, obj );
    act_notchar( text, ch, 0, obj );
  } else {
    act_room( room, text, 0, 0, obj );
  }
}


void update_affect( event_data* event )
{
  thing_data *owner = (thing_data*) event->owner;
  affect_data *affect = (affect_data*) event->pointer;

  if( obj_data *obj = object( owner ) ) {
    // Object affects.

    obj->Select_All( );

    if( Tprog_Data *tprog = aff_obj_table[ affect->type ].prog ) {
      push( );
      clear_variables( );
      var_obj = obj;
      var_i = -( affect->duration - 1 );
      const int result = tprog->execute( );
      pop( );
      if( !result
	  || !obj->Is_Valid( ) ) {
	return;
      }
    }

    if( affect->type == AFF_BURNING ) {
      if( ( obj->condition -= 10 ) <= 0 ) {
	room_message( obj, "$r is reduced to ashes." );
	obj->Extract( );
	return;

      } else if( obj->pIndexData->item_type == ITEM_FOOD
	  && obj->value[1] >= COOK_RAW
	  && obj->value[1] < COOK_BURNT) {
	if( ++obj->value[1] == COOK_BURNT ) {
	  remove_affect( obj, affect, false );
	  return;
	}
      }
    } else if( --affect->duration <= 0 ) {
      remove_affect( obj, affect );
      return;
    }

  } else if( char_data *ch = character( owner ) ) {
    // Character affects.

    ch->Select( 1 );

    if( !affect->leech ) {
      if( --affect->duration == 1 ) {
	if( affect->type != AFF_NONE
	    && affect_level( ch, affect->type, affect ) == 0 ) {
	  act( ch, aff_char_table[ affect->type ].msg_fade, ch );
	  act_social_room( aff_char_table[ affect->type ].msg_fade_room, ch );
	}
      }
      if( affect->type == AFF_DEATH ) {
        update_max_hit( ch );
        update_max_move( ch );
      }
      if( affect->duration <= 0 ) {
        remove_affect( ch, affect );
	return;
      }
    }

  } else if( room_data *room = Room( owner ) ) {
    // Room affects.

    if( !affect->leech ) {
      if( --affect->duration == 1 ) {
	if( affect->type != AFF_NONE ) {
	  exit_data *door = exit( affect->target );
	  act_room( room, aff_room_table[ affect->type ].msg_fade, 0, 0, 0, 0, door );
	  if( door ) {
	    if( exit_data *back = reverse( door ) ) {
	      act_room( door->to_room, aff_room_table[ affect->type ].msg_fade, 0, 0, 0, 0, back );
	    }
	  }
	}
      }
      if( affect->duration <= 0 ) {
        remove_affect( room, affect );
	return;
      }
    }

  } else {
    bug( "Update_Affect: owner is not an object, character, or room." );
    bug( "-- Owner = %s.", owner );
    extract( event );
    return;
  }

  add_queue( event, affect_delay( ) );
}


/*
 *   RESETTING OF AREAS
 */


void area_update( )
{
  struct timeval start;

  gettimeofday( &start, 0 );

  for( int i = 0; i < max_clan; i++ ) 
    if( clan_list[i]->modified )
      save_clans( clan_list[i] );

  for( area_data *area = area_list; area; area = area->next ) {
    if( ++area->age < 15
	&& ( area->nplayer != 0 || area->age < 5 ) )
      continue;
    
    area->update_forage( );

    for( room_data *room = area->room_first; room; room = room->next ) {
      if( !player_in_room( room ) ) {
	reset_room( room );
	room->Save();
      }
    }
    
    area->age = number_range( 0, 3 );
    
    if( area->modified ) {
      area->Save( );
    }
  }
  
  shop_update( );

  pulse_time[ TIME_RESET ] = stop_clock( start );
}


// Hourly cleanup.
void cleanup( )
{
  struct timeval start;
  gettimeofday( &start, 0 );

  unsigned acodes = 0;
  unsigned mprogs = 0;
  unsigned maprogs = 0;
  unsigned oprogs = 0;
  unsigned area_save = 0;
  unsigned area_clear = 0;
  unsigned act_save = 0;
  unsigned act_clear = 0;

  for( area_data *area = area_list; area; area = area->next ) {
    if( !area->used ) {
      for( room_data *room = area->room_first; room; room = room->next ) {
	for( action_data *action = room->action; action; action = action->next ) {
	  if( action->binary ) {
	    action->decompile( );
	    ++acodes;
	  }
	}
      }
    } else {
      area->used = false;
    }
    if( !area->seen ) {
      if( area->loaded ) {
	if( area->dirty ) {
	  area->save_text( );
	  ++area_save;
	}
	area->clear_text( );
	++area_clear;
      }
    } else {
      area->seen = false;
    }
    if( !area->act_used ) {
      if( area->act_loaded ) {
	if( area->act_dirty ) {
	  area->save_actions( );
	  ++act_save;
	}
	area->clear_actions( );
	++act_clear;
      }
    } else {
      area->act_used = false;
    }
  }

  for( int i = 1; i <= species_max; ++i ) {
    if( species_data *species = species_list[i] ) {
      if( !species->used ) {
	if( species->attack->binary ) {
	  species->attack->decompile( );
	  ++maprogs;
	}
	for( mprog_data *mprog = species->mprog; mprog; mprog = mprog->next ) {
	  if( mprog->binary ) {
	    mprog->decompile( );
	    ++mprogs;
	  }
	}
      } else {
	species->used = false;
      }
    }
  }

  for( int i = 1; i <= obj_clss_max; ++i ) {
    if( obj_clss_data *obj_clss = obj_index_list[i] ) {
      if( !obj_clss->used ) {
	for( oprog_data *oprog = obj_clss->oprog; oprog; oprog = oprog->next ) {
	  if( oprog->binary ) {
	    oprog->decompile( );
	    ++oprogs;
	  }
	}
      } else {
	obj_clss->used = false;
      }
    }
  }

  if( acodes || maprogs || mprogs || oprogs ) {
    char *buf = static_string( );
    char *b = buf;

    b += snprintf( buf, THREE_LINES, "Hourly cleanup removed binaries: " );
    if( acodes ) {
      b += sprintf( b, "%d acode%s",
		    acodes,
		    acodes == 1 ? "" : "s" );
    }
    if( maprogs ) {
      b += sprintf( b, "%s%d mob attack prog%s",
		    acodes ? ", " : "",
		    maprogs,
		    maprogs == 1 ? "" : "s" );
    }
    if( mprogs ) {
      b += sprintf( b, "%s%d mprog%s",
		    acodes || maprogs ? ", " : "",
		    mprogs,
		    mprogs == 1 ? "" : "s" );
    }
    if( oprogs ) {
      b += sprintf( b, "%s%d oprog%s",
		    acodes || maprogs || mprogs ? ", " : "",
		    oprogs,
		    oprogs == 1 ? "" : "s");
    }
    b += sprintf( b, "." );
    info( LEVEL_BUILDER, empty_string, 0, buf, IFLAG_MAINT, 3 );
  }

  if( area_clear ) {
    char *buf = static_string( );
    char *b = buf;

    b += snprintf( b, THREE_LINES,
		   "Hourly cleanup removed room desc text: %d area%s cleared",
		   area_clear,
		   area_clear == 1 ? "" : "s" );

    if( area_save ) {
      b += sprintf( b, ", %d swapped to disk", area_save );
    }

    b += sprintf( b, "." );
    info( LEVEL_BUILDER, empty_string, 0, buf, IFLAG_MAINT, 3 );
  }

  if( act_clear ) {
    char *buf = static_string( );
    char *b = buf;

    b += snprintf( b, THREE_LINES,
		   "Hourly cleanup removed acode source text: %d area%s cleared",
		   act_clear,
		   act_clear == 1 ? "" : "s" );

    if( act_save ) {
      b += sprintf( b, ", %d swapped to disk", act_save );
    }

    b += sprintf( b, "." );
    info( LEVEL_BUILDER, empty_string, 0, buf, IFLAG_MAINT, 3 );
  }

  pulse_time[ TIME_CLEANUP ] = stop_clock( start );
}


// Daily maintenance.
void maint( )
{
  struct timeval start;
  gettimeofday( &start, 0 );

  // 1. Purge idle players.
  for( int i = max_pfile-1; i >= 0; --i ) {
    pfile_data *pfile = pfile_list[i];
    if( pfile->trust < LEVEL_APPRENTICE
	&& pfile->level > 0
	&& current_time > pfile->last_on
	                  + 2*weeks( pfile->level )
	                  + 20*weeks( pfile->remort ) ) {
      if( !find_player( pfile ) ) {
	/*
      int j = 0;
      for( ; j < player_list; ++j ) {
	if( player_list[j]->pcdata->pfile == pfile ) {
	  break;
	}
      }
      if( j == player_list ) {
	*/
	link_data *link = new link_data;
	link->connected = CON_PLAYING;
	if( !load_char( link, pfile->name, PLAYER_DIR ) ) {
	  bug( "Maintenance: Non-existent player file (%s)", pfile->name );
	  delete link;
	  continue;
	}
	char *buf = static_string( );
	player_data *pl = link->player;
	snprintf( buf, THREE_LINES, "Daily maintenance purged idle player %s (%d), last on %s.",
		  pfile->name,
		  pfile->level,
		  ltime( pfile->last_on )
		  );
	info( LEVEL_BUILDER, empty_string, 0, buf, IFLAG_MAINT, 2 );
	pl->link = 0;
	purge( pl );
	delete link;
      }
    }
  }

  // 2. Purge idle accounts.
  bool save = false;
  for( int i = max_account-1; i >= 0; --i ) {
    account_data *account = account_list[i];
    const link_data *link;
    for( link = link_list; link; link = link->next ) {
      if( link->account == account ) {
	break;
      }
    }
    if( link )
      continue;
    if( account->banned == -1 ) {
      bool no_players = true;
      for( int j = 0; j < max_pfile; ++j ) {
	pfile_data *pfile = pfile_list[j];
	if( account == pfile->account ) {
	  no_players = false;
	  break;
	}
      }
      if( no_players ) {
	if( account->no_players == -1 ) {
	  account->no_players = current_time;
	  save = true;
	} else if( current_time > account->no_players + weeks( 2 ) ) {
	  char *buf = static_string( );
	  snprintf( buf, THREE_LINES, "Daily maintenance purged idle account %s (%s), no players since %s.",
		    account->name,
		    *account->email ? account->email : "no validated email",
		    ltime( account->no_players )
		    );
	  info( LEVEL_BUILDER, empty_string, 0, buf, IFLAG_MAINT, 2 );
	  extract( account );
	  save = true;
	}
      } else if( account->no_players != -1 ) {
	account->no_players = -1;
	save = true;
      }
    } else if( account->no_players != -1 ) {
      account->no_players = -1;
      save = true;
    }
  }

  if( save )
    save_accounts( );

  pulse_time[ TIME_MAINT ] = stop_clock( start );
}
