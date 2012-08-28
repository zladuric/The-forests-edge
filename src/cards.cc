#include <ctype.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include "define.h"
#include "struct.h"


const char *const suit_name [] = {
  "Candles", "Bells", "Wands", "Swords"
};

const char *const cards = "KMPR98765432S";


const char *const joker_name [] = {
  "Fool",
  "Jester"
};

const char *const card_name [] = {
  "Star",
  "deuce",
  "three",
  "four",
  "five",
  "six",
  "seven",
  "eight",
  "nine",
  "Rogue",
  "Priest",
  "Magus",
  "Knight"
};


#define JOKER_0 13*4
#define JOKER_1 13*4+1


void look_cards( char_data* ch, obj_data* deck )
{
  int count = 0;

  for( int i = 0; i < 4*13+2; ++i ) {
    if( is_set( deck->value, i ) )
      ++count;
  }

  fpage( ch, "%s contains %s card%s.", deck,
	 count == 0 ? "no" : number_word( count, ch ),
	 count == 1 ? "" : "s" );


  /*
  char tmp [ SIX_LINES ];

  page( ch, "%s contains:\n\r\n\r", deck );

  for( int suit = 0; suit < 4; ++suit ) {
    snprintf( tmp, SIX_LINES, "%10s: ", suit_name[suit] );
    char *letter = tmp+12;
    for( int card = 0; card < 13; ++card ) {
      if( is_set( deck->value, 13*suit+card ) ) {
        *letter = cards[card];
	++letter;
      }
    }
    snprintf( letter, SIX_LINES, "%s\n\r",
	      letter == tmp+12 ? "none" : "" );
    page( ch, tmp );
  }

  snprintf( tmp, SIX_LINES, "%10s: ", "Others: " );
  char *letter = tmp+12;

  if( is_set( deck->value, JOKER_0 ) ) {
    snprintf( letter, SIX_LINES, "%s", joker_name[0] );
    letter += strlen( joker_name[0] );
  }

  if( is_set( deck->value, JOKER_1 ) ) {
    snprintf( letter, SIX_LINES, "%s%s",
	      letter == tmp+12 ? "" : ", ",
	      joker_name[1] );
  }

  if( letter == tmp+12 ) {
    snprintf( letter, SIX_LINES, "none" );
  }

  page( ch, "%s\n\r", tmp );
  */
}


void do_draw( char_data *ch, const char *argument )
{
}


#define OBJ_HAND_CARDS 4631
#define OBJ_CARD 4632


static void set_name( obj_data *obj, const char *name )
{
  if( obj->singular != obj->pIndexData->singular ) {
    free_string( obj->singular, MEM_OBJECT );
  }
  obj->singular = alloc_string( name, MEM_OBJECT );
}


void do_deal( char_data *ch, const char *argument )
{
  obj_data *deck = find_type( ch, ch->wearing, ITEM_DECK_CARDS );

  if( !deck ) {
    send( ch, "You must be holding a deck of cards to deal.\n\r" );
    return;
  }

  int flags;

  if( !get_flags( ch, argument, &flags, "a", "deal" ) ) {
    return;
  }

  const bool add = is_set( flags, 0 );

  int number = 1;
  number_arg( argument, number );

  if( number <= 0 || number > 54 ) {
    send( ch, "Not much good at dealing cards, are you?\n\r" );
    return;
  }

  player_data *victim = one_player( ch, argument, "deal to", ch->array );

  if( !victim ) {
    return;
  }

  obj_data *worn = victim->Wearing( WEAR_HELD_R, LAYER_BASE );

  if( worn && worn->pIndexData->vnum != OBJ_HAND_CARDS ) {
    fsend( ch, "You cannot deal to %s, since %s right hand is not empty.",
	   victim, victim->His_Her( ch ) );
    return;
  }

  if( !worn && add ) {
    fsend( ch, "You cannot add to %s's hand, since %s doesn't have one yet.",
	   victim, victim->He_She( ch ) );
    return;
  }

  if( worn && !add ) {
    fsend( ch, "You cannot deal to %s, since %s right hand is not empty.",
	   victim, victim->His_Her( ch ) );
    fsend( ch, "(Use \"deal -a\" to add to %s hand.)", victim->His_Her( ) );
    return;
  }

  if( is_set( victim->pcdata->pfile->flags, PLR_NO_GIVE ) ) {
    fsend( ch, "%s refuses to take any cards.", victim );
    return;
  }

  int count = 0;
  for( int i = 0; i < 4*13+2; ++i ) {
    if( is_set( deck->value, i ) ) {
      ++count;
    }
  }
  
  if( number > count ) {
    fsend( ch, "The deck only contains %d cards.", count );
    return;
  }

  char tmp [THREE_LINES];

  if( !worn ) {
    worn = create( get_obj_index( OBJ_HAND_CARDS ) );
    worn->position = WEAR_HELD_R;
    worn->layer = LAYER_BASE;
    worn->value[0] = 0;
    worn->To( victim->wearing );
  }
  
  for( int i = 0; i < number; ++i ) {
    int n = number_range( 1, count );
    for( int j = 0; j < 4*13+2; ++j ) {
      if( is_set( deck->value, j )
	  && --n == 0 ) {
	remove_bit( deck->value, j );
	--count;
	obj_data *old = 0;
	for( int k = 0; k < worn->contents; ++k ) {
	  old = (obj_data*) worn->contents[k];
	  if( old->pIndexData->vnum == OBJ_CARD
	      && old->value[0] == j ) {
	    break;
	  }
	  old = 0;
	}
	if( old ) {
	  break;
	}
	obj_data *obj = create( get_obj_index( OBJ_CARD ) );
	if( j >= 4*13 ) {
	  snprintf( tmp, THREE_LINES, "%s", joker_name[j-4*13] );
	} else {
	  const int card = j % 13;
	  const int suit = j / 13;
	  snprintf( tmp, THREE_LINES, "%s of %s",
		    card_name[ card ], suit_name[ suit ] );
	}
	set_bit( obj->extra_flags, OFLAG_THE_BEFORE );
	set_bit( obj->extra_flags, OFLAG_THE_AFTER );
	set_name( obj, tmp );
	obj->value[0] = j;
	obj->To( worn );
	++worn->value[0];
	break;
      }
    }
  }

  snprintf( tmp, THREE_LINES, "hand of %d playing card%s",
	    worn->value[0], worn->value[0] == 1 ? "" : "s" );
  set_name( worn, tmp );
  
  if( ch == victim ) {
    if( number == 1 ) {
      fsend( ch, "You deal a card to yourself." );
      fsend_seen( ch, "%s deals a card to %sself.", ch, ch->Him_Her( ) );
    } else {
      fsend( ch, "You deal %s cards to yourself.", number_word( number, ch ) );
      fsend_seen( ch, "%s deals %d cards to %sself.", ch, number, ch->Him_Her( ) );
    }
  } else {
    if( number == 1 ) {
      fsend( ch, "You deal a card to %s.", victim );
      fsend( victim, "%s deals a card to you.", ch );
      fsend_seen( ch, "%s deals a card to %s.", ch, victim );
    } else {
      fsend( ch, "You deal %s cards to %s.", number_word( number, ch ), victim );
      fsend( victim, "%s deals %s cards to you.", ch, number_word( number, victim ) );
      fsend_seen( ch, "%s deals %d cards to %s.", ch, number, victim );
    }
  }
  
}
