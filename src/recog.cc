#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include "define.h"
#include "struct.h"


#define RECOG_INTRODUCE       16
#define RECOG_RECOGNIZE       17
#define RECOG_BEFRIENDER      18
#define RECOG_FILTERING       19
#define RECOG_CONSENTING      20
#define RECOG_FILTERED        21
#define RECOG_BEFRIENDED      22


bool in_character = true;


/*
 *   CLASS KNOWN_DATA
 */


Recognize_Data :: Recognize_Data( int i )
  : size(i)
{
  record_new( sizeof( Recognize_Data ), MEM_RECOGNIZE );
  record_new( i*sizeof( int ), -MEM_RECOGNIZE );

  list = new int[i];
}


Recognize_Data :: ~Recognize_Data( )
{
  record_delete( sizeof( Recognize_Data ), MEM_RECOGNIZE );
  record_delete( size*sizeof( int ), -MEM_RECOGNIZE );

  delete [] list;
}


/*
 *   LOCAL FUNCTIONS
 */


bool  display               ( char_data*, int, const char* );
void  insert                ( Recognize_Data*&, int, int );


/*
 *   SEARCH ROUTINES
 */


/*
static void sort( Recognize_Data* recognize )
{
  int*   list;
  int    size;
  int       i;
  bool  found;

  if( !recognize )
    return;

  list = recognize->list;
  size = recognize->size;

  found = true;
  while( found ) {
    found = false;
    for( i = 0; i < size-1; i++ ) {
      if( ( list[i] & 0xffff ) > ( list[i+1] & 0xffff ) ) {
        swap( list[i], list[i+1] );
        found = true;
      }
    }
  }
}
*/


void insert( Recognize_Data*& recognize, int value, int pos )
{
  if( !recognize ) {
    recognize = new Recognize_Data( 1 );
    recognize->list[0] = value;
    return;
  }
  
  record_new( sizeof( int ), -MEM_RECOGNIZE );

  insert( recognize->list, recognize->size, value, pos );
}


void remove( Recognize_Data*& recognize, int pos )
{
  if( pos < 0 || pos > recognize->size )
    return;

  if( recognize->size == 1 ) {
    delete recognize;   
    recognize = 0;
    return;
  }
  
  record_delete( sizeof( int ), -MEM_RECOGNIZE );

  remove( recognize->list, recognize->size, pos );
}  


int search( const Recognize_Data* recognize, int ident )
{
  int element;

  if( !recognize )
    return -1;

  int min = 0;
  int max = recognize->size-1;

  while( true ) {
    if( max < min )
      return -min-1;

    element = (max+min)/2;
    const int value = ( recognize->list[element] & 0xffff );

    if( ident == value )
      break;
    
    if( ident > value )
      min = element+1;
    else 
      max = element-1;
  }
  
  return element;
}

  
/*
 *   ALTERING KNOWLEDGE ROUTINES
 */


static bool is_recognize_set( const Recognize_Data *recognize, int ident, int bit )
{
  const int pos = search( recognize, ident );

  return( pos >= 0 && is_set( recognize->list[pos], bit ) );
}


static bool remove_recognize_bit( Recognize_Data*& recognize, int ident, int bit )
{
  const int pos = search( recognize, ident );
 
  if( pos >= 0 && is_set( recognize->list[pos], bit ) ) {
    remove_bit( recognize->list[pos], bit );
    if( ( recognize->list[pos] & 0xffff0000 ) == 0 )
      remove( recognize, pos );
    return true;
  }
  
  return false;
} 


static void set_recognize_bit( char_data* ch, int ident, int bit )
{
  Recognize_Data *recognize = ch->pcdata->recognize;
    
  const int pos = search( recognize, ident );

  if( pos >= 0 ) {
    set_bit( recognize->list[pos], bit );
    return;
  }
  
  set_bit( ident, bit );
  insert( ch->pcdata->recognize, ident, -pos-1 );
} 


/*
 *   LOGIN ROUTINE
 */


/*
static void reconcile( char_data *doer, int doer_flags, int do_flag,
		       char_data *done_to, int done_flags, int done_flag )
{
  const bool did = is_set( doer_flags, do_flag );
  const bool done = is_set( done_flags, done_flag );

  if( did != done ) {
    if( did ) {
      set_recognize_bit( done_to, doer->pcdata->pfile->ident, done_flag );
    } else {
      remove_recognize_bit( done_to->pcdata->recognize, doer->pcdata->pfile->ident, done_flag );
    }
  }
}
*/


void reconcile_recognize( char_data* ch )
{
  //  Recognize_Data*& recog_ch = ch->pcdata->recognize;

  /* Done by save.cc:read_char().
  if( recog_ch ) {
    for( int i = recog_ch->size-1; i >= 0; i-- ) 
      if( !get_pfile( recog_ch->list[i] & 0xffff )
	  || ( recog_ch->list[i] >> 16 ) == 0 ) 
	remove( recog_ch, i );
  }
  */

  const int first[] = {
    RECOG_INTRODUCE,
    RECOG_FILTERING,
    RECOG_BEFRIENDER
  };

  const int second[] = {
    RECOG_RECOGNIZE,
    RECOG_FILTERED,
    RECOG_BEFRIENDED
  };

  for( int i = 0; i < player_list; ++i ) {
    player_data *victim = player_list[i];

    if( !victim->Is_Valid( ) || victim == ch )
      continue;

    const int pos_ch = search( ch->pcdata->recognize, victim->pcdata->pfile->ident );
    int flag_ch = ( pos_ch >= 0
		    ? ch->pcdata->recognize->list[ pos_ch ]
		    : victim->pcdata->pfile->ident );

    const int pos_vict = search( victim->pcdata->recognize, ch->pcdata->pfile->ident );
    const int flag_vict = ( pos_vict >= 0
			    ? victim->pcdata->recognize->list[ pos_vict ]
			    : 0 );

    // Victim is already in the game, so assume his recog data is correct.
    for( int j = 0; j < 3; ++j ) {
      assign_bit( flag_ch, first[j], is_set( flag_vict, second[j] ) );
      assign_bit( flag_ch, second[j], is_set( flag_vict, first[j] ) );
    }

    if( pos_ch >= 0 ) {
      if( flag_ch == victim->pcdata->pfile->ident ) {
	remove( ch->pcdata->recognize, pos_ch );
      } else {
	ch->pcdata->recognize->list[ pos_ch ] = flag_ch;
      }
    } else if( flag_ch != victim->pcdata->pfile->ident ) {
      insert( ch->pcdata->recognize, flag_ch, -pos_ch-1 );
    }

    /*
    reconcile( ch, flag_ch, RECOG_INTRODUCE, victim, flag_vict, RECOG_RECOGNIZE );
    reconcile( ch, flag_ch, RECOG_FILTERING, victim, flag_vict, RECOG_FILTERED );
    reconcile( ch, flag_ch, RECOG_BEFRIENDER, victim, flag_vict, RECOG_BEFRIENDED );

    reconcile( victim, flag_vict, RECOG_INTRODUCE, ch, flag_ch, RECOG_RECOGNIZE );
    reconcile( victim, flag_vict, RECOG_FILTERING, ch, flag_ch, RECOG_FILTERED );
    reconcile( victim, flag_vict, RECOG_BEFRIENDER, ch, flag_ch, RECOG_BEFRIENDED );
    */

    /*
    if( ( is_set( flag_ch, RECOG_RECOGNIZE )
	  != is_set( flag_vict, RECOG_INTRODUCE ) )
	|| ( is_set( flag_vict, RECOG_RECOGNIZE )
	     != is_set( flag_ch, RECOG_INTRODUCE ) )
	|| ( is_set( flag_ch, RECOG_FILTERING )
	     != is_set( flag_vict, RECOG_FILTERED ) )
	|| ( is_set( flag_vict, RECOG_FILTERING )
	     != is_set( flag_ch, RECOG_FILTERED ) )
	|| ( is_set( flag_ch, RECOG_BEFRIENDER )
	     != is_set( flag_vict, RECOG_BEFRIENDED ) )
	|| ( is_set( flag_vict, RECOG_BEFRIENDER )
	     != is_set( flag_ch, RECOG_BEFRIENDED ) )
	) {
      remove( recog_ch, pos_ch );
      remove( recog_vict, pos_vict );
    }
    */
  }

  //  ch->pcdata->recognize = recog_ch;
}


/*
 *   DISPLAY LIST WITH BIT SET
 */


static int sort( const int *list,
		 int *sorted,
		 int max )
{
  int j = 0;
  for( int i = 0; i < max; ++i ) {
    const int data = list[i];
    if( get_pfile( data & 0xffff ) )
      sorted[j++] = i;
  }

  for( int n = 0; n < j; ++n ) {
    bool done = true;
    for( int k = 0; k < j-1-n; ++k ) {
      const char *s1 = get_pfile( list[sorted[k]] & 0xffff )->name;
      const char *s2 = get_pfile( list[sorted[k+1]] & 0xffff )->name;
      if( strcasecmp( s1, s2 ) > 0 ) {
	swap( sorted[k], sorted[k+1] );
	done = false;
      }
    }
    if( done )
      break;
  }

  return j;
}


bool display( char_data* ch, int bit, const char* title )
{
  const Recognize_Data *recognize  = ch->pcdata->recognize;

  if( !recognize )
    return false;

  pfile_data*          pfile;
  bool                 found  = false;
  const unsigned columns = ch->pcdata->columns / 17;

  int sorted [ recognize->size ];
  int max = sort( recognize->list, sorted, recognize->size );

  int j = 0;

  for( int i = 0; i < max; ++i ) {
    const int data = recognize->list[ sorted[ i ] ];
    if( !( pfile = get_pfile( data & 0xffff ) )
	//	|| pfile->level >= LEVEL_APPRENTICE
	|| !is_set( data, bit ) )
      continue;
    if( !found ) { 
      page( ch, "%s:\n\r", title );
      found = true;
    }
    page( ch, "%17s%s", pfile->name, j%columns == columns-1 ? "\n\r" : "" );
    ++j;
  }  
  
  if( j%columns != 0 )
    page( ch, "\n\r" );
  
  return found;
}


static pfile_data *remove_name( Recognize_Data*& recognize, const char *name, int bit )
{
  pfile_data *pfile = find_pfile( name );
  
  if( pfile
      && remove_recognize_bit( recognize, pfile->ident, bit ) )
    return pfile;

  return 0;
}


void do_recognize( char_data *ch, const char *argument )
{
  if( is_mob( ch ) )
    return;

  if( !*argument ) {
    send( ch, "Syntax: recognize <player|mob>\n\r" );
    return;
  }

  player_data *pc;
  bool loaded = false;
      
  char arg [ MAX_INPUT_LENGTH ];
      
  argument = one_argument( argument, arg );
      
  if( pfile_data *pfile = find_pfile( arg ) ) {
      
    in_character = false;

    if( pfile != ch->pcdata->pfile
	&& pfile->trust >= get_trust( ch ) ) {
      fsend( ch, "You cannot view the recognize data of %s.", pfile->name );
      return;
    }
      
    if( !( pc = find_player( pfile ) ) ) {
      link_data link;
      link.connected = CON_PLAYING;
      if( !load_char( &link, pfile->name, PLAYER_DIR ) ) {
	bug( "Load_players: error reading player file. (%s)", pfile->name );
	return;
      }
      pc = link.player;
      loaded = true;
    }

    const Recognize_Data *recog = pc->pcdata->recognize;

    if( !recog ) {
      fsend( ch, "%s has no recognize data.\n\r", pc );
    } else {
    
      page_title( ch, "Recognize data for %s", pc );
      page_underlined( ch, "  ID# Serial# Name           Control Status\n\r" );
    
      const int control[4] = {
	RECOG_INTRODUCE,
	RECOG_BEFRIENDER,
	RECOG_FILTERING,
	RECOG_CONSENTING
      };
      const char *const cntls = "IBFC";
    
      const int status[4] = {
	RECOG_RECOGNIZE,
	RECOG_BEFRIENDED,
	RECOG_FILTERED
      };
      const char *const stats = "IBF";
    
      char cntl[4];
      char stat[3];
    
      for( int i = 0; i < recog->size; ++i ) {
	const int data = recog->list[i];
	char *s = cntl;
	for( int j = 0; j < 4; ++j ) {
	  if( is_set( data, control[j] ) )
	    *s++ = cntls[j];
	}
	*s = '\0';
	s = stat;
	for( int j = 0; j < 3; ++j ) {
	  if( is_set( data, status[j] ) )
	    *s++ = stats[j];
	}
	*s = '\0';
	const int id = data & 0xffff;
	pfile_data *pf = get_pfile( id );
	if( !pf ) {
	  page( ch, "%5d   ????? Bad ID#\n\r", id );
	} else {
	  page( ch, "%5d %7d %-14s %-4s    %s\n\r",
		id, pf->serial, pf->name, cntl, stat );
	}
      }
    
      page( ch, "\n\r" );
      page_underlined( ch, "Key:\n\r" );
      page( ch, "  I - Introduce\n\r" );
      page( ch, "  B - Befriend\n\r" );
      page( ch, "  F - Filter\n\r" );
      page( ch, "  C - Consent (control bit only)\n\r" );
    }

    if( loaded ) {
      page( ch, "\n\r" );
      page_centered( ch, "[ Player file was loaded from disk. ]" );
      pc->Extract();
      extracted.delete_list();
    }

    return;
  }

  mob_data *npc = one_mob( ch, arg, "recognize", (thing_array*) ch->array );
  
  if( !npc )
    return;
  
  if( npc->known_by.is_empty( ) ) {
    fsend( ch, "%s is not recognized by any players.", npc );
    return;
  }
  
  page( ch, "%s is recognized by:\n\r", npc );
  
  const unsigned columns = ch->pcdata->columns / 19;
  
  int i = 0;
  
  for( ; i < npc->known_by; ++i ) {
    page( ch, "%19s%s",
	  npc->known_by[i]->Name( ch ),
	  i%columns == columns-1 ? "\n\r" : "" );
  }
  
  if( i%columns != 0 )
    page( ch, "\n\r" );
}


/*
 *   INTRODUCTION FUNCTIONS
 */


bool char_data :: Recognizes( const char_data *victim ) const
{
  if( !pcdata )
    return false;

  // Switched mobs recognize "owner" player.
  if( link && link->player != this && link->player == victim )
    return true;

  if( victim->species ) {
    if( victim->descr == victim->species->descr
	|| victim->leader != this
	|| !*victim->descr->singular )
      return false;
    return true;
  }

  if( victim == this
      || Level() >= LEVEL_APPRENTICE
      || victim->Level() >= LEVEL_APPRENTICE ) 
    return true;
  
  return is_recognize_set( pcdata->recognize,
			   victim->pcdata->pfile->ident,
			   RECOG_RECOGNIZE );
}


void do_introduce( char_data *ch, const char *argument )
{
  if( is_mob( ch ) ) 
    return;

  if( !*argument ) {
    // List intro and recog data.
    if( !display( ch, RECOG_INTRODUCE, "You have been introduced to" ) ) {
      page( ch, "You have not been introduced to anyone.\n\r" );
    }
    page( ch, "\n\r" );
    if( !display( ch, RECOG_RECOGNIZE, "You recognize" ) ) 
      page( ch, "No one has been introduced to you.\n\r" );
    return;
  }

  if( is_fighting( ch, "make introductions" )
      || is_entangled( ch, "make introductions" )
      || is_drowning( ch, "make introductions" ) )
    return;

  if( ch->position < POS_STANDING ) {
    pos_message( ch );
    return;
  }

  char arg [ MAX_INPUT_LENGTH ];

  if( contains_word( argument, "to", arg ) ) {
    // Intro char1 to char2.

    char_data *actor;

    if( !( actor = one_character( ch, arg, "introduce",
				  ch->array ) ) ) {
      return;
    }
    
    if( actor != ch ) {
      char tmp [ TWO_LINES ];
      snprintf( tmp, TWO_LINES, "introduce %s to", actor->Name( ch ) );
      
      char_data *victim;
      
      if( !( victim = one_character( ch, argument, tmp,
				     ch->array ) ) )
	return;

      if( victim == ch ) {
	fsend( ch, "You can't make %s introduce %sself to you.",
	       actor, actor->Him_Her( ch ) );
	return;
      }

      if( actor == victim ) {
	fsend( ch, "%s already recognizes %sself.",
	       actor, actor->Him_Her( ch ) );
	return;
      }

      if( actor->fighting || victim->fighting ) {
	send( ch, "Wait until they are done fighting for introductions.\n\r" );
	return;
      }
      
      if( actor->position < POS_RESTING || victim->position < POS_RESTING ) {
	send( ch, "Introductions only work when all parties are conscious.\n\r" );
	return;
      }

      if( actor->species || victim->species ) {
	send( ch, "Non-player characters can't participate in introductions.\n\r" );
	return;
      }

      if( !ch->Recognizes( actor ) || !ch->Recognizes( victim ) ) {
	send( ch, "You can't introduce players you don't know yourself.\n\r" );
	return;
      }

      if( !actor->Recognizes( ch ) || !victim->Recognizes( ch ) ) {
	send( ch, "You can't make introductions unless both parties recognize you.\n\r" );
	return;
      }

      if( !ch->Seen( actor ) || !ch->Seen( victim ) ) {
	send( ch, "You can't make introductions unless both parties can see you.\n\r" );
	return;
      }

      if( !can_talk( ch, "make introductions" ) ) {
	return;
      }

      if( !consenting( actor, ch, "introduction" ) ) {
	return;
      }

      if( is_set( actor->pcdata->pfile->flags, PLR_NO_INTRO ) ) {
	fsend( ch, "%s doesn't want to be introduced.", actor );
	return;
      }

      if( is_set( victim->pcdata->pfile->flags, PLR_NO_INTRO ) ) {
	fsend( ch, "%s doesn't want introductions.", victim );
	return;
      }

      if( !actor->Seen( victim ) || !victim->Seen( actor ) ) {
	fsend( ch, "%s and %s can't both see each other.", actor, victim );
	return;
      }

      if( victim->Recognizes( actor ) ) {
	fsend( ch, "%s already recognizes %s.", victim, actor );
	return;
      }

      if( get_language( victim, ch->pcdata->speaking+LANG_FIRST ) == UNLEARNT ) {
	fsend( ch,
	       "You attempt to introduce %s to %s, but %s does not seem to understand a word you say.",
	       actor, victim, victim->He_She( ) );
	return;
      }

      fsend( ch, "You introduce %s to %s.", actor, victim );
      fsend( actor, "%s introduces you to %s.", ch, victim );
      fsend( victim, "%s introduces %s to you as '%s'.", ch, actor, actor->descr->name );
      fsend_seen( ch, "%s introduces %s to %s.", ch, actor, victim );

      set_recognize_bit( victim, actor->pcdata->pfile->ident, RECOG_RECOGNIZE ); 
      set_recognize_bit( actor, victim->pcdata->pfile->ident, RECOG_INTRODUCE );
      return;
    }
  }

  char_data *victim;

  if( !( victim = one_character( ch, argument, "introduce yourself to",
				 ch->array ) ) )
    return;
  
  if( ch == victim ) {
    send( ch, "Unfortunately, you already recognize yourself.\n\r" );
    return;
  }
  
  if( victim->fighting ) {
    fsend( ch,
	   "%s is too busy fighting for you to introduce yourself.", victim );
    return;
  }

  if( victim->position < POS_RESTING ) {
    fsend( ch, "%s isn't in a good state for introductions.", victim );
    return;
  }
  
  if( victim->species ) {
    fsend( ch, "Introducing yourself to %s, a non-player character, does nothing useful.",
	   victim );
    return;
  }
  
  if( victim->Recognizes( ch ) ) {
    fsend( ch, "%s already recognizes you.", victim );
    return;
  }

  if( !can_talk( ch, "introduce yourself" ) ) 
    return;

  if( !ch->Seen( victim ) ) {
    fsend( ch, "%s cannot see you.", victim );
    return;
  }
  
  const char *msg = associate( ch, victim, "introduce yourself to" );

  if( !msg ) 
    return;

  if( get_language( victim, ch->pcdata->speaking+LANG_FIRST ) == UNLEARNT ) {
    fsend( ch,
	   "You attempt to introduce yourself to %s, but %s does not seem to understand a word you say.",
	   victim, victim->He_She( ) );
    return;
  }

  if( is_set( victim->pcdata->pfile->flags, PLR_NO_INTRO )
      && !is_recognize_set( victim->pcdata->recognize, ch->pcdata->pfile->ident, RECOG_INTRODUCE ) ) {
    fsend( ch, "As you try to introduce yourself to %s, %s rudely turns %s back.",
	   victim, victim->He_She( ), victim->His_Her( ) );
    fsend( victim, "%s tries to introduce %sself, but you rudely turn your back.",
	   ch, ch->Him_Her( ) );
    fsend_seen( ch, "%s tries to introduce %sself to %s, who rudely turns %s back.",
		ch, ch->Him_Her( ), victim, victim->His_Her( ) );
    return;
  }

  if( !*msg ) {
    fsend( ch, "You introduce yourself to %s.", victim );
  } else {
    fsend( ch, "You %s introduce yourself to %s.", msg, victim );
  }
  fsend( victim, "%s greets you and introduces %sself as '%s'.",
	 ch, ch->Him_Her( ), ch->descr->name );
  fsend_seen( ch, "%s introduces %sself to %s.",
	      ch, ch->Him_Her( ), victim );

  if( !victim->is_affected( AFF_HALLUCINATE ) ) {
    set_recognize_bit( victim, ch->pcdata->pfile->ident, RECOG_RECOGNIZE ); 
    set_recognize_bit( ch, victim->pcdata->pfile->ident, RECOG_INTRODUCE );
  } else {
    interpret( victim, "boggle" );
  }
}


/*
 *   BEFRIEND FUNCTIONS
 */


// This HAS BEFRIENDED pfile.
bool char_data :: Befriended( pfile_data* pfile ) const
{
  if( !pcdata )
    return false;

  // For switched familiars.
  if( pfile == pcdata->pfile )
    return true;

  return is_recognize_set( pcdata->recognize,
			   pfile->ident, RECOG_BEFRIENDER );
}


// This HAS BEFRIENDED ch.
bool char_data :: Befriended( char_data* ch ) const
{
  if( !pcdata || !ch->pcdata )
    return false;
  
  // For switched familiars.
  if( ch->pcdata->pfile == pcdata->pfile )
    return true;

  return is_recognize_set( pcdata->recognize,
			   ch->pcdata->pfile->ident, RECOG_BEFRIENDER );
}


// This HAS BEFRIENDED ch.
bool pfile_data :: Befriended( char_data *ch ) const
{
  if( !ch->pcdata )
    return false;

  // For switched familiars.
  if( ch->pcdata->pfile == this )
    return true;

  return is_recognize_set( ch->pcdata->recognize,
			   ident, RECOG_BEFRIENDED );
}


void do_befriend( char_data* ch, const char *argument )
{
  if( is_mob( ch ) ) 
    return;
  
  if( !*argument ) {
    if( !display( ch, RECOG_BEFRIENDER, "Befriend List" ) )
      send( ch, "You have befriended no one.\n\r" );
    return;
  }

  // Can stop befriending online/offline players.

  if( pfile_data *pfile = remove_name( ch->pcdata->recognize, argument,
				       RECOG_BEFRIENDER ) ) {
    if( player_data *pl = find_player( pfile ) ) {
      remove_recognize_bit( pl->pcdata->recognize, ch->pcdata->pfile->ident, RECOG_BEFRIENDED );
    } else {
      link_data link;
      link.connected = CON_PLAYING;
      if( !load_char( &link, pfile->name, PLAYER_DIR ) ) {
	bug( "Do_Befriend: Non-existent player file (%s)", pfile->name );
	return;
      }
      pl = link.player;
      remove_recognize_bit( pl->pcdata->recognize, ch->pcdata->pfile->ident, RECOG_BEFRIENDED );
      pl->Save( false );
      pl->Extract();
    }
    fsend( ch, "You no longer consider %s a friend.", pfile->name );
    return; 
  }
  
  // Can only begin befriending someone who is online.

  in_character = false;
  char_data *victim;
 
  if( !( victim = one_character( ch, argument, "befriend",
				 ch->array ) ) )
    return;
  
  if( ch == victim ) {
    send( ch, "Befriending yourself is a waste of time.\n\r" );
    return;
  }
  
  if( victim->species ) {
    fsend( ch, "Befriending %s, a non-player character, does nothing useful.",
	   victim );
    return;
  }
  
  if( !ch->Recognizes( victim ) ) {
    fsend( ch, "You cannot befriend %s without first knowing %s name.",
	   victim, victim->His_Her( ) );
    return;
  }
  
  if( !victim->Recognizes( ch ) ) {
    fsend( ch, 
	   "You cannot befriend %s until you have introduced yourself to %s.",
	   victim, victim->Him_Her( ) );
    return;
  }
  
  if( ch->Befriended( victim ) ) {
    send( ch, "You already think %s is your friend.\n\r", victim );
    return;
  }
  
  const char *msg = associate( ch, victim, "befriend" );

  if( !msg )
    return;

  fsend( ch, "You now consider %s a friend.", victim );

  set_recognize_bit( ch, victim->pcdata->pfile->ident, RECOG_BEFRIENDER );
  set_recognize_bit( victim, ch->pcdata->pfile->ident, RECOG_BEFRIENDED );
}


/*
 *   IGNORE ROUTINES
 */


bool char_data :: Ignoring( char_data* victim ) const
{
  if( !pcdata
      || !victim->pcdata
      || victim->Level() >= LEVEL_APPRENTICE )
    return false;

  if( !is_set( pcdata->pfile->flags, PLR_APPROVED ) 
      && has_permission( victim, PERM_APPROVE ) )
    return false;

  int level = level_setting( &pcdata->pfile->settings, SET_IGNORE );

  switch( level ) {
    case 0 : return false;
    case 1 : return !victim->Recognizes( this );
    case 2 : return !Befriended( victim );
  }

  return true;
}


bool char_data :: Filtering( char_data *victim ) const
{
  if( !pcdata || !victim->pcdata || victim->Level( ) >= LEVEL_APPRENTICE )
    return false;

  return is_recognize_set( pcdata->recognize,
			   victim->pcdata->pfile->ident, RECOG_FILTERING );
}


bool char_data :: Filtering( pfile_data *pfile ) const
{
  if( !pcdata || !pfile || pfile->level >= LEVEL_APPRENTICE )
    return false;

  return is_recognize_set( pcdata->recognize,
			   pfile->ident, RECOG_FILTERING );
}


bool pfile_data :: Filtering( char_data *victim ) const
{
  if( !victim->pcdata || victim->Level( ) >= LEVEL_APPRENTICE )
    return false;

  return is_recognize_set( victim->pcdata->recognize,
			   ident, RECOG_FILTERED );
}


void do_filter( char_data* ch, const char *argument )
{
  if( is_mob( ch ) )
    return;
  
  if( !*argument ) {
    if( !display( ch, RECOG_FILTERING, "Filter List" ) ) 
      send( ch, "You are filtering no one.\n\r" );
    return;
  }
  
  // Can stop filtering online/offline players.

  if( pfile_data *pfile = remove_name( ch->pcdata->recognize, argument,
				       RECOG_FILTERING ) ) {
    /*
  if( pfile_data *pfile = find_pfile( argument ) ) {
    if( remove_recognize_bit( ch->pcdata->recognize, pfile->ident, RECOG_FILTERING ) ) {
    */
    if( player_data *pl = find_player( pfile ) ) {
      remove_recognize_bit( pl->pcdata->recognize, ch->pcdata->pfile->ident, RECOG_FILTERED );
    } else {
      link_data link;
      link.connected = CON_PLAYING;
      if( !load_char( &link, pfile->name, PLAYER_DIR ) ) {
	bug( "Do_Filter: Non-existent player file (%s)", pfile->name );
	return;
      }
      pl = link.player;
      remove_recognize_bit( pl->pcdata->recognize, ch->pcdata->pfile->ident, RECOG_FILTERED );
      pl->Save( false );
      pl->Extract();
    }
    fsend( ch, "You will no longer filter %s.", pfile->name );
    return;
  }

  // Can only begin filtering someone who is online.

  in_character = false;
  
  player_data *victim;

  if( !( victim = one_player( ch, argument, "filter",
			      (thing_array*) &player_list ) ) ) 
    return;
  
  if( victim == ch ) {
    send( ch, 
	  "Filtering yourself might be wise, but it is also pointless.\n\r" );
  } else if( victim->Level() >= LEVEL_APPRENTICE ) {
    send( ch, "You may not filter immortals.\n\r" );
  } else if( ch->Filtering( victim ) ) {
    send( ch, "You are already filtering them.\n\r" );
  } else {
    set_recognize_bit( ch, victim->pcdata->pfile->ident, RECOG_FILTERING );
    set_recognize_bit( victim, ch->pcdata->pfile->ident, RECOG_FILTERED );
    fsend( ch, "You are now filtering %s.", victim );
  }
}


/*
 *    ROUTINES
 */


bool consenting( char_data* victim, char_data* ch, const char* word )
{
  if( !ch
      || ch == victim
      || ch->Level() >= LEVEL_APPRENTICE
      || ( is_set( victim->status, STAT_PET ) && victim->leader == ch )
      )
    return true;
  
  if( ch->pcdata
      && victim->pcdata
      && is_recognize_set( victim->pcdata->recognize,
			   ch->pcdata->pfile->ident, RECOG_CONSENTING ) )
    return true;
  
  if( word != empty_string ) {
    fsend( ch, "Since %s does not automatically consent to your actions, the %s fails.",
	   victim, word );
    fsend( victim, "You automatically disrupt %s.", word );
  } 
  
  return false;
}


void do_consent( char_data* ch, const char *argument )
{
  char_data*          victim;
  pfile_data*          pfile;

  if( is_mob( ch ) )
    return;
  
  if( !*argument ) {
    if( !display( ch, RECOG_CONSENTING, "Consent List" ) ) 
      send( ch, "You have consented no one.\n\r" );
    return;
  }

  if( ( pfile = remove_name( ch->pcdata->recognize,
			     argument, RECOG_CONSENTING ) ) ) {
    send( ch, "You no longer automatically consent %s.\n\r", pfile->name );
    return;
  }

  if( !( victim = one_player( ch, argument, "consent",
			      ch->array, (thing_array*) &player_list ) ) )
    return;
  
  if( victim == ch ) {
    send( ch, "That you agree with your own actions is assumed.\n\r" );
    return;
  }
  
  if( !ch->Recognizes( victim ) ) {
    send( ch, "You may only consent someone whom you recognize.\n\r" );
    return;
  } 
  
  if( !victim->pcdata ) {
    send( ch, "Consent only matters with players.\n\r" );
    return;
  }
  
  if( consenting( ch, victim ) ) {
    fsend( ch, "%s is already listed for automatic consent.", victim );
    return;
  }
  
  set_recognize_bit( ch, victim->pcdata->pfile->ident, RECOG_CONSENTING );
  
  fsend( ch, "You now automatically consent to the actions of %s.",
	 victim );
}
