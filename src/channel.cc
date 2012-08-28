#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include "define.h"
#include "struct.h"


path_func   hear_yell;

tell_data *ooc_history = 0;
tell_data *atalk_history = 0;
tell_data *newbie_history = 0;
tell_data *avatar_history;


/*
 *   TELL CLASS AND SUPPORT ROUTINES
 */


Tell_Data :: Tell_Data( const char* pers, const char* txt,
			int tongue )
  : next(0), language( tongue )
{
  record_new( sizeof( tell_data ), MEM_TELL );
  
  name     = alloc_string( pers, MEM_TELL );
  message  = alloc_string( txt, MEM_TELL ); 
  *name    = toupper( *name );
}


Tell_Data :: ~Tell_Data( )
{
  record_delete( sizeof( tell_data ), MEM_TELL );

  free_string( name, MEM_TELL );
  free_string( message, MEM_TELL );
}


static void display( tell_data *const& list, char_data* ch, const char *text,
		     char_data *victim = 0 )
{
  if( !list ) {
    if( &list == &ooc_history ) {
      send( ch, "There have been no OOC's since the mud was started.\n\r" );

    } else if( &list == &atalk_history ) {
      send( ch, "There has been no auction talk since the mud was started.\n\r" );

    } else if( &list == &newbie_history ) {
      send( ch, "There has been no newbie talk since the mud was started.\n\r" );

    } else if( &list == &avatar_history ) {
      send( ch, "There has been no avatar talk since the mud was started.\n\r" );

    } else if( !victim || victim == ch ) {
      send( ch, "You haven't heard any %s since logging in.\n\r", text );

    } else {
      fsend( ch, "%s hasn't heard any %s since logging in.",
	     victim, text );
    }
    return;
  }
  
  char tmp  [ MAX_STRING_LENGTH ];
  size_t i;

  snprintf( tmp, MAX_STRING_LENGTH, "--- " );
  for( i = 0; text[i]; ++i )
    tmp[i+4] = toupper( text[i] );
  sprintf( tmp+i+4, " ---" );

  page_centered( ch, tmp );

  for( const tell_data *tell = list; tell; tell = tell->next ) {
    format_tell( tmp, tell->message, ch );
    if( tell->language != -1 ) {
      if( ch->get_skill( tell->language ) == UNLEARNT ) {
	page( ch, "[%s >> ???]\n\r%s%s\n\r",
	      tell->name,
	      tmp,
	      tell->next ? "\n\r" : "" );
      } else {
	page( ch, "[%s >> %s]\n\r%s%s\n\r",
	      tell->name,
	      skill_entry(tell->language)->name,
	      tmp,
	      tell->next ? "\n\r" : "" );
      }
    } else {
      page( ch, "[%s]\n\r%s%s\n\r",
	    tell->name,
	    tmp,
	    tell->next ? "\n\r" : "" );
    }
  }
}


static void add_tell( tell_data*& list, const char* pers, const char* message,
		      int language = -1 )
{
  tell_data *tell = new tell_data( pers, message, language );

  append( list, tell );

  if( count( list ) > 20 ) {
    tell = list;
    list = tell->next;
    delete tell;
  }
}


/*
 *   LANGUAGE ROUTINES
 */


/*
int skill_language( const char_data* ch, int language )
{
  if( ch->species
      || language < LANG_FIRST
      || language >= LANG_FIRST+table_max[ TABLE_SKILL_LANGUAGE ] )
    return 0;

  if( ch->is_affected( AFF_TONGUES ) ) 
    return 10;

  return ch->get_skill( language );
}
*/


void do_language( char_data *ch, const char *argument )
{
  if( is_confused_pet( ch ) || !ch->pcdata )
    return;

  if( ch->pcdata->speaking < 0
      || ch->pcdata->speaking >= table_max[TABLE_SKILL_LANGUAGE] )
    ch->pcdata->speaking = 0;
  
  if( !*argument ) {
    send( ch, "You are currently speaking %s.\n\r",
	  skill_language_table[ch->pcdata->speaking ].name );
    send( ch, "Type 'language <lang name>' to switch to another language.\n\r" );
    return;
  }
  
  for( int i = 0; i < table_max[TABLE_SKILL_LANGUAGE]; ++i ) {
    if( fmatches( argument, skill_language_table[i].name ) ) {
      if( i == 0 && ch->Level() < LEVEL_APPRENTICE ) {
        send( ch, "Only immortals can speak in Primal.\n\r" );
        return;
      }
      if( ch->get_skill( LANG_PRIMAL + i ) == UNLEARNT
	  && !ch->is_affected( AFF_TONGUES ) ) {
        send( ch, "You don't know that language.\n\r" );
        return;
      }
      ch->pcdata->speaking = i;
      fsend( ch, "You will now speak in %s.", skill_language_table[i].name );
      return;
    }
  }
  
  send( ch, "Unknown language.\n\r" );
}


int get_language( const char_data* ch, int i )
{
  if( i < LANG_FIRST || i >= LANG_FIRST+table_max[ TABLE_SKILL_LANGUAGE] )
    return 0;

  if( !ch->pcdata || ch->is_affected( AFF_TONGUES ) )
    return 10;

  return ch->get_skill(i);
}


static void garble_string( char *output, const char *input, int skill )
{
  skill = (200-10*skill)*10*skill;

  for( ; *input; input++, output++ ) {
    if( number_range( 0, 10000 ) > skill ) {
      if( ispunct( *input ) || isspace( *input ) ) 
        *output = *input;
      else if( ( *input > 90 ) && ( *input < 97 ) )
        *output = *input-10;
      else
        *output = number_range( 'a', 'z' );
    } else 
      *output = *input;
  }
  
  *output = '\0';
}


static const char *slang( char_data* ch, int language )
{
  if( ch->pcdata && !is_set( ch->pcdata->pfile->flags, PLR_LANG_ID ) )
    return empty_string;

  if( get_language( ch, language ) == 0 )
    return " (in an unknown tongue)";

  char* tmp = static_string( );

  snprintf( tmp, THREE_LINES, " (in %s)", skill_language_table[ language - LANG_PRIMAL ].name );
 
  return tmp;
}


static bool munchkin( char_data* ch, const char *text )
{
  const int length = strlen( text );
  int  punct =  0;
   
  for( int i = 0; i < length; i++ )    
    if( text[i] == '?' || text[i] == '!' )
      punct++; 
  
  if( punct > 3 && length/punct < 20 ) { 
    fsend( ch,
	   "Excessive punctuation is sure sign that you shouldn't be heard from. Please surpress your munchkin tendencies." );
    return true;
  }

  return false;
}


/*
 *   CHANNEL SUPPORT ROUTINES
 */


static bool hear_channels( player_data* pc )
{
  return( pc->position != POS_EXTRACTED
	  && ( !pc->link
	       || pc->link->connected == CON_PLAYING ) );
}


static bool subtract_gsp( player_data* pl, const char* text, int cost )
{
  if( get_trust( pl ) < LEVEL_ARCHITECT ) {
    if( cost == 0 ) {
      if( pl->gossip_pts <= 0 ) {
	fsend( pl, "%s doesn't consume any gossip points, but you must have some to use it.",
	       text );
	return false;
      }
    } else {
      if( pl->gossip_pts < cost ) {
	fsend( pl, "%s requires %s gossip point%s.",
	       text, number_word( cost ), cost == 1 ? "" : "s" );
	return false;
      }
      pl->gossip_pts -= cost;
    }
  }
  
  return true;
}


bool can_talk( char_data* ch, const char* string )
{
  if( ch->is_affected( AFF_SILENCE ) ) {
    if( string ) {
      fsend( ch, "You attempt to %s, but oddly fail to make any noise.", string );
    }
    return false;
  }

  if( ch->is_affected( AFF_CHOKING ) ) {
    if( string ) {
      fsend( ch, "You are choking and cannot %s.", string );
    }
    return false;
  }

  if( is_drowning( ch, string ) )
    return false;

  return true;
}


/*
 *   STANDARD CHANNELS ROUTINES
 */


void do_newbie( char_data* ch, const char *argument )
{
  char tmp  [ MAX_STRING_LENGTH ];

  if( is_mob( ch ) )
    return;
  
  player_data *pc = player( ch );
  
  if( pc->Level() > 10
      && get_trust( pc ) < 91 ) {
    send( ch, "You are not permitted to use the newbie channel.\n\r" );
    return;
  }

  if( !*argument ) {
    display( newbie_history, ch, "Newbie Channel" );
    return;
  }
  
  if( toggle( ch, argument, "Newbie channel",
	      ch->pcdata->pfile->flags, PLR_NEWBIE ) )
    return;
  
  if( !is_set( ch->pcdata->pfile->flags, PLR_NEWBIE ) ) {
    send( ch, "You have the newbie channel off.\n\r" );
    return;
  }
  
  if( munchkin( pc, argument ) )
    return;
  
  if( !subtract_gsp( pc, "Using the newbie channel", 0 ) )
    return;
  
  const char *name = ch->real_name( );
  const int name_len = 10+strlen( name );

  for( int i = 0; i < player_list; ++i ) {
    pc = player_list[i];
    if( hear_channels( pc )
	&& is_set( pc->pcdata->pfile->flags, PLR_NEWBIE )
	&& ( pc->Level() <= 10 || get_trust( pc ) >= 91 )
	&& !pc->Filtering( ch ) ) {
      const int max_length = format_tell( tmp, argument, pc );
      char_data *victim = pc->switched ? pc->switched : pc;
      send_color( victim, COLOR_NEWBIE, "[Newbie] %s:%s%s",
		  name,
		  name_len <= max_length ? "" : "\n\r ", tmp );
      send( victim, "\n\r" );
    }
  }
  
  add_tell( newbie_history, name, argument ); 
}


void do_ooc( char_data* ch, const char *argument )
{
  char tmp  [ MAX_STRING_LENGTH ];

  if( is_mob( ch ) )
    return;
  
  player_data *pc = player( ch );
  
  if( !*argument ) {
    display( ooc_history, ch, "OOC CHANNEL" );
    return;
  }
  
  if( toggle( ch, argument, "OOC channel",
	      ch->pcdata->pfile->flags, PLR_OOC ) )
    return;
  
  if( !is_set( ch->pcdata->pfile->flags, PLR_OOC ) ) {
    send( ch, "You have OOC off.\n\r" );
    return;
  }
  
  if( munchkin( pc, argument ) )
    return;
  
  if( !subtract_gsp( pc, "Using OOC", 1 ) )
    return;
  
  const char *const name = ch->real_name( );
  const int name_len = 7+strlen( name );

  for( int i = 0; i < player_list; ++i ) {
    pc = player_list[i];
    if( hear_channels( pc )
	&& is_set( pc->pcdata->pfile->flags, PLR_OOC )
	&& !pc->Filtering( ch ) ) {
      const int max_length = format_tell( tmp, argument, pc );
      char_data *victim = pc->switched ? pc->switched : pc;
      send_color( victim, COLOR_OOC, "[OOC] %s:%s%s",
		  name,
		  name_len <= max_length ? "" : "\n\r ", tmp );
      send( victim, "\n\r" );
    }
  }
  
  add_tell( ooc_history, name, argument ); 
}


void do_chant( char_data* ch, const char *argument )
{
  if( is_mob( ch ) )
    return;

  char tmp  [ MAX_STRING_LENGTH ];

  player_data *pc = player( ch );

  if( ch->pcdata->religion == REL_NONE ) {
    send( ch, "Only those with a religion can chant.\n\r" );
    return;
  }

  if( !*argument ) {
    display( pc->chant, ch, "Chant" );
    return;
  }
  
  if( toggle( ch, argument, "Chant",
	      ch->pcdata->pfile->flags, PLR_CHANT ) )
    return;
  
  if( !is_set( ch->pcdata->pfile->flags, PLR_CHANT ) ) {
    send( ch, "You have chant turned off.\n\r" );
    return;
  }

  if( !subtract_gsp( pc, "Chanting", 1 ) )
    return;

  const int max_length = format_tell( tmp, argument, ch );
  send_color( ch, COLOR_CHANT, "You chant:%s%s",
	      10 <= max_length ? "" : "\n\r ", tmp );
  send( ch, "\n\r" );
  
  const char *const relig = religion_table[ch->pcdata->religion].name;

  for( int i = 0; i < player_list; ++i ) {
    pc = player_list[i];
    if( hear_channels( pc )
	&& ( pc->pcdata->religion == ch->pcdata->religion
	     || is_immortal( pc ) ) ) {
      const char *const name = ch->Seen_Name( pc );
      add_tell( pc->chant, name, argument, -1 ); 
      if( is_set( pc->pcdata->pfile->flags, PLR_CHANT )
	  && pc != ch
	  && !pc->Filtering( ch ) ) {
	const int max_length = format_tell( tmp, argument, pc );
	char_data *victim = pc->switched ? pc->switched : pc;
	if( is_immortal( pc )
	    && pc->pcdata->religion != ch->pcdata->religion ) {
	  send_color( victim, COLOR_CHANT, "%s chants (%s):%s%s",
		      name, relig,
		      8+(int)strlen(name)+(int)strlen(relig) <= max_length ? "" : "\n\r ", tmp );
	} else {
	  send_color( victim, COLOR_CHANT, "%s chants:%s%s",
		      name,
		      8+(int)strlen(name) <= max_length ? "" : "\n\r ", tmp );
	}
	send( victim, "\n\r" );
      }
    }
  }
}


void do_chat( char_data* ch, const char *argument )
{
  if( is_confused_pet( ch ) ) 
    return;

  char buf  [ MAX_STRING_LENGTH ];
  char tmp  [ MAX_STRING_LENGTH ];

  // This is kinda like is_mob( ch ) && !forced.
  if( ch->species
      && !is_set( ch->status, STAT_FORCED ) ) {
    if( !ch->pcdata ) {
      fsend_seen( ch, "%s looks around in confusion.", ch );
    } else {
      send( ch, "You are unable to do that while switched.\n\r" );
    }
    return;
  }

  int language = LANG_PRIMAL;

  player_data *pc = player( ch );

  if( pc ) {
    if( !*argument ) {
      display( pc->chat, ch, "chats" );
      return;
    }
    
    if( toggle( ch, argument, "Chat channel", 
		ch->pcdata->pfile->flags, PLR_CHAT ) )
      return;
    
    if( !is_set( ch->pcdata->pfile->flags, PLR_CHAT ) ) {
      send( ch, "You have chat turned off.\n\r" );
      return;
    }
  
    if( !subtract_gsp( pc, "Chatting", 1 ) )
      return;
    
    language = ch->pcdata->speaking+LANG_PRIMAL;
  }

  garble_string( buf, argument, get_language( ch, language ) );

  if( ch->pcdata ) {
    const char *const lang = slang( ch, language );
    const int max_length = format_tell( tmp, buf, ch );
    send_color( ch, COLOR_CHAT, "You chat%s:%s%s",
		lang,
		9+(int)strlen(lang) <= max_length ? "" : "\n\r ", tmp );
    send( ch, "\n\r" );
  }

  for( int i = 0; i < player_list; ++i ) {
    pc = player_list[i];
    const bool allied = are_allied( ch, pc );
    if( hear_channels( pc )
	&& ( allied
	     || is_immortal( pc )
	     || is_immortal( ch ) ) ) { 
      const char *const name = who_name( pc, ch );
      add_tell( pc->chat, name, buf, language ); 
      if( pc != ch
	  && is_set( pc->pcdata->pfile->flags, PLR_CHAT )
	  && !pc->Filtering( ch ) ) {
	const char *const lang = slang( pc, language );
	const int max_length = format_tell( tmp, buf, pc );
	char_data *victim = pc->switched ? pc->switched : pc;
	send_color( victim, COLOR_CHAT, "%s chats%s:%s%s",
		    name, lang,
		    7+(int)strlen(name)+(int)strlen(lang) <= max_length ? "" : "\n\r ", tmp );
	send( victim, "\n\r" );
      }
    }
  }
}


void do_gossip( char_data* ch, const char *argument )
{
  if( is_confused_pet( ch ) ) 
    return;

  char buf  [ MAX_STRING_LENGTH ];
  char tmp  [ MAX_STRING_LENGTH ];

  // This is kinda like is_mob( ch ) && !forced.
  if( ch->species
      && !is_set( ch->status, STAT_FORCED ) ) {
    if( !ch->pcdata ) {
      fsend_seen( ch, "%s looks around in confusion.", ch );
    } else {
      send( ch, "You are unable to do that while switched.\n\r" );
    }
    return;
  }

  int language = LANG_PRIMAL;

  player_data *pc = player( ch );

  if( pc ) {
    if( !*argument ) {
      display( pc->gossip, ch, "gossips" );
      return;
    }
    
    if( toggle( ch, argument, "Gossip channel", 
		ch->pcdata->pfile->flags, PLR_GOSSIP ) ) 
      return;
    
    if( !is_set( ch->pcdata->pfile->flags, PLR_GOSSIP ) ) {
      send( ch, "You have gossip turned off.\n\r" );
      return;
    }
    
    if( munchkin( pc, argument ) )
      return;
    
    if( !subtract_gsp( pc, "Gossiping", 5 ) )
      return;

    language = ch->pcdata->speaking+LANG_PRIMAL;
  }

  garble_string( buf, argument, get_language( ch, language ) );

  if( ch->pcdata ) {
    const char *const lang = slang( ch, language );
    const int max_length = format_tell( tmp, buf, ch );
    send_color( ch, COLOR_GOSSIP, "You gossip%s:%s%s",
		lang,
		11+(int)strlen(lang) <= max_length ? "" : "\n\r ", tmp );
    send( ch, "\n\r" );
  }
  
  for( int i = 0; i < player_list; ++i ) {
    pc = player_list[i];
    if( hear_channels( pc ) ) {
      const char *const name = who_name( pc, ch );
      add_tell( pc->gossip, name, buf, language ); 
      if( pc != ch
	  && is_set( pc->pcdata->pfile->flags, PLR_GOSSIP )
	  && !pc->Filtering( ch ) ) {
	const char *const lang = slang( pc, language );
	const int max_length = format_tell( tmp, buf, pc );
	char_data *victim = pc->switched ? pc->switched : pc;
        send_color( victim, COLOR_GOSSIP, "%s gossips%s:%s%s",
		    name, lang,
		    9+(int)strlen(name)+(int)strlen(lang) <= max_length ? "" : "\n\r ", tmp );
	send( victim, "\n\r" );
      }
    }
  }
}


void do_atalk( char_data* ch, const char *argument )
{
  if( is_mob( ch ) )
    return;

  char               tmp  [ MAX_STRING_LENGTH ];

  player_data *pc = player( ch );
  
  if( !*argument ) {
    display( atalk_history, ch, "AUCTION CHANNEL" );
    return;
  }
  
  if( toggle( ch, argument, "Auction channel",
	      ch->pcdata->pfile->flags, PLR_ATALK ) )
    return;
  
  if( !is_set( ch->pcdata->pfile->flags, PLR_ATALK ) ) {
    send( ch, "You have ATALK off.\n\r" );
    return;
  }
  
  if( !subtract_gsp( pc, "Using ATALK", 5 ) )
    return;
  
  const char *const name = ch->real_name( );
  const int name_len = 9+strlen( name );

  for( int i = 0; i < player_list; ++i ) {
    pc = player_list[i];
    if( hear_channels( pc )
	&& is_set( pc->pcdata->pfile->flags, PLR_ATALK )
	&& !pc->Filtering( ch ) ) {
      const int max_length = format_tell( tmp, argument, pc );
      char_data *victim = pc->switched ? pc->switched : pc;
      send_color( victim, COLOR_AUCTION, "[ATALK] %s:%s%s",
		  name,
		  name_len <= max_length ? "" : "\n\r ", tmp );
      send( victim, "\n\r" );
    }
  }
  
  add_tell( atalk_history, name, argument ); 
}


/*
 *   IMMORTAL CHANNELS
 */


void do_avatar( char_data* ch, const char *argument )
{
  char              tmp  [ MAX_STRING_LENGTH ];
  wizard_data*   imm  = 0;

  if( !*argument ) {
    if( !( imm = wizard( ch ) ) )
      return;
    display( avatar_history, ch, "avatar channel" );
    return;
  }

  if( toggle( ch, argument, "Avatar channel", 
	      ch->pcdata->pfile->flags, PLR_AVATAR ) )
    return;
        
  if( !is_set( ch->pcdata->pfile->flags, PLR_AVATAR ) ) {
    send( ch, "You have the avatar channel turned off.\n\r" );
    return;
  }   

  const char *const name = ch->real_name( );
  const int name_len = 3+strlen( name );

  for( int i = 0; i < player_list; ++i ) {
    player_data *pc = player_list[i];
    if( hear_channels( pc )
	&& ( imm = wizard( pc ) )
	&& has_permission( pc, PERM_AVATAR_CHAN ) ) {
      //      add_tell( imm->avatar, name, argument ); 
      if( is_set( pc->pcdata->pfile->flags, PLR_AVATAR ) 
	  && !pc->Filtering( ch ) ) {
	const int max_length = format_tell( tmp, argument, pc );
	char_data *victim = pc->switched ? pc->switched : pc;
        send_color( victim, COLOR_WIZARD, "%s --%s%s",
		    name,
		    name_len <= max_length ? "" : "\n\r ", tmp );
	send( victim, "\n\r" );
      }
    }
  }

  add_tell( avatar_history, name, argument ); 
}


void do_buildchan( char_data* ch, const char *argument )
{
  char              tmp  [ MAX_STRING_LENGTH ];
  wizard_data*   imm  = 0;

  if( !*argument ) {
    if( !( imm = wizard( ch ) ) )
      return;
    display( imm->build_chan, ch, "builder channel" );
    return;
  }
  
  if( toggle( ch, argument, "Builder channel", 
	      ch->pcdata->pfile->flags, PLR_BUILDCHAN ) )
    return;
  
  if( !is_set( ch->pcdata->pfile->flags, PLR_BUILDCHAN ) ) {
    send( ch, "You have the builder channel turned off.\n\r" );
    return;
  }   
  
  const char *const name = ch->descr->name ;
  const int name_len = 1+strlen( name );
  
  for( int i = 0; i < player_list; i++ ) {
    player_data *pc = player_list[i];
    if( hear_channels( pc )
	&& ( imm = wizard( pc ) )
	&& has_permission( pc, PERM_BUILD_CHAN ) ) {
      add_tell( imm->build_chan, name, argument ); 
      if( is_set( pc->pcdata->pfile->flags, PLR_BUILDCHAN ) ) {
	const int max_length = format_tell( tmp, argument, pc );
	char_data *victim = pc->switched ? pc->switched : pc;
        send_color( victim, COLOR_WIZARD, "%s:%s%s",
		    name,
		    name_len <= max_length ? "" : "\n\r ", tmp );
	send( victim, "\n\r" );
      }
    }
  }
}


void do_adminchan( char_data* ch, const char *argument )
{
  char              tmp  [ MAX_STRING_LENGTH ];
  wizard_data*   imm  = 0;

  if( !*argument ) {
    if( !( imm = wizard( ch ) ) )
      return;
    display( imm->admin_chan, ch, "admin channel" );
    return;
  }
  
  if( toggle( ch, argument, "Admin channel", 
	      ch->pcdata->pfile->flags, PLR_ADMINCHAN ) )
    return;
  
  if( !is_set( ch->pcdata->pfile->flags, PLR_ADMINCHAN ) ) {
    send( ch, "You have the admin channel turned off.\n\r" );
    return;
  }   
  
  const char *const name = ch->descr->name;
  const int name_len = 2+strlen( name );
  
  for( int i = 0; i < player_list; i++ ) {
    player_data *pc = player_list[i];
    if( hear_channels( pc )
	&& ( imm = wizard( pc ) )
	&& has_permission( pc, PERM_ADMIN_CHAN ) ) {
      add_tell( imm->admin_chan, name, argument );
      if( is_set( pc->pcdata->pfile->flags, PLR_ADMINCHAN ) ) {
	const int max_length = format_tell( tmp, argument, pc );
	char_data *victim = pc->switched ? pc->switched : pc;
        send_color( victim, COLOR_WIZARD, "=%s=%s%s",
		    name,
		    name_len <= max_length ? "" : "\n\r ", tmp );
	send( victim, "\n\r" );
      }
    }
  }
}


void do_immtalk( char_data* ch, const char *argument )
{
  char                tmp  [ MAX_STRING_LENGTH ];
  wizard_data*     imm  = 0;

  if( !*argument ) {
    if( !(imm = wizard( ch )) )
      return;
    display( imm->imm_talk, ch, "immortal channel" );
    return;
  }

  if( toggle( ch, argument, "Immortal channel", 
	      ch->pcdata->pfile->flags, PLR_IMMCHAN ) )
    return;
  
  if( !is_set( ch->pcdata->pfile->flags, PLR_IMMCHAN ) ) {
    send( ch, "You have the immortal channel turned off.\n\r" );
    return;
  }   

  const char *const name = ch->descr->name;
  const int name_len = 2+strlen( name );

  for( int i = 0; i < player_list; ++i ) {
    player_data *pc = player_list[i];
    if( hear_channels( pc )
	&& ( imm = wizard( pc ) )
	&& has_permission( pc, PERM_IMM_CHAN ) ) {
      add_tell( imm->imm_talk, name, argument ); 
      if( is_set( pc->pcdata->pfile->flags, PLR_IMMCHAN ) ) {
	const int max_length = format_tell( tmp, argument, pc );
	char_data *victim = pc->switched ? pc->switched : pc;
        send_color( victim, COLOR_WIZARD, "[%s]%s%s",
		    name,
		    name_len <= max_length ? "" : "\n\r ", tmp );
	send( victim, "\n\r" );
      }
    }
  }
}


void do_god( char_data* ch, const char *argument )
{
  char                tmp  [ MAX_STRING_LENGTH ];
  wizard_data*     imm = 0;

  if( !*argument ) {
    if( !( imm = wizard( ch ) ) )
      return;
    display( imm->god_talk, ch, "God Channel" );
    return;
  }

  const char *const name = ch->descr->name;
  const int name_len = 2+strlen( name );

  for( int i = 0; i < player_list; i++ ) {
    player_data *pc = player_list[i];
    if( hear_channels( pc )
	&& ( imm = wizard( pc ) )
	&& has_permission( pc, PERM_GOD_CHAN ) ) {
      add_tell( imm->god_talk, name, argument ); 
      const int max_length = format_tell( tmp, argument, pc );
      char_data *victim = pc->switched ? pc->switched : pc;
      send_color( victim, COLOR_WIZARD, "\\%s\\%s%s",
		  name,
		  name_len <= max_length ? "" : "\n\r ", tmp );
      send( victim, "\n\r" );
    }
  }
}


/*
 *   YELL ROUTINE
 */
 

void do_yell( char_data* ch, const char *argument )
{
  char              buf  [ MAX_STRING_LENGTH ];
  char              tmp  [ MAX_STRING_LENGTH ];
  char_data*        rch;

  if( is_confused_pet( ch ) )
    return;

  player_data *pc = player( ch );

  if( !*argument ) {
    if( pc )
      display( pc->yell, ch, "yells" );
    return;
  }

  if( ch->position < POS_RESTING ) {
    pos_message( ch );
    return;
  }

  if( !can_talk( ch, "yell" ) )
    return;

  disrupt_spell( ch );
  spoil_hide( ch );

  int language;
  const char *msg;

  if( ch->pcdata ) {
    language = ch->pcdata->speaking+LANG_PRIMAL;
    garble_string( buf, argument, get_language( ch, language ) );
    msg = buf;
  } else {
    language = LANG_PRIMAL;
    msg = argument;
  }

  const int max_length = format_tell( tmp, msg, ch );
  const char *const lang = slang( ch, language );

  if( ch->pcdata ) {
    send( ch, "You yell%s:%s%s\n\r",
	  lang,
	  9+(int)strlen(lang) <= max_length ? "" : "\n\r ", tmp );
    if( pc )
      add_tell( pc->yell, ch->descr->name, msg, language ); 
  }

  for( int i = 0; i < *ch->array; ++i ) {
    if( ( rch = character( ch->array->list[i] ) )
	&& rch != ch ) {
      hear_yell( rch, ch, msg );
    }
  }

  exec_range( ch, 8, hear_yell, msg );
}


static const char *yell_name( char_data* ch, char_data* victim,
			      int dir, int distance )
{
  if( distance == 0 ) {
    if( ch->Seen( victim ) ) 
      return ch->Seen_Name( victim );
    else
      return "someone nearby";
  }
  
  if( distance > 5 ) 
    return "someone far away";
  
  char *tmp = static_string( );
  snprintf( tmp, THREE_LINES, "someone %s", dir_table[dir].where );

  return tmp;
}


bool hear_yell( char_data* victim, thing_data* th, const char *message,
		int dir, int distance, int, int )
{
  if( !victim->pcdata )
    return false;

  char buf  [ MAX_STRING_LENGTH ];
  char tmp  [ MAX_STRING_LENGTH ];

  char_data *ch = (char_data*)th;

  if( !victim->Can_Hear() ) {
    if( ch->in_room == victim->in_room
	&& ch->Seen( victim ) ) {
      fsend( victim, "%s yells something, but you are unable to hear.", ch );
    }
    return false;
  }

  player_data *pc = player( victim );
  if( pc ) {
    if( pc->switched )
      return false;
  } else if( victim->link ) {
    pc = victim->link->player;
  }
  if( !pc ) {
    return false;
  }

  const char *const name = yell_name( ch, victim, dir, distance );
  const int language = ch->pcdata ? ch->pcdata->speaking+LANG_PRIMAL : LANG_PRIMAL;
  garble_string( buf, message, get_language( pc, language ) );
  add_tell( pc->yell, name, buf, language ); 

  const char *const lang = slang( pc, language );
  const int max_length = format_tell( tmp, buf, pc );
  send( victim, "%s yells%s:%s%s\n\r",
	name, lang,
	7+(int)strlen(name)+(int)strlen(lang) <= max_length ? "" : "\n\r ", tmp );
	
  return false;
}


/*
 *   SAY ROUTINES
 */


static const char *say_verb( char_data* ch, char_data* victim )
{
  if( ch->shdata->race == RACE_LIZARD ) {
    return( ch == victim ? "hiss" : "hisses" );
  }

  return( ch == victim ? "say" : "says" );
}


void do_say( char_data* ch, const char *argument )
{
  if( is_confused_pet( ch ) || is_familiar( ch ) )
    return;

  char              buf  [ MAX_STRING_LENGTH ];
  char              tmp  [ MAX_STRING_LENGTH ];
  char              arg  [ MAX_STRING_LENGTH ];
  char_data*     victim;

  player_data *pc = player( ch );

  if( !*argument ) {
    if( pc ) 
      display( pc->say, ch, "says" );
    return;
  }

  if( ch->position < POS_RESTING ) {
    pos_message( ch );
    return;
  }

  if( !can_talk( ch, "speak" ) )
    return;

  disrupt_spell( ch );
  spoil_hide( ch );

  int language;
  const char *msg;

  if( ch->pcdata ) {
    language = ch->pcdata->speaking+LANG_PRIMAL;
    garble_string( buf, argument, get_language( ch, language ) );
    msg = buf;
  } else {
    language = LANG_PRIMAL;
    msg = argument;
  }

  if( ch->pcdata ) {
    if( is_set( ch->pcdata->pfile->flags, PLR_SAY_REPEAT ) ) {
      const int max_length = format_tell( tmp, msg, ch );
      const char *const lang = slang( ch, language );
      const char *const verb = say_verb( ch, ch );
      send( ch, "You %s%s:%s%s\n\r",
	    verb, lang,
	    5+(int)strlen(verb)+(int)strlen(lang) <= max_length ? "" : "\n\r ", tmp );
    } else {
      send( ch, "Ok.\n\r" );
    }

    if( pc )
      add_tell( pc->say, ch->descr->name, msg, language ); 
  }
  
  for( int i = 0; i < *ch->array; ++i ) {
    if( !( victim = character( ch->array->list[i] ) )
	|| ch == victim
	|| !victim->pcdata ) {
      continue;
    }
    if( !victim->Can_Hear() ) {
      if( ch->Seen( victim ) ) {
	fsend( victim, "%s says something, but you are unable to hear.", ch );
      }
      continue;
    }
    if( ( pc = player( victim ) ) ) {
      if( pc->switched )
	continue;
    } else if( victim->link ) {
      pc = victim->link->player;
    }
    if( !pc ) {
      continue;
    }
    pc->improve_skill( language );
    const char *const from = ch->Name( pc );
    garble_string( arg, msg, get_language( pc, language ) );
    add_tell( pc->say, from, arg, language ); 
    const char *const lang = slang( pc, language );
    const char *const verb = say_verb( ch, pc );
    const int max_length = format_tell( tmp, arg, pc );
    send_color( victim, COLOR_SAYS, "%s %s%s:%s%s",
		from, verb, lang,
		2+(int)strlen(from)+(int)strlen(verb)+(int)strlen(lang) <= max_length ? "" : "\n\r ", tmp );
    send( victim, "\n\r" );
  }
}


/*
 *   SHOUT ROUTINE
 */


void do_shout( char_data* ch, const char *argument )
{
  if( is_mob( ch ) )
    return;

  player_data *pc = player( ch );

  if( !*argument ) {
    display( pc->shout, ch, "shouts" );
    return;
  }

  if( ch->position < POS_RESTING ) {
    pos_message( ch );
    return;
  }

  if( is_set( ch->pcdata->pfile->flags, PLR_NO_SHOUT ) ) {
    send( ch, "You have been forbidden from shouting by the gods.\n\r" );
    return;
  }

  if( !subtract_gsp( pc, "Shouting", 500 ) )
    return;
  
  char tmp  [ MAX_STRING_LENGTH ];

  const int max_length = format_tell( tmp, argument, ch );
  send( ch, "You shout:%s%s\n\r",
	10 <= max_length ? "" : "\n\r ", tmp );
  add_tell( pc->shout, ch->descr->name, argument ); 
  
  for( int i = 0; i < player_list; ++i ) {
    pc = player_list[i];
    if( pc != ch
	&& hear_channels( pc ) ) {
      const char *const name = who_name( pc, ch );
      add_tell( pc->shout, name, argument ); 
      const int max_length = format_tell( tmp, argument, pc );
      char_data *victim = pc->switched ? pc->switched : pc;
      send( victim, "%s shouts:%s%s\n\r",
	    name,
	    8+(int)strlen(name) <= max_length ? "" : "\n\r ", tmp );
    }
  }
}


/*
 *   TELL ROUTINES
 */


static void tell_message( char_data* ch, char* msg, char_data* victim, char *s = 0 )
{
  char tmp  [ THREE_LINES ];

  snprintf( tmp, THREE_LINES, "++ " );  

  if( in_character && ch->is_affected( AFF_HALLUCINATE ) ) {
    snprintf( tmp+3, THREE_LINES-10, msg, fake_name( false ), s );
  } else {
    snprintf( tmp+3, THREE_LINES-10, msg, victim->descr->name, s );
  }

  strcat( tmp, " ++\n\r\n\r" );

  tmp[3] = toupper( tmp[3] );

  send( ch, tmp );
}


void process_tell( char_data* ch, char_data* victim, const char *argument )
{
  if( victim->Filtering( ch ) ) {
    fsend( ch, "%s is filtering you - please leave %s in peace.",
	   victim->Seen_Name( ch ), victim->Him_Her( ) );
    return; 
  }

  if( ch->Filtering( victim ) ) {
    fsend( ch,
	   "You are filtering %s and only a chebucto would want to converse with someone they are filtering.",
	   victim->Seen_Name( ch ) );
    return;
  } 
  
  char buf  [ 3*MAX_STRING_LENGTH ];

  player_data *pc = player( victim );

  if( pc && ch->pcdata ) {
    if( !victim->link && ( !pc->switched || !pc->switched->link ) ) {
      tell_message( ch, "%s is link dead", victim );

    } else if( pc->timer+30 < current_time ) {
      sprintf_minutes( buf, current_time-pc->timer );
      tell_message( ch, "%s has been idle for %s",
		    victim, buf );

    } else if( victim->array != ch->array && opponent( victim ) ) {
      tell_message( ch, "%s is in battle", victim );
    }
  }

  if( ch->pcdata ) {
    if( is_set( ch->pcdata->pfile->flags, PLR_SAY_REPEAT ) ) {
      const char *name = victim->Seen_Name( ch );
      const int max_length = format_tell( buf, argument, ch );
      send( ch, "You tell %s:%s%s\n\r",
	    name,
	    10+(int)strlen(name) <= max_length ? "" : "\n\r ", buf );
    } else {
      send( ch, "Ok.\n\r" );
    }
  }

  if( !pc
      && victim->link )
    pc = victim->link->player;

  if( !pc )
    return;
  
  const char *const from = ch->Seen_Name( pc );
  const char *const to = ( victim != pc || !pc->switched )
    ? "you" : pc->descr->name;

  char_data *recv = pc->switched ? pc->switched : pc;

  if( !ch->pcdata ) {
    convert_to_ansi( pc, 3*MAX_STRING_LENGTH, argument, out_buf );
    const int max_length = format_tell( buf, out_buf, pc );
    send( recv, "%s tells %s:%s%s",
	  from, to,
	  8+(int)strlen(from)+(int)strlen(to) <= max_length ? "" : "\n\r ", buf );
    send( recv, "\n\r" );
    return;
  }

  const int max_length = format_tell( buf, argument, pc );
  send_color( recv, COLOR_TELLS, "%s tells %s:%s%s",
	      from, to,
	      8+(int)strlen(from)+(int)strlen(to) <= max_length ? "" : "\n\r ", buf );
  send( recv, "\n\r" );

  if( victim == pc ) {
    add_tell( pc->tell, from, argument );
    if( !is_set( victim->status, STAT_REPLY_LOCK ) ) {
      pc->reply = ch;
    }
  }
}


void do_tell( char_data *ch, const char *argument )
{
  if( is_mob( ch ) )
    return;

  player_data *pc  = player( ch );

  if( !*argument ) {
    if( pc )
      display( pc->tell, ch, "tells" );
    return;
  }

  char arg [ MAX_STRING_LENGTH ];

  argument = one_argument( argument, arg );

  if( !*argument ) {
    send( ch, "Tell whom what?\n\r" );
    return;
  }

  in_character = false;
  player_data *victim;
  
  if( !( victim = one_player( ch, arg, "tell",
			      (thing_array*) &player_list ) ) )
    return;
  
  if( ch == victim ) {
    send( ch, "Talking to yourself is pointless.\n\r" );
    return;
  }
  
  if( victim->Ignoring( ch ) ) {
    fsend( ch, "%s has ignore set to level %d and you cannot tell to %s.",
	   victim->descr->name, 
	   level_setting( &victim->pcdata->pfile->settings, SET_IGNORE ),
	   victim->Him_Her( ) );
    return;
  }

  process_tell( ch, victim, argument );
}


void do_reply( char_data* ch, const char *argument )
{
  if( is_mob( ch ) )
    return;

  player_data *pc = player( ch );

  const char *const noone_msg
    = "No one has told to you or the person who last told to you has quit.\n\r";

  char_data *victim = pc->reply;
  
  in_character = false;
  
  if( !*argument ) {
    if( !pc->reply ) {
      send( ch, "Reply points to no one.\n\r" );
    } else {
      fsend( ch, "Reply %s to %s.",
	     is_set( ch->status, STAT_REPLY_LOCK ) ? "is locked" : "points",
	     victim->Seen_Name( ch ) );
    }
    return;
  }
  
  if( exact_match( argument, "lock" )
      && !*argument ) {
    if( !victim ) { 
      send( ch, noone_msg );
    } else if( is_set( ch->status, STAT_REPLY_LOCK ) ) {
      fsend( ch, "Your reply is already locked to %s.",
	     victim->Seen_Name( ch ) );
    }
    else {
      set_bit( ch->status, STAT_REPLY_LOCK );
      fsend( ch, "Reply locked to %s.", victim->Seen_Name( ch ) );
    }
    return;
  }
  
  if( exact_match( argument, "unlock" )
      && !*argument ) {
    if( !is_set( ch->status, STAT_REPLY_LOCK ) ) {
      send( ch, "Your reply is already unlocked.\n\r" );
    }
    else {
      send( ch, "Reply unlocked.\n\r" );
      remove_bit( ch->status, STAT_REPLY_LOCK );
    }
    return;
  }
  
  if( is_avatar( ch )
      && exact_match( argument, "clear" )
      && !*argument ) {
    for( int i = 0; i < player_list; ++i ) {
      player_data *wch = player_list[i];
      if( wch->Is_Valid( ) && wch->reply == ch ) {
	fsend( ch, "Cleared %s's reply pointer.", wch );
	remove_bit( wch->status, STAT_REPLY_LOCK );
	wch->reply = 0;
      }
    }
    return;
  }

  if( !victim ) {
    send( ch, noone_msg );
    return;
  }

  process_tell( ch, victim, argument );
}


/*
 *   WHISPER/TO
 */


static void trigger_say( char_data* ch, char_data* mob, const char *argument )
{
  for( mprog_data *mprog = mob->species->mprog; mprog; mprog = mprog->next ) {
    if( mprog->trigger == MPROG_TRIGGER_TELL
	&& ( !*mprog->string
	     || is_name( argument, mprog->string ) ) ) {
      clear_variables( );
      var_ch = ch;
      var_mob = mob;
      var_arg = argument;
      var_room = Room( ch->array->where );
      mprog->execute( );
      return;
    }
  }
}


static void ic_tell( char_data* ch, const char *argument, char* verb,
		     tell_data *player_data::*list, bool silent  )
{
  char              buf  [ MAX_STRING_LENGTH ];
  char              tmp  [ MAX_STRING_LENGTH ];
  char              arg  [ MAX_STRING_LENGTH ];
  char_data*     victim;

  argument = one_argument( argument, arg );

  if( !( victim = one_character( ch, arg, verb,
				 ch->array ) ) )
    return;
  
  if( !*argument ) {
    fsend( ch, "%s what to %s?", verb, victim );
    return;
  }
  
  if( ch == victim ) {
    send( ch, "%sing something to yourself does nothing useful.\n\r",
	  verb );
    return;
  }
  
  if( !IS_AWAKE( victim ) ) {
    send( ch, "They are not in a state to hear you.\n\r" );
    return;
  }
  
  disrupt_spell( ch );

  if( !silent )
    spoil_hide( ch );

  int language;
  const char *msg;

  if( ch->pcdata ) {
    language = ch->pcdata->speaking+LANG_PRIMAL;
    garble_string( buf, argument, get_language( ch, language ) );
    msg = buf;
  } else {
    language = LANG_PRIMAL;
    msg = argument;
  }

  if( ch->pcdata ) {
    if( is_set( ch->pcdata->pfile->flags, PLR_SAY_REPEAT ) ) {
      const char *const name = victim->Name( ch );
      const char *const lang = slang( ch, language );
      const int max_length = format_tell( tmp, msg, ch );
      send( ch, "You %s to %s%s:%s%s\n\r",
	    verb, name, lang,
	    9+(int)strlen(verb)+(int)strlen(name)+(int)strlen(lang) <= max_length ? "" : "\n\r ", tmp );
    } else {
      send( ch, "Ok.\n\r" );
    }
  }

  if( silent ) {
    fsend_seen( ch, "%s %ss something to %s.", ch, verb, victim );
  } else {
    for( int i = 0; i < *ch->array; ++i ) {
      char_data *rch = character( ch->array->list[i] );
      if( !rch
	  || rch == ch
	  || rch == victim
	  || !rch->pcdata )
	continue;
      if( !rch->Can_Hear( ) ) {
	if( ch->Seen( rch ) ) {
	  fsend( rch, "%s says something to %s, but you are unable to hear.",
		 ch, victim );
	}
	continue;
      }
      player_data *pc = player( rch );
      if( pc ) {
	if( pc->switched )
	  continue;
      } else if( rch->link ) {
	pc = rch->link->player;
      }
      if( !pc ) {
	continue;
      }
      pc->improve_skill( language );
      const char *const from = ch->Name( pc );
      const char *const to = victim->Name( pc );
      garble_string( arg, msg, get_language( pc, language ) );
      const char *const lang = slang( pc, language );
      const char *const verb = say_verb( ch, pc );
      const int max_length = format_tell( tmp, arg, pc );
      send_color( rch, COLOR_SAYS, "%s %s to %s%s:%s%s",
		  from, verb, to, lang,
		  2+(int)strlen(from)+(int)strlen(verb)+(int)strlen(to)+(int)strlen(lang) <= max_length ? "" : "\n\r ",
		  tmp );
      send( rch, "\n\r" );
    }
  }
  
  if( !victim->pcdata ) {
    trigger_say( ch, victim, msg );
    return;
  }

  player_data *pc = player( victim );

  if( !pc ) {
    if( victim->link ) {
      pc = victim->link->player;
    }
    if( !pc )
      return;
  } else if( pc->switched ) {
    return;
  }

  const char *const from = ch->Name( pc );
  const char *const to = ( victim != pc || !pc->switched )
                   ? "you" : pc->descr->name;

  garble_string( arg, msg, get_language( pc, language ) );
  const char *const lang = slang( pc, language );
  const int max_length = format_tell( tmp, arg, pc );
  char_data *recv = pc->switched ? pc->switched : pc;
  send_color( recv, COLOR_SAYS, "%s %ss to %s%s:%s%s",
	      from, verb, to, lang,
	      10+(int)strlen(from)+(int)strlen(verb)+(int)strlen(to)+(int)strlen(lang) <= max_length ? "" : "\n\r ",
	      tmp );
  send( recv, "\n\r" );

  if( victim == pc ) {
    add_tell( pc->*list, from, arg, language ); 
  }
}


void do_to( char_data* ch, const char *argument )
{
  if( is_confused_pet( ch ) )
    return;

  player_data *pc = player( ch );

  if( !*argument ) {
    if( pc ) {
      display( pc->to, ch, "in-character tells" );
    } else {
      send( ch, "Only players have in-character tell history.\n\r" );
    }
    return;
  }
  
  if( ch->position < POS_RESTING ) {
    pos_message( ch );
    return;
  }

  ic_tell( ch, argument, "say", &player_data::to, false );
}


void do_whisper( char_data* ch, const char *argument )
{  
  if( is_confused_pet( ch ) )
    return;
  
  player_data *pc = player( ch );

  if( !*argument ) {
    if( pc ) {
      display( pc->whisper, ch, "whispers" );
    } else {
      send( ch, "Only players have whisper history.\n\r" );
    }
    return;
  }
  
  if( ch->position < POS_RESTING ) {
    pos_message( ch );
    return;
  }

  ic_tell( ch, argument, "whisper", &player_data::whisper, true );
}


/*
 *   EMOTE ROUTINE
 */


void do_emote( char_data *ch, const char *argument )
{
  if( ch->pcdata
      && is_set( ch->pcdata->pfile->flags, PLR_NO_EMOTE ) ) {
    send( ch, "You can't show your emotions.\n\r" );
    return;
  }
  
  if( !*argument ) {
    send( ch, "Emote what?\n\r" );
    return;
  }
  
  const bool space = strncmp( argument, "'s ", 3 ) && strncmp( argument, ", ", 2 );  
  
  for( int i = 0; i < *ch->array; i++ ) {
    char_data *rch;
    if( !( rch = character( ch->array->list[i] ) )
	|| !rch->Can_See() )
      continue; 
    fsend_color( rch, COLOR_EMOTE, "%s%s%s",
		 ch->Name( rch ), space ? " " : "", argument );
  }
}


/*
 *   GROUP TELL
 */


void do_gtell( char_data* ch, const char *argument )
{
  char              tmp  [ MAX_STRING_LENGTH ];
  char              buf  [ MAX_STRING_LENGTH ];
  char_data*     leader;
  player_data*       pc  = player( ch );

  if( is_mob( ch ) )
    return;

  if( !*argument ) {
    display( pc->gtell, ch, "group tells" );
    return;
  }

  if( is_set( ch->pcdata->pfile->flags, PLR_NO_TELL ) ) {
    send( ch, "You are banned from using tell.\n\r" );
    return;
  }

  if( !( leader = group_leader( ch ) ) ) {
    send( ch, "You aren't in a group.\n\r" );
    return;
  }

  if( leader == ch ) {
    int i = 0;
    for( ; i < ch->followers.size; ++i ) {
      char_data *follower = ch->followers[i];
      if( is_set( follower->status, STAT_IN_GROUP ) )
	break;
    }
    if( i == ch->followers.size ) {
      send( ch, "Talking to yourself is pointless.\n\r" );
      return;
    }
  }

  const int language = ch->pcdata->speaking+LANG_PRIMAL;
  garble_string( buf, argument, get_language( ch, language ) );
  const char *const lang = slang( ch, language );
  const int max_length = format_tell( tmp, buf, ch );
  send_color( ch, COLOR_GTELL, "You tell your group%s:%s%s",
	      lang,
	      20+(int)strlen(lang) <= max_length ? "" : "\n\r ", tmp );
  send( ch, "\n\r" );

  for( int i = 0; i < player_list; ++i ) {
    pc = player_list[i];
    if( hear_channels( pc )
	&& leader == group_leader( pc ) ) {
      const char *name = ch->Seen_Name( pc );
      add_tell( pc->gtell, name, buf, language );
      if( pc != ch ) {
	const char *const lang = slang( pc, language );
	const int max_length = format_tell( tmp, buf, pc );
	char_data *victim = pc->switched ? pc->switched : pc;
        send_color( victim, COLOR_GTELL,"%s tells the group%s:%s%s",
		    name, lang,
		    17+(int)strlen(name)+(int)strlen(lang) <= max_length ? "" : "\n\r ", tmp  );
	send( victim, "\n\r" );
      }
    }
  }
}


/*
 *   CLAN TELL
 */


void do_ctell( char_data* ch, const char *argument )
{
  char               buf  [ MAX_STRING_LENGTH ];
  char               tmp  [ MAX_STRING_LENGTH ];
  player_data*    pc  = player( ch );

  if( is_mob( ch ) )
    return;

  clan_data *clan = ch->pcdata->pfile->clan;

  if( !clan ) {
    send( ch, "You aren't in a clan.\n\r" );
    return;
  }

  if( !*argument ) {
    display( pc->ctell, ch, "clan tells" );
    return;
  }
  
  if( toggle( ch, argument, "Clan channel", 
	      ch->pcdata->pfile->flags, PLR_CTELL ) )
    return;
  
  if( !is_set( ch->pcdata->pfile->flags, PLR_CTELL ) ) {
    send( ch, "You have ctell turned off.\n\r" );
    return;
  }
  
  if( !is_set( clan->flags, CLAN_APPROVED ) ) {
    send( ch, "You cannot use ctell until your clan has been approved.\n\r" );
    return;
  }

  const int language = ch->pcdata->speaking+LANG_PRIMAL;
  garble_string( buf, argument, get_language( ch, language ) );
  const char *const lang = slang( ch, language );
  const int max_length = format_tell( tmp, buf, ch );
  send_color( ch, COLOR_CTELL, "You tell the clan%s:%s%s",
	      lang,
	      18+(int)strlen(lang) <= max_length ? "" : "\n\r ", tmp );
  send( ch, "\n\r" );

  for( int i = 0; i < player_list; ++i ) {
    pc = player_list[i];
    if( hear_channels( pc )
	&& pc->pcdata->pfile->clan == clan ) {
      const char *const name = ch->Seen_Name( pc );
      add_tell( pc->ctell, name, buf, language ); 
      if( pc != ch
	  && is_set( pc->pcdata->pfile->flags, PLR_CTELL ) ) {
	const char *const lang = slang( pc, language );
	const int max_length = format_tell( tmp, buf, pc );
	char_data *victim = pc->switched ? pc->switched : pc;
        send_color( victim, COLOR_CTELL, "%s ctells%s:%s%s",
		    name, lang,
		    8+(int)strlen(name)+(int)strlen(lang) <= max_length ? "" : "\n\r ", tmp );
	send( victim, "\n\r" );
      }
    }
  }
}


/*
 *   REVIEW COMMAND
 */


class review_data
{
public:
  const char *const name;
  tell_data *player_data::*list;
  tell_data *wizard_data::*list2;
};


const review_data reviews[] = {
  { "admin channel", 0, &wizard_data::admin_chan },
  //  { "avatar channel", 0, &wizard_data::avatar },
  { "builder channel", 0, &wizard_data::build_chan },
  { "chat", &player_data::chat, 0 },
  { "chant", &player_data::chant, 0 },
  { "ctell", &player_data::ctell, 0 },
  { "god channel", 0, &wizard_data::god_talk },
  { "gossip", &player_data::gossip, 0 },
  { "gtell", &player_data::gtell, 0 },
  { "immortal channel", 0, &wizard_data::imm_talk },
  { "say", &player_data::say, 0 },
  { "shout", &player_data::shout, 0 },
  { "tell", &player_data::tell, 0 },
  { "to", &player_data::to, 0 },
  { "whisper", &player_data::whisper, 0 },
  { "yell", &player_data::yell, 0 },
  { "", 0, 0 }
};


const review_data imm_reviews[] = {
  { "", 0 }
};


void do_review( char_data* ch, const char *argument )
{
  char arg [ MAX_INPUT_LENGTH ];

  argument = one_argument( argument, arg );
  
  if( !*arg ) {
    send( ch, "For whom do you wish to review recent conversation?\n\r" );
    return;
  }
  
  player_data *victim;

  if( !( victim = one_player( ch, arg, "review", 
			      (thing_array*) &player_list ) ) )
    return;
  
  if( ch == victim ) {
    send( ch,
	  "There are simplier ways to review your own conversations.\n\r" );
    return;
  }
  
  if( get_trust( victim ) >= get_trust( ch ) ) {
    fsend( ch, "You are unable to review %s's conversations.", victim );
    return;
  }
  
  if( !*argument ) {
    send( ch, "Which conversation channel do you want to review?\n\r" );
    return;
  }
  
  wizard_data *wiz = wizard( victim );

  for( const review_data *data = reviews; *data->name; ++data ) {
    if( matches( argument, data->name ) ) {
      if( data->list ) {
	display( victim->*data->list, ch, data->name, victim );
      } else if( wiz ) {
	display( wiz->*data->list2, ch, data->name, victim );
      } else {
	fsend( ch, "%s does not have %s history.",
	       victim, data->name );
      }
      return;
    }
  }

  send( ch, "Unknown history type - see help review.\n\r" );
}
