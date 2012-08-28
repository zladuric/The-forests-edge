#include <ctype.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include "define.h"
#include "struct.h"
 

/*
 *   LOCAL FUNCTIONS
 */


static const void *proc_arg ( arg_type*, program_data*, thing_data* );

bool end_prog;
bool cont_prog;
bool return_prog;
bool queue_prog;
bool now_prog;

queue_data *queue_list = 0;
queue_data *now_list = 0;
stack_data *stack_list = 0;

arg_type *curr_arg;
program_data *curr_prog = 0;

/*
 *   VARIABLES/CONSTANTS
 */


char_data*    var_victim;
char_data*    var_rch;
char_data*    var_mob;  
char_data*    var_ch;
obj_data*     var_obj;
obj_data*     var_act_obj;
obj_data*     var_container;
room_data*    var_room;
const char   *var_cmd;
const char   *var_arg;
int           var_i;
int           var_j;
int           var_k;
int           var_l;
int           var_m;
int           var_n;
exit_data*    var_exit;
const default_data *var_def;
int var_def_type;
thing_data *var_thing;
thing_array var_list;


char_data *orig_ch;
char_data *orig_victim;


/*
 *   BUG ROUTINE
 */


static const char *code_thing_name( const thing_data *thing )
 {
  if( !thing )
    return "NULL";

  return thing->Name( );

  return "???";
}


static const char *code_ch_name( const char_data *ch )
 {
  if( !ch )
    return "NULL";

  if( ch->descr->name && *ch->descr->name ) {
    return ch->descr->name;
  }

  if( ch->descr->singular ) {
    if( *ch->descr->singular == '!' )
      return ch->descr->singular+1;
    return ch->descr->singular;
  }

  return "???";
}


static const char *code_exit_name( const exit_data *exit )
 {
  if( !exit )
    return "NULL";

  return exit->Name( );
}


void code_bug( const char *text )
{
  char tmp [ MAX_INPUT_LENGTH ];

  bug( text );

  snprintf( tmp, MAX_INPUT_LENGTH, "-- Char = %s,  Mob = %s,  Victim = %s",
	    code_ch_name( var_ch ),
	    code_ch_name( var_mob ),
	    code_ch_name( var_victim ) );
  bug( tmp );
  
  snprintf( tmp, MAX_INPUT_LENGTH, "-- Rch = %s,  Room #%d,  Obj = %s",
	    code_ch_name( var_rch ),
	    var_room ? var_room->vnum : -1,
	    code_thing_name( var_obj ) );
  bug( tmp );

  snprintf( tmp, MAX_INPUT_LENGTH, "-- Exit = %s, Thing = %s",
	    code_exit_name( var_exit ), code_thing_name( var_thing ) );
  bug( tmp );

  snprintf( tmp, MAX_INPUT_LENGTH, "-- i = %d,  j = %d,  k = %d",
	    var_i, var_j, var_k );  
  bug( tmp );
  
  snprintf( tmp, MAX_INPUT_LENGTH, "-- l = %d,  m = %d,  n = %d",
	    var_l, var_m, var_n );
  bug( tmp );
}


/*
 *   RUN-TIME FUNCTIONS 
 */


static void save_variables( )
{
  orig_ch = var_ch;
  orig_victim = var_victim;
}


int program_data::execute( thing_data *owner )
{
  if( !binary ) {
    compile( );
    if( !binary ) {
      //      clear_variables( );
      return 0;
    }
  }
  
  if( active > 50 ) {
    code_bug( "Execute: infinite recursive loop." );
    //    clear_variables( );
    return 0;
  }

  save_variables( );

  end_prog = false;
  cont_prog = false;
  return_prog = false;
  queue_prog = false;
  now_prog = false;

  program_data *save_prog = curr_prog;
  curr_prog = this;

  ++active;
  proc_arg( binary, this, owner );

  curr_prog = save_prog;

  if( queue_prog ) {
    //    queue_list->program = this;
    queue_list->owner = owner;
    queue_list->arg = curr_arg;
  } else if( now_prog ) {
    //    now_list->program = this;
    now_list->owner = owner;
    now_list->arg = curr_arg;
  } else {
    --active;
  }

  const int flag = return_prog
    ? -1
    : ( cont_prog
	? 1
	: 0 );

  end_prog = false;
  cont_prog = false;
  return_prog = false;
  queue_prog = false;
  now_prog = false;

  //  clear_variables( );

  return flag;
}


static const void *one_arg( arg_type* arg, program_data *program, thing_data *owner )
{
  if( arg->family == null ) {
    return 0;
  }

  if( arg->family == cont ) {
    cont_prog = true;
    end_prog  = true;
    return 0;
  }

  if( arg->family == ret ) {
    cont_prog = true;
    return_prog = true;
    end_prog  = true;
    return 0;
  }

  if( arg->family == end ) {
    end_prog = true;
    return 0;
  }

  if( arg->family == constant ) {
   return arg->value;
  }

  if( arg->family == variable ) {
    const void *result = *(void**)arg->value;
    if( arg->logic_neg ) {
      return (void *) !result;
    }
    if( arg->arith_neg ) {
      return (void*) -(int)result;
    }
    if( arg->bit_neg ) {
      return (void*) ~(int)result;
    }
    return result;
  }

  if( arg->family == loop ) {
    loop_type *aloop  = (loop_type*) arg;
    char_data *leader = var_ch;
   
    if( aloop->fruit == loop_unknown ) {
      int i;
      for( i = 0; i < 100; ++i ) {
        if( ( proc_arg( aloop->condition, program, owner ) == 0 ) != aloop->logic_neg )
          break;
        proc_arg( aloop->aloop, program, owner );
	if( end_prog )
	  return 0;
      }
      if( i == 100 )
        code_bug( "Proc_arg: Infinite script loop." );
      return 0;
    }

    if( aloop->fruit == loop_rooms_in_area ) {
      if( !var_room ) {
	code_bug( "Proc_arg: NULL room in loop_room_in_area." );
	return 0;
      }

      room_data *save_room = var_room;

      for( room_data *room = save_room->area->room_first; room; room = room->next ) {
	var_room = room;
	proc_arg( aloop->aloop, program, owner );
	if( end_prog )
	  return 0;
      }

      var_room = save_room;

      return 0;
    }

    if( aloop->fruit == loop_all_on_chair ) {
      if( !var_obj ) {
	code_bug( "Proc_arg: NULL obj in loop_all_on_chair." );
	return 0;
      }

      if( var_obj->pIndexData->item_type != ITEM_CHAIR ) {
	code_bug( "Proc_arg:obj in loop_all_on_chair is not a chair." );
	return 0;
      }

      obj_data *save_obj = var_obj;
      Content_Array content = save_obj->contents;
      for( int i = 0; i < content; ++i ) {
	char_data *rch = character( content[i] );
	if( !rch
	    || !rch->Is_Valid( )
	    || !save_obj->Is_Valid( )
	    || rch->pos_obj != save_obj
	    || invis_level( rch ) >= LEVEL_BUILDER )
	  continue; 
	
	var_rch = rch;
	proc_arg( aloop->aloop, program, owner ); // note that this can reset the var_ variables
	if( end_prog )
	  return 0;
	if( program->abort( owner ) ) {
	  end_prog = true;
	  return 0;
	}
      }
      return 0;
    }

    if( !var_room ) {
      code_bug( "Proc_arg: NULL room in loop_followers." );
      return 0;
    }

    room_data *save_room = var_room;
    Content_Array content = save_room->contents;
    for( int i = 0; i < content; ++i ) {
      char_data *rch = character( content[i] );
      if ( !rch
	   || !rch->Is_Valid( )
	   || rch->array != &save_room->contents
	   || invis_level( rch ) >= LEVEL_BUILDER
           || ( aloop->fruit == loop_followers
		&& rch->leader != leader ) )
         continue;

      var_rch = rch;
      proc_arg( aloop->aloop, program, owner ); // note that this can reset the var_ variables
      if( end_prog )
	return 0;
      if( program->abort( owner ) ) {
	end_prog = true;
	return 0;
      }
    }

    return 0;
  }

  if( arg->family == if_clause ) {
    aif_type *aif  = (aif_type*) arg; 
    const void *flag  = proc_arg( aif->condition, program, owner );

    if( !flag ) {
      proc_arg( aif->no, program, owner );
    } else {
      proc_arg( aif->yes, program, owner );
    }

    return 0;
  }

  const void *result;
  const void *farg [ MAX_CFUNC_ARG ];
  vzero( farg, MAX_CFUNC_ARG );

  if( arg->family == assign ) {
    assign_type *ass = (assign_type*) arg;
    farg[0] = ass->arg[0];
    farg[1] = proc_arg( ass->arg[1], program, owner );
    result = ( ass->func->func_call )( farg );

  } else if( arg->family == op ) {
    op_type *op = (op_type*) arg;
    farg[0] = proc_arg( op->arg[0], program, owner );
    if( ( op->func->func_call == &code_and && (int)farg[0] == 0 )
	|| ( op->func->func_call == &code_or && (int)farg[0] != 0 ) ) {
      // Short-circuit && and ||.
      result =(void*)( farg[0] != 0 );
    } else {
      farg[1] = proc_arg( op->arg[1], program, owner );
      result =( op->func->func_call )( farg );
    }

  } else if( arg->family == conv ) {
    conv_type *conv = (conv_type*) arg;
    farg[0] = proc_arg( conv->arg, program, owner );
    result = ( conv->func->func_call )( farg );

  } else {
    afunc_type *afunc = (afunc_type*) arg;
    
    for( int i = 0; i < MAX_CFUNC_ARG; ++i ) {
      if( !afunc->arg[i] )
	break;
      farg[i] = proc_arg( afunc->arg[i], program, owner );
    }
    
    curr_arg = arg;  // Save arg for wait().
    result = ( afunc->func->func_call )( farg );
  }

  end_prog = end_prog || program->abort( owner );

  if( arg->logic_neg ) {
    return (void*) !result;
  }

  if( arg->arith_neg ) {
    return (void*) -(int)result;
  }

  if( arg->bit_neg ) {
    return (void*) ~(int)result;
  }

  return result;
}


const void *proc_arg( arg_type* arg, program_data *program, thing_data *owner )
{
  const void *result = 0;

  while( arg ) {
    result = one_arg( arg, program, owner );
    if( end_prog ) {
      return 0;
    }
    arg = arg->next;
  }

  return result;
}


/*
 *   STACK ROUTINES
 */


void push( )
{
  stack_data *stack = new stack_data;
  
  stack->room      = var_room;
  stack->ch        = var_ch;
  stack->victim    = var_victim;
  stack->obj       = var_obj;
  stack->mob       = var_mob;
  stack->rch       = var_rch;
  stack->i         = var_i;
  stack->j         = var_j;
  stack->k         = var_k;
  stack->l         = var_l;
  stack->m         = var_m;
  stack->n         = var_n;
  stack->act_obj   = var_act_obj;
  stack->container = var_container;
  stack->vcmd      = var_cmd;
  stack->varg      = var_arg;
  stack->exit      = var_exit;
  stack->def       = var_def;
  stack->def_type  = var_def_type;
  stack->thing     = var_thing;
  stack->list      = var_list;

  stack->o_victim  = orig_victim;
  stack->o_ch      = orig_ch;

  stack->next = stack_list;
  stack_list  = stack;
}


void pop( bool discard )
{
  stack_data *stack = stack_list;
  stack_list = stack->next;

  if( !discard ) {
    var_room   	= stack->room;
    var_ch     	= stack->ch;
    var_victim 	= stack->victim;
    var_obj    	= stack->obj;
    var_mob    	= stack->mob;
    var_rch    	= stack->rch;
    var_i      	= stack->i;
    var_j      	= stack->j;
    var_k      	= stack->k;
    var_l      	= stack->l;
    var_m      	= stack->m;
    var_n      	= stack->n;
    var_act_obj   = stack->act_obj;
    var_container = stack->container;
    var_cmd       = stack->vcmd;
    var_arg       = stack->varg;
    var_exit      = stack->exit;
    var_def       = stack->def;
    var_def_type  = stack->def_type;
    var_thing     = stack->thing;
    var_list.swap( stack->list );
    
    orig_victim = stack->o_victim;
    orig_ch = stack->o_ch;
  }

  delete stack;
}


/*
 *   QUEUE ROUTINES
 */


void do_ps( char_data* ch, const char *)
{
  if( !queue_list ) {
    send( ch, "The queue is empty.\n\r" );
    return;
  }

  page_underlined( ch, "Vnum            Type           Location\n\r" );

  for( queue_data *queue = queue_list; queue; queue = queue->next )
    queue->program->display( ch );
}


void update_queue( queue_data *& list )
{
  time_data start;
  gettimeofday( &start, 0 );

  queue_data *queue_next;

  for( queue_data *queue = list; queue; queue = queue_next ) {
    queue_next = queue->next;

    if( queue->time-- > 1 )
      continue;

    error_buf[0] = '\0';
    end_prog     = false;
    cont_prog    = false;
    return_prog    = false;
    queue_prog   = false;
    now_prog   = false;

    if( !queue->program->abort( queue->owner ) ) {
      var_room 		= queue->room;
      var_ch   	  	= queue->ch;
      var_rch   	= queue->rch;
      var_victim	= queue->victim;
      var_mob  		= queue->mob;
      var_obj  		= queue->obj;
      var_i    		= queue->i;
      var_j    		= queue->j;
      var_k    		= queue->k;
      var_l    		= queue->l;
      var_m    		= queue->m;
      var_n    		= queue->n;
      var_act_obj 	= queue->act_obj;
      var_container	= queue->container;
      var_cmd    	= queue->vcmd;
      var_arg    	= queue->varg;
      var_exit          = queue->exit;
      var_def           = queue->def;
      var_def_type      = queue->def_type;
      var_thing         = queue->thing;
      var_list.swap( queue->list );
      
      orig_victim       = queue->o_victim;
      orig_ch           = queue->o_ch;

      curr_prog = queue->program;
      proc_arg( queue->arg->next, queue->program, queue->owner );
      curr_prog = 0;

      if( var_cmd && var_cmd != empty_string )
	extract_strings += var_cmd;

      if( var_arg && var_arg != empty_string )
	extract_strings += var_arg;
    }

    if( queue_prog ) {
      //      queue_list->program = queue->program;
      queue_list->owner = queue->owner;
      queue_list->arg = curr_arg;
    } else if( now_prog ) {
      //      now_list->program = queue->program;
      now_list->owner = queue->owner;
      now_list->arg = curr_arg;
    } else {
      --queue->program->active;
    }
    
    remove( list, queue );
    delete queue;
  }

  //  clear_variables( );

  pulse_time[ TIME_QUEUE ] += stop_clock( start );
}


/*
 *   CLEAR QUEUE OF A VARIABLE
 */


void clear_queue( char_data* ch )
{
  for( queue_data *queue = queue_list; queue; queue = queue->next ) {
    if(     queue->mob == ch ) queue->mob = 0;
    if(      queue->ch == ch ) queue->ch = 0;
    if(  queue->victim == ch ) queue->victim = 0;
    if(     queue->rch == ch ) queue->rch = 0;
    if(   queue->owner == ch ) queue->owner = 0;
    if(   queue->thing == ch ) queue->thing = 0;
    queue->list.remove_all( ch );
    if( queue->o_victim == ch ) queue->o_victim = 0;
    if( queue->o_ch == ch ) queue->o_ch = 0;
  }
  
  for( queue_data *queue = now_list; queue; queue = queue->next ) {
    if(     queue->mob == ch ) queue->mob = 0;
    if(      queue->ch == ch ) queue->ch = 0;
    if(  queue->victim == ch ) queue->victim = 0;
    if(     queue->rch == ch ) queue->rch = 0;
    if(   queue->owner == ch ) queue->owner = 0;
    if(   queue->thing == ch ) queue->thing = 0;
    queue->list.remove_all( ch );
    if( queue->o_victim == ch ) queue->o_victim = 0;
    if( queue->o_ch == ch ) queue->o_ch = 0;
  }
  
  if(    var_mob == ch )      var_mob = 0;
  if(     var_ch == ch )       var_ch = 0;
  if( var_victim == ch )   var_victim = 0;
  if(    var_rch == ch )      var_rch = 0;
  if(  var_thing == ch )    var_thing = 0;
  var_list.remove_all( ch );
  if( orig_victim == ch ) orig_victim = 0;
  if( orig_ch == ch ) orig_ch = 0;
  
  for( stack_data *stack = stack_list; stack; stack = stack->next ) {
    if(     stack->ch == ch ) stack->ch = 0;
    if( stack->victim == ch ) stack->victim = 0;
    if(    stack->rch == ch ) stack->rch = 0;
    if(    stack->mob == ch ) stack->mob = 0;
    if(  stack->thing == ch ) stack->thing = 0;
    stack->list.remove_all( ch );
    if( stack->o_victim == ch ) stack->o_victim = 0;
    if( stack->o_ch == ch ) stack->o_ch = 0;
  }
}


void clear_queue( obj_data *obj_old, obj_data *obj_new )
{
  for( queue_data *queue = queue_list; queue; queue = queue->next ) {
    if(       queue->obj == obj_old ) queue->obj = obj_new;
    if(   queue->act_obj == obj_old ) queue->act_obj = obj_new;
    if( queue->container == obj_old ) queue->container = obj_new;
    if(     queue->owner == obj_old ) queue->owner = obj_new;
    if(     queue->thing == obj_old ) queue->thing = obj_new;
    if( obj_new ) {
      queue->list.replace_all( obj_old, obj_new );
    } else {
      queue->list.remove_all( obj_old );
    }
  }

  for( queue_data *queue = now_list; queue; queue = queue->next ) {
    if(       queue->obj == obj_old ) queue->obj = obj_new;
    if(   queue->act_obj == obj_old ) queue->act_obj = obj_new;
    if( queue->container == obj_old ) queue->container = obj_new;
    if(     queue->owner == obj_old ) queue->owner = obj_new;
    if(     queue->thing == obj_old ) queue->thing = obj_new;
    if( obj_new ) {
      queue->list.replace_all( obj_old, obj_new );
    } else {
      queue->list.remove_all( obj_old );
    }
  }

  if(       var_obj == obj_old )         var_obj = obj_new;
  if(   var_act_obj == obj_old )     var_act_obj = obj_new;
  if( var_container == obj_old )   var_container = obj_new;
  if(     var_thing == obj_old )       var_thing = obj_new;
  if( obj_new ) {
    var_list.replace_all( obj_old, obj_new );
  } else {
    var_list.remove_all( obj_old );
  }

  for( stack_data *stack = stack_list; stack; stack = stack->next ) {
    if(       stack->obj == obj_old ) stack->obj = obj_new;
    if(   stack->act_obj == obj_old ) stack->act_obj = obj_new;
    if( stack->container == obj_old ) stack->container = obj_new;
    if(     stack->thing == obj_old ) stack->thing = obj_new;
    if( obj_new ) {
      stack->list.replace_all( obj_old, obj_new );
    } else {
      stack->list.remove_all( obj_old );
    }
  }

  if( obj_old && obj_new ) {
    if( Content_Array *where = obj_old->array ) {
      if( char_data *ch = character( where->where ) ) {
	if( ch->cast ) {
	  for( int j = 0; j < MAX_SPELL_WAIT; ++j ) {
	    if( ch->cast->reagent[j] == obj_old ) {
	      ch->cast->reagent[j] = obj_new;
	    }
	    if( object( ch->cast->target ) == obj_old ) {
	      ch->cast->target = obj_new;
	    }
	  }
	}
      } else if( Room( where->where ) ) {
	for( int j = 0; j < *where; j++ ) {
	  if( ( ch = character( where->list[j] ) )
	      && ch->cast
	      && object( ch->cast->target ) == obj_old ) {
	    ch->cast->target = obj_new;
	  }
	}
      }
    }
  }
}


void clear_queue( program_data* program )
{
  queue_data *next;

  for( queue_data *queue = now_list; queue; queue = next ) {
    next = queue->next; 
    if( queue->program == program ) {
      remove( now_list, queue );
      free_string( queue->vcmd, MEM_QUEUE );
      free_string( queue->varg, MEM_QUEUE );
      delete queue;
    }
  }
  
  for( queue_data *queue = queue_list; queue; queue = next ) {
    next = queue->next; 
    if( queue->program == program ) {
      remove( queue_list, queue );
      free_string( queue->vcmd, MEM_QUEUE );
      free_string( queue->varg, MEM_QUEUE );
      delete queue;
    }
  }
  
  program->active = 0;
}


/*
 *   MISC SUPPORT ROUTINES
 */


void clear_variables( )
{
  var_victim    = 0;
  var_rch       = 0;
  var_mob       = 0;
  var_ch        = 0;
  var_obj       = 0;
  var_act_obj   = 0;
  var_container = 0;
  var_room      = 0;
  var_i         = 0;
  var_j         = 0;
  var_k         = 0;
  var_l         = 0;
  var_m         = 0;
  var_n         = 0;
  var_cmd       = 0;
  var_arg       = 0;
  var_exit      = 0;
  var_def       = 0;
  var_def_type  = -1;
  var_thing     = 0;
  var_list.clear( );
  orig_victim = 0;
  orig_ch = 0;
}


void do_fwhere( char_data* ch, const char *argument )
{
  if( !*argument ) {
    send( ch, "Syntax: fwhere <function name>\n\r" );
    return;
  }

  const int k = find_cfunc( argument );

  if( k < 0 ) {
    fsend( ch, "No such function: %s.", argument );
    return;
  }

  int i = k;

  /*
  int i;
  for( i = 0; ; ++i ) {
    if( !*cfunc_list[i].name ) {
      fsend( ch, "No such function: %s", argument );
      return;
    }
    if( !strcasecmp( argument, cfunc_list[i].name ) ) {
      break;
    }
  }
  */

  while( !strcasecmp( cfunc_list[i].name, cfunc_list[k].name ) ) {

    page( ch, "\n\r" );
    char tmp [ TWO_LINES ];
    snprintf( tmp, TWO_LINES, "--- Function %s ---", cfunc_list[i].name );
    page_centered( ch, tmp ); 
    page( ch, "\n\r" );
    
    if( !search_code( ch, search_func, (void*)cfunc_list[i].func_call ) )
      page( ch, "No references to function \"%s\" were found.\n\r", cfunc_list[i].name );

    ++i;
  }
}
