#ifndef tfe_interp_h
#define tfe_interp_h


extern room_data *cmd_room;


class Command_Data
{
public:
  const char *string;
  const bool ordered;
  Command_Data *next;
  room_data *room;	// Used by speed command.

  Command_Data( const char *msg, bool bit )
    : ordered(bit), next(0), room(0)
  {
    string = alloc_string( msg, MEM_PLAYER );
  }
  
  ~Command_Data( ) {
    free_string( string, MEM_PLAYER );
  }
};


class Command_Queue
{
private:
  int size;
  command_data *list;
  command_data *last;

public:
  Command_Queue( )
    : size(0), list(0), last(0)
  {}

  ~Command_Queue( )
  {
    clear();
  }

  int entries( ) const
  {
    return size;
  }

  command_data *pop( )
  {
    if( !list )
      return 0;
    command_data *cmd = list;
    list = list->next;
    if( !list )
      last = 0;
    --size;
    //    cmd->next = 0;
    return cmd;
  }
  
  command_data *push( const char *string, bool ordered )
  {
    command_data *cmd = new command_data( string, ordered );
    if( !last ) {
      list = cmd;
    } else {
      last->next = cmd;
    }
    last = cmd;
    ++size;
    return cmd;
  }

  command_data *shift( const char *string, bool ordered )
  {
    command_data *cmd = new command_data( string, ordered );
    if( !last ) {
      last = cmd;
    } else {
      cmd->next = list;
    }
    list = cmd;
    ++size;
    return cmd;
  }

  void clear( )
  {
    while( Command_Data *cmd = list ) {
      list = list->next;
      delete cmd;
    }
    size = 0;
    list = last = 0;
  }
};
  

#endif // tfe_interp_h
