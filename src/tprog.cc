#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include "define.h"
#include "struct.h"


Tprog_Data::Tprog_Data( int t, int e )
  : table(t), entry(e)
{
  record_new( sizeof( Tprog_Data ), -MEM_TABLE );
}


Tprog_Data::~Tprog_Data( )
{
  record_delete( sizeof( Tprog_Data ), -MEM_TABLE );
}


int Tprog_Data::execute( thing_data *owner )
{
  return program_data::execute( owner );
}


void Tprog_Data :: display( char_data* ch ) const
{
  page( ch, "Table %-25s Entry %s\n\r",
	table_name( table ), entry_name( table, entry ) );
}     


void do_tcode( char_data *ch, const char *argument )
{
  wizard_data *imm;
  if( !( imm = wizard( ch ) ) )
    return;

  const int table = imm->table_edit[0];
  const int entry = imm->table_edit[1];

  if( table == -1 ) {
    send( ch, "You are not editing any table.\n\r" );
    return;
  }

  Table_Data *t = table_addr( table, entry );
  Tprog_Data **p = t->program( );

  if( !p ) {
    send( ch, "Table %s does not have programs.\n\r", table_name( table ) );
    return;
  }

  // In case table is locked...
  if( *argument && !edit_table( ch, table ) )
    return;

  if( !*p ) {
    *p = new Tprog_Data( table, entry );
  }

  (*p)->Edit_Code( ch, argument );

  if( *argument || !(*p)->binary ) {
    var_ch = ch;
    (*p)->compile( );
  }

  if( (*p)->Code( ) == empty_string
      && (*p)->Extra_Descr( ).is_empty( ) ) {
    delete *p;
    *p = 0;
  }
}


void do_tdata( char_data *ch, const char *argument )
{
  wizard_data *imm;
  if( !( imm = wizard( ch ) ) )
    return;

  const int table = imm->table_edit[0];
  const int entry = imm->table_edit[1];

  if( table == -1 ) {
    send( ch, "You are not editing any table.\n\r" );
    return;
  }

  Table_Data *t = table_addr( table, entry );
  Tprog_Data **p = t->program( );
    
  if( imm->textra_edit ) {
    if( exact_match( argument, "exit" ) ) {
      imm->textra_edit = 0;
      send( ch, "Tdata now operates on the data list.\n\r" );
      return;
    }

    imm->textra_edit->text
      = edit_string( ch, argument, imm->textra_edit->text, MEM_EXTRA, true );
    
  } else {
    if( !p ) {
      send( ch, "Table %s does not have programs.\n\r", table_name( table ) );
      return;
    }
    
    // In case table is locked...
    if( *argument && !edit_table( ch, table ) )
      return;
    
    if( !*p ) {
      *p = new Tprog_Data( table, entry );
    }

    edit_extra( (*p)->Extra_Descr( ), imm, offset( &imm->textra_edit, imm ),
		argument, "tdata" );
  }

    
  if( *argument || !(*p)->binary ) {
    var_ch = ch;
    (*p)->compile( );
  }
  
  if( (*p)->Code( ) == empty_string
      && (*p)->Extra_Descr( ).is_empty( ) ) {
    delete *p;
    *p = 0;
    imm->textra_edit = 0;
  }
}
