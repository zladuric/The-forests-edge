#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include "define.h"
#include "struct.h"


typedef class Save_Entry  save_entry;


class Save_Entry
{
public:
  char*             name;
  char              type;
  int             length;
  void*             pntr;
  const void*        std;
};


save_array saves;


/*
 *   EXTERNAL ROUTINES
 */


void   read_affects       ( FILE*, char_data* );
void   write_affects      ( FILE*, char_data* );
void   write_affects      ( FILE*, obj_clss_data* );


/* 
 *   LOCAL CONSTANTS
 */


static const int zero = 0;
static const int one = 1; 
static const int minus_one = -1; 


/*
 *   SUPPORT ROUTINES
 */


Save_Data :: ~Save_Data( )
{
  for( int i = 0; i < save_list; ++i ) {
    save_list[i]->save = 0;
    consolidate( save_list[i], false );
  }
}


static Save_Data *save_file( obj_data *obj )
{
  Content_Array *array = obj->array;
  thing_data *prev = 0;

  while( array ) {
    if( thing_data *where = array->where ) {
      //      if( !where->Is_Valid( ) )
      //	return 0;
      if( char_data *ch = character( where ) ) {
	if( player_data *pl = player( ch ) ) {
	  if( array != &pl->junked )
	    return pl;
	} else if( is_set( ch->status, STAT_PET ) ) {
	  if( player_data *pl = player( ch->leader ) )
	    return pl;
	}
	return 0;
      } else if( room_data *room = Room( where ) ) {
	if( is_set( room->room_flags, RFLAG_SAVE_ITEMS ) )
	  return room;
	if( obj_data *obj = object( prev ) ) {
	  if( obj->owner
	      && ( obj->pIndexData->vnum == OBJ_CORPSE_PC
		   || obj->pIndexData->vnum == OBJ_CORPSE_PET
		   || obj->pIndexData->vnum == OBJ_CACHE ) ) {
	    return obj->owner;
	  }
	}
	return 0;
      } else if( auction_data *auction = Auction( where ) ) {
	return auction->seller;
      }
      prev = where;
      array = where->array;
    } else {
      return 0;
    }
  }

  return 0;
}


/*
 *  Loop through all the objects previously saved in the file.
 *  If they are now someplace else, add the new owner to the save_list.
 *  Since the objects' save pointers become zero, may have to consolidate them.
 */
void save_array::Setup( Save_Data *save )
{
  for( int i = 0; i < save->save_list; ++i ) {
    obj_data *obj = save->save_list[i];
    if( Save_Data *file = save_file( obj ) ) {
      if( file != save ) {
	operator+=( file );
      }
    }
    if( obj->save ) {
      obj->save = 0;
      consolidate( obj, false );
    }
  }
  
  save->save_list.clear( );
}


void save_array :: Save( )
{
  while( !is_empty( ) ) {
    Save_Data *save = list[0];
    remove(0);
    if( player_data *pl = dynamic_cast<player_data *>( save ) ) {
      if( is_set( pl->pcdata->message, MSG_AUTOSAVE ) ) {
	send( pl, "Autosave forced.\n\r" );
      }
    }
    save->Save( );
  }
}


static void write_string( FILE* fp, const char* string, char type )
{
  if( type == 's' ) {
    unsigned char c = strlen( string ); 
    fwrite( &c, 1, 1, fp ); 
    fwrite( string, (int)( c ), 1, fp );
  } else {
    int i = strlen( string );
    fwrite( &i, sizeof( int ), 1, fp );
    fwrite( string, i, 1, fp );
  }
}


/*This function takes a file pointer and an item to be printed.  It 
  Determines what kind of item it is, and then outputs it to the fp
  appropriately.  Calls write object for objects. */
static void write_item( FILE* fp, save_entry* item, Save_Data *save )
{
  int                i;
  unsigned char      c;
  void*           pntr  = item->pntr;
  int              num  = item->length;
  char            type  = item->type;
  const void*      std  = item->std;

  /*-- STRINGS --*/
  
  if( toupper( type ) == 'S' ) {
    if( pntr == 0 || ( std != 0
		       && !strcasecmp( *((char**)pntr), *((char**)std) ) ) )
      return;
    fwrite( item->name, 4, 1, fp );
    write_string( fp, *((char**)pntr), type );
    return;
  }

  /*-- temporary for changes in array size --*/
  //  if( type == '+' ) {
  //    return;
  //  }

  /*-- INTEGERS --*/
  // Lowercase 'i' is full integers (not bytes) with range 0-255.
  // Uppercase 'I' is full integers.

  if( toupper( type ) == 'I' ) {
    if( pntr == 0 )
      return;
    if( std != 0 ) {
      for( i = 0; ((int*)pntr)[i] == ((int*)std)[i]; i++ )
        if( i == num-1 )
          return;
    }
    fwrite( item->name, 4, 1, fp );
    if( type == 'I' ) {
      fwrite( pntr, num*sizeof( int ), 1, fp );
    } else {
      for( i = 0; i < num; i++ ) {
        c = ((int*)pntr)[i];
        fwrite( &c, 1, 1, fp );
      }
    }
    return;
  }
  

  /*-- BYTES --*/

  if( type == 'b' ) {
    if( pntr == 0 )
      return;
    if( std != 0 ) {
      for( i = 0; ((unsigned char*)pntr)[i] == ((unsigned char*)std)[i]; i++ )
        if( i == num-1 )
          return;
    }
    fwrite( item->name, 4, 1, fp );
    fwrite( pntr, num*sizeof( unsigned char ), 1, fp );
    return;
  }

  /*-- PFILES --*/

  if( type == 'P' ) {
    if( *(pfile_data**)pntr != 0
	&& ( std == 0 || *(pfile_data**)pntr != *(pfile_data**)std ) ) {
      fwrite( item->name, 4, 1, fp );
      fwrite( &(*(pfile_data**)pntr)->ident, sizeof( int ), 1, fp );
      fwrite( &(*(pfile_data**)pntr)->serial, sizeof( int ), 1, fp );
    }
    return;
  }
  
  /*-- VOTES --*/

  if( type == 'V' ) {
    if( pntr == 0 )
      return;
    fwrite( item->name, 4, 1, fp );
    for( i = 0; i < MAX_VOTE; ++i ) {
      fwrite( &votes[i].serial, sizeof( int ), 1, fp );
      if( pfile_data *pfile = ((pfile_data**)pntr)[i] ) {
	fwrite( &pfile->ident, sizeof( int ), 1, fp );
	fwrite( &pfile->serial, sizeof( int ), 1, fp );
      } else {
	int zero = 0;
	fwrite( &zero, sizeof( int ), 1, fp );
	fwrite( &zero, sizeof( int ), 1, fp );
      }
    }
    return;
  }
  
  /*-- OBJECTS --*/

  if( type == 'O' || type == 'C' ) {
    Content_Array *list = (Content_Array*) pntr; 
    if( !list->is_empty() ) {
      fwrite( item->name, 4, 1, fp );
      write_object( fp, *list, save, type == 'C' );
    }
    return;
  }
}


static void read_string( FILE* fp, char** pntr, char type, int memory, const char *const *def = &empty_string )
{
  unsigned char  c;
  int            i;
  char     *string;

  if( type == 's' ) {
    fread( &c, 1, 1, fp );
    i = (int)( c );
  } else {
    fread( &i, sizeof( int ), 1, fp );
  }
  
  if( i == 0 ) {
    string = empty_string;
  } else {
    string = new char [ i+1 ];
    fread( string, i, 1, fp );
    string[i] = '\0';
    if( pntr != 0 ) {
      record_new( i+1, -memory );
    } else {
      delete [] string;
    }
  }

  if( pntr != 0 ) {
    if( memory != MEM_UNKNOWN ) {
      if( !def || *pntr != *def ) {
	free_string( *pntr, memory );
      }
    }
    *pntr = string;
  }
}


static bool read_item( FILE* fp, save_entry* item, char* tmp, int memory, Save_Data *save )
{
  void*           pntr  = item->pntr;
  int              num  = item->length;
  char            type  = item->type;
  unsigned char      c;
  int             i, j, k;

  if( toupper( type ) == 'S' ) {
    const char *const *def = (const char *const*)(item->std);
    read_string( fp, (char**) pntr, type, memory, def );
    return true;
  }

  /*-- temporary for changes in array size --*/
  //  if( type == '+' ) {
  //    type = 'I';
  //  }

  // Lowercase 'i' is full integers (not bytes) with range 0-255.
  // Uppercase 'I' is full integers.
  if( type == 'i' ) {
    for( i = 0; i < num; i++ ) {
      fread( &c, 1, 1, fp );
      if( pntr != 0 )
        ((int*)pntr)[i] = c;
    }
    return true;
  }

  if( type == 'I' ) {
    if( pntr == 0 ) {
      for( i = 0; i < num; ++i )
        fread( &j, sizeof( int ), 1, fp );
    } else
      fread( pntr, sizeof( int ), num, fp );
    return true;
  }

  if( type == 'b' ) {
    if( pntr == 0 ) {
      for( i = 0; i < num; ++i )
        fread( &j, sizeof( unsigned char ), 1, fp );
    } else
      fread( pntr, sizeof( unsigned char ), num, fp );
    return true;
  }

  if( type == 'O' || type == 'C' ) {
    Content_Array* list = (Content_Array*) pntr;
    return read_object( fp, *list, save, tmp );
  }

  if( type =='P' ) {
    fread( &i, sizeof( int ), 1, fp );
    fread( &j, sizeof( int ), 1, fp );
    if( i < 0 || i >= MAX_PFILE ) {
      roach( "Read_Item: Impossible Pfile index %d.", i );
      *(pfile_data**)pntr = (pfile_data*) -1;
    } else if( j < 0 ) {
      roach( "Read_Item: Impossible Pfile serial number %d.", j );
      *(pfile_data**)pntr = (pfile_data*) -1;
    } else {
      max_serial = max( max_serial, j+1 );  // Must remain ouside the following IF.
      if( ident_list[i] && ident_list[i]->serial == j ) {
	*(pfile_data**)pntr = ident_list[i];
      }
    }
    return true;
  }

  if( type == 'V' ) {
    for( int n = 0; n < MAX_VOTE; ++n ) {
      fread( &k, sizeof( int ), 1, fp );
      fread( &i, sizeof( int ), 1, fp );
      fread( &j, sizeof( int ), 1, fp );
      if( i < 0 || i >= MAX_PFILE ) {
	roach( "Read_Item: Impossible Pfile index %d in vote %d.", i, n );
      } else if( j < 0 ) {
	roach( "Read_Item: Impossible Pfile serial number %d in vote %d.", j, n );
      } else if( k < 0 ) {
	roach( "Read_Item: Impossible Vote serial number %d in vote %d.", k, n );
      } else {
	Vote_Data::max_serial = max( Vote_Data::max_serial, k+1 );  // Must remain ouside the following IF.
	if( votes[n].serial == k && i != 0 ) {
	  max_serial = max( max_serial, j+1 );  // Must remain ouside the following IF.
	  if( ident_list[i] && ident_list[i]->serial == j ) {
	    ((pfile_data**)pntr)[n] = ident_list[i];
	  }
	}
      }
    }
    return true;
  }

  return true;
}


static bool file_list( FILE* fp, save_entry* item, Save_Data *save, char* tmp, int memory = MEM_UNKNOWN )
{
  /* If tmp is the empty string (default), then parse through the table,
     from top to bottom, writing each item of the table to the file */
  if( tmp == empty_string ) {
    for( int i = 0; *item[i].name; i++ ) 
      write_item( fp, &item[i], save );
  }
  /* Otherwise, read the item whose name matches that of tmp into the
     array */
  else {
    for( int i = 0; *item[i].name; i++ ) {
      if( !strncmp( tmp, item[i].name, 4 ) ) {
        if( !read_item( fp, &item[i], tmp, memory, save ) )
          return false;
        *tmp = '\0';
        fread( tmp, 4, 1, fp );
      }
    }
  }

  return true;
}


static void fread_until( FILE* fp, const char* text )
{
  char      tmp  [ ONE_LINE ];
  const int    length  = strlen( text );

  for( int i = 0; i < length; i++ ) {
    fread( &tmp[i], 1, 1, fp );
    if( tmp[i] != text[i] )
      i = -1;
  }
}


/*
 *   READ/WRITE PET ROUTINES
 */


static int is_mount;


static bool file_pet( FILE* fp, mob_data *pet, player_data *pl, char* tmp = empty_string )
{
  mob_data *npc = (mob_data*) pet;

  save_entry list [] = {
    { "Cond",  'I', 4,  pet->condition,   0     },
    { "Hand",  'i', 1,  &pet->hand,       0     },
    { "HpMv",  'I', 6,  &pet->hit,        0     },
    { "Inv.",  'C', 0,  &pet->contents,   0     },
    { "Matu",  'I', 1,  &npc->maturity,   0     },
    { "IsMo",  'i', 1,  &is_mount,        0     },
    { "Name",  's', 0,  &pet->pet_name,   &empty_string     },
    { "Posi",  'i', 1,  &pet->position,   0     },
    { "Sex ",  'i', 1,  &pet->sex,        0     },
    { "StFg",  'I', 2,  &pet->status,     0     },
    { "Worn",  'O', 0,  &pet->wearing,    0     },
    { "",      ' ', 0,  0,                0     } };

  return file_list( fp, list, pl, tmp, MEM_MOBS );
}


static void write_pet( FILE* fp, mob_data *pet, player_data *pl )
{  
  room_data *room;

  fwrite( "Pet.", 4, 1, fp );
  fwrite( &pet->species->vnum, sizeof( int ), 1, fp );

  is_mount = ( pet->rider == pl );

  file_pet( fp, pet, pl );

  /*-- WRITE ROOM --*/

  if( ( room = pet->in_room )
      || ( room = pet->was_in_room ) ) {
    fwrite( "Room", 4, 1, fp );
    fwrite( &room->vnum, sizeof( int ), 1, fp );
  }
  
  write_affects( fp, pet );
  fwrite( "End&", 4, 1, fp );
}


static bool read_pet( FILE* fp, player_data* player, char* tmp )
{
  species_data*     species; 
  int                     i;

  fread( &i, sizeof( int ), 1, fp );

  if( !( species = get_species( i ) ) ) {
    roach( "Read_Pet: Unknown species." );
    roach( "-- Vnum = %d", i );    
    fread_until( fp, "End&" );
    return true;
  } 

  mob_data *pet = new Mob_Data( species );

  fread( tmp, 4, 1, fp );

  is_mount = false;

  if( !file_pet( fp, pet, player, tmp ) ) {
    pet->Extract();
    return false;
  }

  /*-- READ ROOM --*/

  if( !strncmp( tmp, "Room", 4 ) ) {
    fread( &i, sizeof( int ), 1, fp );
    pet->was_in_room = get_room_index( i );
    fread( tmp, 4, 1, fp );
  }

  /*-- READ AFFECTS --*/

  if( !strncmp( tmp, "Afft", 4 ) ) {
    read_affects( fp, pet );
    fread( tmp, 4, 1, fp );
  }
  
  if( strncmp( tmp, "End&", 4 ) ) {
    roach( "Read_Pet: Missing 'End&'." );
    return false;
  }
  
  bool grouped = is_set( player->status, STAT_IN_GROUP )
                 && is_set( pet->status, STAT_IN_GROUP );
  set_bit( pet->status, STAT_PET );
  add_follower( pet, player, 0 );
  update_maxes( pet );
  
  if( grouped ) {
    add_group( player, pet );
  }

  if( is_set( pet->status, STAT_FAMILIAR ) )
    player->familiar = pet;

  if( is_mount ) {
    player->mount = pet;
    pet->rider = player;
  }

  // Done by add_follower:
  //  set_owner( player->pcdata->pfile, pet->wearing ); 
 
  remove_bit( pet->status, STAT_BERSERK );
  remove_bit( pet->status, STAT_FOCUS );
  remove_bit( pet->status, STAT_REPLY_LOCK );
  remove_bit( pet->status, STAT_FORCED );
  remove_bit( pet->status, STAT_NO_SNOOP );
  remove_bit( pet->status, STAT_GARROTING );
  remove_bit( pet->status, STAT_RESPOND );
  //  remove_bit( pet->status, STAT_LEADER );
  //  remove_bit( pet->status, STAT_FOLLOWER );
  remove_bit( pet->status, STAT_NOFOLLOW );
  remove_bit( pet->status, STAT_SNUCK );
  remove_bit( pet->status, STAT_WAITING );
  remove_bit( pet->status, STAT_HOLD_POS );
  remove_bit( pet->status, STAT_STOOD );
  remove_bit( pet->status, STAT_FLEE_FROM );
  remove_bit( pet->status, STAT_GROUP_LOOTER );

  return true;
}


/*
 *   READ/WRITE CHARACTER ROUTINES
 */


static bool file_wizard( FILE* fp, wizard_data* imm, char* tmp = empty_string )
{
  save_entry list [] = {
    { "Bfin", 's', 0,  &imm->bamfin,               &empty_string     },
    { "Bfot", 's', 0,  &imm->bamfout,              &empty_string     },
    { "LvTl", 's', 0,  &imm->level_title,          &empty_string     },
    { "Offc", 'I', 1,  &imm->office,               &zero },
    { "PrmF", 'I', 2,  imm->pcdata->pfile->permission,     0     },	// *** FIX ME: move to file_pfile.
    { "Reca", 'I', 1,  &imm->recall,               &zero },
    { "WzIn", 'i', 1,  &imm->wizinvis,             &zero },
    { "",     ' ', 0,  0,                          0     } };

  return file_list( fp, list, imm, tmp, MEM_WIZARD );
}


static bool file_player( FILE* fp, player_data* player, char* tmp = empty_string )
{
  save_entry list [] = {
    { "Age.", 'I', 1,   &player->base_age,                  0  },
    { "Algn", 'i', 1,   &player->shdata->alignment,         0  },
    { "Bank", 'I', 1,   &player->bank,                      &zero },
    { "Cflg", 'I', MAX_CFLAG,   player->pcdata->cflags,     0  },
    { "Clss", 'i', 1,   &player->pcdata->clss,              0  },
    { "Colr", 'I', SAVED_COLORS,  &player->pcdata->color,             0  }, 
    { "Cond", 'I', 4,   player->condition,                  0  },
    { "Dict", 'S', 1,   &player->pcdata->dictionary,        &empty_string },
    { "Dths", 'I', 1,   &player->shdata->deaths,            0  },
    { "Exp.", 'I', 1,   &player->exp,                       0  },
    { "Fame", 'I', 1,   &player->shdata->fame,              0  },
    { "Gssp", 'I', 1,   &player->gossip_pts,                0  },
    { "Hand", 'i', 1,   &player->hand,                      0  },
    { "HpMv", 'I', 6,   &player->hit,                       0  },
    { "IFlg", 'I', 2,   player->pcdata->iflag,              0  },
    { "Inv.", 'C', 0,   &player->contents,                  0  },
    { "Jrnl", 'S', 1,   &player->pcdata->journal,           &empty_string },
    { "Kill", 'I', 1,   &player->shdata->kills,             0  },
    { "Levl", 'i', 1,   &player->shdata->level,             0  },
    { "Lock", 'C', 0,   &player->locker,                    0  }, 
    { "LvHt", 'I', 4,   &player->pcdata->level_hit,         0  },
    { "Mort", 'I', 1,   &player->remort,                    0  },
    { "Msgs", 'I', 1,   &player->pcdata->message,           0  },
    { "MsSt", 'I', 1,   &player->pcdata->mess_settings,     0  },
    { "Mvmt", 'i', 1,   &player->movement,                  &minus_one  },
    { "Piet", 'I', 1,   &player->pcdata->piety,             0  },
    { "Play", 'I', 1,   &player->played,                    0  },
    { "Posi", 'i', 1,   &player->position,                  0  },
    { "Prac", 'I', 1,   &player->pcdata->practice,          0  },
    { "Prpt", 'S', 0,   &player->pcdata->prompt,            &empty_string  },
    { "Pryr", 'I', 1,   &player->prayer,                    0  },
    { "QsPt", 'I', 1,   &player->pcdata->quest_pts,         0  },
    { "Race", 'i', 1,   &player->shdata->race,              0  },
    { "Reli", 'i', 1,   &player->pcdata->religion,          0  },
    { "RpAl", 'I', 9,   player->reputation.alignment,       0  },          
    { "RpNt", 'I', 15,  player->reputation.nation,          0  },
    { "RpGd", 'I', 1,   &player->reputation.gold,           0  }, 
    { "RpBd", 'I', 1,   &player->reputation.blood,          0  }, 
    { "RpMg", 'I', 1,   &player->reputation.magic,          0  }, 
    { "Sex ", 'i', 1,   &player->sex,                       0  },
    { "Spkg", 'i', 1,   &player->pcdata->speaking,          0  },
    { "Stat", 'b', 5,   &player->shdata->strength,          0  },
    { "StFg", 'I', 2,   &player->status,                    0  },
    { "Term", 'I', 2,   &player->pcdata->terminal,          0  }, // terminal + lines + columns
    { "TmKy", 'S', 0,   &player->pcdata->tmp_keywords,      &empty_string  },
    { "TmAp", 'S', 0,   &player->pcdata->tmp_short,         &empty_string  },
    { "Titl", 's', 0,   &player->pcdata->title,             &empty_string  },
    { "Trst", 'i', 1,   &player->pcdata->trust,             0  },
    { "Wimp", 'I', 1,   &player->pcdata->wimpy,             0  },
    { "Worn", 'O', 0,   &player->wearing,                   0  },
    { "",     ' ', 0,   0,                                  0  } };

  return file_list( fp, list, player, tmp, MEM_PLAYER );
}


static bool file_pfile( FILE* fp, pfile_data* pfile, char* tmp = empty_string )
{
  save_entry list [] = {
    { "Crtd", 'I', 1,   &pfile->created,    0  },
    { "Hmpg", 'S', 0,   &pfile->homepage,   &empty_string  }, 
    { "Idnt", 'I', 1,   &pfile->ident,      0  },
    { "LsHt", 's', 0,   &pfile->last_host,  &empty_string  },
    { "LtOn", 'I', 1,   &pfile->last_on,    0  },
    { "PlFg", 'I', 2,   pfile->flags,       0  },
    { "Pswd", 's', 0,   &pfile->pwd,        &empty_string  },
    { "Seri", 'I', 1,   &pfile->serial,     0  },
    { "Sett", 'I', 1,   &pfile->settings,   0  },
    { "Vote", 'V', 0,   &pfile->vote,       0  },
    { "",     ' ', 0,   0,                  0  } };

  return file_list( fp, list, 0, tmp, MEM_PFILE );
}


static bool file_descr( FILE* fp, descr_data* descr, char* tmp = empty_string )
{
  save_entry list [] = {
    { "Appr", 's', 0,   &descr->singular,   &empty_string  },
    { "Kywd", 's', 0,   &descr->keywords,   &empty_string  },
    { "Long", 'S', 0,   &descr->complete,   &empty_string  },
    { "Name", 's', 0,   &descr->name,       &empty_string  },
    { "",     ' ', 0,   0,                  0  } };

  return file_list( fp, list, 0, tmp, MEM_DESCR );
}


static bool write_pfile( pfile_data *pfile, player_data *player = 0 )
{
  // Clean up corpse list.
  for( int i = 0; i < pfile->corpses; ) {
    obj_data *corpse = pfile->corpses[i];
    Save_Data *save = corpse->save;
    if( save && save != pfile
	|| corpse->contents.is_empty( ) ) {
      pfile->corpses.remove( i );
    } else {
      ++i;
    }
  }

  // Clean up cache list.
  for( int i = 0; i < pfile->caches; ) {
    obj_data *cache = pfile->caches[i];
    Save_Data *save = cache->save;
    if( save && save != pfile
	|| !Room( cache->array->where )
	|| cache->contents.is_empty( ) ) {
      pfile->caches.remove( i );
    } else {
      ++i;
    }
  }

  if( pfile->corpses.is_empty( )
      && pfile->caches.is_empty( )
      && pfile->auction.is_empty( ) ) {
    delete_file( LOST_FOUND_DIR, pfile->name, false );
    if( player ) {
      saves.Setup( pfile );
      saves -= player;
    }
  } else {
    if( FILE *fp = open_file( LOST_FOUND_DIR, pfile->name, "w", false ) ) {
      if( player ) {
	saves.Setup( pfile );
	saves -= player;
      }
      for( int i = 0; i < pfile->corpses; ++i ) {
	fwrite( "Corp", 4, 1, fp );
	write_object( fp, pfile->corpses[i]->contents, pfile, true );
      }
      for( int i = 0; i < pfile->caches; ++i ) {
	fwrite( "Cach", 4, 1, fp );
	obj_data *cache = pfile->caches[i];
	fwrite( &((room_data*)cache->array->where)->vnum, sizeof( int ), 1, fp );
	write_object( fp, cache->contents, pfile, true );
      }
      for( int i = 0; i < pfile->auction; ++i ) {
	fwrite( "Auct", 4, 1, fp );
	write_object( fp, pfile->auction[i]->contents, pfile, true );
      }
      fwrite( "End&", 4, 1, fp );
      fclose( fp );
    } else {
      return false;
    }
  }

  return true;
}


void player_data :: Save( bool update )
{
  /* If isn't a character or is a non-playing link, return */
  if( link &&
      link->connected != CON_PLAYING )
    return;

  /* move the old file, so that it is backed up */
  rename_file( PLAYER_DIR, descr->name,
	       PLAYER_PREV_DIR, descr->name );

  FILE *fp;

  /* Open up a file to store the player in. */
  if( !( fp = open_file( PLAYER_DIR, descr->name, "w" ) ) ) 
    return;

  room_data*             room;
  wizard_data*            imm;
  int                    i, j;

  pfile_data *pfile = pcdata->pfile;

  saves.clear( );
  saves.Setup( this );
  saves -= pfile;

  // Temporarily set played time to total.
  const int playtime = played;
  played = time_played( );

  // Temporarily set last on time to current time.
  const int last_on = pfile->last_on;
  if( update ) {
    pfile->last_on = current_time;
  }

  /* If the character is a wizard, save his wizard data */
  if( ( imm = wizard( this ) )
      && pcdata->trust >= LEVEL_AVATAR ) 
    file_wizard( fp, imm ); 

  /* Write the character specific data to the file. */
  file_player( fp, this );
  file_pfile( fp, pfile );
  file_descr( fp, descr );

  // Restore played time to incremental.
  played = playtime;

  // Restore last on time.
  pfile->last_on = last_on;

  /*-- WRITE ROOM --*/
  
  if( ( room = was_in_room ) 
      || ( room = in_room ) ) {
    fwrite( "Room", 4, 1, fp );
    fwrite( &room->vnum, sizeof( int ), 1, fp );
  }
  
  /*-- WRITE ACCOUNT --*/

  if( pfile->account ) { 
    fwrite( "Acnt", 4, 1, fp );
    write_string( fp, pfile->account->name, 's' );
  } 
  
  /*-- WRITE SKILLS --*/

  int k = 0;
  for( j = 0; j < MAX_SKILL_CAT; ++j ) {
    int m = table_max[ skill_table_number[ j ] ];
    for( i = 0; i < m; ++i ) {
      if( shdata->skills[j][i] != UNLEARNT ) 
	++k;
    }
  }

  if( k != 0 ) {
    fwrite( "Skil", 4, 1, fp );
    fwrite( &k, sizeof( int ), 1, fp );
    for( j = 0; j < MAX_SKILL_CAT; ++j ) {
      int m = table_max[ skill_table_number[ j ] ];
      for( i = 0; i < m; ++i ) {
	if( shdata->skills[j][i] != UNLEARNT ) {
	  write_string( fp, skill_entry( j, i )->name, 's' );
	  fwrite( &shdata->skills[j][i], 1, 1, fp );
	}
      }
    } 
  }

  /*-- WRITE ALIASES --*/
  
  if( !alias.is_empty() ) {
    fwrite( "Alas", 4, 1, fp );
    fwrite( &alias.size, sizeof( int ), 1, fp );
    for( i = 0; i < alias; ++i ) {
      write_string( fp, alias[i]->abbrev,  's' );
      write_string( fp, alias[i]->command, 'S' );  
      fwrite( &alias[i]->length, sizeof( char ), 1, fp );
    }
  }

  /*-- WRITE QUESTS --*/

  {
    bool found = false;
    for( int i = 0; i < MAX_QUEST; ++i ) {
      if( quest_data *quest = quest_list[i] ) {
	if( pcdata->quest_flags[i] != QUEST_NONE ) {
	  if( !found ) {
	    fwrite( "Ques", 4, 1, fp );
	    found = true;
	  }
	  fwrite( &i, sizeof( int ), 1, fp );
	  fwrite( &quest->serial, sizeof( int ), 1, fp );
	  fwrite( &pcdata->quest_flags[i], sizeof( int ), 1, fp );
	}
      }
    }
    if( found ) {
      fwrite( &minus_one, sizeof( int ), 1, fp );
    }
  }

  /*-- WRITE RECOGNIZE CODE --*/
  
  for( int i = 0; pcdata->recognize && i < pcdata->recognize->size; ) {
    const int data = pcdata->recognize->list[i];
    if( !get_pfile( data & 0xffff )
	|| ( data >> 16 ) == 0 ) {
      remove( pcdata->recognize, i );
    } else {
      ++i;
    }
  }
  if( Recognize_Data *recognize = pcdata->recognize ) {
    fwrite( "Recg", 4, 1, fp );
    fwrite( &recognize->size, sizeof( int ), 1, fp );
    for( int i = 0; i < recognize->size; ++i ) {
      const int data = recognize->list[i];
      pfile_data *pfile = get_pfile( data & 0xffff );
      fwrite( &data, sizeof( int ), 1, fp );
      fwrite( &pfile->serial, sizeof( int ), 1, fp );
    }
  }
  
  /*-- WRITE PREPARED SPELLS --*/

  if( cast_data *prep = prepare ) {
    fwrite( "Prep", 4, 1, fp );
    i = count( prep );
    fwrite( &i, sizeof( int ), 1, fp );
    for( ; prep; prep = prep->next ) {
      fwrite( &prep->spell, sizeof( int ), 1, fp );
      fwrite( &prep->times, sizeof( int ), 1, fp );
      fwrite( &prep->mana,  sizeof( int ), 1, fp );
    }
  }
  
  /*-- WRITE PETS --*/

  for( i = 0; i < followers.size; ++i ) {
    char_data *pet = followers[i];
    if( is_set( pet->status, STAT_PET ) )
      write_pet( fp, (mob_data*)pet, this );
  }

  /*-- WRITE AFFECTS --*/

  write_affects( fp, this );

  fwrite( "End.", 4, 1, fp );

  fclose( fp );

  /*-- WRITE LOST+FOUND --*/

  write_pfile( pfile, this );

  saves.Save( );

  fixed = false;

  if( pfile->home_modified ) {
    save_html_players( );
  }

  save_time = max( current_time, save_time+60 );
}


static bool read_char( FILE* fp, wizard_data* imm, player_data* pl )
{
  char              tmp  [ TWO_LINES ];
  int              i, j;
  char*          string;
  unsigned char       c;

  *tmp = '\0';
  fread( tmp, 4, 1, fp );

  if( imm && !file_wizard( fp, imm, tmp ) )
    return false;

  if( !file_player( fp, pl, tmp ) )
    return false;

  if( !file_pfile( fp, pl->pcdata->pfile, tmp ) )
    return false;

  pl->pcdata->pfile->last_note = pl->pcdata->pfile->last_on;

  if( !file_descr( fp, pl->descr, tmp ) )
    return false;

  /*-- READ ROOM --*/

  if( !strncmp( tmp, "Room", 4 ) ) {
    fread( &i, sizeof( int ), 1, fp );
    pl->was_in_room = get_room_index( i );
    fread( tmp, 4, 1, fp );
  }

  /*-- READ ACCOUNT -- */

  if( !strncmp( tmp, "Acnt", 4 ) ) {
    read_string( fp, &string, 's', MEM_UNKNOWN );
    if( !( pl->pcdata->pfile->account = find_account( string ) ) ) {
      roach( "Fread_Char: Non-existent account." );
      roach( "--   Ch = %s", pl->pcdata->pfile->name );
      panic( "-- Acnt = %s", string );
    }
    free_string( string, MEM_UNKNOWN );
    fread( tmp, 4, 1, fp );
  }

  /*-- READ SKILLS --*/

  if( !strncmp( tmp, "Skil", 4 ) ) {
    fread( &i, sizeof( int ), 1, fp );
    for( ; i > 0; i-- ) {
      read_string( fp, &string, 's', MEM_UNKNOWN );
      fread( &c, 1, 1, fp );
      if( ( j = skill_index( string ) ) != -1 ) {
        pl->shdata->skills[ skill_table( j ) ][ skill_number( j ) ] = c;
      } else {
	roach( "Fread_Char: Non-existent skill." );
	roach( "--    Ch = %s", pl->pcdata->pfile->name );
	roach( "-- Skill = %s", string );
      }
      free_string( string, MEM_UNKNOWN );
    }
    fread( tmp, 4, 1, fp );
  }
  
  /*-- READ ALIASES --*/

  if( !strncmp( tmp, "Alas", 4 ) ) {
    int s;
    fread( &s, sizeof( int ), 1, fp );
    pl->alias.clear(s);
    pl->alias.size = s;
    for( i = 0; i < s; ++i ) {
      pl->alias[i] = new alias_data( empty_string, empty_string );
      read_string( fp, &pl->alias[i]->abbrev,  's', MEM_ALIAS ); 
      read_string( fp, &pl->alias[i]->command, 'S', MEM_ALIAS );
      fread( &pl->alias[i]->length, sizeof( char ), 1, fp );
    }
    fread( tmp, 4, 1, fp );
  }

  /*-- READ QUESTS --*/

  if( !strncmp( tmp, "Ques", 4 ) ) {
    int vnum, serial, status;
    while( true ) {
      fread( &vnum, sizeof( int ), 1, fp );
      if( vnum == -1 ) {
	fread( tmp, 4, 1, fp );
	break;
      }
      if( vnum < 0 || vnum >= MAX_QUEST ) {
	roach( "Fread_Char: Bad quest data." );
	panic( "--   Ch = %s", pl->pcdata->pfile->name );
      }
      fread( &serial, sizeof( int ), 1, fp );
      fread( &status, sizeof( int ), 1, fp );
      if( quest_data *quest = quest_list[vnum] ) {
	if( quest->serial == serial ) {
	  pl->pcdata->quest_flags[vnum] = status;
	}
      }
    }
  }

  /*-- READ RECOGNIZE CODE --*/

  if( !strncmp( tmp, "Recg", 4 ) ) {
    fread( &i, sizeof( int ), 1, fp );
    int valid[i];
    int k = 0;
    for( int j = 0; j < i; ++j ) {
      int data, serial;
      fread( &data, sizeof( int ), 1, fp );
      fread( &serial, sizeof( int ), 1, fp );
      if( serial < 0 ) {
	roach( "Load_Char: Bad recognize data." );
	return false;
      }
      if( data >> 16 ) {
	pfile_data *pfile = get_pfile( data & 0xffff );
	if( pfile && pfile->serial == serial ) {
	  valid[k++] = data;
	}
      }
    }
    if( k > 0 ) {
      pl->pcdata->recognize = new Recognize_Data( k );
      vcopy( pl->pcdata->recognize->list, valid, k );
    }
    fread( tmp, 4, 1, fp );
  }

  /*-- READ PREPARED SPELLS --*/

  if( !strncmp( tmp, "Prep", 4 ) ) {
    fread( &i, sizeof( int ), 1, fp );
    for( ; i > 0; i-- ) {
      cast_data *prep = new cast_data;
      fread( &prep->spell, sizeof( int ), 1, fp );
      fread( &prep->times, sizeof( int ), 1, fp );
      fread( &prep->mana,  sizeof( int ), 1, fp );
      append( pl->prepare, prep );
    }
    fread( tmp, 4, 1, fp );
  }

  /*-- READ PETS --*/

  while( !strncmp( tmp, "Pet.", 4 ) ) {
    if( !read_pet( fp, pl, tmp ) ) {
      fclose( fp );
      return false;
    }
    fread( tmp, 4, 1, fp );
  }

  /*-- READ AFFECTS --*/

  if( !strncmp( tmp, "Afft", 4 ) ) {
    read_affects( fp, pl );
    fread( tmp, 4, 1, fp );
  }
  
  fclose( fp );

  if( strncmp( tmp, "End.", 4 ) ) {
    roach( "Load_Char: Missing 'End.'." );
    // Causes some issues if tmp[n] == '\0'.
    //    roach( "--  Tmp = '%c%c%c%c'", tmp[0], tmp[1], tmp[2], tmp[3] ); 
    roach( "-- File = '%s'", pl->descr->name );
    return false;
  }

  return true;
}


static void fix( player_data* pl )
{
  if( not_in_range( pl->pcdata->religion, 0, table_max[ TABLE_RELIGION ]-1 ) ) {
    bug( "Fix_Player: Impossible religion." );
    bug( "-- Ch = %s", pl->descr->name );
    pl->pcdata->religion = 0;
  }

  if( not_in_range( pl->movement, -1, table_max[ TABLE_MOVEMENT ]-1 ) ) {
    bug( "Fix_Player: Impossible movement." );
    bug( "-- Ch = %s", pl->descr->name );
    pl->movement = -1;
  }

  if( pl->movement >= 0
      && !movement_table[ pl->movement ].player
      && pl->Level() < LEVEL_BUILDER ) {
    pl->movement = -1;
  }

  const int term = pl->pcdata->terminal;

  if( term != TERM_ANSI ) 
    for( int i = 0; i < MAX_COLOR; ++i )
      if( pl->pcdata->color[i] < 0
	  || pl->pcdata->color[i] >= term_table[ term ].entries )
        pl->pcdata->color[i] = 0;
  
  if( get_language( pl, LANG_PRIMAL + pl->pcdata->speaking ) == 0
      || ( pl->Level() < LEVEL_APPRENTICE
	   && pl->pcdata->speaking == 0 ) )
    pl->pcdata->speaking = pl->Level() >= LEVEL_APPRENTICE
                           ? 0
                           : skill_number( LANG_HUMANIC+pl->shdata->race );
}


bool load_char( link_data* link, const char *name, const char* dir )
{
  char              tmp  [ TWO_LINES ];
  pfile_data*     pfile;
  player_data*       pl;
  wizard_data*   imm  = 0;
  FILE*              fp;

  check_panic( );

  snprintf( tmp, TWO_LINES, "%s%s", dir, name );

  if( !( fp = fopen( tmp, "r" ) ) )
    return false;

  if( !( pfile = find_pfile_exact( name ) ) ) {
    // Called by startup, before pfile list populated.
    pfile = new pfile_data( name );
    imm = new wizard_data( name );
    pl = imm;
  } else if( pfile->trust >= LEVEL_AVATAR ) {
    imm = new wizard_data( name );
    pl = imm;
  } else {
    pl = new player_data( name );
  }
  
  link->player = pl;
  link->character = pl;
  link->pfile = pfile;
  pl->pcdata->pfile = pfile;
  pl->link = link;
  
  if( !read_char( fp, imm, pl ) ) {
    check_panic( );
    snprintf( tmp, TWO_LINES, "%s%s", PLAYER_PREV_DIR, name );
    roach( "Load_Char: File corrupted!" );
    roach( "-- Name: %s", name );
    pl->Extract( );
    if( imm ) {
      imm = new wizard_data( name );
      pl = imm;
    } else {
      pl = new player_data( name );
    }
    link->player = pl;
    link->character = pl;
    pl->pcdata->pfile = pfile;
    pl->link = link;
    if( strcmp( dir, PLAYER_DIR )
	|| !( fp = fopen( tmp, "r" ) )
	|| !read_char( fp, imm, pl ) )
      panic( "Load_Char: Prev file corrupted!" );
    roach( "Load_Char: Prev file intact." );
  }
  
  // Set owner, only if currently null.
  //  set_owner( pfile, pl->contents );  // Done by Obj::To()
  set_owner( pfile, pl->locker );
  set_owner( pfile, pl->wearing ); 

  fix( pl );

  update_maxes( pl );
  calc_resist( pl );
  modify_pfile( pl );

  pl->pcdata->mail = read_mail( pfile );

  if( pl->fixed ) {
    pl->Save( false );
  }

  return true;
}


/*
 *   READ/WRITE OBJECT ROUTINES
 */


static bool file_object( FILE* fp, obj_data* obj, Save_Data *save, char* tmp = empty_string )
{
  obj_clss_data *oc  = obj->pIndexData;

  pfile_data **pfilep = 0;
  if( tmp == empty_string ) {
    if( player_data *pl = dynamic_cast<player_data *>( save ) ) {
      pfilep = &pl->pcdata->pfile;
    }
  }

  int obj_num = obj->Number( );

  save_entry list [] = {
    { "Afte", 'S', 0, &obj->after,       &oc->after        },
    { "Befo", 'S', 0, &obj->before,      &oc->before       },
    { "Cond", 'I', 1, &obj->condition,   0                 },
    { "Cost", 'I', 1, &obj->cost,        &oc->cost         },
    { "Cntn", 'C', 0, &obj->contents,    0                 },
    { "ExFl", 'I', 2, obj->extra_flags,  &oc->extra_flags  },
    { "Labl", 's', 0, &obj->label,       0                 },
    { "Layr", 'I', 1, &obj->layer,       0                 },
    { "Lght", 'I', 1, &obj->light,       &oc->light        },
    { "Mat.", 'I', 1, &obj->materials,   &oc->materials    },
    { "Numb", 'I', 1, &obj_num,          &one              },
    { "Ownr", 'P', 0, &obj->owner,       pfilep            },
    { "Plur", 's', 0, &obj->plural,      &oc->plural       },
    { "Repa", 'I', 1, &obj->age,         0                 },
    { "Rust", 'i', 1, &obj->rust,        &zero             },
    { "Sing", 's', 0, &obj->singular,    &oc->singular     },
    { "Size", 'I', 1, &obj->size_flags,  0                 },
    { "Sour", 's', 0, &obj->source,      &empty_string     },
    { "Time", 'I', 1, &obj->timer,       0                 },
    { "Valu", 'I', 4, obj->value,        0                 },
    { "Wear", 'I', 1, &obj->position,    0                 },
    { "Wght", 'I', 1, &obj->weight,      &oc->weight       },
    { "",     ' ', 0, 0,                 0                 } };

  bool result = file_list( fp, list, save, tmp, MEM_OBJECT );

  if( tmp != empty_string ) {
    obj->Set_Number( obj_num );
    if( obj->owner == (pfile_data*) -1 ) {
      // Owner is invalid.
      obj->owner = 0;
      remove_bit( obj->extra_flags, OFLAG_ONE_OWNER );
    }
  }

  return result;
}


void write_object( FILE* fp, Content_Array& list, Save_Data *save, bool cons )
{
  obj_data* obj;
  
  for( int i = list.size-1; i >= 0; --i ) {
    if( !( obj = object( list[i] ) )
	|| obj->reset
	|| is_set( obj->extra_flags, OFLAG_NOSAVE ) )
      continue;
    
    if( obj->save != save ) {
      if( obj->save ) {
	// Obj already saved in another file.
	obj->save->save_list -= obj;
	saves += obj->save;
      }
      obj->save = save;
      save->save_list += obj;
      if( cons ) {
	consolidate( obj, false );
      }
    }
  }
  
  for( int i = 0; i < list; ++i ) {
    if( !( obj = object( list[i] ) )
	|| obj->reset
	|| is_set( obj->extra_flags, OFLAG_NOSAVE ) )
      continue;
    
    fwrite( "Obj.", 4, 1, fp );
    fwrite( &obj->pIndexData->vnum, sizeof( int ), 1, fp );

    file_object( fp, obj, save );
    write_affects( fp, obj );
    
    fwrite( "End!", 4, 1, fp );
  }
  
  fwrite( "End!", 4, 1, fp );
}


static void null_object( FILE* fp, Content_Array& array, Save_Data *save, char* tmp )
{
  char      buf1  [ ONE_LINE ];
  char      buf2  [ ONE_LINE ];
  char*    text1  = "End!";
  char*    text2  = "Cntn";
  int          i;
  int          j;

  for( i = j = 0; i < 4; i++, j++ ) {
    if( fread( &buf1[i], 1, 1, fp ) != 1 ) 
      panic( "Null_Object: End of File." );

    buf2[j] = buf1[i];
 
    if( buf1[i] != text1[i] )
      i = -1;
    if( buf2[j] != text2[j] )
      j = -1;

    if( j == 3 ) {
      read_object( fp, array, save, tmp );
      i = j = -1;
    }
  }
}


bool read_object( FILE* fp, Content_Array& array, Save_Data *save, char* tmp )
{
  obj_clss_data*    obj_clss;
  int                      i;

  while( true ) {
    *tmp = '\0';
    fread( tmp, 4, 1, fp );

    if( !strncmp( tmp, "End!", 4 ) )
      break;

    if( strncmp( tmp, "Obj.", 4 ) ) { 
      roach( "Read_Obj: Expected word 'Obj.' missing." );
      return false;
    }

    fread( &i, sizeof( int ), 1, fp );

    *tmp = '\0';
    fread( tmp, 4, 1, fp );

    if( !( obj_clss = get_obj_index( i ) ) ) {
      roach( "Read_Obj: Non-existent object type!" );
      roach( "-- Vnum = %d", i );
      //      roach( "--  Loc = %s", Location( ) );
      null_object( fp, array, save, tmp );
      continue;
    }

    obj_data *obj = create( obj_clss );

    if( !file_object( fp, obj, save, tmp ) ) {
      obj->Extract();
      return false;
    }

    if( !strncmp( tmp, "Afft", 4 ) ) {
      read_affects( fp, obj );
      *tmp = '\0';
      fread( tmp, 4, 1, fp );
    }

    if( obj->Number( ) <= 0 ) {
      obj->Extract();
      return false;
    }

    if( strncmp( tmp, "End!", 4 ) ) {
      roach( "Read_Obj: Missing 'End!'." );
      obj->Extract();
      return false;
    }

    if( save ) {
      obj->save = save;
      save->save_list += obj;
    }

    obj->Select_All( );

    // Prevent counting loaded objects.
    const int boot_save = boot_stage;
    boot_stage = 1;
    obj->To( array );
    boot_stage = boot_save;

    if( save ) {
      for( oprog_data *oprog = obj->pIndexData->oprog; oprog; oprog = oprog->next ) {
	if( oprog->trigger == OPROG_TRIGGER_LOAD ) {
	  push( );
	  clear_variables( );
	  var_obj = obj;
	  var_ch = dynamic_cast<player_data*>( save );
	  var_room = dynamic_cast<room_data*>( save );
	  oprog->execute( );
	  pop( );
	  save->fixed = true;
	}
      }
    }
  }

  return true;
}


void pfile_data :: Save( bool )
{
  saves.clear( );
  saves.Setup( this );

  if( write_pfile( this ) ) {
    saves.Save( );
  }

  fixed = false;  
}


/*
 *   DATA FILE SAVE ROUTINES
 */


void save_mobs( )
{
  rename_file( AREA_DIR, MOB_FILE,
	       AREA_PREV_DIR, MOB_FILE );

  FILE *fp;

  if( !( fp = open_file( AREA_DIR, MOB_FILE, "w" ) ) )
    return;

  descr_data*      descr;
  mprog_data*      mprog;
  species_data*  species;
  share_data*     shdata;

  fprintf( fp, "#MOBILES\n" );

  for( int i = 1; i <= species_max; ++i ) {
    if( !( species = species_list[i] ) )
      continue;

    shdata = species->shdata;
    descr  = species->descr;

    fprintf( fp, "\n#%d\n", species->vnum );
    fwrite_string( fp, descr->name );
    fwrite_string( fp, descr->keywords );
    fwrite_string( fp, descr->singular );
    fwrite_string( fp, descr->prefix_s );
    fwrite_string( fp, descr->adj_s );
    fwrite_string( fp, descr->long_s );
    fwrite_string( fp, descr->plural );
    fwrite_string( fp, descr->prefix_p );
    fwrite_string( fp, descr->adj_p );
    fwrite_string( fp, descr->long_p );
    fwrite_string( fp, descr->complete );
    fwrite_string( fp, species->creator );
    fwrite_string( fp, species->comments );
    fwrite_string( fp, species->last_mod );

    fwrite_string( fp, species->attack->Code( ) );

    write_extras( fp, species->attack->Extra_Descr( ) );
    fprintf( fp, "!\n" );
    
    fprintf( fp, "%d %d %d %d %d %d %d %d\n",
	     species->nation, species->group, shdata->race,
	     species->adult, species->maturity,
	     species->skeleton, species->zombie, species->corpse );

    fprintf( fp, "%d %d %d %d %d %d %d %d\n",
	     species->price, shdata->kills, shdata->deaths,
	     species->wander, species->date, species->light,
	     species->color, species->movement );

    fprintf( fp, "%d %d\n",
	     species->act_flags[0], species->act_flags[1] );

    fprintf( fp, "%d %d %d\n",
	     species->affected_by[0],
	     species->affected_by[1],
	     species->affected_by[2] );

    fprintf( fp, "%d %d\n", shdata->alignment, shdata->level );

    fprintf( fp, "%d %d %d %d %d\n",
	     shdata->strength, shdata->intelligence,
	     shdata->wisdom, shdata->dexterity,
	     shdata->constitution );

    for( int j = 0; j < MAX_RESIST; ++j ) {
      fprintf( fp, "%d%c",
	       shdata->resist[j],
	       j == MAX_RESIST-1 ? '\n' : ' ' );
    }

    for( int j = 0; j < MAX_SKILL_CAT; ++j ) {
      const int m = table_max[ skill_table_number[ j ] ];
      for( int k = 0; k < m; ++k ) {
	if( shdata->skills[j][k] > UNLEARNT ) {
	  fprintf( fp, "%d ", (int)shdata->skills[j][k] );
	  fwrite_string( fp, skill_entry( j, k )->name );
	}
      }
    }
    fprintf( fp, "-1\n" );

    for( int j = 0; j < MAX_ARMOR; j++ ) {
      fprintf( fp, "%d %d ",
	       species->chance[j], species->armor[j] );
      fwrite_string( fp, species->part_name[j] );
    }
    fprintf( fp, "%d\n", species->wear_part );

    fprintf( fp, "%d %d\n", species->hitdice, species->movedice );
    fprintf( fp, "%d %d %d %d %d\n", species->damage, species->rounds,
	     species->special, species->damage_taken, species->exp );
    fprintf( fp, "%c %d %d %d\n",
	     toupper( sex_name[species->sex][0] ),
	     species->gold, species->size, species->weight );

    write( fp, species->reset );
    write_extras( fp, species->extra_descr );

    for( mprog = species->mprog; mprog; mprog = mprog->next ) {
      fprintf( fp, "%d %d\n", mprog->trigger, mprog->value );
      fwrite_string( fp, mprog->string );
      mprog->write( fp );
      //      fwrite_string( fp, mprog->Code( ) );
      //      write_extras( fp, mprog->Extra_Descr( ) );
      fprintf( fp, "!\n" );
    }

    fprintf( fp, "-1\n" );   
  }     

  fprintf( fp, "#0\n\n" );
  fprintf( fp, "#$\n" );
  fclose( fp );

  Species_Data::modified = -1;
}


void save_objects( )
{
  obj_clss_data*  obj_clss;

  rename_file( AREA_DIR, OBJECT_FILE,
	       AREA_PREV_DIR, OBJECT_FILE );

  FILE *fp;

  if( !( fp = open_file( AREA_DIR, OBJECT_FILE, "w" ) ) ) 
    return;

  fprintf( fp, "#OBJECTS\n" );

  for( int i = 1; i <= obj_clss_max; ++i ) {
    if( !( obj_clss = obj_index_list[i] ) )
      continue;

    fprintf( fp, "\n#%d\n",    obj_clss->vnum );

    fwrite_string( fp,      obj_clss->singular );
    fwrite_string( fp,      obj_clss->plural );
    fwrite_string( fp,      obj_clss->before );
    fwrite_string( fp,      obj_clss->after );
    fwrite_string( fp,      obj_clss->long_s );
    fwrite_string( fp,      obj_clss->long_p );
    fwrite_string( fp,      obj_clss->prefix_singular );
    fwrite_string( fp,      obj_clss->prefix_plural );
    fwrite_string( fp,      obj_clss->creator );
    fwrite_string( fp,      obj_clss->comments );
    fwrite_string( fp,      obj_clss->last_mod );

    fprintf( fp, "%d %d %d %d %d %d\n",
	     obj_clss->item_type,         obj_clss->fakes,
	     obj_clss->extra_flags[0],    obj_clss->extra_flags[1],
	     obj_clss->wear_flags,        obj_clss->anti_flags );
    
    fprintf( fp, "%d %d %d %d %d %d %d\n",
	     obj_clss->restrictions,      obj_clss->size_flags,
	     obj_clss->materials,
	     obj_clss->affect_flags[0],   obj_clss->affect_flags[1],
	     obj_clss->affect_flags[2],   obj_clss->layer_flags );
    
    fprintf( fp, "%d %d %d %d\n",
	     obj_clss->value[0], obj_clss->value[1],
	     obj_clss->value[2], obj_clss->value[3] );
    
    fprintf( fp, "%d %d %d %d %d %d %d %d %d\n",
	     obj_clss->weight,
	     obj_clss->cost, obj_clss->level, obj_clss->remort, obj_clss->limit,
	     obj_clss->repair, obj_clss->durability, obj_clss->blocks,
	     obj_clss->light ); 
    
    fprintf( fp, "%d\n", (int)( obj_clss->date ) );
    
    write_affects( fp, obj_clss );
    write_extras( fp, obj_clss->extra_descr );

    fprintf( fp, "P\n" );
    for( oprog_data *oprog = obj_clss->oprog; oprog; oprog = oprog->next ) {
      fprintf( fp, "%d %d %d\n",
	       oprog->trigger,
	       oprog->obj_act ? oprog->obj_act->vnum : -1,
	       oprog->flags );
      fwrite_string( fp, oprog->command );
      fwrite_string( fp, oprog->target );
      oprog->write( fp );
      //      fwrite_string( fp, oprog->Code( ) );
      //      write_extras( fp, oprog->Extra_Descr( ) );
      fprintf( fp, "!\n" );
    }

    fprintf( fp, "-1\n" );   
  }  

  fprintf( fp, "#0\n\n" );
  fprintf( fp, "#$\n" );
  fclose( fp );

  Obj_Clss_Data::modified = -1;
}


bool save_world( )
{
  rename_file( FILES_DIR, WORLD_FILE,
	       FILES_PREV_DIR, WORLD_FILE );
  
  FILE *fp;

  if( !( fp = open_file( FILES_DIR, WORLD_FILE, "w" ) ) ) 
    return false;

  fprintf( fp, "#WORLD\n\n" );

  fprintf( fp, "%lu\n", weather.tick );

  for( int i = 0; i < WEATHER_FRONTS; ++i ) {
    fprintf( fp, "%lu %lu %d %d %d %d %d %d\n",
	     weather.front[i].start, weather.front[i].duration,
	     weather.front[i].humidity_prev, weather.front[i].humidity,
	     weather.front[i].temperature_prev, weather.front[i].temperature,
	     weather.front[i].clouds_prev, weather.front[i].clouds );
  }

  fprintf( fp, "%d\n", record_players );
  
  for( int i = 0; i < MAX_VOTE; ++i ) {
    fprintf( fp, "%d\n", votes[i].serial );
    fwrite_string( fp, votes[i].text );
  } 
  
  fprintf( fp, "#$\n" );
  fclose( fp );

  return true;
}


void load_world( )
{
  echo( "Loading World State ...\n\r" );

  FILE *fp = open_file( FILES_DIR, WORLD_FILE, "r", true );

  if( strcmp( fread_word( fp ), "#WORLD" ) ) 
    panic( "Load_World: header not found." );

  int n = fread_number( fp );
  if( n < 0 )
    panic( "Load_World: bad current time value." );

  weather.set_tick( n );

  for( int i = 0; i < WEATHER_FRONTS; ++i ) {
    weather.front[i].start = fread_unsigned( fp );
    weather.front[i].duration = fread_unsigned( fp );
    weather.front[i].humidity_prev = fread_number( fp );
    weather.front[i].humidity = fread_number( fp );
    weather.front[i].temperature_prev = fread_number( fp );
    weather.front[i].temperature = fread_number( fp );
    weather.front[i].clouds_prev = fread_number( fp );
    weather.front[i].clouds = fread_number( fp );
  }

  n = fread_number( fp );
  if( n < 0 )
    panic( "Load_World: bad record_players value." );

  record_players = n;

  for( int i = 0; i < MAX_VOTE; ++i ) {
    n = fread_number( fp );
    if( n < 0 )
      panic( "Load_World: bad vote serial number." );
    votes[i].serial = n;
    Vote_Data::max_serial = max( Vote_Data::max_serial, n+1 );
    free_string( votes[i].text, MEM_VOTE );
    votes[i].text = fread_string( fp, MEM_VOTE );
  }
  
  if( strcmp( fread_word( fp ), "#$" ) ) 
    panic( "Load_World: terminator not found." );
  
  fclose( fp );
}
