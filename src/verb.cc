#include <sys/types.h>
#include <stdio.h>
#include <syslog.h>
#include "define.h"
#include "struct.h"


void do_climb( char_data* ch, const char *argument )
{
  if( !*argument ) {
    send( ch, "What do you want to climb?\n\r" );
    return;
  }
  
  send( ch, "Whatever that is, you can't climb it.\n\r" );
}


void do_enter( char_data* ch, const char *argument )
{
  if( !ch->in_room )
    return;

  if( !ch->Can_See( true ) )
    return;

  if( !*argument ) {
    send( ch, "Enter what?\n\r" );
    return;
  }

  // Warning: don't use move_char, it bypasses any command acodes like "1 north".

  // First match the names of the doors.
  if( const exit_data *exit = one_exit( ch, argument, empty_string,
					ch->in_room ) ) {
    interpret( ch, dir_table[ exit->direction ].name );
    //    move_char( ch, exit( vis )->direction, false );
    return;
  }

  /*
  if( const visible_data *vis = one_visible( ch, argument, empty_string,
					     (visible_array *) &ch->in_room->exits, EXIT_DATA ) ) {
    interpret( ch, dir_table[ exit( vis )->direction ].name );
    //    move_char( ch, exit( vis )->direction, false );
    return;
  }
  */

  // Now try to match the names of the adjacent rooms.
  bool found = false;
  const exit_data *move = 0;
  for( int i = 0; i < ch->in_room->exits; i++ ) {
    const exit_data *door = ch->in_room->exits[i];
    if( door->Seen( ch )
	&& !is_set( door->exit_info, EX_CLOSED )
	&& door->to_room->Seen( ch )
	&& is_name( argument, door->to_room->Name( ch ) ) ) {
      if( found ) {
	fsend( ch, "More than one exit matches \"%s\".", argument );
	return;
      }
      move = door;
      found = true;
    }
  }

  if( move ) {
    interpret( ch, dir_table[ move->direction ].name );
    //    move_char( ch, move->direction, false );
    return;
  }

  fsend( ch, "Nothing found matching \"%s\".", argument );
}


void do_move( char_data* ch, const char *argument )
{
  if( *argument == '\0' ) {
    send( ch, "What do you want to move?\n\r" );
    return;
  }
 
  send( ch,
	"Whatever that is, trying to move it does nothing interesting.\n\r" );
}

 
void do_pull( char_data* ch, const char *argument )
{
  if( *argument == '\0' ) {
    send( ch, "What do you want to pull?\n\r" );
    return;
  }
  
  send( ch,
	"Whatever that is, pulling it does nothing interesting.\n\r" );
}


void do_push( char_data* ch, const char *argument )
{
  if( *argument == '\0' ) {
    send( ch, "What do you want to push?\n\r" );
    return;
  }
  
  send( ch,
	"Whatever that is, pushing it does nothing interesting.\n\r" );
}


void do_read( char_data* ch, const char *)
{
  send( ch, "Whatever that is, you can't read it.  Perhaps you should try looking at it.\n\r" );
}
