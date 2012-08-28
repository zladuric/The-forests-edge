#include <ctype.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include "define.h"
#include "struct.h"


/*
 *   SKIN FUNCTION
 */


static species_data *can_skin( char_data *ch, obj_data *corpse, const char *verb, bool quiet = false )
{
  if( !corpse
      || corpse->pIndexData->item_type != ITEM_CORPSE ) {
    if( !quiet )
      send( ch, "You can only %s corpses.\n\r", verb );
    return 0;
  }
  
  if( corpse->pIndexData->vnum == OBJ_CORPSE_PC
      || corpse->pIndexData->vnum == OBJ_CORPSE_PET ) {
    if( !quiet )
      send( ch, "You cannot %s that corpse.\n\r", verb );
    return 0;
  }

  species_data *species = get_species( corpse->value[1] );

  if( !species ) {
    if( !quiet )
      send( ch, "You cannot %s that corpse.\n\r", verb );
    return 0;
  }

  for( mprog_data *mprog = species->mprog; mprog; mprog = mprog->next ) {
    if( mprog->trigger == MPROG_TRIGGER_SKIN ) {
      clear_variables( );
      var_cmd = verb;
      //      var_mob = 0;
      var_obj = corpse;
      var_ch = ch;    
      var_room = ch->in_room;
      if( !mprog->execute( corpse ) )
	return 0;
      break;
    }
  }

  return species;
}


static void finish_skin( char_data *ch,
			 obj_data *corpse, obj_array *list,
			 const char *verb )
{
  if( list->is_empty() ) {
    fsend( ch, "You %s %s, but it is too mangled to be of any use.",
	   verb, corpse );
    fsend_seen( ch,
		"%s %ss %s but is unable to extract anything of value.",
		ch, verb, corpse );
  } else {
    fsend( ch,
	   "You %s %s producing %s.",
	   verb, corpse, (thing_array*)list );
    fsend_seen( ch,
		"%s %ss %s producing %s.",
		ch, verb, corpse, (thing_array*)list );
    
    for( int i = 0; i < *list; ) {
      obj_data *obj = list->list[i];
      if( !heavy( obj, ch ) || !many( obj, ch ) ) {
	obj->To( ch->in_room );
	++i;
      } else {
	list->remove( i );
	obj->To( ch );
      }
    }

    if( !list->is_empty( ) ) {
      fsend( ch, "You are unable to carry %s.\n\r", (thing_array*)list );
      fsend_seen( ch, "%s is unable to carry %s.\n\r", ch, (thing_array*)list );
    }
  }

  delete list;

  extract_corpse( corpse );
}


void skin( char_data *ch, obj_data *corpse, bool quiet )
{
  species_data *species = can_skin( ch, corpse, "skin", quiet );

  if( !species )
    return;

  /*
  if( !corpse
      || corpse->pIndexData->item_type != ITEM_CORPSE ) {
    send( ch, "You can only skin corpses.\n\r" );
    return;
  }
  
  if( corpse->pIndexData->vnum == OBJ_CORPSE_PC
      || corpse->pIndexData->vnum == OBJ_CORPSE_PET ) {
    send( ch, "You cannot skin that corpse.\n\r" );
    return;
  }
  
  species_data *species;
  
  if( !( species = get_species( corpse->value[1] ) ) ) {
    send( ch, "Corpses without a species cannot be skinned.\n\r" );
    return;
  }
  
  for( mprog_data *mprog = species->mprog; mprog; mprog = mprog->next )
    if( mprog->trigger == MPROG_TRIGGER_SKIN ) {
      clear_variables( );
//      var_mob  = 0;
      var_obj  = corpse;
      var_ch   = ch;    
      var_room = ch->in_room;
      mprog->execute( corpse ); 
      return;
    }
  */


  obj_array *list = get_skin_list( species );
  
  /*
  for( mprog_data *mprog = species->mprog; mprog; mprog = mprog->next ) {
    if( mprog->trigger == MPROG_TRIGGER_SKIN ) {
      clear_variables( );
      if( list ) {
	var_list = *list;
      } else {
	var_list.clear( );
      }
//      var_mob = 0;
      var_obj = corpse;
      var_ch = ch;    
      var_room = ch->in_room;
      if( !mprog->execute( corpse ) ) {
	extract( var_list );
	return 0;
      }
      break;
    }
  }
  */

  if( !list ) {
    if( !quiet )
      fsend( ch, "You cannot skin %s.", corpse );
    return;
  }
  
  finish_skin( ch, corpse, list, "skin" );

  set_delay( ch, 20 );
}


void do_skin( char_data* ch, const char *argument )
{
  if( is_mob( ch ) )
    return;

  thing_data *thing;

  if( !( thing = one_thing( ch, argument, "skin", ch->array ) ) )
    return;

  if( char_data *rch = character( thing ) ) {
    if( rch->species && is_set( rch->species->act_flags, ACT_MIMIC ) ) {
      fsend( ch, "You cannot skin %s.", thing );
    } else {
      fsend( ch, "Skinning %s alive would be cruel and unusual.", thing );
    }
    return;
  } 
  
  skin( ch, object( thing ) );
}


void do_butcher( char_data* ch, const char *argument )
{
  if( is_mob( ch ) )
    return;

  if( is_mounted( ch, "butcher" )
      || is_ridden( ch, "butcher" )
      || is_entangled( ch, "butcher" )
      || is_fighting( ch, "butcher" )
      || is_drowning( ch, "butcher" ) ) {
    return;
  }

  int skill = ch->get_skill( SKILL_BUTCHER );

  if( skill == UNLEARNT ) {
    send( ch, "You don't know how to butcher.\n\r" );
    return;
  }

  thing_data *thing;

  if( !( thing = one_thing( ch, argument, "butcher", ch->array ) ) )
    return;

  if( char_data *rch = character( thing ) ) {
    if( rch->species && is_set( rch->species->act_flags, ACT_MIMIC ) ) {
      fsend( ch, "You cannot butcher %s.", thing );
    } else {
      fsend( ch, "Butchering %s alive would be cruel and unusual.", thing );
    }
    return;
  } 
  
  obj_data *corpse = object( thing );

  species_data *species = can_skin( ch, corpse, "butcher" );

  if( !species )
    return;

  /*
  if( !( corpse = object( thing ) )
      || corpse->pIndexData->item_type != ITEM_CORPSE ) {
    send( ch, "You can only butcher corpses.\n\r" );
    return;
  }
  
  if( corpse->pIndexData->vnum == OBJ_CORPSE_PC
      || corpse->pIndexData->vnum == OBJ_CORPSE_PET ) {
    send( ch, "You cannot butcher that corpse.\n\r" );
    return;
  }

  species_data *species;

  if( !( species = get_species( corpse->value[1] ) ) ) {
    send( ch, "Corpses without a species cannot be butchered.\n\r" );
    return;
  }
  */

  if( obj_array *list = get_skin_list( species ) ) {
    
    finish_skin( ch, corpse, list, "butcher" );
    /*
    if( list->is_empty() ) {
      fsend( ch, "You butcher %s, but it is too mangled to be of any use.",
	     corpse );
      fsend_seen( ch,
		  "%s butchers %s but is unable to extract anything of value.",
		  ch, corpse );
    } else {
      fsend( ch,
	     "You butcher %s producing %s.",
	     corpse, (thing_array*)list );
      
      fsend_seen( ch,
		  "%s butchers %s producing %s.",
		  ch, corpse, (thing_array*)list );
      
      for( int i = 0; i < *list; ) {
	obj_data *obj = list->list[i];
	if( !heavy( obj, ch ) || !many( obj, ch ) ) {
	  obj->To( ch->in_room );
	  ++i;
	} else {
	  list->remove( i );
	  obj->To( ch );
	}
      }

      if( !list->is_empty( ) ) {
	fsend( ch, "You are unable to carry %s.\n\r", (thing_array*)list );
	fsend_seen( ch, "%s is unable to carry %s.\n\r", ch, (thing_array*)list );
      }
    }
    
    delete list;
    
    extract_corpse( corpse );
    */
    return;
  }

  if( is_set( species->act_flags, ACT_GHOST )
      || is_set( species->act_flags, ACT_MIMIC )
      ) {
    send( ch, "You cannot butcher that corpse.\n\r" );
    return;
  }
  
  int race = species->shdata->race;

  if( race == RACE_UNDEAD ) {
    send( ch, "You cannot butcher the undead. Yuck!\n\r" );
    return;
  }

  if( race == RACE_PLANT
      || race == RACE_GOLEM
      || race == RACE_ELEMENTAL
      || race == RACE_UNKNOWN
      || race == MAX_PLYR_RACE ) {
    send( ch, "You cannot butcher that corpse.\n\r" );
    return;
  }

  bool humanoid = ( race < MAX_PLYR_RACE )
    || is_set( species->act_flags, ACT_HUMANOID );

  bool finger = humanoid
    || is_set( species->wear_part, WEAR_FINGER_R )
    || is_set( species->wear_part, WEAR_FINGER_L );
  bool neck = humanoid || is_set( species->wear_part, WEAR_NECK );
  bool body = humanoid || is_set( species->wear_part, WEAR_BODY );
  bool head = humanoid || is_set( species->wear_part, WEAR_HEAD );
  bool legs = humanoid || is_set( species->wear_part, WEAR_LEGS );
  bool feet = humanoid || is_set( species->wear_part, WEAR_FEET );
  bool hands = humanoid || is_set( species->wear_part, WEAR_HANDS );
  bool arms = humanoid || is_set( species->wear_part, WEAR_ARMS );
  /*
  bool wrist = humanoid
    || is_set( species->wear_part, WEAR_WRIST_R )
    || is_set( species->wear_part, WEAR_WRIST_L );
  */
  bool horse_body = !humanoid && is_set( species->wear_part, WEAR_HORSE_BODY );
  //  bool horse_back = !humanoid && is_set( species->wear_part, WEAR_HORSE_BACK );
  bool horse_feet = !humanoid && is_set( species->wear_part, WEAR_HORSE_FEET );

  bool none = !finger
    && !neck
    && !body
    && !head
    && !legs
    && !feet
    && !hands
    && !arms
    //    && !wrist
    && !horse_body
    //    && !horse_back
    && !horse_feet;

  if( none ) {
    send( ch, "You cannot butcher that corpse.\n\r" );
    return;
  }

  const int head_part = 4;

  const bool part_exist [] = {
    finger,
    finger,
    neck,
    body,
    head,
    legs,
    legs,
    feet,
    feet,
    hands,
    hands,
    arms,
    arms,
    // wrist,
    // wrist,
    horse_body,
    //    horse_back,
    horse_feet,
    horse_feet
  };

  // In thousandths of total corpse weight.
  static const int part_weight [] = {
    2,
    2,
    20,
    300,
    60,
    100,
    100,
    30,
    30,
    20,
    20,
    80,
    80,
    // wrist,
    // wrist,
    400,
    // back,
    30,
    30
  };

  static const char *const part_name [] = {
    "finger",
    "finger",
    "neck",
    "torso",
    "head",
    "leg",
    "leg",
    "foot",
    "foot",
    "hand",
    "hand",
    "arm",
    "arm",
    //    "wrist",
    //    "wrist",
    "torso",
    //    "back",
    "foot",
    "foot",
    0
  };

  static const char *const part_names [] = {
    "fingers",
    "fingers",
    "necks",
    "torsos",
    "heads",
    "legs",
    "legs",
    "feet",
    "feet",
    "hands",
    "hands",
    "arms",
    "arms",
    //    "wrists",
    //    "wrists",
    "torsos",
    //    "backs",
    "feet",
    "feet",
    0
  };

  char buf [ MAX_STRING_LENGTH ];
  thing_array parts;
  for( int i = 0; part_name[i]; ++i ) {
    if( part_exist[i] && number_range( 1, 100 ) <= skill ) {
      obj_data *obj = create( get_obj_index( OBJ_BODY_PART ) );
      //      const char *type = race_table[race].name;
      //      bool vowel = isvowel( *type );
      if( i == head_part ) {
	const char *name;
	if( *species->descr->name ) {
	  name = species->descr->name;
	} else {
	  name = species->descr->singular;
	  if( *name == '!' )
	    ++name;
	}
	snprintf( buf, MAX_STRING_LENGTH, "%s of %s",
		  part_name[i],
		  species->Name( ) );
	obj->singular = alloc_string( buf, MEM_OBJECT );
	snprintf( buf, MAX_STRING_LENGTH, "%s %s",
		  name,
		  part_names[i] );
	obj->plural = alloc_string( buf, MEM_OBJECT );
	set_bit( obj->extra_flags, OFLAG_THE_AFTER );
	set_bit( obj->extra_flags, OFLAG_THE_BEFORE );
      } else {
	snprintf( buf, MAX_STRING_LENGTH, "%s %s",
		  race_table[race].name,
		  part_name[i] );
	obj->singular = alloc_string( buf, MEM_OBJECT );
	snprintf( buf, MAX_STRING_LENGTH, "%s %s",
		  race_table[race].name,
		  part_names[i] );
	obj->plural = alloc_string( buf, MEM_OBJECT );
      }
      obj->weight = corpse->Empty_Weight( ) * part_weight[i] / 1000;
      obj->value[0] = corpse->Empty_Weight( ) * part_weight[i] / 100000;
      obj->value[2] = obj->value[0];
      parts += obj;
    }
  }

  if( parts.is_empty() ) {
    fsend( ch, "You butcher %s, but make a total mess of it.", corpse );
    fsend_seen( ch, "%s butchers %s, but makes a total mess of it.", ch, corpse );
  } else {
    fsend( ch, "You butcher %s, producing %s.", corpse, &parts );
    fsend_seen( ch, "%s butchers %s, producing %s.", ch, corpse, &parts );
    
    for( int i = parts.size-1; i >= 0; --i ) {
      parts[i]->To( *ch->array );
    }

    ch->improve_skill( SKILL_BUTCHER );
  }

  extract_corpse( corpse );
}


/*
 *   SPELLS
 */


bool spell_tame( char_data* ch, char_data* victim, void*, int level, int )
{
  if( null_caster( ch, SPELL_TAME ) )
    return false;

  if( is_set( victim->status, STAT_PET ) ) {
    fsend( ch, "%s is already tame.", victim );
    return false;
  }

  if( victim->position < POS_RESTING ) {
    fsend( ch, "%s is unconscious and so the spell has no affect.",
	   victim );
    return false;
  }
  
  if( victim->species
      && victim->Level() <= ch->Level()
      && !is_set( victim->species->act_flags, ACT_CAN_TAME ) ) {
    fsend( ch, "%s cannot be tamed.", victim );
    fsend_seen( victim, "%s ignores %s.", victim, ch );
    return false;
  }

  if( !victim->species
      || victim->leader
      || victim->Level() > ch->Level()
      || victim->fighting
      || !victim->aggressive.is_empty()
      || makes_save( victim, ch, RES_MIND, SPELL_TAME, level ) ) {
    fsend( ch, "%s ignores you.", victim );
    fsend_seen( victim, "%s ignores %s.", victim, ch );
    return false;
  }

  if( victim->Level() > ch->Level()-pet_levels( ch ) ) {
    send( ch, "You fail because you are unable to control more animals.\n\r" );
    return false;
  }

  if( is_set( victim->species->act_flags, ACT_MOUNT )
      && has_mount( ch ) )
    return false;
  
  if( ch->leader == victim )
    stop_follower( ch );
  
  delete_list( victim->enemy );

  remove_bit( victim->status, STAT_AGGR_ALL );
  remove_bit( victim->status, STAT_AGGR_GOOD );
  remove_bit( victim->status, STAT_AGGR_EVIL );
  remove_bit( victim->status, STAT_AGGR_LAWFUL );
  remove_bit( victim->status, STAT_AGGR_CHAOTIC );

  set_bit( victim->status, STAT_PET );
  set_bit( victim->status, STAT_TAMED );

  remove_bit( victim->status, STAT_SENTINEL );

  // Victim no longer appears in reset location.
  unregister_reset( (mob_data*)victim );

  add_follower( victim, ch );

  return true;
}


/*
bool spell_barkskin( char_data* ch, char_data* victim, void*, int level,
		     int duration )
{
  return spell_affect( ch, victim, level, duration,
		       SPELL_BARKSKIN, AFF_BARKSKIN );
}


bool spell_thorn_shield( char_data* ch, char_data* victim, void*, int level,
			 int duration )
{
  //  leave_shadows( victim );

  return spell_affect( ch, victim, level, duration,
		       SPELL_THORN_SHIELD, AFF_THORN_SHIELD );
}
*/


/*
 *   HEALING SPELLS
 */


/*
bool spell_balm( char_data* ch, char_data* victim, void*,
		 int level, int duration )
{
  return heal_victim( ch, victim, spell_damage( SPELL_BALM, level ), duration );
}


bool spell_surcease( char_data* ch, char_data* victim, void*,
		     int level, int duration )
{
  return heal_victim( ch, victim, spell_damage( SPELL_SURCEASE, level ), duration );
}


bool spell_poultice( char_data* ch, char_data* victim, void*,
		     int level, int duration )
{
  return heal_victim( ch, victim, spell_damage( SPELL_POULTICE, level ), duration );
}
*/


void do_forage( char_data* ch, const char *argument )
{
  if( is_mob( ch )
      || is_mounted( ch, "forage" )
      || is_ridden( ch, "forage" )
      || is_entangled( ch, "forage" )
      || is_fighting( ch, "forage" )
      || is_drowning( ch, "forage" ) ) {
    return;
  }

  int skill = ch->get_skill( SKILL_FORAGE );

  if( skill == UNLEARNT ) {
    send( ch, "You don't know how to forage.\n\r" );
    return;
  }

  room_data *room = ch->in_room;

  if( room->is_indoors( ) ) {
    send( ch, "You can't forage indoors.\n\r" );
    return;
  }

  if( !room->Seen( ch ) ) {
    send( ch, "You can't forage in the dark.\n\r" );
    return;
  }

  const int terrain = room->sector_type;

  Array<Obj_Clss_Data*> foods;
  Array<int> nums;

  for( int i = 0; i < 10; ++i ) {
    if( obj_clss_data *clss = get_obj_index( terrain_table[ terrain ].forage[ 2*i ] ) ) {
      foods.insert( clss, foods.size );
      nums.insert( terrain_table[ terrain ].forage[ 2*i+1 ], nums.size );
    }
  }

  if( foods.is_empty( ) ) {
    send( ch, "This terrain seems devoid of useful forage.\n\r" );
    return;
  }

  const int move = 2*terrain_table[ terrain ].mv_cost;

  if( ch->move < move ) {
    send( ch, "You are too exhausted to search for food.\n\r" );
    return;
  }

  ch->move -= move;

  area_data *area = room->area;

  if( area->forage <= 0 ) {
    send( ch, "You search for provisions, but the area has been picked clean.\n\r" );
    fsend_seen( ch, "%s searches for provisions, but returns empty-handed.", ch );
    area->forage = 0;
    set_delay( ch, 50 );
    return;
  }

  thing_array forage;

  for( int i = 0; i < skill; ++i ) {
    if( number_range( 1, 10 ) == 1 ) {
      int food = number_range( 0, foods.size - 1 );
      obj_data *obj = create( foods[ food ], number_range( 1, nums[ food ] ) );
      forage += obj;
    }
  }

  if( forage.is_empty( ) ) {
    send( ch, "You search for provisions, but find nothing useful.\n\r" );
    fsend_seen( ch, "%s searches for provisions, but returns empty-handed.", ch );
  } else {
    fsend( ch, "You search for provisions, and return with %s.", &forage );
    fsend_seen( ch, "%s searches for provisions, and returns with %s.", ch, &forage );
    for( int i = 0; i < forage; ++i ) {
      forage[i]->To( ch );
    }
    --area->forage;
    ch->improve_skill( SKILL_FORAGE );
  }

  set_delay( ch, 50 );
}


void do_camp( char_data *ch, const char *argument )
{
  const int skill = ch->get_skill( SKILL_CAMPING );

  if( skill == UNLEARNT ) {
    send( ch, "You are unskilled at setting up a campsite.\n\r" );
    return;
  }

  if( matches( argument, "break", (unsigned)3 ) ) {
    send( ch, "There's no camp here to break.\n\r" );
    return;
  }

  if( *argument ) {
    send( ch, "Syntax: camp\n\r\
        camp break\n\r" );
    return;
  }

  if( is_fighting( ch, "build a campfire" )
      || is_entangled( ch, "build a campfire" )
      || is_drowning( ch, "build a campfire" )
      || is_mounted( ch, "build a campfire" )
      || is_confused( ch, "build a campfire" ) ) {
    return;
  }

  room_data *room = ch->in_room;

  if( water_logged( room ) ) {
    send( ch, "It's too wet here to light a campfire.\n\r" );
    return;
  }

  if( midair( ch ) ) {
    send( ch, "You can't light a campfire here.\n\r" );
    return;
  }

  if( obj_data *fire = find_type( ch, *ch->array, ITEM_FIRE ) ) {
    fsend( ch, "You can't light a campfire with %s here.", fire );
    return;
  }

  if( is_set( room->room_flags, RFLAG_ALTAR )
      || is_set( room->room_flags, RFLAG_ALTAR_GOOD )
      || is_set( room->room_flags, RFLAG_ALTAR_NEUTRAL )
      || is_set( room->room_flags, RFLAG_ALTAR_EVIL )
      || is_set( room->room_flags, RFLAG_ALTAR_LAW )
      || is_set( room->room_flags, RFLAG_ALTAR_NEUTRAL2 )
      || is_set( room->room_flags, RFLAG_ALTAR_CHAOS ) ) {
    send( ch, "You feel that the gods might be displeased if you lit a campfire here.\n\r" );
    return;
  }

  if( is_set( room->room_flags, RFLAG_BANK ) ) {
    send( ch, "The bank teller won't permit you to light a campfire here.\n\r" );
    return;
  }

  if( mob_data *keeper = active_shop( ch ) ) {
    fsend( ch, "%s won't permit you to light a campfire here.", keeper );
    return;
  }

  int weight = ( 11 - skill ) * 20;
  thing_array wood;
  for( int i = 0; i < room->contents; ++i ) {
    if( obj_data *obj = object( room->contents[i] ) ) {
      if( !obj->Seen( ch )
	  || !obj->contents.is_empty( )
	  || obj->materials != ( 1 << MAT_WOOD ) )
	continue;
      if( obj->pIndexData->item_type == ITEM_DRINK_CON
	  && obj->value[1] > 0 )
	continue;
      wood += obj;
      if( obj->Weight( ) > weight ) {
	int n = weight / obj->Weight( 1 );
	int m = weight % obj->Weight( 1 );
	if( m > 0 )
	  ++n;
	obj->Select( n );
	weight -= obj->Weight( n );
      } else {
	obj->Select_All( );
	weight -= obj->Weight( );
      }
      if( weight <= 0 ) {
	break;
      }
    }
  }

  if( weight > 0 ) {
    send( ch, "There isn't enough wood here to start a campfire.\n\r" );
    return;
  }

  // In case we added bigger pieces later, remove any smaller, unnecessary pieces.
  for( int i = 0; i < wood; ++i ) {
    obj_data *obj = (obj_data*) wood[i];
    if( weight + obj->Weight( obj->Selected( ) ) <= 0 ) {
      weight += obj->Weight( obj->Selected( ) );
      wood -= obj;
    }
  }

  fsend( ch, "Using %s for fuel, you quickly light a blazing campfire.",
	 &wood );
  fsend_seen( ch, "Using %s for fuel, %s quickly lights a blazing campfire.",
	      &wood, ch );

  ch->improve_skill( SKILL_CAMPING );

  for( int i = 0; i < wood; ++i ) {
    obj_data *obj = (obj_data*) wood[i];
    obj->Extract( obj->Selected( ) );
  }

  obj_data *obj = create( get_obj_index( OBJ_CAMPFIRE ) );

  // NOTE: if you change this formula, change the "camp break"
  // oprogs on all the fire objects!
  obj->value[0] = 5 + skill * 20;

  obj->To( room );
}


void do_cover( char_data* ch, const char * )
{
  if( is_set( ch->status, STAT_COVER_TRACKS ) ) {
    remove_bit( ch->status, STAT_COVER_TRACKS );
    send( ch, "You stop covering your tracks.\n\r" );
    return;
  }

  if( ch->get_skill( SKILL_COVER_TRACKS ) == UNLEARNT ) {
    send( ch, "You don't know how to cover tracks.\n\r" );
    return;
  }
  
  if( is_mounted( ch, "cover tracks" ) )
    return;

  set_bit( ch->status, STAT_COVER_TRACKS );
  
  send( ch, "You start covering your tracks.\n\r" );
  send( ch, "[Covering tracks increases movement cost by 2 points.]\n\r" );
}
