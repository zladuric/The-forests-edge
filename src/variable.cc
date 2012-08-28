#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include "define.h"
#include "struct.h"


/*
 *   COMMANDS
 */


class command_ident
{
public:
  const char*      name;
  do_func*     function;
};


static const command_ident command_list [] =
{
  { "-",           do_avatar     },
  { ":",           do_buildchan  },
  { "=",           do_adminchan  },
  { "accounts",    do_accounts   },
  { "allegiance",  do_allegiance },
  { "ask",         do_ask        },
  { "at",          do_at         },
  { "auction",     do_auction    },
  { "atalk",       do_atalk      },
  { "berserk",     do_berserk    },
  { "bid",         do_bid        },
  { "bite",        do_bite       },
  { "build",       do_build      },
  { "bury",        do_bury       },
  { "butcher",     do_butcher    },
  { "buy",         do_buy        }, 
  { "calculate",   do_calculate  },
  { "camp",        do_camp       },
  { "cedit",       do_cedit      },
  { "changes",     do_changes    },
  { "chant",       do_chant      },
  { "chat",        do_chat       },
  { "clans",       do_clans      },
  { "close",       do_close      },
  { "consent",     do_consent    },
  { "cook",        do_cook       },
  { "cover",       do_cover      },
  { "ctell",       do_ctell      },
  { "custom",      do_custom     },
  { "deal",        do_deal       },
  { "define",      do_define     },
  { "dictionary",  do_dictionary },
  { "dig",         do_dig        },
  { "dip",         do_dip        },
  { "disguise",    do_disguise   },
  { "dismount",    do_dismount   },
  { "dock",        do_dock       },
  { "down",        do_down       },
  { "drink",       do_drink      },
  { "drop",        do_drop       },
  { "east",        do_east       },
  { "edit",        do_edit       },
  { "emote",       do_emote      },
  { "empty",       do_empty      },
  { "enter",       do_enter      },
  { "equipment",   do_equipment  },
  { "exits",       do_exits      },
  { "exp",         do_exp        },
  { "extract",     do_extract    },
  { "filter",      do_filter     },
  { "focus",       do_focus      },
  { "forage",      do_forage     },
  { "fwhere",      do_fwhere     },
  { "garrote",     do_garrote    },
  { "get",         do_get        },
  { "give",        do_give       },
  { "gossip",      do_gossip     },
  { "gtell",       do_gtell      },
  { "hbug",        do_hbug       },
  { "hfind",       do_hfind      },
  { "hstat",       do_hstat      },
  { "homepage",    do_homepage   },
  { "identity",    do_identity   },
  { "iflag",       do_iflag      },
  { "ignite",      do_ignite     },
  { "index",       do_index      },
  { "info",        do_info       },
  { "introduce",   do_introduce  },
  { "journal",     do_journal    },
  { "kick",        do_kick       }, 
  { "kill",        do_kill       }, 
  { "knock",       do_knock      },
  { "label",       do_label      },
  { "language",    do_language   },
  { "ledit",       do_ledit      },
  { "list",        do_list       },
  { "load",        do_load       },
  { "lost",        do_lost       },
  { "lset",        do_lset       },
  { "lstat",       do_lstat      },
  { "marmor",      do_marmor     },
  { "mbug",        do_mbug       },
  { "mdesc",       do_mdesc      },
  { "medit",       do_medit      },
  { "meditate",    do_meditate   },
  { "melt",        do_melt       },
  { "melee",       do_melee      },
  { "message",     do_message    },
  { "mextra",      do_mextra     },
  { "mfind",       do_mfind      },
  { "mflag",       do_mflag      },
  { "miflag",      do_miflag     },
  { "miset",       do_miset      },
  { "mistat",      do_mistat     },
  { "mload",       do_mload      },
  { "mlog",        do_mlog       },
  { "mount",       do_mount      },
  { "mpflag",      do_mpflag     },
  { "mreset",      do_mreset     },
  { "mset",        do_mset       },
  { "mskill",      do_mskill     },
  { "mstat",       do_mstat      },
  { "mwhere",      do_mwhere     },
  { "newbie",      do_newbie     },
  { "north",       do_north      },
  { "ooc",         do_ooc        },
  { "path",        do_path       },
  { "pbug",        do_pbug       },
  { "perform",     do_perform    },
  { "pfind",       do_pfind      },
  { "play",        do_play       },
  { "purchase",    do_purchase   },
  { "pray",        do_pray       },
  { "prompt",      do_prompt     },
  { "ps",          do_ps         },
  { "qstat",       do_qstat      },
  { "queue",       do_queue      },
  { "recognize",   do_recognize  },
  { "repair",      do_repair     },
  { "reply",       do_reply      },
  { "reset",       do_reset      },
  { "rest",        do_rest       },
  { "reboot",      do_reboot     },
  { "rename",      do_rename     },
  { "request",     do_request    },
  { "review",      do_review     },
  { "rmwhere",     do_rmwhere    },
  { "roomlist",    do_roomlist   },
  { "rtable",      do_rtable     },
  { "rtwhere",     do_rtwhere    },
  { "say",         do_say        }, 
  { "sell",        do_sell       },
  { "shout",       do_shout      },
  { "sit",         do_sit        }, 
  { "sleep",       do_sleep      },
  { "smoke",       do_smoke      },
  { "south",       do_south      },
  { "speed",       do_speed      },
  { "split",       do_split      },
  { "stand",       do_stand      },
  { "tell",        do_tell       },
  { "to",          do_to         },
  { "typo",        do_typo       },
  { "up",          do_up         },  
  { "value",       do_value      },
  { "vote",        do_vote       },
  { "wait",        do_wait       },
  { "wake",        do_wake       },
  { "wear",        do_wear       },
  { "west",        do_west       },
  { "whisper",     do_whisper    },
  { "wield",       do_wear       },
  { "yell",        do_yell       },
				   
  { "relations",   do_relations  },
  { "eat",         do_eat        },
  { "fill",        do_fill       },
  { "hold",        do_wear       },
  { "inventory",   do_inventory  },
  { "junk",        do_junk       },
  { "lock",        do_lock       },
  { "open",        do_open       },
  { "pick",        do_pick       },
  { "put",         do_put        },
  { "quaff",       do_quaff      },
  { "remove",      do_remove     },
  { "sacrifice",   do_sacrifice  },
  { "unlock",      do_unlock     },
  { "assist",      do_assist     },
  { "backstab",    do_backstab   },
  { "bash",        do_bash       },
  { "camouflage",  do_camouflage },
  { "charge",      do_charge     },
  { "consider",    do_consider   },
  { "disarm",      do_disarm     },
  { "flee",        do_flee       }, 
  { "glance",      do_glance     },
  { "punch",       do_punch      },
  { "rescue",      do_rescue     },
  { "shoot",       do_shoot      },
  { "spin",        do_spin_kick  },
  { "throw",       do_throw      },
  { "wimpy",       do_wimpy      },
  { "cast",        do_cast       },
  { "leech",       do_leech      },
  { "polymorph",   do_polymorph  },
  { "prepare",     do_prepare    },
  { "recite",      do_recite     },
  { "skill",       do_skill      },
  { "spell",       do_spell      },
  { "switch",      do_switch     },
  { "return",      do_return     },
  { "zap",         do_zap        }, 
  { "alias",       do_alias      },
  { "appearance",  do_appearance },
  { "areas",       do_areas      },
  { "befriend",    do_befriend   },
  { "commands",    do_commands   },
  { "compare",     do_compare    },
  { "color",       do_color      },
  { "description", do_descript   },
  { "help",        do_help       },
  { "keywords",    do_keywords   },
  { "last",        do_last       },
  { "look",        do_look       },
  { "motd",        do_motd       },
  { "options",     do_options    },
  { "peek",        do_peek       },
  { "pets",        do_pets       },
  { "qlook",       do_qlook      },
  { "quests",      do_quests     },
  { "qwho",        do_qwho       },
  { "reputation",  do_reputation },
  { "score",       do_score      },
  { "socials",     do_socials    },
  { "statistics",  do_statistics },
  { "time",        do_time       },
  { "who",         do_who        }, 
  { "whois",       do_whois      },
  { "mail",        do_mail       },
  { "notes",       do_notes      },
  { "title",       do_title      },
  { "follow",      do_follow     },
  { "group",       do_group      },
  { "name",        do_name       },
  { "order",       do_order      },
  { "delete",      do_delete     },
  { "password",    do_password   },
  { "quit",        do_quit       },
  { "save",        do_save       },
  { "balance",     do_balance    },
  { "deposit",     do_deposit    },
  { "withdraw",    do_withdraw   },
  { "abilities",   do_abilities  },
  { "appraise",    do_appraise   },
  { "bandage",     do_bandage    },
  { "climb",       do_climb      },
  { "hands",       do_hands      },
  { "hide",        do_hide       }, 
  { "inspect",     do_inspect    },
  { "practice",    do_practice   },
  { "scan",        do_scan       }, 
  { "search",      do_search     },
  { "skin",        do_skin       }, 
  { "sneak",       do_sneak      },
  { "steal",       do_steal      },
  { "heist",       do_heist      },
  { "energize",    do_energize   },
  { "track",       do_track      },
  { "untrap",      do_untrap     },

  { "advance",     do_advance    },
  { "approve",     do_approve    },
  { "ban",         do_ban        }, 
  { "beep",        do_beep       },
  { "bamfin",      do_bamfin     },
  { "bamfout",     do_bamfout    },
  { "bugs",        do_bugs       },
  { "compile",     do_compile    },
  { "constants",   do_constants  },
  { "disconnect",  do_disconnect },
  { "identify",    do_identify   },
  { "pardon",      do_pardon     },
  { "reimburse",   do_reimburse  },
  { "where",       do_where      },
  { "tcode",       do_tcode      },
  { "tdata",       do_tdata      },
  { "tedit",       do_tedit      },
  { "tset",        do_tset       },
  { "tstat",       do_tstat      }, 
  { "dedit",       do_dedit      }, 
  { "dflag",       do_dflag      }, 
  { "dset",        do_dset       },  
  { "dstat",       do_dstat      }, 
  { "rbug",        do_rbug       },  
  { "rdesc",       do_rdesc      }, 
  { "redit",       do_redit      }, 
  { "religion",    do_religion   },
  { "rfind",       do_rfind      },
  { "rflag",       do_rflag      }, 
  { "rlog",        do_rlog       },  
  { "rset",        do_rset       },  
  { "rstat",       do_rstat      }, 
  { "obug",        do_obug       },
  { "odesc",       do_odesc      },
  { "oedit",       do_oedit      },
  { "oextra",      do_oextra     },
  { "ofind",       do_ofind      },
  { "oflag",       do_oflag      },
  { "oload",       do_oload      },
  { "olog",        do_olog       },
  { "oset",        do_oset       },
  { "ostat",       do_ostat      },
  { "owhere",      do_owhere     },
  { "opedit",      do_opedit     },
  { "opcode",      do_opcode     },
  { "opdata",      do_opdata     },
  { "opflag",      do_opflag     },
  { "opset",       do_opset      },
  { "opstat",      do_opstat     },
  { "oiflag",      do_oiflag     },
  { "oiset",       do_oiset      },
  { "oistat",      do_oistat     },
  { "hdesc",       do_hdesc      },
  { "hedit",       do_hedit      },
  { "hset",        do_hset       },
  { "acode",       do_acode      },
  { "adata",       do_adata      },
  { "aedit",       do_aedit      },
  { "aflag",       do_aflag      },
  { "aset",        do_aset       },
  { "astat",       do_astat      },
  { "mpedit",      do_mpedit     },
  { "mpcode",      do_mpcode     },
  { "mpdata",      do_mpdata     },
  { "mpstat",      do_mpstat     },
  { "mpset",       do_mpset      },
  { "cflag",       do_cflag      },
  { "qbug",        do_qbug       },
  { "qedit",       do_qedit      },
  { "qflag",       do_qflag      },
  { "qremove",     do_qremove    },
  { "qset",        do_qset       },
  { "qwhere",      do_qwhere     },
  { "cwhere",      do_cwhere     },
  { "shcustom",    do_shcustom   },
  { "shedit",      do_shedit     },
  { "shflag",      do_shflag     },
  { "affects",     do_affects    },
  { "high",        do_high       },
  { "level",       do_level      },
  { "memory",      do_memory     },
  { "move",        do_move       },
  { "movement",    do_movement   },
  { "privileges",  do_privileges },
  { "pull",        do_pull       },
  { "push",        do_push       },
  { "read",        do_read       },
  { "system",      do_system     },
  { "wanted",      do_wanted     },
  { "whistle",     do_whistle    },
  { "functions",   do_functions  },
  { "return",      do_return     },
  { "deny",        do_deny       },
  { "echo",        do_echo       },
  { "force",       do_force      },
  { "freeze",      do_freeze     },
  { "god",         do_god        },
  { "godlock",     do_godlock    },
  { "goto",        do_goto       },
  { "holylight",   do_holylight  },
  { "immtalk",     do_immtalk    },
  { "imprison",    do_imprison   },
  { "invis",       do_invis      },
  { "lag",         do_lag        },
  { "map",         do_map        },
  { "peace",       do_peace      },
  { "purge",       do_purge      },
  { "recho",       do_recho      },
  { "restore",     do_restore    },
  { "rowhere",     do_rowhere    },
  { "shutdown",    do_shutdown   },
  { "snoop",       do_snoop      },
  { "slay",        do_slay       }, 
  { "sset",        do_sset       },
  { "status",      do_status     },
  { "testing",     do_testing    },
  { "tfind",       do_tfind      },
  { "train",       do_train      },
  { "transfer",    do_transfer   },
  { "trust",       do_trust      },
  { "users",       do_users      },
  { "weather",     do_weather    },
  { "wizlock",     do_wizlock    },
  { "write",       do_write      },
  { "",            0             }
};


static size_t command_count = 0;


void init_command( int i )
{
  command_table[i].function = 0;

  for( int j = 0; *command_list[j].name; ++j ) {
    if( !strcasecmp( command_table[i].func_name, command_list[j].name ) ) {
      command_table[i].function = command_list[j].function;
      break;
    }
  }
}


void init_commands( )
{
  for( int i = 0; i < table_max[ TABLE_COMMAND ]; ++i ) {
    init_command( i );
  }

  for( command_count = 0; *command_list[command_count].name; ++command_count );
}


void display_commands( char_data *ch, const char *title )
{
  display_array( ch, title,
		 &command_list[0].name, &command_list[1].name,
		 command_count,
		 true );
}


/*
 *   SPELLS 
 */


class spell_ident
{
public:
  const char*       name;
  spell_func*   function;
};


static const spell_ident spell_list [] =
{ 
  { "acid blast",         spell_acid_blast         },
  { "astral gate",        spell_astral_gate        },
  { "augury",             spell_augury             },
  { "banishment",         spell_banishment         },
  { "blind",              spell_blind              },
  { "blink",              spell_blink              },
  { "calm",               spell_calm               },
  { "confuse",            spell_confuse            },
  { "construct golem",    spell_construct_golem    },
  { "create feast",       spell_create_feast       },
  { "create food",        spell_create_food        },
  { "create light",       spell_create_light       },
  { "create water",       spell_create_water       },
  { "cure blindness",     spell_cure_blindness     },
  { "cure poison",        spell_cure_poison        },
  { "eagle eye",          spell_eagle_eye          },
  { "fear",               spell_fear               },
  { "find familiar",      spell_find_familiar      },
  { "fire shield",        spell_fire_shield        },
  { "fireball",           spell_fireball           },
  { "greater animation",  spell_greater_animation  },
  { "hallucinate",        spell_hallucinate        },
  { "ice shield",         spell_ice_shield         },
  { "identify",           spell_identify           },
  { "lesser summoning",   spell_lesser_summoning   },
  { "maelstrom",          spell_maelstrom          },
  { "major enchantment",  spell_major_enchantment  },
  { "mind blade",         spell_mind_blade         },
  { "minor enchantment",  spell_minor_enchantment  },
  { "neutralize",         spell_neutralize         },
  { "paralyze",           spell_paralyze           },
  { "polymorph",          spell_polymorph          },
  { "protection/good",    spell_protect_good       },
  { "protection/evil",    spell_protect_evil       },
  { "protection/law",     spell_protect_law        },
  { "protection/chaos",   spell_protect_chaos      },
  { "recall",             spell_recall             },
  { "remove curse",       spell_remove_curse       },
  { "request ally",       spell_request_ally       },
  { "resurrect",          spell_resurrect          },
  { "revitalize",         spell_revitalize         },
  { "sanctify",           spell_sanctify           },
  { "scry",               spell_scry               },
  { "sleep",              spell_sleep              },
  { "slow",               spell_slow               },
  { "summon",             spell_summon             },
  { "transfer",           spell_transfer           }, 
  { "turn undead",        spell_turn_undead        },
  { "web",                spell_web                },
  { "wizard lock",        spell_wizard_lock        },
  { "youth",              spell_youth              },

  { "holy wrath",         spell_holy_wrath         },
  { "tame",               spell_tame               },
  { "faerie fire",        spell_faerie_fire        },
  { "invisibility",       spell_invisibility       },
  { "mists of sleep",     spell_mists_sleep        },
  { "locust swarm",       spell_locust_swarm       },
  { "poison cloud",       spell_poison_cloud       },
  { "amnesia",            spell_amnesia            },
  { "ignite weapon",      spell_ignite_weapon      },
  { "chain lightning",    spell_chain_lightning    },
  { "animate dead",       spell_animate_dead       },
  { "ogre strength",      spell_ogre_strength      },
  { "silence",            spell_silence            },
  { "drain life",         spell_drain_life         },
  { "blinding light",     spell_blinding_light     },
  { "cure disease",       spell_cure_disease       },
  { "animate clay",       spell_animate_clay       },
  { "replicate",          spell_replicate          },
  { "group serious",      spell_group_serious      },
  { "group critical",     spell_group_critical     },
  { "ion shield",         spell_ion_shield         },
  { "meteor swarm",       spell_meteor_swarm       },
  { "purify",             spell_purify             },
  { "wither",             spell_wither             },
  { "obscure",            spell_obscure            },
  { "conjure elemental",  spell_conjure_elemental  },
  { "find mount",         spell_find_mount         },
  { "hawks view",         spell_hawks_view         },
  { "deafen",             spell_deafen             },
  { "wizard_lock",        spell_wizard_lock        },
  { "ward",               spell_ward               },

  { "song of morale",     song_of_morale           },
  { "song of heroism",    song_of_heroism          },
  { "song of zeal",       song_of_zeal             },
  { "song of valor",      song_of_valor            },
  { "song of grace",      song_of_grace            },
  { "song of fortitude",  song_of_fortitude        },
  { "song of the sentinel", song_of_sentinel       },
  { "song of legends",    song_of_legends          },
  { "song of the mystics",  song_of_mystics        },
  { "song of the wanderer", song_of_wanderer       },
  { "song of the wind",   song_of_wind             },
  { "song of the ward",   song_of_ward             },

  { "arc lightning",      spell_arc_lightning      },
  { "lore",               spell_lore               },
  { "bestiary",           spell_bestiary           },

  { "",                   0                        }
};


static void init_spell( int i )
{
  skill_spell_table[i].function = 0;

  for( int j = 0; *spell_list[j].name; ++j ) {
    if( !strcasecmp( skill_spell_table[i].name, spell_list[j].name ) ) {
      skill_spell_table[i].function = spell_list[j].function;
      break;
    }
  }
  /*
  if( !skill_spell_table[i].function ) 
    panic( "Init_Spell: Null function - %s", skill_spell_table[i].name );
  */
}


void init_spells( )
{
  for( int i = 0; i < table_max[TABLE_SKILL_SPELL]; ++i ) {
    init_spell(i);
  }
}


/*
 *   FUNCTIONS 
 */


static size_t function_count = 0;


void init_function( int i )
{
  function_table[i].function = 0;

  for( int j = 0; *cfunc_list[j].name; ++j ) {
    if( !strcasecmp( function_table[i].func_name, cfunc_list[j].name ) ) {
      function_table[i].function = cfunc_list[j].func_call;
      break;
    }
  }
}


void init_functions( )
{
  for( int i = 0; i < table_max[ TABLE_FUNCTION ]; ++i ) {
    init_command( i );
  }

  for( function_count = 0; *cfunc_list[function_count].name; ++function_count );
}


void display_functions( char_data *ch, const char *title )
{
  display_array( ch, title,
		 &cfunc_list[0].name, &cfunc_list[1].name,
		 function_count,
		 true );
}
