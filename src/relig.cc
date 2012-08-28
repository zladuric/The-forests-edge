#include <sys/types.h>
#include <stdio.h>
#include <syslog.h>
#include "define.h"
#include "struct.h"


static int find_diety( char_data *ch, const char *& argument )
{
  for( int i = 1; i < table_max[ TABLE_RELIGION ]; i++ ) {
    if( matches( argument, religion_table[i].name ) )
      return i;
  }

  send( ch, "Unknown diety.\n\r" );
  return 0;
}


void do_pray( char_data* ch, const char *argument )
{
  char_data*     victim  = ch;
  player_data*       pc  = player( ch );

  if( is_mob( ch ) ) 
    return;

  if( ch->pcdata->religion == REL_NONE ) {
    send( ch, "You are not the follower of any god and cannot expect aid.\n\r" );
    return;
  }
  
  if( *argument
      && !( victim = one_character( ch, argument, "pray for", ch->array ) ) ) 
    return;
  
  /*
  if( ch->shdata->alignment == ALGN_CHAOTIC_EVIL ) {
    send( ch,
	  "As a chaotic evil, the gods will never answer your prayers.\n\r" );
    return;
  }
  */

  // For wimpy pray.
  disrupt_spell( ch );
  set_delay( ch, 20 );

  if( victim != ch ) {
    fsend( ch, "You pray to %s for the salvation of %s.",
	   religion_table[ ch->pcdata->religion ].name,
	   victim );
    fsend( victim, "%s prays to %s for your salvation.",
	   ch,
	   religion_table[ ch->pcdata->religion ].name );
    fsend_seen( ch, "%s prays to %s for the salvation of %s.",
		ch,
		religion_table[ ch->pcdata->religion ].name,
		victim );
  } else {
    fsend( ch, "You pray to %s for salvation.",
	   religion_table[ ch->pcdata->religion ].name );
    fsend_seen( ch, "%s prays to %s for salvation.",
		ch,
		religion_table[ ch->pcdata->religion ].name );
  }

  if( is_set( ch->in_room->room_flags, RFLAG_NO_PRAY ) ) {
    fsend( ch,
	   "As you pray, a sudden chill touches your soul.\
  You doubt there will be any aid forthcoming." );
    send_seen( ch, "Nothing happens.\n\r" );
    return;
  }
  
  int prayer = pc->prayer;
  int factor = 10;

  if( victim->is_affected( AFF_CURSE )
      || ch->is_affected( AFF_CURSE ) )
    ++factor;

  if( victim->is_affected( AFF_BLESS )
      || ch->is_affected( AFF_BLESS ) )
    --factor;

  if( victim != ch ) {
    if( !player( victim ) )
      ++factor;
    else {
      if( victim->pcdata->religion != ch->pcdata->religion )
	factor += 2;
    }
    factor += ch->Align_Distance( victim );
  }

  const int need = victim->fighting ? victim->max_hit/4 : victim->max_hit/3;

  if( prayer >= 15*factor && victim->hit < need ) {
    prayer -= 15*factor;
    victim->hit = victim->max_hit;
    update_pos( victim );
    update_max_move( victim );
    victim->move = victim->max_move;
    send_color( victim, COLOR_WIZARD,
		"A dim light surrounds you curing your wounds." );
    send( victim, "\n\r" );
    fsend_color( *victim->array, COLOR_WIZARD,
		 "A dim light surrounds %s curing %s wounds.", 
		 victim, victim->His_Her( ) );
  } 
  
  if( victim->is_affected( AFF_BLIND ) && prayer >= 20*factor ) {
    prayer -= 20*factor;
    strip_affect( victim, AFF_BLIND );
  }
  
  if( victim->is_affected( AFF_POISON ) && prayer >= 15*factor ) {
    prayer -= 15*factor;
    strip_affect( victim, AFF_POISON );
  }
  
  if( victim == ch ) {
    if( ch->condition[COND_FULL] < 0 && prayer >= 3*factor ) {
      prayer -= 3*factor;
      ch->condition[COND_FULL] = 30;
      send_color( ch, COLOR_WIZARD, "Your stomach feels full." );
      send( ch, "\n\r" );
    }
    
    if( ch->condition[COND_THIRST] < 0 && prayer >= 3*factor ) {
      prayer -= 3*factor;
      ch->condition[COND_THIRST] = 30;
      send_color( ch, COLOR_WIZARD, "You no longer feel thirsty." );
      send( ch, "\n\r" );
    } 
    
    if( ch->in_room->is_dark( ch )
	&& prayer >= 3*factor
	&& !ch->is_affected( AFF_INFRARED )
	&& !ch->is_affected( AFF_BLIND ) ) {
      prayer -= 3*factor;
      obj_data *obj = create( get_obj_index( OBJ_BALL_OF_LIGHT ) );
      obj->light = obj->value[0]/6 + 1;
      set_bit( obj->extra_flags, OFLAG_NO_AUCTION );
      set_bit( obj->extra_flags, OFLAG_NO_SELL );
      set_bit( obj->extra_flags, OFLAG_NOSACRIFICE );
      obj->To( ch );
      fsend_color( ch, COLOR_WIZARD, "%s appears in your hand.", obj );
      fsend_color_seen( ch, COLOR_WIZARD, "%s appears in %s's hand.", obj, ch );
    }
    
    if( ch->move < 10 && ch->max_move > 20 && prayer >= 2*factor ) {
      prayer -= 2*factor;
      ch->move = ch->max_move;
      send_color( ch, COLOR_WIZARD, "You feel rejuvenated." );
      send( ch, "\n\r" );
    }
  }
  
  if( prayer == pc->prayer ) {
    send( ch, "Your prayer goes unanswered.\n\r" );
    send_seen( ch, "Nothing happens.\n\r" );
  }
  
  pc->prayer = max( 0, prayer );
}
  

/*
 *   SACRIFICE 
 */


void do_sacrifice( char_data* ch, const char *argument )
{
  char             arg  [ MAX_INPUT_LENGTH ];
  obj_data*        obj;
  int                i;

  if( is_mob( ch ) ) 
    return;

  const bool any_altar = is_set( ch->in_room->room_flags, RFLAG_ALTAR );
  const bool good_altar = is_set( ch->in_room->room_flags, RFLAG_ALTAR_GOOD );
  const bool neut_altar = is_set( ch->in_room->room_flags, RFLAG_ALTAR_NEUTRAL );
  const bool evil_altar = is_set( ch->in_room->room_flags, RFLAG_ALTAR_EVIL );
  const bool law_altar = is_set( ch->in_room->room_flags, RFLAG_ALTAR_LAW );
  const bool neut2_altar = is_set( ch->in_room->room_flags, RFLAG_ALTAR_NEUTRAL2 );
  const bool chaos_altar = is_set( ch->in_room->room_flags, RFLAG_ALTAR_CHAOS );

  if( !any_altar
      && !good_altar
      && !neut_altar
      && !evil_altar
      && !law_altar
      && !neut2_altar
      && !chaos_altar ) {
    send( ch, "Sacrifices will only be recognized at altars.\n\r" );
    return;
  }

  if( !*argument ) {
    send( ch, "The gods do not value empty sacrifices.\n\r" );
    return;
  }

  if( !two_argument( argument, "to", arg ) ) {
    if( ch->pcdata->religion != REL_NONE ) {
      argument = one_argument( argument, arg );
      i = ch->pcdata->religion;
    } else {
      send( ch, "Syntax: sacrifice <object> [to] <diety>.\n\r" );
      return;
    }
  } else {
    if( !( i = find_diety( ch, argument ) ) )
      return;
    if( ch->pcdata->religion == REL_NONE || i != ch->pcdata->religion ) {
      // Can't sacrifice to opposed dieties.
      if( !is_set( religion_table[i].alignments, ch->shdata->alignment ) ) {
	fsend( ch, "%s does not accept sacrifices from those of your alignment.",
	       religion_table[i].name );
	return;
      }
      if( !is_set( religion_table[i].classes, ch->pcdata->clss ) ) {
	fsend( ch, "%s does not accept sacrifices from those of your profession.",
	       religion_table[i].name );
	return;
      }
      if( !is_set( religion_table[i].sexes, ch->sex ) ) {
	fsend( ch, "%s does not accept sacrifices from those of your gender.",
	       religion_table[i].name );
	return;
      }
      if( ch->shdata->race >= MAX_PLYR_RACE
	  || !is_set( religion_table[i].races, ch->shdata->race ) ) {
	fsend( ch, "%s does not accept sacrifices from those of your race.",
	       religion_table[i].name );
	return;
      }
    }
  }

  if( !( obj = one_object( ch, arg, "sacrifice", 
			   ch->array,
			   &ch->contents ) ) )
    return;
 
  /*
  if( obj->array == &ch->contents
      &&!obj->droppable( ) ) {
    fsend( ch, "You can't let go of %s.", obj );
    return;
  }
  */

  if( obj->array != ch->array ) {
    fsend( ch, "You must drop %s first to sacrifice it.", obj );
    return;
  }

  if( !can_wear( obj, ITEM_TAKE ) ) {
    fsend( ch,
	  "%s is immovable and this makes the required ritual impossible.",
	  obj );
    return;
  }
  
  if( obj->contents != 0 ) {
    fsend( ch, "%s is not empty.", obj );
    return;
  }

  if( is_set( obj->extra_flags, OFLAG_NOSACRIFICE ) ) {
    fsend( ch, "%s is forbidden from sacrifice.", obj );
    fsend( ch, "%s grows angry at your impudence!", religion_table[i].name );
    spell_affect( 0, ch, 10, 0, SPELL_CURSE, AFF_CURSE );
    return;
  }

  if( obj->reset ) {
    fsend( ch, "%s grows angry at your impudence!", religion_table[i].name );
    spell_affect( 0, ch, 10, 0, SPELL_CURSE, AFF_CURSE );
    return;
  }

  if( !forbidden( obj, ch ) ) {
    fsend( ch, "You are forbidden from sacrificing %s.", obj );
    fsend( ch, "%s grows angry at your impudence!", religion_table[i].name );
    spell_affect( 0, ch, 10, 0, SPELL_CURSE, AFF_CURSE );
    return;
  }
  
  const int align = ch->Align_Good_Evil( );
  const int align2 = ch->Align_Law_Chaos( );

  if( !any_altar ) {
    if( align == 0 && !good_altar
	|| align == 1 && !neut_altar
	|| align == 2 && !evil_altar
	|| align2 == 0 && !law_altar
	|| align2 == 1 && !neut2_altar
	|| align2 == 2 && !chaos_altar ) {
      fsend( ch, "%s disappears with a flash.", obj );
      fsend_seen( ch, "%s disappears with a flash.", obj );
      send( ch, "You hear the gods' faint laughter in the distance.\n\r" );
      send( ch, "Perhaps you shouldn't have sacrificed here.\n\r" );
      spell_affect( 0, ch, 10, 0, SPELL_CURSE, AFF_CURSE );
      obj->Extract( 1 );
      return;
    }
  }

  if( ch->pcdata->religion != REL_NONE
      && ch->pcdata->religion != i
      && ( ch->pcdata->clss == CLSS_CLERIC
	   || ch->pcdata->clss == CLSS_PALADIN
	   || ch->pcdata->clss == CLSS_DRUID ) ) {
    fsend( ch, "%s disappears with a flash.", obj );
    fsend_seen( ch, "%s disappears with a flash.", obj );
    fsend( ch, "%s grows angry at your impudence!",
	   religion_table[ch->pcdata->religion].name );
    spell_affect( 0, ch, 10, 0, SPELL_CURSE, AFF_CURSE );
    obj->Extract( 1 );
    return;
  }

  for( action_data *action = ch->in_room->action; action; action = action->next ) 
    if( action->trigger == TRIGGER_SACRIFICE
	&& obj->pIndexData->vnum == action->value ) {
      clear_variables( );
      var_ch = ch;
      var_obj = obj;
      var_room = ch->in_room;
      if( !action->execute( ) )
	return;
      break;
    }

  if( ch->pcdata->religion != REL_NONE
      && ch->pcdata->religion != i ) {
    fsend( ch, "You stop worshipping %s.",
	   religion_table[ch->pcdata->religion].name );
    player( ch )->prayer = 0;
    ch->pcdata->religion = 0;
  }

  fsend( ch, "You sacrifice %s to %s.", obj,
	 religion_table[i].name );
  fsend_seen( ch, "%s sacrifices %s to %s.",
	      ch, obj, religion_table[i].name );

  int cost = obj->Cost( )/10;
  int corpse = 0;
  int corp_align = -1;
  int corp_align2 = -1;

  if( obj->pIndexData->item_type == ITEM_CORPSE ) {
    corpse = obj->Weight(1) / 5000;
    if( const species_data *species = get_species( obj->value[1] ) ) {
      corpse += species->shdata->level;
      corp_align = species->shdata->alignment % 3;
      corp_align2 = species->shdata->alignment / 3;
    }
  }

  bool bonus, penalty;

  if( any_altar ) {
    bonus = align == 0 && good_altar
      || align == 1 && neut_altar
      || align == 2 && evil_altar
      || align2 == 0 && law_altar
      || align2 == 1 && neut2_altar
      || align2 == 2 && chaos_altar;
    penalty = corp_align == 0 && good_altar
      || corp_align == 1 && neut_altar
      || corp_align == 2 && evil_altar
      || corp_align2 == 0 && law_altar
      || corp_align2 == 1 && neut2_altar
      || corp_align2 == 2 && chaos_altar;
  } else {
    bonus = corp_align == 0 && !good_altar
      || corp_align == 1 && !neut_altar
      || corp_align == 2 && !evil_altar
      || corp_align2 == 0 && !law_altar
      || corp_align2 == 1 && !neut2_altar
      || corp_align2 == 2 && !chaos_altar;
    penalty = corp_align == 0 && good_altar
      || corp_align == 1 && neut_altar
      || corp_align == 2 && evil_altar
      || corp_align2 == 0 && law_altar
      || corp_align2 == 1 && neut2_altar
      || corp_align2 == 2 && chaos_altar;
  }

  if( bonus && !penalty ) {
    cost = 3 * cost / 2;
    corpse = 3 * corpse / 2;
  } else if( penalty && !bonus ) {
    cost = 2 * cost / 3;
    corpse = 2 * corpse / 3;
  }

  if( cost == 0
      && corpse == 0 ) {
    send( ch, "Nothing happens.\n\rApparently your sacrifice has been rejected.\n\r" );
    send_seen( ch, "Nothing happens.\n\rApparently the sacrifice has been rejected.\n\r", ch );
    return;
  }

  if( player_data *pc = player( ch ) ) {
    if( is_set( obj->extra_flags, OFLAG_MAGIC ) )
      pc->reputation.magic += (cost+9)/10;
    pc->reputation.gold += cost;
    pc->reputation.blood += corpse;
  }

  fsend( ch, "%s disappears with a flash.", obj );
  fsend_seen( ch, "%s disappears with a flash.", obj );

  send( ch, "Apparently your sacrifice has been accepted.\n\r" );
  send_seen( ch, "Apparently the sacrifice has been accepted.\n\r", ch );

  ch->pcdata->religion = i;  
  modify_pfile( ch );

  obj->Extract( 1 );
}


void do_religion( char_data* ch, const char *argument )
{
  if( is_mob( ch ) ) 
    return;

  if( !*argument ) {
    display_array( ch, "+Religions",
		   &religion_table[1].name, &religion_table[2].name,
		   table_max[ TABLE_RELIGION ] - 1 );
    return;
  }

  const int i = find_diety( ch, argument );

  if( !i )
    return;

  bool found = false;

  int k = 0;
  for( int j = 0; j < max_pfile; ++j ) {
    pfile_data *pfile = pfile_list[j];
    if( pfile->religion == i
	&& !is_incognito( pfile, ch ) ) {
      if( !found ) {
	page_title( ch, "Followers of %s", religion_table[i].name );
	found = true;
      }
      page( ch, "%15s", pfile_list[j]->name );
      if( k++ % 5 == 4 ) {
	page( ch, "\n\r" );
      }
    }
  }

  if( k % 5 != 0 ) {
    page( ch, "\n\r" );
  }

  if( !found ) {
    fsend( ch, "%s has no known followers.", religion_table[i].name );
  }
}
