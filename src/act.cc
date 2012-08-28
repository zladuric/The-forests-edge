#include <ctype.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "define.h"
#include "struct.h"


int act_color = COLOR_DEFAULT;


static char  act_tmp  [ 3*MAX_STRING_LENGTH ];
static char  act_buf  [ 3*MAX_STRING_LENGTH ];


/*
 *   ACT
 */


void act( char_data* to, const char* text,
	  char_data* ch,
	  obj_data* obj,
	  const char* string1,
	  const char* string2 )
{
  if( !to || !to->link || !text || !*text )
    return;

  act_print( act_buf, text, ch, 0, obj, 0, string1, string2, 0, to );   

  if( act_color != COLOR_DEFAULT ) {
    send( to, color_code( to, act_color ) );
  }

  convert_to_ansi( to, 3*MAX_STRING_LENGTH, act_buf, act_tmp, act_color );
  fsend( to, act_tmp );

  if( act_color != COLOR_DEFAULT ) {
    send( to, color_code( to, COLOR_DEFAULT ) );
  }
}


void act( char_data* to, const char* text,
	  char_data* ch,
	  char_data* victim,
	  obj_data* obj1,
	  obj_data* obj2,
	  exit_data *exit,
	  const char *string1,
	  const char *string2 )
{
  if( !to || !to->link || !text || !*text )
    return;

  act_print( act_buf, text, ch, victim, obj1, obj2, string1, string2, exit, to );

  if( act_color != COLOR_DEFAULT ) {
    send( to, color_code( to, act_color ) );
  }

  convert_to_ansi( to, 3*MAX_STRING_LENGTH, act_buf, act_tmp, act_color );
  fsend( to, act_tmp );

  if( act_color != COLOR_DEFAULT ) {
    send( to, color_code( to, COLOR_DEFAULT ) );
  }
}


/*
 *   ACT_AREA
 */


void act_area( const char* text,
	       char_data* ch,
	       char_data* victim,
	       obj_data* obj1,
	       obj_data *obj2,
	       const char *string1,
	       const char *string2 )
{
  if( !ch || !ch->array )
    return;

  room_data*  room;
  char_data*   rch;
  
  if( !( room = Room( ch->array->where ) ) )
    return;
  
  for( room = room->area->room_first; room; room = room->next ) {
    if( room != ch->array->where ) {
      for( int i = 0; i < room->contents; ++i ) {
        if( ( rch = character( room->contents[i] ) )
	    && rch->position > POS_SLEEPING ) {
          act( rch, text, ch, victim, obj1, obj2, 0, string1, string2 );
	}
      }
    }
  }
}


/*
 *   ACT_NOTCHAR
 */


void act_notchar( const char* text,
		  char_data* ch,
		  char_data* victim,
		  obj_data* obj1,
		  obj_data* obj2,
		  exit_data *exit,
		  const char *string1,
		  const char *string2 )
{
  if( !ch || !ch->array )
    return;

  char_data* rch;

  for( int i = 0; i < *ch->array; ++i ) {
    if( ( rch = character( ch->array->list[i] ) )
	&& rch != ch
	&& rch->position > POS_SLEEPING 
	&& rch->Accept_Msg( ch ) ) {
      act( rch, text, ch, victim, obj1, obj2, exit, string1, string2 );
    }
  }
}


void act_notchar( const char* text, char_data* ch, obj_data* obj,
		  const char* string1, const char* string2 )
{
  if( !ch || !ch->array )
    return;

  char_data* rch;

  for( int i = 0; i < *ch->array; ++i ) {
    if( ( rch = character( ch->array->list[i] ) )
	&& rch != ch
	&& rch->position > POS_SLEEPING  
	&& rch->Accept_Msg( ch ) ) {
      act( rch, text, ch, obj, string1, string2 );
    }
  }
}


/* 
 *   ACT_ROOM
 */


void act_room( room_data *room, const char* text,
	       char_data* ch,
	       char_data* victim,
	       obj_data* obj1,
	       obj_data* obj2,
	       exit_data *exit,
	       const char *string1,
	       const char *string2 )
{
  if( !room )
    return;

  char_data* rch;

  for( int i = 0; i < room->contents; ++i ) {
    if( ( rch = character( room->contents[i] ) )
	&& rch->position > POS_SLEEPING ) {
      act( rch, text, ch, victim, obj1, obj2, exit, string1, string2 );
    }
  }
}


void act_room( room_data* room, const char* text,
	       const char* string1,
	       const char* string2 )
{
  if( !room )
    return;

  char_data* rch;
  
  for( int i = 0; i < room->contents; ++i ) {
    if( ( rch = character( room->contents[i] ) )
	&& rch->position > POS_SLEEPING ) {
      act( rch, text, 0, 0, string1, string2 );
    }
  }
}


void act_neither( const char* text,
		  char_data* ch,
		  char_data* victim,
		  obj_data* obj1,
		  obj_data* obj2,
		  const char *string1,
		  const char *string2 )
{
  if( !ch || !ch->array )
    return;

  char_data* rch;
 
  for( int i = 0; i < *ch->array; ++i ) {
    if( ( rch = character( ch->array->list[i] ) )
	&& rch != ch
	&& rch != victim
	&& rch->position > POS_SLEEPING
	&& rch->Accept_Msg( ch ) ) {
      act( rch, text, ch, victim, obj1, obj2, 0, string1, string2 );
    }
  }
}


void act_notvict( const char* text,
		  char_data* ch,
		  char_data* victim,
		  obj_data* obj1,
		  obj_data* obj2,
		  const char *string1,
		  const char *string2 )
{
  if( !ch || !ch->array )
    return;

  char_data* rch;
 
  for( int i = 0; i < *ch->array; ++i ) {
    if( ( rch = character( ch->array->list[i] ) )
	&& rch != victim
	&& rch->position > POS_SLEEPING
	&& rch->Accept_Msg( ch ) ) {
      act( rch, text, ch, victim, obj1, obj2, 0, string1, string2 );
    }
  }
}


void act_seen( const char* text,
	       char_data* ch,
	       char_data* victim,
	       obj_data* obj1,
	       obj_data* obj2,
	       exit_data *exit,
	       const char *string1,
	       const char *string2 )
{
  if( !ch || !ch->array )
    return;

  char_data* rch;

  for( int i = 0; i < *ch->array; ++i ) {
    if( ( rch = character( ch->array->list[i] ) )
	&& rch != ch
	&& ch->Seen( rch )
	&& rch->Accept_Msg( ch ) ) {
      act( rch, text, ch, victim, obj1, obj2, exit, string1, string2 );
    }
  }
}


/*
 *   ACT_SOCIAL
 */


void act_social( char_data* to, const char* text, char_data* ch,
		 char_data* victim, obj_data* obj1, obj_data *obj2,
		 exit_data *exit )
{
  const bool heard = ( *text == '!' );
  
  if( heard )
    ++text;
  
  if( heard
      || ch->Seen( to )
      || ( victim && victim != to && victim->Seen( to ) ) ) {
    act( to, text, ch, victim, obj1, obj2, exit );
  }
}


void act_social( char_data* to, const char* text, char_data* ch,
	       const char* string1, const char* string2 )
{
  const bool heard = ( *text == '!' );
  
  if( heard )
    ++text;
  
  if( heard
      || ch->Seen( to ) ) {
    act( to, text, ch, 0, string1, string2 );
  }
}


void act_social_room( const char *text, char_data *ch,
		      char_data *victim, obj_data *obj1, obj_data *obj2,
		      exit_data *exit )
{
  if( !ch || !ch->array )
    return;

  char_data* rch;
 
  for( int i = 0; i < *ch->array; i++ ) {
    if( ( rch = character( ch->array->list[i] ) )
	&& rch != ch
	&& rch != victim
	&& rch->position > POS_SLEEPING
	&& rch->Accept_Msg( ch ) ) {
      act_social( rch, text, ch, victim, obj1, obj2, exit );
    }
  }
}


void act_social_room( const char *text, char_data *ch,
		      const char* string1, const char* string2 )
{
  if( !ch || !ch->array )
    return;

  char_data* rch;
 
  for( int i = 0; i < *ch->array; i++ ) {
    if( ( rch = character( ch->array->list[i] ) )
	&& rch != ch
	&& rch->position > POS_SLEEPING
	&& rch->Accept_Msg( ch ) ) {
      act_social( rch, text, ch, string1, string2 );
    }
  }
}


/*
 *  MAIN PRINT ROUTINE FOR ACT
 */


void act_print( char* out, const char* in, char_data* ch,
		char_data* victim, obj_data* obj1, obj_data* obj2,
		const char* string1, const char* string2,
		exit_data *exit,
		char_data* to,
		bool upcase )
{
  const char *sub;
  char *start = out;
  bool first = upcase;

  while( true ) {
    if( first && *in == '@' ) {
      // Skip initial ANSI codes, so uppercasing first letter works.
      *out++ = '@';
      if( !*++in )
	break;
      *out++ = *in;
    } else if( *in != '$' ) {
      *out++ = *in;
      first = false;
    } else {

      // Prefixes.
      bool upper = false;
      bool lower = false;
      switch( toupper( *++in ) ) {
      case '>':
	upper = true;
	++in;
	break;
      case '<':
	lower = true;
	++in;
	break;
      }

      char_data *pers = islower( *in ) ? ch : victim;
      obj_data *obj3 = islower( *in ) ? obj1 : obj2;
      const char *string = islower( *in ) ? string1 : string2;
      
      switch( toupper( *in ) ) {

      case '$':
        sub = "$";
        break;       
	
      case 'P':
	sub = ( obj3 ? obj3->Name( to, obj3->Selected( ) ) : "[BUG]");
	break;
	
      case 'Q':
	{
	  if( obj3 ) {
	    const char *me_loc, *them_loc;
	    obj_loc_spam( ch, obj3, 0, me_loc, them_loc );
	    char *buf = static_string();
	    snprintf( buf, THREE_LINES, "%s%s",
		      obj3->Name( to, obj3->Selected( ) ),
		      ( ch == to ) ? me_loc : them_loc );
	    sub = buf;
	  } else {
	    sub = "[BUG]";
	  }
	}
	break;

      case 'R':
	{
	  if( obj3 ) {
	    const char *me_loc, *them_loc;
	    obj_loc_spam( ch, obj3, 0, me_loc, them_loc, to );
	    char *buf = static_string();
	    snprintf( buf, THREE_LINES, "%s%s",
		      obj3->Name( to, obj3->Selected( ) ),
		      ( ch == to ) ? me_loc : them_loc );
	    sub = buf;
	  } else {
	    sub = "[BUG]";
	  }
	}
	break;

      case 'T':
	sub = ( string ? string : "[BUG]" );
        break;
	
      case 'N':
        sub = ( pers ? pers->Name( to ) : "[BUG]");
        break;
	
      case 'H':
        sub = ( pers ? pers->The_Name( to ) : "[BUG]");
        break;
	
      case 'E':
        sub = ( pers ? pers->He_She( to ) : "[BUG]");
        break;
	
      case 'M':
        sub = ( pers ? pers->Him_Her( to ) : "[BUG]");
        break;
	
      case 'S':
        sub = ( pers ? pers->His_Her( to ) : "[BUG]");
        break;
	
      case 'G':
	sub = ( pers ?
		( ( pers->species || pers->pcdata->religion == REL_NONE ) ? "the gods"
		  : religion_table[ pers->pcdata->religion ].name )
		: "[BUG]"
		);
	break;

      case 'X':
        sub = ( pers ? pers->Name( to, 1, true ) : "[BUG]");
	break;

      case 'Y': {
	sub = ( obj3 ? obj3->Name( to, obj3->Selected( ), true ) : "[BUG]");
	break;
      }

      case 'D': {
	sub = ( exit ? exit->Name( to ) : "[BUG]");
	break;
      }

      case 'Z':
	// Plurals, $zX.
	switch( *++in ) {
	case '1':
	  if( obj3 ) {
	    sub = ( obj3->Selected( ) == 1 ) ? "s" : "";
	  } else {
	    sub = "[BUG]";
	  }
	  break;
	case '2':
	  if( obj3 ) {
	    sub = ( obj3->Selected( ) == 1 ) ? "es" : "";
	  } else {
	    sub = "[BUG]";
	  }
	  break;
	default:
	  sub = "[BUG]";
	  break;
	}
	break;
	
      case 'C':
	// Time and date codes, $cX.
	switch( *++in ) {
	case 'A':
	  sub = day_table[weather.day_of_week].name;
	  break;

	case 'B':
	  sub = month_table[weather.month].name;
	  break;

	case 'e':
	  {
	    char *buf = static_string();
	    snprintf( buf, THREE_LINES, "%d", weather.day_of_month );
	    sub = buf;
	  }
	  break;

	case 'Y':
	  {
	    char *buf = static_string();
	    snprintf( buf, THREE_LINES, "%d", weather.year );
	    sub = buf;
	  }
	  break;

	case 'l':
	  {
	    char *buf = static_string();
	    snprintf( buf, THREE_LINES, "%d", ( weather.hour + 11 ) % 12 + 1 );
	    sub = buf;
	  }
	  break; 

	case 'k':
	  {
	    char *buf = static_string();
	    snprintf( buf, THREE_LINES, "%d", weather.hour );
	    sub = buf;
	  }
	  break;

	case 'M':
	  {
	    char *buf = static_string();
	    snprintf( buf, THREE_LINES, "%02d", weather.minute );
	    sub = buf;
	  }
	  break;

	case 'P':
	  sub = ( weather.hour < 12 ) ? "AM" : "PM";
	  break;

	default:
	  sub = "[BUG]";
	  break;
	}
	break;

      default:
        sub = "[BUG]";
        break;
      }
      strcpy( out, sub );
      if( first ) {
	*out = toupper( *out );
	first = false;
      }
      if( upper ) {
	*out = toupper( *out );
      } else if( lower ) {
	*out = tolower( *out );
      }
      out += strlen( sub );
    }
    if( *in++ == '\0' )
      break;
  }
  
  if( upcase )
    *start = toupper( *start );
}
