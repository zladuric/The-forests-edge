#include <ctype.h>
#include <sys/types.h>
#include <stdio.h>
#include <syslog.h>
#include "define.h"
#include "struct.h"


/*
 *   RECURSIVE SEARCH FUNCTIONS
 */


bool search_code( char_data *ch, search_type& func, void *target )
{
  action_data*     action;
  int                j;
  bool              found  = false;

  for( area_data *area = area_list; area; area = area->next ) {
    const bool loaded = area->act_loaded;
    for( room_data *room = area->room_first; room; room = room->next ) {
      for( j = 1, action = room->action; action; j++, action = action->next ) {
	const bool compiled = action->binary;
	if( !compiled ) {
	  action->compile( );
	}
	if( func( ch, action->binary, target, search_acode, j, room->name, room->vnum, empty_string ) ) {
	  found = true;
	}
	if( !compiled )
	  action->decompile( );
      }
    }
    if( !loaded && area->act_loaded ) {
      area->clear_actions( );
    }
  }
  
  species_data *species;
  mprog_data *mprog;

  for( int i = 1; i <= species_max; ++i ) {
    if( !( species = species_list[i] ) ) 
      continue;
    const bool compiled = species->attack->binary;
    if( !compiled )
      species->attack->compile( );
    if( func( ch, species->attack->binary, target, search_attack, 0, species->Name( ), i, empty_string ) ) {
      found = true;
    }
    if( !compiled )
      species->attack->decompile( );
    for( j = 1, mprog = species->mprog; mprog; j++, mprog = mprog->next ) {
      const bool compiled = mprog->binary;
      if( !compiled )
	mprog->compile( );
      if( func( ch, mprog->binary, target, search_mprog, j, species->Name( ), i, empty_string ) ) {
	found = true;
      }
      if( !compiled )
	mprog->decompile( );
    }
  }
  
  obj_clss_data *clss;
  oprog_data *oprog;

  for( int i = 1; i <= obj_clss_max; ++i ) {
    if( !( clss = obj_index_list[i] ) ) 
      continue; 
    for( j = 1, oprog = clss->oprog; oprog; j++, oprog = oprog->next ) {
      const bool compiled = oprog->binary;
      if( !compiled )
	oprog->compile( );
      if( func( ch, oprog->binary, target, search_oprog, j, clss->Name(), clss->vnum, empty_string ) ) {
	found = true;
      }
      if( !compiled )
	oprog->decompile( );
    }
  }

  for( int i = 0; i < MAX_TABLE; ++i ) {
    for( int j = 0; j < table_max[ i ]; ++j ) {
      Table_Data *t = table_addr( i, j );
      Tprog_Data *const *tprog = t->program( );
      if( tprog && *tprog ) {
	const bool compiled = (*tprog)->binary;
	if( !compiled )
	  (*tprog)->compile( );
	if( func( ch, (*tprog)->binary, target, search_tprog,
		  0, table_name( i ), 0, t->name ) ) {
	  found = true;
	}
	if( !compiled )
	  (*tprog)->decompile( );
      }
    }
  }

  return found;
}


static void display_search( char_data *ch, const char *func,
			    int type, int num,
			    const char *name, int vnum,
			    const char *text )
{
  switch( type ) {
  case search_acode:
    page( ch, "  %s() in acode #%d at %s [%d]\n\r",
	  func, num, name, vnum );
    break;
  case search_attack:
    page( ch, "  %s() in attack mprog on %s [%d]\n\r",
	  func, name, vnum );
    break;
  case search_mprog:
    page( ch, "  %s() in mprog #%d on %s [%d]\n\r",
	  func, num, name, vnum );
    break;
  case search_oprog:
    page( ch, "  %s() in oprog #%d on %s [%d]\n\r",
	  func, num, name, vnum );
    break;
  case search_tprog:
    page( ch, "  %s() in tprog on table %s, entry %s\n\r",
	  func, name, text );
    break;
  }
}


bool search_func( char_data *ch, const arg_type *arg, void *target,
		  int type, int num,
		  const char *name, int vnum,
		  const char *text )
{
  if( !arg )
    return false;
   
  bool found = false;

  if( arg->family == if_clause ) {
    const aif_type *aif = (aif_type*) arg;
    found = search_func( ch, aif->condition, target, type, num, name, vnum, text ) || found;
    found = search_func( ch, aif->yes, target, type, num, name, vnum, text ) || found;
    found = search_func( ch, aif->no, target, type, num, name, vnum , text) || found;
  } else if( arg->family == loop ) {
    const loop_type *looper = (loop_type*) arg;
    found = search_func( ch, looper->condition, target, type, num, name, vnum, text ) || found;
    found = search_func( ch, looper->aloop, target, type, num, name, vnum, text ) || found;
  } else if( arg->family == function ) {
    const afunc_type *afunc = (afunc_type*) arg;
    if( afunc->func->func_call == (cfunc*) target ) {
      display_search( ch, afunc->func->name, type, num, name, vnum, text );
      found = true;
    }
    for( int i = 0; i < 6 && afunc->arg[i]; i++ )
      found = search_func( ch, afunc->arg[i], target, type, num, name, vnum, text ) || found;
  } else if( arg->family == assign ) {
    const assign_type *ass = (assign_type*) arg;
    //    found = search_func( ch, ass->arg[0], target, type, num, name, vnum, text ) || found;
    found = search_func( ch, ass->arg[1], target, type, num, name, vnum, text ) || found;
  } else if( arg->family == op ) {
    const op_type *oper = (op_type*) arg;
    found = search_func( ch, oper->arg[0], target, type, num, name, vnum, text ) || found;
    found = search_func( ch, oper->arg[1], target, type, num, name, vnum, text ) || found;
  }

  return search_func( ch, arg->next, target, type, num, name, vnum, text ) || found;
}


bool search_room( char_data *ch, const arg_type *arg, void *target,
		  int type, int num,
		  const char *name, int vnum,
		  const char *text )
{
  if( !arg )
    return false;
   
  bool found = false;

  if( arg->family == if_clause ) {
    const aif_type *aif = (aif_type*) arg;
    found = search_room( ch, aif->condition, target, type, num, name, vnum, text ) || found;
    found = search_room( ch, aif->yes, target, type, num, name, vnum, text ) || found;
    found = search_room( ch, aif->no, target, type, num, name, vnum, text ) || found;
  } else if( arg->family == loop ) {
    const loop_type *looper = (loop_type*) arg;
    found = search_room( ch, looper->condition, target, type, num, name, vnum, text ) || found;
    found = search_room( ch, looper->aloop, target, type, num, name, vnum, text ) || found;
  } else if( arg->family == function ) {
    const afunc_type *afunc = (afunc_type*) arg;
    if( afunc->func->func_call == &code_find_room
	&& afunc->arg[0]
	&& afunc->arg[0]->family == constant ) {
      if( (int)( afunc->arg[0]->value ) == (int) target ) {
	display_search( ch, afunc->func->name, type, num, name, vnum, text );
	found = true;
      }
    }
    for( int i = 0; i < 6 && afunc->arg[i]; i++ )
      found = search_room( ch, afunc->arg[i], target, type, num, name, vnum, text ) || found;
  } else if( arg->family == assign ) {
    const assign_type *ass = (assign_type*) arg;
    //    found = search_room( ch, ass->arg[0], target, type, num, name, vnum, text ) || found;
    found = search_room( ch, ass->arg[1], target, type, num, name, vnum, text ) || found;
  } else if( arg->family == op ) {
    const op_type *oper = (op_type*) arg;
    found = search_room( ch, oper->arg[0], target, type, num, name, vnum, text ) || found;
    found = search_room( ch, oper->arg[1], target, type, num, name, vnum, text ) || found;
  }

  return search_room( ch, arg->next, target, type, num, name, vnum, text ) || found;
}


bool search_oload( char_data *ch, const arg_type *arg, void *target,
		   int type, int num,
		   const char *name, int vnum,
		  const char *text )
{
  if( !arg )
    return false;
   
  bool found = false;

  if( arg->family == if_clause ) {
    const aif_type *aif = (aif_type*) arg;
    found = search_oload( ch, aif->condition, target, type, num, name, vnum, text ) || found;
    found = search_oload( ch, aif->yes, target, type, num, name, vnum , text) || found;
    found = search_oload( ch, aif->no, target, type, num, name, vnum, text ) || found;
  } else if( arg->family == loop ) {
    const loop_type *looper = (loop_type*) arg;
    found = search_oload( ch, looper->condition, target, type, num, name, vnum, text ) || found;
    found = search_oload( ch, looper->aloop, target, type, num, name, vnum, text ) || found;
  } else if( arg->family == function ) {
    const afunc_type *afunc = (afunc_type*) arg;
    if( ( afunc->func->func_call == &code_oload
	  || afunc->func->func_call == &code_has_obj
	  || afunc->func->func_call == &code_wearing_obj
	  || afunc->func->func_call == &code_obj_in_room
	  || afunc->func->func_call == &code_oprog )
	&& afunc->arg[0]
	&& afunc->arg[0]->family == constant ) {
      if( (int)( afunc->arg[0]->value ) == (int) target ) {
	display_search( ch, afunc->func->name, type, num, name, vnum, text );
	found = true;
      }
    } else if( afunc->func->func_call == &code_replace_obj
	&& afunc->arg[1]
	&& afunc->arg[1]->family == constant ) {
      if( (int)( afunc->arg[1]->value ) == (int) target )
	display_search( ch, afunc->func->name, type, num, name, vnum, text );
	found = true;
    }
    for( int i = 0; i < 6 && afunc->arg[i]; i++ )
      found = search_oload( ch, afunc->arg[i], target, type, num, name, vnum, text ) || found;
  } else if( arg->family == assign ) {
    const assign_type *ass = (assign_type*) arg;
    //    found = search_oload( ch, ass->arg[0], target, type, num, name, vnum, text ) || found;
    found = search_oload( ch, ass->arg[1], target, type, num, name, vnum, text ) || found;
  } else if( arg->family == op ) {
    const op_type *oper = (op_type*) arg;
    found = search_oload( ch, oper->arg[0], target, type, num, name, vnum, text ) || found;
    found = search_oload( ch, oper->arg[1], target, type, num, name, vnum, text ) || found;
  }

  return search_oload( ch, arg->next, target, type, num, name, vnum, text ) || found;
}


bool search_mload( char_data *ch, const arg_type *arg, void *target,
		   int type, int num,
		   const char *name, int vnum,
		  const char *text )
{
  if( !arg )
    return false;
   
  bool found = false;

  if( arg->family == if_clause ) {
    const aif_type *aif = (aif_type*) arg;
    found = search_mload( ch, aif->condition, target, type, num, name, vnum, text ) || found;
    found = search_mload( ch, aif->yes, target, type, num, name, vnum, text ) || found;
    found = search_mload( ch, aif->no, target, type, num, name, vnum, text ) || found;
  } else if( arg->family == loop ) {
    const loop_type *looper = (loop_type*) arg;
    found = search_mload( ch, looper->condition, target, type, num, name, vnum, text ) || found;
    found = search_mload( ch, looper->aloop, target, type, num, name, vnum, text ) || found;
  } else if( arg->family == function ) {
    const afunc_type *afunc = (afunc_type*) arg;
    if( ( afunc->func->func_call == &code_mload
	  || afunc->func->func_call == &code_num_mob
	  || afunc->func->func_call == &code_mob_in_room
	  || afunc->func->func_call == &code_mprog )
	&& afunc->arg[0]
	&& afunc->arg[0]->family == constant ) {
      if( (int)( afunc->arg[0]->value ) == (int) target )
	display_search( ch, afunc->func->name, type, num, name, vnum, text );
	found = true;
    }
    for( int i = 0; i < 6 && afunc->arg[i]; i++ )
      found = search_mload( ch, afunc->arg[i], target, type, num, name, vnum, text ) || found;
  } else if( arg->family == assign ) {
    const assign_type *ass = (assign_type*) arg;
    //    found = search_mload( ch, ass->arg[0], target, type, num, name, vnum, text ) || found;
    found = search_mload( ch, ass->arg[1], target, type, num, name, vnum, text ) || found;
  } else if( arg->family == op ) {
    const op_type *oper = (op_type*) arg;
    found = search_mload( ch, oper->arg[0], target, type, num, name, vnum, text ) || found;
    found = search_mload( ch, oper->arg[1], target, type, num, name, vnum, text ) || found;
  }     
  
  return search_mload( ch, arg->next, target, type, num, name, vnum, text ) || found;
}


bool search_quest( char_data *ch, const arg_type *arg, void *target,
		   int type, int num,
		   const char *name, int vnum,
		  const char *text )
{
  if( !arg )
    return false;

  bool found = false;

  if( arg->family == if_clause ) {
    const aif_type *aif = (aif_type*) arg;
    found = search_quest( ch, aif->condition, target, type, num, name, vnum, text ) || found;
    found = search_quest( ch, aif->yes, target, type, num, name, vnum, text ) || found;
    found = search_quest( ch, aif->no, target, type, num, name, vnum, text ) || found;
  } else if( arg->family == loop ) {
    const loop_type *looper = (loop_type*) arg;
    found = search_quest( ch, looper->condition, target, type, num, name, vnum, text ) || found;
    found = search_quest( ch, looper->aloop, target, type, num, name, vnum, text ) || found;
  } else if( arg->family == function ) {
    const afunc_type *afunc = (afunc_type*) arg;
    if( ( afunc->func->func_call == &code_assign_quest
	  || afunc->func->func_call == &code_update_quest
	  || afunc->func->func_call == &code_has_quest
	  || afunc->func->func_call == &code_doing_quest
	  || afunc->func->func_call == &code_done_quest
	  || afunc->func->func_call == &code_get_quest
	  || afunc->func->func_call == &code_set_quest )
	&& afunc->arg[1]
	&& afunc->arg[1]->family == constant ) {
      if( (int)( afunc->arg[1]->value ) == (int) target )
	display_search( ch, afunc->func->name, type, num, name, vnum, text );
	found = true;
    }
    for( int i = 0; i < 6 && afunc->arg[i]; i++ )
      found = search_quest( ch, afunc->arg[i], target, type, num, name, vnum, text ) || found;
  } else if( arg->family == assign ) {
    const assign_type *ass = (assign_type*) arg;
    //    found = search_quest( ch, ass->arg[0], target, type, num, name, vnum, text ) || found;
    found = search_quest( ch, ass->arg[1], target, type, num, name, vnum, text ) || found;
  } else if( arg->family == op ) {
    const op_type *oper = (op_type*) arg;
    found = search_quest( ch, oper->arg[0], target, type, num, name, vnum , text) || found;
    found = search_quest( ch, oper->arg[1], target, type, num, name, vnum, text ) || found;
  }     

  return search_quest( ch, arg->next, target, type, num, name, vnum, text ) || found;
}


bool search_cflag( char_data *ch, const arg_type *arg, void *target,
		   int type, int num,
		   const char *name, int vnum,
		   const char *text )
{
  if( !arg )
    return false;

  bool found = false;

  if( arg->family == if_clause ) {
    const aif_type *aif = (aif_type*) arg;
    found = search_cflag( ch, aif->condition, target, type, num, name, vnum, text ) || found;
    found = search_cflag( ch, aif->yes, target, type, num, name, vnum, text ) || found;
    found = search_cflag( ch, aif->no, target, type, num, name, vnum, text ) || found;
  } else if( arg->family == loop ) {
    const loop_type *looper = (loop_type*) arg;
    found = search_cflag( ch, looper->condition, target, type, num, name, vnum, text ) || found;
    found = search_cflag( ch, looper->aloop, target, type, num, name, vnum, text ) || found;
  } else if( arg->family == function ) {
    const afunc_type *afunc = (afunc_type*) arg;
    if( ( afunc->func->func_call == &code_cflag
	  || afunc->func->func_call == &code_remove_cflag
	  || afunc->func->func_call == &code_set_cflag )
	&& afunc->arg[0]
	&& afunc->arg[0]->family == constant ) {
      if( (int)( afunc->arg[0]->value ) == (int) target )
	display_search( ch, afunc->func->name, type, num, name, vnum, text );
	found = true;
    }
    for( int i = 0; i < 6 && afunc->arg[i]; i++ )
      found = search_cflag( ch, afunc->arg[i], target, type, num, name, vnum, text ) || found;
  } else if( arg->family == assign ) {
    const assign_type *ass = (assign_type*) arg;
    //    found = search_cflag( ch, ass->arg[0], target, type, num, name, vnum, text ) || found;
    found = search_cflag( ch, ass->arg[1], target, type, num, name, vnum, text ) || found;
  } else if( arg->family == op ) {
    const op_type *oper = (op_type*) arg;
    found = search_cflag( ch, oper->arg[0], target, type, num, name, vnum, text ) || found;
    found = search_cflag( ch, oper->arg[1], target, type, num, name, vnum, text ) || found;
  }     

  return search_cflag( ch, arg->next, target, type, num, name, vnum, text ) || found;
}
