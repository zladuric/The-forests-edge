#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <syslog.h>
#include <unistd.h>
#include "define.h"
#include "struct.h"
#include "html.h"


#define HELP_IMMORTAL         0
#define HELP_SPELLS           1
#define HELP_FUNCTIONS        2
#define HELP_LOGIN            3


help_data**      help_list;
int               max_help;
static bool help_modified = true;


/*
 *   HELP_DATA CLASS
 */


Help_Data :: Help_Data( )
  : name(empty_string), text(empty_string), immortal(empty_string),
    category(0), when(-1), by(empty_string)
{
  record_new( sizeof( help_data ), MEM_HELP );
  
  vzero( level, 2 );
}

  
Help_Data :: ~Help_Data( )
{
  record_delete( sizeof( help_data ), MEM_HELP );
  free_string( by, MEM_HELP );
  free_string( name, MEM_HELP );
  free_string( text, MEM_HELP );
  free_string( immortal, MEM_HELP );
  
  for( int pos = 0; pos < max_help; ++pos ) {
    if( help_list[pos] == this ) {
      remove( help_list, max_help, pos );
      break;
    }
  }
}


static void smash_filename( char *tmp )
{
  while( char c = *tmp ) {
    if( c == ' ' ) {
      *tmp = '_';
    } else if( c == '<' ) {
      *tmp = '_';
    } else if( c == '>' ) {
      *tmp = '_';
    } else if( c == '\"' ) {
      *tmp = '_';
    } else if( c == '&' ) {
      *tmp = '+';
    } else if( c == '/' ) {
      *tmp = '_';
    }
    ++tmp;
  }
}


static bool save_html_help( )
{
  char orig_cwd [ MAXPATHLEN+1 ];
  if( !getcwd( orig_cwd, MAXPATHLEN+1 ) )
    return false;

  mkdir( HTML_HELP_DIR, 0755 );

  if( chdir( HTML_HELP_DIR ) < 0 )
    return false;

  char save_cwd [ MAXPATHLEN+1 ];
  if( !getcwd( save_cwd, MAXPATHLEN+1 ) ) {
    return false;
  }
  
  FILE *index = open_file( HTML_INDEX_FILE, "w" );

  if( !index )
    return false;

  html_start( index, "TFE Help Categories", "Help Categories", "../" );

  fprintf( index, "<ul>\n" );

  int categories = table_max[ TABLE_HELP_CAT ];
  unsigned count[ categories ];
  vzero( count, categories );

  int cat_sort [ categories ];
  categories = sort_names( &help_cat_table[0].name,
			   &help_cat_table[1].name,
			   cat_sort,
			   categories,
			   true );
  
  for( int pos = 0; pos < max_help; ++pos ) {
    const help_data *help = help_list[pos];
    if( help->level[0] == 0
	&& help->level[1] == 0 ) {
      ++count[ help->category ];
    }
  }
  
  char *tmp = static_string( );
  
  for( int i = 0; i < categories; ++i ) {
    int k = cat_sort[i];
    if( count[k] > 0 ) {
      snprintf( tmp, THREE_LINES, "%s", help_cat_table[k].name );
      smash_filename( tmp );
      mkdir( tmp, 0755 );
      if( chdir( tmp ) < 0 ) {
	fclose( index );
	return false;
      }
      
      fprintf( index, "<li><a href=\"%s/%s\">%s</a></li>\n",
	       tmp, HTML_INDEX_FILE, help_cat_table[k].name );
      
      FILE *cat = open_file( HTML_INDEX_FILE, "w" );
      
      if( !cat ) {
	fclose( index );
	return false;
      }
      
      snprintf( tmp, THREE_LINES, "Help Category: %s", help_cat_table[k].name );
      *tmp = toupper( *tmp );
      html_start( cat, tmp, tmp, "../../" );
      
      fprintf( cat, "<table width=\"100%%\">\n" );
      
      int j = 0;
      const int cols = 5;
      
      for( int pos = 0; pos < max_help; ++pos ) {
	const help_data *help = help_list[pos];
	if( help->category == k
	    && help->level[0] == 0
	    && help->level[1] == 0 ) {
	  if( j % cols == 0 ) {
	    fprintf( cat, "  <tr>" );
	  }
	  snprintf( tmp, THREE_LINES, "%s.html", help->name );
	  smash_filename( tmp );
	  fprintf( cat, "<td><a href=\"%s\">%s</td>",
		   tmp, help->name );
	  if( j++ % cols == cols-1 ) {
	    fprintf( cat, "</tr>\n" );
	  }

	  FILE *data = open_file( tmp, "w" );

	  if( !data ) {
	    fclose( cat );
	    fclose( index );
	    return false;
	  }

	  snprintf( tmp, THREE_LINES, "Help Topic: %s", help->name );
	  html_start( data, tmp, tmp, "../../" );

	  fprintf( data, "%s\n", html( help->text ).c_str( ) );

	  html_stop( data );
 	}
      }
      
      fprintf( cat, "</table>\n" );

      html_stop( cat );
      
      if( chdir( save_cwd ) < 0 ) {
	fclose( index );
	return false;
      }
    }
  }

  fprintf( index, "</ul>\n" );
  
  html_stop( index );
  
  if( chdir( orig_cwd ) < 0 )
    panic( "Save_HTML_Help: can't chdir back to run directory" );

  return true;
}


/*
 *   READ/WRITE ROUTINES
 */


void load_helps( void )
{
  echo( "Loading Help ...\n\r" );

  help_list = 0;
  max_help  = 0;

  FILE *fp = open_file( AREA_DIR, HELP_FILE, "r" );

  if( strcmp( fread_word( fp ), "#HELPS" ) ) 
    panic( "Load_helps: missing header" );

  while( true ) {
    help_data *help = new help_data;
    help->level[0] = fread_number( fp );
    help->level[1] = fread_number( fp );
    help->category = fread_number( fp );
    help->when = fread_number( fp );
    help->by = fread_string( fp, MEM_HELP );
    help->name = fread_string( fp, MEM_HELP );

    if( *help->name == '$' ) {
      delete help;
      break;
    }

    help->text = fread_string( fp, MEM_HELP );
    help->immortal = fread_string( fp, MEM_HELP );  

    int pos = pntr_search( help_list, max_help, help->name );

    if( pos < 0 )
      pos = -pos-1;
    
    insert( help_list, max_help, help, pos );
  }

  fclose( fp );

  echo( "Writing HTML Help ...\n\r" );

  if( !save_html_help( ) ) {
    bug( "Failed to write HTML help files." );
  }
}


bool save_help( )
{
  if( !help_modified ) 
    return false;

  rename_file( AREA_DIR, HELP_FILE,
	       AREA_PREV_DIR, HELP_FILE );
  
  FILE *fp;

  if( !( fp = open_file( AREA_DIR, HELP_FILE, "w" ) ) )
    return false;

  fprintf( fp, "#HELPS\n" );

  for( int pos = 0; pos < max_help; ++pos ) {
    const help_data *help = help_list[pos];
    fprintf( fp, "%d %d %d %ld\n",
	     help->level[0], help->level[1],
	     help->category,
	     help->when );
    fwrite_string( fp, help->by );
    fwrite_string( fp, help->name );
    fwrite_string( fp, help->text );
    fwrite_string( fp, help->immortal );
  }

  fprintf( fp, "-1 -1 -1 -1\n" );
  fwrite_string( fp, empty_string );
  fwrite_string( fp, "$" );

  fclose( fp );

  help_modified = false;

  if( !save_html_help( ) ) {
    bug( "Failed to write HTML help files.\n" );
  }

  return true;
}


/*
 *   FIND_HELP ROUTINE
 */


static bool can_read( char_data* ch, help_data* help )
{
  return has_permission( ch, help->level );
}



help_data *find_help( char_data* ch, const char* argument )
{
  help_data*        help;
  int              first  = -2;
  int pos;

  if( ch && number_arg( argument, pos ) ) {
    if( pos < 0 || pos >= max_help ) {
      send( ch, "There is no help file with that index.\n\r" ); 
      return 0;
    }
    if( !can_read( ch, help_list[pos] ) ) {
      send( ch, "You do not have the required permission.\n\r" );
      return 0;
    }
    return help_list[pos];
  }
  
  pos = pntr_search( help_list, max_help, argument );
  
  if( pos >= 0 ) {
    if( !ch || can_read( ch, help_list[pos] ) )
      return help_list[pos];
    ++pos;
  } else {
    if( !ch )
      return 0;
    pos = -pos-1;
  }
  
  for( ; pos < max_help; ++pos ) {
    help = help_list[pos];
    if( !fmatches( argument, help->name ) )
      break;
    if( can_read( ch, help ) ) {
      if( first != -2 ) {
        if( first != -1 ) {
          page( ch, "More than one match was found - please be more\
 specific in what topic you\n\rwant help on.\n\r\n\r" );
          page( ch, "  [%4d] %-20s: %s\n\r", first,
		help_cat_table[help_list[first]->category].name,
		help_list[first]->name );
          first = -1;
	}
        page( ch, "  [%4d] %-20s: %s\n\r", pos,
	      help_cat_table[help->category].name, help->name );
      } else {
        first = pos;
      }
    }
  }
  
  if( first >= 0 )
    return help_list[first];

  if( first == -2 )
    send( ch, "No matching help file was found - use index to see a list of\
 topics for\n\rwhich help exists.\n\r" );
      
  return 0;
}


static void skill_help( char_data* ch, int i )
{
  char                tmp  [ 3*MAX_STRING_LENGTH ];
  char                buf  [ TWO_LINES ];
  bool              found  = false;
  int                   j;
  int               level;
  help_data*         help;

  page( ch, "%9sSkill: %s\n\r",
	"",
	skill_physical_table[i].name );
  
  if( is_apprentice( ch ) )
    page( ch, "%10sSlot: %d\n\r", "", i );

  strcpy( tmp, "   Class/Level:" );
  char *t = tmp + 15;
  unsigned c = 15;
  const unsigned width = ( ch && ch->pcdata ) ? ch->pcdata->columns : 80;

  for( j = 0; j < MAX_CLSS; j++ ) {
    if( ( level = skill_physical_table[i].level[j] ) > 0
	&& clss_table[j].open ) {
      const char *name = clss_table[j].name;
      unsigned x = sprintf( buf, "%c%s", toupper( *name ), name+1 );
      if( is_apprentice( ch ) || level <= LEVEL_HERO ) {
	x += sprintf( buf+x, " %d%s",
		      level, number_suffix( level ) );
      }
      c += x;
      if( found && c >= width-1-found ) {  // leave room for comma and space.
	t += sprintf( t, ",\n\r%16s%s", "", buf );
	c = 16+x;
      } else {
	t += sprintf( t, "%s %s", found ? "," : "", buf );
	c += found+1;
	found = true;
      }
    }
  }

  page( ch, "%s%s\n\r", tmp, found ? "" : " none" );
  
  if( !skill_physical_table[i].religions.is_empty( ) ) {
    strcpy( tmp, "     Religions:" );
    t = tmp+15;
    c = 15;
    found = false;
    for( j = 0; j < skill_physical_table[i].religions.size; ++j ) {
      unsigned x = sprintf( buf, "%s",
			    religion_table[skill_physical_table[i].religions.list[j]].name );
      c += x;
      if( found && c >= width-1-found ) {  // leave room for comma and space.
	t += sprintf( t, ",\n\r%16s%s", "", buf );
	c = 16+x;
      } else {
	t += sprintf( t, "%s %s", found ? "," : "", buf );
	c += found+1;
	found = true;
      }
    }
    page( ch, "%s%s\n\r", tmp, found ? "" : " none" );
  }

  page( ch, "--------------------------------\n\r" );
  page( ch, "Description:\n\r\n\r" );

  int pos = pntr_search( help_list, max_help, skill_physical_table[i].name );

  if( pos >= 0 ) {
    help = help_list[pos];
    convert_to_ansi( ch, 3*MAX_STRING_LENGTH, help->text, tmp );
    page( ch, tmp );

    if( help->immortal != empty_string && is_apprentice( ch ) ) {
      page( ch, "\n\r" );
      convert_to_ansi( ch, 3*MAX_STRING_LENGTH, help->immortal, tmp );
      page( ch, tmp );
    }
  } else
    page( ch, "none\n\r" );
}


static void spell_help( char_data* ch, int i )
{
  char                tmp  [ 3*MAX_STRING_LENGTH ];
  char                buf  [ TWO_LINES ];
  bool              found  = false;
  obj_clss_data*  reagent;
  int                j, k;
  int               level;
  help_data*         help;

  page( ch, "%9sSpell: %s\n\r   Energy Cost: %s\n\r",
	"",
	skill_spell_table[i].name,
	skill_spell_table[i].cast_mana );
  
  if( skill_spell_table[i].prepare ) {
    page( ch, " Turns to Prep: %d\n\r",
	  skill_spell_table[i].prepare );
  }

  page( ch, " Turns to Cast: %d\n\r",
	skill_spell_table[i].wait - skill_spell_table[i].prepare );
  
  if( skill_spell_table[i].duration != empty_string
      && strcmp( skill_spell_table[i].duration, "0" ) ) 
    page( ch, "      Duration: %s\n\r", skill_spell_table[i].duration );
  if( skill_spell_table[i].damage != empty_string 
      && strcmp( skill_spell_table[i].damage, "0" ) ) 
    page( ch, "        Damage: %s\n\r", skill_spell_table[i].damage );
  if( skill_spell_table[i].leech_mana != empty_string
      && strcmp( skill_spell_table[i].leech_mana, "0" ) ) 
    page( ch, "  Leech of Max: %s\n\r", skill_spell_table[i].leech_mana );
  if( skill_spell_table[i].regen != empty_string 
      && strcmp( skill_spell_table[i].regen, "0" ) ) 
    page( ch, "   Regen Leech: %s\n\r", skill_spell_table[i].regen );

  if( is_apprentice( ch ) )
    page( ch, "%10sSlot: %d\n\r", "", i );

  strcpy( tmp, "   Class/Level:" );
  char *t = tmp + 15;
  unsigned c = 15;
  const unsigned width = ( ch && ch->pcdata ) ? ch->pcdata->columns : 80;

  for( j = 0; j < MAX_CLSS; j++ ) {
    if( ( level = skill_spell_table[i].level[j] ) > 0
	&& clss_table[j].open ) {
      const char *name = clss_table[j].name;
      unsigned x = sprintf( buf, "%c%s", toupper( *name ), name+1 );
      if( is_apprentice( ch ) || level <= LEVEL_HERO ) {
	x += sprintf( buf+x, " %d%s",
		      level, number_suffix( level ) );
      }
      c += x;
      if( found && c >= width-1-found ) {  // leave room for comma and space.
	t += sprintf( t, ",\n\r%16s%s", "", buf );
	c = 16+x;
      } else {
	t += sprintf( t, "%s %s", found ? "," : "", buf );
	c += found+1;
	found = true;
      }
    }
  }

  page( ch, "%s%s\n\r", tmp, found ? "" : " none" );

  if( !skill_spell_table[i].religions.is_empty( ) ) {
    strcpy( tmp, "     Religions:" );
    t = tmp+15;
    c = 15;
    found = false;
    for( j = 0; j < skill_spell_table[i].religions.size; ++j ) {
      unsigned x = sprintf( buf, "%s",
			    religion_table[skill_spell_table[i].religions.list[j]].name );
      c += x;
      if( found && c >= width-1-found ) {  // leave room for comma and space.
	t += sprintf( t, ",\n\r%16s%s", "", buf );
	c = 16+x;
      } else {
	t += sprintf( t, "%s %s", found ? "," : "", buf );
	c += found+1;
	found = true;
      }
    }
    page( ch, "%s%s\n\r", tmp, found ? "" : " none" );
  }

  found = false;

  page( ch, "--------------------------------\n\r" );
  page( ch, "Description:\n\r\n\r" );

  int pos = pntr_search( help_list, max_help, skill_spell_table[i].name );

  if( pos >= 0 ) {
    help = help_list[pos];
    convert_to_ansi( ch, 3*MAX_STRING_LENGTH, help->text, tmp );
    page( ch, tmp );

    if( help->immortal != empty_string && is_apprentice( ch ) ) {
      page( ch, "\n\r" );
      convert_to_ansi( ch, 3*MAX_STRING_LENGTH, help->immortal, tmp );
      page( ch, tmp );
    }
  } else
    page( ch, "none\n\r" );

  page( ch, "\n\rReagents:\n\r" );

  for( j = 0; j < MAX_SPELL_WAIT; ++j ) {
    for( k = 0; k < j; k++ )
      if( abs( skill_spell_table[i].reagent[j] ) == abs( skill_spell_table[i].reagent[k] ) )
        break;
    if( k != j )
      continue;
    if( !( reagent = get_obj_index( abs( skill_spell_table[i].reagent[j] ) ) ) )
      continue;
    found = true;
    page( ch, "   %s\n\r", reagent->Name( ) );
  }

  if( !found ) 
    page( ch, "  none\n\r" );
}


/*
 *   MAIN HELP ROUTINE
 */


void do_help( char_data* ch, const char *argument )
{
  char          tmp  [ 3*MAX_STRING_LENGTH ];
  help_data*   help;
  int        length  = strlen( argument );

  if( pet_help( ch ) || !ch->link )
    return;
  
  if( exact_match( argument, "search" ) ) {
    if( !*argument ) {
      send( ch, "What text do you wish to search for?\n\r" );
      return;
    }
    unsigned count = 0;
    for( int i = 0; i < max_help; ++i ) {
      help_data *help = help_list[ i ];
      if( !can_read( ch, help ) )
	continue;
      if( search_text( help->name, argument )
	  || search_text( help->text, argument )
	  || is_apprentice( ch ) && search_text( help->immortal, argument ) ) {
	if( count == 0 ) {
	  page_title( ch, "Help Topic Search Results" );
	}
        page( ch, "  [%4d] %-20s: %s\n\r", i,
	      help_cat_table[help->category].name, help->name );
	++count;
      }
    }
    if( count == 0 ) {
      send( ch, "No matching help topic found.\n\r" );
    } else {
      page( ch, "\n\r  %u matching topic%s found.\n\r",
	    count, count == 1 ? "" : "s" );
    }
    return;
  }

  if( !( help = find_help( ch,
			   !*argument ? "summary" : argument ) ) ) 
    return;
  
  if( ch->link->connected != CON_PLAYING ) {
    convert_to_ansi( ch, 3*MAX_STRING_LENGTH, help->text, tmp );
    send( ch, tmp );
    return;
  }

  if( help->category == HELP_SPELLS ) {
    for( int i = 0; i < table_max[TABLE_SKILL_SPELL]; ++i ) {
      if( !strncasecmp( skill_spell_table[i].name, argument, length ) ) {
        spell_help( ch, i );
        return;
      }
    }
  } else {
    int n = find_skill( argument, SKILL_CAT_PHYSICAL );
    if( n >= 0
	&& !strcasecmp( skill_physical_table[skill_number(n)].name, help->name ) ) {
      skill_help( ch, n );
      return;
    }
  }
  
  page( ch, "   Topic: %s\n\r", help->name );
  page( ch, "Category: %s\n\r", help_cat_table[help->category].name );  
  
  if( is_apprentice( ch ) ) {
    strcpy( tmp, "   Level: " );
    permission_flags.sprint( tmp+10, help->level );
    strcat( tmp, "\n\r" );
    page( ch, tmp );
  }

  if( help->when != -1 ) {
    if( help->by != empty_string ) {
      page( ch, "Modified: %s by %s\n\r", ltime( help->when, false, ch ), help->by );
    } else {
      page( ch, "Modified: %s\n\r", ltime( help->when, false, ch ) );
    }
  }

  page( ch, "\n\r" );
  convert_to_ansi( ch, 3*MAX_STRING_LENGTH, help->text, tmp );
  page( ch, tmp );

  if( help->immortal != empty_string && is_apprentice( ch ) ) {
    page( ch, "\n\r" );
    convert_to_ansi( ch, 3*MAX_STRING_LENGTH, help->immortal, tmp );
    page( ch, tmp );
  }
}


void help_link( link_data* link, const char* argument )
{
  int pos = pntr_search( help_list, max_help, argument );

  if( pos < 0 ) {
    send( link, "Help subject \"%s\" not found.\n\r", argument );

  } else if( link->player && link->player->pcdata ) {
    char tmp [ 3*MAX_STRING_LENGTH ];
    convert_to_ansi( link->player, 3*MAX_STRING_LENGTH, help_list[pos]->text, tmp );
    if( link->connected == CON_PLAYING )
      page( link->player, tmp );
    else
      send( link, tmp );

  } else {
    send( link, help_list[pos]->text );
  }
}


void do_motd( char_data* ch, const char *)
{
  if( !ch->link )
    return;

  help_link( ch->link, "motd" );
}


/*
 *   INDEX COMMAND
 */


void do_index( char_data* ch, const char *argument )
{
  char           tmp  [ MAX_STRING_LENGTH ];
  int         length  = strlen( argument ); 
  help_data*    help;
  int           i, j;
  int            pos;
  int          trust  = get_trust( ch );

  int max = table_max[ TABLE_HELP_CAT ];
  int sorted[ max ];
  max = sort_names( &help_cat_table[0].name, &help_cat_table[1].name,
		    sorted, max );

  if( !*argument ) {
    page_title( ch, "Help Categories" );
    for( i = j = 0; i < max; ++i ) {
      int k = sorted[i];
      if( help_cat_table[k].level <= trust ) {
        snprintf( tmp, MAX_STRING_LENGTH, "%19s%s",
		  help_cat_table[k].name,
		  (j++)%4 != 3 ? "" : "\n\r" );
        page( ch, tmp );
      }
    }
    page( ch, "\n\r%s", j%4 != 0 ? "\n\r" : "" );
    page_centered( ch, "[ Type index <category> to see a list of help\
 files in that category. ]" );
    return;
  }

  for( i = 0; i < max; ++i ) {
    int k = sorted[i];
    if( help_cat_table[k].level <= trust
	&& !strncasecmp( argument, help_cat_table[k].name, length ) ) 
      break;
  }

  if( i == max ) {
    send( ch, "Unknown help category.\n\r" );
    return;
  }
  
  int k = sorted[i];

  page_title( ch, "Help Files - %s", help_cat_table[k].name );      
  
  for( j = 0, pos = 0; pos < max_help; pos++ ) {
    help = help_list[pos];
    if( help->category != k || !can_read( ch, help ) )
      continue;
    page( ch, "%26s%s", help->name, ++j%3 ? "" : "\n\r" );
  }
  if( j%3 != 0 )
    page( ch, "\n\r" );
  page( ch, "\n\r" );
  page_centered( ch, "[ Type help <file> to read a help file. ]" );
}


/*
 *   ONLINE EDITING OF HELP
 */


void do_hedit( char_data *ch, const char *argument )
{
  wizard_data *imm = wizard( ch );
  help_data *help  = imm->help_edit;

  if( !*argument ) {
    if( !help ) {
      send( ch, "What help file do you wish to edit?\n\r" );
    } else {
      fsend( ch, "You stop editing the \"%s\" help file.",
	     help->name );
      imm->help_edit = 0;
    }
    return;
  }

  if( exact_match( argument, "delete" ) ) {
    if( !help ) {
      send( ch, "You aren't editing any help file.\n\r" );
      return;
    }
    extract( imm, offset( &imm->help_edit, imm ), "help" );
    help_modified = true;
    fsend( ch, "Help file \"%s\" deleted.", help->name );
    delete help;
    return; 
  }

  if( exact_match( argument, "new" ) ) {
    if( !*argument ) {
      send( ch, "On what subject do you want to create a help?\n\r" );
      return;
    }

    /*
    if( !isalpha( *argument ) ) {
      send( ch, "Help subjects must begin with a letter.\n\r" );
      return;
    }
    */

    int pos = pntr_search( help_list, max_help, argument );

    if( pos >= 0 ) {
      fsend( ch, "There is already a help named \"%s\".", argument );
      return;
    }

    help = new help_data;
    help->name = alloc_string( argument, MEM_HELP );
    //    help->when = current_time;

    set_bit( help->level, PERM_HELP_FILES );

    pos = pntr_search( help_list, max_help, "blank" );
    if( pos >= 0 ) 
      help->text = alloc_string( help_list[pos]->text, MEM_HELP );
    else
      help->text = (char*) empty_string;

    pos = pntr_search( help_list, max_help, argument );

    if( pos < 0 )
      pos = -pos-1;

    insert( help_list, max_help, help, pos );
    imm->help_edit = help;
    fsend( ch, "Help subject \"%s\" created.", help->name );
    help_modified = true;
    return;
  }

  if( !( help = find_help( ch, argument ) ) ) 
    return;

  imm->help_edit = help;
  fsend( ch, "Hdesc, hset, and hbug now operate on \"%s\".", help->name );
}


void do_hdesc( char_data *ch, const char *argument ) 
{
  wizard_data *imm = wizard( ch );
  help_data *help = imm->help_edit;

  if( !help ) {
    send( ch, "You are not editing any subject.\n\r" );
    return;
  }
  
  const char *result = edit_string( ch, argument, help->text, MEM_HELP, true );

  if( strcmp( result, help->text ) ) {
    help->when = current_time;
    help->by = alloc_string( ch->descr->name, MEM_HELP );
    help_modified  = true;
  }

  help->text = result;
}


void do_hbug( char_data* ch, const char *argument ) 
{
  wizard_data *imm = wizard( ch );
  help_data *help = imm->help_edit;

  if( !help ) {
    send( ch, "You are not editing any subject.\n\r" );
    return;
  }

  const char *result = edit_string( ch, argument, help->immortal, MEM_HELP, true );

  if( strcmp( result, help->immortal ) ) {
    //    help->when = current_time;
    //    help->by = ch->descr->name;
    help_modified  = true;
  }

  help->immortal = result;
}


void do_hstat( char_data *ch, const char *argument )
{
  help_data *help;
  wizard_data *imm = wizard( ch );

  if( *argument ) {
    if( !( help = find_help( ch, argument ) ) ) 
      return;
  } else {
    if( !( help = imm->help_edit ) ) {
      send( ch, "You aren't editing any help file.\n\r" );
      return;
    }
  }

  /*
  help_data *help = imm->help_edit;

  if( !help ) {
    if( !( help = find_help( ch, argument ) ) ) 
      return;
  }
  */

  page_underlined( ch, "Help %s\n\r", help->name );
  page( ch, "           name : %s\n\r", help->name );
  char tmp [ FOUR_LINES ];
  permission_flags.sprint( tmp, help->level );
  page( ch, "          level : %s\n\r", tmp );
  page( ch, "       category : %s\n\r", help_cat_table[help->category].name );
}


void do_hset( char_data *ch, const char *argument )
{
  wizard_data *imm = wizard( ch );
  help_data *help = imm->help_edit;

  if( !help ) {
    send( ch, "You are not editing any subject.\n\r" );
    return;
  }
  
  if( !*argument ) {
    do_hstat( ch, "" );
    /*
    page_underlined( ch, "Help %s\n\r", help->name );
    page( ch, "           name : %s\n\r", help->name );
    char tmp [ FOUR_LINES ];
    permission_flags.sprint( tmp, help->level );
    page( ch, "          level : %s\n\r", tmp );
    page( ch, "       category : %s\n\r", help_cat_table[help->category].name );
    */
    return;
  }
  
  if( matches( argument, "level" ) ) {
    help_modified = true;
    permission_flags.set( ch, argument, help->name, help->level );
    return;
  }
  
#define hcn( i )   help_cat_table[i].name
  
  class type_field type_list[] = {
    { "category",  table_max[ TABLE_HELP_CAT ],  &hcn(0),  &hcn(1),  &help->category, true  },
    { "" }
  };
  
#undef hcn

  if( process( type_list, ch, help->name, argument ) ) {
    help_modified = true;
    return;
  }

  if( matches( argument, "name" ) ) {
    if( !*argument ) {
      send( ch, "What do you want to set the help name to?\n\r" );
      return;
    }
    help_modified = true;
    for( int pos = 0; pos < max_help; ++pos )
      if( help_list[pos] == help ) {
	remove( help_list, max_help, pos );
	send( ch, "Help name changed from %s to %s.\n\r",
	      help->name, argument );
	free_string( help->name, MEM_HELP );
	help->name = alloc_string( argument, MEM_HELP );
	if( ( pos = pntr_search( help_list, max_help, help->name ) ) < 0 )
	  pos = -pos-1;
	insert( help_list, max_help, help, pos );
	return;
      }
  }
  
  send( ch, "See help hset.\n\r" );
}


static int select_hfind( help_data *help, char_data *ch, const char *argument )
{
  while( true ) {
    char hyphen = *argument;

    if( !hyphen )
      return 1;

    char letter;

    if( hyphen != '-' ) {
      letter = 'n';
    } else {
      ++argument;
      if( !isalpha( letter = *argument++ ) ) {
        send( ch, "Illegal character for flag - See help hfind.\n\r" );
        return -1;
      }
    }

    bool negative = false;
    skip_spaces( argument );
    
    if( *argument == '!' ) {
      negative = true;
      ++argument;
    }
    
    if( !*argument || *argument == '-' || isspace( *argument ) ) {
      send( ch, "All flags require an argument - See help hfind.\n\r" );
      return -1;
    }
  
    int length = 0;
    char tmp [ MAX_INPUT_LENGTH ];

    while( *argument && strncmp( argument-1, " -", 2 ) ) {
      if( length > ONE_LINE-2 ) {
        send( ch, "Flag arguments must be less than one line.\n\r" );
        return -1;
      }
      tmp[length++] = *argument++;
    }

    for( ; isspace( tmp[length-1] ); --length );

    tmp[length] = '\0';

    if( letter == 'n' ) {
      if( !is_name( tmp, help->name ) )
        return 0;
      continue;
    }

    send( ch, "Unknown flag - See help hfind.\n\r" );
    return -1;
  }
}


static void display_hfind( help_data *help, int i, char_data *ch )
{
  page( ch, "[%4d] %s\n\r", i, help->name );
}


void do_hfind( char_data* ch, const char *argument )
{
  unsigned count = 0;

  for( int i = 0; i < max_help; ++i ) {
    help_data *help = help_list[i];
    if( can_read( ch, help ) ) {
      switch( select_hfind( help, ch, argument ) ) {
      case -1 : return;
      case  1 :
	if( count == 0 ) {
	  page( ch, "\n\r" );
	  page_underlined( ch, "Num    Subject\n\r" );
	}
	++count;
	display_hfind( help, i, ch );
      }
    }
  }

  if( count == 0 ) 
    send( ch, "No help subject matching search was found.\n\r" );
  else {
    page( ch, "\n\rFound %d match%s.\n\r",
	  count,
	  count == 1 ? "" : "es" );
  }
}


/*
 *  FUNCTION HELP
 */


void do_functions( char_data* ch, const char *argument )
{
  char buf [ 3*MAX_STRING_LENGTH ];
  bool   found  = false;
  const int length  = strlen( argument );
  const int pos = pntr_search( help_list, max_help, argument );
  
  if( pos >= 0 && help_list[pos]->category == HELP_FUNCTIONS ) {
    convert_to_ansi( ch, 3*MAX_STRING_LENGTH, help_list[pos]->text, buf );
    page( ch, buf );
    return;
  }
  
  for( int i = 0; *cfunc_list[i].name; ++i ) {
    if( strncasecmp( argument, cfunc_list[i].name, length ) )
      continue;
    if( !found ) {
      found = true;
      page_underlined( ch, "%-13s %-17s %s\n\r",
		       "Returns", "Function Name", "Arguments" );
    }
    /*
    ssize_t l = snprintf( buf, 3*MAX_STRING_LENGTH, "%-15s %-15s (",
			 arg_type_name[ cfunc_list[i].type ],
			 cfunc_list[i].name );
    */
    ssize_t l = 0;
    for( int j = 0; j < 6; ++j )
      if( cfunc_list[i].arg[j] != NONE )
        l += sprintf( buf+l, "%s %s", j == 0 ? "" : ",",
		      arg_type_name[ cfunc_list[i].arg[j] ] );
    //    sprintf( buf+l, " )\n\r" ); 
    page( ch, "%-13s %-17s (%s )\n\r",
	  cfunc_list[i].type == NONE ? "" : arg_type_name[ cfunc_list[i].type ],
	  cfunc_list[i].name,
	  buf );
    //    page( ch, "%-61s %s\n\r", buf,
    //	  arg_type_name[ cfunc_list[i].type ] );
  }

  if( !found ) 
    send( ch, "No matching function found.\n\r" );
}


/*
 *   SKILL HELP ROUTINES
 */


void do_spell( char_data* ch, const char *argument )
{
  if( pet_help( ch ) || !ch->link )
    return;
  
  if( !*argument ) {
    send( ch, "What spell do you want info on?\n\r[To get a list of spells\
 type abil sp <class>.]\n\r" );
    return;
  }
  
  for( int i = 0; i < table_max[TABLE_SKILL_SPELL]; ++i ) {
    if( !strncasecmp( skill_spell_table[i].name, argument,
		      strlen( argument ) ) ) {
      spell_help( ch, i );
      return;
    }
  }
  
  send( ch, "Unknown spell.\n\r" );
}


void do_skill( char_data* ch, const char *argument )
{
  if( pet_help( ch ) || !ch->link )
    return;
  
  if( !*argument ) {
    send( ch, "What skill do you want info on?\n\r[To get a list of skills\
 type abil phy <class>.]\n\r" );
    return;
  }
  
  int n = find_skill( argument, SKILL_CAT_PHYSICAL );

  if( n < 0 ) {
    send( ch, "Unknown skill.\n\r" );
  } else {
    skill_help( ch, n );
  }
}
