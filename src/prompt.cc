#include <ctype.h>
#include <sys/types.h>
#include <stdio.h>
#include <syslog.h>
#include "define.h"
#include "struct.h"


/*
 *   PROMPT LIST
 */


const char* prompt_default = 
  "?p'-- MORE -- '<%f?f|%hhp %ee ?m'[%mmv]'!m'%vmv' ?b'%c'!b'%d'> ";

const char* prompt_immortal =
  "?p'-MORE- '?f'%f '%T %d> ";

const char* prompt_warrior =
  "?p'-- MORE -- '<%f?f|%hhp ?m'[%mmv]'!m'%vmv'> ";

const char* prompt_cleric = 
  "?p'-- MORE -- '<%f?f|%hhp %ee ?m'[%mmv]'!m'%vmv'?l' %C'> ";

const char* prompt_simple = 
  "?p'-- MORE -- '<%f?f|%hhp %ee ?m'[%mmv]'!m'%vmv'> ";

const char* prompt_complex = 
  "?p' -- MORE -- '<%f?f|%hhp %ee ?m'[%mmv]'!m'%vmv' %gwm %xxp?l' %C'\
 %d?b' %c'> ";

const char* prompt_color = 
  "?p' -- MORE -- '<%f?f|@R%hhp@n @G%ee@n @B?m'[%mmv]'!m'%vmv'@n %gwm\
 %xxp?l' %C' %d?b' %c'> ";


/*
 *   DO_PROMPT COMMAND   
 */


void do_prompt( char_data* ch, const char *argument )
{
  const char* prompt_list [] = {
    "default",   empty_string,
    "immortal",  prompt_immortal,
    "simple",    prompt_simple, 
    "warrior",   prompt_warrior,
    "cleric",    prompt_cleric,
    "complex",   prompt_complex,
    "color",     prompt_color,
    "" };

  int  i;
  int  j;

  if( not_player( ch ) )
    return;

  if( !*argument ) {
    send( ch, "Prompt:\n\r%s\n\r\n\r", ch->pcdata->prompt == empty_string
	  ? prompt_default : ch->pcdata->prompt );
    send( ch, "Set your prompt to what?\n\r" );
    return;
  }
  
  if( strlen( argument ) > 120 ) {
    send( ch, "You can't set a prompt longer than 120 characters.\n\r" );
    return;
  }
  
  for( i = 0, j = 0; argument[i]; ++i ) {
    if( argument[i] == '%' ) {
      ++j;
      ++i;
    }
  }
  
  if( j > 20 ) {
    send( ch, "A prompt may contain at most 20 substitutions.\n\r" );
    return;
  }
  
  free_string( ch->pcdata->prompt, MEM_PLAYER );
  
  for( i = 0; *prompt_list[2*i]; ++i )
    if( !strcasecmp( argument, prompt_list[2*i] ) ) {
      ch->pcdata->prompt = alloc_string( prompt_list[2*i+1], MEM_PLAYER );  
      return;
    }

  ch->pcdata->prompt = alloc_string( argument, MEM_PLAYER );
}


/*
 *   SUPPORT FUNCTIONS
 */


static void prompt_flags( char *flags, char_data *ch )
{
  const bool request = ( ( !request_app.is_empty() 
			   && has_permission( ch, PERM_APPROVE )
			   && !is_apprentice( ch ) )
			 //		     && ch->Level() < LEVEL_APPRENTICE )
			 || ( !request_imm.is_empty()
			      && is_apprentice( ch ) ) );

  bool arena = false;
  room_data *room = Room( ch->array->where );
  if( room && is_set( room->room_flags, RFLAG_ARENA ) )
    arena = true;
  
  
  snprintf( flags, 20, "%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s",
	    request                                            	  ? "R" : "",
	    in_sanctuary( ch, false )                         	  ? "S" : "", 
	    !arena && can_pkill( ch, 0, false )               	  ? "P" : "", 
	    arena                                                 ? "A" : "",
	    ch->is_affected( AFF_INVISIBLE )                  	  ? "i" : "",
	    is_set( ch->status, STAT_HIDING )                 	  ? "h" : "",
	    is_set( ch->status, STAT_CAMOUFLAGED )            	  ? "c" : "",
	    is_set( ch->status, STAT_BERSERK )                	  ? "b" : "",
	    is_set( ch->status, STAT_FOCUS )                	  ? "f" : "",
	    is_set( ch->status, STAT_SNEAKING )                   ? "s" : "",
	    is_set( ch->status, STAT_COVER_TRACKS )               ? "C" : "",
	    is_set( ch->pcdata->pfile->flags, PLR_WIZINVIS )      ? "w" : "",
	    is_set( ch->pcdata->pfile->flags, PLR_TRACK )         ? "t" : "",
	    is_set( ch->pcdata->pfile->flags, PLR_SEARCHING )     ? "x" : "",
	    is_set( ch->pcdata->pfile->flags, PLR_PARRY )         ? "p" : "",
	    is_set( ch->pcdata->pfile->flags, PLR_NO_PRIVILEGES ) ? "I" : "",
	    ch->get_burden( ) > 0                                 ? "B" : "" );
}


/*
 *  STATUS BAR PROMPT ROUTINE
 */


void prompt_ansi( link_data* link )
{
  char          tmp  [ FIVE_LINES ];
  char        flags  [ 20 ];
  char        exits  [ TWO_LINES ];
  char         time  [ 10 ];
  char_data *ch  = link->character;

  set_bit( ch->status, STAT_NO_SNOOP );

  move_cursor( ch, ch->pcdata->lines-1, 1 ); 
  
  prompt_flags( flags, ch );

  const int i = exits_prompt( exits, ch );
  add_spaces( exits, 6-i );

  if( ch->Level() >= LEVEL_APPRENTICE ) {
    sprintf_time( time, current_time, ch->pcdata->pfile->account->timezone );
    snprintf( tmp, FIVE_LINES,
      "-- Time: %s   Room: %-5d   Hp: %-4d En: %-4d Mv: %-4d Ex: %s %8s --",
      time, ch->in_room->vnum, ch->hit, ch->mana, ch->move,
      exits, flags );
  } else
    snprintf( tmp, FIVE_LINES,
	      "-- Hp: %s%4d/%-4d%s En: %s%4d/%-4d%s Mv: %s%4d/%-4d%s\
 Xp: %-8d Ex: %s %8s --",
	      bold_red_v( ch ), ch->hit, ch->max_hit, normal( ch ),
	      blue( ch ), ch->mana, ch->max_mana, normal( ch ),
	      green( ch ), ch->move, ch->max_move, normal( ch ),
	      ch->species ? 0 : ( exp_for_level( ch )-ch->exp ),
	      exits, flags );
  
  send( ch, tmp );

  remove_bit( ch->status, STAT_NO_SNOOP );
}


/*
 *   SETTABLE PROMPT ROUTINE
 */


static int sprintf_prompt( char *output, char_data* ch, const char* input,
			    const char *flags ) 
{
  char              tmp  [ TWO_LINES ]; 
  int             i,j,k;
  bool              opt;
  int            length  = strlen( input );
  player_data*   player  = ch->link->player;
  bool             perm  = is_apprentice( ch );
  room_data *room = ch->in_room;
  int last_color = -1;
  
  for( i = 0, j = 0; i < length; ++i ) {
    if( input[i] == '@' ) {
      for( i++, k = 0; ; k++ ) {
        if( !color_key[k] ) {
          output[j++] = input[i];
          break;
	} 
        if( color_key[k] == input[i] && k != last_color ) {
          if( ch->pcdata->terminal != TERM_DUMB ) {
            strcpy( output+j,
		    color_code( ch, k == 0 ? COLOR_DEFAULT : COLOR_MILD+k-1 ) );
            j += strlen( &output[j] );
	    last_color = k ? k : -1;
	  }
          break;
	}
      }
      continue;
    }

    if( input[i] == '%' ) {
      switch( input[++i] ) {
      case 'h'  : sprintf( output+j, "%d", ch->hit );               break;
      case 'H'  : sprintf( output+j, "%d", ch->max_hit );           break;
      case 'e'  : sprintf( output+j, "%d", ch->mana );              break;
      case 'E'  : sprintf( output+j, "%d", ch->max_mana );          break;
      case 'v'  : sprintf( output+j, "%d", ch->move );              break;
      case 'V'  : sprintf( output+j, "%d", ch->max_move );          break;
      case 'g'  : sprintf( output+j, "%d", min_group_move( ch ) );  break;
      case 'G'  : sprintf( output+j, "%d", player->gossip_pts );    break;
      case 'i'  : sprintf( output+j, "%d", min_group_hit( ch ) );   break;
      case 'f'  : strcpy( output+j, flags );                        break;
      case '\\' : strcpy( output+j, "\n\r" );                       break;
      case 'd'  :
	exits_prompt( output+j, ch,
		      last_color > 0 ? COLOR_MILD+last_color-1 : COLOR_DEFAULT );
	break;
	
      case 'c' : 
	strcpy( output+j, condition_short( ch, ch->fighting ) );
	break;
	
      case 'C' : 
	strcpy( output+j, condition_short( ch, ch->leader ) );
	break;
	
      case 's' : 
	strcpy( output+j, condition_short( ch ) );
	break;
	
      case 't' :
	sprintf( output+j,
		 room->sky_state( ) );
	//         sprintf( output+j, "%u:%02u", weather.hour, weather.minute );
	break;

       case 'T' :
         sprintf_time( output+j, current_time, ch->pcdata->pfile->account->timezone );
         break;

       case 'x' :
        sprintf( output+j, "%d",
		 ch->species ? 0 : exp_for_level( ch )-ch->exp );
        break;

       case 'm' :
        if( ch->mount )
          sprintf( output+j, "%d", ch->mount->move );
        else
          sprintf( output+j, "-" );
        break;

       case 'M' :
        if( ch->mount )
          sprintf( output+j, "%d", ch->mount->max_move );
        else
          sprintf( output+j, "-" );
        break;

      case 'r':
	if( perm || room->Seen( ch ) ) {
	  strcpy( output+j, room->name );
	} else {
	  strcpy( output+j, "dark" );
	}
	break;

      case 'R':
	if( perm ) {
	  sprintf( output+j, "%d", room->vnum );
	} else {
	  strcpy( output+j, room->Seen( ch ) ? room->name : "dark" );
	}
	break;
	
      default:
        output[j]   = input[i];
        output[j+1] = '\0';
      }
      j += strlen( output+j );
      continue;
    }

    if( input[i] == '?' || input[i] == '!' ) {      
      opt = ( ( input[++i] == 'p' && ch->link->paged )
	      || ( input[i] == 'f' && *flags )
	      || ( input[i] == 'm' && ch->mount )
	      || ( input[i] == 'b' && ch->fighting )
	      || ( input[i] == 'l' && ch->leader ) );
      if( input[i-1] == '!' )
        opt = !opt;
      if( input[++i] != '\'' ) {
        if( opt )
          output[j++] = input[i];
        continue;
      }
      
      for( k = 0, i++; input[i] && input[i] != '\''; i++ )
        tmp[k++] = input[i];
      tmp[k] = '\0';
      
      if( opt ) {
        sprintf_prompt( output+j, ch, tmp, flags );
        j += strlen( output+j ); 
      }

      continue;
    }
    
    output[j++] = input[i];
  }

  output[j] = '\0';

  return j;
}


/* 
 *   NORMAL PROMPT ROUTINE
 */


void prompt_nml( link_data *link )
{
  static char tmp  [ EIGHT_LINES ];
  static char flags  [ 10 ];

  char_data *ch  = link->character;

  set_bit( ch->status, STAT_NO_SNOOP );

  prompt_flags( flags, ch );

  send( link, "\n\r%s", normal( ch ) );
  int i = sprintf_prompt( tmp, ch, ch->pcdata->prompt == empty_string
			  ? prompt_default : ch->pcdata->prompt, flags );

  bool input = ch->pcdata->color[ COLOR_INPUT ] != 0
               && ( ch->pcdata->color[ COLOR_INPUT ]
		    != ch->pcdata->color[ COLOR_DEFAULT ] );

  if( input ) {
    strcpy( tmp+i, color_code( ch, COLOR_INPUT ) );
  } else {
    strcpy( tmp+i, color_code( ch, COLOR_DEFAULT ) );
  }

  send( link, tmp );

  if( input ) {
    link->prompted = true;
  }

  remove_bit( ch->status, STAT_NO_SNOOP );
}
