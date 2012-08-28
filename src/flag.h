#ifndef tfe_flag_h
#define tfe_flag_h


class flag_data
{
public:
  const char*     title;
  const char**    name1;
  const char**    name2;
  const int *const  max;
  bool sort;

  void   sprint       ( char*, int* );
  void   display      ( char_data*, int* );
  const char *set     ( char_data*, const char *, const char *,
			int*, int* = 0,
			bool = false, bool = true );
};


extern flag_data affect_flags;
extern flag_data permission_flags;
extern flag_data alignment_flags;
extern flag_data material_flags;
extern flag_data terrain_flags;


/*
 *   FUNCTION DECLARATIONS
 */


enum flag_op {
  flag_remove,
  flag_set,
  flag_toggle
};


// This is a horrible hack.
extern int last_bit;
extern int last_flag;


const char*  flag_handler    ( const char**, const char***, const char***,
                               int**, int **, int*, const int*, const bool*,
			       const char*,
			       char_data*, const char *, const char *,
			       int, flag_op = flag_toggle );
void         display_flags   ( const char*, const char* const*, const char* const*,
                               int*, int, char_data*, bool = true );
const char*  set_flags       ( const char *const*, const char *const*,
			       int*, int*,
			       int, const char*,
			       char_data*, const char *, const char *,
                               bool, bool, bool = true, flag_op = flag_toggle );
bool         set_flags       ( char_data*, const char *&, int*, const char* );
bool         toggle          ( char_data*, const char *, const char *, int*, int );
bool         get_flags       ( char_data*, const char *&, int*, const char*,
                               const char* );
void         alter_flags     ( int*, int*, int*, int );
void         set_bool        ( char_data*, const char *, const char*, bool& );

int          sort_names      ( const char *const *, const char *const *,
			       int *, int, bool = true );

void         sort_ints       ( const int*, const int*,
			       int *, int, bool = false );

/*
 *   LEVEL SETTING ROUTINES
 */


void   display_levels  ( const char*, const char**, const char**,
                          int*, int, char_data*, bool = true );
bool   set_levels      ( const char**, const char**, int*,
                         int, char_data*, const char *, bool, bool = true );

#endif // tfe_flag_h
