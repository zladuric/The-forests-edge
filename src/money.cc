#include <ctype.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include "define.h"
#include "struct.h"


const int  coin_vnum  [] = { OBJ_COPPER, OBJ_SILVER, OBJ_GOLD, OBJ_PLATINUM }; 
const int   coin_value [] = { 1, 10, 100, 1000 };
const char*  coin_name [] = { "cp", "sp", "gp", "pp" };


/*
 *  VARIOUS MONEY ROUTINES
 */


int monetary_value( obj_data* obj )
{
  if( obj->pIndexData->item_type == ITEM_MONEY ) 
    for( int i = 0; i < MAX_COIN; i++ )
      if( obj->pIndexData->vnum == coin_vnum[i] ) 
        return obj->Selected( )*coin_value[i];

  return 0;
}


char* coin_phrase( char_data* ch )
{
  int      coins  [ MAX_COIN ];
  obj_data*  obj;

  vzero( coins, MAX_COIN );

  for( int i = 0; i < ch->contents; i++ ) {
    if( ( obj = object( ch->contents[i] ) )
	&& obj->pIndexData->item_type == ITEM_MONEY ) 
      for( int j = 0; j < MAX_COIN; j++ )
        if( obj->pIndexData->vnum == coin_vnum[j] ) 
          coins[j] += obj->Number( );
    }

  return coin_phrase( coins );
}


char* coin_phrase( int* num )
{
  static char   buf  [ TWO_LINES ];
  bool         flag  = false;
  int          last = MAX_COIN;
  unsigned count = 0;

  for ( int i = MAX_COIN-1; i >= 0; --i ) {
    if( num[ i ] != 0 ) {
      ++count;
      last = i;
    }
  }

  *buf = '\0';
  
  for( int i = MAX_COIN - 1; i >= last; --i ) {
    if( num[ i ] == 0 )
      continue;
    int l = strlen(buf);
    snprintf( buf+l, TWO_LINES-l,
	      "%s %s%d %s",
	      (flag && count != 2) ? "," : "",
	      ( i == last && flag ) ? "and " : "",
	      num[i], coin_name[i] );
    flag = true;
  }
  
  if( !flag ) 
    snprintf( buf, TWO_LINES, " none" );
  
  return buf;
}


int get_money( char_data* ch )
{
  obj_data*  obj;
  int        sum  = 0;

  for( int i = 0; i < ch->contents; i++ ) {
    for( int j = 0; j < MAX_COIN; j++ ) {
      if( ( obj = object( ch->contents[i] ) )
	  && obj->pIndexData->vnum == coin_vnum[j] ) {
        sum += coin_value[j]*obj->Number( );
      }
    }
  }

  return sum;
}


bool remove_silver( char_data* ch )
{
  if( obj_data *obj = find_vnum( ch->contents, coin_vnum[1] ) ) {
    obj->Extract( 1 );
    return true;
  }

  return false;
}
 

void add_coins( char_data* ch, int amount,
		const char *message, bool page )
{
  obj_data*  obj;
  int        num  [ MAX_COIN ];
  
  for( int i = MAX_COIN - 1; i >= 0; i-- ) {
    if( ( num[i] = amount/coin_value[i] ) > 0 ) {
      amount -= num[i]*coin_value[i];
      obj = create( get_obj_index( coin_vnum[i] ), num[i] ); 
      obj->To( ch );
    }
  }
  
  if( message ) {
    if( page ) {
      fpage( ch, "%s%s.", message, coin_phrase( num ) );
    } else {
      fsend( ch, "%s%s.", message, coin_phrase( num ) );
    }
  }
}


bool remove_coins( char_data* ch, int amount,
		   const char *message, bool page ) 
{
  obj_data*       obj;
  int             pos  [ MAX_COIN ];
  int             neg  [ MAX_COIN ];

  // It's free.
  if( amount <= 0 )
    return true;

  int coins [ MAX_COIN ];
  vzero( coins, MAX_COIN );

  // Count up how many coins of each type char has.
  int money = 0;
  obj_array coin_obj [ MAX_COIN ];
  for( int i = 0; i < ch->contents; ++i ) {
    obj = (obj_data*) ch->contents[i];
    for( int j = 0; j < MAX_COIN; ++j ) 
      if( obj->pIndexData->vnum == coin_vnum[j] ) {
        coin_obj[j] += obj; 
        coins[j] += obj->Number( );
	money += obj->Number( ) * coin_value[j];
      }
  }

  if( amount > money )
    return false;

  int number [ MAX_COIN ];
  vzero( number, MAX_COIN );
  
  for( int i = MAX_COIN-1; i >= 0; --i ) {
    int num = min( coins[i], ( money - amount ) / coin_value[i] );
    number[i] = coins[i] - num;
    money -= num * coin_value[i];
  }

  // How much change to give?
  amount = money - amount;

  bool flag  = false;	// Is change being given?

  for( int i = MAX_COIN-1; i >= 0; --i ) {
    int num = amount / coin_value[i];
    amount -= num * coin_value[i];
    number[i] -= num;      // negative number[] indicates change to be given.

    neg[i] = pos[i] = 0;
    if( number[i] > 0 ) {
      pos[i] = number[i];
      for( int j = 0; number[i] > 0 && j < coin_obj[i]; ++j ) {
	num = min( number[i], coin_obj[i][j]->Number( ) );
	coin_obj[i][j]->Extract( num );
	number[i] -= num;
      }
    } else if ( number[i] < 0 ) {
      neg[i] = -number[i];
      obj = create( get_obj_index( coin_vnum[i] ), neg[i] );
      obj->To( ch );
      flag = true;
    }
  }
    
  /*
  // First pass: try paying with just coppers.
  // Second pass: try paying with silvers and coppers, in that order.
  // Third pass: try paying with golds, silvers and coppers.
  // Fourth pass: try paying with all possible coins.
  int i = 0;
  int save_amount = amount;
  for( int j = 0; j < MAX_COIN && amount > 0; ++j ) {
    amount = save_amount;
    vzero( number, MAX_COIN );   
    for( i = j; i >= 0 && amount > 0; i-- ) {
      if ( amount <= coins[i]*coin_value[i] ) { // coins of type i suffice
	number[i] = amount/coin_value[i];
	if ( amount % coin_value[i] != 0 ) {   // need to overshoot
	  ++number[i];
	}
	amount -= number[i]*coin_value[i]; // may go negative if need change
      }
      else { // coins of type i not enough, go down to cheaper coin type
	amount -= coins[i]*coin_value[i];
	number[i] = coins[i];
      }
    }
  }
  
  if( amount > 0 )  // player didn't have enough money
    return false;

  amount = -amount;         // amount is now amount of change to give

  // i currently set to highest coin type that can be given out as change
  for( ; i >= 0; i-- ) {
    int dum = amount/coin_value[i];
    amount -= dum*coin_value[i];
    number[i] -= dum;      // negative number[] indicates change to be given
  }
    
  // Done populating number array.  Extract spent coinage, create change coinage.
  for( int i = MAX_COIN-1; i >= 0; --i ) {
    neg[i] = pos[i] = 0;
    if( number[i] > 0 ) {
      pos[i] = number[i];
      for( int j = 0; number[i] > 0 && j < coin_obj[i]; ++j ) {
	int num = min( number[i], coin_obj[i][j]->number );
	coin_obj[i][j]->Extract( num );
	number[i] -= num;
      }
      //      coin_obj[i]->Extract( number[i] );
    } else if ( number[i] < 0 ) {
      neg[i] = -number[i];
      obj = create( get_obj_index( coin_vnum[i] ), neg[i] );
      obj->To( ch );
      flag = true;
    }
  }
  */
  
  if( message ) {
    if( page ) {
      fpage( ch, "%s%s.", message, coin_phrase( pos ) );
    } else {
      fsend( ch, "%s%s.", message, coin_phrase( pos ) );
    }
    if( flag ) {
      if( page ) {
	fpage( ch, "You receive%s in change.", coin_phrase( neg ) );
      } else {
	fsend( ch, "You receive%s in change.", coin_phrase( neg ) );
      }
    }
  }
  
  return true;
}


void do_split( char_data* ch, const char *argument )
{
  if( !*argument ) {
    send( ch, "What amount do you wish to split?\n\r" );
    return;
  }
  
  int amount = atoi( argument );
  
  if( amount < 2 ) {
    send( ch, "It is difficult to split anything less than 2 cp.\n\r" );
    return;
  }
  
  if( get_money( ch ) < amount ) {
    send( ch, "You don't have enough money to split that amount.\n\r" );
    return;
  }

  split_money( ch, amount, true );
}


void split_money( char_data* ch, int amount, bool msg )
{
  if( amount < 2 )
    return;
  
  char_array group;

  // Split with whom?
  for( int j = 0; j < ch->in_room->contents.size; ++j ) {
    if( player_data *pl = player( ch->in_room->contents[j] ) ) {
      if( pl != ch && is_same_group( pl, ch ) && pl->position >= POS_RESTING ) {
	group += pl;
      }
    }
  }

  if( group.is_empty( ) ) {
    if( msg )
      send( ch, "There is no one here to split the coins with.\n\r" );
    return;
  }

  group += ch;

  int coins_held [MAX_COIN];
  vzero( coins_held, MAX_COIN );

  // Find all coins held by splitter.
  obj_array coin_obj [MAX_COIN];
  for( int k = 0; k < ch->contents; ++k ) {
    if( obj_data* obj = object( ch->contents[k] ) ) {
      for( int i = 0; i < MAX_COIN; ++i ) {
	if( obj->pIndexData->vnum == coin_vnum[i] ) {
	  coin_obj[i] += obj; 
	  coins_held[i] += obj->Number( );
	}
      }
    }
  }
  
  // Find a set of coins that equals the amount to be split.
  // Start with the largest coins.
  int coins_split[MAX_COIN];
  int split = amount;
  for( int i = MAX_COIN-1; i >= 0; --i ) {
    const int num = min( split / coin_value[i], coins_held[i] );
    split -= num * coin_value[i];
    coins_split[i] = num;
    coins_held[i] -= num;
  }

  if( split != 0 ) {
    send( ch, "You lack the correct coins to split that amount.\n\r" );
    return;
  }

  // Exchange larger coins for smaller where possible.
  for( int i = MAX_COIN-1; i > 0; --i ) {
    if( coins_split[i] > 0 ) {
      for ( int j = i-1; j >= 0; --j ) {
	const int exchange = coin_value[i] / coin_value [j];
	const int num = min( coins_held[j] / exchange, coins_split[i] );
	coins_split[i] -= num;
	coins_held[i] += num;
	coins_split[j] += num * exchange;
	coins_held[j] -= num * exchange;
      }
    }
  }

  const int members = group.size;

  int split_amount [ members ];
  vzero( split_amount, members );
  int splits [ members ][ MAX_COIN ];

  for( int j = 0; j < members; ++j ) {
    vzero( splits[j], MAX_COIN );

    // Make a copy of the actual coins remaining.
    int coins_tmp[MAX_COIN];
    vcopy( coins_tmp, coins_split, MAX_COIN );
    int tmp_split = amount;

    for( int i = MAX_COIN-1; i >= 0 && amount > 0; --i ) {
      if( coins_tmp[i] > 0 ) {
	// Take a share of the largest coin type.
	int take = coins_tmp[i] / (members-j);
	splits[j][i] += take;
	coins_split[i] -= take;
	split_amount[j] += take*coin_value[i];
	amount -= take*coin_value[i];
	coins_tmp[i] -= take * (members-j);  // Everyone else will take a share.
	tmp_split -= take * (members-j) * coin_value[i];
	if ( coins_tmp[i] > 0 ) {
	  // There are too few left to split evenly. Take one.
	  ++splits[j][i];
	  --coins_split[i];
	  split_amount[j] += coin_value[i];
	  amount -= coin_value[i];
	  --coins_tmp[i];
	  tmp_split -= coin_value[i];
	  // Has this player taken his full share?
	  if( tmp_split <= (members-j-1)*coin_value[i] ) {
	    i = 0;
	  } else {
	    // Remove the coins the others will take.
	    int shares = (members-j-1)*coin_value[i];
	    for( int k = i; k >= 0 && shares > 0; --k ) {
	      int num = min( coins_tmp[k], shares / coin_value[k] );
	      coins_tmp[k] -= num;
	      shares -= num * coin_value[k];
	    }
	  }
	}
      }
    }
  }

  // Dole it out.
  int j = members-1;
  //  bool anything = false;
  //  obj_array mine;
  for( int x = 0; x < members; ++x ) {
    char_data *gch = group[x];
    const int k = number_range( 0, j );
    if( split_amount[k] > 0 ) {
      const char *phrase = coin_phrase( splits[k] );
      if( gch != ch ) {
	fsend( ch, "You give%s to %s.", phrase, gch );
	fsend( gch, "%s gives%s to you.", ch, phrase );
	fsend_seen( ch, "%s gives%s to %s.", ch, phrase, gch );
      } else {
	fsend( ch, "You keep%s for yourself.", phrase );
	fsend_seen( ch, "%s keeps%s for %sself.", ch, phrase, ch->Him_Her() );
      }
      
      if( gch != ch ) {
	for( int i = 0; i < MAX_COIN; ++i ) {
	  if( splits[k][i] > 0 ) {
	    int m = splits[k][i];
	    for( int n = 0; m > 0 && n < coin_obj[i]; ++n ) {
	      if( coin_obj[i][n] ) {
		//	      obj_data *obj = (obj_data*) coin_obj[i][n]->From( num );
		const int num = min( m, coin_obj[i][n]->Number( ) );
		const bool all = ( num == coin_obj[i][n]->Number( ) );
		coin_obj[i][n]->From( num )->To( gch );
		/*
		  } else {
		  // Prevent non-commutative consolidation.
		  mine += obj;
		  }
		*/
		if( all ) {
		  coin_obj[i][n] = 0;
		}
		m -= num;
	      }
	    }
	  }
	}
      }
      //      anything = true;
    }
    // Fill in the used split.
    if( k != j ) {
      vcopy( splits[k], splits[j], MAX_COIN );
      split_amount[k] = split_amount[j];
    }
    --j;
  }

  /*
  for( int x = 0; x < mine; ++x ) {
    mine[x]->To( ch );
  }
  */

  /*
  if( !anything ) {
    bug( "do_split: failed for char %s", ch );
    send( ch, "You lack the correct coins to split that amount.\n\r" );
  }
  */
}
