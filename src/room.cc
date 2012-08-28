#include <ctype.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "define.h"
#include "struct.h"


const char *rflag_name[ MAX_RFLAG ] = {
  "Lit", "Safe", "Indoors", "No.Mob",
  "No.Recall", "No.Magic", "No.Autoscan", "Altar", "Altar.Good",
  "Altar.Neutral", "Altar.Evil", "Bank", "Shop", "Pet.Shop", "Office",
  "No.Pray", "Save.Items", "Underground", "Auction.House",
  "Reset0", "Reset1", "Reset2", "Status0", "Status1", "Status2",
  "No.Mount", "Arena", "No.Rest", "No.Pkill",
  "Altar.Law", "Altar.Neutral2", "Altar.Chaos" };

const char *location_name[ MAX_LOCATION ] =
{ "!Indoors", "!Outside",
  "Sunlight", "Full Moon",
  "Forest", "!Underwater",
  "!Mounted", "!Fighting",
  "Underground", "Aboveground",
  "Perform"
};

int max_location = MAX_LOCATION;

flag_data location_flags = {
  "Location", &location_name[0], &location_name[1], &max_location, true };


const char *tflag_name[ MAX_TFLAG ] = {
  "water",
  "deep",
  "flowing",
  "submerged",
  "arid",
  "fall",
  "plant",
  "bury",
  "rough",
  "forest",
  "tracks"
};

int max_tflag = MAX_TFLAG;

flag_data terrain_flags = {
  "Terrain", &tflag_name[0], &tflag_name[1], &max_tflag, true
};


static void dereference( char_data *wch, room_data *room )
{
  for( int i = wch->events.size-1; i >= 0; --i ) {
    event_data *event = wch->events[i];
    if( event->func == execute_path ) {
      path_data *path = (path_data*) event->pointer;
      if( path->goal == room ) {
	extract( event );
	delay_wander( new event_data( execute_wander, wch ) );
      }
    }
  }

  for( int i = 0; i < player_list; ++i ) {
    player_data *pl = player_list[i];
    if( pl->Is_Valid( ) && pl->was_in_room )
      pl->was_in_room = get_temple( pl );
  }
}


Room_Data::Room_Data( area_data *a, int vnum )
  : next(0), area(a), action(0), reset(0), track(0),
    vnum(vnum),// room_flags(0), temp_flags(0),
    sector_type(0), distance(INT_MAX),
    size(SIZE_HORSE),
    name(empty_string), description(empty_string), comments(empty_string)
{
  record_new( sizeof( room_data ), MEM_ROOM );
  valid = ROOM_DATA;

  vzero( room_flags, 2 );
}


Room_Data::~Room_Data( )
{
  record_delete( sizeof( room_data ), MEM_ROOM );

  extract( contents );

  stop_events( this );
  affected.delete_list();
  exits.delete_list( );
  extra_descr.delete_list( );
  delete_list( action );
  delete_list( reset );
  delete_list( track );
  free_string( name, MEM_ROOM );
  free_string( description, MEM_ROOM );
  free_string( comments, MEM_ROOM );

  //  for( int i = 0; i < player_list; ++i ) 
  //    dereference( player_list[i], this );

  for( int i = 0; i < mob_list; i++ ) 
    dereference( mob_list[i], this );
}


const char *Room_Data :: Name( const char_data *ch, int, bool ) const
{
  return Seen( ch ) ? name : "darkness";
}


void Room_Data :: Look_At( char_data *ch )
{
  if( Seen( ch ) ) {
    action_data *act;
    for( act = action; act; act = act->next ) {
      if( act->trigger == TRIGGER_DESCRIBE
	  && !*act->target ) {
	push( );
	clear_variables( );
	var_room = this;
	var_ch = ch;
	var_arg = empty_string;
	const int result = act->execute( );
	pop( );
	if( result ) {
	  act = 0;
	}
	break;
      }
    }

    if( !act ) {
      char tmp [ 3*MAX_STRING_LENGTH ];
      convert_to_ansi( ch, 3*MAX_STRING_LENGTH, Description( ), tmp );
      send( ch, tmp );
    }

  } else {
    send( ch,
	  "The area is very dark and you can make out no details.\n\r" );
  }

  if( is_apprentice( ch ) && *Comments( ) ) {
    send( ch, "%37s-----\n\r", "" );
    send( ch, Comments( ) );
  }
}


const char *Room_Data :: Description( ) const
{
  if( !area->loaded ) {
    area->load_text( );
  }

  area->seen = true;

  return description;
}


void Room_Data :: Set_Description( char_data *ch, const char *text )
{
  if( !area->loaded ) {
    area->load_text( );
  }

  free_string( description, MEM_ROOM );
  description = alloc_string( text, MEM_ROOM );

  area->seen = true;
  area->dirty = true;
  area->modified = true;
}


void Room_Data :: Edit_Description( char_data *ch, const char *text )
{
  if( !area->loaded ) {
    area->load_text( );
  }

  description = edit_string( ch, text, description, MEM_ROOM, true );

  area->seen = true;

  if( *text ) {
    area->dirty = true;
    area->modified = true;
  }
}


const char *Room_Data :: Comments( ) const
{
  if( !area->loaded ) {
    area->load_text( );
  }

  area->seen = true;

  return comments;
}


void Room_Data :: Set_Comments( char_data *ch, const char *text )
{
  if( !area->loaded ) {
    area->load_text( );
  }

  free_string( comments, MEM_ROOM );
  comments = alloc_string( text, MEM_ROOM );

  area->seen = true;
  area->dirty = true;
  area->modified = true;
}


void Room_Data :: Edit_Comments( char_data *ch, const char *text )
{
  if( !area->loaded ) {
    area->load_text( );
  }

  comments = edit_string( ch, text, comments, MEM_ROOM, false );

  area->seen = true;

  if( *text ) {
    area->dirty = true;
    area->modified = true;
  }
}


extra_array& Room_Data :: Extra_Descr( )
{
  if( !area->loaded ) {
    area->load_text( );
  }

  area->seen = true;

  return extra_descr;
}


/*
 *   DISK ROUTINES
 */


void area_data :: load_rooms( FILE* fp )
{
  if( strcmp( fread_word( fp ), "#ROOMS" ) )
    panic( "Load_rooms: missing header" );

  while( true ) {
    int vnum;
    char letter = fread_letter( fp );

    if( letter != '#' ) 
      panic( "Load_rooms: # not found." );

    if( ( vnum = fread_number( fp ) ) == 0 )
       break;

    if( get_room_index( vnum ) ) 
      panic( "Load_rooms: vnum %d duplicated.", vnum );

    room_data *room    = new room_data( this, vnum );
    room->name         = fread_string( fp, MEM_ROOM );
    room->description  = fread_string( fp, MEM_ROOM );
    room->comments     = fread_string( fp, MEM_ROOM );
    //    room->room_flags   = fread_number( fp );
    room->room_flags[0] = fread_number( fp );
    room->room_flags[1] = fread_number( fp );
    room->sector_type  = fread_number( fp );
    room->size         = fread_number( fp );
    room->reset        = 0;

    if( room->size < 0 || room->size >= MAX_SIZE )
      room->size = SIZE_HORSE;

    const int z = fread_number( fp );

    if( z != 0 )
      panic( "Load_rooms: corrupt file." );

    read_exits( fp, room, vnum );
    read_extras( fp, room->extra_descr );

    while ( ( letter = fread_letter( fp ) ) != 'S' ) {
  
      if( isdigit( letter ) || letter == '-' ) {
        ungetc( letter, fp );
        load( fp, room->reset );
        continue;
      }

      if( letter == 'A' ) {
        room->read_action( fp );
        continue;
      }
      
      panic( "Load_rooms: vnum %d has flag not 'DES'.", vnum );
    }
    
    room_data *room2;
    if( !( room2 = room_first ) ) {
      room_first = room;
      room->next = 0;
    } else {
      if( room2->vnum > room->vnum ) {
        room->next = room2;
        room_first = room;
      } else {
        for( ;
	     room2->next && room2->next->vnum < room->vnum;
	     room2 = room2->next );
        room->next = room2->next;
        room2->next = room;
      }
    }
  }
}


/*
 *   SAVE/LOAD ROOM ITEMS
 */


void Room_Data :: Save( bool )
{
  if( !is_set( room_flags, RFLAG_SAVE_ITEMS ) )
    return;

  char *file  = static_string( );

  snprintf( file, THREE_LINES, "Room.%d", vnum );
  rename_file( ROOM_DIR, file, ROOM_PREV_DIR, file );

  FILE *fp = open_file( ROOM_DIR, file, "w" );

  if( !fp )
    return;

  saves.clear( );
  saves.Setup( this );

  write_object( fp, contents, this, true );

  fclose( fp );

  saves.Save( );

  fixed = false;
}


static void load_room_items( room_data* room )
{
  FILE*        fp;
  char*      file  = static_string( );
  char*       tmp  = static_string( );
  bool       flag = false;
           
  sprintf( file, "Room.%d", room->vnum );

  if( ( fp = open_file( ROOM_DIR, file, "r", false ) ) ) {
    flag = read_object( fp, room->contents, room, tmp );
    fclose( fp );
  }

  if( !flag ) {
    roach( "Load_Room_Items: Room %d corrupted.", room->vnum );
    
    if( !( fp = open_file( ROOM_PREV_DIR, file, "r", false ) ) )
      return; 
    
    if( !read_object( fp, room->contents, room, tmp ) ) 
      panic( "Previous file also corrupted!" );            
    
    fclose( fp );
  }

  if( room->fixed ) {
    // Object load triggers require save.
    echo( "Saving updated room %d.\n\r", room->vnum );
    room->Save( );
  }
}


void load_room_items( )
{
  echo( "Loading Room Items ...\n\r" ); 

  for( area_data *area = area_list; area; area = area->next )
    for( room_data *room = area->room_first; room; room = room->next ) 
      if( is_set( room->room_flags, RFLAG_SAVE_ITEMS ) ) 
        load_room_items( room );

  count_objects( );
}


/*
 *   GET_ROOM_INDEX FUNCTION
 */


room_data *get_room_index( int vnum, bool err )
{
  if( vnum > 0
      && vnum <= MAX_ROOM ) {
    
    for( area_data *area = area_list; area; area = area->next ) {
      if( area->next
	  && area->next->room_first
	  && area->next->room_first->vnum <= vnum )
	continue; 
      for( room_data *room = area->room_first; room; room = room->next )
	if( room->vnum == vnum )
	  return room;
      break;
    }
  }

  if( err ) 
    panic( "Get_room_index: bad vnum %d.", vnum );
  
  return 0;
}


/*
 *   SUPPORT ROUTINES
 */


bool midair( const char_data *ch, const room_data *room )
{
  if( !room
      && !( room = ch->in_room ) )
    return false;

  return is_set( terrain_table[ room->sector_type ].flags, TFLAG_FALL );
}


bool arid( const char_data *ch, const room_data *room )
{
  if( !room
      && !( room = ch->in_room ) )
    return false;

  return is_set( terrain_table[ room->sector_type ].flags, TFLAG_ARID );
}


bool forest( const char_data *ch, const room_data *room )
{
  if( !room
      && !( room = ch->in_room ) )
    return false;

  return is_set( terrain_table[ room->sector_type ].flags, TFLAG_FOREST );
}


const char *room_data :: surface ( ) const
{
  return terrain_table[sector_type].surface;
}


const char *room_data :: position ( ) const
{
  return terrain_table[sector_type].position;
}


const char *room_data :: drop ( ) const
{
  return terrain_table[sector_type].drop;
}


bool room_data :: is_indoors( ) const
{
  return is_set( room_flags, RFLAG_INDOORS );
}


bool char_data :: can_edit( room_data *room, bool msg ) const
{
  if( has_permission( this, PERM_ALL_ROOMS )
      || is_name( descr->name, room->area->creator ) )
    return true;

  if( msg )
    send( this, "You don't have permission to alter this room.\n\r" );

  return false;
}


/*
const char* room_name( room_data* room )
{
  if( !room )
    return "nowhere??";

  static char tmp [ 15 ];
  snprintf( tmp, 15, "Room #%d", room->vnum );
  return tmp;
}
*/
  

/*
 *   ONLINE ROOM COMMANDS
 */


void do_rbug( char_data* ch, const char *argument ) 
{
  if( !*argument || ch->can_edit( ch->in_room ) )
    ch->in_room->Edit_Comments( ch, argument );
}


void do_rdesc( char_data *ch, const char *argument ) 
{
  wizard_data*     imm;
  
  if( !( imm = wizard( ch ) ) )
    return;
 
  room_data *room = ch->in_room;

  if( *argument && !ch->can_edit( room ) ) 
    return;

  if( !imm->room_edit ) {
    room->Edit_Description( ch, argument );
  } else {
    area_data *area = room->area;
    if( !area->loaded ) {
      bug( "Rdesc: %s is editing a swapped-out rdesc.", ch );
      bug( "-- Room = %d", room->vnum );
      imm->room_edit = 0;
      return;
    }
    imm->room_edit->text = edit_string( ch, argument,
					imm->room_edit->text, MEM_EXTRA, true );
    if( *argument ) {
      area->dirty = true;
      area->modified = true;
    }
    area->seen = true;
  }
}

    
void do_rflag( char_data* ch, const char *argument )
{
  int flags;
  if( !get_flags( ch, argument, &flags, "a", "Rflag" ) )
    return;;

  room_data*        room  = ch->in_room;
  const char*   response;

  if( !*argument ) {
    display_flags( "Room",
		   &rflag_name[0], &rflag_name[1],
		   room->room_flags, MAX_RFLAG, ch );
    return;
  }
  
  if( !ch->can_edit( room ) )
    return;

  int prev [2];
  vcopy( prev, room->room_flags, 2 );

  if( !( response = set_flags( &rflag_name[0], &rflag_name[1],
			       room->room_flags, 0, MAX_RFLAG, 0,
			       ch, argument, "room",
			       false, true ) ) )
    return;
  
  room_log( ch, ch->in_room->vnum, response );
  room->area->modified = true;

  if( !is_set( flags, 0 ) )
    return;

  for( room = room->area->room_first; room; room = room->next ) {
    alter_flags( room->room_flags, ch->in_room->room_flags,
		 prev, MAX_RFLAG );
  }
  
  send( ch, "- Set on Area -\n\r" );
}


void do_rset( char_data* ch, const char *argument )
{
  room_data*    room  = ch->in_room;
  area_data*    area  = room->area;
  int          flags;

  if( !get_flags( ch, argument, &flags, "a", "Rset" ) )
    return;
  
  if( !*argument ) {
    do_rstat( ch, "" );
    return;
  }

  if( !ch->can_edit( room ) ) 
    return;

  {
    // Room values.

    class string_field string_list [] = {
      { "name",      MEM_ROOM,  &room->name,     0               },
      { "",          0,         0,               0               }
    };
    
    if( const char *response = process( string_list, ch, "room", argument ) ) {
      if( *response ) {
	room_log( ch, room->vnum, response );
	area->modified = true;
      }
      return;
    }

    int terr  = room->sector_type;
    int size  = room->size;
    
#define tn( i )   terrain_table[i].name
#define sn( i )   size_name[i]
    
    class type_field type_list [] = {
      { "terrain", table_max[TABLE_TERRAIN], &tn(0), &tn(1), &room->sector_type, true  },
      { "size",    MAX_SIZE,                 &sn(0), &sn(1), &room->size, false }, 
      { "" }
    };
    
#undef ts
#undef sn
    
    if( const char *response = process( type_list, ch,
					is_set( flags, 0 ) ? area->name : "room",
					argument ) ) {
      if( *response ) {
	area->modified = true;
	if( is_set( flags, 0 ) ) {
	  if( room->size != size ) {
	    room_log( ch, room->vnum, response );
	    size = room->size;
	    for( room = area->room_first; room; room = room->next ) {
	      if( room->size != size ) {
		room_log( ch, room->vnum, response );
		room->size = size;
		for( int i = 0; i < room->exits; ++i ) {
		  exit_data *exit = room->exits[i];
		  exit->size = min( size, exit->to_room->size );
		}
	      }
	    }
	  } else if( room->sector_type != terr ) {
	    room_log( ch, room->vnum, response );
	    terr = room->sector_type;
	    for( room = area->room_first; room; room = room->next ) {
	      if( room->sector_type != terr ) {
		room_log( ch, room->vnum, response );
		room->sector_type = terr;
	      }
	    }
	  }
	} else {
	  room_log( ch, room->vnum, response );
	}
      }
      return;
    }
  }
  
  {
    // Area values.

    class string_field string_list [] = {
      { "area",      MEM_AREA,  &area->name,     0               },
      { "creator",   MEM_AREA,  &area->creator,  0               },
      { "help",      MEM_AREA,  &area->help,     0               },
      { "filename",  MEM_AREA,  &area->file,     &set_area_file  },
      { "",          0,         0,               0               }
    };
    
    if( const char *response = process( string_list, ch, area->name, argument ) ) {
      if( *response ) {
	//	room_log( ch, room->vnum, response );
	area->modified = true;
      }
      return;
    }

    class int_field int_list [] = {
      { "level",            0,     90,  &area->level       },
      { "reset time",       0,    200,  &area->reset_time  },
      { "",                 0,      0,  0                  }, 
    };
    
    if( const char *response = process( int_list, ch, area->name, argument ) ) {
      if( *response ) {
	//	room_log( ch, room->vnum, response );
	area->modified = true;
      }
      return;
    }

#define as( i )   area_status[i]
#define cn( i )   climate_table[i].name
    
    // You can't rset area status to deleted, use 'area delete' command.
    class type_field type_list_x [] = {
      { "status",  MAX_AREA_STATUS-1,         &as(0), &as(1), &area->status, true       },
      { "climate", table_max[TABLE_CLIMATE],  &cn(0), &cn(1), &area->climate, true      },
      { "" }
    };
    
#undef as
#undef cn
    
    // You cant rset area status at all if area has been deleted.
    // Otherwise, room list wouldn't be saved.
    class type_field *type_list = ( area->status == AREA_DELETED )
      ? type_list_x + 1
      : type_list_x;
    
    if( const char *response = process( type_list, ch, area->name, argument ) ) {
      if( *response ) {
	//	room_log( ch, room->vnum, response );
	area->modified = true;
      }
      return;
    }
  }
  
  send( ch, "Syntax: rset <field> ....\n\r" );
}


void do_rstat( char_data* ch, const char *argument )
{
  room_data *room;

  if( *argument ) {
    int vnum;
    if( !number_arg( argument, vnum ) ) {
      send( ch, "Syntax: rstat [<room #>]\n\r" );
      return;
    }
    if( !( room = get_room_index( vnum ) ) ) {
      send( ch, "No such room: %d\n\r", vnum );
      return;
    }
  } else {
    room = ch->in_room;
  }

  char           tmp  [ MAX_STRING_LENGTH ];
  bool         found = false;

  page( ch, "        Name: " );
  page_color( ch, COLOR_ROOM_NAME, "%s\n\r", room->name );
  page( ch, "        Vnum: %d\n\r", room->vnum );
  page( ch, "       Light: %-14d Weight: %.2f\n\r",
	room->Light(), (double)room->contents.weight/100.0 );
  page( ch, " Temperature: %-12.1f Humidity: %.1f\n\r",
	room->temperature( ), room->humidity( ) );
  page( ch, "     Terrain: %-16s Size: %-13s\n\r",
	terrain_table[ room->sector_type ].name, size_name[room->size] );
  
  strcpy( tmp, "       Exits:" );
  for( int i = 0; i < room->exits; i++ ) {
    sprintf( tmp+strlen( tmp ), " %s",
	     dir_table[ room->exits[i]->direction ].name );
    found = true;
  }
  sprintf( tmp+strlen( tmp ), "%s\n\r", found ? "" : " none" );
  page( ch, tmp );
  
  found = false;
  strcpy( tmp, "  Exits From:" );
  for( area_data *area = area_list; area; area = area->next ) {
    for( room_data *room2 = area->room_first; room2; room2 = room2->next ) {
      for( int i = 0; i < room2->exits; ++i ) {
        if( room2->exits[i]->to_room == room ) {
          found = true;
          sprintf( tmp+strlen( tmp ), " %d", room2->vnum );          
	}
      }
    }
  }
  sprintf( tmp+strlen( tmp ), "%s\n\r", found ? "" : " none" );
  page( ch, tmp );

  page( ch, scroll_line[0] );

  const area_data *const area = room->area;

  page( ch, "        Area: %s\n\r", area->name );
  page( ch, "     Creator: %s\n\r", area->creator );
  page( ch, "        Help: %s\n\r", area->help );
  page( ch, "       Level: %-14d Status: %s\n\r",
	area->level, area_status[ area->status ] );
  page( ch, "  Reset Rate: %-12d Filename: %-14s Climate: %s\n\r",
	area->reset_time, area->file, climate_table[area->climate].name );

  page( ch, scroll_line[0] );

  sprintf( tmp, "Description:\n\r%s", room->Description( ) );
  page( ch, tmp );

  show_extras( ch, room->Extra_Descr( ) );
}


bool can_extract( room_data* room, char_data* ch )
{
  if( room->vnum == ROOM_CHAT ) {
    send( ch, "You can't delete this room.\n\r" );
    return false;
  }

  if( room->area->room_first == room ) {
    send( ch, "You can't delete the first room in an area.\n\r" );
    return false;
  }

  if( room->exits > 0 ) {
    send( ch, "You must remove all doors from the room.\n\r" );
    return false;
  }

  if( room->reset ) {
    send( ch, "Remove all resets first.\n\r" );
    return false;
  }	
  
  for( const trainer_data *trainer = trainer_list; trainer; trainer = trainer->next )
    if( trainer->room == room ) {
      send( ch, "Room still contains a trainer reference.\n\r" );
      return false;
    }
  
  for( const shop_data *shop = shop_list; shop; shop = shop->next ) {
    if( shop->room == room ) {
      send( ch, "Room still contains a shop reference.\n\r" );
      return false;
    }
  }

  for( const area_data *area = area_list; area; area = area->next ) 
    for( room = area->room_first; room; room = room->next )
      for( int i = 0; i < room->exits; i++ )  
        if( room->exits[i]->to_room == ch->in_room ) {
          fsend( ch, "Room %d still has a connection to this room.",
		 room->vnum );
          return false;
	}
  
  return true;
}


void do_redit( char_data *ch, const char *argument )
{
  wizard_data *imm;
  
  if( !( imm = wizard( ch ) ) )
    return;

  if( !ch->can_edit( ch->in_room ) ) 
    return;
 
  if( exact_match( argument, "room" ) ) {
    imm->room_edit = 0;
    send( ch, "Rdesc now operates on current room.\n\r" );
    return;
  }

  if( exact_match( argument, "move" ) ) {
    int n;
    if( !number_arg( argument, n ) ) {
      send( ch, "Syntax: redit move <room #>\n\r" );
      return;
    }
    room_data *room = get_room_index( n );
    if( !room ) {
      fsend( ch, "No such room to move: %d.", n );
      return;
    }
    if( room->area == ch->in_room->area ) {
      fsend( ch, "Room %d is already in this area.", n );
      return;
    }
    if( !ch->can_edit( room, false ) ) {
      fsend( ch, "You don't have permission to move room %d.", n );
      return;
    }
    
    if( room->area->room_first == room ) {
      send( ch, "You can't move the first room in an area.\n\r" );
      return;
    }

    int vnum;
    for( vnum = ch->in_room->area->room_first->vnum+1; ; vnum++ ) {
      room_data *room2;
      if( !( room2 = get_room_index( vnum ) ) )
	break;
      if( vnum > MAX_ROOM
	  || room2->area != ch->in_room->area ) {
	send( ch, "Area is out of room numbers.\n\r" );
	return;
      }
    }

    if( !ch->in_room->area->loaded )
      ch->in_room->area->load_text( );

    if( !room->area->loaded )
      room->area->load_text( );

    if( !ch->in_room->area->act_loaded )
      ch->in_room->area->load_actions( );

    if( !room->area->act_loaded )
      room->area->load_actions( );

    ch->in_room->area->dirty = true;
    ch->in_room->area->modified = true;
    ch->in_room->area->act_dirty = true;

    room->area->dirty = true;
    room->area->modified = true;
    room->area->act_dirty = true;
  
    remove( room->area->room_first, room );

    fsend( ch, "Room %d moved to %d.",
	   room->vnum, vnum );

    room->area = ch->in_room->area;
    room->vnum = vnum;

    room_data *prev = get_room_index( vnum-1 );
    room->next = prev->next;
    prev->next = room;

    return;
  }

  if( exact_match( argument, "copy" ) ) {
    int n;
    if( !number_arg( argument, n ) ) {
      send( ch, "Syntax: redit copy <room #>\n\r" );
      return;
    }
    room_data *room = get_room_index( n );
    if( !room ) {
      fsend( ch, "No such room to copy from: %d.", n );
      return;
    }

    if( !ch->in_room->area->loaded )
      ch->in_room->area->load_text( );

    ch->in_room->Set_Description( ch, room->Description( ) );
    send( ch, "Copied room description from room %d.\n\r", n );
    imm->room_edit = 0;
    send( ch, "Rdesc now operates on current room.\n\r" );
    return;
  }

  room_data *room = ch->in_room;
  area_data *area = room->area;

  if( !strcasecmp( argument, "delete room" ) ) {

    if( !can_extract( room, ch ) )
      return;
    
    char               buf   [ TWO_LINES ];
    char               buf1   [ TWO_LINES ];
    char_data*         rch;
    player_data       *plr;

    send( ch, "You delete the room.\n\r" );

    if( !area->loaded )
      area->load_text( );

    if( !area->act_loaded )
      area->load_actions( );

    area->dirty = true;
    area->modified = true;
    area->act_dirty = true;

    for( int i = room->contents.size-1; i >= 0; --i ) {
      if( ( plr = player( room->contents[i] ) ) ) {
        send( plr, "The room you are in disappears.\n\r" );
        send( plr, "You find yourself elsewhere.\n\r\n\r" );
	room_data *to = is_apprentice( plr )
	                ? get_room_index( ROOM_CHAT )
	                : get_temple( plr );
        plr->From( );
        plr->To( to );
	show_room( plr, plr->in_room, false, false );
      } else if( ( rch = character( room->contents[i] ) ) ) {
	if( rch->leader
	    && !rch->leader->species
	    && is_set( rch->status, STAT_PET ) ) {
	  room_data *to = is_apprentice( rch->leader )
	                  ? get_room_index( ROOM_CHAT )
	                  : get_temple( rch->leader );
	  rch->From( );
	  rch->To( to );
	  show_room( rch, rch->in_room, false, false );
	} else {
	  rch->Extract( );
	}
      } else {
	room->contents[i]->Extract( );
      }
    }

    snprintf( buf, TWO_LINES, "Room %d deleted by %s.",
	     room->vnum, ch->real_name( ) );
    snprintf( buf1, TWO_LINES, "Room %d deleted.",
	     room->vnum );
    info( LEVEL_BUILDER, buf1, invis_level( ch ), buf, IFLAG_WRITES );
    
    snprintf( buf, TWO_LINES, "Room.%d", room->vnum );
    delete_file( ROOM_DIR, buf, false );
    delete_file( ROOM_PREV_DIR, buf, false );

    remove( area->room_first, room );    

    delete room;

    return;
  } 
  
  if( edit_extra( room->Extra_Descr( ),
		  imm,
		  offset( &imm->room_edit, imm ),
		  argument,
		  "rdesc" ) ) {
    area->dirty = true;
    area->modified = true;
  }
}


/*
 *   ROOM PROPERTIES
 */


bool Room_Data :: is_dark( const char_data *ch ) const
{
  if( ch && ch->is_affected( AFF_DARKVISION ) )
    return false;

  return ( Light() <= 0 );
}


/*
 *   LOCATION
 */


bool allowed_location( char_data* ch, int* bit,
		       const char* msg1, const char* msg2 )
{
  if( is_set( bit, LOC_NOT_INDOORS )
      && ch->in_room->is_indoors( ) ) {
    fsend( ch, "You cannot %s %s inside.", msg1, msg2 );
    return false;
  } 
  
  if( is_set( bit, LOC_NOT_OUTDOORS )
      && !ch->in_room->is_indoors( ) ) {
    fsend( ch, "You cannot %s %s outside.", msg1, msg2 );
    return false;
  } 
  
  if( is_set( bit, LOC_UNDERGROUND )
      && !is_set( ch->in_room->room_flags, RFLAG_UNDERGROUND ) ) {
    fsend( ch, "You can only %s %s underground.", msg1, msg2 );
    return false;
  } 
  
  if( is_set( bit, LOC_ABOVEGROUND )
      && is_set( ch->in_room->room_flags, RFLAG_UNDERGROUND ) ) {
    fsend( ch, "You cannot %s %s underground.", msg1, msg2 );
    return false;
  } 
  
  if( is_set( bit, LOC_SUNLIGHT ) && !weather.is_day( ) ) {
    fsend( ch, "You can only %s %s during the day.", msg1, msg2 );
    return false;
  } 
  
  if( is_set( bit, LOC_FULLMOON ) && weather.is_day( ) ) {
    fsend( ch, "You can only %s %s while the moon is full.",
	   msg1, msg2 );
    return false;
  } 
  
  if( is_set( bit, LOC_FOREST ) 
      && !forest( 0, ch->in_room ) ) {
    //      && ch->in_room->sector_type != SECT_FOREST ) {
    fsend( ch, "You can only %s %s while in a forest.", msg1, msg2 );
    return false;
  }
  
  if( is_set( bit, LOC_NOT_UNDERWATER ) 
      && is_submerged( 0, ch->in_room ) ) {
    fsend( ch, "You cannot %s %s underwater.", msg1, msg2 );
    return false;
  }

  if( is_set( bit, LOC_NOT_FIGHTING ) 
      && ch->fighting ) {
    fsend( ch, "You cannot %s %s while fighting.", msg1, msg2 );
    return false;
  }

  return true;
}


static int select( room_data *room, char_data *ch, const char *argument )
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
        send( ch, "Illegal character for flag - See help rfind.\n\r" );
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
      send( ch, "All flags require an argument - See help rfind.\n\r" );
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

    const char *string = 0;

    switch( letter ) {
    case 't' :  string = terrain_table[room->sector_type].name;        break;
    case 's' :  string = size_name[room->size];                        break;
    }

    if( string ) {
      if( !strncasecmp( tmp, string, length ) == negative )
        return 0;
      continue;
    }
    
    if( letter == 'n' ) {
      if( !is_name( tmp, room->Name( ch ) ) ^ negative )
        return 0;
      continue;
    }

    if( letter == 'f' ) {
      int i = 0;
      for( ; i < MAX_RFLAG; ++i ) {
        if( !strncasecmp( tmp, rflag_name[i], length ) ) {
          if( is_set( room->room_flags, i ) == negative ) {
            return 0;
	  }
          break;
	}
      }
      if( i == MAX_RFLAG ) {
	send( ch, "Unknown rflag \"%s\".\n\r", tmp );
	return -1;
      }
      continue;
    }

    if( letter == 'T' ) {
      int i = 0;
      for( ; !fmatches( tmp, action_trigger[i] ); ++i ) {
        if( i == MAX_ATN_TRIGGER-1 ) {
          send( ch, "Unknown trigger type, see help rfind.\n\r" );
          return -1;
	}
      }
      action_data *action = room->action;
      for( ; action && action->trigger != i; action = action->next );
      if( ( action != 0 ) != negative )
	continue;
      return 0;
    }

    send( ch, "Unknown flag - See help rfind.\n\r" );
    return -1;
  }
}


static bool rfind_area( char_data *ch, const char *argument, area_data *area, unsigned& count )
{
  const char *const title_msg =
    "Vnum    Name\n\r";

  for( room_data *room = area->room_first; room; room = room->next ) {
    switch( select( room, ch, argument ) ) {
    case -1 : return true;
    case  1 :
      if( count == 0 ) {
	page( ch, "\n\r" );
	page_underlined( ch, title_msg );
      }
      ++count;
      const char *name = room->Name( ch );
      page( ch, "[%5d] %s\n\r", room->vnum, trunc( name, 71 ) );
    }
  }

  return false;
}


void do_rfind( char_data *ch, const char *argument ) 
{
  int flags;
  get_flags( ch, argument, &flags, "p", 0 );

  if( is_set( flags, 0 ) ) {
    // Search all softcode for find_room( vnum ).

    int index;
    if( !number_arg( argument, index ) ) {
      send( ch, "Syntax: rfind -p <room #>\n\r" );
      return;
    }

    room_data *room = get_room_index( index );

    page( ch, "\n\r" );
    char tmp [ TWO_LINES ];
    snprintf( tmp, TWO_LINES, "--- %s (%d) ---",
	      room ? room->name : "(non-existent room)",
	      index );
    tmp[4] = toupper( tmp[4] );
    page_centered( ch, tmp ); 
    page( ch, "\n\r" );

    if( !search_code( ch, search_room, (void*)index ) )
      page( ch, "No references to room #%d were found.\n\r", index );

    return;
  }

  get_flags( ch, argument, &flags, "A", 0 );

  unsigned count = 0;

  if( is_set( flags, 0 ) ) {
    // All areas.
    if( !*argument ) {
      send( ch, "Rfind -A requires more flags.\n\r" );
      return;
    }
    for( area_data *area = area_list; area; area = area->next ) {
      if( rfind_area( ch, argument, area, count ) )
	return;
    }
  } else {
    // Current area only.
    if( rfind_area( ch, argument, ch->in_room->area, count ) )
      return;
  }

  if( count == 0 ) {
    send( ch, "No room matching search was found.\n\r" );
  } else {
    page( ch, "\n\rFound %d match%s.\n\r",
	  count,
	  count == 1 ? "" : "es" );
  }
}
