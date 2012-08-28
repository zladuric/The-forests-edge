#ifndef tfe_channel_h
#define tfe_channel_h


class Tell_Data
{
 public:
  tell_data*    next;
  char*         name;
  char*         message;
  int           language;

  Tell_Data     ( const char*, const char*, int );
  ~Tell_Data    ( );
};


/*
 *   COMMUNICATION ROUTINES
 */


bool  can_talk    ( char_data*, const char* = 0 );


/*
 *   LANGUAGE ROUTINES
 */


//extern int        max_language;
//extern flag_data  lang_flags;

//int skill_language( const char_data* ch, int language );
int get_language  ( const char_data*, int );


#endif // tfe_channel_h
