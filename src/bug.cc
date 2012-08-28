#include <ctype.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <unistd.h>
#include "define.h"
#include "struct.h"


/*
 *   BUG ROUTINES
 */


static void mob_bugs( char_data* ch, species_data* species, bool& found,
		      bool make )
{
  bool compile = make && !species->attack->binary;

  if( compile ) {
    species->attack->compile( );
  }
  
  if( species->attack->corrupt ) {
    found = true;
    page( ch, "  Mob #%d (%s) attack program is not compiling.\n\r",
	  species->vnum, species ); 
  } else if( compile && !species->attack->binary ) {
    found = true;
    page( ch, "  Mob #%d (%s) has no attack program.\n\r",
	  species->vnum, species ); 
  }

  if( compile ) {
    species->attack->decompile( );
  }

  int i = 1;
  bool desc = false;

  for( mprog_data *mprog = species->mprog; mprog; i++, mprog = mprog->next ) {
    bool compile = make && !mprog->binary;
    if( compile ) {
      mprog->compile( );
    }
    if( mprog->corrupt ) {
      found = true;
      page( ch, "  Mprog #%d on mob #%d (%s) is not compiling.\n\r",
	    i, species->vnum, species ); 
    }
    if( compile ) {
      mprog->decompile( );
    }
    if( mprog->trigger == MPROG_TRIGGER_DESCRIBE )
      desc = true;
  }

  if( !desc && !*species->descr->complete ) {
    page( ch, "  Mob #%d (%s) has no description.\n\r", species->vnum, species );
  }

  if( !is_set( species->act_flags, ACT_HUMANOID ) ) {
    // Check marmor table.
    int max = 0;
    int i;
    for( i = 0; i < MAX_ARMOR; ++i ) {
      if( species->chance[i] <= max ) {
	page( ch, "  Mob #%d (%s) marmor entry #%d has non-increasing chance entry.\n\r",
	      species->vnum, species, i+1 );
      } else {
	max = species->chance[i];
      }
      if( species->chance[i] < 1 || species->chance[i] > 1000 ) {
	page( ch, "  Mob #%d (%s) marmor entry #%d has out-of-range chance entry.\n\r",
	      species->vnum, species, i+1 );
      }
      if( species->chance[i] == 1000 ) {
	break;
      }
    }
    if( i == MAX_ARMOR ) {
      page( ch, "  Mob #%d (%s) marmor table has no 1000 chance entry.\n\r",
	    species->vnum, species );
    }
  }
}


void obj_bugs( char_data* ch, obj_clss_data* obj, bool& found,
	       bool make )
{
  int i = 1;
  bool desc = false;
  
  for( oprog_data *oprog = obj->oprog; oprog; i++, oprog = oprog->next ) {
    bool compile = make && !oprog->binary;
    if( compile ) {
      oprog->compile( );
    }
    if( oprog->corrupt ) {
      found = true;
      page( ch, "  Oprog #%d on obj #%d (%s) is not compiling.\n\r",
	    i, obj->vnum, obj ); 
    }
    if( compile ) {
      oprog->decompile( );
    }
    if( oprog->trigger == OPROG_TRIGGER_DESCRIBE )
      desc = true;
  }
  
  if( !desc ) {
    bool           either  = false;
    bool            after  = false;
    bool           before  = false;
    const char*   keyword;
    
    for( int i = 0; i < obj->extra_descr; i++ ) {
      keyword = obj->extra_descr[i]->keyword;     
      either |= !strcasecmp( keyword, "either" );
      after  |= !strcasecmp( keyword, "after" );
      before |= !strcasecmp( keyword, "before" );
    }
    
    if( !either &&
	( !after ||
	  ( !before && ( obj->fakes == 0
			 || obj->fakes == obj->vnum ) ) ) ) {
      page( ch, "  Object #%-4d (%s) missing description oextra.\n\r",
	    obj->vnum, obj->Name( ) );
      found = true;
    }
  }

  if( is_set( obj->extra_flags, OFLAG_NOSHOW )
      && is_set( obj->wear_flags, ITEM_TAKE ) ) {
    page( ch, "  Object #%-4d (%s) takeable and 'no.show'.\n\r",
	  obj->vnum, obj->Name( ) );
    found = true;
  }

  if( obj->fakes && obj->fakes != obj->vnum ) {
    obj_clss_data *fake = get_obj_index( obj->fakes );
    if( !fake ) {
      page( ch, "  Object #%-4d (%s) fake object %d nonexistent.\n\r",
	    obj->vnum, obj->Name( ), obj->fakes );
      found = true;
    } else if( is_set( obj->extra_flags, OFLAG_THE_BEFORE )
	       != is_set( fake->extra_flags, OFLAG_THE_BEFORE ) ) {
      page( ch, "  Object #%-4d (%s) fake object %d has non-matching oflag %s.\n\r",
	    obj->vnum, obj->Name( ), obj->fakes, oflag_name[OFLAG_THE_BEFORE] );
      found = true;
    } else if( is_set( obj->extra_flags, OFLAG_THE_AFTER )
	       != is_set( fake->extra_flags, OFLAG_THE_AFTER ) ) {
      page( ch, "  Object #%-4d (%s) fake object %d has non-matching oflag %s.\n\r",
	    obj->vnum, obj->Name( ), obj->fakes, oflag_name[OFLAG_THE_AFTER] );
      found = true;
    } else if( strcmp( obj->prefix_singular, fake->prefix_singular ) ) {
      page( ch, "  Object #%-4d (%s) fake object %d has non-matching prefix_s.\n\r",
	    obj->vnum, obj->Name( ), obj->fakes );
      found = true;
    } else if( strcmp( obj->prefix_plural, fake->prefix_plural ) ) {
      page( ch, "  Object #%-4d (%s) fake object %d has non-matching prefix_p.\n\r",
	    obj->vnum, obj->Name( ), obj->fakes );
      found = true;
    } else if( strcmp( separate( obj->singular, false ), separate( fake->singular, false ) ) ) {
      page( ch, "  Object #%-4d (%s) fake object %d has non-matching un-identified singular.\n\r",
	    obj->vnum, obj->Name( ), obj->fakes );
      found = true;
    } else if( strcmp( separate( obj->plural, false ), separate( fake->plural, false ) ) ) {
      page( ch, "  Object #%-4d (%s) fake object %d has non-matching un-identified plural.\n\r",
	    obj->vnum, obj->Name( ), obj->fakes );
      found = true;
    }
  }
}


void room_bugs( char_data* ch, room_data* room, bool& found,
		bool make )
{
  if( midair( 0, room ) ) {
    exit_data *exit = exit_direction( room, DIR_DOWN );
    if( !exit ) {
      page( ch, "  Air room %d has no downward exit.\n\r",
	    room->vnum );
      found = true;
    } else {
      if( is_set( exit->exit_info, EX_ISDOOR ) ) {
	page( ch, "  Air room %d downward exit has %s dflag set.\n\r",
	      room->vnum, dflag_name[EX_ISDOOR] );
	found = true;
      }
      if( is_set( exit->exit_info, EX_CLOSED ) ) {
	page( ch, "  Air room %d downward exit has %s dflag set.\n\r",
	      room->vnum, dflag_name[EX_CLOSED] );
	found = true;
      }
      if( is_set( exit->exit_info, EX_NO_MOB ) ) {
	page( ch, "  Air room %d downward exit has %s dflag set.\n\r",
	      room->vnum, dflag_name[EX_NO_MOB] );
	found = true;
      }
      if( exit->size < room->size ) {
	page( ch, "  Air room %d downward exit size is smaller than room size.\n\r",
	      room->vnum );
	found = true;
      }
      room_data *to = exit->to_room;
      if( is_set( to->room_flags, RFLAG_NO_MOB ) ) {
	page( ch, "  Air room %d downward room %d has %s rflag set.\n\r",
	      room->vnum, to->vnum, rflag_name[RFLAG_NO_MOB] );
	found = true;
      }
      if( is_set( to->room_flags, RFLAG_NO_MOUNT ) ) {
	page( ch, "  Air room %d downward room %d has %s rflag set.\n\r",
	      room->vnum, to->vnum, rflag_name[RFLAG_NO_MOUNT] );
	found = true;
      }
      if( to->size < room->size ) {
	page( ch, "  Air room %d downward room %d size is smaller than room size.\n\r",
	      room->vnum, to->vnum );
	found = true;
      }
      if( is_submerged( 0, to ) ) {
	page( ch, "  Air room %d downward room %d is underwater terrain.\n\r",
	      room->vnum, to->vnum );
	found = true;
      }
      if( room->area != to->area
      	  && to->area->status != AREA_OPEN ) {
	page( ch, "  Air room %d downward room %d is not open to players.\n\r",
	      room->vnum, to->vnum );
	found = true;
      }
    }
  }

  for( int i = 0; i < room->exits; ++i ) {
    exit_data *exit = room->exits[i];
    if( exit_data *exit2 = reverse( exit ) ) {
      if( exit->size != exit2->size
	  && room->vnum < exit->to_room->vnum ) {
	page( ch, "  Room %d exit %s size is inconsistent.\n\r",
	      room->vnum, dir_table[exit->direction].name );
	found = true;
      }
    }
  }

  action_data*  action;
  int             i, j;

  for( i = 1, action = room->action; action; i++, action = action->next ) {
    bool compile = make && !action->binary;
    if( compile ) {
      action->compile( );
    }
    if( action->corrupt ) {      
      found = true;
      page( ch, "  Acode #%d in room %d is not compiling.\n\r",
	    i, room->vnum );
    }
    if( compile ) {
      action->decompile( );
    }
    if( action->trigger == TRIGGER_ENTERING
	|| action->trigger == TRIGGER_LEAVING ) {
      for( j = 0; j < MAX_DIR; ++j )
        if( is_set( action->flags, j ) )
          break;
      if( j == MAX_DOOR ) {
        found = true;
        page( ch,
	      "  Acode #%d in room %d has no direction flag checked.\n\r",
	      i, room->vnum );
      }
    }
  }
}


void do_bugs( char_data* ch, const char *argument )
{
  area_data*          area;
  reset_data*        reset;
  room_data*          room;
  species_data*    species;
  obj_clss_data*       obj;
  bool               found  = false;
  int                    i;
  int                flags;

  if( !get_flags( ch, argument, &flags, "omtrac", "Bugs" ) )
    return;

  if( ( flags & 0x1f ) == 0 )
    flags = 0xf;

  bool compile = is_set( flags, 5 );

  page( ch, "Bugs Found:\n\r" );

  /*-- OBJECTS --*/

  if( is_set( flags, 0 ) ) {
    for( i = 1; i <= obj_clss_max; ++i ) {
      if( ( obj = obj_index_list[i] ) ) {
        obj_bugs( ch, obj, found, compile );
      }
    }
  }

  /*-- TRAINERS --*/

  if( is_set( flags, 2 ) ) {
    for( const trainer_data *trainer = trainer_list; trainer; trainer = trainer->next ) {
      if( !trainer->room ) {
        page( ch, "  Trainer nowhere??\n\r" );
        found = true;
        continue;
      }
      if( !( species = get_species( trainer->trainer ) ) ) {
        page( ch, "  Trainer in room #%d with non-existent species.\n\r", 
	      trainer->room->vnum );
        found = true;
        continue;
      } 
      for( reset = trainer->room->reset; reset; reset = reset->next )
        if( reset->vnum == trainer->trainer
	    && is_set( reset->flags, RSFLAG_MOB ) )
          break;
      if( !reset ) {
        page( ch,
	      "  Trainer entry in room %d for mob %d with no reset.\n\r",
	      trainer->room->vnum, trainer->trainer ); 
        found = true;
      }
    }
  }
  
  /*-- MOBILE BUGS --*/
  
  if( is_set( flags, 1 ) ) {
    for( i = 1; i <= species_max; ++i ) {
      if( ( species = species_list[i] ) ) {
        mob_bugs( ch, species, found, compile );
      }
    }
  }

  /*-- AREAS --*/

  if( is_set( flags, 4 ) ) {
    for( area = area_list; area; area = area->next ) {
      if( area->status == AREA_OPEN ) {
	bool loaded = area->act_loaded;
        for( room = area->room_first; room; room = room->next ) {
          room_bugs( ch, room, found, compile );
	}
	if( !loaded && area->act_loaded ) {
	  area->clear_actions( );
	}
      }
    }
  }

  if( is_set( flags, 3 ) ) {
    bool loaded = ch->in_room->area->act_loaded;
    for( room = ch->in_room->area->room_first; room; room = room->next ) {
      room_bugs( ch, room, found, compile );
    }
    if( !loaded && ch->in_room->area->act_loaded ) {
      ch->in_room->area->clear_actions( );
    }
  }

  if( !found )
    page( ch, "  none\n\r" );
}
