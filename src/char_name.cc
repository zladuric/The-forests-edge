#include <ctype.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "define.h"
#include "struct.h"


//bool use_color = false;


const char *fake_name( bool plural )
{
  const int index = number_range( 0, table_max[ TABLE_HALLUCINATE ]-1 );

  return plural
    ? hallucinate_table[ index ].plural
    : hallucinate_table[ index ].name;
}


/* 
 *   INCOGNITO
 */


bool is_incognito( char_data* victim, char_data* ch )
{
  if( !victim )
    return true;

  if( !victim->pcdata )
    return is_incognito( victim->leader, ch );
   
  return is_incognito( victim->pcdata->pfile, ch );
}


bool is_incognito( pfile_data *pfile, char_data *ch )
{
  /*
    ch->pcdata->pfile could be zero if it's a socket doing a who-list lookup.
  */

  if( ch->pcdata->pfile ) {
    if( pfile == ch->pcdata->pfile
	|| ch->Level() >= LEVEL_APPRENTICE )
      return false;
  }

  const int level = level_setting( &pfile->settings, SET_INCOGNITO );

  if( level == 0 )
    return false;

  if( level == 3 )
    return true; 

  if( ch->pcdata->pfile
      && pfile->clan
      && pfile->clan == ch->pcdata->pfile->clan )
    return false;

  if( ch->pcdata && level == 1 ) {
    return !pfile->Befriended( ch );
    /*
    if( player_data *pl = find_player( pfile ) ) {
      return !pl->Befriended( ch );
    }
    link_data *link = new link_data;
    link->connected = CON_PLAYING;
    if( !load_char( link, pfile->name, PLAYER_DIR ) ) {
      bug( "Is_incognito: error reading player file. (%s)", pfile->name );
      return false;
    }
    bool incog = !link->player->Befriended( ch );
    link->player->Extract( );
    delete link;
    return incog;
    */
  }

  return true;
}


/*
 *   HE/SHE/IT
 */


static const char *const him_her [] = {
  "it",  "him", "her", "him/her"
};


const char *char_data :: He_She( char_data* to ) const
{
  const char *const string [] = { "it",  "he", "she", "he/she" };
  
  return( !to || Seen( to )
	  ? string[sex] : "they" );
}


const char *char_data :: Him_Her( char_data* to ) const
{ 
  return( !to || Seen( to )
	  ? him_her[sex] : "them" );
}


const char *Pfile_Data :: Him_Her( ) const
{
  return him_her[sex];
}


const char *char_data :: His_Her( char_data* to ) const
{
  const char *const string [] = { "its", "his", "her", "his/her" };
  
  return( !to || Seen( to )
	  ? string[sex] : "their" );
}


/*
 *   ADJECTIVES
 */


char *adjectives( const char_data* ch, const char_data* victim )
{
  char *tmp = static_string( );
  char *t = tmp;

  *tmp = '\0';
  
  // Update look_same() if you change these!

  if( victim->is_affected( AFF_INVISIBLE ) ) {
    t += sprintf( t, "invisible" );
  }

  {
    bool found = false;
    
    if( is_lawful( victim ) && ch->detects_law( ) ) {
      t += sprintf( t, "%slawful",
		    *tmp ? ", " : "" );
      found = true;
    } else if( is_chaotic( victim ) && ch->detects_chaos( ) ) {
      t += sprintf( t, "%schaotic",
		    *tmp ? ", " : "" );
      found = true;
    }
    
    if( is_evil( victim ) && ch->detects_evil( ) ) {
      t += sprintf( t, "%s%sevil",
		    *tmp && !found ? ", " : "",
		    found ? " " : "" );
    } else if( is_good( victim ) && ch->detects_good( ) ) {
      t += sprintf( t, "%s%sgood",
		    *tmp && !found ? ", " : "",
		    found ? " " : "" );
    }
  }
  
  if( victim->is_affected( AFF_ION_SHIELD )
      && ( !victim->species
	   || !victim->species->is_affected( AFF_ION_SHIELD ) ) )
    t += sprintf( t, "%sspark-shielded",
		  *tmp ? ", " : "" );

  if( victim->is_affected( AFF_ICE_SHIELD )
      && ( !victim->species
	   || !victim->species->is_affected( AFF_ICE_SHIELD ) ) )
    t += sprintf( t, "%sice-shielded",
		  *tmp ? ", " : "" );

  if( victim->is_affected( AFF_FIRE_SHIELD )
      && ( !victim->species
	   || !victim->species->is_affected( AFF_FIRE_SHIELD ) ) )
    t += sprintf( t, "%sfire-shrouded",
		  *tmp ? ", " : "" );
  
  if( victim->is_affected( AFF_THORN_SHIELD )
      && ( !victim->species
	   || !victim->species->is_affected( AFF_THORN_SHIELD ) ) )
    t += sprintf( t, "%sthorn-cloaked",
		  *tmp ? ", " : "" );

  if( victim->is_affected( AFF_DISPLACE )
      && ( !victim->species
	   || !victim->species->is_affected( AFF_DISPLACE ) ) )
    t += sprintf( t, "%sdisplaced",
		  *tmp ? ", " : "" );
  
  if( *tmp ) {
    t += sprintf( t, " " );
  }
  
  return tmp; 
}


/* 
 *   KEYWORDS
 */


const char *char_data :: Keywords( char_data* ch )
{
  if( ch == this ) { 
    char *tmp = static_string( );
    snprintf( tmp, THREE_LINES, "self %s", Seen_Name( ch ) );
    return tmp;
  }
  
  if( !in_character ) {
    if( descr->name != empty_string ) {
      return descr->name;
    }
  }
  
  if( ch && ch->is_affected( AFF_HALLUCINATE ) ) {
    return Long_Name( ch, 1, false );
  }
 
  char *tmp = static_string( );
  snprintf( tmp, THREE_LINES, "%s %s",
	    Long_Name( ch, 1, false ),
	    separate( descr->keywords, ch ? known_by.includes( ch ) : true ) );
  
  return tmp;
}


/*
 *   NAME
 */


const char *species_data :: Name( bool known, bool is_plural, bool brief ) const
{
  const char *adj = separate( is_plural ? descr->adj_p : descr->adj_s, known );
  const char *app = separate( is_plural ? descr->plural : descr->singular, known );

  bool switch_adj = false;
  bool switch_app = false;

  if( *adj == '!' ) {
    ++adj;
    switch_adj = true;
  }

  if( *app == '!' ) {
    ++app;
    switch_app = !switch_adj;
  }

  const char *article_adj = empty_string;
  const char *article_app = empty_string;

  if( !is_plural ) {
    if( is_set( act_flags, ACT_USE_THE ) ) {
      article_adj = article_app = "the ";
    } else {
      if( *adj ) {
	if( isvowel( *adj ) != switch_adj ) {
	  article_adj = "an ";
	} else {
	  article_adj = "a ";
	}
      }
      if( *app ) {
	if( isvowel( *app ) != switch_app ) {
	  article_app = "an ";
	} else {
	  article_app = "a ";
	}
      }
    }
  }

  char *tmp = static_string( );

  if( *descr->name && !is_plural ) {
    if( *app && !brief ) {
      if( *adj ) {
	snprintf( tmp, THREE_LINES, "%s, %s%s %s",
		  descr->name,
		  article_adj,
		  adj, app );
      } else {
	snprintf( tmp, THREE_LINES, "%s, %s%s",
		  descr->name,
		  article_app,
		  app );
      }
    } else {
      snprintf( tmp, THREE_LINES, "%s",
		descr->name );
    }
    
  } else {
    if( *adj && !brief ) {
      snprintf( tmp, THREE_LINES, "%s%s %s",
		article_adj,
		adj, app );
    } else {
      snprintf( tmp, THREE_LINES, "%s%s",
		article_app,
		app );
    }
  }
  
  return tmp;
}


const char* char_data :: real_name( )
{
  if( link )
    return link->player->descr->name;

  return( species ? descr->singular : descr->name );
}


const char* char_data :: Name( const char_data* ch, int num, bool brief ) const
{
  if( !Seen( ch ) )
    return "someone";

  return Seen_Name( ch, num, brief );
}


const char *char_data :: The_Name( const char_data *ch, int num ) const
{
  if( !Seen( ch ) )
    return "someone";

  if( !species
      || ( ch
	   && ch != this
	   && in_character
	   && ch->is_affected( AFF_HALLUCINATE ) ) ) {
    return Seen_Name( ch, num, true );
  }
  
  const bool known = !ch || ch->knows( this );
  const mob_data *npc = (const mob_data*) this;
  
  if( known
      && npc->pet_name != empty_string )
    return npc->pet_name;

  if( num == 1
      && known
      && descr->name != empty_string )
    return descr->name;

  const char *name = Seen_Name( ch, num, true );

  char *tmp = static_string( );
  
  snprintf( tmp, THREE_LINES, "the %s", name );

  return tmp;
}


const char *char_data :: Seen_Name( const char_data* ch, int num, bool brief ) const
{
  char *tmp = static_string( );

  if( ch
      && ch != this
      && in_character
      && ch->is_affected( AFF_HALLUCINATE ) ) {
    if( num > 1 ) {
      snprintf( tmp, THREE_LINES, "%s %s",
		number_word( num ),
		fake_name( true ) );
      return tmp;
    } else {
      return fake_name( false );
    }
  }
  
  if( !species ) {
    if( !ch
	|| !in_character
	|| brief
	|| ch->Recognizes( this ) )
      return descr->name;
    const char *singular = descr->singular;
    bool reverse = false;
    if( *singular == '!' ) {
      ++singular;
      reverse = true;
    }
    const char *article = ( isvowel( *singular ) == reverse ) ? "a": "an";
    snprintf( tmp, THREE_LINES, "%s %s", article, singular );
    return tmp;
  }
  
  // Leader knows pets by pet_name.
  // Pets of same leader know each other by pet_name.
  /*
  if( ch
      && ((mob_data*)this)->pet_name != empty_string
      && ( leader == ch
	   || ( is_set( ch->status, STAT_PET )
		&& leader == ch->leader ) ) )
    return ((mob_data*)this)->pet_name;
  */

  // Switched mobs use owner's known_by.
  const bool known = !ch || ch->knows( this );
  const mob_data *npc = (const mob_data*) this;

  if( known
      && npc->pet_name != empty_string )
    return npc->pet_name;

  /*
    ? ( ch->species
	? ( ch->link && ch->link->player
	    ? known_by.includes( ch->link->player )
	    : false )
	: known_by.includes( const_cast<char_data *>( ch ) ) )
    : true;
  */
  //  const bool known = ( ch ? known_by.includes( const_cast<char_data *>( ch ) ) : true );

  if( num > 1 ) {
    const char *plural = descr->plural;
    if( *plural == '!' )
      ++plural;
    snprintf( tmp, THREE_LINES, "%s %s",
	      number_word( num ),
	      separate( plural, known ) );
    return tmp;
  }

  if( known && descr->name != empty_string )
    return descr->name;

  const char *singular = separate( descr->singular, known );

  bool reverse = false;

  if( *singular == '!' ) {
    ++singular;
    reverse = true;
  }

  if( brief )
    return singular;

  if( is_set( species->act_flags, ACT_USE_THE ) ) {
    snprintf( tmp, THREE_LINES, "the %s", singular );
  } else {
    const char *article = ( isvowel( *singular ) == reverse ) ? "a": "an";
    snprintf( tmp, THREE_LINES, "%s %s", article, singular );
  }

  return tmp;
}


/*
 *   LONG NAME
 */


const char *char_data :: Long_Name( const char_data* ch, int num, bool colored ) const
{
  char *tmp = static_string( );

  if( ch
      && ch != this
      && in_character
      && ch->is_affected( AFF_HALLUCINATE ) ) {
    if( num > 1 ) {
      snprintf( tmp, THREE_LINES, "%s %s",
		number_word( num ),
		fake_name( true ) );
      return tmp;
    } else {
      return fake_name( false );
    }
  }

  char *adj = adjectives( ch, this );

  // Switched mobs use owner's known_by.
  const bool known = !ch || ch->knows( this );
  /*
    ? ( ch->species
	? ( ch->link && ch->link->player
	    ? known_by.includes( ch->link->player )
	    : false )
	: known_by.includes( const_cast<char_data *>( ch ) ) )
    : true;
  */
  //  const bool known = known_by.includes( ch );

  if( num > 1 ) {
    const char *prefix_p = separate( descr->prefix_p, known );
    const char *adj_p = separate( descr->adj_p, known );
    const char *plural = separate( descr->plural, known );

    if( *prefix_p == '!' ) {
      ++prefix_p;
    }

    if( *adj_p == '!' ) {
      ++adj_p;
    }

    if( *plural == '!' ) {
      ++plural;
    }

    snprintf( tmp, THREE_LINES, "%s%s%s %s%s%s%s%s%s", 
	      prefix_p,
	      prefix_p == empty_string ? "" : " ",
	      number_word( num ), adj,
	      adj_p,
	      adj_p == empty_string ? "" : " ",
	      colored ? color_code( ch, color ) : "",
	      plural,
	      colored ? normal( ch ) : "" );
    return tmp;
  }

  const char *prefix_s = separate( descr->prefix_s, known );
  const char *adj_s = separate( descr->adj_s, known );
  const char *singular = separate( descr->singular, known );

  if( *prefix_s == '!' ) {
    ++prefix_s;
  }
  
  bool reverse = false;

  if( *adj_s == '!' ) {
    ++adj_s;
    if( !*adj ) {
      reverse = true;
    }
  }
  
  if( *adj_s ) {
    sprintf( adj+strlen( adj ), "%s ", adj_s );
  }

  if( *singular == '!' ) {
    ++singular;
    if( !*adj ) {
      reverse = true;
    }
  }
  
  const char *article;
  if( species && is_set( species->act_flags, ACT_USE_THE ) ) {
    article = "the";
  } else {
    article = isvowel( !*adj ? *singular : *adj ) == reverse ? "a" : "an";
  }

  if( !species ) {
    if( ch->Recognizes( this ) ) {
      if( !is_set( ch->pcdata->message, MSG_LONG_NAMES ) )
        return descr->name;
      snprintf( tmp, THREE_LINES, "%s, %s %s%s,",
		descr->name, article,
		adj, singular );
    } else {
      snprintf( tmp, THREE_LINES, "%s %s%s",
		article, adj, singular );
    }
    return tmp;
  }

  char *t = tmp;

  t += snprintf( t, THREE_LINES, "%s%s", 
		 prefix_s,
		 prefix_s == empty_string ? "" : " " );
  
  const char *name = 0;
  const mob_data *npc = (const mob_data*) this;

  if( known ) {
    if( npc->pet_name != empty_string )
      name = npc->pet_name;
    else if( descr->name != empty_string )
      name = descr->name;
  }

  /*
  if( ( leader == ch
	|| ( is_set( ch->status, STAT_PET )
	     && leader == ch->leader )
	|| known )
      && ((mob_data*)this)->pet_name != empty_string ) {
    name = ((mob_data*)this)->pet_name;
  } else if( known
	     && descr->name != empty_string ) {
    name = descr->name;
  }
  */

  if( name ) {
    if( *singular
	&& ( !ch->pcdata || is_set( ch->pcdata->message, MSG_LONG_NAMES ) ) ) {
      t += snprintf( t, THREE_LINES, "%s%s%s, %s %s%s,",
		     colored ? color_code( ch, color ) : "",
		     name,
		     colored ? normal( ch ) : "",
		     article, adj,
		     singular );
    } else {
      t += snprintf( t, THREE_LINES, "%s%s%s",
		     colored ? color_code( ch, color ) : "",
		     name,
		     colored ? normal( ch ) : "" );
    }
  } else {
    t += snprintf( t, THREE_LINES, "%s %s%s%s%s",
		   article, adj,
		   colored ? color_code( ch, color ) : "",
		   singular,
		   colored ? normal( ch ) : "" );
  }
  
  return tmp;
}


const char *char_data :: Show_To( char_data* ch )
{
  const int num = Shown( );
  char *tmp  = static_string( );
  char *buf = static_string( );
  const char *string  = empty_string;

  //  use_color = true;

  if( is_set( status, STAT_CAMOUFLAGED )
      || is_set( status, STAT_HIDING ) ) {
    seen_by += ch;
  }

  if( rider && rider != ch && rider->Seen( ch ) ) {
    return 0;
  }

  if( mount )
    snprintf( tmp, THREE_LINES, "%s mounted on %s %s here.",
	      Long_Name( ch, num ), mount->Long_Name( ch, num ),
	      num == 1 ? "is" : "are" );
 
  else if( rider == ch )
    snprintf( tmp, THREE_LINES, "%s which you are riding is %s here.",
	      Long_Name( ch ), position_name( ) );
  
  else if( position > POS_RESTING
	   && is_affected( AFF_PARALYSIS ) )
    string = "here, staying very still."; 
  
  else if( position > POS_RESTING
	   && is_affected( AFF_ENTANGLED ) )
    string = "struggling to escape a web."; 
  
  else if( position > POS_RESTING
	   && is_affected( AFF_CHOKING ) )
    string = "choking to death."; 
  
  else if( position > POS_RESTING
	   && ( !species || is_set( status, STAT_PET ) )
	   && is_set( status, STAT_HIDING ) )
    string = "trying to hide in the shadows.";
  
  else if( position > POS_RESTING
	   && ( !species || is_set( status, STAT_PET ) )
	   && is_set( status, STAT_CAMOUFLAGED ) )
    string = "trying to blend into the surroundings.";
  
  else if( position > POS_RESTING
	   && ( !species || is_set( status, STAT_PET ) )
	   && is_set( status, STAT_SNEAKING ) )
    string = "sneaking around.";
  
  else if( position == POS_RESTING && pos_obj )
    snprintf( tmp, THREE_LINES, "%s %s resting on %s.",
	      Long_Name( ch, num ),
	      num == 1 ? "is" : "are",
	      pos_obj->Seen_Name( ch ) );
  
  else if( position == POS_SLEEPING && pos_obj )
    snprintf( tmp, THREE_LINES, "%s %s sleeping on %s.",
	      Long_Name( ch, num ),
	      num == 1 ? "is" : "are",
	      pos_obj->Seen_Name( ch ) );
  
  else {
    switch ( position ) {
    case POS_DEAD:       string = "dead.";                break;
    case POS_MORTAL:     string = "mortally wounded.";    break;
    case POS_INCAP:      string = "incapacitated.";       break;
    case POS_STUNNED:    string = "lying here stunned.";  break;
    case POS_SLEEPING:   string = "sleeping here.";       break;
    case POS_MEDITATING: string = "meditating here.";     break;
    case POS_RESTING:    string = "resting here.";        break;
      
    case POS_STANDING:
      if( fighting ) 
	snprintf( tmp, THREE_LINES, "%s %s here, fighting %s%c",
		  Long_Name( ch, num ),
		  num == 1 ? "is" : "are",
		  fighting == ch ? "YOU" : fighting->Name( ch ),
		  fighting == ch ? '!' : '.' );
      
      else if( species
	       && !ch->is_affected( AFF_HALLUCINATE )
	       && !is_set( status, STAT_PET ) )
	snprintf( tmp, THREE_LINES, "%s %s", Long_Name( ch, num ),
		  separate( num == 1 ? descr->long_s : descr->long_p, true ) );

      else {
	snprintf( buf, THREE_LINES, "%s here.",
		  position_name( ch ) );
	string = buf;
      }
    }
  }

  if( string != empty_string )
    snprintf( tmp, THREE_LINES, "%s %s %s", Long_Name( ch, num ),
	      num == 1 ? "is" : "are", string );
  
  //  use_color = false;

  return tmp;
}


/*
 *   LOOK_SAME
 */


bool look_same( char_data *ch, char_data* ch1, char_data* ch2, bool auction )
{
  // auction == scan (brief name)

  if( !ch1->species
      || !ch2->species
      || ch1->species != ch2->species )
    return false;
  
  if( ch ) {
    const mob_data *npc1 = (mob_data*) ch1;
    const bool know1 = ch->knows( ch1 );
    if( know1 && npc1->pet_name != empty_string )
      return false;
    const mob_data *npc2 = (mob_data*) ch2;
    const bool know2 = ch->knows( ch2 );
    if( know2 && npc2->pet_name != empty_string )
      return false;
    /*
    if( ( ch1->leader == ch || !auction && ch1->known_by.includes( ch ) )
	&& ((mob_data*)ch1)->pet_name != empty_string )
      return false;
    if( ( ch2->leader == ch || !auction && ch2->known_by.includes( ch ) )
	&& ((mob_data*)ch2)->pet_name != empty_string )
      return false;
    */
    if( ( *ch1->species->descr->name
	  || *ch1->species->descr->singular == '{'
	  || *ch1->species->descr->prefix_s == '{'
	  || *ch1->species->descr->adj_s == '{'
	  || *ch1->species->descr->plural == '{'
	  || *ch1->species->descr->prefix_p == '{'
	  || *ch1->species->descr->adj_p == '{' )
	&& ( know1 != know2 ) )
      /*
	&& ( ch1->known_by.includes( ch )
	     != ch2->known_by.includes( ch ) ) )
      */
      return false;
  }
  
  // Update adjectives() if you change these!

  if( !auction ) {

    if( ch1->position != ch2->position
	|| ch1->fighting != ch2->fighting )
      return false;
    
    if( ( ch1->is_affected( AFF_FIRE_SHIELD )
	  != ch2->is_affected( AFF_FIRE_SHIELD ) )
	|| ( ch1->is_affected( AFF_ION_SHIELD )
	     != ch2->is_affected( AFF_ION_SHIELD ) )
	|| ( ch1->is_affected( AFF_ICE_SHIELD )
	     != ch2->is_affected( AFF_ICE_SHIELD ) )
	|| ( ch1->is_affected( AFF_DISPLACE )
	     != ch2->is_affected( AFF_DISPLACE ) )
	|| ( ch1->is_affected( AFF_INVISIBLE )
	     != ch2->is_affected( AFF_INVISIBLE ) ) )
      return false;
    
    if( ch ) {
      if( ch->detects_evil( )
	  && is_evil( ch1 ) != is_evil( ch2 ) )
	return false;
      
      if( ch->detects_good( )
	  && is_good( ch1 ) != is_good( ch2 ) )
	return false;
      
      if( ch->detects_law( )
	  && is_lawful( ch1 ) != is_lawful( ch2 ) )
	return false;
      
      if( ch->detects_chaos( )
	  && is_chaotic( ch1 ) != is_chaotic( ch2 ) )
	return false;
    }      
    
    if( is_set( ch1->status, STAT_PET ) != is_set( ch2->status, STAT_PET ) ) {
      return false;
    }
    
    if( ch1->mount && !ch2->mount
	|| ch2->mount && !ch1->mount ) {
      return false;
    }
    
    if( ch1->rider && !ch2->rider
	|| ch2->rider && !ch1->rider ) {
      return false;
    }
    
    if( ch1->rider
	&& ch2->rider
	&& !look_same( ch, ch1->rider, ch2->rider ) ) {
      return false;
    }
    
    if( ch1->color != ch2->color )
      return false;

    if( ( ch1->position > POS_RESTING && ch1->is_affected( AFF_PARALYSIS ) )
	!= ( ch2->position > POS_RESTING && ch2->is_affected( AFF_PARALYSIS ) ) )
      return false;
    
    if( ( ch1->position > POS_RESTING && ch1->is_affected( AFF_ENTANGLED ) )
	!= ( ch2->position > POS_RESTING && ch2->is_affected( AFF_ENTANGLED ) ) )
      return false;
    
    if( ( ch1->position > POS_RESTING && ch1->is_affected( AFF_CHOKING ) )
	!= ( ch2->position > POS_RESTING && ch2->is_affected( AFF_CHOKING ) ) )
      return false;
    
    if( ( ch1->position > POS_RESTING && is_set( ch1->status, STAT_PET ) && is_set( ch1->status, STAT_HIDING ) )
	!= ( ch2->position > POS_RESTING && is_set( ch2->status, STAT_PET ) && is_set( ch2->status, STAT_HIDING ) ) )
      return false;
    
    if( ( ch1->position > POS_RESTING && is_set( ch1->status, STAT_PET ) && is_set( ch1->status, STAT_SNEAKING ) )
	!= ( ch2->position > POS_RESTING && is_set( ch2->status, STAT_PET ) && is_set( ch2->status, STAT_SNEAKING ) ) )
      return false;

    if( ch1->pos_obj != ch2->pos_obj )
      return false;
  }

  if( ch1->descr != ch2->descr ) {
    if( strcmp( ch1->descr->name, ch2->descr->name )
	|| strcmp( ch1->descr->keywords, ch2->descr->keywords )
	|| strcmp( ch1->descr->singular, ch2->descr->singular )
	|| strcmp( ch1->descr->long_s, ch2->descr->long_s )
	|| strcmp( ch1->descr->adj_s, ch2->descr->adj_s )
	|| strcmp( ch1->descr->prefix_s, ch2->descr->prefix_s )
	|| strcmp( ch1->descr->plural, ch2->descr->plural )
	|| strcmp( ch1->descr->long_p, ch2->descr->long_p )
	|| strcmp( ch1->descr->adj_p, ch2->descr->adj_p )
	|| strcmp( ch1->descr->prefix_p, ch2->descr->prefix_p )
	|| strcmp( ch1->descr->complete, ch2->descr->complete )
	)
    return false;
  }

  return true; 
}
