#include <sys/types.h>
#include <stdio.h>
#include <limits.h>
#include "stdlib.h"
#include "define.h"
#include "struct.h"


const char* mprog_trigger[ MAX_MPROG_TRIGGER ] = {
  "entering", "leaving",
  "asking", "blocking", "death", "kill", "give", "reset", "tell",
  "skin", "timer", "attack", "order", "none", "cast", "describe",
  "to_room", "path", "summon", "practice" };


// *** FIX ME: not implemented yet
const char* mprog_value[ MAX_MPROG_TRIGGER ] = {
  "direction",
  "direction",
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0
};


const default_data *mprog_msgs [ MAX_MPROG_TRIGGER ] =
{
  0,
  0,
  0,
  blocking_msg,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0
};


/*
 *   ATTACK_DATA CLASS
 */


Attack_Data::Attack_Data( species_data *species )
  : species(species)
{
  record_new( sizeof( Attack_Data ), MEM_MPROG );
  //  record_delete( sizeof( program_data ), MEM_PROGRAM );
}


Attack_Data::~Attack_Data( )
{
  record_delete( sizeof( Attack_Data ), MEM_MPROG );
  //  record_new( sizeof( program_data ), MEM_PROGRAM );
}


bool Attack_Data::abort( thing_data *owner ) const
{
  if( !owner )
    return true;

  mob_data *mob = (mob_data*) owner;

  return !mob->Is_Valid( )
    || !mob->fighting
    || mob->position < POS_STANDING
    || is_entangled( mob )
    || mob->cast;
}


int Attack_Data::execute( thing_data *owner )
{
  species->used = true;

  if( !owner )
    owner = var_mob;

  return program_data::execute( owner );
}


/*
 *   MPROG_DATA CLASS
 */


Mprog_Data::Mprog_Data( species_data *species )
  : next(0), trigger(MPROG_TRIGGER_NONE), value(-1),
    string(empty_string), species(species)
{
  record_new( sizeof( mprog_data ), MEM_MPROG );
  //  record_delete( sizeof( program_data ), MEM_PROGRAM );
  append( species->mprog, this );
}


Mprog_Data::~Mprog_Data( )
{
  record_delete( sizeof( mprog_data ), MEM_MPROG );
  //  record_new( sizeof( program_data ), MEM_PROGRAM );
  free_string( string, MEM_MPROG );
}


int Mprog_Data::execute( thing_data *owner )
{
  species->used = true;

  if( !owner )
    owner = var_mob;

  return program_data::execute( owner );
}


/*
static void extract( mprog_data *mprog, wizard_data *wizard )
{
  for( int i = 0; i < mprog->data; ++i ) {
    wizard->mpdata_edit = mprog->data[i];
    extract( wizard, offset( &wizard->mpdata_edit, wizard ), "mpdata" );
  }

  wizard->mprog_edit = mprog;
  extract( wizard, offset( &wizard->mprog_edit, wizard ), "mprog" );
}
*/


/*
 *   DISPLAY ROUTINES
 */


void Attack_Data :: display( char_data* ch ) const
{
  if( !species ) {
    page( ch, "%-10s %-10s %s\n\r", "??", "Fight Code", "Null Species??" );
    return;
  }

  page( ch, "Mob %-11d %-20s %s\n\r",
	species->vnum, "Fight Code", species );
}     


void mprog_data :: display( char_data* ch ) const
{
  if( !species ) {
    page( ch, "%-10s %-10s %s\n\r", "??", "Mprog", "Null Species??" );
    return;
  }

  int i = 1;

  for( mprog_data *mprog = species->mprog; mprog != this && mprog; mprog = mprog->next, i++ );

  page( ch, "Mob %-11d Mprog %-8d %s\n\r",
	species->vnum, i, species );
}     


/*
 *   EDITING FUNCTIONS
 */


static mprog_data *find_mprog( char_data *ch, species_data *species, int i )
{
  mprog_data *mprog = 0;

  if( i >= 1 ) {
    int j = i;
    for( mprog = species->mprog; mprog && j != 1; mprog = mprog->next, --j );
  }

  if( !mprog ) {
    send( ch, "No mprog number %d.\n\r", i );
  }

  return mprog;
}


void do_mpedit( char_data* ch, const char *argument )
{
  mprog_data*      mprog;
  species_data*  species;
  int                  i;

  wizard_data *wizard  = (wizard_data*) ch;

  if( !( species = wizard->mob_edit ) ) {
    send( ch, "You aren't editing any mob.\n\r" );
    return;
  }

  if( !*argument ) {
    size_t len = 20;
    for( mprog = species->mprog; mprog; mprog = mprog->next ) {
      if( mprog->trigger == MPROG_TRIGGER_NONE )
	len = max( len, strlen( mprog->string ) );
    }
    page_underlined( ch, "   #  %*s  %s\n\r", len, "Trigger", "Target" );
    page( ch, "[%2d]  (attack)\n\r", 0 );
    for( i = 0, mprog = species->mprog; mprog; mprog = mprog->next ) {
      switch( mprog->trigger ) {
      case MPROG_TRIGGER_NONE:
	page( ch, "[%2d]  %*s\n\r", ++i, len, mprog->string );
	break;
      case MPROG_TRIGGER_GIVE:
	{
	  const char *name = "anything";
	  if( mprog->value != 0 ) {
	    obj_clss_data *clss = get_obj_index( mprog->value );
	    name = clss ? clss->Name() : "??";
	  }
	  page( ch, "[%2d]  %s%*s  %s [%d]%s\n\r", ++i,
		color_code( ch, COLOR_MILD ),
		len, mprog_trigger[mprog->trigger],
		name, mprog->value,
		color_code( ch, COLOR_DEFAULT ) );
	}
	break;
      default:
	page( ch, "[%2d]  %s%*s%s  %s\n\r", ++i,
	      color_code( ch, COLOR_MILD ),
	      len, mprog_trigger[mprog->trigger],
	      color_code( ch, COLOR_DEFAULT ),
	      mprog->string );
	break;
      }
    }
    return;
  }

  if( !ch->can_edit( species ) )
    return;

  if( number_arg( argument, i ) ) {
    if( i == 0 ) {
      wizard->mprog_edit = 0;
      wizard->mpdata_edit = 0;
      send( ch, "You now edit the attack mprog.\n\r" );
      return;
    }
    if( !( mprog = find_mprog( ch, species, i ) ) ) {
      return;
    }
    int j;
    if( isdigit( *argument ) && number_arg( argument, j ) ) {
      //      if( !ch->can_edit( species ) )
      //	return;
      if( j == i ) {
        send( ch, "Moving an mprog to where it already is does nothing interesting.\n\r" ); 
        return;
      }
      if( j == 1 ) {
        remove( species->mprog, mprog );
        mprog->next = species->mprog;
        species->mprog = mprog;
      } else {
        if( j < 1 || j > count( species->mprog ) ) {
          send( ch, "You can only move an mprog to a sensible position.\n\r" );
          return;
	}
        remove( species->mprog, mprog );
        mprog_data *prev = locate( species->mprog, j-1 );
        mprog->next = prev->next;
        prev->next = mprog;

      }
      species->set_modified( ch );
      send( ch, "Mprog %d moved to position %d.\n\r", i, j );
      return;
    }
    wizard->mprog_edit  = mprog;
    wizard->mpdata_edit = 0;
    send( ch, "You now edit mprog %d.\n\r", i );
    return;
  }
  
  if( exact_match( argument, "new" ) ) {
    mprog = new mprog_data( species );
    if( *argument ) {
      for( i = 0; i < MAX_MPROG_TRIGGER; ++i ) {
	if( !strcasecmp( argument, mprog_trigger[i] ) ) {
	  mprog->trigger = i;
	  fsend( ch, "Mprog added with %s trigger.", mprog_trigger[i] );
	  if( i == MPROG_TRIGGER_GIVE ) {
	    mprog->value = 0;
	  }
	  break;
	}
      }
      if( i == MAX_MPROG_TRIGGER ) {
	mprog->string = alloc_string( argument, MEM_MPROG );
	fsend( ch, "Mprog added with command trigger \"%s\".", mprog->string );
      }
    } else {
      send( ch, "Mprog added with no trigger.\n\r" );
    }
    wizard->mprog_edit  = mprog;
    wizard->mpdata_edit = 0;
    species->set_modified( ch );
    return;
  }

  if( exact_match( argument, "delete" ) ) {
    if( !*argument ) {
      if( !( mprog = wizard->mprog_edit ) ) {
	send( ch, "You aren't editing any mprog.\n\r" );
	return;
      }
    } else {
      if( !number_arg( argument, i ) ) {
	send( ch, "Syntax: mpedit delete [#]\n\r" );
	return;
      }
      if( i == 0 ) {
	send( ch, "You can't delete the attack mprog.\n\r" );
	return;
      }
      if( !( mprog = find_mprog( ch, species, i ) ) ) {
	return;
      }
    }
    send( ch, "Mprog deleted.\n\r" );
    mprog_data *old_edit = 0;
    if( wizard->mprog_edit != mprog ) {
      old_edit = wizard->mprog_edit;
      wizard->mprog_edit = mprog;
    } else {
      send( ch, "You now edit the attack mprog.\n\r" );
    }
    extract( wizard, offset( &wizard->mprog_edit, wizard ), "mprog" );
    remove( species->mprog, mprog );
    delete mprog;
    species->set_modified( ch );
    wizard->mprog_edit = old_edit;
    return;
  }

  send( ch, "Illegal syntax.\n\r" );
}


void do_mpcode( char_data* ch, const char *argument )
{
  program_data*    mprog;

  wizard_data *wizard = (wizard_data*) ch;

  species_data *species;

  if( !( species = wizard->mob_edit ) ) {
    send( ch, "You aren't editing any mob.\n\r" );
    return;
  }

  if( *argument ) {
    //    if( !ch->can_edit( species ) )
    //      return;
    species->set_modified( ch );
    //    mob_log( ch, species->vnum, "mpc: %s", argument );
    zero_exp( species );
  }
  
  if( !( mprog = wizard->mprog_edit ) )
    mprog = species->attack;

  mprog->Edit_Code( ch, argument );

  if( *argument || !mprog->binary ) {
    var_ch = ch;
    mprog->compile( );
  }

  /*
  if( mprog == species->attack )
    mprog->active = 1;
  */
}


void do_mpdata( char_data* ch, const char *argument )
{
  wizard_data *wizard = (wizard_data*) ch;
  program_data *mprog  = wizard->mprog_edit;

  species_data *species;

  if( !( species = wizard->mob_edit ) ) {
    send( ch, "You aren't editing any mob.\n\r" );
    return;
  }

  if( !mprog )
    mprog = species->attack;

  if( wizard->mpdata_edit ) {
    if( exact_match( argument, "exit" ) ) {
      wizard->mpdata_edit = 0;
      send( ch, "Mpdata now operates on the data list.\n\r" );
      return;
    }

    wizard->mpdata_edit->text
      = edit_string( ch, argument, wizard->mpdata_edit->text, MEM_EXTRA, true );

    if( *argument ) {
      species->set_modified( ch );
     }

  } else {
    if( !*argument
	&& mprog != species->attack ) {
      show_defaults( ch, ((mprog_data*)mprog)->trigger, mprog_msgs, -1 );
    }

    if( edit_extra( mprog->Extra_Descr( ), wizard,
		    offset( &wizard->mpdata_edit, wizard ), argument, "mprog" ) ) {
      species->set_modified( ch );
    }
  }

  if( *argument || !mprog->binary ) {
    var_ch = ch;
    mprog->compile( ); 
  }

  /*
  if( mprog == species->attack )
    mprog->active = 1;
  */
}


void do_mpstat( char_data* ch, const char *argument )
{
  wizard_data *wizard = (wizard_data*) ch;

  species_data *species;

  if( !( species = wizard->mob_edit ) ) {
    send( ch, "You aren't editing any mob.\n\r" );
    return;
  }

  mprog_data *mprog = wizard->mprog_edit;

  if( *argument ) {
    int num;
    if( number_arg( argument, num ) ) {
      if( num != 0 ) {
	if( !( mprog = find_mprog( ch, species, num ) ) ) {
	  return;
	}
      }
    } else {
      send( ch, "Syntax: mpstat [<mprog #>]\n\r" );
      return;
    }
  }

  // Use buf for long string length.
  //  char buf [ 3*MAX_STRING_LENGTH ];

  if( mprog ) {
    page( ch, "%10s : %s\n\r", "Trigger", mprog_trigger[ mprog->trigger ] );
    page( ch, "%10s : %d\n\r", "Value", mprog->value );
    page( ch, "%10s : %s\n\r\n\r", "String", mprog->string );
    /*
    snprintf( buf, 3*MAX_STRING_LENGTH, "[Code]\n\r%s\n\r", mprog->Code( ) );
    page( ch, buf );
    */
    page( ch, "[Code]\n\r%s\n\r", mprog->Code( ) );
    show_extras( ch, mprog->Extra_Descr( ) );
  } else {
    /*
    snprintf( buf, 3*MAX_STRING_LENGTH, "\n\r[Attack Code]\n\r%s\n\r", species->attack->Code( ) );
    page( ch, buf );
    */
    page( ch, "\n\r[Attack Code]\n\r%s\n\r", species->attack->Code( ) );
    show_extras( ch, species->attack->Extra_Descr( ) );
  }
}


void do_mpset( char_data* ch, const char *argument )
{
  if( !*argument ) {
    do_mpstat( ch, argument );
    return;
  }

  wizard_data *wizard = (wizard_data*) ch;

  species_data *species;

  if( !( species = wizard->mob_edit ) ) {
    send( ch, "You aren't editing any mob.\n\r" );
    return;
  }

  mprog_data *mprog;

  if( !( mprog = wizard->mprog_edit ) ) {
    send( ch, "You aren't editing any mprog.\n\r" );
    return;
  }

  //  if( !ch->can_edit( wizard->mob_edit ) )
  //    return;

#define mt( i )  mprog_trigger[i]

  class type_field type_list[] = {
    { "trigger",   MAX_MPROG_TRIGGER,  &mt(0),  &mt(1),  &mprog->trigger, true },
    { "" }
  };

  if( const char *result = process( type_list, ch, "mpset", argument ) ) {
    if( *result )
      species->set_modified( ch );
    return;
  }

#undef mt

  /*
  if( matches( argument, "trigger" ) ) {
    set_type( ch, argument, mprog->trigger, "Trigger",
	      "mprog", MAX_MPROG_TRIGGER, mprog_trigger );
    return;
  }
  */

  class int_field int_list[] = {
    { "value",      INT_MIN,  INT_MAX,   &mprog->value },
    { "",                 0,        0,               0 }
  };

  if( const char *result = process( int_list, ch, "mpset", argument ) ) {
    if( *result )
      species->set_modified( ch );
    return;
  }

  /*
  if( matches( argument, "value" ) ) { 
    mprog->value = atoi( argument );
    send( ch, "Value on mprog set to %d.\n\r", mprog->value );
    return;
  }
  */

  class string_field string_list[] = {
    { "string",    MEM_MPROG,   &mprog->string,    0 },
    { "",          0,           0,                 0 }
  };

  if( const char *result = process( string_list, ch, "mpset", argument ) ) {
    if( *result )
      species->set_modified( ch );
    return;
  }

  /*
  if( matches( argument, "string" ) ) {
    free_string( mprog->string, MEM_MPROG );
    mprog->string = alloc_string( argument, MEM_MPROG );
    send( ch, "String set to %s.\n\r", argument );
    return;
  }
  */

  send( ch, "Syntax: mpset <field> <value>\n\r" );
}


void do_mpflag( char_data* ch, const char *)
{
  wizard_data *wizard = (wizard_data*) ch;

  species_data *species;

  if( !( species = wizard->mob_edit ) ) {
    send( ch, "You aren't editing any mob.\n\r" );
    return;
  }

  mprog_data *mprog;

  if( !( mprog = wizard->mprog_edit ) ) {
    send( ch, "You aren't editing any mprog.\n\r" );
    return;
  }

  //  species->set_modified( ch );

  send( ch, "Under construction.\n\r" );
}


/*
 *   ASK ROUTINE
 */


void do_ask( char_data* ch, const char *argument )
{
  char            arg  [ MAX_INPUT_LENGTH ];
  char_data*   victim;
  mprog_data*   mprog;

  if( is_mob( ch ) )
    return;

  if( !*argument ) {
    send( ch, "Ask whom?\n\r" );
    return;
  }

  if( is_confused_pet( ch ) )
    return;

  argument = one_argument( argument, arg );

  if( !( victim = one_character( ch, arg, "ask", ch->array ) ) )
    return;

  if( victim->pcdata ) {
    send( ch, "They seem to ignore your request.\n\r" );
    return;
  }

  if( !strcasecmp( "about ", argument ) )
    argument += 6;
  else if( !strcasecmp( "for ", argument ) )
    argument += 4;

  skip_spaces( argument ); 

  if( !*argument ) {
    send( ch, "Ask about what?\n\r" );
    return;
  }

  for( mprog = victim->species->mprog; mprog; mprog = mprog->next )
    if( mprog->trigger == MPROG_TRIGGER_ASKING 
	&& ( !*mprog->string
	     || is_name( argument, mprog->string ) ) ) {
      clear_variables( );
      var_ch  = ch;
      var_mob = victim;
      var_arg  = argument;
      var_room = ch->in_room;
      mprog->execute( );
      return;
    }
  
  /*
  for( mprog = victim->species->mprog; mprog; mprog = mprog->next )
    if( mprog->trigger == MPROG_TRIGGER_ASKING 
	&& member( "default", mprog->string ) ) {
      clear_variables( );
      var_ch  = ch;
      var_mob = victim;
      var_room = ch->in_room;
      mprog->execute( );
      return;
    }
  */
  
  if( victim->species ) {
    if( victim->position >= POS_RESTING )
      fsend( ch, "%s looks at you in bewilderment.", victim );
    return;
  }

  process_tell( victim, ch, "I know nothing about that." );
}
