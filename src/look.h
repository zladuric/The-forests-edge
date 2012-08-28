#ifndef tfe_look_h
#define tfe_look_h

/*
 *   DESCRIPTION STRUCTURE
 */


class Extra_Data : public visible_data
{
public:
  char        *keyword;        
  const char           *text;
  
  Extra_Data( )
    : keyword(0), text(0)
  {
    record_new( sizeof( extra_data ), MEM_EXTRA );
  }

  virtual ~Extra_Data( ) {
    record_delete( sizeof( extra_data ), MEM_EXTRA );
    free_string( keyword, MEM_EXTRA );
    free_string( text, MEM_EXTRA );
  }
  
  Extra_Data( const Extra_Data& other ) {
    record_new( sizeof( extra_data ), MEM_EXTRA );
    keyword = alloc_string( other.keyword, MEM_EXTRA );
    text = alloc_string( other.text, MEM_EXTRA );
  }

  virtual int Type ( ) const
  { return EXTRA_DATA; }
  
  virtual void         Look_At    ( char_data* );
  virtual const char*  Keywords   ( char_data* );
};


bool  obj_descr     ( char_data*, thing_array&, char* );
bool  room_descr    ( char_data*, extra_data*, char* );
void  show_extras   ( char_data*, const extra_array& );
bool  edit_extra    ( extra_array&, wizard_data*, int, const char *, char* );
void  read_extras   ( FILE*, extra_array& );
void  write_extras  ( FILE*, const extra_array& );
void  obj_act_spam  ( int, char_data*, obj_data*, char_data*,
		      const char *, const char *, bool = false );
void  obj_loc_spam  ( char_data*, obj_data*, char_data*,
		      const char *&, const char *&,
		      const char_data *to = 0 );

/*
 *   LOOK ROUTINES
 */


void   look_on       ( char_data*, obj_data* );
void   look_in       ( char_data*, obj_data* );
void   show_room     ( char_data*, room_data*, bool, bool );


/*
 *   SCAN ROUTINES
 */


bool scan_aggro ( char_data*, exit_data* );


/*
 *   SEARCH ROUTINES
 */


bool scan_aggro ( char_data*, exit_data* );
bool search ( char_data* );

/*
 *   WHO ROUTINES
 */


inline bool can_see_who( char_data* ch, char_data* victim )
{
  return get_trust( ch ) >= invis_level( victim );

  /*
  wizard_data* imm;
  
  if( ( imm = wizard( victim ) )
      && is_set( victim->pcdata->pfile->flags, PLR_WIZINVIS )
      && get_trust( ch ) < imm->wizinvis )
    return false;
  
  return true;
  */
}


inline const char* who_name( char_data* ch, char_data* victim )
{
  return can_see_who( ch, victim ) ? victim->Seen_Name( ch ) : "someone";
}


#endif // tfe_look_h
