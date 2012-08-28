#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include "define.h"
#include "struct.h"


bool   remove           ( char_data*, obj_data*& );
void   wear_trigger     ( char_data*, obj_data* ); 


static thing_func cantwear;
static thing_func wrongsize;
static thing_func needremove;
static thing_func underneath;


const char *wear_name [ MAX_ITEM_WEAR ] = {
  "take", "finger", "neck", "body", "head",
  "legs", "feet", "hands", "arms", "?unused0", "?unused1", "waist", "wrist",
  "right_hand", "left_hand", "?unused2", "?unused3", "?unused4", "float_nearby",
  "horse_body", "horse_back", "horse_foot" 
};

const int wear_index [ MAX_WEAR ] = {
  ITEM_WEAR_FLOATING, ITEM_WEAR_FINGER,
  ITEM_WEAR_FINGER, ITEM_WEAR_NECK, ITEM_TAKE, ITEM_WEAR_BODY,
  ITEM_WEAR_HEAD, ITEM_WEAR_LEGS, ITEM_WEAR_FEET, ITEM_WEAR_HANDS,
  ITEM_WEAR_ARMS, ITEM_TAKE, ITEM_TAKE, ITEM_WEAR_WAIST,
  ITEM_WEAR_WRIST, ITEM_WEAR_WRIST, ITEM_HELD_R, ITEM_HELD_L,
  ITEM_TAKE, ITEM_TAKE, ITEM_TAKE, ITEM_TAKE,
  ITEM_WEAR_HORSE_BODY, ITEM_WEAR_HORSE_BACK, ITEM_WEAR_HORSE_FEET
};
  
const char* wear_abbrev[] = {
  "Grnd", "Inv.",
  "Near", "rFgr", "lFgr",
  "Neck", "??  ", "Body", "Head", "Legs", "Feet", "Hand", "Arms", "??  ",
  "??  ", "Wst.", "rWrs", "lWrs", "rHnd", "lHnd", "??  ", "??  ", "??  ",
  "??  ", "??  ", "??  ", "??  " };

const char* skin_abbrev[] = {
  "Skin", "Inv.",
  "Near", "rFgr", "lFgr",
  "Neck", "??  ", "Body", "Head", "Legs", "Feet", "Hand", "Arms", "??  ",
  "??  ", "Wst.", "rWrs", "lWrs", "rHnd", "lHnd", "??  ", "??  ", "??  ",
  "??  ", "??  ", "??  ", "??  " };

const char* reset_wear_name[] = {
  "ground", "inventory",
  "float_nearby", "right_finger", "left_finger",
  "neck", "?unused1", "body", "head", "legs", "feet", "hands", "arms",
  "?unused2", "?unused3", "waist", "right_wrist", "left_wrist",
  "right_hand", "left_hand", "?unused4", "?unused5", "?unused6", "?unused7",
  "horse_body", "horse_back", "horse_foot" };

const char* reset_skin_name[] = {
  "skin", "inventory",
  "float_nearby", "right_finger", "left_finger",
  "neck", "?unused1", "body", "head", "legs", "feet", "hands", "arms",
  "?unused1", "?unused2", "waist", "right_wrist", "left_wrist",
  "right_hand", "left_hand", "?unused3", "?unused4", "?unused5", "?unused6",
  "horse_body", "horse_back", "horse_foot" };


const char** wear_part_name = &reset_wear_name[2];

static const char* wear_verb [] =
{
  "You toss %s into the air.",
  "You place %s on your right ring finger.",
  "You place %s on your left ring finger.",
  "You place %s around your neck.",
  0,
  "You wear %s.",
  "You wear %s on your head.",
  "You wear %s on your legs.", 
  "You wear %s on your feet.",
  "You wear %s on your hands.",
  "You wear %s on your arms.",
  0,
  0,
  "You wear %s around your waist.",
  "You place %s on your right wrist.",
  "You place %s on your left wrist.",
  "You hold %s in your right hand.",
  "You hold %s in your left hand.",
  0,
  0,
  0,
  0,
  "You wear %s on your body.",
  "You wear %s on your back.",
  "You wear %s on your feet."
};

 
static const char* wear_publ [] =
{ 
  "%s tosses %s into the air.",
  "%s places %s on %s left ring finger.",
  "%s places %s on %s right ring finger.",
  "%s places %s around %s neck.",
  0,
  "%s wears %s.",
  "%s wears %s on %s head.",
  "%s wears %s on %s legs.", 
  "%s wears %s on %s feet.",
  "%s wears %s on %s hands.",
  "%s wears %s on %s arms.",
  0,
  0,
  "%s wears %s around %s waist.",
  "%s places %s on %s right wrist.",
  "%s places %s on %s left wrist.",
  "%s holds %s in %s right hand.",
  "%s holds %s in %s left hand.",
  0,
  0,
  0,
  0,
  "%s wears %s on %s body.",
  "%s wears %s on %s back.",
  "%s wears %s on %s feet."
};

 
static const char* remo_verb [] =
{
  "You catch %s that was floating nearby.",
  "You remove %s from your right ring finger.",
  "You remove %s from your left ring finger.",
  "You remove %s from around your neck.",
  0,
  "You remove %s.",
  "You take %s off your head.",
  "You take %s off your legs.", 
  "You take %s off your feet.",
  "You take %s off your hands.",
  "You take %s off your arms.",
  0,
  0,
  "You remove %s from around your waist.",
  "You remove %s from your right wrist.",
  "You remove %s from your left wrist.",
  "You remove %s from your right hand.",
  "You remove %s from your left hand.",
  0,
  0,
  0,
  0,
  "You remove %s from your body.",
  "You remove %s from your back.",
  "You remove %s from your feet."
};

 
static const char* remo_publ [] =
{ 
  "%s catches %s.",
  "%s removes %s from %s right ring finger.",
  "%s removes %s from %s left ring finger.",
  "%s removes %s from around %s neck.",
  0,
  "%s removes %s.",
  "%s takes %s off %s head.",
  "%s takes %s off %s legs.", 
  "%s takes %s off %s feet.",
  "%s takes %s off %s hands.",
  "%s takes %s off %s arms.",
  0,
  0,
  "%s removes %s from around %s waist.",
  "%s removes %s from %s right wrist.",
  "%s removes %s from %s left wrist.",
  "%s removes %s from %s right hand.",
  "%s removes %s from %s left hand.",
  0,
  0,
  0,
  0,
  "%s removes %s from %s body.",
  "%s removes %s from %s back.",
  "%s removes %s from %s feet."
};

 
const char *where_name [ MAX_WEAR ] =
{
  "floating nearby   ",
  "right hand finger ",
  "left hand finger  ",
  "worn around neck  ",
  "??                ",
  "worn on body      ",
  "worn on head      ",
  "worn on legs      ",
  "worn on feet      ",
  "worn on hands     ",
  "worn on arms      ",
  "??                ",
  "??                ",
  "worn about waist  ",
  "right wrist       ",
  "left wrist        ",
  "right hand        ",
  "left hand         ",
  "??                ",
  "??                ", 
  "??                ",
  "??                ",
  "covering body     ", 
  "placed on back    ",
  "attached to feet  "
};


static const char *wear_loc_name [ MAX_WEAR ] = {
  "floating nearby",
  "right hand finger",
  "left hand finger",
  "neck",
  "[BUG]",
  "body",
  "head",
  "legs",
  "feet",
  "hands",
  "arms",
  "[BUG]",
  "[BUG]",
  "waist",
  "right wrist",
  "left wrist",
  "right hand",
  "left hand",
  "[BUG]",
  "[BUG]", 
  "[BUG]",
  "[BUG]",
  "body", 
  "back",
  "feet"
};


const default_data wear_msg[] = {
  { "to_char",  "You wear $p on your $t.", -1 },
  { "to_room",  "$n wears $p on $s $t.", -1 },
  { "", "", -1 }
};

const default_data remove_msg[] = {
  { "to_char",  "You take $p off your $t.", -1 },
  { "to_room",  "$n takes $p off $s $t.", -1 },
  { "", "", -1 }
};


/*
 *   SUPPORT ROUTINES
 */ 


static void sort_by_layer( thing_array& array, bool remove = false )
{
  if( array <= 1 )
    return;

  if( !remove ) {
    // Before equip, layer is arbitrary.
    // Set to lowest layer.
    for( int i = 0; i < array; ++i ) {
      obj_data *obj = (obj_data *) array[i];
      for( int j = 0; j < MAX_LAYER; ++j ) {
	if( is_set(  &obj->pIndexData->layer_flags, j ) ) {
	  obj->layer = j;
	  break;
	}
      }
    }
  }

  int size = array-1;
  bool done = false;

  while( size >= 1 && !done ) {
    done = true;
    for( int i = 0; i < size; ++i ) {
      obj_data *o1 = (obj_data *)array[i];
      obj_data *o2 = (obj_data *)array[i+1];
      if( remove ? o1->layer < o2->layer : o1->layer > o2->layer ) {
	obj_data *tmp = o1;
	array[i] = o2;
	array[i+1] = tmp;
	done = false;
      }
    }
    --size;
  }
}


obj_data* char_data :: Wearing( int slot, int layer )
{
  for( int j = 0; j < wearing; j++ ) {
    obj_data *obj = (obj_data*) wearing[j];
    if( obj->position == slot
	&& ( layer == -1 || obj->layer == layer ) ) {
      return obj;
    }
  }

  return 0;
}


static obj_data *wear_the_thing( char_data* ch, obj_data* obj,
				 int slot, int layer,
				 thing_array* array )
{
  if( !array )
    return obj;

  for( int i = 0; i < *array; ++i ) {
    obj_data *obj2 = (obj_data *) array->list[i];
    if( obj2->position == slot
	&& obj2->layer == layer ) {
      return 0;
    }
    // Can't wear a 2H if something in either hand already.
    if( is_set( obj->extra_flags, OFLAG_TWO_HAND )
	&& obj->layer >= LAYER_BASE
	&& ( obj2->position == WEAR_HELD_R || obj2->position == WEAR_HELD_L )
	&& obj2->layer >= LAYER_BASE ) {
      return 0;
    }
    // Can't wear something in either hand if wearing a 2H already.
    if( is_set( obj2->extra_flags, OFLAG_TWO_HAND )
	&& obj2->layer >= LAYER_BASE
	&& layer >= LAYER_BASE
	&& ( slot == WEAR_HELD_R || slot == WEAR_HELD_L ) ) {
      return 0;
    }
  }

  obj_data *worn = (obj_data*) obj->From( 1 );
  worn->Show( 1 );
  worn->position = slot;
  worn->layer = layer;

  *array += worn;

  if( worn == obj || obj->Selected( ) == 0 ) {
    // Wore the last one, or the last selected one.
    return obj;
  }

  return 0;
}


static obj_data *block;


static obj_data *two_hands( char_data* ch, obj_data* obj, int slot, int layer,
			    thing_array* array )
{
  // Hand-held object rules.

  if( layer < LAYER_BASE )
    return 0;
  
  // If it doesn't even go on this layer, don't bother.
  //  if( !is_set( obj->pIndexData->layer_flags, layer ) )
  //    return 0;

  // If holding a 2-handed object, can't hold anything else, period.
  if( is_set( ch->status, STAT_TWO_HAND ) ) {
    if( !array ) {
      block = ch->Wearing( WEAR_HELD_R );
    }
    return 0;
  }

  // Right-hand rules.
  if( slot == WEAR_HELD_R ) {
    if( is_set( obj->extra_flags, OFLAG_TWO_HAND ) ) {
      // Right hand, 2H object only works if left is completely empty.
      block = ch->Wearing( WEAR_HELD_L );
      if( block ) {
	return 0;
      }
      /*
      if( array ) {
	set_bit( ch->status, STAT_TWO_HAND );
      }
      */
    }

    return wear_the_thing( ch, obj, slot, layer, array );
  }

  // Left-hand rules.
  if( is_set( obj->extra_flags, OFLAG_TWO_HAND ) ) {
    // Left hand, 2H object only works if right is completely empty.
    block = ch->Wearing( WEAR_HELD_R );
    if( block ) {
      return 0;
    }
    /*
    if( array ) {
      set_bit( ch->status, STAT_TWO_HAND );
    }
    */
  }
  
  return wear_the_thing( ch, obj, slot, layer, array );
}


/*
 *   CAN_USE ROUTINES
 */


bool will_fit( char_data* ch, obj_data* obj, int i )
{
  if( wear_index[i] == ITEM_TAKE
      || !can_wear( obj, wear_index[i] ) )
    return false;

  if( ch->is_humanoid( ) )
    return i < MAX_WEAR_HUMANOID;

  return is_set( ch->species->wear_part, i );
}


// CALLED BY AUCTION, CUSTOM
bool can_use( char_data* ch, obj_clss_data* obj_clss, obj_data* obj )
{
  if( !ch->is_humanoid( )
      || !ch->can_carry( ) ) 
    return false;
  
  if( privileged( ch, LEVEL_BUILDER ) )
    return true;

  if( obj ) {
    obj_clss = obj->pIndexData;
    
    // LEVEL
    if( ch->Level( ) < obj->Level( ) ) 
      return false;
    
    // RACE SIZE
    if( is_set( obj->size_flags, SFLAG_RACE )
	&& ( ch->shdata->race >= MAX_PLYR_RACE
	     || !is_set( obj->size_flags, SFLAG_HUMAN+ch->shdata->race ) ) )
      return false;
    
    // SIZE
    if( is_set( obj->size_flags, SFLAG_SIZE )
	&& !is_set( obj->size_flags, wear_size( ch ) ) )
      return false;
    
    // STOLEN
    if( ( ch->species
	  || ch->pcdata->clss != CLSS_THIEF
	  || is_set( obj->extra_flags, OFLAG_ONE_OWNER ) )
	&& !obj->Belongs( ch ) )
      return false;
    
    // FOOD
    if( obj_clss->item_type == ITEM_FOOD ) {
      if( vegetarian( ch )
	  && is_set( obj->materials, MAT_FLESH ) )
	return false;
      if( carnivore( ch )
	  && !is_set( obj->materials, MAT_FLESH ) )
	return false;
    }
    
  } else {
    
    // LEVEL
    if( ch->Level( ) < obj_clss->level ) 
      return false;

    // RACE SIZE
    if( is_set( obj_clss->size_flags, SFLAG_RACE )
	&& ( ch->shdata->race >= MAX_PLYR_RACE
	     || !is_set( obj_clss->size_flags, SFLAG_HUMAN+ch->shdata->race ) ) )
      return false;
    
    // SIZE
    if( is_set( obj_clss->size_flags, SFLAG_SIZE )
	&& !( is_set( obj_clss->size_flags, SFLAG_RANDOM )
	      || is_set( obj_clss->size_flags, wear_size( ch ) ) ) )
      return false;
 
    // FOOD
    if( obj_clss->item_type == ITEM_FOOD ) {
      if( vegetarian( ch )
	  && is_set( obj_clss->materials, MAT_FLESH ) )
	return false;
      if( carnivore( ch )
	  && !is_set( obj_clss->materials, MAT_FLESH ) )
	return false;
    }
  }
  
  // REMORT LEVEL
  if( ch->Remort( ) < obj_clss->remort ) 
    return false;
  
  // ANTI RESTRICTIONS
  if( ( ch->shdata->race < MAX_PLYR_RACE
	&& is_set( obj_clss->anti_flags, ANTI_HUMAN+ch->shdata->race ) )
      || ( !ch->species && is_set( obj_clss->anti_flags, ANTI_MAGE+ch->pcdata->clss ) )
      || is_set( obj_clss->anti_flags, ANTI_GOOD+ch->Align_Good_Evil() )
      || is_set( obj_clss->anti_flags, ANTI_LAWFUL+ch->Align_Law_Chaos() ) )
    return false;
  if( ( ch->sex == SEX_MALE || ch->sex == SEX_HERMAPHRODITE )
      && is_set( obj_clss->anti_flags, ANTI_MALE ) )
    return false;
  if( ( ch->sex == SEX_FEMALE || ch->sex == SEX_HERMAPHRODITE )
      && is_set( obj_clss->anti_flags, ANTI_FEMALE ) )
    return false;
  if( !ch->species
      && ch->pcdata->clss == CLSS_PALADIN
      && is_set( obj_clss->restrictions, RESTR_DISHONORABLE ) )
    return false;
  if( !ch->species
      && ( ch->pcdata->clss == CLSS_CLERIC || ch->pcdata->clss == CLSS_DRUID )
      && is_set( obj_clss->restrictions, RESTR_BLADED ) )
    return false;
  
  return true;
}


/*
 *   WEAR ROUTINES
 */ 


static thing_data *cantwear( thing_data* t1, char_data* ch, thing_data* )
{
  obj_data *obj = (obj_data*) t1;

  bool held = false;
  bool worn = false;
  for( int i = LAYER_BOTTOM; i < MAX_LAYER; ++i ) {
    if( is_set( obj->pIndexData->layer_flags, i ) ) {
      worn = true;
      if( i >= LAYER_BASE ) {
	held = true;
	break;
      }
    }
  }

  if( !worn )
    return 0;

  /*
  for( int i = LAYER_BOTTOM; !is_set( obj->pIndexData->layer_flags, i ); ) {
    if( ++i == MAX_LAYER )
      return 0;
  }
  */

  for( int i = 0; i < MAX_WEAR; i++ ) {
    if( ( i == WEAR_HELD_L || i == WEAR_HELD_R )
	&& !held ) {
      continue;
    }
    if( will_fit( ch, obj, i ) ) {
      return obj;
    }
  }

  return 0;
}


static thing_data *wrongsize( thing_data* t1, char_data* ch, thing_data* )
{
  if( privileged( ch, LEVEL_BUILDER ) )
    return t1;

  obj_data *obj = (obj_data*) t1;

  if( is_set( obj->size_flags, SFLAG_RACE )
      && ( ch->shdata->race >= MAX_PLYR_RACE
	   || !is_set( obj->size_flags, SFLAG_HUMAN+ch->shdata->race ) ) )
    return 0;
  
  if( is_set( obj->size_flags, SFLAG_SIZE )
      && !is_set( obj->size_flags, wear_size( ch ) ) )
    return 0;

  return obj;
}


static thing_data *wearstolen( thing_data* t1, char_data* ch, thing_data* )
{
  if( privileged( ch, LEVEL_BUILDER ) )
    return t1;

  obj_data *obj = (obj_data*) t1;

  if( ( ch->species
	|| ch->pcdata->clss != CLSS_THIEF
	|| is_set( obj->extra_flags, OFLAG_ONE_OWNER ) )
      && !obj->Belongs( ch ) )
    return 0;
  /*
    if( !ch->species
    && ch->pcdata->clss != CLSS_THIEF
    && !stolen( obj, ch ) )		// !stolen() means obj stolen
    return 0;
  */

  return obj;
}


static thing_data *bladed( thing_data* t1, char_data* ch, thing_data* )
{
  if( privileged( ch, LEVEL_BUILDER ) )
    return t1;

  obj_data* obj = (obj_data*) t1;

  if( !ch->species
      && ( ch->pcdata->clss == CLSS_CLERIC || ch->pcdata->clss == CLSS_DRUID )
      && is_set( obj->pIndexData->restrictions, RESTR_BLADED ) )
    return 0;

  return obj;
}


static thing_data *dishonorable( thing_data* t1, char_data* ch, thing_data* )
{
  if( privileged( ch, LEVEL_BUILDER ) )
    return t1;

  obj_data* obj = (obj_data*) t1;

  if( !ch->species
      && ch->pcdata->clss == CLSS_PALADIN 
      && is_set( obj->pIndexData->restrictions, RESTR_DISHONORABLE ) ) 
    return 0;

  return obj;
}


static thing_data *hiding( thing_data *t1, char_data *ch, thing_data* )
{
  if( !is_set( ch->status, STAT_HIDING )
      && !is_set( ch->status, STAT_CAMOUFLAGED ) )
    return t1;

  obj_data *obj = (obj_data*) t1;

  if( is_set( obj->pIndexData->restrictions, RESTR_NO_HIDE )
      || is_set( obj->extra_flags, OFLAG_HUM )
      || is_set( obj->extra_flags, OFLAG_FLAMING )
      || is_set( obj->extra_flags, OFLAG_BURNING ) )
    return 0;
  
  return obj;
}


static thing_data *sneaking( thing_data *t1, char_data *ch, thing_data* )
{
  if( !is_set( ch->status, STAT_SNEAKING ) )
    return t1;

  obj_data *obj = (obj_data*) t1;

  if( is_set( obj->pIndexData->restrictions, RESTR_NO_SNEAK )
      || is_set( obj->extra_flags, OFLAG_HUM )
      || is_set( obj->extra_flags, OFLAG_FLAMING )
      || is_set( obj->extra_flags, OFLAG_BURNING ) )
    return 0;

  return obj;
}


static thing_data *waterlight( thing_data* t1, char_data* ch, thing_data* t2 )
{
  obj_data *obj = (obj_data*) t1;

  if ( obj->pIndexData->item_type != ITEM_LIGHT
       && obj->pIndexData->item_type != ITEM_LIGHT_PERM
       || is_set( obj->extra_flags, OFLAG_WATER_PROOF ) ) {
    return obj;
  }
  
  return ( !is_submerged( ch ) || ch->can_hold_light() ) ? obj : 0;
}


// The list of things (one per wear loc) that could be removed to allow wearing the item.
static thing_array blocking_list;


static thing_data *needremove( thing_data* t1, char_data* ch, thing_data* t2 )
{
  obj_data*       obj  = (obj_data*) t1;
  thing_array*  array  = (thing_array*) t2;

  blocking_list.clear( );
  block = 0;

  for( int i = 0; i < MAX_WEAR; ++i ) {
    if( will_fit( ch, obj, i ) ) {
      int j;
      // Find the highest layer with a non-removable equip.
      for( j = MAX_LAYER-1; j >= 0; --j ) {
	// Note: assumes at least one layer flag is set.
        // cantwear() should have checked for this already.
	if( obj_data *worn = ch->Wearing( i, j ) ) {
	  if( !worn->removable( ch ) ) {
	    block = worn;
	    break;
	  }
	  if( !is_set( worn->extra_flags, OFLAG_PASS_THROUGH ) ) {
	    oprog_data *oprog;
	    for( oprog = worn->pIndexData->oprog; oprog; oprog = oprog->next ) {
	      if( oprog->trigger == OPROG_TRIGGER_WEAR
		  || oprog->trigger == OPROG_TRIGGER_REMOVE ) {
		break;
	      }
	    }
	    if( oprog ) {
	      block = worn;
	      break;
	    }
	  }
	}
      }

      // Starting above that layer, try to wear the object on the lowest layer.
      for( ++j; j < MAX_LAYER; ++j ) {
	if( is_set( obj->pIndexData->layer_flags, j ) ) {
	  obj_data *worn = ch->Wearing( i, j );
	  if( !worn ) {
	    if( i == WEAR_HELD_L || i == WEAR_HELD_R ) {
	      if( two_hands( ch, obj, i, j, array ) ) {
		return obj;
	      }
	    } else {
	      if( wear_the_thing( ch, obj, i, j, array ) ) {
		return obj;
	      }
	    }
	  } else {
	    block = worn;
	  }
	}
      }

      if( block && !array ) {
	block->Select( 1 );
	blocking_list += block;
      }
    }
  }
  
  return 0;
}


bool could_wear( char_data *ch, obj_data *obj, bool trying )
{
  if( obj != cantwear( obj, ch )
      || obj != wrongsize( obj, ch )
      || obj != antiobj( obj, ch ) ) {
    //      || obj != needremove( obj, ch ) ) {
    return false;
  }
  
  if( trying
      && obj != needremove( obj, ch ) )
    return false;

  return true;
}


static oprog_data *wear_trigger( obj_clss_data *clss )
{
  for( oprog_data *oprog = clss->oprog; oprog; oprog = oprog->next ) {
    if( oprog->trigger == OPROG_TRIGGER_WEAR ) {
      return oprog;
    }
  }

  return 0;
}


void do_wear( char_data* ch, const char *argument )
{
  if( !ch->can_carry( )
      && is_confused_pet( ch ) )
    return; 

  thing_array *array;
  if( !( array = several_things( ch, argument, "wear", &ch->contents ) ) )
    return;

  wear( ch, *array );
  
  delete array;
}


void wear( char_data *ch, thing_array& stuff )
{
  sort_by_layer( stuff );

  thing_array   subset  [ 12 ];
  thing_func*     func  [ 12 ]  = { cantwear, wrongsize,
				    wearstolen, bladed, dishonorable,
				    hiding, sneaking,
				    levellimit, antiobj,
				    waterlight, needremove, 0 };
  thing_array worn;

  sort_objects( ch, stuff, (thing_data*) &worn, 12, subset, func );

  for( int i = 0; i < worn; ++i ) {
    obj_data *obj = object( worn[i] );

    // Temporarily recalc light, so room can see messages.
    // Done again in equip().
    if( obj->Light() != 0 ) {
      int max_light = obj->Light();
      for( int i = 0; i < ch->wearing; ++i ) {
	int l = ch->wearing[i]->Light();
	if( abs( l ) > abs( max_light ) ) {
	  max_light = l;
	}
      }
      if( max_light != ch->wearing.light ) {
	ch->wearing.light = max_light;
	if( ch->in_room ) {
	  ch->in_room->recalc_light();
	}
      }
    }

    oprog_data *oprog = wear_trigger( obj->pIndexData );

    // Message to char.
    if( wear_verb[obj->position] ) {
      if( const extra_data *ed = oprog ? oprog->find_extra( "to_char" ) : 0 ) {
	act( ch, ed->text, 0, obj, wear_loc_name[obj->position] );
      } else {
	if( is_set( obj->extra_flags, OFLAG_TWO_HAND ) ) {
	  fsend( ch, "You hold %s in your hands.", obj->Seen_Name( ch ) );
	} else {
	  fsend( ch, wear_verb[obj->position], obj->Seen_Name( ch ) );
	}
      }
      if( !obj->removable( ch ) ) {
	fsend_color( ch, COLOR_MILD, "%s seems to attach itself to you.", obj->Seen_Name( ch ) );
      }
    } else {
      bug( "Wear: bad position %d on obj %s.", obj->position, obj );
    }

    // Message to room.
    if( ch->array ) {
      msg_type = MSG_EQUIP;

      for( int j = 0; j < *ch->array; ++j ) {
	char_data *rch;
	if( !( rch = character( ch->array->list[j] ) )
	    || rch == ch
	    || !rch->pcdata
	    || !ch->Seen( rch )  
	    || !rch->Accept_Msg( ch ) )
	  continue;

	if( worn > 1
	    && !is_set( rch->pcdata->message, MSG_MULTIPLE_ITEMS ) ) {
	  if( i == 0 ) {
	    fsend( rch, "%s wears several items.", ch );
	  }
	} else if( wear_publ[obj->position] ) {
	  if( const extra_data *ed = oprog ? oprog->find_extra( "to_room" ) : 0 ) {
	    act( rch, ed->text, ch, obj, wear_loc_name[obj->position] );
	  } else {
	    if( is_set( obj->extra_flags, OFLAG_TWO_HAND ) ) {
	      fsend( rch, "%s holds %s in %s hands.", ch, obj, ch->His_Her( rch ) );
	    } else {
	      fsend( rch, wear_publ[obj->position], ch, obj, ch->His_Her( rch ) );
	    }
	  }
	}
      }
    }

    obj->To( ch->wearing );
  }

  page_priv( ch, 0, empty_string );
  page_priv( ch, &subset[0], "can't wear" );
  page_priv( ch, &subset[1], "are the wrong size to wear" );
  page_priv( ch, &subset[2], 0, 0, "is stolen", "are stolen" );
  page_priv( ch, &subset[3], 0, 0,
	     "is a bladed weapon and forbidden to you",
	     "are bladed weapons and forbidden to you" ); 
  page_priv( ch, &subset[4], "you consider", 0, "dishonorable" );
  page_priv( ch, &subset[5], "you can't use", 0, "while hiding" );
  page_priv( ch, &subset[6], "you can't use", 0, "while sneaking" );
  page_priv( ch, &subset[7], "can't cope with" );
  page_priv( ch, &subset[8], "sense aversion from" );
  page_priv( ch, &subset[9], "you can't use", 0, "in the water" );

  rehash( ch, subset[10] );
  for( int i = 0; i < subset[10]; ++i ) {
    if( subset[10][i]->Shown( ) ) {
      if( !needremove( subset[10][i], ch )
	  && !blocking_list.is_empty( ) ) {
	rehash( ch, blocking_list );
	for( int j = 0; j < blocking_list; ++j ) {
	  if( blocking_list[j]->Shown( ) > subset[10][i]->Shown( ) ) {
	    blocking_list[j]->Show( subset[10][i]->Shown( ) );
	  }
	}
	fpage( ch, "You must remove %s before you can equip %s.",
	       list_name( ch, &blocking_list, "or", false ).c_str( ), subset[10][i] );
      } else {
	fpage( ch, "You need to remove something before you can equip %s.",
	       subset[10][i] );
      }
    }
  }
}


void put_on( char_data* ch, thing_array* array, char_data* victim )
{
  sort_by_layer( *array );

  thing_array subset [ 12 ];
  thing_func *func [ 12 ] = { cursed, 0,
			      cantwear, wrongsize,
			      wearstolen, // bladed, dishonorable are player-only
			      hiding, sneaking,
			      levellimit, antiobj,
			      waterlight, needremove, 0 };

  sort_objects( ch, *array, 0, 2, subset, func );
  delete array;

  thing_array worn;
  sort_objects( victim, subset[1], (thing_data*) &worn, 10, subset+2, func+2 );
  
  for( int i = 0; i < worn; ++i ) {
    obj_data *obj = object( worn[i] );

    // Temporarily recalc light, so room can see messages.
    // Done again in equip().
    if( obj->Light() != 0 ) {
      int max_light = obj->Light();
      for( int i = 0; i < victim->wearing; ++i ) {
	int l = victim->wearing[i]->Light();
	if( abs( l ) > abs( max_light ) ) {
	  max_light = l;
	}
      }
      if( max_light != victim->wearing.light ) {
	victim->wearing.light = max_light;
	if( victim->in_room ) {
	  victim->in_room->recalc_light();
	}
      }
    }

    // Message to char.
    fsend( ch, "You place %s on %s.", obj->Seen_Name( ch ), victim );
    fsend( victim, "%s places %s on you.", ch, obj->Seen_Name( victim ) );

    // Message to room.
    if( ch->array ) {
      msg_type = MSG_EQUIP;

      for( int j = 0; j < *ch->array; ++j ) {
	char_data *rch;
	if( !( rch = character( ch->array->list[j] ) )
	    || rch == ch
	    || rch == victim
	    || !rch->pcdata
	    || ( !ch->Seen( rch ) && !victim->Seen( rch ) )
	    || !rch->Accept_Msg( ch ) )
	  continue;

	if( worn > 1
	    && !is_set( rch->pcdata->message, MSG_MULTIPLE_ITEMS ) ) {
	  if( i == 0 ) {
	    fsend( rch, "%s places several items on %s.", ch, victim );
	  }
	} else {
	  fsend( rch, "%s places %s on %s.", ch, obj, victim );
	}
      }
    }

    obj->To( victim->wearing );
  }

  page_priv( ch, 0, empty_string );
  page_priv( ch, &subset[0], "can't let go of" );
  page_priv( ch, &subset[2], "can't wear", victim );
  page_priv( ch, &subset[3], "is the wrong size to wear", victim );
  page_priv( ch, &subset[4], "can't put", victim, "on" );
  page_priv( ch, &subset[5], "can't remain hidden and wear", victim );
  page_priv( ch, &subset[6], "can't keep sneaking and wear", victim );
  page_priv( ch, &subset[7], "can't cope with", victim );
  page_priv( ch, &subset[8], "shies away from", victim );
  page_priv( ch, &subset[9], "can't put", victim, "on", "in the water" );

  rehash( ch, subset[10] );
  for( int i = 0; i < subset[10]; ++i ) {
    if( subset[10][i]->Shown( ) ) {
      if( !needremove( subset[10][i], victim )
	  && !blocking_list.is_empty( ) ) {
	rehash( ch, blocking_list );
	for( int j = 0; j < blocking_list; ++j ) {
	  if( blocking_list[j]->Shown( ) > subset[10][i]->Shown( ) ) {
	    blocking_list[j]->Show( subset[10][i]->Shown( ) );
	  }
	}
	fpage( ch, "You must remove %s from %s before you can put %s on %s.",
	       list_name( ch, &blocking_list, "or", false ).c_str( ), victim, subset[10][i], victim->Him_Her( ) );
      } else {
	fpage( ch, "You need to remove something from %s before you can put %s on %s.",
	       victim, subset[10][i], victim->Him_Her( ) );
      }
    }
  }

  standard_delay( ch, worn );
}


void equip( char_data* ch, obj_data* obj, bool in_place )
{
  if( obj->Light() != 0 ) {
    int max_light = obj->Light();
    for( int i = 0; i < ch->wearing; ++i ) {
      const int l = ch->wearing[i]->Light();
      if( abs( l ) > abs( max_light ) ) {
	max_light = l;
      }
    }
    if( max_light != ch->wearing.light ) {
      ch->wearing.light = max_light;
      if( ch->in_room ) {
	ch->in_room->recalc_light();
      }
    }
  }

  if( in_place )
    return;

  if( is_submerged( ch )
      && !ch->can_hold_light()
      && strip_affect( obj, AFF_FLAMING ) ) {
    fsend( ch, "The flames around %s are extinguished by the water.", obj );
    fsend_seen( ch, "The flames around %s are extinguished by the water.", obj );
  }

  // Uses obj class on purpose.
  if( !is_set( obj->pIndexData->extra_flags, OFLAG_ADDITIVE ) ) {
    for( int i = 0; i < ch->wearing; ++i ) {
      obj_data *worn;
      if( ( worn = object( ch->wearing[i] ) )
	  && worn->pIndexData == obj->pIndexData 
	  && worn != obj ) {
        return;
      }
    }
  }
  
  if( ( obj->position == WEAR_HELD_L || obj->position == WEAR_HELD_R )
      && is_set( obj->extra_flags, OFLAG_TWO_HAND ) )
    set_bit( ch->status, STAT_TWO_HAND );
  
  // AFF_*.
  // Aff.Char affects.
  for( int i = 0; i < table_max[ TABLE_AFF_CHAR ]; ++i ) {
    affect_data af;
    if( is_set( obj->pIndexData->affect_flags, i ) ) {
      // Remove conflicting temps and leeches.
      // This should find its way into a gen'l-purpose function someday.
      if( i == AFF_FIRE_SHIELD ) {
	strip_affect( ch, AFF_ICE_SHIELD );
      } else if( i == AFF_ICE_SHIELD ) {
	strip_affect( ch, AFF_FIRE_SHIELD );
      }
      af.type = i;
      af.level = 7;
      modify_affect( ch, &af, true );
    }
  }
  
  // AFF_NONE.
  // Affects like "+3 con", etc.
  for( int i = 0; i < obj->pIndexData->affected; ++i ) {
    modify_affect( ch, obj->pIndexData->affected[i], true );
  }

  // Don't do wear triggers at startup, or when logging on.
  if( ch->array ) {
    if( oprog_data *oprog = wear_trigger( obj->pIndexData ) ) {
      push( );
      clear_variables( );
      var_ch = ch;
      var_obj = obj;
      var_room = ch->in_room;
      oprog->execute( );
      pop( );
      if( !obj->Is_Valid() ) {
	unequip( ch, obj, false );
      	return;
      }
    }
  }
}


/*
 *   REMOVE ROUTINES
 */


static int removed_weight;
static int removed_num;


/*
static thing_data *remove_heavy( thing_data* thing, char_data* ch, thing_data* )
{
  // *** FIX ME: check if item is -str.
  // This isn't fair if someone has a (-str) item;

  // Multiply everything by 2 instead of worrying about
  // integer division.
  int n = thing->Weight( 1 );
  int m = 2*( ch->Empty_Capacity( ) - ch->contents.weight - removed_weight )
    - ch->wearing.weight;

  if( m >= n || n <= 0 ) {
    return thing;
  }

  return 0;
}
*/


static thing_data *remove_from_heavy( thing_data* thing, char_data* ch, thing_data* )
{
  // *** FIX ME: check if item is -str.
  // This isn't fair if someone has a (-str) item;

  // Moving from someone else's worn to ch's inventory.
  // Multiply everything by 2 instead of worrying about
  // integer division.
  int n = thing->Weight( 1 );
  int m = 2*( ch->Empty_Capacity( ) - ch->contents.weight - removed_weight )
    - ch->wearing.weight;
  
  if( m >= 2*n || n <= 0 ) {
    return thing;
  }

  return 0;
}


static thing_data *remove_many( thing_data* thing, char_data* ch, thing_data* )
{
  // *** FIX ME: check if item is -dex.
  // This isn't fair if someone has a (-dex) item;

  int m = ch->can_carry_n( )
    - ch->contents.number
    - removed_num;

  if( m > 0 ) {
    return thing;
  }

  return 0;
}


thing_data* noremove( thing_data* thing, char_data *ch, thing_data* )
{
  obj_data* obj = (obj_data*) thing;

  return obj->removable( ch ) ? obj : 0;
}


/*
thing_data* nocatch( thing_data* thing, char_data*, thing_data* )
{
  obj_data* obj = (obj_data*) thing;

  return( obj->position == WEAR_FLOATING ? 0 : obj );
}
*/


static obj_data *blocking;


static thing_data *underneath( thing_data* thing, char_data* ch, thing_data *t2 )
{
  obj_data*       obj  = (obj_data*) thing;
  thing_array *removed = (thing_array*)t2;

  const int pos = obj->position;
  for( int i = obj->layer + 1; i < MAX_LAYER; ++i ) {
    if( obj_data *worn = ch->Wearing( pos, i ) ) {
      if( !worn->removable( ch ) ) {
	blocking = worn;
	return 0;
      }
      if( !is_set( worn->extra_flags, OFLAG_PASS_THROUGH )
	  && !removed->includes( worn ) ) {
	for( oprog_data *oprog = worn->pIndexData->oprog; oprog; oprog = oprog->next ) {
	  if( oprog->trigger == OPROG_TRIGGER_WEAR
	      || oprog->trigger == OPROG_TRIGGER_REMOVE ) {
	    blocking = worn;
	    return 0;
	  }
	}
      }
    }
  }
  
  return obj;
}


static thing_data *remove( thing_data* thing, char_data* ch, thing_data* )
{
  removed_weight += thing->Weight( 1 );
  ++removed_num;

  return thing;
}


static oprog_data *remove_trigger( obj_clss_data *clss )
{
  for( oprog_data *oprog = clss->oprog; oprog; oprog = oprog->next ) {
    if( oprog->trigger == OPROG_TRIGGER_REMOVE ) {
      return oprog;
    }
  }
  
  return 0;
}


void do_remove( char_data* ch, const char *argument )
{
  char            arg  [ MAX_INPUT_LENGTH ];
  thing_array*  array;

  if( two_argument( argument, "from", arg ) ) {
    if( is_confused_pet( ch ) ) 
      return;

    char_data *victim;
    if( !( victim = one_character( ch, argument, "remove", ch->array ) ) )
      return;
    if( !is_set( victim->status, STAT_PET )
	|| victim->leader != ch ) {
      fsend( ch, "%s is not your pet.", victim );
      return;
    }
    if( !( array = several_things( ch, arg, "remove", &victim->wearing ) ) )
      return;
    
    sort_by_layer( *array, true );
    
    removed_weight = 0;
    removed_num = 0;

    thing_array subset [6];
    thing_func *func [6] = { remove_from_heavy, remove_many, 0,
			     noremove, underneath, remove };
    
    sort_objects( ch, *array, 0, 3, subset, func );
    delete array;

    sort_objects( victim, subset[2], (thing_data*)&subset[5], 3, subset+3, func+3 );
    
    page_priv( ch, 0, empty_string );
    page_priv( ch, &subset[0], "can't lift" );
    page_priv( ch, &subset[1], "can't handle" );
    page_priv( ch, &subset[3], "can't remove", victim, "from" );

    rehash( ch, subset[4] );
    for( int i = 0; i < subset[4]; ++i ) {
      if( subset[4][i]->Shown( ) ) {
	if( !underneath( subset[4][i], victim, (thing_data*)&subset[5] ) ) {
	  fpage( ch, "You must remove %s from %s before you can remove %s.",
		 blocking, victim, subset[3][i] );
	} else {
	  fpage( ch, "You need to remove something from %s before you can remove %s.",
		 victim, subset[4][i] );
	}
      }
    }

    for( int i = 0; i < subset[5]; ++i ) {
      obj_data *obj = object( subset[5][i] );

      // Message to char and vict.
      fsend( ch, "You remove %s from %s.", obj->Seen_Name( ch ), victim );
      fsend( victim, "%s removes your %s.", ch, obj->Seen_Name( victim ) );
      
      // Message to room.
      if( ch->array ) {
	msg_type = MSG_EQUIP;

	for( int j = 0; j < *ch->array; ++j ) {
	  char_data *rch;
	  if( !( rch = character( ch->array->list[j] ) )
	      || rch == ch
	      || rch == victim
	      || !rch->pcdata
	      || ( !ch->Seen( rch ) && !victim->Seen( rch ) )
	      || !rch->Accept_Msg( ch ) )
	    continue;

	  if( subset[5] > 1
	      && !is_set( rch->pcdata->message, MSG_MULTIPLE_ITEMS ) ) {
	    if( i == 0 ) {
	      fsend( rch, "%s removes several items from %s.", ch, victim );
	    }
	  } else {
	    fsend( rch, "%s removes %s from %s.", ch, obj, victim );
	  }
	}
      }

      obj = (obj_data*) obj->From( 1 );
      // Remove trigger might have moved obj someplace else.
      if( obj->Is_Valid( ) && !obj->array ) {
	obj->To( ch );
      }
    }

    standard_delay( ch, subset[5] );

    return;
  }
  
  if( !ch->can_carry( ) ) {
    if( !is_confused_pet( ch ) )
      send( ch, "You are unable to carry items.\n\r" );
    return;
  }

  if( !( array = several_things( ch, argument, "remove", &ch->wearing ) ) )
    return;

  // Remove higher layers first.
  sort_by_layer( *array, true );

  removed_weight = 0;
  removed_num = 0;

  thing_array subset [4];
  thing_func *func [4] = { /*remove_heavy,*/ remove_many,
			   noremove, underneath, remove };
  
  sort_objects( ch, *array, (thing_data*)&subset[3], 4, subset, func );

  page_priv( ch, 0, empty_string );
  //  page_priv( ch, &subset[0], "can't carry" );
  page_priv( ch, &subset[0], "can't handle" );
  page_priv( ch, &subset[1], "can't remove" );
  
  for( int i = 0; i < subset[2]; ++i ) {
    if( !underneath( subset[2][i], ch, (thing_data*)&subset[3] ) ) {
      fpage( ch, "You must remove %s before you can remove %s.",
	     blocking, subset[2][i] );
    } else {
      fpage( ch, "You need to remove something before you can remove %s.",
	     subset[2][i] );
    }
  }

  for( int i = 0; i < subset[3]; ++i ) {
    obj_data *obj = object( subset[3][i] );

    oprog_data *oprog = remove_trigger( obj->pIndexData );

    // Message to char.
    if( remo_verb[obj->position] ) {
      if( const extra_data *ed = oprog ? oprog->find_extra( "to_char" ) : 0 ) {
	act( ch, ed->text, 0, obj, wear_loc_name[obj->position] );
      } else {
	if( is_set( obj->extra_flags, OFLAG_TWO_HAND ) ) {
	  fsend( ch, "You remove %s from your hands.", obj->Seen_Name( ch ) );
	} else {
	  fsend( ch, remo_verb[obj->position], obj->Seen_Name( ch ) );
	}
      }
    } else {
      bug( "Remove: bad position %d on obj %s.", obj->position, obj );
    }

    // Message to room.
    if( ch->array ) {
      msg_type = MSG_EQUIP;

      for( int j = 0; j < *ch->array; ++j ) {
	char_data *rch;
	if( !( rch = character( ch->array->list[j] ) )
	    || rch == ch
	    || !rch->pcdata
	    || !ch->Seen( rch )  
	    || !rch->Accept_Msg( ch ) )
	  continue;

	if( subset[3] > 1
	    && !is_set( rch->pcdata->message, MSG_MULTIPLE_ITEMS ) ) {
	  if( i == 0 ) {
	    fsend( rch, "%s removes several items.", ch );
	  }
	} else if( remo_publ[obj->position] ) {
	  if( const extra_data *ed = oprog ? oprog->find_extra( "to_room" ) : 0 ) {
	    act( rch, ed->text, ch, obj, wear_loc_name[obj->position] );
	  } else {
	    if( is_set( obj->extra_flags, OFLAG_TWO_HAND ) ) {
	      fsend( rch, "%s removes %s from %s hands.", ch, obj, ch->His_Her(rch) );
	    } else {
	      fsend( rch, remo_publ[obj->position], ch, obj, ch->His_Her(rch) );
	    }
	  }
	}
      }
    }

    obj = (obj_data*) obj->From( 1 );
    // Remove trigger might have moved obj someplace else.
    if( obj->Is_Valid( ) && !obj->array ) {
      obj->To( ch );
    }
  }

  delete array;

  //  standard_delay( ch, subset[3] );
}


void unequip( char_data* ch, obj_data* obj, bool in_place )
{
  obj_data*    worn;
  affect_data    af;

  if( obj->Light() != 0 ) {
    int max_light = 0;
    for( int i = 0; i < ch->wearing; ++i ) {
      const int l = ch->wearing[i]->Light();
      if( abs( l ) > abs( max_light ) ) {
	max_light = l;
      }
    }
    if( max_light != ch->wearing.light ) {
      ch->wearing.light = max_light;
      if( ch->in_room ) {
	ch->in_room->recalc_light();
      }
    }
  }

  if( in_place )
    return;

  if( is_set( ch->status, STAT_GARROTING )
      && obj->pIndexData->item_type == ITEM_GARROTE
      && obj->position == WEAR_HELD_R
      && obj->layer == LAYER_BASE ) {
    // Decided to stop throttling the poor fool.
    char_data *victim = ch->fighting;
    if( victim ) {
      affect_data af;
      af.type = AFF_CHOKING;
      modify_affect( victim, &af, false );
    }
    remove_bit( ch->status, STAT_GARROTING );
  }

  // Uses obj class on purpose.
  if( !is_set( obj->pIndexData->extra_flags, OFLAG_ADDITIVE ) )
    for( int i = 0; i < ch->wearing; i++ )
      if( ( worn = object( ch->wearing[i] ) )
	  && worn->pIndexData == obj->pIndexData 
	  && worn != obj )
        return;
  
  if( ( obj->position == WEAR_HELD_L || obj->position == WEAR_HELD_R )
      && is_set( ch->status, STAT_TWO_HAND ) )
    remove_bit( ch->status, STAT_TWO_HAND );
  
  for( int i = 0; i < table_max[ TABLE_AFF_CHAR ]; i++ )
    if( is_set( obj->pIndexData->affect_flags, i ) ) {
      af.type = i;
      af.level = 7;
      modify_affect( ch, &af, false );
    }

  // AFF_NONE.
  for( int i = 0; i < obj->pIndexData->affected; i++ )
    modify_affect( ch, obj->pIndexData->affected[i], false );

  // Don't do remove triggers on logout.
  if( ch->array ) {
    if( oprog_data *oprog = remove_trigger( obj->pIndexData ) ) {
      push( );
      clear_variables( );
      var_ch = ch;
      var_obj = obj;
      var_room = ch->in_room;
      oprog->execute( );
      pop( );
      //      if( !obj->Is_Valid() )
      //	return;
    }
  }
}
