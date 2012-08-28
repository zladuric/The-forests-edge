#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include "define.h"
#include "struct.h"


static int max_quest_serial = 0;

static const char *const qflag_name [ MAX_QFLAG ] = {
  "hidden"
};


quest_data *quest_list  [ MAX_QUEST ];


quest_data *get_quest_index( int i )
{
  if( i < 0 || i >= MAX_QUEST ) 
    return 0;

  return quest_list[i];
}


/*
 *   LOCATING QUESTS
 */


void do_qwhere( char_data* ch, const char *argument )
{
  int index;
  if( !number_arg( argument, index ) ) {
    send( ch, "Syntax: qwhere <quest #>\n\r" );
    return;
  }

  quest_data *quest;
  if( !( quest = get_quest_index( index ) ) ) {
    fsend( ch, "Qwhere: No quest has number %d.", index );
    return;
  }
  
  page( ch, "\n\r" );
  char tmp [ THREE_LINES ];
  snprintf( tmp, THREE_LINES, "--- %s (%d) ---", quest->message, index );
  tmp[4] = toupper( tmp[4] );
  page_centered( ch, tmp ); 
  page( ch, "\n\r" );

  if( !search_code( ch, search_quest, (void*)index ) )
    page( ch, "No references to quest #%d were found.\n\r", index );
}


/*
 *   EDITING OF QUESTS
 */


void do_qedit( char_data* ch, const char *argument )
{
  quest_data*    quest;
  wizard_data*     imm;
  int                i;

  if( !( imm = wizard( ch ) ) )
    return;

  if( !*argument ) {
    if( imm->quest_edit ) {
      send( ch, "You stop editing quest #%d.\n\r", imm->quest_edit->vnum );
      imm->quest_edit = 0;
    } else {
      do_qstat( ch, "" );
    }
    return;
  }
 
  if( matches( argument, "delete" ) ) {
    if( !*argument ) {
      send( ch, "What quest do you want to delete?\n\r" );
      return;
    }
   
    if( ( i = atoi( argument ) ) < 0 || i >= MAX_QUEST
	|| !( quest = quest_list[i] ) ) {
      send( ch, "Quest not found to remove.\n\r" );
      return;
    }

    extract( imm, offset( &imm->quest_edit, imm ), "quest" );
    quest_list[i] = 0;
    send( ch, "Quest removed.\n\r" );
    delete quest;
    
    return; 
  }
  
  if( matches( argument, "new" ) ) {
    for( i = 0; quest_list[i]; ++i )
      if( i == MAX_QUEST ) {
        send( ch, "Quest space is full.\n\r" );
        return;
      }
    
    quest = new quest_data( i );
    quest->serial = max_quest_serial++;
    quest_list[i] = quest;
    imm->quest_edit = quest;
    
    send( ch, "Quest created and assigned #%d.\n\r", i );
    return;
  }

  if( ( i = atoi( argument ) ) < 0 || i >= MAX_QUEST ) {
    send( ch, "Quest #%d out of range.\n\r", i );
    return;
  }
  
  if( !quest_list[i] ) {
    send( ch, "There is no quest #%d.\n\r", i );
    return;
  }

  imm->quest_edit = quest_list[i];
  send( ch, "Qset now operates on quest #%d.\n\r", i );
}


void do_qset( char_data* ch, const char *argument )
{
  quest_data*     quest;
  wizard_data*      imm;
 
  if( !*argument ) {
    do_qstat( ch, "" );
    return;
  }
  
  if( !( imm = wizard( ch ) ) )
    return;
  
  if( !( quest = imm->quest_edit ) ) {
    send( ch, "You aren't editing any quest.\n\r" );
    return;
  }
  
  class byte_field byte_list[] = {
    { "points",        0,  25,  &quest->points },
    { "",              0,   0,  0              }
  };

  if( process( byte_list, ch, "quest", argument ) )
    return;
  
  class string_field string_list[] = {
    { "message",    MEM_QUEST,  &quest->message,  0 },
    { "",           0,          0,                0 }   
  };

  if( process( string_list, ch, "quest", argument ) )
    return;

  send( ch, "Unknown parameter.\n\r" );
}


static void page_qflags( char_data *ch, quest_data *quest )
{
  if( !quest->flags ) {
    page( ch, " none" );
    return;
  }

  bool first = true;
  for( int j = 0; j < MAX_QFLAG; ++j ) {
    if( is_set( quest->flags, j ) ) {
      if( first ) {
	page( ch, " %s", qflag_name[j] );
	first = false;
      } else {
	page( ch, ", %s", qflag_name[j] );
      }
    }
  }
}


void do_qstat( char_data* ch, const char *argument )
{
  wizard_data*     imm;
  quest_data*    quest;

  if( ! ( imm = wizard( ch ) ) )
    return;

  if( *argument ) {
    int value;
    if( !number_arg( argument, value ) ) {
      send( ch, "Syntax: qstat [<quest #>]\n\r" );
      return;
    }
    if( value < 0 || value >= MAX_QUEST ) {
      send( ch, "Quest #%d out of range.\n\r", value );
      return;
    }
    if( !( quest = quest_list[value] ) ) {
      send( ch, "There is no quest #%d.\n\r", value );
      return;
    }
  } else if( !( quest = imm->quest_edit ) ) {
    bool found = false;
    for( int i = 0; i < MAX_QUEST; ++i ) {
      if( !( quest = quest_list[i] ) )
        continue;
      if( !found ) {
	page_title( ch, "Quests" );
	found = true;
      }
      page( ch, "[%3d] %-50s\n", i, quest->message );
      if( quest->flags ) {
	page( ch, "      (" );
	page_qflags( ch, quest );
	page( ch, " )\n\r" );
      }
    }
    if( !found ) 
      send( ch, "No quests found.\n\r" );
    return;
  }

  page_title( ch, "Quest #%d", quest->vnum );
  page( ch, "Serial#: %d\n\r", quest->serial );
  page( ch, "Message: %s\n\r", quest->message );
  page( ch, " Points: %d\n\r", quest->points );
  page( ch, "  Flags:" );
  page_qflags( ch, quest );
  page( ch, "\n\r" );
  
  if( quest->comments != empty_string ) {
    page_centered( ch, "-----" );
    page( ch, quest->comments );
  }
}


void do_qflag( char_data *ch, const char *argument )
{
  wizard_data *imm = wizard( ch );

  if( !imm )
    return;

  quest_data *quest = imm->quest_edit;
  
  if( !quest ) {
    send( ch, "You aren't editing any quest.\n\r" );
    return;
  }

  if( !*argument ) {
    display_flags( "*Quest Flags",
		   &qflag_name[0], &qflag_name[1],
		   &quest->flags,
		   MAX_QFLAG, ch );
    return;
  }

  set_flags( &qflag_name[0], &qflag_name[1],
	     &quest->flags, 0, MAX_QFLAG, 0,
	     ch, argument, "quest",
	     false, true );
}


void do_qbug( char_data *ch, const char *argument )
{
  wizard_data *imm = wizard( ch );

  if( !imm )
    return;

  quest_data *quest = imm->quest_edit;
  
  if( !quest ) {
    send( ch, "You aren't editing any quest.\n\r" );
    return;
  }

  quest->comments = edit_string( ch, argument, quest->comments, MEM_QUEST, true );
}


/*
 *   PLAYER COMMANDS
 */


void do_quests( char_data* ch, const char *argument )
{
  if( not_player( ch ) ) 
    return;

  quest_data*       quest;
  bool              found  = false;
  int               flags;
  int               value;
  const char*   title_msg;
  const char*    none_msg; 

  if( !get_flags( ch, argument, &flags, "df", "Quests" ) )
    return;

  if( is_set( flags, 0 ) ) {
    title_msg = "Completed Quests";
    none_msg  = "not completed any";
    value     = QUEST_DONE;
  } else if( is_set( flags, 1 ) ) {
    title_msg = "Failed Quests";
    none_msg  = "not failed any";
    value     = QUEST_FAILED;
  } else {
    title_msg = "Assigned Quests";
    none_msg  = "no unfinished";
    value     = QUEST_ASSIGNED;
  } 
  
  for( int i = 0; i < MAX_QUEST; ++i ) {
    if( ( quest = quest_list[i] )
	&& !is_set( quest->flags, QFLAG_HIDDEN )
	&& ( ch->pcdata->quest_flags[i] == value
	     || ( value == QUEST_ASSIGNED
		  && ch->pcdata->quest_flags[i] != QUEST_NONE
		  && ch->pcdata->quest_flags[i] != QUEST_DONE
		  && ch->pcdata->quest_flags[i] != QUEST_FAILED ) ) ) {
      if( !found ) {
        page_title( ch, title_msg );
        found = true;
      }
      page_centered( ch, quest->message );   
    }
  }
  
  if( !found ) 
    send( ch, "You have %s quests.\n\r", none_msg );
}


/*
 *   EDITTING QUESTS ON PLAYERS
 */


void do_qremove( char_data* ch, const char *argument )
{
  const char *const usage = "Usage: qremove all|<quest_number>\n\r";

  if( not_player( ch ) ) {
    return;
  }

  if( !*argument ) {
    send( ch, usage );
    return;
  }

  int i;

  if( !strcasecmp( argument, "all" ) ) {
    for( int i = 0; i < MAX_QUEST; i++ ) 
      ch->pcdata->quest_flags[i] = QUEST_NONE;
    
    ch->pcdata->quest_pts = 0;
    update_score( ch );
    
    send( ch, "All quest records erased for your character.\n\r" );
    return;
  } else if( !number_arg( argument, i ) ) {
    send( ch, usage );
    return;
  }
  
  if( ch->pcdata->quest_flags[i] == QUEST_NONE ) {
    send( ch, "You have not been assigned quest #%d.\n\r", i );
    return;
  }
  
  if( ch->pcdata->quest_flags[i] == QUEST_DONE ) {
    ch->pcdata->quest_flags[i] = QUEST_NONE;
    ch->pcdata->quest_pts -= quest_list[i]->points;
    ch->pcdata->quest_pts = max( 0, ch->pcdata->quest_pts );
    send( ch, "Removed completed quest #%d and %d quest points.\n\r",
	  i, quest_list[i]->points );
    return;
  } else if( ch->pcdata->quest_flags[i] == QUEST_FAILED ) {
    ch->pcdata->quest_flags[i] = QUEST_NONE;
    send( ch, "Removed failed quest #%d.\n\r", i );
    return;
  }
  
  ch->pcdata->quest_flags[i] = QUEST_NONE;
  send( ch, "Removed assigned quest #%d.\n\r", i );
}


void do_cflag( char_data* ch, const char *argument )
{
  wizard_data*     imm;
  int                i;

  if( !( imm = wizard( ch ) ) )
    return;
  
  player_data *pc;

  bool loaded = false;

  if( *argument
      && !isdigit( *argument ) ) {
    in_character = false;

    char arg [ MAX_INPUT_LENGTH ];

    argument = one_argument( argument, arg );

    pfile_data *pfile = find_pfile( arg, ch );

    if( !pfile )
      return;

    if( pfile != ch->pcdata->pfile
	&& pfile->trust >= get_trust( ch ) ) {
      fsend( ch, "You cannot view the cflags of %s.", pfile->name );
      return;
    }
    
    if( !( pc = find_player( pfile ) ) ) {
      link_data link;
      link.connected = CON_PLAYING;
      if( !load_char( &link, pfile->name, PLAYER_DIR ) ) {
	bug( "Load_players: error reading player file. (%s)", pfile->name );
	return;
      }
      pc = link.player;
      loaded = true;
    }

  } else {
    pc = ( !imm->player_edit
	   ? (player_data *) ch : imm->player_edit );
  }

  if( !*argument ) {
    const unsigned columns = ch->pcdata->columns / 9;
    page_title( ch, "Cflags of %s", pc->descr->name );
    for( i = 0; i < 32*MAX_CFLAG; ++i ) {
      page( ch, "%5d (%1c)", i,
	    is_set( pc->pcdata->cflags, i ) ? '*' : ' ' );
      if( i%columns == columns-1 )
        page( ch, "\n\r" );
    }
    if( i%columns != 0 )
      page( ch, "\n\r" );

  } else if( !number_arg( argument, i ) ) {
    send( ch, "Syntax: cflags [<cflag #>]\n\r" );

  } else if( i < 0 || i >= 32*MAX_CFLAG ) {
    send( ch, "Cflag number out of range.\n\r" );

  } else {
    switch_bit( pc->pcdata->cflags, i );
    
    fsend( ch, "Cflag %d on %s set to %s.", i,
	   pc == ch ? "yourself" : pc->descr->name,
	   is_set( pc->pcdata->cflags, i ) ? "true" : "false" );
  }

  if( loaded ) {
    send( ch, "\n\r" );
    send_centered( ch, "[ Player file was loaded from disk. ]" );
    pc->Save( false );
    pc->Extract();
    extracted.delete_list();
  }
}


void do_cwhere( char_data* ch, const char *argument )
{
  int index;
  if( !number_arg( argument, index ) ) {
    send( ch, "Syntax: cwhere <cflag #>\n\r" );
    return;
  }

  if( index < 0 || index >= 32*MAX_CFLAG ) {
    send( ch, "No cflag number %d.\n\r", index );
    return;
  }
  
  page( ch, "\n\r" );
  char tmp [ TWO_LINES ];
  snprintf( tmp, TWO_LINES, "--- CFlag %d ---", index );
  page_centered( ch, tmp ); 
  page( ch, "\n\r" );

  if( !search_code( ch, search_cflag, (void*)index ) )
    page( ch, "No references to cflag #%d were found.\n\r", index );
}


/*
 *   FILE ROUTINES
 */

   
void load_quests( )
{
  char        letter;
  int           vnum;

  echo( "Loading quests...\n\r" );

  FILE *fp = open_file( AREA_DIR, QUEST_FILE, "r", true );

  if( strcmp( fread_word( fp ), "#QUESTS" ) ) 
    panic( "Load_quests: header not found" );

  max_quest_serial = fread_number( fp );

  while( true ) {
    if( ( letter = fread_letter( fp ) ) != '#' ) 
      panic( "Load_quests: # not found." );
   
    if( ( vnum = fread_number( fp ) ) == -1 )
      break;

    quest_data *quest = new quest_data( vnum );

    quest->serial = fread_number( fp );
    quest->points = (unsigned char) fread_number( fp );
    quest->flags = fread_number( fp );
    quest->message = fread_string( fp, MEM_QUEST );
    quest->comments = fread_string( fp, MEM_QUEST );

    quest_list[vnum] = quest;
  }
  
  fclose( fp );
}


void save_quests( )
{
  rename_file( AREA_DIR, QUEST_FILE,
	       AREA_PREV_DIR, QUEST_FILE );
  
  FILE *fp;

  if( !( fp = open_file( AREA_DIR, QUEST_FILE, "w" ) ) ) {
    return;
  }

  fprintf( fp, "#QUESTS\n" );
  fprintf( fp, "%d\n\n", max_quest_serial );
 
  for( int i = 0; i < MAX_QUEST; ++i ) 
    if( const quest_data *quest = quest_list[i] ) {
      fprintf( fp, "#%d %d\n", quest->vnum, quest->serial );
      fprintf( fp, "%d\n", (int)quest->points );
      fprintf( fp, "%d\n", (int)quest->flags );
      fwrite_string( fp, quest->message );
      fwrite_string( fp, quest->comments );
      fprintf( fp, "\n" );
    }

  fprintf( fp, "#-1\n" );
  fclose( fp );
}
