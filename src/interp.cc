#include <ctype.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include "define.h"
#include "struct.h"


const char *const bad_command = "<Type 'help' for help>\n\r";


/*
 *   DISALLOW COMMAND ROUTINES
 */


bool is_mob( char_data* ch )
{
  if( not_player( ch ) )
    return true;

  if( ch->species ) {
    send( ch, "You are unable to do that while switched.\n\r" );
    return true;
  }

  return false;
}


bool not_player( char_data* ch )
{
  if( is_confused_pet( ch ) )
    return true;
 
  if( !ch->pcdata ) {
    fsend_seen( ch, "%s looks around in confusion.", ch );
    return true;
  }

  return false;
}


bool is_confused_pet( char_data* ch )
{
  if( ch->pcdata
      || !ch->leader
      || !is_set( ch->status, STAT_ORDERED ) )
    return false;
 
  if( ch->position >= POS_RESTING ) {
    fsend( ch->leader, "%s looks at you in bewilderment.", ch );
    fsend_seen( ch, "%s looks at %s in bewilderment.",
		ch, ch->leader );
  }

  return true;
}


bool pet_help( char_data* ch ) 
{
  if( ch->pcdata || !ch->leader )
    return false;
 
  fsend( ch->leader, "%s thinks you need help more than %s does.",
	 ch, ch->He_She( ) );
  
  return true;
} 


bool is_humanoid( char_data* ch )
{
  if( !ch->species )
    return true;

  send( ch, "You can only do that in humanoid form.\n\r" );

  return false;
}


bool is_familiar( char_data* ch )
{
  if( !ch->pcdata
      || !ch->species
      || ch->pcdata->pfile->level >= LEVEL_BUILDER )
    return false;
  
  send( ch, "You can't do that while switched.\n\r" );

  return true;
}

 
/*
 *   MAIN COMMAND HANDLER
 */


static bool check_progs( char_data* ch, const char *command, const char *argument, int c )
{
  if( check_actions( ch, command, argument, c ) ) 
    return true;

  static char tmp [ MAX_STRING_LENGTH ];
  static char buf [ MAX_INPUT_LENGTH ];

  int number = smash_argument( tmp, argument );

  if( number <= 0 ) {
    return false;
  }

  obj_data *obj;
  oprog_data *oprog;
  mob_data *rch;
  const char *cmd;

  thing_array stuff = ch->contents;

  for( int i = 0; i < stuff; ++i ) {
    obj = (obj_data*) object( stuff[i] );
    if( obj->Is_Valid( ) ) {
      for( oprog = obj->pIndexData->oprog; oprog; oprog = oprog->next ) {
	if( is_set( oprog->flags, OPFLAG_INVENTORY )
	    && ( cmd = member( command, oprog->command, c ) )
	    && ( !*oprog->target
		 || ( *tmp
		      && *oprog->target == '*'
		      && is_name( tmp, obj->Keywords( ch ) )
		      && ( number -= obj->Number( ) ) <= 0 )
		 || ( *tmp
		      && is_name( tmp, oprog->target )
		      && ( number -= obj->Number( ) ) <= 0 ) ) ) {
	  disrupt_spell( ch );

	  // cmd may have multiple words.
	  one_word( cmd, buf );

	  obj->Select( 1 );
	  clear_variables( );
	  var_ch = ch;
	  var_room = ch->in_room;
	  var_cmd = buf;
	  var_arg = tmp;
	  var_obj = obj;
	  if( !oprog->execute( )
	      || !ch->Is_Valid( ) ) 
	    return true;
	  if( !obj->Is_Valid( ) )
	    break;
	}
      }
    }
  }
  
  stuff = ch->wearing;

  for( int i = 0; i < stuff; ++i ) {
    obj = (obj_data*) stuff[i];
    if( obj->Is_Valid( ) ) {
      for( oprog = obj->pIndexData->oprog; oprog; oprog = oprog->next ) {
	if( is_set( oprog->flags, OPFLAG_WORN )
	    && ( cmd = member( command, oprog->command, c ) )
	    && ( !*oprog->target
		 || ( *tmp
		      && *oprog->target == '*'
		      && is_name( tmp, obj->Keywords( ch ) )
		      && ( number -= obj->Number( ) ) <= 0 )
		 || ( *tmp
		      && is_name( tmp, oprog->target )
		      && ( number -= obj->Number( ) ) <= 0 ) ) ) {
	  disrupt_spell( ch );

	  // cmd may have multiple words.
	  one_word( cmd, buf );

	  obj->Select( 1 );
	  clear_variables( );
	  var_ch = ch;
	  var_room = ch->in_room;
	  var_cmd = buf;
	  var_arg = tmp;
	  var_obj = obj;
	  if( !oprog->execute( )
	      || !ch->Is_Valid( ) ) 
	    return true;
	  if( !obj->Is_Valid( ) )
	    break;
	}
      }
    }
  }
  
  if( ch->array ) {
    stuff = *ch->array;

    for( int i = 0; i < stuff; ++i ) {
      if( ( obj = object( stuff[i] ) ) ) {
	if( obj->Is_Valid( ) ) {
	  for( oprog = obj->pIndexData->oprog; oprog; oprog = oprog->next ) {
	    if( is_set( oprog->flags, OPFLAG_ROOM )
		&& ( cmd = member( command, oprog->command, c ) )
		&& ( !*oprog->target
		     || ( *tmp
			  && *oprog->target == '*'
			  && is_name( tmp, obj->Keywords( ch ) )
			  && ( number -= obj->Number( ) ) <= 0 )
		     || ( *tmp
			  && is_name( tmp, oprog->target )
			  && ( number -= obj->Number( ) ) <= 0 ) ) ) {
	      disrupt_spell( ch );

	      // cmd may have multiple words.
	      one_word( cmd, buf );
	      
	      obj->Select( 1 );
	      clear_variables( );
	      var_ch = ch;
	      var_room = ch->in_room;
	      var_cmd = buf;
	      var_arg = tmp;
	      var_obj = obj;
	      if( !oprog->execute( ) )
		return true;
	      if( !obj->Is_Valid( ) )
		break;
	    }
	  }
	}

      } else if( ( rch = mob( stuff[i] ) )
		 && rch->Is_Valid( )
		 && rch->Seen( ch )
		 && *tmp
		 && is_name( tmp, rch->Keywords( ch ) ) ) {
	for( mprog_data *mprog = rch->species->mprog; mprog; mprog = mprog->next ) {
	  if( mprog->trigger == MPROG_TRIGGER_NONE
	      && *mprog->string
	      && ( cmd = member( command, mprog->string, c ) )
	      && --number == 0 ) {
	    disrupt_spell( ch );

	    // cmd may have multiple words.
	    one_word( cmd, buf );
	    
	    clear_variables( );
	    var_ch = ch;
	    var_room = ch->in_room;
	    var_cmd = buf;
	    var_arg = tmp;
	    var_mob = rch;
	    if( !mprog->execute( ) )
	      return true;
	    if( !rch->Is_Valid( ) )
	      break;
	  }
	}
      }
    }
  }
  
  return false;
}


int find_command( const char_data *ch, const char *argument, bool exact )
{
  int length = strlen( argument );
  int cmd = search( command_table, table_max[ TABLE_COMMAND ], argument );

  if( cmd < 0 || ch && !has_permission( ch, command_table[cmd].level ) ) {
    if( exact )
      return -1;
    if( cmd < 0 )
      cmd = (-cmd)-1;
    for( ; cmd < table_max[ TABLE_COMMAND ]; ++cmd ) {
      if( ch && !has_permission( ch, command_table[cmd].level ) )
	continue;
      if( strncasecmp( command_table[cmd].name, argument, length ) )
	break;
      if( length >= command_table[cmd].reqlen ) {
	return cmd;
      }
    }
  } else {
    return cmd;
  }

  return -1;
}


// Clean up after executing a command.
// Register time elapsed.
static void cleanup_cmd( char_data *ch, const time_data& start, int cmd )
{
  in_character = true;
  msg_type = MSG_STANDARD;

  const time_data time = stop_clock( start );
  
  if( command_table[cmd].max_time < time )
    command_table[cmd].max_time = time;
  
  command_table[cmd].total_time += time;
  ++command_table[cmd].calls;
  
  next_page( ch->link );
}


bool interpret( char_data* ch, const char *argument )
{
  char      command  [ MAX_STRING_LENGTH ];
  const char   *arg;

  skip_spaces( argument );  

  if( !argument || !*argument ) {
    next_page( ch->link );
    return true;
  }

  if( *argument == '+' ) {
    skip_spaces( ++argument );  
    ch->cmd_queue.clear();
    disrupt_spell( ch, true );
    if( ch->active.time != -1 && is_set( ch->status, STAT_WAITING ) ) {
      set_delay( ch, 0, false );
    }
  }

  if( !*argument ) {
    next_page( ch->link );
    return true;
  }

  if( ch->link && ch->link->snoop_by ) {
    send_color( ch->link->snoop_by->player, COLOR_WIZARD, "-%s ", ch->descr->name );
    send( ch->link->snoop_by, argument );
    send( ch->link->snoop_by, "\n\r" );
  }

  if( ch->link )
    ch->link->newline = true;

  clear_pager( ch );

  if( ch->pcdata
      && is_set( ch->pcdata->pfile->flags, PLR_FREEZE ) 
      && !is_god( ch ) ) {
    send( ch, "You're totally frozen!\n\r" );
    return true;
  }

  if( !isalpha( *argument ) && !isdigit( *argument ) ) {
    *command = *argument;
    *(command+1) = '\0';
    arg = argument+1;
    skip_spaces( arg );
  } else {
    arg = one_argument( argument, command );
  }

  /*
  if( ch->pcdata
      && is_confused( ch )
      && number_range( 0, 3 ) == 0 ) {
    confused_char( ch );
    return true;
  }
  */

  const int cmd = find_command( ch, command );

  if( cmd >= 0
      && command_table[cmd].queue
      && ch->active.time != -1 ) {
    const int queued = ch->cmd_queue.entries();
    /*
    if( queued == MAX_CMD_QUEUE ) {
      fsend( ch, "[ Queue Overflow: %s ]", argument );
      return false;
    }
    */
    if( ( ch->cast || queued != 0 || time_till( &ch->active ) > 5 )
	&& ch->pcdata
	&& is_set( ch->pcdata->message, MSG_QUEUE ) )
      fsend( ch, "[ Queued %d: %s ]", queued+1, argument );
    ch->cmd_queue.push( argument, is_set( ch->status, STAT_ORDERED ) );
    return true;
  }

  if( check_progs( ch, command, arg, cmd ) ) {
    if( ( is_avatar( ch ) && !is_god( ch ) ) 
	|| ( ch->pcdata
	     && is_set( ch->pcdata->pfile->flags, PLR_LOGFILE ) ) )
      immortal_log( ch, command, arg );
    set_min_delay( ch, MIN_DELAY, false );
    return true;
  }
  
  if( cmd < 0 ) {
    if( !check_social( ch, command, arg )
	&& !is_confused_pet( ch ) ) {
      send( ch, bad_command );
    }
    return true;
  }

  if( is_set( command_table[cmd].level, PERM_DISABLED ) ) {
    fsend( ch, "The '%s' command has been disabled.",
	   command_table[cmd].name );
    return true;
  }

  /*
  if( command_table[cmd].position > POS_SLEEPING
      && ch->is_affected( AFF_PARALYSIS ) ) {
    send( ch, "You are paralyzed and unable to move!\n\r" );
    return true;
  }
  */
  
  if( ch->position < command_table[cmd].position ) {
    pos_message( ch );
    return true;
  }
  
  if( command_table[cmd].position > POS_FIGHTING && ch->fighting ) {
    send( ch, "The current battle has you occupied.\n\r" );
    return true;
  }
  
  if( ( is_avatar( ch ) && !is_god( ch ) ) 
      || ( ch->pcdata
	   && is_set( ch->pcdata->pfile->flags, PLR_LOGFILE ) ) )
    immortal_log( ch, command, arg );
  
  if( command_table[cmd].disrupt
      && !command_table[cmd].queue ) {
    if( ch->cmd_queue.entries() != 0 ) {
      if( ch->pcdata
	  && is_set( ch->pcdata->message, MSG_QUEUE ) ) {
	fsend( ch, "[ De-queued: %d commands ]", ch->cmd_queue.entries() );
      }
      ch->cmd_queue.clear();
    }
    disrupt_spell( ch );
  }

  if( command_table[cmd].reveal )
    spoil_hide( ch );
  
  ch->Show( 1 );

  time_data start;
  gettimeofday( &start, 0 );
  
  if( Tprog_Data *tprog = command_table[cmd].prog ) {
    clear_variables( );
    var_ch = ch;
    var_cmd = command;
    var_arg = arg;
    const int result = tprog->execute( );
    if( !result
	|| !ch->Is_Valid( ) ) {
      cleanup_cmd( ch, start, cmd );
      return true;
    }
  }

  if( !command_table[cmd].function ) {
    send( ch, "Command \"%s\" has no routine assigned to it.\n\r", 
	  command_table[cmd].name );
    bug( "Interpret: command \"%s\" has no routine.",
	 command_table[cmd].name );
    cleanup_cmd( ch, start, cmd );
    return true;
  }
  
  ( *command_table[cmd].function ) ( ch, arg );

  cleanup_cmd( ch, start, cmd );

  /*
  in_character = true;
  msg_type = MSG_STANDARD;

  const time_data time = stop_clock( start );
  
  if( command_table[cmd].max_time < time )
    command_table[cmd].max_time = time;
  
  command_table[cmd].total_time += time;
  ++command_table[cmd].calls;
  
  next_page( ch->link );
  */

  return true;
}


/*
 *   DO_COMMAND ROUTINE
 */


void do_commands( char_data* ch, const char *argument )
{
  int        i;
  int        j;
  int    flags;
  int       pf;
  bool   found  = false;

  if( pet_help( ch ) )
    return;

  if( !get_flags( ch, argument, &flags, "p", "Commands" ) )
    return;

  if( is_set( flags, 0 ) ) {
    for( pf = 0; pf < MAX_PERMISSION; ++pf ) 
      if( matches( argument, permission_name[pf] ) )
        break;
    if( pf == MAX_PERMISSION ) {
      send( ch, "Unknown permission flag.\n\r" );
      return;
    }
    if( !has_permission( ch, pf ) ) {
      send( ch,
	    "You cannot view commands you do not have permission for.\n\r" );
      return;
    }
    for( i = 0, j = 0; i < table_max[ TABLE_COMMAND ]; ++i ) {
      if( is_set( command_table[i].level, pf ) ) {
        if( !found ) {
          page_title( ch, "Commands requiring %s flag", 
		      permission_name[pf] );
          found = true;
	}
        page( ch, "%19s%s", command_table[i].name,
	      ++j%4 ? "" : "\n\r" );
      } 
    }
    if( !found ) 
      fsend( ch,
	     "There are no commands which require the %s permission flag.",
	     permission_name[pf] );
    else if( j%4 != 0 )
      page( ch, "\n\r" );
    return;
  }
  
  int trust = get_trust( ch );
  
  int max = table_max[ TABLE_CMD_CAT ];
  int sorted[ max ];
  max = sort_names( &cmd_cat_table[0].name, &cmd_cat_table[1].name,
		    sorted, max );

  if( *argument == '\0' ) {
    page_title( ch, "Command Categories" );
    for( i = 0, j = 0; i < max; ++i ) {
      int k = sorted[i];
      if( trust >= cmd_cat_table[k].level ) {
        page( ch, "%19s%s", cmd_cat_table[k].name,
	      ++j%4 ? "" : "\n\r" );
      }
    }
    page( ch, "\n\r%s", j%4 != 0 ? "\n\r" : "" );
    page_centered( ch, "[ Type command <category> to see a list commands\
 in that category. ]" );
    return;
  }
  
  for( i = 0; i < max; ++i ) {
    int k = sorted[i];
    if( trust >= cmd_cat_table[k].level 
	&& matches( argument, cmd_cat_table[k].name ) ) {
      page_title( ch, "Commands - %s", cmd_cat_table[k].name );
      for( j = 0; j < table_max[ TABLE_COMMAND ]; ++j ) {
        if( command_table[j].category == k 
	    && has_permission( ch, command_table[j].level ) ) {
          page( ch, "%15s : %s\n\r", command_table[j].name,
		command_table[j].help );
	}
      }
      return;
    }
  }
  
  send( ch, "Unknown command category.\n\r" );
}
