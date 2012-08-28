#include <sys/types.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include "define.h"
#include "struct.h"


auction_array  auction_list;

static char auc_buf [ SIX_LINES ];

static void  transfer_buyer   ( auction_data* ); 
static void  return_seller    ( auction_data*, bool msg = false );
static bool  no_auction       ( char_data*, obj_data* );
static void  display_auction  ( player_data* );


static const char *const undo_msg = "An auction daemon stamps up to you and hands you %s.  It\
 mutters something about making up your mind while tapping its forehead\
 and storms off.";

static const char *const corpse_msg = "An auction daemon runs up to you.  You hand it\
 %s and a silver coin.  It looks at the corpse, looks at you, rips the corpse\
 apart, eats it, smiles happily and disappears with your silver coin.";

static const char *const no_auction_msg = "An auction daemon runs up to you.  You hand\
 it a silver coin and attempt to hand it %s, but it quickly refuses with\
 some mumble about items banned by the gods.  It then disappears in cloud of\
 smoke.  Only afterwards do you realize your silver coin went with it.";

static const char *const distinct_msg = "Due to idiosyncrasies of the accountant daemons\
 you may only auction lots of items which contain at most 5 distinct types of\
 items and distinct is unfortunately defined by them and not by what\
 you see.";

static const char *const gossip_msg = "You are unable to contact the auction daemon, having\
 negative gossip points.";
 

static bool can_auction( char_data* ch, obj_array* array )
{
  if( ch->Level() >= LEVEL_DEMIGOD ) {
    return true;
  }

  player_data *pc = player( ch );

  if( pc
      && pc->gossip_pts < 0 ) {
    fsend( ch, gossip_msg );
    return false;
  }

  if( *array > 5 ) {
    fsend( ch, distinct_msg );
    return false;
  }

  for( int i = 0; i < *array; ++i ) {
    if ( obj_data *obj = object( array->list[i] ) ) {
      
      if( !obj->droppable( ch ) ) {
	send( ch, "You can't auction items which you can't let go of.\n\r" );
	return false;
      }
      
      if( !stolen( obj, ch ) ) {
	send( ch, "You can't auction items which you don't own.\n\r" );
	return false;
      }

      if( !stolen_contents( obj, ch ) ) {
	send( ch, "You can't auction items containing something you don't own.\n\r" );
	return false;
      }

      /*
	Handled in do_auction() now.
      if( obj->pIndexData->item_type == ITEM_MONEY ) {
	send( ch, "You can't auction money!\n\r" );
	return false;
      }
      */
    }
  }

  if( privileged( ch, LEVEL_BUILDER ) ) {
    return true;
  }

  if( !remove_silver( ch ) ) {
    send( ch, "To auction you need a silver coin to bribe the delivery daemon.\n\r" );
    return false;
  }

  // These are done later so the daemon takes your money first.
  for( int i = 0; i < *array; i++ ) {
    if ( obj_data *obj = object( array->list[i] ) ) {
      if( obj->pIndexData->item_type == ITEM_CORPSE ) {
	fsend( ch, corpse_msg, obj );
	  obj->Extract( obj->Selected( ) );
	  return false;
      }
      if( no_auction( ch, obj ) ) {
	fsend( ch, no_auction_msg, obj );
	return false;
      }
    }
  }

  return true;
}


void do_auction( char_data* ch, const char *argument )
{
  if( is_mob( ch ) )
    return;
  
  player_data *pc = player( ch );

  int flags = 0;

  if( has_permission( ch, PERM_PLAYERS )
      && !get_flags( ch, argument, &flags, "cb", "auction" ) )
    return;

  /* 
  if( !strcasecmp( argument, "on" ) || !strcasecmp( argument, "off" ) ) {
    send( ch, "See help iflag for turning on and off auction.\n\r" );
    return;
  }
  */
  
  char arg  [ MAX_INPUT_LENGTH ];
  char tmp1  [ SIX_LINES ];
  auction_data *auction;
  thing_array *array;

  if( is_set( flags, 0 ) ) {
    if( auction_list.is_empty() ) {
      send( ch, "The auction block is empty.\n\r" );
      return;
    }

    snprintf( auc_buf, SIX_LINES, "++ Auction Block cleared by %s ++", ch->real_name( ) );
    info( 0, "++ Auction Block cleared ++", invis_level( ch ), auc_buf, IFLAG_AUCTION, 1, ch );
    send( ch, "-- Auction block cleared --\n\r" );

    clear_auction();
    return;
  }

  if( !*argument ) {
    display_auction( pc );
    return;
  }

  if( is_set( flags, 1 ) ) {
    int slot;
    if( !number_arg( argument, slot )
	|| slot < 0
	|| !*argument ) {
      send( ch, "Syntax: Auction -b <slot> <container> ...\n\r" );
      return;
    }
    for( int i = 0; i < auction_list; ++i ) {
      auction = auction_list[i];
      if( auction->slot != slot )
	continue;
      if( auction->seller ) {
	snprintf( arg, MAX_INPUT_LENGTH, "Auctioned by %s", auction->seller->name );
      }
      browse( ch, argument, &auction->contents,
	      auction->seller ? arg : "Auctioned by a deceased character" );
      return;
    }
    send( ch, "There is no lot #%d on the auction block.\n\r", slot );
    return;
  }

  argument = one_argument( argument, arg );
  
  if( !strcasecmp( arg, "undo" ) ) {
    int slot = -1;
    if( *argument && ( !number_arg( argument, slot )
		       || slot < 0 )
	|| *argument ) {
      send( ch, "Syntax: Auction undo [<slot>]\n\r" );
      return;
    }
    for( int i = 0; i < auction_list; ++i ) {
      auction = auction_list[i];
      const bool override = ( auction->seller != ch->pcdata->pfile );
      if( slot < 0 ) {
	if( override )
	  continue;
      } else {
	if( auction->slot != slot )
	  continue;
	if( override
	    && ch->Level( ) < LEVEL_DEMIGOD ) {
	  send( ch, "Lot #%d was auctioned by someone else, so you can't remove it.\n\r",
		slot );
	  return;
	}
      }
      if( auction->time < AUCTION_TIME-5
	  && ( auction->buyer || auction->deleted )
	  && ch->Level() < LEVEL_DEMIGOD ) {
	send( ch, "Lot #%d has been on the auction block too long to remove it.\n\r",
	      auction->slot );
	return;
      }
      select( auction->contents );
      snprintf( auc_buf, SIX_LINES, "%s has removed lot #%d, %%s from the auction block.",
		ch->real_name( ), auction->slot );
      snprintf( tmp1, SIX_LINES, "Someone has removed lot #%d, %%s from the auction block.",
		auction->slot );
      info( 0, tmp1, invis_level( ch ), auc_buf, IFLAG_AUCTION, 2, ch, 0, 0, &auction->contents );

      //      if( !override )
      //	fsend( ch, undo_msg, &auction->contents );

      //      auction_list -= auction;
      return_seller( auction );

      /*
      // *** FIX ME: add can_carry check.
      thing_array array2 = auction->contents;
      select( array2 );
      transfer_objects( ch, ch->contents, 0, array2 );
      */
      /*
      for( int j = 0; j < array2; j++ ) {
	if( ( obj = object( array2[j] ) ) ) {
	  obj->transfer_object( ch, 0, &ch->contents, obj->Number( ) );
	}
      }
      */
      
      delete auction;
      return;
    }
    if( slot < 0 )
      send( ch, "You have no lots on the auction block.\n\r" );
    else
      send( ch, "There is no lot #%d on the auction block.\n\r", slot );
    return;
  }  
  
  if( is_set( ch->pcdata->pfile->flags, PLR_NO_AUCTION ) ) {
    send( ch, "The immortals have banned you from participating in auctions.\n\r" );
    return;
  }

  if( ch->Level() < LEVEL_DEMIGOD ) {
    int count = 0;
    for( int i = 0; i < auction_list; ++i ) 
      if( auction_list[i]->seller == ch->pcdata->pfile ) {
	if( ++count >= 3 ) {
	  send( ch, "You may only have 3 lots on the auction block at once.\n\r" );
	  return;
	}
      }
  }

  if( ch->position <= POS_MEDITATING ) {
    pos_message( ch );
    return;
  }

  if( ch->Level( ) < LEVEL_APPRENTICE
      && ch->in_room->vnum == ROOM_PRISON ) {
    send( ch, "The auction daemon refuses to visit you in prison.\n\r" );
    return;
  }

  if( !( array = several_things( ch, arg, "auction", &ch->contents ) ) )
    return;

  if( ch->Level() < LEVEL_DEMIGOD ) {
    for( int i = array->size-1; i >= 0; --i ) {
      obj_data *obj = (obj_data*) array->list[i];
      if( obj->pIndexData->item_type == ITEM_MONEY ) {
	array->remove(i);
      }
    }
    
    if( array->is_empty( ) ) {
      send( ch, "You can't auction money!\n\r" );
      return;
    }
  }

  int bid;

  if( !*argument || !number_arg( argument, bid ) || *argument ) {
    send( ch, "Syntax: auction <item> <minimum bid>\n\r" );
    delete array;
    return;
  }

  if( bid < 1 ) {
    send( ch, "Minimum bid must be at least 1 cp.\n\r" );
    delete array;
    return;
  }

  if( bid > MAX_AUCTION_BID ) {
    send( ch, "The minimum bid cannot be greater than %d cp.\n\r", MAX_AUCTION_BID );
    delete array;
    return;
  }

  if( !can_auction( ch, (obj_array*) array ) ) {
    delete array;
    return;
  }

  /* FIND FIRST OPEN SLOT NUMBER */

  int slot = 1;
  for( int i = 0; i != auction_list; ) {
    if( auction_list[i]->slot == slot ) {
      ++slot;
      i = 0;
    } else {
      ++i;
    }
  }

  /* AUCTION ITEM */

  if( ch->Level( ) < LEVEL_DEMIGOD ) {
    fsend( ch, "An auction daemon runs up to you.  You hand it\
 %s and a silver coin and it sprints off to the auction house.",
	   list_name( ch, array, empty_string ).c_str() );
  } else {
    fsend( ch, "An auction daemon runs up to you.  You hand it\
 %s then smack it upside the head. It trudges away to the auction house.",
	   array );
  }

  auction          = new auction_data( ch->pcdata->pfile );
  auction->buyer   = 0;
  auction->bid     = bid;
  auction->time    = AUCTION_TIME;
  auction->slot    = slot;

  transfer_objects( 0, auction->contents, ch, *array );

  delete array;

  snprintf( auc_buf, SIX_LINES, "%s has placed %%s on the auction block.",
	    ch->real_name( ) );
  snprintf( tmp1, SIX_LINES, "Someone has placed %%s on the auction block." );
  info( 0, tmp1, invis_level( ch ), auc_buf, IFLAG_AUCTION, 1, ch, 0, 0, &auction->contents );
}


bool no_auction( char_data *ch, obj_data* obj )
{
  if( obj->pIndexData->item_type == ITEM_CORPSE
      || is_set( obj->extra_flags, OFLAG_NO_AUCTION ) )
    return true;

  for( int i = 0; i < obj->contents; ++i ) {
    obj_data *content;
    if( !( content = object( obj->contents[i] ) )
	|| no_auction( ch, content ) ) {
      return true;
    }
  }

  return false;
}


/*
static const char *const stolen_msg = "An auction daemon runs up to you.  You hand it\
 %s and a silver coin.  It stops and looks at it closely and then declares\
 it stolen property from %s and disappears with a mutter about returning it\
 to the true owner.";

static const char *const floor_stolen_msg = "An auction daemon runs up and hands you %s.\
 It bows deeply and then blinks out of existence.";

static const char *const return_stolen_msg = "An auction daemon runs up to you and hands %s\
 at your feet.  It bows deeply and then blinks out of existence.";


static bool stolen_auction( char_data* ch, obj_data* obj )
{
  player_data* player;
  
  // ***FIX ME: this only checks the item, not its contents!

  if( obj->Belongs( ch ) ) 
    return false;
  
  fsend( ch, stolen_msg, obj, obj->owner->name );
  
  if( ( player = find_player( obj->owner ) ) ) {
    fsend( player, floor_stolen_msg, obj );
    obj->transfer_object( player, 0, player->array, obj->Number( ) );
  } else { 
    transfer_file( obj->owner, obj, 0 );
  }
  
  return true;
}
*/


/*
 *   DISPLAY
 */


void display_auction( player_data* pc )
{
  char         condition  [ 50 ];
  char             buyer  [ 20 ];
  //  char               age  [ 20 ];
  auction_data*  auction;
  obj_data*          obj;
  bool             first;

  if( auction_list.is_empty() ) {
    send( pc, "There are no lots on the auction block.\n\r" );
    return;
  }

  page( pc, "Bank Account: %d cp\n\r\n\r", pc->bank );
  page_centered( pc, "+++ The Auction Block +++" );
  page( pc, "\n\r" );
  snprintf( auc_buf, SIX_LINES, "%3s %3s %4s %7s  %-45s %3s %3s %3s\n\r",
	    "Lot", "Tme", "Buyr", "Min.Bid", "Item", "Use", "Wgt", "Cnd" );
  page_underlined( pc, auc_buf );
 
  for( int i = 0; i < auction_list; ++i ) {
    auction = auction_list[i];
    select( auction->contents );
    rehash( pc, auction->contents, true );
    first = true;
    for( int j = 0; j < auction->contents; j++ ) {
      obj = object( auction->contents[j] );
      if( obj->Shown( ) > 0 ) {
        condition_abbrev( condition, obj, pc );
	//        age_abbrev( age, obj, pc );
	//	int weight = obj->Weight( obj->Shown( ) );
	const char *wgt = float3( obj->Weight( obj->Shown( ) ) );
	/*
	char wgt [4];
	if( weight < 0 || weight > 99949 )
	  snprintf( wgt, 4, "***" );
	else if( weight == 0 )
	  snprintf( wgt, 4, "  0" );
	else if( weight < 100 )
	  snprintf( wgt, 4, ".%02d", weight );
	else if( weight < 995 )
	  snprintf( wgt, 4, "%3.1f", double(weight)/100.0 );
	else
	  snprintf( wgt, 4, "%3d", (weight+50)/100 );
	*/
	const char *const name = obj->Seen_Name( pc, 1, true );
	unsigned namelen = strlen( name );
	char *buf = static_string( );
	bool too_long = false;
	if( obj->Shown( ) != 1 ) {
	  char *buf2 = static_string( );
	  snprintf( buf2, THREE_LINES, " (x%d)", obj->Shown( ) );
	  unsigned numlen = strlen( buf2 );
	  if( namelen + numlen > 45 ) {
	    snprintf( buf, THREE_LINES, "%s%s",
		      trunc( name, 58-numlen ).c_str( ),
		      buf2 );
	    too_long = true;
	  } else {
	    snprintf( buf, THREE_LINES, "%s%s",
		      name, buf2 );
	  }
	} else {
	  if( namelen > 45 ) {
	    snprintf( buf, THREE_LINES, "%s",
		      trunc( name, 58 ).c_str( ) );
	    too_long = true;
	  } else {
	    snprintf( buf, THREE_LINES, "%s",
		      name );
	  }
	}
        if( first ) {
          first = false;
          memcpy( buyer, auction->buyer ? auction->buyer->name : " -- ", 4 );
          buyer[4] = '\0';
	  if( too_long ) {
	    snprintf( auc_buf, SIX_LINES, "%3d %3d %-4s %7d  %s\n\r%68s%3s %s %s\n\r",
		      auction->slot,
		      auction->time,
		      buyer,
		      auction->minimum_bid( ),
		      buf,
		      "",
		      can_use( pc, 0, obj ) ? "yes" : "no",
		      wgt,
		      condition );
	  } else {
	    snprintf( auc_buf, SIX_LINES, "%3d %3d %-4s %7d  %-45s %3s %s %s\n\r",
		      auction->slot,
		      auction->time,
		      buyer,
		      auction->minimum_bid( ),
		      buf,
		      can_use( pc, 0, obj ) ? "yes" : "no",
		      wgt,
		      condition );
	  }
	} else {
	  if( too_long ) {
	    snprintf( auc_buf, SIX_LINES, "                      %s\n\r%68s%3s %s %s\n\r",
		      buf,
		      "",
		      can_use( pc, 0, obj ) ? "yes" : "no",
		      wgt,
		      condition );
	  } else {
	    snprintf( auc_buf, SIX_LINES, "                      %-45s %3s %s %s\n\r",
		      buf,
		      can_use( pc, 0, obj ) ? "yes" : "no",
		      wgt,
		      condition );
	  }
	}
        page( pc, auc_buf );
	if( is_set( obj->extra_flags, OFLAG_NOSAVE ) ) {
	  page_color( pc, COLOR_MILD, "                         (no-save item)\n\r", "" );
	}
      }
    }
  }
}


/*
 *   BID
 */


static const char *const min_bid_msg = "The minimum bid for %s is %d.";


void do_bid( char_data* ch, const char *argument )
{
  if( is_mob( ch ) )
    return;

  if( is_set( ch->pcdata->pfile->flags, PLR_NO_AUCTION ) ) {
    send( ch, "The immortals have banned you from participating in auctions.\n\r" );
    return;
  }

  int slot;

  if( !*argument
      || !number_arg( argument, slot )
      || slot < 0 ) {
    send( ch, "Syntax: Bid <slot> <price> [<proxy>]\n\r" );
    return;
  }

  auction_data *auction = 0;

  for( int i = 0; i < auction_list; ++i ) {
    if( auction_list[i]->slot == slot ) {
      auction = auction_list[i];
      break;
    }
  }
  
  if( !auction ) {
    send( ch,
	  "There is no lot #%d on the auction block.\n\r",
	  slot );
    return;
  }
  
  if( auction->seller == ch->pcdata->pfile ) {
    send( ch, "You can't bid on your own lot!\n\r" );
    return;
  }
  
  const int min_bid = auction->minimum_bid( );

  select( auction->contents );

  if( !*argument ) {
    fsend( ch, min_bid_msg,
	   &auction->contents, min_bid );
    if( auction->buyer == ch->pcdata->pfile
	&& auction->proxy > 0 ) {
      fsend( ch, "You have placed a proxy bid of %d cp.", auction->proxy );
    }
    return;
  }

  int bid;
  int proxy = -1;

  if( !number_arg( argument, bid )
      || bid < 0
      || ( *argument && ( !number_arg( argument, proxy )
			  || proxy < 0 ) )
      || *argument ) {
    send( ch, "Syntax: Bid <slot> <price> [<proxy>]\n\r" );
    return;
  }
  
  if( auction->buyer == ch->pcdata->pfile ) {
    bid = max( bid, proxy );
    if( auction->proxy > 0 ) {
      if( bid <= min_bid ) {
	fsend( ch, "You remove your proxy bid on %s.", &auction->contents );
	auction->proxy = -1;
      } else if( bid == auction->proxy ) {
	fsend( ch, "Your proxy bid on %s is already set at %d cp.",
	       &auction->contents, bid );
      } else {
	fsend( ch, "You change your proxy bid on %s from %d to %d cp.",
	       &auction->contents, auction->proxy, bid );
	auction->proxy = bid;
      }
    } else if( bid < min_bid ) {
      fsend( ch, min_bid_msg,
	     &auction->contents, min_bid );
    } else {
      fsend( ch, "You add a proxy bid on %s of %d cp.",
	     &auction->contents, bid );
      auction->proxy = bid;
    }
    return;
  }
  
  bid = max( bid, min( min_bid, proxy ) );
  
  if( bid < min_bid ) {
    fsend( ch, min_bid_msg,
	   &auction->contents, min_bid );
    return;
  }
  
  if( bid > 3*min_bid && bid > 500 ) {
    fsend( ch, "To protect you from yourself you may not bid more than the\
 greater of 3 times the current minimum bid and 500 cp." );
    return;
  }
  
  player_data *pc = player( ch );

  if( pc
      && pc->gossip_pts < 0 ) {
    fsend( ch, gossip_msg );
    return;
  }

  if( ch->Level( ) < LEVEL_APPRENTICE
      && ch->in_room->vnum == ROOM_PRISON ) {
    send( ch, "The auction daemon refuses to visit you in prison.\n\r" );
    return;
  }

  if( max( bid, proxy ) > free_balance( pc, auction ) ) {
    send( ch, "The bid is not accepted due to insufficent funds in your bank\
 account.\n\r" );
    return;
  }

  if( proxy >= 0 && proxy < bid ) {
    send( ch, "A proxy only makes sense if greater than the bid.\n\r" );
    return;
  }
  
  bid = max( bid, min( auction->proxy+1, proxy ) );

  if( bid <= auction->proxy ) {
    fsend( ch, "You bid %d cp for %s, but it is immediately matched by %s.\n\r",
	      bid, &auction->contents, auction->buyer->name );
    snprintf( auc_buf, SIX_LINES, "%s bids %d cp on lot #%d, %%s.",
	      ch->real_name( ), bid, auction->slot );
    info( 0, empty_string, 0, auc_buf, IFLAG_AUCTION, 3, ch, 0, 0, &auction->contents );
    snprintf( auc_buf, SIX_LINES, "The bid is matched by %s.", auction->buyer->name );
    info( 0, "You match the bid.", 0, auc_buf, IFLAG_AUCTION, 3, ch, 0, auction->buyer );
    auction->bid = bid;
    auction->add_time( );
    return;
  }

  if( proxy > bid ) {
    fsend( ch, "You bid %d cp for %s and will automatically match bids on it up to %d cp.",
	   bid, &auction->contents, proxy );
  } else {
    fsend( ch, "You bid %d cp for %s.",
	   bid, &auction->contents );
    proxy = -1;
  }

  snprintf( auc_buf, SIX_LINES, "%s bids %d cp on lot #%d, %%s.",
	    ch->real_name( ), bid, auction->slot );
  info( 0, empty_string, 0, auc_buf, IFLAG_AUCTION, 3, ch, 0, 0, &auction->contents );

  auction->bid     = bid;
  auction->buyer   = ch->pcdata->pfile;
  auction->deleted = false;
  auction->proxy   = proxy;

  auction->add_time( );
}


static const char *const delivery_msg = "An auction daemon runs up and hands you %s.  It mumbles\
 something about %d cp and raiding your bank account and sprints off.";

static const char *const floor1_msg = "An auction daemon runs up and attempts to hand %s to you.";

static const char *const floor2_msg = "Realizing you are unable to carry %s, the daemon snickers \
 rudely, drops the delivery at your feet and sprints off.";

static const char *const return_msg = "An auction daemon runs up and hands you %s.\
 It then marches off without a word.";

static const char *const return_unseen = "Someone hands you %s.";

static const char *const return_floor_msg = "An auction daemon runs up and drops %s at your feet.\
 It then marches off without a word.";


void auction_update( )
{
  time_data start;
  gettimeofday( &start, 0 );

  for( int i = auction_list.size-1; i >= 0; --i ) {
    auction_data *auction = auction_list[i];
    if( auction->contents.is_empty( ) ) {
      snprintf( auc_buf, SIX_LINES,
		"Lot #%d has ceased to exist.",
		auction->slot );
      info( 0, empty_string, 0, auc_buf, IFLAG_AUCTION, 2, 0, 0, 0 );
      extract( auction->contents );
      delete auction;
      
    } else if( --auction->time == 0 ) {
      if( !auction->buyer && !auction->deleted ) {
	return_seller( auction, true );
      } else {
	transfer_buyer( auction );
      }
      delete auction;
    }
  }

  pulse_time[ TIME_AUCTION ] = stop_clock( start );
}


int free_balance( player_data* player, auction_data* replace )
{
  int credit = player->bank;

  for( int i = 0; i < auction_list; i++ ) {
    auction_data *auction = auction_list[i];
    if( auction->buyer == player->pcdata->pfile && auction != replace ) 
      credit -= max( auction->bid, auction->proxy );
  }

  return credit;
}


void auction_message( char_data* ch )
{ 
  if( int i = auction_list.size ) {
    send_centered( ch, "There %s %s lot%s on the auction block.",
		   i == 1 ? "is" : "are",
		   number_word( i ),
		   i == 1 ? "" : "s" );
  }
}


/*
 *   TRANSFERRING OF OBJECT/MONEY
 */


static void transfer_file( pfile_data *pfile, Content_Array (player_data::*where),
			   thing_array *array, int amount = 0 )
{
  link_data link;

  if( !load_char( &link, pfile->name, PLAYER_DIR ) ) {
    bug( "Transfer_File: Non-existent player file (%s)", pfile->name ); 
    if( array ) {
      extract( *array );
    }
    return;
  }

  player_data *player = link.player;
  link.connected = CON_PLAYING;

  player->bank += amount;

  if( array ) {
    transfer_objects( player, player->*where, 0, *array );
  }

  player->Save( );
  player->Extract( );
}


void return_seller( auction_data* auction, bool msg )
{  
  select( auction->contents );

  if( !auction->seller ) {
    if( msg ) {
      snprintf( auc_buf, SIX_LINES,
		"Lot #%d, %%s returned to the estate of a deceased character.",
		auction->slot );
      info( 0, empty_string, 0, auc_buf, IFLAG_AUCTION, 2, 0, 0, 0, &auction->contents );
    }
    extract( auction->contents );
    return;
  }

  auction->seller->auction -= auction;

  player_data *pc = find_player( auction->seller );
 
  if( msg ) {
    char tmp1 [ SIX_LINES ];
    snprintf( auc_buf, SIX_LINES, "Lot #%d, %%s, returned to %s.",
	      auction->slot, auction->seller->name );
    snprintf( tmp1, SIX_LINES, "Lot #%d, %%s, returned to seller.",
	      auction->slot );
    info( 0, tmp1, invis_level( pc ), auc_buf, IFLAG_AUCTION, 2, pc, 0, 0, &auction->contents );
  }
  
  if( pc ) {
    // *** FIX ME: add can_carry check.
    fsend( pc,
	   pc->Can_See( ) ? return_msg : return_unseen,
	   &auction->contents );
    transfer_objects( pc, pc->contents, 0, auction->contents );
    return;
  }
  
  transfer_file( auction->seller, &player_data::contents, &auction->contents );
}


void transfer_buyer( auction_data *auction ) 
{
  select( auction->contents );

  player_data *seller = auction->seller ? find_player( auction->seller ) : 0;
  player_data *buyer = auction->buyer ? find_player( auction->buyer ) : 0;

  if( auction->seller ) {
    auction->seller->auction -= auction;
  }

  if( auction->buyer ) {
    snprintf( auc_buf, SIX_LINES, "Lot #%d, %%s, sold to %s for %d cp.",
	      auction->slot, auction->buyer->name, auction->bid );
    info( 0, empty_string, 0, auc_buf, IFLAG_AUCTION, 2, buyer, 0, 0, &auction->contents );
  } else {
    snprintf( auc_buf, SIX_LINES, "Lot #%d, %%s, sold to the estate of a deceased character.",
	      auction->slot  );
    info( 0, empty_string, 0, auc_buf, IFLAG_AUCTION, 2, 0, 0, 0, &auction->contents );
    extract( auction->contents );
  }

  if( seller ) {
    // Seller is online.
    // Do this before transferring items, in case saving offline buyer auto-saves seller.
    seller->bank += 19*auction->bid/20;
  }

  if( buyer ) {
    // Buyer is online.
    buyer->bank -= auction->bid;
    
    bool carry = ( auction->contents.number <= buyer->can_carry_n() - buyer->contents.number )
      && ( auction->contents.weight <= buyer->Capacity() );

    if( carry ) {
      fsend( buyer, delivery_msg, &auction->contents, auction->bid );
    } else {
      fsend( buyer, floor1_msg, &auction->contents );
      fsend( buyer, floor2_msg, auction->contents.number > 1 ? "them" : "it" );
    }
    
    if( carry ) {
      transfer_objects( buyer, buyer->contents, 0, auction->contents );
    } else {
      transfer_objects( buyer, *buyer->array, 0, auction->contents );
    }

  } else if( auction->buyer ) {
    // Buyer is offline.
    transfer_file( auction->buyer, &player_data::contents, &auction->contents, -auction->bid );
  }

  // Now that the items have been transferred, we can save the seller if necessary.
  // This may be aa second save of the seller (if auto-save forced by saving off-line buyer)
  // but this would be hard to prevent.
  if( auction->seller && !seller ) {
    // Seller is offline.
    transfer_file( auction->seller, &player_data::contents, 0, 19*auction->bid/20 );
  }
}


void clear_auction( pfile_data* pfile )
{
  if( !pfile ) {
    while( !auction_list.is_empty( ) ) {
      return_seller( auction_list[0] );
      delete auction_list[0];
    }
    return;
  }

  for( int i = 0; i < auction_list; ++i ) {
    if( auction_data *auction = auction_list[i] ) {
      if( auction->seller == pfile ) {
	pfile->auction -= auction;
	auction->seller = 0;
      }
      if( auction->buyer == pfile ) {
	auction->buyer = 0;
	auction->deleted = true;
      }
    }
  }
}
