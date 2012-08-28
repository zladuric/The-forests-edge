#ifndef tfe_output_h
#define tfe_output_h


extern const char *const scroll_line [ 3 ];


/*
 *   ACT ROUTINES
 */

extern int act_color;


#define cc const char
#define cd char_data
#define vd visible_data
#define rd room_data
#define od obj_data
#define ed exit_data

void  act_print     ( char*, cc*, cd*, cd*, od*, od*, cc*, cc*, ed*, cd*, bool = true );

void  act           ( cd*, cc*, cd*, cd* = 0, od* = 0, od* = 0, ed* = 0, cc* = 0, cc* = 0 );
void  act           ( cd*, cc*, cd*, od*, cc*, cc* = 0 ); 

void  act_room      ( rd*, cc*, cd*, cd* = 0, od* = 0, od* = 0, ed* = 0, cc* = 0, cc* = 0 );
void  act_area      ( cc*, cd*, cd*, od*, od* = 0, cc* = 0, cc* = 0 );

void  act_notchar   ( cc*, cd*, cd* = 0, od* = 0, od* = 0, ed* = 0, cc* = 0, cc* = 0 );
void  act_notchar   ( cc*, cd*, od*, cc*, cc* = 0 );
void  act_neither   ( cc*, cd*, cd*, od*, od* = 0, cc* = 0, cc* = 0 );
void  act_notvict   ( cc*, cd*, cd*, od*, od* = 0, cc* = 0, cc* = 0 );
void  act_seen      ( cc*, cd*, cd* = 0, od* = 0, od* = 0, ed* = 0, cc* = 0, cc* = 0 );

void act_social     ( cd*, cc*, cd*, cd* = 0, od* = 0, od* = 0, ed* = 0 );
void act_social     ( cd*, cc*, cd*, cc*, cc* = 0 );

void act_social_room( cc*, cd*, cd* = 0, od* = 0, od* = 0, ed* = 0 );
void act_social_room( cc*, cd*, cc*, cc* = 0 );

#undef cc
#undef cd
#undef vd
#undef rd
#undef od
#undef ed


/*
 *   GENERIC DISPLAY ROUTINES
 */


void  display_array  ( char_data*, const char*,
		       const char *const*, const char *const*,
		       int, bool = true,
		       bool (*)(const char_data*, int) = 0 );

 
/*
 *   STRING CONVERSION ROUTINES
 */


inline const char *tostring( const visible_data *visible, char_data* ch )
{
  return visible->Name( ch, visible->Shown( ) );
}


inline const char *tostring( const species_data *species, char_data* )
{
  return species->Name( );
}


inline const char *tostring( const obj_clss_data *obj_clss, char_data* )
{
  return obj_clss->Name( );
}


inline const char *tostring( thing_array *array, char_data* ch )
{  
  return list_name( ch, array ).c_str();
}


inline const char *tostring( const text& a, char_data *ch )
{
  return a.c_str();
}


inline char          tostring  ( char a, char_data* ch )          { return a; }
inline int           tostring  ( int a, char_data* ch )           { return a; }
inline unsigned      tostring  ( unsigned a, char_data *ch )      { return a; }
inline long          tostring  ( long a, char_data *ch )          { return a; }
inline unsigned long tostring  ( unsigned long a, char_data *ch ) { return a; }
inline long long     tostring  ( long long a, char_data *ch )     { return a; }
inline long long     tostring  ( unsigned long long a, char_data *ch )     { return a; }
inline double        tostring  ( double a, char_data *ch )        { return a; }
inline const char   *tostring  ( const char *a, char_data *ch )   { return a; }


void clear_send_buffers( );


//inline const char *buffer( int n, const text& a )                 { return a.c_str(); }

const char *buffer( int n, species_data *species );
const char *buffer( int n, obj_clss_data* obj_clss );
const char *buffer( int n, const char *a );


inline const visible_data *buffer( int n, const visible_data *visible )  { return visible; }
inline thing_array *buffer( int n, thing_array *array )      { return array; }
inline int buffer( int n, int a )                                 { return a; }
inline unsigned buffer( int n, unsigned a )                       { return a; }
inline long buffer( int n, long a )                               { return a; }
inline unsigned long buffer( int n, unsigned long a )             { return a; }
inline double buffer( int n, double a )                           { return a; }


#define OUT_BUF_LEN	(4*MAX_STRING_LENGTH)
extern char out_buf [ OUT_BUF_LEN ];


/*
 *   ECHO
 */


void   echo                 ( const char* );


template < class T >
void echo( const char* text, T item )
{
  snprintf( out_buf, OUT_BUF_LEN, text, tostring( item, 0 ) );
  echo( out_buf );
}


/*
 *   PAGE
 */


void    page              ( char_data*, const char* );
void    page_centered     ( char_data*, const char* );
void    page_underlined   ( char_data*, const char* );
void    next_page         ( link_data* );


inline void page( char_data* ch, const text& t )
{
  page( ch, t.c_str( ) );
}


template < class T >
void page( char_data* ch, const char* text,
	   T item )
{
  if( ch->link ) {
    snprintf( out_buf, OUT_BUF_LEN, text, tostring( item, ch ) );
    if( *text == '%' && !isdigit( *( text+1 ) ) && *(text+1) != '-' )
      capitalize( out_buf );
    page( ch, out_buf );
  }
}


template < class S, class T >
void page( char_data* ch, const char* text,
	   S item1, T item2 )
{
  if( ch->link ) {
    snprintf( out_buf, OUT_BUF_LEN, text,
	      tostring( item1, ch ),
	      tostring( item2, ch ) );
    if( *text == '%' && !isdigit( *( text+1 ) ) && *(text+1) != '-' )
      capitalize( out_buf );
    page( ch, out_buf );
  }
}


template < class S, class T, class U >
void page( char_data* ch, const char* text,
	   S item1, T item2, U item3 )
{
  if( ch->link ) {
    snprintf( out_buf, OUT_BUF_LEN, text,
	      tostring( item1, ch ),
	      tostring( item2, ch ),
	      tostring( item3, ch ) );
    if( *text == '%' && !isdigit( *( text+1 ) ) && *(text+1) != '-' )
      capitalize( out_buf );
    page( ch, out_buf );
  }
}


template < class S, class T, class U, class V >
void page( char_data* ch, const char* text,
	   S item1, T item2, U item3, V item4 )
{
  if( ch->link ) {
    snprintf( out_buf, OUT_BUF_LEN, text,
	      tostring( item1, ch ), tostring( item2, ch ),
	      tostring( item3, ch ), tostring( item4, ch ) );
    if( *text == '%' && !isdigit( *( text+1 ) ) && *(text+1) != '-' )
      capitalize( out_buf );
    page( ch, out_buf );
  }
}


template < class S, class T, class U, class V, class W >
void page( char_data* ch, const char* text,
	   S item1, T item2, U item3, V item4, W item5 )
{
  if( ch->link ) {
    snprintf( out_buf, OUT_BUF_LEN, text,
	      tostring( item1, ch ), tostring( item2, ch ),
	      tostring( item3, ch ), tostring( item4, ch ),
	      tostring( item5, ch ) );
    if( *text == '%' && !isdigit( *( text+1 ) ) && *(text+1) != '-' )
      capitalize( out_buf );
    page( ch, out_buf );
  }
}


template < class S, class T, class U, class V, class W, class X >
void page( char_data* ch, const char* text,
	   S item1, T item2, U item3, V item4, W item5, X item6 )
{
  if( ch->link ) {
    snprintf( out_buf, OUT_BUF_LEN, text,
	      tostring( item1, ch ), tostring( item2, ch ),
	      tostring( item3, ch ), tostring( item4, ch ),
	      tostring( item5, ch ), tostring( item6, ch ) );
    if( *text == '%' && !isdigit( *( text+1 ) ) && *(text+1) != '-' )
      capitalize( out_buf );
    page( ch, out_buf );
  }
}


template < class S, class T, class U, class V, class W, class X, class Y >
void page( char_data* ch, const char* text,
	   S item1, T item2, U item3, V item4, W item5, X item6, Y item7 )
{
  if( ch->link ) {
    snprintf( out_buf, OUT_BUF_LEN, text,
	      tostring( item1, ch ), tostring( item2, ch ),
	      tostring( item3, ch ), tostring( item4, ch ),
	      tostring( item5, ch ), tostring( item6, ch ),
	      tostring( item7, ch ) );
    if( *text == '%' && !isdigit( *( text+1 ) ) && *(text+1) != '-' )
      capitalize( out_buf );
    page( ch, out_buf );
  }
}


void page_header( char_data* ch, const char* text );


template < class T >
void page_header( char_data* ch, const char* text,
		  T item )
{
  if( ch && ch->link ) {
    snprintf( out_buf, OUT_BUF_LEN, text, tostring( item, ch ) );
    if( *text == '%' && !isdigit( *( text+1 ) ) && *(text+1) != '-' )
      capitalize( out_buf );
    page_header( ch, out_buf );
  }
}


template < class S, class T >
void page_header( char_data* ch, const char* text,
		  S item1, T item2 )
{
  if( ch && ch->link ) {
    snprintf( out_buf, OUT_BUF_LEN, text,
	      tostring( item1, ch ), tostring( item2, ch ) );
    if( *text == '%' && !isdigit( *( text+1 ) ) && *(text+1) != '-' )
      capitalize( out_buf );
    page_header( ch, out_buf );
  }
}




template < class S, class T, class U >
void page_header( char_data* ch, const char* text,
		  S item1, T item2, U item3 )
{
  if( ch && ch->link ) {
    snprintf( out_buf, OUT_BUF_LEN, text,
	      tostring( item1, ch ), tostring( item2, ch ),
	      tostring( item3, ch ) );
    if( *text == '%' && !isdigit( *( text+1 ) ) && *(text+1) != '-' )
      capitalize( out_buf );
    page_header( ch, out_buf );
  }
}


/*
 *   PAGE_UNDERLINED
 */


template < class T >
void page_underlined( char_data* ch, const char* text,
		      T item )
{
  if( ch->link ) {
    snprintf( out_buf, OUT_BUF_LEN, text,
	      tostring( item, ch ) );
    page_underlined( ch, out_buf );
  }
}


template < class S, class T >
void page_underlined( char_data* ch, const char* text,
		      S item1, T item2 )
{
  if( ch->link ) {
    snprintf( out_buf, OUT_BUF_LEN, text,
	      tostring( item1, ch ),
	      tostring( item2, ch ) );
    page_underlined( ch, out_buf );
  }
}


template < class S, class T, class U >
void page_underlined( char_data* ch, const char* text,
		      S item1, T item2, U item3 )
{
  if( ch->link ) {
    snprintf( out_buf, OUT_BUF_LEN, text,
	      tostring( item1, ch ),
	      tostring( item2, ch ),
	      tostring( item3, ch ) );
    page_underlined( ch, out_buf );
  }
}


template < class S, class T, class U, class V >
void page_underlined( char_data* ch, const char* text,
		      S item1, T item2, U item3, V item4 )
{
  if( ch->link ) {
    snprintf( out_buf, OUT_BUF_LEN, text,
	      tostring( item1, ch ),
	      tostring( item2, ch ),
	      tostring( item3, ch ),
	      tostring( item4, ch ) );
    page_underlined( ch, out_buf );
  }
}


template < class S, class T, class U, class V, class W >
void page_underlined( char_data* ch, const char* text,
		      S item1, T item2, U item3, V item4, W item5 )
{
  if( ch->link ) {
    snprintf( out_buf, OUT_BUF_LEN, text,
	      tostring( item1, ch ),
	      tostring( item2, ch ),
	      tostring( item3, ch ),
	      tostring( item4, ch ),
	      tostring( item5, ch ) );
    page_underlined( ch, out_buf );
  }
}


/*
 *   PAGE_CENTERED
 */


template < class S >
void page_centered( char_data* ch, const char* text,
		    S item )
{
  snprintf( out_buf, OUT_BUF_LEN, text,
	    tostring( item, ch ) );
  page_centered( ch, out_buf );
}


template < class S, class T >
void page_centered( char_data* ch, const char* text,
		    S item1, T item2 )
{
  snprintf( out_buf, OUT_BUF_LEN, text,
	    tostring( item1, ch ),
	    tostring( item2, ch ) );
  page_centered( ch, out_buf );
}


template < class S, class T, class U >
void page_centered( char_data* ch, const char* text,
		    S item1, T item2, U item3 )
{
  snprintf( out_buf, OUT_BUF_LEN, text,
	    tostring( item1, ch ),
	    tostring( item2, ch ),
	    tostring( item3, ch ) );
  page_centered( ch, out_buf );
}


/*
 *   PAGE_DIVIDER
 */


void page_divider( char_data* ch, const char* text, int i );


/*
 *   FORMATTED PAGE
 */


void  fpage         ( char_data*, const char* );
void  ipage         ( char_data*, const char*, int );


template < class T >
void fpage( char_data* ch, const char* text,
	    T item )
{
  if( ch->link ) {
    snprintf( out_buf, OUT_BUF_LEN, text,
	      tostring( item, ch ) );
    if( *text == '%' && !isdigit( *( text+1 ) ) && *(text+1) != '-' )
      capitalize( out_buf );
    fpage( ch, out_buf );
  }
}


template < class S, class T >
void fpage( char_data* ch, const char* text,
	    S item1, T item2 )
{
  if( ch->link ) {
    snprintf( out_buf, OUT_BUF_LEN, text,
	      tostring( item1, ch ),
	      tostring( item2, ch ) );
    if( *text == '%' && !isdigit( *( text+1 ) ) && *(text+1) != '-' )
      capitalize( out_buf );
    fpage( ch, out_buf );
  }
}


template < class S, class T, class U >
void fpage( char_data* ch, const char* text,
	    S item1, T item2, U item3 )
{
  if( ch->link ) {
    snprintf( out_buf, OUT_BUF_LEN, text,
	      tostring( item1, ch ),
	      tostring( item2, ch ),
	      tostring( item3, ch ) );
    if( *text == '%' && !isdigit( *( text+1 ) ) && *(text+1) != '-' )
      capitalize( out_buf );
    fpage( ch, out_buf );
  }
}


template < class S, class T, class U, class V >
void fpage( char_data* ch, const char* text,
	    S item1, T item2, U item3, V item4 )
{
  if( ch->link ) {
    snprintf( out_buf, OUT_BUF_LEN, text,
	      tostring( item1, ch ),
	      tostring( item2, ch ),
	      tostring( item3, ch ),
	      tostring( item4, ch ) );
    if( *text == '%' && !isdigit( *( text+1 ) ) && *(text+1) != '-' )
      capitalize( out_buf );
    fpage( ch, out_buf );
  }
}


template < class S, class T, class U, class V, class X >
void fpage( char_data* ch, const char* text,
	    S item1, T item2, U item3, V item4, X item5 )
{
  if( ch->link ) {
    snprintf( out_buf, OUT_BUF_LEN, text,
	      tostring( item1, ch ),
	      tostring( item2, ch ),
	      tostring( item3, ch ),
	      tostring( item4, ch ),
	      tostring( item5, ch ) );
    if( *text == '%' && !isdigit( *( text+1 ) ) && *(text+1) != '-' )
      capitalize( out_buf );
    fpage( ch, out_buf );
  }
}


template < class S, class T, class U, class V, class X, class Y >
void fpage( char_data* ch, const char* text,
	    S item1, T item2, U item3, V item4, X item5, Y item6 )
{
  if( ch->link ) {
    snprintf( out_buf, OUT_BUF_LEN, text,
	      tostring( item1, ch ),
	      tostring( item2, ch ),
	      tostring( item3, ch ),
	      tostring( item4, ch ),
	      tostring( item5, ch ),
	      tostring( item6, ch ) );
    if( *text == '%' && !isdigit( *( text+1 ) ) && *(text+1) != '-' )
      capitalize( out_buf );
    fpage( ch, out_buf );
  }
}


/*
 *   SEND TO LINK
 */


void send ( link_data*, const char* );


template < class T >
void send( link_data* link, const char* text,
	   T item )
{
  snprintf( out_buf, OUT_BUF_LEN, text,
	    tostring( item, 0 ) );
  send( link, *text == '%' && !isdigit( *( text+1 ) ) && *(text+1) != '-' ? capitalize( out_buf ) : out_buf );
}


template < class S, class T, class U >
void send( link_data* link, const char* text,
	   S item1, T item2, U item3 )
{
  snprintf( out_buf, OUT_BUF_LEN, text,
	    tostring( item1, 0 ),
	    tostring( item2, 0 ),
	    tostring( item3, 0 ) );
  send( link, *text == '%' && !isdigit( *( text+1 ) ) && *(text+1) != '-' ? capitalize( out_buf ) : out_buf );
}


template < class S, class T, class U, class V >
void send( link_data* link, const char* text,
	   S item1, T item2, U item3, V item4 )
{
  snprintf( out_buf, OUT_BUF_LEN, text,
	    tostring( item1, 0 ),
	    tostring( item2, 0 ),
	    tostring( item3, 0 ),
	    tostring( item4, 0 ) );
  send( link, *text == '%' && !isdigit( *( text+1 ) ) && *(text+1) != '-' ? capitalize( out_buf ) : out_buf );
}


/*
 *   SEND TO CHARACTER
 */


inline void send( const char_data *ch, const char *text )
{
  if( ch && ch->link ) {
    send( ch->link, text );
  }
}


template < class T >
void send( char_data* ch, const char* text,
	   T item )
{
  if( ch && ch->link ) {
    snprintf( out_buf, OUT_BUF_LEN, text,
	      tostring( item, ch ) );
    send( ch, *text == '%' && !isdigit( *( text+1 ) ) && *(text+1) != '-' ? capitalize( out_buf ) : out_buf );
  }
}


template < class S, class T >
void send( char_data* ch, const char* text,
	   S item1, T item2 )
{
  if( ch && ch->link ) {
    snprintf( out_buf, OUT_BUF_LEN, text,
	      tostring( item1, ch ),
	      tostring( item2, ch ) );
    send( ch, *text == '%' && !isdigit( *( text+1 ) ) && *(text+1) != '-' ? capitalize( out_buf ) : out_buf );
  }
}


template < class S, class T, class U >
void send( char_data* ch, const char* text,
	   S item1, T item2, U item3 )
{
  if( ch->link ) {
    snprintf( out_buf, OUT_BUF_LEN, text,
	      tostring( item1, ch ),
	      tostring( item2, ch ),
	      tostring( item3, ch ) );
    send( ch, *text == '%' && !isdigit( *( text+1 ) ) && *(text+1) != '-' ? capitalize( out_buf ) : out_buf );
  }
}


template < class S, class T, class U, class V >
void send( char_data* ch, const char* text,
	   S item1, T item2, U item3, V item4 )
{
  if( ch && ch->link ) {
    snprintf( out_buf, OUT_BUF_LEN, text,
	      tostring( item1, ch ),
	      tostring( item2, ch ),
	      tostring( item3, ch ),
	      tostring( item4, ch ) );
    send( ch, *text == '%' && !isdigit( *( text+1 ) ) && *(text+1) != '-' ? capitalize( out_buf ) : out_buf );
  }
}


template < class S, class T, class U, class V, class W >
void send( char_data* ch, const char* text,
	   S item1, T item2, U item3, V item4, W item5 )
{
  if( ch && ch->link ) {
    snprintf( out_buf, OUT_BUF_LEN, text,
	      tostring( item1, ch ),
	      tostring( item2, ch ),
	      tostring( item3, ch ),
	      tostring( item4, ch ),
	      tostring( item5, ch ) );
    send( ch, *text == '%' && !isdigit( *( text+1 ) ) && *(text+1) != '-' ? capitalize( out_buf ) : out_buf );
  }
}


template < class S, class T, class U, class V, class W, class X >
void send( char_data* ch, const char* text,
	   S item1, T item2, U item3, V item4, W item5, X item6 )
{
  if( ch && ch->link ) {
    snprintf( out_buf, OUT_BUF_LEN, text,
	      tostring( item1, ch ),
	      tostring( item2, ch ),
	      tostring( item3, ch ),
	      tostring( item4, ch ),
	      tostring( item5, ch ),
	      tostring( item6, ch ) );
    send( ch, *text == '%' && !isdigit( *( text+1 ) ) && *(text+1) != '-' ? capitalize( out_buf ) : out_buf );
  }
}


template < class S, class T, class U, class V, class W, class X, class Y >
void send( char_data* ch, const char* text,
	   S item1, T item2, U item3, V item4, W item5, X item6, Y item7 )
{
  if( ch && ch->link ) {
    snprintf( out_buf, OUT_BUF_LEN, text,
	      tostring( item1, ch ),
	      tostring( item2, ch ),
	      tostring( item3, ch ),
	      tostring( item4, ch ),
	      tostring( item5, ch ),
	      tostring( item6, ch ),
	      tostring( item7, ch ) );
    send( ch, *text == '%' && !isdigit( *( text+1 ) ) && *(text+1) != '-' ? capitalize( out_buf ) : out_buf );
  }
}


template < class S, class T, class U, class V, class W, class X, class Y, class Z >
void send( char_data* ch, const char* text,
	   S item1, T item2, U item3, V item4, W item5, X item6, Y item7, Z item8 )
{
  if( ch && ch->link ) {
    snprintf( out_buf, OUT_BUF_LEN, text,
	      tostring( item1, ch ),
	      tostring( item2, ch ),
	      tostring( item3, ch ),
	      tostring( item4, ch ),
	      tostring( item5, ch ),
	      tostring( item6, ch ),
	      tostring( item7, ch ),
	      tostring( item8, ch ) );
    send( ch, *text == '%' && !isdigit( *( text+1 ) ) && *(text+1) != '-' ? capitalize( out_buf ) : out_buf );
  }
}


/*
 *   SEND_SEEN
 */


void send_seen( char_data* ch, const char* text );


template < class T >
void send_seen( char_data* ch, const char* text,
		T item )
{
  if( ch && ch->array && !ch->was_in_room ) {
    clear_send_buffers( );
    for( int i = 0; i < *ch->array; ++i ) {
      char_data *rch = character( ch->array->list[i] );
      if( rch
	  && rch != ch
	  && ch->Seen( rch )
	  && rch != (char_data*) item )
        send( rch, text,
	      buffer( 0, item ) );
    }
  }
}


template < class S, class T >
void send_seen( char_data* ch, const char* text,
		S item1, T item2 )
{
  if( ch && ch->array && !ch->was_in_room ) {
    clear_send_buffers( );
    for( int i = 0; i < *ch->array; ++i ) {
      char_data *rch = character( ch->array->list[i] );
      if( rch
	  && rch != ch
	  && ch->Seen( rch )
	  && rch != (char_data*) item1
	  && rch != (char_data*) item2 )
        send( rch, text,
	      buffer( 0, item1 ),
	      buffer( 1, item2 ) );
    }
  }
}


template < class S, class T, class U >
void send_seen( char_data* ch, const char* text,
		S item1, T item2, U item3 )
{
  if( ch && ch->array && !ch->was_in_room ) {
    clear_send_buffers( );
    for( int i = 0; i < *ch->array; ++i ) {
      char_data *rch = character( ch->array->list[i] );
      if( rch
	  && rch != ch
	  && ch->Seen( rch )
	  && rch != (char_data*) item1
	  && rch != (char_data*) item2
	  && rch != (char_data*) item3 )
	send( rch, text,
	      buffer( 0, item1 ),
	      buffer( 1, item2 ),
	      buffer( 2, item3 ) );
    }
  }
}


template < class S, class T, class U, class V >
void send_seen( char_data* ch, const char* text,
		S item1, T item2, U item3, V item4 )
{
  if( ch && ch->array && !ch->was_in_room ) {
    clear_send_buffers( );
    for( int i = 0; i < *ch->array; ++i ) {
      char_data *rch = character( ch->array->list[i] );
      if( rch
	  && rch != ch
	  && ch->Seen( rch )
	  && rch != (char_data*) item1
	  && rch != (char_data*) item2
	  && rch != (char_data*) item3
	  && rch != (char_data*) item4 )
	send( rch, text,
	      buffer( 0, item1 ),
	      buffer( 1, item2 ),
	      buffer( 2, item3 ),
	      buffer( 3, item4 ) );
    }
  }
}


/*
 *   SEND_ROOM
 */


void send( thing_array& array, const char* text );


template < class T >
void send( thing_array& array, const char* text,
	   T item )
{
  clear_send_buffers( );
  for( int i = 0; i < array; ++i ) {
    char_data *rch = character( array[i] );
    if( rch
	&& rch->Can_See( )
	&& rch != (char_data*) item )
      send( rch, text,
	    buffer( 0, item ) );
  }
}


template < class S, class T >
void send( thing_array& array, const char* text,
	   S item1, T item2 )
{
  clear_send_buffers( );
  for( int i = 0; i < array; ++i ) {
    char_data *rch = character( array[i] );
    if( rch
	&& rch->Can_See( )
	&& rch != (char_data*) item1
	&& rch != (char_data*) item2 )
      send( rch, text,
	    buffer( 0, item1 ),
	    buffer( 1, item2 ) );
  }
}


template < class S, class T, class U >
void send( thing_array& array, const char* text,
	   S item1, T item2, U item3 )
{
  clear_send_buffers( );
  for( int i = 0; i < array; ++i ) {
    char_data *rch = character( array[i] );
    if( rch
	&& rch->Can_See( )
	&& rch != (char_data*) item1
	&& rch != (char_data*) item2
	&& rch != (char_data*) item3 )
      send( rch, text,
	    buffer( 0, item1 ),
	    buffer( 1, item2 ),
	    buffer( 2, item3 ) );
  }
}


template < class S, class T, class U, class V  >
void send( thing_array& array, const char* text,
	   S item1, T item2, U item3, V item4 )
{
  clear_send_buffers( );
  for( int i = 0; i < array; ++i ) {
    char_data *rch = character( array[i] );
    if( rch
	&& rch->Can_See( )
	&& rch != (char_data*) item1
	&& rch != (char_data*) item2
	&& rch != (char_data*) item3
	&& rch != (char_data*) item4 )
      send( rch, text,
	    buffer( 0, item1 ),
	    buffer( 1, item2 ),
	    buffer( 2, item3 ),
	    buffer( 3, item4 ) );
  }
}


template < class S, class T, class U, class V, class W >
void send( thing_array& array, const char* text,
	   S item1, T item2, U item3, V item4, W item5 )
{
  clear_send_buffers( );
  for( int i = 0; i < array; ++i ) {
    char_data *rch = character( array[i] );
    if( rch
	&& rch->Can_See( )
	&& rch != (char_data*) item1
	&& rch != (char_data*) item2
	&& rch != (char_data*) item3
	&& rch != (char_data*) item4
	&& rch != (char_data*) item5 )
      send( rch, text,
	    buffer( 0, item1 ),
	    buffer( 1, item2 ),
	    buffer( 2, item3 ),
	    buffer( 3, item4 ),
	    buffer( 4, item5 ) );
  }
}


template < class S, class T, class U, class V, class W, class X >
void send( thing_array& array, const char* text,
	   S item1, T item2, U item3, V item4, W item5, X item6 )
{
  clear_send_buffers( );
  for( int i = 0; i < array; ++i ) {
    char_data *rch = character( array[i] );
    if( rch
	&& rch->Can_See( )
	&& rch != (char_data*) item1
	&& rch != (char_data*) item2
	&& rch != (char_data*) item3
	&& rch != (char_data*) item4
	&& rch != (char_data*) item5
	&& rch != (char_data*) item6 )
      send( rch, text,
	    buffer( 0, item1 ),
	    buffer( 1, item2 ),
	    buffer( 2, item3 ),
	    buffer( 3, item4 ),
	    buffer( 4, item5 ),
	    buffer( 5, item6 ) );
  }
}


/*
 *   SEND_UNDERLINED
 */


void send_underlined( char_data*, const char* );


template < class S >
void send_underlined( char_data* ch, const char* text,
		      S item )
{
  snprintf( out_buf, OUT_BUF_LEN, text, tostring( item, ch ) );
  send_underlined( ch, out_buf );
}


/*
 *   SEND_CENTERED
 */


void send_centered( char_data*, const char* );


template < class S >
void send_centered( char_data* ch, const char* text,
		    S item )
{
  snprintf( out_buf, OUT_BUF_LEN, text,
	    tostring( item, ch ) );
  send_centered( ch, out_buf );
}


template < class S, class T >
void send_centered( char_data* ch, const char* text,
		    S item1, T item2 )
{
  snprintf( out_buf, OUT_BUF_LEN, text,
	    tostring( item1, ch ),
	    tostring( item2, ch ) );
  send_centered( ch, out_buf );
}


template < class S, class T, class U >
void send_centered( char_data* ch, const char* text,
		    S item1, T item2, U item3 )
{
  snprintf( out_buf, OUT_BUF_LEN, text,
	    tostring( item1, ch ),
	    tostring( item2, ch ),
	    tostring( item3, ch ) );
  send_centered( ch, out_buf );
}


/*
 *   FSEND_ALL
 */


void fsend_all( room_data *room, const char* text );


template < class T >
void fsend_all( room_data *room, const char* text,
		T item )
{
  clear_send_buffers( );
  for( int i = 0; i < room->contents; ++i ) {
    char_data *rch = character( room->contents[i] );
    if( rch
	&& room->Seen( rch ) )
      fsend( rch, text,
	     buffer( 0, item ) );
  }
}


template < class S, class T >
void fsend_all( room_data *room, const char* text,
		S item1, T item2 )
{
  clear_send_buffers( );
  for( int i = 0; i < room->contents; ++i ) {
    char_data *rch = character( room->contents[i] );
    if( rch
	&& room->Seen( rch ) )
      fsend( rch, text,
	     buffer( 0, item1 ),
	     buffer( 1, item2 ) );
  }
}


/*
 *   FORMATTED SEND
 */


unsigned fput ( char_data*, const char* );
void fsend ( char_data*, const char* );


template < class T >
void fsend( char_data* ch, const char* text, T item )
{
  snprintf( out_buf, OUT_BUF_LEN, text,
	    tostring( item, ch ) );

  if( *text == '%' && !isdigit( *( text+1 ) ) && *(text+1) != '-' )
    capitalize( out_buf );

  fsend( ch, out_buf );
}


template < class S, class T >
void fsend( char_data* ch, const char* text, S item1, T item2 )
{
  snprintf( out_buf, OUT_BUF_LEN, text,
	    tostring( item1, ch ),
	    tostring( item2, ch ) );

  if( *text == '%' && !isdigit( *( text+1 ) ) && *(text+1) != '-' )
    capitalize( out_buf );

  fsend( ch, out_buf );
}


template < class S, class T, class U >
void fsend( char_data* ch, const char* text, S item1, T item2, U item3 )
{
  snprintf( out_buf, OUT_BUF_LEN, text,
	    tostring( item1, ch ),
	    tostring( item2, ch ),
	    tostring( item3, ch ) );

  if( *text == '%' && !isdigit( *( text+1 ) ) && *(text+1) != '-' )
    capitalize( out_buf );

  fsend( ch, out_buf );
}


template < class S, class T, class U, class V >
void fsend( char_data* ch, const char* text,
	    S item1, T item2, U item3, V item4 )
{
  snprintf( out_buf, OUT_BUF_LEN, text,
	    tostring( item1, ch ),
	    tostring( item2, ch ),
	    tostring( item3, ch ),
	    tostring( item4, ch ) );

  if( *text == '%' && !isdigit( *( text+1 ) ) && *(text+1) != '-' )
    capitalize( out_buf );

  fsend( ch, out_buf );
}


template < class S, class T, class U, class V, class W >
void fsend( char_data* ch, const char* text,
	    S item1, T item2, U item3, V item4, W item5 )
{
  snprintf( out_buf, OUT_BUF_LEN, text,
	    tostring( item1, ch ),
	    tostring( item2, ch ),
	    tostring( item3, ch ),
	    tostring( item4, ch ),
	    tostring( item5, ch ) );

  if( *text == '%' && !isdigit( *( text+1 ) ) && *(text+1) != '-' )
    capitalize( out_buf );

  fsend( ch, out_buf );
}


template < class S, class T, class U, class V, class W, class X >
void fsend( char_data* ch, const char* text,
	    S item1, T item2, U item3, V item4, W item5, X item6 )
{
  snprintf( out_buf, OUT_BUF_LEN, text,
	    tostring( item1, ch ),
	    tostring( item2, ch ),
	    tostring( item3, ch ),
	    tostring( item4, ch ),
	    tostring( item5, ch ),
	    tostring( item6, ch ) );

  if( *text == '%' && !isdigit( *( text+1 ) ) && *(text+1) != '-' )
    capitalize( out_buf );

  fsend( ch, out_buf );
}


/*
 *   FORMATTED SEND ROOM
 */


template < class T >
void fsend( thing_array& array, const char* text,
	    T item )
{
  clear_send_buffers( );

  for( int i = 0; i < array; ++i ) {
    char_data *rch = character( array[i] );
    if( rch
	&& rch->Can_See( ) 
	&& rch != (char_data*) item )
      fsend( rch, text,
	     buffer( 0, item ) );
  }
} 


template < class S, class T >
void fsend( thing_array& array, const char* text,
	    S item1, T item2 )
{
  clear_send_buffers( );
  
  for( int i = 0; i < array; ++i ) {
    char_data *rch = character( array[i] );
    if( rch
	&& rch->Can_See( )
	&& rch != (char_data*) item1
	&& rch != (char_data*) item2 )
      fsend( rch, text,
	     buffer( 0, item1 ),
	     buffer( 1, item2 ) );
  }
} 


template < class S, class T, class U >
void fsend( thing_array& array, const char* text,
	    S item1, T item2, U item3 )
{
  clear_send_buffers( );

  for( int i = 0; i < array; ++i ) {
    char_data *rch = character( array[i] );
    if( rch
	&& rch->Can_See( )
	&& rch != (char_data*) item1
	&& rch != (char_data*) item2
	&& rch != (char_data*) item3 )
      fsend( rch, text,
	     buffer( 0, item1 ),
	     buffer( 1, item2 ),
	     buffer( 2, item3 ) );
  }
}


template < class S, class T, class U, class V >
void fsend( thing_array& array, const char* text,
	    S item1, T item2, U item3, V item4 )
{
  clear_send_buffers( );

  for( int i = 0; i < array; ++i ) {
    char_data *rch = character( array[i] );
    if( rch
	&& rch->Can_See( )
	&& rch != (char_data*) item1
	&& rch != (char_data*) item2
	&& rch != (char_data*) item3
	&& rch != (char_data*) item4 )
      fsend( rch, text,
	     buffer( 0, item1 ),
	     buffer( 1, item2 ),
	     buffer( 2, item3 ),
	     buffer( 3, item4 ) );
  }
}


template < class S, class T, class U, class V, class W >
void fsend( thing_array& array, const char* text,
	    S item1, T item2, U item3, V item4, W item5 )
{
  clear_send_buffers( );

  for( int i = 0; i < array; ++i ) {
    char_data *rch = character( array[i] );
    if( rch
	&& rch->Can_See( )
	&& rch != (char_data*) item1
	&& rch != (char_data*) item2
	&& rch != (char_data*) item3
	&& rch != (char_data*) item4
	&& rch != (char_data*) item5 )
      fsend( rch, text,
	     buffer( 0, item1 ),
	     buffer( 1, item2 ),
	     buffer( 2, item3 ),
	     buffer( 3, item4 ),
	     buffer( 4, item5 ) );
  }
}


/*
template < class S, class T, class U, class V, class W, class X >
void fsend( thing_array& array, const char* text,
	    S item1, T item2, U item3, V item4, W item5, X item6 )
{
  clear_send_buffers( );

  for( int i = 0; i < array; ++i ) {
    char_data *rch = character( array[i] );
    if( rch
	&& rch->Can_See( )
	&& rch != (char_data*) item1
	&& rch != (char_data*) item2
	&& rch != (char_data*) item3
	&& rch != (char_data*) item4
	&& rch != (char_data*) item5
	&& rch != (char_data*) item6 )
      fsend( rch, text,
	     buffer( 0, item1 ),
	     buffer( 1, item2 ),
	     buffer( 2, item3 ),
	     buffer( 3, item4 ),
	     buffer( 4, item5 ),
	     buffer( 5, item6 ) );
    }
}
*/


/*
 *   FORMATTED_SEND_SEEN
 */


template < class T >
void fsend_seen( char_data* ch, const char* text,
		 T item )
{
  if( ch->array && !ch->was_in_room ) {
    clear_send_buffers( );
    for( int i = 0; i < *ch->array; ++i ) {
      char_data *rch = character( ch->array->list[i] );
      if( rch
	  && rch != ch
	  && ch->Seen( rch )
	  && rch != (char_data*) item )
	fsend( rch, text,
	       buffer( 0, item ) );
    }
  }
}


template < class S, class T >
void fsend_seen( char_data* ch, const char* text,
		 S item1, T item2 )
{
  if( ch->array && !ch->was_in_room ) {
    clear_send_buffers( );
    for( int i = 0; i < *ch->array; ++i ) {
      char_data *rch = character( ch->array->list[i] );
      if( rch
	  && rch != ch
	  && ch->Seen( rch )
	  && rch != (char_data*) item1
	  && rch != (char_data*) item2 )
	fsend( rch, text,
	       buffer( 0, item1 ),
	       buffer( 1, item2 ) );
    }
  }
}


template < class S, class T, class U >
void fsend_seen( char_data* ch, const char* text,
		 S item1, T item2, U item3 )
{
  if( ch->array && !ch->was_in_room ) {
    clear_send_buffers( );
    for( int i = 0; i < *ch->array; ++i ) {
      char_data *rch = character( ch->array->list[i] );
      if( rch
	  && rch != ch
	  && ch->Seen( rch )
	  && rch != (char_data*) item1
	  && rch != (char_data*) item2
	  && rch != (char_data*) item3 )
	fsend( rch, text,
	       buffer( 0, item1 ),
	       buffer( 1, item2 ),
	       buffer( 2, item3 ) );
    }
  }
}


template < class S, class T, class U, class V >
void fsend_seen( char_data* ch, const char* text,
		 S item1, T item2, U item3, V item4 )
{
  if( ch->array && !ch->was_in_room ) {
    clear_send_buffers( );
    for( int i = 0; i < *ch->array; ++i ) {
      char_data *rch = character( ch->array->list[i] );
      if( rch
	  && rch != ch
	  && ch->Seen( rch )
	  && rch != (char_data*) item1
	  && rch != (char_data*) item2
	  && rch != (char_data*) item3
	  && rch != (char_data*) item4 )
	fsend( rch, text,
	       buffer( 0, item1 ),
	       buffer( 1, item2 ),
	       buffer( 2, item3 ),
	       buffer( 3, item4 ) );
    }
  }
}


template < class S, class T, class U, class V, class W >
void fsend_seen( char_data* ch, const char* text,
		 S item1, T item2, U item3, V item4, W item5 )
{
  if( ch->array && !ch->was_in_room ) {
    clear_send_buffers( );
    for( int i = 0; i < *ch->array; ++i ) {
      char_data *rch = character( ch->array->list[i] );
      if( rch
	  && rch != ch && ch->Seen( rch )
	  && rch != (char_data*) item1
	  && rch != (char_data*) item2
	  && rch != (char_data*) item3
	  && rch != (char_data*) item4
	  && rch != (char_data*) item5 )
	fsend( rch, text,
	       buffer( 0, item1 ),
	       buffer( 1, item2 ),
	       buffer( 2, item3 ),
	       buffer( 3, item4 ),
	       buffer( 4, item5 ) );
    }
  }
}


/*
 *   FORMATTED_SEND_SEEN with Accept_Msg() check
 */


template < class T >
void fsend_mesg( char_data* ch, const char* text,
		 T item )
{
 if( ch->array && !ch->was_in_room ) {
    clear_send_buffers( );
    for( int i = 0; i < *ch->array; ++i ) {
      char_data *rch = character( ch->array->list[i] );
      if( rch
	  && rch != ch
	  && ch->Seen( rch )
	  && rch->Accept_Msg( ch )
	  && rch != (char_data*) item )
	fsend( rch, text,
	       buffer( 0, item ) );
    }
  }
}


template < class S, class T >
void fsend_mesg( char_data* ch, const char* text,
		 S item1, T item2 )
{
  if( ch->array && !ch->was_in_room ) {
    clear_send_buffers( );
    for( int i = 0; i < *ch->array; ++i ) {
      char_data *rch = character( ch->array->list[i] );
      if( rch
	  && rch != ch
	  && ch->Seen( rch )
	  && rch->Accept_Msg( ch )
	  && rch != (char_data*) item1
	  && rch != (char_data*) item2 )
	fsend( rch, text,
	       buffer( 0, item1 ),
	       buffer( 1, item2 ) );
    }
  }
}


template < class S, class T, class U >
void fsend_mesg( char_data* ch, const char* text,
		 S item1, T item2, U item3 )
{
  if( ch->array && !ch->was_in_room ) {
    clear_send_buffers( );
    for( int i = 0; i < *ch->array; ++i ) {
      char_data *rch = character( ch->array->list[i] );
      if( rch
	  && rch != ch
	  && ch->Seen( rch )
	  && rch->Accept_Msg( ch )
	  && rch != (char_data*) item1
	  && rch != (char_data*) item2
	  && rch != (char_data*) item3 )
	fsend( rch, text,
	       buffer( 0, item1 ),
	       buffer( 1, item2 ),
	       buffer( 2, item3 ) );
    }
  }
}


template < class S, class T, class U, class V >
void fsend_mesg( char_data* ch, const char* text,
		 S item1, T item2, U item3, V item4 )
{
  if( ch->array && !ch->was_in_room ) {
    clear_send_buffers( );
    for( int i = 0; i < *ch->array; ++i ) {
      char_data *rch = character( ch->array->list[i] );
      if( rch
	  && rch != ch
	  && ch->Seen( rch )
	  && rch->Accept_Msg( ch )
	  && rch != (char_data*) item1
	  && rch != (char_data*) item2
	  && rch != (char_data*) item3
	  && rch != (char_data*) item4 )
	fsend( rch, text,
	       buffer( 0, item1 ),
	       buffer( 1, item2 ),
	       buffer( 2, item3 ),
	       buffer( 3, item4 ) );
    }
  }
}


/*
 *   FORMATTED_PAGE_SEEN
 */


template < class T >
void fpage_seen( char_data* ch, const char* text,
		 T item )
{
  if( ch->array && !ch->was_in_room ) {
    clear_send_buffers( );
    for( int i = 0; i < *ch->array; ++i ) {
      char_data *rch = character( ch->array->list[i] );
      if( rch
	  && rch != ch
	  && ch->Seen( rch )
	  && rch != (char_data*) item )
	fpage( rch, text,
	       buffer( 0, item ) );
    }
  }
}


template < class S, class T >
void fpage_seen( char_data* ch, const char* text,
		 S item1, T item2 )
{
  if( ch->array && !ch->was_in_room ) {
    clear_send_buffers( );
    for( int i = 0; i < *ch->array; ++i ) {
      char_data *rch = character( ch->array->list[i] );
      if( rch
	  && rch != ch
	  && ch->Seen( rch )
	  && rch != (char_data*) item1
	  && rch != (char_data*) item2 )
	fpage( rch, text,
	       buffer( 0, item1 ),
	       buffer( 1, item2 ) );
    }
  }
}


template < class S, class T, class U >
void fpage_seen( char_data* ch, const char* text,
		 S item1, T item2, U item3 )
{
  if( ch->array && !ch->was_in_room ) {
    clear_send_buffers( );
    for( int i = 0; i < *ch->array; ++i ) {
      char_data *rch = character( ch->array->list[i] );
      if( rch
	  && rch != ch
	  && ch->Seen( rch )
	  && rch != (char_data*) item1
	  && rch != (char_data*) item2
	  && rch != (char_data*) item3 )
	fpage( rch, text,
	       buffer( 0, item1 ),
	       buffer( 1, item2 ),
	       buffer( 2, item3 ) );
    }
  }
}


template < class S, class T, class U, class V >
void fpage_seen( char_data* ch, const char* text,
		 S item1, T item2, U item3, V item4 )
{
  if( ch->array && !ch->was_in_room ) {
    clear_send_buffers( );
    for( int i = 0; i < *ch->array; ++i ) {
      char_data *rch = character( ch->array->list[i] );
      if( rch
	  && rch != ch
	  && ch->Seen( rch )
	  && rch != (char_data*) item1
	  && rch != (char_data*) item2
	  && rch != (char_data*) item3
	  && rch != (char_data*) item4 )
	fpage( rch, text,
	       buffer( 0, item1 ),
	       buffer( 1, item2 ),
	       buffer( 2, item3 ),
	       buffer( 3, item4 ) );
    }
  }
}


/*
 *   SEND_COLOR
 */


template < class T >
void send_color( char_data* ch, int color, const char* text, T item )
{
  snprintf( out_buf, OUT_BUF_LEN, text,
	    tostring( item, ch ) );
  send_color( ch, color, *text == '%' && !isdigit( *( text+1 ) ) && *(text+1) != '-' ? capitalize( out_buf ) : out_buf );
}


template < class S, class T >
void send_color( char_data* ch, int color, const char* text,
		 S item1, T item2 )
{
  snprintf( out_buf, OUT_BUF_LEN, text,
	    tostring( item1, ch ),
	    tostring( item2, ch ) );
  send_color( ch, color, *text == '%' && !isdigit( *( text+1 ) ) && *(text+1) != '-' ? capitalize( out_buf ) : out_buf );
}


template < class S, class T, class U >
void send_color( char_data* ch, int color, const char* text,
		 S item1, T item2, U item3 )
{
  snprintf( out_buf, OUT_BUF_LEN, text,
	    tostring( item1, ch ),
	    tostring( item2, ch ),
	    tostring( item3, ch ) );
  send_color( ch, color, *text == '%' && !isdigit( *( text+1 ) ) && *(text+1) != '-' ? capitalize( out_buf ) : out_buf );
}


template < class S, class T, class U, class V >
void send_color( char_data* ch, int color, const char* text,
		 S item1, T item2, U item3, V item4 )
{
  snprintf( out_buf, OUT_BUF_LEN, text,
	    tostring( item1, ch ),
	    tostring( item2, ch ),
	    tostring( item3, ch ),
	    tostring( item4, ch ) );
  send_color( ch, color, *text == '%' && !isdigit( *( text+1 ) ) && *(text+1) != '-' ? capitalize( out_buf ) : out_buf );
}


template < class S, class T, class U, class V, class W >
void send_color( char_data* ch, int color, const char* text,
		 S item1, T item2, U item3, V item4, W item5 )
{
  snprintf( out_buf, OUT_BUF_LEN, text,
	    tostring( item1, ch ),
	    tostring( item2, ch ),
	    tostring( item3, ch ),
	    tostring( item4, ch ),
	    tostring( item5, ch ) );
  send_color( ch, color, *text == '%' && !isdigit( *( text+1 ) ) && *(text+1) != '-' ? capitalize( out_buf ) : out_buf );
}


template < class S, class T, class U, class V, class W, class X >
void send_color( char_data* ch, int color, const char* text,
		 S item1, T item2, U item3, V item4, W item5, X item6 )
{
  snprintf( out_buf, OUT_BUF_LEN, text,
	    tostring( item1, ch ),
	    tostring( item2, ch ),
	    tostring( item3, ch ),
	    tostring( item4, ch ),
	    tostring( item5, ch ),
	    tostring( item6, ch ) );
  send_color( ch, color, *text == '%' && !isdigit( *( text+1 ) ) && *(text+1) != '-' ? capitalize( out_buf ) : out_buf );
}


/*
 *   FSEND_COLOR
 */


void fsend_color( char_data* ch, int color, const char* text );


template < class T >
void fsend_color( char_data* ch, int color, const char* text,
		  T item )
{
  send( ch, color_code( ch, color ) );
  fsend( ch, text, item );
}


template < class S, class T >
void fsend_color( char_data* ch, int color, const char* text,
		  S item1, T item2 )
{
  send( ch, color_code( ch, color ) );
  fsend( ch, text, item1, item2 );
}


template < class S, class T, class U >
void fsend_color( char_data* ch, int color, const char* text,
		  S item1, T item2, U item3 )
{
  send( ch, color_code( ch, color ) );
  fsend( ch, text, item1, item2, item3 );
}


template < class S, class T, class U, class V >
void fsend_color( char_data* ch, int color, const char* text,
		  S item1, T item2, U item3, V item4 )
{
  send( ch, color_code( ch, color ) );
  fsend( ch, text, item1, item2, item3, item4 );
}


template < class S, class T, class U, class V, class W >
void fsend_color( char_data* ch, int color, const char* text,
		  S item1, T item2, U item3, V item4, W item5 )
{
  send( ch, color_code( ch, color ) );
  fsend( ch, text, item1, item2, item3, item4, item5 );
}


template < class S, class T, class U, class V, class W, class X >
void fsend_color( char_data* ch, int color, const char* text,
		  S item1, T item2, U item3, V item4, W item5, X item6 )
{
  send( ch, color_code( ch, color ) );
  fsend( ch, text, item1, item2, item3, item4, item5, item6 );
}


/*
 *   FORMATTED_COLOR_SEND_SEEN
 */


void fsend_color_seen( char_data* ch, int color, const char* text );


template < class T >
void fsend_color_seen( char_data* ch, int color, const char* text,
		       T item )
{
  if( ch->array && !ch->was_in_room ) {
    clear_send_buffers( );
    for( int i = 0; i < *ch->array; ++i ) {
      char_data *rch = character( ch->array->list[i] );
      if( rch
	  && rch != ch
	  && ch->Seen( rch )
	  && rch != (char_data*) item )
	fsend_color( rch, color, text,
		     buffer( 0, item ) );
    }
  }
}


template < class S, class T >
void fsend_color_seen( char_data* ch, int color, const char* text,
		       S item1, T item2 )
{
  if( ch->array && !ch->was_in_room ) {
    clear_send_buffers( );
    for( int i = 0; i < *ch->array; ++i ) {
      char_data *rch = character( ch->array->list[i] );
      if( rch
	  && rch != ch
	  && ch->Seen( rch )
	  && rch != (char_data*) item1
	  && rch != (char_data*) item2 )
	fsend_color( rch, color, text,
		     buffer( 0, item1 ),
		     buffer( 1, item2 ) );
    }
  }
}


template < class S, class T, class U >
void fsend_color_seen( char_data* ch, int color, const char* text,
		       S item1, T item2, U item3 )
{
  if( ch->array && !ch->was_in_room ) {
    clear_send_buffers( );
    for( int i = 0; i < *ch->array; ++i ) {
      char_data *rch = character( ch->array->list[i] );
      if( rch
	  && rch != ch
	  && ch->Seen( rch )
	  && rch != (char_data*) item1
	  && rch != (char_data*) item2
	  && rch != (char_data*) item3 ) {
	fsend_color( rch, color, text,
		     buffer( 0, item1 ),
		     buffer( 1, item2 ),
		     buffer( 2, item3 ) );
      }
    }
  }
}


template < class S, class T, class U, class V >
void fsend_color_seen( char_data* ch, int color, const char* text,
		       S item1, T item2, U item3, V item4 )
{
  if( ch->array && !ch->was_in_room ) {
    clear_send_buffers( );
    for( int i = 0; i < *ch->array; ++i ) {
      char_data *rch = character( ch->array->list[i] );
      if( rch
	  && rch != ch
	  && ch->Seen( rch )
	  && rch != (char_data*) item1
	  && rch != (char_data*) item2
	  && rch != (char_data*) item3
	  && rch != (char_data*) item4 )
	fsend_color( rch, color, text,
		     buffer( 0, item1 ),
		     buffer( 1, item2 ),
		     buffer( 2, item3 ),
		     buffer( 3, item4 ) );
    }
  }
}


template < class S, class T, class U, class V, class W >
void fsend_color_seen( char_data* ch, int color, const char* text,
		       S item1, T item2, U item3, V item4, W item5 )
{
  if( ch->array && !ch->was_in_room ) {
    clear_send_buffers( );
    for( int i = 0; i < *ch->array; ++i ) {
      char_data *rch = character( ch->array->list[i] );
      if( rch
	  && rch != ch
	  && ch->Seen( rch )
	  && rch != (char_data*) item1
	  && rch != (char_data*) item2
	  && rch != (char_data*) item3
	  && rch != (char_data*) item4
	  && rch != (char_data*) item5 )
	fsend_color( rch, color, text,
		     buffer( 0, item1 ),
		     buffer( 1, item2 ),
		     buffer( 2, item3 ),
		     buffer( 3, item4 ),
		     buffer( 4, item5 ) );
    }
  }
}


template < class S, class T, class U, class V, class W, class X >
void fsend_color_seen( char_data* ch, int color, const char* text,
		       S item1, T item2, U item3, V item4, W item5,
		       X item6 )
{
  if( ch->array && !ch->was_in_room ) {
    clear_send_buffers( );
    for( int i = 0; i < *ch->array; ++i ) {
      char_data *rch = character( ch->array->list[i] );
      if( rch
	  && rch != ch
	  && ch->Seen( rch )
	  && rch != (char_data*) item1
	  && rch != (char_data*) item2
	  && rch != (char_data*) item3
	  && rch != (char_data*) item4
	  && rch != (char_data*) item5
	  && rch != (char_data*) item6 )
	fsend_color( rch, color, text,
		     buffer( 0, item1 ),
		     buffer( 1, item2 ),
		     buffer( 2, item3 ),
		     buffer( 3, item4 ),
		     buffer( 4, item5 ),
		     buffer( 5, item6 ) );
    }
  }
}


/*
 *   FORMATTED COLOR SEND ROOM
 */


void fsend_color( thing_array& array, int color, const char* text );


template < class T >
void fsend_color( thing_array& array, int color, const char* text,
		  T item )
{
  clear_send_buffers( );

  for( int i = 0; i < array; ++i ) {
    char_data *rch = character( array[i] );
    if( rch
	&& rch->Can_See( ) 
	&& rch != (char_data*) item )
      fsend_color( rch, color, text,
		   buffer( 0, item ) );
  }
} 


template < class S, class T >
void fsend_color( thing_array& array, int color, const char* text,
		  S item1, T item2 )
{
  clear_send_buffers( );
  
  for( int i = 0; i < array; ++i ) {
    char_data *rch = character( array[i] );
    if( rch
	&& rch->Can_See( )
	&& rch != (char_data*) item1
	&& rch != (char_data*) item2 )
      fsend_color( rch, color, text,
		   buffer( 0, item1 ),
		   buffer( 1, item2 ) );
  }
}


template < class S, class T, class U >
void fsend_color( thing_array& array, int color, const char* text,
		  S item1, T item2, U item3 )
{
  clear_send_buffers( );

  for( int i = 0; i < array; ++i ) {
    char_data *rch = character( array[i] );
    if( rch
	&& rch->Can_See( )
	&& rch != (char_data*) item1
	&& rch != (char_data*) item2
	&& rch != (char_data*) item3 )
      fsend_color( rch, color, text,
		   buffer( 0, item1 ),
		   buffer( 1, item2 ),
		   buffer( 2, item3 ) );
  }
}


template < class S, class T, class U, class V >
void fsend_color( thing_array& array, int color, const char* text,
		  S item1, T item2, U item3, V item4 )
{
  clear_send_buffers( );

  for( int i = 0; i < array; ++i ) {
    char_data *rch = character( array[i] );
    if( rch
	&& rch->Can_See( )
	&& rch != (char_data*) item1
	&& rch != (char_data*) item2
	&& rch != (char_data*) item3
	&& rch != (char_data*) item4 )
      fsend_color( rch, color, text,
		   buffer( 0, item1 ),
		   buffer( 1, item2 ),
		   buffer( 2, item3 ),
		   buffer( 3, item4 ) );
  }
}


template < class S, class T, class U, class V, class W >
void fsend_color( thing_array& array, int color, const char* text,
		  S item1, T item2, U item3, V item4, W item5 )
{
  clear_send_buffers( );

  for( int i = 0; i < array; ++i ) {
    char_data *rch = character( array[i] );
    if( rch
	&& rch->Can_See( )
	&& rch != (char_data*) item1
	&& rch != (char_data*) item2
	&& rch != (char_data*) item3
	&& rch != (char_data*) item4
	&& rch != (char_data*) item5 )
      fsend_color( rch, color, text,
		   buffer( 0, item1 ),
		   buffer( 1, item2 ),
		   buffer( 2, item3 ),
		   buffer( 3, item4 ),
		   buffer( 4, item5 ) );
  }
}


/*
template < class S, class T, class U, class V, class W, class X >
void fsend_color( thing_array&, int color, const char* text,
		  S item1, T item2, U item3, V item4, W item5, X item6 )
{
  clear_send_buffers( );

  for( int i = 0; i < array; ++i ) {
    char_data *rch = character( array[i] );
    if( rch
	&& rch->Can_See( )
	&& rch != (char_data*) item1
	&& rch != (char_data*) item2
	&& rch != (char_data*) item3
	&& rch != (char_data*) item4
	&& rch != (char_data*) item5
	&& rch != (char_data*) item6 )
      fsend_color( rch, color, text,
		   buffer( 0, item1 ),
		   buffer( 1, item2 ),
		   buffer( 2, item3 ),
		   buffer( 3, item4 ),
		   buffer( 4, item5 ),
		   buffer( 5, item6 ) );
  }
}
*/


/*
 *   PAGE_COLOR
 */


template < class T >
void page_color( char_data* ch, int color, const char* text,
		 T item )
{
  snprintf( out_buf, OUT_BUF_LEN, text,
	    tostring( item, ch ) );
  page_color( ch, color, out_buf );
}


template < class S, class T >
void page_color( char_data* ch, int color, const char* text,
		 S item1, T item2 )
{
  snprintf( out_buf, OUT_BUF_LEN, text,
	    tostring( item1, ch ),
	    tostring( item2, ch ) );
  page_color( ch, color, out_buf );
}


template < class S, class T, class U >
void page_color( char_data* ch, int color, const char* text,
		 S item1, T item2, U item3 )
{
  snprintf( out_buf, OUT_BUF_LEN, text,
	    tostring( item1, ch ),
	    tostring( item2, ch ),
	    tostring( item3, ch ) );
  page_color( ch, color, out_buf );
}


/*
 *   TITLE
 */


void  page_title    ( char_data*, const char* );
void  send_title    ( char_data*, const char* );


template < class S >
void page_title( char_data* ch, const char* text,
		 S item )
{
  snprintf( out_buf, OUT_BUF_LEN, text, tostring( item, ch ) );
  page_title( ch, out_buf );
}


template < class S, class T >
void page_title( char_data* ch, const char* text,
		 S item1, T item2 )
{
  snprintf( out_buf, OUT_BUF_LEN, text,
	    tostring( item1, ch ),
	    tostring( item2, ch ) );
  page_title( ch, out_buf );
}


template < class S >
void send_title( char_data* ch, const char* text,
		 S item )
{
  snprintf( out_buf, OUT_BUF_LEN, text, tostring( item, ch ) );
  send_title( ch, out_buf );
}


#endif // tfe_output_h
