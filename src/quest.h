#ifndef tfe_quest_h
#define tfe_quest_h


#define MAX_QUEST 128

#define QUEST_NONE                  0
#define QUEST_ASSIGNED              1
#define QUEST_DONE                 -1
#define QUEST_FAILED               -2

#define QFLAG_HIDDEN	0
#define MAX_QFLAG	1


class quest_data
{
public:
  quest_data( int i )
    : vnum( i ), serial( -1 ),
      message( empty_string ),
      points( 0 ), flags( 0 ),
      comments( empty_string )
  {
    record_new( sizeof( quest_data ), MEM_QUEST );
  }
  
  ~quest_data ( )
  {
    record_delete( sizeof( quest_data ), MEM_QUEST );
    free_string( message, MEM_QUEST );
    free_string( comments, MEM_QUEST );
  }

  int vnum;
  int serial;
  const char *message;
  unsigned char points;
  int flags;
  const char *comments;
};    


extern quest_data *quest_list [ MAX_QUEST ];


quest_data *get_quest_index ( int );
void load_quests ( );
void save_quests ( );
void update_quest ( char_data* );


#endif // tfe_quest_h
