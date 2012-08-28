#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include "define.h"
#include "struct.h"


static affect_data *find_leech( char_data* ch, const char *argument )
{ 
  int i;

  if( number_arg( argument, i ) ) {
    if( --i < 0 || i >= ch->leech_list ) {
      send( ch, 
	    "You are not supporting any affect with that number.\n\r" );
      return 0;
    }
    return ch->leech_list[i];
  }
  
  for( i = 0; i < ch->leech_list; i++ ) 
    if( fmatches( argument, aff_char_table[ ch->leech_list[i]->type ].name ) ) 
      return ch->leech_list[i];
  
  send( ch, "You are not supporting any such leech.\n\r" );

  return 0;
}   


static void unleech( char_data *ch, affect_data *affect )
{
  if( ch != affect->target ) 
    fsend( ch,
	   "You no longer leech %s on %s.",
	   aff_char_table[affect->type].name, affect->target->Name( ) );
  
  if( char_data *victim = character( affect->target ) ) {
    fsend( victim, "The energy leech supplying %s on you is cut.",
	   aff_char_table[affect->type].name );
  }

  remove_leech( affect );
}


void do_leech( char_data* ch, const char *argument )
{
  affect_data*  affect;
  cast_data*      cast;
  int                i;
  char_data *victim = ch;

  if( *argument ) {
    if( !strcasecmp( "clear", argument ) ) {
      if( ch->leech_list.is_empty() ) {
	send( ch, "You are not supporting any spells.\n\r" );
	return;
      }
      for( i = ch->leech_list.size-1; i >= 0; --i ) {
	unleech( ch, ch->leech_list[i] );
      }
      return;
    }

    if( !has_permission( ch, PERM_PLAYERS )
	|| !( victim = one_player( ch, argument, empty_string,
				   (thing_array*) &player_list ) ) ) {
      if( !( affect = find_leech( ch, argument ) ) )
	return;
      unleech( ch, affect );
      return;
    }

    if( victim != ch
	&& get_trust( victim ) >= get_trust( ch ) ) {
      fsend( ch, "You cannot view the leeches of %s.", victim );
      return;
    }
  }
  
  if( victim->leech_list.is_empty() ) {
    if( victim == ch )
      send( ch, "You are not supporting any spells.\n\r" );
    else
      fsend( ch, "%s is not supporting any spells.", victim );
    return;
  }
  
  int max_regen = victim->Mana_Regen( );
  int max_mana = victim->max_mana;
 
  const int mana = max_mana;
  const int regen = max_regen;
  int prepare = 0;

  for( cast = victim->prepare; cast; cast = cast->next )
    prepare += cast->mana*cast->times;

  max_mana += prepare;
  
  for( i = 0; i < victim->leech_list; ++i ) {
    affect = victim->leech_list[i];
    max_regen += affect->leech_regen;
    max_mana  += affect->leech_max;
  }
  
  if( victim != ch ) {
    page_title( ch, "Leeches for %s", victim );
  }

  char tmp  [ TWO_LINES ];

  if( prepare > 0 )
    snprintf( tmp, TWO_LINES, " ( %d used by prepare )", prepare );
  else
    *tmp = '\0';

  page( ch, "   Max Mana: %4d (%d max)%s\n\r", mana, max_mana, tmp );
  page( ch, "Regen. Rate: %4.1f (%.1f max)\n\r\n\r",
	double( regen/10.0 ), double( max_regen/10.0 ) );
  
  page_underlined( ch,
		   "Num  Spell                    Regen     Max     Character\n\r" );
  
  for( i = 0; i < victim->leech_list; i++ ) {
    affect = victim->leech_list[i];
    page( ch, "%3d  %-24s %5.1f%8d     %s\n\r",
	  i+1, capitalize_words( aff_char_table[affect->type].name ),
	  double( affect->leech_regen/10.0 ), affect->leech_max, 
	  affect->target == victim ? "[self]" : affect->target->Name( ch ) );
  }
  
  page( ch, "\n\r[ Regen is how much supporting the spell lowers your mana regen rate\n\r  and Max is how much it lowers your maximum mana amount. ]\n\r" );
}


void sprintf_leech( char* tmp, int value )
{
  const dice_data dice = value;

  if( dice.side != 0 ) { 
    if( dice.plus == 1 ) {
      if( dice.side == 1 )
        sprintf( tmp, "%d-level", dice.number );
      else
        sprintf( tmp, "%d-%d*level", dice.number, dice.side );
    }
    else {
      if( dice.side == 1 ) 
        sprintf( tmp, "%d-level/%d", dice.number, dice.plus );
      else
        sprintf( tmp, "%d-%d*level/%d", dice.number, dice.side, dice.plus );
    }
  }
  else {
    sprintf( tmp, "%d", dice.number );
  }
} 


int leech_max( char_data* ch )
{
  int j = 0;

  for( int i = 0; i < ch->leech_list; ++i )
    j += ch->leech_list[i]->leech_max;

  return j;
}


int leech_regen( char_data* ch )
{
  int j = 0;

  for( int i = 0; i < ch->leech_list; ++i )
    j += ch->leech_list[i]->leech_regen;

  return j;
}


/*
 *   REMOVE
 */


void remove_leech( affect_data *affect )
{
  char_data *source = affect->leech;
 
  if( !source )
    return;

  source->leech_list -= affect;
  affect->leech = 0;
  
  update_max_mana( source );
}


void remove_leech( char_data* ch )
{
  for( int i = 0; i < ch->leech_list; ++i ) 
    ch->leech_list[i]->leech = 0;
 
  ch->leech_list.clear( );
  update_max_mana( ch );
}
