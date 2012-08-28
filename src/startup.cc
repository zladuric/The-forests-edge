#include <ctype.h>
#include <errno.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include "define.h"
#include "struct.h"


obj_clss_data*   obj_index_list  [ MAX_OBJ_INDEX ];
species_data*      species_list  [ MAX_SPECIES ];
char*                  cmd_help  [ MAX_COMMAND ];

mob_array        mob_list;
player_array  player_list;
obj_array        obj_list;
int            boot_stage;
int            max_serial = 0;
unsigned        max_cfunc = 0;

/*
 *   LOCAL ROUTINES
 */


void  load_helps     ( );
void  load_mobiles   ( );
static void  load_players   ( );
void  load_trainers  ( );

static void check_cfuncs( )
{
  unsigned i = 0;

  for( ; *cfunc_list[i+1].name; ++i ) {
    if( strcasecmp( cfunc_list[i].name, cfunc_list[i+1].name ) > 0 ) {
      roach( "Check_Cfuncs: function name out of order." );
      panic( "-- Name = '%s'", cfunc_list[i+1].name );
    }
  }

  max_cfunc = i+1;
}


static void init_rooms( )
{
  for( area_data *area = area_list; area; area = area->next ) {
    for( room_data *room = area->room_first; room; room = room->next ) {
      for( action_data *action = room->action; action; action = action->next ) {
	if( action->trigger == TRIGGER_INIT ) {
	  clear_variables( );
	  var_room = room;
	  action->execute( );
	}
      }
    }
  }
}


void count_objects( )
{
  for( int i = 0; i < obj_list; ++i ) {
    obj_data *obj = obj_list[i];
    if( obj->Is_Valid( ) ) {
      obj->pIndexData->count += obj->Number( );
    }
  }
}


static void delete_load_triggers( )
{
  bool found = false;

  for( int i = 1; i <= obj_clss_max; ++i ) {
    if( obj_clss_data *obj_clss = obj_index_list[i] ) {
      oprog_data *next;
      for( oprog_data *oprog = obj_clss->oprog; oprog; oprog = next ) {
	next = oprog->next;
	if( oprog->trigger == OPROG_TRIGGER_LOAD ) {
	  remove( obj_clss->oprog, oprog );
	  delete oprog;
	  found = true;
	}
      }
    }
  }

  if( found ) {
    echo( "Saving updated object file.\n\r" );
    save_objects( );
  }
}


static void load_areas( )
{
  echo( "Loading Areas ...\n\r" ); 

  FILE *fp = open_file( AREA_DIR, AREA_LIST, "r", true );

  while( true ) {
    const char *word = fread_word( fp );
    if( *word == '$' )
      break;
    area_data *area = load_area( word );
    area->save_text( );
    area->clear_text( );
    area->save_actions( );
    area->clear_actions( );
  }

  fclose( fp );

  printf( "Fixing exits...\n\r" );

  for( area_data *area = area_list; area; area = area->next ) {
    for( room_data *room = area->room_first; room; room = room->next ) {
      set_bit( room->room_flags, RFLAG_STATUS0 );
      set_bit( room->room_flags, RFLAG_STATUS1 );
      set_bit( room->room_flags, RFLAG_STATUS2 );
      
      for( int i = room->exits-1; i >= 0; i-- ) {
        exit_data *exit = room->exits[i];
        if( !( exit->to_room = get_room_index( (int) exit->to_room ) ) ) {
          roach( "Fix_Exits: Deleting exit from %d to non-existent %d.",
		 room->vnum, (int) exit->to_room );
          room->exits -= exit;
          delete exit;
	}
      }
      
      /*
      species_data *species;
      
      for( reset_data *reset = room->reset; reset; reset = reset->next ) {
	if( is_set( reset->flags, RSFLAG_MOB )
	    && ( species = get_species( reset->vnum ) ) ) {
	  ++species->reset_count;
	}
      }
      */
    }
  }
}


/* 
 *   EXTERNAL ROUTINES
 */    


void boot_db( )
{
  boot_stage = 0;

  check_cfuncs( );

  load_tables( );
  load_world( );
  load_objects( );
  load_mobiles( );
  load_lists( );
  load_rtables( );
  load_areas( );
  load_dictionary( );
  load_helps( );
  load_notes( );
  load_shops( );
  load_trainers( );
  load_quests( );
  load_banned( );
  load_badname( );
  load_remort( );

  boot_stage = 1;

  load_accounts( );
  load_players( );
  load_room_items( );  // Must be after load_player() for item ownership.
  load_clans( );

  delete_load_triggers( );
  init_rooms( );
  weather.init( );

  boot_stage = 2;
}


static void fix_items( Content_Array& stuff )
{
  if( stuff.is_empty() )
    return;

  for( int i = 0; i < stuff; ++i ) {
    if( obj_data *obj = object( stuff[i] ) ) {
      /*
      obj_clss_data *clss = obj->pIndexData;
      if( clss->item_type == ITEM_LIGHT
	  || clss->item_type == ITEM_LIGHT_PERM ) {
	if( obj->value[2] != 0 ) {
	  obj->value[0] = obj->value[2];
	  obj->value[2] = 0;
	}
      }
      */
      fix_items( obj->contents );
    }
  }
}


void do_lost( char_data *ch, const char *argument )
{
  if( is_mob( ch ) )
    return;

  char arg [ MAX_INPUT_LENGTH ];

  in_character = false;

  pfile_data *pfile;

  if( !*argument ) {
    char tmp[ 4 ];
    bool found = false;
    DIR *dirp = opendir( LOST_FOUND_DIR );
    for( struct dirent *dp = readdir( dirp ); dp; dp = readdir( dirp ) ) {
      if( *dp->d_name == '.'
	  || !( pfile = find_pfile_exact( dp->d_name ) ) )
	continue;
      FILE *fp = open_file( LOST_FOUND_DIR, dp->d_name, "r" );
      if( !fp )
	continue;

      Content_Array dummy;
      const char *a = "", *b = "", *c = "";
      while( true ) {
	*tmp = '\0';
	fread( tmp, 4, 1, fp );
	if( !strncmp( tmp, "End&", 4 ) )
	  break;
	if( !strncmp( tmp, "Auct", 4 ) ) {
	  if( !read_object( fp, dummy, 0, tmp ) ) {
	    roach( "Lost: lost+found file auction entry corrupted." );
	    roach( "-- Player = '%s'", pfile->name );
	    break;
	  }
	  a = "A";
	  extract( dummy );
	  continue;
	}
	if( !strncmp( tmp, "Cach", 4 ) ) {
	  int vnum;
	  fread( &vnum, sizeof( int ), 1, fp );
	  if( !read_object( fp, dummy, 0, tmp ) ) {
	    roach( "Lost: lost+found file cache entry corrupted." );
	    roach( "-- Player = '%s'", pfile->name );
	    break;
	  }
	  b = "B";
	  extract( dummy );
	  continue;
	}
	if( !strncmp( tmp, "Corp", 4 ) ) {
	  if( !read_object( fp, dummy, 0, tmp ) ) {
	    roach( "Lost: lost+found file corpse entry corrupted." );
	    roach( "-- Player = '%s'", pfile->name );
	    break;
	  }
	  c = "C";
	  extract( dummy );
	  continue;
	}
	roach( "Load_Lost: lost+found file corrupted." );
	roach( "-- Player = '%s'", pfile->name );
	break;
      }
      fclose( fp );
      if( *a || *b || *c ) {
	if( !found ) {
	  page_underlined( ch, "%-14s Files\n\r", "Name" );
	  found = true;
	}
 	page( ch, "%-14s %s%s%s\n\r", pfile->name, a, b, c );
      }
    }

    if( found ) {
      page( ch, "\n\r" );
      page_underlined( ch, "Key:\n\r" );
      page( ch, "  A - Auction\n\r" );
      page( ch, "  B - Buried caches\n\r" );
      page( ch, "  C - Corpses\n\r" );
    } else {
      send( ch, "No lost+found files exist.\n\r" );
    }
    return;
    //    pfile = ch->pcdata->pfile;
  } else {
    argument = one_argument( argument, arg );
    pfile = find_pfile( arg, ch );
    if( !pfile )
      return;
    if( pfile != ch->pcdata->pfile
	&& pfile->trust >= get_trust( ch ) ) {
      fsend( ch, "You cannot view the lost+found file of %s.", pfile->name );
      return;
    }
  }

  char path [ TWO_LINES ];
  char tmp [ TWO_LINES ];
  snprintf( path, TWO_LINES, "%s%s", LOST_FOUND_DIR, pfile->name );

  FILE *fp = fopen( path, "r" );

  if( !fp ) {
    fsend( ch, "No lost+found file for player %s.", pfile->name );
    return;
  }
  
  Content_Array corpses;
  Content_Array caches;
  Content_Array auction;

  while( true ) {
    *tmp = '\0';
    fread( tmp, 4, 1, fp );
    
    if( !strncmp( tmp, "End&", 4 ) )
      break;
    
    if( !strncmp( tmp, "Corp", 4 ) ) {
      if( !read_object( fp, corpses, 0, tmp ) ) {
	roach( "Lost: lost+found file corpse entry corrupted." );
	roach( "-- File = '%s'", path );
	break;
      }
      continue;
    }
    
    if( !strncmp( tmp, "Cach", 4 ) ) {
      int vnum;
      fread( &vnum, sizeof( int ), 1, fp );
      if( !read_object( fp, caches, 0, tmp ) ) {
	roach( "Lost: lost+found file cache entry corrupted." );
	roach( "-- File = '%s'", path );
	break;
      }
      continue;
    }
    
    if( !strncmp( tmp, "Auct", 4 ) ) {
      if( !read_object( fp, auction, 0, tmp ) ) {
	roach( "Lost: lost+found file auction entry corrupted." );
	roach( "-- File = '%s'", path );
	break;
      }
      continue;
    }
    
    roach( "Load_Lost: lost+found file corrupted." );
    roach( "-- File = '%s'", path );
    break;
  }
  
  fclose( fp );

  page( ch, "\n\r" );

  if( corpses.is_empty( ) ) {
    fpage( ch, "No corpse items in lost+found for %s.", pfile->name );
  } else {
    select( corpses );
    rehash_weight( ch, corpses );
    page_underlined( ch, "Lost+Found Corpse Items for %s (%d corpse%s)\n\r",
		     pfile->name,
		     pfile->corpses.size,
		     pfile->corpses.size == 1 ? "" : "s" );
    for( int i = 0; i < corpses; ++i ) {
      obj_data *obj = (obj_data*) corpses[i];
      if( obj->Shown( ) > 0 ) {
	page( ch, "%-70s %7.2f\n\r",
	      obj->Seen_Name( ch, obj->Shown( ), true ),
	      0.01*(double)obj->temp );
      }
    }
    extract( corpses );
  }

  page( ch, "\n\r" );

  if( caches.is_empty( ) ) {
    fpage( ch, "No cache items in lost+found for %s.", pfile->name );
  } else {
    select( caches );
    rehash_weight( ch, caches );
    page_underlined( ch, "Lost+Found Cache Items for %s (%d cache%s)\n\r",
		     pfile->name,
		     pfile->caches.size,
		     pfile->caches.size == 1 ? "" : "s" );
    for( int i = 0; i < caches; ++i ) {
      obj_data *obj = (obj_data*) caches[i];
      if( obj->Shown( ) > 0 ) {
	page( ch, "%-70s %7.2f\n\r",
	      obj->Seen_Name( ch, obj->Shown( ), true ),
	      0.01*(double)obj->temp );
      }
    }
    extract( caches );
  }

  page( ch, "\n\r" );

  if( auction.is_empty( ) ) {
    fpage( ch, "No auction items in lost+found for %s.", pfile->name );
  } else {
    select( auction );
    rehash_weight( ch, auction );
    page_underlined( ch, "Lost+Found Auction Items for %s (%d lot%s)\n\r",
		     pfile->name,
		     pfile->auction.size,
		     pfile->auction.size == 1 ? "" : "s" );
    for( int i = 0; i < auction; ++i ) {
      obj_data *obj = (obj_data*) auction[i];
      if( obj->Shown( ) > 0 ) {
	page( ch, "%-70s %7.2f\n\r",
	      obj->Seen_Name( ch, obj->Shown( ), true ),
	      0.01*(double)obj->temp );
      }
    }
    extract( auction );
  }
}


static void lost_and_found( player_data *pl )
{
  char path [ TWO_LINES ];
  char tmp [ TWO_LINES ];
  snprintf( path, TWO_LINES, "%s%s", LOST_FOUND_DIR, pl->descr->name );

  // Load lost+found.
  if( FILE *fp = fopen( path, "r" ) ) {
    
    while( true ) {
      *tmp = '\0';
      fread( tmp, 4, 1, fp );

      if( !strncmp( tmp, "End&", 4 ) )
	break;

      if( !strncmp( tmp, "Corp", 4 ) ) {
	Content_Array stuff;
	if( !read_object( fp, stuff, pl, tmp ) ) {
	  roach( "Load_Lost: lost+found file corpse entry corrupted." );
	  panic( "-- File = '%s'", path );
	} else {
	  transfer_objects( pl, pl->locker, 0, stuff );
	  pl->fixed = true;
	}
	continue;
      }

      if( !strncmp( tmp, "Cach", 4 ) ) {
	int vnum;
	fread( &vnum, sizeof( int ), 1, fp );
	room_data *room = get_room_index( vnum );
	if( !room ) {
	  roach( "Load_Lost: lost+found file cache entry contains non-existent room." );
	  roach( "-- File = '%s'", path );
	  roach( "-- Room = %d", vnum );
	}
	Content_Array stuff;
	if( !read_object( fp, stuff, pl, tmp ) ) {
	  roach( "Load_Lost: lost+found file cache entry corrupted." );
	  panic( "-- File = '%s'", path );
	} else {
	  obj_data *cache = create( get_obj_index( OBJ_CACHE ) );
	  cache->owner = pl->pcdata->pfile;
	  transfer_objects( 0, cache->contents, 0, stuff );
	  if( room ) {
	    cache->To( room );
	    setup_cache( pl, cache );
	    pl->pcdata->pfile->caches += cache;
	  } else {
	    cache->Extract( );
	  }
	  pl->fixed = true;
	}
	continue;
      }

      if( !strncmp( tmp, "Auct", 4 ) ) {
	Content_Array stuff;
	if( !read_object( fp, stuff, pl, tmp ) ) {
	  roach( "Load_Lost: lost+found file auction entry corrupted." );
	  panic( "-- File = '%s'", path );
	} else {
	  transfer_objects( pl, pl->contents, 0, stuff );
	  pl->fixed = true;
	}
	continue;
      }

      roach( "Load_Lost: lost+found file corrupted." );
      panic( "-- File = '%s'", path );
    }

    fclose( fp );

    if( unlink( path ) < 0 ) {
      roach( "Delete_File: %s", strerror( errno ) );
      panic( "-- File = '%s'", path );
    }
  }
}


static void check_player( player_data* pl )
{
  if( pl->Level() <  LEVEL_APPRENTICE ) { 
    if( pl->shdata->race >= MAX_PLYR_RACE ) {
      printf( "  -%s is a non-player race.\n", pl->descr->name );
      aphid( "%s is a non-player race.\n", pl->descr->name );
    } else {
      const int points = creation_points( pl );
      
      if( points < 0 ) {
	printf( "  -%s has impossible stats.\n", pl->descr->name );
	aphid( "%s has impossible stats.\n", pl->descr->name );
      }
    }
  }

  lost_and_found( pl );


  //#define STARTUP_FIX_ITEMS
#ifdef STARTUP_FIX_ITEMS

  //  pl->remort = 0;
  /*
  fix_items( pl->contents );
  fix_items( pl->wearing );
  fix_items( pl->locker );

  for( int i = 0; i < pl->followers; ++i ) {
    char_data *pet = pl->followers[i];
    fix_items( pet->contents );
    fix_items( pet->wearing );
  }
  */

  // Force save.
  pl->fixed = true;
#endif

}


void load_players( )
{
  echo( "Loading Players ...\n\r" );
  
  /*
    	First load of all pfiles... get all pfile_data info.
  */

  DIR *dirp = opendir( PLAYER_DIR );
  link_data link;
  link.connected = CON_PLAYING;

  for( struct dirent *dp = readdir( dirp ); dp; dp = readdir( dirp ) ) {
    if( !strcmp( dp->d_name, "." ) || !strcmp( dp->d_name, ".." ) )
      continue;
    
    if( !load_char( &link, dp->d_name, PLAYER_DIR ) ) {
      bug( "Load_players: Non-existent player file (%s)", dp->d_name );
      continue;
    }
    
    player_data *pl = link.player;
    
    max_serial = max( max_serial, pl->pcdata->pfile->serial+1 );

    pl->Extract( );
    extracted.delete_list();
  }


  /*
    	Second load of all pfiles... resolve references.
	Specifically, things like object ownership, recognize data, votes,
	obj load triggers.
  */

  rewinddir( dirp );

  for( struct dirent *dp = readdir( dirp ); dp; dp = readdir( dirp ) ) {
    if( !strcmp( dp->d_name, "." ) || !strcmp( dp->d_name, ".." ) )
      continue;
    
    if( !load_char( &link, dp->d_name, PLAYER_DIR ) ) {
      bug( "Load_players: Non-existent player file (%s)", dp->d_name );
      continue;
    }
    
    player_data *pl = link.player;
    
    check_player( pl );

    if( pl->fixed ) {
      echo( "Saving updated player %s.\n\r", pl );
      pl->Save( false );
    }

    count_objects( );

    pl->Extract( );
    extracted.delete_list();
  }

  closedir( dirp );

  echo( "Writing HTML Players ...\n\r" );

  if( !save_html_players( ) ) {
    bug( "Failed to write HTML player files." );
  }
}
