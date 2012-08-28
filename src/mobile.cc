#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "define.h"
#include "struct.h"


int species_max = 0;

int Species_Data :: modified = -1;


Species_Data::Species_Data( int num )
  : mprog(0), reset(0),
    creator(empty_string), last_mod(empty_string), date(-1),
    vnum(num), serial(-1),
    nation(NATION_NONE), wear_part(0), movement(-1), wander(4000),
    comments(empty_string), used(false)
{
  record_new( sizeof( species_data ), MEM_SPECIES );
  
  descr = new descr_data;
  shdata = new share_data;
  attack = new Attack_Data( this );
  
  vzero( part_name, MAX_ARMOR );
  zero_exp( this );

  species_list[vnum] = this;

  species_max = max( species_max, vnum );
}


Species_Data::~Species_Data( )
{
  record_delete( sizeof( species_data ), MEM_SPECIES );
  
  delete descr;
  delete shdata;
  delete attack;
  
  extra_descr.delete_list( );
  delete_list( mprog );
  delete_list( reset );

  free_string( creator, MEM_SPECIES );
  free_string( comments, MEM_SPECIES );
  free_string( last_mod, MEM_SPECIES );
  
  for( int i = 0; i < MAX_ARMOR; ++i ) {
    free_string( part_name[i], MEM_SPECIES );
  }

  species_list[vnum] = 0;

  if( vnum == species_max ) {
    for( --species_max; species_max > 0 && !species_list[species_max]; --species_max );
  }
}


bool Species_Data :: is_affected( int bit ) const
{
  return is_set( affected_by, bit );
}


void Species_Data :: set_modified( char_data *ch )
{
  if( ch ) {
    free_string( last_mod, MEM_SPECIES );
    last_mod = alloc_string( ch->descr->name, MEM_SPECIES );
  }

  modified = date = current_time;
}


bool Species_Data :: dies_at_zero( ) const
{
  return shdata->race == RACE_UNDEAD
    || shdata->race == RACE_GOLEM;
}


bool can_extract( species_data* species, char_data* ch )
{
  for( int i = 0; i < mob_list; i++ ) {
    mob_data *npc = mob_list[i];
    if( npc->species == species ) {
      send( ch, "There are still some creatures of that species alive.\n\r" );
      return false;
    }
  }
  
  if( has_reset( species ) ) { 
    send( ch, "You must first remove all resets of that mob.\n\r" );
    return false;
  }

 
  return true;
}


/*
 *   LOAD SPECIES
 */


void load_mobiles( void )
{
  echo( "Loading Mobs ...\n\r" );
  vzero( species_list, MAX_SPECIES );
 
  FILE *fp = open_file( AREA_DIR, MOB_FILE, "r", true );

  if( strcmp( fread_word( fp ), "#MOBILES" ) ) 
    panic( "Load_mobiles: missing header" );

  species_data*  species;

  while( true ) {
    int vnum;
    char letter;

    if( ( letter = fread_letter( fp ) ) != '#' ) 
      panic( "Load_mobiles: # not found." );

    if( ( vnum = fread_number( fp ) ) == 0 )
       break;

    if( vnum < 0 || vnum >= MAX_SPECIES ) 
      panic( "Load_mobiles: vnum out of range." );

    if( ( species = get_species( vnum ) ) ) {
      bug( "Load_mobiles: vnum %d duplicated.", vnum );
      bug( fread_string( fp, MEM_UNKNOWN ) );
      bug( species->Name( ) );
      shutdown( "load_mobiles duplicate vnum ", species->Name( ) );
    }

    species = new species_data(vnum);

    share_data *shdata = species->shdata;
    descr_data *descr = species->descr;

    descr->name          = fread_string( fp, MEM_DESCR );
    descr->keywords      = fread_string( fp, MEM_DESCR );
    descr->singular      = fread_string( fp, MEM_DESCR );
    descr->prefix_s      = fread_string( fp, MEM_DESCR );
    descr->adj_s         = fread_string( fp, MEM_DESCR );
    descr->long_s        = fread_string( fp, MEM_DESCR );
    descr->plural        = fread_string( fp, MEM_DESCR );
    descr->prefix_p      = fread_string( fp, MEM_DESCR );
    descr->adj_p         = fread_string( fp, MEM_DESCR );
    descr->long_p        = fread_string( fp, MEM_DESCR );
    descr->complete      = fread_string( fp, MEM_DESCR );

    species->creator       = fread_string( fp, MEM_SPECIES ); 
    species->comments      = fread_string( fp, MEM_SPECIES ); 
    species->last_mod      = fread_string( fp, MEM_SPECIES ); 

    species->attack->read( fp );

    species->nation      = fread_number( fp );
    species->group       = fread_number( fp );
    shdata->race         = fread_number( fp );
    species->adult       = fread_number( fp );
    species->maturity    = fread_number( fp );
    species->skeleton    = fread_number( fp );
    species->zombie      = fread_number( fp );
    species->corpse      = fread_number( fp );
    species->price       = fread_number( fp );
    shdata->kills        = fread_number( fp );
    shdata->deaths       = fread_number( fp );
    species->wander      = fread_number( fp );
    species->date        = fread_number( fp );
    species->light       = fread_number( fp );
    species->color       = fread_number( fp );
    species->movement    = fread_number( fp );

    species->act_flags[0] = fread_number( fp );
    species->act_flags[1] = fread_number( fp );

    species->affected_by[0] = fread_number( fp );
    species->affected_by[1] = fread_number( fp );
    species->affected_by[2] = fread_number( fp );

    shdata->alignment     = fread_number( fp );
    shdata->level         = fread_number( fp );
    shdata->strength      = fread_number( fp );
    shdata->intelligence  = fread_number( fp );
    shdata->wisdom        = fread_number( fp );
    shdata->dexterity     = fread_number( fp );
    shdata->constitution  = fread_number( fp );

    for( int i = 0; i < MAX_RESIST; ++i ) {
      shdata->resist[i] = fread_number( fp ); 
    }

    while( true ) {
      int level = fread_number( fp );

      if( level < 0 )
        break;

      const char *name = fread_string( fp, MEM_UNKNOWN );

      if( level > 10 ) {
	bug( "Load_Mobiles: Mob #%d has skill '%s' at level %d.",
	     species->vnum, name, level );
	level = 10;
      }

      const int number = skill_index( name );
      if( number != -1 ) {
        shdata->skills[skill_table(number)][skill_number(number)] = (unsigned char)level;
      } else {
	bug( "Load_Mobiles: Mob #%d has unknown skill '%s'.",
	     species->vnum, name );
      }
      free_string( name, MEM_UNKNOWN );
    }

    for( int i = 0; i < MAX_ARMOR; ++i ) {
      species->chance[i] = fread_number( fp );
      species->armor[i] = fread_number( fp );
      species->part_name[i] = fread_string( fp, MEM_SPECIES );
    }

    species->wear_part = fread_number( fp );
    species->hitdice   = fread_number( fp );
    species->movedice  = fread_number( fp );

    species->damage       = fread_number( fp );
    species->rounds       = fread_number( fp );
    species->special      = fread_number( fp );
    species->damage_taken = fread_number( fp );
    species->exp          = fread_number( fp );

    switch( fread_letter( fp ) ) {
    case 'M' : species->sex = SEX_MALE;           break;
    case 'F' : species->sex = SEX_FEMALE;         break;
    case 'H' : species->sex = SEX_HERMAPHRODITE;  break;
    case 'R' : species->sex = SEX_RANDOM;         break;
    default  : species->sex = SEX_NEUTRAL;        break;
    }

    species->gold   = fread_number( fp );
    species->size   = fread_number( fp );
    species->weight = fread_number( fp );

    load( fp, species->reset );
    read_extras( fp, species->extra_descr );

    while( true ) {
      const int number = fread_number( fp );

      if( number == -1 )
        break;

      mprog_data *mprog = new mprog_data( species );
      mprog->next    = 0;
      mprog->trigger = number;
      mprog->value   = fread_number( fp );
      mprog->string  = fread_string( fp, MEM_MPROG );

      mprog->read( fp );
    }
  }

  fclose( fp );

  for( int i = 1; i <= species_max; ++i ) {
    if( !( species = species_list[i] ) )
      continue;
    if( species->group < 0
	|| species->group >= table_max[ TABLE_GROUP ] ) {
      roach( "Fix_Species: Non-existent group." );
      roach( "-- Species = %s", species->Name( ) );
      species->group = 0;
    } 
    if( species->nation < 0
	|| species->nation >= table_max[ TABLE_NATION ] ) {
      roach( "Fix_Species: Non-existent nation." );
      roach( "-- Species = %s", species->Name( ) );
      species->nation = 0;
    }
    if( species->shdata->race < 0
	|| species->shdata->race >= table_max[ TABLE_RACE ] ) {
      roach( "Fix_Species: Non-existent race." );
      roach( "-- Species = %s", species->Name( ) );
      species->shdata->race = 0;
    }
  }  
}


static const char *score_name( species_data *species, double score )
{
  const int width = 69;

  if( !species )
    return "---";

  const int n = int( ( score * 1000.0 ) + 0.5 );

  char buf[8];
  const int l = snprintf( buf, 8, "(%s)", int5( n, true ) );

  const int field = width-l-1;

  char *tmp = static_string();
  snprintf( tmp, THREE_LINES, "%s %s",
	    trunc( species->Name( true, false, true ), field ).c_str(), buf );
  return tmp;
}


void high_mob( char_data *ch, const char *argument )
{
  int start = 1;
  if( *argument && !number_arg( argument, start ) ) {
    send( ch, "Syntax: high mob [#]\n\r" );
    return;
  }

  if( start < 1 ) {
    send( ch, "Error - number is out of range.\n\r" );
    return;
  }

  start = min( start-1, species_max - 19 );

  double scores[ start+20 ];
  species_data *high[ start+20 ];

  vzero( scores, start+20 );
  vzero( high, start+20 );

  int top = 0;

  for( int i = 1; i <= species_max; ++i ) {
    species_data *species = species_list[i];
    if( !species
    	|| species->shdata->kills == 0
	|| species->shdata->deaths < 10 )
      continue;
    double score = 0.0;
    //    if( species->shdata->deaths == 0 ) {
    //      score = HUGE_VAL;
    //    } else {
    score = (double) species->shdata->kills / species->shdata->deaths;
      //      score *= (double) species->damage / species->damage_taken;
      //    }
    
    for( int j = 0; j < start+20; ++j ) {
      if( !high[j]
	  || score > scores[j]
	  || ( score == scores[j] && species->shdata->kills > high[j]->shdata->kills )
	  || ( score == scores[j] && species->shdata->deaths < high[j]->shdata->deaths )
	  || ( score == scores[j]
	       && species->shdata->kills == high[j]->shdata->kills
	       && species->shdata->deaths == high[j]->shdata->deaths
	       && strcasecmp( species->Name( ), high[j]->Name( ) ) <= 0 ) ) {
	if( j < start+19 ) {
	  memmove( scores+j+1, scores+j, (start+19-j)*sizeof( double ) );
	  memmove( high+j+1, high+j, (start+19-j)*sizeof( species_data* ) );
	}
	scores[j] = score;
	high[j] = species;
	++top;
	break;
      }
    }
  }

  start = min( start, top-20 );
  start = max( 0, start );

  send( ch, "                  _             _   _          _  ___ \n\r" );
  send( ch, "           |_| | | _ |_|   |V| | | |_)   |  | (_   |  \n\r" );
  send( ch, "           | | | |_| | |   | | |_| |_)   |_ |  _)  |  \n\r" );
  send( ch, "\n\r\n\r" );

  char tmp [ TWO_LINES ];
  char r1 [ 8 ];

  for( int i = 0; i < 20; ++i ) {
    snprintf( r1, 8, "[%d]", start+i+1 );
    if( !is_builder( ch ) ) {
      snprintf( tmp, TWO_LINES, "%8s  %s\n\r",
		r1, score_name( high[start+i], scores[start+i] ) );
    } else {
      snprintf( tmp, TWO_LINES, "%8s  %s - #%d, %s\n\r",
		r1, score_name( high[start+i], scores[start+i] ),
		high[start+i]->vnum, high[start+i]->creator );
    }
    send( ch, tmp );
  }
}


void execute_mob_timer( event_data *event )
{
  mob_data *mob = (mob_data*) event->owner;
  species_data *species = mob->species;

  extract( event );

  for( mprog_data *mprog = species->mprog; mprog; mprog = mprog->next ) {
    if( mprog->trigger == MPROG_TRIGGER_TIMER ) {
      mob->Select( 1 );
      clear_variables( );
      var_mob = mob;
      var_room = mob->in_room;
      if( !mprog->execute( mob )
	  || !mob->Is_Valid( ) ) {
	return;
      }
      stop_events( mob, execute_mob_timer );
      if( mprog->value > 0 ) {
	add_queue( new event_data( execute_mob_timer, mob ), mprog->value*PULSE_MOBILE );
      }
      return;
    }
  }

  bug( "Mob_timer event: no timer mprog." );
  bug( "-- Mob = %s (%d).", mob, species->vnum );
}
