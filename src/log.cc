#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <syslog.h>
#include "define.h"
#include "struct.h"


void  add_log      ( const char*, const char*, char_data*, const char* );


/*
 *   SUPPORT FUNCTIONS
 */


void add_log( const char* function, const char* file, char_data* ch,
  const char* string )
{
  char      tmp  [ MAX_INPUT_LENGTH+20 ];
  FILE*      fp;

  if( !( fp = open_file( file, "a" ) ) ) {
    //    bug( "%s: Could not open file.", function );
    return;
  }
  
  snprintf( tmp, MAX_INPUT_LENGTH+20, "[%s] %s", ltime( current_time, true ), string );

  tmp[20] = toupper( tmp[20] );

  if( strlen( tmp ) > 60 ) {
    snprintf( tmp+60, MAX_INPUT_LENGTH-40, "...  %s\n\r", ch->real_name( ) );
    fprintf( fp, tmp );
  } else {
    fprintf( fp, "%-65s%s\n\r", tmp, ch->real_name( ) );
  }
  
  fclose( fp );
}


static bool view_file( char_data* ch, char* file, const char *title )
{ 
  FILE*      fp;
  char      tmp  [ MAX_INPUT_LENGTH ];
  int    length;

  if( !( fp = fopen( file, "r" ) ) ) 
    return false;

  page_title( ch, title );

  for( length = 0; ; ) {
    tmp[ length ] = getc( fp );
    if( tmp[ length++ ] == EOF ) {
      tmp[ length ] = '\0';
      page( ch, tmp );
      page( ch, "\n\r" );
      break;
    }    
    if( length == MAX_INPUT_LENGTH-1 ) {
      tmp[ MAX_INPUT_LENGTH-1 ] = '\0'; 
      page( ch, tmp );
      length = 0;
    }
  }

  fclose( fp );

  return true;
}


/*
 *   MOB LOG ROUTINES
 */


void mob_log( char_data* ch, int i, const char* string )
{
  char  file  [ ONE_LINE ];

  if( string && *string ) {
    snprintf( file, ONE_LINE, "%smob.%d", MOB_LOG_DIR, i );
    add_log( "Mob_Log", file, ch, string );
  }
}


void do_mlog( char_data* ch, const char *argument )
{
  wizard_data *imm;
  if( !( imm = wizard( ch ) ) )
    return;

  char tmp [ ONE_LINE ];
  char tmp2 [ TWO_LINES ];

  species_data *species = imm->mob_edit && !*argument ? imm->mob_edit : 0;

  if( !species
      && is_number( argument ) ) { 
    if( !( species = get_species( atoi( argument ) ) ) ) {
      send( ch, "There is no mob with that vnum.\n\r" );
      return;
    }
  }

  pfile_data *pfile = imm->player_edit && !*argument ? imm->player_edit->pcdata->pfile : 0;
  if( !pfile
      && !species
      && !( pfile = find_pfile_exact( argument ) ) ) {
    char_data *victim;
    if( !( victim = one_character( ch, argument, "mlog",
				   (thing_array*) &player_list,
				   (thing_array*) &mob_list ) ) ) 
      return;

    if( victim->pcdata )
      pfile = victim->pcdata->pfile;
    else
      species = victim->species;
  }


  if( pfile ) {
    if( !has_permission( ch, PERM_PLAYERS ) ) {
      send( ch, "You can't mlog players.\n\r" );
      return;
    }
    snprintf( tmp, ONE_LINE, "%s%s", PLAYER_LOG_DIR, pfile->name );
    if( !view_file( ch, tmp, pfile->name ) )
      fsend( ch, "%s has no log.", pfile->name );
    return;
  }

  snprintf( tmp, ONE_LINE, "%smob.%d", MOB_LOG_DIR, species->vnum );
  snprintf( tmp2, ONE_LINE, "%s [%d]", species->Name( ), species->vnum );
  if( !view_file( ch, tmp, tmp2 ) )
    fsend( ch, "%s has no log.", species );
}


/*
 *   OBJECT LOG FILES
 */


void obj_log( char_data* ch, int i, const char* string )
{
  char file [ ONE_LINE ];
  
  if( string && *string ) {
    snprintf( file, ONE_LINE, "%sobj.%d", OBJ_LOG_DIR, i );
    add_log( "Obj_Log", file, ch, string );
  }
}


void do_olog( char_data* ch, const char *argument )
{
  wizard_data *imm;
  if( !( imm = wizard( ch ) ) )
    return;

  obj_clss_data*  obj_clss;
  int                    i;

  if( !*argument ) {
    if( !( obj_clss = imm->obj_clss_edit ) ) {
      send( ch,
	    "You are not editing an object type so you must specify a vnum.\n\r" );
      return;
    }
    i = obj_clss->vnum;
  } else if( !( obj_clss = get_obj_index( i = atoi( argument ) ) ) ) {
    send( ch, "There is no object type with that vnum.\n\r" );
    return;
  }
  
  char *tmp  = static_string( );
  char *tmp2  = static_string( );
  snprintf( tmp, THREE_LINES, "%sobj.%d", OBJ_LOG_DIR, i );
  snprintf( tmp2, THREE_LINES, "%s [%d]", obj_clss->Name( ), obj_clss->vnum );

  if( !view_file( ch, tmp, tmp2 ) )
    send( ch, "That object type has no log.\n\r" );
}


/*
 *   ROOM LOG FUNCTIONS
 */


void room_log(  char_data* ch, int i, const char* string )
{
  char file  [ ONE_LINE ];

  if( string && *string ) {
    snprintf( file, ONE_LINE, "%sroom.%d", ROOM_LOG_DIR, i );
    add_log( "Room_Log", file, ch, string );
  }
}


void do_rlog( char_data* ch, const char *)
{
  char tmp [ ONE_LINE ];
  char tmp2 [ TWO_LINES ];

  snprintf( tmp, ONE_LINE, "%sroom.%d", ROOM_LOG_DIR, ch->in_room->vnum );
  snprintf( tmp2, ONE_LINE, "%s [%d]", ch->in_room->name, ch->in_room->vnum );

  if( !view_file( ch, tmp, tmp2 ) )
    send( ch, "There is no log for this room.\n\r" );
}


/*
 *   PLAYER LOG FILES
 */


void player_log( char_data* ch, const char* string )
{
  if( !ch->pcdata
      || !string
      || !*string )
    return;

  char    file  [ ONE_LINE ];
  FILE*     fp;

  snprintf( file, ONE_LINE, "%s%s", PLAYER_LOG_DIR, ch->real_name( ) );

  if( !( fp = open_file( file, "a" ) ) )
    return;
  
  char tmp  [ MAX_INPUT_LENGTH+20 ];
  snprintf( tmp, MAX_INPUT_LENGTH+20, "[%s] %s\n\r", ltime( current_time, true ), string );

  tmp[15] = toupper( tmp[15] );

  fprintf( fp, tmp );

  fclose( fp );
}


void player_log( pfile_data *pfile, const char* string )
{
  if( !string
      || !*string )
    return;

  char    file  [ ONE_LINE ];
  FILE*     fp;

  snprintf( file, ONE_LINE, "%s%s", PLAYER_LOG_DIR, pfile->name );

  if( !( fp = open_file( file, "a" ) ) )
    return;
  
  char tmp  [ MAX_INPUT_LENGTH+20 ];
  snprintf( tmp, MAX_INPUT_LENGTH+20, "[%s] %s\n\r", ltime( current_time, true ), string );

  tmp[15] = toupper( tmp[15] );

  fprintf( fp, tmp );

  fclose( fp );
}


void immortal_log( char_data* ch, const char *cmd, const char *arg )
{
  char    file  [ ONE_LINE ];
  FILE*     fp;
  
  snprintf( file, ONE_LINE, "%s%s", IMMORTAL_LOG_DIR, ch->real_name( ) );
  
  if( ( fp = fopen( file, "a" ) ) ) {
    fprintf( fp, "[%s] %s %s\n",
	     ltime( current_time, true ), cmd, arg );
    fclose( fp );
  }
}
