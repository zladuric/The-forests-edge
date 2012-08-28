#include <ctype.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include "define.h"
#include "struct.h"


const char *skill_cat_name [ MAX_SKILL_CAT ] = {
  "physical",
  "language",
  "spell",
  "trade",
  "weapon"
};


const int skill_table_number[ MAX_SKILL_CAT ] = {
  TABLE_SKILL_PHYSICAL,
  TABLE_SKILL_LANGUAGE,
  TABLE_SKILL_SPELL,
  TABLE_SKILL_TRADE, 
  TABLE_SKILL_WEAPON
};

 
/*
 *   CURRENTLY UNUSED
 */


/*
char* abil_name( int i )
{
  if( i <  0 ) return " (unknown) ";
  if( i <  1 ) return "(appalling)";
  if( i <  5 ) return "(dreadful) ";
  if( i < 10 ) return "  (awful)  ";
  if( i < 20 ) return "   (bad)   ";
  if( i < 30 ) return "  (poor)   ";
  if( i < 50 ) return "  (fair)   ";
  if( i < 60 ) return " (capable) ";
  if( i < 70 ) return "  (good)   ";
  if( i < 80 ) return "(very good)";
  if( i < 90 ) return " (superb)  ";
               return " (wicked)  ";
}
*/


/*
 *   ABILITY ROUTINE
 */


/* Outputs the current players skills */

static void sort_skills( int clss, Array<int>& skills, bool level_sort )
{
  for( int i = 0; i < skills.size-1; ++i ) {
    bool done = true;

    for( int j = 0; j < skills.size-1-i; ++j ) {
      bool test = false;
      Skill_Type *skill1 = skill_entry( skills[j] );
      Skill_Type *skill2 = skill_entry( skills[j+1] );
      if( level_sort ) {
	test = ( ( skill1->level[clss] > skill2->level[clss] )
		 || ( skill1->level[clss] == skill2->level[clss]
		      && strcasecmp( skill1->name, skill2->name ) > 0 ) );
      } else {
	test = ( strcasecmp( skill1->name, skill2->name ) > 0 );
      }

      if( test ) {
        swap( skills[j], skills[j+1] );
        done = false;
      }
    }

    if( done )
      break;
  }
}


void do_abilities( char_data* ch, const char *argument )
{
  if( not_player( ch ) )
    return;

  int flags;

  if( !get_flags( ch, argument, &flags, "l", "abilities" ) )
    return;

  if( !*argument ) {
    send( ch, "Syntax: abilities [-l] <category> (optional class)\n\r" );
    send( ch, "Where <category> is Physical, Weapon, Spell, Language, Trade, or All.\n\r" );
    send( ch, "See help abilities for more detail.\n\r" );
    return;
  }

  char arg [ MAX_STRING_LENGTH ];

  argument = one_argument( argument, arg );

  char        buf  [ FOUR_LINES ];
  int        i, j;
  int    ps1, ps2;
  int      length;

  /* What class is the player? */
  int clss = ch->pcdata->clss;

  Array<int> skills;

  /* If not looking for the skills available
   * for a specific class then print current player's skills
   */
  if( !*argument ) {
    length = strlen( arg );
    bool all = !strncasecmp( "all", arg, length );
    for( j = 0; j < MAX_SKILL_CAT; ++j ) {
      if( all || !strncasecmp( skill_cat_name[j], arg, length ) ) {
	const int m = table_max[ skill_table_number[ j ] ];
	for( i = 0; i < m; ++i ) {
	  const int id = skill_ident( j, i );
	  const Skill_Type *skill = skill_entry( j, i );
          if( ( prac_cost( ch, id ) < 0
		|| skill->level[clss] > ch->Level() )
	      && ch->Level() < LEVEL_APPRENTICE )
            continue;
	  skills.append( id );
	}
	if( !all ) {
	  if( skills.is_empty( ) ) {
	    send( ch, "You have no %s skills.\n\r", skill_cat_name[j] );
	    return;
	  }
	  break;
	}
      } else if( j+1 == MAX_SKILL_CAT ) {
	send( ch, "Unknown skill category.\n\r" );
	return;
      }
    }

    if( skills.is_empty( ) ) {
      send( ch, "You have no skills.\n\r" );
      return;
    }
      
    sort_skills( clss, skills, false );
    
    page_underlined( ch,
		     "Skill                 Level  Cost  Pracs  Prerequistes\n\r" );
    
    for( int k = 0; k < skills; ++k ) {
      const int i = skills[k];
      const int skill = ch->get_skill(i);
      if( skill == UNLEARNT ) 
	sprintf( arg, " unk" );
      else
	sprintf( arg, "%d ", skill );
      Skill_Type *entry = skill_entry( i );
      int b = sprintf( buf,"%-22s%4s",
		       entry->name, arg );
      ps1 = entry->pre_skill[0];
      ps2 = entry->pre_skill[1];
      int cost = prac_cost( ch, i );
      if( skill != 10 && cost >= 0 )
	b += sprintf( buf+b, "%7d%5d%4s",
		      cost, entry->prac_cost[clss], "" );
      else
	b += sprintf( buf+b, "%16s", "" );
      if( ps1 != SKILL_NONE )
	b += sprintf( buf+b, "%s [%d]", skill_entry(ps1)->name, 
		      entry->pre_level[0] );
      if( ps2 != SKILL_NONE ) {
	if( b + 8 + strlen( skill_entry(ps2)->name ) >= ch->pcdata->columns ) {
	  sprintf( buf+b, "\n\r" );
	  page( ch, buf );
	  b = sprintf( buf, "%41s", "" );
	}
	b += sprintf( buf+b, " & %s [%d]", skill_entry(ps2)->name, 
		      entry->pre_level[1] );
      }
      sprintf( buf+b, "\n\r" );
      page( ch, buf );
    }
    
    return;
  }

  /* Something was after abil <type> in the command line.
   * Looking for skills for a specific class...
   * i.e. abil <type> <class>
   */

  length = strlen( argument ); 

  /* Select the correct table for that class. */

  for( clss = 0; clss < MAX_CLSS; ++clss ) 
    if( !strncasecmp( argument, clss_table[clss].name, length ) ) 
      break;
  
  if( clss == MAX_CLSS ) {
    send( ch, "Unknown class.\n\r" );
    return;
  }

  /* Get the correct skill tables for that type.  
   * spell, physical, weapons, language.
   */

  length = strlen( arg );
  const bool all = !strncasecmp( "all", arg, length );

  for( j = 0; j < MAX_SKILL_CAT; j++ ) {
    if( all || !strncasecmp ( skill_cat_name[j], arg, length ) ) {
      int m = table_max[ skill_table_number[ j ] ];
      for( i = 0; i < m; ++i ) {
        if( skill_entry( j, i )->prac_cost[clss] < 0 )
          continue;
	skills.append( skill_ident( j, i ) );
      }

      if( !all ) {
	if( skills.is_empty( ) ) {
	  send( ch, "The %s class has no %s skills.\n\r",
		clss_table[clss].name, skill_cat_name[j] ); 
	  return;
	}
	break;
      }
    } else if( j+1 == MAX_SKILL_CAT ) {
      send( ch, "Unknown skill category.\n\r" );
      return;
    }
  }
  
  if( skills.is_empty( ) ) {
    send( ch, "The %s class has no skills.\n\r",
	  clss_table[clss].name ); 
    return;
  }
  
  sort_skills( clss, skills, is_set( flags, 0 ) );
  
  page_underlined( ch,
		   "Skill                   Pracs  Level  Prerequistes\n\r" );
  
  for( int k = 0; k < skills; ++k ) {
    int i = skills[k];
    Skill_Type *entry = skill_entry( i );
    
    int b;
    if( entry->level[clss] > LEVEL_HERO ) {
      b = snprintf( buf, FOUR_LINES, "%-24s<Unfinished>  ",
		    entry->name );
    } else {
      int cost = entry->prac_cost[clss];
      b = snprintf( buf, FOUR_LINES, "%-22s%5d%7d    ",
		    entry->name, cost,
		    entry->level[clss] );
    }
    
    ps1 = entry->pre_skill[0];
    ps2 = entry->pre_skill[1];
    
    if( ps1 != SKILL_NONE )
      b += sprintf( buf+b, "%s [%d]", skill_entry( ps1 )->name, 
		    entry->pre_level[0] );
    if( ps2 != SKILL_NONE ) {
      if( b + 8 + strlen( skill_entry( ps2 )->name ) >= ch->pcdata->columns ) {
	sprintf( buf+b, "\n\r" );
	page( ch, buf );
	b = sprintf( buf, "%37s", "" );
      }
      b += sprintf( buf+b, " & %s [%d]", skill_entry( ps2 )->name, 
		    entry->pre_level[1] );
    }

    if( !entry->religions.is_empty( ) ) {
      if( ps1 != SKILL_NONE || ps2 != SKILL_NONE ) {
	sprintf( buf+b, "\n\r" );
	page( ch, buf );
	b = sprintf( buf, "%38s", "" );
      }
      b += sprintf( buf+b, "(religion-specific)" );
    }

    sprintf( buf+b, "\n\r" );
    page( ch, buf );
  }
}


/*
 *   PRACTICE POINTS
 */


int total_pracs( char_data* ch )
{
  const int            clss  = ch->pcdata->clss;
  int           total  = 0;
  int            cost  = 0;
  
  for( int j = 0; j < MAX_SKILL_CAT; ++j ) {
    const int m = table_max[ skill_table_number[ j ] ];
    for( int i = 0; i < m; ++i ) {
      skill_type *entry = skill_entry( j, i );
      const int skill = ch->shdata->skills[j][i];
      if( skill != UNLEARNT
	  && ( cost = entry->prac_cost[clss] ) >= 0
	  && entry->religion( ch->pcdata->religion ) ) 
	total += cost*skill;
    }
  }

  total -= 10*skill_entry( LANG_PRIMAL )->prac_cost[clss];
  if( ch->shdata->race < MAX_PLYR_RACE )
    total -= 10*skill_entry( LANG_HUMANIC+ch->shdata->race )->prac_cost[clss];
  total += ch->pcdata->practice;
  
  return total;
}    


/* Does the calcs to see if you have improved at the skill you are using.
 * Uses total pracs to date, expected pracs to date, a prac_timer and
 * weights Intellegence and Wisdom equally.  Also takes into account
 * your level.  If the skill is already at 10 then award a practise point.
 */


// Some physical skills improve like spells, since they don't get automatically
// used ad nauseum.
static bool sparse( int i )
{
  return( i == SKILL_INSPECT
	  || i == SKILL_UNTRAP
	  || i == SKILL_FORAGE
	  || i == SKILL_CAMPING
	  || i == SKILL_BANDAGE
	  || i == SKILL_GARROTE
	  || i == SKILL_BUTCHER
	  || i == SKILL_STEAL
	  || i == SKILL_HEIST );
}


void char_data :: improve_skill( int i )
{
  if( species
      || pcdata->prac_timer > 0
      || Level( ) >= LEVEL_APPRENTICE
      || i == LANG_PRIMAL )
    return;

 /* i is a valid skill, the players prac_timer has counted out, and the
  * player has the skill at least at 1. 
  */
  
  const int l = get_skill( i );

  if( l == UNLEARNT )
    return;
  
  /* Weapon, language or a skill that is being considered
   * for an improve.  If the timer just now counted below 0  
   * or the timer IS 0 then return.  If the timer is 0 then
   * set it to a random number between   -20 and -2.
   */

  const unsigned cat = skill_table( i );
  if( cat != SKILL_CAT_SPELL && cat != SKILL_CAT_TRADE && !sparse( i ) ) {
    if( pcdata->prac_timer < 0 && ++pcdata->prac_timer != -1 ) 
      return;
    if( pcdata->prac_timer == 0 ) {
      pcdata->prac_timer = number_range( -20, -2 );
      return;
    }
  }

  /* nums for calculating if you get an improve.
   * norm --> find expected pracs.
   * total --> total number of pracs that a player has, including skill levels*pracs.
   */

  const int norm = expected_pracs( this );
  const int total = total_pracs( this );
  const int j = max( 0, 2*(total-norm) );

  /* reset the prac timer.*/   

  pcdata->prac_timer = 5;

  /* Using random numbers between 0 and 30+the above j,
   * comparisons to Intelligence and Wisdom are made.  Must be less
   * than both.  
   * Random between 0 and 200 must be greater than Level()...Dunno
   */ 

  const skill_type *entry = skill_entry( i );
  if( number_range( 0, 30+j ) < Intelligence( )
      && number_range( 0, 30+j ) < Wisdom( )
      && number_range( 0, 200 ) > Level() ) {
    /* if the skill is already at 10 give a prac point. */
    send( this, "\n\r" );
    if( l == 10 || number_range( 0, 2 ) != 0 ) {
      ++pcdata->practice;
      send_color( this, COLOR_SKILL,
		  "** You gain a practice point from %s. **",
		  entry->name );
    } else {
      /* otherwise improve. */
      ++shdata->skills[cat][skill_number(i)];
      send_color( this, COLOR_SKILL, "** You improve at %s. **",
		  entry->name );
    }
    send( this, "\n\r\n\r" );
  }
}


int prac_cost( char_data *ch, int skill )
{
  if( skill < 0
      || ch->species ) {
    return -1;
  }

  const skill_type *entry = skill_entry( skill );;
  const int pracs = entry->prac_cost[ch->pcdata->clss];

  if( pracs < 1 || !entry->religion( ch->pcdata->religion ) ) {
    return -1;
  }

  const int class_level = entry->level[ch->pcdata->clss];
  const int skill_level = ch->get_skill( skill );

  return max( 5, (pracs * pracs * class_level * (skill_level + 2))/3 );
}
