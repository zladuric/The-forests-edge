#include <ctype.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include "define.h"
#include "struct.h"


trainer_data *trainer_list;


Trainer_Data :: Trainer_Data( )
  : next(0), room(0)//, mob(0)
{
  record_new( sizeof( trainer_data ), MEM_TRAINER );

  for( int i = 0; i < MAX_SKILL_CAT; ++i ) {
    const int n = ( table_max[ skill_table_number[ i ] ] + 31 ) / 32;
    record_new( n*sizeof( int ), -MEM_TRAINER );
    skills[i] = new int[ n ];
    vzero( skills[i], n );
  }
}  


Trainer_Data :: ~Trainer_Data( )
{
  record_delete( sizeof( trainer_data ), MEM_TRAINER );

  for( int i = 0; i < MAX_SKILL_CAT; ++i ) {
    const int n = ( table_max[ skill_table_number[ i ] ] + 31 ) / 32;
    record_delete( n*sizeof( int ), -MEM_TRAINER );
    delete [] skills[i];
  }
}


/*
 *  SET TRAINER 
 */


void set_trainer( mob_data* mob, room_data* room )
{
  for( trainer_data *trainer = trainer_list; trainer; trainer = trainer->next ) {
    if( trainer->room == room
	//	&& !trainer->mob
	&& trainer->trainer == mob->species->vnum ) {
      mob->pTrainer = trainer;
      //      trainer->mob = mob;
      break;
    }
  }
}


/*
 *   TFIND ROUTINE
 */


void do_tfind( char_data* ch, const char *argument )
{
  if( !*argument ) { 
    send( ch, "Syntax: tfind <skill>\n\r" );
    return;
  }

  bool first  = true;
  bool found;

  for( int j = 0; j < MAX_SKILL_CAT; ++j ) {
    const int m = table_max[ skill_table_number[ j ] ];
    for( int i = 0; i < m; ++i ) {
      Skill_Type *entry = skill_entry( j, i );
      if( !fmatches( argument, entry->name ) )
	continue;
      
      if( !first ) 
	page( ch, "\n\r" );
      
      page( ch, "%s:\n\r", entry->name );
      
      found = false;
      first = false;
      
      for( trainer_data *trainer = trainer_list; trainer; trainer = trainer->next ) {
	species_data *species = get_species( trainer->trainer );
	room_data *room = trainer->room; 
	if( is_set( trainer->skills[j], i ) ) {
	  page( ch, "  [%5d] %2d - %-30s [%5d] %s\n\r",
		trainer->trainer,
		species ? (int)species->shdata->skills[j][i] : 0,
		species ? species->Name( ) : "no one?",
		trainer->room->vnum, room ? room->name : "nowhere?" );
	  found = true;
	}
      }
      
      if( !found )
	page( ch, "  no trainers found\n\r" );
    }
  }
  
  if( first )
    send( ch, "No skill by that name found.\n\r" );
}


/*
 *   TRAIN ROUTINE
 */


void do_train( char_data* ch, const char *argument )
{
  int flags;
  if( !get_flags( ch, argument, &flags, "a", "train" ) )
    return;

  if( !*argument ) {
    const bool all = is_set( flags, 0 );
    bool found = false;
    for( const trainer_data *trainer = trainer_list; trainer; trainer = trainer->next ) {
      for( int i = 0; i < mob_list; ++i ) {
	mob_data *mob = mob_list[i];
	if( mob->Is_Valid( ) && mob->pTrainer == trainer ) {
	  if( species_data *species = get_species( trainer->trainer ) ) {
	    if( mob->in_room == ch->in_room ) {
	      // Trainer is here.
	      if( !found ) {
		page_underlined( ch, "%-7s %-40s  %s\n\r", "Vnum", "Name", "Where" );
		found = true;
	      }
	      page( ch, "[%5d] %-40s  ",
		    trainer->trainer, species->Name( true, false, false ) );
	      if( trainer->room == ch->in_room ) {
		page( ch, "Reset here.\n\r" );
	      } else {
		page( ch, "Wandered here from [%5d].\n\r",
		      trainer->room->vnum );
	      }

	    } else if( all || trainer->room == ch->in_room ) {
	      // Trainer resets here.
	      if( !found ) {
		page_underlined( ch, "%-7s %-40s  %s\n\r", "Vnum", "Name", "Where" );
		found = true;
	      }
	      page( ch, "[%5d] %-40s  ",
		    trainer->trainer, species->Name( true, false, false ) );
	      page( ch, "[%5d] %s\n\r",
		    mob->in_room->vnum, mob->in_room->name );
	    }
	  } else {
	    page( ch, "[Error] Trainer with bad species.\n\r" );
	  }
	}
      }
    }
    if( !found ) {
      send( ch, "No trainers found here.\n\r" );
    }
    return;
  }
  
  char arg [ MAX_INPUT_LENGTH ];
  argument = one_argument( argument, arg );
  
  char_data*       victim;
  mob_data*           npc;
  bool              found  = false;
  
  if( !( victim = one_character( ch, arg, "train", ch->array ) ) ) 
    return;

  if( !( npc = mob( victim ) ) ) {
    send( ch, "Players can't be trainers.\n\r" );
    return;
  }
  
  trainer_data *trainer = npc->pTrainer;

  if( !strcasecmp( argument, "new" ) ) {
    if( !npc->reset ) {
      fsend( ch, "%s was not created by a reset." , victim);
      return;
    }
    if( trainer ) {
      fsend( ch, "%s is already a trainer.", victim );
      return;
    }
    if( !is_listed( ch->in_room->reset, npc->reset ) ) {
      fsend( ch, "%s did not reset here.", victim );
      return;
    }
    trainer          = new trainer_data;
    npc->pTrainer    = trainer;
    trainer->room    = ch->in_room;
    trainer->trainer = npc->species->vnum;
    append( trainer_list, trainer );
    fsend( ch, "%s set as a trainer.", victim );
    return;
  }
  
  if( !trainer ) {
    fsend( ch, "%s is not a trainer.", victim );
    return;
  }
  
  if( !strcasecmp( argument, "delete" ) ) {
    remove( trainer_list, trainer );
    delete trainer;
    npc->pTrainer = 0;
    fsend( ch, "%s is no longer a trainer.", victim );  
    return;
  }          
  
  if( !*argument ) {
    for( int j = 0; j < MAX_SKILL_CAT; ++j ) {
      int m = table_max[ skill_table_number[ j ] ];
      for( int i = 0; i < m; ++i ) {
	if( is_set( trainer->skills[j], i ) ) {
	  if( !found ) {
	    found = true;
	    fsend( ch, "%s can train the following skills:",
		   victim->Seen_Name( ch ) );
	  } 
	  send( ch, "  %s to %d\n\r",
		skill_entry( j, i )->name,
		(int)victim->species->shdata->skills[j][i] );
	}
      }
    }
    if( !found ) 
      fsend( ch, "%s trains no skills.", victim->Seen_Name( ch ) );
    return;
  }

  for( int j = 0; j < MAX_SKILL_CAT; ++j ) {
    int m = table_max[ skill_table_number[ j ] ];
    for( int i = 0; i < m; ++i ) {
      Skill_Type *entry = skill_entry( j, i );
      if( fmatches( argument, entry->name ) ) {
	switch_bit( trainer->skills[j], i );
	fsend( ch, "%s can %s train %s.",
	       victim, is_set( trainer->skills[j], i ) ?
	       "now" : "no longer", entry->name );
	return;
      }
    }
  }

  send( ch, "Unknown skill.\n\r" );
}    


/*
 *   DISK ROUTINES
 */


void load_trainers( void )
{
  echo( "Loading Trainers...\n\r" );

  /*
  if( MAX_TRAIN*32 < MAX_SKILL ) 
    panic( "Load_Trainers: Skill array insufficent." );
  */

  FILE *fp = open_file( AREA_DIR, TRAINER_FILE, "r" );

  if( strcmp( fread_word( fp ), "#TRAINERS" ) ) 
    panic( "Load_trainers: header not found" );

  char*              word;
  char             letter;
  int                room;

  while( true ) {
    if( ( room = fread_number( fp ) ) == -1 )
      break;

    trainer_data *trainer = new trainer_data;
    trainer->trainer = fread_number( fp );
    trainer->room = get_room_index( room );

    species_data *species = get_species( trainer->trainer );

    if( !species ) {
      roach( "Load_Trainers: Deleting as mob %d is non-existent.", trainer->trainer );
      delete trainer;
      trainer = 0;
    } else if( !trainer->room ) {
      roach( "Load_Trainers: Deleting as room %d is non-existent.", room );
      delete trainer;
      trainer = 0;
    } else {
      reset_data *reset = trainer->room->reset;
      for( ; reset; reset = reset->next ) {
        if( reset->vnum == trainer->trainer
	    && is_set( reset->flags, RSFLAG_MOB ) ) {
          break;
	}
      }
      if( !reset ) {
	roach( "Load_Trainers: Deleting trainer, no reset of mob %d in room %d.",
	       trainer->trainer,
	       room );
	delete trainer;
	trainer = 0;
      } else {
	append( trainer_list, trainer );
      }
    }

    while( true ) {
      letter = fread_letter( fp );
      ungetc( letter, fp );
      if( isdigit( letter ) || letter == '-' )
        break;
      word = fread_word( fp );
      int number = skill_index( word );
      if( number != -1 ) {
	if( trainer ) {
	  set_bit( trainer->skills[ skill_table( number ) ], skill_number( number ) );
	}
      } else {
	panic( "Load_Trainers: Unknown Skill - %s", word );
      }
      /*
      if( strcmp( word = fread_word( fp ), "none" ) ) {
	bool done = false;
	for( int j = 0; !done && j < MAX_SKILL_CAT; ++j ) {
	  int m = table_max[ skill_table_number[ j ] ];
	  for( int i = 0; i < m; ++i ) {
	    if( !strcmp( word, skill_entry( j, i )->name ) ) {
	      if( trainer ) {
		set_bit( trainer->skills[j], i );
	      }
	      done = true;
	      break;
	    }
	  }
	}
	if( !done )
	  panic( "Load_Trainers: Unknown Skill - %s", word );
      }
      */
    }
  }
  
  fclose( fp );
}


void save_trainers( )
{
  rename_file( AREA_DIR, TRAINER_FILE,
	       AREA_PREV_DIR, TRAINER_FILE );
  
  FILE *fp = open_file( AREA_DIR, TRAINER_FILE, "w" );

  if( !fp ) 
    return;

  fprintf( fp, "#TRAINERS\n" );

  for( trainer_data *trainer = trainer_list; trainer; trainer = trainer->next ) {
    fprintf( fp, "%5d %5d  ", trainer->room->vnum, trainer->trainer );
    for( int j = 0; j < MAX_SKILL_CAT; ++j ) {
      const int m = table_max[ skill_table_number[ j ] ];
      for( int i = 0; i < m; ++i ) {
	if( is_set( trainer->skills[j], i ) )
	  fprintf( fp, " '%s'", skill_entry( j, i )->name );
      }
    }
    fprintf( fp, "\n" );
  }
  
  fprintf( fp, "-1\n\n#$\n\n" );
  fclose( fp );
}


/*
 *   PRACTICE ROUTINES
 */


/* Check all the prerequisites for the skill that the player is trying to 
 * learn.  The skill is passed in i. 
 */ 

static bool can_study( char_data* ch, int i )
{
  char tmp  [ TWO_LINES ];
  char *t = tmp;
  const Skill_Type  *entry  = skill_entry( i );
  const int             n1  = entry->pre_skill[0];
  const int             n2  = entry->pre_skill[1];
  const skill_type*    ps1 = ( n1 == SKILL_NONE ? 0 : skill_entry( n1 ) );
  const skill_type*    ps2 = ( n2 == SKILL_NONE ? 0 : skill_entry( n2 ) );

  /* Don't have the prereq. */

  if( ( ps1 && (int)ch->get_skill( n1 ) < entry->pre_level[0] )
      || ( ps2 && (int)ch->get_skill( n2 ) < entry->pre_level[1] ) ) {
    t += sprintf( t, "Before you are prepared to study %s you need to know ",
		  entry->name );
    if( !ps2 ) 
      t += sprintf( t, "%s at level %d.",
		    ps1->name, entry->pre_level[0] );
    else
      t += sprintf( t,
		    "%s at level %d and %s at level %d.",
		    ps1->name, entry->pre_level[0], ps2->name, entry->pre_level[1] );
    fsend( ch, tmp );
    return false;
  }

  /* Do you meet the minimum level for that skill? */

  if( ch->Level() < entry->level[ ch->pcdata->clss ] ) {
    fsend( ch, "To study %s you must be at least level %d.",
	   entry->name, entry->level[ ch->pcdata->clss ] );
    return false;
  }  

  return true;
}


void do_practice( char_data* ch, const char *argument )
{ 
  char                 buf  [ MAX_STRING_LENGTH ];
  char                 arg  [ MAX_INPUT_LENGTH ];
  mob_data*        trainer  = 0;
  trainer_data*   pTrainer  = 0;
  int                    i;

  /* So that mobs, NPC's etc cannot attempt to practice things. */

  if( ch->species ) {
    send( ch, "Only players can practice skills.\n\r" );
    return;
  }
  
  /* this determines that there is someone in the room that can
   * train people.
   */  

  const char *skill_name = argument;

  if( contains_word( argument, "from", arg ) ) {
    skill_name = arg;

    char_data *rch;

    if( !( rch = one_character( ch, argument, "practice from", ch->array ) ) ) {
      return;
    }

    if( !( trainer = mob( rch ) )
	|| !( pTrainer = trainer->pTrainer ) ) {
      fsend( ch, "%s cannot teach you any skills.", rch );
      return;
    }

  } else {
    
    for( i = 0; i < *ch->array; ++i ) {
      mob_data *npc = mob( ch->array->list[i] );
      if( npc
	  && npc->pTrainer
	  && npc->Seen( ch ) ) {
	if( trainer ) {
	  fsend( ch, "There is more than one trainer here.  Practice from whom?" );
	  return;
	}
	trainer = npc;
      }
    }

    if( !trainer ) {
      send( ch, "There is no one here who can train you.\n\r" );
      return;
    }

    pTrainer = trainer->pTrainer;
  }

  *buf = '\0';
  char *b = buf;

  /* Display the skills that can be taught to this player. */

  if( !*skill_name ) {
    for( int j = 0; j < MAX_SKILL_CAT; ++j ) {
      const int m = table_max[ skill_table_number[ j ] ];
      for( i = 0; i < m; ++i ) {
	skill_type *entry = skill_entry( j, i );
	if( !is_set( pTrainer->skills[j], i )
	    || entry->prac_cost[ch->pcdata->clss] < 0
	    || !entry->religion( ch->pcdata->religion ) ) 
	  continue;
	
	if( !*buf ) {
	  b += sprintf( b, "Practice Points: %d    Copper Pieces: %d\n\r\n\r",
			ch->pcdata->practice, get_money( ch ) );
	  b += sprintf( b,
			"Skill                 Level       Cost       Prac\n\r" );
	  b += sprintf( b,
			"-----                 -----       ----       ----\n\r" );
	}
	
	const int id = skill_ident( j, i );
	const int skill = ch->get_skill( id );
	if( skill == UNLEARNT )
	  snprintf( arg, MAX_INPUT_LENGTH, "%s", "unk" );
	else
	  snprintf( arg, MAX_INPUT_LENGTH, "%d ", skill );
	
	b += sprintf( b, "%-22s%4s",
		      entry->name, arg );
	
	const int pracs = entry->prac_cost[ ch->pcdata->clss ];
	const int cost = prac_cost( ch, id );
	
	if( skill == 10
	    || skill >= trainer->species->get_skill( id ) ) 
	  b += sprintf( b, "\n\r" );
	else
	  b += sprintf( b, "%12d%10d\n\r",
			cost, pracs );
      }
    }
    if( !*buf ) 
      process_tell( trainer, ch, "Sorry, there is nothing I can teach you" );
    else
      send( ch, buf );
    return;
  }
  
  /* Can you still buy that skill up more? */
  
  for( int j = 0; j < MAX_SKILL_CAT; ++j ) {
    const int m = table_max[ skill_table_number[ j ] ];
    for( i = 0; i < m; ++i ) {
      skill_type *entry = skill_entry( j, i );
      if( !is_set( pTrainer->skills[j], i )
	  || entry->prac_cost[ch->pcdata->clss] < 0
	  || !entry->religion( ch->pcdata->religion ) 
	  || !fmatches( skill_name, entry->name ) )
	continue;
      
      const int id = skill_ident( j, i );
      const int skill = ch->get_skill( id );
      if( skill >= trainer->species->get_skill( id ) ) {
	if( skill < 10 ) {
	  snprintf( buf, MAX_STRING_LENGTH,
		    "You know %s as well as I can teach it.  It can be improved by practice now.",
		    entry->name );
	} else {
	  snprintf( buf, MAX_STRING_LENGTH,
		    "You already know %s as well as possible.",
		    entry->name );
	}
	process_tell( trainer, ch, buf );
	return;
      }

      const int pracs = entry->prac_cost[ch->pcdata->clss];
      const int cost = prac_cost( ch, id );

      /* Can you learn this skill? */
      
      if( !can_study( ch, id ) )
	return;

      /* Do you have the practice points needed? */
      
      if( ch->pcdata->practice < pracs ) {
	fsend( ch, "%s tries to teach %s to you, but fails.", trainer, entry->name );
	process_tell( trainer, ch, 
		      "You need to go out and practice your current skills before you can learn new ones." );
	return;
      }
      
      if( get_money( ch ) < cost ) {
	process_tell( trainer, ch, "You are unable to afford my services." );
	return;
      }

      for( mprog_data *mprog = trainer->species->mprog; mprog; mprog = mprog->next ) {
	if( mprog->trigger == MPROG_TRIGGER_PRACTICE ) {
	  clear_variables( );
	  var_ch = ch;
	  var_mob = trainer;
	  var_room = ch->in_room;
	  var_i = id;
	  var_j = cost;
	  var_k = pracs;
	  const bool result = mprog->execute( );
	  if( !result
	      || !ch->Is_Valid( )
	      || !trainer->Is_Valid( ) )
	    return;
	  break;
	}
      }

      /* Take the money and check for enough money. */
      
      snprintf( buf, MAX_STRING_LENGTH, "You hand %s", trainer->Name( ch ) );
      if( !remove_coins( ch, cost, buf ) ) {
	// Mprog could have removed some coins, I guess...
	process_tell( trainer, ch, "You are unable to afford my services." );
	//      ch->reply = trainer;
	return;
      }

      /* Take the practice points. */

      fsend( ch, "%s teaches you %s%s.",
	     trainer, entry->name,
	     skill == UNLEARNT ? "" : ", improving your ability at it" );
      fsend_seen( ch, "%s practices %s.", ch, entry->name );
      
      ch->pcdata->practice -= pracs;
      ++ch->shdata->skills[j][i];
      
      process_tell( trainer, ch, "Good luck." );
      return;
    }
  }
  
  /* Tried to practice something that the trainer
   * does not know or player spelled it wrong. 
   */
  
  process_tell( trainer, ch, "I can teach you no such skill." );
}
