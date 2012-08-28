#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <limits.h>
#include "define.h"
#include "struct.h"


const char *layer_name [ MAX_LAYER+1 ] = {
  "any",
  "bottom", "under",
  "base", "over", "top"
};

const char* oflag_name [ MAX_OFLAG ] = {
  "glowing",
  "humming",
  "lawful",
  "chaotic",
  "evil",
  "invisible",
  "magic",
  "no_drop",
  "sanctified",
  "flaming",
  "backstab",
  "no_disarm",
  "no_remove",
  "air_rise",
  "divine_symbol",
  "no_major",
  "no_show",
  "no_sacrifice",
  "water_proof",
  "air_fall",
  "no_sell",
  "no_junk",
  "identified",
  "rust_proof",
  "water_float",
  "water_sink",
  "no_save",
  "burning",
  "additive",
  "good",
  "the_before",
  "replicate",
  "known_liquid",
  "poison_coated",
  "no_auction",
  "no_enchant",
  "copied",
  "random_metal",
  "covers",
  "two_hands",
  "one_owner",
  "no_steal",
  "pass_through",
  "no_reset",
  "secret",
  "tool_dig",
  "tool_mine",
  "the_after"
};

const char* oflag_ident [ MAX_OFLAG ] = {
  "Is glowing",
  "Is making noise",
  "Is lawful",
  "Is chaotic",
  "Is evil",
  "Is invisible",
  "Is magical",
  "Cannot be dropped",
  "Has been sanctified",
  "Is flaming",
  "Useful for backstabbing",
  "Cannot be disarmed",
  "Cannot be removed",
  "",
  "Is a divine symbol",
  "Cannot be enchanted with major enchant",
  "",
  "Cannot be sacrificed",
  "Is water-proof",
  "",
  "Cannot be sold",
  "Cannot be junked",
  "",
  "Is rust-proof",
  "",
  "",
  "Does not save",
  "Is burning",
  "Effects are additive",
  "Is good",
  "",
  "",
  "",
  "Is coated with poison",
  "Cannot be auctioned",
  "Cannot be enchanted with minor enchant",
  "Has been replicated",
  "",
  "Conceals other equipment",
  "Requires two hands",
  "Is specific to a single owner",
  "Cannot be stolen",
  "",
  "",
  "",
  "Can be used for digging",
  "Can be used for mining",
  ""
};

const char* restriction_flags [ MAX_RESTRICTION ] = {
  "bladed",
  "no_hide", "no_sneak", "dishonorable" };

const char* restrict_ident [ MAX_RESTRICTION ] = {
  "bladed",
  "can't hide",
  "can't sneak",
  "dishonorable"
};

const char* anti_flags [ MAX_ANTI ] = {
  "anti-human",
  "anti-elf", "anti-gnome", "anti-dwarf", "anti-halfling",
  "anti-ent", "anti-centaur", "anti-lizardman",
  "anti-ogre", "anti-troll", "anti-orc", "anti-goblin", "anti-vyan",
  "?unused",
  "anti-mage", "anti-cleric", "anti-thief", "anti-warrior",
  "anti-paladin", "anti-ranger", "anti-druid", "anti-monk", "anti-bard",
  "?unused",
  "anti-male", "anti-female",
  "anti-good", "only-good-evil", "anti-evil",
  "anti-lawful", "only-law-chaos", "anti-chaotic" };
  
const char* size_flags [ MAX_SFLAG ] = {
  "custom_fit", "size_specific",
  "race_specific", "random_size",
  "tiny", "small", "medium", "large", "giant",
  "human", "elf", "gnome", "dwarf", "halfling", "ent", "centaur",
  "lizardman", "ogre", "troll", "orc", "goblin", "vyan" };

static const char *const no_permission =
"You don't have permission to alter that object.\n\r";


static void  oedit_replace  ( char_data*, const char * );


/*
 *   PERMISSION ROUTINE
 */


bool char_data :: can_edit( obj_clss_data* obj_clss, bool msg ) const
{
  if( obj_clss->level > get_trust( this ) ) {
    if( msg )
      send( this, no_permission );
    return false;
  }

  if( has_permission( this, PERM_ALL_OBJECTS ) 
      || is_name( descr->name, obj_clss->creator ) )
    return true;

  if( msg )
    send( this, no_permission );

  return false;
}


/*
 *   OFIND ROUTINES
 */


static int select_ofind( obj_clss_data* obj, char_data* ch, const char *argument )
{
  char               tmp  [ MAX_INPUT_LENGTH ];
  oprog_data*      oprog;
  char            letter;
  char            hyphen;
  const char*     string;
  bool          negative;
  int               i, j;
  int             length;

#define types 11

  const char flag [types+1] = "sfowirmLIII";
  
  const int max [types] = {
    MAX_SFLAG, table_max[ TABLE_AFF_CHAR ], MAX_OFLAG,
    MAX_ITEM_WEAR, MAX_ANTI,
    MAX_RESTRICTION, table_max[TABLE_MATERIAL], MAX_LAYER,
    MAX_CONT, MAX_TRAP, MAX_CONSUME
  };
  
  const char** name1 [types] = {
    &size_flags[0], &aff_char_table[0].name,
    &oflag_name[0], &wear_name[0], &anti_flags[0], &restriction_flags[0],
    &material_table[0].name, &layer_name[1],
    &cont_flag_name[0], &trap_flags[0], &consume_flags[0]
  };
  
  const char** name2 [types] = {
    &size_flags[1], &aff_char_table[1].name,
    &oflag_name[1], &wear_name[1], &anti_flags[1], &restriction_flags[1],
    &material_table[1].name, &layer_name[2],
    &cont_flag_name[1], &trap_flags[1], &consume_flags[1]
  };

  const int *flag_value [types] = {
    &obj->size_flags, obj->affect_flags,
    obj->extra_flags, &obj->wear_flags, &obj->anti_flags,
    &obj->restrictions, &obj->materials, &obj->layer_flags,
    &obj->value[1], &obj->value[0], &obj->value[3]
  };

  const bool used [types] = {
    true, true,
    true, true, true,
    true, true, true,
    obj->item_type == ITEM_CONTAINER,
    obj->item_type == ITEM_TRAP,
    ( obj->item_type == ITEM_FOOD
      || obj->item_type == ITEM_DRINK_CON
      || obj->item_type == ITEM_FOUNTAIN )
  };

  while( true ) {
    if( !( hyphen = *argument ) )
      return 1;
    
    if( hyphen != '-' ) {
      letter = 'n';
    }
    else {
      ++argument;
      if( !isalpha( letter = *argument++ )
	  && letter != '#' ) {
        send( ch, "Illegal character for flag - See help ofind.\n\r" );
        return -1;
      }
    }

    negative = false;
    skip_spaces( argument );

    if( *argument == '!' ) {
      negative = true;
      argument++;
    }

    if( *argument == '-' || isspace( *argument ) || !*argument ) {
      send( ch, "All flags require an argument - See help ofind.\n\r" );
      return -1;
    }
  
    for( i = 0; strncmp( argument-1, " -", 2 ) && *argument; ) {
      if( i > ONE_LINE-2 ) {
        send( ch, "Flag arguments must be less than one line.\n\r" );
        return -1;
      } 
      tmp[i++] = *argument++;
    }

    for( ; isspace( tmp[i-1] ); i-- );

    tmp[i] = '\0';
    string = 0;
    length = i;

    switch( letter ) {
    case 't' :  string = item_type_name[ obj->item_type ];     break;
    case 'c' :  string = obj->creator;                         break;
    }
    
    if( string ) {
      if( !strncasecmp( tmp, string, length ) == negative )
        return 0;
      continue;
    }

    if( letter == 'a' || letter == 'n' ) {
      if( !is_name( tmp, obj->Name( 1, false, true ) ) ^ negative ) {
        return 0;
      }
      continue;
    }

    if( letter == 'b' ) {
      if( !is_name( tmp, obj->Name( 1, false, false ) ) ^ negative ) {
        return 0;
      }
      continue;
    }

    if( letter == 'l' ) {
      int min, max;
      atorange( tmp, min, max );
      if( max < 0 )
	max = INT_MAX;
      if( negative
	  ^ ( obj->level < min
	      || obj->level > max ) )
	return 0;
      continue;
    }

    if( letter == '#' ) {
      int min, max;
      atorange( tmp, min, max );
      if( max < 0 )
	max = INT_MAX;
      if( negative
	  ^ ( obj->limit < min
	      || obj->limit > max ) )
	return 0;
      continue;
    }

    if( letter == 'T' ) {
      for( i = 0; !fmatches( tmp, oprog_trigger[i] ); i++ ) 
        if( i == MAX_OPROG_TRIGGER-1 ) {
          send( ch, "Unknown trigger type, see help ofind.\n\r" );
          return -1;
	} 
      for( oprog = obj->oprog; oprog && oprog->trigger != i;
	   oprog = oprog->next );
      if( ( oprog != 0 ) != negative )
        continue;
      return 0;
    }

    if( letter == 'W' ) {
      if( obj->item_type != ITEM_WEAPON ) 
        return 0;
      for( i = 0; i < table_max[TABLE_SKILL_WEAPON]; ++i ) 
        if( fmatches( tmp, skill_weapon_table[ i ].name ) ) {
          if( ( obj->value[3] == i ) == negative )
            return 0;
          break;
	}
      if( i == table_max[TABLE_SKILL_WEAPON] ) {
        send( ch, "Unknown weapon class - See help ofind.\n\r" );
        return -1;
      }
      continue;
    }

    for( i = 0; i < types; ++i ) {
      if( letter == flag[i] && used[i] ) {
	break;
      }
    }

    if( i != types ) {
      for( j = 0; j < max[i]; j++ ) {
        if( fmatches( tmp, *(name1[i]+j*(name2[i]-name1[i])) ) ) {
          if( is_set( flag_value[i], j ) == negative )
            return 0;
          break;
	}
      }
      if( j != max[i] ) {
        continue;
      }
    }

    if( letter == 'I' )
      return 0;

    send( ch, "Unknown flag - See help ofind.\n\r" );
    return -1;
  }

#undef types
}


static void display_ofind( obj_clss_data* obj, char_data* ch, char* buf,
			   int& length )
{
  const char *name = obj->Name( );

  if( strlen( name ) > 33 ) {
    length += snprintf( buf+length, MAX_STRING_LENGTH-length, "[%5d] %-71s\n\r",
			obj->vnum,
			trunc( name, 71 ).c_str() );
    length += snprintf( buf+length, MAX_STRING_LENGTH-length, "%41s %s %s %s%4d%4d%c%s ",
			"",
			int4( obj->cost ),
			int4( obj->weight ),
			int4( obj->count ),
			obj->blocks,
			obj->level,
			obj->remort ? '+' : ' ',
			int4( obj->durability ) ); 
  } else {
    length += snprintf( buf+length, MAX_STRING_LENGTH-length, "[%5d] %-33s %s %s %s%4d%4d%c%s ",
			obj->vnum,
			name,
			int4( obj->cost ),
			int4( obj->weight ),
			int4( obj->count ),
			obj->blocks,
			obj->level,
			obj->remort ? '+' : ' ',
			int4( obj->durability ) ); 
  }
  
  switch( obj->item_type ) {
  case ITEM_ARMOR :
    length += snprintf( buf+length, MAX_STRING_LENGTH, "AC: %d",
			obj->value[1] );
    break;
  case ITEM_WEAPON :
  case ITEM_ARROW :
  case ITEM_TRAP :
    length += snprintf( buf+length, MAX_STRING_LENGTH, "Dm: %s",
			dice_string( obj->value[1] ) );
    break;
  }
  
  length += snprintf( buf+length, MAX_STRING_LENGTH-length, "\n\r" );
  
  if( length > MAX_STRING_LENGTH-200 ) {
    page( ch, buf );
    length = 0;
    *buf = '\0';
  } 
}


void do_ofind( char_data* ch, const char *argument )
{
  char                buf  [ MAX_STRING_LENGTH ];
  obj_clss_data*      obj;
  int              length  = 0;
  unsigned count = 0;

  for( int i = 1; i <= obj_clss_max; ++i ) {
    if( ( obj = obj_index_list[i] ) ) { 
      switch( select_ofind( obj, ch, argument ) ) {
      case -1 : return;
      case  1 :
        if( count == 0 ) {
	  page( ch, "\n\r" );
          page_underlined( ch, "Vnum    Name                         \
     Cost  Wgt  Num Ing Lvl  Dur\n\r" );
	}
	++count;
        display_ofind( obj, ch, buf, length );
      }
    }
  }

  if( count == 0 ) 
    send( ch, "No object class matching search was found.\n\r" );
  else {
    page( ch, buf );
    page( ch, "\n\rFound %d match%s.\n\r",
	  count,
	  count == 1 ? "" : "es" );
  }
}


void do_identify( char_data* ch, const char *argument )
{
  if( !*argument ) {
    send( ch, "Identify what?\n\r" );
    return;
  }

  obj_data* obj;

  if( !( obj = one_object( ch, argument, "identity",
			   &ch->contents,
			   &ch->wearing,
			   ch->array) ) ) 
    return;
  
  identify( ch, obj );
}


/*
 *   OBJECT ONLINE COMMANDS
 */


/*
static void extract( obj_clss_data *obj_clss, wizard_data *wizard )
{
  for( int i = 0; i < obj_clss->data; ++i ) {
    wizard->oextra_edit = obj_clss->data[i];
    extract( wizard, offset( &wizard->oextra_edit, wizard ), "oextra" );
  }

  wizard->obj_clss_edit = obj_clss;
  extract( wizard, offset( &wizard->obj_clss_edit, wizard ), "object class" );
}
*/


void do_oedit( char_data* ch, const char *argument )
{
  char                 buf  [ MAX_INPUT_LENGTH ];
  char                 buf1  [ THREE_LINES ];
  obj_clss_data*  obj_clss;
  obj_clss_data*  obj_copy; 
  wizard_data*         imm;
  int                    i;

  if( !( imm = wizard( ch ) ) )
    return;

  if( !*argument ) {
    if( imm->obj_clss_edit ) {
      fsend( ch, "You stop editing %s.", imm->obj_clss_edit );
      imm->obj_clss_edit = 0;
      imm->oprog_edit = 0;
      imm->oextra_edit = 0;
    } else {
      send( ch, "Which object type do you want to edit?\n\r" );
    }
    return;
  }
  
  if( matches( argument, "new" ) ) {
    if( !*argument ) {
      send( ch, "What do you want to name the new object type?\n\r" );
      return;
    }

    if( number_arg( argument, i ) ) {
      if( !( obj_copy = get_obj_index( i ) ) ) {
        send( ch, "The vnum %d corresponds to no existing object type.\n\r", i );
        return;
      }
    } else {
      obj_copy = 0;
    }
     
    for( i = 1; ; i++ ) {
      if( i >= MAX_OBJ_INDEX ) {
        send( ch, "MUD is out of object type vnums.\n\r" );
        return;
      }
      if( !obj_index_list[i] )
        break;
    }
    
    obj_clss = new obj_clss_data(i);

    // This doesn't properly handle affected and extra_descr arrays!
    //    if( obj_copy ) 
    //      memcpy( obj_clss, obj_copy, sizeof( obj_clss_data ) );

    obj_clss->creator  = alloc_string( ch->descr->name, MEM_OBJ_CLSS );     

    obj_clss->singular = alloc_string( argument, MEM_OBJ_CLSS );
    snprintf( buf, MAX_INPUT_LENGTH, "%ss", obj_clss->singular );
    obj_clss->plural  = alloc_string( buf, MEM_OBJ_CLSS );

    if( obj_copy ) {
      obj_clss->before   = alloc_string( obj_copy->before, MEM_OBJ_CLSS );
      obj_clss->after    = alloc_string( obj_copy->after,  MEM_OBJ_CLSS );
      obj_clss->long_p   = alloc_string( obj_copy->long_p, MEM_OBJ_CLSS );
      obj_clss->long_s   = alloc_string( obj_copy->long_s, MEM_OBJ_CLSS );
      obj_clss->prefix_singular = alloc_string( obj_copy->prefix_singular, MEM_OBJ_CLSS );
      obj_clss->prefix_plural   = alloc_string( obj_copy->prefix_plural, MEM_OBJ_CLSS );
      obj_clss->item_type       = obj_copy->item_type;
      obj_clss->size_flags      = obj_copy->size_flags;
      obj_clss->anti_flags      = obj_copy->anti_flags;
      obj_clss->restrictions    = obj_copy->restrictions;
      obj_clss->materials       = obj_copy->materials;
      obj_clss->wear_flags      = obj_copy->wear_flags;
      obj_clss->layer_flags     = obj_copy->layer_flags;
      obj_clss->weight          = obj_copy->weight;
      obj_clss->cost            = obj_copy->cost;
      obj_clss->remort          = obj_copy->remort;
      obj_clss->level           = min( obj_copy->level, get_trust( ch ) );
      obj_clss->limit           = obj_copy->limit;
      obj_clss->repair          = obj_copy->repair;
      obj_clss->durability      = obj_copy->durability;
      obj_clss->blocks          = obj_copy->blocks;
      obj_clss->light           = obj_copy->light;

      vcopy( obj_clss->extra_flags, obj_copy->extra_flags, 2 );
      vcopy( obj_clss->value, obj_copy->value, 4 );
      vcopy( obj_clss->affect_flags, obj_copy->affect_flags, AFFECT_INTS );

      // Copy all oextras.
      for( int j = 0; j < obj_copy->extra_descr; ++j ) {
	obj_clss->extra_descr += new extra_data( *obj_copy->extra_descr[j] );
      }

      // Copy all oprogs and opdatas.
      for( oprog_data *oprog_copy = obj_copy->oprog; oprog_copy; oprog_copy = oprog_copy->next ) {
	oprog_data *oprog = new oprog_data( obj_clss );
	oprog->obj_act = oprog_copy->obj_act;
	oprog->obj_vnum = oprog_copy->obj_vnum;
	oprog->trigger = oprog_copy->trigger;
	oprog->value = oprog_copy->value;
	oprog->flags = oprog_copy->flags;
	oprog->target = alloc_string( oprog_copy->target, MEM_OPROG );
	oprog->command = alloc_string( oprog_copy->command, MEM_OPROG );
	oprog->Set_Code( oprog_copy->Code( ) );
	extra_array& extras = oprog->Extra_Descr( );
	extra_array& old_extras = oprog_copy->Extra_Descr( );
	for( int j = 0; j < old_extras; ++j ) {
	  extras += new extra_data( *old_extras[j] );
	}
      }
    }
     
    imm->obj_clss_edit = obj_clss;
    imm->oprog_edit = 0;
    imm->oextra_edit = 0;

    obj_clss->set_modified( 0 );
    obj_log( ch, obj_clss->vnum, "Created as %s.", obj_clss );
    fsend( ch, "Object type \"%s\" created, assigned vnum %d.",
	   obj_clss, obj_clss->vnum );

    obj_data *obj = create( obj_clss );
    //    enchant_object( obj );
    //    set_alloy( obj, get_trust( ch ) );
    //    set_quality( obj );
    //    set_size( obj, ch );    
    //    set_owner( obj, ch, 0 );
    if( is_set( obj->pIndexData->wear_flags, ITEM_TAKE ) ) {
      obj->To( ch );
    } else {
      obj->To( ch->in_room );
    }

    return;
  }
  
  if( exact_match( argument, "replace" ) ) {
    oedit_replace( ch, argument );
    return;
  }

  if( matches( argument, "delete" ) ) {
    if( !*argument ) {
      if( !( obj_clss = imm->obj_clss_edit ) ) {
        send( ch, "You aren't editing any object type.\n\r" );
        return;
      }
    } else if( !( obj_clss = get_obj_index( atoi( argument ) ) ) ) {
      send( ch, "There is no object type by that number.\n\r" );
      return;
    }

    if( !ch->can_edit( obj_clss )
	|| !can_extract( obj_clss, ch ) )
      return;
    
    obj_log( ch, obj_clss->vnum, "Deleted as %s.", obj_clss );
    fsend( ch, "You delete object type %d, \"%s\".", obj_clss->vnum, obj_clss );

    snprintf( buf, MAX_INPUT_LENGTH, "Object type \"%s\" deleted by %s",
	      obj_clss->Name( ), ch->descr->name );
    snprintf( buf1, THREE_LINES, "Object type \"%s\" deleted",
	      obj_clss->Name( ) );
    info( LEVEL_BUILDER, buf1, invis_level( imm ), buf, IFLAG_WRITES ); 

    obj_clss->set_modified( ch );
    imm->obj_clss_edit = obj_clss;
    extract( imm, offset( &imm->obj_clss_edit, imm ), "object type" );
    delete obj_clss;
    return;
  }
  
  if( number_arg( argument, i ) ) {
    if( !( obj_clss = get_obj_index( i ) ) ) {
      send( ch, "No object type has that vnum.\n\r" );
      return;
    }
    /*
    if( !ch->can_edit( obj_clss ) ) {
      return;
    }
    */
    imm->obj_clss_edit = obj_clss;

  } else {
    obj_data *obj;

    if( !( obj = one_object( ch, argument, 
			     "oedit", &ch->contents, &ch->wearing, 
			     ch->array ) ) )
      return;
    /*
    if( !ch->can_edit( obj->pIndexData ) ) {
      return;
    }
    */
    imm->obj_clss_edit = obj->pIndexData;
  }
  
  imm->oextra_edit = 0;
  imm->oprog_edit = 0;

  fsend( ch, "Ostat, oset, oflag, odesc, and obug now operate on object type: %s.",
	 imm->obj_clss_edit );
}


void oedit_replace( char_data* ch, const char *argument )
{
  obj_clss_data*      obj1  = 0;
  obj_clss_data*      obj2  = 0;
  int                count  = 0;
  int                 i, j;

  if( !number_arg( argument, i )
      || !number_arg( argument, j ) ) {
    send( ch, "Syntax: oedit replace <vnum_old> <vnum_new>.\n\r" );
    return;
  }

  if( !( obj1 = get_obj_index( i ) )
      || !( obj2 = get_obj_index( j ) ) ) {
    send( ch, "Vnum %d doesn't correspond to an existing object type.\n\r",
	  obj1 ? j : i );
    return;
  }

  for( area_data *area = area_list; area; area = area->next ) 
    for( room_data *room = area->room_first; room; room = room->next ) 
      for( reset_data *reset = room->reset; reset; reset = reset->next ) 
        if( reset->vnum == i 
	    && is_set( reset->flags, RSFLAG_OBJECT ) ) {
          reset->vnum = j;
          count++;
          area->modified = true;
	}
  
  for( int k = 1; k <= species_max; ++k ) 
    if( species_data *species = species_list[k] ) 
      for( reset_data *reset = species->reset; reset; reset = reset->next ) 
        if( reset->vnum == i 
	    && is_set( reset->flags, RSFLAG_OBJECT ) ) {
          reset->vnum = j;
          count++;
	}
  
  obj_log( ch, i, "Replaced by #%d in %d reset%s.",
	   j, count, count != 1 ? "s" : "" );
  send( ch, "Object type vnum %d replaced by %d in %d reset%s.\n\r",
	i, j, count, count != 1 ? "s" : "" );
}


/*
 *   OBJECT DESCRIPTIONS
 */


void do_odesc( char_data* ch, const char *argument )
{
  wizard_data*  imm;

  if( !( imm = wizard( ch ) ) )
    return;

  obj_clss_data *obj_clss;

  if( !( obj_clss = imm->obj_clss_edit ) ) {
    send( ch, "You aren't editing any object type.\n\r" );
    return;
  }

  if( !imm->oextra_edit ) {
    send( ch, "You aren't editing any object extra.\n\r" );
    return;
  }

  if( !ch->can_edit( obj_clss ) ) {
    return;
  }

  imm->oextra_edit->text
    = edit_string( ch, argument, imm->oextra_edit->text, MEM_EXTRA, true );

  if( *argument ) {
    obj_clss->set_modified( ch );
  }
}


void do_oextra( char_data* ch, const char *argument )
{
  obj_clss_data*  obj_clss;
  wizard_data*      imm;

  if( !( imm = wizard( ch ) ) )
    return;

  if( !( obj_clss = imm->obj_clss_edit ) ) {
    send( ch, "You aren't editing any object type.\n\r" );
    return;
  }
  
  if( !ch->can_edit( obj_clss ) ) {
    return;
  }

  if( edit_extra( obj_clss->extra_descr, imm,
		  offset( &imm->oextra_edit, imm ), argument, "odesc" ) ) {
    obj_clss->set_modified( ch );
  }
}


/*
  An affect on an object class has been modified.
  Must update its effects everywhere.
*/
static void update_affect( obj_clss_data *obj_clss, int sn, bool add )
{
  affect_data af;
  af.type = sn;
  af.level = 7;

  for( int i = 0; i < mob_list; ++i ) {
    mob_data *mob = mob_list[i];
    if( !mob->Is_Valid( ) )
      continue;
    for( int j = 0; j < mob->wearing; ++j ) {
      obj_data *obj = (obj_data*) mob->wearing[j];
      if( obj->pIndexData != obj_clss )
	continue;
      modify_affect( mob, &af, add, true );
      if( is_set( obj->pIndexData->extra_flags, OFLAG_ADDITIVE ) )
	break;
    }
  }

  for( int i = 0; i < player_list; ++i ) {
    player_data *pl = player_list[i];
    if( !pl->Is_Valid( ) )
      continue;
    for( int j = 0; j < pl->wearing; ++j ) {
      obj_data *obj = (obj_data*) pl->wearing[j];
      if( obj->pIndexData != obj_clss )
	continue;
      modify_affect( pl, &af, add, true );
      if( is_set( obj->pIndexData->extra_flags, OFLAG_ADDITIVE ) )
	break;
    }
  }

  for( int i = 0; i < obj_list; ++i ) {
    obj_data *obj = obj_list[i];
    if( !obj->Is_Valid( )
	|| obj->pIndexData != obj_clss
	|| !obj->array
	|| is_set( obj->pIndexData->wear_flags, ITEM_TAKE ) )
      continue;
    room_data *room = Room( obj->array->where );
    if( !room )
      continue;
    for( int j = 0; j < room->contents; ++j ) {
      char_data *rch = character( room->contents[j] );
      if( !rch )
	continue;
      modify_affect( rch, &af, add, true );
    }
  }
}


void do_oflag( char_data* ch, const char *argument )
{
  obj_clss_data*   obj_clss;
  wizard_data*          imm;

  if( !( imm = wizard( ch ) ) )
    return;

  if( !( obj_clss = imm->obj_clss_edit ) ) {
    page( ch, "You aren't editing any object type.\n\r" );
    return;
  }

  int flags;
  if( !get_flags( ch, argument, &flags, "s", "oflag" ) )
    return;

#define types 11

  const char *title [types] = { "Size", "Affect", "Object", "Wear", "Layer",
				"Anti", "Restriction", "Material",
				"Container", "Trap", "Consume" };
  int max [types] = { MAX_SFLAG, table_max[ TABLE_AFF_CHAR ], MAX_OFLAG,
		      MAX_ITEM_WEAR, MAX_LAYER, MAX_ANTI,
		      MAX_RESTRICTION, table_max[ TABLE_MATERIAL ],
		      MAX_CONT, MAX_TRAP, MAX_CONSUME };
  
  const char** name1 [types] = { &size_flags[0], &aff_char_table[0].name,
				 &oflag_name[0], &wear_name[0], &layer_name[1],
				 &anti_flags[0], &restriction_flags[0],
				 &material_table[0].name,
				 &cont_flag_name[0], &trap_flags[0], &consume_flags[0] };
  const char** name2 [types] = { &size_flags[1], &aff_char_table[1].name,
				 &oflag_name[1], &wear_name[1], &layer_name[2],
				 &anti_flags[1], &restriction_flags[1],
				 &material_table[1].name,
				 &cont_flag_name[1], &trap_flags[1], &consume_flags[1] };

  int *flag_value [types] = { &obj_clss->size_flags, obj_clss->affect_flags,
			      obj_clss->extra_flags, &obj_clss->wear_flags, &obj_clss->layer_flags,
			      &obj_clss->anti_flags,
			      &obj_clss->restrictions, &obj_clss->materials,
			      &obj_clss->value[1], &obj_clss->value[0], &obj_clss->value[3] };

  const int uses_flag [types] = { 1, 1, 1, 1, 1, 1, 1, 1, 
				  obj_clss->item_type == ITEM_CONTAINER,
				  obj_clss->item_type == ITEM_TRAP,
				  ( obj_clss->item_type == ITEM_FOOD
				    || obj_clss->item_type == ITEM_DRINK_CON
				    || obj_clss->item_type == ITEM_FOUNTAIN ) };
 
  const bool sort [types] = {
    false, true, true, false, false,
    false, true, true, true, true, true };
 
  int affect_flags [ AFFECT_INTS ];
  vcopy( affect_flags, obj_clss->affect_flags, AFFECT_INTS );

  int section = -1;
  const char *string;

  if( is_set( flags, 0 ) ) {
    // Select section.
    char arg [MAX_INPUT_LENGTH];
    
    argument = one_argument( argument, arg );

    const int l = strlen( arg );

    for( int i = 0; i < types; ++i ) {
      if( !strncasecmp( arg, title[i], l ) ) {
	section = i;
	break;
      }
    }

    if( section == -1 || !uses_flag[section] ) {
      fsend( ch, "Unknown section \"%s\".", arg );
      return;
    }

    if( !*argument ) {
      display_flags( title[section],
		     name1[section], name2[section],
		     flag_value[section], max[section], ch, sort[section] );
      return;
    }
    
    string = set_flags( name1[section], name2[section],
			flag_value[section], 0, max[section],
			ch->can_edit( obj_clss, false ) ? 0 : no_permission,
			ch, argument, obj_clss->Name( ),
			false, true, sort[section] );

  } else {
    string = flag_handler( title, name1, name2,
			   flag_value, 0, max, uses_flag, sort,
			   ch->can_edit( obj_clss, false ) ? 0 : no_permission, 
			   ch, argument, obj_clss->Name( ),
			   types );
  }

  if( string && *string ) {
    obj_clss->set_modified( ch );
    obj_log( ch, obj_clss->vnum, string );
    
    if( section == -1 || flag_value[section] == obj_clss->affect_flags ) {
      for( int i = 0; i < AFFECT_INTS; ++i ) {
	if( int flags = affect_flags[i] ^ obj_clss->affect_flags[i] ) {
	  int j = 0;
	  for( ; !(flags & 0x1); ++j ) {
	    flags >>= 1;
	  }
	  update_affect( obj_clss, 32*i+j, is_set( obj_clss->affect_flags[i], j ) );
	  break;
	}
      }
    }
  }

#undef types
}


void do_oload( char_data* ch, const char *argument )
{  
  const char *const syntax = "Syntax: oload [-n <count>] <vnum>.\n\r";
  
  if( !*argument ) {
    send( ch, syntax );
    return;
  }

  int flags;
  if( !get_flags( ch, argument, &flags, "n", "oload" ) )
    return;

  int num = 1;
  if( is_set( flags, 0 ) ) {
    if( !number_arg( argument, num )
	|| num <= 0 ) {
      send( ch, syntax );
      return;
    }
  }

  int vnum;
  if( !number_arg( argument, vnum ) ) {
    send( ch, syntax );
    return;
  }

  obj_clss_data *obj_clss;
  if( !( obj_clss = get_obj_index( vnum ) ) ) {
    send( ch, "No object has that vnum.\n\r" );
    return;
  }
  
  const int trust = get_trust( ch );

  if( !is_demigod( ch )
      && num * obj_clss->level > trust ) {
    send( ch, "You are limited to loading %d object levels at a time.\n\r", trust );
    return;
  }

  obj_data *obj = create( obj_clss, num );

  free_string( obj->source,  MEM_OBJECT );
  obj->source = alloc_string( ch->real_name( ), MEM_OBJECT );
  
  enchant_object( obj );
  set_alloy( obj, trust );
  set_quality( obj );
  set_size( obj, ch );

  obj->condition = repair_condition( obj );

  //  set_owner( obj, ch->pcdata->pfile );
  
  fsend( ch, "You have created %s!", obj );
  fsend_seen( ch, "%s has created %s!", ch, obj );

  obj->To( can_wear( obj, ITEM_TAKE ) ? ch : ch->array->where );

  if( obj_clss->limit >= 0
      && obj_clss->count > obj_clss->limit ) {
    send_color( ch, COLOR_MILD,
    		"Warning! You have exceeded this object's instance limit.\n\r" );
    send( ch, "[ Limit: %d. Instances: %d. ]\n\r",
	  obj_clss->limit, obj_clss->count );
    /*
    bug( "Object limit exceeded" );
    bug( "-- Char = %s.", ch );
    bug( "-- Object = %s.", obj );
    bug( "-- Limit = %d.", obj_clss->limit );
    bug( "-- Count = %d.", obj_clss->count );
    */
  }

}


/*
 *   OSET
 */


bool obj_clss_arg( char_data *ch, const char *& argument,
		   obj_clss_data *clss, obj_data *obj,
		   char *name, int& val )
{
  if( !matches( argument, name ) )
    return false;
  
  obj_data *obj2;
  
  if( !*argument ) {
    obj_clss_data *clss2 = get_obj_index( val );
    if( clss ) {
      send( ch, "Set %s of %s to?\n\r[ Current value: %d (%s) ]\n\r",
	    name, clss, val, clss2 ? clss2->Name( ) : "none" );
    } else {
      send( ch, "Set %s of %s to?\n\r[ Current value: %d (%s) ]\n\r",
	    name, obj, val, clss2 ? clss2->Name( ) : "none" );
    }
    return true;
  }
  
  int i;
  if( matches( argument, "none" ) ) {
    i = 0;
  } else if( !number_arg( argument, i ) ) {
    if( !( obj2 = one_object( ch, argument, name,
			      &ch->contents,
			      ch->array ) ) ) {
      return true;
    }
    i = obj2->pIndexData->vnum;
  }

  obj_clss_data *clss2 = get_obj_index( i );
  
  if( i != 0 && !clss2 ) {
    fsend( ch, "%s must be a valid object, object vnum, \"none\", or 0.", name );
    return true;
  }

  const char *string = clss2 ? clss2->Name( ) : "none";

  if( i == val ) {
    if( clss ) {
      fsend( ch, "%s on %s is already %d (%s).",
	     name, clss, i, string );
    } else {
      fsend( ch, "%s on %s is already %d (%s).",
	     name, obj, i, string );
    }
    return true;
  }

  if( clss ) {
    clss->set_modified( ch );
    obj_log( ch, clss->vnum, "%s set to %d (%s).",
	     name, i, string );
    fsend( ch, "%s on %s set to %d (%s).",
	   name, clss, i, string );
  } else {
    fsend( ch, "%s on %s set to %d (%s).",
	   name, obj, i, string );
  }
  
  val = i;
  return true;
}


static bool oset_drink_container( char_data* ch, obj_clss_data* obj, const char *argument )
{
  class int_field int_list[] = {
    { "capacity",        -1,    10000,   &obj->value[0]       },
    { "contains",        -1,    10000,   &obj->value[1]       },
    { "",                 0,        0,   0                    }
  };

  if( process( int_list, ch, argument, obj ) )
    return true;

#define ltn( i )   liquid_table[i].name
  class type_field type_list[] = {
    { "liquid",    table_max[ TABLE_LIQUID ],  &ltn(0),  &ltn(1),  &obj->value[2], true  },
    { "" }
  };
#undef ltn

  return process( type_list, ch, argument, obj );
}


static bool oset_fountain( char_data* ch, obj_clss_data* obj, const char *argument )
{
#define ltn( i )   liquid_table[i].name
  class type_field type_list[] = {
    { "liquid",    table_max[ TABLE_LIQUID ],  &ltn(0),  &ltn(1),  &obj->value[2], true  },
    { "" }
  };
#undef ltn

  return process( type_list, ch, argument, obj );
}


static bool oset_scroll( char_data* ch, obj_clss_data* obj, const char *argument )
{
  class int_field int_list[] = {
    { "cast level",       1, MAX_SPELL_LEVEL,   &obj->value[1]       },
    { "duration",         0,    10000,   &obj->value[2]       },
    { "",                 0,        0,   0                    }
  };

  if( process( int_list, ch, argument, obj ) )
    return true;

#define stn( i )   skill_spell_table[i].name
  class type_field type_list[] = {
    { "spell", table_max[TABLE_SKILL_SPELL],  &stn(0),  &stn(1),  &obj->value[0], true  },
    { "" }
  };
#undef stn

  return process( type_list, ch, argument, obj );
}


static bool oset_potion( char_data* ch, obj_clss_data* obj, const char *argument )
{
  if( obj_clss_arg( ch, argument, obj, 0, "empty", obj->value[3] ) ) {
    return true;
  }

  class int_field int_list[] = {
    { "cast level",       1, MAX_SPELL_LEVEL,   &obj->value[1]       },
    { "duration",         0,    10000,   &obj->value[2]       },
    { "",                 0,        0,   0                    }
  };

  if( process( int_list, ch, argument, obj ) )
    return true;

#define stn( i )   skill_spell_table[i].name
  class type_field type_list[] = {
    { "spell", table_max[TABLE_SKILL_SPELL],  &stn(0),  &stn(1),  &obj->value[0], true  },
    { "" }
  };
#undef stn

  return process( type_list, ch, argument, obj );
}


static bool oset_wand( char_data* ch, obj_clss_data* obj, const char *argument )
{
  class int_field int_list[] = {
    { "cast level",       1, MAX_SPELL_LEVEL,   &obj->value[1]       },
    { "duration",         0,    10000,   &obj->value[2]       },
    { "charges",         -1,     1000,   &obj->value[3]       },
    { "",                 0,        0,   0                    }
  };

  if( process( int_list, ch, argument, obj ) )
    return true;

#define stn( i )   skill_spell_table[i].name
  class type_field type_list[] = {
    { "spell", table_max[TABLE_SKILL_SPELL],  &stn(0),  &stn(1),  &obj->value[0], true  },
    { "" }
  };
#undef stn

  return process( type_list, ch, argument, obj );
}


static bool oset_container( char_data* ch, obj_clss_data* obj, const char *argument )
{
  class int_field int_list[] = {
    { "capacity",         0,    10000,   &obj->value[0]       },
    { "",                 0,        0,   0                    }
  };

  if( process( int_list, ch, argument, obj ) )
    return true;

  if( obj_clss_arg( ch, argument, obj, 0, "key", obj->value[2] ) ) {
    obj_clss_data *key = get_obj_index( obj->value[2] );
    if( key && key->item_type != ITEM_KEY ) {
      send( ch, "Warning: %s is not a key.\n\r", key );
    }
    return true;
  }

  return false;
}


static bool oset_table( char_data* ch, obj_clss_data* obj, const char *argument )
{
  class int_field int_list[] = {
    { "capacity",         0,    10000,   &obj->value[0]       },
    { "",                 0,        0,   0                    }
  };

  return process( int_list, ch, argument, obj );
}


static bool oset_keyring( char_data* ch, obj_clss_data* obj, const char *argument )
{
  class int_field int_list[] = {
    { "capacity",         0,    10000,   &obj->value[0]       },
    { "",                 0,        0,   0                    }
  };

  return process( int_list, ch, argument, obj );
}


static bool oset_chair( char_data* ch, obj_clss_data* obj, const char *argument )
{
  class int_field int_list[] = {
    { "seats",         0,    100,   &obj->value[0]       },
    { "",                 0,        0,   0                    }
  };

  return process( int_list, ch, argument, obj );
}


static bool oset_light( char_data* ch, obj_clss_data* obj, const char *argument )
{
  class int_field int_list[] = {
    { "lifetime",         -1,    1000,   &obj->value[0]       },
    { "",                 0,        0,   0                    }
  };

  return process( int_list, ch, argument, obj );
}


static bool oset_fire( char_data* ch, obj_clss_data* obj, const char *argument )
{
  class int_field int_list[] = {
    { "lifetime",         -1,    1000,   &obj->value[0]       },
    { "",                 0,        0,   0                    }
  };

  return process( int_list, ch, argument, obj );
}


static bool oset_gate( char_data* ch, obj_clss_data* obj, const char *argument )
{
  class int_field int_list[] = {
    { "lifetime",         -1,    1000,   &obj->value[0]       },
    { "to",         -1,    1000000,   &obj->value[1]       },
    { "",                 0,        0,   0                    }
  };

  return process( int_list, ch, argument, obj );
}


static bool oset_lock_pick( char_data* ch, obj_clss_data* obj, const char *argument )
{
  class int_field int_list[] = {
    { "modifier",         -100,    100,   &obj->value[0]       },
    { "",                 0,        0,   0                    }
  };

  return process( int_list, ch, argument, obj );
}


static bool oset_reagent( char_data* ch, obj_clss_data* obj, const char *argument )
{
  class int_field int_list[] = {
    { "charges",         -1,      100,   &obj->value[0]       },
    { "",                 0,        0,   0                    }
  };

  return process( int_list, ch, argument, obj );
}


static bool oset_food( char_data* ch, obj_clss_data* obj, const char *argument )
{
  class int_field int_list[] = {
    { "nourishment",   -100,      100,   &obj->value[0]       },
    { "",                 0,        0,   0                    }
  };

  if( process( int_list, ch, argument, obj ) )
    return true;

  class type_field type_list[] = {
    { "cooked", MAX_COOK+1, cook_word,  cook_word+1,  &obj->value[1], true },
    { "" }
  };

  ++obj->value[1];
  if( process( type_list, ch, argument, obj ) ) {
    --obj->value[1];
    return true;
  }
  --obj->value[1];

  return false;
}


static bool oset_gem( char_data* ch, obj_clss_data* obj, const char *argument )
{
  class type_field type_list[] = {
    { "quality", MAX_GEM+1, gem_quality,  gem_quality+1,  &obj->value[1], true },
    { "" }
  };

  ++obj->value[1];
  if( process( type_list, ch, argument, obj ) ) {
    --obj->value[1];
    return true;
  }
  --obj->value[1];

  return false;
}


static bool oset_armor( char_data* ch, obj_clss_data* obj, const char *argument )
{
  class int_field int_list[] = {
    { "enchantment",     -5,        5,   &obj->value[0]       },
    { "ac",             -50,       50,   &obj->value[1]       },
    { "global ac",      -50,       50,   &obj->value[2]       },
    { "",                 0,        0,   0                    }
  };

  return process( int_list, ch, argument, obj );
}


static bool oset_corpse( char_data* ch, obj_clss_data* obj, const char *argument )
{
  class int_field int_list[] = {
    { "halflife",   0,  QUEUE_LENGTH-1,   &obj->value[0]       },
    { "species",    0,     species_max,   &obj->value[1]       },
    { "",           0,               0,   0                    }
  };

  return process( int_list, ch, argument, obj );
}


static bool oset_weapon( char_data* ch, obj_clss_data* obj, const char *argument )
{
  class int_field int_list[] = {
    { "enchantment",     -5,        5,   &obj->value[0]       },
    { "",                 0,        0,   0                    }
  };

  if( process( int_list, ch, argument, obj ) )
    return true;

  class dice_field dice_list[] = {
    { "damage", LEVEL_OBJECT, &obj->value[1] },
    { "",                 -1,              0 }
  };

  if( process( dice_list, ch, argument, obj ) )
    return true;

#define wn(i) skill_weapon_table[ i ].name

  class type_field type_list[] = {
    { "class", table_max[TABLE_SKILL_WEAPON], &wn(0),  &wn(1),  &obj->value[3], true },
    { "" }
  };

#undef wn

  return process( type_list, ch, argument, obj );
}


static bool oset_trap( char_data* ch, obj_clss_data* obj, const char *argument )
{
  class dice_field dice_list[] = {
    { "damage", LEVEL_OBJECT, &obj->value[1] },
    { "",                 -1,              0 }
  };

  return process( dice_list, ch, argument, obj );

  /*
  class int_field int_list[] = {
    { "damdice",          0,       50,   &obj->value[1]       },
    { "damside",          0,       50,   &obj->value[2]       },
    { "",                 0,        0,   0                    }
  };

  return process( int_list, ch, argument, obj );
  */
}


static bool oset_arrow( char_data* ch, obj_clss_data* obj, const char *argument )
{
  class int_field int_list[] = {
    { "enchantment",     -5,        5,   &obj->value[0]       },
    //    { "damdice",          0,       50,   &obj->value[1]       },
    //    { "damside",          0,       50,   &obj->value[2]       },
    { "",                 0,        0,   0                    }
  };

  if( process( int_list, ch, argument, obj ) )
    return true;

  class dice_field dice_list[] = {
    { "damage", LEVEL_OBJECT, &obj->value[1] },
    { "",                 -1,              0 }
  };

  if( process( dice_list, ch, argument, obj ) )
    return true;

#define wn(i) skill_weapon_table[ i ].name

  class type_field type_list[] = {
    { "class", table_max[TABLE_SKILL_WEAPON], &wn(0),  &wn(1),  &obj->value[3], true },
    { "" }
  };

#undef wn

  return process( type_list, ch, argument, obj );
}


static bool oset_whistle( char_data* ch, obj_clss_data* obj, const char *argument )
{
  class int_field int_list[] = {
    { "range",            0,      100,   &obj->value[0]       },
    { "",                 0,        0,   0                    }
  };

  return process( int_list, ch, argument, obj );
}


static bool oset_garrote( char_data* ch, obj_clss_data* obj, const char *argument )
{
  class dice_field dice_list[] = {
    { "damage", LEVEL_OBJECT, &obj->value[1] },
    { "",                 -1,              0 }
  };

  return process( dice_list, ch, argument, obj );
}


/*
  An affect on an object class has been modified.
  Must update its effects everywhere.
*/
static void update_affect( obj_clss_data *obj_clss, affect_data *paf, bool add )
{
  for( int i = 0; i < mob_list; ++i ) {
    mob_data *mob = mob_list[i];
    if( !mob->Is_Valid( ) )
      continue;
    for( int j = 0; j < mob->wearing; ++j ) {
      obj_data *obj = (obj_data*) mob->wearing[j];
      if( obj->pIndexData != obj_clss )
	continue;
      modify_affect( mob, paf, add, true );
      if( is_set( obj->pIndexData->extra_flags, OFLAG_ADDITIVE ) )
	break;
    }
  }

  for( int i = 0; i < player_list; ++i ) {
    player_data *pl = player_list[i];
    if( !pl->Is_Valid( ) )
      continue;
    for( int j = 0; j < pl->wearing; ++j ) {
      obj_data *obj = (obj_data*) pl->wearing[j];
      if( obj->pIndexData != obj_clss )
	continue;
      modify_affect( pl, paf, add, true );
      if( is_set( obj->pIndexData->extra_flags, OFLAG_ADDITIVE ) )
	break;
    }
  }

  for( int i = 0; i < obj_list; ++i ) {
    obj_data *obj = obj_list[i];
    if( !obj->Is_Valid( )
	|| obj->pIndexData != obj_clss
	|| !obj->array
	|| is_set( obj->pIndexData->wear_flags, ITEM_TAKE ) )
      continue;
    room_data *room = Room( obj->array->where );
    if( !room )
      continue;
    for( int j = 0; j < room->contents; ++j ) {
      char_data *rch = character( room->contents[j] );
      if( !rch )
	continue;
      modify_affect( rch, paf, add, true );
    }
  }
}


static void oset_affect( char_data* ch, obj_clss_data* obj_clss, const char *argument )
{
  if( !*argument ) {
    display_array( ch, "Affect Types",
		   &affect_location[1], &affect_location[2],
		   MAX_AFF_LOCATION - 1 );
    return;
  }
  
  affect_data *paf  = 0;
  int num;
  int j;
  for( int i = 1; i < MAX_AFF_LOCATION; i++ ) {
    if( matches( argument, affect_location[i] ) ) {
      for( j = 0; j < obj_clss->affected; j++ ) {
        if( obj_clss->affected[j]->location == i ) {
          paf = obj_clss->affected[j];
          break;
	}
      }
      if( ( num = atoi( argument ) ) == 0 ) {
        if( paf ) {
	  obj_log( ch, obj_clss->vnum, "%s modifier removed.",
		   affect_location[i] );
	  fsend( ch, "%s modifier removed from object type: %s.", 
		 affect_location[i], obj_clss );
	  update_affect( obj_clss, paf, false );
          obj_clss->affected.remove(j);
	  obj_clss->set_modified( ch );
	} else {
	  fsend( ch, "%s modifier has not been set.", 
		 affect_location[i] );
	}
      } else {
        if( !paf ) {
	  obj_log( ch, obj_clss->vnum, "%s modifier of %d added.",
		   affect_location[i], num );
	  fsend( ch, "%s modifier of %d added to object type: %s.", 
		 affect_location[i], num, obj_clss );
          paf = new affect_data;
          paf->type = AFF_NONE;
          paf->duration = -1;
          paf->location = i;
	  paf->modifier = num;
          obj_clss->affected += paf;
	  update_affect( obj_clss, paf, true );
	  obj_clss->set_modified( ch );
	} else if( paf->modifier != num ) {
	  obj_log( ch, obj_clss->vnum, "%s modifier changed to %d.",
		   affect_location[i], num );
	  fsend( ch, "%s modifier changed to %d on object type: %s.", 
		 affect_location[i], num, obj_clss );
	  paf->modifier -= num;
	  update_affect( obj_clss, paf, false );
	  paf->modifier = num;
	  obj_clss->set_modified( ch );
	} else {
	  fsend( ch, "%s modifier is already %d.", 
		 affect_location[i], num );
	}
      }
      return;
    }
  }
  
  send( ch, "Unknown affect location.\n\r" );
}


void do_oset( char_data* ch, const char *argument )
{
  obj_clss_data*  obj_clss;
  obj_data*            obj;
  wizard_data*         imm;
  char*             string;

  if( !( imm = wizard( ch ) ) )
    return;

  if( !( obj_clss = imm->obj_clss_edit ) ) {
    send( ch, "You aren't editing any object type.\n\r" );
    return;
  }

  if( !*argument ) {
    do_ostat( ch, "" );
    return;
  }

  if( !ch->can_edit( obj_clss ) ) {
    return;
  }

  {
    class int_field int_list[] = {
      { "level",            0, ch->Level(), &obj_clss->level      },
      { "remort",           0,    10000,   &obj_clss->remort      },
      { "value0",     INT_MIN,  INT_MAX,   &obj_clss->value[0]    },
      { "value1",     INT_MIN,  INT_MAX,   &obj_clss->value[1]    },
      { "value2",     INT_MIN,  INT_MAX,   &obj_clss->value[2]    },
      { "value3",     INT_MIN,  INT_MAX,   &obj_clss->value[3]    },
      { "limit",           -1,   100000,   &obj_clss->limit       },
      { "repair",           0,       10,   &obj_clss->repair      },
      { "durability",       1,    10000,   &obj_clss->durability  },
      { "ingots",          -1,      100,   &obj_clss->blocks      },
      { "cost",             0,  1000000,   &obj_clss->cost        },
      { "light",         -100,      100,   &obj_clss->light       },
      { "",                 0,        0,   0                      }
    };

    if( process( int_list, ch, argument, obj_clss ) )
      return;
  }

  {
    class cent_field cent_list[] = {
      { "weight",           0,  1000000,   &obj_clss->weight      },
      { "",                 0,        0,   0                      }
    };
    
    if( process( cent_list, ch, argument, obj_clss ) ) {
      return;
    }
  }

  if( obj_clss_arg( ch, argument, obj_clss, 0, "fakes", obj_clss->fakes ) ) {
    return;
  }

  const char *const word[] = {
    "singular", "plural", "after", "before",
    "long_s", "long_p", "creator",
    "prefix_singular", "prefix_plural"
  };
  
  char **const pChar[] = {
    &obj_clss->singular, &obj_clss->plural,
    &obj_clss->after, &obj_clss->before, &obj_clss->long_s,
    &obj_clss->long_p, &obj_clss->creator,
    &obj_clss->prefix_singular, &obj_clss->prefix_plural
  };
  
  for( int i = 0; i < 9; i++ ) {
    if( matches( argument, word[i] ) ) {
      obj_clss->set_modified( ch );
      obj_log( ch, obj_clss->vnum, "%s set to %s.",
	       word[i], argument );
      send( ch, "%s on %s set.\n\r[ New:  %s ]\n\r[ Prev: %s ]\n\r",
	    word[i], obj_clss, argument, *pChar[i] );

      string = alloc_string( argument, MEM_OBJ_CLSS );

      if( i < 4 ) {
        for( int j = 0; j < obj_list; j++ ) {
          obj = obj_list[j];
          if( obj->pIndexData == obj_clss ) {
            char **pChar2[] = { &obj->singular, &obj->plural,
				&obj->after, &obj->before };
            if( *pChar2[i] == *pChar[i] )
              *pChar2[i] = string;
	  }
	}
      }
      free_string( *pChar[i], MEM_OBJ_CLSS );
      *pChar[i] = string;
      return;
    }          
  }

#define itn item_type_name

  class type_field type_list [] = {
    { "type",   MAX_ITEM,    &itn[0], &itn[1],  &obj_clss->item_type, true },
    { "" }
  };

  if( process( type_list, ch, argument, obj_clss ) )
    return;

#undef itn

  switch( obj_clss->item_type ) {
  case ITEM_WEAPON :
    if( oset_weapon( ch, obj_clss, argument ) )
      return;
    break;
  case ITEM_ARROW :
    if( oset_arrow( ch, obj_clss, argument ) )
      return;
    break;
  case ITEM_ARMOR :
    if( oset_armor( ch, obj_clss, argument ) )
      return;
    break;
  case ITEM_CONTAINER :
    if( oset_container( ch, obj_clss, argument ) )
      return;
    break;
  case ITEM_TABLE :
    if( oset_table( ch, obj_clss, argument ) )
      return;
    break;
  case ITEM_KEYRING :
    if( oset_keyring( ch, obj_clss, argument ) )
      return;
    break;
  case ITEM_CHAIR :
    if( oset_chair( ch, obj_clss, argument ) )
      return;
    break;
  case ITEM_REAGENT :
    if( oset_reagent( ch, obj_clss, argument ) )
      return;
    break;
  case ITEM_FOOD :
    if( oset_food( ch, obj_clss, argument ) )
      return;
    break;
  case ITEM_DRINK_CON :
    if( oset_drink_container( ch, obj_clss, argument ) )
      return;
    break;
  case ITEM_FOUNTAIN :
    if( oset_fountain( ch, obj_clss, argument ) )
      return;
    break;
  case ITEM_CORPSE :
    if( oset_corpse( ch, obj_clss, argument ) )
      return;
    break;
  case ITEM_GEM :
    if( oset_gem( ch, obj_clss, argument ) )
      return;
    break;
  case ITEM_LOCK_PICK :
    if( oset_lock_pick( ch, obj_clss, argument ) )
      return;
    break;
  case ITEM_TRAP :
    if( oset_trap( ch, obj_clss, argument ) )
      return;
    break;
  case ITEM_LIGHT :   
  case ITEM_LIGHT_PERM :   
    if( oset_light( ch, obj_clss, argument ) )
      return;
    break;
  case ITEM_SCROLL:
    if( oset_scroll( ch, obj_clss, argument ) )
      return;
    break;
  case ITEM_POTION :
    if( oset_potion( ch, obj_clss, argument ) )
      return;
    break;
  case ITEM_WAND :
    if( oset_wand( ch, obj_clss, argument ) )
      return;
    break;
  case ITEM_FIRE :
    if( oset_fire( ch, obj_clss, argument ) )
      return;
    break;
  case ITEM_GATE :
    if( oset_gate( ch, obj_clss, argument ) )
      return;
    break;
  case ITEM_WHISTLE :
    if( oset_whistle( ch, obj_clss, argument ) )
      return;
    break;
  case ITEM_GARROTE :
    if( oset_garrote( ch, obj_clss, argument ) )
      return;
    break;
  default:
    break;
  }

  if( matches( argument, "affect" ) ) {
    oset_affect( ch, obj_clss, argument );
    return;
  }
  
  send( ch, "Unknown field.\n\r" );
}


/*
 *  OSTAT ROUTINE
 */


void do_ostat( char_data* ch, const char *argument )
{
  char                   tmp  [ 3*MAX_STRING_LENGTH ];
  obj_data*              obj  = 0;
  obj_clss_data*    obj_clss;
  wizard_data*           imm;
  int i;

  if( !( imm = wizard( ch ) ) )
    return;

  if( !*argument ) {
    if( !( obj_clss = imm->obj_clss_edit ) ) {
      send( ch, "Specify object type or select one with oedit.\n\r" );
      return;
    }

  } else if( number_arg( argument, i ) ) {
    if( !( obj_clss = get_obj_index( i ) ) ) {
      send( ch, "No object type has that vnum.\n\r" );
      return;
    }

  } else {
    if( !( obj = one_object( ch, argument, "ostat",
			     &ch->contents,
			     &ch->wearing,
			     ch->array ) ) )
      return;
    obj_clss = obj->pIndexData;
  }

  if( obj )
    page_title( ch, obj->Name( ) );
  else {
    page_title( ch, obj_clss->Name( ) );
  }

  page( ch, "          Vnum: %-10d Creator: %s%-13s%s Type: %s\n\r",
	obj_clss->vnum,
	color_code( ch, COLOR_BOLD_YELLOW ), obj_clss->creator, normal( ch ),
	item_type_name[ obj_clss->item_type ] );

  if( is_angel( ch ) ) 
    page( ch, "        Number: %-12d Limit: %-9d Last Mod: %s\n\r",
	  obj_clss->count, obj_clss->limit, obj_clss->last_mod ); 

  page( ch, "        Weight: %-12.2f Light: %-8d Base Cost: %d\n\r",
	(double)obj_clss->weight/100.0,
	obj_clss->light, obj_clss->cost );
  page( ch, "         Level: %-11d Remort: %d\n\r",
	obj_clss->level, obj_clss->remort );
  page( ch, "        Repair: %-10d Durabil: %-11d Ingots: %d\n\r",
	obj_clss->repair, obj_clss->durability, obj_clss->blocks );

  const obj_clss_data *fakes;
  page( ch, "         Fakes: (%d) %s\n\r",
	obj_clss->fakes,
	!( fakes = get_obj_index( obj_clss->fakes ) )
	? "none" : fakes->Name( ) );

  page( ch, "        Values: (0) %d, (1) %d, (2) %d, (3) %d\n\r",
	obj_clss->value[0], obj_clss->value[1],
	obj_clss->value[2], obj_clss->value[3] );

  switch( obj_clss->item_type ) {
  case ITEM_WEAPON :
    page( ch, "\n\r" );
    page( ch, "         Class: %-11s Attack: %s\n\r",
	  weapon_class( obj_clss ),
	  ( obj_clss->value[3] >= 0 && obj_clss->value[3] < table_max[TABLE_SKILL_WEAPON] ) ?
	  skill_weapon_table[ obj_clss->value[3] ].noun[0] : "none" );
    page( ch, "        Damage: %-26s Enchantment: %d\n\r",
	  dice_string( obj_clss->value[1] ),
	  obj_clss->value[0] );
    break;
    
  case ITEM_TRAP   :
    page( ch, "\n\r" );
    page( ch, "        Damage: %-26s\n\r",
	  dice_string( obj_clss->value[1] ) );
    break;
      
  case ITEM_ARROW :
    page( ch, "\n\r" );
    page( ch, "         Class: %-11s Attack: %s\n\r",
	  weapon_class( obj_clss ),
	  ( obj_clss->value[3] >= 0 && obj_clss->value[3] < table_max[TABLE_SKILL_WEAPON] ) ?
	  skill_weapon_table[ obj_clss->value[3] ].noun[0] : "none" );
    page( ch, "        Damage: %-26s Enchantment: %d\n\r",
	  dice_string( obj_clss->value[1] ),
	  obj_clss->value[0] );
    break;
    
  case ITEM_SCROLL :
    {
      const int spell = obj_clss->value[0];
      const char *name = "unknown";
      if( spell >= 0 && spell < table_max[TABLE_SKILL_SPELL] ) {
	name = skill_spell_table[spell].name;
      }
      page( ch, "\n\r" );
      page( ch, "    Cast Level: %-9d Duration: %-12d Spell: %s\n\r",
	    obj_clss->value[1], obj_clss->value[2], name );
    }
    break;

  case ITEM_POTION :
    {
      const int spell = obj_clss->value[0];
      const char *name = "unknown";
      const obj_clss_data *empty;
      if( spell >= 0 && spell < table_max[TABLE_SKILL_SPELL] ) {
	name = skill_spell_table[spell].name;
      }
      page( ch, "\n\r" );
      page( ch, "    Cast Level: %-9d Duration: %-12d Spell: %s\n\r",
	    obj_clss->value[1], obj_clss->value[2], name );
      page( ch, "         Empty: %d (%s)\n\r",
	    obj_clss->value[3],
	    !( empty = get_obj_index( obj_clss->value[3] ) )
	    ? "none" : empty->Name( ) );
    }
    break;
      
  case ITEM_WAND :
    {
      const int spell = obj_clss->value[0];
      const char *name = "unknown";
      if( spell >= 0 && spell < table_max[TABLE_SKILL_SPELL] ) {
	name = skill_spell_table[spell].name;
      }
      page( ch, "\n\r" );
      page( ch, "    Cast Level: %-9d Duration: %-12d Spell: %s\n\r",
	    obj_clss->value[1], obj_clss->value[2], name );
      page( ch, "       Charges: %d\n\r",
	    obj_clss->value[3] );
    }
    break;
      
  case ITEM_ARMOR :
    page( ch, "\n\r" );
    page( ch, "            AC: %-8d Global AC: %-6d Enchantment: %d\n\r",
	  obj_clss->value[1], obj_clss->value[2], obj_clss->value[0] );
    break;

  case ITEM_CONTAINER :   
    {
      const obj_clss_data *key;
      page( ch, "\n\r" );
      page( ch, "      Capacity: %-14d Key: %d (%s)\n\r",
	    obj_clss->value[0], obj_clss->value[2],
	    !( key = get_obj_index( obj_clss->value[2] ) )
	    ? "none" : key->Name( ) );
    }
    break;

  case ITEM_TABLE :   
    page( ch, "\n\r" );
    page( ch, "      Capacity: %d\n\r",
	  obj_clss->value[0] );
    break;

  case ITEM_KEYRING :   
    page( ch, "\n\r" );
    page( ch, "      Capacity: %d\n\r",
	  obj_clss->value[0] );
    break;

  case ITEM_CHAIR :   
    page( ch, "\n\r" );
    page( ch, "         Seats: %d\n\r",
	  obj_clss->value[0] );
    break;

  case ITEM_FIRE:
  case ITEM_LIGHT :   
  case ITEM_LIGHT_PERM :   
    page( ch, "\n\r" );
    page( ch, "      Lifetime: %d\n\r",
	  obj_clss->value[0] );
    break;

  case ITEM_DRINK_CON :
    {
      int liquid = obj_clss->value[2];
      page( ch, "\n\r" );
      page( ch, "      Capacity: %-10d Contains: %-10d Liquid: %s\n\r",
	    obj_clss->value[0], obj_clss->value[1],
	    ( liquid < 0 || liquid > table_max[ TABLE_LIQUID ] ) ?
	    "unknown" : liquid_name( liquid, true ) );
    }
    break;

  case ITEM_FOUNTAIN :
    {
      int liquid = obj_clss->value[2];
      page( ch, "\n\r" );
      page( ch, "        Liquid: %s\n\r",
	    ( liquid < 0 || liquid > table_max[ TABLE_LIQUID ] ) ?
	    "unknown" : liquid_name( liquid, true ) );
    }
    break;

  case ITEM_REAGENT :
    page( ch, "\n\r" );
    page( ch, "       Charges: %d\n\r", obj_clss->value[0] );
    break;

  case ITEM_GATE :
    {
      room_data *to;
      page( ch, "\n\r" );
      page( ch, "      Lifetime: %-15d To: %d (%s)\n\r",
	    obj_clss->value[0],
	    obj_clss->value[1],
	    !( to = get_room_index( obj_clss->value[1], false ) )
	    ? "none" : to->name );
    }
    break;

  case ITEM_LOCK_PICK :
    page( ch, "\n\r" );
    page( ch, "      Modifier: %d\n\r",
	  obj_clss->value[0] );
    break;

  case ITEM_FOOD :
    {
      int cooked = obj_clss->value[1] + 1;
      if( cooked < 0 || cooked > MAX_COOK ) cooked = 0;
      page( ch, "\n\r" );
      page( ch, "   Nourishment: %-11d Cooked: %-11s\n\r",
	    obj_clss->value[0], cook_word[ cooked ] );
    }
    break;

  case ITEM_GEM:
    {
      int quality = obj_clss->value[1] + 1;
      if( quality > MAX_GEM ) quality = 1;
      else if( quality < 0 ) quality = 0;
      page( ch, "\n\r" );
      page( ch, "       Quality: %s\n\r",
	    gem_quality[ quality ]
	    );
    }
    break;

  case ITEM_CORPSE:
    {
      species_data *species  = get_species( obj_clss->value[1] );      
      page( ch, "\n\r" );
      page( ch, "      Lifetime: %d\n\r", obj_clss->value[0] );
      page( ch, "       Species: %-5d (%s)\n\r",
	    obj_clss->value[1], species ? species->Name( ) : "none" ); 
      break;
    }
  
  case ITEM_WHISTLE:
    page( ch, "\n\r" );
    page( ch, "         Range: %d\n\r",
	  obj_clss->value[0] );
    break;

  case ITEM_GARROTE:
    page( ch, "\n\r" );
    page( ch, "        Damage: %s\n\r",
	  dice_string( obj_clss->value[1] ) );
    break;
  }

  if( obj ) {
    page( ch, "\n\rInstance Data:\n\r" );
    page( ch, "        Number: %-12d Owner: %s\n\r",
	  obj->Number( ),
	  obj->owner ? obj->owner->name : "no one" );
    page( ch, "        Weight: %-12.2f Light: %-13d Cost: %d\n\r",
	  (double)obj->weight/100.0,
	  obj->light,
	  obj->cost );
    page( ch, "     Condition: %-14d Age: %-13d Rust: %d\n\r",
	  obj->condition, obj->age, obj->rust );
    page( ch, "        Values: (0) %d, (1) %d, (2) %d, (3) %d\n\r",
	  obj->value[0], obj->value[1], obj->value[2], obj->value[3] );

    if( obj->timer > 0 )
      page( ch, "         Timer: %d\n\r",
	    obj->timer );
    if( obj->reset ) {
      page( ch, "         Reset: %s\n\r", name( obj->reset ) );
    }

    page( ch, "\n\rCalculated Instance Values:\n\r" );
    page( ch, "         Level: %-12d Value: %-11d Weight: %.2f\n\r",
	  obj->Level( ), obj->Cost( ),
	  (double)obj->Weight( 1 )/100.0 );
    int rc = repair_condition( obj );
    page( ch, "    Durability: %-7d Max Repair: %d (%s be repaired)\n\r",
	  obj->Durability( ),
	  rc,
	  obj->Damaged( )
	  ? "can"
	  : "cannot" );
  }

  if( !obj_clss->affected.is_empty( ) ) {
    page( ch, "\n\rAffects:\n\r" );
    for( int i = 0; i < obj_clss->affected; i++ ) {
      affect_data *paf = obj_clss->affected[i];
      if( paf->type == AFF_NONE )  
	page( ch, "  %s by %+d.\n\r",
	      affect_location[ paf->location ], paf->modifier );
    }
  }

  page( ch, "\n\r" );
  page( ch, "Prefix_S: %s\n\r", obj_clss->prefix_singular );
  page( ch, "Singular: %s\n\r", obj_clss->singular );
  page( ch, "  Long_S: %s\n\r", obj_clss->long_s );
  page( ch, "Prefix_P: %s\n\r", obj_clss->prefix_plural );
  page( ch, "  Plural: %s\n\r", obj_clss->plural );
  page( ch, "  Long_P: %s\n\r", obj_clss->long_p );
  page( ch, "  Before: %s\n\r", obj_clss->before );
  page( ch, "   After: %s\n\r", obj_clss->after );

  show_extras( ch, obj_clss->extra_descr );

  if( obj_clss->fakes != 0
      && obj_clss->fakes != obj_clss->vnum ) {
    sprintf( tmp, "\n\r[ before ][ from Obj %d ]\n\r%s\n\r",
	     obj_clss->fakes, before_descr( obj_clss ) );
    page( ch, tmp );
  }   

  if( *obj_clss->comments ) {
    page( ch, "\n\rComments:\n\r" );
    page( ch, obj_clss->comments );
  }
}


void do_obug( char_data* ch, const char *argument ) 
{
  wizard_data *imm;
  if( !( imm = wizard( ch ) ) )
    return;

  obj_clss_data *obj_clss;
  if( !( obj_clss = imm->obj_clss_edit ) ) {
    send( ch, "You aren't editing any object type.\n\r" );
    return;
  }

  if( !*argument || ch->can_edit( obj_clss ) ) {
    obj_clss->comments = edit_string( ch, argument, obj_clss->comments, MEM_OBJ_CLSS, false );
    if( *argument ) {
      obj_clss->set_modified( ch );
    }
  }
}


/*
 *   OWHERE
 */


static int select_owhere( obj_clss_data* obj_clss, obj_data* obj,
		   char_data* ch, const char *argument )
{
  char               tmp  [ MAX_INPUT_LENGTH ];
  char            letter;
  char            hyphen;
  const char*     string;
  bool          negative;
  int             length;

  while (true) {
    if( ( hyphen = *argument ) == '\0' )
      return 1;

    if( hyphen != '-' ) {
      letter = 'n';
    } else {
      argument++;
      if( !isalpha( letter = *argument++ ) ) {
        send( ch, "Illegal character for flag - See help owhere.\n\r" );
        return -1;
      }
    }

    negative = false;
    skip_spaces( argument );

    if( *argument == '!' ) {
      negative = true;
      argument++;
    }

    if( *argument == '-' || isspace( *argument ) || *argument == '\0' ) {
      send( ch, "All flags require an argument - See help owhere.\n\r" );
      return -1;
    }
  
    for( int i = 0; ; ) {
      if( !strncmp( argument-1, " -", 2 ) || *argument == '\0' ) {
        for( ; isspace( tmp[i-1] ); i-- );
        tmp[i] = '\0';
        length = i;
        break;
      }
      if( i > ONE_LINE-2 ) {
        send( ch, "Flag arguments must be less than one line.\n\r" );
        return -1;
      } 
      tmp[i++] = *argument++;
    }

    string = 0;

    if( obj ) {
      switch( letter ) {
      case 's' :  string = obj->source;     break;
      case 'l' :  string = obj->label;      break;
      }
    }
  
    if( string ) {
      if( !strncasecmp( tmp, string, length ) == negative )
        return 0;
      continue;
    }
    
    if( letter == 'n' ) {
      if( !is_name( tmp, obj ? obj->Seen_Name( ch ) : obj_clss->Name( ) ) )
        return 0;
      continue;
    }

    send( ch, "Unknown flag - See help owhere.\n\r" );
    return -1;
  }
}


static void display_owhere( obj_data* obj, char_data* ch, char* tmp,
			    int& length )
{
  room_data*          room  = 0;
  char_data*         owner  = 0;   
  obj_data*      container  = 0;
  thing_data*        where = obj;
  Content_Array *last;
  const char *loc = 0;

  while( ( last = where->array ) && ( where = last->where ) ) {
    if( Room( where ) ) {
      room = (room_data*) where;
      break;
    } else if( object( where ) ) {
      container = (obj_data*) where;
    } else if( character( where ) ) {
      owner = (char_data*) where;
      if( last == &owner->contents ) {
	loc = "(inventory)";
      } else if( last == &owner->wearing ) {
	loc = "(worn)";
      } else if( !owner->species && last == &((player_data*)owner)->locker ) {
	loc = "(bank)";
      } else if( !owner->species && last == &((player_data*)owner)->junked ) {
	loc = "(junked)";
      }
      break;
    } else if( Auction( where ) ) {
      loc = "(auction block)";
      break;
      /*
    } else if( Shop_Data *shop = Shop( where ) ) {
      loc = "(shop)";
      break;
      */
    }
  }
  
  char tmp1  [ TWO_LINES ];
  char tmp2  [ TWO_LINES ];
  char tmp3  [ TWO_LINES ];

  strcpy( tmp1, room ? room->name : loc ? loc : "???" );
  strcpy( tmp2, owner ? owner->Name( ) : "" );
  strcpy( tmp3, container ? container->Name( ) : "" );
  
  length += sprintf( tmp+length, "%26s%7d%5s  %-20s %s\n\r",
		     trunc( tmp1, 25 ).c_str(),
		     room ? room->vnum : owner ? owner->in_room->vnum : -1,
		     int4( obj->Number( ) ),
		     trunc( tmp2, 20 ).c_str(),
		     tmp3 );
  //		     trunc( tmp3, 20 ).c_str() );
  
  if( length > MAX_STRING_LENGTH ) {
    page( ch, tmp );
    length = 0;
    *tmp = '\0';
  }
}


void do_owhere( char_data* ch, const char *argument )
{
  char                  tmp  [ 2*MAX_STRING_LENGTH ];
  obj_data*             obj;
  obj_clss_data*   obj_clss;
  int                length  = 0;
  bool                found;
  bool                first;
  bool             anything  = false;
  int                  j = 0;

  if( !*argument ) {
    fsend( ch, "Please supply an argument to owhere; finding all objects creates unacceptable lag." );
    return;
  }

  snprintf( tmp, MAX_STRING_LENGTH, "%26s%7s%5s  %-20s %s\n\r",
	    "Room", "Vnum", "Nmbr", "Carried By", "Container" );
  page_underlined( ch, tmp );

  int vnum;
  if( number_arg( argument, vnum, true ) ) {
    if( obj_clss_data *obj_clss = get_obj_index( vnum ) ) {
      page_divider( ch, obj_clss->Name( ), vnum );
      first = true;
      for( ; j < obj_list && ( first || obj_list[j]->pIndexData == obj_clss ); ++j ) {
	if( obj_list[j]->pIndexData == obj_clss
	    && ( obj = obj_list[j] )->Is_Valid( ) ) {
	  first = false;
	  display_owhere( obj, ch, tmp, length );
	}
      }
      
      if( first ) {
	page_centered( ch, "None found" );
      } else {
	page( ch, tmp );
      } 
      return;
    }

  } else {
    
    for( int i = 1; i <= obj_clss_max; ++i ) {
      if( !( obj_clss = obj_index_list[i] ) )
	continue;
      
      if( ( found = select_owhere( obj_clss, 0, ch, argument ) ) ) {
	page_divider( ch, obj_clss->Name( ), i );
      }
      
      first = true;
      
      for( ; j < obj_list && obj_list[j]->pIndexData == obj_clss; ++j ) {
	if( ( obj = obj_list[j] )->Is_Valid( ) ) {
	  switch( select_owhere( obj_clss, obj, ch, argument ) ) {
	  case -1 : return;
	  case  1 : 
	    if( !found ) {
	      found = true;
	      page_divider( ch, obj_clss->Name( ), i );
	    }
	    first = false;
	    display_owhere( obj, ch, tmp, length );
	  }
	}
      }
      
      if( found ) {
	if( first )
	  page_centered( ch, "None found" );
	else {
	  length = 0;
	  page( ch, tmp );
	} 
	anything = true;
      }
    }
  }

  if( !anything )
    page_centered( ch, "Nothing found" );
}


/*
    Object Instance Functions
*/


#define types 6


static unsigned oiflag_assign( obj_data *obj, int bit, int flag, int val )
{
  int *const flag_value [types] = {
    &obj->size_flags,
    obj->extra_flags,
    &obj->materials,
    &obj->value[1],
    &obj->value[0],
    &obj->value[3]
  };

  if( is_set( flag_value[bit], flag ) != val ) {
    assign_bit( flag_value[bit], flag, val );
    return obj->Number( );
  }

  return 0;
}


static unsigned oiflag_support( char_data *ch, char_data *pl, Content_Array& stuff,
				const obj_clss_data *clss, int bit, int flag, bool val,
				unsigned& count )
{
  if( stuff.is_empty() )
    return 0;

  unsigned fix = 0;

  for( int i = 0; i < stuff; ++i ) {
    if( obj_data *obj = object( stuff[i] ) ) {
      if( obj->pIndexData == clss ) {
	count += obj->Number( );
	fix += oiflag_assign( obj, bit, flag, val );
      }

      fix += oiflag_support( ch, pl, obj->contents, clss, bit, flag, val, count );
    }
  }

  return fix;
}


void do_oiflag( char_data* ch, const char *argument )
{
  wizard_data *imm;
  if( !( imm = wizard( ch ) ) )
    return;

  int flags;
  if( !get_flags( ch, argument, &flags, "SRs", "oiflag" ) )
    return;

  char arg  [ MAX_INPUT_LENGTH ];

  argument = one_argument( argument, arg );

  int section = -1;

  const char *title [types] = { "Size", "Object", "Material",
				"Container", "Trap", "Consume" };
  
  if( *arg && is_set( flags, 2 ) ) {
    // Select section.

    const int l = strlen( arg );

    for( int i = 0; i < types; ++i ) {
      if( !strncasecmp( arg, title[i], l ) ) {
	section = i;
	break;
      }
    }

    argument = one_argument( argument, arg );
  }

  obj_data *obj;
  if( !( obj = one_object( ch, arg, "oiflag",
			   ch->array,
			   &ch->contents,
			   &ch->wearing ) ) )
    return;

  bool display = !*argument;

  obj_clss_data *obj_clss = obj->pIndexData;

  int *flag_value [types] = {
    &obj->size_flags,
    obj->extra_flags,
    &obj->materials,
    &obj->value[1],
    &obj->value[0],
    &obj->value[3]
  };

  int max [types] = { MAX_SFLAG, MAX_OFLAG, table_max[ TABLE_MATERIAL ],
		      MAX_CONT, MAX_TRAP, MAX_CONSUME };
  
  const char** name1 [types] = { &size_flags[0],
				 &oflag_name[0],
				 &material_table[0].name,
				 &cont_flag_name[0], &trap_flags[0], &consume_flags[0] };
  const char** name2 [types] = { &size_flags[1],
				 &oflag_name[1],
				 &material_table[1].name,
				 &cont_flag_name[1], &trap_flags[1], &consume_flags[1] };
  
  const int uses_flag [types] = { 1, 1, 1,
				  obj_clss->item_type == ITEM_CONTAINER,
				  obj_clss->item_type == ITEM_TRAP,
				  ( obj_clss->item_type == ITEM_FOOD
				    || obj_clss->item_type == ITEM_DRINK_CON
				    || obj_clss->item_type == ITEM_FOUNTAIN ) };
  
  const bool sort [ types ] = { false, true, true, true, true, true };

  if( is_set( flags, 2 ) ) {
    if( section == -1 || !uses_flag[section] ) {
      fsend( ch, "Unknown section \"%s\".", arg );
      return;
    }

    if( display ) {
      display_flags( title[section],
		     name1[section], name2[section],
		     flag_value[section], max[section], ch, sort[section] );
      return;
    }

    if( !( is_set( flags, 0 ) || is_set( flags, 1 ) ) ) {
      set_flags( name1[section], name2[section],
		 flag_value[section], 0, max[section],
		 ch->can_edit( obj_clss, false ) ? 0 : no_permission,
		 ch, argument, obj->Name( ),
		 false, true, sort[section] );
      return;
    }
  }

  if( !display && ( is_set( flags, 0 ) || is_set( flags, 1 ) ) ) {
    flag_op op = is_set( flags, 0 ) ? flag_set : flag_remove;

    const char *response;

    if( section != -1 ) {
      response = set_flags( name1[section], name2[section],
			    flag_value[section], 0, max[section],
			    ch->can_edit( obj_clss, false ) ? 0 : no_permission,
			    ch, argument, obj->Name( ),
			    false, true, sort[section] );
    } else {
      response = flag_handler( title, name1, name2,
			       flag_value, 0, max, uses_flag, sort,
			       ch->can_edit( obj_clss, false ) ? 0 : no_permission, 
			       ch, argument, obj->Name( ch ),
			       types, op );
    }

    if( response ) {
      if( *response ) {
	const bool val = is_set( flag_value[last_bit], last_flag );
	// First, fix all existing instances.
	unsigned count = 0, fix = 0;
	for( int i = 0; i < obj_list; ++i ) {
	  obj_data *obj2 = obj_list[i];
	  if( obj2 != obj
	      && obj2->pIndexData == obj_clss ) {
	    count += obj2->Number( );
	    fix += oiflag_assign( obj2, last_bit, last_flag, val );
	  }
	}
	fsend( ch, "Changed %u of %u other existing instances.", fix, count );

	// Fix player files.
	count = fix = 0;
	link_data link;
	link.connected = CON_PLAYING;
	for( int i = 0; i < max_pfile; ++i ) {
	  pfile_data *pfile = pfile_list[i];
	  if( !find_player( pfile ) ) {
	    if( !load_char( &link, pfile->name, PLAYER_DIR ) ) {
	      bug( "Load_players: error reading player file. (%s)", pfile->name );
	      continue;
	    }
	    unsigned fix2 = 0;
	    player_data *pl = link.player;
	    fix2 += oiflag_support( ch, pl, pl->contents,
				    obj_clss, last_bit, last_flag, val, count );
	    fix2 += oiflag_support( ch, pl, pl->wearing,
				    obj_clss, last_bit, last_flag, val, count );
	    fix2 += oiflag_support( ch, pl, pl->locker,
				    obj_clss, last_bit, last_flag, val, count );
	    for( int j = 0; j < pl->followers; ++j ) {
	      char_data *pet = pl->followers[j];
	      fix2 += oiflag_support( ch, pet, pet->contents,
				      obj_clss, last_bit, last_flag, val, count );
	      fix2 += oiflag_support( ch, pet, pet->wearing,
				      obj_clss, last_bit, last_flag, val, count );
	    }
	    if( fix2 != 0 ) {
	      fix += fix2;
	      pl->Save( false );
	    }
	    pl->Extract();
	    extracted.delete_list();
	  }
	}
	fsend( ch, "Changed %u of %u other player file instances.", fix, count );
      }
    }

    return;
  }

  if( !display ) {
    obj = (obj_data *) obj->From( 1, true );
  }

  flag_handler( title, name1, name2,
		flag_value, 0, max, uses_flag, sort,
		ch->can_edit( obj_clss, false ) ? 0 : no_permission, 
		ch, argument, obj->Name( ch ),
		types );

  if( !display )
    obj->To( );
}


#undef types


static void oistat( char_data *ch, obj_data *obj )
{
  page_title( ch, "%s (%d)", obj->Name( ), obj->pIndexData->vnum );

  page( ch, "        Number: %-12d Owner: %s\n\r",
	obj->Number( ),
	obj->owner ? obj->owner->name : "no one" );
  page( ch, "        Weight: %-12.2f Light: %-13d Cost: %d\n\r",
	(double)obj->weight/100.0,
	obj->light,
	obj->cost );
  page( ch, "     Condition: %-14d Age: %-13d Rust: %d\n\r",
	obj->condition, obj->age, obj->rust );
  page( ch, "        Values: (0) %d, (1) %d, (2) %d, (3) %d\n\r",
	obj->value[0], obj->value[1], obj->value[2], obj->value[3] );

  if( obj->timer > 0 )
    page( ch, "         Timer: %d\n\r",
	  obj->timer ); 
  if( obj->reset ) {
    page( ch, "         Reset: %s\n\r", name( obj->reset ) );
  }

  switch( obj->pIndexData->item_type ) {
  case ITEM_WEAPON :
    page( ch, "\n\r" );
    page( ch, "         Class: %-11s Attack: %s\n\r",
	  weapon_class( obj ),
	  ( obj->value[3] >= 0 && obj->value[3] < table_max[TABLE_SKILL_WEAPON] ) ?
	  skill_weapon_table[ obj->value[3] ].noun[0] : "none" );
    page( ch, "        Damage: %-26s Enchantment: %d\n\r",
	  dice_string( obj->value[1] ),
	  obj->value[0] );
    break;
    
  case ITEM_TRAP   :
    page( ch, "\n\r" );
    page( ch, "        Damage: %-26s Enchantment: %d\n\r",
	  dice_string( obj->value[1] ),
	  obj->value[0] );
    break;
      
  case ITEM_ARROW :
    page( ch, "\n\r" );
    page( ch, "         Class: %-11s Attack: %s\n\r",
	  weapon_class( obj ),
	  ( obj->value[3] >= 0 && obj->value[3] < table_max[TABLE_SKILL_WEAPON] ) ?
	  skill_weapon_table[ obj->value[3] ].noun[0] : "none" );
    page( ch, "        Damage: %-26s Enchantment: %d\n\r",
	  dice_string( obj->value[1] ),
	  obj->value[0] );
    break;
    
  case ITEM_SCROLL :
    {
      const int spell = obj->value[0];
      const char *name = "unknown";
      if( spell >= 0 && spell < table_max[TABLE_SKILL_SPELL] ) {
	name = skill_spell_table[spell].name;
      }
      page( ch, "\n\r" );
      page( ch, "    Cast Level: %-9d Duration: %-12d Spell: %s\n\r",
	    obj->value[1], obj->value[2], name );
    }
    break;

  case ITEM_POTION :
    {
      const int spell = obj->value[0];
      const char *name = "unknown";
      obj_clss_data *empty;
      if( spell >= 0 && spell < table_max[TABLE_SKILL_SPELL] ) {
	name = skill_spell_table[spell].name;
      }
      page( ch, "\n\r" );
      page( ch, "    Cast Level: %-9d Duration: %-12d Spell: %s\n\r",
	    obj->value[1], obj->value[2], name );
      page( ch, "         Empty: %d (%s)\n\r",
	    obj->value[3],
	    !( empty = get_obj_index( obj->value[3] ) )
	    ? "none" : empty->Name( ) );
    }
    break;
      
  case ITEM_WAND :
    {
      const int spell = obj->value[0];
      const char *name = "unknown";
      if( spell >= 0 && spell < table_max[TABLE_SKILL_SPELL] ) {
	name = skill_spell_table[spell].name;
      }
      page( ch, "\n\r" );
      page( ch, "    Cast Level: %-9d Duration: %-12d Spell: %s\n\r",
	    obj->value[1], obj->value[2], name );
      page( ch, "       Charges: %d\n\r",
	    obj->value[3] );
    }
    break;
      
  case ITEM_ARMOR :
    page( ch, "\n\r" );
    page( ch, "            AC: %-8d Global AC: %-6d Enchantment: %d\n\r",
	  obj->value[1], obj->value[2], obj->value[0] );
    break;

  case ITEM_CONTAINER :   
    {
      obj_clss_data*         key;
      page( ch, "\n\r" );
      page( ch, "      Capacity: %-14d Key: %d (%s)\n\r",
	    obj->value[0], obj->value[2],
	    !( key = get_obj_index( obj->value[2] ) )
	    ? "none" : key->Name( ) );
    }
    break;

  case ITEM_TABLE :   
    page( ch, "\n\r" );
    page( ch, "      Capacity: %d\n\r",
	  obj->value[0] );
    break;

  case ITEM_KEYRING :   
    page( ch, "\n\r" );
    page( ch, "      Capacity: %d\n\r",
	  obj->value[0] );
    break;

  case ITEM_CHAIR :   
    page( ch, "\n\r" );
    page( ch, "         Seats: %d\n\r",
	  obj->value[0] );
    break;

  case ITEM_FIRE :
  case ITEM_LIGHT :   
  case ITEM_LIGHT_PERM :   
    page( ch, "\n\r" );
    page( ch, "      Lifetime: %d\n\r",
	  obj->value[0] );
    break;

  case ITEM_DRINK_CON :
    {
      int liquid = obj->value[2];
      page( ch, "\n\r" );
      page( ch, "      Capacity: %-10d Contains: %-10d Liquid: %s\n\r",
	    obj->value[0], obj->value[1],
	    ( liquid < 0 || liquid > table_max[ TABLE_LIQUID ] ) ?
	    "unknown" : liquid_name( liquid, true ) );
    }
    break;

  case ITEM_FOUNTAIN :
    {
      int liquid = obj->value[2];
      page( ch, "\n\r" );
      page( ch, "        Liquid: %s\n\r",
	    ( liquid < 0 || liquid > table_max[ TABLE_LIQUID ] ) ?
	    "unknown" : liquid_name( liquid, true ) );
    }
    break;

  case ITEM_REAGENT :
    page( ch, "\n\r" );
    page( ch, "       Charges: %d\n\r", obj->value[0] );
    break;

  case ITEM_GATE :
    {
      room_data *to;
      page( ch, "\n\r" );
      page( ch, "      Lifetime: %-15d To: %d (%s)\n\r",
	    obj->value[0],
	    obj->value[1],
	    !( to = get_room_index( obj->value[1], false ) )
	    ? "none" : to->name );
    }
    break;

  case ITEM_LOCK_PICK :
    page( ch, "\n\r" );
    page( ch, "      Modifier: %d\n\r",
	  obj->value[0] );
    break;

  case ITEM_FOOD :
    {
      int cooked = obj->value[1] + 1;
      if( cooked < 0 || cooked > MAX_COOK ) cooked = 0;
      page( ch, "\n\r" );
      page( ch, "   Nourishment: %-11d Cooked: %-11s\n\r",
	    obj->value[0], cook_word[ cooked ] );
    }
    break;

  case ITEM_GEM:
    {
      int quality = obj->value[1] + 1;
      if( quality > MAX_GEM ) quality = 1;
      else if( quality < 0 ) quality = 0;
      page( ch, "\n\r" );
      page( ch, "       Quality: %s\n\r",
	    gem_quality[ quality ]
	    );
    }
    break;

  case ITEM_CORPSE:
    page( ch, "\n\r" );
    page( ch, "      Lifetime: %d\n\r",
	  obj->value[0] );
    if( obj->pIndexData->vnum != OBJ_CORPSE_PC
	&& obj->pIndexData->vnum != OBJ_CORPSE_PET ) {
      species_data *species = get_species( obj->value[1] );
      page( ch, "       Species: %-5d (%s)\n\r",
	    obj->value[1], species ? species->Name( ) : "none" ); 
    }
    break;
  
  case ITEM_WHISTLE:
    page( ch, "\n\r" );
    page( ch, "         Range: %d\n\r",
	  obj->value[0] );
    break;
    
  case ITEM_GARROTE:
    page( ch, "\n\r" );
    page( ch, "        Damage: %s\n\r",
	  dice_string( obj->value[1] ) );
    break;
  }

  page( ch, "\n\rCalculated Values:\n\r" );
  page( ch, "         Level: %-12d Value: %-11d Weight: %.2f\n\r",
	obj->Level( ), obj->Cost( ),
	(double)obj->Weight( 1 )/100.0 );
  int rc = repair_condition( obj );
  page( ch, "    Durability: %-7d Max Repair: %d (%s be repaired)\n\r",
	obj->Durability( ),
	rc,
	obj->Damaged( )
	? "can"
	: "cannot" );

  if( !obj->affected.is_empty( ) ) {
    page( ch, "\n\rAffects:\n\r" );
    for( int i = 0; i < obj->affected; i++ ) {
      affect_data *paf = obj->affected[i];
      if( paf->type == AFF_NONE )  
	page( ch, "  %s by %d.\n\r",
	      affect_location[ paf->location ], paf->modifier );
    }
  }
  page( ch, "\n\r" );
  page( ch, "Singular: %s\n\r", obj->singular );
  page( ch, "  Plural: %s\n\r", obj->plural );
  page( ch, "  Before: %s\n\r", obj->before );
  page( ch, "   After: %s\n\r", obj->after );
}


void do_oistat( char_data* ch, const char *argument )
{
  char           arg  [ MAX_INPUT_LENGTH ];

  wizard_data *imm;
  if( !( imm = wizard( ch ) ) )
    return;

  argument = one_argument( argument, arg );

  obj_data *obj;
  if( !( obj = one_object( ch, arg, "oistat",
			   ch->array,
			   &ch->contents,
			   &ch->wearing ) ) )
    return;

  oistat( ch, obj );
}


static bool oiset_drink_container( char_data* ch, obj_data* obj, const char *argument )
{
  class int_field int_list[] = {
    { "capacity",        -1,    10000,   &obj->value[0]       },
    { "contains",        -1,    10000,   &obj->value[1]       },
    { "",                 0,        0,   0                    }
  };

  if( process( int_list, ch, argument, obj ) )
    return true;

#define ltn( i )   liquid_table[i].name
  class type_field type_list[] = {
    { "liquid",    table_max[ TABLE_LIQUID ],  &ltn(0),  &ltn(1),  &obj->value[2], true  },
    { "" }
  };
#undef ltn

  return process( type_list, ch, argument, obj );
}


static bool oiset_fountain( char_data* ch, obj_data* obj, const char *argument )
{
#define ltn( i )   liquid_table[i].name
  class type_field type_list[] = {
    { "liquid",    table_max[ TABLE_LIQUID ],  &ltn(0),  &ltn(1),  &obj->value[2], true  },
    { "" }
  };
#undef ltn

  return process( type_list, ch, argument, obj );
}


static bool oiset_scroll( char_data* ch, obj_data* obj, const char *argument )
{
  class int_field int_list[] = {
    { "cast level",       1, MAX_SPELL_LEVEL,   &obj->value[1]       },
    { "duration",         0,    10000,   &obj->value[2]       },
    { "",                 0,        0,   0                    }
  };

  if( process( int_list, ch, argument, obj ) )
    return true;

#define stn( i )   skill_spell_table[i].name
  class type_field type_list[] = {
    { "spell", table_max[TABLE_SKILL_SPELL],  &stn(0),  &stn(1),  &obj->value[0], true  },
    { "" }
  };
#undef stn

  return process( type_list, ch, argument, obj );
}


static bool oiset_potion( char_data* ch, obj_data* obj, const char *argument )
{
  if( obj_clss_arg( ch, argument, 0, obj, "empty", obj->value[3] ) ) {
    return true;
  }

  class int_field int_list[] = {
    { "cast level",       1, MAX_SPELL_LEVEL,   &obj->value[1]       },
    { "duration",         0,    10000,   &obj->value[2]       },
    { "",                 0,        0,   0                    }
  };

  if( process( int_list, ch, argument, obj ) )
    return true;

#define stn( i )   skill_spell_table[i].name
  class type_field type_list[] = {
    { "spell", table_max[TABLE_SKILL_SPELL],  &stn(0),  &stn(1),  &obj->value[0], true  },
    { "" }
  };
#undef stn

  return process( type_list, ch, argument, obj );
}


static bool oiset_wand( char_data* ch, obj_data* obj, const char *argument )
{
  class int_field int_list[] = {
    { "cast level",       1, MAX_SPELL_LEVEL,   &obj->value[1]       },
    { "duration",         0,    10000,   &obj->value[2]       },
    { "charges",         -1,     1000,   &obj->value[3]       },
    { "",                 0,        0,   0                    }
  };

  if( process( int_list, ch, argument, obj ) )
    return true;

#define stn( i )   skill_spell_table[i].name
  class type_field type_list[] = {
    { "spell", table_max[TABLE_SKILL_SPELL],  &stn(0),  &stn(1),  &obj->value[0], true  },
    { "" }
  };
#undef stn

  return process( type_list, ch, argument, obj );
}


static bool oiset_container( char_data* ch, obj_data* obj, const char *argument )
{
  class int_field int_list[] = {
    { "capacity",         0,    10000,   &obj->value[0]       },
    { "",                 0,        0,   0                    }
  };

  if( process( int_list, ch, argument, obj ) )
    return true;

  if( obj_clss_arg( ch, argument, 0, obj, "key", obj->value[2] ) ) {
    obj_clss_data *key = get_obj_index( obj->value[2] );
    if( key && key->item_type != ITEM_KEY ) {
      send( ch, "Warning: %s is not a key.\n\r", key );
    }
    return true;
  }

  return false;
}


static bool oiset_table( char_data* ch, obj_data* obj, const char *argument )
{
  class int_field int_list[] = {
    { "capacity",         0,    10000,   &obj->value[0]       },
    { "",                 0,        0,   0                    }
  };

  return process( int_list, ch, argument, obj );
}


static bool oiset_keyring( char_data* ch, obj_data* obj, const char *argument )
{
  class int_field int_list[] = {
    { "capacity",         0,    10000,   &obj->value[0]       },
    { "",                 0,        0,   0                    }
  };

  return process( int_list, ch, argument, obj );
}


static bool oiset_chair( char_data* ch, obj_data* obj, const char *argument )
{
  class int_field int_list[] = {
    { "seats",         0,    100,   &obj->value[0]       },
    { "",                 0,        0,   0                    }
  };

  return process( int_list, ch, argument, obj );
}


static bool oiset_light( char_data* ch, obj_data* obj, const char *argument )
{
  class int_field int_list[] = {
    { "lifetime",         -1,    1000,   &obj->value[0]       },
    { "",                 0,        0,   0                    }
  };

  return process( int_list, ch, argument, obj );
}


static bool oiset_fire( char_data* ch, obj_data* obj, const char *argument )
{
  class int_field int_list[] = {
    { "lifetime",         -1,    1000,   &obj->value[0]       },
    { "",                 0,        0,   0                    }
  };

  return process( int_list, ch, argument, obj );
}


static bool oiset_gate( char_data* ch, obj_data* obj, const char *argument )
{
  class int_field int_list[] = {
    { "lifetime",         -1,    1000,      &obj->value[0]       },
    { "to",               -1,    1000000,   &obj->value[1]       },
    { "",                 0,        0,   0                    }
  };

  return process( int_list, ch, argument, obj );
}


static bool oiset_lock_pick( char_data* ch, obj_data* obj, const char *argument )
{
  class int_field int_list[] = {
    { "modifier",         -100,    100,   &obj->value[0]       },
    { "",                 0,        0,   0                    }
  };

  return process( int_list, ch, argument, obj );
}


static bool oiset_reagent( char_data* ch, obj_data* obj, const char *argument )
{
  class int_field int_list[] = {
    { "charges",         -1,      100,   &obj->value[0]       },
    { "",                 0,        0,   0                    }
  };

  return process( int_list, ch, argument, obj );
}


static bool oiset_food( char_data* ch, obj_data* obj, const char *argument )
{
  class int_field int_list[] = {
    { "nourishment",      0,      100,   &obj->value[0]       },
    { "",                 0,        0,   0                    }
  };

  if( process( int_list, ch, argument, obj ) )
    return true;

  class type_field type_list[] = {
    { "cooked", MAX_COOK+1, cook_word,  cook_word+1,  &obj->value[1], true },
    { "" }
  };

  ++obj->value[1];
  if( process( type_list, ch, argument, obj ) ) {
    --obj->value[1];
    return true;
  }
  --obj->value[1];

  return false;
}


static bool oiset_gem( char_data* ch, obj_data* obj, const char *argument )
{
  class type_field type_list[] = {
    { "quality", MAX_GEM+1, gem_quality,  gem_quality+1,  &obj->value[1], true },
    { "" }
  };

  ++obj->value[1];
  if( process( type_list, ch, argument, obj ) ) {
    --obj->value[1];
    return true;
  }
  --obj->value[1];

  return false;
}


static bool oiset_armor( char_data* ch, obj_data* obj, const char *argument )
{
  class int_field int_list[] = {
    { "enchantment",     -5,        5,   &obj->value[0]       },
    { "ac",             -50,       50,   &obj->value[1]       },
    { "global ac",      -50,       50,   &obj->value[2]       },
    { "",                 0,        0,   0                    }
  };

  return process( int_list, ch, argument, obj );
}


static bool oiset_corpse( char_data* ch, obj_data* obj, const char *argument )
{
  class int_field int_list[] = {
    { "lifetime",   0,  QUEUE_LENGTH-1,   &obj->value[0]       },
    { "species",    0,     species_max,   &obj->value[1]       },
    { "",           0,               0,   0                    }
  };

  return process( int_list, ch, argument, obj );
}


static bool oiset_weapon( char_data* ch, obj_data* obj, const char *argument )
{
  class int_field int_list[] = {
    { "enchantment",     -5,        5,   &obj->value[0]       },
    { "",                 0,        0,   0                    }
  };

  if( process( int_list, ch, argument, obj ) )
    return true;

  class dice_field dice_list[] = {
    { "damage", LEVEL_OBJECT, &obj->value[1] },
    { "",                 -1,              0 }
  };

  if( process( dice_list, ch, argument, obj ) )
    return true;

#define wn(i) skill_weapon_table[ i ].name

  class type_field type_list[] = {
    { "class", table_max[TABLE_SKILL_WEAPON], &wn(0),  &wn(1),  &obj->value[3], true },
    { "" }
  };

#undef wn

  return process( type_list, ch, argument, obj );
}


static bool oiset_trap( char_data* ch, obj_data* obj, const char *argument )
{
  class dice_field dice_list[] = {
    { "damage", LEVEL_OBJECT, &obj->value[1] },
    { "",                 -1,              0 }
  };

  return process( dice_list, ch, argument, obj );
}


static bool oiset_arrow( char_data* ch, obj_data* obj, const char *argument )
{
  class int_field int_list[] = {
    { "enchantment",     -5,        5,   &obj->value[0]       },
    { "",                 0,        0,   0                    }
  };

  if( process( int_list, ch, argument, obj ) )
    return true;

  class dice_field dice_list[] = {
    { "damage", LEVEL_OBJECT, &obj->value[1] },
    { "",                 -1,              0 }
  };

  if( process( dice_list, ch, argument, obj ) )
    return true;

#define wn(i) skill_weapon_table[ i ].name

  class type_field type_list[] = {
    { "class", table_max[TABLE_SKILL_WEAPON], &wn(0),  &wn(1),  &obj->value[3], true },
    { "" }
  };

#undef wn

  return process( type_list, ch, argument, obj );
}


static bool oiset_whistle( char_data* ch, obj_data* obj, const char *argument )
{
  class int_field int_list[] = {
    { "range",            0,      100,   &obj->value[0]       },
    { "",                 0,        0,   0                    }
  };
  
  return process( int_list, ch, argument, obj );
}


static bool oiset_garrote( char_data* ch, obj_data* obj, const char *argument )
{
  class dice_field dice_list[] = {
    { "damage", LEVEL_OBJECT, &obj->value[1] },
    { "",                 -1,              0 }
  };

  return process( dice_list, ch, argument, obj );
}


static void update_affect( obj_data *obj, affect_data *paf, bool add )
{
  /*
    obj_data::affected doesn't currently do anything, so no
    active affects need to be updated.
  */
}


static void oiset_affect( char_data* ch, obj_data* obj, const char *argument )
{
  int                  i;
  int                col;
  int                num;
  
  if( !*argument ) {
    send( ch, "Affect Types:\n\r" );
    for( i = 1, col = 0; i < MAX_AFF_LOCATION; i++ ) {
      send( ch, "%15s", affect_location[i] );
      if( ++col%3 == 0 )
        send( ch, "\n\r" );
    }
    if( col%3 != 0 )
      send( ch, "\n\r" );
    return;
  }
  
  affect_data *paf  = 0;
  int j;
  for( i = 1; i < MAX_AFF_LOCATION; i++ ) {
    if( matches( argument, affect_location[i] ) ) {
      for( j = 0; j < obj->affected; j++ ) {
        if( obj->affected[j]->location == i ) {
          paf = obj->affected[j];
          break;
	}
      }
      if( ( num = atoi( argument ) ) == 0 ) {
        if( paf ) {
	  fsend( ch, "%s modifier removed from object: %s.", 
		 affect_location[i], obj->Name( ) );
	  update_affect( obj, paf, false );
          obj->affected.remove(j);
	} else {
	  fsend( ch, "%s modifier has not been set.", 
		 affect_location[i] );
	}
      } else {
        if( !paf ) {
	  fsend( ch, "%s modifier of %d added to object: %s.", 
		 affect_location[i], num, obj->Name( ) );
          paf = new affect_data;
          paf->type = AFF_NONE;
          paf->duration = -1;
          paf->location = i;
	  paf->modifier = num;
          obj->affected += paf;
	  update_affect( obj, paf, true );
	} else if( paf->modifier != num ) {
	  fsend( ch, "%s modifier changed to %d on object: %s.", 
		 affect_location[i], num, obj->Name( ) );
	  paf->modifier -= num;
	  update_affect( obj, paf, false );
	  paf->modifier = num;
	} else {
	  fsend( ch, "%s modifier is already %d.", 
		 affect_location[i], num );
	}
      }
      return;
    }
  }
  
  send( ch, "Unknown affect location.\n\r" );
}


void do_oiset( char_data* ch, const char *argument )
{
  char           arg  [ MAX_INPUT_LENGTH ];

  wizard_data *imm;
  if( !( imm = wizard( ch ) ) )
    return;

  int flags;
  if( !get_flags( ch, argument, &flags, "a", "oiset" ) )
    return;

  argument = one_argument( argument, arg );

  obj_data *obj;
  if( !( obj = one_object( ch, arg, "oiset",
			   ch->array,
			   &ch->contents,
			   &ch->wearing ) ) )
    return;

  if( is_set( flags, 0 ) ) {
    obj->Select_All( );
  }

  if( !*argument ) {
    oistat( ch, obj );
    return;
  }

  if( !ch->can_edit( obj->pIndexData ) ) {
    return;
  }

  obj = (obj_data *) obj->From( obj->Selected( ), true );

  {
    class int_field int_list[] = {
      { "value0",     INT_MIN,  INT_MAX,   &obj->value[0]    },
      { "value1",     INT_MIN,  INT_MAX,   &obj->value[1]    },
      { "value2",     INT_MIN,  INT_MAX,   &obj->value[2]    },
      { "value3",     INT_MIN,  INT_MAX,   &obj->value[3]    },
      { "light",         -100,      100,   &obj->light       },
      { "condition",        0,     1000,   &obj->condition   },
      { "age",              0,     1000,   &obj->age         },
      { "rust",             0,        3,   &obj->rust        },
      { "timer",            0, 1000000000, &obj->timer       },
      { "cost",             0,  1000000,   &obj->cost        },
      { "",                 0,        0,   0                 }
    };
    
    if( process( int_list, ch, argument, obj ) ) {
      obj->To( );
      return;
    }
  }

  {
    class cent_field cent_list[] = {
      { "weight",           0,  1000000,   &obj->weight      },
      { "",                 0,        0,   0                      },
    };
    
    if( process( cent_list, ch, argument, obj ) ) {
      obj->To( );
      return;
    }
  }

  {
    const char *word[] = { "singular", "plural", "after", "before" };
    
    char **const pChar[] = { &obj->singular, &obj->plural,
			     &obj->after, &obj->before };
    
    for( int i = 0; i < 4; ++i ) {
      if( matches( argument, word[i] ) ) {
	send( ch, "The %s of %s is now:\n\r%s\n\r",
	      word[i], obj, argument );
	if( strcmp( *pChar[i], argument ) ) {
	  char *string = alloc_string( argument, MEM_OBJECT );
	  *pChar[i] = string;
	  obj->To( );
	  return;
	}
      }
    }
  }

  if( matches( argument, "owner" ) ) {
    if( !*argument ) {
      send( ch, "Set owner of %s to?\n\r[ Current Value: %s ]\n\r",
	    obj, obj->owner->name );
    } else if( !strcasecmp( "none", argument ) ) {
      send( ch, "Owner of %s set to none.\n\r", obj );
      obj->owner = 0;
    } else if( pfile_data *pfile = find_pfile( argument, ch ) ) {
      send( ch, "Owner of %s set to %s.\n\r", obj, pfile->name );
      obj->owner = pfile;
    }
    obj->To( );
    return;
  }

  switch( obj->pIndexData->item_type ) {
  case ITEM_WEAPON :
    if( oiset_weapon( ch, obj, argument ) ) {
      obj->To( );
      return;
    }
    break;
  case ITEM_ARROW :
    if( oiset_arrow( ch, obj, argument ) ) {
      obj->To( );
      return;
    }
    break;
  case ITEM_ARMOR :
    if( oiset_armor( ch, obj, argument ) ) {
      obj->To( );
      return;
    }
    break;
  case ITEM_CONTAINER :
    if( oiset_container( ch, obj, argument ) ) {
      obj->To( );
      return;
    }
    break;
  case ITEM_TABLE :
    if( oiset_table( ch, obj, argument ) ) {
      obj->To( );
      return;
    }
    break;
  case ITEM_KEYRING :
    if( oiset_keyring( ch, obj, argument ) ) {
      obj->To( );
      return;
    }
    break;
  case ITEM_CHAIR :
    if( oiset_chair( ch, obj, argument ) ) {
      obj->To( );
      return;
    }
    break;
  case ITEM_REAGENT :
    if( oiset_reagent( ch, obj, argument ) ) {
      obj->To( );
      return;
    }
    break;
  case ITEM_FOOD :
    if( oiset_food( ch, obj, argument ) ) {
      obj->To( );
      return;
    }
    break;
  case ITEM_DRINK_CON :
    if( oiset_drink_container( ch, obj, argument ) ) {
      obj->To( );
      return;
    }
    break;
  case ITEM_FOUNTAIN :
    if( oiset_fountain( ch, obj, argument ) ) {
      obj->To( );
      return;
    }
    break;
  case ITEM_CORPSE :
    if( oiset_corpse( ch, obj, argument ) ) {
      obj->To( );
      return;
    }
    break;
  case ITEM_GEM :
    if( oiset_gem( ch, obj, argument ) ) {
      obj->To( );
      return;
    }
    break;
  case ITEM_LOCK_PICK :
    if( oiset_lock_pick( ch, obj, argument ) ) {
      obj->To( );
      return;
    }
    break;
  case ITEM_TRAP :
    if( oiset_trap( ch, obj, argument ) ) {
      obj->To( );
      return;
    }
    break;
  case ITEM_LIGHT :   
  case ITEM_LIGHT_PERM :   
    if( oiset_light( ch, obj, argument ) ) {
      obj->To( );
      return;
    }
    break;
  case ITEM_SCROLL:
    if( oiset_scroll( ch, obj, argument ) ) {
      obj->To( );
      return;
    }
    break;
  case ITEM_POTION :
    if( oiset_potion( ch, obj, argument ) ) {
      obj->To( );
      return;
    }
    break;
  case ITEM_WAND :
    if( oiset_wand( ch, obj, argument ) ) {
      obj->To( );
      return;
    }
    break;
  case ITEM_FIRE :
    if( oiset_fire( ch, obj, argument ) ) {
      obj->To( );
      return;
    }
    break;

  case ITEM_GATE :
    if( oiset_gate( ch, obj, argument ) ) {
      obj->To( );
      return;
    }
    break;
  case ITEM_WHISTLE :
    if( oiset_whistle( ch, obj, argument ) ) {
      obj->To( );
      return;
    }
    break;
  case ITEM_GARROTE:
    if( oiset_garrote( ch, obj, argument ) ) {
      obj->To( );
      return;
    }
    break;
  default:
    break;
  }

  if( matches( argument, "affect" ) ) {
    oiset_affect( ch, obj, argument );
    obj->To( );
    return;
  }
  
  obj->To( );
  send( ch, "oiset: Unknown field.\n\r" );
}
