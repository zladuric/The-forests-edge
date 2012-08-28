#ifndef tfe_group_h
#define tfe_group_h

/*
 *   PET ROUTINES
 */


char_data *has_mount         ( char_data*, bool = true );
bool   has_elemental     ( char_data* );
int    undead_pets       ( char_data* );
int    number_of_pets    ( char_data* );
int    pet_levels        ( char_data* );


inline bool is_pet( char_data* ch )
{
  return( !ch->pcdata && is_set( ch->status, STAT_PET ) );
}  


/*
 *   FOLLOWING ROUTINES
 */


//char_array*  followers       ( char_data*, Content_Array* = 0 );
//void         add_followers   ( char_data*, char_array&, Content_Array* = 0 );
void         add_follower    ( char_data*, char_data*, const char* = empty_string ); 
void         stop_follower   ( char_data* );


/*
 *   GROUP ROUTINES
 */


int min_group_move        ( char_data* );
int min_group_hit         ( char_data* );

void add_group( char_data*, char_data* );
char_data *group_leader( char_data* );


#endif // tfe_group_h
