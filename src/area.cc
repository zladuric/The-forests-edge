#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "define.h"
#include "struct.h"


area_data *area_list = 0;
static unsigned area_count = 0;


const char*  area_status [ MAX_AREA_STATUS ] = {
  "open", "worthless", "abandoned", "progressing", "pending",
  "blank", "immortal", "deleted" };


/*
 *   AREA_DATA CLASS
 */


area_data :: area_data( const char *filename )
  : next(0), room_first(0),
    creator(empty_string), name(empty_string), help(empty_string),
    modified(false), used(false),
    loaded(false), dirty(false), seen(false),
    act_loaded(false), act_dirty(false), act_used(false),
    nplayer(0), level(1),
    reset_time(0), age(15), status(AREA_BLANK),
    climate(CLIMATE_NONE), forage(0)
{
  record_new( sizeof( area_data ), MEM_AREA );

  file = alloc_string( filename, MEM_AREA );
  
  ++area_count;
}


area_data::~area_data( )
{
  free_string( file, MEM_AREA );
  free_string( creator, MEM_AREA );
  free_string( name, MEM_AREA );
  free_string( help, MEM_AREA );

  --area_count;
}


void area_data::update_forage( )
{
  forage = min( forage + 1, climate_table[ climate ].forage );
}


bool area_data :: save_text( )
{
  if( !loaded ) {
    bug( "area_data::Save_Text: tried to save text while area %s swapped out.",
	 file );
    return false;
  }

  FILE *fp = open_file( ROOM_SWAP_DIR, file, "w" );

  if( !fp )
    return false;

  fprintf( fp, "#ROOM_SWAP\n" );

  for( room_data *room = room_first; room; room = room->next ) {
    fprintf( fp, "#%d\n", room->vnum );
    fwrite_string( fp, room->description );
    fwrite_string( fp, room->comments ); 
    write_extras( fp, room->extra_descr );
  }

  fprintf( fp, "#0\n" );
  fclose( fp );

  dirty = false;

  return true;
}


void area_data :: clear_text( )
{
  if( !loaded ) {
    bug( "area_data::Clear_Text: tried to clear text while area %s swapped out.",
	 file );
    return;
  }

  if( dirty ) {
    bug( "area_data::Clear_Text: tried to clear modified text in area %s.",
	 file );
    return;
  }

  for( room_data *room = room_first; room; room = room->next ) {
    free_string( room->description, MEM_ROOM );
    room->description = 0;
    free_string( room->comments, MEM_ROOM );
    room->comments = 0;
    room->extra_descr.delete_list( );
  }

  for( int i = 0; i < player_list; ++i ) {
    wizard_data *pl = wizard( player_list[i] );
    if( !pl
	|| !pl->in_room
	|| !pl->Is_Valid( )
	|| !pl->room_edit )
      continue;
    if( pl->in_room->area == this ) {
      send( pl, "** The rdesc you were editing has been swapped to disk. **\n\r" );
      pl->room_edit = 0;
    }
  }

  loaded = false;
}


bool area_data :: load_text( )
{
  if( loaded ) {
    bug( "area_data::Load_Text: tried to load text while area %s in memory.",
	 file );
    return false;
  }

  if( dirty ) {
    bug( "area_data::Load_Text: tried to load text after area %s modified.",
	 file );
    return false;
  }

  FILE *fp = open_file( ROOM_SWAP_DIR, file, "r", true );
  
  if( strcmp( fread_word( fp ), "#ROOM_SWAP" ) ) 
    panic( "area_data::Load_Text: missing header" );

  for( room_data *room = room_first; room; room = room->next ) {

    char letter = fread_letter( fp );

    if( letter != '#' ) {
      fclose( fp );
      panic( "area_data::Load_Text: # not found." );
    }

    int vnum = fread_number( fp );

    if( vnum != room->vnum ) {
      fclose( fp );
      panic( "area_data::Load_Text: bad room number." );
    }

    free_string( room->description, MEM_ROOM );
    room->description = fread_string( fp, MEM_ROOM );
    free_string( room->comments, MEM_ROOM );
    room->comments = fread_string( fp, MEM_ROOM );
    read_extras( fp, room->extra_descr );
  }

  char letter = fread_letter( fp );
  
  if( letter != '#' ) {
    fclose( fp );
    panic( "area_data::Load_Text: terminator not found." );
  }
  
  int vnum = fread_number( fp );
  
  if( vnum != 0 ) {
    fclose( fp );
    panic( "area_data::Load_Text: terminator not found." );
  }

  fclose( fp );
  
  loaded = true;

  return true;
}


bool area_data :: save_actions( )
{
  if( !act_loaded ) {
    bug( "area_data::Save_Actions: tried to save actions while area %s swapped out.",
	 file );
    return false;
  }

  FILE *fp = open_file( ACTION_SWAP_DIR, file, "w" );

  if( !fp )
    return false;

  fprintf( fp, "#ACTION_SWAP\n" );

  for( room_data *room = room_first; room; room = room->next ) {
    fprintf( fp, "#%d\n", room->vnum );
    for( action_data *action = room->action; action; action = action->next ) {
      fwrite_string( fp, action->program_data::Code( ) );
      write_extras( fp, action->program_data::Extra_Descr( ) );
    }
  }

  fprintf( fp, "#0\n" );
  fclose( fp );

  act_dirty = false;

  return true;
}


void area_data :: clear_actions( )
{
  if( !act_loaded ) {
    bug( "area_data::Clear_Actions: tried to clear actions while area %s swapped out.",
	 file );
    return;
  }

  if( act_dirty ) {
    bug( "area_data::Clear_Actions: tried to clear modified actions in area %s.",
	 file );
    return;
  }

  for( room_data *room = room_first; room; room = room->next ) {
    for( action_data *action = room->action; action; action = action->next ) {
      action->program_data::Set_Code( 0 );
      //      free_string( action->code, MEM_CODE );
      //      action->code = 0;
      if( !action->binary ) {
	action->program_data::Extra_Descr( ).delete_list( );
      }
    }
  }

  for( int i = 0; i < player_list; ++i ) {
    wizard_data *pl = wizard( player_list[i] );
    if( !pl
	|| !pl->in_room
	|| !pl->Is_Valid( )
	|| !pl->action_edit
	|| !pl->adata_edit )
      continue;
    if( pl->action_edit->room->area == this
	&& !pl->action_edit->binary ) {
      send( pl, "** The adata you were editing has been swapped to disk. **\n\r" );
      pl->adata_edit = 0;
    }
  }

  act_loaded = false;
}


bool area_data :: load_actions( )
{
  if( act_loaded ) {
    bug( "area_data::Load_Actions: tried to load actions while area %s in memory.",
	 file );
    return false;
  }

  if( act_dirty ) {
    bug( "area_data::Load_Actions: tried to load actions after area %s modified.",
	 file );
    return false;
  }

  FILE *fp = open_file( ACTION_SWAP_DIR, file, "r", true );
  
  if( strcmp( fread_word( fp ), "#ACTION_SWAP" ) ) 
    panic( "area_data::Load_Actions: missing header" );

  for( room_data *room = room_first; room; room = room->next ) {

    char letter = fread_letter( fp );

    if( letter != '#' ) {
      fclose( fp );
      panic( "area_data::Load_Actions: # not found." );
    }

    int vnum = fread_number( fp );

    if( vnum != room->vnum ) {
      fclose( fp );
      panic( "area_data::Load_Actions: bad room number." );
    }

    for( action_data *action = room->action; action; action = action->next ) {
      action->program_data::Read_Code( fp );
      //      free_string( action->code, MEM_CODE );
      //      action->code = fread_string( fp, MEM_CODE );
      if( !action->binary ) {
	read_extras( fp, action->program_data::Extra_Descr( ) );
      } else {
	extra_array tmp;
	read_extras( fp, tmp );
	tmp.delete_list( );
      }
    }
  }

  char letter = fread_letter( fp );
  
  if( letter != '#' ) {
    fclose( fp );
    panic( "area_data::Load_Actions: terminator not found." );
  }
  
  int vnum = fread_number( fp );
  
  if( vnum != 0 ) {
    fclose( fp );
    panic( "area_data::Load_Actions: terminator not found." );
  }

  fclose( fp );
  
  act_loaded = true;

  return true;
}


/*
 *   DISK ROUTINES
 */


static bool is_valid_area_filename( char_data *ch, const char *name )
{
  if( !name || !isalpha( *name ) ) {
    send( ch, "An area file name must begin with a letter.\n\r" );
    return false;
  }

  unsigned i = 0;
  while( char c = *name++ ) {
    if( !isalnum( c ) && c != '_' ) {
      send( ch, "An area file name may contain only letters, numbers, and underscores.\n\r" );
      return false;
    }
    ++i;
  }

  if ( i < 3 || i > 14 ) {
    send( ch, "An area file name must be between 3 and 14 letters.\n\r" );
    return false;
  }

  return true;
}


static bool write_area_list( )
{
  rename_file( AREA_DIR, AREA_LIST,
	       AREA_PREV_DIR, AREA_LIST );
  
  FILE *fp;

  if( !( fp = open_file( AREA_DIR, AREA_LIST, "w" ) ) ) {
    bug( "Unable to open area list for write." );
    return false;
  }
  
  for( const area_data *area = area_list; area; area = area->next ) {
    if( area->status != AREA_DELETED ) {
      fprintf( fp, "%s\n", area->file );
    }
  }

  fprintf( fp, "$\n\n" );
  fclose( fp );

  return true;
}


bool set_area_file( char_data* ch, const char *arg, const char *& name )
{
  area_data *area = ch->in_room->area;

  if( name != area->file ) {
    bug( "Set_Area_File: bad filename pointer" );
    return false;
  }

  if( !is_valid_area_filename( ch, arg ) ) {
    return false;
  }

  for( const area_data *area2 = area_list; area2; area2 = area2->next ) 
    if( !strcasecmp( area2->file, arg ) ) {
      send( ch, "There is already an area with that filename.\n\r" );
      return false;
    }
  
  if( !area->loaded ) {
    area->load_text( );
  }
  if( !area->act_loaded ) {
    area->load_actions( );
  }

  const char *old_name = area->file;
  area->file = alloc_string( arg, MEM_AREA );

  if( !write_area_list( ) ) {
    send( ch, "Area rename failed.\n\r" );
    free_string( area->file, MEM_AREA );
    area->file = old_name;
    return false;
  }

  free_string( old_name, MEM_AREA );

  area->modified = true;

  area->Save( );
  area->save_text( );
  area->save_actions( );

  return true;
}


area_data *load_area( const char* file )
{
  char *tmp = static_string( );

  snprintf( tmp, THREE_LINES, "%s.are", file );
  FILE *fp = open_file( AREA_DIR, tmp, "r", true );

  area_data *area = new area_data( file );

  if( strcmp( fread_word( fp ), "#AREA" ) ) 
    panic( "Load_area: missing header" );

  area->name      = fread_string( fp, MEM_AREA );
  area->creator   = fread_string( fp, MEM_AREA );
  area->help      = fread_string( fp, MEM_AREA );
 
  area->level       = fread_number( fp );
  area->reset_time  = fread_number( fp );
  area->status      = fread_number( fp );
  area->climate     = fread_number( fp );

  append( area_list, area );
  area->load_rooms( fp );

  fclose( fp );

  area->loaded = true;
  area->act_loaded = true;

  return area;
}


bool area_data:: Save( bool forced )
{
  if( !forced && !modified )
    return false;

  bool temp_load = !loaded;

  if( temp_load ) {
    load_text( );
  }

  bool act_load = !act_loaded;

  if( act_load ) {
    load_actions( );
  }

  char *tmp = static_string( );            

  sprintf( tmp, "%s.are", file );

  rename_file( AREA_DIR, tmp,
	       AREA_PREV_DIR, tmp );

  FILE *fp;

  if( !( fp = open_file( AREA_DIR, tmp, "w" ) ) ) {
    if( temp_load ) {
      clear_text( );
    }
    if( act_load ) {
      clear_actions( );
    }
    return false;
  }

  fprintf( fp, "#AREA\n" );
  fwrite_string( fp, name );
  fwrite_string( fp, creator );
  fwrite_string( fp, help );
  fprintf( fp, "%d %d\n", level, reset_time );
  fprintf( fp, "%d\n", status );
  fprintf( fp, "%d\n", climate );
  fprintf( fp, "#ROOMS\n\n" );
  
  for( room_data *room = room_first; room; room = room->next ) {
    fprintf( fp, "#%d\n", room->vnum );
    fwrite_string( fp, room->name );
    fwrite_string( fp, room->description );
    fwrite_string( fp, room->comments ); 
    //    fprintf( fp, "%d %d\n",
    //	     room->room_flags, room->temp_flags );
    fprintf( fp, "%d %d\n",
    	     room->room_flags[0], room->room_flags[1] );
    fprintf( fp, "%d %d 0\n",
	     room->sector_type,
	     room->size );
    
    for( int i = 0; i < room->exits; i++ ) {
      exit_data *exit = (exit_data *) room->exits[i];
      fprintf( fp, "D%d\n", (int)exit->direction );
      fwrite_string( fp, exit->name );
      fwrite_string( fp, exit->keywords );
      fprintf( fp, "%d %d %d %d %d\n",
	       exit->exit_info, exit->key, exit->to_room->vnum,
	       (int)exit->light, (int)exit->size );
      }

    write_extras( fp, room->extra_descr );
    room->write_actions( fp );

    write( fp, room->reset );
 
    fprintf( fp, "S\n\n" );
  }

  fprintf( fp, "#0\n\n" );
  fclose( fp );

  if( temp_load ) {
    clear_text( );
  }

  if( act_load ) {
    clear_actions( );
  }

  modified = false;

  return true;
}


/*
 *   AREA LIST COMMAND
 */


void room_range( area_data* area, int& low, int& high, int &nRooms )
{
  low    = MAX_ROOM;
  high   = 0;
  nRooms = 0;

  for( room_data *room = area->room_first; room; room = room->next ) {
    ++nRooms;
    low  = min( low, room->vnum );
    high = max( high, room->vnum );
  }
}


/*
 *   AREA/ROOM SUMMARY COMMANDS
 */


void do_areas( char_data* ch, const char *argument )
{
  if( !ch->link )
    return;

  int flags;
  if( !get_flags( ch, argument, &flags, "al", "areas" ) )
    return;

  bool all = is_apprentice( ch ) && is_set( flags, 0 );
  bool level = is_set( flags, 1 );

  int start = 0;
  int stop = MAX_LEVEL;
  if( number_arg( argument, start ) ) {
    level = true;
    if( number_arg( argument, stop ) ) {
      if( start < 0 || start > stop ) {
	send( ch, "Bad area level range.\n\r" );
	return;
      }
    }
  }

  char         tmp  [ TWO_LINES ];
  area_data*  area;
  int          min;
  int       nRooms;
  int            i;
  int       status;
  int       length  = strlen( argument );	// Must be after flags check.

  area_data *areas[ area_count ];
  const char *area_names[ area_count ];
  int area_levels[ area_count ];
  int count = 0;
  for( area = area_list; area; area = area->next ) {
    if( ( all || area->help != empty_string )
	&& area->status == AREA_OPEN
	&& ( !level || area->level >= start && area->level <= stop ) ) {
      areas[count] = area;
      if( !strncasecmp( area->name, "the ", 4 ) ) {
	area_names[count] = area->name + 4;
      } else if( !strncasecmp( area->name, "an ", 3 ) ) {
	area_names[count] = area->name + 3;
      } else if( !strncasecmp( area->name, "a ", 2 ) ) {
	area_names[count] = area->name + 2;
      } else {
	area_names[count] = area->name;
      }
      area_levels[count] = area->level;
      ++count;
    }
  }

  if( count == 0 ) {
    send( ch, "No areas found.\n\r" );
    return;
  }

  int *sorted;

  // Sort areas by name.
  int sorted1[ count ];
  int max = sort_names( &area_names[0], &area_names[1], sorted1, count, true );

  if( level ) {
    // Sort areas by level.
    // This will maintain name alphabetization within each level group.
    int levels[ max ];
    for( i = 0; i < max; ++i ) {
      levels[i] = area_levels[sorted1[i]];
    }
    int sorted2[ max ];
    sort_ints( &levels[0], &levels[1], sorted2, max );
    for( int i = 0; i < max; ++i ) {
      area_levels[i] = sorted1[sorted2[i]];
    }
    sorted = area_levels;
  } else {
    sorted = sorted1;
  }

  if( !*argument ) {
    page_title( ch, "Areas" );
    if( level ) {
      page_underlined( ch, "Name                           Level   Creator\n\r" );
    }
    int j = 0;
    bool found = false;
    for( i = 0; i < max; ++i ) {
      area = areas[ sorted[i] ];
      if( area->help != empty_string ) {
	if( level ) {
	  page( ch, "%-30s %5d   %s\n\r",
		trunc( area->name, 30 ), area->level, trunc( area->creator, 40 ) );
	} else {
	  page( ch, "%26s%s", area->name, (j++)%3 != 2 ? "" : "\n\r" );
	}
      } else {
	if( level ) {
	  page_color( ch, COLOR_MILD, "%-30s %5d   %s\n\r",
		      trunc( area->name, 30 ),
		      area->level,
		      trunc( area->creator, 40 ) );
	} else {
	  page_color( ch, COLOR_MILD, "%26s%s", area->name, (j++)%3 != 2 ? "" : "\n\r" );
	}
	found = true;
      }
    }
    if( j%3 != 0 )
      page( ch, "\n\r" );
    page( ch, "\n\r" );
    if( found ) {
      page_centered( ch, "[ Highlighted areas are missing help files and are not visible to players. ]" );
      page( ch, "\n\r" );
    }
    page_centered( ch, "[ Type areas <name> to see more information. ]" );
    return;
  }

  if( is_architect( ch ) ) {
    if( !strncasecmp( argument, "new ", 4 ) ) {
      argument += 4;

      if( word_count( argument ) != 2 ) {
	send( ch, "Usage: areas new <area filename> <first room vnum>\n\r" );
	return;
      }

      char arg [MAX_INPUT_LENGTH];
      argument = one_argument( argument, arg );

      if( !is_valid_area_filename( ch, arg ) ) {
	return;
      }

      int vnum;
      if( !is_number( argument )
	  || ( vnum = atoi( argument ) ) < 1 || vnum > MAX_ROOM ) {
	send( ch, "\"%s\" is not a valid room vnum.\n\r", argument );
	return;
      }

      area_data *prev = 0;
      for( area = area_list; area; area = area->next ) {
	if( !strcasecmp( arg, area->file ) ) {
	  send( ch, "Area filename \"%s\" already exists.\n\r", area->file );
	  return;
	}
	
	for( room_data *room = area->room_first; room; room = room->next ) {
	  if( vnum == room->vnum ) {
	    send( ch, "Room %d already exists.\n\r", vnum );
	    return;
	  }
	}

	if( area->room_first && area->room_first->vnum < vnum ) {
	  prev = area;
	}
      }

      area = new area_data( arg );

      area->loaded = true;
      area->act_loaded = true;

      area->room_first = new room_data( area, vnum );

      area->room_first->Set_Description( ch, "Under Construction.\n\r" );
      area->room_first->name = alloc_string( "Under Construction", MEM_ROOM );

      set_bit( area->room_first->room_flags, RFLAG_NO_PKILL );

      if( !prev ) {
	area_list = area;
      } else {
	area->next = prev->next;
	prev->next = area;
      }

      char buf [ TWO_LINES ];
      snprintf( buf, TWO_LINES, "Room.%d", vnum );
      delete_file( ROOM_DIR, buf, false );
      delete_file( ROOM_PREV_DIR, buf, false );

      write_area_list();
      area->Save( );
      area->save_text( );
      area->save_actions( );

      send( ch, "New area and room %d created.\n\r", vnum );
      return;
    }

    if( !strcasecmp( argument, "delete" ) ) {
      if( !ch->in_room )
	return;

      if( ch->in_room->area->status == AREA_DELETED ) {
	fsend( ch, "Area \"%s\" status set to immortal.",
	       ch->in_room->area->name );
	ch->in_room->area->status = AREA_IMMORTAL;
	ch->in_room->area->modified = true;
	ch->in_room->area->Save( );
	write_area_list();
	return;
      }

      for( area = area_list; area; area = area->next ) {
	if( area == ch->in_room->area )
	  continue;

	for( room_data *room = area->room_first; room; room = room->next ) {
	  for( i = 0; i < room->exits; ++i ) {
	    exit_data *exit = room->exits[i];
	    if( exit->to_room->area == ch->in_room->area ) {
	      fsend( ch, "Room %d connects to area \"%s\" - delete failed.",
		     room->vnum, ch->in_room->area->name );
	      return;
	    }
	  }
	}
      }

      fsend( ch, "Area \"%s\" status set to deleted.",
	     ch->in_room->area->name );
      ch->in_room->area->status = AREA_DELETED;
      ch->in_room->area->modified = true;
      ch->in_room->area->Save( );
      write_area_list();
      return;
    }
  }

  if( is_apprentice( ch ) ) {
    status = -1;
    if( strncasecmp( argument, "summary", length ) )
      for( status = 0; status < MAX_AREA_STATUS; ++status )
        if( !strncasecmp( argument, area_status[status], length ) )
          break;
    
    if( status != MAX_AREA_STATUS ) {
      snprintf( tmp, TWO_LINES, "%25s   %11s   %5s  %s\n\r", "Area Name",
		"Vnum Range", "#Plyr", "Creator" );
      page_underlined( ch, tmp );
      
      unsigned total = 0;
      unsigned count = 0;

      for( i = 0; i < MAX_AREA_STATUS; ++i ) {
        if( status != -1 && i != status )
          continue;
        *tmp = '\0';
        for( area = area_list; area; area = area->next ) {
	  if( status != -1 || i == 0 ) {
	    ++total;
	  }
          if( area->status != i )
            continue;
	  ++count;
          if( !*tmp ) {
            page( ch, "\n\r" );
            snprintf( tmp, TWO_LINES, "--- %s ---", area_status[i] );
            tmp[4] = toupper( tmp[4] );
            page_centered( ch, tmp ); 
            page( ch, "\n\r" );
	  }
          room_range( area, min, max, nRooms );
          page( ch, "%25s  %6d-%-6d  %3d    %s\n\r",
		area->name, min, max, area->nplayer, area->creator );
	}
      }
      page( ch, "\n\r" );
      page( ch, "%5d total areas.\n\r", total );
      if( count != total )
	page( ch, "%5d listed.\n\r", count );
      return;
    }
  }
  
  for( i = 0; i < max; ++i ) {
    // Match area name.
    area = areas[ sorted[i] ];
    const char *name = area_names[ sorted[i] ];
    if( !strncasecmp( name, argument, length )
	|| name != area->name && !strncasecmp( area->name, argument, length ) ) {
      page( ch, "         Name: %s\n\r", area->name );
      page( ch, "      Creator: %s\n\r", area->creator );
      
      if( is_builder( ch ) ) {
	room_range( area, min, max, nRooms );
	page( ch, "        Rooms: %d\n\r", nRooms );
	page( ch, "   Vnum Range: %d to %d\n\r", min, max );
      }
      
      page( ch, "Approx. Level: %d\n\r\n\r", area->level );
      
      if( area->help != empty_string ) {
	help_link( ch->link, area->help );
      }
      return;
    }
  }

  send( ch, "No area matching that name found.\n\r" );
}


void do_roomlist( char_data* ch, const char *argument )
{
  room_data *room;
  if( !( room = Room( ch->array->where ) ) ) {
    send( ch, "You aren't in a room.\n\r" );
    return;
  }

  int flags;
  if( !get_flags( ch, argument, &flags, "cCmMsS", "roomlist" ) )
    return;

  in_character = false;

  bool noncomments = is_set( flags, 1 );
  bool comments = is_set( flags, 0 ) || noncomments;
  bool nonmobs = is_set( flags, 3 );
  bool mobs = is_set( flags, 2 ) || nonmobs;
  bool nonspelling = is_set( flags, 5 );
  bool spelling = is_set( flags, 4 ) || nonspelling;
  
  page_underlined( ch, "Vnum     Name of Room\n\r" );

  for( room_data *room2 = room->area->room_first; room2; room2 = room2->next ) {
    bool found = false;
    bool stuff = false;

    if( !noncomments && !nonmobs && !nonspelling
	|| *room2->Comments( ) && comments ) {
      found = true;
      page( ch, "%-6d   ", room2->vnum );
      page_color( ch, COLOR_ROOM_NAME, "%s\n\r", room2->name );
      if( comments ) {
	if( *room2->Comments( ) ) {
	  stuff = true;
	  page( ch, room2->Comments( ) ); 
	}
      }
    }

    if( mobs ) {
      select( room2->contents, ch );
      for( int i = 0; i < room2->contents; ++i ) {
	if( !character( room2->contents[i] )
	    || !room2->contents[i]->Seen( ch ) ) {
	  room2->contents[i]->Select( 0 ); 
	}
      }
      rehash( ch, room2->contents, true );
      for( int i = 0; i < room2->contents; ++i ) {
	char_data *rch = character( room2->contents[i] );
	if( rch && rch->Shown( ) > 0 ) {
	  if( !found ) {
	    page( ch, "%-6d   ", room2->vnum );
	    page_color( ch, COLOR_ROOM_NAME, "%s\n\r", room2->name );
	    found = true;
	  } else if( comments && *room2->Comments( ) ) {
	    page( ch, "\n\r" );
	  }
	  stuff = true;
	  if( player( rch ) ) {
	    page( ch, "  -> player %s\n\r",
		  rch->descr->name );
	  } else if( rch->Shown( ) > 1 ) {
	    page( ch, "  -> #%d, %s (x%d)\n\r",
		  rch->species->vnum,
		  rch->Seen_Name( ch, 1, false ),
		  rch->Shown( ) );
	  } else {
	    page( ch, "  -> #%d, %s\n\r",
		  rch->species->vnum,
		  rch->Seen_Name( ch, 1, false ) );
	  }
	}
      }
    }

    if( spelling ) {
      if( const char *s = spell( ch, room2->name, "Name: " ) ) {
	if( !found ) {
	  found = true;
	  page( ch, "%-6d   ", room2->vnum );
	  page_color( ch, COLOR_ROOM_NAME, "%s\n\r", room2->name );
	}
	stuff = true;
	page( ch, s );
      }
      if( const char *s = spell( ch, room2->Description( ) ) ) {
	if( !found ) {
	  found = true;
	  page( ch, "%-6d   ", room2->vnum );
	  page_color( ch, COLOR_ROOM_NAME, "%s\n\r", room2->name );
	}
	stuff = true;
	page( ch, "Description:\n\r" );
	page( ch, s );
      }
      extra_array& extras = room2->Extra_Descr( );
      for( int i = 0; i < extras; ++i ) {
	extra_data *extra = extras[i];
	if( const char *s = spell( ch, extra->text ) ) {
	  if( !found ) {
	    found = true;
	    page( ch, "%-6d   ", room2->vnum );
	    page_color( ch, COLOR_ROOM_NAME, "%s\n\r", room2->name );
	  }
	  stuff = true;
	  page( ch, "[%s]\n\r", extra->keyword );
	  page( ch, s );
	}
      }
    }

    if( stuff ) {
      page( ch, "\n\r" );
      //      page( ch, "%37s-----\n\r", "" );
    }
  }
}
