#ifndef tfe_log_h
#define tfe_log_h


/*
 *   LOG ROUTINES
 */


void   immortal_log       ( char_data*, const char *, const char * );
void   mob_log            ( char_data*, int, const char* ); 
void   obj_log            ( char_data*, int, const char* ); 
void   room_log           ( char_data*, int, const char* ); 
void   player_log         ( char_data*, const char* );
void   player_log         ( pfile_data*, const char* );


template < class S >
void mob_log( char_data* ch, int vnum, const char* text,
	      S item1 )
{
  snprintf( out_buf, SIX_LINES, text,
	    tostring( item1, ch ) );
  mob_log( ch, vnum, out_buf );
}

  
template < class S, class T >
void mob_log( char_data* ch, int vnum, const char* text,
	      S item1,
	      T item2 )
{
  snprintf( out_buf, SIX_LINES, text,
	    tostring( item1, ch ),
	    tostring( item2, ch ) );
  mob_log( ch, vnum, out_buf );
}


template < class S, class T, class U >
void mob_log( char_data* ch, int vnum, const char* text,
	      S item1,
	      T item2,
	      U item3 )
{
  snprintf( out_buf, SIX_LINES, text,
	    tostring( item1, ch ),
	    tostring( item2, ch ),
	    tostring( item3, ch ) );
  mob_log( ch, vnum, out_buf );
}


template < class S, class T, class U, class V >
void mob_log( char_data* ch, int vnum, const char* text,
	      S item1,
	      T item2,
	      U item3,
	      V item4 )
{
  snprintf( out_buf, SIX_LINES, text,
	    tostring( item1, ch ),
	    tostring( item2, ch ),
	    tostring( item3, ch ),
	    tostring( item4, ch ) );
  mob_log( ch, vnum, out_buf );
}


template < class S >
void obj_log( char_data* ch, int vnum, const char* text,
	      S item1 )
{
  snprintf( out_buf, SIX_LINES, text,
	    tostring( item1, ch ) );
  obj_log( ch, vnum, out_buf );
}

  
template < class S, class T >
void obj_log( char_data* ch, int vnum, const char* text,
	      S item1,
	      T item2 )
{
  snprintf( out_buf, SIX_LINES, text,
	    tostring( item1, ch ),
	    tostring( item2, ch ) );
  obj_log( ch, vnum, out_buf );
}


template < class S, class T, class U >
void obj_log( char_data* ch, int vnum, const char* text,
	      S item1,
	      T item2,
	      U item3 )
{
  snprintf( out_buf, SIX_LINES, text,
	    tostring( item1, ch ),
	    tostring( item2, ch ),
	    tostring( item3, ch ) );
  obj_log( ch, vnum, out_buf );
}


template < class S >
void player_log( char_data* ch, const char* text,
		 S item1 )
{
  snprintf( out_buf, SIX_LINES, text,
	    tostring( item1, ch ) );
  player_log( ch, out_buf );
}

  
template < class S, class T >
void player_log( char_data* ch, const char* text,
		 S item1,
		 T item2 )
{
  snprintf( out_buf, SIX_LINES, text,
	    tostring( item1, ch ),
	    tostring( item2, ch ) );
  player_log( ch, out_buf );
}

  
template < class S, class T, class U >
void player_log( char_data* ch, const char* text,
		 S item1,
		 T item2,
		 U item3 )
{
  snprintf( out_buf, SIX_LINES, text,
	    tostring( item1, ch ),
	    tostring( item2, ch ),
	    tostring( item3, ch ) );
  player_log( ch, out_buf );
}

  
template < class S, class T, class U, class V >
void player_log( char_data* ch, const char* text,
		 S item1,
		 T item2,
		 U item3,
		 V item4 )
{
  snprintf( out_buf, SIX_LINES, text,
	    tostring( item1, ch ),
	    tostring( item2, ch ),
	    tostring( item3, ch ),
	    tostring( item4, ch ) );
  player_log( ch, out_buf );
}


template < class S >
void room_log( char_data* ch, int vnum, const char* text,
	       S item1 )
{
  snprintf( out_buf, SIX_LINES, text,
	    tostring( item1, ch ) );
  room_log( ch, vnum, out_buf );
}

  
template < class S, class T >
void room_log( char_data* ch, int vnum, const char* text,
	       S item1,
	       T item2 )
{
  snprintf( out_buf, SIX_LINES, text,
	    tostring( item1, ch ),
	    tostring( item2, ch ) );
  room_log( ch, vnum, out_buf );
}


#endif // tfe_log_h
