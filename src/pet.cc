#include <ctype.h>
#include <stdlib.h>
#include <sys/types.h>
#include <stdio.h>
#include <syslog.h>
#include "define.h"
#include "struct.h"


/*
 *   NAME FUNCTIONS
 */


static char *valid_pet_name( char_data* ch, const char* name )
{
  int i;
  
  for( i = 0; name[i]; ++i ) {
    if( !isalpha( name[i] ) && name[i] != ' ' ) {
      send( ch, "Pet names may only contain letters and spaces.\n\r" );
      return 0;
    }
  }
  
  if( i >= 15 ) {
    send( ch, "Pet names must be less than 15 characters.\n\r" );
    return 0;
  } 
  
  char *new_name = alloc_string( name, MEM_MOBS );

  bool word = true;
  for( char *s = new_name; *s; ++s ) {
    if( word ) {
      *s = toupper( *s );
      word = false;
    } else {
      word = ( *s == ' ' );
    }
  }

  return new_name;
}


void do_name( char_data* ch, const char *argument )
{
  char           arg  [ MAX_INPUT_LENGTH ];
  char_data*  victim;

  argument = one_argument( argument, arg );

  if( !( victim = one_character( ch, arg, "name", ch->array ) ) )
    return;

  if( !victim->species
      || victim->leader != ch
      || !is_set( victim->status, STAT_PET ) ) {
    send( ch, "You can only name pets of your own.\n\r" );
    return;
  } 
  
  if( !*argument ) {
    send( ch, "What do you want to name them?\n\r" );
    return;
  }

  const char *name = valid_pet_name( ch, argument );

  if( !name ) {
    return;
  }

  fsend( ch, "%s is now named '%s'.", victim, name );

  free_string( ((mob_data*)victim)->pet_name, MEM_MOBS );
  // alloc_string() done by valid_pet_name().
  ((mob_data*)victim)->pet_name = name;
}


/*
 *   PET ROUTINES
 */


void abandon( player_data* pc, char_data* pet )
{
  pet->Show( 1 );
  fsend( pc, "You abandon %s.", pet );
  player_log( pc, "Abandoned pet %s.", pet->Name( ) );

  if( pc->familiar == pet )
    pc->familiar = 0;

  if( is_set( pet->status, STAT_FAMILIAR ) ) {
    fsend( pc, "%s leaves in disgust.", pet );
    fsend_seen( pc, "%s fades away with a hurt look at %s.", pet, pc );
    pet->Extract();
    return;
  }

  if( pet->shdata && pet->shdata->race == RACE_UNDEAD ) {
    pet->wearing.To( pet->contents );
    send_publ( pet, &pet->contents, "crumbles to dust", "dropping" );
    set_owner( pet->contents, 0, pc );
    pet->contents.To( pet->in_room->contents );
    pet->Extract();
    return;
  }

  if( pet->shdata && pet->shdata->race == RACE_GOLEM ) {
    pet->wearing.To( pet->contents );
    send_publ( pet, &pet->contents, "falls to pieces", "dropping" );
    set_owner( pet->contents, 0, pc );
    pet->contents.To( pet->in_room->contents );
    pet->Extract();
    return;
  }

  stop_follower( pet );
}


void do_pets( char_data* ch, const char *argument )
{
  char_data*        pet;
  char_data*     victim;
  bool            found  = false;
  int              i, j;

  if( is_mob( ch ) )
    return;

  player_data *pc = player( ch );

  if( exact_match( argument, "abandon" ) ) {
    j = atoi( argument );
    for( i = 0; i < ch->followers.size; i++ ) {
      if( is_pet( pet = ch->followers[i] ) && --j == 0 ) {
        abandon( pc, pet );
        return;
      }
    }
    send( ch, "You have no pet with that number.\n\r" );
    return;
  }

  bool loaded = false;

  if( !*argument ) {
    victim = ch;

  } else if( !has_permission( ch, PERM_PLAYERS ) ) {
    send( ch, "Unknown syntax - See help pets.\n\r" );
    return;

  } else {
    in_character = false;

    char arg [ MAX_INPUT_LENGTH ];

    argument = one_argument( argument, arg );

    if( pfile_data *pfile = find_pfile( arg ) ) {
      
      if( pfile != ch->pcdata->pfile
	  && pfile->trust >= get_trust( ch ) ) {
	fsend( ch, "You cannot view the pets of %s.", pfile->name );
	return;
      }
      
      if( !( victim = find_player( pfile ) ) ) {
	link_data link;
	link.connected = CON_PLAYING;
	if( !load_char( &link, pfile->name, PLAYER_DIR ) ) {
	  bug( "Load_players: error reading player file. (%s)", pfile->name );
	  return;
	}
	victim = link.player;
	loaded = true;
      }

    } else {
      if( !( victim = one_mob( ch, arg, "pets", (thing_array*) ch->array ) ) )
	return;
    }
  }
  
  for( i = j = 0; i < victim->followers.size; i++ ) {
    if( is_pet( pet = victim->followers[i] ) ) {
      if( !found ) {
	if( victim != ch ) {
	  page_title( ch, "Pets for %s", victim );
	}
        page_underlined( ch, "Num  Name                     Location\n\r" );
        found = true;
      }
      room_data *room = loaded ? pet->was_in_room : pet->in_room;
      if( ( has_permission( ch, PERM_PLAYERS )
	    || victim == ch && is_apprentice( ch ) )
	  && room ) {
	page( ch, "%3d  %-24s %s [%d]\n\r", ++j, 
	      pet->Seen_Name( ch ),
	      room->name,
	      room->vnum );
      } else {
	page( ch, "%3d  %-24s %s\n\r", ++j, 
	      pet->Seen_Name( ch ),
	      room ? room->name : "nowhere??" );
      }
    }
  }
  
  if( !found ) {
    if( ch == victim )
      page( ch, "You have no pets.\n\r" );
    else
      fpage( ch, "%s has no pets.", victim );
  }

  if( loaded ) {
    page( ch, "\n\r" );
    page_centered( ch, "[ Player file was loaded from disk. ]" );
    victim->Extract();
    extracted.delete_list();
  }
}


/*
 *   PET SUPPORT FUNCTIONS
 */


char_data *has_mount( char_data* ch, bool msg )
{
  for( int i = 0; i < ch->followers.size; ++i ) {
    char_data *mount = ch->followers[i];
    if( mount->species
	&& is_set( mount->species->act_flags, ACT_MOUNT ) ) {
      if( msg )
	send( ch, "You are only able to acquire one mount at a time.\n\r" );
      return mount;
    }
  }

  return 0;
}



bool has_elemental( char_data* ch )
{
  for( int i = 0; i < ch->followers; ++i ) {
    char_data *buddy = ch->followers[i];
    if( buddy->species
	&& is_set( buddy->species->act_flags, ACT_ELEMENTAL ) ) {
      return true;
    }
  }

  return false;
}

int number_of_pets( char_data* ch )
{
  int num = 0;

  for( int i = 0; i < ch->followers.size; i++ ) {
    char_data *pet = ch->followers[i];
    if( is_set( pet->status, STAT_PET )
	&& !is_set( pet->status, STAT_TAMED )
	&& !is_set( pet->species->act_flags, ACT_MOUNT ) ) 
      ++num;
    }

  return num;
}


int pet_levels( char_data* ch )
{
  int level = 0;

  for( int i = 0; i < ch->followers.size; i++ ) {
    char_data *pet = ch->followers[i];
    if( is_set( pet->status, STAT_TAMED ) )
      level += pet->Level();
  }
  
  return level;
}
