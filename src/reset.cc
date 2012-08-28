#include <ctype.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include "define.h"
#include "struct.h"


/*
 *   CONSTANTS
 */


static const char* rsflags_mob [] = {
  "Leader",
  "Follower",
  "Sentinel",
  "Night",
  "Day",
  "Aggressive",
  "Mount",
  "Chair",
  "Peaceful",
  ""
};

static const char* rsflags_obj [] = {
  "Container",
  "Inside",
  "Overload",
  "Hidden",
  ""
};

static const char* reset_pos_name [] = {
  "sleeping", "meditating",
  "resting", "standing" }; 

const char* reset_pos_abbrev [] = {
  "Slee", "Medi",
  "Rest", "Stan" };

const int rspos_index [] = {
  POS_SLEEPING, POS_MEDITATING,
  POS_RESTING, POS_STANDING };


/*
 *   LOCAL FUNCTIONS
 */


void    extract         ( reset_data* );

void    display         ( char_data*, reset_data*, int ); 

static void place       ( obj_data*, reset_data*, room_data*,
			  mob_data*, obj_data*&, obj_data*& );
static void reset_mob   ( reset_data*, room_data*, mob_data*&, char_data*&, obj_data*& );
static void reset_obj   ( reset_data*, room_data*, mob_data*, obj_data*&, obj_data*& );
static void reset_table ( reset_data*, int, room_data*, mob_data*,
			  obj_data*&, obj_data*& );


int rtable_calls;


/*
 *   RESET_DATA CLASS
 */


Reset_Data :: Reset_Data( int num )
  : next(0), vnum(num), flags(1 << RSFLAG_REROLL),
    chances(100 + ( 90 << 24 )), value(0),
    count(0), liquid(LIQ_WATER)
{
  record_new( sizeof( reset_data ), MEM_RESET );
}


Reset_Data :: Reset_Data( const Reset_Data& copy )
  : next(0), vnum( copy.vnum ), flags( copy.flags ),
    chances( copy.chances ), value( copy.value ),
    count( 0 ), liquid( copy.liquid )
{
}


Reset_Data :: ~Reset_Data( )
{
  record_delete( sizeof( reset_data ), MEM_RESET );
}


/*
 *   SUPPORT ROUTINES
 */


void extract( reset_data* reset )
{
  for( int i = 0; i < mob_list; i++ )
    if( mob_list[i]->reset == reset )
      mob_list[i]->reset = 0;

  for( int i = 0; i < obj_list; i++ )
    if( obj_list[i]->reset == reset )
      obj_list[i]->reset = 0;

  /*
  if( is_set( reset->flags, RSFLAG_MOB ) ) {
    if( species_data *species = get_species( reset->vnum ) ) {
      --species->reset_count;
    }
  }
  */

  delete reset;
}


const char *name( const reset_data *reset )
{
  if( !reset )
    return "none";

  return "somewhere";
}


/*
 *   ONLINE EDITING COMMAND
 */


void do_reset( char_data* ch, const char *argument )
{
  wizard_data*     imm;

  if( !( imm = wizard( ch ) ) )
    return;
  
  int flags;
  if( !get_flags( ch, argument, &flags, "f", "reset" ) )
    return;

  if( !strcasecmp( argument, "room" ) ) {
    reset_room( ch->in_room );
    ch->in_room->Save();
    send( ch, "Room reset.\n\r" );
    return;
  }
  
  if( !strcasecmp( argument, "shop" ) ) {
    if( mob_data *keeper = active_shop( ch ) ) {
      reset_shop( keeper );
      send( ch, "Shop reset.\n\r" );
    } else {
      send( ch, "There is no shop here.\n\r" );
    }
    return;
  }
  
  if( !strcasecmp( argument, "area" ) ) {
    area_data *area = ch->in_room->area;
    area->update_forage( );
    for( room_data *room = area->room_first; room; room = room->next ) {
      if( is_set( flags, 0 ) || !player_in_room( room ) ) {
	reset_room( room );
	room->Save();
      }
    }
    area->age = number_range( 0, 3 );
    send( ch, "Area reset.\n\r" );
    return;
  }
  
  edit_reset( ch, argument, ch->in_room->vnum, ch->in_room->reset, RST_ROOM );
}


void do_mreset( char_data* ch, const char *argument )
{
  species_data*  species;
  wizard_data*       imm;

  if( !( imm = wizard( ch ) ) )
    return;
  
  if( !( species = imm->mob_edit ) ) {
    send( ch, "You aren't editing any mob.\n\r" );
    return;
  }

  edit_reset( ch, argument, species->vnum, species->reset, RST_MOB );
}


static void set_rsflags( char_data* ch, reset_data* reset, const char *& argument )
{
  const char *const *flags = ( is_set( reset->flags, RSFLAG_MOB )
			       ? rsflags_mob : rsflags_obj );

  while( *argument == '+' || *argument == '-' ) {
    bool set = ( *argument++ == '+' );

    for( ; *argument && *argument != ' '; ++argument ) {
      if( *argument >= '1' && *argument <= '3' ) {
	int i = *argument - '1';
	assign_bit( reset->flags, RSFLAG_REROLL+i, set );
	fsend( ch, "Reroll bit %d set %s.", i+1,
	       true_false( &reset->flags, RSFLAG_REROLL+i ) );

      } else {
	int i = 0;
	for( ; *flags[i]; ++i ) {
	  if( toupper( *argument ) == *flags[i] ) {
	    assign_bit( reset->flags, RSFLAG_LEADER+i, set );
	    fsend( ch, "%s flag set %s.", flags[i],
		   true_false( &reset->flags, RSFLAG_LEADER+i ) );
	    break;
	  }
	}
	if( !*flags[i] ) { 
	  send( ch, "Unknown flag -- %c\n\r", *argument );
	}
      }
    }
    
    skip_spaces( argument );
  }
}


static void reset_log( char_data *ch, int num, int type,
		       const char *text,
		       int vnum = -1, const char *name = empty_string )
{
  fsend( ch, text, vnum, name );

  if( type == RST_ROOM ) {
    room_log( ch, num, text, vnum, name );
  } else if( type == RST_MOB ) {
    mob_log( ch, num, text, vnum, name );
  }
}


static void modify_reset( char_data* ch, int num, reset_data* reset, int type, const char *argument )
{
  set_rsflags( ch, reset, argument );

  if( !*argument )
    return; 

  int *chances = unpack_int( reset->chances ); 

  if( is_set( reset->flags, RSFLAG_MOB ) ) {
    // Mob reset in a room.

#define rpn( i )   reset_pos_name[i]
    class type_field type_list[] = {
      { "Position",  MAX_RESET_POS,  &rpn(0),  &rpn(1), &reset->value, true },
      { "" }
    };
#undef rpn

    if( process( type_list, ch, "reset", argument ) )
      return;

    if( matches( argument, "Vnum" ) ) {    
      
      species_data *old_species = get_species( reset->vnum );

      if( !*argument ) {
	send( ch, "Set Vnum of reset to?\n\r[ Current value: %d (%s) ]\n\r",
	      reset->vnum, old_species );
	return;
      }
      
      int i;
      mob_data *mob;
      species_data *species;
      if( !number_arg( argument, i ) ) {
	if( !( mob = one_mob( ch, argument, "Vnum",
			      &ch->contents,
			      ch->array ) ) ) {
	  return;
	}
	species = mob->species;
	i = species->vnum;
      } else {
	species = get_species( i );
      }
      
      if( !species ) {
	fsend( ch, "Vnum must be a valid mob or mob vnum." );
	return;
      }
      
      if( i == reset->vnum ) {
	fsend( ch, "Vnum on reset is already %d (%s).",
	       i, species );
	return;
      }
      
      reset_log( ch, num, type, "Reset vnum changed from mob [%d] %s,",
		 reset->vnum, get_species( reset->vnum )->Name( ) );
      reset_log( ch, num, type, "  to mob [%d] %s.",
		 i, species->Name( ) );

      //      --old_species->reset_count;
      //      ++species->reset_count;

      reset->vnum = i;
      return;
    }

  } else {
    // Object reset of some kind.
    if( is_set( reset->flags, RSFLAG_OBJECT ) ) {
      // Object in reset or mreset or rtable.
      // Has rust, liquid, obj vnum fields.
      class int_field int_list[] = {
	{ "Rust",              0,  100,  &chances[3]     },
	{ "",                  0,    0,  0               }
      };
      
      if( process( int_list, ch, "reset", argument ) ) {
	reset->chances = pack_int( chances ); 
	return;
      }
      
      // Separate variable for older compilers.
      const int dim = table_max[ TABLE_LIQUID ] + 1;
      Liquid_Type liq_tab_none [ dim ];
      liq_tab_none[0].name = "empty";
      for( int i = 0; i < table_max[ TABLE_LIQUID ]; ++i ) {
	liq_tab_none[i+1].name = liquid_table[i].name;
      }

      ++reset->liquid;
      
#define ltn( i )   liq_tab_none[i].name
      class type_field type_list[] = {
	{ "liquid",    table_max[ TABLE_LIQUID ]+1,  &ltn(0),  &ltn(1),  &reset->liquid, true  },
	{ "" }
      }; 
#undef ltn
     
      const bool flag = process( type_list, ch, "reset", argument );
      
      --reset->liquid;

      // Prevent free_string() on table copy entries.
      for( int i = 0; i <= table_max[ TABLE_LIQUID ]; ++i ) {
	liq_tab_none[i].name = 0;
      }
      if( flag ) 
	return;
      
      if( matches( argument, "Vnum" ) ) {    
	
	if( !*argument ) {
	  obj_clss_data *clss = get_obj_index( reset->vnum );
	  send( ch, "Set Vnum of reset to?\n\r[ Current value: %d (%s) ]\n\r",
		reset->vnum, clss );
	  return;
	}
	
	int i;
	obj_data *obj;
	obj_clss_data *clss;
	if( !number_arg( argument, i ) ) {
	  if( !( obj = one_object( ch, argument, "Vnum",
				   &ch->contents,
				   ch->array ) ) ) {
	    return;
	  }
	  clss = obj->pIndexData;
	  i = clss->vnum;
	} else {
	  clss = get_obj_index( i );
	  if( !clss ) {
	    fsend( ch, "Vnum must be a valid object or object vnum." );
	    return;
	  }
	}
	
	if( i == reset->vnum ) {
	  fsend( ch, "Vnum on reset is already %d (%s).",
		 i, clss );
	  return;
	}
	
	reset_log( ch, num, type, "Reset vnum changed from obj [%d] %s,",
		   reset->vnum, get_obj_index( reset->vnum )->Name( ) );
	reset_log( ch, num, type, "  to obj [%d] %s.",
		   i, clss->Name( ) );
	
	reset->vnum = i;
	return;
      }

    } else {
      // Table reset, has a table number.

      ++reset->vnum;

      class int_field int_list[] = {
	{ "Table",             1,  num_rtable,  &reset->vnum },
	{ "",                  0,           0,  0            }
      };
      
      const bool flag = process( int_list, ch, "reset", argument );
      
      --reset->vnum;

      if( flag )
	return;
    }

    if( type != RST_TABLE ) {
      // Not in a table, has position field.
      const char **pos_names = ( type == RST_MOB )
	? reset_skin_name
	: reset_wear_name;

      reset->value += 2;

#define rwn( i )   pos_names[i]
      class type_field type_list[] = {
 	{ "position",  MAX_WEAR+2,   &rwn(0),  &rwn(1),  &reset->value, true   },
	{ "" }
      };
#undef rwn

      const bool flag = process( type_list, ch, "reset", argument );

      reset->value -= 2;

      if( flag ) 
	return;
    }
  }

  class int_field int_list[] = {
    { "1_Chance",          0,  100,  &chances[0]     },
    { "2_Chance",          0,  100,  &chances[1]     },
    { "3_Chance",          0,  100,  &chances[2]     },
    { "",                  0,    0,  0               }
  };

  if( process( int_list, ch, "reset", argument ) ) {
    reset->chances = pack_int( chances ); 
    return;
  }

  send( ch, "Unknown Field - See help reset.\n\r" );
}


void edit_reset( char_data* ch, const char *argument,
		 int num, reset_data*& list, int type,
		 const char* name )
{
  if( !*argument ) {
    if( name != empty_string ) {
      page_centered( ch, "-=- %s -=-", name );
      page( ch, "\n\r" );
    }
    display( ch, list, type );
    return;
  }
  
  reset_data*        reset;
  reset_data*         prev;
  int                 i, j;

  if( type == RST_ROOM )
    ch->in_room->area->modified = true;

  if( number_arg( argument, i ) ) {
    if( !*argument ) {
      send( ch, "What flag or option do you want to set?\n\r" );
      return;
    }
    if( !( reset = locate( list, i ) ) ) {
      send( ch, "No reset exists with that number.\n\r" );
      return;
    }
    if( isdigit( *argument ) && number_arg( argument, j ) ) {
      if( j == i ) {
        send( ch, "Moving a reset to where it already is does nothing\
 interesting.\n\r" ); 
        return;
      }
      if( j == 1 ) {
        remove( list, reset );
        reset->next = list;
        list        = reset;
      } else {
        if( j < 1 || j > count( list ) ) {
          send( ch, "You can only move a reset to a sensible position.\n\r" );
          return;
	}
        remove( list, reset );
        prev        = locate( list, j-1 );
        reset->next = prev->next;
        prev->next  = reset;
      }
      send( ch, "Reset %d moved to position %d.\n\r", i, j );
      return;
    }
    modify_reset( ch, num, reset, type, argument );
    return;
  }

  if( matches( argument, "delete" ) ) {
    if( !list ) {
      send( ch, "The list of resets is empty.\n\r" );
      return;
    }
    if( !strcasecmp( argument, "all" ) ) {
      for( reset = list; reset; reset = list ) {
        list = list->next;
        extract( reset );
      }
      reset_log( ch, num, type, "All resets deleted." );
      return;
    }
    if( !( reset = locate( list, atoi( argument ) ) ) ) {
      send( ch, "No reset with that number found to delete.\n\r" );
      return;
    }
    if( is_set( reset->flags, RSFLAG_OBJECT ) ) {
      reset_log( ch, num, type, "Reset deleted for obj [%d] %s.",
		 reset->vnum, get_obj_index( reset->vnum )->Name( ) );
    } else if( is_set( reset->flags, RSFLAG_MOB ) ) {
      reset_log( ch, num, type, "Reset deleted for mob [%d] %s.",
		 reset->vnum, get_species( reset->vnum )->Name( ) );
    } else {
      reset_log( ch, num, type, "Reset deleted for table [%d] %s.",
		 reset->vnum, rtable_list[reset->vnum]->name );
    }
    remove( list, reset );
    extract( reset );
    return;
  }
  
  if( matches( argument, "table" ) ) {
    if( !*argument ) {
      send( ch, "What rtable do you wish to reset?\n\r" );
      return;
    }
    if( number_arg( argument, i ) ) {
      if( --i < 0 || i >= num_rtable ) { 
        send( ch, "No rtable exists with that index.\n\r" );
        return;
      }
    } else {
      for( i = 0; !matches( argument, rtable_list[i]->name ); ) {
        if( ++i >= num_rtable ) {
          send( ch, "No such rtable exists.\n\r" );
          return;
	} 
      }
    }
    
    reset_log( ch, num, type, "Reset added for rtable [%d] %s.",
	       i+1, rtable_list[i]->name );
    
    reset = new reset_data( i );
    reset->flags = ( 1 << RSFLAG_REROLL );

    if( type == RST_MOB ) {
      reset->value = RSPOS_INV;
    } else {
      reset->value = RSPOS_GROUND;
    }

    append( list, reset );
    return;
  }

  thing_data *thing;
  if( !( thing = one_thing( ch, argument, 
			    "reset",
			    ch->array,
			    &ch->contents ) ) )
    return;

  if( mob_data *npc = mob( thing ) ) {
    if( type != RST_ROOM ) {
      send( ch, "You can't reset a mob on a mob or in a table.\n\r" );
      return;
    }
    
    reset = new reset_data( npc->species->vnum );
    reset->flags = ( 1 << RSFLAG_MOB ) | ( 1 << RSFLAG_REROLL );
    reset->value = RSPOS_STANDING;
    
    append( list, reset );
    //    ++npc->species->reset_count;
    reset_log( ch, num, type, "Reset added for mob [%d] %s.",
	       npc->species->vnum, npc->species->Name( ) );
    return;
  }
  
  if( obj_data *obj = object( thing ) ) {
    reset = new reset_data( obj->pIndexData->vnum );
    reset->flags = ( 1 << RSFLAG_OBJECT ) | ( 1 << RSFLAG_REROLL );
    if( type == RST_MOB ) {
      reset->value = RSPOS_INV;
    } else {
      reset->value = RSPOS_GROUND;
    }
    
    append( list, reset );
    reset_log( ch, num, type, "Reset added for obj [%d] %s.",
	       obj->pIndexData->vnum, obj->pIndexData->Name( ) );
    return;
  }
  
  fsend( ch, "You can't reset %s.", thing );
}


void display( char_data* ch, reset_data* reset, int type ) 
{
  char                  buf  [ TWO_LINES ];
  char                flags  [ 32 ];
  char               liquid  [ 32 ];
  char                 rust  [ 5 ];
  int*              chances;
  int               i, j, k;
  obj_clss_data*   obj_clss;
  species_data*     species; 

  if( !reset ) {
    send( ch, "No resets found.\n\r" );
    return;
  }  

  page_underlined( ch,
    "Nmbr Rrl  1_Ch 2_Ch 3_Ch  Rust %sLiq Flags Vnum Descr\n\r",
    type == RST_TABLE ? "" : "Posi " );

  for( i = 1; reset; i++, reset = reset->next ) {
    chances = unpack_int( reset->chances );
    
    if( is_set( reset->flags, RSFLAG_OBJECT ) ) {
      obj_clss = get_obj_index( reset->vnum );
      for( k = 0, j = RSFLAG_LEADER; j < MAX_RSFLAG; j++ ) {
	if( *rsflags_obj[ j-RSFLAG_LEADER ] == '\0' )
	  break;
        if( is_set( reset->flags, j ) )
          flags[k++] = rsflags_obj[ j-RSFLAG_LEADER ][0];
        }
      flags[k] = '\0';
      snprintf( liquid, 32, ( obj_clss
			      && ( obj_clss->item_type == ITEM_DRINK_CON
				   || obj_clss->item_type == ITEM_FOUNTAIN ) ) 
		? ( reset->liquid < 0 ? "emp" : liquid_table[reset->liquid].name )
		: "   " );
      liquid[3] = '\0';
      //      include_liquid = false;
      if( obj_clss
	  && obj_clss->metal( ) 
	  && !is_set( obj_clss->extra_flags, OFLAG_RUST_PROOF ) )
        snprintf( rust, 5, "%3d%%", chances[3] );
      else
        snprintf( rust, 5, "    " );
      if( type == RST_TABLE ) 
        snprintf( buf, TWO_LINES,
		  "[%2d] %c%c%c  %3d%% %3d%% %3d%%  %s %s %-5s %4d %s\n\r", 
		  i, is_set( reset->flags, RSFLAG_REROLL ) ? '*' : ' ',
		  is_set( reset->flags, RSFLAG_REROLL+1 ) ? '*' : ' ',
		  is_set( reset->flags, RSFLAG_REROLL+2 ) ? '*' : ' ',
		  chances[0], chances[1], chances[2], rust,
		  liquid, flags, reset->vnum,
		  obj_clss ? trunc( obj_clss->Name( 1, true ), 29 ).c_str() : "## BUG -- no such object ##" );
      else
        snprintf( buf, TWO_LINES,
		  "[%2d] %c%c%c  %3d%% %3d%% %3d%%  %s %s %s %-5s %4d %s\n\r", 
		  i, is_set( reset->flags, RSFLAG_REROLL ) ? '*' : ' ',
		  is_set( reset->flags, RSFLAG_REROLL+1 ) ? '*' : ' ',
		  is_set( reset->flags, RSFLAG_REROLL+2 ) ? '*' : ' ',
		  chances[0], chances[1], chances[2], rust,
		  ( type == RST_MOB )
		  ? skin_abbrev[ reset->value+2 ]
		  : wear_abbrev[ reset->value+2 ],
		  liquid, flags, reset->vnum,
		  obj_clss ? trunc( obj_clss->Name( 1, true ), 29 ).c_str() : "## BUG -- no such object ##" );
      //      include_liquid = true;
    } else if( is_set( reset->flags, RSFLAG_MOB ) ) {
      species = get_species( reset->vnum );
      for( k = 0, j = RSFLAG_LEADER; j < MAX_RSFLAG; j++ ) {
	if( *rsflags_mob[ j-RSFLAG_LEADER ] == '\0' )
	  break;
        if( is_set( reset->flags, j ) )
          flags[k++] = rsflags_mob[ j-RSFLAG_LEADER ][0];
      }
      flags[k] = '\0';
      snprintf( buf, TWO_LINES,
		"[%2d] %c%c%c  %3d%% %3d%% %3d%%       %s     %-5s %4d %s\n\r", 
		i, is_set( reset->flags, RSFLAG_REROLL ) ? '*' : ' ',
		is_set( reset->flags, RSFLAG_REROLL+1 ) ? '*' : ' ',
		is_set( reset->flags, RSFLAG_REROLL+2 ) ? '*' : ' ',
		chances[0], chances[1], chances[2],
		reset_pos_abbrev[ reset->value ], flags, reset->vnum,
		species ? species->Name( ) : "## BUG -- no such mob ##" );
    } else {
      if( type == RST_TABLE ) {
	*flags = '\0';
	snprintf( buf, TWO_LINES,
		  "[%2d] %c%c%c  %3d%% %3d%% %3d%%           %-5s  TBL %s\n\r", 
		  i, is_set( reset->flags, RSFLAG_REROLL ) ? '*' : ' ',
		  is_set( reset->flags, RSFLAG_REROLL+1 ) ? '*' : ' ',
		  is_set( reset->flags, RSFLAG_REROLL+2 ) ? '*' : ' ',
		  chances[0], chances[1], chances[2],
		  flags, trunc( rtable_list[reset->vnum]->name, 29 ).c_str() );
      } else {
	for( k = 0, j = RSFLAG_LEADER; j < MAX_RSFLAG; j++ ) {
	  if( !*rsflags_obj[ j-RSFLAG_LEADER ] )
	    break;
	  if( is_set( reset->flags, j ) )
	    flags[k++] = rsflags_obj[ j-RSFLAG_LEADER ][0];
        }
	flags[k] = '\0';
        snprintf( buf, TWO_LINES,
		  "[%2d] %c%c%c  %3d%% %3d%% %3d%%       %s     %-5s  TBL %s\n\r", 
		  i, is_set( reset->flags, RSFLAG_REROLL ) ? '*' : ' ',
		  is_set( reset->flags, RSFLAG_REROLL+1 ) ? '*' : ' ',
		  is_set( reset->flags, RSFLAG_REROLL+2 ) ? '*' : ' ',
		  chances[0], chances[1], chances[2],
		  ( type == RST_MOB )
		  ? skin_abbrev[reset->value+2]
		  : wear_abbrev[reset->value+2],
		  flags, 
		  trunc( rtable_list[reset->vnum]->name, 29 ).c_str() );
      }
    }
    page( ch, buf );
  }
}


/*
 *   MAIN ROUTINE TO RESET AN AREA
 */


static bool passes( reset_data *reset, int *roll )
{
  const int *const chances = unpack_int( reset->chances );

  int sum = 0;
  for( int i = 0; i < 3; ++i ) {
    if( is_set( reset->flags, RSFLAG_REROLL+i ) ) {
      roll[i] = -102;
    }
    sum += chances[i];
    if( chances[i] > 0 ) {
      if( roll[i] == -102 ) {
        roll[i] = number_range( 0, 99 );
      } else if( roll[i] < 0 ) {
        roll[i] = -101;
        return false;
      }
      if( ( roll[i] -= chances[i] ) >= 0 ) {
        return false;
      }
    } else { 
      if( roll[i] == -101 || roll[i] >= 0 ) {
        return false;
      }
    }
  }

  if( sum != 0 ) {
    // Would pass roll, check if it exceeds obj limits.
    if( is_set( reset->flags, RSFLAG_OBJECT ) ) {
      if( obj_clss_data *clss = get_obj_index( reset->vnum ) ) {
	if( clss->limit >= 0 && clss->count >= clss->limit ) {
	  for( int i = 0; i < 3; ++i ) {
	    if( chances[i] > 0 ) {
	      roll[i] = -101;
	    }
	  }
	  return false;
	}
      }
    }

    return true;
  }

  return false;
}


void reset_room( room_data* room )
{
  /* OPEN, CLOSE DOORS */

  static const int reset_flags [] = { EX_RESET_CLOSED, EX_RESET_LOCKED, EX_RESET_OPEN };
  static const int status_flags [] = { EX_CLOSED, EX_LOCKED, EX_CLOSED };
  static const int value [] = { 1, 1, 0 }; 

  for( int i = 0; i < room->exits; ++i ) {
    exit_data *exit = room->exits[i];
    if( is_set( exit->exit_info, EX_ISDOOR ) ) {
      int *w1 = &exit->exit_info;
      int *w2;
      if( exit_data *back = reverse( exit ) ) {
        if( player_in_room( exit->to_room ) )
          continue;
        w2 = &back->exit_info;
      } else {
        w2 = w1;
      } 
      for( int j = 0; j < 3; ++j ) {
        if( is_set( w1, reset_flags[j] ) ) {
          assign_bit( w1, status_flags[j], value[j] );
          assign_bit( w2, status_flags[j], value[j] );
	}
      }
    }
  }
  
  thing_array stuff = room->contents;

  for( int i = 0; i < stuff; ++i  ) {
    thing_data *thing = stuff[i];
    if( !thing->Is_Valid( ) ) {
      continue;
    }
    if( mob_data *rch = mob( thing ) ) {
      for( mprog_data *mprog = rch->species->mprog; mprog; mprog = mprog->next ) {
	if( mprog->trigger == MPROG_TRIGGER_RESET ) {
	  clear_variables( );
	  var_mob = rch;
	  var_room = room;
	  mprog->execute( rch );
	  break;
	}
      }
    } else if( obj_data *obj = object( thing ) ) {
      oprog_data *oprog;
      for( oprog = obj->pIndexData->oprog; oprog; oprog = oprog->next ) {
	if( oprog->trigger == OPROG_TRIGGER_RESET ) {
	  clear_variables( );
	  var_obj = obj;
	  var_room = room;
	  obj->Select_All( );
	  if( !oprog->execute( obj )
	      || !obj->Is_Valid( ) ) {
	    break;
	  }
	}
      }
      if( !obj->Is_Valid( )
	  || oprog
	  || ( !obj->reset
	       && is_set( obj->extra_flags, OFLAG_NO_RESET ) )
	  || ( !obj->reset
	       && !is_set( obj->extra_flags, OFLAG_NOSAVE )
	       && is_set( room->room_flags, RFLAG_SAVE_ITEMS ) )
	  || ( obj->pIndexData->item_type == ITEM_CHAIR
	       && !obj->contents.is_empty() ) ) {
	continue;
      }
      obj->Extract( );
    }
  }
  
  char_data*       leader = 0;
  mob_data*           rch = 0;
  int                roll [ 3 ] = { -102, -102, -102 };
  int               count = -1; 
  obj_data*     container = 0;
  obj_data          *seat = 0;

  for( reset_data *reset = room->reset; reset; reset = reset->next ) {
    if( count < 0
	|| ( is_set( reset->flags, RSFLAG_REROLL )
	     && ( is_set( reset->flags, RSFLAG_MOB )
		  || reset->value == RSPOS_GROUND ) ) ) {
      count = reset->count;
    }
    if( count <= 0 && passes( reset, roll ) ) {
      if( is_set( reset->flags, RSFLAG_MOB ) ) {
        reset_mob( reset, room, rch, leader, seat );
      } else if( is_set( reset->flags, RSFLAG_OBJECT ) ) {
        reset_obj( reset, room, rch, container, seat );
      } else {
        rtable_calls = 0;
        reset_table( reset, reset->vnum, room, rch, container, seat );
      }
    }
  }

  action_data *action;

  for( action = room->action; action; action = action->next ) {
    if( action->trigger == TRIGGER_RESET ) {
      clear_variables( );
      var_room = room;
      if( action->execute( ) ) {
	// Allow set of reset flags.
	action = 0;
      }
      break;
    }
  }

  if( !action ) {
    set_bit( room->room_flags, RFLAG_RESET0 );
    set_bit( room->room_flags, RFLAG_RESET1 );
    set_bit( room->room_flags, RFLAG_RESET2 );
  }
}


static void register_reset( obj_data *obj, room_data* room )
{
  reset_data *mark = room->reset;

  for( reset_data *reset = room->reset; ; reset = reset->next ) {
    if( is_set( reset->flags, RSFLAG_REROLL ) ) {
      mark = reset;
    }
    if( reset == obj->reset ) {
      break;
    }
  }

  ++mark->count;
  obj->reset = mark;
}


void unregister_reset( obj_data *obj )
{
  if( obj->reset ) {
    --obj->reset->count;
    obj->reset = 0;
    consolidate( obj );
  }
}


static void register_reset( mob_data *mob, room_data *room )
{
  reset_data *mark = room->reset;

  for( reset_data *reset = room->reset; ; reset = reset->next ) {
    if( is_set( reset->flags, RSFLAG_REROLL ) ) {
      mark = reset;
    }
    if( reset == mob->reset ) {
      break;
    }
  }

  ++mark->count;
  mob->reset = mark;
}


void unregister_reset( mob_data *mob )
{
  if( mob->reset ) {
    --mob->reset->count;
    mob->reset = 0;
  }
}


/*
 *   OBJECT RESET FUNCTION
 */


static obj_data* create( obj_clss_data* obj_clss, reset_data* reset, mob_data* mob )
{
  obj_data *obj = create( obj_clss );

  const int *chances = unpack_int( reset->chances );

  if( obj_clss->item_type == ITEM_FOUNTAIN ) {
    if( reset->liquid >= 0 ) {
      obj->value[2] = reset->liquid;
    }
  } else if( obj_clss->item_type == ITEM_DRINK_CON ) {
    if( reset->liquid < 0 ) {
      obj->value[1] = 0;
    } else {
      obj->value[1] = obj->value[0];
      obj->value[2] = reset->liquid;
    }
  }
  
  enchant_object( obj ); 
  set_alloy( obj, 10 );
  rust_object( obj, chances[3] );
  set_quality( obj );
  set_size( obj, mob );
  
  return obj;
}


static void place( obj_data* obj, reset_data* reset, room_data* room,
		   mob_data* mob, obj_data*& container, obj_data*& seat )
{
  if( is_set( reset->flags, RSFLAG_INSIDE ) ) {
    if( !container ) {
      bug( "Reset_Obj: Inside flag with no container - %s %d.",
	   room ? "Room" : "Species",
	   room ? room->vnum : mob->species->vnum );
      obj->Extract( );
      return;
    }
    obj->To( container );

  } else if( reset->value == RSPOS_GROUND ) {
    if( !room ) {
      bug( "Reset_Obj: Ground Object with NULL room - Species %d.",
	   mob->species->vnum );
      obj->Extract( );
      return;
    }
    if( is_set( reset->flags, RSFLAG_HIDDEN ) ) {
      if( can_wear( obj, ITEM_TAKE ) ) {
	bug( "Reset_Obj: Hidden object that can be taken - Room %d.", room->vnum );
      } else {
	set_bit( obj->extra_flags, OFLAG_NOSHOW );
      }
    }
    obj->reset = reset;
    register_reset( obj, room );
    if( obj->timer > 0 ) {
      obj->timer = -obj->timer;
    }
    obj->To( room );
    // If we place a corpse, keep it from rotting.
    // If we place a gate, keep it from disappearing.
    // If we place a fire, keep it from burning out.
    //    stop_events( obj, execute_decay );

  } else {
    if( !mob ) {
      bug( "Reset_Obj: Wear loc with null mob - Room %d.", room->vnum );
      obj->Extract( );
      return;
    }
    obj->reset = reset;
    if( reset->value >= 0 ) {
      obj->position = reset->value; 
      obj->layer = -1;
      // Find a valid layer if possible.
      if( can_wear( obj, wear_index[obj->position] ) ) {
	obj_data *worn = 0;
	for( int i = LAYER_BOTTOM; i < MAX_LAYER; ++i ) {
 	  if( is_set( obj->pIndexData->layer_flags, i ) ) {
	    if( !( worn = mob->Wearing( obj->position, i ) ) ) {
	      obj->layer = i;
	      break;
	    }
	  }
	}
	if( worn && is_set( reset->flags, RSFLAG_OVERLOAD ) ) {
	  obj->layer = worn->layer;
	  worn->Extract( );
	}
      }
      // Set two-hand flag.
      if( is_set( obj->extra_flags, OFLAG_TWO_HAND )
	  && ( obj->position == WEAR_HELD_R
	       || obj->position == WEAR_HELD_L ) ) {
	set_bit( mob->status, STAT_TWO_HAND );
      }
      if( obj->timer > 0 ) {
	obj->timer = -obj->timer;
      }
      obj->To( mob->wearing );
      //      stop_events( obj, execute_decay );
    } else {
      // position == inventory
      obj->To( mob );
    }
  }
  
  if( is_set( reset->flags, RSFLAG_CONTAINER ) )
    container = obj;

  if( obj->pIndexData->item_type == ITEM_CHAIR ) {
    seat = obj;
  }
}


static void reset_obj( reset_data* reset, room_data* room, mob_data* mob,
		       obj_data*& container, obj_data*& seat )
{
  obj_clss_data *obj_clss = get_obj_index( reset->vnum );
  
  if( !obj_clss
      || ( !room
	   && reset->value == RSPOS_SKIN ) ) {
    return;
  }

  if( room
      && mob
      && mob->pShop
      && reset->value == RSPOS_INV ) {
    return;
  }

  obj_data *obj = create( obj_clss, reset, mob );
  place( obj, reset, room, mob, container, seat );
}


/*
 *   RESET TABLE FUNCTION
 */


static void reset_table( reset_data* base_reset, int n, room_data* room,
			 mob_data* mob, obj_data*& container, obj_data*& seat )
{
  obj_clss_data*   obj_clss;
  obj_data*             obj;
  int                  roll  [ 3 ] = { -102, -102, -102 };
  
  if( n < 0 || n >= num_rtable ) {
    bug( "Reset_Table: Invalid table." );
    bug( "-- Index = %d", n );
    return;
  }

  ++rtable_calls;

  if( rtable_calls >= 100 ) {
    if( rtable_calls == 100 ) {
      bug( "Reset_Table: Infinite lookup chain." );
    }
    return;
  }

  for( reset_data *reset = rtable_list[n]->reset; reset; reset = reset->next ) {
    if( passes( reset, roll ) ) {
      if( is_set( reset->flags, RSFLAG_OBJECT ) ) {
        if( !( obj_clss = get_obj_index( reset->vnum ) ) ) 
          return;
        obj = create( obj_clss, reset, mob );
        place( obj, base_reset, room, mob, container, seat );
      } else {
        reset_table( base_reset, reset->vnum, room, mob, container, seat );
      }
    }
  }
}


/*
 *   MOB RESET FUNCTIONS
 */


static void reset_mob( reset_data* reset, room_data* room,
		       mob_data*& mob, char_data*& leader, obj_data*& seat )
{
  if( ( is_set( reset->flags, RSFLAG_NIGHT ) && weather.is_day( ) )
      || ( is_set( reset->flags, RSFLAG_DAY ) && !weather.is_day( ) ) )
    return;

  species_data *species;
  if( !( species = get_species( reset->vnum ) ) ) {
    return;
  }
  
  mob = new Mob_Data( species );
  
  if( is_set( reset->flags, RSFLAG_SENTINEL ) 
      || is_set( mob->species->act_flags, ACT_SENTINEL ) ) { 
    set_bit( mob->status, STAT_SENTINEL );    
  }

  if( is_set( reset->flags, RSFLAG_AGGRESSIVE ) ) {
    set_bit( mob->status, STAT_AGGR_ALL );    
  } else if( is_set( reset->flags, RSFLAG_PEACEFUL ) ) {
    remove_bit( mob->status, STAT_AGGR_ALL );
    remove_bit( mob->status, STAT_AGGR_GOOD );
    remove_bit( mob->status, STAT_AGGR_EVIL );
    remove_bit( mob->status, STAT_AGGR_LAWFUL );
    remove_bit( mob->status, STAT_AGGR_CHAOTIC );
  }

  mreset_mob( mob );
  mob->To( room );
  mob->reset = reset;
  mob->position = rspos_index[ reset->value ];
  
  if( mob->position != POS_SLEEPING ) {
    remove_bit( mob->affected_by, AFF_SLEEP );
  }
  
  mob_setup( mob, room );
  register_reset( mob, room );

  if( is_set( reset->flags, RSFLAG_CHAIR )
      && seat ) {
    if( mob->position != POS_SLEEPING
	&& mob->position != POS_RESTING ) {
      bug( "Reset_Mob: In room %d, chair flag but not resting or sleeping - species %d.",
	   room->vnum, mob->species->vnum );
    } else {
      bool sleep = mob->position == POS_SLEEPING;
      sit( mob, seat, true );
      if( sleep ) {
	do_sleep( mob, "" );
      }
    }
  }

  if( is_set( reset->flags, RSFLAG_MOUNT ) ) {
    if( leader && !leader->mount ) {
      set_bit( mob->status, STAT_PET );
      add_follower( mob, leader );
      mount( leader, mob );
    }
  } else if( is_set( reset->flags, RSFLAG_FOLLOWER ) ) {
    if( leader ) {
      add_follower( mob, leader );
    }
  }

  if( is_set( reset->flags, RSFLAG_LEADER ) ) {
    leader = mob;
  }
}


void mob_setup( mob_data* mob, room_data* room )
{
  //  if( room->is_dark( mob ) )
  //    set_bit( mob->affected_by, AFF_INFRARED );

  for( shop_data *shop = shop_list; shop; shop = shop->next ) {
    if( shop->room == room
	&& shop->keeper == mob->species->vnum ) {
      mob->pShop = shop;
    }
  }

  set_trainer( mob, room );
}


/*
 *   MRESET FUNCTIONS
 */


void mreset_mob( mob_data* mob )
{
  obj_data*    container = 0;
  obj_data*         seat = 0;
  int               roll [ 3 ] = { -102, -102, -102 };

  for( reset_data *reset = mob->species->reset; reset; reset = reset->next ) {
    if( passes( reset, roll ) ) {
      if( is_set( reset->flags, RSFLAG_OBJECT ) ) {
        reset_obj( reset, 0, mob, container, seat );
      } else {
        rtable_calls = 0;
        reset_table( reset, reset->vnum, 0, mob, container, seat );
      }
    }
  }
}


obj_array *get_skin_list( species_data* species )
{
  obj_array *list = 0;
  int roll [ 3 ] = { -102, -102, -102 };

  for( reset_data *reset = species->reset; reset; reset = reset->next ) {
    obj_clss_data *obj_clss;
    if( !( obj_clss = get_obj_index( reset->vnum ) )
	|| reset->value != RSPOS_SKIN ) {
      continue;
    }

    if( !list ) {
      list = new obj_array; 
    }
    
    if( !passes( reset, roll ) ) {
      continue;
    }
    
    obj_data *obj = create( obj_clss );
    enchant_object( obj ); 
    set_alloy( obj, 10 );
    rust_object( obj, 100-species->shdata->level, true );
    set_quality( obj );
    set_size( obj );

    *list += obj;
  }
  
  return list;
}


/*
 *   SHOP RESET FUNCTION
 */


void reset_shop( mob_data *keeper )
{
  int roll [ 3 ] = { -102, -102, -102 };

  for( reset_data *reset = keeper->reset->next; reset; reset = reset->next ) {
    if( !is_set( reset->flags, RSFLAG_OBJECT )
	|| reset->value == RSPOS_GROUND ) {
      break;
    }
  
    if( passes( reset, roll ) && reset->value == RSPOS_INV ) {
      obj_clss_data *clss = get_obj_index( reset->vnum );
      obj_data *obj = create( clss );
      set_bit( obj->extra_flags, OFLAG_IDENTIFIED );    
      if( obj->pIndexData->item_type == ITEM_DRINK_CON ) {
	if( reset->liquid < 0 ) {
	  obj->value[1] = 0;
	} else {
	  set_bit( obj->extra_flags, OFLAG_KNOWN_LIQUID );
	  obj->value[1] = obj->value[0];
	  obj->value[2] = reset->liquid;
	}
      }
      //_int( reset->chances );
      //      enchant_object( obj ); 
      set_alloy( obj, 10 );
      //      rust_object( obj, chances[3] );
      //      set_quality( obj );
      set_size( obj );
      obj->for_sale = true;
      obj->To( keeper );
    }
  }
}


/*
 *   DISK ROUTINES
 */


void load( FILE* fp, reset_data*& list )
{
  int i;

  while ( ( i = fread_number( fp ) ) != -1 ) {
    reset_data *reset = new reset_data(i);
    reset->flags    = fread_number( fp );
    reset->chances  = fread_number( fp );
    reset->value    = fread_number( fp );
    reset->liquid   = fread_number( fp );
    append( list, reset );
  }
}


void write( FILE* fp, reset_data* reset )
{
  for( ; reset; reset = reset->next ) {
    fprintf( fp, "%d %d %d %d %d\n", 
	     reset->vnum, reset->flags, reset->chances,
	     reset->value, reset->liquid );
  }
  
  fprintf( fp, "-1\n" );
}


/*
 *   COMMANDS TO LOCATE RESETS 
 */


bool has_reset( obj_clss_data* obj_clss )
{
  for( area_data *area = area_list; area; area = area->next ) {
    for( room_data *room = area->room_first; room; room = room->next ) {
      for( reset_data *reset = room->reset; reset; reset = reset->next ) {
        if( is_set( reset->flags, RSFLAG_OBJECT )
	    && reset->vnum == obj_clss->vnum ) {
          return true;
	}
      }
    }
  }

  for( int i = 1; i <= species_max; ++i ) {
    species_data *species;
    if( !( species = species_list[i] ) ) {
      continue;
    }
    for( reset_data *reset = species->reset; reset; reset = reset->next ) {
      if( is_set( reset->flags, RSFLAG_OBJECT )
	  && reset->vnum == obj_clss->vnum ) {
        return true;
      }
    }
  }

  return false;
}


bool has_reset( species_data* species )
{
  for( area_data *area = area_list; area; area = area->next ) {
    for( room_data *room = area->room_first; room; room = room->next ) {
      for( reset_data *reset = room->reset; reset; reset = reset->next ) {
        if( is_set( reset->flags, RSFLAG_MOB )
	    && reset->vnum == species->vnum ) {
          return true;
	}
      }
    }
  }

  return false;
}


/* 
 *   ROWHERE ROUTINES
 */


static void rowhere_key( char_data* ch, obj_clss_data* key )
{
  //  if( key->item_type != ITEM_KEY )
  //    return;
  
  for( area_data *area = area_list; area; area = area->next ) {
    for( room_data *room = area->room_first; room; room = room->next ) {
      for( int i = 0; i < room->exits; i++ ) {
        if( room->exits[i]->key == key->vnum ) { 
          page( ch, "  key to %s door of %s [%d]\n\r",
		dir_table[ room->exits[i]->direction ].name,
		room->name, room->vnum );
	}
      }
    }
  }
  
  for( int index = 1; index <= obj_clss_max; ++index ) {
    if( obj_clss_data *container = obj_index_list[index] ) {
      if( container->item_type == ITEM_CONTAINER 
	  && container->value[2] == key->vnum ) {
	page( ch, "  key to %s [%d]\n\r",
	      container, container->vnum );
      }
      if( container->fakes == key->vnum ) {
	page( ch, "  fake of %s [%d]\n\r",
	      container, container->vnum );
      }
    }
  }
}


void do_rowhere( char_data* ch, const char *argument )
{
  area_data*           area;
  custom_data*       custom;
  obj_clss_data*   obj_clss;
  room_data*           room;
  shop_data*           shop;
  species_data*     species;
  bool                first = true;
  bool                found = true;
  int                 flags;

  if( !get_flags( ch, argument, &flags, "p", "Rowhere" ) ) {
    return;
  }

  if( !*argument ) {
    send( ch, "Syntax: rowhere [-p] <object>\n\r" );
    return;
  }
  
  int vnum = 0;
  number_arg( argument, vnum );
  
  if( vnum < 0 ) {
    send( ch, "Negative vnum specified.\n\r" );
    return;
  }

  if( is_set( flags, 0 ) && vnum == 0 ) {
    send( ch, "You must specify an obj vnum to use the -p option.\n\r" );
    return;
  }

  for( int index = 1; index <= obj_clss_max; ++index ) {
    if( !( obj_clss = obj_index_list[index] )
	|| vnum > 0 && index != vnum ) {
      continue;
    }
    
    const char *name = obj_clss->Name( );
    
    if( vnum == 0 && !is_name( argument, name ) ) {
      continue;
    }

    page_divider( ch, name, index ); 

    first = false;
    found = false;

    /* SEARCH MRESETS AND MPROGS */

    for( int i = 1; i <= species_max; ++i ) {
      if( !( species = species_list[i] ) ) 
        continue;
      for( reset_data *reset = species->reset; reset; reset = reset->next ) {
        if( obj_clss->vnum == reset->vnum 
	    && is_set( reset->flags, RSFLAG_OBJECT ) ) {
          page( ch, "  mreset on %s [%d]\n\r", species, i );
          found = true;
	}
      }
      
      if( is_set( flags, 0 ) ) {
	const bool compiled = species->attack->binary;
	if( !compiled ) {
	  species->attack->compile( );
	}
	if( search_oload( ch, species->attack->binary, (void*)index,
			  search_attack, 0, species->Name( ), i, empty_string ) ) {
	/*
        if( ( name = search_oload( ch, species->attack->binary, index ) ) ) {
          page( ch, "  %s() in attack mprog on %s [%d]\n\r", name, species, i );
	*/
          found = true;
	}
	if( !compiled ) {
	  species->attack->decompile( );
	}
	int j = 1;
        for( mprog_data *mprog = species->mprog; mprog; mprog = mprog->next ) {
	  if( mprog->trigger == MPROG_TRIGGER_GIVE
	      && mprog->value == index ) {
	    page( ch, "  give trigger value on mprog #%d on %s [%d]\n\r",
		  j, species, index );
	  }
	  const bool compiled = mprog->binary;
	  if( !compiled ) {
	    mprog->compile( );
	  }
	  if( search_oload( ch, mprog->binary, (void*)index,
			    search_mprog, j, species->Name( ), i, empty_string ) ) {
	    /*
          if( ( name = search_oload( ch, mprog->binary, index ) ) ) {
            page( ch, "  %s() in mprog #%d on %s [%d]\n\r", name, j, species, i );
	    */
            found = true;
	  }
	  if( !compiled ) {
	    mprog->decompile( );
	  }
	  ++j;
	}
      }
    }
    
    /* SEARCH ROOM RESETS AND ACODES*/
    
    for( area = area_list; area; area = area->next ) {
      const bool loaded = area->act_loaded;
      for( room = area->room_first; room; room = room->next ) {
        species = 0;
        for( reset_data *reset = room->reset; reset; reset = reset->next ) {
          if( is_set( reset->flags, RSFLAG_MOB ) ) {
            species = get_species( reset->vnum );
            continue;
	  }
          if( obj_clss->vnum != reset->vnum 
	      || !is_set( reset->flags, RSFLAG_OBJECT ) ) {
            continue;
	  }
          found = true;
          if( reset->value == RSPOS_GROUND ) {
            page( ch, "  reset at %s [%d]\n\r", room->name, room->vnum );
	  } else if( !species ) {
            page( ch, "  [BUG] Illegal reset structure [%d]\n\r",
		  room->vnum );
	  } else {
            page( ch, "  reset on %s [%d] at %s [%d]\n\r",
		  species, species->vnum,
		  room->name, room->vnum );
	  }
	}

	int j = 1;
	for( action_data *action = room->action; action; action = action->next ) {
	  if( action_target( action, OBJ_DATA, index ) ) {
	    page( ch, "  object target in acode #%d at %s [%d]\n\r",
		  j, room->name, room->vnum );
	  }
	  if( is_set( flags, 0 ) ) {
	    const bool compiled = action->binary;
	    if( !compiled ) {
	      action->compile( );
	    }
	    if( search_oload( ch, action->binary, (void*)index,
			      search_acode, j, room->name, room->vnum, empty_string ) ) {
	      /*
		if( ( name = search_oload( ch, action->binary, index ) ) ) {
		page( ch, "  %s() in acode #%d at %s [%d]\n\r",
		name, j, room->name, room->vnum );
	      */
              found = true;
	    }
	    if( !compiled ) {
	      action->decompile( );
	    }
	  }
	  ++j;
	}
      }
      if( !loaded && area->act_loaded ) {
	area->clear_actions( );
      }
    }
    
    
    /* SEARCH OPROGS */
    
    for( int i = 1; i <= obj_clss_max; ++i ) {
      if( obj_clss_data *clss = obj_index_list[i] ) {
	int j = 1;
	for( oprog_data *oprog = clss->oprog; oprog; oprog = oprog->next ) {
	  if( oprog->obj_act && index == oprog->obj_act->vnum ) {
	    page( ch, "  obj_act in oprog #%d on %s [%d]\n\r",
		  j, clss, vnum );
	  }
	  if( is_set( flags, 0 ) ) {
	    const bool compiled = oprog->binary;
	    if( !compiled ) {
	      oprog->compile( );
	    }
	    if( search_oload( ch, oprog->binary, (void*)index,
			      search_oprog, j, clss->Name(), clss->vnum, empty_string ) ) {
	      /*
		if( ( name = search_oload( ch, oprog->binary, index ) ) ) {
		page( ch, "  %s() in oprog #%d on %s [%d]\n\r",
		name, j, clss, clss->vnum );
	      */
	      found = true;
	    }
	    if( !compiled ) {
	      oprog->decompile( );
	    }
	  }
	  ++j;
	}
      }
    }
      
    /* SEARCH TABLES */
    
    for( int i = 0; i < MAX_TABLE; ++i ) {
      for( int j = 0; j < table_max[ i ]; ++j ) {
	Table_Data *t = table_addr( i, j );
	Tprog_Data *const *tprog = t->program( );
	if( tprog && *tprog ) {
	  const bool compiled = (*tprog)->binary;
	  if( !compiled )
	    (*tprog)->compile( );
	  if( search_oload( ch, (*tprog)->binary, (void*)index, search_tprog,
			    0, table_name( i ), 0, t->name ) ) {
	    found = true;
	  }
	  if( !compiled )
	    (*tprog)->decompile( );
	}
      }
    }

    /* SEARCH RTABLES */
    
    for( int i = 0; i < num_rtable; i++ ) {
      for( reset_data *reset = rtable_list[i]->reset; reset; reset = reset->next ) {
        if( reset->vnum == index 
	    && is_set( reset->flags, RSFLAG_OBJECT ) ) {
          page( ch, "  in rtable #%d, %s.\n\r", 
		i+1, rtable_list[i]->name );
          found = true;
	}
      }
    }
    
    if( table_rowhere( ch, index ) )
      found = true;

    /* Search lists */

    for( int i = 0; i < MAX_LIST; ++i ) {
      if( isdigit( *list_entry[i][0] )
	  && *list_entry[i][2] == 'O' ) {
	const int num = atoi( list_entry[i][0] );
	for( int j = 0; j < num; ++j ) {
	  if( list_value[i][j] == index ) {
	    page( ch, "  list \"%s\" entry %s[%2d]\n\r",
		  list_entry[0][i], list_entry[i][1], j );
	    found = true;
	  }
	}
      }
    }

    /* SEARCH SHOPS */
    
    for( shop = shop_list; shop; shop = shop->next ) 
      for( custom = shop->custom; custom; custom = custom->next ) {
        if( custom->item == obj_clss ) {
          page( ch, "  custom at %s [%d]\n\r", 
		shop->room->name, shop->room->vnum );
	  found = true;
	}  
        for( int i = 0; i < MAX_INGRED; i++ ) {
          if( custom->ingred[i] == obj_clss ) {
            page( ch, 
		  "  ingredient for making %s at %s [%d]\n\r",  
		  custom->item,
		  shop->room->name, shop->room->vnum );
            found = true;
	  }
	}
      }
    
    /* SEARCH SPELLS */
    
    for( int i = 0; i < table_max[TABLE_SKILL_SPELL]; ++i ) {
      for( int j = 0; j < MAX_SPELL_WAIT; ++j ) {
        if( abs( skill_spell_table[i].reagent[j] ) == index ) {
          page( ch, "  reagent for spell %s.\n\r",
		skill_spell_table[i].name );
          found = true;
          break;
	}
      }
    }
    
    /* CORPSE? */

    for( int i = 1; i <= species_max; ++i ) {
      if( !( species = species_list[i] ) ) 
        continue;
      if( species->corpse == index ) {
        page( ch, "  corpse of %s [%d]\n\r",
	      species, species->vnum );
        found = true;
      }
    }

    rowhere_key( ch, obj_clss );
    
    if( !found ) {
      page_centered( ch, "  none found" ); 
    }
  }
  
  if( first ) {
    send( ch, "Nothing like that in hell, earth, or heaven.\n\r" );
  }
}


void do_rmwhere( char_data* ch, const char *argument )
{
  int flags;

  if( !get_flags( ch, argument, &flags, "p", "Rowhere" ) ) {
    return;
  }
  
  if( !*argument ) {
    send( ch, "Syntax: rmwhere <mob>\n\r" );
    return;
  }

  int vnum = 0;
  number_arg( argument, vnum );
  
  if( vnum < 0 ) {
    send( ch, "Negative vnum specified.\n\r" );
    return;
  }

  if( is_set( flags, 0 ) && vnum == 0 ) {
    send( ch, "You must specify a mob vnum to use the -p option.\n\r" );
    return;
  }

  action_data*    action;
  area_data*        area;
  reset_data*      reset;
  room_data*        room;
  species_data*  species;
  species_data*   undead;
  bool             first  = true;
  bool             found  = true;

  for( int index = 1; index <= species_max; ++index ) { 
    if( !( species = species_list[index] )
	|| vnum > 0 && index != vnum ) {
      continue;
    }
    
    const char *name = species->Name( );
    
    if( vnum == 0 && !is_name( argument, name ) ) {
      continue;
    }

    page( ch, "%s%s [%d]:\n\r", ( first ? "" : "\n\r" ),
	  name, species->vnum );
    
    first = false;
    found = false;


    /* SEARCH MPROGS */
    
    if( is_set( flags, 0 ) ) {
      for( int i = 1; i <= species_max; ++i ) {
	species_data *species;
	if( !( species = species_list[i] ) ) {
	  continue;
	}
	
	const bool compiled = species->attack->binary;
	if( !compiled ) {
	  species->attack->compile( );
	}
	if( search_mload( ch, species->attack->binary, (void*)index,
			  search_attack, 0, species->Name(), i, empty_string ) ) {
	  /*
        if( ( name = search_mload( ch, species->attack->binary, index ) ) ) {
          page( ch, "  %s() in attack mprog on %s [%d]\n\r", name, species, i );
	  */
          found = true;
	}
	if( !compiled ) {
	  species->attack->decompile( );
	}
	int j = 1;
        for( mprog_data *mprog = species->mprog; mprog; 
	     j++, mprog = mprog->next ) {
	  const bool compiled = mprog->binary;
	  if( !compiled )
	    mprog->compile( );
	  if( search_mload( ch, mprog->binary, (void*)index,
			    search_mprog, j, species->Name(), i, empty_string ) ) {
	    /*
          if( ( name = search_mload( ch, mprog->binary, index ) ) ) {
            page( ch, "  %s() in mprog #%d on %s [%d]\n\r", name, j, species, i );
	    */
            found = true;
	  }
	  if( !compiled ) {
	    mprog->decompile( );
	  }
	}
      }
    }
    

    /* SEARCH ROOM RESETS AND ACODES */
    
    for( area = area_list; area; area = area->next ) {
      const bool loaded = area->act_loaded;
      for( room = area->room_first; room; room = room->next ) {
        for( reset = room->reset; reset; reset = reset->next ) {
          if( reset->vnum == index && is_set( reset->flags, RSFLAG_MOB ) ) {
            page( ch, "  reset at %s [%d]\n\r", room->name, room->vnum );
            found = true;
	  }
	}
	
	action = room->action;
	for( int j = 1; action; j++, action = action->next ) {
	  if( action_target( action, CHAR_DATA, index ) ) {
	    page( ch, "  mob target in acode #%d at %s [%d]\n\r",
		  j, room->name, room->vnum );
	  }
	  if( is_set( flags, 0 ) ) {
	    const bool compiled = action->binary;
	    if( !compiled ) {
	      action->compile( );
	    }
	    if( search_mload( ch, action->binary, (void*)index,
			      search_acode, j, room->name, room->vnum, empty_string ) ) {
	      /*
		if( ( name = search_mload( ch, action->binary, index ) ) ) {
		page( ch, "  %s() in acode #%d at %s [%d]\n\r",
		name, j, room->name, room->vnum );
	      */
              found = true;
	    }
	    if( !compiled ) {
	      action->decompile( );
	    }
	  }
	}
      }
      if( !loaded && area->act_loaded ) {
	area->clear_actions( );
      }
    }
    

    /* SEARCH OPROGS */
    
    if( is_set( flags, 0 ) ) {
      for( int i = 1; i <= obj_clss_max; ++i ) {
	if( obj_clss_data *clss = obj_index_list[i] ) {
	  int j = 1;
	  for( oprog_data *oprog = clss->oprog; oprog; ++j, oprog = oprog->next ) {
	    const bool compiled = oprog->binary;
	    if( !compiled ) {
	      oprog->compile( );
	    }
	    if( search_mload( ch, oprog->binary, (void*)index,
			      search_oprog, j, clss->Name(), clss->vnum, empty_string ) ) {
	      /*
	    if( ( name = search_mload( ch, oprog->binary, index ) ) ) {
              page( ch, "  %s() in oprog #%d on %s [%d]\n\r",
		    name, j, clss, clss->vnum );
	      */
              found = true;
	    }
	    if( !compiled ) {
	      oprog->decompile( );
	    }
	  }
	}
      }
    }
      

    /* SEARCH ALTERNATE MOB FORMS */
    
    for( int j = 1; j <= species_max; ++j ) { 
      if( ( undead = species_list[j] ) ) {
        if( undead->zombie == index ) {
          page( ch, "  zombie form of %s [%d]\n\r",
		undead, j );
          found = true;
	}
        if( undead->skeleton == index ) {
          page( ch, "  skeletal form of %s [%d]\n\r",
		undead, j );
          found = true;
	}
      }
    }
    
      
    /* SEARCH TABLES */
    
    for( int i = 0; i < MAX_TABLE; ++i ) {
      for( int j = 0; j < table_max[ i ]; ++j ) {
	Table_Data *t = table_addr( i, j );
	Tprog_Data *const *tprog = t->program( );
	if( tprog && *tprog ) {
	  const bool compiled = (*tprog)->binary;
	  if( !compiled )
	    (*tprog)->compile( );
	  if( search_mload( ch, (*tprog)->binary, (void*)index, search_tprog,
			    0, table_name( i ), 0, t->name ) ) {
	    found = true;
	  }
	  if( !compiled )
	    (*tprog)->decompile( );
	}
      }
    }

    /* Search lists */

    for( int i = 0; i < MAX_LIST; ++i ) {
      if( isdigit( *list_entry[i][0] )
	  && *list_entry[i][2] == 'M' ) {
	const int num = atoi( list_entry[i][0] );
	for( int j = 0; j < num; ++j ) {
	  if( list_value[i][j] == index ) {
	    page( ch, "  list \"%s\" entry %s[%2d]\n\r",
		  list_entry[0][i], list_entry[i][1], j );
	    found = true;
	  }
	}
      }
    }

    if( !found ) {
      page( ch, "  No resets found\n\r" );
    }
  }
  
  if( first ) {
    send( ch, "Nothing like that in hell, earth, or heaven.\n\r" );
  }
}
