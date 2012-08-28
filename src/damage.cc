#include <ctype.h>
#include <sys/types.h>
#include <stdio.h>
#include <syslog.h>
#include "define.h"
#include "struct.h"


/*
 *   GENERAL MESSAGE ROUTINES
 */


static void strcat_cond( const char *tmp, char_data* ch, char_data* victim )
{
  const char *cond_word  = condition_word( victim ); 
  bool           visible  = true;
  int             length  = 0;
  
  if( !victim->Seen( ch ) ) {
    fsend( ch, "%s", tmp );
    return;
  }

  for( int i = 0; tmp[i]; ++i ) {
    if( visible ) {
      if( tmp[i] == '' )
        visible = false;
      else
        ++length;
    } else if( tmp[i] == 'm' ) {
      visible = true;
    }
  }
  
  const unsigned width = ( ch && ch->pcdata ) ? ch->pcdata->columns : 80;
  const unsigned col = fput( ch, tmp );

  if( col + 6 + strlen( cond_word ) >= width ) {
    send( ch, "\n\r[ %s ]\n\r", cond_word );
  } else {
    send( ch, "  [ %s ]\n\r", cond_word );
  }
}
  

void dam_local( char_data* victim, char_data* ch, int damage,
		const char* dt, bool plural, const char* loc_name )
{
  if( victim->is_affected( AFF_SANCTUARY ) )
    damage = 0;
  else
    damage = min( damage, DAMAGE_LIMIT );

  const char *dam_name = lookup( physical_index, damage, plural );

  if( !*loc_name ) {
    dam_message( victim, ch, damage, dt, dam_name );
    return;
  }
  
  char               tmp  [ MAX_STRING_LENGTH ];
  char_data*         rch;

  const char punct = ( damage <= 24 ? '.' : '!' );
  victim->hit -= damage;
  
  if( victim->link ) {
    snprintf( tmp, MAX_STRING_LENGTH, "%s's %s %s%s%s your %s%c",
	      ch->Name( victim, 1 ),
	      dt,
	      to_self( victim ), dam_name, normal( victim ),
	      loc_name,
	      punct );
    *tmp = toupper( *tmp );
    fsend( victim, tmp );
  }
  
  if( ch->link ) {
    if( victim->Seen( ch ) ) {
      snprintf( tmp, MAX_STRING_LENGTH, "Your %s %s%s%s %s's %s%c",
		dt, by_self( ch ), dam_name, normal( ch ),
		victim->Name( ch, 1 ), loc_name, punct );
    } else {
      snprintf( tmp, MAX_STRING_LENGTH, "Your %s %s%s%s someone%c",
		dt, by_self( ch ),
		dam_name, normal( ch ), punct );
    }
    
    strcat_cond( tmp, ch, victim );
  }
  
  
  const int flag = ( !victim->pcdata
	       && ( !is_set( victim->status, STAT_PET ) || victim->leader->species )
	       ? MSG_DAMAGE_MOBS : MSG_DAMAGE_PLAYERS );
  
  for( int i = 0; i < *ch->array; i++ ) {
    if( !( rch = character( ch->array->list[i] ) )
	|| rch == ch
	|| rch == victim
	|| !rch->link
	|| !is_set( rch->pcdata->message, flag )
	|| !rch->Can_See( ) )
      continue;
    if( victim->Seen( rch ) ) {
      snprintf( tmp, MAX_STRING_LENGTH, "%s's %s %s%s%s %s's %s%c",
		ch->Name( rch, 1 ), dt, damage_color( rch, ch, victim ),
		dam_name, normal( rch ), victim->Seen_Name( rch ),
		loc_name, punct );
    } else {
      snprintf( tmp, MAX_STRING_LENGTH, "%s's %s %s%s%s someone%c",
		ch->Name( rch ), dt, damage_color( rch, ch, victim ),
		dam_name, normal( rch ), punct );
    }
    *tmp = toupper( *tmp );
    strcat_cond( tmp, rch, victim );
  }
  
  victim->hit += damage;
}


static void msg_attack( char_data* victim, char_data* ch, const char* dt,
			const char* word, char punct )
{
  char           tmp  [ MAX_STRING_LENGTH ];
  char_data*     rch;

  if( ch->link ) {
    snprintf( tmp, MAX_STRING_LENGTH, "Your %s %s%s%s %s%c",
	      dt, by_self( ch ), word, normal( ch ),
	      victim->Name( ch ), punct );
    strcat_cond( tmp, ch, victim );
  }
  
  if( victim->link ) {
    snprintf( tmp, MAX_STRING_LENGTH, "%s's %s %s%s%s you%c",
	      ch->Name( victim ), dt, to_self( victim ),
	      word, normal( victim ), punct );
    *tmp = toupper( *tmp );
    fsend( victim, tmp );
  }
  
  const int flag = ( !victim->pcdata
		     && ( !is_set( victim->status, STAT_PET ) || victim->leader->species )
		     ? MSG_DAMAGE_MOBS : MSG_DAMAGE_PLAYERS );
  
  for( int i = 0; i < *ch->array; i++ ) {
    if( !( rch = character( ch->array->list[i] ) )
	|| rch == ch
	|| rch == victim
	|| !rch->link
	|| !is_set( rch->pcdata->message, flag )
	|| !rch->Can_See( ) )
      continue;
    snprintf( tmp, MAX_STRING_LENGTH, "%s's %s %s%s%s %s%c",
	      ch->Name( rch ), dt,
	      damage_color( rch, ch, victim ), word, normal( rch ),
	      victim->Name( rch ), punct );
    *tmp = toupper( *tmp );
    strcat_cond( tmp, rch, victim );
  }
}


static void msg_other( char_data *victim, char_data *ch, const char* dt,
		       const char* word, char punct )
{
  char                tmp  [ MAX_STRING_LENGTH ];
  char_data*          rch;

  if( victim->link ) {
    snprintf( tmp, MAX_STRING_LENGTH, "%s %s%s%s you%c", dt,
	      to_self( victim ), word, normal( victim ), punct );
    *tmp = toupper( *tmp );
    fsend( victim, tmp );
  }

  const int flag = ( !victim->pcdata
		     && ( !is_set( victim->status, STAT_PET ) || victim->leader->species )
		     ? MSG_DAMAGE_MOBS : MSG_DAMAGE_PLAYERS );
  
  for( int i = 0; i < *victim->array; i++ ) {
    if( !( rch = character( victim->array->list[i] ) )
	|| rch == victim
	|| !rch->link
	|| rch != ch && !is_set( rch->pcdata->message, flag )
	|| !rch->Can_See( ) )
      continue;
    snprintf( tmp, MAX_STRING_LENGTH, "%s %s%s%s %s%c", dt,
	      damage_color( rch, ch, victim ), word, normal( rch ),
	      victim->Name( rch ), punct );
    *tmp = toupper( *tmp );
    strcat_cond( tmp, rch, victim );
  }
}


void dam_message( char_data* victim, char_data* ch, int damage,
		  const char* dt, const char* word )
{
  char tmp [ MAX_STRING_LENGTH ];
  int i;

  if( victim->is_affected( AFF_SANCTUARY ) )
    damage = 0;
  else
    damage = min( damage, DAMAGE_LIMIT );

  victim->hit -= damage;

  strcpy( tmp, dt );
  if( ( i = strlen( tmp ) ) > 2 && tmp[i-2] == '\n' )
    tmp[i-2] = '\0';

  const char punct = ( damage <= 24 ? '.' : '!' );

  if( !ch || *dt == '*' ) {
    // Something damages victim.
    msg_other( victim, ch, *tmp == '*' ? tmp+1 : tmp, word, punct );
  } else {
    // Your attack damages victim.
    // ch's attack damages you.
    // ch's attack damages victim.
    msg_attack( victim, ch, tmp, word, punct );
  }

  victim->hit += damage;
}


/*
 *   CONDITION ROUTINES
 */


const char *condition_prep( char_data *ch )
{
  if( ch->hit <= 0 )
    return "is";

  const int percent = (100*ch->hit)/ch->max_hit;

  if( percent >= 100 )
    return "is in";
  if( percent >=  90 )
    return "is";
  if( percent >=  80 )
    return "has a";
  if( percent >=  70 )
    return "has";
  if( percent >=  60 )
    return "has";
  if( percent >=  50 )
    return "is";
  if( percent >=  40 )
    return "has";

  return "is";
}


const char *condition_word( char_data* ch )
{
  if( ch->hit <= -11
      || ch->species && ch->species->dies_at_zero( ) && ch->hit <= 0 )
    return "DEAD!";
  if( ch->hit <= -6 )
    return "mortally wounded";
  if( ch->hit <= -3 )
    return "incapacitated";
  if( ch->hit <= 0 )
    return "stunned";
  
  const int percent = (100*ch->hit)/ch->max_hit;

  if( percent >= 100 )
    return "perfect health";
  if( percent >=  90 )
    return "slightly scratched";
  if( percent >=  80 )
    return "few bruises";
  if( percent >=  70 )
    return "some cuts";
  if( percent >=  60 )
    return "several wounds";
  if( percent >=  50 )
    return "badly wounded";
  if( percent >=  40 )
    return "many nasty wounds";
  if( percent >=  30 )
    return "bleeding freely";
  if( percent >=  20 )
    return "covered in blood";
  if( percent >=  10 )
    return "leaking guts";

  return "mostly dead";
}


const char *condition_short( char_data* ch, char_data* victim )
{
  if( !victim )
    return "-";

  if( victim->array != ch->array
      || !victim->Seen( ch ) )
    return "??";

  return condition_short( victim );
}


const char *condition_short( char_data* ch )
{
  if( ch->hit <= -11
      || ch->species && ch->species->dies_at_zero( ) && ch->hit <= 0 )
    return "DEAD!";
  if( ch->hit <= -6 )
    return "dying";
  if( ch->hit <= -3 )
    return "incapacitated";
  if( ch->hit <= 0 )
    return "stunned";

  const int percent = (100*ch->hit)/ch->max_hit;

  if( percent >= 100 )
    return "perfect";
  if( percent >=  90 )
    return "scratched";
  if( percent >=  80 )
    return "bruised";
  if( percent >=  70 )
    return "cut";
  if( percent >=  60 )
    return "wounded";
  if( percent >=  50 )
    return "badly wounded";
  if( percent >=  40 )
    return "nastily wounded";
  if( percent >=  30 )
    return "bleeding freely";
  if( percent >=  20 )
    return "covered in blood";
  if( percent >=  10 )
    return "leaking guts";

  return "mostly dead";
}


/*
 *   GENERAL INFLICT ROUTINE
 */


bool inflict( char_data* victim, char_data* ch, int dam, const char *dt, bool passive )
{
  if( victim->position == POS_DEAD )
    return true;

  victim->Show( 1 );

  if( dt && *dt == '+' ) {
    ++dt;
  }

  if( victim->is_affected( AFF_SANCTUARY )
      || dam <= 0 ) {
    return false;
 } else if( dam > DAMAGE_LIMIT ) {
    const char *const type = ch ? ch->Name( ) : dt;
    bug( "Inflict: on %s by %s in room %d, more than %d points!",
	 victim, type, victim->in_room->vnum, DAMAGE_LIMIT );
    dam = DAMAGE_LIMIT;
  }

  record_damage( victim, ch, dam );

  victim->hit -= dam;

  update_pos( victim );
  update_max_move( victim );

  set_bit( victim->status, STAT_WIMPY );

  // Note: awakened character remains on pos_obj initially.
  if( victim->position == POS_SLEEPING ) {
    const bool slept = victim->is_affected( AFF_SLEEP );
    if( slept && !passive ) {
      // Prevent immediate standing up when whacked.
      remove_bit( victim->status, STAT_HOLD_POS );
      victim->position = POS_RESTING;
      strip_affect( victim, AFF_SLEEP );
      victim->position = POS_SLEEPING;
    }
    if( !passive
	|| !slept && number_range( 1, 2 ) == 1 ) {
      if( victim->position == POS_SLEEPING ) {
	send( victim, "You are suddenly awakened by the feeling of pain.\n\r" );
	fsend_seen( victim, "%s wakes up in obvious pain.", victim );
	if( deep_water( victim ) ) {
	  victim->position = POS_STANDING;
	} else {
	  victim->position = POS_RESTING;
	}
	remove_bit( victim->status, STAT_HOLD_POS );
	renter_combat( victim );
      }
    }
  }

  /*
  if( is_confused( victim ) && dam > number_range( 0, 100 ) ) {
    strip_affect( victim, AFF_CONFUSED );
    send( victim, "You are knocked to your senses.\n\r");
    act_notchar( "$n is knocked to $s senses.", victim );
//    remove_bit( victim->status, STAT_BERSERK );
//    remove_bit( victim->status, STAT_FOCUS );
    set_fighting( victim, 0 );
    victim->aggressive = 0;
    for( rch = victim->in_room->people; rch; rch = rch->next_in_room )
      if( rch->species && !is_set( rch->status, STAT_PET ) ) {
        if( rch->fighting == ch ) {
//	remove_bit( rch->status, STAT_BERSERK );
//	remove_bit( rch->status, STAT_FOCUS );
    set_fighting( rch, 0 );
        if( rch->aggressive == ch )
          rch->aggressive = 0;
	  }
    }
    */

  switch( victim->position ) {
    case POS_MORTAL:
      send( victim,
	    "You are mortally wounded, and will die soon, if not aided.\n\r" );
      fsend_seen( victim,
		  "%s is mortally wounded, and will die soon, if not aided.",
		  victim );
      break;
 
    case POS_INCAP:
      send( victim, 
	    "You are incapacitated and will slowly die, if not aided.\n\r" );
      fsend_seen( victim,
		  "%s is incapacitated and will slowly die, if not aided.",
		  victim );
      break;

    case POS_STUNNED:
      send( victim, "You are stunned, but will probably recover.\n\r" );
      fsend_seen( victim,
		  "%s is stunned, but will probably recover.",
		  victim );
      break;

    case POS_DEAD:
      death_message( victim );
      break;

    default:
      if( dam > victim->max_hit/4 )
        send( victim, "That really did HURT!\n\r" );
      if( victim->pcdata && victim->hit < victim->max_hit/4
	  && is_set( victim->pcdata->message, MSG_BLEEDING )
	  && !passive )
        send( victim, "You sure are BLEEDING!\n\r" );
      break;
  }
  
  if( victim->rider && victim->position < POS_FIGHTING ) {
    const char *drop = victim->rider->in_room->drop( );
    if( *drop ) {
      fsend( victim->rider,
	     "You are thrown %s as your mount is %s.",
	     drop,
	     victim->position == POS_DEAD ? "killed" : "incapacitated" );
      fsend_seen( victim->rider,
		  "%s is thrown %s as %s mount is %s.",
		  victim->rider, drop,
		  victim->His_Her( ),
		  victim->position == POS_DEAD ? "killed" : "incapacitated" );
    } else {
      fsend( victim->rider,
	     "You are thrown down as your mount is %s.",
	     victim->position == POS_DEAD ? "killed" : "incapacitated" );
      fsend_seen( victim->rider,
		  "%s is thrown down as %s mount is %s.",
		  victim->rider,
		  victim->His_Her( ),
		  victim->position == POS_DEAD ? "killed" : "incapacitated" );
    }
    dismount( victim->rider,
	      deep_water( victim ) ? POS_STANDING : POS_RESTING );
  }
  
  if( ch
      && ch->species
      && ch->position > POS_RESTING
      && !is_entangled( ch )
      && !ch->cast
      && !is_set( ch->status, STAT_PET ) ) 
    ch->species->damage += dam;
  
  if( victim->position == POS_DEAD ) {
    death( victim, ch, dt );
    return true;
  }

  return false;
} 


/*
 *   SPAM ROUTINES
 */


void spam_char( char_data *ch, const char *text )
{
  if( !ch->link
      || !is_set( ch->pcdata->message, MSG_MISSES ) )
    return;

  fsend( ch, text );
}


void spam_room( char_data *ch, const char *text )
{
  for( int i = 0; i < *ch->array; ++i ) {
    char_data *rch = character( ch->array->list[i] );
    if( !rch
	|| !rch->link
	|| rch == ch
	|| !rch->Can_See()
	|| !is_set( rch->pcdata->message, MSG_MISSES ) )
      continue;
    fsend( rch, text );
  }
}

/*
void spam_room( const char* text, char_data* ch, char_data* victim, thing_data *thing )
{
  char         tmp  [ MAX_STRING_LENGTH ];
  char_data*   rch;

  for( int i = 0; i < *ch->array; i++ ) {
    if( !( rch = character( ch->array->list[i] ) )
	|| !rch->link
	|| rch == ch
	|| rch == victim
	|| rch == thing
	|| !rch->Can_See()
	|| !is_set( rch->pcdata->message, MSG_MISSES ) )
      continue;
    if( thing ) {
      snprintf( tmp, MAX_STRING_LENGTH, text,
		ch->Name( rch ),
		victim ? victim->Name( rch ) : "[BUG]",
		thing->Name( rch ) );
    } else {
      snprintf( tmp, MAX_STRING_LENGTH, text,
		ch->Name( rch ),
		victim ? victim->Name( rch ) : "[BUG]" );
    }
    *tmp = toupper( *tmp );
    fsend( rch, tmp );
  }
}
*/
 
/*
 *   PHYSICAL DAMAGE ROUTINES
 */


const index_data physical_index [] = 
{
  { "has no effect on",     "have no effect on",     0 },
  { "scratches",            "scratch",               1 },
  { "grazes",               "graze",                 2 },
  { "hits",                 "hit",                   4 },
  { "injures",              "injure",                6 },
  { "wounds",               "wound",                10 },
  { "mauls",                "maul",                 15 },
  { "decimates",            "decimate",             21 },
  { "devastates",           "devastate",            28 },
  { "maims",                "maim",                 35 },
  { "SAVAGES",              "SAVAGE",               45 },
  { "CRIPPLES",             "CRIPPLE",              55 },
  { "MUTILATES",            "MUTILATE",             70 },
  { "DISEMBOWELS",          "DISEMBOWEL",           85 },
  { "* DISMEMBERS *",       "* DISMEMBER *",       100 },
  { "* EVISCERATES *",      "* EVISCERATE *",      120 },
  { "* MASSACRES *",        "* MASSACRE *",        145 },
  { "* PULVERIZES *",       "* PULVERIZE *",       170 },
  { "** DEMOLISHES **",     "** DEMOLISH **",      200 },
  { "** EXTIRPATES **",     "** EXTIRPATE **",     240 },
  { "*** OBLITERATES ***",  "*** OBLITERATE ***",  280 },
  { "*** ERADICATES ***",   "*** ERADICATE ***",   330 },
  { "*** ANNIHILATES ***",  "*** ANNIHILATE ***",   -1 }
};


bool damage_physical( char_data* victim, char_data* ch, int damage,
		      const char* string, bool plural,
		      const char *die )
{
  if( victim->is_affected( AFF_SANCTUARY ) )
    damage = 0;
  else
    add_percent_average( damage, -victim->Save_Physical( ch ) );

  dam_message( victim, ch, damage, string,
	       lookup( physical_index, damage, plural ) );
  
  return inflict( victim, ch, damage, die );
}


bool damage_magic( char_data* victim, char_data* ch, int damage,
		   const char* string, bool plural,
		   const char *die )
{
  if( victim->is_affected( AFF_SANCTUARY ) )
    damage = 0;
  else
    add_percent_average( damage, -victim->Save_Magic( ) );

  dam_message( victim, ch, damage, string,
	       lookup( physical_index, damage, plural ) );
  
  return inflict( victim, ch, damage, die );
}


bool damage_mind( char_data* victim, char_data* ch, int damage,
		  const char* string, bool plural,
		  const char *die )
{
  if( victim->is_affected( AFF_SANCTUARY ) )
    damage = 0;
  else
    add_percent_average( damage, -victim->Save_Mind( ) );
  
  dam_message( victim, ch, damage, string,
	       lookup( physical_index, damage, plural ) );
  
  return inflict( victim, ch, damage, die );
}
