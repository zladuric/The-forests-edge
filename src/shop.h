#ifndef tfe_shop_h
#define tfe_shop_h


/*
 *   SHOPS
 */


class Shop_Data//: public thing_data
{
public:
  shop_data*        next;        
  room_data*        room;
  custom_data*    custom; 
  int             keeper;
  int          open_hour;       
  int         close_hour;      
  int             repair; 
  int              flags;
  int           buy_type  [ 2 ];
  int          materials;

  Shop_Data( )
    : next(0), custom(0)
  {
    record_new( sizeof( shop_data ), MEM_SHOP );
    //    valid = SHOP_DATA;
  }
  
  ~Shop_Data( )
  {
    record_delete( sizeof( shop_data ), MEM_SHOP );
    delete_list( custom );
  }

  /*
  virtual int Type ( ) const
  { return SHOP_DATA; }
  virtual void Look_At ( char_data* )
  { return; }
  virtual const char *Location ( Content_Array* = 0 )
  { return "shop"; }
  */
};


extern shop_data *shop_list;


void          load_shops     ( void );
void          save_shops     ( void );
mob_data     *active_shop    ( char_data* );
void          shop_update    ( void );


#define SHOP_STOLEN  0
#define SHOP_NOMELT  1
#define MAX_SHOP     2


/*
 *   CUSTOMS
 */


class Custom_Data
{
public:
  custom_data*      next;
  int               cost;
  obj_clss_data*    item;
  obj_clss_data*  ingred  [ MAX_INGRED ];
  int             number  [ MAX_INGRED ];
  
  Custom_Data( )
    : next(0), cost(100), item(0)
  {
    record_new( sizeof( custom_data ), MEM_CUSTOM );
    vzero( ingred, MAX_INGRED );
    vzero( number, MAX_INGRED );
  }
  
  ~Custom_Data( )
  {
    record_delete( sizeof( custom_data ), MEM_CUSTOM );
  }
};


const char *item_name( const custom_data*, bool = true );


#endif // tfe_shop_h
