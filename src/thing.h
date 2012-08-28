#ifndef tfe_thing_h
#define tfe_thing_h


class thing_data;
class char_data;
class player_data;


class visible_data
{
public:
  visible_data( )
  { }
  
  virtual ~visible_data( )
  { }
  
  virtual int Number( ) const;
  virtual int Selected( ) const;
  virtual int Shown( ) const;

  virtual void Select( int num );
  virtual void Select_All( );
  virtual void Show( int num );

  /* TYPE CASTING */
  
  friend thing_data *thing( visible_data* visible ) {
    return( visible
	    && visible->Type( ) != EXIT_DATA
	    && visible->Type( ) != EXTRA_DATA
	    ? (thing_data*) visible : 0 );
  }
  
  friend char_data *character( visible_data* visible ) {
    return( visible
	    && visible->Type( ) >= CHAR_DATA
	    && visible->Type( ) <= WIZARD_DATA
	    ? (char_data*) visible : 0 );
  }
  
  friend player_data *player( visible_data* visible ) {
    return( visible
	    && visible->Type( ) >= PLAYER_DATA
	    && visible->Type( ) <= WIZARD_DATA
	    ? (player_data*) visible : 0 );
  }
  
  friend mob_data *mob( visible_data* visible ) {
    return( visible
	    && visible->Type( ) == MOB_DATA
	    ? (mob_data*) visible : 0 );
  }
  
  friend wizard_data *wizard( visible_data* visible ) {
    return( visible
	    && visible->Type( ) == WIZARD_DATA
	    ? (wizard_data*) visible : 0 );
  }
  
  friend obj_data *object( visible_data* visible ) {
    return( visible
	    && visible->Type( ) == OBJ_DATA
	    ? (obj_data*) visible : 0 ); 
  }
  
  friend room_data *Room( visible_data* visible ) {
    return( visible
	    && visible->Type( ) == ROOM_DATA
	    ? (room_data*) visible : 0 ); 
  }

  friend exit_data *exit( visible_data* visible ) {
    return( visible
	    && visible->Type( ) == EXIT_DATA
	    ? (exit_data*) visible : 0 ); 
  }
  
  friend extra_data *extra( visible_data* visible ) {
    return( visible
	    && visible->Type( ) == EXTRA_DATA
	    ? (extra_data*) visible : 0 ); 
  }
  
  friend auction_data *Auction( visible_data* visible ) {
    return( visible
	    && visible->Type( ) == AUCTION_DATA
	    ? (auction_data*) visible : 0 ); 
  }
  
  /*
  friend Shop_Data *Shop( visible_data* visible ) {
    return( visible
	    && visible->Type( ) == SHOP_DATA
	    ? (Shop_Data*) visible : 0 ); 
  }
  */

  friend const thing_data *thing( const visible_data* visible ) {
    return( visible
	    && visible->Type( ) != EXIT_DATA
	    && visible->Type( ) != EXTRA_DATA
	    ? (const thing_data*) visible : 0 );
  }
  
  friend const char_data *character( const visible_data* visible ) {
    return( visible
	    && visible->Type( ) >= CHAR_DATA
	    && visible->Type( ) <= WIZARD_DATA
	    ? (const char_data*) visible : 0 );
  }
  
  friend const player_data *player( const visible_data* visible ) {
    return( visible
	    && visible->Type( ) >= PLAYER_DATA
	    && visible->Type( ) <= WIZARD_DATA
	    ? (const player_data*) visible : 0 );
  }
  
  friend const mob_data *mob( const visible_data* visible ) {
    return( visible
	    && visible->Type( ) == MOB_DATA
	    ? (const mob_data*) visible : 0 );
  }
  
  friend const wizard_data *wizard( const visible_data* visible ) {
    return( visible
	    && visible->Type( ) == WIZARD_DATA
	    ? (const wizard_data*) visible : 0 );
  }
  
  friend const obj_data *object( const visible_data* visible ) {
    return( visible
	    && visible->Type( ) == OBJ_DATA
	    ? (const obj_data*) visible : 0 ); 
  }
  
  friend const room_data *Room( const visible_data* visible ) {
    return( visible
	    && visible->Type( ) == ROOM_DATA
	    ? (const room_data*) visible : 0 ); 
  }

  friend const exit_data *exit( const visible_data* visible ) {
    return( visible
	    && visible->Type( ) == EXIT_DATA
	    ? (const exit_data*) visible : 0 ); 
  }
  
  friend const extra_data *extra( const visible_data* visible ) {
    return( visible
	    && visible->Type( ) == EXTRA_DATA
	    ? (const extra_data*) visible : 0 ); 
  }
  
  friend const auction_data *Auction( const visible_data* visible ) {
    return( visible
	    && visible->Type( ) == AUCTION_DATA
	    ? (const auction_data*) visible : 0 ); 
  }
  
  /*
  friend const Shop_Data *Shop( const visible_data* visible ) {
    return( visible
	    && visible->Type( ) == SHOP_DATA
	    ? (const Shop_Data*) visible : 0 ); 
  }
  */
  
  /* BASE */

  virtual int   Type    ( ) const = 0;

  /* NAME */

  virtual const char* Name       ( const char_data* = 0, int = 1, bool = false ) const;
  virtual const char* Keywords   ( char_data* );
  virtual bool        Seen       ( const char_data* ) const
  { return true; }
  virtual void        Look_At    ( char_data* ) = 0;
};


/* 
 *   OBJECT ARRAYS
 */


#include "array.h"


//class thing_data;


//typedef Array<thing_data*> thing_array;

/*
class thing_array : public Array<thing_data*>
{
public:
  thing_array& operator = ( const thing_array& other ) {
    Array<thing_data*>::operator=( other );
    return *this;
  }
};
*/


class Content_Array : public thing_array
{
public:
  int          weight;
  int          number;
  int           light;
  thing_data*   where;

  Content_Array( thing_data *thing = 0 )
    : weight(0), number(0), light(0), where(thing)
  { }
  virtual ~Content_Array( )
  { }

  void To( Content_Array& );
};


class thing_data : public visible_data
{
public:
  Content_Array   contents;
  affect_array    affected;
  event_array       events;
  int                 temp;
  Content_Array*     array;
  int                valid;
   
  thing_data( )
    : contents( this ), temp(0), array(0), valid(0)
  { }

  virtual ~thing_data( )
  {
    if( array ) {
      bug( "thing_data::~( ) called with  non-null location", this );
    }
    if( !contents.is_empty( ) ) {
      bug( "thing_data::~( ) called with non-empty contents", this );
    }
    if( !affected.is_empty( ) ) {
      bug( "thing_data::~( ) called with non-empty affects", this );
    }
    if( !events.is_empty( ) ) {
      bug( "thing_data::~( ) called with non-empty events", this );
    }
  }

  /* BASE */

  virtual void  Extract ( );

  /* NAME */

  virtual const char* Seen_Name  ( const char_data* = 0, int = 1, bool = false ) const;
  virtual const char* Show_To    ( char_data* );
  virtual const char* Location   ( Content_Array* = 0 );

  /* PROPERTIES */

  virtual bool         Is_Valid        ( ) const
  { return valid > 0; }
  virtual bool         In_Game         ( ) const
  { return true; }
  virtual int          Count           ( int = -1 ) const;
  virtual int          Light           ( int = -1 ) const;
  virtual int          Weight          ( int = -1 );
  virtual int          Empty_Weight    ( int = -1 );
  virtual int          Capacity        ( );
  virtual int          Empty_Capacity  ( ) const;
  virtual void         To              ( Content_Array& );
  virtual void         To              ( thing_data* = 0 );
  virtual thing_data*  From            ( int = 1, bool = false );
};


extern thing_array extracted;


/*
 *   BASIC FUNCTIONS
 */


void         extract            ( thing_array& );
int          select             ( thing_array& );
int          select             ( thing_array&, char_data* );
text&        list_name          ( char_data*, thing_array*, const char* = "and", bool = true );
void         remove_weight      ( thing_data*, int );
bool         none_shown         ( thing_array& );
thing_data*  one_shown          ( thing_array& );
void         show_contents      ( char_data*, thing_data* );


/*
 *   FINDING THINGS
 */


#define va visible_array
#define vd visible_data
#define ta thing_array
#define td thing_data
#define cd char_data
#define md mob_data
#define od obj_data
#define pd player_data
#define ed exit_data
#define rd room_data
#define cc const char

vd* one_visible        ( cd*, cc*, cc*,
			 va*,     int = -1,
			 va* = 0, int = -1,
			 va* = 0, int = -1,
			 va* = 0, int = -1,
			 va* = 0, int = -1,
			 bool = false );
va* several_visible    ( cd*, cc*, cc*,
			 va*,
			 va* = 0,
			 va* = 0,
			 va* = 0 );

td* one_thing          ( cd*, cc*, cc*,
			 ta*, ta* = 0, ta* = 0, int = THING_DATA, bool = false );
ta* several_things     ( cd*, cc*, cc*, ta*, ta* = 0, ta* = 0 );

cd* one_character      ( cd*, cc*, cc*, ta*, ta* = 0, ta* = 0, bool = false );
md* one_mob            ( cd*, cc*, cc*, ta*, ta* = 0, ta* = 0 );
pd* one_player         ( cd*, cc*, cc*, ta*, ta* = 0, ta* = 0 );
od* one_object         ( cd*, cc*, cc*, ta*, ta* = 0, ta* = 0 );
ed* one_exit           ( cd*, cc*, cc*, rd* );

#undef va
#undef vd
#undef ta
#undef td
#undef cd
#undef md
#undef od
#undef pd
#undef cc


bool standard_delay( char_data*, const thing_array&, const obj_data* = 0 );


/*
 *   PROPERTIES
 */


/*
inline const char *location( Content_Array *array )
{
  if( !array || !array->where )
    return "Nowhere?";
 
  return array->where->Location( array );
}
*/


/*
inline char_data* carried_by( thing_data* thing )
{
  while( thing->array ) {
    thing = thing->array->where;
    if( character( thing ) )
      return (char_data*) thing;
  }
  
  return 0;
}
*/


#endif // tfe_thing_h
