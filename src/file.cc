#include <ctype.h>
#include <errno.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <unistd.h>
#include "define.h"
#include "struct.h"


/*
 *   FILE SUBROUTINES
 */


FILE *open_file( const char* dir, const char* file, const char* type,
		 bool fatal )
{
  check_panic( );

  char  tmp  [ TWO_LINES ];

  snprintf( tmp, TWO_LINES, "%s%s", dir, file );

  return open_file( tmp, type, fatal );
}


/* This function opens a file.  It opens up a file in directory path, with
   name type.  If fatal is true, and it can't find the file, the MUD 
   shutsdown */
FILE *open_file( const char* path, const char* type, bool fatal )
{
  check_panic( );

  /* Try to open file */
  if( FILE *fp = fopen( path, type ) )
    return fp;

  /* Print error */
  roach( "Open_File: %s", strerror( errno ) );
  roach( "-- File = '%s'", path );

  /* Check fatal */
  if( fatal ) {
    shutdown( "open_file fatal: ", path );
  }

  return 0;
}


bool delete_file( const char* dir, const char* file, bool msg )
{
  char *path  = static_string( );
  int i  = strlen( dir );

  snprintf( path, THREE_LINES, "%s%s", dir, file );
  path[i] = toupper( path[i] );

  check_panic( );

  if( unlink( path ) < 0 ) {
    if( msg ) {
      roach( "Delete_File: %s", strerror( errno ) );
      roach( "-- File = '%s'", path );
    }
    return false;
  }

  return true;
}


bool rename_file( const char* dir1, const char* file1,
		  const char* dir2, const char* file2 )
{
  char *tmp1 = static_string( );
  snprintf( tmp1, THREE_LINES, "%s%s", dir1, file1 );

  if( !exist_control_file( tmp1 ) )
    return false;

  char *tmp2 = static_string( );
  snprintf( tmp2, THREE_LINES, "%s%s", dir2, file2 );

  check_panic( );

  if( rename( tmp1, tmp2 ) < 0 ) {
    roach( "Rename_File: %s", strerror( errno ) );
    roach( "-- Old File = '%s'", tmp1 );
    roach( "-- New File = '%s'", tmp2 );
    return false;
  }

  return true;
}


/*
 *   FREAD ROUTINES
 */


char fread_letter( FILE *fp )
{
  char c;

  do {
    c = getc( fp );
  } while( isspace( c ) );
  
  return c;
}


int fread_number( FILE *fp )
{
  int number = 0;
  int sign = 1;
  char c;
  
  do {
    c = getc( fp );
  } while( isspace( c ) );
  
  switch( c ) {
  case '-' :  sign = -1;
  case '+' :  c = getc( fp );
  }
  
  if( !isdigit( c ) ) {
    bug( "Fread_number: bad format." );
    printf( "%s\n", fread_string( fp, MEM_UNKNOWN ) );
    shutdown( "fread_number bad format " );
  }
  
  while( isdigit( c ) ) {
    number = number * 10 + c - '0';
    c = getc( fp );
  }
  
  number *= sign;
  
  while( c == ' ' ) {
    c = getc( fp );
  }
  
  ungetc( c, fp );

  return number;
}


unsigned fread_unsigned( FILE *fp )
{
  unsigned number = 0;
  char c;
  
  do {
    c = getc( fp );
  } while( isspace( c ) );
  
  if( !isdigit( c ) ) {
    bug( "Fread_unsigned: bad format." );
    printf( "%s\n", fread_string( fp, MEM_UNKNOWN ) );
    shutdown( "fread_unsigned bad format " );
  }
  
  while( isdigit( c ) ) {
    number = number * 10 + c - '0';
    c = getc( fp );
  }
  
  while( c == ' ' ) {
    c = getc( fp );
  }
  
  ungetc( c, fp );

  return number;
}


char *fread_string( FILE* fp, int type )
{
  char c;

  do {
    c = getc( fp );
  } while( isspace( c ) );
  
  if( c != '\"' ) {
    panic( "Fread_String: start-of-string delimiter not found." );
  }

  char buf [ 4*MAX_STRING_LENGTH ];
  size_t len = 0;

  while( ( c = getc( fp ) ) && c != '\"' ) {
    if( c == '\\' ) {
      c = getc( fp );
      if( !c ) {
	panic( "Fread_String: EOF inside string." ); 
      }
      if( c != '\"' && c != '\\' ) {
	panic( "Fread_String: Bad escaped character." ); 
      }
    }
    buf[ len++ ] = c;
  }

  if( !c ) {
    panic( "Fread_String: EOF inside string." ); 
  }

  buf[ len ] = '\0';

  return alloc_string( buf, type );
}


/*
char *fread_string( FILE* fp, int type )
{
  char        buf  [ 4*MAX_STRING_LENGTH ];
  int      length  = 0;
 
  do {
    *buf = getc( fp );
  } while( *buf == '\n' || *buf == '\r' || *buf == ' ' );

  if( *buf == '.' ) {
    *buf = getc( fp );
  }

  for( ; buf[ length ] != '~' && buf[ length ] != EOF; ) 
    buf[ ++length ] = getc( fp );    

  if( buf[ length ] == EOF ) 
    panic( "Fread_string: EOF" ); 

  buf[ length ] = '\0';

  return alloc_string( buf, type );
}
*/


void fread_to_eol( FILE *fp )
{
  char c;

  do {
    c = getc( fp );
  } while( c != '\n' && c != '\r' && c != EOF );

  do {
    c = getc( fp );
  } while( c == '\n' || c == '\r' );

  ungetc( c, fp );
}


char *fread_word( FILE *fp )
{
  static char    buf  [ MAX_STRING_LENGTH ];
  char        letter;
  char             c;

  do {
    letter = getc( fp );
  } while( isspace( letter ) );
  
  char *pWord = buf;

  if( letter != '\'') {
    *buf = letter;
    ++pWord;
  }

  for( ; pWord < &buf[ MAX_STRING_LENGTH ]; pWord++ ) {
    *pWord = getc( fp );
    if( ( isspace( *pWord ) && letter != '\'' )
	|| ( *pWord == '\'' && letter == '\'' ) ) {
      *pWord = '\0';
      do {
        c = getc( fp );
      } while( c == ' ' );
      ungetc( c, fp );
      return buf;
    }
  }
  
  bug( "Fread_word: word too long." );
  printf( "%s\n\r", buf );

  panic( "fread_word too long ", buf );
  //  shutdown( "fread_word too long ", buf );
  return 0;	// To prevent compiler warning.
}


/*
char *fread_block( FILE *fp )
{
  static char buf[ MAX_STRING_LENGTH ];
  char letter, c;

  do {
    letter = getc( fp );
  } while( isspace( letter ) );

  char *pWord = buf;

  *buf = letter;
  ++pWord;

  for( ; pWord < &buf[ MAX_STRING_LENGTH ]; pWord++ ) {
    *pWord = getc( fp );
    if( isspace( *pWord ) ) {
      *pWord = '\0';
      do {
        c = getc( fp );
      } while( c == ' ' );
      ungetc( c, fp );
      return buf;
    }
  }
  
  bug( "Fread_block: word too long." );
  printf( "%s\n\r", buf );

  shutdown( "fread_block word too long ", buf );
  return 0;	// To prevent compiler warning.
}
*/


/*
 *   FWRITE ROUTINES
 */


void fwrite_string( FILE *fp, const char *s )
{
  if( fputc( '\"', fp ) == EOF ) {
    bug( "Fwrite_String: can't write start-of-string delimiter." );
  }

  if( s ) {
    while( *s ) {
      size_t n = 0;
      const char *t = s;
      for( ; *t && *t != '\"' && *t != '\\'; ++t ) {
	++n;
      }
      if( n > 0 ) {
	if( fwrite( s, sizeof( char ), n, fp ) != n ) {
	  bug( "Fwrite_String: can't write string text." );
	}
	s += n;
      }
      
      if( *t == '\"' ) {
	if( fputs( "\\\"", fp ) == EOF ) {
	  bug( "Fwrite_String: can't write string text." );
	}
	++s;
      } else if( *t == '\\' ) {
	if( fputs( "\\\\", fp ) == EOF ) {
	  bug( "Fwrite_String: can't write string text." );
	}
	++s;
      }
    }
  }

  if( fputs( "\"\n", fp ) == EOF ) {
    bug( "Fwrite_String: can't write end-of-string delimiter." );
  }
}


/*
 *   CHANGES
 */


void do_changes( char_data* ch, const char *argument )
{
  int days = 5;

  number_arg( argument, days );

  if( days <= 0 || days > 100 ) {
    send( ch, "Changes: range is 1 to 100 days.\n\r" );
    return;
  }

  page_title( ch, "Changes within the last %d days", days );

  bool found = false;

  page( ch, "\n\r" );
  page_centered( ch, "-- Mobs --\n\r" );

  for( int i = 1; i <= species_max; ++i ) 
    if( species_data *species = species_list[i] )
      if( species->date > current_time-days*24*60*60 ) {
        page( ch, "[%5d] %s\n\r", species->vnum, trunc( species->Name( ), (size_t)71 ) );
	found = true;
      }

  if( !found ) {
    page( ch, "None found.\n\r" );
  }

  found = false;

  page( ch, "\n\r" );
  page_centered( ch, "-- Objects --\n\r" );

  for( int i = 1; i <= obj_clss_max; ++i ) {
    if( obj_clss_data *obj_clss = obj_index_list[i] ) {
      if( obj_clss->date > current_time-days*24*60*60 ) {
        page( ch, "[%5d] %s\n\r", obj_clss->vnum, trunc( obj_clss->Name( ), (size_t)71 ) );
	found = true;
      }
    }
  }

  if( !found ) {
    page( ch, "None found.\n\r" );
  }
}


/*
 *   DO_WRITE ROUTINE
 */


void write_all( bool forced )
{
  for( area_data *area = area_list; area; area = area->next ) {
    area->Save( forced );
    /*
    if( forced ) {
      for( room_data *room = area->room_first; room; room = room->next ) {
	room->Save();
      }
    }
    */
  }

  save_world( );
  save_accounts( );
  save_mobs( );
  save_objects( );
  save_notes( -1 );
  save_help( );
  save_trainers( );
  save_quests( );
  save_shops( );
  save_tables( );
  save_rtables( );
  save_lists( );
  save_clans( );
  save_dictionary( );
}


void do_load( char_data* ch, const char *argument )
{
  if( !*argument ) {
    send( ch, "Which file do you wish to load?\n\r" );
    return;
  }
  
  if( matches( argument, "tables" ) ) {
    load_tables( );
    return;
  }

  send( ch, "Unknown file.\n\r" );
}


void do_write( char_data* ch, const char *argument )
{
  char           buf  [ MAX_INPUT_LENGTH ];
  char           buf1  [ MAX_INPUT_LENGTH ];
  int          flags;

  if( !get_flags( ch, argument, &flags, "f", "write" ) )
    return;

  if( !strcasecmp( argument, "all" ) ) {
    if( !has_permission( ch, PERM_WRITE_ALL, true ) ) 
      return;
    write_all( is_set( flags, 0 ) );
    send( ch, "All files written.\n\r" );
    snprintf( buf, MAX_INPUT_LENGTH, "All files written by %s.", ch->descr->name );
    snprintf( buf1, MAX_INPUT_LENGTH, "All files written." );
    info( LEVEL_BUILDER, buf1, invis_level( ch ), buf, IFLAG_WRITES, 1, ch );
    return;
  }
  
  if( !*argument ) {
    if( ch->can_edit( ch->in_room ) ) {
      if( !ch->in_room->area->modified ) {
	send( ch, "Area has not been modified so was not saved.\n\r" );
      } else {
	ch->in_room->area->Save( );
	send( ch, "Area written.\n\r" );
      }
    }
    return;
  }

  bool match = true;

  while( match ) {
    match = false;

    if( matches( argument, "areas" ) ) {
      match = true;
      if( !has_permission( ch, PERM_WRITE_AREAS, true ) )
	continue;
      int number = 0;
      for( area_data *area = area_list; area; area = area->next ) 
	number += area->Save( is_set( flags, 0 ) );
      if( number > 0 ) {
	send( ch, "All modified areas written (%d files).\n\r", number );
	snprintf( buf, MAX_INPUT_LENGTH, "All modified areas written by %s.", ch->real_name() );
	snprintf( buf1, MAX_INPUT_LENGTH, "All modified areas written." );
	info( LEVEL_BUILDER, buf1, invis_level( ch ), buf, IFLAG_WRITES, 1, ch );
      } else {
	send( ch, "No area needed saving.\n\r" );
      }
      continue;
    }

    if( matches( argument, "accounts" ) ) {
      match = true;
      if( !has_permission( ch, PERM_ACCOUNTS, true ) ) 
	continue;
      save_accounts( );
      send( ch, "Accounts written.\n\r" );
      snprintf( buf, MAX_INPUT_LENGTH, "Accounts written by %s.", ch->real_name() );
      snprintf( buf1, MAX_INPUT_LENGTH, "Accounts written." );
      info( LEVEL_BUILDER, buf1, invis_level( ch ), buf, IFLAG_WRITES, 1, ch );
      continue;
    }
  
    if( matches( argument, "rtables" ) ) {
      match = true;
      if( !has_permission( ch, PERM_RTABLES, true ) ) 
	continue;
      save_rtables( );
      send( ch, "Rtables written.\n\r" );
      snprintf( buf, MAX_INPUT_LENGTH, "Rtables written by %s.", ch->real_name() );
      snprintf( buf1, MAX_INPUT_LENGTH, "Rtables written." );
      info( LEVEL_BUILDER, buf1, invis_level( ch ), buf, IFLAG_WRITES, 1, ch );
      continue;
    }
  
    if( matches( argument, "mobs" ) ) {
      match = true;
      if( !has_permission( ch, PERM_MOBS, true ) )
	continue;
      save_mobs( );
      send( ch, "Mob file written.\n\r" );
      snprintf( buf, MAX_INPUT_LENGTH, "Mob file written by %s.", ch->real_name() );
      snprintf( buf1, MAX_INPUT_LENGTH, "Mob file written." );
      info( LEVEL_BUILDER, buf1, invis_level( ch ), buf, IFLAG_WRITES, 1, ch );
      continue;
    }

    if( matches( argument, "objects" ) ) {
      match = true;
      if( !has_permission( ch, PERM_OBJECTS, true ) )
	continue;
      save_objects( );
      send( ch, "Object file written.\n\r" );
      snprintf( buf, MAX_INPUT_LENGTH, "Object file written by %s.", ch->real_name() );
      snprintf( buf1, MAX_INPUT_LENGTH, "Object file written." );
      info( LEVEL_BUILDER, buf1, invis_level( ch ), buf, IFLAG_WRITES, 1, ch );
      continue;
    }
  
    if( matches( argument, "notes" ) ) {
      match = true;
      if( !has_permission( ch, PERM_NOTEBOARD, true ) )
	continue;
      save_notes( -1 );
      send( ch, "All noteboards written.\n\r" );
      snprintf( buf, MAX_INPUT_LENGTH, "All noteboards written by %s.", ch->real_name() );
      snprintf( buf1, MAX_INPUT_LENGTH, "All noteboards written." );
      info( LEVEL_BUILDER, buf1, invis_level( ch ), buf, IFLAG_WRITES, 1, ch );
      continue;
    }
  
    if( matches( argument, "shops" ) ) {
      match = true;
      if( !has_permission( ch, PERM_ROOMS, true ) )
	continue;
      save_shops( );
      send( ch, "Shop file written.\n\r" );
      snprintf( buf, MAX_INPUT_LENGTH, "Shop file written by %s.", ch->real_name() );
      snprintf( buf1, MAX_INPUT_LENGTH, "Shop file written." );
      info( LEVEL_BUILDER, buf1, invis_level( ch ), buf, IFLAG_WRITES, 1, ch );
      continue;
    }

    if( matches( argument, "tables" ) ) {
      match = true;
      if( !has_permission( ch, PERM_MISC_TABLES )
	  && !has_permission( ch, PERM_SOCIALS, true ) ) 
	continue;
      save_tables( );
      send( ch, "Table files written.\n\r" );
      snprintf( buf, MAX_INPUT_LENGTH, "Table files written by %s.", ch->real_name() );
      snprintf( buf1, MAX_INPUT_LENGTH, "Table files written." );
      info( LEVEL_IMMORTAL, buf1, invis_level( ch ), buf, IFLAG_WRITES, 1, ch );
      continue;
    }

    if( matches( argument, "trainers" ) ) {
      match = true;
      if( !has_permission( ch, PERM_ALL_MOBS, true ) )
	continue;
      save_trainers( );
      send( ch, "Trainer file written.\n\r" );
      snprintf( buf, MAX_INPUT_LENGTH, "Trainer file written by %s.", ch->real_name() );
      snprintf( buf1, MAX_INPUT_LENGTH, "Trainer file written." );
      info( LEVEL_BUILDER, buf1, invis_level( ch ), buf, IFLAG_WRITES, 1, ch );
      continue;
    }

    if( matches( argument, "help" ) ) {
      match = true;
      if( !has_permission( ch, PERM_HELP_FILES, true ) )
	continue;
      if( !save_help( ) ) {
	send( ch, "Help was not modified so was not saved.\n\r" );
	continue;
      }
      send( ch, "Help file written.\n\r" );
      snprintf( buf, MAX_INPUT_LENGTH, "Help file written by %s.", ch->real_name() );
      snprintf( buf1, MAX_INPUT_LENGTH, "Help file written." );
      info( LEVEL_BUILDER, buf1, invis_level( ch ), buf, IFLAG_WRITES, 1, ch );
      continue;
    }
  
    if( matches( argument, "quests" ) ) {
      match = true;
      if( !has_permission( ch, PERM_QUESTS, true ) )
	continue;
      save_quests( );
      send( ch, "Quest file written.\n\r" );
      snprintf( buf, MAX_INPUT_LENGTH, "Quest file written by %s.", ch->real_name() );
      snprintf( buf1, MAX_INPUT_LENGTH, "Quest file written." );
      info( LEVEL_BUILDER, buf1, invis_level( ch ), buf, IFLAG_WRITES, 1, ch );
      continue;
    }
  
    if( matches( argument, "world" ) ) {
      match = true;
      if( !has_permission( ch, PERM_WRITE_ALL, true ) ) 
	continue;
      save_world( );
      send( ch, "World file written.\n\r" );
      snprintf( buf, MAX_INPUT_LENGTH, "World file written by %s.", ch->descr->name );
      snprintf( buf1, MAX_INPUT_LENGTH, "World file written." );
      info( LEVEL_IMMORTAL, buf1, invis_level( ch ), buf, IFLAG_WRITES, 1, ch );
      continue;
    }

    if( matches( argument, "clans" ) ) {
      match = true;
      if( !has_permission( ch, PERM_CLANS, true ) )
	continue;
      save_clans( );
      send( ch, "Clan files written.\n\r" );
      snprintf( buf, MAX_INPUT_LENGTH, "Clan files written by %s.", ch->real_name() );
      snprintf( buf1, MAX_INPUT_LENGTH, "Clan files written." );
      info( LEVEL_IMMORTAL, buf1, invis_level( ch ), buf, IFLAG_WRITES, 1, ch );
      continue;
    }

    if( matches( argument, "lists" ) ) {
      match = true;
      if( !has_permission( ch, PERM_LISTS, true ) ) 
	continue;
      save_lists( );
      send( ch, "Table file written.\n\r" );
      snprintf( buf, MAX_INPUT_LENGTH, "List file written by %s.", ch->real_name() );
      snprintf( buf1, MAX_INPUT_LENGTH, "List file written." );
      info( LEVEL_IMMORTAL, buf1, invis_level( ch ), buf, IFLAG_WRITES, 1, ch );
      continue;
    }

    if( matches( argument, "dictionary" ) ) {
      match = true;
      if( !has_permission( ch, PERM_MISC_TABLES, true ) ) 
	continue;
      save_dictionary( );
      send( ch, "Dictionary files written.\n\r" );
      snprintf( buf, MAX_INPUT_LENGTH, "Dictionary files written by %s.", ch->real_name() );
      snprintf( buf1, MAX_INPUT_LENGTH, "Dictionary files written." );
      info( LEVEL_IMMORTAL, buf1, invis_level( ch ), buf, IFLAG_WRITES, 1, ch );
      continue;
    }
  }

  if( *argument ) {
    send( ch, "What do you want to write?\n\r" );
    return;
  }
}


void quit( int status )
{
  fflush( stdout );
  fflush( stderr );

  exit( status );
}


void shutdown( const char *reason, const char *text )
{
  create_control_file( SHUTDOWN_FILE, reason, text );
  quit( 1 );
}


void reboot( const char *reason, const char *text )
{
  create_control_file( REBOOT_FILE, reason, text );
  quit( 1 );
}


bool create_control_file( const char *name, const char *reason, const char *msg )
{
  check_panic( );

  if( FILE *sf = open_file( name, "w", false ) ) {
    if( !msg )
      msg = empty_string;
    fprintf( sf, "By %s%s at %s\n", reason, msg, ltime( time( 0 ) ) );
    fclose( sf );
    return false;
  }

  return true;
}


bool exist_control_file( const char *name )
{
  if( FILE *fp = fopen( name, "r" ) ) {
    fclose( fp );
    return true;
  }

  return false;
}


void delete_control_file( const char *name )
{
  delete_file( empty_string, name, false );
}


void check_panic( )
{
  if( exist_control_file( PANIC_FILE ) )
    quit( 1 );
}
