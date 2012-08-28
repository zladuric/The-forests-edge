#ifndef tfe_list_h
#define tfe_list_h


#define LIST_LEVELS         0
#define LIST_PERMISSIONS    1
#define LIST_FOOD_OMNI      2
#define LIST_FOOD_CARNI     3
#define LIST_FOOD_HERBI     4
#define LIST_FEAST_OMNI     5
#define LIST_FEAST_CARNI    6
#define LIST_FEAST_HERBI    7
#define LIST_LS_SPECIES     8
#define LIST_LS_REAGENT     9
#define LIST_FF_SPECIES    10
#define LIST_FF_REAGENT    11
#define LIST_RA_SPECIES    12
#define LIST_RA_REAGENT    13
#define LIST_FM_SPECIES    14
#define LIST_FM_REAGENT    15
#define LIST_CE_SPECIES    16
#define LIST_CE_REAGENT    17
#define LIST_CG_SPECIES    18
#define LIST_CG_REAGENT    19
#define MAX_LIST           20


extern int list_value [ MAX_LIST ][ 30 ];
extern const char *list_entry [ MAX_LIST ][ 30 ];


void   save_lists   ();
void   load_lists   ();


#endif // tfe_list_h
