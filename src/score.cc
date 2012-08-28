#include <ctype.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include "define.h"
#include "struct.h"


char*   armor_name         ( int i );
char*   fame_name          ( int i );
char*   piety_name         ( int i );


/*
 *   STATUS WORD ROUTINES
 */


static const char *condition_name( char_data* ch )
{
  char *buf = static_string();
  
  if( ch->species ) {
    snprintf( buf, THREE_LINES, "%s%s%s%s", 
	      !is_set( ch->status, STAT_PET ) ? " bloodthirsty" : " tame",
	      ch->is_affected( AFF_HALLUCINATE ) ? " delirious" : "",
	      ch->is_affected( AFF_POISON ) ? " poisoned" : "",
	      ch->is_affected( AFF_PLAGUE ) ? " diseased" : "" );

  } else {    
    snprintf( buf, THREE_LINES, "%s%s%s%s%s%s", 
	      ( ch->is_drunk( )
		? " drunk"
		: ( ch->is_affected( AFF_HALLUCINATE ) 
		    ? ""
		    : " sober" ) ),
	      ch->is_affected( AFF_HALLUCINATE ) ? " delirious" : "",
	      ch->condition[COND_FULL] < 0 ? " hungry" : "",
	      ch->condition[COND_THIRST] < 0 ? " thirsty" : "",
	      ch->is_affected( AFF_POISON ) ? " poisoned" : "",
	      ch->is_affected( AFF_PLAGUE ) ? " diseased" : "" );
  }

  return buf;
}

index_data fame_index [] =
{
  { "unknown",    "",    25 },
  { "obscure",    "",    75 },
  { "marginal",   "",   150 },
  { "familiar",   "",   300 },
  { "known",      "",   500 },
  { "famous",     "",   750 },
  { "renowned",   "",   900 },
  { "legendary",  "",    -1 }
};


index_data piety_index [] =
{ 
  { "sacrilegous", "",  -900 },
  { "blasphemous", "",  -700 },
  { "ungodly",     "",  -350 },
  { "impious",     "",  -100 },
  { "indevout",    "",     0 },
  { "moral",       "",   100 },
  { "pious",       "",   350 },
  { "religious",   "",   700 },
  { "devout",      "",   900 },
  { "fanatical",   "",    -1 }

  // orthodox, saintly, godly - some of these imply goodness
};


/* 
 *   SHOW IDENTITY ROUTINE
 */


void do_identity( char_data* ch, const char *argument )
{
  char               tmp  [ MAX_STRING_LENGTH ];

  if( is_confused_pet( ch ) )
    return;

  char_data *victim  = ch;

  if( *argument && ch->Level() >= LEVEL_APPRENTICE ) {
    in_character = false;
    if( !( victim = one_player( ch, argument, "identity", 
				(thing_array*) &player_list ) ) )
      return;
    if( victim != ch
	&& get_trust( victim ) >= get_trust( ch ) ) {
      send( ch, "You can only see identity on players of lower trust than yourself.\n\r" );
      return;
    }
  }

  pc_data*        pcdata  = victim->pcdata;
  descr_data*      descr  = victim->descr; 
  player_data*        pc  = player( victim );
  wizard_data*       imm  = wizard( victim );

  page_title( ch, descr->name );
  
  if( !victim->species )
    page( ch, "           ID#: %-10d Serial#: %-13d Class: %s\n\r",
	  pcdata->pfile->ident, pcdata->pfile->serial, clss_table[ pcdata->clss ].name );
  
  page( ch, "          Race: %-14s Sex: %-9s Alignment: %s\n\r",
	race_table[ victim->shdata->race ].name,
	sex_name[victim->sex],
	alignment_table[ victim->shdata->alignment ].name );
  
  if( pc ) {
    page( ch, "      Religion: %-12s Piety: %-14s Fame: %s\n\r",
	  religion_table[ pcdata->religion ].name,
	  lookup( piety_index, pcdata->piety ),
	  lookup( fame_index, victim->shdata->fame ) );
    page( ch, "         Kills: %-10d  Deaths: %-8d Recog Char: %d\n\r",
	  victim->shdata->kills, victim->shdata->deaths,
	  pcdata->recognize ? pcdata->recognize->size : 0 );
    
    snprintf( tmp, MAX_STRING_LENGTH, "%d'%d\"",
	      pc->Height( )/12, pc->Height( )%12 );
    page( ch, "        Height: %-9s   Weight: %.2f lbs\n\n\r",
	  tmp, (double) victim->Empty_Weight( )/100.0 );
    
    snprintf( tmp, MAX_STRING_LENGTH, "-- Time Played: " );
    sprintf_minutes( tmp+16, pc->time_played( ) );
    sprintf( tmp+strlen( tmp ), " --" );
    page_centered( ch, tmp );
    page( ch, "\n\r" );
    
    page( ch, "    Appearance: %s\n\r", descr->singular ); 
    if( pcdata->tmp_short != empty_string ) 
      page( ch, "    Unapproved: %s\n\r", pcdata->tmp_short );
  } 
  
  page( ch, "      Keywords: %s\n\r", victim->descr->keywords );
  
  if( !victim->species && pcdata->tmp_keywords != empty_string ) 
    page( ch, "    Unapproved: %s\n\r", pcdata->tmp_keywords );
  
  if( has_permission( ch, PERM_GOTO )
      && has_permission( victim, PERM_GOTO ) ) {
    page( ch, "        Bamfin: %s\n\r",
	  imm->bamfin == empty_string ? "none" : imm->bamfin );
    page( ch, "       Bamfout: %s\n\r",
	  imm->bamfout == empty_string ? "none" : imm->bamfout );
  }
}


/*
 *   SCORE ROUTINE
 */


void show_resists( char_data *ch, char_data *victim, bool pager )
{
  char              tmp  [ MAX_INPUT_LENGTH ];
  const char* name [] = { "Magic", "Fire", "Cold", "Mind", "Electric", 
			  "Acid", "Poison" }; 
  
  const int resist [] = {
    victim->Save_Magic( ), victim->Save_Fire(  ),
    victim->Save_Cold( ),  victim->Save_Mind( ), 
    victim->Save_Shock( ), victim->Save_Acid( ), 
    victim->Save_Poison( ) };
  
  const char* color [] = { 
    magenta( ch ), red( ch ), blue( ch ), cyan( ch ), yellow( ch ), green( ch ), "" };
  
  for( int i = 0; i < 7; i++ ) {
    sprintf( tmp+40*i, "%10s", "" );
    sprintf( tmp+40*i+9-strlen( name[i] ), "%s%s: %+d%%%s%s",
	     color[i], name[i], resist[i],
	     normal( ch ), abs( resist[i] ) > 99 ? "" 
	     : ( abs( resist[i] ) > 9 ? " " : "  " ) ); 
  }
  
  if( pager ) {
    page( ch, "\n\r   %s%s%s%s\n\r", tmp, tmp+40, tmp+80, tmp+120 );
    page( ch, "   %s%s%s\n\r", tmp+160, tmp+200, tmp+240 );
  } else {
    send( ch, "\n\r   %s%s%s%s\n\r", tmp, tmp+40, tmp+80, tmp+120 );
    send( ch, "   %s%s%s\n\r", tmp+160, tmp+200, tmp+240 );
  }
}


void do_score( char_data* ch, const char *argument )
{
  if( is_confused_pet( ch ) )
    return;

  char_data *victim  = ch;

  if( *argument && ch->Level() >= LEVEL_APPRENTICE ) {
    in_character = false;
    if( !( victim = one_player( ch, argument, "score", 
				(thing_array*) &player_list ) ) )
      return;
    if( victim != ch
	&& get_trust( victim ) >= get_trust( ch ) ) {
      send( ch, "You can only see score on players of lower trust than yourself.\n\r" );
      return;
    }
  }

  char buf  [ MAX_STRING_LENGTH ];

  pc_data *pcdata = victim->pcdata;
  player_data *pc = player( victim );
  
  if( victim->species ) {
    page_title( ch, victim->species->Name() );
  } else {
    page_title( ch, victim->descr->name );
  }

  ssize_t len = snprintf( buf, MAX_STRING_LENGTH, "         Level: %-11d", victim->Level() );

  if( pc ) {
    if( victim->Level() >= LEVEL_BUILDER ) {
      snprintf( buf+len, MAX_STRING_LENGTH-len, "  Trust: %d\n\r", get_trust( victim ) );
    } else {
      if( pc->remort > 0 ) {
	len += snprintf( buf+len, MAX_STRING_LENGTH-len, " Remort: %-11d", pc->remort );
      }
      if( victim->Level() < LEVEL_HERO && exp_for_level( victim ) > victim->exp )
	snprintf( buf+len, MAX_STRING_LENGTH-len, "Exp.Lvl: %d\n\r", exp_for_level( victim )-victim->exp );
      else 
	snprintf( buf+len, MAX_STRING_LENGTH-len, "Exp.Lvl: -\n\r" );
    }
  } else {
    snprintf( buf+25, MAX_STRING_LENGTH-25, "\n\r" );
  }
  
  page( ch, buf );

  sprintf( buf+5,  "     Hits: %d/%d       ", victim->hit,  victim->max_hit );
  sprintf( buf+25, "   Energy: %d/%d       ", victim->mana, victim->max_mana );
  sprintf( buf+45, "    Moves: %d/%d\n\r",    victim->move, victim->max_move );
  page( ch, buf );

  sprintf( buf+5,  "    Items: %d/%d       ",
	   victim->contents.number, victim->can_carry_n( ) );
  sprintf( buf+25, "   Weight: %d/%d       ",
	   victim->contents.weight/100, victim->Capacity( )/100 );

  if( pc )
    sprintf( buf+45, "      Age: %d years\n\r", pc->Age( ) );
  else
    sprintf( buf+45, "\n\r" ); 
  page( ch, buf );

  if( pc ) 
    page( ch, "      Qst_Pnts: %-8d Prac_Pnts: %-9d Gsp_Pnts: %d\n\r",
      pcdata->quest_pts, pcdata->practice, pc->gossip_pts );

  page( ch, "     Hit Regen: %-8.1f Ene Regen: %-9.1f Mv Regen: %.1f\n\r",
	(double)victim->Hit_Regen( ) / 10.0, (double)victim->Mana_Regen( ) / 10.0,
	(double)victim->Move_Regen( ) / 10.0 );

  obj_data *wield, *secondary, *shield;
  get_wield( victim, wield, secondary, shield );

  if( !secondary ) {
    page( ch, "       Hitroll: %-+10.2f Damroll: %+.2f\n\r\n\r", 
	  victim->Hitroll( wield ), victim->Damroll( wield ) );
  } else {
    page( ch,
	  "     Hitroll: %+3.2f/%+3.2f      Damroll: %+3.2f/%+3.2f\n\r\n\r", 
	  victim->Hitroll( wield ), victim->Hitroll( secondary ),
	  victim->Damroll( wield ), victim->Damroll( secondary ) );
  }

  /*  ABILITIES */

  sprintf( buf+5, "Str: %2d(%2d)  Int: %2d(%2d)  Wis: %2d(%2d)",
	   victim->Strength( ),     victim->shdata->strength,
	   victim->Intelligence( ), victim->shdata->intelligence,
	   victim->Wisdom( ),       victim->shdata->wisdom );
  sprintf( buf+strlen( buf ), "  Dex: %2d(%2d)  Con: %2d(%2d)\n\r",
	   victim->Dexterity( ),    victim->shdata->dexterity,
	   victim->Constitution( ), victim->shdata->constitution );
  page( ch, buf );
  
  /*  RESISTANCES */
  show_resists( ch, victim );
  
  /*  MONEY, POSITION, CONDITION */

  page( ch, "\n\r        Coins:%s.\n\r", coin_phrase( victim ) );
  
  if( pc && ( pc->bank != 0 || !pc->locker.is_empty( ) ) ) {
    page( ch,"         Bank: %d cp and %.2f lbs of equipment.\n\r",
	  pc->bank,
	  0.01*(double)pc->locker.weight );
  }
  
  page( ch, "     Position: [ %s ]  Condition: [%s ]\n\r\n\r",
	victim->position_name( ), condition_name( victim ) );
  
  page_centered( ch,
		 "[Also try the command identity for more information.]" ); 
}


/*
 *   STATISTICS ROUTINE
 */


void do_statistics( char_data* ch, const char *argument )
{
  if( is_confused_pet( ch ) )
    return;

  char_data *victim  = ch;

  if( *argument && ch->Level() >= LEVEL_APPRENTICE ) {
    in_character = false;
    if( !( victim = one_player( ch, argument, "stat", 
				(thing_array*) &player_list ) ) )
      return;
    if( victim != ch
	&& get_trust( victim ) >= get_trust( ch ) ) {
      send( ch, "You can only see statistics on players of lower trust than yourself.\n\r" );
      return;
    }
  }

  int move = victim->base_move + victim->mod_move;
  int dam = move - ( victim->hit*move/victim->max_hit );

  if( victim->species ) {
    page_title( ch, victim->species->Name() );
  } else {
    page_title( ch, victim->descr->name );
  }

  page( ch, "\n\r" );

  page_underlined( ch, "%20s%20s%20s\n\r",
		   "Hit Points", "Energy", "Move" );

  page( ch, "%13s:%7d%13s:%7d%13s:%7d\n\r",
	"Base", victim->base_hit,
	"Base", victim->base_mana,
	"Base", victim->base_move );

  page( ch, "%13s:%7d)%12s:%7d)%12s:%7d)\n\r",
	"(Exp", (int) victim->Mean_Hp( ),
	"(Exp", (int) victim->Mean_Mana( ),
	"(Exp", (int) victim->Mean_Move( ) );

  page( ch, "%13s:%7d%13s:%7d%13s:%7d\n\r",
	"+Mod", victim->mod_hit,
	"+Mod", victim->mod_mana,
	"+Mod", victim->mod_move );

  page( ch, "%13s:%7d%13s:%7d%13s:%7d\n\r",
	"+Con", victim->Level()*(victim->Constitution( )-12)/2,
	"+Int", victim->Level()*victim->Intelligence( )/4,
	"-Wounds", -dam );

  page( ch, "%13s:%7d%13s:%7d%13s:%7d\n\r",
	"Max", victim->max_hit,
	"-Leech", -leech_max( victim ),
	"Max", victim->max_move );

  page( ch, "%34s:%7d\n\r",
	"-Prep", -prep_max( victim ) );

  page( ch, "%34s:%7d\n\r",
	"Max", victim->max_mana );
}
