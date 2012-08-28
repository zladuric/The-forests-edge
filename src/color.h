#ifndef tfe_color_h
#define tfe_color_h

#define TERM_DUMB                   0
#define TERM_VT100                  1
#define TERM_ANSI                   2
#define MAX_TERM                    3


#define COLOR_DEFAULT               0
#define COLOR_ROOM_NAME             1
#define COLOR_TELLS                 2
#define COLOR_SAYS                  3
#define COLOR_GOSSIP                4
#define COLOR_WEATHER               5
#define COLOR_WIZARD                6
#define COLOR_SKILL                 7
#define COLOR_CHANT                 8
#define COLOR_NEWBIE                9
#define COLOR_TITLES               10
#define COLOR_CTELL                11
#define COLOR_CHAT                 12
#define COLOR_OOC                  13
#define COLOR_GTELL                14
#define COLOR_AUCTION              15
#define COLOR_INFO                 16
#define COLOR_TO_SELF              17
#define COLOR_TO_GROUP             18
#define COLOR_BY_SELF              19
#define COLOR_BY_GROUP             20

#define COLOR_MILD                 21
#define COLOR_STRONG               22
#define COLOR_BLACK                23
#define COLOR_RED                  24
#define COLOR_BOLD_RED             25
#define COLOR_GREEN                26
#define COLOR_BOLD_GREEN           27
#define COLOR_YELLOW               28
#define COLOR_BOLD_YELLOW          29
#define COLOR_BLUE                 30
#define COLOR_BOLD_BLUE            31
#define COLOR_MAGENTA              32
#define COLOR_BOLD_MAGENTA         33
#define COLOR_CYAN                 34
#define COLOR_BOLD_CYAN            35
#define COLOR_WHITE                36
#define COLOR_BOLD_WHITE           37

#define COLOR_REVERSE              38
#define COLOR_UNDERLINE            39
#define COLOR_EMOTE                40
#define COLOR_SOCIAL               41
#define COLOR_INPUT                42

#define MAX_COLOR                  43

#define SAVED_COLORS               45

#define ANSI_NORMAL                 0
#define ANSI_BOLD                   1
#define ANSI_REVERSE                7
#define ANSI_UNDERLINE              4
#define ANSI_BLACK                 30
#define ANSI_RED                   31
#define ANSI_GREEN                 32
#define ANSI_YELLOW                33
#define ANSI_BLUE                  34
#define ANSI_MAGENTA               35
#define ANSI_CYAN                  36
#define ANSI_WHITE                 37
#define ANSI_BOLD_RED              (64*ANSI_BOLD+ANSI_RED)
#define ANSI_BOLD_GREEN            (64*ANSI_BOLD+ANSI_GREEN)
#define ANSI_BOLD_YELLOW           (64*ANSI_BOLD+ANSI_YELLOW)
#define ANSI_BOLD_BLUE             (64*ANSI_BOLD+ANSI_BLUE)
#define ANSI_BOLD_MAGENTA          (64*ANSI_BOLD+ANSI_MAGENTA)
#define ANSI_BOLD_CYAN             (64*ANSI_BOLD+ANSI_CYAN)
#define ANSI_BOLD_WHITE            (64*ANSI_BOLD+ANSI_WHITE)


#define VT100_NORMAL                0
#define VT100_BOLD                  1
#define VT100_REVERSE               2
#define VT100_UNDERLINE             3
#define MAX_VT100                   4


typedef const char* term_func  ( int );


class Term_Type
{
  public:
    char*                name;
    int               entries;
    const char**       format;
    term_func*          codes;
    const int*       defaults;
};


extern  const char*      format_vt100  [ ];
extern  const char*      codes_vt100   [ ];
extern  const char*      format_ansi   [ ];
extern  const char*      codes_ansi    [ ];
extern  const char*      color_fields  [ ];
extern  const term_type  term_table    [ ];
extern  const char*      color_key;


const char*   bold_red_v       ( char_data* );
const char*   bold_cyan_v      ( char_data* );
const char*   bold_magenta_v      ( char_data* );
const char*   bold_green_v      ( char_data* );
const char*   bold_v           ( char_data* );


const char *color_reverse( char_data*, int );


#define normal( ch )           color_code( ch, COLOR_DEFAULT )
#define red( ch )              color_code( ch, COLOR_RED )
#define green( ch )            color_code( ch, COLOR_GREEN )
#define yellow( ch )           color_code( ch, COLOR_YELLOW )
#define blue( ch )             color_code( ch, COLOR_BLUE )
#define magenta( ch )          color_code( ch, COLOR_MAGENTA )
#define cyan( ch )             color_code( ch, COLOR_CYAN )
#define to_self( ch )          color_code( ch, COLOR_TO_SELF )
#define by_self( ch )          color_code( ch, COLOR_BY_SELF )


const char*   color_scale      ( char_data*, int );
 
unsigned      convert_to_ansi  ( char_data*, size_t max, const char*, char*, int = COLOR_DEFAULT );  
void          send_color       ( char_data*, int, const char* );
void          page_color       ( char_data*, int, const char* );


const char* color_code( const char_data*, int );


/*
 *   SCREEN ROUTINES
 */


void  scroll_window  ( char_data* );
void  command_line   ( char_data* );
void  setup_screen   ( char_data* );
void  clear_screen   ( char_data* );
void  reset_screen   ( char_data* );
void set_window_title ( char_data* );

void save_cursor      ( char_data* ch );
void restore_cursor   ( char_data* ch );
void cursor_on        ( char_data* ch );
void cursor_off       ( char_data* ch );
void lock_keyboard    ( char_data* ch );
void unlock_keyboard  ( char_data* ch );


void move_cursor( char_data* ch, int line, int column );
void scroll_region( char_data* ch, int top, int bottom );


#endif // tfe_color_h
