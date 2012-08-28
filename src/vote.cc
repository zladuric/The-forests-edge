#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include "define.h"
#include "struct.h"


int Vote_Data::max_serial = 1;

Vote_Data votes [ MAX_VOTE ];


static void vote_summary( char_data* ch, int i, note_data *note = 0 )
{
  char paragraph [ 3*MAX_STRING_LENGTH ];
  int p = 0;

  if( note ) {
    p = snprintf( paragraph, 3*MAX_STRING_LENGTH,
		  "\n\r%*s--| %s |--\n\r\n\r", 40-8-(int)strlen( votes[i].text )/2, "", votes[i].text );
  } else {
    send( ch, "\n\r" );
    send_centered( ch, "--| %s |--", votes[i].text );
    send( ch, "\n\r" );
  }

  int  count_array  [ MAX_PFILE ];
  int    sort_array  [ 20 ];
  int         k;

  vzero( count_array, MAX_PFILE );

  for( int j = 0; j < 20; ++j )
    sort_array[j] = -1;
  
  /* COUNT VOTES */

  for( int j = 0; j < max_pfile; ++j )
    if( pfile_list[j]->vote[i] ) {
      ++count_array[ pfile_list[j]->vote[i]->ident ];
    }
  
  /* SORT VOTES */
  
  for( int j = 0; j < max_pfile; ++j ) {
    for( k = 20; k > 0
	   && ( sort_array[k-1] == -1
		|| count_array[pfile_list[j]->ident] > count_array[sort_array[k-1]] );
	 --k )
      if( k < 20 )
	sort_array[k] = sort_array[k-1];
    if( k < 20 )
      sort_array[k] = pfile_list[j]->ident;
  }
  
  /* DISPLAY VOTES */

  for( int n = 0; n < 10; ++n ) {
    char r1 [ 5 ];
    char r2 [ 5 ];
    char buf1 [ 21 ];
    char buf2 [ 21 ];
    snprintf( r1, 5,  "[%d]", n+1 );
    snprintf( r2, 5,  "[%d]", n+11 );
    int count1 = 0;
    if( sort_array[n] != -1 ) {
      count1 = count_array[ sort_array[n] ];
    }
    if( count1 != 0 ) {
      snprintf( buf1, 21, "%s (%d)", ident_list[sort_array[n]]->name, count1 );
    } else {
      snprintf( buf1, 21, "---" );
    }
    int count2 = 0;
    if( sort_array[n+10] != -1 ) {
      count2 = count_array[ sort_array[n+10] ];
    }
    if( count2 != 0 ) {
      snprintf( buf2, 21, "%s (%d)", ident_list[sort_array[n+10]]->name, count2 );
    } else {
      snprintf( buf2, 21, "---" );
    }
    char tmp [ THREE_LINES ];
    snprintf( tmp, TWO_LINES, "%12s  %-20s %5s  %s\n\r",
	      r1, buf1,
	      r2, buf2 );
    if( note ) {
      p += snprintf( paragraph + p, 3*MAX_STRING_LENGTH-p, tmp );
    } else {
      send( ch, tmp );
    }
  }

  if( note ) {
    note->message = alloc_string( paragraph, MEM_NOTE );
  }
}

void do_vote( char_data* ch, const char *argument )
{
  if( is_mob( ch ) )
    return;

  if( !*argument ) {
    int for_me[ 10 ];
    int for_someone[ 10 ];
    vzero( for_me, 10 );
    vzero( for_someone, 10 );
    for( int j = 0; j < max_pfile; ++j ) {
      for( int i = 0; i < MAX_VOTE; ++i ) {
	if( pfile_list[j]->vote[i] ) {
	  ++for_someone[i];
	  if( pfile_list[j]->vote[i] == ch->pcdata->pfile )
	    ++for_me[i];
	}
      }
    }
    bool found = false;
    for( int i = 0; i < MAX_VOTE; ++i ) {
      if( votes[i].serial == 0 )
	continue;
      if( !found ) {
	send( ch, "\n\r" );
	send_centered( ch, "--| Your Votes |--" );
	send( ch, "\n\r" );
	found = true;
      } else {
	send( ch, "\n\r" );
      }
      send( ch, "[%d] %s (%d vote%s)\n",
	    i+1, votes[i].text, for_someone[i],
	    ( for_someone[i] == 1 ) ? "" : "s" );
      if( pfile_data *pfile = ch->pcdata->pfile->vote[i] ) {
	send( ch, "      You voted for: %s\n\r", pfile->name );
      } else {
	send( ch, "      You have not voted on this issue.\n\r" );
      }
      if( for_me[i] ) {
	send( ch, "      " );
	fsend( ch, "%s player%s voted for you.\n\r",
	       number_word( for_me[i], ch ),
	       for_me[i] == 1 ? " has" : "s have" );
      }
    }
    if( !found ) {
      send( ch, "There are no issues open for voting.\n\r" );
    }
    return;
  }
  
  if( matches( argument, "results" ) ) {
    int i;
    if( !number_arg( argument, i ) ) {
      send( ch, "What issue number do you want summarized?\n\r" );
      return;
    }
    if( i < 1 || i > MAX_VOTE || votes[i-1].serial == 0 ) {
      send( ch, "There is no issue number %d.\n\r", i );
      return;
    }
    vote_summary( ch, i-1 );
    return;
  }
  
  if( is_demigod( ch ) ) {
    if( matches( argument, "new" ) ) {
      int i;
      if( !number_arg( argument, i ) ) {
	send( ch, "What issue number do you want to create?\n\r" );
	return;
      }
      if( i < 1 || i > MAX_VOTE ) {
	send( ch, "There are only %d issue slots.\n\r", MAX_VOTE );
	return;
      }
      if( votes[i-1].serial != 0 ) {
	send( ch, "Issue number %d is already being used.\n\r", i );
	return;
      }
      if( !*argument ) {
	send( ch, "What is the text of issue %d?\n\r", i );
	return;
      }
      votes[i-1].serial = Vote_Data::max_serial++;
      votes[i-1].text = alloc_string( argument, MEM_VOTE );
      fsend( ch, "Issue number %d created: %s", i, argument );
      char *tmp = static_string( );
      snprintf( tmp, THREE_LINES, "New voting issue #%d: \"%s\".",
		i, argument );
      info( 0, empty_string, 0, tmp, IFLAG_ADMIN, 1, ch );
      save_world( );
      return;
    }

    if( matches( argument, "delete" ) ) {
      int i;
      if( !number_arg( argument, i ) ) {
	send( ch, "What issue number do you want to delete?\n\r" );
	return;
      }
      if( i < 1 || i > MAX_VOTE || votes[i-1].serial == 0 ) {
	send( ch, "There is no issue number %d.\n\r", i );
	return;
      }
      votes[i-1].serial = 0;
      free_string( votes[i-1].text, MEM_VOTE );
      votes[i-1].text = empty_string;
      for( int j = 0; j < max_pfile; ++j ) {
	pfile_list[j]->vote[i-1] = 0;
      }
      fsend( ch, "Issue number %d deleted.", i );
      char *tmp = static_string( );
      snprintf( tmp, THREE_LINES, "Voting issue #%d has been deleted.",
		i );
      info( 0, empty_string, 0, tmp, IFLAG_ADMIN, 1, ch );
      save_world( );
      return;
    }

    if( matches( argument, "rename" ) ) {
      int i;
      if( !number_arg( argument, i ) ) {
	send( ch, "What issue number do you want to rename?\n\r" );
	return;
      }
      if( i < 1 || i > MAX_VOTE || votes[i-1].serial == 0 ) {
	send( ch, "There is no issue number %d.\n\r", i );
	return;
      }
      if( !*argument ) {
	send( ch, "To what should the text of issue %d be changed?\n\r", i );
	return;
      }
      free_string( votes[i-1].text, MEM_VOTE );
      votes[i-1].text = alloc_string( argument, MEM_VOTE );
      fsend( ch, "Issue number %d changed to \"%s\".", i, argument );
      char *tmp = static_string( );
      snprintf( tmp, THREE_LINES, "Voting issue #%d has been modified.",
		i );
      info( 0, empty_string, 0, tmp, IFLAG_ADMIN, 3, ch );
      save_world( );
      return;
    }

    if( matches( argument, "summary" ) ) {
      int i;
      if( !number_arg( argument, i ) ) {
	send( ch, "What issue number do you want to summarize?\n\r" );
	return;
      }
      if( i < 1 || i > MAX_VOTE || votes[i-1].serial == 0 ) {
	send( ch, "There is no issue number %d.\n\r", i );
	return;
      }
      player_data *pc = (player_data*) ch;
      if( pc->note_edit ) {
	send( ch, "You are currently editing another note.\n\r", i );
	return;
      }
      pc->note_edit = new note_data;
      char tmp [ THREE_LINES ];
      snprintf( tmp, THREE_LINES, "Voting summary: %s", votes[i-1].text );
      pc->note_edit->title = alloc_string( tmp, MEM_NOTE );
      pc->note_edit->from = alloc_string( ch->descr->name, MEM_NOTE );
      pc->note_edit->noteboard = -1;
      fsend( ch, "Note created with title \"%s\".", pc->note_edit->title );
      send( ch, "\n\r" );
      vote_summary( ch, i-1, pc->note_edit );
      pc->note_edit->message = edit_string( ch, "", pc->note_edit->message, MEM_NOTE, true );
      return;
    }

    if( matches( argument, "clear" ) ) {
      int i;
      if( !number_arg( argument, i ) ) {
	send( ch, "What issue number do you want to clear?\n\r" );
	return;
      }
      if( i < 1 || i > MAX_VOTE || votes[i-1].serial == 0 ) {
	send( ch, "There is no issue number %d.\n\r", i );
	return;
      }
      for( int j = 0; j < max_pfile; ++j ) {
	pfile_list[j]->vote[i-1] = 0;
      }
      fsend( ch, "Issue number %d cleared.", i );
      char *tmp = static_string( );
      snprintf( tmp, THREE_LINES, "Voting issue #%d has been cleared.",
		i );
      info( 0, empty_string, 0, tmp, IFLAG_ADMIN, 2, ch );
      return;
    }
  }

  int i;
  if( !number_arg( argument, i ) ) {
    send( ch, "On which issue number do you wish to vote?\n\r" );
    return;
  }
  
  if( i < 1 || i > MAX_VOTE || votes[i-1].serial == 0 ) {
    send( ch, "There is no issue number %d.\n\r", i );
    return;
  }
  
  if( !*argument ) {
    if( ch->pcdata->pfile->vote[i-1] ) {
      fsend( ch, "You rescind your vote for %s on issue number %d, \"%s\".",
	     ch->pcdata->pfile->vote[i-1]->name, i, votes[i-1].text );
      ch->pcdata->pfile->vote[i-1] = 0;
    } else {
      fsend( ch, "You have not voted on issue number %d, \"%s\".",
	     i, votes[i-1].text );
    }
    return;
  }

  pfile_data *pfile;
  if( !( pfile = find_pfile( argument, ch ) ) )  {
    //    send( ch, "That player does not exist.\n\r" );
    return;
  }
  
  if( pfile->level >= LEVEL_APPRENTICE ) {
    send( ch, "You may not vote for immortals.\n\r" );
    return;
  }

  if( is_banned( pfile->account )
      || is_set( pfile->flags, PLR_DENY ) ) {
    fsend( ch, "You may not currently vote for %s.", pfile->name );
    return;
  }

  fsend( ch, "For issue %d, \"%s\", you vote for %s.",
	 i, votes[i-1].text, pfile->name );
  
  ch->pcdata->pfile->vote[i-1] = pfile;
}  
