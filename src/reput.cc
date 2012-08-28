#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include "define.h"
#include "struct.h"


void   recruit_member        ( char_data *ch, char_data *member );
void   add_to_faction        ( char_data *ch, int faction );
void   remove_from_faction   ( char_data *ch, int faction );


bool are_allied( char_data* ch, char_data* victim )
{
  if( ch->shdata->race >= MAX_PLYR_RACE
      || victim->shdata->race >= MAX_PLYR_RACE )
    return true;

  if( ( ch->shdata->race > RACE_LIZARD ) == 
      ( victim->shdata->race > RACE_LIZARD ) )
    return true;

  return false;
}


const char *associate( char_data* ch, char_data* victim, const char *fail_msg )
{
  if( ch->Level() >= LEVEL_IMMORTAL ) {
    return empty_string;
  }

  if( fail_msg == empty_string ) {
    fail_msg = "associate with";
  }

  char_data *whom = victim;

  if( is_set( victim->status, STAT_PET ) ) {
    whom = victim->leader;
  }

  const int dist = ch->Align_Distance( whom ) + ch->Race_Distance( whom );

  switch( dist ) {
  case -1:
  case 0:
    //    if( msg ) {
    //      fsend( ch, "You are happy to %s %s.", msg, victim );
    return "happily";
    //    }
    //    return empty_string;
  case 1:
  case 2:
    return empty_string;
  case 3:
    //    if( msg ) {
    //      fsend( ch, "You only grudgingly %s %s.", msg, victim );
    return "grudgingly";
    //    }
    //    return empty_string;
  case 4:
    if( fail_msg ) {
      fsend( ch, "You would never %s %s.", fail_msg, victim );
    }
    return 0;
  case 5:
  case 6:
  case 7:
  default:
    if( fail_msg ) {
      fsend( ch, "You would sooner kill %s than %s %s.",
	     victim, fail_msg, victim->Him_Her() );
    }
    if( dist > 7 ) {
      bug( "Associate: distance > 7 between %s - %s - %s", ch, fail_msg, victim );
    }
    return 0;
  }

  /*
  if( are_allied( ch, victim ) )
    return true;

  const char* race = race_table[ victim->shdata->race ].name;

  send( ch, "You would never associate with a%s %s.\n\r",
     isvowel( *race ) ? "n" : "", race );
      
  return false;
  */
}


/*
 *   MODIFY ROUTINE
 */


void modify_reputation( char_data*, char_data*, int )
{
  /*
  char             tmp  [ MAX_STRING_LENGTH ];
  pfile_data*    pfile;
  int         vic_clan  = victim->shdata->clan;
  int          ch_clan  = ch->shdata->clan;
  int              mod;
  int                i;
  
  if( is_set( &ch->in_room->room_flags, RFLAG_ARENA ) ) 
    return;
 
  if( ch->species != NULL || vic_clan < 2 || victim == ch )
    return;

  pfile = ch->pcdata->pfile;

  if( type == REP_STOLE_FROM ) {
    pfile->reputation[vic_clan-2] += victim->species ? -100 : -200;
    send( ch, "Your reputation changes for stealing from %s.\n\r",
      victim->Seen_Name( ch ) ); 
    }

  if( type == REP_ATTACKED ) {
    pfile->reputation[vic_clan-2] += victim->species ? -100 : -200; 
    send( ch, "Your reputation changes for attacking %s.\n\r",
      victim->Seen_Name( ch ) ); 
    }
  
  if( type == REP_KILLED ) {
    for( i = 2; i < table_max[ TABLE_CLAN ]; i++ ) {
      mod = ( victim->species == NULL ? -300 : -500 );
      if( i != vic_clan )
        mod /= 3;
      mod = mod*(clan_table[vic_clan].relation[i-2]-50)/100;
      pfile->reputation[i-2] += mod;
      }
    send( ch, "Your reputation changes for killing %s.\n\r",
      victim->Seen_Name( ch ) ); 
    }

  if( ch_clan >= 2 && ch_clan < table_max[ TABLE_CLAN ]
    && pfile->reputation[ch_clan-2] < 0 ) {
    send( ch, "%sYou are cast out of your clan!!%s\n\r",
      bold_v( ch ), normal( ch ) );  
    sprintf( tmp, "%s is cast out of the %s clan for fighting %s.",
      ch->descr->name, clan_table[ch_clan].name, victim->descr->name );
    info( 0, empty_string, 0, tmp, IFLAG_CLANS, 1, ch );
    ch->shdata->clan = CLAN_NONE;
    }
    */
}


/*
 *   REPUTATION COMMAND
 */


int reputation( char_data *ch, int nation )
{
  const unsigned scale = 3;

  player_data *pc = player( ch );
  
  int race_reputation = 0;
  if( ch->shdata->race < MAX_PLYR_RACE ) {
    race_reputation = scale*2*range( 0, nation_table[nation].race[ch->shdata->race], 100 );

    if( race_reputation != 0 ) {
      race_reputation -= 101*scale;
    }

    if( pc ) {
      int grievances = pc->reputation.nation[nation];
      if( grievances > 0 ) {
	const unsigned limit = 100;
	const unsigned factor = 9;
	int grievance_reputation = scale*limit*grievances/(grievances + factor);
	race_reputation -= 2*grievance_reputation;
      }
    }
  }

  int alignment_reputation = 2*scale*range( 0, nation_table[nation].alignment[ch->shdata->alignment], 100 );

  if( alignment_reputation != 0 ) {
    alignment_reputation -= 101*scale;
  }

  if( pc ) {
    //    int j = 0;
    int total = 0;
    int count = 0;
    for( int i = 0; i < table_max[ TABLE_ALIGNMENT ]; i++ ) {
      int x = range( 0, nation_table[nation].alignment[i], 100 );
      int y = range( 0, (unsigned)pc->reputation.alignment[i], 100 );
      if( x > 0 && y > 0 ) {
	total += x*y;
	count += y;
	//	++j;
      }
    }
    if( count > 0 ) {
      alignment_reputation -= 2*scale*total/count - 101*scale;
    }
  }

  return race_reputation + alignment_reputation;
}


const index_data reputation_index [] =
{ 
  { "abhorred",  "",   -600 },
  { "despised",  "",   -400 },
  { "hated",     "",   -200 },
  { "wanted",    "",    -50 },
  { "suspect",   "",      0 },
  { "dubious",   "",     50 },
  { "accepted",  "",    200 },
  { "credible",  "",    400 },
  { "reputable", "",    600 },
  { "esteemed",  "",     -1 },
};


void do_reputation( char_data* ch, const char *argument )
{
  player_data*     pc  = (player_data*) ch;
  char_data*   victim;

  if( not_player( ch ) )
    return;

  if( ch->Level() >= LEVEL_BUILDER && *argument ) {
    if( !( victim = one_character( ch, argument, "show reputation",
				   ch->array, (thing_array*) &player_list ) ) )
      return;
    if( !( pc = player( victim ) ) ) {
      send( ch, "Reputation cannot act on npcs.\n\r" );
      return;
    }
  }

  page_underlined( ch, "Sacrifices\n\r" );
  page( ch, "%17s: %d\n\r", "Value", pc->reputation.gold );
  page( ch, "%17s: %d\n\r", "Blood", pc->reputation.blood );
  page( ch, "%17s: %d\n\r", "Magic", pc->reputation.magic );
  page( ch, "\n\r" );
  
  page_underlined( ch, "Nation Grievances\n\r" );
  for( int i = 1; i < table_max[ TABLE_NATION ]; i++ ) 
    if( ch->Level() >= LEVEL_BUILDER ) {
      page( ch, "%17s: %-7d %-10s  (* %8d *)\n\r",
	    nation_table[i].name,
	    pc->reputation.nation[i],
	    lookup( reputation_index, reputation( pc, i ) ),
	   reputation( pc, i ) );
    } else {
      page( ch, "%17s: %-7d %s\n\r", nation_table[i].name,
	    pc->reputation.nation[i],
	    lookup( reputation_index, reputation( pc, i ) ) );
    }
  double total = 0.0;

  for( int i = 0; i < table_max[ TABLE_ALIGNMENT ]; i++ ) 
   total += (double)(unsigned)pc->reputation.alignment[i];

  if( total <= 0.0 )
    return;

  page( ch, "\n\r" );
  page_underlined( ch, "Alignment Exp (%)\n\r" );

  for( int i = 0; i < table_max[ TABLE_ALIGNMENT ]; i++ ) {
    if( ch->Level() >= LEVEL_BUILDER ) {
      page( ch, "%17s: %6.2f  (* %10u *)\n\r",
	    alignment_table[i].name,
	    (double)(unsigned)pc->reputation.alignment[i]*100.0/total,
	    (unsigned)pc->reputation.alignment[i] );
    } else {
      page( ch, "%17s: %6.2f\n\r", alignment_table[i].name,
	    (double)(unsigned)pc->reputation.alignment[i]*100.0/total );
    }
  }
}


/*
 *   WANTED COMMAND
 */


void do_wanted( char_data*, const char * )
{
  /*
  char   buf  [ MAX_STRING_LENGTH ];
  int   clan  = ch->shdata->clan;
  int  count;
  int      i; 
 
  if( is_confused_pet( ch ) )
    return;

  send( "Command is disabled.\n\r", ch );
  return;

  if( argument[0] != '\0' ) {
    for( clan = CLAN_IMMORTAL+1; clan < table_max[ TABLE_CLAN ]; clan++ ) 
      if( !strncasecmp( argument, clan_table[clan].name,
        strlen( argument ) ) )
        break;

    if( clan == table_max[ TABLE_CLAN ] ) {
      send( ch, "That clan is unknown.\n\r" );
      return;
      }
    }

  if( clan >= table_max[ TABLE_CLAN ] ) {
    send( ch, "Your clan does not have a wanted list.\n\r" );
    return;
    }

  count = 0;
  page_title( ch, "WANTED" );
  sprintf( buf, "%24sName%7sBounty  Kills\n\r", " ", " " );
  page( ch, buf );

  for( i = 0; i < max_pfile; i++ ) {
    if( pfile_list[i] != NULL && pfile_list[i]->reputation[clan] < 0 ) {
      sprintf( buf, "%24s%-10s  %-7d  %-5d\r\n", "", pfile_list[i]->name,
        -pfile_list[i]->reputation[clan], 0 );
      page( ch, buf );
      count++;
      }
    }

  if( count == 0 )
    page(
      "                        Wanted list is currently empty.\n\r", ch );
      */
}

/*
 *   RELATIONS COMMAND
 */


static char *relation_string( int rel )
{
  if( rel > 90 )  return "+++";		// 10%
  else if( rel > 75 )  return "+ +";	// 15%
  else if( rel > 55 )  return " + ";	// 20%
  else if( rel > 45 )  return " = ";	// 10%
  else if( rel > 35 )  return " - ";	// 10%
  else if( rel > 15 )  return "- -";	// 20%
  else if( rel > 0 )  return "---";	// 16%
  return "   ";
}


void do_relations( char_data* ch, const char * )
{
  char   tmp  [ MAX_STRING_LENGTH ];

  page_title( ch, "Nation vs Race" );

  page( ch, "\n\r%19s", "" );
  for( int i = 0; i < MAX_PLYR_RACE; i++ )
    sprintf( tmp+i*4, "%s ", race_table[i].abbrev );
  strcat( tmp, "\n\r" );
  page( ch, tmp );

  bool valid_race = ch->shdata->race < MAX_PLYR_RACE;

  for( int i = 1; i < table_max[ TABLE_NATION ]; i++ ) {
    page( ch, "%8s(%3d) %s  ",
	  "",
	  valid_race ? nation_table[i].race[ch->shdata->race] : 0,
	  nation_table[i].abbrev );
    for( int j = 0; j < MAX_PLYR_RACE; j++ ) 
      page( ch, "%3s%s", relation_string( nation_table[i].race[j] ),
        j != MAX_PLYR_RACE-1 ? " " : "\n\r" );
    }

  page( ch, "\n\r" );
  page_title( ch, "Nation vs Alignment" );

  page( ch, "\n\r%27s", "" );
  for( int i = 0; i < table_max[ TABLE_ALIGNMENT ]; i++ )
    sprintf( tmp+i*4, "%s  ", alignment_table[i].abbrev );
  strcat( tmp, "\n\r" );
  page( ch, tmp );

  for( int i = 1; i < table_max[ TABLE_NATION ]; i++ ) {
    page( ch, "%16s(%3d) %s  ",
	  "",
	  nation_table[i].alignment[ch->shdata->alignment],
	  nation_table[i].abbrev );
    for( int j = 0; j < table_max[ TABLE_ALIGNMENT ]; j++ ) 
      page( ch, "%3s%s", relation_string( nation_table[i].alignment[j] ),
        j != table_max[ TABLE_ALIGNMENT ] - 1 ? " " : "\n\r" );
  }
  
  page( ch, "\n\r" );
  page_centered( ch, "[ See help relations for explanation of table ]" );
}
