#include <ctype.h>
#include <sys/types.h>
#include <signal.h>
#include <stdio.h>
#include <syslog.h>
#include <stdlib.h>
#include "define.h"
#include "struct.h"


const char* memory_name [ MAX_MEMORY ] = {
  "Unknown", "Accounts",
  "Actions", "Affects", "Aliases", "Areas", "Arrays", "Auction",
  "Bad Names", "Bans", "Clans", "Code", "Customs", "Char. Descr.",
  "Dictionary", "Enemies", "Events",
  "Exits", "Extras", "Helps", "Info",
  "Links", "Memory", "Mobs", "Mob_Progs",
  "Notes", "Obj_Classes", "Objects", "Obj_Progs", "Paths",
  "Pfiles", "Players", "Programs", "Quests", "Queue",
  "Recognize", "Requests",
  "Resets", "Rooms", "Shdata", "Shops", "Species",
  "Spells", "Stack", "Tables", "Tells", "Tracks", "Trainers",
  "Votes", "Wizards"
};


int memory_number [ 2*MAX_MEMORY-1 ];
int memory_size   [ 2*MAX_MEMORY-1 ];


/*
 *   RECORD ROUTINES
 */


void record_new( const int size, const int type ) 
{
  memory_number [ MAX_MEMORY+type-1 ]++;
  memory_size   [ MAX_MEMORY+type-1 ] += size;
}


void record_delete( const int size, const int type )
{
  memory_number [ MAX_MEMORY+type-1 ]--;
  memory_size   [ MAX_MEMORY+type-1 ] -= size;
}


void do_memory( char_data* ch, const char *argument )
{
  int flags;
  if( !get_flags( ch, argument, &flags, "s", "memory" ) )
    return;

  if( is_set( flags, 0 ) ) {
    unsigned area_txt_in = 0;
    unsigned area_txt_out = 0;
    unsigned area_act_in = 0;
    unsigned area_act_out = 0;
    for( area_data *area = area_list; area; area = area->next ) {
      if( area->loaded ) {
	++area_txt_in;
      } else {
	++area_txt_out;
      }
      if( area->act_loaded ) {
	++area_act_in;
      } else {
	++area_act_out;
      }
    }

    const char *const line = "%18s %15u %15u %15u\n\r";

    page_underlined( ch, "%18s %15s %15s %15s\n\r",
		     "", "In Memory", "On Disk", "Total" );
    page( ch, line, "Area Text", area_txt_in, area_txt_out, area_txt_in + area_txt_out );
    page( ch, line, "Area Actions", area_act_in, area_act_out, area_act_in + area_act_out );
    return;
  }

  char    tmp  [ TWO_LINES ];
  int  number;
  int  blocks;
  int   bytes  = 0;

  for( int i = 0; i < 2*MAX_MEMORY-1; i++ )
    bytes += memory_size[i];

  page( ch, "      Mobs: %d\n\r", mob_list.size );
  page( ch, "   Objects: %d\n\r", obj_list.size );
  page( ch, "   Players: %d\n\r", player_list.size );
  page( ch, " Extracted: %d\n\r", extracted.size );
  page( ch, "\n\r" );

  page( ch, "Memory Allocated: %dk\n\r\n\r", bytes/1024 );

  snprintf( tmp, TWO_LINES, "%15s %8s %8s %8s\n\r", "Structure", "Number",
    "Blocks", "Bytes" );
  page_underlined( ch, tmp );

  for( int i = 0; i < MAX_MEMORY; i++ ) {
    number = memory_number[ MAX_MEMORY+i-1];
    blocks = number;
    bytes = memory_size[ MAX_MEMORY+i-1 ];
    if( i != 0 ) { 
      blocks += memory_number[ MAX_MEMORY-i-1 ];
      bytes += memory_size[ MAX_MEMORY-i-1 ];
    }
    snprintf( tmp, TWO_LINES, "%15s %8d %8d %8d\n\r",
	      memory_name[i], number, blocks, bytes );
    page( ch, tmp );
  }
}


/*
 *   INIT MEMORY
 */
 

void init_memory ()
{
  for( int i = 0; i < 2*MAX_MEMORY-1; i++ ) {
    memory_number[i] = 0;
    memory_size[i] = 0;
  }
}


/*
 *   STRINGS 
 */


char *alloc_string( const char *string, int type )
{
  if( !string )
    return 0;

  const size_t length  = strlen( string )+1;

  if( length == 1 )
    return empty_string;

  char *string_new = new char[length];
  memcpy( string_new, string, length );

  record_new( length, -type );

  return string_new;
}


void free_string( const char *string, int type )
{
  if( string && string != empty_string ) {
    record_delete( strlen( string )+1, -type );
    delete [] string;
  }
}


/*
 *   EDITED OBJECTS
 */


player_data *edit_lock( player_data *pl, void *edit, int offset, const char *text, bool unlock )
{
  //  void **edit = (void**)( ((int) pl )+offset );

  for( int i = 0; i < player_list; ++i ) {
    player_data *ch = player_list[i];
    if( !ch->Is_Valid( )
	|| ch == pl )
      continue;
    void **pntr = (void**)( ((int) ch )+offset );
    if( edit == *pntr ) {
      if( unlock
	  && get_trust( pl ) > get_trust( ch ) ) {
	fsend( pl, "Removed %s's lock on that %s.", ch, text );
	send_color( ch, COLOR_WIZARD,
		    "** %s just removed your lock on the %s you were editing. **\n\r",
		    pl, text );
	*pntr = 0;
	return 0;
      }
      return ch;
    }
  }

  return 0;
}


void extract( wizard_data *imm, int offset, const char *text )
{
  char tmp [ TWO_LINES ];
  snprintf( tmp, TWO_LINES,
	    "** %s just deleted the %s you were editing. **\n\r",
	    imm->real_name( ),
	    text );

  void **edit = (void**)( ((int) imm )+offset );
  
  for( int i = 0; i < player_list; ++i ) {
    player_data *ch = player_list[i];
    if( !ch->Is_Valid( )
	|| ch == imm
	|| !wizard( ch ) )
      continue;
    void **pntr = (void**)( ((int) ch )+offset );
    if( *edit == *pntr ) {
      *pntr = 0;
      send_color( ch, COLOR_WIZARD, tmp );
    }
  }
  
  *edit = 0;
}
