#ifndef tfe_account_h
#define tfe_account_h

/*
 *   ACCOUNT CLASS
 */


class account_data
{
public:
  account_data*   next;
  const char           *name;
  const char          *email;
  const char            *pwd;
  const char        *confirm;
  const char      *new_email; 
  const char          *notes; 
  const char    *timezone;
  int       last_login;
  int          balance;
  int           banned;
  int       no_players;

  account_data( );
  ~account_data( );

  bool Seen( char_data* ) const;

  friend const char *name( account_data* account ) {
    return account->name;
  }
};


extern int              max_account;
extern account_data**  account_list;


/*
 *   BAN ROUTINES
 */


void  load_badname       ( );
void  save_badname       ( );
void  load_banned        ( );
void  save_banned        ( );
void  load_remort        ( );
void  save_remort        ( );


extern const char**  badname_array;
extern int           max_badname;
       
extern const char**  remort_array;
extern int           max_remort;
       

/*
 *   GLOBAL ROUTINES
 */


void            load_accounts       ();
void            save_accounts       ();
void            display_account     ( char_data*, account_data*, bool& );
void            extract             ( account_data* );
account_data*   find_account        ( const char *, bool = false );
account_data*   account_arg         ( const char *& );


#endif // tfe_account_h
