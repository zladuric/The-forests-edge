#include <ctype.h>
#include <sys/types.h>
#include <stdio.h>
#include <syslog.h>
#include "define.h"
#include "struct.h"


void  use_start_table    ( char_data*, int );


/*
 *   CHARACTER CREATION ROUTINES
 */


int creation_points( player_data *pl )
{
  int points = 500 + 50 * pl->remort;
  const share_data *const shdata = pl->shdata;
  const int *const bonus  = plyr_race_table[shdata->race].stat_bonus;
  const unsigned char *const stat[] = {
    &shdata->strength, &shdata->intelligence,
    &shdata->wisdom, &shdata->dexterity,
    &shdata->constitution };
  
  for( int j = 0; j < 5; ++j )
    points -= (((int)*stat[j])-bonus[j])*(((int)*stat[j])-bonus[j]+1)/2;

  return points;
}


static int racial_max( player_data *pl, int stat )
{
  const int *const bonus = plyr_race_table[pl->shdata->race].stat_bonus;

  return 18 + bonus[stat] + pl->remort;
}


static void wrong_choice( link_data* link )
{
  send( link, "\n\rWhat you entered was unintelligible.  Please press\
 return and then select\n\rone of the choices given between the left\
 and right brackets.\n\r" );
  press_return( link );
}


/*
 *   NAME
 */


bool check_parse_name( link_data* link, const char *name )
{
  species_data*  species;
  int             length  = strlen( name );
  int           capitals  = 0;

  /* ALPHA AND PUNCTUATION */

  if( ispunct( *name ) || ispunct( name[length] ) ) {
    send( link, "\n\rThe first and last letters of a name may not be\
 punctuation to stop the\n\rsegment of the population who insist on the\
 name --Starbright--.\n\r" );
    return false;
  }
  
  for( const char *letter = name; *letter; ++letter ) {
    if( isdigit( *letter ) ) {
      send( link, "\n\rNames are not allowed to contain numbers.\n\r" );
      return false;
    }
    if( ispunct( *letter ) && *letter != '-' && *letter != '\'' ) {
      send( link, "\n\rNames are not allowed to contain punctuation other\
 than dashes and\n\rapostrophes.\n\r" );
      return false;
    }
    if( isupper( *letter ) && ++capitals > 3 ) {
      send( link, "\n\rNames can contain at most 3 capital letters.\n\r" );
      return false;
    } 
  }
  
  if( length < 3 || length > 12 ) {
    send( link,
	  "\n\rNames must be greater than 2 and less than 13 letters.\n\r" );
    return false;
  }
  
  /* WORDS USED IN GAME */
  
  for( int i = 1; i <= species_max; ++i ) {
    if( !( species = species_list[i] ) )
      continue;
    if( is_name( name, species->descr->name ) ) {
      send( link, "\n\rThat is the name of monster in the game and thus is not allowed.\n\r" );
      return false;
    }
  }
  
  for( int i = 0; i < max_clan; ++i ) {
    if( is_name( name, clan_list[i]->abbrev ) ) {
      send( link, "\n\rThat name cannot be used; choose another.\n\r" );
      return false;
    }
  }

  if( find_pfile_exact( name ) ) {
    send( link, "\n\rA player already exists with that name.\n\r" );
    return false;
  }
  
  if( find_pfile( name ) || find_pfile_substring( name ) ) {
    help_link( link, "Name_Similiar" );
    return false;
  }
  
  if( pntr_search( badname_array, max_badname, name ) >= 0
      || pntr_search( remort_array, max_remort, name ) >= 0 ) {
    help_link( link, "Bad_Name" );
    return false;
  }

  // Check remorts in progress.
  for( int i = 0; i < player_list; ++i ) {
    player_data *pl = player_list[i];
    if( pl->Is_Valid( ) && pl->remort_name ) {
      if( !strcasecmp( pl->remort_name, name )
	  || !strncasecmp( pl->remort_name, name, strlen( name ) )
	  || !strncasecmp( pl->remort_name, name, strlen( pl->remort_name ) ) ) {
	help_link( link, "Bad_Name" );
	return false;
      }
    }
  }

  return true;
}


void nanny_new_name( link_data* link, const char *argument )
{
  if( no_input( link, argument ) )
    return;

  char arg [ MAX_STRING_LENGTH ];
  strcpy( arg, argument );
  *arg = toupper( *arg );

  // Remort.
  player_data *remort = link->player;
  if( remort ) {
    // Remove pfile from list, so check_parse_name works correctly.
    remove_list( pfile_list, max_pfile, remort->pcdata->pfile );
  }

  const bool name_ok = check_parse_name( link, arg );

  if( remort ) {
    add_list( pfile_list, max_pfile, remort->pcdata->pfile );
  }

  if( !name_ok ) {
    send( link, "\n\rSelect another name: " );
    return;
  }

  if( remort ) {
    remort->remort_name = alloc_string( arg, MEM_PLAYER );
    ++remort->remort;
    remort->shdata->strength
      = remort->shdata->intelligence
      = remort->shdata->wisdom
      = remort->shdata->dexterity
      = remort->shdata->constitution
      = 10;
    nanny_help_sex( link, "" );
    return;
  }

  player_data *pl = new player_data( arg );
  pfile_data *pfile = new pfile_data( arg );
  
  pl->pcdata->pfile = pfile;
  pl->link          = link;
  
  link->player      = pl;
  link->character   = pl;
  link->pfile       = pfile;
  link->connected = CON_GET_NEW_PASSWORD;
  
  // Removed... this would put the incomplete player's data in pfile_list.
  //  modify_pfile( pl );
  
  help_link( link, "Login_password" );
  send( link, "Password: " );
}


void nanny_new_password( link_data* link, const char *argument )
{
  //  const char *p;

  if( strlen( argument ) < 5 ) {
    send( link,
      "\n\rPassword must be at least five characters long.\n\rPassword: " );
    return;
  }

  if( strlen( argument ) > 16 ) {
    send( link,
      "\n\rPassword must be at most sixteen characters long.\n\rPassword: " );
    return;
  }

  /*
  for( p = argument; *p; ++p ) {
    if( *p == '~' ) {
      send( link,
        "\n\rNew password not acceptable, try again.\n\rPassword: " );
      return;
    }
  }
  */

  char_data *ch = link->character;

  free_string( ch->pcdata->pfile->pwd, MEM_PFILE );
  ch->pcdata->pfile->pwd = alloc_string( argument, MEM_PFILE );

  send( link, "Please retype password: " );
  link->connected = CON_CONFIRM_PASSWORD;
}


void nanny_confirm_password( link_data* link, const char *argument )
{
  char_data *ch  = link->character;

  send( link, "\n\r" );

  if( strcmp( argument, ch->pcdata->pfile->pwd ) ) {
    send( link, "Passwords don't match, try again.\n\rPassword: " );
    link->connected = CON_GET_NEW_PASSWORD;
    return;
  }

  help_link( link, "Login.Display" );
  send( link, "Display type? [ None Ansi VT52 ]: " );

  link->connected = CON_SET_TERM;
}


/*
 *   TERMINAL TYPE
 */


void nanny_set_term( link_data* link, const char *argument )
{
  char_data* ch  = link->character;

  switch( toupper( *argument ) ) {
  case 'V' :  ch->pcdata->terminal = TERM_VT100;  break;
  case 'A' :  ch->pcdata->terminal = TERM_ANSI;   break;
  default :   ch->pcdata->terminal = TERM_DUMB;   break;
  }

  if( ch->pcdata->terminal != TERM_DUMB ) 
    for( int i = 0; i < MAX_COLOR; ++i )
      ch->pcdata->color[i] = term_table[ ch->pcdata->terminal ].defaults[i];

  setup_screen( ch );

  help_link( link, "Gamedesc_1" );
  press_return( link );

  link->connected = CON_READ_GAME_RULES;
}


/*
 *   GAME RULES
 */


void nanny_show_rules( link_data* link, const char * )
{
  char_data *ch  = link->character;

  clear_screen( ch );

  help_link( link, "GAMEDESC_2" );
  send( link, "\r\nDo you wish to continue (Y/N)? " );

  link->connected = CON_AGREE_GAME_RULES;
}


void nanny_agree_rules( link_data* link, const char *argument )
{
  if( toupper( *argument ) == 'N' ) {
    send( link, "Good bye.\r\n" );
    close_socket( link );
    return;
  }
  
  if( toupper( *argument ) != 'Y' ) {
    send( link, "Please answer Y or N. " );
    return;
  }

  nanny_help_sex( link, "" );
}    


/*
 *   SEX
 */


void nanny_help_sex( link_data* link, const char * )
{
  char_data *ch = link->character;

  clear_screen( ch );

  help_link( link, "login_sex" );
  send( link, "\n\rPlease enter your sex [Male Female]: " );

  link->connected = CON_GET_NEW_SEX;
}


void nanny_sex( link_data* link, const char *argument )
{
  char_data *ch  = link->character;
  
  switch( toupper( *argument ) ) {
  case 'M': ch->sex = SEX_MALE;    break;
  case 'F': ch->sex = SEX_FEMALE;  break;
  default:
    wrong_choice( link );
    link->connected = CON_HELP_SEX;
    return;
  }

  nanny_help_class( link, argument );
}


void nanny_help_class( link_data* link, const char * )
{
  char       tmp  [ TWO_LINES ];
  int          i;
  char_data  *ch  = link->character;

  clear_screen( ch );

  help_link( link, "login_classes" );

  strcpy( tmp, "\n\rSelect a class [" );
  
  for( i = 0; i < MAX_CLSS; i++ ) {
    if( clss_table[i].open ) {
      if( i > 0 )
        strcat( tmp, " " );
      strcat( tmp, clss_table[i].abbrev );
    }
  }
  strcat( tmp, "]: " );

  send( link, tmp );
  link->connected = CON_GET_NEW_CLSS;
}


void nanny_class( link_data* link, const char *argument )
{
  char_data*    ch  = link->character;
  int            i;
  bool        help  = false;

  link->connected = CON_HELP_CLSS;

  if( !*argument ) {
    wrong_choice( link );
    return;
  }
  
  if( *argument == '?' ) {
    ++argument;
    help = true;
  }
  
  for( i = 0; i < MAX_CLSS; i++ ) 
    if( clss_table[i].open 
	&& ( fmatches( argument, clss_table[i].name, 3 )
	     || !strcasecmp( argument, clss_table[i].abbrev ) ) )
      break;
  
  if( i == MAX_CLSS ) {
    wrong_choice( link );
    return;
  }

  if( help ) {
    clear_screen( ch );
    help_link( link, clss_table[i].name );
    press_return( link );
    return;
  }   

  ch->pcdata->clss = i;
  nanny_help_race( link, argument );
}


/*
 *   RACES
 */


bool allowed_race( char_data* ch, int race )
{
  if( !plyr_race_table[race].open ) 
    return false;

  for( int i = 0; i < table_max[ TABLE_ALIGNMENT ]; ++i ) 
    if( is_set( clss_table[ ch->pcdata->clss ].alignments, i ) 
	&& is_set( plyr_race_table[ race ].alignments, i ) ) 
      return true;

  return false;
}


void nanny_help_race( link_data* link, const char * )
{
  char         tmp  [ TWO_LINES ];
  char_data*    ch  = link->character;
  bool       first  = true;
  int            i;

  clear_screen( ch );

  help_link( link, "login_races" );
  strcpy( tmp, "\n\rSelect a race [" );

  for( i = 0; i < MAX_PLYR_RACE; i++ ) {
    if( allowed_race( ch, i ) ) {
      if( !first ) 
        strcat( tmp, " " );
      strcat( tmp, race_table[i].abbrev );
      first = false;
    }
  }

  strcat( tmp, "]: " );
  send( link, tmp );

  link->connected = CON_GET_NEW_RACE; 
}

   
void nanny_race( link_data* link, const char *argument )
{
  char_data*  ch  = link->character;
  int          i;
  bool      help  = false;

  link->connected = CON_HELP_RACE;

  if( *argument == '?' ) {
    ++argument;
    help = true;
  }
  
  if( !*argument ) {
    wrong_choice( link );
    return;
  }
  
  for( i = 0; i < MAX_PLYR_RACE; i++ ) 
    if( plyr_race_table[i].open 
	&& ( fmatches( argument, race_table[i].name, 3 )  
	     || !strcasecmp( argument, race_table[i].abbrev ) ) ) 
      break;
  
  if( i == MAX_PLYR_RACE ) {
    wrong_choice( link );
    return;
  }

  if( help ) {
    clear_screen( ch );
    help_link( link, race_table[i].plural );
    press_return( link );
    return;
  }    
  
  if( !allowed_race( ch, i ) ) {
    send( link, "\n\rA%s %s cannot be a%s %s.\n\r",
	  isvowel( *race_table[i].name ) ? "n" : "",
	  race_table[i].name,
	  isvowel( *clss_table[ch->pcdata->clss].name ) ? "n" : "",
	  clss_table[ch->pcdata->clss].name
	  );
    press_return( link );
    return;
  }
  
  ch->shdata->race = i;

  init_skills( ch );

  /*
  ch->pcdata->speaking = LANG_HUMANIC+i-LANG_FIRST;

  for( int j = 0; j < MAX_SKILL_CAT; ++j ) {
    vzero( ch->shdata->skills[j], table_max[ skill_table_number[ j ] ] );
  }

  ch->shdata->skills[ SKILL_CAT_LANGUAGE ][ skill_number( LANG_HUMANIC+i ) ] = 10;
  ch->shdata->skills[ SKILL_CAT_LANGUAGE ][ skill_number( LANG_PRIMAL ) ] = 10;
  */

  /*
  link->connected = CON_DECIDE_STATS; 
  nanny_stats( link, "" );
  */

  nanny_help_alignment( link, "" );  
}


/*
 *   STATS
 */


void nanny_stats( link_data* link, const char *argument )
{
  player_data *pl = link->player;
  share_data *shdata = pl->shdata;
  const int *const bonus = plyr_race_table[pl->shdata->race].stat_bonus;
  int          number;
  int               i;
  char              c;

  unsigned char *const stat[] = { &shdata->strength, &shdata->intelligence,
				  &shdata->wisdom, &shdata->dexterity, &shdata->constitution };
  const char letter[] = { 'S', 'I', 'W', 'D', 'C' };
  const char *const name[] = { "Strength", "Intelligence", "Wisdom", "Dexterity",
			       "Constitution" };

  if( !strcasecmp( "done", argument ) ) { 
    clear_screen( pl );
    help_link( link, "Introduction" );
    press_return( link );
    link->connected = CON_READ_IMOTD;
    //    nanny_help_alignment( link, "" );
    return; 
  }
  
  clear_screen( pl );
  
  if( strlen( argument ) < 2 || ( strlen( argument ) < 3 
				  && isdigit( argument[1] ) ) ) {
    help_link( link, "login_stats" );
    send( link, "\n\r" );
  } else {
    
    if( isdigit( argument[1] ) ) {
      c = toupper( argument[2] );
      number = argument[1]-'0';
    } else {
      c = toupper( argument[1] );
      number = 1;
    }
    
    for( i = 0; i < 5; i++ ) 
      if( c == letter[ i ] ) 
        break;
    
    if( i != 5 ) {
      switch( argument[0] ) {
      case '+' :
        if( ((int)*stat[i])+number > racial_max( pl, i ) )
          send( link, 
		"\n\rYou can not raise a stat over its racial maximum.\n\r\n\r" );
        else
          *stat[i] += number;
        break;
      case '-' :
        if( *stat[i] < number + 3 )
	  send( link, 
		"\n\rYou can not lower a stat below 3.\n\r\n\r" );
        else
          *stat[i] -= number;
        break;
      case '?' :
	help_link( link, name[i] );
	send( link, "\n\r" );
	break;
      default:
        i = 5;
        break;
      }
    }
   
    if( i == 5 ) {
      help_link( link, "login_stats" );
      send( link, "\n\r" );
    } else {
      const int points = creation_points( pl );
      
      if( points < 0 ) {
        send( link,
	      "\n\rYou don't have enough points for that.\n\r" );
        *stat[i] -= number;
      }
    }
  }
  
  const int points = creation_points( pl );

  send( link, "Points Left: %d\n\r\n\r", points );
  send( link,
	"Ability             Character    Racial Max    Cost to inc.\n\r" );
  send( link,
	"-------             ---------    ----------    ------------\n\r" );
  
  for( i = 0; i < 5; i++ ) {
    send( link, "%-15s%10d%13d%15d\n\r",
	  name[i], *stat[i], racial_max( pl, i ),*stat[i]-bonus[i]+1 );
  }

  send( link, "\n\rEnter choice: " );
}


/*
 *   ALIGNMENT
 */


bool allowed_alignment( char_data* ch, int align )
{
  return( is_set( clss_table[ ch->pcdata->clss ].alignments, align ) 
	  && is_set( plyr_race_table[ ch->shdata->race ].alignments, align ) );  
}


void nanny_help_alignment( link_data* link, const char * )
{
  char_data*    ch  = link->character;
  char         tmp  [ TWO_LINES ];

  strcpy( tmp, "\n\rEnter an alignment [" );

  unsigned j = 0;
  for( int i = 0; i < table_max[ TABLE_ALIGNMENT ]; ++i ) {
    if( allowed_alignment( ch, i ) ) {
      if( j != 0 ) 
        strcat( tmp, " " );
      strcat( tmp, alignment_table[i].abbrev );
      ++j;
    }
  }

  clear_screen( ch );
  
  if( j == 0 ) {
    bug( "No valid alignment for character creation." );
    bug( "-- Class = %s.", clss_table[ ch->pcdata->clss ].name );
    bug( "--  Race = %s.", race_table[ ch->shdata->race ].name );
    send( link, "\n\rNo valid alignment found for your chosen class and race.\n\r" );
    send( link, "\n\rPlease choose again.\n\r" );
    press_return( link );
    link->connected = CON_GET_NEW_CLSS;
    return;
  }

  help_link( link, "login_alignment" );
  
  strcat( tmp, "]: " );
  send( link, tmp );

  link->connected = CON_GET_NEW_ALIGNMENT;
} 


void nanny_alignment( link_data* link, const char *argument )
{
  player_data *pl  = link->player;
  int              i;

  const char*   word  = empty_string; 
  bool          help  = false;

  link->connected = CON_HELP_ALIGNMENT;

  if( *argument == '?' ) {
    ++argument;
    help = true;
  }
  
  if( !*argument ) {
    wrong_choice( link );
    return;
  }
  
  for( i = 0; i < table_max[ TABLE_ALIGNMENT ]; ++i ) {
    if( !strcasecmp( argument, alignment_table[i].abbrev )
	|| fmatches( argument, alignment_table[i].name, 3 ) )
      break;
  }

  if( i == table_max[ TABLE_ALIGNMENT ] ) {
    wrong_choice( link );
    return;
  }   
  
  if( help ) {
    clear_screen( pl );
    help_link( link, alignment_table[i].name );
    press_return( link );
    return;
  }    

  if( !is_set( clss_table[ pl->pcdata->clss ].alignments, i ) ) {
    word = clss_table[ pl->pcdata->clss ].name;
  } else if( !is_set( plyr_race_table[ pl->shdata->race ].alignments, i ) ) {
    word = race_table[ pl->shdata->race ].name;
  } else {
    pl->shdata->alignment = i;
    link->connected = CON_DECIDE_STATS; 
    nanny_stats( link, "" );
    /*
    clear_screen( pl );
    help_link( link, "Introduction" );
    press_return( link );
    link->connected = CON_READ_IMOTD;
    */
    return;
  }
  
  send( link, "\n\rA%s %s would not choose to be %s.\n\r",
	isvowel( *word ) ? "n" : "",
	word,
	alignment_table[i].name );

  press_return( link );
}


/*
 *   INITIALIZE NEW PLAYER
 */


void new_player( player_data* pl )
{
  char               tmp  [ MAX_STRING_LENGTH ];
  const int         clss  = pl->pcdata->clss;
  int                  i;

  pl->shdata->level     = 1;
  pl->exp               = 0;
  pl->pcdata->practice  = 25;

  pl->base_hit          = 20;
  pl->base_mana         = 50;
  pl->base_move         = 100;
  pl->base_age          = plyr_race_table[ pl->shdata->race ].start_age;

  update_max_hit( pl );
  update_max_mana( pl );

  pl->hit  = pl->max_hit;
  pl->mana = pl->max_mana;
  
  update_max_move( pl );

  pl->move = pl->max_move;

  /* OPTIONS */

  set_bit( pl->pcdata->pfile->flags, PLR_GOSSIP );
  set_bit( pl->pcdata->pfile->flags, PLR_SAY_REPEAT );
  set_bit( pl->pcdata->pfile->flags, PLR_AUTO_EXIT );
  set_bit( pl->pcdata->pfile->flags, PLR_PORTAL );
  set_bit( pl->pcdata->pfile->flags, PLR_INFO );
  set_bit( pl->pcdata->pfile->flags, PLR_LANG_ID );
  set_bit( pl->pcdata->pfile->flags, PLR_OOC );
  set_bit( pl->pcdata->pfile->flags, PLR_CHAT );
  set_bit( pl->pcdata->pfile->flags, PLR_CHANT );
  set_bit( pl->pcdata->pfile->flags, PLR_ATALK );
  set_bit( pl->pcdata->pfile->flags, PLR_NEWBIE );
  set_bit( pl->pcdata->pfile->flags, PLR_REVERSE );
  
  set_level( pl->pcdata->pfile->settings, SET_ROOM_INFO, 3 );
  set_level( pl->pcdata->pfile->settings, SET_AUTOLOOT, 1 );

  /* IFLAGS */

  for( i = 0; i < MAX_IFLAG; ++i )
    set_level( pl->pcdata->iflag, i, 3 );

  set_level( pl->pcdata->iflag, IFLAG_AUCTION, 1 );
  set_level( pl->pcdata->iflag, IFLAG_LOGINS, 1 );
  set_level( pl->pcdata->iflag[1], NOTE_ANNOUNCEMENTS, 1 );
  set_level( pl->pcdata->iflag[1], NOTE_GENERAL, 1 );
  set_level( pl->pcdata->iflag[1], NOTE_INFORMATION, 1 );

  /* MESSAGES, all flags on by construction */

  remove_bit( pl->pcdata->message, MSG_MAX_MOVE );

  set_level( pl->pcdata->mess_settings, MSG_LOOK, 3 );
  set_level( pl->pcdata->mess_settings, MSG_BANK, 3 );
  set_level( pl->pcdata->mess_settings, MSG_PREPARE, 3 );
  set_level( pl->pcdata->mess_settings, MSG_EQUIP, 3 );
  set_level( pl->pcdata->mess_settings, MSG_INVENTORY, 3 );

  calc_resist( pl );
  modify_pfile( pl ); 

  pl->pcdata->pfile->account = pl->link->account;

  free_string( pl->descr->singular, MEM_DESCR );
  snprintf( tmp, MAX_STRING_LENGTH, "%s %s",
	    sex_name[ pl->sex ],
	    race_table[ pl->shdata->race ].name );
  /*
	    race_table[ pl->shdata->race ].name,
	    clss_table[ clss ].name );
  */
  pl->descr->singular = alloc_string( tmp, MEM_DESCR );
  
  free_string( pl->descr->long_s, MEM_DESCR );
  snprintf( tmp, MAX_STRING_LENGTH, "%s is standing here.",
	    pl->descr->singular );
  tmp[0] = toupper( tmp[0] );
  pl->descr->long_s = alloc_string( tmp, MEM_DESCR );

  pl->set_default_title( );

  free_string( pl->descr->keywords, MEM_DESCR );
  snprintf( tmp, MAX_STRING_LENGTH, "%s %s",
	    sex_name[ pl->sex ],
	    race_table[ pl->shdata->race ].name );
  /*
	    race_table[pl->shdata->race].name,
	    clss_table[clss].name );
  */
  pl->descr->keywords = alloc_string( tmp, MEM_DESCR );

  /* SET STANDARD EQUIP/SKILLS */

  use_start_table( pl, 0 );
  use_start_table( pl, clss+1 );
  use_start_table( pl, MAX_CLSS+pl->shdata->race+1 );

  pl->To( get_temple( pl ) );

  pl->Save( );
  save_html_players( );

  if( pl->remort == 0 ) {
    // Send a welcome email.
    if( help_data *help = find_help( 0, "Welcome_Mail" ) ) {
      note_data *note = new note_data;
      note->title = alloc_string( "Welcome to The Forest's Edge!", MEM_NOTE );
      note->from = alloc_string( "TFE Administrators", MEM_NOTE );
      note->message = alloc_string( help->text, MEM_NOTE );
      note->noteboard = NOTE_PRIVATE;
      note->date = current_time;
      append( pl->pcdata->mail, note );
      save_mail( pl->pcdata->pfile, pl->pcdata->mail );
    } else {
      aphid( "Help file \"Welcome_Mail\" does not exist." );
    }

    char *tmp = static_string( );
    snprintf( tmp, THREE_LINES, "New character %s created on account %s.",
	      pl->descr->name,
	      pl->pcdata->pfile->account->name );
    info( LEVEL_IMMORTAL, empty_string, 0, tmp, IFLAG_LOGINS, 3, pl );
    player_log( pl, "Created on account \"%s\" as \"%s\".",
		pl->pcdata->pfile->account->name,
		pl->descr->name );
  }
}


void use_start_table( char_data* ch, int entry )
{
  obj_clss_data*  index;
  obj_data*         obj;
  int             skill;

  for( int i = 0; i < 5; i++ ) 
    if( ( skill = (int)starting_table[entry].skill[i] ) >= 0 )
      ch->shdata->skills[ skill_table( skill ) ][ skill_number( skill ) ]
	= (unsigned char)starting_table[entry].level[i];

  thing_array stuff;
  for( int i = 0; i < 5; i++ ) {
    if( ( index = get_obj_index(
      starting_table[entry].object[2*i] ) ) ) {
      obj = create( index, starting_table[ entry ].object[ 2*i+1 ] );
      //  enchant_object( obj );
      set_alloy( obj, 0 );
      set_quality( obj );
      set_size( obj, ch );
      obj->To( ch );
      switch( obj->pIndexData->item_type ) {
      case ITEM_ARMOR :
      case ITEM_WEAPON :
	stuff += obj;
      }
    }
  }

  wear( ch, stuff );
}


bool newbie_abuse( char_data* ch )
{
  if( ch->species
      || is_set( ch->status, STAT_FORCED )
      || ( (player_data*)ch)->time_played( ) > 5*60 )
    return false;

  send( ch, "To stop abuse you cannot drop or give anything for the\
 first 5 minutes\n\rof play.  To get rid of an item use junk.\n\r" );

  return true;
}
