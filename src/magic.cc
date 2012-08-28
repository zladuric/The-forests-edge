#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "define.h"
#include "struct.h"


const char *stype_name [MAX_STYPE] = {
  "offensive", "peaceful", "self_only", "object", "exit", "world",
  "peaceful_other", "weapon", "drink_container", "mob_only", "annoying",
  "corpse", "recall", "weapon_armor", "replicate", "astral", "no_target",
  "wearable", "peaceful_any", "summon", "song"
};


bool null_caster( char_data* ch, int spell )
{
  if( !ch ) {
    bug( "%s: Null pointer for caster.", skill_spell_table[ skill_number(spell) ].name );
    return true;
  }

  return false;
}


/*
 *   LOCAL FUNCTIONS 
 */


void disrupt_spell( char_data* ch, bool self )
{
  if( !ch || !ch->cast )
    return;

  if( ch->cast->prepare ) {
    if( self ) 
      fsend( ch, "You abort preparing %s.",
	     skill_spell_table[ ch->cast->spell ].name );
    else
      send( ch, "\n\r>> Your %s preparation is disrupted. <<\n\r\n\r",
	    skill_spell_table[ch->cast->spell].name );
  } else {
    if( self ) {
      const char *const cast_word = is_set( skill_spell_table[ch->cast->spell].location, LOC_PERFORM )
	? " performing" : "casting";    
      fsend( ch, "You abort %s %s.",
	     cast_word, skill_spell_table[ ch->cast->spell ].name );
    } else {
      const char *const cast_word = is_set( skill_spell_table[ch->cast->spell].location, LOC_PERFORM )
	? " performance" : "casting";    
      send( ch, "\n\r>> Your %s %s is disrupted. <<\n\r\n\r",
	    skill_spell_table[ch->cast->spell].name, cast_word );
    }
  }

  delete ch->cast;
  ch->cast = 0;

  update_max_mana( ch );

  //  set_delay( ch, 3 );
}
 

/*
 *   CAST_DATA CREATOR/DESTRUCTOR
 */


cast_data :: cast_data( )
  : next(0), level(-1), speed(-1), target(0)
{
  record_new( sizeof( cast_data ), MEM_SPELL );
  vzero( reagent, MAX_SPELL_WAIT );
}
 

cast_data :: ~cast_data( )
{
  record_delete( sizeof( cast_data ), MEM_SPELL );
}


/*
 *   CHECK_MANA ROUTINES
 */


static int mana_absorption( char_data* ch )
{
  int total = 0; 

  for( int i = 0; i < ch->wearing; i++ ) {
    obj_data *obj = (obj_data*) ch->wearing[i];
    if( obj->metal( ) ) {
      int cost = 0;
      unsigned number = 0;
      for( int j = 0; j <= table_max[TABLE_MATERIAL]; j++ ) 
        if( is_set( obj->materials, j ) ) {
          ++number;
          if( j >= MAT_BRONZE )
            cost += obj->Empty_Weight( )*material_table[j].mana;
	}
      if( number > 0 )
        total += cost/number;
    }
  }
  
  return total;
}


int mana_cost( char_data* ch, int spell, const char *func )
{
  if( ch->species )
    return 0;

  const int sp = SPELL_FIRST + spell;
  const int level = ch->get_skill( sp );
  bool error = false;
  int mana = evaluate( skill_spell_table[spell].cast_mana, error, level );

  if( error ) {
    if( func ) {
      code_bug( "%s: Evaluate failed for %s.", func, skill_spell_table[spell].name );
    } else {
      bug( "Mana_Cost: Evaluate failed for %s.", skill_spell_table[spell].name );
    }
  }

  if( ch->pcdata && ch->pcdata->clss == CLSS_MAGE )
    mana += mana*mana_absorption( ch )/10000;

  // This allows some spells to have cast mana and leech mana formulae that overlap.
  // The casting cost is the greater of the two.

  const int leech = leech_mana( sp, level );

  return max( mana, leech );
}


int check_mana( char_data* ch, int spell, const char *const msg )
{
  const int i = mana_cost( ch, spell );

  if( ch->mana < i ) {
    send( ch, "You need %d energy points to %s %s.\n\r",
	  i, msg, skill_spell_table[spell].name );
    return -1;
  }

  return i;
}


/*
 *   REAGENT ROUTINES
 */


static int used_reagent( cast_data* cast, obj_data* obj )
{
  int count = 0;

  if( obj_data *target = object( cast->target ) ) {
    if( target == obj )
      ++count;
  }

  for( int i = 0; i < MAX_SPELL_WAIT; ++i )
    if( cast->reagent[i] == obj 
	&& skill_spell_table[cast->spell].reagent[i] > 0 )
      ++count;
  
  return count;
}


static int reagent_uses( obj_data *obj )
{
  if( obj->pIndexData->item_type != ITEM_REAGENT ) {
    return obj->Number( );
  }

  if( obj->value[0] < 0 ) {
    return INT_MAX;
  }

  if( obj->value[0] == 0 ) {
    return obj->Number( );
  }

  return obj->Number( ) * obj->value[0];
}


bool has_reagents( char_data* ch, cast_data* cast )
{ 
  int                spell  = cast->spell;
  bool             prepare  = cast->prepare;
  int         wait_prepare  = skill_spell_table[spell].prepare;
  obj_data*            obj;
  obj_clss_data*  obj_clss;
  obj_data *target = object( cast->target );

  for( int i = ( prepare ? 0 : wait_prepare );
       i < ( prepare ? wait_prepare : MAX_SPELL_WAIT );
       ++i ) {
    
    if( !( obj_clss = get_obj_index( abs( skill_spell_table[spell].reagent[i] ) ) ) )
      continue;
    
    bool found = false;
    bool cross = is_set( obj_clss->extra_flags, OFLAG_DIVINE );

    if( !cross ) {
      for( int j = 0; j < ch->contents; ++j ) {
	if( ( obj = object( ch->contents[j] ) )
	    && obj->pIndexData == obj_clss ) {
	  if( used_reagent( cast, obj ) < reagent_uses( obj ) ) {
	    cast->reagent[i] = obj;
	    break;
	  } else if( target == obj ) {
	    found = true;
	  }
	}
      }
    }
    
    if( !cast->reagent[i] ) {
      for( int j = 0; j < ch->wearing; ++j ) {
	if( ( obj = object( ch->wearing[j] ) )
	    && ( obj->pIndexData == obj_clss
		 || ( cross
		      && is_set( obj->extra_flags, OFLAG_DIVINE ) ) ) ) {
	  if( used_reagent( cast, obj ) < reagent_uses( obj ) ) {
	    cast->reagent[i] = obj;
	    break;
	  } else if( target == obj ) {
	    found = true;
	  }
	}
      }
    }

    if( !cast->reagent[i] ) {
      if( !cross
	  && ( !ch->pcdata
	       || privileged( ch, LEVEL_APPRENTICE ) ) ) {
	//	       || ch->Level() >= LEVEL_APPRENTICE ) ) {
	fsend( ch, "You create %s.", obj_clss->Name( ) );
        obj = create( obj_clss );
        cast->reagent[i] = obj;
	// If ch->cast isn't set, consolidation in To() won't update other reagents.
	ch->cast = cast;
        obj->To( ch );
	ch->cast = 0;
        continue;
      }
      
      const char *const cast_word = is_set( skill_spell_table[spell].location, LOC_PERFORM )
	? " perform" : "cast";

      if( found ) {
	if( target->pIndexData == obj_clss ) {
	  fsend( ch, "You cannot %s %s upon %s, since it would be used up as a reagent.",
		 cast_word, skill_spell_table[spell].name, target );
	  return false;
	}
      }

      const char *const word = prepare ? "prepare" : cast_word;

      if( !cross ) {
	fsend( ch, "You need %s to %s %s.",
	       obj_clss->Name( ), word, skill_spell_table[spell].name );
      } else {
	fsend( ch, "You must be wearing or holding %s to %s %s.",
	       obj_clss->Name( ), word, skill_spell_table[spell].name );
      }
      return false;
    }
  }
  
  return true;
}


static void remove_reagent( cast_data *cast, obj_data* reagent, char_data* ch )
{
  if( reagent->pIndexData->item_type != ITEM_REAGENT ) {
    reagent->Extract( 1 );
    return;
  }
  
  if( reagent->value[0] < 0 ) {
    return;
  }
  
  if( reagent->value[0] <= 1 ) {
    reagent->Extract( 1 );
    return;
  }
  
  if( reagent->Number( ) > 1 ) {
    // Keep using the first reagent through repeated castings.
    obj_data *others = (obj_data *) reagent->From( reagent->Number( ) - 1, true );
    --reagent->value[0];
    others->To( );
  } else {
    --reagent->value[0];
  }
}


/*
 *   CASTING ROUTINES
 */


static int match_spell( const char *& argument, const char *name )
{
  if( !*argument )
    return 0;

  char word [MAX_INPUT_LENGTH];

  // Have to match the first word.
  name = one_argument( name, word );

  if( !matches( argument, word ) )
    return 0;

  int count = 1;

  while( *word && *argument ) {
    name = one_argument( name, word );
    if( matches( argument, word ) )
      ++count;
  }

  return count;
}


bool find_spell( char_data *ch, const char *& argument,
		 int& spell, bool perf )
{
  int k = 0;
  int l = 0;
  int m = -1;
  int n = -1;

  const char *tmp = 0;

  for( int i = 0; i < table_max[ TABLE_SKILL_SPELL ]; ++i ) {
    const char *str = argument;
    if( int count = match_spell( str, skill_spell_table[i].name ) ) {
      if( ch->get_skill( SPELL_FIRST+i ) != UNLEARNT ) {
	if( count > k ) {
	  k = count;
	  n = i;
	  tmp = str;
	}
      } else if( count > l ) {
	// Matches a spell name, don't have the spell.
	l = count;
	m = i;
      }
    }
  }

  if( k > 0 && k >= l ) {
    spell = n;
    argument = tmp;
    return true;
  }

  if( l > 0 ) {
    if( perf ) {
      fsend( ch, "You don't know how to perform %s.",
	     skill_spell_table[m].name );
    } else {
      fsend( ch, "You don't know the spell %s.",
	     skill_spell_table[m].name );
    }
  } else {
    if( perf ) {
      send( ch, "Unknown performance.\n\r" );
    } else {
      send( ch, "Unknown spell.\n\r" );
    }
  }

  return false;
}


void do_cast( char_data* ch, const char *argument )
{
  if( is_confused_pet( ch )
      || is_familiar( ch )
      || is_silenced( ch, "cast spells" )
      || is_entangled( ch, "cast spells" )
      || is_drowning( ch, "cast spells" ) ) {
    return;
  }
  
  if( !*argument ) {
    send( ch, "What spell do you want to cast?\n\r" );
    return;
  }

  int spell;

  if( !find_spell( ch, argument, spell ) )
    return;

  if( is_set( skill_spell_table[spell].location, LOC_PERFORM ) ) {
    fsend( ch, "You can't cast %s; try performing it instead.",
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
  } else if( ( mana = check_mana( ch, spell, "cast" ) ) < 0 ) {
    return;
  }

  if( !allowed_location( ch, &skill_spell_table[spell].location,
			 "cast", skill_spell_table[spell].name ) )
    return;

  cast_data *cast = new cast_data;
  cast->spell = spell;
  cast->prepare = false;
  cast->wait = skill_spell_table[spell].prepare-1;
  cast->mana = mana;

  if( !get_target( ch, cast, argument )
      || !has_reagents( ch, cast ) ) {
    delete cast;
    return;
  }
  
  fsend( ch, "You begin casting %s.", skill_spell_table[spell].name );
  
  if( !ch->species && skill_spell_table[spell].prepare != 0 ) {
    if( --prepare->times == 0 ) { 
      remove( ch->prepare, prepare );
      delete prepare;
    } else if( is_set( ch->pcdata->message, MSG_SPELL_COUNTER ) ) {
      send( ch, "[ You have %s %s spell%s remaining. ]\n\r", 
	    number_word( prepare->times, ch ), skill_spell_table[spell].name, 
	    prepare->times == 1 ? "" : "s" );
    }
  }
  
  ch->cast = cast;
  ch->mana -= mana;

  set_delay( ch, 10 - ch->get_skill( SPELL_FIRST+spell )/2 );
}


void do_prepare( char_data* ch, const char *argument )
{
  char           tmp  [ MAX_INPUT_LENGTH ];
  cast_data*    cast;
  int          spell;
  int           mana;

  if( ch->species ) {
    send( ch, "Only players can prepare spells.\n\r" );
    return;
  }

  if( !*argument ) {
    if( !ch->prepare ) {
      send( ch, "You have no spells prepared.\n\r" );
      return;
    }
    page_underlined( ch, "Num  Spell                   Mana\n\r" );
    for( cast = ch->prepare; cast; cast = cast->next ) {
      snprintf( tmp, MAX_INPUT_LENGTH, "%3d  %-25s%3d\n\r", cast->times,
		skill_spell_table[cast->spell].name, cast->mana*cast->times );
      page( ch, tmp );
    }
    
    return;
  }
  
  if( !strcasecmp( "clear", argument ) ) {
    delete_list( ch->prepare );
    send( ch, "All prepared spells forgotten.\n\r" );
    update_max_mana( ch );
    return;
  }

  if( ch->position < POS_RESTING ) {
    pos_message( ch );
    return;
  }

  if( is_silenced( ch, "prepare spells" )
      || is_entangled( ch, "prepare spells" )
      || is_drowning( ch, "prepare spells" ) ) {
    return;
  }
  
  if( !find_spell( ch, argument, spell ) )
    return;

  if( *argument ) {
    send( ch, "Unknown Spell.\n\r" );
    return;
  }

  if( skill_spell_table[spell].prepare == 0 ) {
    fsend( ch, "%s is not a spell which requires preparation.",
	   skill_spell_table[spell].name );
    return;
  } 

  if( ( mana = check_mana( ch, spell, "prepare" ) ) < 0 )
    return;

  cast           = new cast_data;
  cast->spell    = spell;
  cast->wait     = -1;
  cast->prepare  = true;
  cast->mana     = mana;
  
  if( !has_reagents( ch, cast ) ) {
    delete cast;
    return;
  }
  
  fsend( ch, "You begin preparing %s.", skill_spell_table[spell].name );

  ch->cast    = cast;
  ch->mana   -= mana;

  set_delay( ch, 16 - ch->get_skill( SPELL_FIRST+spell )/2 );
}


cast_data* has_prepared( char_data* ch, int spell )
{
  if( !ch->species && skill_spell_table[spell].prepare != 0 ) 
    for( cast_data *prepare = ch->prepare; prepare; prepare = prepare->next )
      if( prepare->spell == spell )
        return prepare;
  
  return 0;
}


/*
 *   UPDATE ROUTINE
 */


static void spell_action( int action, char_data* ch, visible_data* target,
			  obj_data* reagent )
{
  if( action < 0 || action > table_max[ TABLE_SPELL_ACT ] ) {
    roach( "Spell_Action: Impossible action %d.", action );
    roach( "-- Caster = %s", ch->Name( ) );
    return;
  }
  
  // act_print() uses obj->selected, not obj->shown.

  if( reagent ) {
    reagent->Select( 1 );
  } else {
    reagent = object( target );
  }
  
  if( target == ch ) {
    act( ch, spell_act_table[action].self_self, ch, 0, reagent );
    act_social_room( spell_act_table[action].others_self, ch, 0, reagent );
  } else if( char_data *victim = character( target ) ) {
    act( ch, spell_act_table[action].self_other, ch, victim, reagent );
    if( *spell_act_table[ action ].victim_other ) {
      if( victim->position > POS_SLEEPING ) {
	act_social( victim, spell_act_table[ action ].victim_other, ch, victim, reagent );
      }
      act_social_room( spell_act_table[ action ].others_other, ch, victim, reagent );
    } else {
      act_social_room( spell_act_table[ action ].others_other, ch, 0, reagent );
    }
  } else if( obj_data *obj = object( target ) ) {
    act( ch, spell_act_table[action].self_other, ch, 0, reagent, obj );
    act_social_room( spell_act_table[action].others_other, ch, 0, reagent, obj );
  } else if( exit_data *door = exit( target ) ) {
    act( ch, spell_act_table[action].self_other, ch, 0, reagent, 0, door );
    act_social_room( spell_act_table[action].others_self, ch, 0, reagent, 0, door );
  } else {
    act( ch, spell_act_table[action].self_self, ch, 0, reagent );
    act_social_room( spell_act_table[action].others_self, ch, 0, reagent );
  }
}


bool react_spell( char_data *ch, char_data *victim, int spell )
{
  if( !ch || !ch->Is_Valid( ) )
    return true;

  if( skill_spell_table[spell].type == STYPE_ANNOYING ) {
    room_data *room = ch->in_room;
    thing_array stuff = room->contents;
    for( int i = 0; i < stuff; ++i ) {
      if( !ch->Is_Valid( )
	  || ch->in_room != room )
	return false;
      char_data *rch = character( stuff[i] );
      if( rch
	  && rch != ch
	  && rch->Is_Valid()
	  && rch->in_room == room
	  && can_kill( ch, rch, false ) ) {
	if( ch->fighting != rch ) {
	  trigger_attack( ch, rch );
	}
	react_attack( ch, rch );
      }
    }
  } else if( skill_spell_table[spell].type == STYPE_OFFENSIVE ) {
    if( !ch->fighting ) {
      if( !set_fighting( ch, victim ) ) {
	return false;
      }
      set_delay( ch, 2 );
    } else {
      react_attack( ch, victim );
    }
  }

  return true;
}


bool cast_triggers( int spell, char_data *ch,
		    visible_data *target, char_data *victim, obj_data *obj )
{
  room_data *room = ch ? ch->in_room :
    victim ? victim->in_room : 0;

  if( room ) {
    for( action_data *action = room->action; action; action = action->next ) {
      if( action->trigger == TRIGGER_CAST
	  && ( !*action->target
	       || !strcasecmp( action->target, skill_spell_table[spell].name ) ) ) {
	push( );
	clear_variables( );
	var_ch = ch;
	var_victim = victim;
	var_obj = obj;
	var_exit = exit( target );
	var_room = room;
	var_arg = skill_spell_table[spell].name;
	const int result = action->execute( );
	pop( );
	if( !result
	    || ch && !ch->Is_Valid( )
	    || victim && !victim->Is_Valid( )
	    || obj && !obj->Is_Valid( )
	    )
	  return true;
	if( *action->target )
	  break;
      }
    }

    if( !victim ) {
      thing_array stuff = room->contents;
      for( int i = 0; i < stuff; ++i ) {
	char_data *rch = mob( stuff[i] );
	if( rch && rch->Is_Valid( ) ) {
	  for( mprog_data *mprog = rch->species->mprog; mprog; mprog = mprog->next ) {
	    if( mprog->trigger == MPROG_TRIGGER_CAST
		&& ( !*mprog->string
		     || !strcasecmp( mprog->string, skill_spell_table[spell].name ) ) ) {
	      push( );
	      clear_variables( );
	      var_ch = ch;
	      var_victim = rch;
	      var_obj = obj;
	      var_exit = exit( target );
	      var_room = room;
	      var_arg = skill_spell_table[spell].name;
	      const int result = mprog->execute( );
	      pop( );
	      if( !result )
		return true;
	      if( *mprog->string
		  || !rch->Is_Valid( ) )
		break;
	    }
	  }
	}
      }
    }

    /*
    if( !obj ) {
      thing_array stuff = room->contents;
      for( int i = 0; i < stuff; ++i ) {
	obj_data obj2 = object( stuff[i] );
	if( obj2 && obj2->Is_Valid( ) ) {
	  for( oprog_data *oprog = obj2->pIndexData->oprog; oprog; oprog = oprog->next ) {
	    if( oprog->trigger == OPROG_TRIGGER_CAST
		&& ( !*oprog->target
		     || !strcasecmp( oprog->target, skill_spell_table[spell].name ) ) ) {
	    }
	  }
	}
      }
    }
    */

    /*
    thing_array stuff = room->contents;
    for( int i = 0; i < stuff; ++i ) {
      char_data *rch = character( stuff[i] );
      if( rch && rch->Is_Valid( ) {
	thing_array worn = rch->wearing;
	for( int j = 0; j < worn; ++j ) {
	  obj_data *obj2 = (obj_data*) worn[j];
	  for( oprog_data *oprog = obj2->pIndexData->oprog; oprog; oprog = oprog->next ) {
	    if( oprog->trigger == OPROG_TRIGGER_CAST_WORN
		&& ( !*oprog->target
		     || !strcasecmp( oprog->target, spell_table[spell].name ) ) ) {

	    }
	  }
	}
      }
    }
    */
  }
  
  if( victim && victim->species && !obj ) {
    for( mprog_data *mprog = victim->species->mprog; mprog; mprog = mprog->next ) {
      if( mprog->trigger == MPROG_TRIGGER_CAST
	    && ( !*mprog->string
		 || !strcasecmp( mprog->string, skill_spell_table[spell].name ) ) ) {
	push( );
	clear_variables( );
	var_ch = ch;
	var_victim = victim;
	var_room = room;
	var_arg = skill_spell_table[spell].name;
	const int result = mprog->execute( );
	pop( );
	if( !result
	    || !victim->Is_Valid( )
	    || ch && !ch->Is_Valid( )
	    )
	  return true;
	if( *mprog->string )
	  break;
      }
    }
  }

  if( obj ) {
    for( oprog_data *oprog = obj->pIndexData->oprog; oprog; oprog = oprog->next ) {
      if( oprog->trigger == OPROG_TRIGGER_CAST
	  && ( !*oprog->target
	       || !strcasecmp( oprog->target, skill_spell_table[spell].name ) ) ) {
	push( );
	clear_variables( );
	var_ch = ch;
	var_victim = victim;
	var_obj = obj;
	var_room = room;
	var_arg = skill_spell_table[spell].name;
	const int result = oprog->execute( );
	pop( );
	if( !result
	    || !obj->Is_Valid( )
	    || victim && !victim->Is_Valid( )
	    || ch && !ch->Is_Valid( )
	    )
	  return true;
	if( *oprog->target )
	  break;
      }
    }
  }

  return false;
}


void finish_spell( const time_data& start, int spell )
{
  const time_data time = stop_clock( start );
  
  if( skill_spell_table[spell].max_time < time )
    skill_spell_table[spell].max_time = time;
  
  skill_spell_table[spell].total_time += time;
  ++skill_spell_table[spell].calls;
}


void spell_update( char_data* ch )
{
  cast_data *cast = ch->cast;
  visible_data *target = cast->target;
  char_data *victim = character( target );
  obj_data *obj = object( target );
  
  const int spell = cast->spell;
  const bool prepare = cast->prepare;
  const int wait = ++cast->wait;
  const int level = ( cast->level == -1 )
    ? ch->get_skill( SPELL_FIRST+spell )
    : cast->level;
  const int speed  = ( cast->speed == -1 )
    ? level
    : cast->speed;
  const int type = skill_spell_table[spell].type;

  if( target )
    target->Select( 1 );

  if( type == STYPE_SONG ) {
    // For songs, audience members must be able to hear each step.
    // If they leave the room, stop_fight( ) has already removed them from audience.
    for( int i = 0; i < cast->audience; ) {
      char_data *rch = cast->audience[i];
      if( !rch->Can_Hear( )
	  || invis_level( rch ) != 0 ) {
	cast->audience.remove( i );
      } else {
	++i;
      }
    }
  }

  if( wait < ( cast->prepare
	       ? skill_spell_table[spell].prepare 
	       : skill_spell_table[spell].wait ) ) {
    
    obj_data *reagent  = cast->reagent[wait];
    const int action = skill_spell_table[spell].action[wait];
    msg_type = cast->prepare ? MSG_PREPARE : MSG_STANDARD;
    
    spell_action( action, ch, target, reagent );

    if( reagent ) {
      cast->reagent[wait] = 0;
      if( skill_spell_table[spell].reagent[wait] >= 0 ) {
	// Note: setting reagent to zero prevents remove_reagent() from
	// disrupting the spell in Obj_Data::From().
	remove_reagent( cast, reagent, ch );
      }
    }
    set_delay( ch, 35 - speed );
    return;
  }
  
  ch->cast = 0;

  set_delay( ch, 2 );
  update_max_mana( ch );
  
  if( !prepare ) { 
    char_array audience;
    audience.swap( cast->audience );
    delete cast;

    if( is_set( ch->in_room->room_flags, RFLAG_NO_MAGIC ) ) {
      fsend( ch,
	     "As you cast %s, you feel the energy drain from you and nothing happens.",
	     skill_spell_table[spell].name );
      fsend_seen( ch, "%s casts %s, but nothing happens.",
		  ch, skill_spell_table[spell].name );
      return;
    }
    
    if( type == STYPE_PEACEFUL
	|| type == STYPE_PEACEFUL_OTHER
	|| type == STYPE_PEACEFUL_ANY ) {
      if( skill_spell_table[spell].regen != empty_string
	  && victim
	  && victim != ch
	  && !victim->species
	  && !ch->species
	  && is_set( victim->pcdata->pfile->flags, PLR_NO_LEECH ) ) {
	fsend( ch,
	       "As you cast %s, you feel resistance from %s and nothing happens.",
	       skill_spell_table[spell].name, victim );
	fsend( victim, "%s casts %s, but you resist the spell.",
	       ch, skill_spell_table[spell].name );
	fsend_seen( ch, "%s casts %s, but %s resists the spell.",
		    ch, skill_spell_table[spell].name, victim );
	return;
      }
    } else if( type == STYPE_WORLD
	       || type == STYPE_SUMMON ) {
      if( victim
	  && victim->in_room
	  && is_set( victim->in_room->room_flags, RFLAG_NO_MAGIC ) ) {
	fsend( ch,
	       "As you cast %s, you sense that %s is beyond your reach.",
	       skill_spell_table[spell].name, victim );
	fsend_seen( ch, "%s casts %s, but nothing happens.",
		    ch, skill_spell_table[spell].name );
	return;
      }
    }

    remove_bit( ch->status, STAT_WIMPY );
    strip_affect( ch, AFF_INVISIBLE );
    leave_shadows( ch );

    const char *const cast_word = is_set( skill_spell_table[spell].location, LOC_PERFORM )
      ? "perform" : "cast";

    send( ch, "+++ You %s %s +++\n\r", cast_word, skill_spell_table[spell].name );
    send_seen( ch, "%s %ss %s.\n\r", ch, cast_word, skill_spell_table[spell].name );

    if( cast_triggers( spell, ch, target, victim, obj ) ) {
      return;
    }

    if( !react_spell( ch, victim, spell ) ) {
      return;
    }
  
    time_data start;
    gettimeofday( &start, 0 );
  
    if( Tprog_Data *tprog = skill_spell_table[spell].prog ) {
      clear_variables( );
      var_i = level;
      //      var_j = 0; // duration 0
      var_ch = ch;
      var_room = ch->in_room;
      var_victim = victim;
      var_obj = obj;
      var_exit = exit( target );
      var_list = (thing_array&)audience;
      const int result = tprog->execute( );
      if( !ch->Is_Valid( ) ) {
	finish_spell( start, spell );
	return;
      }
      if( !result ) {
	if( !ch->species )
	  ch->pcdata->prac_timer = 5;
	finish_spell( start, spell );
	return;
      }
      if( !skill_spell_table[spell].function ) {
	ch->improve_skill( SPELL_FIRST+spell );
	finish_spell( start, spell );
	return;
      }
    }

    void *vo = ( skill_spell_table[spell].type != STYPE_SONG )
      ? (void*)target : (void*)&audience;

    if( skill_spell_table[spell].function
	&& ( *skill_spell_table[spell].function )( ch, character( target ),
						   vo, level, 0 ) ) {
      ch->improve_skill( SPELL_FIRST+spell );
    } else if( !ch->species ) {
      ch->pcdata->prac_timer = 5;
    }
    
    finish_spell( start, spell );
    return;
  }
  
  cast_data *prev;

  for( prev = ch->prepare; prev; prev = prev->next )
    if( prev->spell == spell ) {
      ++prev->times;
      delete cast;
      break;
    }
  
  if( !prev ) {
    cast->times = 1; 
    cast->next  = ch->prepare;
    ch->prepare = cast;
    prev        = cast;
  }
  
  if( prev->times > 1 ) {
    fsend( ch, "You now have %s incantations of %s prepared.",
	   number_word( prev->times, ch ), skill_spell_table[spell].name );
  } else {
    fsend( ch, "You have prepared %s.", skill_spell_table[spell].name );
  }
  
  update_max_mana( ch );
}


/*
 *   RESISTANCE ROUTINES  
 */


bool makes_save( char_data *victim, char_data *ch,
		 int type, int spell, int level )
{
  if( ch == victim )
    return false;
  
  int chance;

  switch( type ) {
  case RES_POISON:
    chance = victim->Save_Poison( );
    break;

  case RES_MAGIC:
    chance = victim->Save_Magic( );
    break;
    
  case RES_MIND:
    chance = victim->Save_Mind( );
    break;
    
  case RES_DEXTERITY:
    if( victim->position <= POS_RESTING )
      return false;
    chance = 3*victim->Dexterity( );
    break;
    
  default:
    bug( "Makes_Save: Unknown Resistance." );
    return true;
  }
  
  if( ch ) {
    chance += victim->Level()-ch->Level();
    /*
    if( ch->pcdata ) {
      int clss_level = skill_table[spell].level[ ch->pcdata->clss ];
      if( clss_level > 0 ) {
	chance += clss_level / 2;
      }
    }
    */
  } else {
    chance += victim->Level() / 2;
  }
  
  if( number_range( 0, 99 ) < chance )
    return true;
  
  if( level > 0 && number_range( 0, 99 ) < 75-15*level/2 )
    return true;

  return false;
}


/*
 *   TABLE EVALUATE ROUTINES
 */


int spell_damage( int spell, int level, int var )
{
  bool error  = false;

  if( level < 0 ) 
    level = (-level)%100;

  spell -= SPELL_FIRST;
  const int damage  = evaluate( skill_spell_table[spell].damage, error, level, var );

  if( error ) {
    bug( "Spell_Damage: Evaluate failed for %s.", skill_spell_table[spell].name );
  }

  return max( 0, damage );
}


int duration( int spell, int level, int var )
{
  bool error = false;

  spell -= SPELL_FIRST;
  const int duration  = evaluate( skill_spell_table[spell].duration, error, level, var );

  if( error ) { 
    bug( "Duration: Evaluate failed for %s.", skill_spell_table[spell].name );
  }

  return max( 0, duration );
}


int leech_regen( int spell, int level, int var )
{
  bool error = false;

  spell -= SPELL_FIRST;
  const int regen = evaluate( skill_spell_table[spell].regen, error, level, var );

  if( error ) {
    bug( "Leech_Regen: Evaluate failed for %s.", skill_spell_table[spell].name );
  }

  return max( 0, regen );
}


int leech_mana( int spell, int level, int var )
{
  bool error = false;

  spell -= SPELL_FIRST;
  const int mana = evaluate( skill_spell_table[spell].leech_mana, error, level, var );

  if( error ) {
    bug( "Leech_Mana: Evaluate failed for %s.", skill_spell_table[spell].name );
  }

  return max( 0, mana );
}


bool spell_affect( char_data* ch, char_data* victim, int level,
		   int time, int spell, int type, int var )
{
  if( time == -4 || time == -3 ) {
    // Filled or dipped object, spell doesn't affect filler/dipper.
   return false;
  }

  affect_data affect;

  affect.type = type;
  affect.level = level;
  
  if( !ch || time > 0 ) {
    if( time > 0 ) {
      affect.duration = time;
    } else {
      affect.duration = duration( spell, level, var );
    }
  } else {
    affect.duration = duration( spell, level, var );
    if( skill_spell_table[ spell-SPELL_FIRST ].regen != empty_string ) {
      affect.leech_regen = leech_regen( spell, level, var );
      affect.leech_max = leech_mana( spell, level, var );
      affect.leech = ch;
    }
  }

  return add_affect( victim, &affect );
}


bool spell_affect( char_data *ch, obj_data* obj, int level,
		   int time, int spell, int type, int var )
{
  affect_data affect;

  affect.type = type;
  affect.level = level;
  
  if( !ch || time > 0 ) {
    affect.duration = time;
  } else {
    affect.duration = duration( spell, level, var );
    //    affect.leech = 0;
    /*
    if( skill_spell_table[ spell-SPELL_FIRST ].regen != empty_string ) {
      affect.leech_regen = leech_regen( spell, level, var );
      affect.leech_max   = leech_mana( spell, level, var );
      affect.leech       = ch;
    }
    */
  }

  add_affect( obj, &affect );
  
  return true;
}


bool spell_affect( char_data *ch, room_data *room, exit_data *exit, bool both, int level,
		   int time, int spell, int type, int var )
{
  affect_data affect;

  affect.type = type;
  affect.level = level;

  if( exit ) {
    affect.target = exit;
    if( both ) {
      affect.location = APPLY_BOTH_SIDES;
    }
  }
  
  if( !ch || time > 0 ) {
    affect.duration = time;
  } else {
    affect.duration = duration( spell, level, var );
    //    affect.leech = 0;
    /*
    if( skill_spell_table[ spell-SPELL_FIRST ].regen != empty_string ) {
      affect.leech_regen = leech_regen( spell, level, var );
      affect.leech_max   = leech_mana( spell, level, var );
      affect.leech       = ch;
    }
    */
  }

  add_affect( room, &affect );
  
  return true;
}


/*
 *   CLERIC/PALADIN SPELLS
 */


/*
bool spell_cause_light( char_data *ch, char_data *victim, void*,
			int level, int duration )
{
  if( duration == -4 || duration == -3 )
    return false;

  damage_magic( victim, ch, spell_damage( SPELL_CAUSE_LIGHT, level ),
		"*The spell" );
  
  return true;
}


bool spell_cause_serious( char_data *ch, char_data *victim, void*,
			  int level, int duration )
{
  if( duration == -4 || duration == -3 )
    return false;

  damage_magic( victim, ch, spell_damage( SPELL_CAUSE_SERIOUS, level ),
		"*The spell" );
  
  return true;
}


bool spell_cause_critical( char_data *ch, char_data *victim, void*,
			   int level, int duration )
{
  if( duration == -4 || duration == -3 )
    return false;

  damage_magic( victim, ch, spell_damage( SPELL_CAUSE_CRITICAL, level ),
		"*The spell" );
  
  return true;
}
*/


bool spell_create_food( char_data *ch, char_data*, void*,
			int level, int )
{
  obj_data*            obj;  
  obj_clss_data*  obj_clss;
  
  if( null_caster( ch, SPELL_CREATE_FOOD ) )
    return false;
  
  level = range( 1, level, 10 );
  
  int type;

  if( vegetarian( ch ) )
    type = LIST_FOOD_HERBI;
  else if( carnivore( ch ) )
    type = LIST_FOOD_CARNI;
  else
    type = LIST_FOOD_OMNI;

  while( true ) {
    int item = number_range( 0, 3*level-1 ); 
    item = max( number_range( 0, 3*level-1 ), item ); 
    item = list_value[ type ][ item ];

    if( ( obj_clss = get_obj_index( item ) ) )
      break;
  }
  
  if( !( obj = create( obj_clss ) ) ) {
    bug( "Create_Food: NULL object" );
    return false;
  }  

  //  if( ch->pcdata ) {
  //    set_owner( obj, ch->pcdata->pfile );
  //  }

  set_bit( obj->extra_flags, OFLAG_NO_AUCTION );
  set_bit( obj->extra_flags, OFLAG_NO_SELL );
  set_bit( obj->extra_flags, OFLAG_NOSACRIFICE );

  if( ch->can_carry_n( ) < ch->contents.number + 1
      || ch->Capacity( ) < obj->Weight( ) ) {
    if( ch->in_room ) {
      obj->To( ch->in_room );
      const char *drop = ch->in_room->drop( );
      if( *drop ) {
	fsend( ch, "%s appears nearby, and falls %s.", obj,
	       drop );
	fsend_seen( ch, "%s appears near %s, and falls %s.", obj, ch,
		    drop );
      } else {
	fsend( ch, "%s appears here.", obj );
	fsend_seen( ch, "%s appears here.", obj );
       }
    }
  } else {
    obj->To( ch );
    fsend( ch, "%s appears in your hand.", obj );
    fsend_seen( ch, "%s appears in %s's hand.", obj, ch );
  }
  
  return true;    
}


bool spell_create_feast( char_data *ch, char_data*, void*,
			 int level, int )
{
  obj_data*            obj;  
  obj_clss_data*  obj_clss;
  
  if( null_caster( ch, SPELL_CREATE_FEAST ) )
    return false;
  
  level = range( 1, level, 10 );

  Content_Array list;

  int type;

  if( vegetarian( ch ) )
    type = LIST_FEAST_HERBI;
  else if( carnivore( ch ) )
    type = LIST_FEAST_CARNI;
  else
    type = LIST_FEAST_OMNI;

  for( int i = 0; i < level; ++i ) {
    while( true ) {
      int item = number_range( 0, 3*level-1 ); 
      item = list_value[ type ][ item ];
      
      if( ( obj_clss = get_obj_index( item ) ) )
	break;
    }
    
    const int count = number_range( 1, (level+3)/2 );
    if( !( obj = create( obj_clss, count ) ) ) {
      bug( "Create_Feast: NULL object" );
      return false;
    }

    if( obj->pIndexData->item_type == ITEM_FOOD ) {
      if( obj->value[1] >= 0 ) {
	obj->value[1] = COOK_COOKED;
      }
    } else if( obj->pIndexData->item_type == ITEM_DRINK_CON ) {
      set_bit( obj->extra_flags, OFLAG_KNOWN_LIQUID );
    }

    set_bit( obj->extra_flags, OFLAG_NO_AUCTION );
    set_bit( obj->extra_flags, OFLAG_NO_SELL );
    set_bit( obj->extra_flags, OFLAG_NOSACRIFICE );

    list += obj;
  }

  if( ch->in_room ) {
    send( ch, "An assortment of food appears here.\n\r", 0 );
    send_seen( ch, "An assortment of food appears here.\n\r", 0 );
    
    for( int i = 0; i < list; ++i ) {
      list[i]->To( ch->in_room );
    }
  }

  return true;
}


bool spell_cure_blindness( char_data* ch, char_data* victim, void*,
			   int level, int time )
{
  if( time == -4 || time == -3 )
    return false;
  
  if( !victim->is_affected( AFF_BLIND ) ) {
    if( ch != victim )
      send( ch, "%s wasn't blind.\n\r", victim );
    else
      send( ch, "You aren't blind!\n\r" );
    return false;
  }

  // Removes same level of blindness.
  int dur = duration( SPELL_BLIND, level );

  if( shorten_affect( victim, AFF_BLIND, dur ) > 0 ) {
    fsend( victim, "Colors swim briefly before your eyes, but you remain blind." );
    fsend_seen( victim, "%s looks a little better.", victim );

  } else if( victim->is_affected( AFF_BLIND ) ) {
    if( ch != victim ) {
      fsend( ch, "%s is still blind!", victim );
    }
    send( victim, "You are still blind!\n\r" );
  }

  //  strip_affect( victim, AFF_BLIND );

  return true;
}


/*
bool spell_curse( char_data* ch, char_data* victim, void*,
		  int level, int duration )
{
  return spell_affect( ch, victim, level, duration,
		       SPELL_CURSE, AFF_CURSE );
}


bool spell_detect_evil( char_data* ch, char_data* victim, void*,
			int level, int duration )
{
  return spell_affect( ch, victim, level, duration,
		       SPELL_DETECT_EVIL, AFF_DETECT_EVIL );
}


bool spell_detect_good( char_data* ch, char_data* victim, void*,
			int level, int duration )
{
  return spell_affect( ch, victim, level, duration,
		       SPELL_DETECT_GOOD, AFF_DETECT_GOOD );
}


bool spell_detect_law( char_data* ch, char_data* victim, void*,
		       int level, int duration )
{
  return spell_affect( ch, victim, level, duration,
		       SPELL_DETECT_LAW, AFF_DETECT_LAW );
}


bool spell_detect_chaos( char_data* ch, char_data* victim, void*,
			 int level, int duration )
{
  return spell_affect( ch, victim, level, duration,
		       SPELL_DETECT_CHAOS, AFF_DETECT_CHAOS );
}
*/


bool spell_faerie_fire( char_data* ch, char_data* victim, void*,
			int level, int duration )
{
  if( duration == -4 || duration == -3 )
    return false;

  strip_affect( victim, AFF_INVISIBLE );
  leave_shadows( victim );

  return spell_affect( ch, victim, level, duration,
		       SPELL_FAERIE_FIRE, AFF_FAERIE_FIRE );
}


/*
bool spell_harm( char_data *ch, char_data *victim, void*,
		 int level, int duration )
{
  if( duration == -4 || duration == -3 )
    return false;

  damage_magic( victim, ch, spell_damage( SPELL_HARM, level ),
		"heaven sent bolt of energy" );
  
  return true;
}
*/


bool spell_neutralize( char_data *ch, char_data *victim, void*, int level, int time )
{
  if( time == -4 || time == -3 )
    return false;

  if( !victim->is_affected( AFF_HALLUCINATE )
      && !victim->is_affected( AFF_SILENCE ) ) {
    send( ch, "Nothing happens.\n\r" );
    send_seen( ch, "Nothing happens.\n\r" );
    return false;
  }

  // Removes same level of hallucinate.
  int dur = duration( SPELL_HALLUCINATE, level );

  if( shorten_affect( victim, AFF_HALLUCINATE, dur ) > 0 ) {
    fsend( victim, "Things almost snap back into focus, but the colors soon return." );
    fsend_seen( victim, "%s looks a little better.", victim );

  } else if( victim->is_affected( AFF_HALLUCINATE ) ) {
    if( ch != victim ) {
      fsend( ch, "%s is still hallucinating!", victim );
    }
    send( victim, "You are still hallucinating!\n\r" );
  }

  // Removes same level of silense.
  dur = duration( SPELL_SILENCE, level );

  if( shorten_affect( victim, AFF_SILENCE, dur ) > 0 ) {
    fsend( victim, "You detect an easing of the silence that grips you, but you remain unable to speak." );
    fsend_seen( victim, "%s looks a little better.", victim );

  } else if( victim->is_affected( AFF_SILENCE ) ) {
    if( ch != victim ) {
      fsend( ch, "%s is still silenced!", victim );
    }
    send( victim, "You are still silenced!\n\r" );
  }

  return true;
}


bool spell_revitalize( char_data* ch, char_data* victim, void* obj,
		       int level, int duration )
{
  if( duration == -4 || duration == -3 )
    return false;

  int move = victim->move+15*level;

  bool any = strip_affect( victim, AFF_DEATH );

  update_maxes( victim );

  if( !any && victim->move >= victim->max_move ) {
    send( ch, "Nothing happens.\n\r" );
    send_seen( ch, "Nothing happens.\n\r" );
    return false;
  }

  if( move >= victim->max_move ) {
    send( victim, "You are completely revitalized.\n\r" );
    fsend_seen( victim, "%s is completely revitalized.", victim );
    victim->move = victim->max_move;
    return true;
  }
  
  send( victim, "You are partially revitalized.\n\r" );
  fsend_seen( victim, "%s is partially revitalized.", victim );
	      
  victim->move = move;

  return true;
}


/*
bool spell_slay( char_data *ch, char_data *victim, void*,
		 int level, int duration )
{

  if( duration == -4 || duration == -3 )
    return false;

  damage_magic( victim, ch, spell_damage( SPELL_SLAY, level ),
		"*The divine fury of the channeled power" );

  return true;
}
*/


/*
 *   RANGER SPELLS
 */


/*
bool spell_protection_plants( char_data *ch, char_data *victim, void*,
			      int level, int duration )
{
  return spell_affect( ch, victim, level, duration,
		       SPELL_PROT_PLANTS, AFF_PROT_PLANTS );
}
*/

/*
 *   MAGE SPELLS
 */


/*
bool spell_detect_hidden( char_data *ch, char_data *victim, void*,
			  int level, int duration )
{
  return spell_affect( ch, victim, level, duration,
		       SPELL_DETECT_HIDDEN, AFF_DETECT_HIDDEN );
}


bool spell_displace( char_data *ch, char_data *victim, void*,
		     int level, int duration )
{
  return spell_affect( ch, victim, level, duration,
		       SPELL_DISPLACE, AFF_DISPLACE );
}
*/


bool spell_invisibility( char_data* ch, char_data* victim, void*,
			 int level, int duration )
{
  if( duration == -4 || duration == -3 )
    return false;
  
  if( victim->is_affected( AFF_FAERIE_FIRE ) ) {
    if( ch != victim ) {
      send( ch, "Nothing happens.\n\r" );
    }
    fsend( victim, "Your fiery glow makes it impossible to become invisible." );
    return false;
  }
  
  /*
  if( victim->is_affected( AFF_FIRE_SHIELD )
      || victim->is_affected( AFF_FAERIE_FIRE ) ) {
    if( ch != victim ) {
      send( ch, "Nothing happens.\n\r" );
    }
    fsend( victim, "Your fiery glow makes it impossible to become invisible." );
    return false;
  }
  
  if( victim->is_affected( AFF_ION_SHIELD ) ) {
    if( ch != victim ) {
      send( ch, "Nothing happens.\n\r" );
    }
    fsend( victim, "Your shield of sparks makes it impossible to become invisible." );
    return false;
  }
  
  if( victim->is_affected( AFF_ICE_SHIELD ) ) {
    if( ch != victim ) {
      send( ch, "Nothing happens.\n\r" );
    }
    fsend( victim, "Your shield of whirling ice crystals makes it impossible to become invisible." );
    return false;
  }
  */
  
  return spell_affect( ch, victim, level, duration,
		       SPELL_INVISIBILITY, AFF_INVISIBLE );
}


bool spell_locust_swarm( char_data *ch, char_data*, void*,
			 int level, int )
{
  if( null_caster( ch, SPELL_LOCUST_SWARM ) )
    return false;

  if( is_submerged( 0, ch->in_room ) ) {
    //ch->in_room->sector_type == SECT_UNDERWATER ) {
    send( ch, "The insects don't seem to be responding.\n\r" );
    fsend_seen( ch, "%s looks around expectantly and frowns.", ch );
    return false;
  }

  const int duration = number_range( (level+1)/2, level+1 );

  obj_data *obj;

  if( ( obj = find_vnum( *ch->array, OBJ_LOCUST_SWARM ) ) ) {
    if( duration > obj->value[0] ) {
      send( ch, "The existing swarm thickens.\n\r" );
      // Don't re-run to_room trigger.
      //      obj->From( obj->number );
      obj->value[0] = duration;
      //      obj->To( ch->array );
      return true;
    }
    send( ch, "Nothing seems to happen.\n\r" );
    return false;
  }
  
  obj = create( get_obj_index( OBJ_LOCUST_SWARM ) );
  obj->value[0] = duration;
  obj->value[3] = (int) ch;
  obj->To( *ch->array );

  return true;
}


bool spell_poison_cloud( char_data *ch, char_data*, void*, 
			 int level, int )
{
  if( null_caster( ch, SPELL_POISON_CLOUD ) )
    return true;
  
  if( is_submerged( 0, ch->in_room ) ) {
    //  if( ch->in_room->sector_type == SECT_UNDERWATER ) {
    send( ch, "You fail to raise the cloud underwater.\n\r" );
    fsend_seen( ch, "%s looks around expectantly and frowns.", ch );
    return true;
  }
  
  int duration = number_range( (level+1)/2, level+1 );

  obj_data *obj;
  
  if( ( obj = find_vnum( *ch->array, OBJ_POISON_CLOUD ) ) ) {
    if( duration > obj->value[0] ) {
      send( ch, "The existing cloud thickens.\n\r" );
      // Don't re-run to_room trigger.
      //      obj->From( obj->number );
      obj->value[0] = duration;
      //      obj->To( ch->array );
      return true;
    }
    send( ch, "Nothing seems to happen.\n\r" );
    return false;
  }
  
  obj = create( get_obj_index( OBJ_POISON_CLOUD ) );
  obj->value[0] = duration;
  obj->value[3] = (int) ch;
  obj->To( *ch->array );
  
  return true;
}


/*
bool spell_mystic_shield( char_data* ch, char_data* victim, void*,
			  int level, int duration )
{
  return spell_affect( ch, victim, level, duration, SPELL_MYSTIC_SHIELD,
		       AFF_PROTECT );
}


bool spell_infravision( char_data *ch, char_data *victim, void*,
			int level, int duration )
{
  return spell_affect( ch, victim, level, duration,
		       SPELL_INFRAVISION, AFF_INFRARED );
}


bool spell_darkvision( char_data *ch, char_data *victim, void*,
		       int level, int duration )
{
  return spell_affect( ch, victim, level, duration,
		       SPELL_DARKVISION, AFF_DARKVISION );
}


bool spell_detect_invisible( char_data* ch, char_data* victim, void*,
			     int level, int duration )
{
  return spell_affect( ch, victim, level, duration,
		       SPELL_DETECT_INVISIBLE, AFF_SEE_INVIS );
}    


bool spell_vitality( char_data *ch, char_data *victim, void*, int level,
		     int duration )
{
  return spell_affect( ch, victim, level, duration,
		       SPELL_VITALITY, AFF_VITALITY );
}
*/


bool spell_calm( char_data* ch, char_data* victim, void*, int level, int )
{
  if( null_caster( ch, SPELL_CALM ) ) 
    return false;

  if( victim->position < POS_RESTING ) {
    fsend( ch, "%s is unconscious and so the spell has no affect.",
	   victim );
    return false;
  }
  
  if( victim->fighting != ch ) {
    fsend( ch, "%s isn't fighting you.", victim );
    return false;
  }
  
  if( makes_save( victim, ch, RES_MIND, SPELL_CALM, level ) ) {
    send( victim, "You are unaffected by the calm spell.\n\r" );
    fsend_seen( victim,
	       "%s seems to pause a moment but then continues to fight!",
		victim );
    return false;
  }
  
  stop_fight( victim );

  fsend( ch, "%s stops attacking you.", victim );
  fsend( victim, "You don't feel like fighting %s any more.", ch );
  fsend_seen( victim, "%s stops attacking %s.", ch, victim );

  return true;
} 
