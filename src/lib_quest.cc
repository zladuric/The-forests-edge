#include <ctype.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include "define.h"
#include "struct.h"


const void *code_get_quest( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];
  const int i = (int) argument[1];

  if( i < 0 || i >= MAX_QUEST ) {
    code_bug( "Get_Quest: impossible quest number." );
    return 0;
  }

  if( !ch ) {
    code_bug( "Get_Quest: Null character." );
    return 0;
  }

  quest_data *quest = get_quest_index( i );

  if( !quest ) {
    code_bug( "Get_Quest: nonexistent quest." );
    return 0;
  }

  if( ch->species )
    return 0;
  
  return (void*) ch->pcdata->quest_flags[i];
}


const void *code_set_quest( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];
  const int i = (int) argument[1];
  const int value = (int) argument[2];

  if( i < 0 || i >= MAX_QUEST ) {
    code_bug( "Set_Quest: impossible quest number." );
    return 0;
  }

  if( !ch ) {
    code_bug( "Set_Quest: Null character." );
    return 0;
  }

  quest_data *quest = get_quest_index( i );

  if( !quest ) {
    code_bug( "Set_Quest: nonexistent quest." );
    return 0;
  }

  if( ch->species )
    return 0;

  ch->pcdata->quest_flags[i] = value;

  return 0;
}


const void *code_doing_quest( const void **argument )
{
  char_data*     ch  = (char_data*)(thing_data*) argument[0];
  const int         i  = (int)        argument[1];

  if( i < 0 || i >= MAX_QUEST ) {
    code_bug( "Doing_Quest: impossible quest number." );
    return 0;
  }

  if( !ch ) {
    code_bug( "Doing_Quest: Null character." );
    return 0;
  } 

  quest_data *quest = get_quest_index( i );

  if( !quest ) {
    code_bug( "Doing_Quest: nonexistent quest." );
    return 0;
  }

  if( ch->species )
    return 0;

  return (void*) ( ch->pcdata->quest_flags[i] != QUEST_NONE
		   && ch->pcdata->quest_flags[i] != QUEST_DONE
		   && ch->pcdata->quest_flags[i] != QUEST_FAILED );
}


const void *code_done_quest( const void **argument )
{
  char_data*     ch  = (char_data*)(thing_data*) argument[0];
  const int         i  = (int)        argument[1];

  if( i < 0 || i >= MAX_QUEST ) {
    code_bug( "Done_Quest: impossible quest number." );
    return 0;
  }

  if( !ch ) {
    code_bug( "Done_Quest: NULL character." );
    return 0;
  }

  quest_data *quest = get_quest_index( i );

  if( !quest ) {
    code_bug( "Done_Quest: nonexistent quest." );
    return 0;
  }

  if( ch->species )
    return 0;

  return (void*) ( ch->pcdata->quest_flags[i] == QUEST_DONE );
}


const void *code_failed_quest( const void **argument )
{
  char_data*     ch  = (char_data*)(thing_data*) argument[0];
  const int         i  = (int)        argument[1];

  if( i < 0 || i >= MAX_QUEST ) {
    code_bug( "Failed_Quest: impossible quest number." );
    return 0;
  }

  if( !ch ) {
    code_bug( "Failed_Quest: NULL character." );
    return 0;
  }

  quest_data *quest = get_quest_index( i );

  if( !quest ) {
    code_bug( "Failed_Quest: nonexistent quest." );
    return 0;
  }

  if( ch->species )
    return 0;

  return (void*) ( ch->pcdata->quest_flags[i] == QUEST_FAILED );
}


const void *code_has_quest( const void **argument )
{
  char_data*     ch  = (char_data*)(thing_data*) argument[0];
  const int         i  = (int)        argument[1];

  if( i < 0 || i >= MAX_QUEST ) {
    code_bug( "Has_Quest: impossible quest number." );
    return 0;
  }

  if( !ch ) {
    code_bug( "Has_Quest: Null character." );
    return 0;
  }

  quest_data *quest = get_quest_index( i );

  if( !quest ) {
    code_bug( "Has_Quest: nonexistent quest." );
    return 0;
  }

  if( ch->species )
    return 0;

  return (void*) ( ch->pcdata->quest_flags[i] != QUEST_NONE );
}


const void *code_assign_quest( const void **argument )
{
  char_data*    ch  = (char_data*)(thing_data*) argument[0];
  const int        i  = (int)        argument[1];

  if( i < 0 || i >= MAX_QUEST ) {
    code_bug( "Assign_Quest: impossible quest number." );
    return 0;
  }

  if( !ch ) {
    code_bug( "Assign_Quest: Null character." );
    return 0;
  }

  quest_data *quest = get_quest_index( i );

  if( !quest ) {
    code_bug( "Assign_Quest: nonexistent quest." );
    return 0;
  }

  if( ch->species )
    return 0;

  if( ch->pcdata->quest_flags[i] != QUEST_NONE ) {
    code_bug( "Assign_Quest: quest status nonzero." );
    return 0;
  }

  ch->pcdata->quest_flags[i] = QUEST_ASSIGNED;
  send( ch, "\n\r-*- You have been assigned a quest! -*-\n\r" ); 
  
  return 0;
}


const void *code_update_quest( const void **argument )
{
  char_data*      ch  = (char_data*)(thing_data*) argument[0];
  const int              i  = (int)        argument[1];

  if( i < 0 || i >= MAX_QUEST ) {
    code_bug( "Update_Quest: impossible quest number." );
    return 0;
  }

  if( !ch ) {
    code_bug( "Update_Quest: Null character." );
    return 0;
  }

  quest_data *quest = get_quest_index( i );

  if( !quest ) {
    code_bug( "Update_Quest: nonexistent quest." );
    return 0;
  }

  if( ch->species )
    return 0;

  if( ch->pcdata->quest_flags[i] == QUEST_NONE
      || ch->pcdata->quest_flags[i] == QUEST_DONE
      || ch->pcdata->quest_flags[i] == QUEST_FAILED ) {
    code_bug( "Update_Quest: quest is unassigned, done, or failed." );
    return 0;
  }

  ch->pcdata->quest_flags[i] = QUEST_DONE;
  ch->pcdata->quest_pts += quest->points;
  update_score( ch );
  if( quest->points != 0 ) 
    send( ch, "-*- You gain %d quest point%s! -*-\n\r",
	  quest->points,
	  ( quest->points != 1 )
	  ? "s"
	  : "" );

  return 0;
}


const void *code_fail_quest( const void **argument )
{
  char_data*      ch  = (char_data*)(thing_data*) argument[0];
  const int              i  = (int)        argument[1];

  if( i < 0 || i >= MAX_QUEST ) {
    code_bug( "Fail_Quest: impossible quest number." );
    return 0;
  }

  if( !ch ) {
    code_bug( "Fail_Quest: Null character." );
    return 0;
  }

  quest_data *quest = get_quest_index( i );

  if( !quest ) {
    code_bug( "Fail_Quest: nonexistent quest." );
    return 0;
  }

  if( ch->species )
    return 0;

  if( ch->pcdata->quest_flags[i] == QUEST_NONE
      || ch->pcdata->quest_flags[i] == QUEST_DONE
      || ch->pcdata->quest_flags[i] == QUEST_FAILED ) {
    code_bug( "Fail_Quest: quest is unassigned, done, or failed." );
    return 0;
  }

  ch->pcdata->quest_flags[i] = QUEST_FAILED;
  send( ch, "-*- You have failed a quest! -*-\n\r" );

  return 0;
}


const void *code_remove_quest( const void **argument )
{
  char_data*      ch  = (char_data*)(thing_data*) argument[0];
  const int              i  = (int)        argument[1];

  if( i < 0 || i >= MAX_QUEST ) {
    code_bug( "Remove_Quest: impossible quest number." );
    return 0;
  }

  if( !ch ) {
    code_bug( "Remove_Quest: Null character." );
    return 0;
  }

  quest_data *quest = get_quest_index( i );

  if( !quest ) {
    code_bug( "Remove_Quest: nonexistent quest." );
    return 0;
  }

  if( ch->species )
    return 0;

  if( ch->pcdata->quest_flags[i] == QUEST_NONE
      || ch->pcdata->quest_flags[i] == QUEST_DONE
      || ch->pcdata->quest_flags[i] == QUEST_FAILED ) {
    code_bug( "Remove_Quest: quest is unassigned, done, or failed." );
    return 0;
  }

  ch->pcdata->quest_flags[i] = QUEST_NONE;
  send( ch, "-*- You are unable to complete a quest! -*-\n\r" );

  return 0;
}
