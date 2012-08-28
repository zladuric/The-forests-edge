#include <ctype.h>
#include <sys/types.h>
#include <stdio.h>
#include <limits.h>
#include "define.h"
#include "struct.h"


void   room_info          ( char_data*, room_data* );
void   show_tracks        ( char_data* );
bool   show_characters    ( char_data* );
void   show               ( char_data*, thing_data* );

void   show_equipment     ( char_data*, char_data* );
void   show_inventory     ( char_data*, char_data* );
void   show_description   ( char_data*, char_data* );


/*
 *   CAN_SEE ROUTINES
 */


bool char_data :: Can_See( bool msg ) const
{
  if( position <= POS_SLEEPING
      || is_affected( AFF_BLIND ) ) {
    if( msg )
      send( this, "You can't see a thing!\n\r" );
    return false;
  }
  
  return true;
}


bool Room_Data :: Seen( const char_data* ch ) const
{
  if( ch ) {
    if( !ch->Can_See( ) )
      return false;
    
    // Note: switched imms' holylight works.
    if( has_holylight( ch ) )
      return true;
  }

  return !is_dark( ch );
}


bool char_data :: Seen( const char_data* ch ) const
{
  if( !In_Game( ) )
    return false;

  if( !ch
      || this == ch
      || rider == ch
      || mount == ch )
    return true;

  if( get_trust( ch ) < invis_level( this ) ) {
    return false;
  }

  if( !in_character )
    return true;
  
  if( !ch->Can_See( ) )
    return false;
 
  // Note: switched imms' holylight works.
  if( has_holylight( ch ) )
    return true;
  
  if( was_in_room )
    return false;

  if( rider
      && rider->Seen( ch ) )
    return true;

  if( in_room
      && in_room->is_dark( ch )
      && ( !ch->is_affected( AFF_INFRARED ) 
	   || ( species && !is_set( species->act_flags, ACT_WARM_BLOODED ) ) ) )
    return false;
  
  if( is_affected( AFF_INVISIBLE )
      && !ch->Sees_Invis( ) )
    return false;
  
  if( is_set( status, STAT_CAMOUFLAGED )
      && !ch->Sees_Camo( )
      && !seen_by.includes( const_cast<char_data *>( ch ) ) )
    return false;
  
  if( is_set( status, STAT_HIDING )
      && !ch->Sees_Hidden( )
      && !seen_by.includes( const_cast<char_data *>( ch ) ) )
    return false;

  return true;
}


bool Obj_Data :: Seen( const char_data* ch ) const
{
  //  if( array && !array->where )
  //    return true;

  if( is_set( extra_flags, OFLAG_SECRET )
      && !has_holylight( ch ) ) {
    return false;
  }

  // Auctioned items.
  if( array && Auction( array->where ) )
    return true;

  if( ch->position <= POS_SLEEPING ) {
    return false;
  }

  // Can always see own inventory and worn.
  if( array == &ch->contents
      || array == &ch->wearing )
    return true;
  
  // Can always see in bags held or worn.
  if( array
      && array->where
      && ( array->where->array == &ch->contents
	   || array->where->array == &ch->wearing ) )
      return true;

  if( ch->is_affected( AFF_BLIND ) )
    return false;
  
  // Note: switched imms' holylight works.
  if( has_holylight( ch ) )
    return true;

  if( is_set( extra_flags, OFLAG_IS_INVIS )
      && !ch->Sees_Invis( ) )
    return false;

  if( ch->in_room && ch->in_room->is_dark( ch ) )
    return false;

  return true;
}


/*
 *   ATTRIBUTES
 */


bool char_data :: detects_evil( ) const
{
  return( is_affected( AFF_TRUE_SIGHT ) 
	  || is_affected( AFF_DETECT_EVIL ) );
}


bool char_data :: detects_good( ) const
{
  return( is_affected( AFF_TRUE_SIGHT ) 
	  || is_affected( AFF_DETECT_GOOD ) );
}


bool char_data :: detects_law( ) const
{
  return( is_affected( AFF_TRUE_SIGHT ) 
	  || is_affected( AFF_DETECT_LAW ) );
}


bool char_data :: detects_chaos( ) const
{
  return( is_affected( AFF_TRUE_SIGHT ) 
	  || is_affected( AFF_DETECT_CHAOS ) );
}


bool char_data :: Sees_Invis( ) const
{
  if( has_holylight( this ) )
    /*
  if( pcdata
      && is_set( pcdata->pfile->flags, PLR_HOLYLIGHT ) )
    */
    return true;

  return( is_affected( AFF_SEE_INVIS ) 
	  || is_affected( AFF_TRUE_SIGHT ) );
}


bool char_data :: Sees_Hidden( ) const
{
  if( has_holylight( this ) )
    /*
  if( pcdata
      && is_set( pcdata->pfile->flags, PLR_HOLYLIGHT ) )
    */
    return true;
  
  return( is_affected( AFF_DETECT_HIDDEN )
	  || is_affected( AFF_SEE_CAMOUFLAGE )
	  || is_affected( AFF_SENSE_LIFE ) );
}


bool char_data :: Sees_Camo( ) const
{
  if( has_holylight( this ) )
    /*
  if( pcdata
      && is_set( pcdata->pfile->flags, PLR_HOLYLIGHT ) )
    */
    return true;
  
  return( is_affected( AFF_SEE_CAMOUFLAGE ) 
	  || is_affected( AFF_SENSE_LIFE ) );
}


/* 
 *   SHOW OBJECT ROUTINES
 */


/*
// Somewhat similar to page_priv()...
static void page_contents( char_data* ch, thing_data *thing,
			   const char *msg1,
			   const char *msg2 )
{
  // Set the list of selected things to the list of things
  select( thing->contents, ch );

  // Compile the list so that like looking items are grouped together.
  rehash( ch, thing->contents );

  if( none_shown( thing->contents ) ) {
    if( victim && victim != ch ) {
      fpage( ch, "%s %s nothing.", victim, text );
    } else {
      fpage( ch, "%s nothing.", text );
    }
    return;
  }

  thing_data *thing;
  if( ( thing = one_shown( thing->contents ) ) ) {
    if( victim && victim != ch ) {
      fpage( ch, "%s %s %s.", victim, text, thing->contents );
    } else {
      fpage( ch, "%s %s", text, thing->contents );
    }
    return;
  }

  if( victim && victim != ch ) {
    fpage( ch, "%s %s:", victim, text );
  } else {
    fpage( ch, "%s:", text );
  }

  for( int i = 0; i < thing->contents; ++i ) {
    thing = thing->contents[i];
    if( thing->shown > 0 ) {
      page( ch, "  %s\n\r", thing );
    }
  }
}
*/


/*
static void send( char_data* ch, thing_array& array )
{
  thing_data*   thing;
  bool        nothing  = true;

  select( array );
  rehash( ch, array );
  
  for( int i = 0; i < array; i++ ) {
    thing = array[i];
    if( thing->shown > 0 ) {
      nothing = false;
      send( ch, "  %s\n\r", thing );
      }
    }

  if( nothing )
    send( ch, "  nothing\n\r" );
}
*/


/* 
 *   TRACK AND SEARCH ROUTINES
 */


bool search( char_data *ch )
{
  for( int i = 0; i < ch->in_room->exits; ++i ) {
    exit_data *exit = ch->in_room->exits[i];
    if( is_set( exit->exit_info, EX_SEARCHABLE )
	&& is_set( exit->exit_info, EX_CLOSED )
	&& is_set( exit->exit_info, EX_SECRET ) 
	&& !ch->seen_exits.includes( exit )
	&& ch->check_skill( SKILL_SEARCHING ) ) {
      send( ch, "\n\r%s>> You detect something unusual %s. <<%s\n\r",
	    bold_v( ch ), dir_table[ exit->direction ].name, normal( ch ) );
      ch->seen_exits += exit;
      ch->improve_skill( SKILL_SEARCHING );
      return true;
    }
  }

  return false;
}


static void show_secret( char_data* ch ) 
{
  if( !ch->pcdata
      || !is_set( ch->pcdata->pfile->flags, PLR_SEARCHING )
      || !ch->in_room->Seen( ch )
      || ch->move == 0 )
    return;

  --ch->move;

  search( ch );
}


/*
 *   GLANCE ROUTINES
 */


static void glance( char_data* ch, char_data* victim )
{
  if( victim == ch ) {
    page( ch, "Your position is %s.\n\r",
	  victim->position_name( ch ) );
    page( ch, "Your condition is %s.\n\r",
	  condition_word( ch ) );
    return;
  }
  
  const char *pos;
  if( victim->position < POS_SLEEPING ) {
    pos = "lying";
  } else {
    pos = victim->position_name( ch );
  }

  page( ch, "%s is %s here, and %s %s%s%s%c\n\r",
	victim->Seen_Name( ch ), pos,
	condition_prep( victim ),
	color_scale( ch, 6-6*victim->hit/victim->max_hit ),
	condition_word( victim ),
	normal( ch ), victim->hit > 0 ? '.' : '!' );
}


void do_glance( char_data *ch, const char *argument )
{
  if( !ch->Can_See( true ) )
    return;

  char_data *victim;

  if( !( victim = one_character( ch, argument, "glance at", ch->array ) ) )
    return;

  msg_type = MSG_LOOK;

  if( ch != victim ) {
    fsend( ch, "You glance at %s.", victim );
    send( ch, "\n\r" );
    if( ch->Seen( victim ) ) {
      fsend( victim, "%s glances at you.", ch );
    }
    fsend_mesg( ch, "%s glances at %s.", ch, victim );
  } else {
    send( ch, "You glance at yourself.\n\r\n\r" );
    fsend_mesg( ch, "%s glances at %sself.",
		ch, ch->Him_Her( ) );
  }
  
  glance( ch, victim );
}


/*
 *   LOOK AT
 */


void char_data :: make_known( char_data *ch )
{
  if( species && ch->pcdata ) {
    if( ch->species ) {
      // Mobs/pets gain no name recognition by looking.
      // Switched familiars give knowledge of name to player.
      if( ch->link && ch->link->player ) {
	known_by += ch->link->player;
      }
    } else {
      known_by += ch;
    }
  }
}


void char_data :: Look_At( char_data* ch )
{
  make_known( ch );

  msg_type = MSG_LOOK;

  if( ch != this ) {
    fsend( ch, "You look at %s.", this );
    send( ch, "\n\r" );
    if( ch->Seen( this ) ) {
      fsend( this, "%s looks at you.", ch );
    }
    fsend_mesg( ch, "%s looks at %s.", ch, this );
  } else {
    send( ch, "You look at yourself.\n\r\n\r" );
    fsend_mesg( ch, "%s looks at %sself.",
		ch, ch->Him_Her( ) );
  }
  
  show_description( ch, this );
  page( ch, scroll_line[0] );
  glance( ch, this );
  page( ch, "\n\r" );
  show_equipment( ch, this );
}


void Exit_Data :: Look_At( char_data* ch )
{
  msg_type = MSG_LOOK;

  if( is_set( exit_info, EX_CLOSED ) ) {
    fsend( ch, "%s %s closed.", this,
	   exit_verb( ch, this ) );
    fsend_mesg( ch, "%s looks at %s.",
		ch, this );
  } else {
    fsend( ch, "%s is %s.",
	   dir_table[ direction ].where,
	   to_room->Name( ch ) );
    fsend_mesg( ch, "%s looks %s.",
		ch, dir_table[ direction ].name );
  }
}


void Extra_Data :: Look_At( char_data* ch )
{
  char tmp [ 3*MAX_STRING_LENGTH ];

  convert_to_ansi( ch, 3*MAX_STRING_LENGTH, text, tmp );
  send( ch, tmp );
}


/* 
 *   SHOW CHARACTER FUNCTIONS
 */


void do_peek( char_data* ch, const char *argument )
{
  if( !ch->Can_See( true ) )
    return;

  char_data *victim;

  if ( ch->get_skill( SKILL_PEEK ) == UNLEARNT ) {
    send( ch, "You are not adept at peeking.\n\r" );
    return;
  }

  if( !( victim = one_character( ch, argument, "peek at", ch->array ) ) )
    return;
  
  show_equipment( ch, victim );
  page( ch, "\n\r" );
 
  // Don't do this, mob may have inv. resets.
  //  if( !victim->can_carry( ) )
  //    return;

  if( ch == victim ) {
    show_inventory( ch, victim );
  } else if( victim->Level() < LEVEL_BUILDER
	     && ch->check_skill( SKILL_PEEK, 15 ) ) {
    show_inventory( ch, victim );
    ch->improve_skill( SKILL_PEEK );
  } else {
    if( ch->Seen( victim ) && !ch->check_skill( SKILL_PEEK, 15 ) ) {
      fpage( ch, "%s notices you peeking at %s!",
	     victim, victim->Him_Her() );
      fsend( victim, "You notice %s trying get a peek at your inventory!",
	     ch );
    } else {
      fpage( ch, "You fail to get a peek at %s's inventory.", victim );
    }
  }
}


void do_qlook( char_data *ch, const char *argument )
{
  if( !ch->Can_See( true ) )
    return;
  
  char_data* victim;

  if( !( victim = one_character( ch, argument, "look quickly at",
				 ch->array ) ) )
    return;
  
  victim->make_known( ch );

  msg_type = MSG_LOOK;

  if( ch != victim ) {
    fsend( ch, "You take a quick look at %s.", victim );
    send( ch, "\n\r" );
    if( ch->Seen( victim ) ) {
      fsend( victim, "%s takes a quick look at you.", ch );
    }
    fsend_mesg( ch, "%s takes a quick look at %s.", ch, victim );
  } else {
    send( ch, "You take a quick look at yourself.\n\r\n\r" );
    fsend_mesg( ch, "%s takes a quick look at %sself.",
		ch, ch->Him_Her( ) );
  }
  
  show_description( ch, victim );
}


void show_inventory( char_data* ch, char_data* victim )
{
  const bool something = ( select( victim->contents, ch ) != 0 );

  if( ch == victim ) {
    if( something ) {
      page_priv( ch, 0, empty_string );
      page_priv( ch, &victim->contents, "are carrying" );
    } else {
      page( ch, "You are not carrying anything.\n\r" );
      //    page_contents( ch, ch, "You are carrying" );
    }
  } else {
    if( something ) {
      include_empty = false;
      page_priv( ch, 0, empty_string );
      page_priv( ch, &victim->contents, "is carrying", victim );
      include_empty = true;
    } else {
      fpage( ch, "%s is not carrying anything.", victim );
      //    page_contents( ch, victim, "is carrying" );
    }
  }
}


void show_description( char_data *ch, char_data *victim )
{
  if( victim->species ) {
    for( mprog_data *mprog = victim->species->mprog; mprog; mprog = mprog->next ) {
      if( mprog->trigger == MPROG_TRIGGER_DESCRIBE
	  && !*mprog->string ) {
	clear_variables( );
	var_room = Room( victim->array->where );
	var_ch = ch;
	var_mob = victim;
	var_arg = empty_string;
	if( !mprog->execute( ) )
	  return;
	break;
      }
    }
  }

  char tmp  [ 3*MAX_STRING_LENGTH ];
  
  if( *victim->descr->complete
      && ( victim->species
	   || is_set( victim->pcdata->pfile->flags, PLR_APPROVED ) ) ) {
    convert_to_ansi( ch, 3*MAX_STRING_LENGTH, victim->descr->complete, tmp );
    page( ch, tmp );
  } else {
    fpage( ch, "You see nothing special about %s.  In fact, you doubt\
 %s has any special distinguishing characteristics at all.",
	   victim->Him_Her( ), victim->He_She( ) );
  }
}


/*
 *   EQUIPMENT 
 */


void do_equipment( char_data* ch, const char *argument )
{
  char_data *victim;
  bool loaded = false;

  if( has_permission( ch, PERM_PLAYERS )
      && *argument ) {
    in_character = false;

    char arg [ MAX_INPUT_LENGTH ];

    argument = one_argument( argument, arg );

    if( pfile_data *pfile = find_pfile( arg ) ) {

      if( pfile != ch->pcdata->pfile
	  && pfile->trust >= get_trust( ch ) ) {
	fsend( ch, "You cannot view the inventory of %s.", pfile->name );
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
      if( !( victim = one_mob( ch, arg, "equipment", (thing_array*) ch->array ) ) )
	return;
    }

    if( *argument ) {
      if( ch != victim ) {
	snprintf( arg, MAX_INPUT_LENGTH, "Equipment for %s", victim->Name( ch ) );
      }
      browse( ch, argument, &victim->wearing,
	      ( ch == victim ) ? 0 : arg );

      if( loaded ) {
	page( ch, "\n\r" );
	page_centered( ch, "[ Player file was loaded from disk. ]" );
	victim->Extract();
	extracted.delete_list();
      }
      return;
    }

  } else {
    victim = ch;
  }

  if( victim != ch
      && !victim->wearing.is_empty( ) ) {
    page_title( ch, "Equipment for %s", victim );
  }

  show_equipment( ch, victim );

  if( loaded ) {
    page( ch, "\n\r" );
    page_centered( ch, "[ Player file was loaded from disk. ]" );
    victim->Extract();
    extracted.delete_list();
  }
}


void show_equipment( char_data* ch, char_data* victim )
{
  const char *const format  = "%-22s %s%-s%s%*s %s\n\r";
  char*          tmp  = static_string( );
  bool         found  = false;
  obj_data**    list  = (obj_data**) victim->wearing.list;
  int           j;

  bool any_concealed = false;
  const bool show_all = ( ch == victim );
  include_empty = show_all;

  select( victim->wearing );

  for( int i = 0; i < victim->wearing; i = j ) {
    for( j = i+1; j < victim->wearing
	   && list[i]->position == list[j]->position; ++j ) {
      if( is_set( list[j]->extra_flags, OFLAG_COVER ) ) {
	if( !show_all
	    && !has_holylight( ch ) ) {
	  i = j;
	} else {
	  any_concealed = true;
	  for( int k = i; k < j; ++k ) {
	    list[k]->Select( 0 );
	  }
	}
      }
    }
    
    for( j = i; j < victim->wearing
	   && list[i]->position == list[j]->position; ++j ) {
      if( list[j]->Seen( ch ) ) {
        if( !found ) {
          page_centered( ch, "+++ Equipment +++" );
          snprintf( tmp, THREE_LINES, format, "Body Location", "", "Item", "", 42-4, "", "Condition" );
          page_underlined( ch, tmp );
          found = true;
	}
	const bool concealed = ( list[j]->Selected( ) == 0 );
	const char *const name = list[j]->Name( ch, 1, true );
	const size_t len = strlen( name );
	page( ch, format,
	      j == i ? where_name[ list[i]->position ] : "",
	      concealed ? color_code( ch, COLOR_MILD ) : "",
	      name,
	      concealed ? color_code( ch, COLOR_DEFAULT ) : "",
	      42-len, "",
	      list[j]->condition_name( ch, true ) );
      }
    }
  }

  include_empty = true;

  if( !found ) {
    if( ch == victim )
      page( ch, "You have nothing equipped.\n\r" );
    else
      page( ch, "%s has nothing equipped.\n\r", victim );

  } else {
    if( ch == victim ) {
      if( any_concealed ) {
	page( ch, "\n\r%22s[ %sHighlighted%s items are concealed. ]\n\r",
	      "",
	      color_code( ch, COLOR_MILD ),
	      color_code( ch, COLOR_DEFAULT ) );
      }
      page( ch, "\n\rWeight: %.2f lbs\n\r", 
	    (double)ch->wearing.weight/100.0 );
    }
  }
}


/*
 *   SCAN FUNCTIONS
 */


bool scan_aggro( char_data *ch, exit_data *exit )
{
  if( is_set( exit->exit_info, EX_CLOSED )
      || is_set( exit->exit_info, EX_NO_SCAN ) )
    return false;

  room_data *room = exit->to_room;

  for( int i = 0; i < room->contents; ++i ) {
    char_data *rch = character( room->contents[i] );
    if( rch
	&& ( !rch->species || !is_set( rch->species->act_flags, ACT_NO_SCAN ) )
	&& ( !rch->species || !is_set( rch->species->act_flags, ACT_MIMIC ) )
	&& rch->Seen( ch )
	&& is_aggressive( ch, rch ) ) {
      return true;
    }
  }

  return false;
}


static bool scan_room( char_data* ch, room_data* room, const char* word,
		       bool need_return, bool from_dark )
{
  char           tmp  [ MAX_STRING_LENGTH ];
  char_data*     rch;
  bool         found  = false;
  int         length  = 0; 
  int l = 0;

  room->distance = 0;

  select( room->contents, ch );
  ch->Select( 0 );
  rehash( ch, room->contents, true );
  
  for( int i = 0; i < room->contents; ++i ) {
    if( !( rch = character( room->contents[i] ) )
	|| rch->Shown( ) == 0
	|| rch->species && is_set( rch->species->act_flags, ACT_NO_SCAN )
	|| rch->species && is_set( rch->species->act_flags, ACT_MIMIC ) )
      continue;
    
    //    if( rch->Seen( ch ) ) {
    const char *name = rch->Seen_Name( ch, rch->Shown( ) );
      //    } else if( ch->is_affected( AFF_SENSE_DANGER ) ) {
      //      name = "someone";
      //    } else {
      //      break;
      //    }
    
    const bool is_pet = is_set( rch->status, STAT_PET )
      && rch->species
      && ((mob_data*)rch)->pet_name != empty_string
      && ch->knows( rch );

    if( !found ) {
      if( need_return )
	send( ch, "\n\r" );
      if( is_pet ) {
	l = snprintf( tmp, MAX_STRING_LENGTH, "%12s : %s%s%s",
		      word,
		      color_code( ch, COLOR_MILD ),
		      name,
		      color_code( ch, COLOR_DEFAULT ) );
      } else {
	l = snprintf( tmp, MAX_STRING_LENGTH, "%12s : %s", word, name );
      }
      *tmp = toupper( *tmp );
      length = l;
      found = true;
    } else {
      const int n = strlen( name );
      length += n+2;
      if( length > 78 ) {
	if( is_pet ) {
	  l += snprintf( tmp+l, MAX_STRING_LENGTH-l, ",\n\r               %s%s%s",
			 color_code( ch, COLOR_MILD ),
			 name,
			 color_code( ch, COLOR_DEFAULT ) );
	} else {
	  l += snprintf( tmp+l, MAX_STRING_LENGTH-l, ",\n\r               %s", name );
	}
	length = n+15;
      } else if( is_pet ) {
	l += snprintf( tmp+l, MAX_STRING_LENGTH-l, ", %s%s%s",
		       color_code( ch, COLOR_MILD ),
		       name,
		       color_code( ch, COLOR_DEFAULT ) );
      } else {
	l += snprintf( tmp+l, MAX_STRING_LENGTH-l, ", %s", name );
      }
    }
  }

  if( found ) {
    sprintf( tmp+l, "\n\r" );
    send( ch, tmp );
  } else {
    if( !from_dark
	&& !room->Seen( ch )
	) {
      if( need_return )
        send( ch, "\n\r" );
      send( ch, "%12s : darkness\n\r", word );
      return true;
    }
  }

  return found;
}


static void show_scan( char_data *ch, bool is_auto, bool is_short, bool is_near )
{
  bool anything = false;
  bool far = false;
  char tmp [ ONE_LINE ];
  room_data *room = ch->in_room;

  if( !is_auto && !is_short ) 
    anything = scan_room( ch, room, "[Here]", false, false );
  
  if( !is_near && ch->get_skill( SKILL_SCAN ) == UNLEARNT )
    is_near = true;

  room->distance = 0;

  bool dark = !room->Seen( ch );

  for( int i = 0; i < room->exits; ++i ) {
    exit_data *exit1 = room->exits[i];
    if( is_set( exit1->exit_info, EX_CLOSED ) )
      continue;
    const bool need_return = !anything && is_auto;
    if( is_set( exit1->exit_info, EX_NO_SCAN ) ) {
      if( need_return )
        send( ch, "\n\r" );
      send( ch, "%12s : unknown\n\r", dir_table[exit1->direction].name );
      anything = true;
      continue;
    }
    room_data *room1 = exit1->to_room;
    anything |= scan_room( ch, room1,
			   dir_table[exit1->direction].name,
			   need_return,
			   dark );
    if( !is_near ) {
      bool dark2 = !room1->Seen( ch );
      for( int j = 0; j < room1->exits; j++ ) {
	exit_data *exit2 = room1->exits[j];
        if( is_set( exit2->exit_info, EX_CLOSED ) )
          continue;
	bool need_return = !anything && is_auto;
        room_data *room2 = exit2->to_room;
        if( room2->distance != 0 ) {
          snprintf( tmp, ONE_LINE, "%s %s",
		    exit1->direction == exit2->direction
		    ? "far" : dir_table[ exit1->direction ].name,
		    dir_table[ room1->exits[j]->direction ].name );
	  if( is_set( exit2->exit_info, EX_NO_SCAN ) ) {
	    if( need_return )
	      send( ch, "\n\r" );
	    send( ch, "%12s : unknown\n\r", tmp );
	    anything = true;
	    continue;
	  }
          if( scan_room( ch, room2, tmp,
			 need_return,
			 dark2 ) ) {
	    anything = true;
	    far = true;
	  }
	}
      }
    }
  } 
  
  if( !anything && !is_auto ) 
    send( ch, "You see nothing in the vicinity.\n\r" );

  if( far && number_range( 1, 10 ) == 1 )
    ch->improve_skill( SKILL_SCAN );

  /*--  CLEANUP DISTANCE --*/
  
  room->distance = INT_MAX;
  
  for( int i = 0; i < room->exits; ++i ) { 
    room_data *room1 = room->exits[i]->to_room;
    room1->distance = INT_MAX;
    for( int j = 0; j < room1->exits; ++j )
      room1->exits[j]->to_room->distance = INT_MAX;
  }
}


void do_scan( char_data* ch, const char *argument )
{
  int flags;

  if( !get_flags( ch, argument, &flags, "sn", "scan" ) )
    return;

  if( !ch->Can_See( true ) ) {
    return;
  }

  show_scan( ch, false, is_set( flags, 0 ), is_set( flags, 1 ) );
  
  set_delay( ch, 2 );
}


/*
 *   LOOK ON OBJECT
 */


void look_on( char_data* ch, obj_data* obj )
{
  // Note: these use page in case called from Obj_Data::Look_At().
  obj->Select( 1 );

  switch( obj->pIndexData->item_type ) {
  case ITEM_SPELLBOOK:
  case ITEM_DRINK_CON:
  case ITEM_FOUNTAIN:
  case ITEM_CONTAINER:
  case ITEM_KEYRING :
  case ITEM_CORPSE :
  case ITEM_PIPE:
    //  case ITEM_WEAPON:
    page( ch, "Perhaps you should look *in* %s, instead of on it.\n\r", obj );
    return;

  case ITEM_TABLE :
  case ITEM_CHAIR:
    {
      obj_act_spam( MSG_LOOK, ch, obj, 0, "look on", "looks on", true );
      page( ch, "\n\r" );
      if( select( obj->contents, ch ) == 0 ) {
	fpage( ch, "You see nothing on %s.", obj );
      } else {
	page_priv( ch, 0, empty_string );
	page_priv( ch, &obj->contents, "see", obj, empty_string, "on" );
      }
      return;
    }
     
  default:
    page( ch, "%s is not something you can look on.\n\r", obj );
    return;
  }
}


static void look_on( char_data* ch, const char *argument )
{
  if( !*argument ) {
    send( ch, "Look on what?\n\r" );
    return;
  }

  obj_data *obj;

  if( !(obj = one_object( ch, argument, "look on",
			  ch->array ) ) ) {
    return;
  }

  look_on( ch, obj );
}


/*
 *   LOOK IN OBJECT
 */


static const char *liquid_seen( obj_data *obj )
{
  if( obj->value[2] < 0 || obj->value[2] >= table_max[TABLE_LIQUID] )
    return "[BUG]";

  if( is_set( obj->extra_flags, OFLAG_KNOWN_LIQUID ) ) {
    return liquid_table[obj->value[2]].name;
  }

  return liquid_table[obj->value[2]].color;
}


void look_in( char_data* ch, obj_data* obj )
{
  // Note: these use page in case called from Obj_Data::Look_At().
  obj->Select( 1 );

  switch( obj->pIndexData->item_type ) {
  case ITEM_TABLE:
  case ITEM_CHAIR:
    page( ch, "Perhaps you should look *on* %s, instead of in it.\n\r", obj );
    return;

  case ITEM_SPELLBOOK:
    obj_act_spam( MSG_LOOK, ch, obj, 0, "look in", "looks in", true );
    page( ch, "\n\r" );
    page( ch, "%s is blank.\n\r", obj );
    return;
    
  case ITEM_DRINK_CON:
    obj_act_spam( MSG_LOOK, ch, obj, 0, "look in", "looks in", true );
    page( ch, "\n\r" );
    if ( obj->value[1] == 0 ) {
      page( ch, "Not a drop of liquid to be seen.\n\r" );
    } else if ( obj->value[1] > 0
		&& obj->value[1] < obj->pIndexData->value[0]/4 ) {
      include_liquid = true;
      fpage ( ch, "%s is nearly empty.\n\r", obj );
    } else {
      include_liquid = false;
      const char *word = empty_string;
      if ( obj->value[1] > 0 ) {
	if ( obj->value[1] < obj->pIndexData->value[0] / 2 ) {
	  word = "less than half ";
	} else if ( obj->value[1] < 3*obj->pIndexData->value[0] / 4 ) {
	  word = "more than half ";
	} else if ( obj->value[1] < obj->pIndexData->value[0] ) {
	  word = "nearly ";
	}
	fpage( ch, "%s is %sfull of %s.", obj, word, liquid_seen( obj ) );
      }
    }
    include_liquid = true;
    return;
    
  case ITEM_FOUNTAIN:
    obj_act_spam( MSG_LOOK, ch, obj, 0, "look in", "looks in", true );
    page( ch, "\n\r" );
    fpage( ch, "%s is full of %s.", obj, liquid_seen( obj ) );
    return;
    
  case ITEM_CONTAINER:
    if( is_set( &obj->value[1], CONT_CLOSED ) ) {
      const char *me_loc, *them_loc;
      obj_loc_spam( ch, obj, 0, me_loc, them_loc );
      include_closed = false;
      page( ch, "%s%s is closed.\n\r", obj, me_loc );
      include_closed = true;
      return;
    }
    // Continues...
  case ITEM_KEYRING :
  case ITEM_CORPSE :
  case ITEM_PIPE:
    {
      obj_act_spam( MSG_LOOK, ch, obj, 0, "look in", "looks in", true );
      page( ch, "\n\r" );
      include_closed = false;
      if( select( obj->contents, ch ) == 0 ) {
	fpage( ch, "%s is empty.", obj );
      } else {
	page_priv( ch, 0, empty_string );
	page_priv( ch, &obj->contents, "contains", obj );
      }
      include_closed = true;
      return;
    }

  case ITEM_WEAPON:
    if( obj->pIndexData->value[3] == WEAPON_BOW-WEAPON_UNARMED ) {
      obj_act_spam( MSG_LOOK, ch, obj, 0, "look in", "looks in", true );
      page( ch, "\n\r" );
      include_closed = false;
      if( select( obj->contents, ch ) == 0 ) {
	fpage( ch, "%s is empty.", obj );
      } else {
	page_priv( ch, 0, empty_string );
	page_priv( ch, &obj->contents, "contains", obj );
      }
      return;
    }
    // Continues...
  default:
    page( ch, "%s is not something you can look in.\n\r", obj );
    return;
  }
}


static void look_in( char_data* ch, const char *argument )
{
  if( !*argument ) {
    send( ch, "Look in what?\n\r" );
    return;
  }

  obj_data *obj;

  if( !( obj = one_object( ch, argument, "look in",
			   ch->array,
			   &ch->contents,
			   &ch->wearing  ) ) ) {
    return;
  }

  look_in( ch, obj );
}


/*
 *   MAIN LOOK ROUTINE
 */


void do_look( char_data* ch, const char *argument )
{
  room_data *room = ch->in_room;

  if( !*argument ) {
    //    send_seen( ch, "%s looks around.\n\r", ch );
    show_room( ch, room, false, false );
    return;
  }

  if( !ch->Can_See( true ) )
    return;

  if( !strncasecmp( argument, "in ", 3 ) ) {
    argument += 3;
    look_in( ch, argument );
    return;
  }

  if( !strncasecmp( argument, "on ", 3 ) ) {
    argument += 3;
    look_on( ch, argument );
    return;
  }

  if( !strncasecmp( argument, "at ", 3 ) )
    argument += 3;
  
  char arg [ MAX_INPUT_LENGTH ];
  argument = one_argument( argument, arg );

  visible_data* vis;
  if( !( vis = one_visible( ch, arg, "look at",
			    (visible_array*) &room->Extra_Descr( ), -1,
			    (visible_array*) &room->exits, -1,
			    (visible_array*) ch->array, -1,
			    (visible_array*) &ch->contents, -1,
			    (visible_array*) &ch->wearing, -1 ) ) )
    return;

  if( extra( vis ) ) {
    for( action_data *act = room->action; act; act = act->next ) {
      if( act->trigger == TRIGGER_DESCRIBE
	  && is_name( act->target, vis->Keywords( ch ) ) ) {
	clear_variables( );
	var_room = room;
	var_ch = ch;
	var_arg = arg;
	if( !act->execute( ) )
	  return;
	break;
      }
    }

    vis->Look_At( ch );
    return;
  }

  obj_data *obj = object( vis );
  mob_data *npc = mob( vis );
  char_data *rch = character( vis );

  if( !*argument ) {
    vis->Look_At( ch );
    return;
  }

  if( obj ) {
    // Object extras.
    if( extra_data *ext = (extra_data *) one_visible( ch, argument, empty_string,
						      (visible_array *) &obj->pIndexData->extra_descr, -1 ) ) {

      // Special keywords.
      if( strcmp( ext->keyword, "either" )
	  && strcmp( ext->keyword, "before" )
	  && strcmp( ext->keyword, "after" ) ) {
	
	obj_act_spam( MSG_LOOK, ch, obj, 0, "look at", "looks at" );
	
	send( ch, "\n\r" );
	
	for( oprog_data *oprog = obj->pIndexData->oprog; oprog; oprog = oprog->next ) {
	  if( oprog->trigger == OPROG_TRIGGER_DESCRIBE
	      && is_name( oprog->target, ext->Keywords( ch ) ) ) {
	    clear_variables( );
	    var_room = room;
	    var_ch = ch;
	    var_obj = obj;
	    var_arg = argument;
	    if( !oprog->execute( ) )
	      return;
	    break;
	  }
	}
	
	ext->Look_At( ch );
	return;
      }
    }

  } else if( npc ) {
    // Mob extras.
    if( extra_data *ext = (extra_data *) one_visible( ch, argument, empty_string,
						      (visible_array *) &npc->species->extra_descr, -1 ) ) {
      
      msg_type = MSG_LOOK;
      
      if( npc != ch ) {
	fsend( ch, "You look at %s.", npc );
	fsend_mesg( ch, "%s looks at %s.", ch, npc );
	send( ch, "\n\r" );
      } else {
	send( ch, "You look at yourself.\n\r\n\r" );
	fsend_mesg( ch, "%s looks at %sself.",
		    ch, ch->Him_Her( ) );
      }
      
      for( mprog_data *mprog = npc->species->mprog; mprog; mprog = mprog->next ) {
	if( mprog->trigger == MPROG_TRIGGER_DESCRIBE
	    && is_name( mprog->string, ext->Keywords( ch ) ) ) {
	  clear_variables( );
	  var_room = room;
	  var_ch = ch;
	  var_mob = npc;
	  var_arg = argument;
	  if( !mprog->execute( ) )
	    return;
	  break;
	}
      }
      
      ext->Look_At( ch );
      return;
    }
  }

  if( rch ) {
    if( ( obj = one_object( ch, argument, empty_string, &rch->wearing ) ) ) {
      obj->Look_At( ch );
      return;
    }
  }

  fsend( ch, "Nothing matching '%s' found on %s.",
	 argument, vis );
}


void show_room( char_data* ch, room_data* room, bool brief, bool scan )
{
  if( !ch->pcdata
      || !ch->Can_See( true ) )
    return;

  room_info( ch, room );
   
  if( !brief || !is_set( ch->pcdata->pfile->flags, PLR_BRIEF ) ) {
    room->Look_At( ch );
  }
  
  show_secret( ch );
  show_tracks( ch );

  /* SHOW CONTENTS */

  select( room->contents, ch );

  obj_data *obj;

  if( !has_holylight( ch ) ) {
    //ch->species
    //      || !is_set( ch->pcdata->pfile->flags, PLR_HOLYLIGHT ) ) {
    for( int i = 0; i < room->contents; i++ ) {
      thing_data *thing = room->contents[i];
      if( ( obj = object( thing ) )  
	  && is_set( obj->extra_flags, OFLAG_NOSHOW ) ) {
	room->contents[i]->Select( 0 );
      }
    }
  }

  ch->Select( 0 );
  rehash( ch, room->contents );
  
  bool found = false;
  
  for( int i = 0; i < room->contents; i++ ) {
    thing_data *thing = room->contents[i];
    if( thing->Shown( ) > 0 && thing != ch ) {
      if( !found ) {
        found = true;
        send( ch, "\n\r" );
      }
      show( ch, thing );
    }
  }

  /* SCAN */

  const int level = level_setting( &ch->pcdata->pfile->settings,
			     SET_AUTOSCAN );
  
  if( scan
      && level != 0
      && ( level == 3 || !is_set( room->room_flags, RFLAG_NO_AUTOSCAN ) ) )
    show_scan( ch, true, false, false );       
}


void show( char_data* ch, thing_data* thing )
{
  if( const char *desc = thing->Show_To( ch ) ) {
    fsend( ch, "%s", desc );
  }
}


/*
 *   ROOM INFO BOX
 */


static char *room_flags( room_data* room )
{
  char *tmp = static_string( );
  int t;
  
  if( is_set( room->room_flags, RFLAG_SAFE ) )
    t = snprintf( tmp, THREE_LINES, "safe" );
  else {
    *tmp = '\0';
    t = 0;
  }
  
  if( is_set( room->room_flags, RFLAG_NO_MOB ) ) 
    t += sprintf( tmp+t, "%s%s",
		  t == 0 ? "" : ", ", "no.mob" );
  
  if( is_set( room->room_flags, RFLAG_NO_MAGIC ) ) 
    t += sprintf( tmp+t, "%s%s",
		  t == 0 ? "" : ", ", "no.magic" );
  
  if( room->is_indoors( ) ) 
    t += sprintf( tmp+t, "%s%s",
		  t == 0 ? "" : ", ", "inside" );
  
  if( is_set( room->room_flags, RFLAG_LIT ) ) 
    t += sprintf( tmp+t, "%s%s",
		  t == 0 ? "" : ", ", "lit" );
  
  if( is_set( room->room_flags, RFLAG_SHOP ) ) 
    t += sprintf( tmp+t, "%s%s",
		  t == 0 ? "" : ", ", "shop" );
  
  if( t == 0 )
    return "--";
  
  return tmp;
}


void room_info( char_data* ch, room_data* room )
{
  const bool can_see  = room->Seen( ch );
  const int     term  = ch->pcdata->terminal;

  const char *name = can_see ? room->name : "DARKNESS";
  const int detail = level_setting( &ch->pcdata->pfile->settings, SET_ROOM_INFO );

  if( detail < 2 ) {
    if( is_apprentice( ch ) ) {
      send_color( ch, COLOR_ROOM_NAME, "#%d : %s",
		  room->vnum, name );
    } else {
      send_color( ch, COLOR_ROOM_NAME, "%s", name );
    }
    send( ch, "\n\r" );
    if( detail == 1 && can_see )
      autoexit( ch );
    send( ch, "\n\r" );
    return;
  }
  
  char *tmp = static_string( );

  if( term != TERM_DUMB ) {
    snprintf( tmp, THREE_LINES, "%%%ds%s%%s%s\n\r",
	      40-strlen( name )/2,
	      color_code( ch, COLOR_ROOM_NAME ),
   	      normal( ch ) );
    send( ch, tmp, "", name );
  } else {
    send_centered( ch, name );
  }
  
  send( ch, scroll_line[2] );
  
  send( ch, "|   Lighting: %-15s Time: %-16s  Terrain: %-13s |\n\r",
	light_name( room->Light() ),
	room->sky_state( ),
	terrain_table[ room->sector_type ].name );
  
  const int i = exits_prompt( tmp, ch );
  add_spaces( tmp, 12-i );
  
  send( ch, "|      Exits: %s    Moon: %-15s Room Size: %-13s |\n\r",
	tmp,
	room->moon_state(),
	size_name[room->size] );

  if( room->is_indoors( ) || is_submerged( 0, room ) ) {
    send( ch, "|    Weather: %-62s |\n\r",
	  weather_word( room->temperature( ), room->humidity( ) ) );
  } else {
    snprintf( tmp, THREE_LINES, "%s; %s; %s",
	      weather_word( room->temperature( ), room->humidity( ) ),
	      cloud_word( room->clouds( ) ),
	      wind_word( room->wind_speed( ), room->wind_angle( ) ) );
    send( ch, "|    Weather: %-62s |\n\r", tmp );
  }

  if( is_apprentice( ch ) )
    send( ch, "|       Vnum: %-14d Flags: %-40s |\n\r",
	  room->vnum, room_flags( room ) );
  
  send( ch, scroll_line[2] );
}
