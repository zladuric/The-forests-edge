#include <ctype.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include "define.h"
#include "struct.h"


int              num_rtable  = 0;
rtable_data**   rtable_list  = 0;


/*
 *   RTABLE_DATA NEW AND DELETE
 */


Rtable_Data :: Rtable_Data( )
  : name(empty_string), reset(0)
{
}


Rtable_Data :: ~Rtable_Data( )
{
  free_string( name, MEM_RESET );
}


/*
 *   SUPPORT ROUTINES
 */


static bool has_reset( int index )
{
  for( int i = 1; i <= species_max; ++i ) 
    if( species_data *species = species_list[i] )
      for( reset_data *reset = species->reset; reset; reset = reset->next ) 
        if( index == reset->vnum 
	    && !is_set( &reset->flags, RSFLAG_MOB )
	    && !is_set( &reset->flags, RSFLAG_OBJECT ) ) 
          return true;
  
  for( area_data *area = area_list; area; area = area->next )
    for( room_data *room = area->room_first; room; room = room->next )
      for( reset_data *reset = room->reset; reset; reset = reset->next )
        if( index == reset->vnum 
	    && !is_set( &reset->flags, RSFLAG_MOB )
	    && !is_set( &reset->flags, RSFLAG_OBJECT ) ) 
          return true;
  
  for( int i = 0; i < num_rtable; i++ ) 
    for( reset_data *reset = rtable_list[i]->reset; reset; reset = reset->next )
      if( index == reset->vnum 
	  && !is_set( &reset->flags, RSFLAG_MOB )
	  && !is_set( &reset->flags, RSFLAG_OBJECT ) )
        return true;
  
  return false;
}


static bool can_extract( int index, char_data* ch )
{
  if( has_reset( index ) ) {
    send( ch, "That table still has resets and cannot be extracted.\n\r" );
    return false;
  }

  return true;
}


static void renumber_rtable( char_data* ch, int i, int j )
{
  if( --j < 0 || j >= num_rtable ) {
    send( ch, "You can only move a rtable to a sensible position.\n\r" );
    return;
  }
  
  if( i == j ) {
    send( ch, "Moving a rtable to where it already is does nothing\
 interesting.\n\r" );
    return;
  }
  
  rtable_data *rtable  = rtable_list[i];
  remove( rtable_list, num_rtable, i );
  insert( rtable_list, num_rtable, rtable, j );
  
  send( ch, "Rtable \"%s\" moved to position %d.\n\r",
	rtable->name, j+1 );
  
  /* RENUMBER RESETS */

  for( area_data *area = area_list; area; area = area->next )
    for( room_data *room = area->room_first; room; room = room->next )
      for( reset_data *reset = room->reset; reset; reset = reset->next )
        if( !is_set( reset->flags, RSFLAG_OBJECT )
	    && !is_set( reset->flags, RSFLAG_MOB )
	    && renumber( reset->vnum, i, j ) )
          area->modified = true;

  for( int k = 1; k <= species_max; ++k )
    if( species_data *species = species_list[k] )
      for( reset_data *reset = species->reset; reset; reset = reset->next )
        if( !is_set( reset->flags, RSFLAG_OBJECT )
          && !is_set( reset->flags, RSFLAG_MOB ) )
          renumber( reset->vnum, i, j );
}


static bool edit_rtables( char_data *ch )
{
  if( has_permission( ch, PERM_RTABLES ) )
    return true;

  send( ch, "You don't have permission to edit rtables.\n\r" );

  return false;
}



/*
 *  EDITING ROUTINE
 */


void do_rtable( char_data* ch, const char *argument )
{
  wizard_data *imm;
  if( !( imm = wizard( ch ) ) )
    return;

  rtable_data*   rtable;
  int                 i  = imm->rtable_edit;
  int                 j;
  bool             flag;

  if( matches( argument, "exit" )
      || !strcmp( argument, ".." ) ) {
    if( i == - 1 )
      send( ch, "You already aren't editing a rtable.\n\r" );
    else {
      send( ch, "You stop editing rtable \"%s\".\n\r",
	    rtable_list[i]->name );
      imm->rtable_edit = -1;
    }
    return;
  }
  
  if( i != -1 ) {
    if( *argument
	&& !edit_rtables( ch ) )
      return;
    edit_reset( ch, argument, 
		i, rtable_list[i]->reset, RST_TABLE,
		rtable_list[i]->name );
    return;
  }
  
  if( !*argument ) {
    page_title( ch, "Reset Tables" );
    for( i = 0; i < num_rtable; ++i ) 
      page( ch, "[%2i] %s\n\r", i+1, rtable_list[i]->name );
    return;
  }
  
  if( exact_match( argument, "new" ) ) { 
    if( !edit_rtables( ch ) )
      return;
    if( !*argument ) {
      send( ch, "Name of new table?\n\r" );
      return;
    }
    rtable = new rtable_data;
    rtable->name  = alloc_string( argument, MEM_RESET );
    insert( rtable_list, num_rtable, rtable, num_rtable );
    send( ch, "Rtable \"%s\" created.\n\r", argument );
    return;
  }
  
  flag = matches( argument, "delete" );

  if( number_arg( argument, i ) ) {
    if( --i < 0 || i >= num_rtable ) { 
      send( ch, "No rtable exists with that index.\n\r" );
      return;
    }
    rtable = rtable_list[i];
    if( flag ) {
      if( !edit_rtables( ch ) )
	return;
      if( can_extract( i, ch ) ) {
        send( ch, "Rtable %d, %s removed.\n\r", i+1, rtable->name );
        remove( rtable_list, num_rtable, i );
        delete rtable;
      }
      return;
    } 
    if( !*argument ) {
      imm->rtable_edit = i;
      send( ch, "You are now editing rtable %d, \"%s\".\n\r",
	    i+1, rtable->name );
    } else if( number_arg( argument, j ) ) {
      if( !edit_rtables( ch ) )
	return;
      renumber_rtable( ch, i, j );
    } else {
      if( !edit_rtables( ch ) )
	return;
      send( ch, "Rtable %d, \"%s\" renamed \"%s\".\n\r", 
	    i+1, rtable->name, argument );
      free_string( rtable->name, MEM_RESET );
      rtable->name = alloc_string( argument, MEM_RESET );
    }
    return;
  }

  send( ch, "Illegal syntax - See help rtable.\n\r" );
}


/*
 *   DISK ROUTINES
 */


void load_rtables( )
{
  echo( "Loading Reset Tables...\n\r" );
 
  FILE *fp = open_file( FILES_DIR, RTABLE_FILE, "r" );

  num_rtable = fread_number( fp );
  rtable_list = new rtable_data*[num_rtable];

  for( int i = 0; i < num_rtable; i++ ) {
    rtable_data *rtable = new rtable_data;
    rtable->name   = fread_string( fp, MEM_RESET );
    rtable_list[i] = rtable;
    int j;
    while ( ( j = fread_number( fp ) ) != -1 ) {
      reset_data *reset = new reset_data(j);
      reset->flags   = fread_number( fp );
      reset->chances = fread_number( fp );
      reset->value   = fread_number( fp );
      append( rtable->reset, reset );
    }
  }

  fclose( fp );
}


void save_rtables( )
{
  rename_file( FILES_DIR, RTABLE_FILE,
	       FILES_PREV_DIR, RTABLE_FILE );
  
  FILE *fp;

  if( !( fp = open_file( FILES_DIR, RTABLE_FILE, "w" ) ) ) 
    return;

  fprintf( fp, "%d\n\n", num_rtable );

  for( int i = 0; i < num_rtable; i++ ) {
    fwrite_string( fp, rtable_list[i]->name );
    for( reset_data *reset = rtable_list[i]->reset; reset; reset = reset->next ) {
      fprintf( fp, "%d %d %d %d\n", reset->vnum, reset->flags,
	       reset->chances, reset->value );
    }
    fprintf( fp, "-1\n\n" );
  }
  
  fclose( fp );
}


/*
 *   RTWHERE
 */


void do_rtwhere( char_data* ch, const char *argument )
{
  if( !*argument ) {
    send( ch, "Syntax: rtwhere <table>\n\r" );
    return;
  }

  reset_data*         reset;
  species_data*     species;
  int                     i;

  int index = atoi( argument )-1;
  bool found = false;

  if( index < 0 || index >= num_rtable ) {
    send( ch, "That isn't an acceptable rtable index.\n\r" );
    return;
  }

  /* SEARCH MRESETS */

  for( i = 1; i <= species_max; ++i ) {
    if( !( species = species_list[i] ) ) 
      continue;
    for( reset = species->reset; reset; reset = reset->next ) {
      if( index == reset->vnum 
	  && !is_set( reset->flags, RSFLAG_OBJECT )
	  && !is_set( reset->flags, RSFLAG_OBJECT ) ) {
        page( ch, "  Mreset on %s [%d]\n\r", species->Name(), i );
        found = true;
      }
    }
  }

  /* SEARCH ROOM RESETS */

  for( area_data *area = area_list; area; area = area->next ) {
    for( room_data *room = area->room_first; room; room = room->next ) {
      species =  0;
      for( reset = room->reset; reset; reset = reset->next ) {
        if( is_set( reset->flags, RSFLAG_MOB ) ) {
          species = get_species( reset->vnum );
          continue;
          }
        if( index != reset->vnum 
          || is_set( reset->flags, RSFLAG_OBJECT ) )
          continue;
        found = true;
        if( reset->value == -2 ) {
          page( ch, "  Reset at %s [%d]\n\r", room->name, room->vnum );
	} else if( !species ) {
          page( ch, "  [BUG] Illegal reset structure [%d]\n\r",
		room->vnum );
	} else {
          page( ch, "  Reset on %s [%d] at %s [%d]\n\r",
		species->Name(), species->vnum, room->name, room->vnum );
	}
      }
    }
  }
  
  /* SEARCH TABLES */

  for( i = 0; i < num_rtable; i++ ) 
    for( reset = rtable_list[i]->reset; reset; reset = reset->next )
      if( reset->vnum == index 
	  && !is_set( reset->flags, RSFLAG_OBJECT ) ) {
        page( ch, "  In rtable #%d, %s.\n\r", 
	      i+1, rtable_list[i]->name );
        found = true;
      }
  
  if( !found )
    page( ch, "  no resets found\n\r" ); 
}
