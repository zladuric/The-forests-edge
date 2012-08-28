#include <ctype.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include "define.h"
#include "struct.h"


/*
 *   DESCR_DATA
 */


Descr_Data :: Descr_Data( )
  : name(empty_string), keywords(empty_string),
    singular(empty_string), long_s(empty_string), adj_s(empty_string), prefix_s(empty_string),
    plural(empty_string), long_p(empty_string), adj_p(empty_string), prefix_p(empty_string),
    complete(empty_string)
{
  record_new( sizeof( descr_data ), MEM_DESCR );
}


Descr_Data :: ~Descr_Data( )
{
  record_delete( sizeof( descr_data ), MEM_DESCR );

  free_string( name,        MEM_DESCR );
  free_string( complete,    MEM_DESCR );
  free_string( keywords,    MEM_DESCR );
  free_string( singular,    MEM_DESCR );
  free_string( prefix_s,    MEM_DESCR );
  free_string( adj_s,       MEM_DESCR );
  free_string( long_s,      MEM_DESCR );
  free_string( plural,      MEM_DESCR );
  free_string( prefix_p,    MEM_DESCR );
  free_string( adj_p,       MEM_DESCR );
  free_string( long_p,      MEM_DESCR );
}


/*
 *   EXTRA_DATA
 */


const char *Extra_Data :: Keywords( char_data* )
{
  return keyword;
}


/*
 *   READ/WRITE EXTRAS
 */


void read_extras( FILE* fp, extra_array& list )
{
  char         letter;

  while( ( letter = fread_letter( fp ) ) == 'E' ) {
    extra_data *extra = new extra_data;
    extra->keyword  = fread_string( fp, MEM_EXTRA );
    extra->text     = fread_string( fp, MEM_EXTRA );
    list += extra;
  }

  if( letter != '!' )
    ungetc( letter, fp );
}


void write_extras( FILE* fp, const extra_array& list )
{
  for( int i = 0; i < list; ++i ) {
    fprintf( fp, "E\n" );
    fwrite_string( fp, list.list[i]->keyword );
    fwrite_string( fp, list.list[i]->text );
  }
}


/* 
 *   SUPPORT ROUTINES
 */


void show_extras( char_data* ch, const extra_array& list )
{
  if( list.is_empty() )
    return;

  page( ch, "\n\rExtra Descriptions:\n\r" );

  for( int i = 0; i < list; ++i ) {
    page( ch, "[%2d]  %s\n\r", i+1, list.list[i]->keyword );
    page( ch, list.list[i]->text );
  }
}


void page_descr( char_data* ch, const char* text )
{
  char tmp  [ 3*MAX_STRING_LENGTH ];

  convert_to_ansi( ch, 3*MAX_STRING_LENGTH, text, tmp );
  page( ch, tmp );
}


/*
 *   SUBROUTINES
 */


void obj_loc_spam( char_data *ch, obj_data *obj, char_data *victim,
		   const char *& me, const char *& them,
		   const char_data *to )
{
  static char buf [ THREE_LINES ];

  if( obj->array == &ch->wearing ) {
    if( obj->position == WEAR_FLOATING ) {
      me = " floating nearby";
      snprintf( buf, THREE_LINES, " floating near %s",
		to ? ch->Name( to ) : ch->Him_Her() );
    } else if( obj->position == WEAR_HELD_R || obj->position == WEAR_HELD_L ) {
      if( obj->pIndexData->item_type == ITEM_WEAPON ) {
	me = " you are wielding";
	snprintf( buf, THREE_LINES, " %s is wielding",
		  to ? ch->Name( to ) : ch->He_She() );
      } else {
	me = " you are holding";
	snprintf( buf, THREE_LINES, " %s is holding",
		  to ? ch->Name( to ) : ch->He_She() );
      }
    } else {
      me = " you are wearing";
      snprintf( buf, THREE_LINES, " %s is wearing",
		to ? ch->Name( to ) : ch->He_She() );
    }
    them = buf;
  } else if( obj->array == &ch->contents ) {
    me = " you are carrying";
    snprintf( buf, THREE_LINES, " %s is carrying",
	      to ? ch->Name( to ) : ch->He_She() );
    them = buf;
  } else if( victim && victim != ch ) {
    if( obj->array == &victim->wearing ) {
      if( obj->position == WEAR_FLOATING ) {
	snprintf( buf, THREE_LINES, " floating near %s",
		  to ? victim->Name( to ) : victim->Name( ch ) );
	them = " floating nearby";
      } else if( obj->position == WEAR_HELD_R || obj->position == WEAR_HELD_L ) {
	if( obj->pIndexData->item_type == ITEM_WEAPON ) {
	  snprintf( buf, THREE_LINES, " %s is wielding",
		    to ? victim->Name( to ) : victim->Name( ch ) );
	  them = " you are wielding";
	} else {
	  snprintf( buf, THREE_LINES, " %s is holding",
		    to ? victim->Name( to ) : victim->Name( ch ) );
	  them = " you are holding";
	}
      } else {
	snprintf( buf, THREE_LINES, " %s is wearing",
		  to ? victim->Name( to ) : victim->Name( ch ) );
	them = " you are wearing";
      }
      me = buf;
    } else if( obj->array == &victim->contents ) {
      snprintf( buf, THREE_LINES, " %s is carrying",
		to ? victim->Name( to ) : victim->Name( ch ) );
      them = " you are carrying";
      me = buf;
    }
  } else {
    me = empty_string;
    them = empty_string;
  }
}


void obj_act_spam( int type, char_data *ch, obj_data *obj, char_data *victim,
		   const char *me, const char *them, bool page )
{
  const char *me_loc, *them_loc;

  obj_loc_spam( ch, obj, victim, me_loc, them_loc );

  if( page ) {
    fpage( ch, "You %s %s%s.", me, obj, me_loc );
  } else {
    fsend( ch, "You %s %s%s.", me, obj, me_loc );
  }

  msg_type = type;

  if( !victim || victim == ch ) {
    fsend_mesg( ch, "%s %s %s%s.", ch, them, obj, them_loc );
    return;
  }

  if( ch->array && !ch->was_in_room ) {
    if( ch->Seen( victim ) ) {
      fsend( victim, "%s %s %s%s.", ch, them, obj, them_loc );
    }
    for( int i = 0; i < *ch->array; ++i ) {
      char_data *rch = character( ch->array->list[i] );
      if( rch
	  && rch != ch
	  && rch != victim
	  && ch->Seen( rch )
	  && rch->Accept_Msg( ch ) ) {
	obj_loc_spam( ch, obj, victim,
		      me_loc, them_loc, rch );
	fsend( rch, "%s %s %s%s.", ch, them, obj, me_loc );
      }
   }
  }
}


void Obj_Data :: Look_At( char_data* ch )
{
  char_data *victim = character( array->where );

  obj_act_spam( MSG_LOOK, ch, this, victim,
		"look at", "looks at" );
  send( ch, "\n\r" );
  
  oprog_data *oprog;
  for( oprog = pIndexData->oprog; oprog; oprog = oprog->next ) {
    if( oprog->trigger == OPROG_TRIGGER_DESCRIBE
	&& !*oprog->target ) {
      push( );
      clear_variables( );
      var_room = Room( array->where );
      var_victim = victim;
      var_ch = ch;
      var_obj = this;
      var_arg = empty_string;
      if( oprog->execute( ) )
	oprog = 0;
      pop( );
      break;
    }
  }

  if( !oprog ) {
    page_descr( ch, is_set( extra_flags, OFLAG_IDENTIFIED ) 
		? after_descr( pIndexData ) : before_descr( pIndexData ) );
  }

  bool line = false;

  if( pIndexData->is_wearable( )
      || condition < Durability( ) ) {
    page( ch, "\n\r    Condition: %s\n\r",
	  condition_name( ch, true ) );
    line = true;
  }

  if( pIndexData->item_type == ITEM_WEAPON ) {
    if( !line ) {
      page( ch, "\n\r" );
      line = true;
    }
    page( ch, " Weapon Class: %s\n\r",
	  weapon_class( this ) );

  } else if( pIndexData->item_type == ITEM_DRINK_CON ) {
    if( !line ) {
      page( ch, "\n\r" );
      line = true;
    }
    page( ch, "       Volume: %.2f liters\n\r",
	  (double)value[0] / 100.0 );

  } else if( pIndexData->item_type == ITEM_CONTAINER ) {
    if( !line ) {
      page( ch, "\n\r" );
      line = true;
    }
    if( !is_set( value[1], CONT_CLOSED ) ) {
      page( ch, "     Capacity: %.2f lbs\n\r",
	    double( value[0] ) );
      page( ch, "     Contains: %.2f lbs\n\r", (double)contents.weight / 100.0 );
    }

  } else if( pIndexData->item_type == ITEM_TABLE ) {
    if( !line ) {
      page( ch, "\n\r" );
      line = true;
    }
    page( ch, "     Capacity: %.2f lbs\n\r",
	  double( value[0] ) );
    page( ch, "     Contains: %.2f lbs\n\r", (double)contents.weight / 100.0 );

  } else if( pIndexData->item_type == ITEM_CHAIR ) {
    if( !line ) {
      page( ch, "\n\r" );
      line = true;
    }
    if( value[0] == 1 ) {
      page( ch, "        Seats: one person.\n\r" );
    } else if( value[0] > 1 ) {
      page( ch, "        Seats: up to %d people.\n\r", value[0] );
    }
  }

  const bool sized = is_set( size_flags, SFLAG_RACE )
    || is_set( size_flags, SFLAG_SIZE );

  if( sized ) {
    if( !line ) {
      page( ch, "\n\r" );
    }
    page( ch, "         Size:" );
    line = false;
  }

  bool found = false;
  if( sized && is_set( size_flags, SFLAG_RACE ) ) {
    for( int i = 0; i < MAX_PLYR_RACE; ++i ) {
      if( is_set( size_flags, SFLAG_HUMAN+i ) ) {
	if( found ) {
	  page( ch, "," );
	} else {
	  found = true;
	}
	page( ch, " %s", plyr_race_table[i].name );
      }
    }
  }

  if( sized && !found && is_set( size_flags, SFLAG_SIZE ) ) {
    for( int i = SFLAG_TINY; i <= SFLAG_GIANT ; ++i ) {
      if( is_set( size_flags, i ) ) {
	if( found ) {
	  page( ch, "," );
	} else {
	  found = true;
	}
	page( ch, " %s", ::size_flags[i] );
      }
    }
  }

  if( found )
    page( ch, "\n\r" );
  else if( sized )
    page( ch, " [BUG]\n\r" );

  if( pIndexData->item_type == ITEM_DECK_CARDS ) { 
    page( ch, "\n\r" );
    look_cards( ch, this );
    
  } else if( pIndexData->item_type == ITEM_KEYRING ) {
    page( ch, "\n\r" );
    look_in( ch, this );
  }
}


const char* before_descr( obj_clss_data* obj )
{
  int fakes = obj->fakes;

  if( fakes == 0 )
    fakes = obj->vnum;

  if( !( obj = get_obj_index( fakes ) ) )
    return "Item is bugged - Fakes non-existent object.\n\r"; 

  for( int i = 0; i < obj->extra_descr; i++ ) {
    extra_data *ed = obj->extra_descr[i]; 
    if( !strcmp( ed->keyword, "either" )
	|| !strcmp( ed->keyword, "before" ) )
      return ed->text;
    }

  return "Item is bugged - Missing non-identified description.\n\r";
}


const char* after_descr( obj_clss_data* obj )
{
  for( int i = 0; i < obj->extra_descr; i++ ) {
    extra_data *ed = obj->extra_descr[i];
    if( !strcmp( ed->keyword, "either" )
	|| !strcmp( ed->keyword, "after" ) )
      return ed->text;
    }

  return "Item is bugged - Missing identified description.\n\r";
}


/*
  for( ed = obj->extra_descr; ; ed = ed->next ) {
  if( !ed ) {
  if( !first || !( ed = obj->pIndexData->extra_descr ) )
  break;
  first = false;
  }
  if( is_name( argument, ed->keyword ) 
  && strcmp( ed->keyword, "before" )
  && strcmp( ed->keyword, "after" )
  && strcmp( ed->keyword, "either" ) ) {  
  send_descr( ch, ed->text, obj, false );
  return true;
	}
	}
	
	return false;
	}
*/


/*
 *   EDITING ROUTINE
 */


bool edit_extra( extra_array& array, wizard_data* wizard, int offset,
		 const char *argument, char* text )
{
  char            arg  [ MAX_INPUT_LENGTH ];
  extra_data*      ed;
  int               i;
 
  extra_data **edit = (extra_data**) ( (int)(wizard) + offset );
  
  if( !*argument ) {
    if( array.is_empty() ) {
      page( wizard, "No extras found.\n\r" );
    } else {
      for( i = 0; i < array; ++i ) {
        page( wizard, "[%2d]  %s\n\r", i+1, array[i]->keyword );
        page( wizard, array[i]->text );
      }
    }
    return false;
  }

  if( exact_match( argument, "delete" ) ) {
    if( !number_arg( argument, i ) ) {
      page( wizard, "Which number extra do you want to remove?\n\r" );
      return false;
    }
    if( i < 0
	|| i > array ) {
      page( wizard, "Extra %d not found to remove.\n\r", i );
      return false;
    }
    --i;
    page( wizard, "Extra %d, '%s' removed.\n\r",
	  i+1, array[i]->keyword );
    *edit = array[i];
    array.remove( i );
    extract( wizard, offset, text );
    return true;
  }

  if( exact_match( argument, "new" ) ) {
    for( int j = 0; j < array; ++j ) {
      if( !strcasecmp( argument, array[j]->keyword ) ) {
	page( wizard, "An extra named '%s' already exists.\n\r", argument );
	return false;
      }
    }
    ed = new extra_data;
    ed->keyword = alloc_string( argument, MEM_EXTRA );
    ed->text = alloc_string( "No description.\n\r", MEM_EXTRA );
    *edit = ed; 
    array += ed;
    page( wizard, "Extra '%s' created.\n\r", ed->keyword );
    return true;
  }

  argument = one_argument( argument, arg );
  const char *save = arg;

  if( !number_arg( save, i )
      || --i < 0
      || i >= array ) {
    for( i = 0; ; i++ ) {
      if( i == array ) {
        page( wizard, "Extra not found.\n\r" );
        return false;
      }
      if( member( arg, array[i]->keyword ) ) 
        break;
    }
  }

  ed = array[i]; 

  if( *argument ) {
    for( int j = 0; j < array; ++j ) {
      if( !strcasecmp( argument, array[j]->keyword ) ) {
	page( wizard, "An extra named '%s' already exists.\n\r", argument );
	return false;
      }
    }
    free_string( ed->keyword, MEM_OBJ_CLSS );
    ed->keyword = alloc_string( argument, MEM_OBJ_CLSS );
    page( wizard, "Extra name set to '%s'.\n\r", ed->keyword );
    return true;
  }

  *edit = ed;

  page( wizard,
	"%s now operates on extra %d, '%s'.\n\r",
	text, i+1, ed->keyword );

  return false;
}
