#include <ctype.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "define.h"
#include "struct.h"


const char *He_She  [] = { "it",  "he",  "she" };
const char *Him_Her [] = { "it",  "him", "her" };
const char *His_Her [] = { "its", "his", "her" };


const char *const scroll_line[3] =
{
   "-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\
=-=-=-=-\n\r",
   ">>>>==--------====--<>--====--------==<<<<\n\r",
   "+=-------------------------------------------------------------------\
-------=+\n\r"
};


char out_buf [ OUT_BUF_LEN ];


static char format_buf [ 3*MAX_STRING_LENGTH ];


/*
 *  String buffering, in case looping output (send_seen(), fsend_mesg(), etc.)
 *  arguments consume static strings, possibly overwriting other arguments.
 *  Only really necessary for (const char*) arguments, but could save some CPU
 *  not to regenerate others each time through loop.
 *  Can't do this for argument types that depend on ch.
 */


#define SEND_BUFFERS 6

static bool clear[ SEND_BUFFERS ];
static char send_buffers[ SEND_BUFFERS ][ MAX_STRING_LENGTH ];


void clear_send_buffers( )
{
  for( int i = 0; i < SEND_BUFFERS; ++i )
    clear[i] = true;
}


const char *buffer( int n, species_data *species )
{
  if( clear[n] ) {
    clear[n] = false;
    strcpy( send_buffers[n], species->Name( ) );
  }

  return send_buffers[n];
}


const char *buffer( int n, obj_clss_data* obj_clss )
{
  if( clear[n] ) {
    clear[n] = false;
    strcpy( send_buffers[n], obj_clss->Name( ) );
  }

  return send_buffers[n];
}


const char *buffer( int n, const char *a )
{
  if( clear[n] ) {
    clear[n] = false;
    strcpy( send_buffers[n], a );
  }

  return send_buffers[n];
}


/*
 *   LOW LEVEL ROUTINES
 */


void send( link_data *link, const char *message )
{
  static char snoop_buf [ ONE_LINE ];

  if( link && message ) {
    if( link->prompted ) {
      text_data *text = new text_data( color_code( link->character, COLOR_DEFAULT ) );
      append( link->send, text );
      text = new text_data( "[K" );
      append( link->send, text );
      link->prompted = false;
    }
    while( *message ) {
      text_data *text = new text_data( message, true );
      append( link->send, text );
      message += text->message.length;
      if( link->snoop_by
	  && link->snoop_by->player
	  && !is_set( link->player->status, STAT_NO_SNOOP ) ) {
	if( link->newline ) {
	  snprintf( snoop_buf, ONE_LINE, "|%s ", link->player->descr->name );
	  send_color( link->snoop_by->player, COLOR_WIZARD, snoop_buf );
	}
	send( link->snoop_by, text->message.text );
      }
      link->newline = text->message.newline( );
    }
  }
}


/*
 *   ACCEPT MESSAGE?
 */


bool player_data :: Accept_Msg( char_data* ch ) const
{
  if( msg_type == MSG_STANDARD
      || ch == (char_data *)this )
    return true;

  const int level = level_setting( &pcdata->mess_settings, msg_type );

  if( level == 3 )
    return true;

  if( level == 0 ) 
    return false;

  if( Befriended( ch ) )
    return true;

  if( level == 1 )
    return false;

  return is_same_group( this, ch );
}


bool Mob_Data :: Accept_Msg( char_data* ) const
{
  return link != 0;
}


/* 
 *   PAGER ROUTINES
 */


/* This function takes a character and a text string as input.  It parses
   the string creating a new text_data item that is appended to the 
   end of of the character's link->paged list for output */
void page( char_data *ch, const char *txt )
{
  static char page_buf [ 3*MAX_STRING_LENGTH ];

  text_data*   send;
  link_data*   link;
  char*        line;

  /* Set link to point at the characters link.  If the character doesn't
     exist, or the link doesn't exist, return. */
  if( !ch || !( link = ch->link ) )
    return;

  
  /* For loop parses through the inputed string, until end of line or end
     of string.  If it hits end of string, it puts an end of line character
     before the end of string.  If it hits end of line, it skips the 
     carriage return and line feed, and continues parsing the string. */
  while( true ) {
    /* Copy txt to line (which points to the front of the buffer, until
       end of string or new line. */
    for( line = page_buf; *txt != '\n' && *txt; )
      *(line++) = *(txt++);

    /* If hit end of string, put a newline and carriage return at the end
       of line */
    if( *txt ) {
      *(line++) = '\n';
      *(line++) = '\r';
    }

    /* Terminate the line string */
    *line = '\0';

    /* If the text pointer is at the end of line, skip the newline
       and carriage return. */ 
    if( *txt == '\n' )
      if( *(++txt) == '\r' )
        txt++;
    
    /* If the buffer doesn't exist, then don't send it to the character */
    if( !*page_buf )
      break;

    /* Create a new text_data item with tmp as the string, and append it to 
       the link->paged list (for output) */
    send = new text_data( page_buf );
    append( link->paged, send );
  }
}


void fpage( char_data* ch, const char *text )
{
  if( !ch || !ch->link )
    return;

  format( format_buf, text, false, 0, ch );

  page( ch, format_buf );
}    


void ipage( char_data* ch, const char *text, int indent )
{
  if( !ch || !ch->link )
    return;

  format( format_buf, text, false, indent, ch );

  page( ch, format_buf );
}    


void clear_pager( char_data* ch )
{
  if( !ch->link || !ch->link->paged )
    return;

  delete_list( ch->link->paged );
}


void next_page( link_data* link )
{
  text_data*   next;
  text_data*   send;
  char*      letter;
  int         lines  = 0;

  if( !link || !( send = link->paged ) )
    return;

  for( ; send->next; send = send->next ) { 
    for( letter = send->message.text; *letter; ++letter )
      if( *letter == '\n' )
        ++lines;
    if( lines > link->character->pcdata->lines-4 )
      break;
  }

  if( link->prompted ) {
    text_data *text = new text_data( color_code( link->character, COLOR_DEFAULT ) );
    append( link->send, text );
    text = new text_data( "[K" );
    append( link->send, text );
    link->prompted = false;
  }

  // Append paged stuff to output.
  next = send->next;
  send->next = 0;
  cat( link->send, link->paged );
  link->paged = next;
}


void page_centered( char_data* ch, const char* text )
{
  const unsigned length = strlen( text );
  const unsigned width = /*ch->pcdata ? ch->pcdata->columns :*/ 80;

  if( length > width ) {
    page( ch, text );
    page( ch, "\n\r" );
    /*
    roach( "Page_Centered: Length of text > screen width (%d).", width );
    roach( "-- Char = %s", ch );
    roach( "-- Text = %s", text );
    */
    return;
  }

  snprintf( format_buf, 3*MAX_STRING_LENGTH, "%*s%s\n\r", (width-length)/2, "", text );

  page( ch, format_buf );
}


void page_underlined( char_data* ch, const char* text )
{
  size_t i;

  page( ch, text );

  for( i = 0; i < strlen( text ); i++ )
    if( text[i] == ' ' || text[i] == '\n' || text[i] == '\r' )
      format_buf[i] = text[i];
    else
      format_buf[i] = '-';

  format_buf[i] = '\0';

  page( ch, format_buf );
}  


/*
 *   FSEND_TO_AREA
 */

/*
void fsend_to_area( area_data *area, const char* string )
{
  char_data *rch;

  for( room_data *room = area->room_first; room; room = room->next )   
    for( int i = 0; i < room->contents; i++ )    
      if( ( rch = character( room->contents[i] ) )
	  && rch->link ) 
        fsend( rch, string );
}
*/


/*
 *   FORMATTED SEND
 */


unsigned fput( char_data *ch, const char *text )
{
  if( !ch
      || !ch->link
      || ch->was_in_room
      || !text )
    return 0;

  const unsigned col = format( format_buf, text, false, 0, ch, false );

  send( ch->link, format_buf );

  return col;
}


void fsend( char_data* ch, const char *text )
{
  if( !ch
      || !ch->link
      || ch->was_in_room
      || !text )
    return;

  format( format_buf, text, false, 0, ch );

  send( ch->link, format_buf );
}


void send_seen( char_data* ch, const char* text )
{
  if( ch && ch->array && !ch->was_in_room ) {
    char_data *rch;
    for( int i = 0; i < *ch->array; i++ ) 
      if( ( rch = character( ch->array->list[i] ) )
	  && rch != ch
	  && ch->Seen( rch ) )
        send( rch, text );
  }
}


void send_centered( char_data* ch, const char* text )
{
  const unsigned length = strlen( text );
  const unsigned width = /*ch->pcdata ? ch->pcdata->columns :*/ 80;

  if( length > width ) {
    send( ch, text );
    send( ch, "\n\r" );
    //    bug( "Send_Centered: Text > 80 Characters." );
    //    bug( text );
    return;
  }

  snprintf( format_buf, MAX_STRING_LENGTH, "%*s%s\n\r", (width-length)/2, "", text );

  send( ch, format_buf );
}


void send_underlined( char_data* ch, const char* text )
{
  size_t     i;

  send( ch, text );

  for( i = 0; i < strlen( text ); i++ )
    if( text[i] == ' ' || text[i] == '\n' || text[i] == '\r' )
      format_buf[i] = text[i];
    else
      format_buf[i] = '-';

  format_buf[i] = '\0';

  send( ch, format_buf );
}


void fsend_color( char_data* ch, int color, const char* text )
{
  send( ch, color_code( ch, color ) );
  fsend( ch, text );
}


void fsend_color_seen( char_data* ch, int color, const char* text )
{
  if( ch->array && !ch->was_in_room ) {
    for( int i = 0; i < *ch->array; i++ ) {
      char_data *rch;
      if( ( rch = character( ch->array->list[i] ) )
	  && rch != ch
	  && ch->Seen( rch ) ) {
	fsend_color( rch, color, text );
      }
    }
  }
}


void fsend_color( thing_array& array, int color, const char* text )
{
  for( int i = 0; i < array; i++ ) {
    char_data *rch;
    if( ( rch = character( array[i] ) )
	&& rch->Can_See() )
      fsend_color( rch, color, text );
  }
} 


void page_header( char_data* ch, const char* text )
{
  link_data *link;

  if( ch && ( link = ch->link ) ) {
    text_data *paged = link->paged;
    link->paged = 0; 
    page( ch, text );
    cat( link->paged, paged );
  }
}


void page_divider( char_data* ch, const char* text, int i )
{
  page( ch, "\n\r" );
  page_centered( ch, "-- %c%s (%d) --", toupper(*text), text+1, i );
  page( ch, "\n\r" );
}


void send( thing_array& array, const char *text )
{
  for( int i = 0; i < array; ++i ) {
    char_data *rch;
    if( ( rch = character( array[i] ) )
	&& rch->Can_See() )
      send( rch, text );
  }
}


void fsend_all( room_data* room, const char* text )
{
  for( int i = 0; i < room->contents; ++i ) {
    char_data *rch;
    if( ( rch = character( room->contents[i] ) )
	&& room->Seen( rch ) )
      fsend( rch, text );
  }
}
