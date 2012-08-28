#include <ctype.h>
#include <sys/types.h>
#include <stdio.h>
#include <syslog.h>
#include "define.h"
#include "struct.h"


void        death_cry       ( char_data* );
static obj_data*   make_corpse     ( char_data*, Content_Array& );
static void        loot_corpse     ( obj_data*, char_data* );


/*
 *   HAS LIVE-SAVING?
 */ 


static bool can_die( char_data* victim )
{
  // Mobs can always die.
  if( victim->species )
    return true;

  obj_data*    obj;
  room_data*  room;

  // Can't really die in the arena.
  if( ( room = Room( victim->array->where ) )
      && is_set( room->room_flags, RFLAG_ARENA ) ) {
    fsend( *victim->array, "In a flash of light %s disappears.", victim );
    victim->hit = 1;
    update_pos( victim );
    victim->From( );
    victim->To( get_temple( victim ) );
    dismount( victim );
    victim->position = POS_RESTING;
    return false;
  }  
  
  // Imms can't really die.
  if( victim->Level() >= LEVEL_BUILDER ) {
    fsend_seen( victim, "The body of %s slowly fades out of existence.",
		victim );
    rejuvenate( victim );
    update_pos( victim );
    fsend( *victim->array,
	   "A swirling mist appears and %s slowly reforms.", victim );
    send( victim, "You find yourself alive again.\n\r" );
    return false;
  }
  
  for( int i = 0 ; ; ++i ) {
    if( i >= victim->affected ) {
      for( int j = 0; ; ++j ) {
        if( j >= victim->wearing )
          return true;
        obj = (obj_data*) victim->wearing[j];
        if( is_set( obj->pIndexData->affect_flags, AFF_LIFE_SAVING ) )
          break;
      }
      fsend( *victim->array,
	     "%s that %s was wearing starts to glow a deep purple!",
	     obj, victim );
      fsend_seen( victim, "The body of %s slowly disappears!", victim );
      obj->Extract( 1 ); 
      break;
    }

    // Protect life spell.
    if( victim->affected[i]->type == AFF_LIFE_SAVING
	&& number_range( 0, 100 ) > 60-2*victim->affected[i]->level ) {
      fsend_seen( victim,
		 "%s disappears in an explosion of light and energy.", victim );
      break;
    }
  }  
  
  victim->From( );
  
  remove_leech( victim );
  remove_affect( victim );
  
  rejuvenate( victim );
  
  victim->To( get_temple( victim ) );
  dismount( victim );
  remove_bit( victim->status, STAT_HOLD_POS );
  victim->position = POS_RESTING;
  
  fsend_seen( victim,
	      "%s slowly materializes. From %s appearance %s obviously came close to death.",
	     victim, victim->His_Her( ), victim->He_She( ) );
  send( victim, "You wake up confused and dazed, but seem alive despite your memories.\n\r" );

  return false;
}


/*
 *   DEATH HANDLER
 */


static void register_death( char_data* victim, char_data* ch, const char *dt )
{
  if( victim->species ) {
    if( is_set( victim->status, STAT_PET )
	&& victim->leader
	&& victim->leader->pcdata ) {
      player_log( victim->leader, "%s [PET] killed by %s at %s.",
		  victim->Name( ),
		  ch ? ch->Name( ) : dt,
		  victim->Location( ) );
    }
    for( mprog_data *mprog = victim->species->mprog; mprog; mprog = mprog->next ) {
      if( mprog->trigger == MPROG_TRIGGER_DEATH ) {
	push( );
	clear_variables( );
        var_mob  = victim;
        var_ch   = ch;
        var_room = Room( victim->array->where );
        mprog->execute( );
	pop( );
      }
    }
    return;
  }

  char tmp1 [ TWO_LINES ];
  char tmp2 [ TWO_LINES ];

  snprintf( tmp1, TWO_LINES, "%s killed by %s.", victim->Name( ),
	    ch ? ch->Name( ) : dt );
  snprintf( tmp2, TWO_LINES, "%s killed by %s at %s.", victim->Name( ),
	    ch ? ch->Name( ) : dt, victim->Location( ) );
  info( 0, tmp1, LEVEL_APPRENTICE, tmp2, IFLAG_DEATHS, 1, victim );

  player_log( victim, "Killed by %s at %s.",
	      ch ? ch->real_name( ) : dt, victim->Location( ) );
  
  if( ch && ch->pcdata ) {
    player_log( ch, "Pkilled %s at %s.",
		victim->real_name( ), victim->Location( ) );
  }

  // Do this after player_log() or it shows the death after lost levels.
  const int exp = death_exp( victim, ch );
  add_exp( victim, -exp, "You lose %d exp for dying.\n\r" );
}


static void player_kill( player_data *pl )
{
  remove_leech( pl );
  remove_affect( pl );
  
  dismount( pl );
  leave_shadows( pl );
  remove_bit( pl->status, STAT_SNEAKING );
  remove_bit( pl->status, STAT_COVER_TRACKS );

  remove_bit( pl->pcdata->pfile->flags, PLR_TRACK );
  remove_bit( pl->pcdata->pfile->flags, PLR_SEARCHING );

  pl->position = POS_RESTING;
  pl->hit = max( 1, pl->hit  );
  pl->mana = max( 1, pl->mana );
  pl->move = max( 1, pl->move );
 
  pl->condition[ COND_FULL ] = max( 25, pl->condition[ COND_FULL ] );
  pl->condition[ COND_THIRST ] = max( 25, pl->condition[ COND_THIRST ] );

  affect_data affect; 
  affect.type = AFF_DEATH;
  affect.duration = 20;
  affect.level = 10;
  add_affect( pl, &affect );

  delete_list( pl->prepare );
  update_maxes( pl );

  pl->To( get_room_index( ROOM_DEATH, false ) );

  clear_enemies( pl );
}


void death( char_data* victim, char_data* ch, const char *dt )
{
  char tmp  [ TWO_LINES ];
  Content_Array& where = *victim->array;
  char_data *rch;
  
  if( !ch ) {
    for( int i = 0; i < where; i++ ) {
      if( ( rch = character( where[i] ) )
	  && rch->aggressive.includes( victim ) ) {
        ch = rch;
        break;
      }
    }
  }
  
  stop_fight( victim );
  clear_queue( victim );
  
  remove_bit( victim->status, STAT_HOLD_POS );

  if( !can_die( victim ) )
    return;
 
  if( ( !victim->species || !is_set( victim->status, STAT_PET ) )
      && ( !ch || ch->Level() < LEVEL_APPRENTICE ) ) {
    ++victim->shdata->deaths;
    
    if( victim->species
	&& victim->species->shdata != victim->shdata ) {
      ++victim->species->shdata->deaths;
    }
  }

  if( ch
      && ( !ch->species || ( !victim->species
			     && !is_set( ch->status, STAT_PET ) ) ) ) {
    ++ch->shdata->kills;
    if( ch->species
	&& ch->species->shdata != ch->shdata ) {
      ++ch->species->shdata->kills;
    }
  }

  disburse_exp( victim );
  register_death( victim, ch, dt );

  if( !victim->Is_Valid( ) )
    return;

  clear_queue( victim );
  death_cry( victim );
  
  char_data *looter = ch;
  if( ch
      && is_set( ch->status, STAT_PET )
      && ch->leader
      && ch->leader->in_room == victim->in_room )
    looter = ch->leader;
  
  victim->From( );

  player_data *pl = player( victim );

  if( pl )
    player_kill( pl );

  obj_data *corpse = make_corpse( victim, where );

  if( corpse ) {
    if( victim->species ) {
      loot_corpse( corpse, looter );
    } else if( room_data *room = Room( where.where ) ) {
      if( !is_set( room->room_flags, RFLAG_NO_PKILL )
	  && looter
	  && !looter->species ) {
	// PKill, items are up for grabs.
	set_owner( corpse->contents, 0, victim );
      }
    }
  }

  if( ch
      && ch->species ) {
    for( mprog_data *mprog = ch->species->mprog; mprog; mprog = mprog->next ) {
      if( mprog->trigger == MPROG_TRIGGER_KILL ) {
	push( );
	clear_variables( );
	var_mob = ch;
	var_victim = victim;
	var_obj = corpse;
        var_room = Room( ch->in_room );
	mprog->execute( );
	pop( );
      }
    }
  }

  // For the future... some players could die forever and not return here...
  // Death limits for CE's, for example...
  if( pl /*&& !die_forever*/ ) {
    pl->Save( );
    /*
    // Save the corpse after the owning player.
    // This prevents "Autosave forced." message.
    if( corpse && corpse->owner) {
      corpse->owner->Save( );
    }
    */
    return;
  }

  if( victim->species ) {
    // Is it a player pet?
    // If so, need to save the player after extracting the pet.
    if( is_set( victim->status, STAT_PET ) )
      pl = player( victim->leader );
    victim->Extract( );
    if( pl )
      pl->Save( );
    /*
    // Save the corpse after the owning player.
    // This prevents "Autosave forced." message.
    if( corpse && corpse->owner) {
      corpse->owner->Save( );
    }
    */
    return;
  }

  /*
   *   Players who die forever.
   *   Shouldn't currently happen.
   */

  snprintf( tmp, TWO_LINES, "%s's soul is taken by death.", victim->Name( ) );
  info( 0, empty_string, 0, tmp, IFLAG_DEATHS, 1, victim );
  
  //  clear_screen( victim );
  reset_screen( victim );
  
  send( victim, "Death is surprisingly peaceful.\n\r" );
  send( victim, "Good night.\n\r" );

  purge( pl );
}


/* 
 *   DEATH CRY
 */


void death_message( char_data* victim )
{
  char_data*     rch;

  for( int i = 0; i < *victim->array; i++ ) {
    if( !( rch = character( victim->array->list[i] ) )
	|| !rch->link
	|| rch == victim
	|| !victim->Seen( rch ) ) 
      continue;
    const char *name = victim->Name( rch );
    const int name_len = strlen(name)+10;
    //    snprintf( tmp, ONE_LINE, "+-- %*s --+", name_len, "" );
    //    snprintf( tmp1, ONE_LINE, "%s is DEAD!!", name );
    send( rch, "  " );
    send_color( rch, COLOR_BOLD_RED, "+-- %*s --+", name_len, "" );
    send( rch, "\n\r      " );
    send_color( rch, COLOR_BOLD_RED, "%s is DEAD!!", name );
    send( rch, "\n\r  " );
    send_color( rch, COLOR_BOLD_RED, "+-- %*s --+", name_len, "" );        
    send( rch, "\n\r" );
  }

  send( victim, "You have been KILLED!!\n\r" );
}


void death_cry( char_data* ch )
{
  room_data *room = Room( ch->array->where );
 
  if( !room )
    return;

  if( can_talk( ch ) ) {
    const char *const msg = ch->species
      ? "You hear something's death cry.\n\r"
      : "You hear someone's death cry.\n\r";
    
    bool underwater = is_submerged( 0, room );
    
    for( int i = 0; i < room->exits; ++i ) {
      room_data *to = room->exits[i]->to_room;
      if( underwater ^ is_submerged( 0, to ) ) {
	continue;
      }
      for( int i = 0; i < to->contents; ++i ) {
	if( char_data *rch = character( to->contents[i] ) ) {
	  if( rch->Can_Hear( false ) ) {
	    send( rch, msg );
	  }
	}
      }
      //      send( to->contents, msg );
    }
  }

  if( is_set( ch->status, STAT_FAMILIAR ) && ch->leader ) {
    send( ch->leader, "You sense your familiar's death!\n\r" );
  }
}


/*
 *   CORPSE ROUTINES
 */


obj_data *make_corpse( char_data* ch, Content_Array& where )
{
  /* GHOSTS */

  if( ch->species && is_set( ch->species->act_flags, ACT_GHOST ) ) {
    ch->wearing.To( ch->contents );
    ch->To( where );
    send_publ( ch, &ch->contents, "fades out of existence", "dropping" );
    ch->contents.To( where );
    ch->From( );

    return 0;
  }

  /* CREATE CORPSE */

  obj_data*          corpse;
  obj_clss_data*   obj_clss;
  thing_data*         thing;

  if( ch->species ) {
    if( is_set( ch->status, STAT_PET )
	&& ch->leader
	&& !ch->leader->species ) {
      corpse = create( get_obj_index( OBJ_CORPSE_PET ) );
      corpse->owner = ch->leader->pcdata->pfile;
      ch->leader->pcdata->pfile->corpses += corpse;
    } else {
      if( !( obj_clss = get_obj_index( ch->species->corpse ) ) ) {
	ch->wearing.To( ch->contents );
	ch->To( where );
	send_publ( ch, &ch->contents, "disintegrates", "dropping" );
	ch->contents.To( where );
	ch->From( );
	return 0;
      }
      corpse = create( obj_clss );
      if( obj_clss->item_type == ITEM_CORPSE )
	corpse->value[1] = ch->species->vnum;
    }
  } else {
    corpse = create( get_obj_index( OBJ_CORPSE_PC ) );
    corpse->owner = ch->pcdata->pfile;
    ch->pcdata->pfile->corpses += corpse;
  }

  /* WEIGHT */

  if( corpse->pIndexData->item_type == ITEM_CORPSE ) {
    if( corpse->pIndexData->weight == 0 ) {
      corpse->weight = ch->Empty_Weight( );
      // Save weight for partial eating.
      corpse->value[2] = corpse->weight;
    }
  }
  
  /* NAME CORPSE */

  if( !strncmp( corpse->pIndexData->singular, "corpse of", 9 ) ) {
    char* tmp = static_string( );
    
    if( ch->descr->name != empty_string ) {
      snprintf( tmp, THREE_LINES, "corpse of %s", ch->descr->name );
      corpse->singular = alloc_string( tmp, MEM_OBJECT );
      const char *name = ch->descr->name;
      if( !strncasecmp( name, "the ", 4 ) )
	name += 4;
      else if( !strncasecmp( name, "an ", 3 ) )
	name += 3;
      else if( !strncasecmp( name, "a ", 2 ) )
	name += 2;
      snprintf( tmp, THREE_LINES, "corpses of %s", ch->descr->name );
      corpse->plural = alloc_string( tmp, MEM_OBJECT );
    } else {
      snprintf( tmp, THREE_LINES, "corpse of %s", ch->Name( 0 ) );
      corpse->singular = alloc_string( tmp, MEM_OBJECT );
      const char *singular = separate( ch->descr->singular, true );
      if( *singular == '!' )
	++singular;
      snprintf( tmp, THREE_LINES, "%s corpses", singular );
      corpse->plural = alloc_string( tmp, MEM_OBJECT );
    }
  }
 
  /* TRANSFER ITEMS TO CORPSE */

  if( corpse->pIndexData->item_type == ITEM_CORPSE ) {
    for( int i = ch->wearing-1; i >= 0; i-- ) {
      if( ch->species || number_range( 0,10 ) == 0 ) {
	thing = ch->wearing[i];
	thing = thing->From( thing->Number( ) );
	thing->To( corpse );
      }
    }
    
    for( int i = ch->contents-1; i >= 0; i-- ) {
      if( ch->species || number_range( 0,10 ) == 0 ) {
	thing = ch->contents[i];
	thing = thing->From( thing->Number( ) );
	thing->To( corpse );
      }
    }
  }

  corpse->To( where );

  return corpse;
}


static void loot_corpse( obj_data *corpse, char_data *ch )
{
  if( !ch
      || ch->species
      || !corpse
      || corpse->array->where != ch->in_room )
    return;

  char_data *looter = ch;
  char_data *skinner = ch;
  int level = level_setting( &ch->pcdata->pfile->settings, SET_AUTOLOOT );

  if( is_set( ch->status, STAT_IN_GROUP )
      && !is_set( ch->status, STAT_GROUP_LOOTER ) ) {
    char_data *gloot = 0;
    char_data *leader = group_leader( ch );
    if( is_set( leader->status, STAT_GROUP_LOOTER ) ) {
      if( leader->in_room == ch->in_room
	  && leader->position > POS_SLEEPING
	  && corpse->Seen( leader ) ) {
	gloot = leader;
      }
    } else {
      for( int i = 0; i < leader->followers; ++i ) {
	char_data *rch = leader->followers[i];
	if( is_set( rch->status, STAT_IN_GROUP )
	    && is_set( rch->status, STAT_GROUP_LOOTER ) ) {
	  if( rch->in_room == ch->in_room
	      && rch->position > POS_SLEEPING
	      && corpse->Seen( rch ) ) {
	    gloot = rch;
	  }
	  break;
	}
      }
    }
    if( gloot ) {
      int glev = level_setting( &gloot->pcdata->pfile->settings, SET_AUTOLOOT );
      if( glev != 0 ) {
	if( !corpse->Seen( ch )
	    || level == 0
	    || consenting( ch, leader )
	    || ch->Befriended( gloot ) ) {
	  looter = gloot;
	  level = glev;
	}
      }
      if( is_set( gloot->pcdata->pfile->flags, PLR_AUTO_SKIN ) ) {
	if( !corpse->Seen( ch )
	    || !is_set( ch->pcdata->pfile->flags, PLR_AUTO_SKIN )
	    || consenting( ch, leader )
	    || ch->Befriended( gloot ) ) {
	  skinner = gloot;
	}
      }
    }
  }

  if( corpse->Seen( looter ) ) {
    switch( level ) {
    case 0:
      break;
    case 1:
      select( corpse->contents, looter );
      send_priv( looter, &corpse->contents, "contains", corpse ); 
      break;
    case 2:
    case 3:
      if( corpse->contents.is_empty() ) {
	fsend( looter, "%s contains nothing.", corpse );
      } else {
	int amount = 0;
	const bool split = is_set( looter->pcdata->pfile->flags, PLR_AUTO_SPLIT );
	thing_array loot;
	obj_data *obj;
	const bool no_copper = is_set( looter->pcdata->pfile->flags, PLR_NO_COPPER );
	for( int j = 0 ; j < corpse->contents.size ; ++j ) {
	  if( ( obj = object( corpse->contents[j] ) )
	      && ( level > 2
		   || obj->pIndexData->item_type == ITEM_MONEY )
	      && ( !no_copper || obj->pIndexData->vnum != OBJ_COPPER ) ) {
	    obj->Select_All( );
	    loot += obj;
	    if( split && obj->pIndexData->item_type == ITEM_MONEY ) {
	      for( int i = 0; i < MAX_COIN; ++i ) {
		if( obj->pIndexData->vnum == coin_vnum[i] ) {
		  amount += obj->Number( ) * coin_value[i];
		}
	      }
	    }
	  }
	}
	
	get_obj( looter, loot, corpse, false );
	
	if( amount > 1 && split ) 
	  split_money( looter, amount, false );
	
	if( level == 2 && corpse->contents > 0 ) {
	  select( corpse->contents, looter );
	  send_priv( looter, &corpse->contents, "contains", corpse );
	}
	
	set_delay( looter, 20 );
      }
      break;
    }
  }

  if( corpse->Seen( skinner )
      && is_set( skinner->pcdata->pfile->flags, PLR_AUTO_SKIN ) ) {
    skin( skinner, corpse, true );
  }
}


/*
 *   SLAY ROUTINE
 */


void do_slay( char_data* ch, const char *argument )
{
  char_data*  victim;
    
  if( ch->Level() < LEVEL_BUILDER ) {
    send( ch, "To prevent abuse you are unable to use slay in mortal form.\n\r" );
    return;
  }

  if( !( victim = one_character( ch, argument, "slay",
				 ch->array ) ) )
    return;
  
  if( ch == victim ) {
    send( ch, "Suicide is a mortal sin.\n\r" );
    return;
  }
  
  if( !victim->species
      && ( get_trust( victim ) >= get_trust( ch )
	   || !has_permission( ch, PERM_PLAYERS ) ) ) {
    send( ch, "You failed.\n\r" );
    return;
  }
  
  fsend( ch, "You slay %s in cold blood!", victim );
  fsend( victim, "%s slays you in cold blood!", ch );
  fsend_seen( ch, "%s slays %s in cold blood!", ch, victim );
  
  victim->From();

  player_data *pl = player( victim );

  if( pl )
    player_kill( pl );

  make_corpse( victim, *ch->array );

  // Note: Slaying should not count against any limit on player deaths.
  if( pl ) {
    pl->Save( );
    /*
    // Save the corpse after the owning player.
    // This prevents "Autosave forced." message.
    if( corpse && corpse->owner) {
      corpse->owner->Save( );
    }
    */
    return;
  }

  if( victim->species ) {
    // Is it a player pet?
    // If so, need to save the player after extracting the pet.
    if( is_set( victim->status, STAT_PET )
	&& victim->leader
	&& !victim->leader->species ) {
      pl = ((player_data*) victim->leader );
    }
    victim->Extract( );
    if( pl )
      pl->Save( );
    /*
    // Save the corpse after the owning player.
    // This prevents "Autosave forced." message.
    if( corpse && corpse->owner) {
      corpse->owner->Save( );
    }
    */
    //    return;
  }
}


/*
 *   EXTRACTION ROUTINES
 */


static void dereference( char_data* wch, char_data* ch )
{
  if( !wch->Is_Valid( ) )
    return;

  if( wch->cast ) {
    if( wch->cast->target == ch ) {
      disrupt_spell( wch );
    }
    //    wch->cast->audience -= ch;	// Done by From() -> stop_fight().
  }

  if( wch->fighting == ch ) {
    set_fighting( wch, 0 );
  }

  for( int i = wch->events.size-1; i >= 0; --i ) {
    event_data *event = wch->events[i];
    if( event->func == execute_path ) {
      path_data *path = (path_data*) event->pointer;
      if( path->summoner == ch )
	path->summoner = 0;
      if( path->notify == ch )
	path->notify = 0;
    }
  }

  wch->seen_by -= ch;
  wch->known_by -= ch;

  wizard_data *imm;

  if( ( imm = wizard( wch ) ) ) {
    if( imm->player_edit == ch ) {
      send( imm, "The player you were editing just quit.\n\r" );
      imm->player_edit = 0;
      imm->account_edit = 0;
    }
    if( imm->docking == ch ) {
      send( imm, "The player you were docking just quit.\n\r" );
      imm->docking->docker = 0;
      imm->docking = 0;
    }
  }
  
  if( player_data *pc = player( wch ) ) {
    if( pc->reply == ch ) {
      if( is_set( pc->status, STAT_REPLY_LOCK ) ) {
	fsend( pc, "%s has quit - reply lock removed.", ch );
	remove_bit( pc->status, STAT_REPLY_LOCK );
      }
      pc->reply = 0;
    }

    if( pc->familiar == ch ) {
      pc->familiar = 0;
    }

    if( pc->docker == ch ) {
      wizard( pc->docker )->docking = 0;
      pc->docker = 0;
    }
  }
} 


void player_data :: Extract( )
{
  // Prevent counting extracted objects.
  const int boot_save = boot_stage;
  boot_stage = 1;
  char_data::Extract( );
  boot_stage = boot_save;
}


void char_data :: Extract( )
{
  /* REMOVE WORLD REFERENCES */

  if( !Is_Valid( ) ) {
    roach( "Extract Char: Extracting invalid character." ); 
    return;
  }

  if( species ) {
    if( link ) 
      do_return( this, "" );
    unregister_reset( (mob_data*) this );
  }
  
  if( link ) {
    link->character = 0;
    link->player = 0;
    link = 0;
  }
  
  if( this->array ) {
    was_in_room = 0;
    From( );
  }
  
  remove_leech( this );
  remove_affect( this );

  /* CLEAR EVENTS */  
  clear_queue( this );
  unlink( &active );
  unlink( &update );
  unlink( &regen );
  active.time = -1;
  remove_bit( status, STAT_WAITING );
  stop_events( this );
  
  player_data *pc = player( this );

  /* EXTRACT OBJECTS */
  if( pc ) {
    remove( request_app, pc );
    remove( request_imm, pc );

    /*
    unlink( &pc->hunger );
    pc->hunger.time = -1;
    unlink( &pc->thirst );
    pc->thirst.time = -1;
    unlink( &pc->drunk );
    pc->drunk.time = -1;
    */

    // Clear the save list, don't wait for ~Save_Data(), since
    // ch persists in extracted list for a bit.
    for( int i = 0; i < pc->save_list; ++i ) {
      pc->save_list[i]->save = 0;
    }
    pc->save_list.clear( );

    extract( pc->locker );
    extract( pc->junked );
  }
  
  extract( contents );
  extract( wearing );
  
  /* REMOVE FOLLOWERS */
  
  if( leader ) {
    if( player_data *pc = player( leader ) ) { 
      if( pc->familiar == this )
	pc->familiar = 0;
    }
    leader->followers -= this;
    leader = 0;
  }
  
  for( int i = 0; i < followers; ++i ) {
    char_data *wch = followers[i];
    if( is_set( wch->status, STAT_IN_GROUP ) ) {
      fsend( wch, "%s disbands the group.", Seen_Name( wch ) );
      remove_bit( wch->status, STAT_GROUP_LOOTER );
    }
    fsend( wch, "You stop following %s.", Seen_Name( wch ) );
    wch->leader = 0;
    if( wch->species && is_set( wch->status, STAT_PET ) ) {
      if( !species ) {
	wch->Show( 1 );
	fsend_seen( wch,
		    "%s is gone, although you didn't see %s leave.",
		    wch, wch->Him_Her( ) );
	wch->Extract( );
      } else {
	remove_bit( wch->status, STAT_PET );
	remove_bit( wch->status, STAT_TAMED );
	remove_bit( wch->status, STAT_FAMILIAR );
	remove_bit( wch->status, STAT_IN_GROUP );
	assign_bit( wch->status, STAT_AGGR_ALL,
		    is_set( wch->species->act_flags, ACT_AGGR_ALL ) );
	assign_bit( wch->status, STAT_AGGR_GOOD,
		    is_set( wch->species->act_flags, ACT_AGGR_GOOD ) );
	assign_bit( wch->status, STAT_AGGR_EVIL,
		    is_set( wch->species->act_flags, ACT_AGGR_EVIL ) );
	assign_bit( wch->status, STAT_AGGR_LAWFUL,
		    is_set( wch->species->act_flags, ACT_AGGR_LAWFUL ) );
	assign_bit( wch->status, STAT_AGGR_CHAOTIC,
		    is_set( wch->species->act_flags, ACT_AGGR_CHAOTIC ) );
	free_string( ((mob_data*)wch)->pet_name, MEM_MOBS );
	((mob_data*)wch)->pet_name = empty_string; 
      }
    }
  }
  
  followers.clear( );
  
  /* REMOVE REFERENCES ON OTHER PLAYERS */

  for( int i = 0; i < player_list; ++i ) 
    dereference( player_list[i], this );

  for( int i = 0; i < mob_list; ++i ) 
    dereference( mob_list[i], this );

  dismount( this );
  dismount( rider, POS_RESTING );
  
  /*
   *   FREE MEMORY
   */
  
  //remove_leech( this );
  //remove_affect( this );

  /*
  for( int i = 0; i < affected; i++ ) {
    affect_data *affect = affected[i]; 
    if( affect->leech ) {
      if( affect->leech != this ) {
	fsend( affect->leech, "Leech for %s on %s dissipated.",
	       aff_char_table[affect->type].name, affect->target );
      }
      remove_leech( affect );
    }
  }
  */

  // May still be some innate affects in the list.
  affected.delete_list();

  delete_list( enemy );
  delete_list( prepare );
  
  cmd_queue.clear();
  known_by.clear( );
  seen_by.clear( );
  seen_exits.clear( );

  position = POS_EXTRACTED;
  valid    = -1;

  extracted += this;
}
