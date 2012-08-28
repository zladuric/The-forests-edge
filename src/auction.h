#ifndef tfe_auction_h
#define tfe_auction_h


extern auction_array auction_list;


class auction_data: public thing_data
{
public:
  pfile_data*      seller;
  pfile_data*       buyer;
  int                 bid;
  int               proxy;
  int                time;
  int                slot;
  bool            deleted;

  auction_data( pfile_data *pfile )
    : seller(pfile), buyer(0),
      bid(0), proxy(0), time(50),
      slot(1), deleted(false)
  {
    record_new( sizeof( auction_data ), MEM_AUCTION );
    valid = AUCTION_DATA;
    auction_list += this;
    seller->auction += this;
  }

  virtual ~auction_data( ) {
    record_delete( sizeof( auction_data ), MEM_AUCTION );
    auction_list -= this;
    if( seller )
      seller->auction -= this;
  }

  /*
   *   From visible_data and thing_data:
   */

  virtual int Type ( ) const
  { return AUCTION_DATA; }
  virtual void Look_At ( char_data* )
  { return; }
  virtual const char *Location ( Content_Array* = 0 )
  { return "auction block"; }

  int minimum_bid( ) const {
    if( !buyer && !deleted )
      return bid;
    return max( 21*bid/20, bid+5 );
  }

  void add_time( ) {
    if( time < 30 )
      time = max( 5, time+2 );
  }
};


int   free_balance      ( player_data*, auction_data* = 0 );
void  clear_auction     ( pfile_data* = 0 );
void  auction_message   ( char_data* );
void  auction_update    ( void );


#endif // tfe_auction_h
