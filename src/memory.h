#ifndef tfe_memory
#define tfe_memory

/*
 *   MEMORY HEADER
 */


#define MEM_UNKNOWN                 0
#define MEM_ACCOUNT                 1
#define MEM_ACTION                  2
#define MEM_AFFECT                  3
#define MEM_ALIAS                   4
#define MEM_AREA                    5
#define MEM_ARRAY                   6
#define MEM_AUCTION                 7
#define MEM_BADNAME                 8
#define MEM_BAN                     9
#define MEM_CLAN                   10
#define MEM_CODE                   11
#define MEM_CUSTOM                 12
#define MEM_DESCR                  13
#define MEM_DICTIONARY             14
#define MEM_ENEMY                  15
#define MEM_EVNT                   16
#define MEM_EXIT                   17
#define MEM_EXTRA                  18
#define MEM_HELP                   19
#define MEM_INFO                   20
#define MEM_LINK                   21
#define MEM_MEMORY                 22
#define MEM_MOBS                   23 
#define MEM_MPROG                  24
#define MEM_NOTE                   25
#define MEM_OBJ_CLSS               26
#define MEM_OBJECT                 27
#define MEM_OPROG                  28
#define MEM_PATH                   29
#define MEM_PFILE                  30
#define MEM_PLAYER                 31
#define MEM_PROGRAM                32
#define MEM_QUEST                  33
#define MEM_QUEUE                  34            
#define MEM_RECOGNIZE              35
#define MEM_REQUEST                36 
#define MEM_RESET                  37
#define MEM_ROOM                   38
#define MEM_SHDATA                 39
#define MEM_SHOP                   40
#define MEM_SPECIES                41
#define MEM_SPELL                  42
#define MEM_STACK                  43
#define MEM_TABLE                  44
#define MEM_TELL                   45
#define MEM_TRACK                  46
#define MEM_TRAINER                47
#define MEM_VOTE                   48
#define MEM_WIZARD                 49
#define MAX_MEMORY                 50


char *alloc_string        ( const char*, int );
void   free_string        ( const char*, int );
void   record_new         ( int, int );
void   record_delete      ( int, int );
void   extract            ( wizard_data*, int, const char* );
player_data *edit_lock    ( player_data*, void*, int, const char*, bool = false );


inline int offset( void* pntr1, void* pntr2 ) {
  return( (int)( pntr1 ) - (int)( pntr2 ) );
}


/*
 *   MEMORY STRUCTURES
 */


class mem_block
{
 public:
  class mem_block*    next;
  char*               pntr;
  int                 size;
  
  mem_block( int i ) {
    record_new( sizeof( mem_block ), MEM_MEMORY );
    size = i;
    pntr = new char[size];
    next = 0;
  }
  
  ~mem_block( ) {
    record_delete( sizeof( mem_block ), MEM_MEMORY );
    delete [] pntr;
  }
};


#endif // tfe_memory
