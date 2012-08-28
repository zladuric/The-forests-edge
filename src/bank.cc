#include <ctype.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include "define.h"
#include "struct.h"


static bool        in_bank            ( char_data* );
obj_data*   obj_from_locker    ( obj_data*, char_data*, int );


/*
 *   BANK FUNCTIONS
 */


bool in_bank( char_data* ch )
{
  if( is_mob( ch ) )
    return false;
  
  if( !ch->Can_See( true ) )
    return false;

  room_data *room;

  if( !( room = Room( ch->array->where ) )
      || !is_set( room->room_flags, RFLAG_BANK ) ) {
    send( ch, "You are not in a banking institution.\n\r" );
    return false;
  }
  
  return true;
}


void do_balance( char_data* ch, const char *argument )
{
  player_data *pc;
  bool loaded = false;

  if( has_permission( ch, PERM_PLAYERS )
      && *argument ) {
    in_character = false;

    char arg [ MAX_INPUT_LENGTH ];

    argument = one_argument( argument, arg );

    pfile_data *pfile = find_pfile( arg, ch );

    if( !pfile )
      return;

    if( pfile != ch->pcdata->pfile
	&& pfile->trust >= get_trust( ch ) ) {
      fsend( ch, "You cannot view the bank account of %s.", pfile->name );
      return;
    }

    if( !( pc = find_player( pfile ) ) ) {
      link_data link;
      link.connected = CON_PLAYING;
      if( !load_char( &link, pfile->name, PLAYER_DIR ) ) {
	bug( "Load_players: error reading player file. (%s)", pfile->name );
	return;
      }
      pc = link.player;
      loaded = true;
    }

    if( *argument ) {
      if( ch != pc ) {
	snprintf( arg, MAX_INPUT_LENGTH, "Bank account for %s", pc->Name( ch ) );
      }
      browse( ch, argument, &pc->locker, ( ch == pc ) ? 0 : arg );

      if( loaded ) {
	page( ch, "\n\r" );
	page_centered( ch, "[ Player file was loaded from disk. ]" );
	pc->Extract();
	extracted.delete_list();
      }
      return;
    }

  } else {
    
    if( !in_bank( ch ) )
      return;
    
    pc = player( ch );
  }

  bool same = ( ch == pc );

  if( pc->locker.is_empty() ) {
    if( pc->bank == 0 ) {
      fpage( ch, "%s %s no items or coins stored with the bank.",
	     same ? "You" : pc->Name( ch ),
	     same ? "have" : "has" );
    } else {
      fpage( ch,
	    "%s %s %d copper coin%s, but no items stored with the bank.",
	     same ? "You" : pc->Name( ch ),
	     same ? "have" : "has",
	     pc->bank, pc->bank == 1 ? "" : "s" );
    }

  } else {
    
    select( pc->locker );
    rehash_weight( ch, pc->locker );
    
    if( !same ) {
      page_title( ch, "Bank account for %s", pc );
    }
    
    page( ch, "Copper Coins: %d\n\r", pc->bank );
    page( ch, "Total Weight: %.2f lbs\n\r\n\r",
	  0.01*(double)pc->locker.weight );
    
    page_underlined( ch,
		     "Items in Storage                                                        Weight\n\r" );
    
    for( int i = 0; i < pc->locker; i++ ) {
      obj_data *obj = (obj_data*) pc->locker[i];
      if( obj->Shown( ) > 0 ) {
	page( ch, "%-70s %7.2f\n\r",
	      obj->Seen_Name( ch, obj->Shown( ), true ),
	      0.01*(double)obj->temp );
      }
    }
  }

  if( loaded ) {
    page( ch, "\n\r" );
    page_centered( ch, "[ Player file was loaded from disk. ]" );
    pc->Extract();
    extracted.delete_list();
  }
}


/*
 *   DEPOSIT
 */


thing_data* deposit( thing_data* thing, char_data* ch, thing_data* )
{
  obj_data *obj = (obj_data*) thing->From( thing->Selected( ) );
  player_data *pc = (player_data*) ch;

  if( obj->pIndexData->item_type != ITEM_MONEY ) {
    obj->To( pc->locker );
  } else {
    pc->bank += monetary_value( obj );
    obj->Extract( );
  }

  return obj;
}


void do_deposit( char_data* ch, const char *argument )
{
  if( !in_bank( ch ) )
    return;

  player_data *pc = player( ch );

  if( !*argument ) {
    send( ch, "What do you want to deposit?\n\r" );
    return;
  }

  int amount;
  if( number_arg( argument, amount ) ) {
    if( amount <= 0 ) {
      send( ch, "You may only deposit positive amounts.\n\r" );
    } else if( !remove_coins( ch, amount, "You deposit" ) ) {
      send( ch, "You don't have that much to deposit.\n\r" );
    } else {
      pc->bank += amount;
      send( ch, "You now have %d cp in your account.\n\r", pc->bank );
    }
    return;
  }

  thing_array *array;
  if( !( array = several_things( ch, argument, "deposit", &ch->contents ) ) )
    return;

  const bool empty = pc->bank == 0 && pc->locker.is_empty( );

  thing_array    subset  [ 6 ];
  thing_func*      func  [ 6 ]  = { cursed, stolen, stolen_contents,
				    corpse, no_room, deposit };

  sort_objects( ch, *array, 0, 6, subset, func );

  msg_type = MSG_BANK;

  page_priv( ch, 0, empty_string );
  page_priv( ch, &subset[0], "can't let go of" );
  page_priv( ch, &subset[1], "don't own" );
  page_priv( ch, &subset[2], "don't own something in" );
  page_priv( ch, &subset[3], 0, 0, "is refused", "are refused" );
  page_priv( ch, &subset[4], "don't have space for" );

  if( !subset[5].is_empty() && empty ) {
    page( ch, "You open a bank account.\n\r" );
  }

  page_publ( ch, &subset[5], "deposit" );

  delete array;
}


/*
 *   WITHDRAW
 */


static const char *const auction_msg = "That would take your bank balance below what\
 you have bid on the\n\rauction block and if you did that the delivery daemons would rip you apart\n\rand play catch with the pieces.\n\r";


void do_withdraw( char_data* ch, const char *argument )
{
  if( !in_bank( ch ) )
    return;

  player_data *pc = player( ch );

  if( !*argument ) {
    send( ch, "What do you want to withdraw?\n\r" );
    return;
  }
  
  int amount;
  // Use isdigit() so +1, -1, etc. are keywords, not amounts.
  // (For +1 weapon/armor, etc.)
  if( isdigit( *argument ) && number_arg( argument, amount ) ) {
    if( amount == 0 ) {
      send( ch, "Your balance is %d copper coins.\n\r", pc->bank );
      send( ch, "How much do you wish to withdraw?\n\r" );
      return;
    } else if( amount < 0 ) {
      send( ch, "Number out of range.\n\rTry a smaller amount.\n\r" );
      return;
    } if( amount > pc->bank ) {
      send( ch, "That is more than you have in your account.\n\r" );
    } else if( amount > free_balance( pc ) ) {
      send( ch, auction_msg );
    } else {
      add_coins( ch, amount, "The bank teller hands you" );
      fsend_seen( ch, "%s withdraws some money.", ch );
      pc->bank -= amount;
      if( pc->bank == 0 && pc->locker.is_empty( ) ) {
	page( ch, "You close your bank account.\n\r" );
      } else {
	send( ch, "Your remaining balance is %d copper coins.\n\r", pc->bank );
      }
    }
    return;
  }
  
  thing_array *array;

  if( !( array = several_things( ch, argument, "withdraw", &pc->locker ) ) ) 
    return;

  /* HANDLE ITEM LIST */ 

  thing_array   subset  [ 3 ];
  thing_func*     func  [ 3 ]  = { heavy, many, to_char };

  sort_objects( ch, *array, 0, 3, subset, func );

  msg_type = MSG_BANK;

  page_priv( ch, 0, empty_string );
  page_priv( ch, &subset[0], "can't lift" );
  page_priv( ch, &subset[1], "can't handle" );
  page_publ( ch, &subset[2], "withdraw" );

  if( pc->bank == 0 && pc->locker.is_empty( ) ) {
    page( ch, "You close your bank account.\n\r" );
  }

  delete array;
}
