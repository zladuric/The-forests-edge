#ifndef tfe_set_h
#define tfe_set_h

/*
 *   FIELD CLASSES
 */


class int_field {
 public:
  const char*    name;
  int             min;
  int             max;
  int*          value;     

  const char*    set          ( char_data*, const char *, const char * );    
};


class byte_field {
 public:
  const char*    name;
  unsigned char             min;
  unsigned char             max;
  unsigned char*          value;     

  const char*    set          ( char_data*, const char *, const char * );    
};


class cent_field {
 public:
  const char*    name;
  int             min;
  int             max;
  int*          value;     

  const char*    set          ( char_data*, const char *, const char * );    
};


class dice_field {
 public:
  const char*     name;
  int            level;
  int*           value;

  const char*      set       ( char_data*, const char *, const char * );
};


class string_field {
 public:
  const char*        name;
  int            mem_type;
  const char**      value;
  set_func*          func;

  const char*         set    ( char_data*, const char *, const char * );
};


class type_field {
 public:
  const char*    name;
  int            max;
  const char**   first;
  const char**   second;
  int*           value;
  bool           sort;

  const char*              set    ( char_data*, const char *, const char * );
  inline const char*   element    ( int i );
};


inline const char* type_field :: element( int i )
{
  if( i < 0 || i >= max )
    i = 0;

  return *(first+i*(second-first));
}


/*
 *   TEMPLATES
 */


template < class T >
const char* process( T *field, char_data* ch, const char* subject,
		     const char *argument )
{
  for( int i = 0; *field[i].name; ++i ) 
    if( matches( argument, field[i].name ) )
      return field[i].set( ch, subject, argument );

  return 0;
}


template < class T >
const char *process( T* field, char_data* ch, const char* subject, const char *argument,
		     species_data* species, player_data* player )
{
  if( const char *response = process( field, ch, subject, argument ) ) { 
    if( *response ) {
      if( species ) {
	species->set_modified( ch );
	mob_log( ch, species->vnum, response );
      } else if( player ) {
	modify_pfile( player );
	player_log( player, response );
      }
    }
    return response;
  }

  return 0;
}


template < class T >
const char *process( T* field, char_data* ch, const char *argument, obj_clss_data *clss )
{
  if( const char *response = process( field, ch, clss->Name( ), argument ) ) { 
    if( *response ) {
      clss->set_modified( ch );
      obj_log( ch, clss->vnum, response );
    }
    return response;
  }
  
  return 0;
}


template < class T >
const char *process( T* field, char_data* ch, const char *argument, obj_data* obj )
{
  return process( field, ch, obj->Name( ), argument );
}


/*
 *   FUNCTIONS
 */


void set_string   ( char_data*, const char *, const char*&, const char*, int );
/*
void set_type     ( char_data*, const char *, int&, const char*,
                    const char*, int,
                    const char**, const char** = 0 );
*/

#endif // tfe_set_h
