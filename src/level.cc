#include <sys/types.h>
#include <stdio.h>
#include <syslog.h>
#include "define.h"
#include "struct.h"


/*
const char* level_name( int i )
{
  char* tmp = static_string( );

  if( i < LEVEL_AVATAR ) 
    snprintf( tmp, THREE_LINES, "level %d", i );
  else 
    snprintf( tmp, THREE_LINES, "a%s %s",
	      isvowel( *imm_title[ i-LEVEL_AVATAR ] ) ? "n" : "",
	      imm_title[ i-LEVEL_AVATAR ] );

  return tmp;
}
*/


void do_level( char_data* ch, const char *argument )
{
  if( is_confused_pet( ch ) )
    return;

  if( *argument ) {
    int i;
    if( !number_arg( argument, i ) ) 
      send( ch, "Unknown syntax - see help level.\n\r" );
    else if( i > LEVEL_HERO ) 
      send( ch, "Gaining levels beyond %d via experience points is impossible.\n\r",
	    LEVEL_HERO );
    else if( i < 2 ) 
      send( ch, "Levels below 2 do not require experience.\n\r" );
    else
      send( ch, "Gaining level %d costs %d experience points.\n\r",
	    i, exp_for_level( i-1 ) );
    return;
  }    
  
  if( is_mob( ch ) ) 
    return;

  const int level = ch->Level( );

  send( ch, "You are level %d.\n\r", level );

  if( ch->Level() <= LEVEL_HERO ) { 
    send( ch, "You have acquired %d experience point%s so far this level.\n\r",
	  ch->exp,
	  ch->exp == 1 ? "" : "s" );
  }

  if( ch->Level() < LEVEL_HERO ) {
    bool regain = ( ch->pcdata->max_level != -1 );
    int x = exp_for_level( ch )-ch->exp;

    if( x <= 0 ) {
      send( ch, "You have enough experience points to %sgain level %d.\n\r",
	    regain ? "re" : "",
	    level+1 );
      if( !is_set( ch->pcdata->pfile->flags, PLR_APPROVED )
	  && ch->Level() >= 5 ) {
	char buf [ TWO_LINES ];
	convert_to_ansi( ch, TWO_LINES, "Perhaps you should read @ehelp approvals@n?\n\r", buf );
	send( ch, buf );
      }
    } else {
      send( ch, "You need %d experience point%s to %sgain level %d.\n\r",
	    x,
	    x == 1 ? "" : "s",
	    regain ? "re" : "",
	    level+1 );
    }
  }
}


void advance_level( char_data* ch, bool message )
{
  if( ch->species )
    return;

  char        buf  [ MAX_STRING_LENGTH ];
  int        clss  = ch->pcdata->clss;
  int      add_hp;
  int    add_mana;
  int    add_move;
  int       pracs  = 0;

  ++ch->shdata->level;

  const bool regain = ( ch->pcdata->max_level != -1 );

  if( message ) {
    send( ch, "\n\r" );
    send_color( ch, COLOR_MILD, "-=-= CONGRATULATIONS!!! =-=-" );
    send( ch, "\n\r\n\r" );
    send( ch, "You have %sgained level %d.\n\r",
	  regain ? "re" : "",  ch->Level() );
    if( ch->Level() < LEVEL_HERO ) {
      send( ch, "You need %d experience points for the next level.\n\r",
	    exp_for_level( ch )-ch->exp );
    } else {
      send( ch, "\n\r" );
      send_color( ch, COLOR_STRONG, "You have attained the maximum level available to mortals." );
      send( ch, "\n\r" );
      send_color( ch, COLOR_STRONG, "Please consider remorting... see help remort." );
      send( ch, "\n\r\n\r" );
    }
    
    snprintf( buf, MAX_STRING_LENGTH, "%s has %sgained level %d.", ch->descr->name,
	      regain ? "re" : "", ch->Level() );
    info( 0, empty_string, 0, buf, IFLAG_LEVELS, regain ? 2 : 1, ch );
  }

  /*
  if( ch->Level() == LEVEL_HERO ) {
    ch->exp = 0;
  }
  */

  if( ch->pcdata->max_level == ch->Level() ) {
    ch->pcdata->max_level  = -1;
    ch->base_hit           = ch->pcdata->level_hit;
    ch->base_mana          = ch->pcdata->level_mana;
    ch->base_move          = ch->pcdata->level_move;
  } else {
    add_hp = number_range( clss_table[ clss ].hit_min,
			   clss_table[ clss ].hit_max );
    add_mana = number_range( clss_table[ clss ].mana_min,
			     clss_table[ clss ].mana_max );
    add_move = number_range( clss_table[ clss ].move_min,
			     clss_table[ clss ].move_max );

    ch->base_hit += add_hp;
    ch->base_mana += add_mana;
    ch->base_move += add_move;

    if( ch->pcdata->max_level == -1 ) {      
      if( ( pracs = 12-ch->Level() ) > 0 ) {
        if( message ) {
	  send( ch, "\n\r" );
          send_color( ch, COLOR_SKILL, ">> You gain %d practice point%s. <<",
		      pracs, pracs == 1 ? "" : "s" );
	  send( ch, "\n\r\n\r" );
	}
        ch->pcdata->practice += pracs;
      }
      remove_bit( ch->pcdata->pfile->flags, PLR_FAMILIAR );
    }
  }

  if( message ) {
    player_log( ch, "%sained level %d.",
		regain ? "Reg" : "G", ch->Level() );
  }
  
  ch->pcdata->pfile->level = ch->Level();
  calc_resist( (player_data*)ch );
  update_maxes( ch );
}


void lose_level( char_data* ch, bool message )
{
  char       tmp  [ ONE_LINE ];
  int     add_hp;
  int   add_mana;
  int   add_move;

  if( !ch->pcdata || ch->Level() < 2 )
    return;

  if( ch->pcdata->max_level == -1 ) {
    ch->pcdata->max_level  = ch->Level();
    ch->pcdata->level_hit  = ch->base_hit;
    ch->pcdata->level_mana = ch->base_mana;
    ch->pcdata->level_move = ch->base_move;
  }

  --ch->shdata->level;
  ch->pcdata->pfile->level = ch->Level();

  if( message && ch->Level() < LEVEL_BUILDER ) {
    snprintf( tmp, ONE_LINE, "%s has lost a level.", ch->descr->name );
    info( 0, empty_string, 0, tmp, IFLAG_LEVELS, 2, ch );
  }

  add_hp   = ( ch->base_hit-20 )/( ch->Level() );
  add_mana = ( ch->base_mana-50 )/( ch->Level() );
  add_move = ( ch->base_move-100 )/( ch->Level() );

  ch->base_hit   -= add_hp;
  ch->base_mana  -= add_mana;
  ch->base_move  -= add_move;

  calc_resist( (player_data*)ch );
  update_maxes( ch );

  if( message ) {
    send( ch, "You have lost a level!\n\r" );
    player_log( ch, "Lost level %d.", ch->Level( )+1 );
  }
}


/*
 *   EXPECTED FUNCTIONS (PLAYER) 
 */


double player_data :: Mean_Hp( ) const
{
  return 20+(Level()-1)*(clss_table[ pcdata->clss ].hit_min
    +clss_table[ pcdata->clss ].hit_max)/2;
}


double player_data :: Mean_Mana( ) const
{
  return 50+(Level()-1)*(clss_table[ pcdata->clss ].mana_min
    +clss_table[ pcdata->clss ].mana_max)/2;
}


double player_data :: Mean_Move( ) const
{
  return 100+(Level()-1)*(clss_table[ pcdata->clss ].move_min
    +clss_table[ pcdata->clss ].move_max)/2;
}


/*
 *   EXPECTED FUNCTIONS (MOB) 
 */


double Mob_Data :: Mean_Hp( ) const
{
  return 0.0;
}


double Mob_Data :: Mean_Mana( ) const
{
  return 0.0;
}


double Mob_Data :: Mean_Move( ) const
{
  return 0.0;
}
