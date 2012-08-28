#include <sys/types.h>
#include <stdio.h>
#include "define.h"
#include "struct.h"


const char *sex_name [ MAX_SEX ] = {
  "neuter", "male", "female", "hermaphrodite", "random" };

const char *hand_name [ MAX_HAND ] = {
  "none", "left", "right", "both", "random" };

const char *size_name [ MAX_SIZE ] = {
  "Ant", "Rat", "Dog", "Gnome", "Human", "Ogre",
  "Horse", "Giant", "Elephant", "Dinosaur" };


/*
 *   CHARACTER DATA
 */


char_data :: char_data( )
  : mount(0), rider(0), fighting(0),
    descr(0), species(0),
    leader(0), wearing(this),
    cast(0), link(0), pcdata(0), shdata(0), enemy(0), prepare(0), pos_obj(0),
    in_room(0), was_in_room(0),
    hit(0), mana(0), move(0),
    mod_hit(0), mod_move(0), mod_mana(0), max_hit(0), max_move(0), max_mana(0),
    mod_str(0), mod_int(0), mod_wis(0), mod_dex(0), mod_con(0),
    mod_armor(0), exp(0), hitroll(0), damroll(0), move_regen(0), mana_regen(0), hit_regen(0),
    room_position(-1), sex(SEX_RANDOM), hand(HAND_NONE), color(0),
    position(POS_STANDING),
    selected(true), shown(1)
{
  vzero( mod_resist, MAX_RESIST );
  vzero( affected_by, AFFECT_INTS );
  vzero( status, STATUS_INTS );

  active.func = &next_action;
  active.owner = this;

  update.func = &execute_update;
  update.owner = this;

  regen.func = &execute_regen;
  regen.owner = this;
  
  condition[ COND_ALCOHOL ] = 0;
  condition[ COND_FULL ]    = 24;
  condition[ COND_THIRST ]  = 24;
  condition[ COND_DRUNK ]   = 0;
}


char_data :: ~char_data( )
{
  delete_list( enemy );
  delete_list( prepare );
}


int char_data :: Selected( ) const
{ return selected ? 1 : 0; }


int char_data :: Shown( ) const
{ return shown; }


void char_data :: Select( int num )
{
  if( num < 0 ) {
    bug( "char_data::Select( num ): num < 0" );
    bug( "-- Char = %s", this );
    bug( "-- Num = %d", num );
    bug( "-- Location = %s", Location( ) );
    selected = false;
  } else if( num > 1 ) {
    bug( "char_data::Select( num ): num > 1" );
    bug( "-- Char = %s", this );
    bug( "-- Num = %d", num );
    bug( "-- Location = %s", Location( ) );
    selected = true;
    shown = 1;
  } else {
    selected = ( num != 0 );
    shown = num;
  }
}


void char_data :: Select_All( )
{
  selected = true;
  shown = 1;
}


void char_data :: Show( int num )
{
  if( num < 0 ) {
    bug( "char_data::Show( num ): num < 0" );
    bug( "-- Char = %s", this );
    bug( "-- Num = %d", num );
    bug( "-- Location = %s", Location( ) );
    shown = 0;
  } else {
    shown = num;
  }
}


bool char_data :: In_Game( ) const
{
  return( Is_Valid( ) &&
	  ( !link || link->connected == CON_PLAYING ) );
}


bool char_data :: is_affected( int bit ) const
{
  return is_set( affected_by, bit );
}


bool char_data :: can_carry( ) const
{
  return !species
    || is_set( species->act_flags, ACT_CAN_CARRY )
    || get_trust( this ) >= LEVEL_APPRENTICE;
}


bool char_data :: is_humanoid ( ) const
{
  return !species
    || is_set( species->act_flags, ACT_HUMANOID );
}


void char_data :: set_default_title( )
{
  if( !shdata || !pcdata )
    return;
  
  char tmp [ MAX_STRING_LENGTH ];
  free_string( pcdata->title, MEM_PLAYER );
  snprintf( tmp, MAX_STRING_LENGTH, " the %s",
	    race_table[ shdata->race ].name );
  /*
  snprintf( tmp, MAX_STRING_LENGTH, " the %s %s",
	    race_table[ shdata->race ].name,
	    clss_table[ pcdata->clss ].name );
  */
  pcdata->title = alloc_string( tmp, MEM_PLAYER );
}


bool char_data :: knows( const char_data *victim ) const
{
  if( !victim || !victim->species )
    return false;

  mob_data *npc = (mob_data*) victim;

  if( npc->pet_name != empty_string
      && ( npc->leader == this
	   || ( is_set( status, STAT_PET )
		&& leader == npc->leader ) ) )
    return true;

  if( !pcdata )
    return false;

  if( species ) {
    if( link && link->player ) {
      // Switched mob.
      return victim->known_by.includes( link->player );
    }
    return false;
  }

  return victim->known_by.includes( const_cast< char_data* >( this ) );
}


/*
 *   PLAYER_DATA
 */


player_data :: player_data( const char *name )
  : switched(0), familiar(0), reply(0),
    base_age(17), bank(0), prayer(150), gossip_pts(50),
    whistle(0), noteboard(0), note_edit(0),
    docker(0), remort(0), remort_name(0), movement(-1),
    logon(current_time), save_time(current_time), played(0), timer(0),
    locker( this ), junked( this ),
    chant(0), say(0), yell(0), shout(0), tell(0), gtell(0),
    chat(0), gossip(0), ctell(0), whisper(0), atalk(0), to(0)
{
  record_new( sizeof( player_data ), MEM_PLAYER );

  player_list += this;

  /*-- INITIALISE VARIABLES --*/

  valid = PLAYER_DATA;

  pcdata = new pc_data;
  shdata = new share_data;
  descr = new descr_data;

  descr->name = alloc_string( name, MEM_DESCR );

  //  vzero( shdata->skill, MAX_SKILL );

  set_update( this );
  set_regen( this );

  /*
  hunger.func = &execute_hunger;
  hunger.owner = this;
  add_queue( &hunger, 50 );

  thirst.func = &execute_thirst;
  thirst.owner = this;
  add_queue( &thirst, 50 );

  drunk.func = &execute_drunk;
  drunk.owner = this;
  add_queue( &drunk, 50 );
  */
}


player_data :: ~player_data( )
{
  record_delete( sizeof( player_data ), MEM_PLAYER );

  alias.delete_list();
  delete_list( tell );
  delete_list( gtell );
  delete_list( ctell );
  delete_list( whisper );
  delete_list( chat );
  delete_list( gossip );
  delete_list( say );
  delete_list( yell );
  delete_list( shout );
  delete_list( atalk );
  delete_list( to );
  delete_list( chant );

  free_string( remort_name, MEM_PLAYER );

  if( note_edit && note_edit->noteboard == -1 ) {
    delete note_edit;
  }

  player_list -= this;

  delete pcdata;
  delete shdata;
  delete descr;
}


/*
 *   WIZARD_DATA
 */


Wizard_Data :: Wizard_Data( const char *name )
  : player_data( name ),
    action_edit(0), player_edit(0), exit_edit(0), adata_edit(0),
    mpdata_edit(0), opdata_edit(0), oextra_edit(0), mextra_edit(0), textra_edit(0),
    room_edit(0), mprog_edit(0), obj_clss_edit(0),
    oprog_edit(0), quest_edit(0), mob_edit(0), help_edit(0), account_edit(0),
    custom_edit(0), rtable_edit(-1), list_edit(-1),
    office(0), wizinvis(0), recall(0),
    bamfin(empty_string), bamfout(empty_string),
    level_title(empty_string),
    build_chan(0), admin_chan(0), imm_talk(0), god_talk(0),
    docking(0)
{
  record_new( sizeof( wizard_data ), MEM_WIZARD );
  record_delete( sizeof( player_data ), MEM_PLAYER );

  valid = WIZARD_DATA;

  table_edit[0] = -1;

  //  vzero( permission, 2 );
}


Wizard_Data :: ~Wizard_Data( )
{
  record_delete( sizeof( wizard_data ), MEM_WIZARD );
  record_new( sizeof( player_data), MEM_PLAYER );

  delete_list( build_chan );
  delete_list( admin_chan );
  delete_list( imm_talk );
  delete_list( god_talk );

  free_string( bamfin,       MEM_WIZARD );
  free_string( bamfout,      MEM_WIZARD );
  free_string( level_title,  MEM_WIZARD );
}


Mob_Data::Mob_Data( species_data *mob_species )
  : prev(0), reset(0), pTrainer(0), pShop(0),
    pet_name(empty_string)
{
  if( !mob_species ) 
    panic( "Create mob_data: NULL species." );

  record_new( sizeof( Mob_Data ), MEM_MOBS );
  valid = MOB_DATA;
  mob_list += this;

  species = mob_species;
  shdata = species->shdata;
  descr = species->descr;
  color = species->color;
  
  assign_bit( status, STAT_SENTINEL,
	      is_set( species->act_flags, ACT_SENTINEL ) );
  assign_bit( status, STAT_AGGR_ALL,
	      is_set( species->act_flags, ACT_AGGR_ALL ) );
  assign_bit( status, STAT_AGGR_GOOD,
	      is_set( species->act_flags, ACT_AGGR_GOOD ) );
  assign_bit( status, STAT_AGGR_EVIL,
	      is_set( species->act_flags, ACT_AGGR_EVIL ) );
  assign_bit( status, STAT_AGGR_LAWFUL,
	      is_set( species->act_flags, ACT_AGGR_LAWFUL ) );
  assign_bit( status, STAT_AGGR_CHAOTIC,
	      is_set( species->act_flags, ACT_AGGR_CHAOTIC ) );

  assign_bit( status, STAT_SNEAKING,
	      species->is_affected( AFF_SNEAK ) );

  if( species->is_affected( AFF_CAMOUFLAGE ) ) {
    set_bit( status, STAT_CAMOUFLAGED );
  } else if( species->is_affected( AFF_HIDE ) ) {
    set_bit( status, STAT_HIDING );
  }

  init_affects( this );

  // *** SEE ALSO: update.cc:update( mob )

  base_hit = max( 1, dice_data( species->hitdice ).roll( ) );
  base_move = max( 0, dice_data( species->movedice ).roll( ) );
  base_mana = 100;

  if( ( sex = species->sex ) == SEX_RANDOM ) 
    sex = ( number_range( 0, 1 ) == 0 ? SEX_MALE : SEX_FEMALE ); 

  if( ( hand = species->hand ) == HAND_RANDOM ) {
    const int num = number_range( 1, 100 );
    if( num > 10 ) {
      hand = HAND_RIGHT;
    } else if( num > 1 ) {
      hand = HAND_LEFT;
    } else {
      hand = HAND_BOTH;
    }
  }

  update_max_hit( this );
  update_max_mana( this );
  
  hit = max_hit;
  mana = max_mana;

  update_max_move( this );

  move = max_move;

  if( is_affected( AFF_SLEEP ) )
    position = POS_SLEEPING;
  else
    position = POS_STANDING;

  int coins = species->gold;
  for( int type = MAX_COIN - 1; type >= 0; type-- ) {
    int i;
    if( ( i = number_range( 0, coins/coin_value[type] ) ) > 0 ) {
      obj_data *obj = create( get_obj_index( coin_vnum[type] ), i );
      obj->To( this );
      coins -= i*coin_value[type];
    }
  }
  
  //  number = 1;

  maturity = species->maturity;

  set_update( this );
  set_regen( this );

  for( mprog_data *mprog = species->mprog; mprog; mprog = mprog->next ) {
    if( mprog->trigger == MPROG_TRIGGER_TIMER
	&& mprog->value > 0 ) {
      add_queue( new event_data( execute_mob_timer, this ), mprog->value*PULSE_MOBILE );
    }
  }

  delay_wander( new event_data( execute_wander, this ) );
}


Mob_Data::~Mob_Data( )
{
  record_delete( sizeof( Mob_Data ), MEM_MOBS );
  
  /*
  if( pTrainer ) {
    remove( trainer_list, pTrainer );
    delete pTrainer;
  }
  */
  /*
  if( pTrainer ) {
    pTrainer->mob = 0;
  }
  */
  
  mob_list -= this;

  if( shdata != species->shdata ) {
    delete shdata;
  }

  if( descr != species->descr ) {
    delete descr;
  }

  free_string( pet_name, MEM_MOBS );
}


/*
 *   SHARE_DATA
 */


Share_Data :: Share_Data( )
  : strength(10), intelligence(10), wisdom(10), dexterity(10), constitution(10),
    level(0), alignment(ALGN_PURE_NEUTRAL), race(RACE_HUMAN),
    kills(0), deaths(0), fame(0)
{
  record_new( sizeof( share_data ), MEM_SHDATA );

  vzero( resist, MAX_RESIST );
  //  vzero( skill, MAX_SKILL );

  for( int i = 0; i < MAX_SKILL_CAT; ++i ) {
    const int n = table_max[ skill_table_number[ i ] ];
    record_new( n*sizeof( unsigned char ), -MEM_SHDATA );
    skills[i] = new unsigned char [ n ];
    vzero( skills[i], n );
  }
}


Share_Data :: ~Share_Data( )
{
  record_delete( sizeof( share_data ), MEM_SHDATA );

  for( int i = 0; i < MAX_SKILL_CAT; ++i ) {
    const int n = table_max[ skill_table_number[ i ] ];
    record_delete( n*sizeof( unsigned char ), -MEM_SHDATA );
    delete [] skills[i];
  }
}


/*
 *   AGE
 */

 
int player_data :: Age( )
{
  return base_age
    + ( played + current_time-logon )/144000
    + pcdata->mod_age;
}


/*
 *   RESISTANCES
 */


void calc_resist( player_data *pl )
{
  int j;

  for( int i = 0; i < MAX_RESIST; i++ ) {
    pl->shdata->resist[i] = 5 * pl->remort;
    
    if( ( j = clss_table[pl->pcdata->clss].resist[i] ) != 0 )
      pl->shdata->resist[i] += (pl->Level()+19)*j/10;
    
    if( pl->shdata->race < MAX_PLYR_RACE )
      pl->shdata->resist[i] += plyr_race_table[pl->shdata->race].resist[i];
  }
}


int char_data :: Save_Physical( char_data *ch ) const
{
  unsigned long long dam = 1;

  if( is_affected( AFF_INVULNERABILITY ) ) {
    const int n = affect_level( this, AFF_INVULNERABILITY );
    if( !species && pcdata->clss == CLSS_CLERIC ) {
      dam *= 95 - n/2;
    } else {
      dam *= 90 - n;
    }
  } else {
    dam *= 100;
  }

  if( is_affected( AFF_BARKSKIN ) ) {
    const int n = affect_level( this, AFF_BARKSKIN );
    dam *= 95 - n/2;
  } else {
    dam *= 100;
  }

  if( ch
      && is_affected( AFF_PROTECT_GOOD )
      && is_good( ch )
      && is_evil( this ) ) {
    const int n = affect_level( this, AFF_PROTECT_GOOD );
    dam *= 90 - n;
  } else {
    dam *= 100;
  }

  
  if( ch && is_affected( AFF_PROTECT_EVIL )
      && is_evil( ch )
      && is_good( this ) ) {
    const int n = affect_level( this, AFF_PROTECT_EVIL );
    dam *= 90 - n;
  } else {
    dam *= 100;
  }

  if( ch
      && is_affected( AFF_PROTECT_LAW )
      && is_lawful( ch )
      && is_chaotic( this ) ) {
    const int n = affect_level( this, AFF_PROTECT_LAW );
    dam *= 90 - n;
  } else {
    dam *= 100;
  }
  
  if( ch && is_affected( AFF_PROTECT_CHAOS )
      && is_chaotic( ch )
      && is_lawful( this ) ) {
    const int n = affect_level( this, AFF_PROTECT_CHAOS );
    dam *= 90 - n;
  } else {
    dam *= 100;
  }

  dam = ((long long)100*100*100*100*100*100) - dam;
  dam /= ((long long)100*100*100*100*100*100)/100;

  return (int) dam;
}


int char_data :: Save_Magic( ) const
{
  const int res = shdata->resist[RES_MAGIC]
    + mod_resist[RES_MAGIC]
    + 2*Intelligence( )
    + Wisdom( ) - 36;

  return min( 100, res );
}


int char_data :: Save_Mind( ) const
{
  const int res = shdata->resist[RES_MIND]
    + mod_resist[RES_MIND]
    + Intelligence( )
    + 2*Wisdom( ) - 36;

  return min( 100, res );
}


int char_data :: Save_Poison( ) const
{
  int res = shdata->resist[RES_POISON]
    + mod_resist[RES_POISON]
    + 2*Constitution( ) - 24;
  
  if( is_affected( AFF_RESIST_POISON ) ) {
    res = 34+2*res/3;
  }

  return min( 100, res );
}


int char_data :: Save_Fire( ) const
{
  int res = shdata->resist[RES_FIRE]
            + mod_resist[RES_FIRE];

  if( is_affected( AFF_RESIST_FIRE ) ) {
    res = 34+2*res/3;
  }

  if( is_affected( AFF_ICE_SHIELD ) ) {
    const int n = affect_level( this, AFF_ICE_SHIELD );
    res += n+2;
  }

  return min( 100, res );
}


int char_data :: Save_Cold( ) const
{
  int res = shdata->resist[RES_COLD]
    + mod_resist[RES_COLD];

  if( is_affected( AFF_RESIST_COLD ) )
    res = 34+2*res/3;

  if( is_affected( AFF_FIRE_SHIELD ) ) {
    const int n = affect_level( this, AFF_FIRE_SHIELD );
    res += n+2;
  }

  return min( 100, res );
}


int char_data :: Save_Shock( ) const
{
  int res = shdata->resist[RES_SHOCK]
    + mod_resist[RES_SHOCK];

  if( is_affected( AFF_RESIST_SHOCK ) ) {
    res = 34+2*res/3;
  }

  return min( 100, res );

}


int char_data :: Save_Acid( ) const
{
  int res = shdata->resist[RES_ACID]
    + mod_resist[RES_ACID];

  if( is_affected( AFF_RESIST_ACID ) ) {
    res = 34+2*res/3;
  }
  
  return min( 100, res );
}


int char_data :: Save_Sound( ) const
{
  int res = 0;

  if( is_affected( AFF_DEAFNESS ) ) {
    res = 50;
  }

  return min( 100, res );

}


/*
 *   ABILITY ROUTINES
 */


int char_data :: Strength( ) const
{
  int i = shdata->strength;

  if( is_affected( AFF_OGRE_STRENGTH ) ) {
    const int add = 5 - i/5;
    if( add > 0 ) {
      i += add;
    }
  }

  i -= affect_duration( this, AFF_DEATH )/2;

  return range( 3, i+mod_str, 30 );
}


int char_data :: Intelligence( ) const
{
  return range( 3, shdata->intelligence+mod_int, 30 );
}


int char_data :: Wisdom( ) const
{
  return range( 3, shdata->wisdom+mod_wis, 30 );
}


int char_data :: Dexterity( ) const
{
  return range( 3, shdata->dexterity+mod_dex-get_burden( ), 30 );
}


int char_data :: Constitution( ) const
{
  int i = shdata->constitution+mod_con;

  i -= affect_duration( this, AFF_DEATH )/2;

  return range( 3, i, 30 );
}


/*
 *  HIT/MANA/MOVE
 */


void rejuvenate( char_data* ch )
{
  update_max_hit( ch );
  update_max_mana( ch );

  ch->hit  = ch->max_hit;
  ch->mana = ch->max_mana;

  update_max_move( ch );

  ch->move = ch->max_move;
}


void update_maxes( char_data* ch )
{
  if( ch ) {
    update_max_hit( ch );
    update_max_move( ch );
    update_max_mana( ch );
  }
}


void update_max_hit( char_data* ch )
{
  if( ch->species ) {
    ch->max_hit = ch->base_hit*ch->Constitution()/ch->shdata->constitution
      +ch->mod_hit;
  } else {
    ch->max_hit = max( 1, ch->base_hit+ch->mod_hit
		       +ch->Level()*(ch->Constitution( )-12)/2 );
  }
  
  ch->hit = min( ch->hit, ch->max_hit );
}


void update_max_move( char_data* ch )
{
  int move = ch->base_move+ch->mod_move;
  move = ( ch->hit*move )/ch->max_hit;

  ch->max_move = max( 0, move );
  ch->move     = min( ch->move, ch->max_move );
}


void update_max_mana( char_data* ch )
{
  int mana = ch->base_mana+ch->mod_mana+ch->Level()*ch->Intelligence( )/4;
  mana -= leech_max( ch );
  mana -= prep_max( ch );

  ch->max_mana = max( 0, mana );
  ch->mana     = min( ch->mana, ch->max_mana );
}


/*
 *   MISC ATTRIBUTES 
 */


int Mob_Data :: Size( ) const
{
  return species->size;
}


int player_data :: Size( ) const
{
  return( shdata->race < MAX_PLYR_RACE ?
	  plyr_race_table[ shdata->race ].size : SIZE_HUMAN );
}


/*
 *   MISC
 */


char_data* random_pers( room_data* room, char_data *ch )
{
  int count  = 0;

  for( int i = 0; i < room->contents; ++i ) {
    char_data *rch = character( room->contents[i] );
    if( rch
	&& invis_level( rch ) < LEVEL_BUILDER
	&& rch != ch
	&& ( !ch || rch->Seen( ch ) ) ) {
      ++count;
    }
  }
  
  if( count == 0 )
    return 0;

  count = number_range( 1, count );

  for( int i = 0; ; ++i ) {
    char_data *rch = character( room->contents[i] );
    if( rch
	&& invis_level( rch ) < LEVEL_BUILDER
	&& rch != ch
	&& ( !ch || rch->Seen( ch ) )
	&& --count == 0 ) {
      rch->Show( 1 );
      return rch;
    }
  }
} 


int char_data::Race_Distance( const char_data *ch ) const
{
  // Humans/lizardmen are truly "neutral".
  if( shdata->race == RACE_HUMAN
      || shdata->race == RACE_LIZARD
      || ch->shdata->race == RACE_HUMAN
      || ch->shdata->race == RACE_LIZARD ) {
    return 1;
  }

  if( shdata->race >= MAX_PLYR_RACE
      || ch->shdata->race >= MAX_PLYR_RACE )
    return 0;

  // Return value:
  //   0: they like each other.
  //   1: they are neutral to each other.
  //   2: they are distrustful of each other.
  //   3. they hate each other.
  int dist = 0;

  // Base rules:
  //   Same race: distance = 0.
  //   Both light, both dark: distance = 1.
  //   One light, one dark: distance = 2.
  const int r1 = min( shdata->race, ch->shdata->race );
  const int r2 = max( shdata->race, ch->shdata->race );
  
  if ( r1 == r2 ) {
    dist = 0;
  } else if ( r1 > RACE_LIZARD
	      || r2 <= RACE_LIZARD ) {
    dist = 1;
  } else {
    dist = 2;
  }

  /*
  // Humans are neutral to all races including humans.
  // Lizards are neutral to all other races, but like other lizards.
  if ( r1 == r2 && r1 != RACE_HUMAN) {
    dist = 0;
  } else if ( r1 == RACE_HUMAN		// One or both human
	      || r1 >= RACE_LIZARD	// One lizard or both dark
	      || r2 <= RACE_LIZARD	// One lizard or both light
	      ) {
    dist = 1;
  } else {
    dist = 2;
  }

  // Some specific likes:
  //   elves, ents (0)
  //   dwarves, gnomes (0)
  if ( r1 == RACE_ELF && r2 == RACE_ENT
       || r1 == RACE_GNOME && r2 == RACE_DWARF
       ) {
    --dist;
    
  // Some specific dislikes:
  //   dwarves, ents (2)
  //   gnomes, elves (2)
  } else if ( r1 == RACE_DWARF && r2 == RACE_ENT
	      || r1 == RACE_ELF && r2 == RACE_GNOME ) {
    ++dist;
  }
  */

  return dist;
}


int char_data::Align_Distance (const char_data *ch) const
{
  int dist =  abs( Align_Good_Evil() - ch->Align_Good_Evil() )
    + abs( Align_Law_Chaos() - ch->Align_Law_Chaos() );

  const player_data *p1 = player( this );
  const player_data *p2 = player( ch );

  if( p1 && p2
      && ( ( p1->pcdata->religion != REL_NONE
	     && p1->pcdata->religion == p2->pcdata->religion )
	   || ( p1->pcdata->pfile->clan
		&& p1->pcdata->pfile->clan == p2->pcdata->pfile->clan ) ) ) {
    --dist;
  }

  return dist;
}
