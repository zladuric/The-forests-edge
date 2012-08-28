#ifndef tfe_network_h
#define tfe_network_h


/*
 *   NETWORK ROUTINES
 */


#define CON_ACNT_REQUEST           -8
#define CON_ACNT_MENU              -9
#define CON_ACNT_NAME             -10
#define CON_ACNT_PWD              -11
#define CON_ACNT_EMAIL            -12
#define CON_ACNT_ENTER            -13
#define CON_ACNT_CONFIRM          -14
#define CON_ACNT_CHECK            -15
#define CON_ACNT_CHECK_PWD        -16
#define CON_UNDEFINED              -4
#define CON_POLICIES               -3
#define CON_FEATURES               -2
#define CON_PAGE                   -1
#define CON_PLAYING                 0
#define CON_INTRO                   1 
#define CON_PASSWORD_ECHO           2
#define CON_PASSWORD_NOECHO         3
#define CON_DISC_OLD                4
#define CON_GET_NEW_NAME            5
#define CON_GET_NEW_PASSWORD        6
#define CON_CONFIRM_PASSWORD        7
#define CON_SET_TERM                8
#define CON_READ_GAME_RULES        10
#define CON_AGREE_GAME_RULES       11
#define CON_GET_EMAIL              12
#define CON_HELP_SEX               13
#define CON_GET_NEW_SEX            14 
#define CON_HELP_CLSS              15
#define CON_GET_NEW_CLSS           16
#define CON_HELP_RACE              17
#define CON_GET_NEW_RACE           18
#define CON_DECIDE_STATS           19
#define CON_GET_NEW_ALIGNMENT      20
#define CON_HELP_ALIGNMENT         21
#define CON_READ_IMOTD             22
#define CON_READ_MOTD              23
#define CON_CLOSING_LINK           24
#define CON_CE_ACCOUNT             25
#define CON_CE_PASSWORD            26
#define CON_CE_EMAIL               27
#define CON_VE_ACCOUNT             28
#define CON_VE_VALIDATE            29
#define CON_VE_CONFIRM             30
#define CON_ACNT_RESEND_REQ        31
#define CON_ACNT_RESEND            32


class text_data
{
public:
  text_data *next;
  String message;
  char *ptr;
  char *cont;
  
  text_data( const char* text, bool split = false )
    : next(0), message( text, split ), ptr( message.text ), cont(0) {
    record_new( sizeof( text_data ), -MEM_LINK );
    record_new( message.length+1, -MEM_LINK );
  }
  
  ~text_data( ) {
    record_delete( sizeof( text_data ), -MEM_LINK );
    record_delete( message.length+1, -MEM_LINK );
  }
  
  size_t remain( ) const {
    return message.length - ( ptr - message.text );
  }
};


class link_data
{
public:
  link_data*          next;       //points to the next link
  link_data*      snoop_by;       //points to who is snooping on the link
  char_data*     character;       //points to the character using the link
  player_data*      player;       //points to the player using the link
  pfile_data*        pfile;       //points to the player file of the player
  account_data*    account;       //points to the characters account
  char*               host;       //contains the characters host string
  int              channel;       //the channel the character is using
  int            connected;       //the links connected status
  int                 idle;       //how long the link has been idling
  text_data*       receive;       //the next command that has been recieved
  text_data*          send;       //the next thing to be sent to the link
  text_data*         paged;
  const char*        rec_pending;       //any pending stuff that has been received
  const char*           rec_prev;       //the previous command used by link
  bool             command;       //is there a command to be executed?
  bool               again;       //did a write return EAGAIN?
  bool             newline;
  bool            prompted;

  // Telnet stuff.
  bool in_command;
  bool in_option;
  bool after_iac;
  text_data *send_telnet;
  unsigned char cmd;

  link_data( )
    : next(0), snoop_by(0), character(0), player(0),
      pfile(0), account(0), host(0), channel(-1), connected(CON_UNDEFINED),
      idle(0), receive(0), send(0), paged(0),
      rec_pending(empty_string), rec_prev(empty_string),
      command(false), again(false), newline(true), prompted(false),
      in_command(false), in_option(false), after_iac(false), send_telnet(0)
  {
    record_new( sizeof( link_data ), MEM_LINK );
  }

  ~link_data( ) {
    record_delete( sizeof( link_data ), MEM_LINK );
    delete_list( receive );
    delete_list( send ); 
    delete_list( paged );
    free_string( host, MEM_LINK );
    free_string( rec_prev, MEM_LINK );
    free_string( rec_pending, MEM_LINK );
  }

  bool past_password () const;
  void set_playing();
};


extern link_data* link_list;


extern const char  echo_off_str  [];
extern const char  echo_on_str   [];
extern const char  go_ahead_str  [];
extern const char  refuse_tm_str [];


extern int mud_socket;
extern int who_socket;
extern int port;
extern unsigned who_calls;


int    open_port            ( int );
void   new_player           ( player_data* );
bool   check_parse_name     ( link_data*, const char * );
void   prompt_ansi          ( link_data* );
void   prompt_nml           ( link_data* );
void   write_greeting       ( link_data* );
void   send_to_status       ( char_data*, char* );
void   close_socket         ( link_data*, bool = false );
void   kill_daemon          ( );


/*
 *   LOGIN ROUTINES
 */

extern bool  godlock;    
extern bool  wizlock;    


void   press_return             ( link_data* );
bool   no_input                 ( link_data*, const char * );
void   nanny                    ( link_data*, const char * );
void   nanny_intro              ( link_data*, const char * );
void   nanny_acnt_name          ( link_data*, const char * );
void   nanny_acnt_password      ( link_data*, const char * );
void   nanny_acnt_email         ( link_data*, const char * );
void   nanny_acnt_enter         ( link_data*, const char * );
void   nanny_acnt_confirm       ( link_data*, const char * );
void   nanny_acnt_check         ( link_data*, const char * );
void   nanny_acnt_check_pwd     ( link_data*, const char * );
void   nanny_old_password       ( link_data*, const char * );
void   nanny_motd               ( link_data*, const char * );
void   nanny_imotd              ( link_data*, const char * );
void   nanny_new_name           ( link_data*, const char * );
void   nanny_acnt_menu          ( link_data*, const char * );
void   nanny_acnt_request       ( link_data*, const char * );
void   nanny_new_password       ( link_data*, const char * ); 
void   nanny_confirm_password   ( link_data*, const char * );     
void   nanny_set_term           ( link_data*, const char * );
void   nanny_agree_rules        ( link_data*, const char * );
void   nanny_show_rules         ( link_data*, const char * );
void   nanny_alignment          ( link_data*, const char * );
void   nanny_help_alignment     ( link_data*, const char * );
void   nanny_disc_old           ( link_data*, const char * );
void   nanny_help_class         ( link_data*, const char * );
void   nanny_class              ( link_data*, const char * );
void   nanny_help_race          ( link_data*, const char * );
void   nanny_race               ( link_data*, const char * );
void   nanny_stats              ( link_data*, const char * );
void   nanny_help_sex           ( link_data*, const char * );
void   nanny_sex                ( link_data*, const char * );
void   nanny_ve_validate        ( link_data*, const char * );


/*
 *   HOST ROUTINES
 */

bool   init_daemon          ( );
void   write_host           ( link_data*, const struct in_addr& );
void   read_host            ( );


/* 
 *   BAN ROUTINES
 */


bool   is_banned            ( const char* );
bool   is_banned            ( account_data*, link_data* = 0 );


#endif // tfe_network_h
