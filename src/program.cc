#include <ctype.h>
#include <sys/types.h>
#include <stdio.h>
#include <syslog.h>
#include "define.h"
#include "struct.h"


program_data :: program_data( )
  : binary(0), memory(0),
    active(0), corrupt(false),
    code(empty_string)
{
}
  
program_data ::  ~program_data( )
{
  if( active != 0 ) {
    bug( "Program: deleting active program." );
    bug( "-- Active count = %d", active );
  }

  clear_queue( this );
  free_string( code, MEM_CODE );
  delete_list( memory );
  data.delete_list( );
  delete binary;
}


void program_data :: read( FILE *fp )
{
  code = fread_string( fp, MEM_CODE );
  read_extras( fp, data );
}


void program_data :: write( FILE *fp )
{
  fwrite_string( fp, code );
  write_extras( fp, data );
}


void program_data :: Set_Code( const char *text )
{
  free_string( code, MEM_CODE );

  if( !text )
    code = 0;
  else
    code = alloc_string( text, MEM_CODE );
}


void program_data :: Read_Code( FILE *fp )
{
  free_string( code, MEM_CODE );
  code = fread_string( fp, MEM_CODE );
}


void program_data :: Edit_Code( char_data *ch, const char *text )
{
  code = edit_string( ch, text, code, MEM_CODE, false );
}


extra_array& program_data :: Extra_Descr( )
{
  return data;
}


extra_data *program_data :: find_extra( const char *key )
{
  extra_array& extras = Extra_Descr( );

  for( int i = 0; i < extras; ++i ) {
    if( !strcasecmp( extras[i]->keyword, key ) ) {
      return extras[i];
    }
  }

  return 0;
}


const char *program_data :: Code( ) const
{
  return code;
}


const char *prog_msg( program_data *action, const default_data& defaults )
{
  if( action ) {
    if( const extra_data *extra = action->find_extra( defaults.name ) ) {
      return extra->text;
    }
    /*
    extra_array& extras = action->Extra_Descr( );
    for( int i = 0; i < extras; i++ ) {
      const extra_data *extra = extras[i];
      if( !strcasecmp( extra->keyword, defaults.name ) ) {
        return extra->text;
      }
    }
    */
  }

  return defaults.msg;
}


void show_defaults( char_data *ch, int trigger, const default_data **data, int type )
{
  if( const default_data *list = data[ trigger ] ) {
    bool found = false;
    for( int i = 0; *list[i].name; ++i ) {
      if( list[i].type == type ) {
	if( !found ) {
	  page( ch, "Defaults:\n\r" );
	  found = true;
	}
	page( ch, "  %s: \"%s\"\n\r", list[i].name, list[i].msg );
      }
    }

    if( found )
      page( ch, "\n\r" );

    else if( type >= 0 ) {
      for( int i = 0; *list[i].name; ++i ) {
	if( list[i].type < 0 ) {
	  if( !found ) {
	    page( ch, "Defaults:\n\r" );
	    found = true;
	  }
	  page( ch, "  %s: \"%s\"\n\r", list[i].name, list[i].msg );
	}
      }

      if( found )
	page( ch, "\n\r" );
    }
  }
  /*
  if( const char **list = data[ trigger ] ) {
    page( ch, "Defaults:\n\r" );
    char tmp  [ TWO_LINES ];
    for( int i = 0; *list[i] != '\0'; ) {
      if( *list[i+2] ) {
	snprintf( tmp, TWO_LINES, "  %s (%s): \"%s\"\n\r", list[i], list[i+2], list[i+1] );
      } else {
	snprintf( tmp, TWO_LINES, "  %s: \"%s\"\n\r", list[i], list[i+1] );
      }
      i += 3;
      page( ch, tmp );
    }
    page( ch, "\n\r" );
  }
  */
}
