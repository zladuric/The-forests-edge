#include <stdio.h>
#include "define.h"
#include "struct.h"
#include "dictionary.h"


string_array dictionary;
const char **custom = 0;
int max_custom = 0;
bool dict_modified = false;
bool cust_modified = false;


void save_dictionary( )
{
  if( dict_modified ) {
    rename_file( FILES_DIR, DICTIONARY_FILE,
		 FILES_PREV_DIR, DICTIONARY_FILE );
    
    if( FILE *dict = open_file( FILES_DIR, DICTIONARY_FILE, "w" ) ) {
      fprintf( dict, "#DICTIONARY\n\n" );
      
      for( int i = 0; i < dictionary; ++i ) {
	fwrite_string( dict, dictionary[i] );
      }
      
      fwrite_string( dict, "$" );
      fclose( dict );

      dict_modified = false;
    }
  }

  if( cust_modified ) {
    rename_file( FILES_DIR, CUSTOM_FILE,
		 FILES_PREV_DIR, CUSTOM_FILE );
    
    if( FILE *dict = open_file( FILES_DIR, CUSTOM_FILE, "w" ) ) {
      fprintf( dict, "#DICTIONARY\n\n" );
      
      for( int i = 0; i < max_custom; ++i ) {
	fwrite_string( dict, custom[i] );
      }
      
      fwrite_string( dict, "$" );
      fclose( dict );

      cust_modified = false;
    }
  }
}


void load_dictionary( )
{
  echo( "Loading System Dictionary ..." );

  FILE *dict = open_file( FILES_DIR, DICTIONARY_FILE, "r", true );
  
  if( strcmp( fread_word( dict ), "#DICTIONARY" ) )
    panic( "Load_dictionary: missing header" );

  //  char buf[ ONE_LINE ];
  unsigned line = 0;
  
  while( true ) {
    const char *const word = fread_string( dict, MEM_DICTIONARY );
    if( *word == '$' ) {
      free_string( word, MEM_DICTIONARY );
      break;
    }
    record_new( sizeof( char* ), MEM_DICTIONARY );
    dictionary.append( word );
    ++line;
  }
  
  fclose( dict );

  echo( " %u words.\n\r", line );

  // Make sure dictionary file's alphabetization matches ours.
  echo( "Sorting ..." );
  unsigned swaps = 0;
  unsigned discards = 0;
  int j = dictionary.size;
  for( int n = 0; n < j; ++n ) {
    bool done = true;
    for( int k = 0; k < j-1-n; ++k ) {
      const int cmp = strcasecmp( dictionary[k], dictionary[k+1] );
      if( cmp > 0 ) {
	++swaps;
	swap( dictionary[k], dictionary[k+1] );
	done = false;
      } else if( cmp == 0 ) {
	if( !strcmp( dictionary[k]+1, dictionary[k+1]+1 ) ) {
	  // Words are exact duplicates, or
	  // only difference is capitalization of first letter.
	  // No reason to have both in dictionary.
	  if( isupper( *dictionary[k] ) ) {
	    free_string( dictionary[k], MEM_DICTIONARY );
	    dictionary.remove( k );
	  } else {
	    free_string( dictionary[k+1], MEM_DICTIONARY );
	    dictionary.remove( k+1 );
	  }
	  record_delete( sizeof( char* ), MEM_DICTIONARY );
	  --j;
	  --k;
	  ++discards;
	} else if( strcmp( dictionary[k], dictionary[k+1] ) > 0 ) {
	  // Make sure capitalization differences appear in order.
	  // e.g. "start" vs. "START".
	  ++swaps;
	  swap( dictionary[k], dictionary[k+1] );
	  done = false;
	}
      }
    }
    if( done )
      break;
  }
  
  echo( " %u discards,", discards );
  echo( " %u swaps.\n\r", swaps );

  if( swaps != 0 || discards != 0 ) {
    echo( "Saving System Dictionary ...\n\r" );
    save_dictionary( );
  }
  
  echo( "Loading Custom Dictionary ..." );

  line = 0;
  FILE *fp = open_file( FILES_DIR, CUSTOM_FILE, "r", true );
  
  if( strcmp( fread_word( fp ), "#DICTIONARY" ) )
    panic( "Load_dictionary: missing header" );

  while( true ) {
    const char *const name = fread_string( fp, MEM_DICTIONARY );
    if( *name == '$' ) {
      free_string( name, MEM_DICTIONARY );
      break;
    }
    record_new( sizeof( char* ), MEM_DICTIONARY );
    insert( custom, max_custom, name, max_custom );
    ++line;
  }

  fclose( fp );

  echo( " %u words.\n\r", line );
}


static const char *spell_word( const char *word, char_data *ch )
{
  static char line [ MAX_INPUT_LENGTH ];

  // 1. Search system dictionary.
  int pos = pntr_search( dictionary.list, dictionary.size, word );

  if( pos >= 0 ) {
    while( pos > 0 && !strcasecmp( dictionary[pos-1], word ) )
      --pos;
    while( isupper( *dictionary[pos] ) && islower( *word ) ) {
      ++pos;
      if( pos == dictionary.size || strcasecmp( dictionary[pos], word ) )
	return dictionary[pos-1];
    }
    return dictionary[pos];
  }

  // 2. Search TFE dictionary.
  pos = pntr_search( custom, max_custom, word );

  if( pos >= 0 ) {
    while( pos > 0 && !strcasecmp( custom[pos-1], word ) )
      --pos;
    while( isupper( *custom[pos] ) && islower( *word ) ) {
      ++pos;
      if( pos == max_custom || strcasecmp( custom[pos], word ) )
	return custom[pos-1];
    }
    return custom[pos];
  }

  // 3. Search player's custom dictionary.
  if( ch && ch->pcdata ) {
    const char *input = ch->pcdata->dictionary;

    while( true ) {
      input = one_line( input, line );
      if( !*line )
	break;
      char *s = line;
      while( true ) {
	while( *s && !isalpha( *s ) ) {
	  ++s;
	}
	if( !*s )
	  break;
	const char *t = s;
	while( *s &&
	       ( isalpha( *s ) ||
		 ( *s == '\'' || *s == '-' ) && isalpha( *(s+1) ) ) ) {
	  ++s;
	}
	char term = *s;
	*s = '\0';
	if( !strcasecmp( t, word ) )
	  return s;
	*s = term;
      }
    }
  }

  // 4. Tables containing miscellaneous nouns.
  const int noun_tables[] = {
    TABLE_SKILL_LANGUAGE,
    -1
  };

  for( int i = 0; noun_tables[i] > 0; ++i ) {
    const int table = noun_tables[i];
    for( int j = 0; j < table_max[table]; ++j ) {
      const char *name = entry_name( table, j );
      if( !strcasecmp( name, word ) )
	return name;
    }
  }

  // 5. Tables containing only proper nouns.
  const int proper_tables[] = {
    TABLE_DAYS, TABLE_MONTHS, TABLE_NATION, TABLE_RELIGION, TABLE_TOWN,
    -1
  };

  for( int i = 0; proper_tables[i] > 0; ++i ) {
    const int table = proper_tables[i];
    for( int j = 0; j < table_max[table]; ++j ) {
      const char *const name = entry_name( table, j );
      if( !strcasecmp( name, word ) )
	return name;
      int l = strlen( name );
      if( !strncasecmp( word, name, l )
	  && !strcasecmp( word+l, "'s" ) ) {
	snprintf( line, MAX_INPUT_LENGTH, "%s's", name );
	return line;
      }
    }
  }

  return 0;
}


char *spell( char_data *ch, const char *input, const char *header )
{
  static char output [ 3*MAX_STRING_LENGTH ];
  unsigned length = 0;

  const char *line = input;
  unsigned num = 1;
  bool any = false;

  char errs [ MAX_INPUT_LENGTH ];
  char *e = errs;
  bool err = false;

  char word [ MAX_INPUT_LENGTH ];

  int hl = 0;
  if( header ) {
    hl = strlen( header );
  }

  while( true ) {

    if( *input == '\n' ) {
      if( err ) {
	*e = '\0';
	if( !header ) {
	  length += sprintf( output+length, "[%2d]  %.*s\n\r", num, input-line, line );
	  length += sprintf( output+length, "      %s\n\r", errs );
	} else {
	  length += sprintf( output+length, "%s%.*s\n\r", header, input-line, line );
	  length += sprintf( output+length, "%*s%s\n\r", hl, "", errs );
	}
	err = false;
      }
      e = errs;
      ++input;
      if( *input == '\r' )
	++input;
      line = input;
      num += 2;
    }

    while( *input && *input != '\n' && !isalpha( *input ) ) {
      if( *input == '@' ) {
	*e++ = ' ';
	if( !*++input )
	  break;
      }
      *e++ = ' ';
      ++input;
    }

    if( !*input ) {
      break;
    }

    if( *input == '\n' )
      continue;

    // Delineate a single word.

    const char *s = input;
    char *w = word;
    bool period = false;
    bool hyphen = false;
    
    while( true ) {
      if( isalpha( *s ) ) {
	// Letter.
	*w++ = *s++;
	continue;
      }
      if( *s == '\'' ) {
	if( isalpha( *(s+1) ) ) {
	  // Letter-apostrophe-letter.
	  *w++ = *s++;
	  continue;
	}
	break;
      }
      if( *s == '.' ) {
	if( hyphen )
	  break;
	// Letter-period.
	*w++ = *s++;
	if( isalpha( *s ) ) {
	  // Letter-period-letter.
	  period = true;
	  continue;
	}
	break;
      }
      if( *s == '-' ) {
	if( period )
	  break;
	// Hyphenated words can span lines.
	while( iscntrl( *++s ) );
	if( isalpha( *s ) ) {
	  // Letter-dash-letter.
	  *w++ = '-';
	  hyphen = true;
	  continue;
	}
	break;
      }
      break;
    }
    
    *w = '\0';
    int len = w - word;
    int skip = 0;

    // Check word spelling.

    const char *dict = spell_word( word, ch );
    bool bad = true;
    bool check_cap = false;

    if( dict ) {
      if( isupper( *dict ) ) {
	check_cap = true;
      }
      bad = false;
    }

    if( bad && *(word+len-1) == '.' && !period ) {
      // Period after word?
      // Try re-spelling without period.
      --len;
      *(word+len) = '\0';
      if( ( dict = spell_word( word, ch ) ) ) {
	if( isupper( *dict ) ) {
	  check_cap = true;
	}
	bad = false;
      }
    }
    
    if( bad && len > 2 && *(word+len-1) == 's' && *(word+len-2) == '\'' ) {
      // Possessive?
      // Try re-spelling without 's.
      len -= 2;
      skip = 2;
      *(word+len) = '\0';
      if( ( dict = spell_word( word, ch ) ) ) {
	if( isupper( *dict ) ) {
	  check_cap = true;
	}
	bad = false;
      }
    }
    
    if( bad && hyphen ) {
      // Hyphenated word.
      // Try re-spelling first segment only.
      skip = 0;
      for( len = 0; *(word+len) != '-'; ++len );
      *(word+len) = '\0';
      if( ( dict = spell_word( word, ch ) ) ) {
	if( isupper( *dict ) ) {
	  check_cap = true;
	}
	bad = false;
      }
      
      if( bad && *(word+len-1) == '.' && !period ) {
	// Period after word?
	// Try re-spelling without period.
	--len;
	*(word+len) = '\0';
	if( ( dict = spell_word( word, ch ) ) ) {
	  if( isupper( *dict ) ) {
	    check_cap = true;
	  }
	  bad = false;
	}
      }
      
      if( bad && len > 2 && *(word+len-1) == 's' && *(word+len-2) == '\'' ) {
	// Possessive?
	// Try re-spelling without 's.
	len -= 2;
	skip = 2;
	*(word+len) = '\0';
	if( ( dict = spell_word( word, ch ) ) ) {
	  if( isupper( *dict ) ) {
	    check_cap = true;
	  }
	  bad = false;
	}
      }
    }

    bool start = true;

    while( len > 0 ) {
      if( isprint( *input ) ) {
	if( bad ) {
	  if( len == 1 || start ) {
	    *e++ = '^';
	  } else {
	    *e++ = '-';
	  }
	  err = true;
	  any = true;
	} else if( check_cap && *dict != *input ) {
	  *e++ = '^';
	  err = true;
	  any = true;
	} else {
	  *e++ = ' ';
	}
	start = false;
	++dict;
	++input;
	--len;
      } else if( *input == '\n' ) {
	if( err ) {
	  *e = '\0';
	  if( !header ) {
	    length += sprintf( output+length, "[%2d]  %.*s\n\r", num, input-line, line );
	    length += sprintf( output+length, "      %s\n\r", errs );
	  } else {
	    length += sprintf( output+length, "%s %.*s\n\r", header, input-line, line );
	    length += sprintf( output+length, "%*s %s\n\r", hl, "", errs );
	  }
	  err = false;
	}
	e = errs;
	++input;
	if( *input == '\r' )
	  ++input;
	line = input;
	num += 2;
      }
    }
    
    while( skip-- > 0 ) {
      *e++ = ' ';
      ++input;
    }
  }

  if( err ) {
    // Came across \0, but not just after \n\r.
    // Shouldn't happen.
    *e = '\0';
    if( !header ) {
      length += sprintf( output+length, "[%2d]  %s\n\r", num, line );
      length += sprintf( output+length, "      %s\n\r", errs );
    } else {
      length += sprintf( output+length, "%s %.*s\n\r", header, input-line, line );
      length += sprintf( output+length, "%*s %s\n\r", hl, "", errs );
    }
  }

  if( !any )
    return 0;

  *(output+length) = '\0';
  return output;
}


static bool check_dict_word( char_data *ch, const char *word )
{
  if( !isalpha( *word ) ) {
    send( ch, "Dictionary words must begin with a letter.\n\r" );
    return false;
  }

  for( const char *letter = word; *letter; ++letter ) {
    if( !isalpha( *letter )
	&& !( ( *letter == '-' || *letter == '\'' ) && isalpha( *(letter+1) ) )
	&& !( *letter == '.' && isalpha( *(letter-1) ) ) ) {
      fsend( ch, "The word \"%s\" contains improper punctuation.", word );
      return false;
    }
  }

  if( islower( *word ) ) {
    for( const char *letter = word+1; *letter; ++letter ) {
      if( !islower( *letter )
	  && !( ( *letter == '-' || *letter == '\'' ) && isalpha( *(letter+1) ) )
	  && !( *letter == '.' && isalpha( *(letter-1) ) ) ) {
	fsend( ch, "The word \"%s\" contains improper capitalization.", word );
	return false;
      }
    }
  }

  return true;
}


void do_dictionary( char_data *ch, const char *argument )
{
  if( not_player( ch ) )
    return;

  char arg [ MAX_INPUT_LENGTH ];
  char buf [ TWO_LINES ];

  if( is_builder( ch ) ) {
    int flags;
    if( !get_flags( ch, argument, &flags, "crpsa", "dictionary" ) ) {
      return;
    }

    if( is_set( flags, 0 ) ) {
      // Custom dictionary.
      if( !*argument ) {
	// List custom.
	if( !custom ) {
	  send( ch, "The custom dictionary is empty.\n\r" );
	} else {
	  display_array( ch, "Custom Dictionary",
			 &custom[0], &custom[1],
			 max_custom, false );
	  page( ch, "\n\r%d word%s.\n\r", max_custom,
		max_custom == 1 ? "" : "s" );
	}
	return;
      }

      if( is_set( flags, 1 ) ) {
	// Remove custom.
	while( *argument ) {
	  argument = one_argument( argument, arg );
	  int i = pntr_search( custom, max_custom, arg );
	  if( i < 0 ) {
	    fpage( ch, "The word \"%s\" isn't in the custom dictionary.", arg );
	  } else {
	    fpage( ch, "Word \"%s\" removed from custom dictionary.", custom[i] );
	    free_string( custom[i], MEM_DICTIONARY );
	    record_delete( sizeof( char* ), MEM_DICTIONARY );
	    remove( custom, max_custom, i );
	    cust_modified = true;
	  }
	}
	return;
      }

      if( is_set( flags, 4 ) ) {
	// Add custom.
	while( *argument ) {
	  argument = one_argument( argument, arg );
	  int i = pntr_search( custom, max_custom, arg );
	  if( i >= 0 ) {
	    fpage( ch, "The word \"%s\" is already in the custom dictionary.", custom[i] );
	  } else if( check_dict_word( ch, arg ) ) {
	    i = -i-1;
	    const char *const name = alloc_string( arg, MEM_DICTIONARY );
	    record_new( sizeof( char* ), MEM_DICTIONARY );
	    insert( custom, max_custom, name, i );
	    cust_modified = true;
	    fpage( ch, "The word \"%s\" has been added to the custom dictionary.", name );
	  }
	}
	return;
      }

      // Search custom.
      while( *argument ) {
	argument = one_argument( argument, arg );
	const int l = strlen( arg );
	int i = pntr_search( custom, max_custom, arg, l );
	if( i < 0 )
	  i = -i-1;
	int j = 0;
	char *b = buf;
	while( i < max_custom && !strncasecmp( custom[i], arg, l ) ) {
	  if( j == 0 ) {
	    page_title( ch, "Custom dictionary: %s", arg );
	  }
	  b += sprintf( b, "%19s", custom[i] );
	  if( j++ % 4 == 3 ) {
	    page( ch, buf );
	    page( ch, "\n\r" );
	    b = buf;
	  }
	  ++i;
	}
	if( j % 4 != 0 ) {
	  page( ch, buf );
	  page( ch, "\n\r" );
	}
	if( j == 0 ) {
	  fpage( ch, "No words matching \"%s\" found in custom dictionary.", arg );
	} else {
	  page( ch, "\n\r%d matching word%s found.\n\r",
		j, j == 1 ? "" : "s" );
	}
      }
      return;
    }

    if( is_set( flags, 3 ) ) {
      if( !has_permission( ch, PERM_MISC_TABLES ) ) {
	send( ch, "You don't have permission to modify the system dictionary.\n\r" );
	return;
      }
      
      if( !*argument ) {
	// List system.
	send( ch, "%d words in system dictionary.\n\r", dictionary.size );
	send( ch, "You must specify a word to look for.\n\r" );
	return;
      }
      
      if( is_set( flags, 1 ) ) {
	// Remove system.
	while( *argument ) {
	  argument = one_argument( argument, arg );
	  int i = pntr_search( dictionary.list, dictionary.size, arg );
	  if( i < 0 ) {
	    fsend( ch, "The word \"%s\" isn't in the system dictionary.", arg );
	  } else {
	    fsend( ch, "Word \"%s\" removed from system dictionary.", dictionary[i] );
	    free_string( dictionary[i], MEM_DICTIONARY );
	    record_delete( sizeof( char* ), MEM_DICTIONARY );
	    dictionary.remove( i );
	    dict_modified = true;
	  }
	}
	return;
      }

      if( is_set( flags, 4 ) ) {
	// Add system.
	while( *argument ) {
	  argument = one_argument( argument, arg );
	  int i = pntr_search( dictionary.list, dictionary.size, arg );
	  if( i >= 0 ) {
	    fsend( ch, "The word \"%s\" is already in the system dictionary.", dictionary[i] );
	  } else if( check_dict_word( ch, arg ) ) {
	    i = -i-1;
	    const char *const name = alloc_string( arg, MEM_DICTIONARY );
	    record_new( sizeof( char* ), MEM_DICTIONARY );
	    dictionary.insert( name, i );
	    dict_modified = true;
	    fsend( ch, "The word \"%s\" has been added to the system dictionary.", name );
	  }
	}
	return;
      }

      // Search system.
      while( *argument ) {
	argument = one_argument( argument, arg );
	const int l = strlen( arg );
	int i = pntr_search( dictionary.list, dictionary.size, arg, l );
	if( i < 0 )
	  i = -i-1;
	int j = 0;
	char *b = buf;
	while( i < dictionary && !strncasecmp( dictionary[i], arg, l ) ) {
	  if( j == 0 ) {
	    page_title( ch, "System dictionary: %s", arg );
	  }
	  b += sprintf( b, "%19s", dictionary[i] );
	  if( j++ % 4 == 3 ) {
	    page( ch, buf );
	    page( ch, "\n\r" );
	    b = buf;
	  }
	  ++i;
	}
	if( j % 4 != 0 ) {
	  page( ch, buf );
	  page( ch, "\n\r" );
	  
	}
	if( j == 0 ) {
	  fsend( ch, "No words matching \"%s\" found in system dictionary.", arg );
	} else {
	  page( ch, "\n\r%d matching word%s found.\n\r",
		j, j == 1 ? "" : "s" );
	}
      }
      return;
    }
    
    if( is_set( flags, 2 ) ) {
      if( !has_permission( ch, PERM_SNOOP ) ) {
	send( ch, "You don't have permission to look at players' dictionaries.\n\r" );
	return;
      }

      in_character = false;

      pfile_data *pfile = find_pfile( argument, ch );
      
      if( !pfile )
	return;

      if( pfile != ch->pcdata->pfile
	  && pfile->trust >= get_trust( ch ) ) {
	fsend( ch, "You cannot view the dictionary of %s.", pfile->name );
	return;
      }
      
      player_data *pc = find_player( pfile );
      bool loaded = false;
      
      if( !pc ) {
	link_data link;
	link.connected = CON_PLAYING;
	if( !load_char( &link, pfile->name, PLAYER_DIR ) ) {
	  bug( "Load_players: error reading player file. (%s)", pfile->name );
	  return;
	}
	pc = link.player;
	loaded = true;
      }
      
      if( !*pc->pcdata->dictionary ) {
	if( ch == pc ) {
	  page( ch, "Your custom dictionary is empty.\n\r" );
	} else {
	  page( ch, "%s's custom dictionary is empty.\n\r", pc );
	}

      } else {
	
	if( ch == pc ) {
	  page_title( ch, "Your custom dictionary" );
	} else {
	  page_title( ch, "%s's custom dictionary", pc );
	}
	
	char tmp [ 3*MAX_STRING_LENGTH ];
	convert_to_ansi( ch, 3*MAX_STRING_LENGTH, pc->pcdata->dictionary, tmp );
	page( ch, tmp );
      }
      
      if( loaded ) {
	page( ch, "\n\r" );
	page_centered( ch, "[ Player file was loaded from disk. ]" );
	pc->Extract();
	extracted.delete_list();
      }
      return;
    }
  }
  
  if( !*argument ) {
    if( !*ch->pcdata->dictionary ) {
      send( ch, "Your custom dictionary is empty.\n\r" );
      send( ch, "Type \"dictionary <words>\" to add words.\n\r" );
      return;
    }
    page_title( ch, "Your custom dictionary" );
  }

  ch->pcdata->dictionary = edit_string( ch, argument, ch->pcdata->dictionary, MEM_PLAYER, false );
}
