#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <limits.h>
#include "define.h"
#include "struct.h"


const char *plr_name [MAX_PLR] = {
  // Player opts.
  "Auto.Assist", "Auto.Exit",
  "Newbie", "No.Give", "Auto.Skin", "Auto.Split", "Brief", "Chant",
  "Chat", "Ctell", "Email.Public", "Gossip", "Group.Incog", "Numeric",
  "No.Intro", "No.Wake", "Info", "Lang.Id", "Join.Fight", "No.Summon", "Ooc",
  "Parry", "Pet.Assist", "Editor.Quiet", "Reverse", "Window.Name", "Say.Repeat",
  "Info.Time","Searching", "No.Leech", "Status.Bar", "Track",
  "Portal", "Atalk", "No.Copper", "??", "??",
  // Imm opts below.
  "No.Shout", "No.Tell",
  "Deny", "Freeze", "No.Auction", "Familiar", "No.Notes", "??",
  "Is.Approved", "??", "Remort", "Crash.Quit", "Avatar.Chnnl",
  "Build.Chnnl", "Wizinvis", "Holylight", "Logged", "Imm.Chnnl",
  "Admin.Chnnl", "No.Privileges", "No.Mail", "No.Emote" };

const char *act_name [MAX_ACT] = {
  "No_Scan", "Sentinel", "Scavenger", "Can_Tame",
  "Aggressive", "Stay_Area", "Wimpy", "Humanoid", "Warm_blooded",
  "Summonable", "Assist_Group", "Can_Fly", "Rest_Regen", "Mount",
  "Open_Doors", "Can_Carry", "Has_Eyes", "Has_Skeleton", "Ghost",
  "Zero_Regen", "Aggr_Lawful", "Aggr_Chaotic", "No_Bash", "Mimic", "Stay_Terrain",
  "No_Trip", "Aggr_Evil", "Aggr_Good", "Carnivore", "Elemental",
  "Use.The", "Predator", "Follow_Flee" };

static const char *const no_permission =
"You do not have permission to alter that species.\n\r";

static const char *const xp_reset = "Mob experience reset.\n\r";

bool mobs_modified  = true;


int   select         ( species_data*, char_data*, const char * );
void  display        ( species_data*, char_data*, char*, int& );
void  medit_replace  ( char_data*, const char * );

/*
 *   MLOAD FUNCTION
 */


void do_mload( char_data* ch, const char *argument )
{
  const char *const syntax = "Syntax: mload [-n <count>] <vnum>.\n\r";
  
  if( !*argument ) {
    send( ch, syntax );
    return;
  }

  int flags;
  if( !get_flags( ch, argument, &flags, "n", "mload" ) )
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

  species_data*  species;
  if( !( species = get_species( vnum ) ) ) {
    send( ch, "No mob has that vnum.\n\r" );
    return;
  }

  int trust = get_trust( ch );

  if( !is_demigod( ch )
      && num * species->shdata->level > trust ) {
    send( ch, "You are limited to loading %d mob levels at a time.\n\r", trust );
    return;
  }

  thing_array mobs;

  mob_data *mob = 0;  // For compiler warning.
  for( int i = 0; i < num; ++i ) {
    mob = new Mob_Data( species );
    mreset_mob( mob );
    mob->To( *ch->array );
    mobs += mob;
  }
  
  fsend( ch, "You create %s.", &mobs );
  fsend_seen( mob, "%s suddenly appear%s!", &mobs,
	      num > 1 ? "" : "s" );
}


/*
 *   MFIND ROUTINES
 */


int select( species_data* species, char_data* ch, const char *argument )
{
  char               tmp  [ MAX_INPUT_LENGTH ];
  char            letter;
  char            hyphen;
  const char*     string;
  int                  i;
  int             length;
  int           min, max;
  mprog_data  *mprog = 0;

  while( true ) {
    if( !( hyphen = *argument ) )
      return 1;
    
    if( hyphen != '-' ) {
      letter = 'n';
    } else {
      ++argument;
      if( !isalpha( letter = *argument++ ) ) {
        send( ch, "Illegal character for flag - See help mfind.\n\r" );
        return -1;
      }
    }
    
    bool negative = false;
    skip_spaces( argument );
    
    if( *argument == '!' ) {
      negative = true;
      ++argument;
    }

    if( *argument == '-' || isspace( *argument ) || !*argument ) {
      send( ch, "All flags require an argument - See help mfind.\n\r" );
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
    length = strlen( tmp );

    switch( letter ) {
    case 'g' :  string = group_table[species->group].name;        break;
    case 'c' :  string = species->creator;                        break;
    case 'r' :  string = race_table[species->shdata->race].name;  break; 
    case 'N' :  string = nation_table[species->nation].name;      break;
    case 'm' :
      if( species->movement >= 0 )
	string = movement_table[species->movement].name;
      else
	string = "none";
      break;
    }

    if( string ) {
      if( !strncasecmp( tmp, string, length ) == negative )
        return 0;
      continue;
    }
    
    if( letter == 'n' ) {
      if( !is_name( tmp, species->Name( true, false, false ) ) ^ negative )
        return 0;
      continue;
    }
    
    if( letter == 'l' ) {
      atorange( tmp, min, max );
      if( max < 0 )
	max = INT_MAX;
      if( negative
	  ^ ( species->shdata->level < min
	      || species->shdata->level > max ) )
	  return 0;
      continue;
    }

    if( letter == 'w' ) {
      for( i = 0; i < MAX_WEAR; i++ ) 
        if( !strncasecmp( tmp, wear_part_name[i], length ) ) {
          if( is_set( species->wear_part, i ) == negative )
            return 0;
          break;
	}
      if( i == MAX_WEAR ) {
	send( ch, "Unknown wear part \"%s\".\n\r", tmp );
	return -1;
      }
      continue;
    }

    if( letter == 'f' ) {
      for( i = 0; i < MAX_ACT; i++ ) 
        if( !strncasecmp( tmp, act_name[i], length ) ) {
          if( is_set( species->act_flags, i ) == negative )
            return 0;
          break;
	}
      if( i == MAX_ACT ) {
	send( ch, "Unknown act flag \"%s\".\n\r", tmp );
	return -1;
      }
      continue;
    }
    
    if( letter == 'a' ) {
      for( i = 0; i < table_max[ TABLE_AFF_CHAR ]; i++ ) 
        if( !strncasecmp( tmp, aff_char_table[i].name, length ) ) {
          if( species->is_affected( i ) == negative )
            return 0;
          break;
	}
      if( i == table_max[ TABLE_AFF_CHAR ] ) {
	send( ch, "Unknown affect \"%s\".\n\r", tmp );
	return -1;
      }
      continue;
    }
    
    if( letter == 'T' ) {
      for( i = 0; !fmatches( tmp, mprog_trigger[i] ); ++i ) {
        if( i == MAX_MPROG_TRIGGER-1 ) {
          send( ch, "Unknown trigger type, see help mfind.\n\r" );
          return -1;
	}
      }
      for( mprog = species->mprog; mprog && mprog->trigger != i; mprog = mprog->next );
      if( ( mprog != 0 ) != negative )
        continue;
      return 0;
    }
    
    if( letter == 'V' ) {
      if( !mprog ) {
	send( ch, "Flag 'V' requires a successful 'T' trigger search.\n\r" );
	return -1;
      }
      if( ( mprog->value == atoi( tmp ) ) ^ negative ) {
	continue;
      }
      return 0;
    }

    send( ch, "Unknown flag - See help mfind.\n\r" );
    return -1;
  }
}


void display( species_data* species, char_data* ch, char* buf, int& length )
{
  const int hp = dice_data( species->hitdice ).average( );
  
  const char *name = species->Name( true, false, false );

  if( strlen( name ) > 47 ) {
    length += snprintf( buf+length, MAX_STRING_LENGTH-length, "[%5d] %-71s\n\r",
			species->vnum, trunc( name, 71 ).c_str( ) );
    //    length += strlen( buf+length );
    length += snprintf( buf+length, MAX_STRING_LENGTH-length, "%-55s %5d %5s %5d %5d\n\r",
			"",
			species->shdata->level,
			species->shdata->deaths == 0 ? "??"
			: int5( species->exp/species->shdata->deaths ),
			hp, species->shdata->deaths );
  } else {
    length += snprintf( buf+length, MAX_STRING_LENGTH-length, "[%5d] %-47s %5d %5s %5d %5d\n\r",
			species->vnum, name,
			species->shdata->level,
			species->shdata->deaths == 0 ? "??"
			: int5( species->exp/species->shdata->deaths ),
			hp, species->shdata->deaths );
  }
  //  length += strlen( buf+length );
  
  if( length > MAX_STRING_LENGTH-200 ) {
    page( ch, buf );
    length  = 0;
    *buf = '\0';
  }        
}


void do_mfind( char_data* ch, const char *argument )
{
  const char *const title_msg =
    "Vnum    Name                                            Level   Exp    Hp  Dths\n\r";

  char                buf  [ MAX_STRING_LENGTH ];
  int              length  = 0;
  unsigned count = 0;

  for( int i = 1; i <= species_max; ++i ) {
    if( species_data *species = species_list[i] ) {
      switch( select( species, ch, argument ) ) {
       case -1 : return;
       case  1 :
        if( count == 0 ) {
	  page( ch, "\n\r" );
          page_underlined( ch, title_msg );
	}
	++count;
        display( species, ch, buf, length );
      }
    }
  }
  
  if( count == 0 ) 
    send( ch, "No creature matching search was found.\n\r" );
  else {
    page( ch, buf );
    page( ch, "\n\rFound %d match%s.\n\r",
	  count,
	  count == 1 ? "" : "es" );
  }
}
        

/*
 *   MWHERE ROUTINE
 */


static char *status( mob_data* mob )
{
  if( mob->leader
    && is_set( mob->status, STAT_PET ) ) {
    char *tmp = static_string( );
    snprintf( tmp, THREE_LINES, "Pet of %s", mob->leader->Name( ) );
    return tmp;
  }

  if( mob->reset )
    return "Reset";

  return "Mloaded?";
}


void do_mwhere( char_data* ch, const char *argument )
{
  char          tmp1  [ TWO_LINES ];
  mob_data*   victim;
  bool         found;
  char*         name;

  if( !*argument ) {
    send( ch, "Usage: mwhere <mob>\n\r" );
    return;
  }
  
  int vnum = atoi( argument );

  snprintf( tmp1, TWO_LINES,
	    "%26s  %6s  %-15s  %s\n\r", "Room",
	    "Vnum", "Status", "Area" );
  page_underlined( ch, tmp1 );

  for( int i = 1; i <= species_max; ++i ) {
    if( !species_list[i] )
      continue;

     name = (char *) species_list[i]->Name( );

     if( vnum != i && !is_name( argument, name ) )
      continue;

    page_divider( ch, name, i );
    found = false;

    for( int j = 0; j < mob_list; j++ ) {
      victim = mob_list[j];
      if( victim->Is_Valid( )
        && victim->species->vnum == i ) {
        page( ch, "%26s  %6d  %-15s  %s\n\r",
	      trunc( victim->in_room->name, 25 ),
	      victim->in_room->vnum,
	      status( victim ),
	      trunc( victim->in_room->area->name, 25 ) );
        found = true;
      }
    }
    
    if( !found )
      page_centered( ch, "None found" );
  }
}


/*
 *   MEDIT
 */


bool char_data :: can_edit( species_data* species, bool msg ) const
{
  if( species->shdata->level > get_trust( this ) ) {
    if( msg )
      send( this, no_permission );
    return false;
  }

  if( has_permission( this, PERM_ALL_MOBS ) 
      || is_name( descr->name, species->creator ) )
    return true;
  
  if( msg )
    send( this, no_permission );

  return false;
}


static species_data *new_species( char_data *ch,
				  const species_data *species_copy,
				  const char *name )
{
  // Find an unused vnum.
  int i;
  for( i = 1; get_species( i ); ++i );
  
  if( i == MAX_SPECIES ) {
    send( ch, "MUD is out of species vnums.\n\r" );
    return 0;
  }
  
  species_data *species = new species_data( i );

  share_data *shdata = species->shdata;
  descr_data *descr = species->descr;
  
  // Can't memcpy share_data, because skills are now alloc'ed arrays.
  //  memcpy( shdata, species_copy->shdata, sizeof( share_data ) );
  shdata->strength = species_copy->shdata->strength;
  shdata->intelligence = species_copy->shdata->intelligence;
  shdata->wisdom = species_copy->shdata->wisdom;
  shdata->dexterity = species_copy->shdata->dexterity;
  shdata->constitution = species_copy->shdata->constitution;
  shdata->level = species_copy->shdata->level;
  shdata->alignment = species_copy->shdata->alignment;
  shdata->race = species_copy->shdata->race;
  shdata->kills = species_copy->shdata->kills;
  shdata->deaths = species_copy->shdata->deaths;
  shdata->fame = species_copy->shdata->fame;
  vcopy( shdata->resist, species_copy->shdata->resist, MAX_RESIST );
  for( i = 0; i < MAX_SKILL_CAT; ++i ) {
    int n = table_max[ skill_table_number[ i ] ];
    vcopy( shdata->skills[i], species_copy->shdata->skills[i], n );
  }
  
  //  descr->keywords = alloc_string( name, MEM_DESCR );
  descr->singular = alloc_string( name, MEM_DESCR );
  descr->plural = alloc_string( name, MEM_DESCR );
  descr->complete = alloc_string( species_copy->descr->complete, MEM_DESCR );
  descr->long_s = alloc_string( "is here", MEM_DESCR );
  descr->long_p = alloc_string( "are here", MEM_DESCR );
  
  // Copy all mextras.
  for( int j = 0; j < species_copy->extra_descr; ++j ) {
    species->extra_descr += new extra_data( *species_copy->extra_descr[j] );
  }
  
  // Copy attack mprog and mpdatas.
  //    species->attack->code = alloc_string( species_copy->attack->code, MEM_CODE );
  species->attack->Set_Code( species_copy->attack->Code( ) );
  extra_array& extras = species->attack->Extra_Descr( );
  extra_array& old_extras = species_copy->attack->Extra_Descr( );
  for( int j = 0; j < old_extras; ++j ) {
    extras += new extra_data( *old_extras[j] );
  }
  
  // Copy all other mprogs and mpdatas.
  for( mprog_data *mprog_copy = species_copy->mprog; mprog_copy; mprog_copy = mprog_copy->next ) {
    mprog_data *mprog = new mprog_data( species );
    mprog->trigger = mprog_copy->trigger;
    mprog->value = mprog_copy->value;
    mprog->string = alloc_string( mprog_copy->string, MEM_MPROG );
    //      mprog->code = alloc_string( mprog_copy->code, MEM_CODE );
    mprog->Set_Code( mprog_copy->Code( ) );
    extra_array& extras = mprog->Extra_Descr( );
    extra_array& old_extras = mprog_copy->Extra_Descr( );
    for( int j = 0; j < old_extras; ++j ) {
      extras += new extra_data( *old_extras[j] );
    }
  }
  
  species->hitdice = species_copy->hitdice;
  species->movedice = species_copy->movedice;
  species->sex = species_copy->sex;
  species->hand = species_copy->hand;
  species->adult = species_copy->adult;
  species->maturity = species_copy->maturity;
  species->language = species_copy->language;
  species->nation = species_copy->nation;
  species->group = species_copy->group;
  species->gold = species_copy->gold;
  species->color = species_copy->color;
  species->size = species_copy->size;
  species->weight = species_copy->weight;
  species->act_flags[0] = species_copy->act_flags[0];
  species->act_flags[1] = species_copy->act_flags[1];
  //    species->wear_part = 0;
  species->skeleton = species_copy->skeleton;
  species->zombie = species_copy->zombie;
  species->corpse = species_copy->corpse;
  species->price = species_copy->price;
  species->wander = species_copy->wander;
  species->light = species_copy->light;
  species->movement = species_copy->movement;
  
  vcopy( species->affected_by, species_copy->affected_by, AFFECT_INTS );
  vcopy( species->chance, species_copy->chance, MAX_ARMOR );
  vcopy( species->armor, species_copy->armor, MAX_ARMOR );
  
  species->creator = alloc_string( ch->descr->name, MEM_SPECIES );
  
  // Copy marmor.
  for( i = 0; i < MAX_ARMOR; ++i )
    species->part_name[i] = alloc_string( species_copy->part_name[i], MEM_SPECIES );
  
  // Copy mresets.
  for( const reset_data *reset = species_copy->reset; reset; reset = reset->next ) {
    append( species->reset, new reset_data( *reset ) );
  }

  species->set_modified( 0 );
  mob_log( ch, species->vnum, "Created from #%d as %s.",
	   species_copy->vnum, species );
  fsend( ch, "Mob type \"%s\" created, assigned vnum %d.",
	 species, species->vnum );
  
  return species;
}


void do_medit( char_data* ch, const char *argument )
{
  char                     arg  [ MAX_INPUT_LENGTH ];
  char_data*            victim;
  species_data*        species;
  wizard_data*             imm;

  if( !( imm = wizard( ch ) ) )
    return;

  if( exact_match( argument, "delete" ) ) {
    if( !*argument ) {
      if( !( species = imm->mob_edit ) ) {
        send( ch, "Which mob do you want to delete?\n\r" );
        return;
      }
      if( !ch->can_edit( species ) )
        return;
    } else 
      if( !( species = get_species( atoi( argument ) ) ) ) {
        send( ch, "There is no species with that number.\n\r" );
        return;
      }
    
    if( !can_extract( species, ch ) )
      return;
    
    mob_log( ch, species->vnum, "Deleted as %s.", species );
    fsend( ch, "You genocide mob type %d, \"%s\".",
    	   species->vnum, species );

    char buf [ THREE_LINES ];
    char buf1 [ THREE_LINES ];

    snprintf( buf, THREE_LINES, "Mob type \"%s\" deleted by %s.",
	      species->Name( ), ch->descr->name );
    snprintf( buf1, THREE_LINES, "Mob type \"%s\" deleted.",
	      species->Name( ) );
    info( LEVEL_BUILDER, buf1, invis_level( imm ), buf, IFLAG_WRITES ); 

    /*
    for( int i = 0; i < species->attack->data; i++ ) {
      imm->mpdata_edit = species->attack->data[i];
      extract( imm, offset( &imm->mpdata_edit, imm ), "mpdata" );
    }

    for( int i = 0; i < species->extra_descr; i++ ) {
      imm->mextra_edit = species->extra_descr[i];
      extract( imm, offset( &imm->mextra_edit, imm ), "mextra" );
    }
    */

    imm->mob_edit = species;
    extract( imm, offset( &imm->mob_edit, imm ), "species" );
    delete species;
    return;
  }
  
  if( exact_match( argument, "new" ) ) {
    species_data *species_copy;
    if( isdigit( *argument ) ) {
      argument = one_argument( argument, arg );
      species_copy = get_species( atoi( arg ) );
      if( !species_copy ) {
        send( ch, "No mob has given vnum to copy.\n\r" );
        return;
      }
    } else {
      species_copy = get_species( MOB_BLANK );    
    }
    
    if( !*argument ) {
      send( ch, "You need to give the new mob a name.\n\r" );
      return;
    }

    if( !( species = new_species( ch, species_copy, argument ) ) )
      return;

    imm->mob_edit = species;
    imm->mextra_edit = 0;
    imm->mprog_edit = 0;
    imm->mpdata_edit = 0;
    imm->player_edit = 0;
    imm->account_edit = 0;

    mob_data *mob = new Mob_Data( species );
    stop_events( mob, execute_wander );
    mob->To( *ch->array );

    return;
  }

  if( exact_match( argument, "newzombie" ) ) {
    if( !imm->mob_edit ) {
      send( ch, "You are not editing any mob.\n\r" );
      return;
    }
    
    if( !ch->can_edit( imm->mob_edit ) )
      return;

    if( !*argument ) {
      send( ch, "You need to give the new mob a name.\n\r" );
      return;
    }

    if( get_species( imm->mob_edit->zombie ) ) {
      send( ch, "This species already has a zombie mob set.\n\r" );
      return;
    }

    if( !( species = new_species( ch, imm->mob_edit, argument ) ) )
      return;

    imm->mob_edit->zombie = species->vnum;

    species->zombie = species->vnum;
    species->group = GROUP_NONE;
    species->nation = NATION_NONE;
    species->adult = 0;
    species->maturity = 0;
    species->corpse = OBJ_CORPSE_NPC;
    species->light = 0;
    species->price = 0;
    species->gold = 0;

    // Zombie flags.
    set_bit( species->affected_by, AFF_SLEEP_RESIST );
    set_bit( species->affected_by, AFF_RESIST_POISON );
    set_bit( species->affected_by, AFF_SLOW );
    set_bit( species->affected_by, AFF_INFRARED );

    set_bit( species->act_flags, ACT_ZERO_REGEN );
    remove_bit( species->act_flags, ACT_WARM_BLOODED );
    remove_bit( species->act_flags, ACT_CAN_TAME );
    remove_bit( species->act_flags, ACT_HAS_SKELETON );
    remove_bit( species->act_flags, ACT_REST_REGEN );
    remove_bit( species->act_flags, ACT_WIMPY );

    species->shdata->resist[ RES_MIND ] = 100;
    species->shdata->resist[ RES_POISON ] = 100;
    species->shdata->alignment = ALGN_CHAOTIC_EVIL;
    species->shdata->intelligence = 0;
    species->shdata->wisdom = 0;
    species->shdata->race = RACE_UNDEAD;
    species->shdata->level = ( imm->mob_edit->shdata->level + 1 ) / 2;

    dice_data hitroll( species->hitdice );
    hitroll.number = ( hitroll.number+2 ) / 3;
    hitroll.plus = ( hitroll.plus+2 ) / 3;
    species->hitdice = hitroll;

    dice_data moveroll( species->movedice );
    moveroll.number *= 3;
    moveroll.plus *= 3;
    species->movedice = moveroll;

    for( int i = 0; i < MAX_ARMOR; ++i ) {
      if( species->armor[i] > 0 ) {
	species->armor[i] = ( species->armor[i] + 2 ) / 3;
      }
    }

    imm->mob_edit = species;
    imm->mextra_edit = 0;
    imm->mprog_edit = 0;
    imm->mpdata_edit = 0;
    imm->player_edit = 0;
    imm->account_edit = 0;

    mob_data *mob = new Mob_Data( species );
    stop_events( mob, execute_wander );
    mob->To( *ch->array );

    return;
  }

  if( exact_match( argument, "newskeleton" ) ) {
    if( !imm->mob_edit ) {
      send( ch, "You are not editing any mob.\n\r" );
      return;
    }
    
    if( !ch->can_edit( imm->mob_edit ) )
      return;

    if( !*argument ) {
      send( ch, "You need to give the new mob a name.\n\r" );
      return;
    }

    if( get_species( imm->mob_edit->skeleton ) ) {
      send( ch, "This species already has a skeleton mob set.\n\r" );
      return;
    }

    if( !( species = new_species( ch, imm->mob_edit, argument ) ) )
      return;

    imm->mob_edit->skeleton = species->vnum;
    set_bit( imm->mob_edit->act_flags, ACT_HAS_SKELETON );
    if( imm->mob_edit->zombie ) {
      if( species_data *zombie = get_species( imm->mob_edit->zombie ) ) {
	if( !ch->can_edit( zombie, false ) ) {
	  send( ch, "Warning: unable to update zombie mob #%d with new skeleton.\n\r",
		zombie->vnum );
	} else if( get_species( zombie->skeleton ) ) {
	  send( ch, "Warning: zombie mob #%d already has a skeleton set.\n\r",
		zombie->vnum );
	} else {
	  zombie->skeleton = species->vnum;
	  set_bit( zombie->act_flags, ACT_HAS_SKELETON );
	}
      }
    }
    species->skeleton = species->vnum;
    species->group = GROUP_NONE;
    species->nation = NATION_NONE;
    species->adult = 0;
    species->maturity = 0;
    species->corpse = OBJ_CORPSE_NPC;
    species->light = 0;
    species->price = 0;
    species->gold = 0;

    // Skeleton flags.
    set_bit( species->affected_by, AFF_SLEEP_RESIST );
    set_bit( species->affected_by, AFF_RESIST_POISON );
    set_bit( species->affected_by, AFF_INFRARED );

    set_bit( species->act_flags, ACT_ZERO_REGEN );
    remove_bit( species->act_flags, ACT_WARM_BLOODED );
    remove_bit( species->act_flags, ACT_CAN_TAME );
    set_bit( species->act_flags, ACT_HAS_SKELETON );
    remove_bit( species->act_flags, ACT_REST_REGEN );
    remove_bit( species->act_flags, ACT_WIMPY );
    remove_bit( species->act_flags, ACT_HAS_EYES );

    species->shdata->resist[ RES_MIND ] = 100;
    species->shdata->resist[ RES_POISON ] = 100;
    species->shdata->alignment = ALGN_CHAOTIC_EVIL;
    species->shdata->intelligence = 0;
    species->shdata->wisdom = 0;
    species->shdata->race = RACE_UNDEAD;

    dice_data hitroll( species->hitdice );
    hitroll.number = ( hitroll.number+1 ) / 2;
    hitroll.plus = ( hitroll.plus+1 ) / 2;
    species->hitdice = hitroll;

    dice_data moveroll( species->movedice );
    moveroll.number *= 3;
    moveroll.plus *= 3;
    species->movedice = moveroll;

    imm->mob_edit = species;
    imm->mextra_edit = 0;
    imm->mprog_edit = 0;
    imm->mpdata_edit = 0;
    imm->player_edit = 0;
    imm->account_edit = 0;

    mob_data *mob = new Mob_Data( species );
    stop_events( mob, execute_wander );
    mob->To( *ch->array );

    return;
  }

  if( exact_match( argument, "replace" ) ) {
    medit_replace( ch, argument );
    return;
  }
  
  if( !*argument ) {
    if( imm->player_edit ) {
      fsend( ch, "You stop editing %s.", imm->player_edit );
    } else if( imm->mob_edit ) {
      fsend( ch, "You stop editing %s.", imm->mob_edit );
    } else {
      send( ch, "Which player or mob do you want to edit?\n\r" );
      return;
    }
    imm->player_edit = 0;
    imm->account_edit = 0;
    imm->mob_edit = 0;
    imm->mextra_edit = 0;
    imm->mprog_edit = 0;
    imm->mpdata_edit = 0;
    return;
  }

  int vnum;

  if( number_arg( argument, vnum ) ) {
    if( !( species = get_species( vnum ) ) ) {
      send( ch, "No mob has that vnum.\n\r" );
      return;
    }

  } else {
    if( !( victim = one_character( ch, argument, "medit",
				   ch->array,
				   (thing_array*) &player_list ) ) )
      return;
    
    if( !victim->species ) {
      if( ch != victim ) {
        if( is_builder( victim ) && !is_demigod( ch ) ) {
          send( ch, "You can't edit immortals.\n\r" );
          return;
	}
        if( get_trust( victim ) >= get_trust( ch ) ) {
          send( ch, "You can't edit them.\n\r" );
          return;
	}
        if( !has_permission( ch, PERM_PLAYERS ) ) {
          send( ch, "You can't edit players.\n\r" );
          return;
	}
      }
      
      imm->player_edit = (player_data*) victim;
      imm->account_edit = victim->pcdata->pfile->account;
      imm->mob_edit    = 0;
      imm->mextra_edit = 0;
      imm->mprog_edit  = 0;
      imm->mpdata_edit = 0;
      
      fsend( ch, "Mstat, mset, mflag, and mskill now operate on %s.", victim );
      return;
    }

    species = victim->species;    
  }

  imm->player_edit  = 0;
  imm->account_edit = 0;
  imm->mob_edit     = species;
  imm->mextra_edit  = 0;
  imm->mprog_edit   = 0;
  imm->mpdata_edit = 0;

  fsend( ch, "Mstat, mset, mflag, mdesc, mreset, marmor, mskill, and mbug now operate on %s.", species );
}


void medit_replace( char_data* ch, const char *argument )
{
  species_data*   species1  = 0;
  species_data*   species2  = 0;
  int                count  = 0;
  int                 i, j;

  if( !number_arg( argument, i )
      || !number_arg( argument, j ) ) {
    send( ch, "Syntax: medit replace <vnum_old> <vnum_new>.\n\r" );
    return;
  }
  
  if( !( species1 = get_species( i ) )
      || !( species2 = get_species( j ) ) ) {
    send( ch, "Vnum %d doesn't correspond to an existing species.\n\r",
	  species1 ? j : i );
    return;
  }
  
  for( area_data *area = area_list; area; area = area->next ) 
    for( room_data *room = area->room_first; room; room = room->next ) 
      for( reset_data *reset = room->reset; reset; reset = reset->next ) 
        if( reset->vnum == i 
	    && is_set( reset->flags, RSFLAG_MOB ) ) {
          reset->vnum = j;
          ++count;
          area->modified = true;
	}

  mob_log( ch, i, "Replaced by #%d in %d reset%s.",
	   j, count, count != 1 ? "s" : "" );
  send( ch, "Species %d replaced by %d in %d reset%s.\n\r",
	i, j, count, count != 1 ? "s" : "" );
}


/*
 *   MARMOR
 */


void do_marmor( char_data *ch, const char *argument )
{
  char               arg  [ MAX_STRING_LENGTH ];
  char               buf  [ MAX_STRING_LENGTH ];
  species_data*  species;
  wizard_data*    imm;
  int             chance;
  int              armor; 
  int                  i;

  if( !( imm = wizard( ch ) ) )
    return;

  if( !( species = imm->mob_edit ) ) {
    send( ch, "You aren't editing any mob.\n\r" );
    return;
  }

  if( is_set( species->act_flags, ACT_HUMANOID ) ) {
    fsend( ch, "%s is humanoid so marmor has no affect.", species );
    return;
  }

  if( *argument ) {
    if( !ch->can_edit( species ) )
      return;
    
    argument = one_argument( argument, arg );
    i = atoi( arg )-1;
    
    if( i < 0 || i >= MAX_ARMOR ) {
      send( ch, "Part number out of range.\n\r" );
      return;
    }
    
    argument = one_argument( argument, arg );
    chance =  atoi( arg );
    
    if( chance < 1 || chance > 1000 ) {
      send( ch, "Chance out of range.\n\r" );
      return;
    }
    
    argument = one_argument( argument, arg );
    armor =  atoi( arg );
    
    if( armor < -100 || armor > 10000 ) {
      send( ch, "Armor out of range.\n\r" );
      return;
    }
    
    species->armor[i]     = armor;
    species->chance[i]    = chance;
    species->part_name[i] = alloc_string( argument, MEM_SPECIES );

    species->set_modified( ch );
    mob_log( ch, species->vnum, "Marmor [2%d] %5d %5d %s.",
	     i+1, species->chance[i], species->armor[i], species->part_name[i] );
    if( species->damage != 0 )
      send( ch, xp_reset );
    zero_exp( species );
  }

  page_title( ch, species->Name() );
  for( i = 0; i < MAX_ARMOR; i++ ) {
    snprintf( buf, MAX_STRING_LENGTH, "[%2d] %5d %5d %s\n\r", i+1,
	      species->chance[i], species->armor[i], species->part_name[i] );
    page( ch, buf );
  }
}


/*
 *   MDESC
 */


void do_mdesc( char_data* ch, const char *argument )
{
  species_data*  species;
  wizard_data*    imm;

  if( !( imm = wizard( ch ) ) )
    return;

  if( !( species = imm->mob_edit ) ) {
    send( ch, "You aren't editing any mob - use medit <mob>.\n\r" );
    return;
  }

  if( *argument
      && !ch->can_edit( species ) )
    return;

  if( !imm->mextra_edit ) {
    species->descr->complete = edit_string( ch, argument,
					    species->descr->complete, MEM_DESCR, true );
  } else {
    imm->mextra_edit->text = edit_string( ch, argument,
					  imm->mextra_edit->text, MEM_EXTRA, true );
  }

  if( *argument ) {
    species->set_modified( ch );
  }
  
  /*
  if( *argument != '\0' ) {
    mob_log( ch, species->vnum, "mdesc: %s", argument );
  }
  */
}


void do_mextra( char_data* ch, const char *argument )
{
  wizard_data *imm = wizard( ch );

  if( !imm )
    return;

  species_data *species;

  if( !( species = imm->mob_edit ) ) {
    send( ch, "You aren't editing any mob type.\n\r" );
    return;
  }
  
  if( matches( argument, "mob" ) ) {
    imm->mextra_edit = 0;
    send( ch, "Mdesc now operates on mob description.\n\r" );
    return;
  }

  if( ch->can_edit( species ) ) {
    if( edit_extra( species->extra_descr, imm,
		    offset( &imm->mextra_edit, imm ), argument, "mdesc" ) ) {
      species->set_modified( ch );
    }
  }
}


void do_mbug( char_data* ch, const char *argument ) 
{
  wizard_data *imm;
  if( !( imm = wizard( ch ) ) )
    return;

  species_data *species;

  if( !( species = imm->mob_edit ) ) {
    send( ch, "You aren't editing any mob type.\n\r" );
    return;
  }
  
  if( !*argument || ch->can_edit( species ) ) {
    species->comments = edit_string( ch, argument, species->comments, MEM_SPECIES, false );
    if( *argument ) {
      species->set_modified( ch );
    }
  }
}


/*
 *   MFLAG
 */


void do_mflag( char_data* ch, const char *argument )
{
  wizard_data*         imm;
  const char*     response;

#define types 3
  
  if( !( imm = wizard( ch ) ) )
    return;  

  char_data *pc = imm->player_edit;
  species_data *mob = imm->mob_edit;
  
  if( !pc && !mob ) {
    send( ch, "You aren't editing a mob or player.\n\r" );
    return;
  }

  int flags;
  if( !get_flags( ch, argument, &flags, "s", "mflag" ) )
    return;

  int section = -1;

  if( !pc ) {    
    const char* title [types] = { "Act", "Affect", "Wear" };
    int max [types] = { MAX_ACT,
			table_max[ TABLE_AFF_CHAR ],
			MAX_WEAR };

    const char** name1 [types] = { &act_name[0],
				   &aff_char_table[0].name,
				   &wear_part_name[0] }; 
    const char** name2 [types] = { &act_name[1],
				   &aff_char_table[1].name,
				   &wear_part_name[1] };

    int* flag_value [types] = { mob->act_flags,
				mob->affected_by,
				&mob->wear_part };
    const int uses_flag [types] = { 1, 1, 1 };
    const bool sort [types] = { true, true, false };

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

      response = set_flags( name1[section], name2[section],
			    flag_value[section], 0, max[section],
			    ch->can_edit( mob, false ) ? 0 : no_permission,
			    ch, argument, mob->Name( ch ),
			    false, true, sort[section] );
    } else {
      response = flag_handler( title, name1, name2, flag_value, 0, max,
			       uses_flag, sort,
			       ch->can_edit( mob, false ) ? 0 : no_permission,
			       ch, argument, mob->Name( ch ),
			       types );
    }

    if( response && *response ) {
      mob->set_modified( ch );
      mob_log( ch, mob->vnum, response );
      if( mob->damage != 0 )
	send( ch, xp_reset );
      zero_exp( mob );
    }
    return;
  }
  
  imm = wizard( pc );
  
  const char* title [types] = { "Option", "Affect", "Permission" };
  int max [types] = { has_permission( ch, PERM_PLAYERS ) ? MAX_PLR : MAX_PLR_OPTION,
		      table_max[ TABLE_AFF_CHAR ],
		      MAX_PERMISSION };
  
  const char** name1 [types] = { &plr_name[0],
				 &aff_char_table[0].name,
				 &permission_name[0] }; 
  const char** name2 [types] = { &plr_name[1],
				 &aff_char_table[1].name,
				 &permission_name[1] };
  
  int* flag_value [types] = { pc->pcdata->pfile->flags,
			      pc->affected_by,
			      imm ? imm->pcdata->pfile->permission : 0 };
  const int uses_flag [types] = { -1, 1, is_demigod( ch ) ? -1 : 1 };
  const bool sort [types] = { true, true, true };
  
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
    
    response = set_flags( name1[section], name2[section],
			  flag_value[section], 0, max[section],
			  "That flag isn't setable or you don't have permission.\n\r",
			  ch, argument, pc->Name( ch ),
			  false, true, sort[section] );
    
  } else {
    response = flag_handler( title, name1, name2,
			     flag_value, 0, max, uses_flag, sort,
			     "That flag isn't setable or you don't have permission.\n\r",
			     ch, argument, pc->Name( ch ),
			     imm ? types : 2 );
  }
  
  if( response && *response )
    player_log( pc, "(%s) %s", ch->real_name( ), response );
  
#undef types
}


/*
 *   MSET
 */


static int display_quest( char_data *ch, player_data *pc, int i )
{
  int value = pc->pcdata->quest_flags[i];

  if( value == QUEST_NONE ) {
    page( ch, "[%3d] Not Assigned: %s\n\r", i, quest_list[i]->message );
  } else if( value == QUEST_FAILED ) {
    page( ch, "[%3d] Failed: %s\n\r", i, quest_list[i]->message );
  } else if( value == QUEST_DONE ) {
    page( ch, "[%3d] Completed (%d): %s\n\r", i,
	  quest_list[i]->points, quest_list[i]->message );
    return quest_list[i]->points;
  } else if( value == QUEST_ASSIGNED ) {
    page( ch, "[%3d] Assigned: %s\n\r", i, quest_list[i]->message );
  } else {
    page( ch, "[%3d] Status %d: %s\n\r", i, value, quest_list[i]->message );
  }

  return 0;
}


void do_mset( char_data* ch, const char *argument )
{
  descr_data*      descr;
  pc_data*        pcdata;
  share_data*     shdata;
  wizard_data*       imm;
  account_data*  account;
  clan_data*        clan;
  const char*       name;

  if( !( imm = wizard( ch ) ) )
    return;

  species_data *species = imm->mob_edit;
  player_data *pc = imm->player_edit;

  if( !species && !pc ) {
    send( ch, "You aren't editing any mob - use medit <mob>.\n\r" );
    return;
  }

  if( !*argument ) {
    do_mstat( ch, "" );
    return;
  }

  if( !species ) {
    shdata = pc->shdata;
    pcdata = pc->pcdata;
    descr  = pc->descr;
    imm    = wizard( pc );
    name   = descr->name;
  } else {
    if( !ch->can_edit( species ) )
      return;
    shdata = species->shdata;
    pcdata = 0;
    descr  = species->descr;
    pc     = 0;
    imm    = 0;
    name   = species->Name( );
  }

  { 
    class byte_field byte_list[] = {
      { "strength",          0,    30,  &shdata->strength      },
      { "intelligence",      0,    30,  &shdata->intelligence  },
      { "wisdom",            0,    30,  &shdata->wisdom        },
      { "dexterity",         0,    30,  &shdata->dexterity     },
      { "constitution",      0,    30,  &shdata->constitution  },
      { "",                  0,     0,  0                      }
    };

    if( const char *response = process( byte_list, ch, name, argument, species, pc ) ) {
      if( *response ) {
	if( species ) {
	  if( species->damage != 0 )
	    send( ch, xp_reset );
	  zero_exp( species );
	} else {
	  update_maxes( pc );
	}
      }
      return;
    }
  }
  
  {
    class int_field int_list[] = {
      { "fame",              0,  1000,  &shdata->fame          },
      { "",                  0,     0,  0                      },
    };
    
    if( const char *response = process( int_list, ch, name, argument, species, pc ) ) {
      if( *response ) {
	if( species ) {
	  if( species->damage != 0 )
	    send( ch, xp_reset );
	  zero_exp( species );
	} else {
	  update_maxes( pc );
	}
      }
      return;
    }

#define an( i )   alignment_table[i].name
#define rn( i )   race_table[i].name
#define mea       table_max[ TABLE_ALIGNMENT ]
#define mer       table_max[ TABLE_RACE ]
    
    int old_race = shdata->race;

    class type_field type_list[] = {
      { "alignment", mea,   &an(0), &an(1), &shdata->alignment, true  },
      { "race",      mer,   &rn(0), &rn(1), &shdata->race,      true  },
      { "" }
    };
    
#undef mea
#undef mer
#undef an
#undef rn
    
    if( const char *response = process( type_list, ch, name, argument, species, pc ) ) {
      if( species && *response ) {
	if( species->damage != 0 )
	  send( ch, xp_reset );
	zero_exp( species );
      }
      if( pc && *response && shdata->race != old_race ) {
	// Changed a player's race; need to update innate affects.
	init_affects( pc, 0, old_race );
      }
      return;
    }
  }
  
  {
    class string_field string_list[] = {
      { "keywords",      MEM_DESCR,(const char**)  &descr->keywords,    0 },
      { "singular",      MEM_DESCR,(const char**)  &descr->singular,    0 },
      { "",              0,                        0,                   0 },   
    };
    
    if( process( string_list, ch, name, argument, species, 0 ) )
      return;
  }
  
  if( species ) {
    {
      class string_field string_list[] = {
	{ "name",         MEM_DESCR,(const char**) &descr->name,          0 },
	{ "plural",       MEM_DESCR,(const char**) &descr->plural,        0 },
	{ "long_s",       MEM_DESCR,(const char**) &descr->long_s,        0 },
	{ "long_p",       MEM_DESCR,(const char**) &descr->long_p,        0 },
	{ "prefix_s",     MEM_DESCR,(const char**) &descr->prefix_s,      0 },
	{ "prefix_p",     MEM_DESCR,(const char**) &descr->prefix_p,      0 },
	{ "adj_s",        MEM_DESCR,(const char**) &descr->adj_s,         0 },
	{ "adj_p",        MEM_DESCR,(const char**) &descr->adj_p,         0 },
	{ "creator",      MEM_SPECIES,(const char**) &species->creator,     0 },
	{ "",             0,                       0,                     0 },   
      };
      
      if( process( string_list, ch, name, argument, species, 0 ) )
	return;
    }

    {
      class int_field int_list[] = {
	{ "level",               0,           90,  &shdata->level        },
	{ "magic res.",       -200,          100,  &shdata->resist[0]    },
	{ "fire res.",        -200,          100,  &shdata->resist[1]    },
	{ "cold res.",        -200,          100,  &shdata->resist[2]    },
	{ "electric res.",    -200,          100,  &shdata->resist[3]    },
	{ "mind res.",        -200,          100,  &shdata->resist[4]    },
	{ "acid res.",        -200,          100,  &shdata->resist[5]    },
	{ "poison res.",      -200,          100,  &shdata->resist[6]    },
	{ "coins",               0,       100000,  &species->gold        },
	{ "maturity",            0,       100000,  &species->maturity    },
	{ "adult",               0,  species_max,  &species->adult       },
	{ "skeleton",            0,  species_max,  &species->skeleton    },
	{ "zombie",              0,  species_max,  &species->zombie      },
	{ "corpse",              0,        10000,  &species->corpse      },
	{ "price",               1,     10000000,  &species->price       },
	{ "wander delay",      200,QUEUE_LENGTH-1, &species->wander      },  
	{ "light",            -100,          100,  &species->light       },  
	{ "",                    0,            0,  0                     }, 
      };
      
      if( const char *response = process( int_list, ch, name, argument, species, 0 ) ) {
	if( *response ) {
	  if( species->damage != 0 )
	    send( ch, xp_reset );
	  zero_exp( species );
	}
	return;
      }
    }

    {
      class cent_field cent_list[] = {
	{ "weight",              0,     10000000,  &species->weight      },  
	{ "",                    0,            0,  0                     }, 
      };
      
      if( const char *response = process( cent_list, ch, name, argument, species, 0 ) ) {
	if( *response ) {
	  if( species->damage != 0 )
	    send( ch, xp_reset );
	  zero_exp( species );
	}
	return;
      }
    }

    {
      class dice_field dice_list[] = {
	{ "movedice",    LEVEL_MOB,     &species->movedice },
	{ "hitdice",     LEVEL_MOB,     &species->hitdice },    
	{ "",            -1,            0 }
      };
      
      if( const char *response = process( dice_list, ch, name, argument, species, 0 ) ) {
	if( *response ) {
	  if( species->damage != 0 )
	    send( ch, xp_reset );
	  zero_exp( species );
	}
	return;
      }
    }
    
    {
#define gtn( i )    group_table[i].name
#define nn( i )     nation_table[i].name
#define sn( i )     size_name[i]
#define sxn( i )    sex_name[i]
      
      class type_field type_list[] = {
	{ "group",    table_max[ TABLE_GROUP ],    &gtn(0), &gtn(1), &species->group, true },
	{ "nation",   table_max[ TABLE_NATION ],   &nn(0),  &nn(1),  &species->nation, true   },
	{ "sex",      MAX_SEX,                     &sxn(0), &sxn(1), &species->sex, true      },
	{ "size",     MAX_SIZE,                    &sn(0),  &sn(1),  &species->size, false },
	{ "" }
      };
            
#undef gtn
#undef nn
#undef sn
#undef sxn

      if( const char *response = process( type_list, ch, name, argument, species, 0 ) ) {
	if( *response ) {
	  if( species->damage != 0 )
	    send( ch, xp_reset );
	  zero_exp( species );
	}
	return;
      }
    }

    {
#define cn( i )     color_fields[i]
#define mn( i )     movement_table[i].name
      
      class type_field type_list[] = {
	{ "color",    MAX_COLOR,                   &cn(0),  &cn(1),  &species->color, true    },
	{ "movement", table_max[ TABLE_MOVEMENT ], &mn(0),  &mn(1),  &species->movement, true },
	{ "" }
      };
            
#undef cn
#undef mn

      if( process( type_list, ch, name, argument, species, 0 ) ) {
	return;
      }
    }
  }

  if( imm ) {
    class int_field int_list [] = {
      { "office",          0,  MAX_ROOM,  &imm->office    },
      { "recall",          0,  MAX_ROOM,  &imm->recall    },
      { "",                0,         0,  0               },
    };
    
    if( process( int_list, ch, name, argument ) )
      return;
  }

  if( pc ) {
    {
      class int_field int_list[] = {
	{ "deaths",          0,2000000000,  &shdata->deaths                   },
	{ "kills",           0,2000000000,  &shdata->kills                    },
	{ "exp",             0,2000000000,  &pc->exp                          },
	{ "remort",          0,      1000,  &pc->remort                       },
	{ "gsp_points",  -1000,      1000,  &pc->gossip_pts                   },
	{ "qst_points",      0,   1000000,  &pcdata->quest_pts                },
	{ "piety",           0,      1000,  &pcdata->piety                    },
	{ "prayer",          0,      1000,  &pc->prayer                       },
	{ "prac_points",  -100,      1000,  &pcdata->practice                 },
	{ "hunger",       -100,       100,  &pc->condition[COND_FULL]     },
	{ "thirst",       -100,       100,  &pc->condition[COND_THIRST]   },
	{ "drunk",        -100,       100,  &pc->condition[COND_DRUNK]    },
	{ "alcohol",      -100,       100,  &pc->condition[COND_ALCOHOL]  },
	{ "move",            0,      1000,  &pc->move           		    },
	{ "hits",            0,      1000,  &pc->hit            		    },
	{ "mana",            0,      1000,  &pc->mana           		    },
	{ "base_move",       0,      1000,  &pc->base_move      		    },
	{ "base_hits",       0,      1000,  &pc->base_hit       		    },
	{ "base_mana",       0,      1000,  &pc->base_mana      		    },
	{ "base_age",        1,      1000,  &pc->base_age       		    },
	{ "",                0,         0,  0                   		    },
      };
    
      if( process( int_list, ch, name, argument, 0, pc ) ) {
	update_maxes( pc );
	return;
      }
    }
    
    {
#define ctn( i )    clss_table[i].name
#define rn( i )     religion_table[i].name
#define sxn( i )    sex_name[i]
#define mn( i )     movement_table[i].name
#define max_relig   table_max[ TABLE_RELIGION ]
      
      class type_field type_list[] = {
	{ "sex",       MAX_SEX-1,   &sxn(0),  &sxn(1),  &pc->sex, true           },
	{ "class",     MAX_CLSS,    &ctn(0),  &ctn(1),  &pcdata->clss, true      },
	{ "religion",  max_relig,   
	  (const char**) &rn(0), (const char**) &rn(1),  
	  &pcdata->religion, true  },
	{ "movement", table_max[ TABLE_MOVEMENT ], &mn(0), &mn(1), &pc->movement, true },
	{ "" }
      };
      
#undef max_relig
#undef mn
#undef ctn
#undef rn
#undef sxn
    
      if( process( type_list, ch, name, argument, 0, pc ) )
	return;
    }
    
    if( matches( argument, "clan" ) ) {
      clan_data *old_clan = pc->pcdata->pfile->clan;
      if( !*argument ) {
        send( ch, "Set %s to which clan?\n\r", pc );
	send( ch, "[ Current: %s ]\n\r",
	      old_clan ? old_clan->abbrev : "none" );
        return;
      }  
      if( exact_match( argument, "none" ) ) {
	if( !old_clan ) {
	  if( ch == pc ) {
	    fsend( ch, "You have no clan." );
	  } else {
	    fsend( ch, "%s has no clan.", pc );
	  }
	  return;
	}
        if( ch == pc ) {
          fsend( ch, "You remove yourself from clan %s.", old_clan->abbrev );
	} else {
          fsend( ch, "You remove %s from clan %s.", pc, old_clan->abbrev );
          fsend( pc, "%s removes you from clan %s.", ch, old_clan->abbrev );
	}
        remove_member( pc );   
        remove_member( pc->pcdata->pfile );
	return;
      }
      if( ( clan = find_clan( ch, argument ) ) ) {
	if( clan == old_clan ) {
	  if( ch == pc ) {
	    fsend( ch, "You are already in clan %s.", clan->abbrev );
	  } else {
	    fsend( ch, "%s is already in clan %s.", pc, clan->abbrev );
	  }
	  return;
	}
	player_log( pc, "Clan set to %s by %s.", clan->abbrev, ch->real_name( ) );
        if( ch == pc ) {
          fsend( ch, "You set yourself into clan %s.", clan->abbrev );
	} else {
          fsend( ch, "You set %s into clan %s.", pc, clan->abbrev );
          fsend( pc, "%s sets you into clan %s.", ch, clan->abbrev );
	}
	send( ch, "[ Previous: %s ]\n\r",
	      old_clan ? old_clan->abbrev : "none" );
        remove_member( pc );   
        remove_member( pc->pcdata->pfile );
        add_member( clan, pc->pcdata->pfile );
        save_clans( clan );
      }
      return;
    }

    if( matches( argument, "account" ) ) {
      if( !*argument ) {
        fsend( ch, "To which account do you wish to switch %s?", pc );
        return;
      }
      if( !( account = find_account( argument ) ) ) {
        send( ch, "No such account exists.\n\r" );
        return;
      }
      if( account == pc->pcdata->pfile->account ) {
	fsend( ch, "%s already has account %s.", pc, account->name );
	return;
      }
      player_log( pc, "Account set to %s by %s.", account->name, ch->real_name( ) );
      send( ch, "%s switched to account %s.\n\r",
	    pc, account->name );
      send( ch, "[ Previous: %s ]\n\r",
	    pc->pcdata->pfile->account->name );
      pc->pcdata->pfile->account = account;
      return;
    }

    if( matches( argument, "quest" ) ) {
      if( !*argument ) {
	bool found = false;
	int total = 0;
	for( int i = 0; i < MAX_QUEST; ++i ) {
	  if( quest_list[i] ) {
	    if( pc->pcdata->quest_flags[i] != QUEST_NONE ) {
	      if( !found ) {
		page_title( ch, "Quests for %s", pc );
	      }
	      total += display_quest( ch, pc, i );
	      found = true;
	    }
	  }
	}
	if( !found ) {
	  fsend( ch, "%s has no quests assigned or completed.", pc );
	} else {
	  page( ch, "\n\r" );
	  fpage( ch, "%s has earned %d quest points.", pc, total );
	  if( total != pc->pcdata->quest_pts ) {
	    fpage( ch, "WARNING! This does not match that player's %d quest points.",
		   pc->pcdata->quest_pts );
	  }
	}
	return;
      }

      int index;
      if( !number_arg( argument, index, true ) ) {
	send( ch, "Syntax: mset quest [<quest #> [<value>] ]\n\r" );
	return;
      }

      if( !quest_list[index] ) {
	send( ch, "There is no quest #%d.\n\r", index );
	return;
      }

      if( !*argument ) {
	page_title( ch, "Quest %d for %s", index, pc );
	display_quest( ch, pc, index );
	if( quest_list[index]->comments != empty_string ) {
	  page_centered( ch, "-----" );
	  page( ch, quest_list[index]->comments );
	}
	return;
      }

      int status;
      if( !number_arg( argument, status ) ) {
	send( ch, "Syntax: mset quest [<quest #> [<value>] ]\n\r" );
	return;
      }

      if( status == pc->pcdata->quest_flags[index] ) {
	fsend( ch, "Quest #%d status for %s already set to %d.",
	       index, pc, status );
	return;
      }

      int old = pc->pcdata->quest_flags[index];

      fsend( ch, "Quest #%d status for %s set to %d.",
	     index, pc, status );

      if( old == QUEST_DONE ) {
	send( ch, "Removed completed quest #%d and %d quest points.\n\r",
	      index, quest_list[index]->points );
	pc->pcdata->quest_pts -= quest_list[index]->points;
	pc->pcdata->quest_pts = max( 0, pc->pcdata->quest_pts );
      } else if( status == QUEST_DONE ) {
	send( ch, "Player granted %d quest points for completing quest #%d.\n\r",
	      quest_list[index]->points, index );
	pc->pcdata->quest_pts += quest_list[index]->points;
      }

      pc->pcdata->quest_flags[index] = status;

      return;
    }

    if( is_god( ch )
	&& matches( argument, "balance" ) ) {
      if( !pcdata->pfile->account ) {
        send( ch, "Null Account.\n\r" );
        return;
      }
      int bal;
      if( !number_arg( argument, bal ) /*|| bal < 0*/ ) {
	send( ch, "Set account balance to what?\n\r" );
	send( ch, "[ Current value: $%.2f ]\n\r",
	      (double) pcdata->pfile->account->balance/100.0 );
	return;
      }
      int old = pcdata->pfile->account->balance;
      //      pcdata->pfile->account->balance = atoi( argument );
      pcdata->pfile->account->balance = bal;
      player_log( pc, "Account balance set to $%.2f by %s.",
		  (double) bal / 100.0, ch->real_name( ) );
      fsend( ch, "%s's account balance set to $%.2f.", pc,
	     (double) bal / 100.0 );
      send( ch, "[ Previous value: $%.2f ]\n\r",
	    (double) old / 100.0 );
      save_accounts( );
      return;
    }
  }
  
  send( ch, "Unknown field - see help mset.\n\r" );
}


void do_mskill( char_data* ch, const char *argument )
{
  wizard_data *wiz = wizard( ch );

  if( !wiz )
    return;

  species_data *species = wiz->mob_edit;
  player_data *victim = wiz->player_edit;

  if( !species && !victim ) {
    send( ch, "You aren't editing any mob - use medit <mob>.\n\r" );
    return;
  }

  descr_data *descr = species ? species->descr : victim->descr;
  share_data *shdata = species ? species->shdata : victim->shdata;
  const char *name = species ? species->Name( ) : descr->name;

  if( !*argument ) {
    int col = 0;
    for( int j = 0; j < MAX_SKILL_CAT; ++j ) {
      int m = table_max[ skill_table_number[ j ] ];
      for( int i = 0; i < m; ++i ) {
	if( shdata->skills[j][i] == UNLEARNT )
	  continue;
	if( col == 0 ) {
	  page_title( ch, name );
	}
	page( ch, "%21s (%2d)%s", skill_entry( j, i )->name,
	      (int)shdata->skills[j][i], ++col%3 == 0 ? "\n\r" : "" );
      }
    }
    if ( col == 0 ) {
      fsend( ch, "%s has no skills.", name );
    } else if( col % 3 != 0 ) {
      page( ch, "\n\r" );
    }

    return;
  }

  char arg [MAX_INPUT_LENGTH];

  argument = one_argument( argument, arg );

  int n = find_skill( arg );

  if( n < 0 ) {
    send( ch, "Unknown skill \"%s\".\n\r", arg );
    return;
  }

  int j = skill_table(n);
  int i = skill_number(n);

  skill_type *entry = skill_entry( n );

  if( !*argument ) {
      /*
    for( int j = 0; j < MAX_SKILL_CAT; ++j ) {
      int m = table_max[ skill_table_number[ j ] ];
      for( int i = 0; i < m; ++i ) {
	skill_type *entry = skill_entry( j, i );
	if( !strncasecmp( entry->name, arg, strlen( arg ) ) ) {
      */
    fsend( ch, "Skill '%s' on %s is currently %d.",
	   entry->name, name, (int)shdata->skills[j][i] );
    /*
	  return;
	}
      }
    }
      */
    return;
  }

  int value;
  if( !number_arg( argument, value )
      || value > 10
      || value < 0 ) {
    send( ch, "Skill level can range from 0 to 10.\n\r" );
    return;
  }
  
  if( species && !ch->can_edit( species ) )
    return;
  
  shdata->skills[j][i] = (unsigned char)value;

  if( species ) {
    species->set_modified( ch );
    mob_log( ch, species->vnum, "Skill '%s' set to %d.",
	     entry->name, value );
    if( species->damage != 0 )
      send( ch, xp_reset );
    zero_exp( species );
  } else {
    player_log( victim, "Skill '%s' set to %d by %s.",
		entry->name, value, ch->real_name( ) );
  }
  fsend( ch, "Skill '%s' on %s set to %d.",
	 entry->name, name, value );
  
  //  pc_data *pcdata = species ? 0 : victim->pcdata;
  //  player_data *pc = species ? 0 : player( victim );
  //  wizard_data *imm = species ? 0 : wizard( victim );

  /*
  for( int j = 0; j < MAX_SKILL_CAT; ++j ) {
    int m = table_max[ skill_table_number[ j ] ];
    for( int i = 0; i < m; ++i ) {
      skill_type *entry = skill_entry( j, i );
      if( !strncasecmp( entry->name, arg, strlen( arg ) ) ) {
	shdata->skills[j][i] = (unsigned char)value;
	if( species ) {
	  species->set_modified( ch );
	  mob_log( ch, species->vnum, "Skill '%s' set to %d.",
		   entry->name, value );
	  if( species->damage != 0 )
	    send( ch, xp_reset );
	  zero_exp( species );
	} else {
	  player_log( victim, "Skill '%s' set to %d by %s.",
		      entry->name, value, ch->real_name( ) );
	}
	fsend( ch, "Skill '%s' on %s set to %d.",
	       entry->name, name, value );
	return;
      }
    }
  }

  send( ch, "Unknown skill '%s'.\n\r", arg );
  */
}


void do_mstat( char_data* ch, const char *argument )
{
  char                buf  [ MAX_STRING_LENGTH ];
  char_data*       victim;
  descr_data*       descr;
  pc_data*         pcdata;
  share_data*      shdata;
  species_data*   species;
  player_data*         pc;
  int               flags;

  wizard_data *imm = wizard( ch );

  if( !get_flags( ch, argument, &flags, "a", "Mstat" ) )
    return;
  
  if( !*argument ) {
    if( !( species = imm->mob_edit ) ) {
      if( !imm->player_edit ) {
        send( ch, "Specify victim or use medit to select one.\n\r" );
        return;
      }
      victim = imm->player_edit;
      shdata = victim->shdata;
      pcdata = victim->pcdata;
      descr  = victim->descr;
      pc     = player( victim );
    } else {
      victim = 0;
      shdata = species->shdata;
      pcdata = 0;  
      descr  = species->descr;
      pc = 0;
    }
  } else {
    int vnum;
    if( number_arg( argument, vnum ) ) {
      if( !( species = get_species( vnum ) ) ) {
        send( ch, "There is no species with that number.\n\r" );
        return;
      }
      victim = 0;
      shdata = species->shdata;
      pcdata = 0;  
      descr  = species->descr;
      pc = 0;
    } else {
      if( !( victim = one_character( ch, argument, "mstat",
				     ch->array, (thing_array*) &player_list ) )  )
        return;
      
      shdata  = victim->shdata;
      pcdata  = victim->pcdata;
      species = victim->species;   
      descr   = victim->descr;
      if( ( pc  = player( victim ) )
	  && victim != ch
	  && get_trust( victim ) >= get_trust( ch ) ) {
	send( ch, "You can't mstat them.\n\r" );
	return;
      }
    }
  }
  
  sprintf( buf, "        " );
  
  if( !is_set( flags, 0 ) ) {
    page_title( ch, victim
		? victim->Seen_Name( ch ) : species->Name( true, false, false) );
    
    if( pc ) {
      sprintf( buf+5,  "    Class: %s        ",
	       clss_table[ pcdata->clss ].name );
      sprintf( buf+25, "   Remort: %d        ",
	       pc->remort );
      if( pc->Level() <= LEVEL_HERO )
	sprintf( buf+45, "  Exp.Lvl: %d\n\r",
		 exp_for_level( victim ) - victim->exp );
      else
	sprintf( buf+45, "\n\r" );
      page( ch, buf );
    } else {
      page( ch, "          Vnum: %-10d Creator: %s%s%s\n\r",
	    species->vnum, color_code( ch, COLOR_BOLD_YELLOW ),
	    species->creator, normal( ch ) );
    }
    
    if( !pc ) 
      page( ch, "        Nation: %-12s Group: %s\n\r",
	    nation_table[species->nation].name,
	    group_table[species->group].name );
    
    page( ch, "         Level: %-12d Kills: %-11d Deaths: %d\n\r",
	  shdata->level, shdata->kills, shdata->deaths );

    if( pc ) {
      page( ch, "          Fame: %d\n\r",
	    shdata->fame );
    } else {
      page( ch, "          Fame: %-9d Last Mod: %s\n\r",
	    shdata->fame,
	    species->last_mod );
    } 

    page( ch,  "           Sex: %-13s Race: %-12s Align: %s\n\r",
	  sex_name[ victim ? victim->sex : species->sex ],
	  race_table[ shdata->race ].name,
	  alignment_table[ shdata->alignment ].name );
    
    if( pc ) {
      const clan_data *clan = pcdata->pfile->clan;
      page( ch, "         Piety: %-11d Prayer: %-9d Religion: %s\n\r",
	    pcdata->piety, pc->prayer, religion_table[ pcdata->religion ].name );
      page( ch, "           Age: %-12d Pracs: %-12d Total: %d (%d)\n\r",
	    pc->Age( ), pcdata->practice,
	    total_pracs( victim ), expected_pracs( victim ) );
      page( ch, "      Qst_Pnts: %-9d Gsp_Pnts: %-13d Clan: %s\n\r",
	    pcdata->quest_pts, pc->gossip_pts,
	    clan ? clan->abbrev : "none" );
      page( ch, "      Movement: %s\n\r",
	    pc->movement < 0
	    ? "none"
	    : movement_table[ pc->movement ].name );
      page( ch, "          Bank: %-12d Coins:%s\n\r",
	    pc->bank, coin_phrase( victim ) );

      wizard_data *imm = wizard( pc );
      if( imm
	  && is_apprentice( imm ) ) {
	page( ch, "\n\r        Office: %-11d Recall: %d\n\r",
	      imm->office, imm->recall );
      }

    } else {
      page( ch, "         Coins: %-11d Wander: %-12d Light: %d\n\r",
	    species->gold, species->wander, species->light );

      if( species->weight != 0 )
	page( ch, "          Size: %-11s Weight: %.2f lbs\n\r",
	      size_name[ species->size ], (double) species->weight/100.0 );
      else if( shdata->race >= MAX_PLYR_RACE )
	page( ch, "          Size: %-11s Weight: (%.2f lbs)\n\r",
	      size_name[ species->size ], (double) default_weight[ species->size ]/100.0 );
      else if( species->sex == SEX_RANDOM )
	page( ch, "          Size: %-11s Weight: (%.2f/%.2f lbs)\n\r",
	      size_name[ species->size ],
	      (double) plyr_race_table[ shdata->race ].weight_m/100.0,
	      (double) plyr_race_table[ shdata->race ].weight_f/100.0 );
      else if( species->sex == SEX_MALE )
	page( ch, "          Size: %-11s Weight: (%.2f lbs)\n\r",
	      size_name[ species->size ],
	      (double) plyr_race_table[ shdata->race ].weight_m/100.0 );
      else if( species->sex == SEX_FEMALE )
	page( ch, "          Size: %-11s Weight: (%.2f lbs)\n\r",
	      size_name[ species->size ],
	      (double) plyr_race_table[ shdata->race ].weight_f/100.0 );
      else
	page( ch, "          Size: %-11s Weight: (%.2f lbs)\n\r",
	      size_name[ species->size ],
	      (double) ( plyr_race_table[ shdata->race ].weight_m/100.0
			 + plyr_race_table[ shdata->race ].weight_f ) / 200.0 );

      page( ch, "       HitDice: %-29s MoveDice: %s\n\r",
	    dice_string( species->hitdice ),
	    dice_string( species->movedice ) );      
      page( ch, "      Maturity: %-12d Price: %-9d Movement: %s\n\r",
	    species->maturity, species->price,
	    movement_table[species->movement].name );

      species_data*     adult  = get_species( species->adult );
      species_data*  skeleton  = get_species( species->skeleton );
      species_data*    zombie  = get_species( species->zombie );
      obj_clss_data*   corpse  = get_obj_index( species->corpse );

      page( ch, "         Adult: %-5d (%s)\n\r",
        species->adult,
        adult ? adult->Name( ) : "none" );
      page( ch, "      Skeleton: %-5d (%s)\n\r",
        species->skeleton,
        skeleton ? skeleton->Name( ) : "none" );
      page( ch, "        Zombie: %-5d (%s)\n\r",
        species->zombie,
        zombie ? zombie->Name( ) : "none" );
      page( ch, "        Corpse: %-5d (%s)\n\r",
        species->corpse,
        corpse ? corpse->Name( ) : "none" );
      }

    if( victim ) {
      page( ch, "\n\r" );

      sprintf( buf+5,  "  Hit Pts: %d/%d       ", victim->hit,
	       victim->max_hit );
      sprintf( buf+25, "   Energy: %d/%d       ", victim->mana,
	       victim->max_mana );
      sprintf( buf+45, "     Move: %d/%d\n\r", victim->move,
	       victim->max_move );
      page( ch, buf );
      
      page( ch, "     Hit Regen: %-8d Ene Regen: %-9d Mv Regen: %d\n\r",  
	    victim->Hit_Regen( ), victim->Mana_Regen( ),
	    victim->Move_Regen( ) );
      
      page( ch, "        Weight: %-8.2f  Wght Inv: %-8.2f Wght Worn: %.2f\n\r",
	    (double) victim->Empty_Weight( )/100.0,
	    (double) victim->contents.weight/100.0,
	    (double) victim->wearing.weight/100.0 );
      
      page( ch, "        Leader: %s\n\r",
	    victim->leader ? victim->leader->Name( ch ) : "no one" );
      
      if( victim->species ) 
        page( ch, "         Reset: %s\n\r", name( ((mob_data*)victim)->reset ) );

      snprintf( buf+5, MAX_STRING_LENGTH-5, "   Hunger: %d          ",
		victim->condition[COND_FULL] );
      snprintf( buf+25, MAX_STRING_LENGTH-25, "   Thirst: %d          ",
		victim->condition[COND_THIRST] );
      snprintf( buf+45, MAX_STRING_LENGTH-45, "    Drunk: %d (%d)\n\r",
		victim->condition[ COND_DRUNK ], victim->condition[ COND_ALCOHOL ] );
      page( ch, buf );
    }

    page( ch, "\n\r" );  
    
    if( victim ) {
      snprintf( buf+5, MAX_STRING_LENGTH-5,
		"Str: %2d(%2d)  Int: %2d(%2d)  Wis: %2d(%2d)  Dex: %2d(%2d)\
  Con: %2d(%2d).\n\r\n\r",
		victim->Strength( ), shdata->strength,
		victim->Intelligence( ), shdata->intelligence,
		victim->Wisdom( ), shdata->wisdom,
		victim->Dexterity( ), shdata->dexterity,
		victim->Constitution( ), shdata->constitution );
      page( ch, buf );

      snprintf( buf+5, MAX_STRING_LENGTH-5,
		"Mag: %2d(%2d)  Fir: %2d(%2d)  Col: %2d(%2d)  Min: %2d(%2d)\n\r",
		victim->Save_Magic(),    shdata->resist[RES_MAGIC],
		victim->Save_Fire(),     shdata->resist[RES_FIRE],
		victim->Save_Cold(),     shdata->resist[RES_COLD],
		victim->Save_Mind(),     shdata->resist[RES_MIND] );   
      page( ch, buf );
      
      snprintf( buf+5, MAX_STRING_LENGTH-5,
		"Ele: %2d(%2d)  Aci: %2d(%2d)  Poi: %2d(%2d)\n\r\n\r",
		victim->Save_Shock(),    shdata->resist[RES_SHOCK],
		victim->Save_Acid(),     shdata->resist[RES_ACID],
		victim->Save_Poison(),   shdata->resist[RES_POISON] );

    } else {
      snprintf( buf+5, MAX_STRING_LENGTH-5,
		"Str: %2d  Int: %2d  Wis: %2d  Dex: %2d  Con: %2d\n\r\n\r",
		shdata->strength, shdata->intelligence, shdata->wisdom,
		shdata->dexterity, shdata->constitution );
      page( ch, buf );
      
      snprintf( buf+5, MAX_STRING_LENGTH-5,
		"Mag: %2d  Fir: %2d  Col: %2d  Min: %2d\n\r",
		shdata->resist[RES_MAGIC], shdata->resist[RES_FIRE],
		shdata->resist[RES_COLD],  shdata->resist[RES_MIND] );
      page( ch, buf );
      
      snprintf( buf+5, MAX_STRING_LENGTH-5,
		"Ele: %2d  Aci: %2d  Pos: %2d\n\r\n\r",
		shdata->resist[RES_SHOCK], shdata->resist[RES_ACID],
		shdata->resist[RES_POISON] );
    }
    page( ch, buf );
  }
  
  page( ch, "       Name: %s\n\r", descr->name );
  page( ch, "   Singular: %s\n\r", descr->singular );
  
  if( pc ) {
    if( pcdata->tmp_short ) 
      page( ch, " Unapproved: %s\n\r", pcdata->tmp_short );
    page( ch, "   Keywords: %s\n\r", descr->keywords );
    if( pcdata->tmp_keywords )
      page( ch, " Unapproved: %s\n\r", pcdata->tmp_keywords );
  } else {
    page( ch, "   Prefix_S: %s\n\r", descr->prefix_s );
    page( ch, "      Adj_S: %s\n\r", descr->adj_s );
    page( ch, "     Long_S: %s\n\r", descr->long_s );
    page( ch, "     Plural: %s\n\r", descr->plural );
    page( ch, "   Prefix_P: %s\n\r", descr->prefix_p );
    page( ch, "      Adj_P: %s\n\r", descr->adj_p );
    page( ch, "     Long_P: %s\n\r", descr->long_p );
    page( ch, "   Keywords: %s\n\r", descr->keywords );
    page( ch, "      Color: %s\n\r", color_fields[ species->color ] );
  }

  snprintf( buf, MAX_STRING_LENGTH, "\n\rDescription: \n\r%s",
	    *descr->complete ? descr->complete : "(none).\n\r" );
  page( ch, buf );

  if( species ) {
    show_extras( ch, species->extra_descr );
    
    if( *species->comments ) {
      page( ch, "\n\rComments:\n\r" );
      page( ch, species->comments );
    }
  }
}


static void mistat( char_data *ch, char_data *victim )
{
  player_data *pc = player( victim );
  mob_data *npc = mob( victim );

  in_character = false;

  page_title( ch, victim->Seen_Name( ) );

}


void do_mistat( char_data* ch, const char *argument )
{
  wizard_data *imm;
  if( !( imm = wizard( ch ) ) )
    return;

  char arg [ MAX_INPUT_LENGTH ];

  argument = one_argument( argument, arg );

  char_data *victim;
  if( !( victim = one_character( ch, arg, "mistat",
				 ch->array ) ) )
    return;
  
  mistat( ch, victim );
}


void do_miset( char_data* ch, const char *argument )
{
  wizard_data *imm;
  if( !( imm = wizard( ch ) ) )
    return;

  char arg [ MAX_INPUT_LENGTH ];

  argument = one_argument( argument, arg );

  char_data *victim;
  if( !( victim = one_character( ch, arg, "miset",
				 ch->array ) ) )
    return;
  
  if( !*argument ) {
    mistat( ch, victim );
    return;
  }

}


void do_miflag( char_data* ch, const char *argument )
{
  wizard_data *imm;
  if( !( imm = wizard( ch ) ) )
    return;

  char arg [ MAX_INPUT_LENGTH ];

  argument = one_argument( argument, arg );

  char_data *victim;
  if( !( victim = one_character( ch, arg, "miflag",
				 ch->array ) ) )
    return;

  player_data *pc = player( victim );
  mob_data *npc = mob( victim );

  in_character = false;
  
  if( pc ) {

  } else {

  }
}
