#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include "define.h"
#include "struct.h"


bool song_of_morale( char_data* ch, char_data *victim, void *vo,
		     int level, int duration )
{
  char_array *audience = (char_array*)vo;

  /*
  room_data *room = spell_room( ch, victim );

  if( !room ) {
    bug( "%s: Null room.", skill_entry(SONG_OF_MORALE)->name );
    return false;
  }
  */

  bool add = false;

  for( int i = 0; i < *audience; ++i ) {
    if( player_data *pl = player( audience->list[i] ) )
      if( pl->Can_Hear( true )
	  && invis_level( pl ) == 0 ) {
	if( spell_affect( ch, pl, level, duration,
			  SONG_OF_MORALE, AFF_MORALE ) )
	  add = true;
      }
  }

  return add;
}


bool song_of_heroism( char_data* ch, char_data *victim, void *vo,
		      int level, int duration )
{
  char_array *audience = (char_array*)vo;

  /*
  room_data *room = spell_room( ch, victim );

  if( !room ) {
    bug( "%s: Null room.", skill_entry(SONG_OF_HEROISM)->name );
    return false;
  }
  */

  bool add = false;

  for( int i = 0; i < *audience; ++i ) {
    if( player_data *pl = player( audience->list[i] ) )
      if( pl->Can_Hear( true )
	  && invis_level( pl ) == 0 ) {
	if( spell_affect( ch, pl, level, duration,
			  SONG_OF_HEROISM, AFF_HEROISM ) )
	  add = true;
      }
  }

  return add;
}


bool song_of_zeal( char_data* ch, char_data *victim, void *vo,
		   int level, int duration )
{
  char_array *audience = (char_array*)vo;

  /*
  room_data *room = spell_room( ch, victim );

  if( !room ) {
    bug( "%s: Null room.", skill_entry(SONG_OF_ZEAL)->name );
    return false;
  }
  */

  bool add = false;

  for( int i = 0; i < *audience; ++i ) {
    if( player_data *pl = player( audience->list[i] ) )
      if( pl->Can_Hear( true )
	  && invis_level( pl ) == 0 ) {
	if( spell_affect( ch, pl, level, duration,
			  SONG_OF_ZEAL, AFF_ZEAL ) )
	  add = true;
      }
  }

  return add;
}


bool song_of_valor( char_data* ch, char_data *victim, void *vo,
		   int level, int duration )
{
  char_array *audience = (char_array*)vo;

  /*
  room_data *room = spell_room( ch, victim );

  if( !room ) {
    bug( "%s: Null room.", skill_entry(SONG_OF_VALOR)->name );
    return false;
  }
  */

  bool add = false;

  for( int i = 0; i < *audience; ++i ) {
    if( player_data *pl = player( audience->list[i] ) )
      if( pl->Can_Hear( true )
	  && invis_level( pl ) == 0 ) {
	if( spell_affect( ch, pl, level, duration,
			  SONG_OF_VALOR, AFF_VALOR ) )
	  add = true;
      }
  }

  return add;
}


bool song_of_grace( char_data* ch, char_data *victim, void *vo,
		   int level, int duration )
{
  char_array *audience = (char_array*)vo;

  /*
  room_data *room = spell_room( ch, victim );

  if( !room ) {
    bug( "%s: Null room.", skill_entry(SONG_OF_GRACE)->name );
    return false;
  }
  */

  bool add = false;

  for( int i = 0; i < *audience; ++i ) {
    if( player_data *pl = player( audience->list[i] ) )
      if( pl->Can_Hear( true )
	  && invis_level( pl ) == 0 ) {
	if( spell_affect( ch, pl, level, duration,
			  SONG_OF_GRACE, AFF_GRACE ) )
	  add = true;
      }
  }

  return add;
}


bool song_of_fortitude( char_data* ch, char_data *victim, void *vo,
			int level, int duration )
{
  char_array *audience = (char_array*)vo;

  /*
  room_data *room = spell_room( ch, victim );

  if( !room ) {
    bug( "%s: Null room.", skill_entry(SONG_OF_FORTITUDE)->name );
    return false;
  }
  */

  bool add = false;

  for( int i = 0; i < *audience; ++i ) {
    if( player_data *pl = player( audience->list[i] ) )
      if( pl->Can_Hear( true )
	  && invis_level( pl ) == 0 ) {
	if( spell_affect( ch, pl, level, duration,
			  SONG_OF_FORTITUDE, AFF_FORTITUDE ) )
	  add = true;
      }
  }

  return add;
}


bool song_of_sentinel( char_data* ch, char_data *victim, void *vo,
		       int level, int duration )
{
  char_array *audience = (char_array*)vo;

  /*
  room_data *room = spell_room( ch, victim );

  if( !room ) {
    bug( "%s: Null room.", skill_entry(SONG_OF_SENTINEL)->name );
    return false;
  }
  */

  bool add = false;

  for( int i = 0; i < *audience; ++i ) {
    if( player_data *pl = player( audience->list[i] ) )
      if( pl->Can_Hear( true )
	  && invis_level( pl ) == 0 ) {
	if( spell_affect( ch, pl, level, duration,
			  SONG_OF_SENTINEL, AFF_SENTINEL ) )
	  add = true;
      }
  }

  return add;
}


bool song_of_legends( char_data* ch, char_data *victim, void *vo,
		      int level, int duration )
{
  char_array *audience = (char_array*)vo;

  /*
  room_data *room = spell_room( ch, victim );

  if( !room ) {
    bug( "%s: Null room.", skill_entry(SONG_OF_LEGENDS)->name );
    return false;
  }
  */

  bool add = false;

  for( int i = 0; i < *audience; ++i ) {
    if( player_data *pl = player( audience->list[i] ) )
      if( pl->Can_Hear( true )
	  && invis_level( pl ) == 0 ) {
	if( spell_affect( ch, pl, level, duration,
			  SONG_OF_LEGENDS, AFF_LEGENDS ) )
	  add = true;
      }
  }

  return add;
}


bool song_of_mystics( char_data* ch, char_data *victim, void *vo,
		      int level, int duration )
{
  char_array *audience = (char_array*)vo;

  /*
  room_data *room = spell_room( ch, victim );

  if( !room ) {
    bug( "%s: Null room.", skill_entry(SONG_OF_MYSTICS)->name );
    return false;
  }
  */

  bool add = false;

  for( int i = 0; i < *audience; ++i ) {
    if( player_data *pl = player( audience->list[i] ) )
      if( pl->Can_Hear( true )
	  && invis_level( pl ) == 0 ) {
	if( spell_affect( ch, pl, level, duration,
			  SONG_OF_MYSTICS, AFF_MYSTICS ) )
	  add = true;
      }
  }

  return add;
}


bool song_of_wanderer( char_data* ch, char_data *victim, void *vo,
		      int level, int duration )
{
  char_array *audience = (char_array*)vo;

  /*
  room_data *room = spell_room( ch, victim );

  if( !room ) {
    bug( "%s: Null room.", skill_entry(SONG_OF_WANDERER)->name );
    return false;
  }
  */

  bool add = false;

  for( int i = 0; i < *audience; ++i ) {
    if( player_data *pl = player( audience->list[i] ) )
      if( pl->Can_Hear( true )
	  && invis_level( pl ) == 0 ) {
	if( spell_affect( ch, pl, level, duration,
			  SONG_OF_WANDERER, AFF_WANDERER ) )
	  add = true;
      }
  }

  return add;
}


bool song_of_wind( char_data *ch, char_data *victim, void *vo,
		   int level, int duration )
{
  room_data *room = (room_data*) vo;

  if( !room ) {
    send( ch, "Nothing happens.\n\r" );
    send_seen( ch, "Nothing happens.\n\r" );
    return false;
  }

  if( room == ch->in_room
      || is_submerged( 0, room )
      //      || room->sector_type == SECT_UNDERWATER
      || is_set( ch->in_room->room_flags, RFLAG_NO_RECALL ) ) {
    send( ch, "Nothing happens.\n\r" );
    send_seen( ch, "Nothing happens.\n\r" );
    return false;
  }

  send( ch, "You are swept away by a sudden gust of wind.\n\r" );
  fsend_seen( ch, "A sudden gust of wind carries %s away.", ch );

  ch->From( );
  ch->To( room );

  if( ch->mount ) {
    ch->mount->From( );
    ch->mount->To( room );
  }
  
  fsend_seen( ch, "%s appears, borne upon a sudden, fierce wind.", ch );

  send( ch, "\n\r" );
  show_room( ch, ch->in_room, false, false );

  return true;
}


bool song_of_ward( char_data *ch, char_data *victim, void*,
		   int level, int duration )
{
  if( !victim ) {
    send( ch, "Nothing happens.\n\r" );
    send_seen( ch, "Nothing happens.\n\r" );
    return false;
  }

  room_data *room = victim->in_room;

  if( !room ) {
    bug( "%s: Null room.", skill_entry(SONG_OF_WARD)->name );
    return false;
  }

  if( !consenting( victim, ch, empty_string )
      || room == ch->in_room
      || is_set( ch->in_room->room_flags, RFLAG_NO_RECALL ) ) {
    send( ch, "Nothing happens.\n\r" );
    send_seen( ch, "Nothing happens.\n\r" );
    return false;
  }

  fsend( ch, "You feel yourself pulled toward %s.", victim );
  fsend_seen( ch, "%s suddenly disappears.", ch );

  ch->From( );
  ch->To( room );

  if( ch->mount ) {
    ch->mount->From( );
    ch->mount->To( room );
  }

  fsend_seen( ch, "%s materializes before you.", ch );

  send( ch, "\n\r" );
  show_room( ch, ch->in_room, false, false );

  return true;
}


bool spell_lore( char_data *ch, char_data *victim, void *vo,
		 int level, int duration )
{
  if( duration == -4 )
    return false;

  //  if( null_caster( ch, SPELL_LORE ) )
  //    return false;

  obj_data *obj = (obj_data*) vo;
  //  obj_clss_data *obj_clss = obj->pIndexData;

  // Do not remove pIndexData here: we want to know if the base object is magical.
  if( !is_set( obj->pIndexData->extra_flags, OFLAG_MAGIC ) ) {
    fsend( ch, "You can recall no lore regarding %s.", obj );
    send_seen( ch, "Nothing happens.\n\r" );
    return false;
  }

  return identify( ch, obj );
}


bool spell_bestiary( char_data *ch, char_data *victim, void *vo,
		     int level, int duration )
{
  species_data *species = victim->species;

  if( *species->descr->name
      || !*species->descr->plural
      || species->shdata->race < MAX_PLYR_RACE
      || species->nation != NATION_NONE
      || species->is_affected( AFF_SANCTUARY )
      || is_set( species->act_flags, ACT_USE_THE )
      || is_set( species->act_flags, ACT_GHOST ) ) {
    fsend( ch, "You learn nothing about %s.", victim );
    return false;
  }

  if( ch->is_affected( AFF_HALLUCINATE ) ) {
    send( ch, "In your condition, the bestiary spell cannot be relied upon.\n\r" );
    return false;
  }

  const bool improve = !ch->knows( victim );
  victim->make_known( ch );

  const int skill = ch->get_skill( SPELL_BESTIARY );
  const share_data *shdata = victim->shdata;

  send( ch, scroll_line[0] );
  send_title( ch, victim->Seen_Name( ch ) );
  
  send( ch, "         Level: %-4d             Sex: %s\n\r",
	victim->Level( ), sex_name[ victim->sex ] );

  send( ch, "          Race: %-12s  Nation: %-12s   Align: %s\n\r",
	race_table[ shdata->race ].name,
	nation_table[ species->nation ].name,
	alignment_table[ shdata->alignment ].name );

  send( ch, "          Size: %-10s    Weight: %-.2f lbs\n\r",
	size_name[ species->size ], (double) victim->Empty_Weight( )/100.0 );

  if( skill >= 9 ) {
    send( ch, "         Kills: %-10d    Deaths: %d\n\r",
	  shdata->kills, shdata->deaths );
  }

  if( skill >= 3 ) {
    send( ch, "     Avg. Hits: %-6d    Avg. Moves: %d\n\r",
	  dice_data( species->hitdice ).average( ),
	  dice_data( species->movedice ).average( ) );
  }

  if( skill >= 5 ) {
    if( skill >= 7 ) {
      send( ch, "\n\r     Str: %2d(%2d)  Int: %2d(%2d)  Wis: %2d(%2d)",
	    victim->Strength( ),     shdata->strength,
	    victim->Intelligence( ), shdata->intelligence,
	    victim->Wisdom( ),       shdata->wisdom );
      send( ch, "  Dex: %2d(%2d)  Con: %2d(%2d)\n\r",
	    victim->Dexterity( ),    shdata->dexterity,
	    victim->Constitution( ), shdata->constitution );
    } else {
      send( ch, "\n\r     Str: %2d      Int: %2d      Wis: %2d    ",
	    shdata->strength,
	    shdata->intelligence,
	    shdata->wisdom );
      send( ch, "  Dex: %2d      Con: %2d\n\r",
	    shdata->dexterity,
	    shdata->constitution );
    }
  }

  if( skill == 10 ) {
    show_resists( ch, victim, false );
  }

  if( skill >= 2 ) {
    species_data *blank = get_species( MOB_BLANK );
    bool found = false;
    size_t col = 10;
    for( int j = 0; j < MAX_SKILL_CAT; ++j ) {
      const int m = table_max[ skill_table_number[ j ] ];
      for( int k = 0; k < m; ++k ) {
	if( shdata->skills[j][k] > UNLEARNT
	    && blank->shdata->skills[j][k] == UNLEARNT ) {
	  const char *name = skill_entry( j, k )->name;
	  const size_t len = strlen( name );
	  if( !found ) {
	    send( ch, "\n\r%15s\n\r%10s%s",
		  "Skills:", "",
		  name );
	    found = true;
	  } else if( col+len+3 >= 80 ) {
	    send( ch, ",\n\r%10s%s", "", name );
	    col = 10;
	  } else {
	    send( ch, ", %s", name );
	    col += 2;
	  }
	  col += len;
	}
      }
    }
    if( found ) {
      send( ch, ".\n\r" );
    }
  }

  if( skill >= 8 ) {
    bool found = false;
    size_t col = 10;
    for( int i = 0; i < table_max[ TABLE_AFF_CHAR ]; ++i ) {
      if( i == AFF_HIDE
	  || i == AFF_SNEAK
	  || i == AFF_CAMOUFLAGE )
	continue;
      if( species->is_affected( i ) ) {
	const char *name = aff_char_table[ i ].name;
	const size_t len = strlen( name );
	if( !found ) {
	  send( ch, "\n\r%15s\n\r%10s%s",
		"Affects:", "",
		name );
	  found = true;
	} else if( col+len+3 >= 80 ) {
	  send( ch, ",\n\r%10s%s", "", name );
	  col = 10;
	} else {
	  send( ch, ", %s",
		name );
	  col += 2;
	}
	col += len;
      }
    }
    if( found ) {
      send( ch, ".\n\r" );
    }
  }

  send( ch, "\n\r" );
  send( ch, scroll_line[0] );

  return improve;
}


void do_play( char_data *ch, const char *argument )
{
  if( !*argument ) {
    send( ch, "What do you want to play?\n\r" );
    return;
  }

  obj_data *instrument = one_object( ch, argument, "play", &ch->wearing );

  if( !instrument )
    return;
  
  if( is_entangled( ch, "play" )
      || is_drowning( ch, "play" ) ) {
    return;
  }

  if( instrument->pIndexData->item_type != ITEM_INSTRUMENT ) {
    fsend( ch, "%s is not something you can play.", instrument );
    return;
  }
  
  oprog_data *oprog;

  for( oprog = instrument->pIndexData->oprog; oprog; oprog = oprog->next ) {
    if( oprog->trigger == OPROG_TRIGGER_USE ) {
      clear_variables( );
      var_ch = ch;
      var_room = ch->in_room;
      var_obj = instrument;
      var_def = use_msg;
      var_def_type = ITEM_INSTRUMENT;
      if( !oprog->execute( )
	  || !instrument->Is_Valid( ) )
        return;
      break;
    }
  }
  
  act( ch, prog_msg( oprog, use_msg[4] ), 0, 0, instrument );
  act_notchar( prog_msg( oprog, use_msg[5] ), ch, 0, instrument );

  set_delay( ch, 32 );
}


void do_perform( char_data* ch, const char *argument )
{
  if( is_confused_pet( ch )
      || is_familiar( ch )
      || is_silenced( ch, "perform" )
      || is_entangled( ch, "perform" )
      || is_drowning( ch, "perform" ) ) {
    return;
  }
  
  if( !*argument ) {
    send( ch, "What do you want to perform?\n\r" );
    return;
  }

  int spell;

  if( !find_spell( ch, argument, spell, true ) )
    return;

  if( !is_set( skill_spell_table[spell].location, LOC_PERFORM ) ) {
    fsend( ch, "You can't perform %s; try casting it instead.",
	   skill_spell_table[spell].name );
    return;
  }


  int mana = 0;
  cast_data *prepare  = 0;
  
  if( skill_spell_table[spell].prepare != 0 ) {
    if( !( prepare = has_prepared( ch, spell ) ) ) {
      fsend( ch, "You don't have %s prepared.", skill_spell_table[spell].name );
      return;
    }
  } else if( ( mana = check_mana( ch, spell, "perform" ) ) < 0 ) {
    return;
  }

  if( !allowed_location( ch, &skill_spell_table[spell].location,
			 "perform", skill_spell_table[spell].name ) )
    return;

  cast_data *cast = new cast_data;
  cast->spell = spell;
  cast->prepare = false;
  cast->wait = skill_spell_table[spell].prepare-1;
  cast->mana = mana;

  if( !get_target( ch, cast, argument, "perform", "performing" )
      || !has_reagents( ch, cast ) ) {
    delete cast;
    return;
  }
  
  fsend( ch, "You begin performing %s.", skill_spell_table[spell].name );
  
  if( !ch->species && skill_spell_table[spell].prepare != 0 ) {
    if( --prepare->times == 0 ) { 
      remove( ch->prepare, prepare );
      delete prepare;
    } else if( is_set( ch->pcdata->message, MSG_SPELL_COUNTER ) ) {
      send( ch, "[ You have %s %s performance%s remaining. ]\n\r", 
	    number_word( prepare->times, ch ), skill_spell_table[spell].name, 
	    prepare->times == 1 ? "" : "s" );
    }
  }
  
  ch->cast = cast;
  ch->mana -= mana;

  set_delay( ch, 10 - ch->get_skill( SPELL_FIRST+spell )/2 );  
}
