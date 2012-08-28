#ifndef tfe_fight_h
#define tfe_fight_h

/*
 *   ATTACK ROUTINES
 */


#define ATT_GARROTE     -3
#define ATT_BACKSTAB    -2
#define ATT_CHARGE      -1
#define ATT_PHYSICAL     0
#define ATT_FIRE         1
#define ATT_COLD         2
#define ATT_ACID         3
#define ATT_SHOCK        4
#define ATT_MAGIC        5
#define ATT_MIND         6
#define ATT_SOUND        7
//#define MAX_ATTACK       8

/*
#define ATT_LEAP         0
#define ATT_DEFAULT      1
#define ATT_KICK         2
#define ATT_PUNCH        3
#define ATT_BITE         4
#define ATT_SPIN_KICK    5
#define ATT_BASH         6
#define ATT_BACKSTAB     7
#define ATT_FLEE         8
#define ATT_GARROTE      9
#define ATT_CHARGE      10
#define ATT_GOUGE       11
#define ATT_MELEE       12
*/

bool         attack         ( char_data*, char_data*, const char*, const char *,
			      obj_data*, int, int, int = ATT_PHYSICAL );
char_data*   get_victim     ( char_data*, const char *, const char * );

void         add_round      ( species_data*, int );
int          player_round   ( char_data*, char_data* );
int          mob_round      ( char_data*, char_data* );

bool         set_fighting   ( char_data*, char_data* );
void         init_attack    ( char_data*, char_data* = 0 );
void         react_attack   ( char_data*, char_data* );
void         renter_combat  ( char_data* ); 
void         stop_fight     ( char_data*, bool = false );
void         set_attack     ( char_data*, char_data* );
bool         jump_feet      ( char_data* );
void         leap_message   ( char_data*, char_data* );
void         fight_round    ( char_data* );

bool         can_kill       ( char_data*, char_data* = 0, bool = true );
bool         can_pkill      ( char_data*, char_data* = 0, bool = true );
bool         in_sanctuary   ( char_data*, bool = true );
char_data   *opponent       ( char_data* );
char_data   *opponent2      ( char_data* );
char_data   *opponent3      ( char_data* );

bool         can_defend     ( char_data*, char_data* );

bool         damage_weapon  ( char_data*, obj_data*& );

void         update_aggression ( char_data* );
void         update_aggression ( room_data* );

bool is_fighting            ( char_data*, const char* = empty_string );
bool not_fighting           ( char_data*, const char* = empty_string );

void get_wield( char_data *ch, obj_data *& wield, obj_data *& secondary, obj_data *& shield );

bool join_fight( char_data* victim, char_data* ch, char_data* rch );
bool trigger_attack( char_data* ch, char_data* victim );

const char *attack_noun ( int );
const char *attack_verb ( int );
bool uses_claws ( char_data* );


/*
 *   DAMAGE ROUTINES
 */


const char*  condition_short    ( char_data* );
const char*  condition_short    ( char_data*, char_data* );
const char*  word_physical      ( int );


#define cd char_data

bool  damage_fire        ( cd*, cd*, int, const char*, bool = false,
			   const char* = "fire" );
bool  damage_cold        ( cd*, cd*, int, const char*, bool = false,
			   const char* = "cold" );
bool  damage_mind        ( cd*, cd*, int, const char*, bool = false,
			   const char* = "a mind attack" );
bool  damage_physical    ( cd*, cd*, int, const char*, bool = false,
			   const char* = "physical damage" );
bool  damage_shock       ( cd*, cd*, int, const char*, bool = false,
			   const char* = "electricity" );
bool  damage_magic       ( cd*, cd*, int, const char*, bool = false,
			   const char* = "magic" );
bool  damage_acid        ( cd*, cd*, int, const char*, bool = false,
			   const char* = "acid" );
bool  damage_sound       ( cd*, cd*, int, const char*, bool = false,
			   const char* = "sound" );

bool  inflict            ( cd*, cd*, int, const char*, bool = false );

void  dam_message        ( cd*, cd*, int, const char*, const char* );
void  dam_local          ( cd*, cd*, int, const char*, bool, const char* );

#undef cd

extern const index_data physical_index [];
extern const index_data shock_index [];
extern const index_data fire_index [];
extern const index_data cold_index [];
extern const index_data acid_index [];
extern const index_data sound_index [];


/*
 *   DEATH
 */


void        death_message      ( char_data* );


/*
 *   EXP ROUTINES
 */


void   add_exp        ( char_data*, int, const char* = 0 );
int    exp_for_level  ( int );
int    death_exp      ( char_data*, char_data* );
void   disburse_exp   ( char_data* );


inline int exp_for_level( char_data* ch )
{
  return exp_for_level( ch->Level( ) );
}


/*
 *   ENEMY ROUTINES
 */


bool        is_enemy           ( char_data*, char_data* );
int         damage_done        ( char_data*, char_data* );
void        clear_enemies      ( char_data* );
void        record_damage      ( char_data*, char_data*, int = 0 );
void        share_enemies      ( char_data*, char_data* );


/*
 *   FLEE ROUTINES
 */


void check_wimpy     ( char_data* );
bool attempt_flee    ( char_data*, exit_data* = 0 );


/*
 *   WEAPONS
 */


const char* weapon_class( const obj_clss_data* );
const char* weapon_class( const obj_data* );


/*
 *   SPAM ROUTINES
 */


void         spam_char          ( char_data*, const char* );


template < class T >
void spam_char( char_data* ch, const char* text, T item )
{
  if( !ch->link
      || !is_set( ch->pcdata->message, MSG_MISSES ) )
    return;

  fsend( ch, text, item );
}


template < class S, class T >
void spam_char( char_data* ch, const char* text, S item1, T item2 )
{
  if( !ch->link
      || !is_set( ch->pcdata->message, MSG_MISSES ) )
    return;

  fsend( ch, text, item1, item2 );
}


template < class S, class T, class U >
void spam_char( char_data* ch, const char* text, S item1, T item2, U item3 )
{
  if( !ch->link
      || !is_set( ch->pcdata->message, MSG_MISSES ) )
    return;

  fsend( ch, text, item1, item2, item3 );
}


template < class S, class T, class U, class V >
void spam_char( char_data* ch, const char* text, S item1, T item2, U item3, V item4 )
{
  if( !ch->link
      || !is_set( ch->pcdata->message, MSG_MISSES ) )
    return;

  fsend( ch, text, item1, item2, item3, item4 );
}


void spam_room( char_data*, const char* );


template < class T >
void spam_room( char_data *ch, const char *text, T item )
{
  clear_send_buffers( );
  for( int i = 0; i < *ch->array; ++i ) {
    char_data *rch = character( ch->array->list[i] );
    if( !rch
	|| !rch->link
	|| rch == ch
	|| !rch->Can_See()
	|| !is_set( rch->pcdata->message, MSG_MISSES )
	|| rch == (char_data*) item )
      continue;
    fsend( rch, text,
	   buffer( 0, item ) );
  }
}


template < class S, class T >
void spam_room( char_data *ch, const char *text, S item1, T item2 )
{
  clear_send_buffers( );
  for( int i = 0; i < *ch->array; ++i ) {
    char_data *rch = character( ch->array->list[i] );
    if( !rch
	|| !rch->link
	|| rch == ch
	|| !rch->Can_See()
	|| !is_set( rch->pcdata->message, MSG_MISSES )
	|| rch == (char_data*) item1
	|| rch == (char_data*) item2 )
      continue;
    fsend( rch, text,
	   buffer( 0, item1 ),
	   buffer( 1, item2 ) );
  }
}


template < class S, class T, class U >
void spam_room( char_data *ch, const char *text, S item1, T item2, U item3 )
{
  clear_send_buffers( );
  for( int i = 0; i < *ch->array; ++i ) {
    char_data *rch = character( ch->array->list[i] );
    if( !rch
	|| !rch->link
	|| rch == ch
	|| !rch->Can_See()
	|| !is_set( rch->pcdata->message, MSG_MISSES )
	|| rch == (char_data*) item1
	|| rch == (char_data*) item2
	|| rch == (char_data*) item3 )
      continue;
    fsend( rch, text,
	   buffer( 0, item1 ),
	   buffer( 1, item2 ),
	   buffer( 2, item3 ) );
  }
}


template < class S, class T, class U, class V >
void spam_room( char_data *ch, const char *text, S item1, T item2, U item3, V item4 )
{
  clear_send_buffers( );
  for( int i = 0; i < *ch->array; ++i ) {
    char_data *rch = character( ch->array->list[i] );
    if( !rch
	|| !rch->link
	|| rch == ch
	|| !rch->Can_See()
	|| !is_set( rch->pcdata->message, MSG_MISSES )
	|| rch == (char_data*) item1
	|| rch == (char_data*) item2
	|| rch == (char_data*) item3
	|| rch == (char_data*) item4 )
      continue;
    fsend( rch, text,
	   buffer( 0, item1 ),
	   buffer( 1, item2 ),
	   buffer( 2, item3 ),
	   buffer( 3, item4 ) );
  }
}


template < class S, class T, class U, class V, class W >
void spam_room( char_data *ch, const char *text,
		S item1, T item2, U item3, V item4, W item5 )
{
  clear_send_buffers( );
  for( int i = 0; i < *ch->array; ++i ) {
    char_data *rch = character( ch->array->list[i] );
    if( !rch
	|| !rch->link
	|| rch == ch
	|| !rch->Can_See()
	|| !is_set( rch->pcdata->message, MSG_MISSES )
	|| rch == (char_data*) item1
	|| rch == (char_data*) item2
	|| rch == (char_data*) item3
	|| rch == (char_data*) item4
	|| rch == (char_data*) item5 )
      continue;
    fsend( rch, text,
	   buffer( 0, item1 ),
	   buffer( 1, item2 ),
	   buffer( 2, item3 ),
	   buffer( 3, item4 ),
	   buffer( 4, item5 ) );
  }
}

     //void         spam_room          ( const char*, char_data*, char_data*, thing_data* = 0 );


#endif // tfe_fight_h
