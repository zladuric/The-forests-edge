#include <stdio.h>
#include <syslog.h>
#include "define.h"
#include "struct.h"


static int max_players = 0;
int record_players = 0;


/*
 *   LAST ROUTINES 
 */


/* This function takes a pfile and a character as input, and displays the 
   last time the character the pfile references to the character that was 
   inputed.  If that character that is recieving the info is a demigod or
   greater, the host of the pfile is also displayed. */
static void display_last( pfile_data* pfile, char_data* ch )
{
  page( ch, "%-15s %s  %s\n\r", pfile->name,
	ltime( pfile->last_on, false, ch ),
	is_demigod( ch ) ? pfile->last_host : "" );
}


/* This function does the last command.  It takes a character and an argument.
   If the argument is avatar or immortal, it shows the last time all of the
   immortals and avatars have been on.  Otherwise it shows the last time
   the character is on.  */
void do_last( char_data *ch, const char *argument ) 
{
  bool          found = false;
  int            pos;
  int         length;

  /* If the character is not a player, they can't do last */
  if( not_player( ch ) )
    return;

  /* If no argument, tell the person that they need to input more
     information */
  if( !*argument ) {
    send( ch, "Specify first few letters of name to search for.\n\r" );
    return;
  }

  /* If the argument is avatar:  Step through the pfiles, checking to
     see if they are avatars (their level is less then 91 and their trust
     is higher then 91). If an avatar is found, show the last time they were
     on. */
  if( exact_match( argument, "avatar" ) ) {
    for( pos = 0; pos < max_pfile; pos++ ) 
      if( pfile_list[pos]->level < LEVEL_AVATAR
	  && pfile_list[pos]->trust >= LEVEL_AVATAR ) {
        found = true;
        display_last( pfile_list[pos], ch );
      }
  }
  
  /* If the argument is immortal:  Step through the pfiles, checking
     to see if they are above LEVEL_APPRENTICE.  If they are, display
     the last time they were on */
  else if( exact_match( argument, "immortal" ) ) {
    for( pos = 0; pos < max_pfile; pos++ ) 
      if( pfile_list[pos]->level >= LEVEL_APPRENTICE ) {
        found = true;
        display_last( pfile_list[pos], ch );
      }
  }
  
  else if( exact_match( argument, "clan" ) ) {

    if( !*argument ) {

      if( !ch->pcdata->pfile->clan ) {
	send( ch, "You aren't in a clan.\n\r" );
	return;
      }
      for( pos = 0; pos < max_pfile; ++pos ) 
	if( pfile_list[pos]->clan == ch->pcdata->pfile->clan ) {
	  found = true;
	  display_last( pfile_list[pos], ch );
	}

    } else {
      
      for( int i = 0; i < max_clan; ++i ) {
	const clan_data *clan = clan_list[i];
	if( knows_members( ch, clan )
	    && fmatches( argument, clan->abbrev ) ) {
	  for( pos = 0; pos < max_pfile; ++pos ) 
	    if( pfile_list[pos]->clan == clan ) {
	      found = true;
	      display_last( pfile_list[pos], ch );
	    }
	  break;
	}
      }
    }
  }

  /* Otherwise, search through the pfile_list looking for a match to the
     argument.  If it finds one, display the last time the player
     was on.  Otherwise, continue.  Does name completion.  */
  else {
    
    if( ( pos = pntr_search( pfile_list, max_pfile, argument ) ) < 0 )
      pos = -pos-1;
    length = strlen( argument );
    for( ; pos < max_pfile; pos++ ) {
      if( strncasecmp( pfile_list[pos]->name, argument, length ) )
	break;
      found = true;
      display_last( pfile_list[pos], ch );
    }
  }
  
  /* If the argument doesn't match a pfile, and isn't immortal or avatar, 
     tell the character that no matches were found */
  if( !found ) 
    send( ch, "No matches found.\n\r" );
}



/* This function does the whois command.  Sends lots of personal info 
   from the player file. */
void do_whois( char_data* ch, const char *argument )
{
  /* NOTE: character is ch, and victim is the character referenced in the
     argument */

  /* If the character is not a player, they can't do whois */
  if( not_player( ch ) )
    return;

  /* If the argument is null, it tells the character to specify a user */
  if( !*argument ) {
    send( ch, "Syntax: whois <name>\n\r" );
    return;
  }

  bool         email;
  /* get the wizard_data of the character */
  //  wizard_data*   imm  = wizard( ch );

  int pos = pntr_search( pfile_list, max_pfile, argument );

  if( pos < 0 ) {
    pos = -pos-1;
    int len = strlen( argument );
    if( pos >= max_pfile
	|| strncasecmp( pfile_list[pos]->name, argument, len ) ) {
      fsend( ch, "No player matching \"%s\" exists.", argument );
      return;
    }

    if( pos >= max_pfile-1
	|| !strncasecmp( pfile_list[pos+1]->name, argument, len ) ) {
      send( ch, "Multiple partial matches found:\n\r" );
      while( pos < max_pfile
	     && !strncasecmp( pfile_list[pos]->name, argument, len ) ) {
	display_last( pfile_list[pos], ch );
	++pos;
      }
      return;
    }
  }

  pfile_data *pfile = pfile_list[pos];

  if( pfile->level == 0 ) {
    // Player is in creation mode.
    fsend( ch, "No player matching \"%s\" exists.", argument );
    return;
  }

  /* If the character doing whois is a demigod, or if the character is doing
     whois on themselves, let them see the private data */
  bool see_private = ( is_demigod( ch ) || ch->pcdata->pfile == pfile );
  /* If the character is an immortal, and he can see the account of the
     victim, set see_account to true. */
  bool see_account = pfile->account->Seen( ch );

  /* Send the character some formatting stuff (the line of ---- at the top ) */
  send( ch, scroll_line[1] );
  send( ch, "\n\r" );
  
  /* Send the name and race of the victim */
  send( ch, "        Name: %s\n\r", pfile->name );
  send( ch, "        Race: %s\n\r", race_table[ pfile->race ].name );

  /* If the victim isn't incognito, send the class, sex, and level of the
     victim */
  if( !is_incognito( pfile, ch ) ) {
    send( ch, "       Class: %s\n\r", clss_table[ pfile->clss ].name );
    send( ch, "         Sex: %s\n\r", sex_name[ pfile->sex] );
    send( ch, "    Religion: %s\n\r", religion_table[ pfile->religion ].name );

    /* If the victim isn't an immortal, then send the level of the victim. 
       Otherwise send the immortal title of the victim */
    if( pfile->level < LEVEL_APPRENTICE )
      send( ch, "       Level: %d  [ Rank %d%s ]\n\r",
	    pfile->level, pfile->rank+1, number_suffix( pfile->rank+1 ) );
    else
      send( ch, "       Level: %s\n\r",
	    imm_title[ pfile->level-LEVEL_AVATAR ] );
  }

  /* If the character is a demigod, send the trust of the victim */
  if( is_demigod( ch ) && pfile->trust > pfile->level )
    send( ch, "       Trust: %d\n\r", pfile->trust );

  /* If the victim isn't an immortal, send the bounty on his head */
  if( pfile->level < LEVEL_APPRENTICE )
    send( ch, "      Bounty: %d\n\r", pfile->bounty );

  /* If the victim is in a clan, and the character knows the members
     of the clan, then send the clan the victim is in */
  send( ch, "        Clan: %s\n\r\n\r",
	( !pfile->clan || !knows_members( ch, pfile->clan ) ) 
	? "none" : pfile->clan->name );

  /* Send the formatting stuff (line of ---) */
  send( ch, scroll_line[1] );
  send( ch, "\n\r" );

  /* Send the last the victim logged on, and when he was created */
  send( ch, "  Last Login: %s\n\r", ltime( pfile->last_on, false, ch ) );
  send( ch, "     Created: %s\n\r", ltime( pfile->created, false, ch ) );

  /* If the character is a god, then send the victim's password */
  if( is_god( ch ) ) 
    send( ch, "    Password: %s\n\r", pfile->pwd );

  /* If character can see_account or see_private, then send the victims
     last connection host, tehir account name, and the account password */
  if( see_account || see_private ) {
    send( ch, "        Site: %s\n\r", pfile->last_host );
    send( ch, "     Account: %s\n\r",
	  pfile->account ? pfile->account->name : "none ");
    if( is_god( ch ) && pfile->account ) 
      send( ch, "  Acnt. Pswd: %s\n\r", pfile->account->pwd );
  }

  /* If the victim has email private set, then send (Hidden) as email. 
     Otherwise send the email. */
  if( pfile->account &&
      ( ( email = is_set( pfile->flags, PLR_EMAIL_PUBLIC ) ) || see_private ) ) 
    send( ch, "       Email: %s%s\n\r", pfile->account->email,
	  email ? "" : "  (Hidden)" );
  
  /* If the victim has homepage sent, send homepage.  Otherwise send none. */
  send( ch, "    Homepage: %s\n\r",
	pfile->homepage == empty_string ? "none" : pfile->homepage );

  /* If character can see_private, then send account balance info. */
  if( see_private ) {
    send( ch, "\n\r" );
    send( ch, scroll_line[1] );
    send( ch, "\n\r" );
    send( ch, "     Balance: $%.2f\n\r",
	  pfile->account ? (double)pfile->account->balance/100.0 : 0.0 );
    }

  /* More formating */
  send( ch, "\n\r" );
  send( ch, scroll_line[1] );
}


/*
 *   WHO ROUTINES
 */


/* This function takes a character and an argument.  It sends to the character
   a list of the characters currently on.  If the argument is -i, it sends
   only the list of introduced characters.  If the arguement is -b, it sends
   only the list of befriended characters.  If the arguement is a name, it
   sends the name of that character. */
void do_qwho( char_data* ch, const char *argument )
{
  char          tmp  [ ONE_LINE ];
  char_data*    wch;
  link_data*   link;
  int             i  = 0;
  int         count  = 0;
  int hidcount = 0;
  int         flags;

  /* Non-players can't do who */
  if( not_player( ch ) )
    return;

  /* Check and see if the argument contains the flags i or b.  If it
     does, set the appropriate flag */
  if( !get_flags( ch, argument, &flags, "ib", "Qwho" ) )
    return;

  /* Print some formated stuff to the character */
  page( ch, scroll_line[0] );
  page_title( ch, "Players" );
  //  page( ch, "\n\r" );

  /* Scroll through the link_list until you hit the end */
  for( link = link_list; link; link = link->next ) {
    /* If the link isn't playing yet, skip it */
    if( link->connected != CON_PLAYING )
      continue;

    /* Set the temp character pointer to the character using the link */
    wch = link->player;

    /* If character can't see the character (invis imm), the character used
       -i and doesn't recognize the wch, the character used -b and hasn't 
       befriended the wch, skip the link */
    if( !can_see_who( ch, wch )
	|| ( flags == 1 && !ch->Recognizes( wch ) )
	|| ( flags == 2 && !ch->Befriended( wch ) ) )
      continue;
    
    /* Increase the count of the number of players connected */
    ++count;

    if( is_set( wch->pcdata->pfile->flags, PLR_WIZINVIS ) ) {
      ++hidcount;
    }
    
    /* If the character is using TERM_ANSI, send colored text, otherwise
       send vanilla text.  Send the character's name. */
    if( ch->pcdata->terminal != TERM_ANSI ) {
      page( ch, "%15s%s", wch->descr->name, ++i%5 ? "" : "\n\r" );
    } else { 
      snprintf( tmp, ONE_LINE, "%s%15s%s%s",
		same_clan( ch, wch ) ? red( ch )
		: ( ch->Befriended( wch ) ? green( ch )
		    : ( ch->Recognizes( wch ) ? yellow( ch ) : "" ) ),
		wch->descr->name, normal( ch ),
		++i%5 ? "" : "\n\r" );
      page( ch, tmp );
    }
  }
  
  /* If we got out of the loop before the end o line thingy, go ahead and
     end the line. */
  if( i%5 != 0 )
    page( ch, "\n\r" );
  
  /* Send the number of people on and the max number */
  //  int tmp_max = max( max_players, count );
  if( count - hidcount > max_players ) {
    max_players = count - hidcount;
    if( max_players > record_players ) {
      record_players = max_players;
    }
  }
  //  page( ch, "\n\r" );
  page( ch, scroll_line[0] );
  snprintf( tmp, ONE_LINE, "[ %d player%s | %d high | %d record ]",
	    count, ( count == 1 ? "" : "s" ),
	    max_players, record_players );
  page_centered( ch, tmp );
}



void do_who( char_data* ch, const char *argument )
{
  if( not_player( ch ) )
    return;

  int flags;
  if( !get_flags( ch, argument, &flags, "n", "who" ) )
    return;

  show_who( ch, true, is_set( flags, 0 ) );
}


void show_who( char_data *ch, bool pager, bool bw_titles )
{
  if( !ch->link )
    return;

  int save_term = ch->pcdata->terminal;

  /* Send that scrolly thingy */
  if( pager ) {
    page( ch, scroll_line[0] );
  } else {
    send( ch, scroll_line[0] );
  }

  char tmp [ SIX_LINES ];
  char buf [ MAX_STRING_LENGTH ];
  int i = 0;
  int count = 0;
  int hidcount = 0;
  title_data *title;

  for( int type = 0; type < 4; ++type ) {
    bool found = false;
    *buf = '\0';

    /* Step through the link_list. */
    for( link_data *link = link_list; link; link = link->next ) {
      /* If a link isn't playing, skip it */
      if( link->connected != CON_PLAYING )
        continue;
      
      /* Set wch to the player using the link */
      player_data *wch = link->player;

      if( type == 0 ) {
	if( wch->Level() < LEVEL_APPRENTICE )
	  continue;
      } else if( wch->Level() >= LEVEL_APPRENTICE ) {
	continue;
      } else if( wch == ch ) {
	if( type != 1 )
	  continue;
      } else if( ch->Recognizes( wch ) ) {
	if( type == 3
	    || ch->Befriended( wch ) != ( type == 1 ) )
	  continue; 
      } else if( type != 3 )
	continue;
      
      if( !can_see_who( ch, wch ) ) {
        continue;
      }

      ++count;

      if( is_set( wch->pcdata->pfile->flags, PLR_WIZINVIS ) ) {
	++hidcount;
      }

      if( !found ) {
        if( i++ != 0 ) {
	  if( pager ) {
	    page( ch, "\n\r" );
	  } else {
	    send( ch, "\n\r" );
	  }
	}
	if( pager ) {
	  page_title( ch, type > 1 ? 
		      ( type == 2 ? "Known" : "Unknown" ) :
		      ( type == 0 ? "Immortals" : "Befriended" ) );
	} else {
	  send_title( ch, ( type == 0 ? "Immortals" : "Mortals" ) );
	}
        found = true;
      }
    
      clan_data *clan = wch->pcdata->pfile->clan;
      wizard_data *imm    = wizard( wch );
      char *abbrev = "  ";
    
      if( wch->Level() >= LEVEL_APPRENTICE ) {
        const char *lvl_title = ( imm
				  && imm->level_title != empty_string ) 
          ? imm->level_title
          : imm_title[ wch->Level()-LEVEL_AVATAR ];
        int length = strlen( lvl_title );
        strcpy( buf, "[               ]" );
        memcpy( buf+8-length/2, lvl_title, length );

      } else {
	if( wch->pcdata->trust >= LEVEL_AVATAR ) {
          if( has_permission( wch, PERM_APPROVE ) )
            abbrev = "AV";
          else if( ch->pcdata->trust >= LEVEL_AVATAR )
            abbrev = "IP";

	} else if( clan
		   && ( title = get_title( wch->pcdata->pfile ) )
		   && knows_members( ch, clan ) ) {
	  if( is_set( clan->flags, CLAN_APPROVED ) )
	    abbrev = "CL";
	  else
	    abbrev = "cl";
	}
	
        if( is_incognito( wch, ch ) ) {
          snprintf( buf, MAX_STRING_LENGTH, "[   ??   %s %s ]",
            race_table[wch->shdata->race].abbrev, abbrev );
	} else {
          snprintf( buf, MAX_STRING_LENGTH, "[ %2d %s %s %s ]", wch->Level(),
		    clss_table[wch->pcdata->clss].abbrev,
		    race_table[wch->shdata->race].abbrev, abbrev );
	}
      }
      
      const int namelen = strlen( wch->descr->name );
      const unsigned width = 53;
      char title [ MAX_STRING_LENGTH ];

      if( bw_titles ) {
	ch->pcdata->terminal = TERM_DUMB;
      }

      unsigned titlen = convert_to_ansi( ch, MAX_STRING_LENGTH, wch->pcdata->title, title );

      if( !bw_titles
	  && namelen + titlen > width ) {
	// Too long? switch to monochrome.
	ch->pcdata->terminal = TERM_DUMB;
	titlen = convert_to_ansi( ch, MAX_STRING_LENGTH, wch->pcdata->title, title );
      }

      ch->pcdata->terminal = save_term;

      int diff = 0;
      if( namelen + titlen > width ) {
	titlen = width - namelen;
      } else {
	int len = strlen( title );
	diff = len - titlen;
	titlen = len;
      }

      /*
      const char *col = ( wch == ch ) ? bold_magenta_v( ch )
	: ch->pcdata->pfile && same_clan( wch, ch ) ? bold_green_v( ch )
	: bold_cyan_v( ch );
      */
      const char *const col = bold_cyan_v( ch );
      if( type > 0 ) {
	// Non-imms.
        snprintf( tmp, SIX_LINES, " %%s%%s%%s%%-%ds  %%s\n\r", 
		  width+diff-namelen );
        snprintf( buf+17, MAX_STRING_LENGTH-17, tmp,
		  col,
		  wch->descr->name, normal( ch ),
		  trunc( title, titlen ).c_str(),
		  clan && knows_members( ch, clan )
		  ? clan->abbrev : " -- " );
      } else {
        if( imm && imm->wizinvis > 0
	    && is_set( wch->pcdata->pfile->flags, PLR_WIZINVIS ) ) {
	  // Hidden imms.
	  snprintf( tmp, SIX_LINES, " %%s%%s%%s%%-%ds   %d\n\r", 
		    width+diff-namelen,
		    imm->wizinvis );
	} else {
	  // Unhidden imms.
	  snprintf( tmp, SIX_LINES, " %%s%%s%%s%%-%ds\n\r",
		    width+diff-namelen );
	}

        snprintf( buf+17, MAX_STRING_LENGTH-17, tmp,
		  col,
		  wch->descr->name,
		  normal( ch ),
		  trunc( title, titlen ).c_str() );
      }
      
      if( pager ) {
	page( ch, buf );
      } else {
	send( ch, buf );
      }
    }
  }

  // Print the number of players listed and the max seen so far.
  //  int tmp_max = max( max_players, count );
  if( count - hidcount > max_players ) {
    max_players = count - hidcount; 
    if( max_players > record_players ) {
      record_players = max_players;
    }
  }

  snprintf( tmp, TWO_LINES,
	    "[ %d player%s | %d high | %d record ]",
	    count, ( count == 1 ? "" : "s" ),
	    max_players, record_players );

  if( pager ) {
    page( ch, scroll_line[0] );
    page_centered( ch, tmp );
  } else {
    send( ch, scroll_line[0] );
    send_centered( ch, tmp );
  }
}


/*
 *   USERS ROUTINE
 */


static int compare_hostname( link_data *link1, link_data *link2 )
{
  const int cmp =  strcasecmp( link1->host, link2->host );

  return ( cmp <= 0 ) ? -1 : 1;
}


void do_users( char_data* ch, const char *argument )
{
  char            tmp  [ THREE_LINES ];
  player_data* victim;
  int           count  = 0;
  int           flags;

  if( !get_flags( ch, argument, &flags, "aiws", "Users" ) )
    return;

  int length = strlen( argument );

  if( is_set( flags, 0 ) ) {
    snprintf( tmp, THREE_LINES, "%-15s   %s\n\r",
	      "Name", "Appearance" );
  }
  else if( is_set( flags, 1 ) ) {
    snprintf( tmp, THREE_LINES, "%-18s%3s %3s %3s %3s %3s   %3s   %4s %4s %4s   %-7s\n\r",
	      "Name", "Cls", "Rce", "Ali", "Lvl", "Trs", "Idl",
	      "Hits", "Enrg", "Move", "Bank" );
  }
  else if( is_set( flags, 2 ) ) {
    snprintf( tmp, THREE_LINES, "%-15s  %4s  %s\n\r", "Name", "Idle", "What?" );
  }
  else {
    snprintf( tmp, THREE_LINES, "%-15s   %-30s   %s\n\r",
	      "Name", "Site", "Location" );
  }

  page_underlined( ch, tmp );

  Array<link_data*> links;
  if( is_set( flags, 3 ) ) {
    for( link_data *link = link_list; link; link = link->next ) {
      victim = link->player;
      if( is_demigod( ch )
	  || victim == ch
	  || ( victim && victim->Level() < LEVEL_APPRENTICE ) ) {
	links.insert( link, links.binary_search( link, compare_hostname ) );
      }
    }
  }
  for( link_data *link = link_list; link; link = link->next ) {
    victim = link->player;
    if( is_set( flags, 3 )
	&& ( is_demigod( ch )
	     || victim == ch
	     || ( victim && victim->Level() < LEVEL_APPRENTICE ) ) )
      continue;
    links += link;
  }

  for( int i = 0; i < links.size; ++i  ) {
    link_data *link = links[i];
    victim = link->player;
    if( ( is_god( ch ) || ( victim && can_see_who( ch, victim ) ) )
	&& !strncasecmp( argument, victim ? victim->descr->name : "", length ) ) {
      if( !is_set( flags, 0 )
	  && !is_set( flags, 1 )
	  && !is_set( flags, 2 ) ) {
        snprintf( tmp, THREE_LINES, "%-15s   %-30s   %s",
		  victim ? victim->descr->name : "(Logging In)",
		  is_demigod( ch )
		  || victim == ch
		  || ( victim && victim->Level() < LEVEL_APPRENTICE ) 
		  ? &link->host[ max( 0, strlen( link->host )-30 ) ] : "(protected)",
		  ( !victim || !victim->array )
		  ? "(nowhere)" : victim->Location( ) );
        trunc( tmp, 78 );
        strcat( tmp, "\n\r" );

      } else {
        if( link->connected != CON_PLAYING ) {
          snprintf( tmp, THREE_LINES, "-- Logging In --\n\r" );
	} else if( is_set( flags, 0 ) ) {
          snprintf( tmp, THREE_LINES, "%-15s   %s\n\r",
		    victim->descr->name, victim->descr->singular );          
	} else if( is_set( flags, 2 ) ) {
          snprintf( tmp, THREE_LINES, "%-15s  %4d  %s\n\r",
		    victim->descr->name, (int) current_time-victim->timer,
		    "??" );          
	} else {
          snprintf( tmp, THREE_LINES,
            "%-18s%3s %3s  %2s %3d %3d   %3d   %4d %4d %4d   %-7d\n\r",
		    victim->descr->name,
		    clss_table[victim->pcdata->clss].abbrev,
		    race_table[victim->shdata->race].abbrev,
		    alignment_table[ victim->shdata->alignment ].abbrev,
		    victim->Level(),
		    victim->pcdata->trust,
		    (int) current_time-victim->timer,
		    victim->max_hit, victim->max_mana, victim->max_move,
		    victim->bank );
	}
      }
      page( ch, tmp );
      ++count;
    }
  }
}


/*
 *   HOMEPAGE
 */


void do_homepage( char_data* ch, const char *argument )
{
  if( is_mob( ch ) ) 
    return;

  if( !is_set( ch->pcdata->pfile->flags, PLR_APPROVED ) ) {
    send( ch, "You cannot set your homepage until your appearance has been approved.\n\r" );
    return;
  }

  if( ((player_data*)ch)->gossip_pts < 0 ) {
    send( ch, "You cannot set your homepage when your gossip points are negative.\n\r" );
    return;
  }

  if( !*argument ) {
    send( ch, "What is your homepage address?\n\r" );
    return;
  } 
  
  if( strlen( argument ) > 60 ) {
    send( ch, "You homepage address must be less than 60 characters.\n\r" );
    return;
  } 

  const char *const new_home = strcasecmp( argument, "none" ) ? argument : empty_string;

  ch->pcdata->pfile->home_modified = strcmp( ch->pcdata->pfile->homepage, new_home );

  free_string( ch->pcdata->pfile->homepage, MEM_PFILE );
  ch->pcdata->pfile->homepage = alloc_string( new_home, MEM_PFILE );

  if( new_home == empty_string ) {
    send( ch, "Your homepage has been cleared.\n\r" );
  } else {
    fsend( ch, "Your homepage is set to \"%s\".", new_home );
  }
}  
