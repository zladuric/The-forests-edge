#ifndef tfe_update_h
#define tfe_update_h


#define PULSE_PER_SECOND          16

#define PULSE_VIOLENCE            2*PULSE_PER_SECOND
#define PULSE_MOBILE              6*PULSE_PER_SECOND
#define PULSE_ROOM               15*PULSE_PER_SECOND
#define PULSE_TICK               50*PULSE_PER_SECOND
#define PULSE_AREA               80*PULSE_PER_SECOND
#define PULSE_CLEANUP         60*60*PULSE_PER_SECOND
#define PULSE_MAINT        24*60*60*PULSE_PER_SECOND


void    mobile_update    ( void );


/*
 *   REGENERATION
 */


void    update_maxes       ( char_data* );
void    update_max_hit     ( char_data* );
void    update_max_move    ( char_data* );
void    update_max_mana    ( char_data* );

void    regenerate         ( char_data* );
void    rejuvenate         ( char_data* );
void    regen_update       ( void );


/*
 *   CONDITION
 */


bool    condition_update   ( char_data* );
void    gain_condition     ( char_data*, int, int, bool = false );


#endif // tfe_update_h
