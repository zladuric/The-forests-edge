#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "define.h"
#include "struct.h"


/*
 *   LOCAL_CONSTANTS
 */


const default_data use_msg [] =
{
  { "to_char", "You smoke $p, inhaling the aroma from $P.", ITEM_TOBACCO },
  { "to_room", "$n smokes $p.", ITEM_TOBACCO },
  { "to_char", "You blow $p, producing a loud shriek.", ITEM_WHISTLE },
  { "to_room", "$n blows $p producing a loud shriek.", ITEM_WHISTLE },
  { "to_char", "You play $p, producing a brief interlude of beautiful music.", ITEM_INSTRUMENT },
  { "to_room", "$n plays $p, producing a brief interlude of beautiful music.", ITEM_INSTRUMENT },
  { "", "", -1 }
};


const char *item_type_name [MAX_ITEM] = { "other", "light", "scroll", "wand",
  "staff", "weapon", "gem", "spellbook", "treasure", "armor", "potion",
  "reagent", "furniture", "trash", "vehicle", "object_container", "lock_pick", 
  "drink_container", "key", "food", "money", "key_ring", "boat",
  "corpse", "instrument", "fountain", "whistle", "trap",
  "light_perm", "bandage", "bounty", "gate", "arrow", "skin",
  "body_part", "chair", "table", "book", "pipe", "tobacco", "deck_cards",
  "fire", "garrote", "toy" };

const char *cont_flag_name [MAX_CONT] = {
  "closeable", "pickproof", "closed",
  "locked", "holding"
};

const char *consume_flags [MAX_CONSUME] = { "poisonous", "plaguer" };

const char *trap_flags[MAX_TRAP] = { "poisonous", "blinding" };


int obj_clss_max = 0;

int Obj_Clss_Data :: modified = -1;


/*
 *   OBJ_CLSS_DATA CLASS
 */


Obj_Clss_Data :: Obj_Clss_Data( int num )
  : oprog(0),
    singular(empty_string), plural(empty_string),
    before(empty_string), after(empty_string),
    prefix_singular(empty_string), prefix_plural(empty_string),
    long_s(empty_string), long_p(empty_string),
    creator(0), last_mod(empty_string),
    date(-1), vnum(num), serial(-1), fakes(0),
    item_type(ITEM_TRASH), size_flags(0), restrictions(0),
    anti_flags(0), materials(0), wear_flags(1 << ITEM_TAKE),
    layer_flags(0), count(0), limit(-1), weight(1),
    cost(0), level(1), remort(0), repair(10), durability(1000),
    blocks(0), light(0), comments(empty_string), used(false)
{
  record_new( sizeof( obj_clss_data ), MEM_OBJ_CLSS );

  vzero( extra_flags, 2 );
  vzero( value, 4 );
  vzero( affect_flags, AFFECT_INTS );

  obj_index_list[vnum] = this;

  obj_clss_max = max( obj_clss_max, vnum );
}


Obj_Clss_Data :: ~Obj_Clss_Data( )
{
  record_delete( sizeof( obj_clss_data ), MEM_OBJ_CLSS );

  free_string( singular, MEM_OBJ_CLSS );
  free_string( plural, MEM_OBJ_CLSS );
  free_string( before, MEM_OBJ_CLSS );
  free_string( after, MEM_OBJ_CLSS );
  free_string( prefix_singular, MEM_OBJ_CLSS );
  free_string( prefix_plural, MEM_OBJ_CLSS );
  free_string( long_s, MEM_OBJ_CLSS );
  free_string( long_p, MEM_OBJ_CLSS );
  free_string( creator, MEM_OBJ_CLSS );
  free_string( last_mod, MEM_OBJ_CLSS );
  free_string( comments, MEM_OBJ_CLSS );

  delete_list( oprog );
  affected.delete_list( );
  extra_descr.delete_list( );

  obj_index_list[vnum] = 0;

  if( vnum == obj_clss_max ) {
    for( --obj_clss_max; obj_clss_max > 0 && !obj_index_list[species_max]; --obj_clss_max );
  }
}


int Obj_Data :: Number( ) const
{ return number; }


int Obj_Data :: Selected( ) const
{ return selected; }


int Obj_Data :: Shown( ) const
{ return shown; }


void Obj_Data :: Select( int num )
{
  if( num < 0 ) {
    bug( "Obj::Select( num ): num < 0" );
    bug( "-- Obj = %s", this );
    bug( "-- Num = %d", num );
    bug( "-- Location = %s", Location( ) );
    selected = shown = 0;
  } else if( num > number ) {
    bug( "Obj_Data::Select( num ): num > number" );
    bug( "-- Obj = %s", this );
    bug( "-- Num = %d", num );
    bug( "-- Number = %d", number );
    bug( "-- Location = %s", Location( ) );
    selected = shown = number;
  } else {
    selected = shown = num;
  }
}


void Obj_Data :: Select_All( )
{
  selected = shown = number;
}


void Obj_Data :: Show( int num )
{
  if( num < 0 ) {
    bug( "Obj_Data::Show( num ): num < 0" );
    bug( "-- Obj = %s", this );
    bug( "-- Num = %d", num );
    bug( "-- Location = %s", Location( ) );
    shown = 0;
  } else {
    shown = num;
  }
}


void Obj_Data :: Set_Number( int num )
{
  if( num <= 0 ) {
    bug( "Obj_Data::Set_Number( num ): num <= 0" );
    bug( "-- Obj = %s", this );
    bug( "-- Num = %d", num );
    bug( "-- Location = %s", Location( ) );
  } else {
    /*
    if( array ) {
      bug( "Obj_Data::Set_Number( num ): array != 0" );
      bug( "-- Obj = %s", this );
      bug( "-- Num = %d", num );
      bug( "-- Location = %s", Location( ) );
    }
    */
    number = num;
  }
}


void Obj_Clss_Data :: set_modified( char_data *ch )
{
  if( ch ) {
    free_string( last_mod, MEM_OBJ_CLSS );
    last_mod = alloc_string( ch->descr->name, MEM_OBJ_CLSS );
  }

  modified = date = current_time;
}


/*
 *   OBJ_DATA
 */


static int compare_vnum( obj_data* obj1, obj_data* obj2 )
{
  const int a = obj1->pIndexData->vnum;
  const int b = obj2->pIndexData->vnum;

  return( a < b ? -1 : ( a > b ? 1 : 0 ) );
}


Obj_Data :: Obj_Data( obj_clss_data* obj_clss )
  : pIndexData(obj_clss), save(0),
    source(empty_string), label(empty_string),
    singular(obj_clss->singular), plural(obj_clss->plural),
    before(obj_clss->before), after(obj_clss->after),
    owner(0),
    size_flags(obj_clss->size_flags),
    materials(obj_clss->materials),
    weight(obj_clss->weight), cost(obj_clss->cost),
    timer(0),
    condition(obj_clss->durability),
    rust(0), age(0), layer(0), position(-1),
    light(obj_clss->light), reset(0),
    for_sale(false), sold(false),
    number(1), selected(1), shown(1)
{
  record_new( sizeof( obj_data ), MEM_OBJECT );

  vcopy( extra_flags, obj_clss->extra_flags, 2 );
  vcopy( value, obj_clss->value, 4 );

  valid = OBJ_DATA;

  obj_list.insert( this, obj_list.binary_search( this, compare_vnum ) );
}


Obj_Data :: ~Obj_Data( )
{
  record_delete( sizeof( obj_data ), MEM_OBJECT );
  obj_list -= this;

  free_string( label, MEM_OBJECT );
  free_string( source, MEM_OBJECT );

  if( singular != pIndexData->singular )
    free_string( singular, MEM_OBJECT );
  if( plural != pIndexData->plural )
    free_string( plural, MEM_OBJECT );
  if( after != pIndexData->after )
    free_string( after, MEM_OBJECT );
  if( before != pIndexData->before )
    free_string( before, MEM_OBJECT );
}


/*
 *   SUPPORT FUNCTIONS
 */


bool can_extract( obj_clss_data* obj_clss, char_data* ch )
{
  for( int i = 0; i < obj_list; ++i ) 
    if( obj_list[i]->pIndexData == obj_clss ) {
      fsend( ch, "You must destroy all examples of %s first.",
	     obj_clss );
      return false;
    }

  if( has_reset( obj_clss ) ) { 
    send( ch, "You must first remove all resets of that object.\n\r" );
    return false;
  }

  for( shop_data *shop = shop_list; shop; shop = shop->next ) {
    for( custom_data *custom = shop->custom; custom; custom = custom->next ) {
      if( custom->item == obj_clss ) {
        send( ch, "A custom in room %d creates that item.\n\r",
	      shop->room->vnum );
        return false;
      }
      for( int i = 0; i < MAX_INGRED; ++i ) {
        if( custom->ingred[i] == obj_clss ) {
          send( ch,
		"A custom in room %d requires that item as an ingredient.\n\r",
		shop->room->vnum );
          return false;
	}
      }
    }
  }

  return true;
}        
  

/*
 *   LOW LEVEL OBJECT ROUTINES
 */


// If this returns true, obj1 can be safeley extracted, and
// replaced by obj2; but *not* vice-versa.
bool is_same( const obj_data* obj1, const obj_data* obj2, bool keep )
{
  if( obj1->pIndexData != obj2->pIndexData
      || !obj1->contents.is_empty() || !obj2->contents.is_empty()
      || !obj1->affected.is_empty() || !obj2->affected.is_empty()
      || !obj1->events.is_empty() || !obj2->events.is_empty()
      || obj1->extra_flags[0] != obj2->extra_flags[0] 
      || obj1->extra_flags[1] != obj2->extra_flags[1] 
      || obj1->size_flags != obj2->size_flags
      || obj1->timer != obj2->timer
      || obj1->condition != obj2->condition 
      || obj1->materials != obj2->materials
      || obj1->rust != obj2->rust
      || obj1->age != obj2->age
      || obj1->owner != obj2->owner
      || obj1->weight != obj2->weight
      || obj1->cost != obj2->cost
      || obj1->light != obj2->light
      || obj1->reset != obj2->reset
      || obj1->for_sale != obj2->for_sale
      || obj1->sold != obj2->sold
      || obj1->save != obj2->save
      /*
      || obj1->save && ( keep || obj2->save )		// Not commutative.
         && obj1->save != obj2->save
      */
      )
    return false;
  
  if( obj1->singular != obj2->singular
      && strcasecmp( obj1->singular, obj2->singular ) )
    return false;
  
  if( obj1->plural != obj2->plural
      && strcasecmp( obj1->plural, obj2->plural ) )
    return false;
  
  if( obj1->before != obj2->before
      && strcasecmp( obj1->before, obj2->before ) )
    return false;
  
   if( obj1->after != obj2->after
      && strcasecmp( obj1->after, obj2->after ) )
    return false;
  
  for( int i = 0; i < 4; ++i )
    if( obj1->value[i] != obj2->value[i] )
      return false;
  
  if( strcmp( obj1->label, obj2->label ) ) {
    return false;
  }

  return true;
}


/*
 *   OWNERSHIP
 */


bool Obj_Data :: Belongs( pfile_data* pfile )
{
  return !owner
    || pfile == owner;
}


bool Obj_Data :: Belongs( char_data* ch )
{
  if( !owner )
    return true;

  if( ch ) {
    if( !ch->species ) {
      return ch->pcdata->pfile == owner;
    } else {
      return is_set( ch->status, STAT_PET )
	&& ch->leader
	&& !ch->leader->species
	&& ch->leader->pcdata->pfile == owner;
    }
  }

  return false;
}


void set_owner( pfile_data* pfile, thing_array& array )
{
  for( int i = array.size-1; i >= 0; --i ) {
    if( obj_data *obj = object( array[i] ) ) {
      if( !obj->owner
	  && obj->pIndexData->item_type != ITEM_MONEY ) {
        obj->owner = pfile; 
	consolidate( obj );
      }
      set_owner( pfile, obj->contents );
    }
  }
}


/*
void set_owner( obj_data* obj, pfile_data* buyer )
{
  if( obj->pIndexData->item_type != ITEM_MONEY )
    obj->owner = buyer;
  
  for( int i = 0; i < obj->contents; i++ )
    if( obj_data *content = object( obj->contents[i] ) )
      set_owner( content, buyer );
}
*/

  
void set_owner( obj_data* obj, pfile_data* buyer, pfile_data* seller )
{
  if( obj->Belongs( seller )
      && obj->pIndexData->item_type != ITEM_MONEY
      && ( !obj->owner || !is_set( obj->extra_flags, OFLAG_ONE_OWNER ) ) ) {
    obj->owner = buyer;
    consolidate( obj );
  }

  set_owner( obj->contents, buyer, seller );
}


void set_owner( thing_array& array, pfile_data* buyer, pfile_data* seller )
{
  for( int i = array.size-1; i >= 0; --i )
    if( obj_data *content = object( array[i] ) )
      set_owner( content, buyer, seller );
}

  
void set_owner( obj_data* obj, char_data* buyer, char_data* seller )
{
  if( obj->Belongs( seller )
      && obj->pIndexData->item_type != ITEM_MONEY
      && ( !obj->owner || !is_set( obj->extra_flags, OFLAG_ONE_OWNER ) ) ) {
    obj->owner = ( ( buyer && buyer->pcdata )
		   ? buyer->pcdata->pfile : 0 );
    consolidate( obj );
  }

  set_owner( obj->contents, buyer, seller );
}


void set_owner( thing_array& array, char_data* buyer, char_data* seller )
{
  for( int i = array.size-1; i >= 0; --i )
    if( obj_data *content = object( array[i] ) )
      set_owner( content, buyer, seller );
}


/*
 *   MISC ROUTINES
 */


static const int cond_colors[] = {
  COLOR_RED,	// 0
  COLOR_RED,	// 1
  COLOR_RED,	// 2
  COLOR_YELLOW,	// 3
  COLOR_YELLOW,	// 4
  COLOR_GREEN,	// 5
  COLOR_GREEN,	// 6
  COLOR_GREEN,	// 7
  COLOR_BLUE,	// 8
  COLOR_BLUE,   // 9
  COLOR_MAGENTA // 10
};


static const char *cond_color( char_data *ch, int i )
{
  return color_code( ch, cond_colors[ i ] );
}


void condition_abbrev( char* tmp, obj_data* obj, char_data* ch )
{
  const char *const abbrev [11] = {
    "wls", "dmg", "vwn", "wrn", "vsc",
    "scr", "rea", "goo", "vgo", "exc", "prf" };

  int i = 1000*obj->condition/obj->Durability(); 
  i = range( 0, i/100, 10 );

  sprintf( tmp, "%s%s%s", cond_color( ch, i ), abbrev[i], normal( ch ) );
}


void age_abbrev( char* tmp, obj_data*, char_data* )
{
  sprintf( tmp, "   " );
}


const char *Obj_Data :: condition_name( char_data* ch, bool ansi, int cond )
{
  const char *const conditions [11] = {
    "worthless",
    "damaged",
    "very worn",
    "worn",
    "very scratched",
    "scratched",
    "reasonable",
    "good",
    "very good",
    "excellent",
    "perfect"
  };

  if( cond < 0 )
    cond = condition;

  int i = 1000*cond/Durability(); 
  i = range( 0, i/100, 10 );

  const char *txt = conditions[ i ];

  if( !ansi || !ch->pcdata || ch->pcdata->terminal != TERM_ANSI )
    return txt;

  static char tmp [ ONE_LINE ];
  snprintf( tmp, ONE_LINE, "%s%s%s", cond_color( ch, i ), txt, normal( ch ) );
  
  return tmp;
}


/*
 *   OBJECT UTILITY ROUTINES
 */


void enchant_object( obj_data* obj )
{
  int i;

  if( obj->pIndexData->item_type == ITEM_WAND ) {
    obj->value[3] = number_range( 0, obj->pIndexData->value[3] );
    return;
  }
  
  if( ( obj->pIndexData->item_type == ITEM_WEAPON
	|| obj->pIndexData->item_type == ITEM_ARMOR )
      && !is_set( obj->extra_flags, OFLAG_NO_ENCHANT ) ) {
    if( ( i = number_range( 0, 1000 ) ) >= 900 ) {
      obj->value[0] = ( i > 950 ? ( i > 990 ? ( i == 1000 ? 3 : 2 ) : 1 )
			: ( i < 910 ? ( i == 900 ? -3 : -2 ) : -1 ) );
      set_bit( obj->extra_flags, OFLAG_MAGIC );
      if( obj->value[0] < 0 )
        set_bit( obj->extra_flags, OFLAG_NOREMOVE );
    }
  }
}


void rust_object( obj_data* obj, int chance, bool skin )
{
  if( obj->metal( )
      && !is_set( obj->extra_flags, OFLAG_RUST_PROOF )
      && number_range( 1, 100 ) <= chance ) {
    obj->rust = number_range( 1, 3 );
  }
  
  if( obj->pIndexData->item_type == ITEM_WEAPON
      || obj->pIndexData->item_type == ITEM_ARMOR ) {
    obj->age = skin ? 0 : number_range( 0, obj->Durability()/25-1 );
    obj->condition = number_range( 1, repair_condition( obj ) );
  }
}


void set_quality( obj_data *obj )
{
  if( obj->pIndexData->item_type == ITEM_GEM ) {
    if( obj->pIndexData->value[1] >= MAX_GEM ) {
      obj->value[1] = 0;
    } else if ( obj->pIndexData->value[1] < 0 ) {
      obj->value[1] = 0;
      const int val = number_range( 1, 100 );
      if( val > 95 ) obj->value[1] = GEM_FLAWLESS;
      else if( val > 85 ) obj->value[1] = GEM_SCRATCHED;
      else if( val > 75 ) obj->value[1] = GEM_BLEMISHED;
      else if( val > 65 ) obj->value[1] = GEM_CHIPPED;
      else if( val > 55 ) obj->value[1] = GEM_FRACTURED;
    }
  }
}


void set_alloy( obj_data* obj, int level )
{
  if( !is_set( obj->pIndexData->extra_flags, OFLAG_RANDOM_METAL ) )
    return;

  int metal;

  for( metal = MAT_BRONZE; metal <= MAT_ADAMANTINE; metal++ )
    if( is_set( obj->materials, metal ) )
      break;

  if( metal > MAT_ADAMANTINE ) {
    metal = MAT_BRONZE;
    while( true ) {
      if( metal == MAT_ADAMANTINE
	  || number_range( 0, level+75 ) > level-10*(metal-MAT_BRONZE) )
        break;
      ++metal;
    }
    set_bit( obj->materials, metal );
  }

  if( obj->pIndexData->item_type == ITEM_ARMOR ) {
    obj->value[1] = obj->pIndexData->value[1]-MAT_BRONZE+metal;
  }

  obj->condition = obj->Durability( );
}


void set_size( obj_data *obj, char_data *ch )
{
  const obj_clss_data *obj_clss = obj->pIndexData;

  if( ch ) {
    if( is_set( obj_clss->size_flags, SFLAG_CUSTOM ) ) {
      obj->size_flags = 0;
      if( ch->shdata->race < MAX_PLYR_RACE ) {
	set_bit( obj->size_flags, SFLAG_HUMAN+ch->shdata->race );
	set_bit( obj->size_flags, SFLAG_RACE );
      } else {
	set_bit( obj->size_flags, wear_size( ch ) );
	set_bit( obj->size_flags, SFLAG_SIZE );
      }
      set_bit( obj->size_flags, SFLAG_CUSTOM );
    } else if( is_set( obj_clss->size_flags, SFLAG_RANDOM ) ) {
      obj->size_flags = 0;
      set_bit( obj->size_flags, wear_size( ch ) );
      set_bit( obj->size_flags, SFLAG_SIZE );
    }

  } else {
    if( is_set( obj_clss->size_flags, SFLAG_RANDOM ) ) {
      obj->size_flags = 0;
      set_bit( obj->size_flags, number_range( SFLAG_TINY, SFLAG_GIANT ) );      
      set_bit( obj->size_flags, SFLAG_SIZE );
    } else {
      remove_bit( obj->size_flags, SFLAG_CUSTOM );
    }
  }
}


obj_data* create( obj_clss_data* obj_clss, int number )
{
  if( !obj_clss ) {
    roach( "Create_object: NULL obj_clss." );
    return 0;
  }

  obj_data *obj = new obj_data( obj_clss );

  obj->Set_Number( number );
  obj->Select( number );
  
  switch( obj_clss->item_type ) {
  case ITEM_LIGHT:
  case ITEM_LIGHT_PERM:
    if( obj->value[0] > 0 ) {
      obj->value[2] = obj->value[0];
      obj->timer = 50*PULSE_PER_SECOND;
    }
    break;

  case ITEM_FIRE:
  case ITEM_GATE:
    if( obj->value[0] > 0 ) {
      obj->timer = 50;
    }
    break;

  case ITEM_CORPSE:
    if( obj->value[0] > 0 ) {
      obj->timer = 50;
    }
    // Save weight for partial eating.
    obj->value[2] = obj->weight;
    break;

  case ITEM_FOOD:
    obj->value[2] = obj->value[0];
    break;
  }
  
  if( obj_clss->item_type != ITEM_ARMOR && obj_clss->item_type != ITEM_WEAPON
      && !strcmp( obj_clss->before, obj_clss->after )
      && *obj_clss->plural != '{' && *obj_clss->singular != '{' ) {
    set_bit( obj->extra_flags, OFLAG_IDENTIFIED );
  }

  return obj;
}


obj_data *duplicate( obj_data* copy, int num )
{
  obj_clss_data *obj_clss = copy->pIndexData;
  obj_data *obj = new obj_data( obj_clss );
  
  char *const string_copy [] = { copy->singular, copy->plural,
				 copy->before, copy->after }; 
  char **const string_obj [] = { &obj->singular, &obj->plural,
				 &obj->before, &obj->after }; 
  char *const string_index [] = { obj_clss->singular, obj_clss->plural,
				  obj_clss->before, obj_clss->after }; 
  
  for( int i = 0; i < 4; i++ ) {
    *string_obj[i] = ( string_copy[i] != string_index[i] 
		       ? alloc_string( string_copy[i], MEM_OBJECT ) : string_index[i] );
  }
  
  obj->age            = copy->age;
  obj->size_flags     = copy->size_flags;
  obj->weight         = copy->weight;
  obj->condition      = copy->condition;
  obj->rust           = copy->rust;
  obj->timer          = copy->timer; 
  obj->materials      = copy->materials;
  obj->owner          = copy->owner;
  obj->light          = copy->light;
  obj->temp           = copy->temp;

  obj->Set_Number( num );
  obj->Select_All( );

  vcopy( obj->extra_flags, copy->extra_flags, 2 );
  vcopy( obj->value, copy->value, 4 );

  //  obj_clss->count += num;

  if( copy->save ) {
    copy->save->save_list += obj;
    obj->save = copy->save;
  }

  return obj;
}


/*
 *   OBJECT EXTRACTION ROUTINES
 */


void Obj_Data :: Extract( int i )
{
  if( !Is_Valid( ) ) {
    roach( "Extracting invalid object." );
    roach( "-- Valid = %d", valid );
    roach( "--   Obj = %s", this );
    roach( "--     i = %d", i );
    return;
  }

  if( number > i ) {
    if( array ) {
      From( i )->Extract( );
    } else {
      remove_weight( this, i );
      number -= i;
    }
    return;
  }

  if( i > number ) {
    roach( "Extract( Obj ): number > amount." );
    roach( "-- Obj = %s", this );
    roach( "-- Number = %d", i ); 
    roach( "-- Amount = %d", number );
  }

  Extract( );
}


void Obj_Data :: Extract( )
{
  if( !Is_Valid( ) ) {
    roach( "Extracting invalid object." );
    roach( "-- Valid = %d", valid );
    roach( "--   Obj = %s", this );
    return;
  }

  if( array )
    From( number );

  extract( contents );

  clear_queue( this );
  stop_events( this );
  
  // Extracting player corpse, remove from lost+found file.
  if( owner ) {
    if( pIndexData->vnum == OBJ_CORPSE_PC
	|| pIndexData->vnum == OBJ_CORPSE_PET ) {
      const int i = owner->corpses.find( this );
      if( i >= 0 ) {
	owner->corpses -= this;
	owner->Save( );
      }
    } else if( pIndexData->vnum == OBJ_CACHE ) {
      const int i = owner->caches.find( this );
      if( i >= 0 ) {
	owner->caches -= this;
	owner->Save( );
      }
    }
  }

  if( save ) {
    save->save_list -= this;
    save = 0;
  } 

  affected.delete_list();

  timer      = -2;
  valid      = -1;

  extracted += this;
}


/*
 *   DISK ROUTINES
 */


void fix( obj_clss_data* obj_clss )
{
  //  if( obj_clss->item_type == ITEM_SCROLL ) 
  //    set_bit( obj_clss->materials, MAT_PAPER );

  //  for( int i = 0; i < MAX_ANTI; i++ )
  //    if( !strncasecmp( anti_flags[i], "unused", 6 ) )
  //      remove_bit( obj_clss->anti_flags, i );
}


void load_objects( void )
{
  obj_clss_data*  obj_clss;
  oprog_data*        oprog;
  char              letter;
  int                 vnum;

  echo( "Loading Objects ...\n\r" );
  vzero( obj_index_list, MAX_OBJ_INDEX );

  FILE *fp = open_file( AREA_DIR, OBJECT_FILE, "r", true );

  if( strcmp( fread_word( fp ), "#OBJECTS" ) ) 
    panic( "Load_objects: header not found" );

  while (true) {
    letter = fread_letter( fp );

    if( letter != '#' ) 
      panic( "Load_objects: # not found." );

    if( ( vnum = fread_number( fp ) ) == 0 )
      break;
   
    if( vnum < 0 || vnum >= MAX_OBJ_INDEX ) 
      panic( "Load_objects: vnum out of range." );

    if( obj_index_list[vnum] ) 
      panic( "Load_objects: vnum %d duplicated.", vnum );

    obj_clss = new obj_clss_data(vnum);
 
    obj_clss->singular         = fread_string( fp, MEM_OBJ_CLSS );
    obj_clss->plural           = fread_string( fp, MEM_OBJ_CLSS );
    obj_clss->before           = fread_string( fp, MEM_OBJ_CLSS );
    obj_clss->after            = fread_string( fp, MEM_OBJ_CLSS );
    obj_clss->long_s           = fread_string( fp, MEM_OBJ_CLSS );
    obj_clss->long_p           = fread_string( fp, MEM_OBJ_CLSS );
    obj_clss->prefix_singular  = fread_string( fp, MEM_OBJ_CLSS );
    obj_clss->prefix_plural    = fread_string( fp, MEM_OBJ_CLSS );
    obj_clss->creator          = fread_string( fp, MEM_OBJ_CLSS );
    obj_clss->comments         = fread_string( fp, MEM_OBJ_CLSS );
    obj_clss->last_mod         = fread_string( fp, MEM_OBJ_CLSS );      

    obj_clss->item_type       = fread_number( fp );
    obj_clss->fakes           = fread_number( fp );
    obj_clss->extra_flags[0]  = fread_number( fp );
    obj_clss->extra_flags[1]  = fread_number( fp );
    obj_clss->wear_flags      = fread_number( fp );
    obj_clss->anti_flags      = fread_number( fp );
    obj_clss->restrictions    = fread_number( fp );
    obj_clss->size_flags      = fread_number( fp );
    obj_clss->materials       = fread_number( fp );

    obj_clss->affect_flags[0] = fread_number( fp );
    obj_clss->affect_flags[1] = fread_number( fp );
    obj_clss->affect_flags[2] = fread_number( fp );
    obj_clss->layer_flags     = fread_number( fp );

    obj_clss->value[0]      = fread_number( fp );
    obj_clss->value[1]      = fread_number( fp );
    obj_clss->value[2]      = fread_number( fp );
    obj_clss->value[3]      = fread_number( fp );

    obj_clss->weight        = fread_number( fp );
    obj_clss->cost          = fread_number( fp );
    obj_clss->level         = fread_number( fp );
    obj_clss->remort        = fread_number( fp );
    obj_clss->limit         = fread_number( fp );
    obj_clss->repair        = fread_number( fp );
    obj_clss->durability    = fread_number( fp );
    obj_clss->blocks        = fread_number( fp );
    obj_clss->light         = fread_number( fp );

    obj_clss->date          = fread_number( fp );

    read_affects( fp, obj_clss ); 
    read_extras( fp, obj_clss->extra_descr );

    fread_letter( fp );

    while( true ) {
      int number = fread_number( fp );

      if( number == -1 )
        break;

      oprog = new oprog_data( obj_clss );
      oprog->trigger  = number;
      oprog->obj_vnum = fread_number( fp );
      oprog->flags    = fread_number( fp );
      oprog->command  = fread_string( fp, MEM_OPROG );
      oprog->target   = fread_string( fp, MEM_OPROG );

      oprog->read( fp );
    }
    
    fix( obj_clss );
  }

  fclose( fp );

  for( int i = 1; i <= obj_clss_max; ++i ) 
    if( obj_index_list[i] )
      for( oprog = obj_index_list[i]->oprog; oprog; oprog = oprog->next )
        if( oprog->obj_vnum > 0 )
          oprog->obj_act = get_obj_index( oprog->obj_vnum );
}
