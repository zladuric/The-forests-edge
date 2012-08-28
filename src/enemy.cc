#include <ctype.h>
#include <sys/types.h>
#include <stdio.h>
#include <syslog.h>
#include "define.h"
#include "struct.h"


/*
 *   ENEMY_DATA CLASS
 */


Enemy_Data :: Enemy_Data( )
  : damage(0)
{
  record_new( sizeof( enemy_data ), MEM_ENEMY );
}


Enemy_Data :: ~Enemy_Data( )
{
  record_delete( sizeof( enemy_data ), MEM_ENEMY );
}


/*
 *   GENERIC ROUTINES
 */


bool is_enemy( char_data* ch, char_data* victim )
{
  // Note: this includes switched mobs for now.
  // Not sure if that's right.
  if( !victim->pcdata )
    return false;

  for( const enemy_data *enemy = ch->enemy; enemy; enemy = enemy->next )
    if( enemy->pfile == victim->pcdata->pfile )
      return true;

  return false;
}


int damage_done( char_data* ch, char_data* victim )
{
  for( const enemy_data *enemy = victim->enemy; enemy; enemy = enemy->next )
    if( enemy->pfile == ch->pcdata->pfile )
      return enemy->damage;
  
  return -1;
}


/*  
 *   DELETION ROUTINES
 */


void clear_enemies( char_data* victim )
{
  if( !victim->pcdata )
    return;

  for( int i = 0; i < mob_list; ++i ) {
    mob_data *ch = mob_list[i];
    if( !ch->Is_Valid( ) )
      continue;
    for( enemy_data *enemy = ch->enemy; enemy; enemy = enemy->next ) {
      if( enemy->pfile == victim->pcdata->pfile ) {
        remove( ch->enemy, enemy );
	delete enemy;
        break;
      }
    }
  }
}


/*
 *   CREATION ROUTINES
 */


void share_enemies( char_data* ch1, char_data* ch2 )
{
  if( !is_set( ch2->species->act_flags, ACT_ASSIST_GROUP ) )
    return; 
  
  for( enemy_data *e1 = ch1->enemy; e1; e1 = e1->next ) {
    for( enemy_data *e2 = ch2->enemy; ; e2 = e2->next ) {
      if( !e2 ) {
        e2 = new enemy_data;
        e2->pfile = e1->pfile;
        e2->next = ch2->enemy;
        ch2->enemy = e2;
        break;
      }
      if( e2->pfile == e1->pfile )
        break;
    }
  }
}


void record_damage( char_data* victim, char_data* ch, int damage )
{
  if( !victim
      || !victim->species
      || !ch
      || !ch->pcdata )
    return;
  
  for( enemy_data *enemy = victim->enemy; enemy; enemy = enemy->next ) {
    if( enemy->pfile == ch->pcdata->pfile ) {
      enemy->damage += damage;
      return;
    }
  }
  
  enemy_data *enemy = new enemy_data;
  enemy->pfile = ch->pcdata->pfile;
  enemy->damage = damage;
  enemy->next = victim->enemy;
  victim->enemy = enemy;
}
