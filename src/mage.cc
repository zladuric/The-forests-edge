#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include "define.h"
#include "struct.h"


/*
 *   DAMAGE
 */


bool spell_meteor_swarm( char_data* ch, char_data*, void *vo, int level, int )
{
  room_data *room = ch ? ch->in_room : (room_data*)vo;

  if( !room ) {
    bug( "%s: Null caster and room.",
	 skill_spell_table[ skill_number( SPELL_METEOR_SWARM ) ].name );
    return false;
  }

  //  if( null_caster( ch, SPELL_METEOR_SWARM ) )
  //    return false;

  unsigned count = 0;

  for( int i = room->contents.size-1; i >= 0; i-- ) {
    char_data *rch = character( room->contents.list[i] );
    if( rch
	&& rch != ch
	&& can_kill( ch, rch, false ) ) {
      damage_physical( rch, ch, spell_damage( SPELL_METEOR_SWARM, level ),
		       "*The swarm of fiery meteors" );
      ++count;
    }
  }

  if( count == 0 ) {
    send( ch, "Nothing happens.\n\r" );
    return false;
  }

  return true;
}


bool spell_maelstrom( char_data* ch, char_data*, void *vo, int level, int )
{
  room_data *room = ch ? ch->in_room : (room_data*)vo;

  if( !room ) {
    bug( "%s: Null caster and room.",
	 skill_spell_table[ skill_number( SPELL_METEOR_SWARM ) ].name );
    return false;
  }

  //  if( null_caster( ch, SPELL_MAELSTROM ) )
  //    return false;

  unsigned count = 0;

  for( int i = room->contents.size-1; i >= 0; i-- ) {
    char_data *rch = character( room->contents.list[i] );
    if( rch
	&& rch != ch
	&& can_kill( ch, rch, false ) ) {
      damage_magic( rch, ch, spell_damage( SPELL_MAELSTROM, level ),     
		    "*The maelstrom" );
      ++count;
    }
  }

  if( count == 0 ) {
    send( ch, "Nothing happens.\n\r" );
  }

  return true;
}


/*
 *   MISSILE SPELLS
 */


/*
bool spell_magic_missile( char_data* ch, char_data* victim, void*,
			  int level, int )
{
  //  if( null_caster( ch, SPELL_MAGIC_MISSILE ) )
  //    return true;
  
  damage_magic( victim, ch, spell_damage( SPELL_MAGIC_MISSILE, level ),
		"*The magic missile" );

  return true;
}


bool spell_prismic_missile( char_data* ch, char_data* victim, void*,
			    int level, int )
{
  //  if( null_caster( ch, SPELL_PRISMIC_MISSILE ) )
  //    return true;

  damage_magic( victim, ch, spell_damage( SPELL_PRISMIC_MISSILE, level ),
		"*A multi-hued burst of light" );

  return true;
}
*/


/*
 *   SLEEP SPELLS
 */


static bool sleep_affect( char_data* ch, char_data* victim,
			  int level, int duration )
{
  if( ch && !can_kill( ch, victim, false ) )
    return false;
  
  if( victim->is_affected( AFF_SLEEP_RESIST ) ) {
    return false;
  }
  
  if( victim->position < POS_SLEEPING )
    return false;

  if( makes_save( victim, ch, RES_MAGIC, SPELL_SLEEP, level ) ) {
    if( victim->position > POS_SLEEPING ) {
      send( victim, "You feel drowsy but quickly shrug it off.\n\r" );
      fsend_seen( victim, "%s looks drowsy but quickly shrugs it off.", victim );
      return true;
    }
    return false;
  }

  spell_affect( ch, victim, level, duration, SPELL_SLEEP, AFF_SLEEP );

  if( ch && ch->pcdata ) {
    record_damage( victim, ch );    // Provides an experience share.
    stop_fight( victim, true );
  } else {
    //    remove_bit( victim->status, STAT_BERSERK );
    //    remove_bit( victim->status, STAT_FOCUS );
    set_fighting( victim, 0 );
  }

  if( victim->position == POS_SLEEPING ) {
    send( victim, "You slip deeper into slumber.\n\r" );
    fsend_seen( victim, "%s slips deeper into slumber.", victim );
    return true;
  }

  send( victim, "You feel sleepy and suddenly fall asleep!\n\r" );
  sleep( victim );

  if( deep_water( victim ) ) {
    fsend_seen( victim, "%s falls asleep.", victim );    
  } else {
    const char *drop = victim->in_room->drop( );
    if( *drop ) {
      fsend_seen( victim, "%s drops %s asleep.", victim, drop );
    } else {
      fsend_seen( victim, "%s falls asleep.", victim );
    }
  }

  return true;
}


bool spell_sleep( char_data* ch, char_data* victim, void*, int level,
		  int duration )
{
  // Fill, dip.
  if( duration == -4 || duration == -3 )
    return false;

  if( !sleep_affect( ch, victim, level, duration ) ) {
    send( ch, "Nothing happens.\n\r" );
    send_seen( ch, "Nothing happens.\n\r", ch );
    return false;
  }

  if( victim->position != POS_SLEEPING )
    return false;

  if( ch
      && !ch->pcdata
      && ch->fighting == victim ) {
    char_array list1, list2;
    for( int i = 0; i < *ch->array; ++i ) {
      char_data *rch = character( ch->array->list[i] );
      if( rch
	  && rch != victim
	  && rch->position > POS_SLEEPING ) {
	if( rch->fighting == ch ) {
	  list1 += rch;
	} else if( is_aggressive( ch, rch ) ) {
	  list2 += rch;
	}
      }
    }

    char_data *rch = 0;

    if( !list1.is_empty() ) {
      // 50-50 chance of switch if someone else attacking.
      if( number_range( 0, 1 ) == 0 ) {
	rch = list1[ number_range( 0, list1.size-1 ) ];
      }
    } else if( !list2.is_empty() ) {
      // 50-50 chance of switch if someone else to attack.
      if( number_range( 0, 1 ) == 0 ) {
	rch = list2[ number_range( 0, list2.size-1 ) ];
      }
    } else {
      // 50-50 chance of ending fight if only target.
      if( is_set( ch->species->act_flags, ACT_WIMPY )
	  && !is_set( ch->status, STAT_AGGR_ALL )
	  && !( is_evil( victim )
		&& is_set( ch->status, STAT_AGGR_EVIL ) )
	  && !( is_good( victim )
		&& is_set( ch->status, STAT_AGGR_GOOD ) )
	  && !( is_lawful( victim )
		&& is_set( ch->status, STAT_AGGR_LAWFUL ) )
	  && !( is_chaotic( victim )
		&& is_set( ch->status, STAT_AGGR_CHAOTIC ) )
	  && number_range( 0, 1 ) == 0 ) {
	stop_fight( victim, true );
      }
    }

    if( rch ) {
      fsend( rch, "%s stops attacking %s and leaps to attack you.", ch, victim );
      fsend_seen( ch, "%s stops attacking %s and leaps to attack %s.",
		  ch, victim, rch );
      stop_fight( victim );
      if( !set_fighting( ch, rch ) )
	return true;
    }
  }

  return true;
}


bool spell_mists_sleep( char_data* ch, char_data*, void*, int level,
			int duration )
{
  if( null_caster( ch, SPELL_MISTS_SLEEP ) )
    return false;

  if( is_submerged( 0, ch->in_room ) ) {
    //ch && ch->in_room->sector_type == SECT_UNDERWATER ) {
    send( ch, "You are unable to summon the mists underwater.\n\r" );
    return false;
  }
  
  unsigned count = 0;

  for( int i = 0; i < *ch->array; i++ ) {
    if( char_data *rch = character( ch->array->list[i] ) ) {
      rch->Select( 1 );
      if( rch != ch
	  && sleep_affect( ch, rch, level, duration ) ) {
	++count;
      }
    }
  }

  if( count == 0 ) {
    send( ch, "Nothing happens.\n\r" );
    send_seen( ch, "Nothing happens.\n\r", ch );
    return false;
  }

  if( ch
      && !ch->pcdata
      && ch->fighting
      && ch->fighting->position == POS_SLEEPING ) {
    char_array list1, list2;
    for( int i = 0; i < *ch->array; ++i ) {
      char_data *rch = character( ch->array->list[i] );
      if( rch
	  && rch->position > POS_SLEEPING ) {
	if( rch->fighting == ch ) {
	  list1 += rch;
	} else if( is_aggressive( ch, rch ) ) {
	  list2 += rch;
	}
      }
    }

    char_data *rch = 0;

    if( !list1.is_empty() ) {
      // 50-50 chance of switch if someone else attacking.
      if( number_range( 0, 1 ) == 0 ) {
	rch = list1[ number_range( 0, list1.size-1 ) ];
      }
    } else if( !list2.is_empty() ) {
      // 50-50 chance of switch if someone else to attack.
      if( number_range( 0, 1 ) == 0 ) {
	rch = list2[ number_range( 0, list2.size-1 ) ];
      }
    } else {
      // 50-50 chance of ending fight if only target.
      if( is_set( ch->species->act_flags, ACT_WIMPY )
	  && !is_set( ch->status, STAT_AGGR_ALL )
	  && !( is_evil( ch->fighting )
		&& is_set( ch->status, STAT_AGGR_EVIL ) )
	  && !( is_good( ch->fighting )
		&& is_set( ch->status, STAT_AGGR_GOOD ) )
	  && !( is_lawful( ch->fighting )
		&& is_set( ch->status, STAT_AGGR_LAWFUL ) )
	  && !( is_chaotic( ch->fighting )
		&& is_set( ch->status, STAT_AGGR_CHAOTIC ) )
	  && number_range( 0, 1 ) == 0 ) {
	stop_fight( ch->fighting );
      }
    }

    if( rch ) {
      fsend( rch, "%s stops attacking %s and leaps to attack you.", ch, ch->fighting );
      fsend_seen( ch, "%s stops attacking %s and leaps to attack %s.",
		  ch, ch->fighting, rch );
      stop_fight( ch->fighting );
      if( !set_fighting( ch, rch ) )
	return true;
    }
  }

  return true;
}


/*
 *   STRENGTH SPELL
 */


bool spell_ogre_strength( char_data* ch, char_data* victim, void*,
			  int level, int duration )
{
  if( duration == -4 || duration == -3 )
    return false;

  const int str = victim->Strength();
  const int add = 5 - str/5;

  if( add < 1 ) {
    if( ch )
      send( ch, "Nothing happens.\n\r" );
    if( victim != ch ) {
      send( victim, "Nothing happens.\n\r" );
    }
    return false;
  }

  return spell_affect( ch, victim, level, duration,
		       SPELL_OGRE_STRENGTH, AFF_OGRE_STRENGTH, str+add );
}


/*
 *   ENCHANTMENT SPELLS
 */


/*
bool spell_detect_magic( char_data* ch, char_data* victim, void*,
			 int level, int duration )
{
  return spell_affect( ch, victim, level, duration,
		       SPELL_DETECT_MAGIC, AFF_DETECT_MAGIC );
}
*/


bool spell_minor_enchantment( char_data* ch, char_data*, void* vo,
			      int level, int duration )
{
  // Fill.
  if( duration == -4 )
    return false;

  if( null_caster( ch, SPELL_MINOR_ENCHANTMENT ) ) 
    return false;

  obj_data *obj = (obj_data*) vo;

  obj->Select( 1 );
  
  if( is_set( obj->extra_flags, OFLAG_NO_ENCHANT ) ) {
    fsend( ch, "%s cannot be enchanted.", obj );
    return false;
  }

  if( obj->value[0] < 0 ) {
    fsend( *ch->array,
	  "%s starts to glow, but the light turns black and then fades.",
	  obj );
    return false;
  }

  int roll = number_range( 0, 100 );

  if( roll < 30-level+10*obj->value[0] ) {
    fsend( *ch->array,
	   "%s glows briefly, but then suddenly explodes in a burst of energy!",
	   obj );
    obj->Extract( 1 );
    return false;
  } 
  
  if( roll <= 60-2*level+20*obj->value[0] ) {
    fsend( *ch->array,
	   "%s glows briefly but the enchantment fails.",
	   obj );
    return false;
  }
  
  fsend( *ch->array, "%s glows for a while.", obj );

  obj = (obj_data*) obj->From( 1, true );
 
  ++obj->value[0];
  set_bit( obj->extra_flags, OFLAG_MAGIC );

  obj->To( );

  return true;
}


bool spell_major_enchantment( char_data* ch, char_data*, void* vo, 
			      int level, int duration )
{
  // Fill.
  if( duration == -4 )
    return false;

  if( null_caster( ch, SPELL_MAJOR_ENCHANTMENT ) ) 
    return false;
  
  obj_data *obj = (obj_data*) vo;

  obj->Select( 1 );

  if( is_set( obj->extra_flags, OFLAG_NO_MAJOR ) ) {
    fsend( ch, "%s cannot be enchanted.", obj );
    return false;
  }

  if( obj->value[0] < 0 ) {
    fsend( *ch->array,
	  "%s starts to glow, but the light turns black and then fades.",
	  obj );
    return false;
  }

  int roll = number_range( 0, 100 );
  
  if( obj->value[0] >= 3
      || roll <= 30-level+20*obj->value[0] ) {
    fsend( *ch->array,
	   "%s glows briefly but the enchantment fails.",
	   obj );
    return false;
  }

  /*
  if( is_set( obj->extra_flags, OFLAG_NO_ENCHANT ) ) {
    fsend( ch, "%s cannot be enchanted.", obj );
    return true;
  }

  if( obj->value[0] < 0 ) {
    fsend( *ch->array,
	  "%s starts to glow, but the light turns black and then fades.",
	  obj );
    return true;
  }

  if( obj->value[0] >= 3
      && is_set( obj->extra_flags, OFLAG_NO_MAJOR ) ) {
    fsend( *ch->array,
	   "%s glows briefly but the enchantment fails.",
	   obj );
    return true;
  }

  int roll = number_range( 0, 100 );

  if( roll <= 45-2*level+15*obj->value[0] ) {
    fsend( *ch->array,
	   "%s glows briefly but the enchantment fails.",
	   obj );
    return true;
  }
  */

  fsend( *ch->array, "%s glows for a while.", obj );

  obj = (obj_data*) obj->From( 1, true );

  ++obj->value[0];
  set_bit( obj->extra_flags, OFLAG_MAGIC );

  obj->To( );

  return true;
}


/*
 *   REPLICATE
 */


bool spell_replicate( char_data* ch, char_data*, void* vo,
		      int level, int duration )
{
  // Fill.
  if( duration == -4 )
    return false;

  if( null_caster( ch, SPELL_REPLICATE ) ) 
    return false;
  
  obj_data *obj = (obj_data*) vo; 
  
  if( is_set( obj->extra_flags, OFLAG_COPIED ) ) {
    fsend( ch, "You feel %s has already been copied and lacks the essence\
 required for you to replicate it.", obj );
    return false;
  }
  
  fsend( ch, "In a swirl of colors, %s materializes in your hand.", obj );
  fsend_seen( ch, "In a swirl of colors, %s materializes in %s's hand.",
	      obj, ch );   
  
  obj = (obj_data *) obj->From( 1, true );

  obj->Set_Number( 2 );

  set_bit( obj->extra_flags, OFLAG_COPIED );

  obj->To( );

  return true;
}
 
 
/*
 *   IDENTIFY SPELL
 */


bool identify( char_data *ch, obj_data *obj )
{
  char                  buf  [ MAX_INPUT_LENGTH ];
  affect_data*          paf;
  obj_clss_data*   obj_clss  = obj->pIndexData;
  int                     i;

  Content_Array *array = obj->array;

  if( array ) {
    obj = (obj_data *) obj->From( 1, true );
  }
  
  set_bit( obj->extra_flags, OFLAG_IDENTIFIED );
  set_bit( obj->extra_flags, OFLAG_KNOWN_LIQUID );
  
  send( ch, scroll_line[0] );

  send_title( ch, obj->Seen_Name( ch, 1 ) );
  
  send( ch, "     Base Cost: %-12d Level: %-11d Weight: %.2f lbs\n\r",
	obj->Cost( ), obj->Level( ), obj->Empty_Weight( 1 )/100. );

  if( obj_clss->remort > 0 ) {
    send( ch, "  Remort Level: %d\n\r",
	  obj_clss->remort );
  }

  switch( obj_clss->item_type ) {
  case ITEM_WEAPON :
    send( ch, "        Damage: %-12s Class: %-11s Attack: %s\n\r",
	  dice_string( obj->value[1] ),
	  ( obj->value[3] >= 0 && obj->value[3] < table_max[TABLE_SKILL_WEAPON] )
	  ? skill_weapon_table[ obj->value[3] ].name : "none",
	  ( obj->value[3] >= 0 && obj->value[3] < table_max[TABLE_SKILL_WEAPON] )
	  ? skill_weapon_table[ obj->value[3] ].noun[0] : "none" );
    break;
    
  case ITEM_ARROW :
    send( ch, "        Damage: %s\n\r",
	  dice_string( obj->value[1] ) );
    break;
    
  case ITEM_WAND :
    send( ch, "       Charges: %-12d Spell: %s\n\r",
	  obj->value[3],
	  ( obj->value[0] >= 0 && obj->value[0] < table_max[TABLE_SKILL_SPELL] )
	  ? skill_spell_table[ obj->value[0] ].name : "none" );
    break;
    
  case ITEM_WHISTLE:
    if( obj->value[0] > 0 )
      send( ch, "         Range: %d\n\r", obj->value[0] );
    break;
    
  case ITEM_SCROLL :
  case ITEM_POTION :
    send( ch, "    Cast Level: %-12d Spell: %s\n\r",
	  obj->value[1],
	  ( obj->value[0] >= 0 && obj->value[0] < table_max[TABLE_SKILL_SPELL] )
	  ? skill_spell_table[ obj->value[0] ].name : "none" );
    break;

    /* Trap damage dice not implemented.
  case ITEM_TRAP :
    sprintf( buf+5,  "   Damage: %s           ", 
	     dice_string( obj->value[1] ) );
    break;
    */

  case ITEM_ARMOR :
    send( ch, "   Armor Class: %-5d", armor_class( obj ) );
    if( obj->value[2] != 0 ) {
      send( ch, " Global Armor: %d", obj->value[2] );
    }
    send( ch, "\n\r" );
    break;
    
  } 
  
  snprintf( buf, MAX_INPUT_LENGTH, "%d%%", obj->vs_acid( ) );
  snprintf( buf+20, MAX_INPUT_LENGTH-20, "%d%%", obj->vs_fire( ) );
  snprintf( buf+40, MAX_INPUT_LENGTH-40, "%d%%", obj->vs_cold( ) );
  
  send( ch, "          Acid: %-13s Fire: %-13s Cold: %s\n\r\n\r",
	buf, buf+20, buf+40 );
  
  /* CONDITION */ 
  
  if( obj_clss->item_type == ITEM_ARMOR
      || obj_clss->item_type == ITEM_WEAPON )  
    send( ch, "     Condition: %s\n\r",
	  obj->condition_name( ch, true ) );
  
  /* MATERIALS */
  
  strcpy( buf, "     Materials: " );
  material_flags.sprint( &buf[16], &obj->materials );
  send( ch, buf );
  send( ch, "\n\r" );
  
  /* WEAR LOC */
  
  buf[0] = '\0';
  for( i = 1; i < MAX_ITEM_WEAR; i++ )
    if( is_set( obj_clss->wear_flags, i ) ) {
      if( !*buf ) 
        sprintf( buf, "     Wear Loc.: %s", wear_name[i] );
      else
        sprintf( buf+strlen( buf ), ", %s", wear_name[i] );
    }
  if( *buf ) {
    send( ch, buf );
    send( ch, "\n\r" );
  }
  
  /* ANTI-FLAGS */
  
  buf[0] = '\0';
  for( i = 0; i < MAX_ANTI; i++ )
    if( is_set( obj_clss->anti_flags, i ) ) {
      if( !*buf ) 
        sprintf( buf, "\n\r    Anti-Flags: %s", anti_flags[i] );
      else
        sprintf( buf+strlen( buf ), ", %s", anti_flags[i] );
    }
  if( *buf ) {
    strcat( buf, "\n\r" );
    send( ch, buf );
  }
  
  /* RESTRICTIONS */
  
  buf[0] = '\0';
  for( i = 0; i < MAX_RESTRICTION; ++i )
    if( is_set( obj_clss->restrictions, i ) && *restrict_ident[i] != '\0' ) {
      if( !*buf ) 
        sprintf( buf, "\n\r  Restrictions: %s", restrict_ident[i] );
      else
        sprintf( buf+strlen( buf ), ", %s", restrict_ident[i] );
    }
  if( *buf ) {
    strcat( buf, "\n\r" );
    send( ch, buf );
  }
  
  /* AFFECTS */
  
  bool found = false;

  for( i = 0; i < obj_clss->affected; i++ ) {
    paf = obj_clss->affected[i];
    if( paf->type == AFF_NONE ) { 
      if( !found ) {
        send( ch, "\n\r       Affects:\n\r" );
        found = true;    
      }
      if( paf->location == APPLY_MANA_REGEN
	  || paf->location == APPLY_HIT_REGEN
	  || paf->location == APPLY_MOVE_REGEN ) {
	send( ch, "%10s%s by %+.1f.\n\r", "",
	      affect_location[ paf->location ], (double) paf->modifier / 10.0 );
      } else {
	send( ch, "%10s%s by %+d.\n\r", "",
	      affect_location[ paf->location ], paf->modifier );
      }
    }
  }
  
  for( i = 0; i < table_max[ TABLE_AFF_CHAR ]; i++ ) {
    if( is_set( obj_clss->affect_flags, i ) && *aff_char_table[i].id_line ) {
      if( !found ) {
        send( ch, "\n\r       Affects:\n\r" );
        found = true;    
      }
      send( ch, "%10s%s\n\r", "", aff_char_table[i].id_line );
    }
  }
  
  for( i = 0; i < MAX_OFLAG; i++ ) {
    if( is_set( obj->extra_flags, i ) && *oflag_ident[i] != '\0' ) {
      if( !found ) {
        send( ch, "\n\r       Affects:\n\r" );
        found = true;    
      }
      send( ch, "%10s%s.\n\r", "", oflag_ident[i] );
    }
  }
  
  if( obj->light != 0 ) {
    if( !found ) {
      send( ch, "\n\r       Affects:\n\r" );
      found = true;    
    }
    if( obj->light > 0 ) {
      send( ch, "%10s%s.\n\r", "", "Provides light" );
    } else {
      send( ch, "%10s%s.\n\r", "", "Provides darkness" );
    }
  }

  send( ch, "\n\r" );
  send( ch, scroll_line[0] );
  
  if( array )
    obj->To( );
  
  return true;
}


bool spell_identify( char_data* ch, char_data*, void* vo, int level, int time )
{
  // Fill.
  if( time == -4 )
    return false;

  if( null_caster( ch, SPELL_IDENTIFY ) )
    return false;

  obj_data *obj = (obj_data*) vo;

  return identify( ch, obj );
}


bool spell_obscure( char_data* ch, char_data*, void* vo, int, int time )
{
  // Fill.
  if( time == -4 )
    return false;

  obj_data *obj = (obj_data*) vo;

  if( !is_set( obj->extra_flags, OFLAG_IDENTIFIED )
      && !is_set( obj->extra_flags, OFLAG_KNOWN_LIQUID ) ) {
    if( ch )
      send( *ch->array, "Nothing happens.\n\r" );
    return false;
  }

  Content_Array *array = obj->array;

  if( array ) {
    obj = (obj_data *) obj->From( 1, true );
  }

  remove_bit( obj->extra_flags, OFLAG_IDENTIFIED );
  remove_bit( obj->extra_flags, OFLAG_KNOWN_LIQUID );

  if( ch )
    fsend( *ch->array, "You suddenly don't recognize %s.", obj );

  if( array )
    obj->To( );

  return true;

}

/*
 *   FLASH OF LIGHT
 */


bool spell_blind( char_data* ch, char_data* victim, void*,
		  int level, int duration )
{
  if( duration == -4 || duration == -3 )
    return false;

  if( victim->species
      && !is_set( victim->species->act_flags, ACT_HAS_EYES ) ) {
    fsend( ch, "%s does not see with %s eyes so blind has no affect.",
	   victim, victim->His_Her( ) );
    return false;
  }
  
  if( !can_kill( ch, victim ) )
    return false;
  
  if( makes_save( victim, ch, RES_MAGIC, SPELL_BLIND, level ) ) {
    if( !victim->is_affected( AFF_BLIND ) ) {
      send( victim, "Your eyes burn for a moment, but the feeling passes.\n\r" );
    }
    return false;
  }
  
  return spell_affect( ch, victim, level, duration, SPELL_BLIND, AFF_BLIND );
}


bool spell_blinding_light( char_data* ch, char_data*, void*,
			   int level, int duration )
{
  /*
  if( null_caster( ch, SPELL_BLINDING_LIGHT ) )
    return false;

  if( !ch->array )
    return false;

  unsigned count = 0;

  for( int i = 0; i < *ch->array; ++i ) {
    char_data *rch;
    if( ( rch = character( ch->array->list[i] ) )
	&& rch != ch ) {
      if( victim->species
	  && !is_set( &victim->species->act_flags, ACT_HAS_EYES ) ) {
	continue;
      }
    }
  }

  if( count == 0 ) {
    send( ch, "Nothing happens.\n\r" );
    return false;
  }
  */

  /*
  char_data* rch;

  for( rch = ch->in_room->people; rch; rch = rch->next_in_room ) {
    if( rch == ch || !rch->Can_See( ) || ( rch->species != NULL
      && !is_set( &rch->species->act_flags, ACT_HAS_EYES ) ) )
      continue;
    send( rch, "### The room explodes in a flash of light. ###\n\r" );
    if( makes_save( rch, ch, RES_MAGIC, SPELL_BLINDING_LIGHT, level ) ) {
      send( rch, 
        "Fortunately you were not looking at the blast and\
 are unaffected.\n\r" );
      }
    else if( !can_kill( ch, rch ) ) {
      send( rch, "Oddly the flash has no affect on you.\n\r" );
      }
    else {
      spell_affect( ch, rch, level, duration,
        SPELL_BLINDING_LIGHT, AFF_BLIND );
      }
    }
  */

  return true;
}


bool spell_wither( char_data* ch, char_data* victim, void*, int level, int time )
{
  if( time == -4 || time == -3 )
    return false;

  if( !can_kill( ch, victim ) )
    return false;

  if( makes_save( victim, ch, RES_MAGIC, SPELL_WITHER, level ) ) {
    send( ch, "The spell fails.\n\r" );
    return false;
  }

  send( victim, "You feel your skin shrivel.\n\r" );
  fsend_seen( victim, "%s seems to shrivel before you.", victim );
 
  affect_data affect;

  affect.type      = AFF_NONE;
  affect.location  = APPLY_CON;
  affect.modifier  = -1; 
  affect.duration  = level*3;
  affect.level     = level;

  add_affect( victim, &affect );

  damage_magic( victim, ch, spell_damage( SPELL_WITHER, level ),
		"*A withering stare" );

  return true;
}


bool spell_drain_life( char_data* ch, char_data* victim, void*,
		       int level, int time )
{
  if( time == -4 || time == -3 )
    return false;

  if( victim->shdata->race == RACE_UNDEAD ) {
    send( ch, "You cannot drain life from the undead.\n\r" );
    return false;
  }

  if( victim->shdata->race == RACE_GOLEM
      || victim->shdata->race == RACE_ELEMENTAL ) {
    send( ch, "Nothing happens.\n\r" );
    send_seen( ch, "Nothing happens.\n\r" );
    return false;
  }

  if( !can_kill( ch, victim ) )
    return false;

  if( makes_save( victim, ch, RES_MAGIC, SPELL_DRAIN_LIFE, level ) ) {
    send( ch, "The spell fails.\n\r" );
    return false;
  }

  send( victim, "The chill of death momentarily touchs your soul.\n\r" );
  fsend_seen( victim, "%s shivers for a brief instant.", victim );
 
  affect_data affect;

  affect.type      = AFF_NONE;
  affect.location  = APPLY_CON;
  affect.modifier  = -1; 
  affect.duration  = level*3;
  affect.level     = level;

  add_affect( victim, &affect );

  return true;
}


/*
 *   WEB SPELLS
 */


bool spell_web( char_data* ch, char_data* victim, void*,
		int level, int duration )
{
  if( duration == -4 || duration == -3 )
    return false;

  if( !can_kill( ch, victim ) )
    return false;

  if( victim->is_affected( AFF_ENTANGLED ) ) {
    affect_data* paf;
    for( int i = 0; i < victim->affected; i++ ) {
      paf = victim->affected[i];
      if( paf->type == AFF_ENTANGLED ) {
        paf->level += 1+number_range( 0, level/3 );
	paf->level = min( paf->level, 20 );
        paf->duration = max( level/2, paf->duration+1 ); 
	paf->duration = min( paf->duration, 20 );
        break;
      }
    }
    send( victim, "The web tightens around you.\n\r" );
    fsend_seen( victim, "The web trapping %s grows tighter.", victim );
    return true;
  }

  send( victim, "A web begins to form around you.\n\r" );
  fsend_seen( victim, "A web begins to form around %s.", victim );

  if( victim->is_affected( AFF_FIRE_SHIELD ) ) {
    send( victim, "Your fire shield quickly vaporizes the web.\n\r" );
    fsend_seen( victim, "%s's fire shield quickly vaporizes the web.", victim );
    return false;
  }

  if( victim->is_affected( AFF_ICE_SHIELD ) ) {
    send( victim, "Your ice shield quickly shreds the web.\n\r" );
    fsend_seen( victim, "%s's ice shield quickly shreds the web.", victim );
    return false;
  }

  if( makes_save( victim, ch, RES_DEXTERITY, SPELL_WEB, level )  
      && makes_save( victim, ch, RES_MAGIC, SPELL_WEB, level ) )  {
    send( victim, "Luckily you avoid becoming entangled.\n\r" );
    fsend_seen( victim, "%s skillfully avoids the web.", victim );
    return false;
  }
  
  const bool add = spell_affect( ch, victim, level, duration, SPELL_WEB, AFF_ENTANGLED );
  set_min_delay( victim, 32 );

  return add;
}


/*
 *   MIND SPELLS
 */


bool spell_confuse( char_data* ch, char_data* victim, void*, int level,
		    int duration )
{
  if( duration == -4 || duration == -3 )
    return false;

  if( makes_save( victim, ch, RES_MIND, SPELL_CONFUSE, level ) ) {
    send( *victim->array, "Nothing happens.\n\r" );
    return false;
  }

  return spell_affect( ch, victim, level, duration, SPELL_CONFUSE, AFF_CONFUSED );
}


bool spell_hallucinate( char_data* ch, char_data* victim, void*,
			int level, int duration )
{
  if( duration == -4 || duration == -3 )
    return false;

  if( makes_save( victim, ch, RES_MIND, SPELL_HALLUCINATE, level ) ) {
    send( *victim->array, "Nothing happens.\n\r" );
    return false;
  }

  return spell_affect( ch, victim, level, duration,
		       SPELL_HALLUCINATE, AFF_HALLUCINATE );
}


/*
 *   VISION SPELLS
 */


/*
bool spell_sense_danger( char_data* ch, char_data* victim, void*,
			 int level, int duration )
{
  return spell_affect( ch, victim, level, duration,
		       SPELL_SENSE_DANGER, AFF_SENSE_DANGER );
}
*/


bool spell_eagle_eye( char_data* ch, char_data* victim, void*, int level, int )
{
  if( null_caster( ch, SPELL_EAGLE_EYE ) )
    return false;

  if( !victim ) {
    send( ch, "Nothing happens.\n\r" );
    send_seen( ch, "Nothing happens.\n\r" );
    return false;
  }

  if( !victim->in_room ) {
    send( ch, "They are somewhere unusual.\n\r" );
    return false;
  }
  
  if( victim->in_room->area->status != AREA_OPEN && ch->Level( ) < LEVEL_APPRENTICE
      || is_set( victim->in_room->room_flags, RFLAG_NO_MAGIC )
      || ( victim->Level() >= LEVEL_BUILDER
	   && get_trust( ch ) < get_trust( victim ) ) ) {
    send( ch, "The spell is mysteriously blocked.\n\r" );
    return false;
  }

  if( victim->position > POS_SLEEPING ) {
    send( victim, "You suddenly feel as though you are being watched.\n\r" );
  }

  room_data *room = ch->in_room;
  ch->in_room = victim->in_room;
  send( ch, "\n\r" );
  do_look( ch, "" );
  ch->in_room = room;

  return true;
}


bool spell_scry( char_data* ch, char_data* victim, void*, int level, int )
{
  if( null_caster( ch, SPELL_SCRY ) )
    return false;

  if( !victim ) {
    send( ch, "The image dissipates without forming anything recognizable.\n\r" );
    send_seen( ch, "Nothing happens.\n\r" );
    return false;
  }

  if( !victim->in_room ) {
    send( ch, "They are somewhere unusual.\n\r" );
    return false;
  }

  if( victim->in_room->area->status != AREA_OPEN && ch->Level( ) < LEVEL_APPRENTICE
      || is_set( victim->in_room->room_flags, RFLAG_NO_MAGIC )
      || ( victim->Level() >= LEVEL_BUILDER
	 && get_trust( ch ) < get_trust( victim ) ) ) {
    send( ch, "The spell is mysteriously blocked.\n\r" );
    return false;
  }

  if( victim->position > POS_SLEEPING ) {
    send( victim, "You suddenly feel as though you are being watched.\n\r" );
  }

  room_data *room = ch->in_room;
  ch->in_room = victim->in_room;
  send( ch, "\n\r" );
  do_look( ch, "" );
  ch->in_room = room;

  return true;
}



bool spell_hawks_view( char_data* ch, char_data* victim, void*, int level, int )
{
  if( null_caster( ch, SPELL_HAWKS_VIEW ) )
    return false;

  if( !victim ) {
    send( ch, "Nothing happens.\n\r" );
    send_seen( ch, "Nothing happens.\n\r" );
    return false;
  }

  if( !victim->in_room ) {
    send( ch, "They are somewhere unusual.\n\r" );
    return false;
  }
  
  if( victim->in_room->area->status != AREA_OPEN && ch->Level( ) < LEVEL_APPRENTICE
      || is_set( victim->in_room->room_flags, RFLAG_NO_MAGIC )
      || ( victim->Level() >= LEVEL_BUILDER
	   && get_trust( ch ) < get_trust( victim ) ) ) {
    send( ch, "The spell is mysteriously blocked.\n\r" );
    return false;
  }
  
  if( level < 4 && victim->in_room->is_indoors( ) ) {
    send( ch, "Your victim is not visible from the sky.\n\r" );
    return false;
  }
  
  if( level < 8 && is_set( victim->in_room->room_flags, RFLAG_UNDERGROUND ) ) {
    send( ch, "Your victim is nowhere above ground.\n\r" );
    return false;
  }

  if( victim->Can_Hear() ) {
    send( victim, "You hear the piercing cry of a hawk that has found its prey.\n\r" );
  }

  room_data *room = ch->in_room;
  ch->in_room = victim->in_room;
  send( ch, "\n\r" );
  do_look( ch, "" );
  ch->in_room = room;

  return true;
}



/*
 *   INVULNERABILITY
 */


/*
bool spell_invulnerability( char_data* ch, char_data* victim, void*,
			    int level, int duration )
{
  return spell_affect( ch, victim, level, duration,
		       SPELL_INVULNERABILITY, AFF_INVULNERABILITY );
}
*/


/*
 *   LEVITATION/FLY
 */


/*
bool spell_float( char_data* ch, char_data* victim, void*, int level,
		  int duration )
{
  return spell_affect( ch, victim, level, duration,
		       SPELL_FLOAT, AFF_FLOAT );
}


bool spell_fly( char_data* ch, char_data* victim, void*, int level,
		int duration )
{
  return spell_affect( ch, victim, level, duration,
		       SPELL_FLY, AFF_FLY );
}
*/


/*
 *  SLOW/HASTE
 */


/*
bool spell_haste( char_data* ch, char_data* victim, void*, int level,
		  int duration )
{
  if( duration == -4 || duration == -3 )
    return false;

  if( victim->shdata->race == RACE_UNDEAD ) {
    send( *victim->array, "Nothing happens.\n\r" );
    return false;
  }

  return spell_affect( ch, victim, level, duration,
		       SPELL_HASTE, AFF_HASTE );
}
*/


bool spell_slow( char_data* ch, char_data* victim, void*, int level,
		 int duration )
{
  if( duration == -4 || duration == -3 )
    return false;

  if( victim->shdata->race == RACE_UNDEAD ) {
    send( *victim->array, "Nothing happens.\n\r" );
    return false;
  }

  if( !victim->is_affected( AFF_SLOW )
      && makes_save( victim, ch, RES_MAGIC, SPELL_SLOW, level ) ) {
    send( ch, "Nothing happens.\n\r" );
    send_seen( ch, "Nothing happens.\n\r", ch );
    return false;
  }

  if( spell_affect( ch, victim, level, duration,
		    SPELL_SLOW, AFF_SLOW ) ) {
    record_damage( victim, ch );    // Provides an experience share.
    return true;
  }

  return false;
}
