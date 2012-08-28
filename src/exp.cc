#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <math.h>
#include "define.h"
#include "struct.h"


int death_exp( char_data* victim, char_data* )
{
  return exp_for_level( victim )/5;
}


int exp_for_level( int level )
{
  int exp = level*(110*level*level+400*level-411);
  exp += (int)( pow( level*level*level*level, 1.11 ) ); 

  return exp;
}


/*
 *   EXP CALCULATION SUB-ROUTINES
 */


static long long base_exp( species_data* species )
{
  if( species->shdata->deaths == 0
      || species->rounds == 0 )
    return 0;
  
  const dice_data dice = species->hitdice;
  
  double xp = double( species->damage )
              * ( dice.twice_average( )
		  + 2.0*species->damage_taken/species->shdata->deaths )
              / (4.0*species->rounds);
  xp = pow( xp, double( 1.07 ) ) + 5.0;

  return (long long) xp;
}


/*
static long long death_exp( species_data *species, long long base )
{
  double x = (double)species->shdata->kills / (2+species->shdata->deaths);

  if( x > 1.5 )
    return (1.5*base);

  return (long long)(x*base);
}
*/


static long long special_exp( species_data *species, long long base )
{
  if( species->rounds == 0 )
    return 0;

  return base * species->special / species->rounds / 2;
}


static long long level_exp( species_data *species, long long base )
{
  return base * species->shdata->level / 100;
}


static long long modify_exp( species_data *species, long long base )
{
  if( is_set( species->act_flags, ACT_AGGR_ALL ) )
    return base/10;

  return 0;
}


static long long modify_exp( char_data *mob, long long base )
{
  if( is_set( mob->status, STAT_AGGR_ALL ) ) 
    return base/10;

  return 0;
}


/*
 *   EXP DISPLAY FUNCTION
 */


void do_exp( char_data* ch, const char *argument )
{
  wizard_data *imm = wizard( ch );

  species_data *species;

  if( *argument ) {
    if( !( species = get_species( atoi( argument ) ) ) ) {
      send( ch, "No mob has that vnum.\n\r" );
      return;
    }
  } else if( !imm->mob_edit ) {
    send( ch, "For what mob do you want to see exp calculation?\n\r" );
    return;
  } else {
    species = imm->mob_edit;
  }

  const long long base  = base_exp( species );
  //  const long long death = death_exp( species, base );
  const long long spec  = special_exp( species, base );
  const long long modif = modify_exp( species, base );
  //  const long long total = base+death+spec+modif;
  long long total = base+spec+modif;

  const long long level = level_exp( species, total );
  total += level;

  page_title( ch, "Exp for %s", species );

  if( species->shdata->deaths == 0 )
    page( ch, "       Ave Exp: ??\n\r" );
  else
    page( ch, "       Ave Exp: %d\n\r",
	  species->exp/species->shdata->deaths );

  dice_data dice = species->hitdice;

  page( ch, "        Damage: %-10d Rounds: %-11d Dam/Rnd: %.2f\n\r ",
	species->damage, species->rounds,
	(double)species->damage / species->rounds );
  
  page( ch, "      Avg. Hp: %-7d Dmg_Taken: %-9d Avg_Taken: ",
	dice.average( ), species->damage_taken );
  
  if( species->shdata->deaths == 0 )
    page( ch, "??\n\r" );
  else
    page( ch, "%d\n\r",
	  species->damage_taken/species->shdata->deaths );
  
  page( ch,"         Kills: %-10d Deaths: %-13d Level: %d\n\r",
	species->shdata->kills,
	species->shdata->deaths,
	species->shdata->level );
  page( ch, "       Special: %-8d Spec/Rnd: %.2f\n\r\n\r",
	species->special, (double)species->special / species->rounds );
  
  page( ch, "   Base Exp: %8lld\n\r", base  );
  //  page( ch, "  Death Exp: %8lld\n\r", death );
  page( ch, "    Special: %8lld\n\r", spec  );
  page( ch, "  Modifiers: %8lld\n\r", modif );
  page( ch, "      Level: %8lld\n\r", level );
  page( ch, "             --------\n\r" );
  page( ch, "      Total: %8lld\n\r", total );
}


/*
 *   EXP COMPUTATION FUNCTION
 */

 
long long xp_compute( char_data* victim )
{
  long long xp;

  if( species_data *species = victim->species ) {
    xp  = base_exp( species );
    //    xp += death_exp( species, xp );
    xp += special_exp( species, xp ) + modify_exp( victim, xp );
    xp += level_exp( species, xp );
  } else {
    obj_data *wield, *secondary, *shield;
    get_wield( victim, wield, secondary, shield );
    long long damage;
    if( wield ) {
      damage = dice_data( wield->value[1] ).twice_average( );
      //      damage = wield->value[1]*(wield->value[2]+1)/2;
    } else {
      damage = 5;
    }
    damage += (long long) ( 2.0*victim->Damroll( wield ) );
    xp = damage*victim->max_hit/4;
    const long long ceil = victim->exp / 2;
    if( xp > ceil )
      xp = ceil;
    //    xp = min( xp, victim->exp/2 );
  }
  
  if( xp < 0 )
    xp = 0;
  //  xp = max( 0, xp );
  
  return xp;
}


/*
 *   GROUP EXP FUNCTION
 */


static void gain_exp( char_data* ch, char_data* victim, int gain, const char *msg )
{
  if( ch->species )
    return;

  /*
  int level = exp_for_level( ch );

  if( gain > 0 ) {
    gain = (int)( gain/sqr(1.0+sqrt(1.0*gain/level)) );
  }
  */

  add_exp( ch, gain, msg );

  /* MODIFY REPUTATION */

  if( !victim )
    return;

  player_data *pc = (player_data*) ch;

  pc->reputation.alignment[ victim->shdata->alignment ] += gain;

  if( pc->reputation.alignment[ victim->shdata->alignment ] < 0 ) {
    // Overflow, scale by 1/2.
    for( int i = 0; i < table_max[ TABLE_ALIGNMENT ]; ++i ) {
      unsigned int x = pc->reputation.alignment[ i ];
      x /= 2;
      pc->reputation.alignment[ i ] = x;
    }
  }

  if( victim->species )
    ++pc->reputation.nation[ victim->species->nation ];
}


void add_exp( char_data* ch, int gain, const char* msg )
{
  if( ch->species )
    return;

  if( msg )
    send( ch, msg, abs( gain ), abs( gain ) == 1 ? "" : "s" );

  const int old_level = ch->Level();

  if( old_level > LEVEL_HERO ) {
    ch->exp = 0;
    update_score( ch );
    return;
  }

  const int level = exp_for_level( ch );
  
  ch->exp += gain;
  
  while( ch->Level() > 1
	 //	 && ch->Level() <= LEVEL_HERO
	 && ch->exp < 0 ) {
    lose_level( ch, true );
    ch->exp += exp_for_level( ch ); 
  }
  
  ch->exp = max( 0, ch->exp );
  
  if( ch->exp >= level
      //      && ch->Level() <= LEVEL_HERO
      && ch->pcdata->quest_pts >= 0
      && gain > 0 ) {
    if( ch->Level() == LEVEL_HERO ) {
      ch->exp = level;
    } else if( !is_set( ch->pcdata->pfile->flags, PLR_APPROVED )
	       && ch->Level() >= 5 ) {
      send( ch,
	    "To gain levels past 5th you must have an approved appearance.\n\r" );
    } else {
      ch->exp -= level;
      advance_level( ch, true );
    }
  }

  if( ch->Level( ) != old_level )
    init_affects( ch, old_level );

  update_score( ch );
}


void disburse_exp( char_data* victim )
{
  if( victim->pcdata )
    return;

  double xpc = (double) xp_compute( victim );
  
  if( victim->species )
    victim->species->exp += (int)( xpc );

  int tot_damage = 0;
  int members = 0;
  player_array list;
  
  for( int i = 0; i < *victim->array; ++i ) {
    player_data *ch = player( victim->array->list[i] );
    if( ch
	&& ch != victim ) {
      int damage = damage_done( ch, victim );
      if( damage >= 0 || ch->fighting ) {
	//      remove_bit( ch->status, STAT_GAINED_EXP );      
	tot_damage += max( 0, damage );
	members += cube( ch->Level() + 5 );
	list += ch;
      }
    }
  }

  xpc /= max( tot_damage, victim->max_hit );

  double xp;
  for( int i = 0; i < list; ++i ) {
    if( ( xp = double( tot_damage )
	  * cube( list[i]->Level()+5 )
	  * xpc / members ) > 0.0 ) {
      int x = (int) xp;
      if( x > 0 ) {
	const int level = exp_for_level( list[i] );
	x = (int)( x/sqr(1.0+sqrt(1.0*x/level)) );
      }
      if( x > 0 ) {
	gain_exp( list[i], victim, x,
		  "You receive %d experience point%s.\n\r" );
      } else {
	gain_exp( list[i], victim, 0,
		  "You don't receive any experience points.\n\r" );
      }
    }
    list[i]->shdata->fame = min( list[i]->shdata->fame
				 +victim->Level(), 1000 );
  }

  /*
    This was being abused to powerlever lowbies.
    Lowbies following ungrouped can gain much more xp
    because of the cube calculation.

  // Each attacker can be a separate group.
  for( int i = 0; i < *victim->array; ++i ) 
    if( ( ch = player( victim->array->list[i] ) )
	&& ch != victim
	&& !is_set( ch->status, STAT_GAINED_EXP ) ) {
      group_gain( victim, ch, xpc );
    }
  */
}


/*
void group_gain( char_data* victim, char_data* ch, double xpc )
{
  player_data*      gch;
  player_array     list;
  int           members  = 0;
  int        tot_damage  = 0;
  int            damage;
  double             xp;

  for( int i = 0; i < *ch->array; ++i ) {
    if( !( gch = player( ch->array->list[i] ) )
	|| !is_same_group( gch, ch ) )
      continue;
    damage = damage_done( gch, victim );
    if( damage >= 0 || gch->fighting ) {
      tot_damage  += damage;
      members     += cube( gch->Level()+5 );
      list        += gch;
    }
    set_bit( gch->status, STAT_GAINED_EXP );
  }
  
  for( int i = 0; i < list; ++i ) {
    if( ( xp = double( tot_damage )
               * cube( list[i]->Level()+5 )
	       * xpc / members ) > 0 ) {   
      gain_exp( list[i], victim, (int)( xp ),
		"You receive %d experience point%s.\n\r" );
    }
    list[i]->shdata->fame = min( list[i]->shdata->fame
				 +victim->Level(), 1000 );
  }
}

*/

void zero_exp( species_data* species )
{
  species->rounds         = 1;
  species->damage         = 0;
  species->damage_taken   = 0;
  species->shdata->deaths = 0;
  species->shdata->kills  = 0;
  species->exp            = 0;
  species->special        = 0;
}
