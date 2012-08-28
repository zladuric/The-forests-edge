#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include "define.h"
#include "struct.h"


const char* oprog_trigger[ MAX_OPROG_TRIGGER ] = {
  "put", "get", "timer", "hit", "none", "to_room", "entering", "wear", "consume",
  "sit", "random", "untrap", "use", "unlock", "lock", "throw", "leaving", "remove",
  "load", "cast", "reset", "describe",
  "get_from"
};


const default_data *oprog_msgs [ MAX_OPROG_TRIGGER ] =
{
  0,
  0,
  timer_msg,
  0,
  0,
  0,
  0,
  wear_msg,
  consume_msg,
  0,
  0,
  0,
  use_msg,
  unlock_msg,
  lock_msg,
  0,
  0,
  remove_msg,
  0,
  0,
  0,
  0
};


const char *oprog_flags [ MAX_OPFLAG ] =
{
  "inventory",
  "worn",
  "room"
};


Oprog_Data::Oprog_Data( obj_clss_data *clss )
  : next(0), obj_act(0), obj_vnum(0),
    trigger(OPROG_TRIGGER_NONE),
    target(empty_string), command(empty_string),
    value(-1),
    flags( ( 1<<OPFLAG_INVENTORY )
	   | ( 1<<OPFLAG_ROOM )
	   | ( 1<<OPFLAG_WORN ) ),
    obj_clss(clss)
{
  record_new( sizeof( oprog_data ), MEM_OPROG );
  //  record_delete( sizeof( program_data ), MEM_PROGRAM );
  append( clss->oprog, this );
}


Oprog_Data::~Oprog_Data( )
{
  record_delete( sizeof( oprog_data ), MEM_OPROG );
  //  record_new( sizeof( program_data ), MEM_PROGRAM );
  free_string( command, MEM_OPROG );
  free_string( target,  MEM_OPROG );
}


int Oprog_Data::execute( thing_data *owner )
{
  obj_clss->used = true;

  if( !owner )
    owner = var_obj;

  return program_data::execute( owner );
}


bool Oprog_Data::abort( thing_data *owner ) const
{
  if( trigger == OPROG_TRIGGER_HIT ) {
    return ( !owner || !orig_ch || !orig_victim );
  }

  return false;
}


/*
 *   DISPLAY ROUTINE
 */


void oprog_data :: display( char_data* ch ) const
{
  if( !obj_clss ) {
    page( ch, "%-10s %-10s %s\n\r", "??", "Oprog", "Null object class??" );
    return;
  }

  int i = 1;

  for( oprog_data *oprog = obj_clss->oprog; oprog != this && oprog; oprog = oprog->next, i++ );

  page( ch, "Obj %-11d Oprog %-8d %s\n\r",
	obj_clss->vnum, i, obj_clss->Name() );
}     


/*
 *   EDITING ROUTINES
 */


static oprog_data *find_oprog( char_data *ch, obj_clss_data *obj_clss, int i )
{
  oprog_data *oprog = 0;

  if( i >= 1 ) {
    int j = i;
    for( oprog = obj_clss->oprog; oprog && j != 1; oprog = oprog->next, --j );
  }

  if( !oprog ) {
    send( ch, "No oprog number %d.\n\r", i );
  }

  return oprog;
}


/*
static void extract( oprog_data *oprog, wizard_data *wizard )
{
  for( int i = 0; i < oprog->data; ++i ) {
    wizard->opdata_edit = oprog->data[i];
    extract( wizard, offset( &wizard->opdata_edit, wizard ), "opdata" );
  }

  wizard->oprog_edit = oprog;
  extract( wizard, offset( &wizard->oprog_edit, wizard ), "oprog" );
}
*/


static void display_oprog( char_data* ch, obj_clss_data *clss )
{
  if( !clss->oprog ) {
    send( ch, "This object has no programs.\n\r" );
    return;
  }
  
  size_t len = 20;
  for( oprog_data *oprog = clss->oprog; oprog; oprog = oprog->next ) {
    len = max( len, strlen( oprog->command ) );
  }

  page_underlined( ch, "   #  %*s  %s\n\r", len, "Trigger", "Target" );

  int i = 0;

  for( oprog_data *oprog = clss->oprog; oprog; oprog = oprog->next ) {
    if( oprog->trigger == OPROG_TRIGGER_NONE ) {
      page( ch, "[%2d]  %*s  %s\n\r",
	    ++i, len, oprog->command, oprog->target );
    } else {
      page( ch, "[%2d]  %s%*s%s  %s\n\r",
	    ++i,
	    color_code( ch, COLOR_MILD ),
	    len, oprog_trigger[oprog->trigger],
	    color_code( ch, COLOR_DEFAULT ),
	    *oprog->command ? "" : oprog->target );
      if( *oprog->command ) {
	page( ch, "      %*s  %s\n\r",
	      len, oprog->command, oprog->target );
      }
    }
  }
    /*
    switch( oprog->trigger ) {
    case OPROG_TRIGGER_NONE :
      snprintf( tmp, MAX_INPUT_LENGTH, "%20s %s",
		oprog->command, oprog->target );
      break;
    case OPROG_TRIGGER_CAST :
    case OPROG_TRIGGER_DESCRIBE :
      snprintf( tmp, MAX_INPUT_LENGTH, "%20s %s",
		oprog_trigger[oprog->trigger], oprog->target );
      break;
    default :
      snprintf( tmp, MAX_INPUT_LENGTH, "%20s",
		oprog_trigger[oprog->trigger] );
      break;
    }
    page( ch, "[%2d]  %s\n\r", ++i, tmp );
  }
    */
}


void do_opedit( char_data* ch, const char *argument )
{
  obj_clss_data*  obj_clss;
  oprog_data*        oprog;
  int                    i;

  wizard_data *wizard = (wizard_data*) ch;

  if( !( obj_clss = wizard->obj_clss_edit ) ) {
    send( ch, "You aren't editing any object type - use oedit <obj>.\n\r" );
    return;
  }
  
  if( !*argument ) {
    display_oprog( ch, obj_clss );
    return;
  }
  
  if( !ch->can_edit( obj_clss ) )
    return;

  if( number_arg( argument, i ) ) {
    if( !( oprog = find_oprog( ch, obj_clss, i ) ) ) {
      return;
    }
    int j;
    if( isdigit( *argument ) && number_arg( argument, j ) ) {
      //      if( !ch->can_edit( obj_clss ) )
      //	return;
      if( j == i ) {
        send( ch, "Moving an oprog to where it already is does nothing interesting.\n\r" ); 
        return;
      }
      if( j == 1 ) {
        remove( obj_clss->oprog, oprog );
        oprog->next = obj_clss->oprog;
        obj_clss->oprog = oprog;
      } else {
        if( j < 1 || j > count( obj_clss->oprog ) ) {
          send( ch, "You can only move an oprog to a sensible position.\n\r" );
          return;
	}
        remove( obj_clss->oprog, oprog );
        oprog_data *prev = locate( obj_clss->oprog, j-1 );
        oprog->next = prev->next;
        prev->next = oprog;

      }
      obj_clss->set_modified( ch );
      send( ch, "Oprog %d moved to position %d.\n\r", i, j );
      return;
    }
    wizard->oprog_edit = oprog;
    wizard->opdata_edit = 0;
    send( ch, "You now edit oprog %d.\n\r", i );
    return;
  }
  
  if( exact_match( argument, "new" ) ) {
    oprog = new oprog_data( obj_clss );
    if( *argument ) {
      for( i = 0; i < MAX_OPROG_TRIGGER; ++i ) {
	if( !strcasecmp( argument, oprog_trigger[i] ) ) {
	  oprog->trigger = i;
	  fsend( ch, "Oprog added with %s trigger.", oprog_trigger[i] );
	  break;
	}
      }
      if( i == MAX_OPROG_TRIGGER ) {
	oprog->command = alloc_string( argument, MEM_OPROG );
	fsend( ch, "Oprog added with command trigger \"%s\".", oprog->command );
      }
    } else {
      send( ch, "Oprog added with no trigger.\n\r" );
    }
    wizard->oprog_edit = oprog;
    wizard->opdata_edit = 0;
    obj_clss->set_modified( ch );
    return;
  }
  
  if( exact_match( argument, "delete" ) ) {
    if( !*argument ) {
      if( !( oprog = wizard->oprog_edit ) ) {
	send( ch, "You aren't editing any oprog.\n\r" );
	return;
      }
    } else {
      if( !number_arg( argument, i ) ) {
	send( ch, "Syntax: opedit delete [#]\n\r" );
	return;
      }
      if( !( oprog = find_oprog( ch, obj_clss, i ) ) ) {
	return;
      }
    }
    send( ch, "Oprog deleted.\n\r" );
    oprog_data *old_edit = 0;
    if( wizard->oprog_edit != oprog ) {
      old_edit = wizard->oprog_edit;
      wizard->oprog_edit = oprog;
    }
    extract( wizard, offset( &wizard->oprog_edit, wizard ), "oprog" );
    remove( obj_clss->oprog, oprog );
    delete oprog;
    obj_clss->set_modified( ch );
    wizard->oprog_edit = old_edit;
    return;
  }

  send( ch, "Illegal syntax.\n" );
}


void do_opcode( char_data* ch, const char *argument )
{
  oprog_data*          oprog;

  wizard_data *wizard = (wizard_data*) ch;

  /*
  if( !( obj_clss = wizard->obj_clss_edit ) ) {
    send( ch, "You aren't editing any object type.\n\r" );
    return;
  }
  */
  
  if( !( oprog = wizard->oprog_edit ) ) {
    send( ch, "You aren't editing any oprog.\n\r" );
    return;
  }

  obj_clss_data *obj_clss = oprog->obj_clss;

  //  if( *argument && !ch->can_edit( obj_clss ) )
  //    return;

  oprog->Edit_Code( ch, argument );

  if( *argument )
    obj_clss->set_modified( ch );

  if( *argument || !oprog->binary ) {
    var_ch = ch;
    oprog->compile( );
  }
}


void do_opdata( char_data* ch, const char *argument )
{
  oprog_data*        oprog;

  wizard_data *wizard = (wizard_data*) ch;

  /*
  if( !( obj_clss = wizard->obj_clss_edit ) ) {
    send( ch, "You aren't editing any object type.\n\r" );
    return;
  }
  */
  
  if( !( oprog = wizard->oprog_edit ) ) {
    send( ch, "You aren't editing any oprog.\n\r" );
    return;
  }
  
  obj_clss_data *obj_clss = oprog->obj_clss;

  //  if( *argument && !ch->can_edit( obj_clss ) )
  //    return;
 
  if( wizard->opdata_edit ) {
    if( exact_match( argument, "exit" ) ) {
      wizard->opdata_edit = 0;
      send( ch, "Opdata now operates on the data list.\n\r" );
      return;
    }
    wizard->opdata_edit->text = edit_string( ch, 
					     argument, wizard->opdata_edit->text, MEM_EXTRA, true );
    if( *argument ) {
      obj_clss->set_modified( ch );
    }

  } else {
    if( !*argument ) {
      show_defaults( ch, oprog->trigger, oprog_msgs, obj_clss->item_type );
    }
    
    if( edit_extra( oprog->Extra_Descr( ), wizard, offset( &wizard->opdata_edit, wizard ),
		    argument, "opdata" ) ) {
      obj_clss->set_modified( ch );
    }
  }

  if( *argument || !oprog->binary ) {
    var_ch = ch;
    oprog->compile( );
  }
}


static void show_flags( char_data *ch, oprog_data *oprog )
{
  if( oprog->trigger != OPROG_TRIGGER_NONE )
    return;

  page( ch, "%10s : ", "Flags" );

  bool found = false;

  for( int i = 0; i < MAX_OPFLAG; ++i ) {
    if( is_set( oprog->flags, i ) ) {
      if( !found ) {
	found = true;
	page( ch, oprog_flags[i] );
      } else {
	page( ch, ", %s", oprog_flags[i] );
      }
    }
  }

  if( !found ) {
    page( ch, "none\n\r\n\r" );
  } else {
    page( ch, "\n\r\n\r" );
  }
}


void do_opstat( char_data* ch, const char *argument )
{
  oprog_data*     oprog;

  wizard_data *wizard = (wizard_data*) ch;

  int num;

  if( !*argument ) {
    if( !( oprog = wizard->oprog_edit ) ) {
      send( ch, "You aren't editing any oprog.\n\r" );
      return;
    }
  } else if( number_arg( argument, num ) ) {
    if( obj_clss_data *obj_clss = wizard->obj_clss_edit ) {
      if( !( oprog = find_oprog( ch, obj_clss, num ) ) ) {
	return;
      }
    } else {
      send( ch, "You aren't editing any object type.\n\r" );
      return;
    }
  } else {
    send( ch, "Syntax: opstat [<oprog #>]\n\r" );
    return;
  }

  page( ch, "%10s : %s\n\r", "Trigger",
	oprog_trigger[ oprog->trigger ] );
  
  page( ch, "%10s : %s\n\r", "Obj_Act",
	oprog->obj_act ? oprog->obj_act->Name( ) : "none" );

  page( ch, "%10s : %s\n\r", "Command", oprog->command );
  page( ch, "%10s : %s\n\r", "Target", oprog->target );
  page( ch, "%10s : %d\n\r", "Rand_Value", oprog->value );

  show_flags( ch, oprog );

  /*
  // Use buf for long string length.
  char buf [ 3*MAX_STRING_LENGTH ];
  snprintf( buf, 3*MAX_STRING_LENGTH, "\n\r[Code]\n\r%s\n\r", oprog->Code( ) );
  page( ch, buf );
  */
  page( ch, "\n\r[Code]\n\r%s\n\r", oprog->Code( ) );
  show_extras( ch, oprog->Extra_Descr( ) );
}


void do_opset( char_data* ch, const char *argument )
{
  oprog_data*     oprog;

  wizard_data *wizard = (wizard_data*) ch;  

  /*
  if( !wizard->obj_clss_edit ) {
    send( ch, "You aren't editing any object type.\n\r" );
    return;
  }
  */
  
  if( !( oprog = wizard->oprog_edit ) ) {
    send( ch, "You aren't editing any oprog.\n\r" );
    return;
  }
  
  if( !*argument ) {
    do_opstat( ch, "" );
    return;
  }
  
  obj_clss_data *obj_clss = oprog->obj_clss;

#define ot( i )  oprog_trigger[i]

  class type_field type_list[] = {
    { "trigger",   MAX_OPROG_TRIGGER,  &ot(0),  &ot(1),  &oprog->trigger, true },
    { "" }
  };

#undef ot

  if( const char *result = process( type_list, ch, "opset", argument ) ) {
    if( *result )
      obj_clss->set_modified( ch );
    return;
  }

  int obj_act = oprog->obj_act ? oprog->obj_act->vnum : 0;

  if( obj_clss_arg( ch, argument, obj_clss, 0, "obj_act", obj_act ) ) {
    oprog->obj_act = get_obj_index( obj_act );
    return;
  }

  class string_field string_list[] = {
    { "target",    MEM_OPROG,   &oprog->target,    0 },
    { "command",   MEM_OPROG,   &oprog->command,   0 },
    { "",          0,           0,                 0 }
  };

  if( const char *result = process( string_list, ch, "opset", argument ) ) {
    if( *result )
      obj_clss->set_modified( ch );
    return;
  }

  send( ch, "Syntax: opset <field> <value>\n\r" );
}


void do_opflag( char_data *ch, const char *argument )
{
  wizard_data *wizard = (wizard_data*) ch;

  oprog_data *oprog;

  /*
  if( !wizard->obj_clss_edit ) {
    send( ch, "You aren't editing any object type.\n\r" );
    return;
  }
  */
  
  if( !( oprog = wizard->oprog_edit ) ) {
    send( ch, "You aren't editing any oprog.\n\r" );
    return;
  }
  
  if( !*argument ) {
    display_flags( "Object Program",
		   &oprog_flags[0], &oprog_flags[1],
		   &oprog->flags, MAX_OPFLAG, ch );
    return;
  }
  
  int max = MAX_OPFLAG;

  if( !set_flags( &oprog_flags[0], &oprog_flags[1],
		  &oprog->flags, 0,
		  max, 0,
		  ch, argument, "oprog",
		  false, true ) ) 
    return;
}
