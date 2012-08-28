#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <syslog.h>
#include <unistd.h>
#include "define.h"
#include "struct.h"


const char* noteboard_name [ MAX_NOTEBOARD ] = { "general", "immortal",
						 "ideas",
						 "bugs", "jobs", "announcements", "information", "stories", "changes",
						 "wanted", "fixed", "code", "avatar", "clan" };


note_data**  note_list [ MAX_NOTEBOARD ];
int          max_note  [ MAX_NOTEBOARD ];


note_data*   find_mail        ( char_data*, int );
static void  note_summary     ( char_data* );
static void  display_note     ( char_data*, int, const note_data* );
static void  display_noteboard( player_data* );
static void  display_mail     ( char_data*, int, const note_data* );
static void  display_mailbox  ( char_data* );
static void  save_noteboard   ( char*, note_data**, int );
static void  load_noteboard   ( char*, note_data**&, int&, int );


static int noteboard_level( int board )
{
  switch( board ) {
  case NOTE_IMMORTAL:
  case NOTE_JOBS:
  case NOTE_CHANGES:
  case NOTE_CODE:
    return LEVEL_APPRENTICE;
  case NOTE_AVATAR:
    return LEVEL_AVATAR;
  }

  return 0;
}


static bool can_read( char_data* ch, int i, bool msg = false )
{ 
  if( noteboard_level( i ) > ch->pcdata->trust ) {
    if( msg )
      fsend( ch,
	     "You cannot read the %s noteboard.",
	     noteboard_name[i] );
    return false;
  }

  if( i == NOTE_CLAN ) {
    clan_data *clan = ch->pcdata->pfile->clan;
    if( !clan ) {
      if( msg )
	send( ch,
	      "You aren't in a clan and thus have no clan noteboard.\n\r" );
      return false;
    }
    if( !is_set( clan->flags, CLAN_APPROVED ) ) {
      if( msg )
	send( ch,
	      "You cannot use the clan noteboard until your clan has been approved.\n\r" );
      return false;
    }
  }

  return true;
}
 

/*
 *   NOTE_DATA CLASS
 */


Note_Data :: Note_Data( )
  : next(0), from(empty_string), title(empty_string), message(empty_string),
    date(0), update(0), noteboard(0)
{
  record_new( sizeof( note_data ), MEM_NOTE );
}


Note_Data :: ~Note_Data( )
{
  record_delete( sizeof( note_data ), MEM_NOTE );

  free_string( title,   MEM_NOTE );
  free_string( message, MEM_NOTE );
  free_string( from,    MEM_NOTE );
}


void extract( char_data* ch, note_data* note )
{
  for( int i = 0; i < player_list; i++ ) {
    player_data *victim = player_list[i];
    if( victim->Is_Valid( ) && victim->note_edit == note ) {
      if( victim != ch )
        fsend( victim,
	       "%s just deleted the note you were editing.", ch );
      victim->note_edit = 0;
    }
  } 

  int noteboard  = note->noteboard;
  for( int i = 0; i < max_note[noteboard]; i++ )
    if( note_list[noteboard][i] == note ) {
      remove( note_list[noteboard], max_note[noteboard], i );
      delete note;
      return;
    }

  bug( "Extract( note ): Non-existent note!?" );
}        


/*
 *   SUPPORT FUNCTIONS
 */


static bool acceptable_title( char_data* ch, const char* title )
{
  if( !*title ) {
    send( ch, "You need to specify a message title.\n\r" );
    return false;
  }

  if( strlen( title ) <= 40 ) 
    return true;

  send( ch, "Message titles must be less than 40 characters.\n\r" );

  return false;
}


void recent_notes( char_data* ch )
{
  int           i, j;
  int         recent  = 0;
  if( const clan_data *clan = ch->pcdata->pfile->clan ) {
    note_list[ NOTE_CLAN ] = clan->note_list;
    max_note[ NOTE_CLAN ]  = clan->max_note;
  }

  for( recent = i = 0; i < MAX_NOTEBOARD; i++ ) 
    if( can_read( ch, i ) ) 
      for( j = max_note[i]-1; j >= 0; j--, recent++ ) 
        if( note_list[i][j]->Date( ) < ch->pcdata->pfile->last_note )
          break;

  if( recent > 0 ) {
    char tmp [ TWO_LINES ];
    snprintf( tmp, TWO_LINES, "%s note%s been posted since last login.",
	      number_word( recent ), recent == 1 ? " has" : "s have" );
    *tmp = toupper( *tmp );
    send_centered( ch, tmp );
  }
}


static bool can_write( player_data *pc, int i, bool msg )
{
  if( i < 0 )
    return true;

  if( is_set( pc->pcdata->pfile->flags, PLR_NO_NOTES )
      && i != NOTE_CLAN ) {
    if( msg )
      send( pc, "The immortals have revoked your noteboard privileges.\n\r" );
    return false;
  }

  if( !is_apprentice( pc )
      && ( i == NOTE_IMMORTAL || i == NOTE_JOBS
	   || i == NOTE_CHANGES || i == NOTE_CODE
	   || i == NOTE_FIXED || i == NOTE_ANNOUNCEMENTS ) ) {
    if( msg ) 
      fsend( pc, "Only immortals can modify the %s noteboard.",
	     noteboard_name[i] );
    return false;
  }

  if( i == NOTE_AVATAR && !is_avatar( pc )  ) {
    if( msg )
      fsend( pc, "You aren't an avatar and cannot modify the %s board.",
	     noteboard_name[i] );
    return false;
  }

  if( i == NOTE_CLAN && !pc->pcdata->pfile->clan ) {
    if( msg )
      send( pc, "You aren't in a clan and thus have no clan noteboard.\n\r" );
    return false;
  }
  
  if( !is_apprentice( pc )
      && pc->gossip_pts < 0
      && i != NOTE_CLAN ) {
    if( msg )
      send( pc, "You cannot modify public noteboards when you have negative gossip points.\n\r" );
    return false;
  }

  return true;
}


static bool can_remove( player_data *pc, note_data *note, const char *msg )
{
  if( !can_write( pc, note->noteboard, msg ) ) {
    return false;
  }

  // Avatars can edit avatar noteboard.
  if( note->noteboard == NOTE_AVATAR
      && is_avatar( pc ) )
    return true;

  // Author can edit own notes.
  // Noteboard perms allow edit of any note.
  if( !strcmp( pc->descr->name, note->from )
      || has_permission( pc, PERM_NOTEBOARD ) )
    return true;
  
  //  if( note->noteboard == NOTE_BUGS && is_apprentice( pc ) )
  //    return true;
  
  title_data *title;

  if( note->noteboard == NOTE_CLAN
      && ( title = get_title( pc->pcdata->pfile ) )
      && is_set( title->flags, TITLE_REMOVE_NOTES ) ) 
    return true;

  if( msg )
    fsend( pc, "You do not have permission to %s that note.", msg );

  return false;
}


static int find_note( char_data* ch, int i, const char *argument )
{
  int number = atoi( argument )-1;

  if( is_set( ch->pcdata->pfile->flags, PLR_REVERSE ) )
    number = max_note[i]-number-1;

  if( number < 0 || number >= max_note[i] ) {
    send( ch, "There is no note by that number.\n\r" );
    return -1;
  }

  return number;
}
 

static bool find_notes( char_data* ch, int i, const char *argument, int& a, int& b )
{
  atorange( argument, a, b );

  if( a < 1 || a > max_note[i] ) {
    send( ch, "There is no note number %d.\n\r", a );
    return false;
  }

  if( b < 0 )
    b = max_note[i];
  else if( b == 0 || b > max_note[i] ) {
    send( ch, "There is no note number %d.\n\r", b );
    return false;
  }

  if( a > b ) {
    send( ch, "Bad range.\n\r", b );
    return false;
  }

  --a;
  --b;

  if( is_set( ch->pcdata->pfile->flags, PLR_REVERSE ) ) {
    swap( a, b );
    a = max_note[i]-a-1;
    b = max_note[i]-b-1;
  }

  return true;
}
 

static int find_noteboard( char_data* ch, const char *& argument, bool msg = false )
{
  for( int i = 0; i < MAX_NOTEBOARD; i++ ) 
    if( matches( argument, noteboard_name[i] ) ) 
      return i;
  
  if( msg ) 
    send( ch, "No such noteboard exists.\n\r" );

  return -1;
}


void reverse( note_data*& list )
{
  note_data *pntr = list;
  list = 0;

  while( pntr ) {
    note_data *temp = pntr->next;
    pntr->next = list;
    list = pntr;
    pntr = temp;
  }
}


/*
 *   NOTEBOARD ROUTINE
 */


void do_notes( char_data* ch, const char *argument )
{
  if( is_mob( ch ) ) 
    return;

  char            tmp1  [ 3*MAX_STRING_LENGTH ];
  char            tmp2  [ THREE_LINES ];
  int                i;

  player_data *pc = player( ch );
  int noteboard = pc->noteboard;

  clan_data *clan;
  if( ( clan = ch->pcdata->pfile->clan ) ) {
    note_list[ NOTE_CLAN ] = clan->note_list;
    max_note[ NOTE_CLAN ] = clan->max_note;
  }

  note_data *note;
  if( !( note = pc->note_edit ) ) {

    if( !*argument ) {
      display_noteboard( pc );
      return;
    }
    
    if( fmatches( argument, "summary" ) ) {
      note_summary( ch );
      return;
    }
    
    if( !strcasecmp( argument, "clear" ) ) {
      ch->pcdata->pfile->last_note = current_time;
      send( ch, "Cleared note summary.\n\r" );
      return;
    }

    if( matches( argument, "move" ) ) {
      if( ( noteboard = find_noteboard( ch, argument, true ) ) == -1
	  || !can_write( pc, noteboard, true )
	  || !can_write( pc, pc->noteboard, true )
	  || ( i = find_note( ch, pc->noteboard, argument ) ) == -1 )
        return;
      
      note = note_list[ pc->noteboard ][i];
      
      if( strcmp( ch->descr->name, note->from )
	  && !has_permission( ch, PERM_NOTEBOARD ) ) {
        send( ch, "You are unable to move that note.\n\r" );
        return;
      }

      if( noteboard == pc->noteboard ) {
        fsend( ch, "The note is already on the %s board.",
	      noteboard_name[noteboard] );
        return;
      }

      remove( note_list[ pc->noteboard ],
	      max_note[ pc->noteboard ], i );
      insert( note_list[noteboard], max_note[noteboard], note,
	      max_note[noteboard] );
      fsend( ch, "Note \"%s\" moved to %s board.",
	     note->title, noteboard_name[noteboard] );
      
      snprintf( tmp1, 3*MAX_STRING_LENGTH, "Note \"%s\" moved by %s to %s board from %s board.",
		note->title, ch->descr->name, noteboard_name[ noteboard ],
		noteboard_name[ pc->noteboard ] );
      snprintf( tmp2, THREE_LINES, "Note \"%s\" moved by a mail daemon to %s board from %s board.",
		note->title, noteboard_name[ noteboard ],
		noteboard_name[ pc->noteboard ] );
      info( min( noteboard_level( noteboard ), noteboard_level( pc->noteboard ) ),
	    tmp2,
	    invis_level( pc ),
	    tmp1, MAX_IFLAG+noteboard, 2, ch );
      note->noteboard = noteboard;
      save_notes( noteboard, clan );
      save_notes( pc->noteboard, clan );
      return;
    }
    
    if( matches( argument, "delete" ) ) {
      if( !strcasecmp( argument, "all" ) ) {
        if( !has_permission( ch, PERM_SHUTDOWN, true ) ) 
          return;
        if( max_note[noteboard] == 0 ) {
          fsend( ch, "The %s noteboard is already clear.",
		 noteboard_name[noteboard] );
          return;
	}
        for( ; max_note[noteboard] > 0; ) 
          extract( ch, note_list[noteboard][0] );
        fsend( ch, "You clear the %s noteboard.", 
	       noteboard_name[noteboard] );
        snprintf( tmp1, 3*MAX_STRING_LENGTH, "%s noteboard cleared by %s.",
		  noteboard_name[noteboard], ch->real_name( ) );
	snprintf( tmp2, THREE_LINES, "%s noteboard cleared by a mail daemon.",
		  noteboard_name[noteboard] );
        *tmp1 = toupper( *tmp1 );
        *tmp2 = toupper( *tmp2 );
        info( noteboard_level( noteboard ),
	      tmp2,
	      invis_level( pc ),
	      tmp1, MAX_IFLAG+noteboard, 1, ch );
        save_notes( noteboard, clan );
        return;
      }
      if( !*argument
	  || !isdigit( *argument ) ) {
        send( ch, "Which note do you wish to delete?\n\r" );
        return;
      }
      int min, max;
      if( !find_notes( ch, noteboard, argument, min, max ) )
	return;
      const int count = max - min + 1;
      if( count == 1 ) {
	note = note_list[noteboard][min];
	if( !can_remove( pc, note, "delete" ) ) {
	  return;
	}
	fsend( ch, "Note \"%s\" removed from %s board.",
	       note->title, noteboard_name[noteboard] );
	snprintf( tmp1, 3*MAX_STRING_LENGTH, "Note \"%s\" deleted from the %s board by %s.",
		  note->title, noteboard_name[noteboard], ch->descr->name );
	snprintf( tmp2, THREE_LINES, "Note \"%s\" deleted from the %s board by a mail daemon.",
		  note->title, noteboard_name[noteboard] );
	extract( ch, note );
      } else {
	for( int j = min; j <= max; ++j ) {
	  note = note_list[noteboard][j];
	  if( !can_remove( pc, note, 0 ) ) {
	    fsend( pc, "You do not have permission to delete all those notes." );
	    return;
	  }
	}
	for( int j = min; j <= max; ++j ) {
	  note = note_list[noteboard][min];
	  extract( ch, note );
	}
	fsend( ch, "%s notes removed from %s board.",
	       number_word( count, ch ), noteboard_name[noteboard] );
	snprintf( tmp1, 3*MAX_STRING_LENGTH, "%s notes deleted from the %s board by %s.",
		  number_word( count, ch ), noteboard_name[noteboard], ch->descr->name );
	snprintf( tmp2, THREE_LINES, "%s notes deleted from the %s board by a mail daemon.",
		  number_word( count, ch ), noteboard_name[noteboard] );
      }
      *tmp1 = toupper( *tmp1 );
      *tmp2 = toupper( *tmp2 );
      info( noteboard_level( noteboard ),
	    tmp2,
	    invis_level( pc ),
	    tmp1, MAX_IFLAG+noteboard, 3, ch, 
	    noteboard == NOTE_CLAN ? ch->pcdata->pfile->clan : 0 );
      save_notes( noteboard, clan );
      return;
    }
    
    if( isdigit( *argument ) ) {
      if( ( i = find_note( ch, noteboard, argument ) ) == -1 )
        return;
      note = note_list[noteboard][i]; 
      page( ch, "   Title: %s\n\r    From: %s\n\r  Posted: %s\n\r",
	    note->title, note->from,
	    ltime( note->date, false, ch ) );
      if( note->update != 0 ) {
	page( ch, "Modified: %s\n\r",
	      ltime( note->update, false, ch ) );
      }
      page( ch, "\n\r" );
      convert_to_ansi( ch, 3*MAX_STRING_LENGTH, note->message, tmp1 );
      page( ch, tmp1 );
      return;
    }

    if( matches( argument, "edit" ) ) {
      bool unlock = false;
      if( is_builder( ch ) && matches( argument, "unlock" ) ) {
	unlock = true;
      }

      int number;
      if( !number_arg( argument, number ) ) {
        send( ch, "What number note do you want to edit?\n\r" );
        return;
      }

      if( --number < 0
	  || number >= max_note[noteboard] ) {
        send( ch, "There is no note by that number.\n\r" );
        return;
      }

      if( is_set( ch->pcdata->pfile->flags, PLR_REVERSE ) )
        number = max_note[noteboard]-number-1;

      note = note_list[noteboard][number];

      if( !can_remove( pc, note, "edit" ) ) {
        return;
      }

      if( player_data *pl = edit_lock( pc, note, offset( &pc->note_edit, pc ), "note", unlock ) ) {
	if( unlock ) {
	  send( ch, "You are unable to unlock that note.\n\r" );
	} else {
	  fsend( ch, "%s is currently editing that note.", pl );
	}
	return;
      }

      fsend( ch, "You now edit the note \"%s\".", note->title );
      pc->note_edit = note;
      return;
    }

    if( matches( argument, "search" ) ) {
      if( !*argument ) {
        send( ch, "What text do you wish to search for?\n\r" );
        return;
      }
      
      bool reverse = is_set( ch->pcdata->pfile->flags, PLR_REVERSE );
      int max = max_note[ noteboard ];
      unsigned count = 0;

      for( int n = 0; n < max; ++n ) {
	const int number = reverse ? max-1-n : n;
	note = note_list[noteboard][number];

	if( search_text( note->from, argument )
	    || search_text( note->title, argument )
	    || search_text( note->message, argument ) ) {
	  if( count == 0 ) {
	    page_title( ch, "%s Noteboard Search Results", noteboard_name[ noteboard ] );
	  }
	  display_note( ch, n+1, note );
	  ++count;
	}
      }

      if( count == 0 ) {
	send( ch, "No matching notes found on %s noteboard.\n\r",
	      noteboard_name[noteboard] );
      } else {
	page( ch, "\n\r  %u matching note%s found.\n\r",
	      count, count == 1 ? "" : "s" );
      }
      return;
    }


    if( !exact_match( argument, "new" )
	&& ( i = find_noteboard( ch, argument ) ) != -1 ) {
      if( can_read( ch, i, true ) ) {
	if( isdigit( *argument ) ) {
	  // Read a note on another board without switching noteboards.
	  const int j = find_note( ch, i, argument );
	  if( j == -1 )
	    return;
	  note = note_list[i][j]; 
	  if( i != noteboard ) {
	    page( ch, "   Board: %c%s\n\r",
		  toupper( *noteboard_name[i] ), noteboard_name[i]+1 );
	  }
	  page( ch, "   Title: %s\n\r    From: %s\n\r  Posted: %s\n\r",
		note->title, note->from, ltime( note->date, false, ch ) );
	  if( note->update != 0 ) {
	    page( ch, "Modified: %s\n\r",
		  ltime( note->update, false, ch ) );
	  }
	  page( ch, "\n\r" );
	  convert_to_ansi( ch, 3*MAX_STRING_LENGTH, note->message, tmp1 );
	  page( ch, tmp1 );
	  return;
	}
	// Switch noteboards.
	pc->noteboard = i;
	fsend( ch, "Note now works on the %s noteboard. ( %d notes )",
	       noteboard_name[i], max_note[i] );
      }
      return;
    }

    if( !can_write( pc, noteboard, true ) )
      return;

    if( !acceptable_title( ch, argument ) )
      return;

    note = new note_data;
    note->title = alloc_string( argument, MEM_NOTE );
    note->from = alloc_string( ch->descr->name, MEM_NOTE );
    note->noteboard = -1;
    fsend( ch, "Note created with title \"%s\".", note->title );
    pc->note_edit = note;
    return;
  }

  if( !strcasecmp( argument, "delete" ) ) {
    if( note->noteboard != -1 ) {
      send( ch, "You stop editing the note.\n\r" );
      pc->note_edit = 0;
      return;
    }
    send( ch, "You delete the unposted note.\n\r" );
    delete note;
    pc->note_edit = 0;
    return;
  }

  if( exact_match( argument, "title" ) ) {
    if( !can_write( pc, note->noteboard, true )
	|| !acceptable_title( ch, argument ) )
      return;
    free_string( note->title, MEM_NOTE );
    note->title = alloc_string( argument, MEM_NOTE );
    fsend( ch, "Note title changed to \"%s\".", note->title );
    return;
  }

  if( exact_match( argument, "preview" ) ) {
    page( ch, " Title: %s\n\r  From: %s\n\r\n\r",
	  note->title, note->from );
    convert_to_ansi( ch, 3*MAX_STRING_LENGTH, note->message, tmp1 );
    page( ch, tmp1 );
    return;
  }

  if( !strcasecmp( argument, "post" ) ) {
    if( !can_write( pc, note->noteboard, true ) )
      return;
    if( note->noteboard == -1 ) {
      note->noteboard = noteboard;
      fsend( ch, "Note \"%s\" posted on %s board.",
	     note->title, noteboard_name[noteboard] );
      snprintf( tmp1, 3*MAX_STRING_LENGTH, "Note \"%s\" posted by %s on the %s board.",
		note->title, ch->descr->name, noteboard_name[noteboard] );
      info( noteboard_level( noteboard ),
	    empty_string,
	    0,
	    tmp1, MAX_IFLAG+noteboard, 1, ch, 
	    noteboard == NOTE_CLAN ? ch->pcdata->pfile->clan : 0 );
      note->date = current_time;
    } else {
      fsend( ch, "Modifications to note \"%s\" saved on %s board.",
	     note->title, noteboard_name[ note->noteboard ] );
      snprintf( tmp1, 3*MAX_STRING_LENGTH, "Note \"%s\" on %s board modified by %s.",
		note->title, noteboard_name[ note->noteboard ],
		ch->descr->name );
      snprintf( tmp2, THREE_LINES, "Note \"%s\" on %s board modified by a mail daemon.",
		note->title, noteboard_name[ note->noteboard ] );
      info( noteboard_level( noteboard ),
	    tmp2,
	    invis_level( pc ),
	    tmp1, MAX_IFLAG+noteboard, 2, ch,
	    noteboard == NOTE_CLAN ? ch->pcdata->pfile->clan : 0 );
      note->update = current_time;
      for( i = 0; note_list[noteboard][i] != note; i++ ) {
        if( i == max_note[noteboard]-1 ) {
          bug( "Do_Note: Modified note not found!" );
          return;
	}
      }
      remove( note_list[noteboard], max_note[noteboard], i );
    }
    insert( note_list[noteboard], max_note[noteboard], note,
	    max_note[noteboard] );
    pc->note_edit = 0;
    save_notes( noteboard, clan );
    return;
  }

  if( *argument
      && !can_write( pc, note->noteboard, true ) )
    return;

  const char *s = 0;
  if( !strcmp( argument, "spell" ) && ( s = spell( ch, note->title, "Title: " ) ) ) {
    page( ch, s );
    if( ( s = spell( ch, pc->note_edit->message ) ) ) {
      page( ch, s );
    } else {
      page( ch, "No spelling errors found in note text.\n\r" );
    }
    return;

  } else if( !*argument || !is_set( ch->pcdata->pfile->flags, PLR_EDITOR_QUIET ) ) {
    page( ch, "Title: %s\n\r\n\r", note->title );
  }

  pc->note_edit->message = edit_string( ch, argument,
					pc->note_edit->message, MEM_NOTE, true );
}


void display_note( char_data *ch, int i, const note_data *note )
{
  page( ch, "[%3d] %-15s %-40s %s\n\r",
	i, note->from, note->title, ltime( note->Date( ), false, ch ) );
}


void display_noteboard( player_data* ch )
{
  bool reverse  = is_set( ch->pcdata->pfile->flags, PLR_REVERSE );
  int noteboard = ch->noteboard;
  int max       = max_note[ noteboard ];

  bool found = false;

  for( int i = 0; i < max; i++ ) {
    const note_data *note = note_list[ noteboard ][ reverse ? max-i-1 : i ];
    if( !found ) {
      found = true;
      page_title( ch, "%s Noteboard", noteboard_name[ noteboard ] );
    }
    display_note( ch, i+1, note );
  }

  if( !found ) 
    send( ch, "There are no messages.\n\r" );
}


void note_summary( char_data* ch )
{
  bool found  = false;

  if( const clan_data *clan = ch->pcdata->pfile->clan ) {
    note_list[ NOTE_CLAN ] = clan->note_list;
    max_note[ NOTE_CLAN ] = clan->max_note;
  }

  for( int i = 0; i < MAX_NOTEBOARD; ++i ) {
    if( !can_read( ch, i ) )
      continue; 
    const int max = max_note[i];
    int j = 0;
    for( ; j < max; ++j )
      if( note_list[i][max-j-1]->Date( ) < ch->pcdata->pfile->last_note ) 
        break;
    if( j > 0 ) {
      if( !found ) { 
        send_underlined( ch, "New Notes\n\r" );
        found = true;
      }
      send( ch, "%15s : %d\n\r", noteboard_name[i], j );
    }
  }

  if( !found )
    send( ch, "There have been no new notes since last login.\n\r" );
}


/*
 *   MAIL ROUTINES
 */

static const char *const message0 =
"A mail daemon runs up to you. You hand it your letter and a silver coin.";

static const char *const message1 =
"The mail daemon takes the letter, opens it, reads it carefully,\
 rolls on the floor laughing, and wanders off to find %s.";

static const char *const message2 =
"The mail daemon takes the letter and sprints off to find %s.";

static const char *const message3 =
"You give a mail daemon your letter.  The mail daemon loiters nearby and\
 whistles a tune.  When it realizes you aren't going to pay postage, it\
 stamps around just out of reach and mumbles rudely about immortals,\
 before slowly wandering off in the wrong direction to find %s.";

static const char *const message4 =
"You give a mail daemon your letter.  It stands nearby with its hand out\
 for a few minutes and then grumbles and wanders off to find %s, while chewing on a\
 corner of the letter.";

static const char *const message5 =
"The mail daemon looks at you, then at %s. It thumps you on the head,\
 hands your letter to %s, and wanders away grumbling.";

static const char *const dream_msg =
"You really don't want to invite a daemon into your dreams.\n\r";


void mail_message( char_data* ch )
{
  /*
  int total = 0;
  //  int         recent  = 0;

  for( const note_data *note = ch->pcdata->pfile->mail; note; note = note->next ) {
    ++total;
    //    if( note->date > ch->pcdata->pfile->last_note ) 
    //      ++recent;
  }
  */

  const int total = count( ch->pcdata->mail );

  if( total != 0 ) {
    /*
    if( recent != 0 ) 
      snprintf( tmp, TWO_LINES, "You have %s mail message%s.",
		number_word( total ), total == 1 ? "" : "s" );
    else 
    */
    send_centered( ch, "You have %s mail message%s.",
		   number_word( total ), total == 1 ? "" : "s" );
  }
}


note_data* find_mail( char_data* ch, int number )
{
  note_data *note = ch->pcdata->mail;

  if( is_set( ch->pcdata->pfile->flags, PLR_REVERSE ) )
    number = count( note )-number+1;

  for( int line = 0; note; note = note->next )
    if( ++line == number )
      return note;

  send( ch, "You have no mail by that number.\n\r" );

  return 0;
}


static void send_mail( char_data* ch, char letter, const char *argument )    
{
  if( !*argument ) {
    send( ch, "%s the message to whom?\n\r",
	  letter == 'C' ? "Cc" : "Send" );
    return;
  }

  pfile_data *pfile;
  
  if( exact_match( argument, "self" ) ) {
    pfile = ch->pcdata->pfile;
  } else if( !( pfile = find_pfile( argument ) ) ) {
    send( ch, "There is no one by that name.\n\r" );
    return;
  }

  player_data *pl;

  if( ch->pcdata->pfile != pfile ) {
    if( ch->position == POS_SLEEPING ) {
      send( ch, dream_msg );
      return;
    }
    if( pfile->Filtering( ch ) ) {
      fsend( ch, "%s is filtering you - please leave %s in peace.",
	     pfile->name, pfile->Him_Her( ) );
      return; 
    }
    if( ch->Filtering( pfile ) ) {
      fsend( ch,
	     "You are filtering %s and only a chebucto would want to send mail to with someone they are filtering.",
	     pfile->name );
      return;
    } 
    if( !remove_silver( ch ) ) {
      if( ch->Level() < LEVEL_APPRENTICE ) {
        send( ch,
	      "You don't have the silver coin required to mail a letter.\n\r" );
        return;
      }
      fsend( ch, number_range( 0, 1 ) == 0 ? message3 : message4,
	     pfile->name );
      pl = find_player( pfile );
    } else {
      fsend( ch, message0 );
      pl = find_player( pfile );
      if( pl
	  && pl->in_room == ch->in_room
	  && pl->Seen( ch )
	  && ch->Seen( pl )
	  && ch->Recognizes( pl ) ) {
	fsend( ch, message5, pfile->name, pfile->name );
      } else {
	fsend( ch, number_range( 0, 25 ) == 0 ? message1 : message2,
	       pfile->name );
      }
    }
  } else {
    fsend( ch, "You %s the letter to yourself.",
	   letter == 'S' ? "send" : "cc a copy of" );
    pl = player( ch );
  }

  note_data *note = ch->pcdata->mail_edit;

  if( letter != 'S' ) {
    note_data *note_new   = new note_data;
    note_new->title       = alloc_string( note->title,    MEM_NOTE );
    note_new->message     = alloc_string( note->message,  MEM_NOTE );
    note_new->from        = alloc_string( note->from,     MEM_NOTE );
    note_new->noteboard   = NOTE_PRIVATE;
    note                  = note_new;
  } else {
    ch->pcdata->mail_edit = 0;
  }

  note->date = current_time;

  if( pl ) {
    append( pl->pcdata->mail, note );
    save_mail( pfile, pl->pcdata->mail );
    if( pl != ch
	&& pl->link
	&& pl->link->connected == CON_PLAYING ) {
      fsend( pl, "A mail daemon runs up and hands you a letter from %s.", ch->descr->name );
    }
  } else {
    note_data *mail = read_mail( pfile );
    append( mail, note );
    save_mail( pfile, mail );
    delete_list( mail );
  }

  /*
  if( ch->pcdata->pfile != pfile ) 
    for( link_data *link = link_list; link; link = link->next ) {
      if( link->character && link->connected == CON_PLAYING
	  && link->character->pcdata->pfile == pfile ) {
        fsend( link->character,
	       "A mail daemon runs up and hands you a letter from %s.", ch );
        break;
      }
    }
  */
}


void do_mail( char_data* ch, const char *argument )
{
  char               tmp  [ 3*MAX_STRING_LENGTH ];
  note_data*        note;
  note_data*    note_new;
  int                  i;

  if( is_confused_pet( ch ) || !ch->pcdata )
    return;

  if( ch->species ) {
    send( ch, "You can only use the mail system in human form.\n\r" );
    return;
  }
  
  if( !( note = ch->pcdata->mail_edit ) ) {
    if( !*argument ) {
      display_mailbox( ch );
      return;
    }
    
    if( isdigit( *argument ) ) {
      if( !( note = find_mail( ch, atoi( argument ) ) ) )
        return;
      page( ch, "Title: %s\n\r", note->title );
      page( ch, " From: %s\n\r", note->from );
      page( ch, " Sent: %s\n\r\n\r", ltime( note->date, false, ch ) );
      convert_to_ansi( ch, 3*MAX_STRING_LENGTH, note->message, tmp );
      page( ch, tmp );
      return;
    }
    
    if( exact_match( argument, "delete" ) ) {
      if( !ch->pcdata->mail ) {
        send( ch, "You have no mail to delete.\n\r" );
        return;
      }
      if( !strcasecmp( argument, "all" ) ) {
        delete_list( ch->pcdata->mail );
        send( ch, "All mail messages deleted.\n\r" );
      } else {
        if( !( note = find_mail( ch, atoi( argument ) ) ) )
          return;
        remove( ch->pcdata->mail, note );
        fsend( ch, "Mail message \"%s\" deleted.",
	       note->title );
        delete note;
      }
      save_mail( ch->pcdata->pfile, ch->pcdata->mail );
      return;
    }

    if( exact_match( argument, "edit" ) ) {
      if( !( note = find_mail( ch, i = atoi( argument ) ) ) )
        return;
      note_new               = new note_data;
      note_new->title        = alloc_string( note->title, MEM_NOTE );
      note_new->message      = alloc_string( note->message, MEM_NOTE );
      note_new->from         = alloc_string( ch->real_name( ), MEM_NOTE );
      note_new->noteboard    = NOTE_PRIVATE;
      ch->pcdata->mail_edit  = note_new;
      send( ch, "Copied message %d to mail buffer.\n\r", i );
      return;
    }

    if( exact_match( argument, "search" ) ) {
      if( !*argument ) {
        send( ch, "What text do you wish to search for?\n\r" );
        return;
      }

      const bool rev = is_set( ch->pcdata->pfile->flags, PLR_REVERSE );

      if( rev )
	reverse( ch->pcdata->mail );

      int line = 1;
      unsigned count = 0;

      for( const note_data *note = ch->pcdata->mail; note; note = note->next ) {
	if( search_text( note->from, argument )
	    || search_text( note->title, argument )
	    || search_text( note->message, argument ) ) {
	  if( count == 0 ) {
	    page_title( ch, "Mailbox Search Results" );
	  }
	  display_mail( ch, line, note );
	  ++count;
	}
	++line;
      }

      if( rev )
	reverse( ch->pcdata->mail );

      if( count == 0 ) {
	send( ch, "No matching message found in your mailbox.\n\r" );
      } else {
	page( ch, "\n\r  %u matching message%s found.\n\r",
	      count, count == 1 ? "" : "s" );
      }
      return;
    }

    if( is_set( ch->pcdata->pfile->flags, PLR_NO_MAIL ) ) {
      send( ch, "The immortals have revoked your mail privileges.\n\r" );
      return;
    }

    exact_match( argument, "new" );

    if( !acceptable_title( ch, argument ) )
      return;
 
    note                   = new note_data;
    note->title            = alloc_string( argument, MEM_NOTE );
    note->message          = alloc_string( empty_string, MEM_NOTE );
    note->from             = alloc_string( ch->real_name( ), MEM_NOTE );
    note->noteboard        = NOTE_PRIVATE;
    ch->pcdata->mail_edit  = note;
    fsend( ch, "Starting new message with subject \"%s\".", note->title );
    return;
  }
  
  if( exact_match( argument, "title" ) ) {
    if( !acceptable_title( ch, argument ) )
      return;
    free_string( note->title, MEM_NOTE );
    note->title = alloc_string( argument, MEM_NOTE );
    fsend( ch, "Message subject changed to \"%s\".", argument );
    return;
  }    
  
  if( exact_match( argument, "preview" ) ) {
    page( ch, "Title: %s\n\r", note->title );
    page( ch, " From: %s\n\r\n\r", note->from );
    convert_to_ansi( ch, 3*MAX_STRING_LENGTH, note->message, tmp );
    page( ch, tmp );
    return;
  }

  if( exact_match( argument, "send" ) ) {
    send_mail( ch, 'S', argument );
    return;
  }
  
  if( exact_match( argument, "cc" ) ) {
    send_mail( ch, 'C', argument );
    return;
  }
  
  if( !strcasecmp( argument, "delete" ) ) {
    send( ch, "The message you were editing has been deleted.\n\r" );
    delete note;
    ch->pcdata->mail_edit = 0;
    return;
  }
  
  const char *s = 0;
  if( !strcmp( argument, "spell" ) && ( s = spell( ch, note->title, "Subject: " ) ) ) {
    page( ch, s );
    if( ( s = spell( ch, ch->pcdata->mail_edit->message ) ) ) {
      page( ch, s );
    } else {
      page( ch, "No spelling errors found in message text.\n\r" );
    }
    return;

  } else if( !*argument || !is_set( ch->pcdata->pfile->flags, PLR_EDITOR_QUIET ) ) {
    page( ch, "Subject: %s\n\r\n\r", note->title );
  }
  
  ch->pcdata->mail_edit->message = edit_string( ch, argument,
						ch->pcdata->mail_edit->message, MEM_NOTE, true );
}


void display_mail( char_data *ch, int i, const note_data *note )
{
  page( ch, "[%3d] %-15s %-40s %s\n\r",
	i, note->from, note->title, ltime( note->date, false, ch ) );
}


void display_mailbox( char_data* ch )
{
  if( !ch->pcdata->mail ) { 
    send( ch, "You have no mail.\n\r" );
    return;
  }
  
  if( is_set( ch->pcdata->pfile->flags, PLR_REVERSE ) )
    reverse( ch->pcdata->mail );
  
  int line = 0;
  
  bool found = false;

  for( const note_data *note = ch->pcdata->mail; note; note = note->next ) {
    if( !found ) {
      page_title( ch, "Your Mailbox" );
     found = true;
    }
    display_mail( ch, ++line, note );
  }
  
  if( is_set( ch->pcdata->pfile->flags, PLR_REVERSE ) )
    reverse( ch->pcdata->mail );
}


/*
 *   DISK ROUTINES
 */


note_data *read_mail( pfile_data *pfile )
{
  note_data *mail = 0;

  char tmp [ TWO_LINES ];

  snprintf( tmp, TWO_LINES, "%s%s", MAIL_DIR, pfile->name );

  FILE *fp;
  if( ( fp = fopen( tmp, "r" ) ) ) { 
    if( strcmp( fread_word( fp ), "#MAIL" ) ) {
      bug( "Read_mail: missing header", 0 );
      shutdown( "read_mail missing header ", tmp );
    }

    while( true ) {
      char *title = fread_string( fp, MEM_NOTE );
      if( title[0] == '$' ) {
        free_string( title, MEM_NOTE );
        break;
      }

      //      if( strlen( title ) > 40 )
      //        title[40] = '\0';

      note_data *note = new note_data;
      note->title     = title;
      note->from      = fread_string( fp, MEM_NOTE );
      note->message   = fread_string( fp, MEM_NOTE ); 
      note->noteboard = NOTE_PRIVATE;
      note->date      = fread_number( fp ); 
      
      append( mail, note );
    }
    fclose( fp );
  }

  return mail;
}


void save_mail( pfile_data *pfile, note_data *mail )
{
  /*
  if( !pfile ) {
    for( int i = 0; i < max_pfile; ++i ) {
      save_mail( pfile_list[i] );
    }
    return;
  }
  */

  if( !mail ) {
    delete_file( MAIL_DIR, pfile->name, false );
  } else if( FILE *fp = open_file( MAIL_DIR, pfile->name, "w" ) ) {
    fprintf( fp, "#MAIL\n\n" );
    
    for( const note_data *note = mail; note; note = note->next ) {
      fwrite_string( fp, note->title );
      fwrite_string( fp, note->from );
      fwrite_string( fp, note->message );
      fprintf( fp, "%d\n",  (int)( note->date ) );
    }
    
    fwrite_string( fp, "$" );
    fclose( fp );
  }
}


/*
 *   NOTEBOARD DISK ROUTINES
 */


void load_notes( void )
{
  char tmp [ TWO_LINES ];

  echo( "Loading Notes ...\n\r" );

  for( int i = 0; i < MAX_NOTEBOARD; i++ ) 
    if( i != NOTE_CLAN ) {
      snprintf( tmp, TWO_LINES, "%s%s", NOTE_DIR, noteboard_name[i] );
      load_noteboard( tmp, note_list[i], max_note[i], i );
    }
}


void load_notes( clan_data* clan )
{
  char tmp [ ONE_LINE ];

  snprintf( tmp, ONE_LINE, "%s%s", CLAN_NOTE_DIR, clan->abbrev );
  load_noteboard( tmp, clan->note_list, clan->max_note, NOTE_CLAN );
}


void load_noteboard( char* file, note_data**& list, int& max, int noteboard )
{
  list = 0;
  max  = 0;

  FILE *fp = open_file( file, "r" );

  if( strcmp( fread_word( fp ), "#NOTES" ) )
    panic( "Load_notes: missing header" );

  while (true) {
    char *title = fread_string( fp, MEM_NOTE );
    
    if( *title == '$' ) {
      free_string( title, MEM_NOTE );
      break;
    }
    
    note_data *note = new note_data;
    note->title     = title;
    note->from      = fread_string( fp, MEM_NOTE );
    note->message   = fread_string( fp, MEM_NOTE ); 
    note->date      = fread_number( fp ); 
    note->update      = fread_number( fp ); 
    note->noteboard = noteboard;

    insert( list, max, note, max );
  }

  fclose( fp );
}


void save_notes( int i, clan_data* clan )
{
  if( i == -1 ) {
    for( i = 0; i < MAX_NOTEBOARD; ++i ) {
      if( i != NOTE_CLAN ) {
        save_notes( i );
      }
    }
    for( i = 0; i < max_clan; ++i ) {
      save_notes( clan_list[i] );
    }
    return;
  }

  if( i == NOTE_CLAN ) {
    clan->note_list = note_list[ NOTE_CLAN ];
    clan->max_note  = max_note[ NOTE_CLAN ];
    save_notes( clan );
  } else {
    char tmp [ TWO_LINES ];
    snprintf( tmp, TWO_LINES, "%s%s", NOTE_DIR, noteboard_name[i] );
    save_noteboard( tmp, note_list[i], max_note[i] );
  }
}


void save_notes( clan_data *clan )
{
  char tmp [ ONE_LINE ];
  snprintf( tmp, ONE_LINE, "%s%s", CLAN_NOTE_DIR, clan->abbrev );
  save_noteboard( tmp, clan->note_list, clan->max_note );
}


void save_noteboard( char* file, note_data** list, int max )
{ 
  FILE*          fp;

  if( !( fp = open_file( file, "w" ) ) ) 
    return;

  fprintf( fp, "#NOTES\n\n" );

  for( int i = 0; i < max; ++i ) {
    const note_data *note = list[i];
    fwrite_string( fp, note->title );
    fwrite_string( fp, note->from );
    fwrite_string( fp, note->message );
    fprintf( fp, "%d %d\n",
	     (int)( note->date ), (int)( note->update ) );
  }

  fwrite_string( fp, "$" );
  fclose( fp );
}
