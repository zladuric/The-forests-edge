#ifndef tfe_pos
#define tfe_pos


#define POS_EXTRACTED              -1    
#define POS_DEAD                    0
#define POS_MORTAL                  1
#define POS_INCAP                   2
#define POS_STUNNED                 3
#define POS_SLEEPING                4
#define POS_MEDITATING              5
#define POS_RESTING                 6
#define POS_FIGHTING                7
#define POS_STANDING                8
#define MAX_POSITION                9

#define POS_FLYING                  9
#define POS_FALLING                10
#define POS_HOVERING               11
#define POS_WADING                 12
#define POS_SWIMMING               13
#define POS_DROWNING               14
#define MAX_MOD_POSITION           15


bool rest               ( char_data*, bool );
bool stand              ( char_data*, bool );
bool sit                ( char_data*, obj_data*, bool );
bool mount              ( char_data*, char_data* );
bool is_mounted         ( char_data*, const char* = 0 );
bool is_ridden          ( char_data*, const char* = 0 );
bool dismount           ( char_data*, int = POS_STANDING );
void sleep              ( char_data* );
bool mob_pos            ( mob_data* );
void pos_message        ( const char_data* );


#endif // tfe_pos
