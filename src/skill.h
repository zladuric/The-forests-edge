#ifndef tfe_skill_h
#define tfe_skill_h

/*
 *   PRACTICES
 */


void init_skills  ( char_data* );
int  total_pracs  ( char_data* );


inline int expected_pracs( char_data* ch )
{
  return 10+2*ch->Level( )*(10+ch->Wisdom( )
			    +ch->Intelligence( ))/3;
}


/*
 *  TRAINER HEADER
 */


class Trainer_Data
{
 public:
  trainer_data*   next;
  room_data*      room;
  //  char_data*       mob;
  int          trainer;
  int          *skills [ MAX_SKILL_CAT ];

  Trainer_Data( );
  ~Trainer_Data( );
};


/* List of trainers and what they train */
extern trainer_data *trainer_list;


void   load_trainers    ();
void   save_trainers    ();
void   set_trainer      ( mob_data*, room_data* );


int find_skill ( const char*, int = -1 );
int skill_index ( const char*, int = -1 );
void display_skills ( char_data*, int = -1 );


/* cp cost to practice a skill at a trainer */
int prac_cost ( char_data *ch, int skill );


//extern Skill_Type *skill_tables[ MAX_SKILL_CAT ];
extern const char *skill_cat_name [ MAX_SKILL_CAT ];
extern const int skill_table_number[ MAX_SKILL_CAT ];


inline unsigned skill_number( unsigned skill )
{
  return skill & 0xffff;
}


inline unsigned skill_table( unsigned skill )
{
  return ( skill >> 16 ) & 0xffff;
}


inline unsigned skill_ident( unsigned table, unsigned number )
{
  return ( table << 16 ) + number;
}


inline Skill_Type *skill_entry( unsigned table, unsigned number )
{
  switch( table ) {
  case SKILL_CAT_PHYSICAL:
    return &skill_physical_table[ number ];
  case SKILL_CAT_LANGUAGE:
    return &skill_language_table[ number ];
  case SKILL_CAT_SPELL:
    return &skill_spell_table[ number ];
  case SKILL_CAT_TRADE:
    return &skill_trade_table[ number ];
  case SKILL_CAT_WEAPON:
    return &skill_weapon_table[ number ];
  }

  return 0;
}


inline Skill_Type *skill_entry( unsigned skill )
{
  switch( skill_table( skill ) ) {
  case SKILL_CAT_PHYSICAL:
    return &skill_physical_table[ skill_number( skill ) ];
  case SKILL_CAT_LANGUAGE:
    return &skill_language_table[ skill_number( skill ) ];
  case SKILL_CAT_SPELL:
    return &skill_spell_table[ skill_number( skill ) ];
  case SKILL_CAT_TRADE:
    return &skill_trade_table[ skill_number( skill ) ];
  case SKILL_CAT_WEAPON:
    return &skill_weapon_table[ skill_number( skill ) ];
  }

  return 0;
}


#endif // tfe_skill_h
