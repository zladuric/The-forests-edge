#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "define.h"
#include "struct.h"


const char*   list_word  ( int, int );


/*
 *   LIST EDITING
 */


const char *list_entry [ MAX_LIST ][ 30 ] = {
  { "Lists", "Permissions",
    "Food-Omnivore", "Food-Carnivore", "Food-Herbivore",
    "Feast-Omnivore", "Feast-Carnivore", "Feast-Herbivore",
    "LS-Species", "LS-Reagent",
    "FF-Species", "FF-Reagent",
    "RA-Species", "RA-Reagent",
    "FM-Species", "FM-Reagent", 
    "CE-Species", "CE-Reagent",
    "CG-Species", "CG-Reagent",
    "" },
  { "All Rooms", "Help", "Mobs", "Objects", "Write All",
    "Edit Players", "Site Names", "Private Email", "Tables", "Lists",
    "All Mobs", "Accounts", "Passwords", "All Objects", "Rooms",
    "Exp", "Ban Sites", "Switch", "" },
  { "30", "Item", "O" },
  { "30", "Item", "O" },
  { "30", "Item", "O" },
  { "30", "Item", "O" },
  { "30", "Item", "O" },
  { "30", "Item", "O" },
  { "30", "Mob",  "M" },
  { "30", "Item", "O" },
  { "30", "Mob",  "M" },
  { "30", "Item", "O" },
  { "30", "Mob",  "M" },
  { "30", "Item", "O" },
  { "30", "Mob",  "M" },
  { "30", "Item", "O" },
  { "30", "Mob",  "M" },
  { "30", "Item", "O" },
  { "30", "Mob",  "M" },
  { "30", "Item", "O" }
};

int list_value [ MAX_LIST ][ 30 ];


const char* list_word( int list, int i )
{
  const char*        blank  = "-blank-";
  obj_clss_data*  obj_clss;
  species_data*    species;
 
  if( *list_entry[list][2] == 'O' ) {
    obj_clss = get_obj_index( list_value[list][i] );
    return( obj_clss ? obj_clss->Name( ) : blank );
  }
  
  if( *list_entry[list][2] == 'M' ) {
    species = get_species( list_value[list][i] );
    return( species ? species->Name( ) : blank );
  }
  
  return empty_string;
}  


static int find_list( const char *argument )
{
  int l = strlen( argument );

  for( int i = 0; i < MAX_LIST; ++i ) {
    if( !strncasecmp( argument, list_entry[0][i], l ) ) {
      return i;
    }
  }

  return -1;
}


void do_ledit( char_data* ch, const char *argument )
{
  wizard_data *imm;
  
  if( !( imm = wizard( ch ) ) )
    return;
  
  if( !*argument ) {
    int i = imm->list_edit;
    if( i != -1 ) {
      fsend( ch, "You stop editing list '%s'.", list_entry[0][i] );
      imm->list_edit = -1;
    } else {
      do_lstat( ch, "" );
    }
    return;
  }
  
  int i = find_list( argument );

  if( i < 0 ) {
    send( ch, "No list with that name found.\n\r" );
    return;
  }

  fsend( ch, "Lstat and lset now operate on list '%s'.", list_entry[0][i] );
  imm->list_edit = i;
}


void do_lstat( char_data* ch, const char *argument )
{
  char             tmp  [ TWO_LINES ];
  int                i;
  int           number;
  int list;

  wizard_data *imm = wizard( ch );

  if( *argument ) {
    if( ( list = find_list( argument ) ) < 0 ) {
      send( ch, "No list with that name found.\n\r" );
      return;
    }

  } else if( ( list = imm->list_edit ) < 0 ) {
    display_array( ch, "Lists", &list_entry[0][0],
		   &list_entry[0][1], MAX_LIST );
    return;
  }

  page( ch, "%s:\n\r", list_entry[0][list] );

  const unsigned columns = 2;
  const unsigned width = ( ch->pcdata->columns - 1 ) / columns;

  if( isdigit( *list_entry[list][0] ) ) {
    number = atoi( list_entry[list][0] );
    for( i = 0; i < number; ++i ) {
      snprintf( tmp, TWO_LINES, "%6s[%2d] : %-5d%-*s",
		list_entry[list][1], i,
		list_value[list][i],
		width-18,
		list_word( list, i ) );
      page( ch, trunc( tmp, width ) );
      if( i%columns == columns-1 )
        page( ch, "\n\r" );
    }
  } else {
    for( i = 0; *list_entry[list][i]; i++ ) {
      /*
      snprintf( tmp, TWO_LINES, "%20s : %-10d%s",
		list_entry[list][i], list_value[list][i],
		i%2 ? "\n\r" : "" );
      page( ch, tmp );
      */
      page( ch, "%20s : %-10d%s",
	    list_entry[list][i], list_value[list][i],
	    i%columns == columns-1 ? "\n\r" : "" );
    }
  }

  if( i%columns != 0 )
    page( ch, "\n\r" );
}


void do_lset( char_data* ch, const char *argument )
{
  char             arg  [ MAX_INPUT_LENGTH ];
  int i;
  int           number;
  int           length;

  wizard_data *imm = wizard( ch );
  int list = imm->list_edit; 
  
  if( list == -1 ) {
    send( ch, "You are not editing any list.\n\r" );
    return;
  }

  if( !*argument ) {
    do_lstat( ch, "" );
    return;
  }

  if( list_value[0][list] > get_trust( ch ) ) {
    fsend( ch, "You need to be level %d to edit '%s' list.",
	   list_value[0][list], list_entry[0][list] );
    return;
  }
  
  argument = one_argument( argument, arg );
  
  if( isdigit( *list_entry[list][0] ) ) {
    number = atoi( list_entry[list][0] );
    i = atoi( arg );
    if( i < 0 || i >= number ) {
      send( ch, "Index for list out of range.\n\r" );
      return;
    }
    number = atoi( argument );
    send( ch, "List entry %d set to %d.\n\r", i, number );
  } else {
    length = strlen( arg );
    for( i = 0; ; i++ ) {
      if( !*list_entry[list][i] ) {
        send( ch, "Entry not found in list.\n\r" );
        return;
      }
      if( !strncasecmp( arg, list_entry[list][i], length ) )
        break;
    }
    number = atoi( argument );
    send( ch, "List entry '%s' set to %d.\n\r",
	  list_entry[list][i], number );
  }
  
  list_value[list][i] = number;
}


/*
 *   DISK ROUTINES
 */


void save_lists( )
{
  rename_file( TABLE_DIR, LIST_FILE,
	       TABLE_PREV_DIR, LIST_FILE );
  
  FILE *fp = open_file( TABLE_DIR, LIST_FILE, "w" );
  
  if( !fp )
    return;

  for( int i = 0; i < MAX_LIST; ++i ) {
    for( int j = 0; j < 3; ++j ) {
      for( int k = 0; k < 10; ++k ) {
        fprintf( fp, "%4d ", list_value[i][10*j+k] );
      }
      fprintf( fp, "\n" );
    }
    fprintf( fp, "\n" );
  }
  
  fclose( fp );
}


void load_lists( )
{
  echo( "Loading Lists...\n\r" );
 
  FILE *fp = open_file( TABLE_DIR, LIST_FILE, "r", true );

  for( int i = 0; i < MAX_LIST; ++i ) {
    for( int j = 0; j < 30; ++j ) {
      list_value[i][j] = fread_number( fp );
    }
  }

  fclose( fp );
}
