#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include "define.h"
#include "struct.h"


const char *color_key = "neEKrRgGyYbBmMcCwWvu";

const char* color_fields [ MAX_COLOR ] = {
  "default", "room.name", "tell", "say",
  "gossip", "weather", "wizard", "skill",
  "chant", "newbie", "titles", "ctell",
  "chat", "ooc", "gtell", "auction",
  "info", "to.self", "to.group", "by.self",
  "by.group", "mild.emphasis", "strong.emphasis", "black",
  "red1", "red2", "green1", "green2",
  "yellow1", "yellow2", "blue1", "blue2",
  "magenta1", "magenta2", "cyan1", "cyan2",
  "white1", "white2", "reverse", "underline",
  "emote", "social", "input"
};

const char* format_vt100 [] = {
  "none", "bold", "reverse", "underline" };

const int defaults_vt100 [ MAX_COLOR ] = {
  VT100_NORMAL,     VT100_BOLD,   VT100_REVERSE,  VT100_NORMAL,
  VT100_BOLD,       VT100_NORMAL, VT100_NORMAL,   VT100_NORMAL,
  VT100_NORMAL,     VT100_BOLD,   VT100_BOLD,     VT100_NORMAL,
  VT100_NORMAL,     VT100_BOLD,   VT100_NORMAL,   VT100_UNDERLINE,
  VT100_NORMAL,     VT100_BOLD,   VT100_NORMAL,   VT100_UNDERLINE,
  VT100_UNDERLINE,  VT100_BOLD,   VT100_REVERSE,  VT100_NORMAL,
  VT100_NORMAL,     VT100_NORMAL, VT100_NORMAL,   VT100_NORMAL,
  VT100_NORMAL,     VT100_NORMAL, VT100_NORMAL,   VT100_NORMAL,
  VT100_NORMAL,     VT100_NORMAL, VT100_NORMAL,   VT100_NORMAL,
  VT100_NORMAL,     VT100_NORMAL, VT100_REVERSE,  VT100_UNDERLINE,
  VT100_NORMAL,     VT100_NORMAL, VT100_NORMAL
};

const int defaults_ansi [ MAX_COLOR ] = {
  ANSI_NORMAL,      ANSI_BOLD_RED,     ANSI_GREEN,      ANSI_BOLD,
  ANSI_BOLD_YELLOW, ANSI_BLUE,         ANSI_MAGENTA,    ANSI_CYAN,
  ANSI_RED,         ANSI_BOLD,         ANSI_BOLD,       ANSI_YELLOW,
  ANSI_BOLD_MAGENTA,ANSI_BOLD_CYAN,    ANSI_BOLD_GREEN, ANSI_WHITE,
  ANSI_BLUE,        ANSI_BOLD_RED,     ANSI_RED,        ANSI_BOLD_GREEN,  
  ANSI_GREEN,       ANSI_BOLD,         ANSI_REVERSE,    ANSI_BLACK, 
  ANSI_RED,         ANSI_BOLD_RED,     ANSI_GREEN,      ANSI_BOLD_GREEN,
  ANSI_YELLOW,      ANSI_BOLD_YELLOW,  ANSI_BLUE,       ANSI_BOLD_BLUE,
  ANSI_MAGENTA,     ANSI_BOLD_MAGENTA, ANSI_CYAN,       ANSI_BOLD_CYAN,
  ANSI_WHITE,       ANSI_BOLD_WHITE,   ANSI_REVERSE,    ANSI_UNDERLINE,
  ANSI_BOLD_MAGENTA,ANSI_BOLD_CYAN,    ANSI_NORMAL
};

const char *format_ansi [] = {
  "bold", "reverse", "blinking", "underlined",
  "black.text", "red.text", "green.text", "yellow.text",
  "blue.text", "magenta.text", "cyan.text", "white.text",
  "black.bg", "red.bg", "green.bg", "yellow.bg", "blue.bg",
  "magenta.bg", "cyan.bg", "white.bg" };

const int index_ansi [] = {
   1,  7,  5,  4,
  30, 31, 32, 33,
  34, 35, 36, 37,
  40, 41, 42, 43,
  44, 45, 46, 47 };

term_func ansi_codes;
term_func vt100_codes;


const term_type term_table [] =
{
  { "dumb",   0,  0,             0,            0               },
  { "vt100",  4,  format_vt100,  vt100_codes,  defaults_vt100  },
  { "ansi",  20,  format_ansi,   ansi_codes,   defaults_ansi   }
};


/*
 *   SCREEN ROUTINES
 */


void save_cursor( char_data* ch )
{
  send( ch, "7" );
}


void restore_cursor( char_data* ch )
{
  send( ch, "8" );
}


void cursor_on( char_data* ch )
{
  send( ch, "[?25h" );
}


void cursor_off( char_data* ch )
{
  send( ch, "[?25l" );
}


void lock_keyboard( char_data* ch )
{
  send( ch, "[2l" );
}


void unlock_keyboard( char_data* ch )
{
  send( ch, "[2h" );
}


void move_cursor( char_data* ch, int line, int column )
{
  send( ch, "[%d;%dH", line, column );
}
  

void scroll_region( char_data* ch, int top, int bottom )
{
  send( ch, "[%d;%dr", top, bottom );
}


/*
 *   ANSI
 */


const char *ansi_colors( int code )
{
  char*   tmp  = static_string( );
  int     i,j;

  if( code == 0 ) 
    return "none";

  *tmp = '\0';
  
  for( i = 0; code != 0 && i < 5; i++ ) {
    for( j = 0; code%64 != index_ansi[j]; j++ )
      if( j == term_table[ TERM_ANSI ].entries-1 ) 
        return "Impossible Index??";
    int l = strlen(tmp);
    snprintf( tmp+l, THREE_LINES-l, "%s%s", *tmp == '\0' ? "" : " ",
	      format_ansi[j] );
    code = code >> 6;
  }
  
  return tmp;
}


const char *ansi_codes( int code )
{
  static char tmp [ 200 ];
  static int pntr = 0;

  bool found = false;
  char *t = 0;

  for( int i = 0; code != 0 && i < 5; ++i ) {
    const int c = code%64;
    if( c != ANSI_NORMAL ) {
      if( !found ) {
	pntr = (pntr+20)%200;
	t = tmp+pntr;
	t += sprintf( t, "[0" );
	found = true;
      }
      t += sprintf( t, ";%d", c );
    }
    code >>= 6;
  }
  
  if( !found )
    return "[0m";

  *t++ = 'm';
  *t++ = '\0';

  return tmp+pntr;
}


/*
 *   VT100
 */


const char *vt100_codes( int i )
{
  const char *const escape_codes [] = { "[0m","[1m", "[7m", "[4m" };

  return escape_codes[i];
}


/*
 *   MAIN COLOR ROUTINE
 */


void do_color( char_data* ch, const char *argument )
{
  char      tmp1  [ MAX_INPUT_LENGTH ];
  int       i, j;
  int       code;

  if( !ch->link ) 
    return;

  int *color = ch->pcdata->color;
  int term = ch->pcdata->terminal;

  if( !*argument ) {
    const unsigned columns = ch->pcdata->columns / 19;
    page_title( ch, "Terminal" );
    for( i = 0; i < MAX_TERM; ++i ) {
      page( ch, "%15s (%c)%s", term_table[i].name,
	    term == i ? '*' : ' ',
	    i%columns == columns-1 ? "\n\r" : "" );
      }
    if( i%columns != 0 )
      page( ch, "\n\r" );

    page( ch, "\n\r" );

    if( term == TERM_DUMB )
       return;

    int len = snprintf( tmp1, MAX_INPUT_LENGTH, "%15s (%hd)",
			"Lines",
			ch->pcdata->lines );
    snprintf( tmp1+len, MAX_INPUT_LENGTH-len, "%*s (%hd)\n\r\n\r",
	      34-len,
	      "Columns",
	      ch->pcdata->columns );
    page( ch, tmp1 );

    page_title( ch, "Color Options" );

    for( i = 0; i < term_table[term].entries; i++ ) {
      page( ch, "%19s%s", term_table[term].format[i],
	    i%columns == columns-1 ? "\n\r" : "" );
    }
    if( i%columns != 0 )
      page( ch, "\n\r" );

    page( ch, "\n\r" );
    
    page_title( ch, "Color Settings" );
    for( i = 0; i < MAX_COLOR; ++i ) {
      page( ch, "%16s : %s%s%s\n\r", color_fields[i],
	    color_code( ch, i ),
	    term == TERM_VT100
	    ? term_table[ TERM_VT100 ].format[ ch->pcdata->color[i] ]
	    : ansi_colors( ch->pcdata->color[i] ),
	    normal( ch ) );
    }
    
    return;
  }

  if( matches( argument, "lines" ) ) {
    if( ( i = atoi( argument ) ) < 10 || i > 500 ) {
      send( ch, "Number of screen lines must be from 10 to 500.\n\r" );
      return;
    }
    ch->pcdata->lines = i;
    setup_screen( ch );
    send( ch, "Number of screen lines set to %d.\n\r", i );
    return;
  }

  if( matches( argument, "columns" ) ) {
    if( ( i = atoi( argument ) ) < 40 || i > 300 ) {
      send( ch, "Number of screen columns must be from 40 to 300.\n\r" );
      return;
    }
    ch->pcdata->columns = i;
    //    setup_screen( ch );
    send( ch, "Number of screen columns set to %d.\n\r", i );
    return;
  }

  for( i = 0; i < MAX_TERM; ++i )
    if( matches( argument, term_table[i].name ) ) {
      if( i != TERM_DUMB ) {
        for( j = 0; j < MAX_COLOR; ++j ) {
          color[j] = term_table[i].defaults[j];
	}
      }
      reset_screen( ch );
      ch->pcdata->terminal = i;
      setup_screen( ch );
      send( ch, "Terminal type set to %s.\n\r", term_table[i].name );
      return;
    }

  for( i = 0; i < MAX_COLOR; ++i )
    if( matches( argument, color_fields[i] ) )
      break;
 
  if( i == MAX_COLOR ) {
    send( ch, "Unknown item to color terminal type.\n\r" );
    return;
  }

  if( !*argument ) {
    send( ch, "%s - current setting: %s%s%s\n\r",
	  color_fields[i], color_code( ch, i ), "sample", normal( ch ) );
    send( ch, "To what color do you wish to set %s?\n\r",
	  color_fields[i] );
    return;
  }

  if( !strcasecmp( argument, "none" ) ) {
    ch->pcdata->color[i] = 0;
    send( ch, "Color for %s removed.\n\r", color_fields[i] );
    return;
  }

  if( term == TERM_ANSI ) {
    for( code = 0; *argument; ) {
      for( j = 0; !matches( argument, term_table[term].format[j] ); j++ ) {
        if( j == term_table[term].entries-1 ) {
          send( ch, "Unknown ansi format.\n\r" );
          return;
	} 
      }
      code = ( code << 6 )+index_ansi[j];
    }
    ch->pcdata->color[i] = code;
    send( ch, "Color for %s set to %s.\n\r",
	  color_fields[i], term == TERM_VT100
	  ? term_table[ TERM_VT100 ].format[code] : ansi_colors( code ) );
    return;
  }
  
  for( j = 0; j < term_table[term].entries; j++ )
    if( matches( argument, term_table[term].format[j] ) )
      break;

  if( j == term_table[term].entries ) {
    send( ch, "Unknown format.\n\r" );
    return;
  }

  ch->pcdata->color[i] = j;

  send( ch, "Format of %s set to %s.\n\r",
	color_fields[i], term_table[term].format[j] );
}
  

/*
 *   CONVERT TEXT TO COLOR CODES
 */


unsigned convert_to_ansi( char_data *ch, size_t max,
			  const char *input, char *output,
			  int normal )
{
  if( !ch->pcdata ) {
    *output = '\0';
    return 0;
  }

  const int term = ch->pcdata->terminal;

  char n [ THREE_LINES ];
  strcpy( n, color_code( ch, normal ) );
  const size_t nl = strlen( n );

  bool color = false;
  unsigned length = 0;

  for( ; *input; input++ ) {
    if( *input != '@' ) {
      if( length + 1 < max ) {
	*output++ = *input;
	++length;
      }
      continue;
    }
    
    if( !*++input ) {
      if( length + 1 < max ) {
	*output++ = '@';
	++length;
      }
      break;
    }
    
    const char c = *input;

    switch( c ) {
    case '@' : 
      if( length + 1 < max ) {
	*output++ = '@';
	++length;
      }
      continue;
      
    case 'I' :
      if( length + 2 < max ) {
	*output++ = ' ';
	*output++ = ' ';
	length += 2;
      }
      continue;
    }
    
    if( term == TERM_DUMB )
      continue;
    
    for( int i = 0; color_key[i]; ++i ) {
      if( color_key[i] == c ) {
	const bool nocolor = i == 0 || ( COLOR_MILD+i-1 == normal );
	const char *const s = i == 0 ? n : color_code( ch, COLOR_MILD+i-1 );
	const size_t l = i == 0 ? nl : strlen( s );
	const size_t x =  nocolor ? 0 : nl;	// Leave space for return to default color.
	if( length + l + x + 1 < max ) {
	  strcpy( output, s );
	  output += l;
	  color = !nocolor;
	  //	  color = ( i != 0 ) || normal != COLOR_DEFAULT;
	}
	break;
      }
    }
  }
  
  if( term == TERM_DUMB ) {
    *output = '\0';
    return length;
  }
  
  if( color ) {
    strcpy( output, n );
  } else {
    *output = '\0';
  }

  return length;
}     


/*
 *  WINDOW OPERATIONS
 */


void scroll_window( char_data* ch )
{
  if( !ch->pcdata )
    return;

  int lines = ch->pcdata->lines;

  set_bit( ch->status, STAT_NO_SNOOP );
  lock_keyboard( ch );
  save_cursor( ch );
  cursor_off( ch );
  scroll_region( ch, 1, lines-2 );
  move_cursor( ch, lines-2, 1 );
  remove_bit( ch->status, STAT_NO_SNOOP );
}


void command_line( char_data* ch )
{
  if( !ch->pcdata )
    return;

  int lines = ch->pcdata->lines;

  set_bit( ch->status, STAT_NO_SNOOP );
  scroll_region( ch, lines, lines );
  restore_cursor( ch );
  cursor_on( ch );
  unlock_keyboard( ch );
  remove_bit( ch->status, STAT_NO_SNOOP );
}


void setup_screen( char_data* ch )
{
  if( !ch->pcdata )
    return;

  int lines = ch->pcdata->lines;

  reset_screen( ch );
  clear_screen( ch );

  if( ch->pcdata->terminal == TERM_DUMB 
      || !is_set( ch->pcdata->pfile->flags, PLR_STATUS_BAR ) ) 
    return;
 
  set_bit( ch->status, STAT_NO_SNOOP );
  move_cursor( ch, lines, 1 );
  remove_bit( ch->status, STAT_NO_SNOOP );

  scroll_window( ch );
}


void clear_screen( char_data* ch )
{
  if( !ch->pcdata )
    return;

  set_bit( ch->status, STAT_NO_SNOOP );

  if( ch->pcdata->terminal != TERM_DUMB )
    send( ch, "[2J[1;1H" );
  else
    send( ch, "\n\r" );

 remove_bit( ch->status, STAT_NO_SNOOP );
}


void reset_screen( char_data* ch )
{
  if( !ch->pcdata )
    return;

  if( ch->pcdata->terminal != TERM_DUMB ) {
    set_bit( ch->status, STAT_NO_SNOOP );
    if( is_set( ch->pcdata->pfile->flags, PLR_STATUS_BAR ) ) {
      send( ch, "[r" );  // Undo scroll_region().
      //    send( ch, "c" );
      //    send( ch, "[2J[1;1H" );
    }
    if( is_set( ch->pcdata->pfile->flags, PLR_WINDOW_NAME ) ) {
      send( ch, "]2;" );
    }
    remove_bit( ch->status, STAT_NO_SNOOP );
  }
}


void set_window_title( char_data *ch )
{
  if( !ch || !ch->pcdata )
    return;

  if( ch->pcdata->terminal != TERM_DUMB
      && is_set( ch->pcdata->pfile->flags, PLR_WINDOW_NAME ) ) {
    set_bit( ch->status, STAT_NO_SNOOP );
    send( ch, "]2;TFE: %s",
	  ch->in_room->Name( ch ) );
    remove_bit( ch->status, STAT_NO_SNOOP );
  }
}


/*
 *   SEND_COLOR
 */


void send_color( char_data* ch, int type, const char* msg )
{
  if( ch->pcdata ) {
    int term = ch->pcdata->terminal;

    if( term != TERM_DUMB ) {
      int color = ch->pcdata->color[type];
      send( ch, term_table[term].codes( color ) );
      send( ch, msg );
      if( color != COLOR_DEFAULT )
	send( ch, term_table[term].codes( ch->pcdata->color[ COLOR_DEFAULT ] ) );
      return;
    }
  }
  
  send( ch, msg );
}


void page_color( char_data* ch, int type, const char* msg )
{
  if( ch->pcdata ) {
    int term = ch->pcdata->terminal;

    if( term != TERM_DUMB ) {
      int color = ch->pcdata->color[type];
      page( ch, term_table[term].codes( color ) );
      page( ch, msg );
      if( color != COLOR_DEFAULT )
	page( ch, term_table[term].codes( ch->pcdata->color[ COLOR_DEFAULT ] ) );
      return;
    }
  }

  page( ch, msg );
}


const char *color_reverse( char_data *ch, int color )
{
  if( !ch->pcdata )
    return empty_string;
  
  if( ch->pcdata->terminal == TERM_VT100 ) {
    return vt100_codes( VT100_REVERSE );
  } else if( ch->pcdata->terminal == TERM_ANSI ) {
    int col = ch->pcdata->color[ color ];
    return ansi_codes( ( col << 6 ) + ANSI_REVERSE );
  }
  
  return empty_string;
}


/*
 *   VT100 COLOR ROUTINES
 */


const char* bold_v( char_data* ch )
{
  if( !ch->pcdata )
    return empty_string;
  
  switch( ch->pcdata->terminal ) {
  case TERM_VT100:  return vt100_codes( VT100_BOLD );
  case TERM_ANSI:   return  ansi_codes( ANSI_BOLD*64+ANSI_WHITE );
  }
  
  return empty_string;
}


const char* bold_red_v( char_data* ch )
{
  if( !ch->pcdata )
    return empty_string;
  
  switch( ch->pcdata->terminal ) {
  case TERM_VT100:  return vt100_codes( VT100_BOLD );
  case TERM_ANSI:   return  ansi_codes( ANSI_BOLD*64+ANSI_RED );
  }
  
  return empty_string;
}


const char* bold_cyan_v( char_data* ch )
{
  if( !ch->pcdata )
    return empty_string;
  
  switch( ch->pcdata->terminal ) {
  case TERM_VT100:  return vt100_codes( VT100_BOLD );
  case TERM_ANSI:   return  ansi_codes( ANSI_BOLD*64+ANSI_CYAN );
  }
  
  return empty_string;
}


const char* bold_magenta_v( char_data* ch )
{
  if( !ch->pcdata )
    return empty_string;
  
  switch( ch->pcdata->terminal ) {
  case TERM_VT100:  return vt100_codes( VT100_BOLD );
  case TERM_ANSI:   return  ansi_codes( ANSI_BOLD*64+ANSI_MAGENTA );
  }
  
  return empty_string;
}


const char* bold_green_v( char_data* ch )
{
  if( !ch->pcdata )
    return empty_string;
  
  switch( ch->pcdata->terminal ) {
  case TERM_VT100:  return vt100_codes( VT100_BOLD );
  case TERM_ANSI:   return  ansi_codes( ANSI_BOLD*64+ANSI_GREEN );
  }
  
  return empty_string;
}


/*
 *   ANSI COLOR ROUTINES
 */


/*
const char* normal( char_data* ch )
{
  if( !ch->pcdata || ch->pcdata->terminal == TERM_DUMB )
    return empty_string;

  return term_table[ ch->pcdata->terminal ].codes( ch->pcdata->color[ COLOR_DEFAULT ] );
}


const char* red( char_data* ch )
{
  if( !ch->pcdata || ch->pcdata->terminal == TERM_DUMB )
    return empty_string;

  return term_table[ ch->pcdata->terminal ].codes( ch->pcdata->color[ COLOR_RED ] );
}


const char* green( char_data *ch )
{
  if( !ch->pcdata || ch->pcdata->terminal == TERM_DUMB )
    return empty_string;

  return term_table[ ch->pcdata->terminal ].codes( ch->pcdata->color[ COLOR_GREEN ] );
}


const char* blue( char_data* ch )
{
  if( !ch->pcdata || ch->pcdata->terminal == TERM_DUMB )
    return empty_string;

  return term_table[ ch->pcdata->terminal ].codes( ch->pcdata->color[ COLOR_BLUE ] );
}


const char* yellow( char_data* ch )
{
  if( !ch->pcdata || ch->pcdata->terminal == TERM_DUMB )
    return empty_string;

  return term_table[ ch->pcdata->terminal ].codes( ch->pcdata->color[ COLOR_YELLOW ] );
}


const char* magenta( char_data* ch )
{
  if( !ch->pcdata || ch->pcdata->terminal == TERM_DUMB )
    return empty_string;

  return term_table[ ch->pcdata->terminal ].codes( ch->pcdata->color[ COLOR_MAGENTA ] );
}


const char* cyan( char_data* ch )
{
  if( !ch->pcdata || ch->pcdata->terminal == TERM_DUMB )
    return empty_string;

  return term_table[ ch->pcdata->terminal ].codes( ch->pcdata->color[ COLOR_CYAN ] );
}
*/


/*
 *   COLOR SCALE 
 */


static const int scale[] = {
  COLOR_WHITE,		// 0
  COLOR_GREEN,		// 1
  COLOR_CYAN,		// 2
  COLOR_BLUE,		// 3
  COLOR_MAGENTA,	// 4
  COLOR_YELLOW,		// 5
  COLOR_RED		// 6
};


const char *color_scale( char_data* ch, int i )
{
  if( !ch->pcdata || ch->pcdata->terminal != TERM_ANSI )
    return empty_string;

  i = range( 0, i, 6 );

  return term_table[ ch->pcdata->terminal ].codes( ch->pcdata->color[ scale[i] ] );
  //  return term_table[ ch->pcdata->terminal ].codes( ch->pcdata->color[ COLOR_WHITE-2*i ] );
}


/*
 *   BATTLE COLOR FUNCTIONS
 */


const char *damage_color( char_data* rch, char_data* ch, char_data* victim )
{
  int term;

  if( !rch->pcdata || ( term = rch->pcdata->terminal ) == TERM_DUMB )
    return empty_string;

  if( rch == victim )
    return term_table[term].codes( rch->pcdata->color[ COLOR_TO_SELF ] );

  if( is_same_group( rch, victim ) )
    return term_table[term].codes( rch->pcdata->color[ COLOR_TO_GROUP ] );

  if( rch == ch )
    return term_table[term].codes( rch->pcdata->color[ COLOR_BY_SELF ] );

  if( is_same_group( rch, ch ) )
    return term_table[term].codes( rch->pcdata->color[ COLOR_BY_GROUP ] );
 
  return empty_string;
}


/*
const char* by_self( char_data* ch )
{
  if( !ch->pcdata || ch->pcdata->terminal == TERM_DUMB )
    return empty_string;

  return term_table[ ch->pcdata->terminal ].codes( ch->pcdata->color[ COLOR_BY_SELF ] );
}


const char* to_self( char_data* ch )
{
  if( !ch->pcdata || ch->pcdata->terminal == TERM_DUMB )
    return empty_string;

  return term_table[ ch->pcdata->terminal ].codes( ch->pcdata->color[ COLOR_TO_SELF ] );
}
*/


const char *color_code( const char_data *ch, int color )
{
  if( !ch->pcdata || ch->pcdata->terminal == TERM_DUMB )
    return empty_string;
  
  return term_table[ ch->pcdata->terminal ].codes( ch->pcdata->color[ color ] );
}
