#include <ctype.h>
#include <sys/types.h>
#include <stdio.h>
#include <syslog.h>
#include "define.h"
#include "struct.h"


static void social_effects( char_data *ch, const Social_Type *soc )
{
  if( soc->disrupt ) {
    if( ch->cmd_queue.entries() != 0 ) {
      if( ch->pcdata
	  && is_set( &ch->pcdata->message, MSG_QUEUE ) ) {
	fsend( ch, "[ De-queued: %d commands ]", ch->cmd_queue.entries() );
      }
      ch->cmd_queue.clear();
    }
    disrupt_spell( ch );
  }

  if( soc->reveal ) {
    spoil_hide( ch );
  }
}


default_data social_msg [] =
{
  { "to_char",  empty_string, -1 },
  { "to_room",  empty_string, -1 },
  { "to_victim",  empty_string, -1 },
  { "", "", -1 }
};


static void social_no_arg( char_data* ch, Social_Type* soc1, Social_Type* soc2 )
{
  if( soc1->char_no_arg == empty_string )
    soc1 = soc2;

  if( Tprog_Data *tprog = soc1->prog ) {
    clear_variables( );
    var_ch = ch;
    var_i = -1;
    social_msg[0].msg = soc1->char_no_arg;
    social_msg[1].msg = soc1->others_no_arg;
    social_msg[2].msg = empty_string;
    var_def = social_msg;
    var_def_type = -1;
    const int result = tprog->execute( );
    if( !result
	|| !ch->Is_Valid( ) ) {
      return;
    }
  }

  social_effects( ch, soc1 );

  act_color = COLOR_SOCIAL;
  act( ch, soc1->char_no_arg, ch );
  act_social_room( soc1->others_no_arg, ch );
  act_color = COLOR_DEFAULT;
}


static void social_auto( char_data* ch, Social_Type* soc1, Social_Type* soc2 )
{
  if( soc1->char_auto == empty_string )
    soc1 = soc2;

  if( Tprog_Data *tprog = soc1->prog ) {
    clear_variables( );
    var_ch = ch;
    var_victim = ch;
    var_i = -1;
    social_msg[0].msg = soc1->char_auto;
    social_msg[1].msg = soc1->others_auto;
    social_msg[2].msg = empty_string;
    var_def = social_msg;
    var_def_type = -1;
    const int result = tprog->execute( );
    if( !result
	|| !ch->Is_Valid( ) ) {
      return;
    }
  }

  social_effects( ch, soc1 );

  act_color = COLOR_SOCIAL;
  act( ch, soc1->char_auto, ch );
  act_social_room( soc1->others_auto, ch );
  act_color = COLOR_DEFAULT;
}


static void social_victim( char_data* ch, Social_Type* soc1,
			   Social_Type* soc2, char_data* victim )
{
  if( soc1->char_found == empty_string ) {
    if( soc2->char_found == empty_string ) {
      fsend( ch, "%s and %s do nothing together.",
	     soc1->name, victim );
      return;
    }
    soc1 = soc2;
  }

  if( Tprog_Data *tprog = soc1->prog ) {
    clear_variables( );
    var_ch = ch;
    var_victim = victim;
    var_i = -1;
    social_msg[0].msg = soc1->char_found;
    if( victim->position > POS_SLEEPING ) {
      social_msg[1].msg = soc1->vict_found;
    } else {
      social_msg[1].msg = soc1->vict_sleep;
    }
    social_msg[2].msg = soc1->others_found;
    var_def = social_msg;
    var_def_type = -1;
    const int result = tprog->execute( );
    if( !result
	|| !ch->Is_Valid( )
	|| !victim->Is_Valid( ) ) {
      return;
    }
  }

  social_effects( ch, soc1 );

  act_color = COLOR_SOCIAL;
  act( ch, soc1->char_found, ch, victim );

  if( victim->position > POS_SLEEPING ) {
    act_social( victim, soc1->vict_found, ch, victim );
  } else if( victim->position == POS_SLEEPING ) {
    act( victim, soc1->vict_sleep, ch, victim );
  }
  
  act_social_room( soc1->others_found, ch, victim );
  act_color = COLOR_DEFAULT;
}


/*
 *   DIRECTION AND SOCIAL
 */


static void social_dir( char_data* ch, Social_Type* soc1, Social_Type* soc2,
			int dir )
{
  if( soc1->dir_self == empty_string ) {
    soc1 = soc2;
    if( soc1->dir_self == empty_string ) {
      fsend( ch, "%s and a direction does nothing.", 
	     soc1->name );
      return;
    }
  }
  
  if( Tprog_Data *tprog = soc1->prog ) {
    clear_variables( );
    var_ch = ch;
    var_i = dir;
    social_msg[0].msg = soc1->dir_self;
    social_msg[1].msg = soc1->dir_others;
    social_msg[2].msg = empty_string;
    var_def = social_msg;
    var_def_type = -1;
    const int result = tprog->execute( );
    if( !result
	|| !ch->Is_Valid( ) ) {
      return;
    }
  }

  social_effects( ch, soc1 );

  act_color = COLOR_SOCIAL;
  act( ch, soc1->dir_self, ch, 0, dir_table[dir].name );
  act_social_room( soc1->dir_others, ch, dir_table[dir].name );
  act_color = COLOR_DEFAULT;
}


/*
 *   OBJECT AND SOCIAL
 */


static void social_obj( char_data* ch, Social_Type* soc1, Social_Type* soc2,
			obj_data* obj )
{
  if( soc1->obj_self == empty_string ) {
    if( soc2->obj_self == empty_string ) {
      fsend( ch, "%s and an object does nothing.", soc2->name );
      return;
    }
    soc1 = soc2;
  }
  
  if( Tprog_Data *tprog = soc1->prog ) {
    clear_variables( );
    var_ch = ch;
    var_obj = obj;
    var_i = -1;
    social_msg[0].msg = soc1->obj_self;
    social_msg[1].msg = soc1->obj_others;
    social_msg[2].msg = empty_string;
    var_def = social_msg;
    var_def_type = -1;
    const int result = tprog->execute( );
    if( !result
	|| !ch->Is_Valid( )
	|| !obj->Is_Valid( ) ) {
      return;
    }
  }

  social_effects( ch, soc1 );

  act_color = COLOR_SOCIAL;
  act( ch, soc1->obj_self, ch, 0, obj );
  act_social_room( soc1->obj_others, ch, 0, obj );
  act_color = COLOR_DEFAULT;
}


/*
 *   OBJECT, VICTIM AND SOCIAL
 */


static void social_self_obj( char_data* ch, Social_Type* soc1, Social_Type* soc2,
			     obj_data* obj )
{
  if( soc1->self_obj_self == empty_string ) {
    if( soc2->self_obj_self == empty_string ) {
      fsend( ch, "Mixing %s and %s does nothing interesting.",
	     soc2->name, obj );
      return;
    }
    soc1 = soc2;
  }

  if( Tprog_Data *tprog = soc1->prog ) {
    clear_variables( );
    var_ch = ch;
    var_victim = ch;
    var_obj = obj;
    var_i = -1;
    social_msg[0].msg = soc1->self_obj_self;
    social_msg[1].msg = soc1->self_obj_others;
    social_msg[2].msg = empty_string;
    var_def = social_msg;
    var_def_type = -1;
    const int result = tprog->execute( );
    if( !result
	|| !ch->Is_Valid( )
	|| !obj->Is_Valid( ) ) {
      return;
    }
  }

  social_effects( ch, soc1 );

  act_color = COLOR_SOCIAL;
  act( ch, soc1->self_obj_self, ch, 0, obj );
  act_social_room( soc1->self_obj_others, ch, 0, obj );
  act_color = COLOR_DEFAULT;
}


static void social_ch_obj( char_data* ch, Social_Type* soc1, Social_Type* soc2,
			   char_data* victim, obj_data* obj )
{
  if( ch == victim ) {
    social_self_obj( ch, soc1, soc2, obj );
    return;
  }
  
  if( soc1->ch_obj_self == empty_string ) {
    if( soc2->ch_obj_self == empty_string ) {
      fsend( ch, "Mixing %s, %s and %s does nothing interesting.",
	     soc2->name, victim, obj );
      return;
    }
    soc1 = soc2;
  }
  
  if( Tprog_Data *tprog = soc1->prog ) {
    clear_variables( );
    var_ch = ch;
    var_victim = victim;
    var_obj = obj;
    var_i = -1;
    social_msg[0].msg = soc1->ch_obj_self;
    if( victim->position > POS_SLEEPING ) {
      social_msg[1].msg = soc1->ch_obj_victim;
    } else {
      social_msg[1].msg = soc1->ch_obj_sleep;
    }
    social_msg[2].msg = soc1->ch_obj_others;
    var_def = social_msg;
    var_def_type = -1;
    const int result = tprog->execute( );
    if( !result
	|| !ch->Is_Valid( )
	|| !victim->Is_Valid( )
	|| !obj->Is_Valid( ) ) {
      return;
    }
  }

  social_effects( ch, soc1 );

  act_color = COLOR_SOCIAL;
  act( ch, soc1->ch_obj_self, ch, victim, obj );
  
  if( victim->position > POS_SLEEPING ) {
    act_social( victim, soc1->ch_obj_victim, ch, victim, obj );
  } else if( victim->position == POS_SLEEPING ) {
    act( victim, soc1->ch_obj_sleep, ch, victim, obj );
  }
  
  act_social_room( soc1->ch_obj_others, ch, victim, obj );
  act_color = COLOR_DEFAULT;
}


static Social_Type *find_social( Social_Type* table, int max, const char *command )
{
  if( max == 0 )
    return 0;

  int cmd;

  if( ( cmd = search( table, max, command ) ) < 0 )
    cmd = -cmd-1;

  if( cmd == max
      || strncasecmp( command, table[cmd].name, strlen( command ) ) )
    return 0;

  return &table[cmd];
}


/*
 *   MAIN HANDLER
 */


bool check_social( char_data* ch, const char *command, const char *argument,
		   thing_data *target )
{
  Social_Type *soc1 = 0;

  if( ch->shdata->race < MAX_PLYR_RACE )
    soc1 = find_social( social_table[ ch->shdata->race+1 ],
			table_max[ ch->shdata->race+1 ], command );

  Social_Type *soc2 = find_social( social_table[0], 
				   table_max[TABLE_SOC_DEFAULT], command );

  if( !soc1 && !( soc1 = soc2 ) ) {
    if( !argument )
      bug( "Check_Social: Social %s not found.", command );
    return false;
  }

  if( !soc2 )
    soc2 = soc1;

  if( ch->pcdata
      && argument
      && is_set( ch->pcdata->pfile->flags, PLR_NO_EMOTE ) ) {
    send( ch, "You are anti-social!\n\r" );
    return true;
  }

  if( ch->position < soc1->position ) {
    if( argument )
      pos_message( ch );
    return true;

  } else if( soc1->position >= POS_STANDING ) {
    // Fighting position is odd, since ch position is never fighting.
    if( is_fighting( ch, argument ? empty_string : 0 ) ) {
      return true;
    }
  }
    
  msg_type = MSG_STANDARD;

  if( ( !argument || !*argument ) && !target ) {
    social_no_arg( ch, soc1, soc2 );
    return true;
  }

  thing_data *t1 = 0;
  int dir1 = -1;
  
  if( argument ) {
    char arg [ MAX_INPUT_LENGTH ];

    argument = one_argument( argument, arg );
    
    if ( soc1->dir_self != empty_string
	 || soc2->dir_self != empty_string ) {
      // Can be used with a direction.
      const char *a = arg;
      dir1 = direction_arg( a );
    }
    
    if( dir1 < 0 ) {
      if( !( t1 = one_thing( ch, arg, target ? 0 : "social",
			     &ch->contents,
			     &ch->wearing,
			     ch->array ) ) )
	return true;
    }

  } else {
    t1 = target;
  }

  obj_data *obj = object( t1 );
  char_data *victim = character( t1 );

  if( !argument || !*argument ) {
    if( dir1 >= 0 )
      social_dir( ch, soc1, soc2, dir1 );
    else if( obj )
      social_obj( ch, soc1, soc2, obj );
    else if( victim == ch )
      social_auto( ch, soc1, soc2 );
    else
      social_victim( ch, soc1, soc2, victim );
    return true;
  }

  int dir2 = -1;
  if ( soc1->dir_self != empty_string
       || soc2->dir_self != empty_string ) {
    // Can be used with a direction.
    dir2 = direction_arg( argument );
  }

  thing_data *t2 = 0;  
  if( dir2 < 0 ) {
    if( !( t2 = one_thing( ch, argument, "social",
			   &ch->contents,
			   &ch->wearing,
			   ch->array ) ) )
      return true;
  }

  if( !obj )
    obj = object( t2 );

  if( !victim )
    victim = character( t2 );

  if( obj && victim ) {
    social_ch_obj( ch, soc1, soc2, victim, obj );
  } else if( dir1 >= 0 || dir2 >= 0 ) {
    fsend( ch, "Using %s like that doesn't make sense.", soc1->name );
  } else {
    fsend( ch, "Mixing %s, %s, and %s does nothing interesting.",
	   soc1->name, t1, t2 );
  }
 
  return true;
}


/* 
 *   DISPLAY SOCIAL LIST
 */


void do_socials( char_data* ch, const char *argument )
{
  if( !ch->pcdata )
    return;

  int table = TABLE_SOC_DEFAULT;

  if( !*argument ) {
    page_title( ch, "Default Socials" );
  } else {
    for( table = 0; ; table++ ) {
      if( table == MAX_PLYR_RACE ) {
        send( ch, "Syntax: Social [race]\n\r" );
        return;
      }
      if( matches( argument, plyr_race_table[table].name ) )
        break;
    }
    page_title( ch, "%s Socials", plyr_race_table[table].name );
    table++;
  }

  const unsigned columns = ch->pcdata->columns / 19;

  int i;
  for( i = 0; i < table_max[table]; ++i ) {
    page( ch, "%19s", social_table[table][i].name );
    if( i%columns == columns-1 )
      page( ch, "\n\r" );
  }
  if( i%columns != 0 )
    page( ch, "\n\r" );
}
