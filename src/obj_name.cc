#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include "define.h"
#include "struct.h"


bool include_liquid = true;
bool include_empty  = true;
bool include_closed = true;


/*
 *   SUPPORT ROUTINES
 */


const char *color_word [] = {
  "grey", "red", "orange", "yellow",
  "green", "cyan", "blue", "indigo", "violet", "purple", "white"
};

const char *gem_quality [] = {
  "random", "none", "uncut",
  "fractured", "chipped", "blemished", "scratched", "flawless"
};


static const char *eaten_name( const obj_data *obj )
{
  int i = -1;

  if( obj->pIndexData->item_type == ITEM_FOOD ) {
    if( obj->value[0] < obj->value[2] ) {
      i = 10*obj->value[0]/obj->value[2];
    }
  } else if( obj->pIndexData->item_type == ITEM_CORPSE ) {
    if( obj->weight < obj->value[2] ) {
      i = 10*obj->weight/obj->value[2];
    }
  } else {
    return 0;
  }

  if( i >= 0 ) {
    switch( i ) {
    case 9:
    case 8:
      return "slightly-nibbled";
    case 7:
    case 6:
    case 5:
      return "partially-eaten";
    case 4:
    case 3:
      return "heavily-gnawed";
    case 2:
    case 1:
    case 0:
      return "mostly-consumed";
    }
  }

  return 0;
}


static void get_obj_adj( char* tmp, const char_data* ch, const obj_data* obj )
{
  *tmp = '\0';
  char *t = tmp;

  /*
  if( is_set( obj->pIndexData->size_flags, SFLAG_RANDOM ) ) {
    for( i = SFLAG_TINY; i < SFLAG_GIANT; i++ ) 
      if( is_set( obj_clss->size_flags, i ) )
        break;
  */

  if( const char *e = eaten_name( obj ) ) {
    t += sprintf( t, e );
  }

  if( ch ) {
    bool found = false;

    if( is_set( obj->extra_flags, OFLAG_LAWFUL )
	&& ch->is_affected( AFF_DETECT_LAW ) ) {
      t += sprintf( t, "%s+lawful",
		    *tmp ? ", " : "" );
      found = true;
    } else if( is_set( obj->extra_flags, OFLAG_CHAOTIC ) 
	&& ch->is_affected( AFF_DETECT_CHAOS ) ) {
      t += sprintf( t, "%s+chaotic",
		    *tmp ? ", " : "" );
      found = true;
    }

    if( is_set( obj->extra_flags, OFLAG_EVIL )
	&& ch->is_affected( AFF_DETECT_EVIL ) ) {
      t += sprintf( t, "%s%cevil",
		    *tmp && !found ? ", " : "",
		    found ? ' ' : '+' );
      found = true;
    } else if( is_set( obj->extra_flags, OFLAG_GOOD ) 
	&& ch->is_affected( AFF_DETECT_GOOD ) ) {
      t += sprintf( t, "%s%cgood",
		    *tmp && !found ? ", " : "",
		    found ? ' ' : '+' );
      found = true;
    }
    
    if( found ) {
      t += sprintf( t, "+" );
    }

    if( ch->is_affected( AFF_DETECT_MAGIC )
	&& is_set( obj->extra_flags, OFLAG_MAGIC ) )
      t += sprintf( t, "%senchanted",
		    *tmp ? ", " : "" );
  }
  
  if( is_set( obj->extra_flags, OFLAG_IS_INVIS )
      && !is_set( obj->pIndexData->extra_flags, OFLAG_IS_INVIS ) )
    t += sprintf( t, "%sinvisible",
		  *tmp ? ", " : "" );
  
  if( is_set( obj->extra_flags, OFLAG_GLOW )
      && !is_set( obj->pIndexData->extra_flags, OFLAG_GLOW ) )
    t += sprintf( t, "%sglowing",
		  *tmp ? ", " : "" );
  
  if( is_set( obj->extra_flags, OFLAG_BURNING )
      && !is_set( obj->pIndexData->extra_flags, OFLAG_BURNING ) )
    t += sprintf( t, "%sburning",
		  *tmp ? ", " : "" );
  
  if( is_set( obj->extra_flags, OFLAG_FLAMING )
      && !is_set( obj->pIndexData->extra_flags, OFLAG_FLAMING ) )
    t += sprintf( t, "%sflaming",
		  *tmp ? ", " : "" );
  
  if( is_set( obj->extra_flags, OFLAG_POISON_COATED )
      && !is_set( obj->pIndexData->extra_flags, OFLAG_POISON_COATED ) )
    t += sprintf( t, "%spoison-coated",
		  *tmp ? ", " : "" );
  
  switch( obj->pIndexData->item_type ) {
  case ITEM_DRINK_CON : 
    if( obj->value[1] == 0 && include_empty ) 
      t += sprintf( t, "%sempty",
		    *tmp ? ", " : "" );
    break;
    
  case ITEM_FOOD :
    if( obj->value[1] >= COOK_RAW && obj->value[1] <= COOK_BURNT ) 
      t += sprintf( t, "%s%s",
		    *tmp ? ", " : "",
		    cook_word[ obj->value[1] + 1 ] );
    break;
    
  case ITEM_CONTAINER :
    if( include_closed ) {
      if( is_set( obj->value[1], CONT_CLOSED ) ) {
	t += sprintf( t, "%sclosed",
		      *tmp ? ", " : "" );
      } else if( include_empty && obj->contents == 0 ) {
	t += sprintf( t, "%sempty",
		      *tmp ? ", " : "" );
      }
    }
    break;
    
  case ITEM_GEM :
    if( obj->value[1] > GEM_NONE && obj->value[1] < MAX_GEM ) {
      t += sprintf( t, "%s%s",
		    *tmp ? ", " : "",
		    gem_quality[ obj->value[1] + 1 ] );
    }
    break;
  }

  int metal = -1;
  if( obj->rust != 0 ) {
    if( ( metal = obj->metal() ) != 0 ) {
      t += sprintf( t, "%s%s",
		    *tmp ? ", " : "",
		    material_table[ metal ].rust[ obj->rust ] );
    }
  }
  
  if( obj->pIndexData->vnum == OBJ_BALL_OF_LIGHT ) 
    t += sprintf( t, "%s%s",
		  *tmp ? ", " : "",
		  color_word[ range( 0, obj->value[0]/3, 10 ) ] );
  
  if( is_set( obj->pIndexData->extra_flags, OFLAG_RANDOM_METAL ) ) {
    if( metal < 0 ) {
      metal = obj->metal();
    }
    if( metal > 0 ) {
      t += sprintf( t, "%s%s",
	       *tmp ? ", " : "",
      	       material_table[metal].name );
    }
  }
}


static void append_liquid( char *& t, const obj_data *obj )
{
  if( !include_liquid )
    return;
  
  if( obj->pIndexData->item_type != ITEM_DRINK_CON
      || ( obj->value[1] != -1 && obj->value[1] <= 0 )
      || obj->value[2] < 0
      || obj->value[2] >= table_max[ TABLE_LIQUID ] )
    return;
  
  if( is_set( obj->extra_flags, OFLAG_KNOWN_LIQUID ) )
    t += sprintf( t, " of %s",
		  liquid_table[obj->value[2]].name );
  else {
    t += sprintf( t, " containing %s",
		  liquid_table[obj->value[2]].color );
  }
}


/*
 *   LOOK_SAME?
 */


bool look_same( char_data *ch, obj_data* obj1, obj_data* obj2, bool auction )
{
  const obj_clss_data *p1 = obj1->pIndexData;
  const obj_clss_data *p2 = obj2->pIndexData;

  if( strcmp( p1->prefix_singular, p2->prefix_singular ) )
    return false;

  const char *rust1 = empty_string;
  if( obj1->rust != 0 ) {
    if( int metal = obj1->metal( ) ) {
      rust1 = material_table[ metal ].rust[ obj1->rust ];
    }
  }

  const char *rust2 = empty_string;
  if( obj2->rust != 0 ) {
    if( int metal = obj2->metal( ) ) {
      rust2 = material_table[ metal ].rust[ obj2->rust ];
    }
  }

  if( strcmp( rust1, rust2 ) )
    return false;

  bool id1 = is_set( obj1->extra_flags, OFLAG_IDENTIFIED ); 
  bool id2 = is_set( obj2->extra_flags, OFLAG_IDENTIFIED );
  
  const char *noun1 = separate( obj1->singular, id1 );
  const char *noun2 = separate( obj2->singular, id2 );

  if( strcasecmp( noun1, noun2 ) )
    return false;

  int type1 = p1->item_type;

  if( id2 ) {
    swap( obj1, obj2 );
    swap( id1, id2 );
  }

  if( id1 ) {
    if( !id2 ) {
      // Only one ID'ed.
      
      if( ( type1 == ITEM_WEAPON
	    || type1 == ITEM_ARMOR )
	  && obj1->value[0] != 0 )
        return false;

      if( strcasecmp( obj1->after, obj2->before ) )
        return false;
    }
    
    // At least one ID'ed.
    if( p1 != p2 )
      return false;
    
    if( id2 ) {
      // Both ID'ed.
    
      if( ( type1 == ITEM_WEAPON
	    || type1 == ITEM_ARMOR )
	  && obj1->value[0] != obj2->value[0] )
	return false;

      if( strcasecmp( obj1->after, obj2->after ) )
	return false;
    }

  } else {
    // Neither ID'ed.
    if( strcasecmp( obj1->before, obj2->before ) ) {
      return false;
    }
  }
  
  if( auction ) {
    if( obj1->condition_name( 0, false ) != obj2->condition_name( 0, false ) )
      return false;
    if( obj1->weight != obj2->weight )
      return false;
    if( is_set( obj1->extra_flags, OFLAG_NOSAVE ) != is_set( obj2->extra_flags, OFLAG_NOSAVE ) )
      return false;
  }

  if( p1->vnum == OBJ_BALL_OF_LIGHT
      && range( 0, obj1->value[0]/3, 10 ) != range( 0, obj2->value[0]/3, 10 ) )
    return false;
  
  if( ch ) {
    if( ch->is_affected( AFF_DETECT_EVIL )
	&& ( is_set( obj1->extra_flags, OFLAG_EVIL )
	     != is_set( obj2->extra_flags, OFLAG_EVIL ) ) )
      return false;
  
    if( ch->is_affected( AFF_DETECT_GOOD )
	&& ( is_set( obj1->extra_flags, OFLAG_GOOD )
	     != is_set( obj2->extra_flags, OFLAG_GOOD ) ) )
      return false;

    if( ch->is_affected( AFF_DETECT_MAGIC )
	&& ( is_set( obj1->extra_flags, OFLAG_MAGIC )
	     != is_set( obj2->extra_flags, OFLAG_MAGIC ) ) )
      return false;
    
    if( type1 == ITEM_CHAIR
	/*|| type1 == ITEM_TABLE*/ ) {
      for( int i = 0; i < obj1->contents; ++i ) {
	if( obj1->contents[i]->Seen( ch ) )
	  return false;
      }
      for( int i = 0; i < obj2->contents; ++i ) {
	if( obj2->contents[i]->Seen( ch ) )
	  return false;
      }
    }
  }
    
  if( is_set( obj1->extra_flags, OFLAG_IS_INVIS )
      != is_set( obj2->extra_flags, OFLAG_IS_INVIS ) )
    return false;
  
  if( is_set( obj1->extra_flags, OFLAG_GLOW )  
      != is_set( obj2->extra_flags, OFLAG_GLOW ) )
    return false;
  
  if( is_set( obj1->extra_flags, OFLAG_BURNING )  
      != is_set( obj2->extra_flags, OFLAG_BURNING ) )
    return false;
  
  if( is_set( obj1->extra_flags, OFLAG_FLAMING )  
      != is_set( obj2->extra_flags, OFLAG_FLAMING ) )
    return false;
  
  if( is_set( obj1->extra_flags, OFLAG_POISON_COATED )  
      != is_set( obj2->extra_flags, OFLAG_POISON_COATED ) )
    return false;
  
  if( is_set( p1->extra_flags, OFLAG_RANDOM_METAL )
      && obj1->materials != obj2->materials )
    return false;
  
  if( type1 == ITEM_CONTAINER ) {
    if( include_closed ) {
      if( is_set( obj1->value[1], CONT_CLOSED ) != is_set( obj2->value[1], CONT_CLOSED ) )
	return false;
      if( !is_set( obj1->value[1], CONT_CLOSED )
	  && include_empty
	  && ( obj1->contents == 0 ) != ( obj2->contents == 0 ) )
	return false;
    }

  } else if( type1 == ITEM_TABLE
      && ( obj1->contents != 0 || obj2->contents != 0 ) ) {
    return false;

  } else if( type1 == ITEM_DRINK_CON ) {
    if( ( obj1->value[1] == 0 ) != ( obj2->value[1] == 0 ) ) {
      if( include_empty || include_liquid )
	return false;
    } else if( obj1->value[1] != 0
	       && include_liquid ) {
      if( obj1->value[2] != obj2->value[2] )
	return false;
      if( is_set( obj1->extra_flags, OFLAG_KNOWN_LIQUID )  
	  != is_set( obj2->extra_flags, OFLAG_KNOWN_LIQUID ) )
	return false;
    }

  } else if( type1 == ITEM_FOOD ) {
    if( obj1->value[1] != obj2->value[1]		// raw/cooked/burnt
	|| eaten_name( obj1 ) != eaten_name( obj2 ) )	// partially-eaten
      return false;
    
  } else if( type1 == ITEM_GEM ) {
    if( obj1->value[1] != obj2->value[1] )
      return false;

  } else if( type1 == ITEM_CORPSE ) {
    if( eaten_name( obj1 ) != eaten_name( obj2 ) )	// partially-eaten
      return false;
  }
  
  if( strcmp( obj1->label, obj2->label ) )
    return false;

  return true;
}


/*
 *   OBJECT CLASS NAME ROUTINES
 */


const char* obj_clss_data :: Keywords( )
{
  return Name( );
}


static bool name_cat( char *& buf, const char *word1, const char *word2 )
{
  switch( *word1 ) {
  case '\0' :
    if( *word2 == '!' ) {
      buf += sprintf( buf, word2+1 );
      return true;
    } else {
      buf += sprintf( buf, word2 );
      return false;
    }
    
  case '+' :
    if( *word2 == '!' ) {
      buf += sprintf( buf, "%s %s", word2+1, word1+1 );
      return true;
    } else {
      buf += sprintf( buf, "%s %s", word2, word1+1 );
      return false;
    }
    
  default :
    if( !*word2 ) {
      if( *word1 == '!' ) {
	buf += sprintf( buf, word1+1 );
	return true;
      } else {
	buf += sprintf( buf, word1 );
	return false;
      }
    } else {
      if( *word1 == '!' ) {
	buf += sprintf( buf, "%s %s", word1+1, word2 );
	return true;
      } else {
	buf += sprintf( buf, "%s %s", word1, word2 );
	return false;
      }
    }
  }
}


const char* obj_clss_data :: Name( int num, bool brief, bool ident ) const
{
  char *tmp = static_string();
  char *t = tmp;
  const bool the = is_set( extra_flags, ident ? OFLAG_THE_AFTER : OFLAG_THE_BEFORE );
  
  if( num == 1 ) {
    if( !brief ) {
      t += snprintf( t, THREE_LINES, the ? "the " : "an " );
    }
  } else {
    t += snprintf( t, THREE_LINES, "%s ", number_word( num ) );
  }

  bool start = true;
  const bool show_article = !brief && num == 1 && !the;
  bool change_article = false;

  if( !brief ) {
    const char *prefix = ( num == 1 ) ? prefix_singular : prefix_plural;
    if( prefix != empty_string ) {
      if( *prefix == '!' ) {
	++prefix;
	change_article = true;
      }
      start = false;
      t += snprintf( t, THREE_LINES, "%s ", prefix );
    }
  }

  const char *name = ( num == 1 ) ? singular : plural;
  const char *mod = ident ? after : before;

  if( name_cat( t, mod, separate( name, ident ) ) && start ) {
    change_article = true;
  }

  //  append_liquid( t, this, ident );

  if( show_article
      && isvowel( tmp[3] ) == change_article ) {
    tmp[1] = 'a';
    return tmp+1;
  }

  return tmp;
}


/*
 *   OBJECT NAME ROUTINES
 */


const char* Obj_Data :: Name( const char_data* ch, int number, bool brief ) const
{
  if( ch && !Seen( ch ) ) {
    return "something";
  }

  return Seen_Name( ch, number, brief );
}


const char* Obj_Data :: Seen_Name( const char_data* ch, int num, bool brief ) const
{
  char *string;
  const char *noun;
  const bool identified = is_set( extra_flags, OFLAG_IDENTIFIED );
  const bool the = is_set( extra_flags, identified ? OFLAG_THE_AFTER : OFLAG_THE_BEFORE );

  char *tmp = static_string( );
  char *t = tmp;

  bool start = true;
  const bool show_article = !brief && num == 1 && !the;
  bool change_article = false;

  if( num == 1 || brief ) {
    if( !brief ) {
      t += snprintf( t, THREE_LINES, the ? "the " : "an " );
      const char *prefix = pIndexData->prefix_singular;
      if( prefix != empty_string ) {
	if( *prefix == '!' ) {
	  ++prefix;
	  change_article = true;
	}
	start = false;
        t += snprintf( t, THREE_LINES, "%s ", prefix );
      }
    }
    noun = separate( singular, identified );

  } else {
    t += snprintf( t, THREE_LINES, "%s ", number_word( num, ch ) );
    const char *prefix = pIndexData->prefix_plural;
    if( prefix != empty_string ) {
      if( *prefix == '!' ) {
	++prefix;
	change_article = true;
      }
      start = false;
      t += snprintf( t, THREE_LINES, "%s ", prefix );
    }
    noun = separate( plural, identified );
  }

  char adj [ TWO_LINES ];
  get_obj_adj( adj, ch, this );  
  if( *adj ) {
    start = false;
  }
  t += snprintf( t, THREE_LINES, "%s%s", adj, !*adj ? "" : " " );

  char plus [ 6 ]; 
  *plus = '\0';

  if( identified ) {
    string = after;
    if( ( pIndexData->item_type == ITEM_WEAPON
	  || pIndexData->item_type == ITEM_ARMOR ) && value[0] != 0 )
      snprintf( plus, 6, " %+d", value[0] );
  } else {
    string = before;
  }
  
  if( name_cat( t, string, noun ) && start ) {
    change_article = true;
  }

  t += snprintf( t, THREE_LINES, plus );
  append_liquid( t, this );

  if( label != empty_string ) {
    t += snprintf( t, THREE_LINES, " labeled %s", label );
  }

  if( brief
      && num != 1 ) {
    t += snprintf( t, THREE_LINES, " (x%d)", num );
  }

  if( show_article
      && ( isvowel( tmp[3] )
	   || ( tmp[3] == '+' && isvowel( tmp[4] ) ) ) == change_article ) {
    tmp[1] = 'a';
    return tmp+1;
  }

  return tmp;
}


const char *Obj_Data :: Keywords( char_data* ch )
{
  switch( pIndexData->item_type ) {
  case ITEM_WEAPON:
  case ITEM_ARMOR:
  case ITEM_FOOD:
  case ITEM_REAGENT:
  case ITEM_INSTRUMENT:
    {
      char *tmp = static_string( );
      snprintf( tmp, THREE_LINES, "%s %s %s", Seen_Name( ch ),
		condition_name( ch ), item_type_name[ pIndexData->item_type ] );
      return tmp;
    }
  default:
    return Seen_Name( ch );
  }
}


const char* Obj_Data :: Show_To( char_data* ch )
{ 
  int num = Shown( );
  char *tmp = static_string( );
  
  if( is_set( extra_flags, OFLAG_NOSHOW ) ) {
    snprintf( tmp, THREE_LINES, "%sno-show:%s %s.",
	      color_code( ch, COLOR_MILD ),
	      color_code( ch, COLOR_DEFAULT ),
	      Seen_Name( ch, num ) );
    return tmp;
  }

  if( is_set( extra_flags, OFLAG_SECRET ) ) {
    snprintf( tmp, THREE_LINES, "%ssecret:%s %s.",
	      color_code( ch, COLOR_MILD ),
	      color_code( ch, COLOR_DEFAULT ),
	      Seen_Name( ch, num ) );
    return tmp;
  }

  if( pIndexData->item_type == ITEM_CHAIR ) {
    for( int i = 0; i < contents; ++i ) {
      if( contents[i]->Seen( ch ) ) {
	return 0;
      }
    }
  }
  
  snprintf( tmp, THREE_LINES, "%s %s", Seen_Name( ch, num ),
	    num == 1 ? ( !*pIndexData->long_s ?
			 "lies here." : pIndexData->long_s )  
	    : ( !*pIndexData->long_p ?
		"lie here." : pIndexData->long_p ) );
  
  if( pIndexData->item_type == ITEM_TABLE ) {
    rehash( ch, contents );
    if( !none_shown( contents ) ) {
      obj_data *obj = object( one_shown( contents ) );
      if( obj ) {
	int l = strlen(tmp);
	snprintf( tmp+l, THREE_LINES-l, " %s %s on the %s.",
		  obj->Seen_Name( ch, obj->Shown( ) ),
		  obj->Shown( ) == 1 ? "is" : "are",
		  Seen_Name( ch, 1, true ) );
	*(tmp+l+1) = toupper( *(tmp+l+1) );
      } else {
	snprintf( tmp+strlen( tmp ), THREE_LINES, " Something is on the %s.",
		  Seen_Name( ch, 1, true ) );
      }
    }
  }

  return tmp;
}


/*
 *   LABEL ROUTINE
 */


static bool valid_label( char_data* ch, const char* label )
{
  int i;
  
  for( i = 0; label[i]; ++i ) {
    if( !isalpha( label[i] ) && label[i] != ' ' ) {
      send( ch, "Labels may only contain letters and spaces.\n\r" );
      return false;
    }
  }
  
  if( i >= 15 ) {
    send( ch, "Labels must be less than 15 characters.\n\r" );
    return false;
  } 
  
  return true;
}


void do_label( char_data* ch, const char *argument )
{
  if( is_mob( ch ) )
    return;

  char        arg  [ MAX_INPUT_LENGTH ];
  obj_data*   obj;
  char*     label;

  const bool remove = !contains_word( argument, "as", arg );

  if( !*argument || ( !remove && !*arg ) ) {
    send( ch, "Label what item and as what?\n\r" );
    return;
  }

  if( !( obj = one_object( ch, remove ? argument : arg,
			   "label", &ch->contents ) ) ) 
    return;
  
  if( obj->pIndexData->item_type != ITEM_CONTAINER ) {
    send( ch, "You may only label containers.\n\r" );
    return;
  }
  
  if( !obj->Belongs( ch ) ) {
    fsend( ch, "You don't own %s.", obj );
    return;
  }

  if( remove ) {
    if( obj->label == empty_string ) {
      fsend( ch, "%s isn't labeled.", obj );
      return;
    }
    const bool move = ( obj->Number( ) > 1 );
    if( move ) {
      obj = (obj_data *) obj->From( 1, true );
    }
    free_string( obj->label, MEM_OBJECT );
    obj->label = empty_string;
    if( move )
      obj->To( );
    fsend( ch, "You remove the label from %s.", obj );
    return;
  }

  if( !valid_label( ch, argument ) )
    return;
  
  if( ( label = obj->label ) != empty_string ) {
    obj->label = empty_string;
    fsend( ch, "%s is already labeled as \"%s\".", obj, label );
    obj->label = label;
    return;
  }

  const bool move = ( obj->Number( ) > 1 );
  if( move ) {
    obj = (obj_data *) obj->From( 1, true );
  }
 
  if( obj->label == empty_string ) {
    fsend( ch, "You label %s as \"%s\".", obj, argument );
  } else {
    free_string( obj->label, MEM_OBJECT );
    obj->label = empty_string;
    fsend( ch,
	   "You remove the old label from %s and replace it with \"%s\".", 
	   obj, argument );
  }
  
  obj->label = alloc_string( argument, MEM_OBJECT );

  if( move )
    obj->To( );
}
