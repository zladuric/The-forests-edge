#include <ctype.h>
#include <dirent.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <unistd.h>
#include "define.h"
#include "struct.h"
#include "html.h"


clan_data**   clan_list  = 0;
int            max_clan  = 0;


void           display           ( char_data*, clan_data* );
static void           extract           ( clan_data* );
bool           edit_clan         ( char_data*, clan_data*, bool = false );
static void           expel             ( char_data*, clan_data*, pfile_data* );


const char *clan_flags [ MAX_CLAN_FLAGS ] = { "Approved", "Exist.Known",
					      "Members.Known", "Immortal" };


Clan_Data ::  Clan_Data( const char* s1 )
  : name(empty_string),charter(empty_string),
    min_level(10), classes(0), races(0),
    alignments(0), sexes(0),
    date(current_time), modified(false),
    note_list(0), max_note(0)
{
  record_new( sizeof( clan_data ), MEM_CLAN );
  abbrev = alloc_string( s1, MEM_CLAN );
  *abbrev = toupper( *abbrev );
  vzero( flags, 2 );
  int pos = pntr_search( clan_list, max_clan, abbrev );
  if( pos < 0 )
    pos = -pos-1;
  insert( clan_list, max_clan, this, pos );
}


Clan_Data ::~Clan_Data( )
{
  record_delete( sizeof( clan_data ), MEM_CLAN );
  free_string( name, MEM_CLAN );
  free_string( abbrev, MEM_CLAN );
  free_string( charter, MEM_CLAN );
}


static bool knows( char_data* ch, clan_data* clan )
{
  return( ch && ch->pcdata->pfile->clan == clan
	  || ( is_set( clan->flags, CLAN_APPROVED )
	       && is_set( clan->flags, CLAN_KNOWN ) )
	  || has_permission( ch, PERM_CLANS ) );
}


/*
 *   CLAN ROUTINES
 */


clan_data* find_clan( char_data* ch, const char *argument )
{
  clan_data*  clan;

  for( int i = 0; i < max_clan; i++ ) 
    if( knows( ch, clan = clan_list[i] )
	&& ( fmatches( argument, clan->name )
	     || fmatches( argument, clan->abbrev ) ) )
      return clan;

  send( ch, "No such clan exists.\n\r" );

  return 0;
}


bool edit_clan( char_data* ch, clan_data* clan, bool msg )
{
  title_data*  title;

  if( !clan ) {
    if( msg ) 
      send( ch, "You are not in a clan so editing it makes no sense.\n\r" );
    return false;
  }
  
  if( !has_permission( ch, PERM_CLANS ) ) {
    if( is_set( clan->flags, CLAN_APPROVED ) ) {
      if( msg )
        send( ch, "After the clan is approved it may not be edited.\n\r" );
      return false;
    }
    if( !( title = get_title( ch->pcdata->pfile ) )
	|| !is_set( title->flags, TITLE_EDIT_CHARTER ) ) {
      if( msg )
        send( ch, "You don't have permission to edit the clan.\n\r" );
      return false;
    }
  }
  
  return true;
}


static void clan_options( char_data* ch, clan_data* clan,
			  const char *argument )
{
#define types 5

  if( !clan ) {
    send( ch,
      "You cannot edit or view options for a clan you are not in.\n\r" );
    return;
  }

  const char* title [types] = {
    "*Options", "*Races", "*Classes",
    "*Alignments", "*Sexes" };
  int max [types] = {
    MAX_CLAN_FLAGS, MAX_PLYR_RACE, MAX_CLSS, table_max[ TABLE_ALIGNMENT ], MAX_SEX-1 };
  const char **name1 [types] = {
    &clan_flags[0], &race_table[0].name, &clss_table[0].name,
    &alignment_table[0].name, &sex_name[0] }; 
  const char **name2 [types] = {
    &clan_flags[1], &race_table[1].name, &clss_table[1].name,
    &alignment_table[1].name, &sex_name[1] }; 
  int* flag_value [types] = { clan->flags, &clan->races, &clan->classes,
			      &clan->alignments, &clan->sexes };
  const int can_edit [types] = { 1, -1, -1, -1, -1 };
  const int not_edit [types] = { 1,  1,  1,  1,  1 };

  const bool sort [types] = { true, true, true, false, false };
  
  //  const char *response =
  flag_handler( title, name1, name2, flag_value, 0, max,
		edit_clan( ch, clan ) ? can_edit : not_edit, sort,
		has_permission( ch, PERM_CLANS ) ? 0
		: "You do not have permission to alter that flag.\n\r",
		ch, argument, "clan",
		types );
  
  clan->modified = true;
  
#undef types
}


static void clan_level( char_data* ch, clan_data* clan,
			const char *argument )
{
  if( !clan ) {
    send( ch,
	  "You cannot change the minimum level for a clan you are not in.\n\r" );
    return;
  }

  if( !edit_clan( ch, clan, true ) )
    return;

  if( !*argument ) {
    fsend( ch, "The minimum experience level for players to join the clan is %d.",
	   clan->min_level );
    return;
  }
  
  int level;

  if( !number_arg( argument, level ) ) {
    send( ch, "Syntax: clans -l <level>\n\r" );
    return;
  }

  if( level < 1 || level > 90 ) {
    send( ch, "The level can be set from 1 to 90.\n\r" );
    return;
  }

  if( level > ch->Level( )
      && !has_permission( ch, PERM_CLANS ) ) {
    send( ch, "That would render you unable to join your own clan!\n\r" );
    return;
  }

  send( ch, "Minimum level to join the clan set to %d.\n\r", level );
  clan->min_level = level;
  clan->modified = true;
}


/*
static void clan_titles( char_data* ch, clan_data* clan, const char *argument )
{
  if( !clan ) {
    send( ch, "Since you aren't in a clan the T option is meaningless.\n\r" );
    return;
  }

  if( !*argument ) {
    page_underlined( ch, "%-20s %s\n\r", "Title", "Character" );
    for( int i = 0; i < clan->titles.size; i++ ) {
      title_data *title = clan->titles.list[i];
      page( ch, "%-20s %s\n\r", title->name,
	    title->pfile ? title->pfile->name : "No one" );
    }
    return;
  }
  
  send( ch, "Editing of titles is not yet enabled.\n\r" );
}
*/


static clan_data *create_clan( pfile_data* pfile, const char *name )
{
  clan_data*    clan  = new clan_data( name );
  title_data*  title  = new title_data( "Founder", pfile );

  set_bit( pfile->flags, PLR_CTELL );
  add_member( clan, pfile );
  insert( clan->titles.list, clan->titles.size, title, 0 );

  set_bit( title->flags, TITLE_SET_FLAGS    );
  set_bit( title->flags, TITLE_EDIT_CHARTER );
  set_bit( title->flags, TITLE_RECRUIT      );
  set_bit( title->flags, TITLE_REMOVE_NOTES );

  set_bit( clan->classes, pfile->clss );
  set_bit( clan->races, pfile->race );
  set_bit( clan->alignments, pfile->alignment );
  set_bit( clan->sexes, pfile->sex );

  save_clans( clan );
  save_notes( clan );

  return clan;
}


static void rename_clan( clan_data* clan, const char *new_name )
{
  const char *old = clan->abbrev;
  clan->abbrev = alloc_string( new_name, MEM_CLAN );
  *clan->abbrev = toupper( *clan->abbrev );

  rename_file( CLAN_DIR,      old, CLAN_DIR,      clan->abbrev );
  rename_file( CLAN_NOTE_DIR, old, CLAN_NOTE_DIR, clan->abbrev );

  free_string( old, MEM_CLAN );
}


static bool good_clan_abbrev( char_data* ch, const char *name, const clan_data *clan ) 
{
  int l = strlen( name );
  
  if( l < 2 || l > 4 ) {
    send( ch,
	  "Clan acronym must be at least two and no more than four letters.\n\r" );
    return false;
  }
  
  for( int i = 0; i < l && name[i]; i++ ) {
    if( !isalpha( name[i] ) ) {
      send( ch, "The clan acronym may only contain letters.\n\r" );
      return false;
    }
  }
  
  for( int i = 0; i < max_clan; i++ ) {
    if( clan && clan_list[i] == clan ) {
      continue;
    }
    int n = min( l, strlen( clan_list[i]->abbrev ) );
    if( !strcasecmp( name, clan_list[i]->abbrev ) ) {
      send( ch, "There is already a clan using that acronym.\n\r" );
      return false;
    }
    if( !strncasecmp( name, clan_list[i]->abbrev, n ) ) {
      fsend( ch, "That acronym is too similar to the existing clan \"%s\".", clan_list[i]->abbrev );
      return false;
    }
  }
  
  return true;
} 


static const char* create_msg = "Summoning a scribe daemon you request that the\
 paperwork be drawn up to form a clan with the acronym of \"%s\".  It politely\
 nods its head, scribbles a few lines on the back of an envelope, hands you\
 the envelope, and then informs you that your bank account will be debited\
 2000 sp for services rendered.";


void do_clans( char_data* ch, const char *argument )
{
  title_data*    title;
  pfile_data*    pfile;
  int            flags;
  int                i;

  if( is_mob( ch ) )
    return;

  player_data *pc = player( ch );
  clan_data *clan = ch->pcdata->pfile->clan;

  if( !get_flags( ch, argument, &flags, "folcna", "Clans" ) )
    return;
  
  if( exact_match( argument, "expel" ) ) {
    if( !( title = get_title( ch->pcdata->pfile ) )
	|| !is_set( title->flags, TITLE_RECRUIT ) ) {
      send( ch, "You don't have permission to expel clan members.\n\r" );
      return;
    }
    if( !*argument ) {
      send( ch, "Whom do you wish to expel from the clan?\n\r" );
      return;
    }
    if( ( pfile = find_pfile( argument, ch ) ) ) 
      expel( ch, clan, pfile );
    return;
  }
  
  if( has_permission( ch, PERM_CLANS )
      && exact_match( argument, "delete" ) ) {
    if( !*argument ) {
      send( ch, "Which clan do you wish to delete?\n\r" );
    } else if( ( clan = find_clan( ch, argument ) ) ) {
      char tmp [ TWO_LINES ];
      char tmp1 [ TWO_LINES ];
      snprintf( tmp, TWO_LINES, "Clan \"%s\" deleted.",
		clan->abbrev );
      snprintf( tmp1, TWO_LINES, "Clan \"%s\" deleted by %s.",
		clan->abbrev, ch->pcdata->pfile->name );
      info( LEVEL_BUILDER, tmp, invis_level( ch ), tmp1, IFLAG_ADMIN, 1, ch );
      send( ch, "Clan \"%s\" deleted.\n\r", clan->abbrev );
      info( 0, empty_string, 0, "Your clan has been deleted.", IFLAG_CLANS, 1, ch, clan );
      extract( clan );
    }
    return;
  }
  
  if( is_set( flags, 0 ) ) {
    if( clan ) {
      send( ch, "You cannot form a clan if already a member of one.\n\r" );
      return;
    }
    if( pc->Level() < 20 ) {
      send( ch, "You must be at least 15th level to form a clan.\n\r" );
      return;
    }
    if( pc->bank < 20000 ) {
      fsend( ch,
	     "There is a 2000 sp charge, withdrawn from bank, for forming a clan and you lack sufficent funds." );
      return;
    }
    if( !*argument ) {
      send( ch, "What acronym do you wish for the clan?\n\r" );
      return;
    }
    if( good_clan_abbrev( ch, argument, 0 ) ) {
      clan = create_clan( ch->pcdata->pfile, argument );
      pc->bank -= 20000;
      fsend( ch, create_msg, clan->abbrev );
      char tmp [ TWO_LINES ];
      snprintf( tmp, TWO_LINES, "New clan \"%s\" created by %s.",
		clan->abbrev, ch->pcdata->pfile->name );
      info( LEVEL_BUILDER, empty_string, 0, tmp, IFLAG_ADMIN, 1, ch );
    }
    return;
  }
  
  if( is_set( flags, 1 ) ) {
    clan_options( ch, clan, argument );
    return;
  }

  if( is_set( flags, 2 ) ) {
    clan_level( ch, clan, argument );
    return;
  }

  if( is_set( flags, 4 ) ) {
    if( !edit_clan( ch, clan, true ) )
      return;
    if( !*argument ) {
      send( ch, "What do you want to set the name of the clan to?\n\r" );
      return;
    }
    free_string( clan->name, MEM_CLAN );
    clan->name = alloc_string( argument, MEM_CLAN );
    fsend( ch, "Name of clan set to \"%s\".", argument );
    return;
  }    
  
  if( is_set( flags, 5 ) ) {
    if( !edit_clan( ch, clan, true ) )
      return;
    if( !*argument ) {
      send( ch, "What do you want to set the acronym for the clan to?\n\r" );
      return;
    }
    if( !good_clan_abbrev( ch, argument, clan ) )
      return; 
    rename_clan( clan, argument );
    fsend( ch, "Clan acronym changed to \"%s\".", clan->abbrev );
    return;
  }

  if( is_set( flags, 3 ) ) {
    if( !edit_clan( ch, clan, true ) )
      return;
    clan->modified = true;
    clan->charter = edit_string( ch, argument,
				 clan->charter, MEM_CLAN, true );
    return;
  }
  
  if( !*argument ) {
    page_underlined( ch, "Abrv  %-50s Members\n\r", "Clan" );
    for( i = 0; i < max_clan; i++ ) {
      if( knows( ch, clan = clan_list[i] ) ) {
	int j;
	if( is_set( clan->flags, CLAN_IMMORTAL )
	    || is_apprentice( ch ) ) {
	  j = clan->members.size;
	} else {
	  j = 0;
	  for( int k = 0; k < clan->members.size; ++k ) {
	    pfile_data *pfile = clan->members.list[k];
	    if( pfile->level < LEVEL_APPRENTICE ) {
	      ++j;
	    }
	  }
	}
        page( ch, "%-4s  %-50s %s\n\r", clan->abbrev, clan->name,
	      knows_members( ch, clan ) ? number_word( j, ch )
	      : "???" );
      }
    }
    return;
  }

  if( ( clan = find_clan( ch, argument ) ) ) 
    display( ch, clan ); 
}


/*
 *   CLAN EDITTING
 */


static void display_edit( char_data* ch, clan_data* clan )
{
  title_data** list  = clan->titles.list;
  const char *const flags  = "FCRN";
  char*         tmp  = static_string( );
  char*      letter;
  int          i, j;

  send( ch, "    Name: %s\n\r", clan->name );
  send( ch, "  Abbrev: %s\n\r\n\r", clan->abbrev );

  send_underlined( ch,
		   "Nmbr  Title               Holder              Flags\n\r" );  

  for( i = 0; i < clan->titles.size; i++ ) { 
    letter = tmp;
    for( j = 0; j < MAX_TITLE; j++ )
      if( is_set( list[i]->flags, j ) )
        *letter++ =  flags[j];
    *letter = '\0';
    send( ch, "%-6d%-20s%-20s%s\n\r", 
	  i+1, list[i]->name, list[i]->pfile ?
	  list[i]->pfile->name : "no one", tmp );
  }
}


void do_cedit( char_data* ch, const char *argument )
{
  if( is_mob( ch ) )
    return;

  title_data *title = get_title( ch->pcdata->pfile );

  if( !has_permission( ch, PERM_CLANS )
      && !( title && is_set( title->flags, TITLE_SET_FLAGS ) ) ) {
    send( ch, bad_command );
    return;
  }

  clan_data*    clan;
  pfile_data*  pfile;
  int              i;

  if( !( clan = ch->pcdata->pfile->clan ) ) {
    fsend( ch, "Cedit operates on the clan you are in, but you aren't in a clan." );
    return;
  }
  
  if( !*argument ) {
    display_edit( ch, clan );
    return;
  }
  
  if( matches( argument, "new" ) ) {
    if( !has_permission( ch, PERM_CLANS ) ) {
      send( ch, "Permission denied.\n\r" );
      return;
    }
    if( !*argument ) {
      send( ch, "What should the new title be called?\n\r" );
      return;
    }
    if( clan->titles.size > 5 ) {
      send( ch, "A clan is restricted to 5 titles.\n\r" );
      return;
    }
    insert( clan->titles.list, clan->titles.size,
	    new title_data( argument, 0 ), clan->titles.size );
    fsend( ch, "New title \"%s\" added.", argument );
    return;
  }
  
  if( matches( argument, "delete" ) ) {
    if( !has_permission( ch, PERM_CLANS ) ) {
      send( ch, "Permission denied.\n\r" );
      return;
    }
    if( !*argument ) {
      send( ch, "Which title do you wish to delete?\n\r" );
      return;
    }
    if( ( i = atoi( argument ) ) < 1 || i > clan->titles.size ) {
      send( ch, "No such title exists.\n\r" );
      return;
    }
    title = clan->titles.list[--i];
    fsend( ch, "Title \"%s\" removed.", title->name );
    remove( clan->titles.list, clan->titles.size, i );
    delete title;
    return;
  }

  if( number_arg( argument, i ) ) {
    if( i < 1 || i > clan->titles.size ) {
      send( ch, "No such title exists.\n\r" );
      return;
    }
    title = clan->titles.list[--i]; 
    if( set_flags( ch, argument, title->flags, "FCRN" ) )
      return;
    if( matches( argument, "title" ) ) {
      if( !has_permission( ch, PERM_CLANS ) ) {
	send( ch, "Permission denied.\n\r" );
	return;
      }
      set_string( ch, argument, title->name, "title", MEM_CLAN );
      return;
    }
    if( matches( argument, "holder" ) ) {
      if( !*argument ) {
	title->pfile = 0;
	fsend( ch, "%s set to no one.", title->name );
      } else if( ( pfile = find_pfile( argument, ch ) ) ) {
        if( pfile->clan != clan ) {
          fsend( ch, "%s isn't a member of %s.",
		 pfile->name, clan->name );
	} else {
          title->pfile = pfile;
          fsend( ch, "%s set to %s.", title->name, title->pfile->name );
	}
      }
      return;
    }
  }

  send( ch, "Unknown syntax - see help cedit.\n\r" );
}


/*
 *   ALLEGIANCE ROUTINES
 */


bool can_join( char_data* ch, clan_data* clan )
{
  const char*  word;
  int             i;
  
  if( has_permission( ch, PERM_CLANS ) )
    return true;

  /*
  if( !is_set( clan->flags, CLAN_APPROVED ) ) {
    fsend( ch, "%s has not been approved so may not accept members.",
	   clan->Name( ) );
    return false;
  }
  */
  
  if( ch->Level() < clan->min_level ) {
    fsend( ch, "To join %s you must be at least level %d.",
	   clan->Name( ), clan->min_level );
    return false;
  }
  
#define types 4
  
  const char** name1 [types] = {
    &race_table[0].name, &clss_table[0].name,
    &alignment_table[0].name, &sex_name[0] }; 
  const char** name2 [types] = {
    &race_table[1].name, &clss_table[1].name,
    &alignment_table[1].name, &sex_name[1] }; 
  int* flag_value [types] = {
    &clan->races, &clan->classes,
    &clan->alignments, &clan->sexes };
  int player_value [types] = {
    ch->shdata->race, ch->pcdata->clss, ch->shdata->alignment,
    ch->sex };

  for( i = 0; i < types; i++ ) {
    if( !is_set( flag_value[i], player_value[i] ) ) {
      word = *(name1[i]+player_value[i]*(name2[i]-name1[i]));
      fsend( ch, "As %s %s you would not be allowed to join %s.",
        isvowel( *word ) ? "an" : "a", word, clan->Name( ) );
      return false;
    }
  }
  
  #undef types

  return true;
}


void do_allegiance( char_data* ch, const char *argument )
{
  char_data*    victim;

  if( is_mob( ch ) )
    return;

  player_data *pc = player( ch );
  clan_data *clan = ch->pcdata->pfile->clan;
  
  if( !*argument && clan ) {
    fsend( ch, "You have sworn allegiance to %s.", clan->Name( ) );
    send( ch, "Switch your allegiance to whom?\n\r" );
    return; 
  }
  
  if( !strcasecmp( argument, "none" ) ) {
    if( !clan ) {
      send( ch, "You have not sworn allegiance to any clan.\n\r" );
    } else {
      char tmp [ TWO_LINES ];
      snprintf( tmp, THREE_LINES, "%s has quit the clan.",
		ch->pcdata->pfile->name );
      info( 0, empty_string, 0, tmp, IFLAG_CLANS, 1, ch, clan );
      fsend( ch, "You revoke your allegiance to %s.", clan->Name( ) );
      player_log( ch, "Revoked allegiance to clan \"%s\".", clan->Name( ) );
      remove_member( pc );   
      remove_member( pc->pcdata->pfile );
    }
    return;
  }
  
  if( !( victim = one_player( ch, argument, "swear allegiance to",
			      ch->array ) ) )
    return;
  
  if( clan ) {
    fsend( ch, "You must first revoke your allegiance to %s.", clan->Name( ) );
    return;
  }
  
  if( ch == victim ) {
    send( ch, "Allegiance to yourself is assumed.\n\r" );
    return;
  }

  clan = victim->pcdata->pfile->clan;
  
  if( !clan || ( !knows_members( ch, clan )
		 && !victim->Befriended( ch ) ) ) {
    send( ch, "They are not a member of any clan.\n\r" );
    return;
  }

  if( !can_join( ch, clan ) )
    return;
  
  if( !consenting( victim, ch ) ) {
    fsend( ch, "Swearing allegiance to %s, who has not consented to\
 you joining %s clan, is pointless.", victim, victim->His_Her( ) );
    return;
  }

  title_data *title = get_title( victim->pcdata->pfile );

  if( !title 
      || !is_set( title->flags, TITLE_RECRUIT ) ) {
    send( ch, "%s is not permitted to recruit members.\n\r", victim );
    return;
  }
  
  fsend( ch, "You kneel down and swear allegiance to %s.", clan->Name( ) );
  fsend( *victim->array,
	 "%s kneels down and swears allegiance to %s.", ch, clan->Name( ) );

  char tmp [ TWO_LINES ];
  snprintf( tmp, THREE_LINES, "%s has joined the clan.",
	    ch->pcdata->pfile->name );
  info( 0, empty_string, 0, tmp, IFLAG_CLANS, 1, victim, clan );

  set_bit( ch->pcdata->pfile->flags, PLR_CTELL );
  add_member( clan, ch->pcdata->pfile );
  save_clans( clan );
  
  player_log( ch, "Swore allegiance to %s.", victim->real_name( ) );
  player_log( ch, "Joined clan \"%s\".", clan->Name( ) );

  player_log( victim, "Accepted %s's allegiance into clan \"%s\".",
	      ch->real_name( ), clan->Name( ) );
}


/*
 *   INTERNAL ROUTINES
 */


title_data *get_title( pfile_data* pfile )
{
  clan_data*  clan  = pfile->clan;
  int            i;

  if( !clan )
    return 0;

  for( i = 0; i < clan->titles.size; i++ )
    if( clan->titles.list[i]->pfile == pfile )
      return clan->titles.list[i];

  return 0;
}


/*
 *   ADDING/REMOVING MEMBERS
 */


void add_member( clan_data* clan, pfile_data* pfile )
{
  if( pfile->clan ) {
    bug( "Add_Member: Character %s is already in a clan.", pfile->name );
    return;
  }

  clan->members += pfile;
  pfile->clan = clan;
}


void remove_member( player_data* player )
{
  if( player->noteboard == NOTE_CLAN )
    player->noteboard = NOTE_GENERAL;

  if( player->note_edit
      && player->note_edit->noteboard == NOTE_CLAN )
    player->note_edit = 0;
} 


void remove_member( pfile_data* pfile )
{
  clan_data*  clan;
  int            i;

  if( !( clan = pfile->clan ) )
    return;

  clan->members -= pfile;

  for( i = 0; i < clan->titles.size; i++ ) 
    if( clan->titles.list[i]->pfile == pfile )
      clan->titles.list[i]->pfile = 0;

  pfile->clan = 0;

  if( clan->members.is_empty() ) {
    char tmp [ TWO_LINES ];
    snprintf( tmp, TWO_LINES, "Empty clan \"%s\" deleted--final member was %s.",
	      clan->abbrev, pfile->name );
    info( LEVEL_BUILDER, empty_string, 0, tmp, IFLAG_ADMIN, 1 );
    extract( clan );
    return;
  }
  
  save_clans( clan );
}


void extract( clan_data* clan )
{
  int i;
  
  for( i = 0; i < max_clan; i++ ) 
    if( clan_list[i] == clan )
      break;
  
  if( i == max_clan ) {
    bug( "Extract( Clan ): Non-existent clan!?" );
    return;
  }
  
  remove( clan_list, max_clan, i );

  for( i = 0; i < player_list; ++i ) {
    player_data *pc = player_list[i]; 
    if( pc->Is_Valid( )
	&& pc->pcdata->pfile->clan == clan )
      remove_member( pc );
  }

  for( i = 0; i < clan->members; ++i )
    clan->members[i]->clan = 0;

  for( i = 0; i < MAX_IFLAG+1; ++i ) {
    info_data *prev = 0;
    for( info_data *info = info_history[i]; info; ) {
      if( info->clan == clan ) {
	if( info == info_history[i] ) {
	  info_history[i] = info->next;
	  delete info;
	  info = info_history[i];
	} else {
	  prev->next = info->next;
	  delete info;
	  info = prev->next;
	}
      } else {
	prev = info;
	info = info->next;
      }
    }
  }
  
  delete_file( CLAN_DIR, clan->abbrev );
  delete_file( CLAN_NOTE_DIR, clan->abbrev );

  char tmp [ TWO_LINES ];
  snprintf( tmp, TWO_LINES, "%s.html", clan->abbrev );
  delete_file( HTML_CLAN_DIR, tmp, false );

  delete clan;
} 


void expel( char_data* ch, clan_data* clan, pfile_data* pfile )
{
  if( pfile->clan != clan ) {
    send( ch, "%s already isn't in your clan.\n\r", pfile->name );
    return;
  }

  if( ch->pcdata->pfile == pfile ) {
    send( ch, "You can't expel yourself - use allegiance none.\n\r" );
    return;
  }

  for( int i = 0; i < player_list; i++ ) { 
    player_data *pc = player_list[i];
    if( pc->Is_Valid( )
	&& pc->pcdata->pfile == pfile ) {
      send_color( pc, COLOR_STRONG, ">> %s has expelled you from your clan. <<\n\r",
		  ch->descr->name );
      remove_member( pc );
      break;
    }
  }

  remove_member( pfile );
  fsend( ch, "%s expelled.", pfile->name );

  char tmp [ TWO_LINES ];
  snprintf( tmp, THREE_LINES, "%s has been expelled from the clan by %s.",
	    pfile->name, ch->descr->name );
  info( 0, empty_string, 0, tmp, IFLAG_CLANS, 1, ch, clan );

  player_log( ch, "Expelled %s from clan \"%s\".",
	      pfile->name, clan->name );

  snprintf( tmp, THREE_LINES, "Expelled from clan \"%s\" by %s.",
	    clan->Name( ), ch->real_name( ) );
  player_log( pfile, tmp );
}


/*
 *   DISK ROUTINES
 */


static bool save_html_clans( )
{
  char orig_cwd [ MAXPATHLEN+1 ];
  if( !getcwd( orig_cwd, MAXPATHLEN+1 ) )
    return false;

  mkdir( HTML_CLAN_DIR, 0755 );

  if( chdir( HTML_CLAN_DIR ) < 0 )
    return false;

  char save_cwd [ MAXPATHLEN+1 ];
  if( !getcwd( save_cwd, MAXPATHLEN+1 ) ) {
    return false;
  }
  
  FILE *index = open_file( HTML_INDEX_FILE, "w" );

  if( !index )
    return false;

  char tmp [ TWO_LINES ];

  html_start( index, "TFE Player Clans", "Known Player Clans", "../" );

  fprintf( index, "<table width=\"100%%\">\n" );
  fprintf( index, "<tr><td><h3><u>Abbrev<td><h3><u>Name<td><h3><u>Members</tr>\n" );

  for( int i = 0; i < max_clan; ++i ) {
    clan_data *clan = clan_list[i];
    snprintf( tmp, TWO_LINES, "%s.html", clan->abbrev );
    if( knows( 0, clan ) ) {
      int j;
      if( is_set( clan->flags, CLAN_IMMORTAL ) ) {
	j = clan->members.size;
      } else {
	j = 0;
	for( int k = 0; k < clan->members.size; ++k ) {
	  pfile_data *pfile = clan->members.list[k];
	  if( pfile->level < LEVEL_APPRENTICE ) {
	    ++j;
	  }
	}
      }
      fprintf( index, "<tr><td><a href=\"%s.html\">%s</a><td><a href=\"%s.html\">%s</a><td>%s</tr>",
	       clan->abbrev, clan->abbrev,
	       clan->abbrev, clan->name,
	       knows_members( 0, clan ) ? number_word( j ) : "???" );

      FILE *charter = open_file( tmp,  "w" );

      if( !charter ) {
	fclose( index );
	return false;
      }

      snprintf( tmp, TWO_LINES, "Clan: %s (%s)", clan->name, clan->abbrev );

      html_start( charter, tmp, tmp, "../" );

      if( knows_members( 0, clan ) ) {
	fprintf( charter, "<p><h3><u>Leaders</u></h3></p>\n" );
	fprintf( charter, "<div style=\"margin-left: 10px;\">" );
	for( int j = 0; j < clan->titles.size; ++j ) {
	  pfile_data *pfile = clan->titles.list[j]->pfile;
	  fprintf( charter, "<p>%s :: <b>%s</b></p>\n",
		   clan->titles.list[j]->name,
		   pfile ? pfile->name : "No one" );
	}
	fprintf( charter, "</div>\n" );

	fprintf( charter, "<p><h3><u>Members</u></h3></p>\n" );

	fprintf( charter, "<table cellpadding=10>" );

	int k = 0;
	for( int j = 0; j < clan->members.size; ++j ) {
	  pfile_data *pfile = clan->members.list[j];
	  if( is_set( clan->flags, CLAN_IMMORTAL )
	      || pfile->level < LEVEL_APPRENTICE ) {
	    if( k%6 == 0 ) {
	      fprintf( charter, "<tr>" );
	    }
	    fprintf( charter, "<td><p align=center><b>%s</b></p>\n", pfile->name );
	    ++k;
	  }
	}

	fprintf( charter, "</table>" );
      }

      fprintf( charter, "<p><h3><u>Charter</u></h3></p>\n" );
      fprintf( charter, "%s\n", html( clan->charter ).c_str( ) );

      html_stop( charter );

    } else {
      unlink( tmp );
    }
  }

  fprintf( index, "</table>\n" );

  html_stop( index );
  
  if( chdir( orig_cwd ) < 0 )
    panic( "Save_HTML_Clans: can't chdir back to run directory" );

  return true;
}


void save_clans( clan_data* clan )
{
  FILE*               fp;
  pfile_data*      pfile;
  
  if( !clan ) {
    for( int i = 0; i < max_clan; i++ )
      save_clans( clan_list[i] );
    return;
  }

  if( !( fp = open_file( CLAN_DIR, clan->abbrev, "w" ) ) ) 
    return;

  fprintf( fp, "#CLAN\n\n" );

  fwrite_string( fp, clan->name );
  fwrite_string( fp, clan->charter );
  fprintf( fp, "%d %d\n", clan->flags[0], clan->flags[1] );
  fprintf( fp, "%d %d %d %d %d\n",
	   clan->min_level, clan->races, clan->classes,
	   clan->alignments, clan->sexes );
  fprintf( fp, "%d\n", clan->date );

  title_array* titles = &clan->titles;

  fprintf( fp, "%d %d\n\n", titles->size, clan->members.size );

  for( int i = 0; i < titles->size; i++ ) {
    pfile = titles->list[i]->pfile;
    fwrite_string( fp, titles->list[i]->name );
    fprintf( fp, "%d %d %d\n", pfile ? pfile->ident : -1,
      titles->list[i]->flags[0], titles->list[i]->flags[1] );
    }

  for( int i = 0; i < clan->members; i++ )
    fprintf( fp, "%d\n", clan->members[i]->ident );

  fclose( fp );

  clan->modified = false;

  if( !save_html_clans( ) ) {
    bug( "Failed to write HTML clan files.\n" );
  }
}


void load_clans( void )
{
  clan_data*       clan;
  title_array*   titles;
  pfile_data*     pfile;
  int                 i;
  int            length;

  echo( "Loading Clans ...\n\r" );

  clan_list = 0;
  max_clan  = 0;
  DIR *dirp = opendir( CLAN_DIR );
 
  for( struct dirent *dp = readdir( dirp ); dp; dp = readdir( dirp ) ) {
    if( !strcmp( dp->d_name, "." ) || !strcmp( dp->d_name, ".." ) )
      continue;

    FILE *fp = open_file( CLAN_DIR, dp->d_name, "r" );

    if( strcmp( fread_word( fp ), "#CLAN" ) ) 
      panic( "Load_Clans: Missing header." );

    clan              = new clan_data( dp->d_name );
    clan->name        = fread_string( fp, MEM_CLAN );
    clan->charter     = fread_string( fp, MEM_CLAN );
    clan->flags[0]    = fread_number( fp );
    clan->flags[1]    = fread_number( fp );
    clan->min_level   = fread_number( fp );
    clan->races       = fread_number( fp );
    clan->classes     = fread_number( fp );
    clan->alignments  = fread_number( fp );
    clan->sexes       = fread_number( fp );
    clan->date        = fread_number( fp );
 
    titles        = &clan->titles;
    titles->size  = fread_number( fp );
    titles->list  = new title_data* [ titles->size ];
    length        = fread_number( fp );

    for( i = 0; i < titles->size; i++ ) {
      titles->list[i]           = new title_data;
      titles->list[i]->name     = fread_string( fp, MEM_CLAN );
      titles->list[i]->pfile    = get_pfile( fread_number( fp ) );
      titles->list[i]->flags[0] = fread_number( fp );
      titles->list[i]->flags[1] = fread_number( fp );
    }

    for( i = 0; i < length; i++ ) 
      if( ( pfile = get_pfile( fread_number( fp ) ) ) ) 
        add_member( clan, pfile );

    fclose( fp );
    load_notes( clan );
  }

  closedir( dirp );

  echo( "Writing HTML Clans ...\n\r" );

  if( !save_html_clans( ) ) {
    bug( "Failed to write HTML clan files." );
  }
}


/*
 *   DISPLAY ROUTINES
 */


void display( char_data* ch, clan_data* clan )
{
  char           tmp  [ 3*MAX_STRING_LENGTH ];
  int           i, j;

  page( ch, scroll_line[0] );
  if( clan->name == empty_string ) {
    page_centered( ch, clan->Name( ) );
  } else {
    page_centered( ch, "%s (%s)", clan->Name( ), clan->abbrev );
  }
  page( ch, scroll_line[0] );

  page( ch, "\n\r" );

  if( knows_members( ch, clan ) ) {
    page_title( ch, "Leaders" );
    for( i = 0; i < clan->titles.size; ++i ) {
      pfile_data *pfile = clan->titles.list[i]->pfile;
      page( ch, "%38s :: %s\n\r", clan->titles.list[i]->name,
	    pfile ? pfile->name : "No one" );
    }
    
    page( ch, "\n\r" );
    page_title( ch, "Members" );
    for( i = j = 0; i < clan->members.size; ++i ) {
      pfile_data *pfile = clan->members.list[i];
      if( is_set( clan->flags, CLAN_IMMORTAL )
	  || is_apprentice( ch )
	  || pfile->level < LEVEL_APPRENTICE ) {
	page( ch, "%18s%s", pfile->name,
	      (j++)%4 != 3 ? "" : "\n\r" );
      }
    }
    page( ch, "\n\r%s", j%4 != 0 ? "\n\r" : "" );
  }
  
  page_title( ch, "Charter" );
  
  convert_to_ansi( ch, 3*MAX_STRING_LENGTH, clan->charter, tmp );
  page( ch, tmp );
}
