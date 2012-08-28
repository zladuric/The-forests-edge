#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include "define.h"
#include "struct.h"


/*
 *   LOCAL VARIABLES
 */


static flag_data lang_flags = {
  "Languages",
  &skill_language_table[0].name, &skill_language_table[1].name,
  &table_max[ TABLE_SKILL_LANGUAGE ], true };

static flag_data class_flags = {
  "classes", &clss_table[0].name, &clss_table[1].name,
  &table_max[ TABLE_CLSS ], true
};

static flag_data race_flags = {
  "races", &plyr_race_table[0].name, &plyr_race_table[1].name,
  &table_max[ TABLE_PLYR_RACE ], true
};

static int max_sex = MAX_SEX - 1;

static flag_data sex_flags = {
  "sexes", &sex_name[0], &sex_name[1], &max_sex, false
};


Social_Type    	 social_table         [ MAX_PLYR_RACE+1 ][ MAX_SOCIAL ];
Spell_Act_Type 	 spell_act_table      [ MAX_SPELL_ACT ];
Liquid_Type    	 liquid_table         [ MAX_LIQUID ];
Town_Type      	 town_table           [ MAX_TOWN ];
Religion_Data  	 religion_table       [ MAX_RELIGION ];
Skill_Type       skill_physical_table [ ABS_PHY_SKILL ];
Skill_Type       skill_language_table [ ABS_LANG_SKILL ];
Spell_Skill_Type skill_spell_table    [ ABS_SPELL_SKILL ];
Skill_Type       skill_trade_table    [ ABS_TRADE_SKILL ];
Weapon_Skill_Type skill_weapon_table  [ ABS_WEAP_SKILL ];
Material_Type  	 material_table       [ MAX_MATERIAL ];
Nation_Data    	 nation_table         [ MAX_NATION ];
Group_Data     	 group_table          [ MAX_GROUP ];     
Race_Data      	 race_table           [ MAX_RACE ];
Plyr_Race_Data 	 plyr_race_table      [ MAX_PLYR_RACE ];
Aff_Char_Type  	 aff_char_table       [ MAX_AFF_CHAR ];
Aff_Obj_Type   	 aff_obj_table        [ MAX_AFF_OBJ ];
Aff_Room_Type  	 aff_room_table       [ MAX_AFF_ROOM ];
Command_Type   	 command_table        [ MAX_COMMAND ];
Category_Data  	 cmd_cat_table        [ MAX_CMD_CAT ];
Clss_Type      	 clss_table           [ MAX_CLSS ];
Starting_Data  	 starting_table       [ MAX_CLSS+MAX_PLYR_RACE+1 ];
Tedit_Data     	 tedit_table          [ MAX_TABLE ];
Recipe_Data    	 build_table          [ MAX_BUILD ];
Category_Data  	 help_cat_table       [ MAX_HELP_CAT ];
Town_Type      	 astral_table         [ MAX_ASTRAL ];
Alignment_Data 	 alignment_table      [ MAX_ALIGNMENT ];
Terrain_Data   	 terrain_table        [ MAX_TERRAIN ];
Climate_Data   	 climate_table        [ MAX_CLIMATE ];
Month_Data     	 month_table          [ MAX_MONTH ];
Day_Data       	 day_table            [ MAX_DAY ];
Movement_Data  	 movement_table       [ MAX_MOVEMENT ];
Hallucinate_Data hallucinate_table    [ MAX_HALLUCINATE ];
Function_Data    function_table       [ MAX_FUNCTION ];


int table_max [ MAX_TABLE ];


Table_Data *tables[ MAX_TABLE ] = {
  social_table[ 0 ],
  social_table[ RACE_HUMAN+1 ],
  social_table[ RACE_ELF+1 ],
  social_table[ RACE_GNOME+1 ],
  social_table[ RACE_DWARF+1 ],
  social_table[ RACE_HALFLING+1 ],
  social_table[ RACE_ENT+1 ],
  social_table[ RACE_CENTAUR+1 ],
  social_table[ RACE_LIZARD+1 ],
  social_table[ RACE_OGRE+1 ],
  social_table[ RACE_TROLL+1 ],
  social_table[ RACE_ORC+1 ],
  social_table[ RACE_GOBLIN+1 ],
  social_table[ RACE_VYAN+1 ],
  spell_act_table,
  liquid_table,
  town_table,
  religion_table,
  skill_physical_table,
  skill_language_table,
  skill_spell_table,
  skill_trade_table,
  skill_weapon_table,
  material_table,
  nation_table,
  group_table,
  race_table,
  plyr_race_table,
  aff_char_table,
  aff_obj_table,
  aff_room_table,
  command_table,
  cmd_cat_table,
  clss_table,
  starting_table,
  tedit_table,
  build_table,
  help_cat_table,
  astral_table,
  alignment_table,
  terrain_table,
  climate_table,
  month_table,
  day_table,
  movement_table,
  hallucinate_table,
  function_table
};


/*
 *   LOCAL CONSTANTS
 */


static const int table_abs_max [ MAX_TABLE ] = {
  MAX_SOCIAL, MAX_SOCIAL, MAX_SOCIAL, MAX_SOCIAL, MAX_SOCIAL, MAX_SOCIAL,
  MAX_SOCIAL, MAX_SOCIAL, MAX_SOCIAL, MAX_SOCIAL, MAX_SOCIAL, MAX_SOCIAL,
  MAX_SOCIAL, MAX_SOCIAL, MAX_SPELL_ACT, MAX_LIQUID, MAX_TOWN, MAX_RELIGION,
  ABS_PHY_SKILL, ABS_LANG_SKILL, ABS_SPELL_SKILL, ABS_TRADE_SKILL, ABS_WEAP_SKILL,
  MAX_MATERIAL, MAX_NATION, MAX_GROUP, MAX_RACE, MAX_PLYR_RACE,
  MAX_AFF_CHAR, MAX_AFF_OBJ, MAX_AFF_ROOM, MAX_COMMAND, MAX_CMD_CAT, MAX_CLSS,
  MAX_CLSS+MAX_PLYR_RACE+1,
  MAX_TABLE, MAX_BUILD, MAX_HELP_CAT, MAX_ASTRAL,
  MAX_ALIGNMENT, MAX_TERRAIN, MAX_CLIMATE, MAX_MONTH, MAX_DAY, MAX_MOVEMENT,
  MAX_HALLUCINATE, MAX_FUNCTION
};


static const int table_size [ MAX_TABLE ] = {
  sizeof( Social_Type ),
  sizeof( Social_Type ),
  sizeof( Social_Type ),
  sizeof( Social_Type ),
  sizeof( Social_Type ),
  sizeof( Social_Type ),
  sizeof( Social_Type ),
  sizeof( Social_Type ),
  sizeof( Social_Type ),
  sizeof( Social_Type ),
  sizeof( Social_Type ),
  sizeof( Social_Type ),
  sizeof( Social_Type ),
  sizeof( Social_Type ),
  sizeof( Spell_Act_Type ),
  sizeof( Liquid_Type ),
  sizeof( Town_Type ),
  sizeof( Religion_Data ),
  sizeof( Skill_Type ), 
  sizeof( Skill_Type ), 
  sizeof( Spell_Skill_Type ), 
  sizeof( Skill_Type ), 
  sizeof( Weapon_Skill_Type ), 
  sizeof( Material_Type ),
  sizeof( Nation_Data ),
  sizeof( Group_Data ),
  sizeof( Race_Data ),
  sizeof( Plyr_Race_Data ),
  sizeof( Aff_Char_Type ),
  sizeof( Aff_Obj_Type ),
  sizeof( Aff_Room_Type ),
  sizeof( Command_Type ),
  sizeof( Category_Data ),
  sizeof( Clss_Type ),
  sizeof( Starting_Data ),
  sizeof( Tedit_Data ),
  sizeof( Recipe_Data ),
  sizeof( Category_Data ),
  sizeof( Town_Type ),
  sizeof( Alignment_Data ),
  sizeof( Terrain_Data ),
  sizeof( Climate_Data ),
  sizeof( Month_Data ),
  sizeof( Day_Data ),
  sizeof( Movement_Data ),
  sizeof( Hallucinate_Data ),
  sizeof( Function_Data )
};


#define VAR_CHAR                 0
#define VAR_INT                  1
#define VAR_SA                   2
#define CNST_CHAR                3
#define VAR_SKILL                4
#define VAR_SIZE     		 5
#define VAR_COMMAND   		 6
#define VAR_CC       		 7
#define VAR_POS                  8
#define VAR_AFF_FLAGS            9
#define VAR_OBJ       	        10
#define VAR_DICE      	        11
#define VAR_STYPE     	        12
#define VAR_PERM_FLAGS          13
#define VAR_FORMULA	        14
#define VAR_LEECH  	        15
#define VAR_BOOL   	        16
#define VAR_ALIGN_FLAGS         17
#define VAR_LANG_FLAGS          18
#define VAR_AFF_LOC             19
#define VAR_LOC_FLAGS           20
#define VAR_CENT                21
#define VAR_SEX                 22
#define VAR_CLASS_FLAGS         23
#define VAR_TEMP                24
#define VAR_BLANK               25
#define VAR_DELETE              26
#define VAR_SEX_FLAGS           27
#define VAR_RACE_FLAGS          28
#define VAR_RACE                29
#define VAR_COLOR               30
#define VAR_PERCENT             31
#define VAR_TERRAIN_FLAGS       32
#define VAR_RELIGIONS           33
#define VAR_SPELL               34
#define VAR_PROG                35
#define VAR_FUNCTION            36
#define VAR_ARG_TYPE            37
#define VAR_VARIABLE            38


static const Entry_Data social_entry[] = {
  {  &social_table[0][0].name,           VAR_CHAR,   true,  true  },
  {  &social_table[0][0].position,       VAR_POS,    true,  true  },
  {  &social_table[0][0].aggressive,     VAR_BOOL,   true,  true  },
  {  &social_table[0][0].reveal,         VAR_BOOL,   true,  true  },
  {  &social_table[0][0].disrupt,        VAR_BOOL,   true,  true  },
  {  &social_table[0][0].char_no_arg,    VAR_CHAR,   true,  true  },
  {  &social_table[0][0].others_no_arg,  VAR_CHAR,   true,  true  },
  {  &social_table[0][0].char_found,     VAR_CHAR,   true,  true  },
  {  &social_table[0][0].others_found,   VAR_CHAR,   true,  true  },
  {  &social_table[0][0].vict_found,     VAR_CHAR,   true,  true  },
  {  &social_table[0][0].vict_sleep,     VAR_CHAR,   true,  true  },
  {  &social_table[0][0].char_auto,      VAR_CHAR,   true,  true  },
  {  &social_table[0][0].others_auto,    VAR_CHAR,   true,  true  },
  {  &social_table[0][0].obj_self,       VAR_CHAR,   true,  true  },
  {  &social_table[0][0].obj_others,     VAR_CHAR,   true,  true  },
  {  &social_table[0][0].dir_self,       VAR_CHAR,   true,  true  },
  {  &social_table[0][0].dir_others,     VAR_CHAR,   true,  true  },
  {  &social_table[0][0].ch_obj_self,    VAR_CHAR,   true,  true  },
  {  &social_table[0][0].ch_obj_victim,  VAR_CHAR,   true,  true  },
  {  &social_table[0][0].ch_obj_others,  VAR_CHAR,   true,  true  },
  {  &social_table[0][0].ch_obj_sleep,   VAR_CHAR,   true,  true  },
  {  &social_table[0][0].self_obj_self,  VAR_CHAR,   true,  true  },
  {  &social_table[0][0].self_obj_others,VAR_CHAR,   true,  true  },
  {  &social_table[0][0].prog,           VAR_PROG,   true,  true  }
};

static const Entry_Data spell_act_entry[] = {
  {  &spell_act_table[0].name,           VAR_CHAR,   true,  true    },
  {  &spell_act_table[0].self_other,     VAR_CHAR,   true,  true    },
  {  &spell_act_table[0].victim_other,   VAR_CHAR,   true,  true    },
  {  &spell_act_table[0].others_other,   VAR_CHAR,   true,  true    },
  {  &spell_act_table[0].self_self,      VAR_CHAR,   true,  true    },
  {  &spell_act_table[0].others_self,    VAR_CHAR,   true,  true    }
};

static const Entry_Data liquid_entry[] = {
  {  &liquid_table[0].name,           VAR_CHAR,   true,  true    },
  {  &liquid_table[0].color,          VAR_CHAR,   true,  true    },   
  {  &liquid_table[0].hunger,         VAR_INT,   true,  true     },
  {  &liquid_table[0].thirst,         VAR_INT,   true,  true     },
  {  &liquid_table[0].alcohol,        VAR_INT,   true,  true     },
  {  &liquid_table[0].cost,           VAR_INT,   true,  true     },
  {  &liquid_table[0].create,         VAR_BOOL,   true,  true    },
  {  &liquid_table[0].spell,          VAR_SPELL,   true,  true   }
};

static const Entry_Data town_entry[] = {
  { &town_table[0].name,              VAR_CHAR,   true,  true    },
  { &town_table[0].recall,            VAR_INT,   true,  true     }
};

static const Entry_Data religion_entry[] = {
  { &religion_table[0].name,                    VAR_CHAR,   true,  true    },
  { &religion_table[0].sex,                     VAR_SEX,    true,  true    },
  { &religion_table[0].alignments,              VAR_ALIGN_FLAGS,  true,  true    },
  { &religion_table[0].classes,                 VAR_CLASS_FLAGS,  true,  true    },
  { &religion_table[0].sexes,                   VAR_SEX_FLAGS,  true,  true    },
  { &religion_table[0].races,                   VAR_RACE_FLAGS,  true,  true    }
};

static const Entry_Data skill_physical_entry[] = {
  { &skill_physical_table[0].name,                         VAR_CHAR,  true,  true     },
  { &skill_physical_table[0].pre_skill[0],                 VAR_SKILL, true,  true     },
  { &skill_physical_table[0].pre_level[0],                 VAR_INT,   true,  true     },
  { &skill_physical_table[0].pre_skill[1],                 VAR_SKILL, true,  true     },
  { &skill_physical_table[0].pre_level[1],                 VAR_INT,   true,  true     },
  { &skill_physical_table[0].prac_cost[ CLSS_MAGE ],       VAR_INT,   true,  true     },  
  { &skill_physical_table[0].prac_cost[ CLSS_CLERIC ],     VAR_INT,   true,  true     },  
  { &skill_physical_table[0].prac_cost[ CLSS_THIEF ],      VAR_INT,   true,  true     },  
  { &skill_physical_table[0].prac_cost[ CLSS_WARRIOR ],    VAR_INT,   true,  true     },  
  { &skill_physical_table[0].prac_cost[ CLSS_PALADIN ],    VAR_INT,   true,  true     },  
  { &skill_physical_table[0].prac_cost[ CLSS_RANGER ],     VAR_INT,   true,  true     },  
  { &skill_physical_table[0].prac_cost[ CLSS_MONK ],       VAR_INT,   true,  true     },  
  { &skill_physical_table[0].prac_cost[ CLSS_DRUID ],      VAR_INT,   true,  true     },
  { &skill_physical_table[0].prac_cost[ CLSS_BARD ],       VAR_INT,   true,  true     },
  { &skill_physical_table[0].level[ CLSS_MAGE ],           VAR_INT,   true,  true     },
  { &skill_physical_table[0].level[ CLSS_CLERIC ],         VAR_INT,   true,  true     },  
  { &skill_physical_table[0].level[ CLSS_THIEF ],          VAR_INT,   true,  true     },  
  { &skill_physical_table[0].level[ CLSS_WARRIOR ],        VAR_INT,   true,  true     },  
  { &skill_physical_table[0].level[ CLSS_PALADIN ],        VAR_INT,   true,  true     },  
  { &skill_physical_table[0].level[ CLSS_RANGER ],         VAR_INT,   true,  true     },  
  { &skill_physical_table[0].level[ CLSS_MONK ],           VAR_INT,   true,  true     },  
  { &skill_physical_table[0].level[ CLSS_DRUID ],          VAR_INT,   true,  true     },
  { &skill_physical_table[0].level[ CLSS_BARD ],           VAR_INT,   true,  true     },
  { &skill_physical_table[0].religions,                    VAR_RELIGIONS, true, true }
};

static const Entry_Data skill_language_entry[] = {
  { &skill_language_table[0].name,                         VAR_CHAR,  true,  true     },
  { &skill_language_table[0].pre_skill[0],                 VAR_SKILL, true,  true     },
  { &skill_language_table[0].pre_level[0],                 VAR_INT,   true,  true     },
  { &skill_language_table[0].pre_skill[1],                 VAR_SKILL, true,  true     },
  { &skill_language_table[0].pre_level[1],                 VAR_INT,   true,  true     },
  { &skill_language_table[0].prac_cost[ CLSS_MAGE ],       VAR_INT,   true,  true     },  
  { &skill_language_table[0].prac_cost[ CLSS_CLERIC ],     VAR_INT,   true,  true     },  
  { &skill_language_table[0].prac_cost[ CLSS_THIEF ],      VAR_INT,   true,  true     },  
  { &skill_language_table[0].prac_cost[ CLSS_WARRIOR ],    VAR_INT,   true,  true     },  
  { &skill_language_table[0].prac_cost[ CLSS_PALADIN ],    VAR_INT,   true,  true     },  
  { &skill_language_table[0].prac_cost[ CLSS_RANGER ],     VAR_INT,   true,  true     },  
  { &skill_language_table[0].prac_cost[ CLSS_MONK ],       VAR_INT,   true,  true     },  
  { &skill_language_table[0].prac_cost[ CLSS_DRUID ],      VAR_INT,   true,  true     },
  { &skill_language_table[0].prac_cost[ CLSS_BARD ],       VAR_INT,   true,  true     },
  { &skill_language_table[0].level[ CLSS_MAGE ],           VAR_INT,   true,  true     },
  { &skill_language_table[0].level[ CLSS_CLERIC ],         VAR_INT,   true,  true     },  
  { &skill_language_table[0].level[ CLSS_THIEF ],          VAR_INT,   true,  true     },  
  { &skill_language_table[0].level[ CLSS_WARRIOR ],        VAR_INT,   true,  true     },  
  { &skill_language_table[0].level[ CLSS_PALADIN ],        VAR_INT,   true,  true     },  
  { &skill_language_table[0].level[ CLSS_RANGER ],         VAR_INT,   true,  true     },  
  { &skill_language_table[0].level[ CLSS_MONK ],           VAR_INT,   true,  true     },  
  { &skill_language_table[0].level[ CLSS_DRUID ],          VAR_INT,   true,  true     },
  { &skill_language_table[0].level[ CLSS_BARD ],           VAR_INT,   true,  true     },
  { &skill_language_table[0].religions,                    VAR_RELIGIONS, true, true }
};

static const Entry_Data skill_spell_entry[] = {
  { &skill_spell_table[0].name,                         VAR_CHAR,  true,  true     },
  { &skill_spell_table[0].pre_skill[0],                 VAR_SKILL, true,  true     },
  { &skill_spell_table[0].pre_level[0],                 VAR_INT,   true,  true     },
  { &skill_spell_table[0].pre_skill[1],                 VAR_SKILL, true,  true     },
  { &skill_spell_table[0].pre_level[1],                 VAR_INT,   true,  true     },
  { &skill_spell_table[0].prac_cost[ CLSS_MAGE ],       VAR_INT,   true,  true     },  
  { &skill_spell_table[0].prac_cost[ CLSS_CLERIC ],     VAR_INT,   true,  true     },  
  { &skill_spell_table[0].prac_cost[ CLSS_THIEF ],      VAR_INT,   true,  true     },  
  { &skill_spell_table[0].prac_cost[ CLSS_WARRIOR ],    VAR_INT,   true,  true     },  
  { &skill_spell_table[0].prac_cost[ CLSS_PALADIN ],    VAR_INT,   true,  true     },  
  { &skill_spell_table[0].prac_cost[ CLSS_RANGER ],     VAR_INT,   true,  true     },  
  { &skill_spell_table[0].prac_cost[ CLSS_MONK ],       VAR_INT,   true,  true     },  
  { &skill_spell_table[0].prac_cost[ CLSS_DRUID ],      VAR_INT,   true,  true     },
  { &skill_spell_table[0].prac_cost[ CLSS_BARD ],       VAR_INT,   true,  true     },
  { &skill_spell_table[0].level[ CLSS_MAGE ],           VAR_INT,   true,  true     },
  { &skill_spell_table[0].level[ CLSS_CLERIC ],         VAR_INT,   true,  true     },  
  { &skill_spell_table[0].level[ CLSS_THIEF ],          VAR_INT,   true,  true     },  
  { &skill_spell_table[0].level[ CLSS_WARRIOR ],        VAR_INT,   true,  true     },  
  { &skill_spell_table[0].level[ CLSS_PALADIN ],        VAR_INT,   true,  true     },  
  { &skill_spell_table[0].level[ CLSS_RANGER ],         VAR_INT,   true,  true     },  
  { &skill_spell_table[0].level[ CLSS_MONK ],           VAR_INT,   true,  true     },  
  { &skill_spell_table[0].level[ CLSS_DRUID ],          VAR_INT,   true,  true     },
  { &skill_spell_table[0].level[ CLSS_BARD ],           VAR_INT,   true,  true     },
  { &skill_spell_table[0].religions,                    VAR_RELIGIONS, true, true },
  { &skill_spell_table[0].prepare,         		  VAR_INT,   true,  true       },
  { &skill_spell_table[0].wait,            		  VAR_INT,   true,  true       },
  { &skill_spell_table[0].type,            		  VAR_STYPE,   true,  true     },
  { &skill_spell_table[0].damage,          		  VAR_FORMULA,   true,  true   },
  { &skill_spell_table[0].cast_mana,       		  VAR_FORMULA,   true,  true   },
  { &skill_spell_table[0].leech_mana,      		  VAR_FORMULA,   true,  true   },
  { &skill_spell_table[0].regen,           		  VAR_FORMULA,   true,  true   },
  { &skill_spell_table[0].duration,        		  VAR_FORMULA,   true,  true   },
  { &skill_spell_table[0].location,        		  VAR_LOC_FLAGS,   true,  true },
  { &skill_spell_table[0].action[0],       		  VAR_SA,   true,  true        },
  { &skill_spell_table[0].action[1],       		  VAR_SA,   true,  true        },
  { &skill_spell_table[0].action[2],       		  VAR_SA,   true,  true        },
  { &skill_spell_table[0].action[3],       		  VAR_SA,   true,  true        },
  { &skill_spell_table[0].action[4],       		  VAR_SA,   true,  true        },
  { &skill_spell_table[0].reagent[0],      		  VAR_INT,   true,  true       },
  { &skill_spell_table[0].reagent[1],      		  VAR_INT,   true,  true       },
  { &skill_spell_table[0].reagent[2],      		  VAR_INT,   true,  true       },
  { &skill_spell_table[0].reagent[3],      		  VAR_INT,   true,  true       },
  { &skill_spell_table[0].reagent[4],      		  VAR_INT,   true,  true       },
  { &skill_spell_table[0].prog,                           VAR_PROG, true,  true       }
};

static const Entry_Data skill_trade_entry[] = {
  { &skill_trade_table[0].name,                         VAR_CHAR,  true,  true     },
  { &skill_trade_table[0].pre_skill[0],                 VAR_SKILL, true,  true     },
  { &skill_trade_table[0].pre_level[0],                 VAR_INT,   true,  true     },
  { &skill_trade_table[0].pre_skill[1],                 VAR_SKILL, true,  true     },
  { &skill_trade_table[0].pre_level[1],                 VAR_INT,   true,  true     },
  { &skill_trade_table[0].prac_cost[ CLSS_MAGE ],       VAR_INT,   true,  true     },  
  { &skill_trade_table[0].prac_cost[ CLSS_CLERIC ],     VAR_INT,   true,  true     },  
  { &skill_trade_table[0].prac_cost[ CLSS_THIEF ],      VAR_INT,   true,  true     },  
  { &skill_trade_table[0].prac_cost[ CLSS_WARRIOR ],    VAR_INT,   true,  true     },  
  { &skill_trade_table[0].prac_cost[ CLSS_PALADIN ],    VAR_INT,   true,  true     },  
  { &skill_trade_table[0].prac_cost[ CLSS_RANGER ],     VAR_INT,   true,  true     },  
  { &skill_trade_table[0].prac_cost[ CLSS_MONK ],       VAR_INT,   true,  true     },  
  { &skill_trade_table[0].prac_cost[ CLSS_DRUID ],      VAR_INT,   true,  true     },
  { &skill_trade_table[0].prac_cost[ CLSS_BARD ],       VAR_INT,   true,  true     },
  { &skill_trade_table[0].level[ CLSS_MAGE ],           VAR_INT,   true,  true     },
  { &skill_trade_table[0].level[ CLSS_CLERIC ],         VAR_INT,   true,  true     },  
  { &skill_trade_table[0].level[ CLSS_THIEF ],          VAR_INT,   true,  true     },  
  { &skill_trade_table[0].level[ CLSS_WARRIOR ],        VAR_INT,   true,  true     },  
  { &skill_trade_table[0].level[ CLSS_PALADIN ],        VAR_INT,   true,  true     },  
  { &skill_trade_table[0].level[ CLSS_RANGER ],         VAR_INT,   true,  true     },  
  { &skill_trade_table[0].level[ CLSS_MONK ],           VAR_INT,   true,  true     },  
  { &skill_trade_table[0].level[ CLSS_DRUID ],          VAR_INT,   true,  true     },
  { &skill_trade_table[0].level[ CLSS_BARD ],           VAR_INT,   true,  true     },
  { &skill_trade_table[0].religions,                    VAR_RELIGIONS, true, true }
};

static const Entry_Data skill_weapon_entry[] = {
  { &skill_weapon_table[0].name,                         VAR_CHAR,  true,  true     },
  { &skill_weapon_table[0].pre_skill[0],                 VAR_SKILL, true,  true     },
  { &skill_weapon_table[0].pre_level[0],                 VAR_INT,   true,  true     },
  { &skill_weapon_table[0].pre_skill[1],                 VAR_SKILL, true,  true     },
  { &skill_weapon_table[0].pre_level[1],                 VAR_INT,   true,  true     },
  { &skill_weapon_table[0].prac_cost[ CLSS_MAGE ],       VAR_INT,   true,  true     },  
  { &skill_weapon_table[0].prac_cost[ CLSS_CLERIC ],     VAR_INT,   true,  true     },  
  { &skill_weapon_table[0].prac_cost[ CLSS_THIEF ],      VAR_INT,   true,  true     },  
  { &skill_weapon_table[0].prac_cost[ CLSS_WARRIOR ],    VAR_INT,   true,  true     },  
  { &skill_weapon_table[0].prac_cost[ CLSS_PALADIN ],    VAR_INT,   true,  true     },  
  { &skill_weapon_table[0].prac_cost[ CLSS_RANGER ],     VAR_INT,   true,  true     },  
  { &skill_weapon_table[0].prac_cost[ CLSS_MONK ],       VAR_INT,   true,  true     },  
  { &skill_weapon_table[0].prac_cost[ CLSS_DRUID ],      VAR_INT,   true,  true     },
  { &skill_weapon_table[0].prac_cost[ CLSS_BARD ],       VAR_INT,   true,  true     },
  { &skill_weapon_table[0].level[ CLSS_MAGE ],           VAR_INT,   true,  true     },
  { &skill_weapon_table[0].level[ CLSS_CLERIC ],         VAR_INT,   true,  true     },  
  { &skill_weapon_table[0].level[ CLSS_THIEF ],          VAR_INT,   true,  true     },  
  { &skill_weapon_table[0].level[ CLSS_WARRIOR ],        VAR_INT,   true,  true     },  
  { &skill_weapon_table[0].level[ CLSS_PALADIN ],        VAR_INT,   true,  true     },  
  { &skill_weapon_table[0].level[ CLSS_RANGER ],         VAR_INT,   true,  true     },  
  { &skill_weapon_table[0].level[ CLSS_MONK ],           VAR_INT,   true,  true     },  
  { &skill_weapon_table[0].level[ CLSS_DRUID ],          VAR_INT,   true,  true     },
  { &skill_weapon_table[0].level[ CLSS_BARD ],           VAR_INT,   true,  true     },
  { &skill_weapon_table[0].religions,                    VAR_RELIGIONS, true, true },
  { &skill_weapon_table[0].noun[0],                 	   VAR_CHAR,   true,  true    },
  { &skill_weapon_table[0].noun[1],                 	   VAR_CHAR,   true,  true    },
  { &skill_weapon_table[0].noun[2],                 	   VAR_CHAR,   true,  true    },
  { &skill_weapon_table[0].noun[3],                 	   VAR_CHAR,   true,  true    },
  { &skill_weapon_table[0].noun[4],                 	   VAR_CHAR,   true,  true    },
  { &skill_weapon_table[0].verb[0],                 	   VAR_CHAR,   true,  true    },
  { &skill_weapon_table[0].verb[1],                 	   VAR_CHAR,   true,  true    },
  { &skill_weapon_table[0].verb[2],                 	   VAR_CHAR,   true,  true    },
  { &skill_weapon_table[0].verb[3],                 	   VAR_CHAR,   true,  true    },
  { &skill_weapon_table[0].verb[4],                 	   VAR_CHAR,   true,  true    }
};

static const Entry_Data material_entry[] = {
  { &material_table[0].name,                      VAR_CHAR,  true,  true     },
  { &material_table[0].cost,                      VAR_INT,   true,  true     },
  { &material_table[0].weight,                    VAR_INT,   true,  true     },
  { &material_table[0].mana,                      VAR_INT,   true,  true     },
  { &material_table[0].armor,                     VAR_INT,   true,  true     },
  { &material_table[0].enchant,                   VAR_INT,   true,  true     },
  { &material_table[0].save_fire,                 VAR_INT,   true,  true     },
  { &material_table[0].save_cold,                 VAR_INT,   true,  true     },
  { &material_table[0].save_acid,                 VAR_INT,   true,  true     },
  { &material_table[0].msg_fire,                  VAR_CHAR,  true,  true     },
  { &material_table[0].msg_cold,                  VAR_CHAR,  true,  true     },
  { &material_table[0].msg_acid,                  VAR_CHAR,  true,  true     },
  { &material_table[0].rust_name,                 VAR_CHAR,  true,  true     },
  { &material_table[0].rust_verb,                 VAR_CHAR,  true,  true     },
  { &material_table[0].rust[0],                   VAR_CHAR,  true,  true     },
  { &material_table[0].rust[1],                   VAR_CHAR,  true,  true     },
  { &material_table[0].rust[2],                   VAR_CHAR,  true,  true     },
  { &material_table[0].rust[3],                   VAR_CHAR,  true,  true     },
  { &material_table[0].ingot[0],                  VAR_OBJ,   true,  true     }
};

static const Entry_Data nation_entry[] = {
  { &nation_table[0].name,                        VAR_CHAR,  true,  true     },
  { &nation_table[0].abbrev,                      VAR_CHAR,  true,  true     },
  { &nation_table[0].temple,                      VAR_INT,   true,  true     },
  { &nation_table[0].room[0],                     VAR_INT,   true,  true     },
  { &nation_table[0].room[1],                     VAR_INT,   true,  true     },
  { &nation_table[0].race[0],                     VAR_INT,   true,  true     },
  { &nation_table[0].race[1],                     VAR_INT,   true,  true     },
  { &nation_table[0].race[2],                     VAR_INT,   true,  true     },
  { &nation_table[0].race[3],                     VAR_INT,   true,  true     },
  { &nation_table[0].race[4],                     VAR_INT,   true,  true     },
  { &nation_table[0].race[5],                     VAR_INT,   true,  true     },
  { &nation_table[0].race[6],                     VAR_INT,   true,  true     },
  { &nation_table[0].race[7],                     VAR_INT,   true,  true     },
  { &nation_table[0].race[8],                     VAR_INT,   true,  true     },
  { &nation_table[0].race[9],                     VAR_INT,   true,  true     },
  { &nation_table[0].race[10],                    VAR_INT,   true,  true     },
  { &nation_table[0].race[11],                    VAR_INT,   true,  true     },
  { &nation_table[0].race[12],                    VAR_INT,   true,  true     },
  { &nation_table[0].alignment[0],                VAR_INT,   true,  true     },
  { &nation_table[0].alignment[1],                VAR_INT,   true,  true     },
  { &nation_table[0].alignment[2],                VAR_INT,   true,  true     },
  { &nation_table[0].alignment[3],                VAR_INT,   true,  true     },
  { &nation_table[0].alignment[4],                VAR_INT,   true,  true     },
  { &nation_table[0].alignment[5],                VAR_INT,   true,  true     },
  { &nation_table[0].alignment[6],                VAR_INT,   true,  true     },
  { &nation_table[0].alignment[7],                VAR_INT,   true,  true     },
  { &nation_table[0].alignment[8],                VAR_INT,   true,  true     }
};

static const Entry_Data group_entry[] = {
  { &group_table[0].name,                       VAR_CHAR,   true,  true    }
};

static const Entry_Data race_entry[] = {
  { &race_table[0].name,                        VAR_CHAR,   true,  true    },
  { &race_table[0].plural,                      VAR_CHAR,   true,  true    },
  { &race_table[0].abbrev,                      VAR_CHAR,   true,  true    },
  { &race_table[0].track,                       VAR_CHAR,   true,  true    },
  { &race_table[0].family,                      VAR_RACE,   true,  true    }
};

static const Entry_Data plyr_race_entry[] = {
  { &plyr_race_table[0].name,                   VAR_CHAR,  true,  true     },
  { &plyr_race_table[0].hp_bonus,               VAR_INT,   true,  true     },
  { &plyr_race_table[0].mana_bonus,             VAR_INT,   true,  true     },
  { &plyr_race_table[0].move_bonus,             VAR_INT,   true,  true     },
  { &plyr_race_table[0].size,                   VAR_SIZE,  true,  true     },
  { &plyr_race_table[0].tolerance,              VAR_INT,   true, true },
  { &plyr_race_table[0].weight_m,               VAR_CENT,  true,  true     },
  { &plyr_race_table[0].weight_f,               VAR_CENT,  true,  true     },
  { &plyr_race_table[0].height_m,               VAR_INT,   true,  true     },
  { &plyr_race_table[0].height_f,               VAR_INT,   true,  true     },
  { &plyr_race_table[0].stat_bonus[0],          VAR_INT,   true,  true     },
  { &plyr_race_table[0].stat_bonus[1],          VAR_INT,   true,  true     },
  { &plyr_race_table[0].stat_bonus[2],          VAR_INT,   true,  true     },
  { &plyr_race_table[0].stat_bonus[3],          VAR_INT,   true,  true     },
  { &plyr_race_table[0].stat_bonus[4],          VAR_INT,   true,  true     },
  { &plyr_race_table[0].resist[0],              VAR_INT,   true,  true     }, 
  { &plyr_race_table[0].resist[1],              VAR_INT,   true,  true     }, 
  { &plyr_race_table[0].resist[2],              VAR_INT,   true,  true     }, 
  { &plyr_race_table[0].resist[3],              VAR_INT,   true,  true     }, 
  { &plyr_race_table[0].resist[4],              VAR_INT,   true,  true     }, 
  { &plyr_race_table[0].resist[5],              VAR_INT,   true,  true     }, 
  { &plyr_race_table[0].resist[6],              VAR_INT,   true,  true     }, 
  { &plyr_race_table[0].affect[0],              VAR_AFF_FLAGS,   true,  true     },
  { &plyr_race_table[0].start_room[0],          VAR_INT,   true,  true     },
  { &plyr_race_table[0].start_room[1],          VAR_INT,   true,  true     },
  { &plyr_race_table[0].start_room[2],          VAR_INT,   true,  true     },
  { &plyr_race_table[0].portal,                 VAR_INT,   true,  true     },
  { &plyr_race_table[0].start_age,              VAR_INT,   true,  true     },
  { &plyr_race_table[0].life_span,              VAR_INT,   true,  true     },
  { &plyr_race_table[0].alignments,             VAR_ALIGN_FLAGS,   true,  true   },
  { &plyr_race_table[0].language,               VAR_LANG_FLAGS,   true,  true    },
  { &plyr_race_table[0].open,                   VAR_BOOL,   true,  true    }
  //    { &plyr_race_table[0].hunger_time,            VAR_INT,   true,  true     },
  //    { &plyr_race_table[0].thirst_time,            VAR_INT,   true,  true     },
  //    { &plyr_race_table[0].drunk_time,             VAR_INT,   true,  true     }
};

static const Entry_Data aff_char_entry[] = {
  { &aff_char_table[0].name,                    VAR_CHAR,   true,  true      },
  { &aff_char_table[0].id_line,                 VAR_CHAR,   true,  true      },
  { &aff_char_table[0].score_name,              VAR_CHAR,   true,  true      },
  { &aff_char_table[0].msg_on,                  VAR_CHAR,   true,  true      },
  { &aff_char_table[0].msg_on_room,             VAR_CHAR,   true,  true      },
  { &aff_char_table[0].msg_fade,                VAR_CHAR,   true,  true      },
  { &aff_char_table[0].msg_fade_room,           VAR_CHAR,   true,  true      },
  { &aff_char_table[0].msg_off,                 VAR_CHAR,   true,  true      },
  { &aff_char_table[0].msg_off_room,            VAR_CHAR,   true,  true      },
  { &aff_char_table[0].location,                VAR_AFF_LOC,   true,  true   },
  { &aff_char_table[0].modifier,                VAR_CHAR,   true,  true      }
};

static const Entry_Data aff_obj_entry[] = {
  { &aff_obj_table[0].name,                     VAR_CHAR,   true,  true      },
  { &aff_obj_table[0].msg_on,                   VAR_CHAR,   true,  true      },
  { &aff_obj_table[0].msg_off,                  VAR_CHAR,   true,  true      },
  { &aff_obj_table[0].location,                 VAR_INT,    true,  true      },
  { &aff_obj_table[0].prog,                     VAR_PROG,   true,  true      }
};

static const Entry_Data aff_room_entry[] = {
  { &aff_room_table[0].name,                    VAR_CHAR,   true,  true      },
  { &aff_room_table[0].msg_on,                  VAR_CHAR,   true,  true      },
  { &aff_room_table[0].msg_fade,                VAR_CHAR,   true,  true      },
  { &aff_room_table[0].msg_off,                 VAR_CHAR,   true,  true      }
};

static const Entry_Data command_entry[] = {
  { &command_table[0].name,                     CNST_CHAR,   true,  true   },
  { &command_table[0].help,                     VAR_CHAR,   true,  true    },
  { &command_table[0].func_name,                VAR_COMMAND,   true,  true    },
  { &command_table[0].level[0],                 VAR_PERM_FLAGS,   true,  true    },
  { &command_table[0].reqlen,                   VAR_INT,   true,  true     },
  { &command_table[0].position,                 VAR_POS,   true,  true     },
  { &command_table[0].category,                 VAR_CC,   true,  true      },
  { &command_table[0].disrupt,                  VAR_BOOL,   true,  true    },
  { &command_table[0].reveal,                   VAR_BOOL,   true,  true    },
  { &command_table[0].queue,                    VAR_BOOL,   true,  true    },
  { &command_table[0].prog,                     VAR_PROG,  true,  true    }
};
 
static const Entry_Data cmd_cat_entry[] = {
  { &cmd_cat_table[0].name,                     VAR_CHAR,   true,  true    },
  { &cmd_cat_table[0].level,                    VAR_INT,   true,  true     }
};

static const Entry_Data clss_entry[] = {
  { &clss_table[0].name,                        VAR_CHAR,   true,  true    },
  { &clss_table[0].abbrev,                      VAR_CHAR,   true,  true    },
  { &clss_table[0].hit_min,                     VAR_INT,   true,  true     },
  { &clss_table[0].hit_max,                     VAR_INT,   true,  true     },
  { &clss_table[0].mana_min,                    VAR_INT,   true,  true     },
  { &clss_table[0].mana_max,                    VAR_INT,   true,  true     },
  { &clss_table[0].move_min,                    VAR_INT,   true,  true     },
  { &clss_table[0].move_max,                    VAR_INT,   true,  true     },
  { &clss_table[0].hit_bonus,                   VAR_INT,   true,  true     },
  { &clss_table[0].mana_bonus,                  VAR_INT,   true,  true     },
  { &clss_table[0].move_bonus,                  VAR_INT,   true,  true     },
  { &clss_table[0].resist[0],                   VAR_INT,   true,  true     },
  { &clss_table[0].resist[1],                   VAR_INT,   true,  true     },
  { &clss_table[0].resist[2],                   VAR_INT,   true,  true     },
  { &clss_table[0].resist[3],                   VAR_INT,   true,  true     },
  { &clss_table[0].resist[4],                   VAR_INT,   true,  true     },
  { &clss_table[0].resist[5],                   VAR_INT,   true,  true     },
  { &clss_table[0].resist[6],                   VAR_INT,   true,  true     },
  { &clss_table[0].alignments,                  VAR_ALIGN_FLAGS,   true,  true   },
  { &clss_table[0].open,                        VAR_BOOL,   true,  true    }
};

static const Entry_Data starting_entry[] = {
  { &starting_table[0].name,                    VAR_CHAR,   true,  true    },
  { &starting_table[0].object[0],               VAR_OBJ,   true,  true     },
  { &starting_table[0].object[2],               VAR_OBJ,   true,  true     },
  { &starting_table[0].object[4],               VAR_OBJ,   true,  true     },
  { &starting_table[0].object[6],               VAR_OBJ,   true,  true     },
  { &starting_table[0].object[8],               VAR_OBJ,   true,  true     },
  { &starting_table[0].skill[0],                VAR_SKILL, true,  true     },
  { &starting_table[0].skill[1],                VAR_SKILL, true,  true     },
  { &starting_table[0].skill[2],                VAR_SKILL, true,  true     },
  { &starting_table[0].skill[3],                VAR_SKILL, true,  true     },
  { &starting_table[0].skill[4],                VAR_SKILL, true,  true     },
  { &starting_table[0].level[0],                VAR_INT,   true,  true     },
  { &starting_table[0].level[1],                VAR_INT,   true,  true     },
  { &starting_table[0].level[2],                VAR_INT,   true,  true     },
  { &starting_table[0].level[3],                VAR_INT,   true,  true     },
  { &starting_table[0].level[4],                VAR_INT,   true,  true     }
};
  
static const Entry_Data tedit_entry[] = {
  { &tedit_table[0].name,                       VAR_CHAR,  true,  true     },
  { &tedit_table[0].edit,                       VAR_INT,   true,  true     },
  { &tedit_table[0].new_delete,                 VAR_INT,   true,  true     },
  { &tedit_table[0].sort,                       VAR_BOOL,  true,  true     },
  { &tedit_table[0].lock,                       VAR_BOOL,  true,  true     }
};

static const Entry_Data build_entry[] = {
  { &build_table[0].name,                       VAR_CHAR,   true,  true    },
  { &build_table[0].result[0],                  VAR_OBJ,   true,  true     },
  { &build_table[0].ingredient[0],              VAR_OBJ,   true,  true     },
  { &build_table[0].ingredient[2],              VAR_OBJ,   true,  true     },
  { &build_table[0].ingredient[4],              VAR_OBJ,   true,  true     },
  { &build_table[0].ingredient[6],              VAR_OBJ,   true,  true     },
  { &build_table[0].ingredient[8],              VAR_OBJ,   true,  true     },
  { &build_table[0].ingredient[10],             VAR_OBJ,   true,  true     },
  { &build_table[0].ingredient[12],             VAR_OBJ,   true,  true     },
  { &build_table[0].ingredient[14],             VAR_OBJ,   true,  true     },
  { &build_table[0].ingredient[16],             VAR_OBJ,   true,  true     },
  { &build_table[0].ingredient[18],             VAR_OBJ,   true,  true     },
  { &build_table[0].skill[0],                   VAR_SKILL,   true,  true   },
  { &build_table[0].skill[1],                   VAR_SKILL,   true,  true   },
  { &build_table[0].skill[2],                   VAR_SKILL,   true,  true   },
  { &build_table[0].tool[0],                    VAR_OBJ,   true,  true     },
  { &build_table[0].tool[2],                    VAR_OBJ,   true,  true     }
};

static const Entry_Data help_cat_entry[] = {
  { &help_cat_table[0].name,                    VAR_CHAR,   true,  true    },
  { &help_cat_table[0].level,                   VAR_INT,   true,  true     }
};

static const Entry_Data astral_entry[] = {
  { &astral_table[0].name,                      VAR_CHAR,   true,  true    },
  { &astral_table[0].recall,                    VAR_INT,   true,  true     }
};

static const Entry_Data alignment_entry[] = {
  { &alignment_table[0].name,                   VAR_CHAR,   true,  true     },
  { &alignment_table[0].abbrev,                 VAR_CHAR,   true,  true     }
};

static const Entry_Data terrain_entry[] = {
  { &terrain_table[0].name,                     VAR_CHAR,   true,  true    },
  { &terrain_table[0].mv_cost,                  VAR_INT,    true,  true    },
  { &terrain_table[0].color,                    VAR_COLOR,  true,  true    },
  { &terrain_table[0].light,                    VAR_PERCENT,true,  true    },
  { &terrain_table[0].wind,                     VAR_PERCENT,true,  true    },
  { &terrain_table[0].flags,                    VAR_TERRAIN_FLAGS, true, true    },
  { &terrain_table[0].surface,                  VAR_CHAR,   true,  true    },
  { &terrain_table[0].position,                 VAR_CHAR,   true,  true    },
  { &terrain_table[0].drop,                     VAR_CHAR,   true,  true    },
  { &terrain_table[0].forage[0],                VAR_OBJ,    true,  true    },
  { &terrain_table[0].forage[2],                VAR_OBJ,    true,  true    },
  { &terrain_table[0].forage[4],                VAR_OBJ,    true,  true    },
  { &terrain_table[0].forage[6],                VAR_OBJ,    true,  true    },
  { &terrain_table[0].forage[8],                VAR_OBJ,    true,  true    },
  { &terrain_table[0].forage[10],               VAR_OBJ,    true,  true    },
  { &terrain_table[0].forage[12],               VAR_OBJ,    true,  true    },
  { &terrain_table[0].forage[14],               VAR_OBJ,    true,  true    },
  { &terrain_table[0].forage[16],               VAR_OBJ,    true,  true    },
  { &terrain_table[0].forage[18],               VAR_OBJ,    true,  true    }
};

static const Entry_Data climate_entry[] = {
  { &climate_table[0].name,                     VAR_CHAR,   true,  true    },
  { &climate_table[0].temp_summer,              VAR_INT,    true,  true   },
  { &climate_table[0].temp_winter,              VAR_INT,    true,  true   },
  { &climate_table[0].humid_summer,             VAR_PERCENT,true,  true    },
  { &climate_table[0].humid_winter,             VAR_PERCENT,true,  true    },
  { &climate_table[0].forage,                   VAR_INT,    true,  true    }
};

static const Entry_Data month_entry[] = {
  { &month_table[0].name,                       VAR_CHAR,   true,  true    },
  { &month_table[0].days,                       VAR_INT,    true,  true    }
};

static const Entry_Data day_entry[] = {
  { &day_table[0].name,                         VAR_CHAR,   true,  true    }
};

static const Entry_Data movement_entry[] = {
  { &movement_table[0].name,                    VAR_CHAR,   true,  true    },
  { &movement_table[0].leave,                   VAR_CHAR,   true,  true    },
  { &movement_table[0].arrive,                  VAR_CHAR,   true,  true    },
  { &movement_table[0].position,                VAR_CHAR,   true,  true    },
  { &movement_table[0].player,                  VAR_BOOL,   true,  true    }
};

static const Entry_Data hallucinate_entry[] = {
  { &hallucinate_table[0].name,                 VAR_CHAR,   true, true     },
  { &hallucinate_table[0].plural,               VAR_CHAR,   true, true     }
};


static const Entry_Data function_entry[] = {
  { &function_table[0].name,                    CNST_CHAR,   true,  true   },
  { &function_table[0].func_name,               VAR_FUNCTION, true,  true  },
  { &function_table[0].return_type,             VAR_ARG_TYPE, true,  true  },
  { &function_table[0].arg_type[0],             VAR_ARG_TYPE, true,  true  },
  { &function_table[0].var[0],                  VAR_VARIABLE, true,  true  },
  { &function_table[0].arg_type[1],             VAR_ARG_TYPE, true,  true  },
  { &function_table[0].var[1],                  VAR_VARIABLE, true,  true  },
  { &function_table[0].arg_type[2],             VAR_ARG_TYPE, true,  true  },
  { &function_table[0].var[2],                  VAR_VARIABLE, true,  true  },
  { &function_table[0].arg_type[3],             VAR_ARG_TYPE, true,  true  },
  { &function_table[0].var[3],                  VAR_VARIABLE, true,  true  },
  { &function_table[0].arg_type[4],             VAR_ARG_TYPE, true,  true  },
  { &function_table[0].var[4],                  VAR_VARIABLE, true,  true  },
  { &function_table[0].arg_type[5],             VAR_ARG_TYPE, true,  true  },
  { &function_table[0].var[5],                  VAR_VARIABLE, true,  true  },
  { &function_table[0].prog,                    VAR_PROG,  true,  true    }
};
 
static const Entry_Data *table_entry[ MAX_TABLE-MAX_PLYR_RACE] = {
  social_entry,
  spell_act_entry,
  liquid_entry,
  town_entry,
  religion_entry,
  skill_physical_entry,
  skill_language_entry,
  skill_spell_entry,
  skill_trade_entry,
  skill_weapon_entry,
  material_entry,
  nation_entry,
  group_entry,
  race_entry,
  plyr_race_entry,
  aff_char_entry,
  aff_obj_entry,
  aff_room_entry,
  command_entry,
  cmd_cat_entry,
  clss_entry,
  starting_entry,
  tedit_entry,
  build_entry,
  help_cat_entry,
  astral_entry,
  alignment_entry,
  terrain_entry,
  climate_entry,
  month_entry,
  day_entry,
  movement_entry,
  hallucinate_entry,
  function_entry
};


static const char* table_names [ MAX_TABLE ] = {
  "Soc.Default",
  "Soc.Human", "Soc.Elf", "Soc.Gnome", "Soc.Dwarf",
  "Soc.Halfling", "Soc.Ent", "Soc.Centaur", "Soc.Lizard",
  "Soc.Ogre", "Soc.Troll", "Soc.Orc", "Soc.Goblin", "Soc.Vyan",
  "Spell.Actions",
  "Liquids", "Towns", "Religions",
  "Skills.Physical", "Skills.Language", "Skills.Spell", "Skills.Trade", "Skills.Weapon",
  "Materials", "Nations",
  "Groups", "Races", "Player.Races", "Aff.Char", "Aff.Obj", "Aff.Room",
  "Commands",
  "Cmd.Categories", "Classes", "Starting", "Tables", "Build",
  "Help.Categories", "Astral", "Alignments",
  "Terrain", "Climate", "Months", "Days", "Movement",
  "Hallucinate", "Functions" };


static const char* social_fields [] = {
  "name",
  "position",
  "aggressive",
  "disrupt",
  "reveal",
  "no_arg.self",
  "no_arg.others",
  "ch.self",
  "ch.others",
  "ch.victim",
  "ch.sleep",
  "self.self",
  "self.others",
  "obj.self",
  "obj.others",
  "dir.self",
  "dir.others",
  "ch/obj.self",
  "ch/obj.victim",
  "ch/obj.others",
  "ch/obj.sleep",
  "self/obj.self",
  "self/obj.others",
  "*program",
  0
};


static const char* spell_action_fields [] = {
  "name",
  "self_other",
  "victim_other",
  "others_other",
  "self_self",
  "others_self",
  0
};


static const char* liquid_fields [] = {
  "name",
  "color",
  "hunger",
  "thirst",
  "alcohol",
  "cp/liter",
  "creatable", 
  "spell",
  0
};


static const char* town_fields [] = {
  "name",
  "recall_loc",
  0
};


static const char* skill_fields [] = {
  "Name",
  "+Prereq[1]",
  "Level[1]",
  "Prereq[2]",
  "Level[2]",
  "+Cost[Mage]",
  "Cost[Cleric]",
  "Cost[Thief]",
  "Cost[Warrior]",
  "Cost[Paladin]",
  "Cost[Ranger]",
  "Cost[Monk]",
  "Cost[Druid]",
  "Cost[Bard]",
  "+Level[Mage]",
  "Level[Cleric]",
  "Level[Thief]",
  "Level[Warrior]",
  "Level[Paladin]",
  "Level[Ranger]",
  "Level[Monk]",
  "Level[Druid]",
  "Level[Bard]",
  "+Religions",
  0
};


static const char *spell_skill_fields [] = {
  "Name",
  "+Prereq[1]",
  "Level[1]",
  "Prereq[2]",
  "Level[2]",
  "+Cost[Mage]",
  "Cost[Cleric]",
  "Cost[Thief]",
  "Cost[Warrior]",
  "Cost[Paladin]",
  "Cost[Ranger]",
  "Cost[Monk]",
  "Cost[Druid]",
  "Cost[Bard]",
  "+Level[Mage]",
  "Level[Cleric]",
  "Level[Thief]",
  "Level[Warrior]",
  "Level[Paladin]",
  "Level[Ranger]",
  "Level[Monk]",
  "Level[Druid]",
  "Level[Bard]",
  "+Religions",
  "+prepare",
  "wait",
  "type",
  "damage",
  "cast_mana",
  "leech_mana",
  "regen",
  "duration",
  "location",
  "+action[1]",
  "action[2]",
  "action[3]",
  "action[4]",
  "action[5]",
  "+reagent[1]",
  "reagent[2]",
  "reagent[3]",
  "reagent[4]",
  "reagent[5]",
  "*program",
  0
};


static const char* weapon_skill_fields [] = {
  "Name",
  "+Prereq[1]",
  "Level[1]",
  "Prereq[2]",
  "Level[2]",
  "+Cost[Mage]",
  "Cost[Cleric]",
  "Cost[Thief]",
  "Cost[Warrior]",
  "Cost[Paladin]",
  "Cost[Ranger]",
  "Cost[Monk]",
  "Cost[Druid]",
  "Cost[Bard]",
  "+Level[Mage]",
  "Level[Cleric]",
  "Level[Thief]",
  "Level[Warrior]",
  "Level[Paladin]",
  "Level[Ranger]",
  "Level[Monk]",
  "Level[Druid]",
  "Level[Bard]",
  "+Religions",
  "+noun[0]",
  "noun[1]",
  "noun[2]",
  "noun[3]",
  "noun[4]",
  "+verb[0]",
  "verb[1]",
  "verb[2]",
  "verb[3]",
  "verb[4]",
  0
};


static const char* material_fields [] = {
  "Name",
  "Cost",
  "Weight",
  "Mana",
  "Armor", 
  "Enchant",
  "Save[Fire]",
  "Save[Cold]",
  "Save[Acid]",
  "Msg[Fire]",
  "Msg[Cold]", 
  "Msg[Acid]",
  "Rust_Name",   
  "Rust_Verb",   
  "Rust[0]",
  "Rust[1]",
  "Rust[2]",
  "Rust[3]",
  "Ingot",
  0
};


static const char* nation_fields [] = {
  "Name",
  "Abbrev.",
  "Temple",
  "+Room[1]",
  "Room[2]",
  "+Rela[Human]",
  "Rela[Elf]",
  "Rela[Gnome]",
  "Rela[Dwarf]",
  "Rela[Halfling]",
  "Rela[Ent]",
  "Rela[Centaur]",
  "Rela[Lizardman]",
  "Rela[Ogre]",
  "Rela[Troll]",
  "Rela[Orc]",
  "Rela[Goblin]",
  "Rela[Vyan]",
  "+Rela[LG]",
  "Rela[LN]",
  "Rela[LE]",
  "Rela[NG]",
  "Rela[PN]",
  "Rela[NE]",
  "Rela[CG]",
  "Rela[CN]",
  "Rela[CE]",
  0
};


static const char* group_fields [] = {
  "Name",
  0
};


static const char* race_fields [] = { 
  "Name",
  "Plural",
  "Abbrev.",
  "Tracks",
  "Family",
  0
};


static const char* player_race_fields [] = {
  "Name",
  "Hitpoints",
  "Energy",
  "Move",
  "Size",
  "Tolerance",
  "+Weight[Male]",
  "Weight[Female]",
  "+Height[Male]",
  "Height[Female]",
  "+Strength",
  "Intelligence",
  "Wisdom",
  "Dexterity",
  "Constitution",
  "+Magic",
  "Fire",
  "Cold",
  "Electricity",
  "Mind",
  "Acid",
  "Poison",
  "+Affect",
  "+Start[Good]",
  "Start[Neutral]",
  "Start[Evil]",
  "+Portal",
  "Start.Age",
  "Life.Span",
  "Alignments",
  "Language",
  "Open",
  //    "Hunger.Time",
  //    "Thirst.Time",
  //    "Drunk.Time",
  0
};


static const char* aff_char_fields [] = {
  "Name",
  "Id.line",
  "Score.name",
  "Msg.on",
  "Msg.on_room",
  "Msg.fade",
  "Msg.fade_room",
  "Msg.off",
  "Msg.off_room",
  "Location",
  "Modifier",
  0
};


static const char* aff_obj_fields [] = {
  "Name",
  "Msg.On",
  "Msg.Off",
  "Location",
  "*program",
  0
};


static const char* aff_room_fields [] = {
  "Name",
  "Msg.On",
  "Msg.Fade",
  "Msg.Off",
  0
};


static const char* command_fields [] = {
  "name",
  "help",
  "function",
  "level",
  "reqlen",
  "position",
  "category",
  "disrupt",
  "reveal",
  "queue",
  "*program",
  0
};


static const char* cmd_cat_fields [] = {
  "name",
  "level",
  0
};


static const char *class_fields [] = {
  "Name",
  "Abbrev.",
  "+Hit_Min",
  "Hit_Max",
  "Energy_Min",
  "Energy_Max",
  "Move_Min",
  "Move_Max",
  "+Hit_Regen",
  "Energy_Regen",
  "Move_Regen",
  "+Magic",
  "Fire",
  "Cold",
  "Electricity",
  "Mind",
  "Acid",
  "Poison",
  "+Alignments",
  "Open", 
  0
};


static const char* starting_fields [] = {
  "Class",
  "1_Object",
  "2_Object",
  "3_Object",
  "4_Object",
  "5_Object",
  "1_Skill",
  "2_Skill",
  "3_Skill",
  "4_Skill",
  "5_Skill",
  "1_Level",
  "2_Level",
  "3_Level",
  "4_Level",
  "5_Level",
  0
};


static const char* table_fields [] = {
  "Name",
  "Edit",
  "New_Delete",
  "Sort",
  "Lock",
  0
};


static const char* build_fields [] = {
  "name",
  "result",
  "ingred[1]",
  "ingred[2]",
  "ingred[3]",
  "ingred[4]",
  "ingred[5]",
  "ingred[6]",
  "ingred[7]",
  "ingred[8]",
  "ingred[9]",
  "ingred[10]",
  "skill[1]",
  "skill[2]",
  "skill[3]",
  "tool[1]",
  "tool[2]",
  0
};


static const char* help_cat_fields [] = {
  "name",
  "level",
  0
};


static const char* astral_fields [] = {
  "name",
  "location",
  0
};


static const char* religion_fields [] = {
  "name",
  "sex",
  "alignments",
  "classes",
  "sexes",
  "races",
  0
};


static const char* alignment_fields [] = {
  "name",
  "abbrev",
  0
};


static const char* terrain_fields [] = {
  "name",
  "move cost",
  "color",
  "light",
  "wind",
  "flags",
  "surface",
  "position",
  "drop",
  "forage[1]",
  "forage[2]",
  "forage[3]",
  "forage[4]",
  "forage[5]",
  "forage[6]",
  "forage[7]",
  "forage[8]",
  "forage[9]",
  "forage[10]",
  0
};


static const char* climate_fields [] = {
  "name",
  "+Temp[Summer]",
  "Temp[Winter]",
  "+Humid[Summer]",
  "Humid[Winter]",
  "+forage",
  0
};


static const char* month_fields [] = {
  "name",
  "days",
  0
};


static const char* day_fields [] = {
  "name",
  0
};


static const char *movement_fields [] = {
  "name",
  "leaves",
  "arrives",
  "position",
  "player",
  0
};


static const char *hallucinate_fields [] = {
  "name",
  "plural",
  0
};


static const char* function_fields [] = {
  "name",
  "+function",
  "+return type",
  "+type[0]",
  "var[0]",
  "+type[1]",
  "var[1]",
  "+type[2]",
  "var[2]",
  "+type[3]",
  "var[3]",
  "+type[4]",
  "var[4]",
  "+type[5]",
  "var[5]",
  "*program",
  0
};


static const char *const *table_field [ MAX_TABLE ] = {
  social_fields,
  social_fields,
  social_fields,
  social_fields,
  social_fields,
  social_fields,
  social_fields,
  social_fields,
  social_fields,
  social_fields,
  social_fields,
  social_fields,
  social_fields,
  social_fields,
  spell_action_fields,
  liquid_fields,  
  town_fields,
  religion_fields,
  skill_fields,
  skill_fields,
  spell_skill_fields,
  skill_fields,
  weapon_skill_fields,
  material_fields,
  nation_fields,
  group_fields,
  race_fields,
  player_race_fields,
  aff_char_fields,
  aff_obj_fields,
  aff_room_fields,
  command_fields,
  cmd_cat_fields,
  class_fields,
  starting_fields,
  table_fields,
  build_fields,
  help_cat_fields,
  astral_fields,
  alignment_fields,
  terrain_fields,
  climate_fields,
  month_fields,
  day_fields,
  movement_fields,
  hallucinate_fields,
  function_fields
};


/*
 *   SPECIFIC TABLE CONSTRUCTORS/DESTRUCTORS
 */


Terrain_Data :: Terrain_Data( )
  : mv_cost(1), color(COLOR_DEFAULT), light(100), wind(100), flags(0)
{
  surface = alloc_string( "ground", MEM_TABLE );
  position = alloc_string( "on the ground", MEM_TABLE );
  drop = alloc_string( "to the ground", MEM_TABLE );

  vzero( forage, 20 );
}


Terrain_Data :: ~Terrain_Data( )
{
  free_string( surface, MEM_TABLE );
  free_string( position, MEM_TABLE );
  free_string( drop, MEM_TABLE );
}


bool Skill_Type :: religion( int i ) const
{
    if( i < REL_NONE || i >= table_max[ TABLE_RELIGION ] )
      return false;

    return religions.is_empty( ) || religions.includes( i );
}


/*
 *   ENTRY INFO
 */


const char *table_name( int table )
{
  if( table < 0
      || table >= MAX_TABLE ) {
    return 0;
  }

  return table_names[ table ];
}


const char *entry_name( int table, int col )
{
  if( table >= MAX_TABLE
      || col >= table_max[table] )
    return 0;

  if( table <= MAX_PLYR_RACE )
    return social_table[table][col].name;

  int pntr = (int)table_entry[ table-MAX_PLYR_RACE ][0].offset
    +col*table_size[table];

  return (char*) *((char**) pntr);
}


static int entry_type( int table, int entry )
{
  table = max( 0,table-MAX_PLYR_RACE );
 
  return table_entry[table][entry].type;
}


// Returns address of Table_Data::name.
static void *table_pntr( int table, int entry, int col )
{
  if( table <= MAX_PLYR_RACE ) 
    return (void*) ((int)&social_table[table][col]-(int)&social_table[0][0]
		    +(int)table_entry[0][entry].offset);

  return (void *)((int)table_entry[ table-MAX_PLYR_RACE ][entry].offset
		  +col*table_size[table]);
}


// Returns address of Table_Data.
Table_Data *table_addr( int table, int entry )
{
  if( table < 0 || table >= MAX_TABLE
      || entry < 0 || entry >= table_abs_max[table] )
    return 0;

  return (Table_Data*)( ((int)tables[table])+entry*table_size[table] );
}


/*
 *   SUPPORT ROUTINES
 */


static bool can_see( const char_data *ch, int table )
{
  int flag;
  
  switch( table ) {
  case TABLE_COMMAND     :  flag = PERM_COMMANDS;       break;
  case TABLE_SPELL_ACT   :  flag = PERM_SPELLS;         break;
  case TABLE_FUNCTION    :
    return has_permission( ch, PERM_COMMANDS ) || has_permission( ch, PERM_SPELLS );
  default                :  flag = PERM_MISC_TABLES;    break; 
  }
  
  // MISC_TABLES will work for all tables.
  if( table == TABLE_SOC_DEFAULT
      || table <= MAX_PLYR_RACE && table == ch->shdata->race+1 ) {
    if( has_permission( ch, PERM_SOCIALS ) ) {
      flag = PERM_SOCIALS;
    }
  }

  return has_permission( ch, flag );
}


bool edit_table( char_data *ch, int table )
{
  if( !can_see( ch, table ) ) {
    send( ch, "You do not have permission to edit that table.\n\r" );
    return false;
  }

  const int t = max( 0, table - MAX_PLYR_RACE );

  if( table != TABLE_TEDIT && tedit_table[t].lock ) {
    fsend( ch, "The %s table is locked.", table_names[table] );
    return false;
  }

  return true;
}


static int find_table( char_data* ch, const char *& argument, const char* msg1,
		       const char* msg2 = empty_string )
{
  if( !*argument ) {
    if( msg2 != empty_string ) 
      fsend( ch, msg2 );
    return -2;
  }
  
  for( int i = 0; i < MAX_TABLE; ++i ) {
    if( can_see( ch, i )
	&& exact_match( argument, table_names[i] ) ) {
      return i;
    }
  }

  for( int i = 0; i < MAX_TABLE; ++i ) {
    if( can_see( ch, i )
	&& matches( argument, table_names[i] ) ) {
      return i;
    }
  }

  fsend( ch, msg1 );
  return -1;
}


static int find_entry( char_data* ch, const char *& argument, int table, const char* msg = empty_string )
{
  if( !*argument ) {
    if( msg && msg != empty_string )
      fsend( ch, msg );
    return -2;
  }
  
  for( int i = 0; i < table_max[table]; ++i )
    if( exact_match( argument, entry_name( table, i ) ) ) 
      return i;
  
  for( int i = 0; i < table_max[table]; ++i )
    if( fmatches( argument, entry_name( table, i )) ) 
      return i;

  if( msg )
    fsend( ch, "The %s table does not contain such an entry.",
	   table_names[table] );

  return -1;
}


/*
 *   ADDING/REMOVING ENTRIES
 */


static void init_entry( int i, const char *name )
{
  void**  pntr;
  int        j  = table_max[i];
  int        k;

  pntr = (void**)table_pntr( i,0,j );
  *pntr = (void*) alloc_string( name, MEM_TABLE );

  for( k = 1; table_field[i][k]; ++k ) { 
    pntr = (void**)table_pntr( i,k,j );
    switch( entry_type( i,k ) ) {
    case VAR_CHAR :
    case VAR_COMMAND :
    case VAR_FUNCTION :
    case VAR_FORMULA :
      free_string( (char*) *pntr, MEM_TABLE );
      *(const char**)pntr = empty_string;
      break;
    case VAR_SKILL :
    case VAR_SPELL :
      *(int*)pntr = -1;
      break;
    case VAR_INT :
    case VAR_SA :
    case VAR_SIZE :
    case VAR_CC :
    case VAR_OBJ :
    case VAR_STYPE :
    case VAR_COLOR :
    case VAR_PERCENT :
    case VAR_TERRAIN_FLAGS :
    case VAR_ARG_TYPE :
    case VAR_VARIABLE:
     *pntr = 0;
      break;
    case VAR_RACE:
      *pntr = (void*)MAX_PLYR_RACE;
      break;
    case VAR_PROG:
      delete (Tprog_Data*) *pntr;
      *(Tprog_Data**)pntr = 0;
      break;
    }
  }
  
  Table_Data *tab = table_addr( i, table_max[i] );
  tab->init( );

  ++table_max[i];

  for( int j = 0; j < MAX_SKILL_CAT; ++j ) {
    if( i == skill_table_number[ j ] ) {
      // Added an entry to a skill table; fix up trainers and shdatas.
      const int m = table_max[i];
      const int n = ( m + 31 ) / 32;
      if( n > ( m + 30 ) / 32 ) {
	for( trainer_data *trainer = trainer_list; trainer; trainer = trainer->next ) {
	  int *const temp = new int [ n ];
	  vcopy( temp, trainer->skills[j], n-1 );
	  temp[n-1] = 0;
	  delete [] trainer->skills[j];
	  trainer->skills[j] = temp;
	}
      }

      for( int k = 0; k < player_list; ++k ) {
	player_data *pl = player_list[k];
	if( !pl->Is_Valid( ) )
	  continue;
	unsigned char *const temp = new unsigned char [ m ];
	vcopy( temp, pl->shdata->skills[j], m-1 );
	temp[m-1] = 0;
	delete [] pl->shdata->skills[j];
	pl->shdata->skills[j] = temp;
      }

      for( int k = 0; k < mob_list; ++k ) {
	mob_data *mob = mob_list[k];
	if( !mob->Is_Valid( ) )
	  continue;
	if( mob->shdata != mob->species->shdata ) {
	  unsigned char *const temp = new unsigned char [ m ];
	  vcopy( temp, mob->shdata->skills[j], m-1 );
	  temp[m-1] = 0;
	  delete [] mob->shdata->skills[j];
	  mob->shdata->skills[j] = temp;
	}
      }

      for( int k = 1; k <= species_max; ++k ) {
	if( species_data *species = species_list[k] ) {
	  unsigned char *const temp = new unsigned char [ m ];
	  vcopy( temp, species->shdata->skills[j], m-1 );
	  temp[m-1] = 0;
	  delete [] species->shdata->skills[j];
	  species->shdata->skills[j] = temp;
	}
      }

      break;
    }
  }
}


/*
 *   SWAPPING OF ENTRIES
 */


static void swap_entries( int table, int e1, int e2 )
{
  const int           size  = table_size[table];
  void*        pntr1  = table_pntr( table,0,e1 );
  void*        pntr2  = table_pntr( table,0,e2 );
  wizard_data*   imm;
  
  char *temp = new char[size];
  
  memcpy( temp,  pntr1, size );
  memcpy( pntr1, pntr2, size );
  memcpy( pntr2, temp,  size );
  
  delete [] temp;
  
  for( int i = 0; i < player_list; ++i )
    if( player_list[i]->Is_Valid( ) 
	&& ( imm = wizard( player_list[i] ) )
	&& imm->table_edit[0] == table )
      exchange( imm->table_edit[1], e1, e2 ); 
  
  if( table == TABLE_SPELL_ACT ) {
    for( int i = 0; i < table_max[TABLE_SPELL_ACT]; ++i ) {
      for( int j = 0; j < MAX_SPELL_WAIT; ++j ) {
        exchange( skill_spell_table[i].action[j], e1, e2 );
      }
    }
  }
  /*
  else if( table == TABLE_MOVEMENT ) {
    for( int i = 1; i <= species_max; ++i ) {
      if( species_data *species = species_list[i] ) {
	exchange( species->movement, e1, e2 );
      }
    }
    for( int i = 0; i < player_list; ++i ) {
      player_data *pl = player_list[i];
      if( pl->Is_Valid( ) ) {
	exchange( pl->movement, e1, e2 );
      }
    }
  }
  */

  Tprog_Data **p1 = table_addr( table, e1 )->program();
  if( p1 && *p1 ) {
    (*p1)->entry = e2;
  }

  Tprog_Data **p2 = table_addr( table, e2 )->program();
  if( p2 && *p2 ) {
    (*p2)->entry = e1;
  }
}


/*
 *  DELETING ENTRIES
 */


static bool extract_nation( char_data* ch, int nation )
{
  if( nation+1 != table_max[ TABLE_NATION ] ) {
    send( ch, "You may only delete the last entry of that table.\n\r" );
    return false;
  }
  
  for( int i = 1; i <= species_max; ++i )
    if( species_list[i]
	&& species_list[i]->nation == nation ) {
      send( ch, "Mobs belonging to that nation still exist.\n\r" );
      return false;
    }
  
  return true;
}


/*
static bool extract_movement( char_data *ch, int movement )
{
  for( int i = 1; i <= species_max; ++i ) {
    if( species_data *species = species_list[i] ) {
      if( species->movement == movement )
	species->movement = -1;
    }
  }

  for( int i = 0; i < player_list; ++i ) {
    player_data *pl = player_list[i];
    if( pl->Is_Valid( )
	&& pl->movement == movement ) {
      pl->movement = -1;
    }
  }

  return true;
}
*/


static void sort_table( char_data* ch, int table )
{
  if( table > TABLE_SOC_VYAN
      && table != TABLE_COMMAND
      && table != TABLE_FUNCTION )
    return;

  const int max = table_max[table];

  for( int i = 0; i < max-1; ++i ) {
    int min = i;
    for( int j = i; j < max; ++j ) 
      if( strcasecmp( entry_name( table, j ), entry_name( table, min ) ) < 0 )
        min = j;
    if( min != i ) 
      swap_entries( table, i, min );
  }
  
  fsend( ch, "%s table sorted.", table_names[table] );
}


static void remove_entry( char_data* ch, int table, int entry )
{
  if( table > TABLE_SOC_VYAN
      && table != TABLE_COMMAND
      && table != TABLE_FUNCTION
      && table != TABLE_NATION
      && table != TABLE_HALLUCINATE ) {
    send( ch, "That table cannot have entries deleted from it.\n\r" );
    return;
  }
  
  if( table == TABLE_NATION && !extract_nation( ch, entry ) ) 
    return;
  
  //  if( table == TABLE_MOVEMENT && !extract_movement( ch, entry ) ) 
  //    return;
  
  fsend( ch, "Entry %s removed from table %s.",
	 entry_name( table, entry ), table_names[table] );
  
  wizard_data *imm;
  
  for( int i = 0; i < player_list; ++i ) {
    if( player_list[i]->Is_Valid( )
	&& ( imm = wizard( player_list[i] ) )
	&& imm->table_edit[0] == table
	&& imm->table_edit[1] == entry ) {
      send( imm, "The table entry you were editing was deleted.\n\r" );
      imm->table_edit[0] = -1;
    }
  }
  
  for( int i = entry+1; i < table_max[table]; i++ )
    swap_entries( table, i-1, i );

  --table_max[table];
}


/*
 *   DISK ROUTINES
 */


static void load_table( int table, bool spoof )
{
  fprintf( stdout, "  -%s\n\r", table_names[table] );

  FILE *fp = open_file( TABLE_DIR, table_names[table], "r", true );

  if( strcmp( fread_word( fp ), "#TABLE" ) ) 
    panic( "Load_tables: missing header" );

  table_max[table] = fread_number( fp );

  if( table_max[table] > table_abs_max[table] ) 
    panic( "Load_Tables: Number of entries in table %s > table max.", 
	   table_names[table] );

  for( int j = 0; j < table_max[table]; ++j )
    for( int k = 0; table_field[table][k]; ++k ) {
      char **pntr = (char**)table_pntr( table, k, j );
      int n = max( 0,table-MAX_PLYR_RACE );
      if( table_entry[n][k].load ) {
	switch( entry_type( table, k ) ) {
	case VAR_TEMP :
	  *(int *)pntr = 0;
	  break;
	    
	case VAR_BLANK :
	  *pntr = empty_string;
	  break;
	    
	case VAR_FORMULA :
	case VAR_CHAR :
	case CNST_CHAR : 
	case VAR_COMMAND :
	case VAR_FUNCTION :
	  *pntr = fread_string( fp, MEM_TABLE );
	  break;
	    
	case VAR_BOOL :
	  *(bool*)pntr = (bool) fread_number( fp );
	  break;

	case VAR_LEECH :
	case VAR_SIZE :
	case VAR_SA :
	case VAR_INT :
	case VAR_CC :
	case VAR_DICE :
	case VAR_STYPE :
	case VAR_ALIGN_FLAGS :
	case VAR_LANG_FLAGS :
	case VAR_AFF_LOC :
	case VAR_LOC_FLAGS :
	case VAR_CENT :
	case VAR_SEX :
	case VAR_CLASS_FLAGS :
	case VAR_SEX_FLAGS :
	case VAR_RACE_FLAGS :
	case VAR_RACE :
	case VAR_COLOR :
	case VAR_TERRAIN_FLAGS :
	case VAR_ARG_TYPE :
	  *(int *)pntr = fread_number( fp );
	  break;
	    
	case VAR_POS :
	  if( (*(int *)pntr = fread_number( fp ) ) < 0
	      || (*(int *)pntr ) >= MAX_POSITION )
	    panic( "Load_Tables: Impossible position." ); 
	  break;         
	    
	case VAR_DELETE :
	  fread_number( fp );
	  break;
	    
	case VAR_VARIABLE :
	  {
	    char *string = fread_string( fp, MEM_UNKNOWN );
	    if( string != empty_string ) {
	      const var_data *var = find_variable( string );
	      if( !var )
		  panic( "Load_Tables: Nonexistent variable." ); 
	      *(const var_data **)pntr = var;
	    }
	    free_string( string, MEM_UNKNOWN );
	  }
	  break;

	case VAR_SKILL :
	  {
	    char *string = fread_string( fp, MEM_UNKNOWN );
	    if( !spoof ) {
	      *(int *)pntr = skill_index( string );
	    }
	    free_string( string, MEM_UNKNOWN );
	  }
	  break;
	    
	case VAR_SPELL :
	  {
	    char *string = fread_string( fp, MEM_UNKNOWN );
	    if( !spoof ) {
	      *(int *)pntr = skill_index( string, SKILL_CAT_SPELL );
	    }
	    free_string( string, MEM_UNKNOWN );
	  }
	  break;
	    
	case VAR_PERM_FLAGS :
	case VAR_OBJ  :
	  *(int *)pntr     = fread_number( fp );
	  *((int *)pntr+1) = fread_number( fp );
	  break;
	    
	case VAR_AFF_FLAGS  :
	  *(int *)pntr     = fread_number( fp );
	  *((int *)pntr+1) = fread_number( fp );
	  *((int *)pntr+2) = fread_number( fp );
	  break;

	case VAR_PERCENT :
	  if( (*(int *)pntr = fread_number( fp ) ) < 0
	      || (*(int *)pntr ) > 100 )
	    panic( "Load_Tables: Impossible percentage." ); 
	  break;

	case VAR_RELIGIONS :
	  {
	    const int count = fread_number( fp );
	    if( !spoof && ( count < 0 || count >= table_max[ TABLE_RELIGION ] ) ) {
	      panic( "Load_Tables: Impossible religion count." );
	    }
	    for( int i = 0; i < count; ++i ) {
	      const int relig = fread_number( fp );
	      if( !spoof ) {
		if( relig < 0 || relig >= table_max[ TABLE_RELIGION ] ) {
		  panic( "Load_Tables: Impossible religion." );
		}
		( (Array<int>*) pntr )->append( relig );
	      }
	    }
	  }
	  break;

	case VAR_PROG:
	  {
	    Tprog_Data *prog = new Tprog_Data( table, j );
	    prog->read( fp );
	    if( prog->Code( ) != empty_string || !prog->Extra_Descr( ).is_empty( ) ) {
	      *(Tprog_Data**)pntr = prog;
	    } else {
	      delete prog;
	    }
	  }
	  break;
	}
      }
    }

  fclose( fp );

  if( !spoof ) {
    record_new( table_abs_max[table] * table_size[ table ], MEM_TABLE );
  }
}


void load_tables( )
{
  echo( "Loading Tables ...\n\r" );

  for( int i = 0; i < MAX_TABLE; ++i ) {
    for( int j = 0; j < table_abs_max[ i ]; ++j ) {
      Table_Data *tab = table_addr( i, j );
      tab->init( );
    }
  }

  for( int i = 0; i < MAX_SKILL_CAT; ++i ) {
    load_table( skill_table_number[ i ], true );
  }

  for( int i = 0; i < MAX_TABLE; ++i ) {
    load_table( i, false );
  }

  /*
  if( table_max[ TABLE_SKILL ] != MAX_SKILL ) {
    roach( "Load_Tables: Entries in skill table != max_skill." );
    roach( "-- Max_Skill = %d", MAX_SKILL );
    panic( "--   Entries = %d", table_max[ TABLE_SKILL ] );
  }
  */

  init_commands( );
  init_spells( );
  init_functions( );
}


void save_table( int num )
{
  rename_file( TABLE_DIR, table_names[num],
	       TABLE_PREV_DIR, table_names[num] );
  
  FILE *fp = open_file( TABLE_DIR, table_names[num], "w" );
  
  if( !fp )
    return;

  fprintf( fp, "#TABLE\n\n" );
  fprintf( fp, "%d\n\n", table_max[num] );
  
  char **pntr;

  for( int j = 0; j < table_max[num]; ++j ) {
    for( int k = 0; table_field[num][k] != '\0'; ++k ) {
      int n = max( 0,num-MAX_PLYR_RACE );
      if( table_entry[n][k].save ) {
	pntr = (char**)table_pntr( num,k,j );
	switch( entry_type( num,k ) ) {
	case VAR_DELETE :
	  break;
	  
	case VAR_BLANK :
	case CNST_CHAR :
	case VAR_COMMAND :
	case VAR_FUNCTION :
	case VAR_CHAR :
	case VAR_FORMULA :
	  fwrite_string( fp, *pntr );
	  break;
	  
	case VAR_BOOL :
	  fprintf( fp, "%d\n", (int)*(bool*)pntr ); 
	  break;

	case VAR_SIZE :
	case VAR_INT : 
	case VAR_SA :
	case VAR_CC:
	case VAR_POS:
	case VAR_DICE :
	case VAR_STYPE :
	case VAR_LEECH :
	case VAR_TEMP :
	case VAR_ALIGN_FLAGS :
	case VAR_LANG_FLAGS :
	case VAR_AFF_LOC :
	case VAR_LOC_FLAGS :
	case VAR_CENT :
	case VAR_SEX :
	case VAR_CLASS_FLAGS :
	case VAR_SEX_FLAGS :
	case VAR_RACE_FLAGS :
        case VAR_RACE :
        case VAR_COLOR :
	case VAR_PERCENT :
	case VAR_TERRAIN_FLAGS :
	case VAR_ARG_TYPE :
	  fprintf( fp, "%d\n", *(int *)pntr ); 
	  break;
	  
	case VAR_SKILL :
	case VAR_SPELL :
	  if( *(int *)pntr < 0 )
	    fwrite_string( fp, "" );
	  else
	    fwrite_string( fp, skill_entry( *(int *)pntr )->name );
	  break;
	  
	case VAR_VARIABLE :
	  {
	    const var_data *var = *(const var_data **)pntr;
	    if( !var  )
	      fwrite_string( fp, "" );
	    else
	      fwrite_string( fp, var->name );
	  }
	  break;
	  
	case VAR_PERM_FLAGS :
	case VAR_OBJ :
	  fprintf( fp, "%d %d\n", *(int *)pntr, *((int *)pntr+1) );
	  break;
	  
	case VAR_AFF_FLAGS:
	  fprintf( fp, "%d %d %d\n", *(int *)pntr,
		   *((int *)pntr+1), *((int *)pntr+2) );
	  break;

	case VAR_RELIGIONS :
	  {
	    int count = ( (Array<int>*) pntr )->size;
	    fprintf( fp, "%d", count );
	    for( int i = 0; i < count; ++i ) {
	      fprintf( fp, " %d", ( (Array<int>*) pntr )->list[i] );
	    }
	    fprintf( fp, "\n" );
	  }
	  break;

	case VAR_PROG:
	  {
	    Tprog_Data *prog = *(Tprog_Data**) pntr;
	    if( !prog ) {
	      fwrite_string( fp, empty_string );
	    } else {
	      prog->write( fp );
	    }
	  }
	}
      }
    }
    fprintf( fp, "\n" ); 
  }
  fprintf( fp, "\n" ); 
  fclose( fp );
}


void save_tables(  )
{
  for( int i = 0; i < MAX_TABLE; i++ ) {
    save_table( i );
  }
}


/*
 *   TEDIT ROUTINE
 */


void do_tedit( char_data* ch, const char *argument )
{
  wizard_data*  imm;
  int             i  = -1;
  int             j  = -1;

  if( !( imm = wizard( ch ) ) )
    return;

  if( !*argument ) {
    i = imm->table_edit[0];
    if( i != -1 ) {
      j = imm->table_edit[1];
      fsend( ch, "You stop editing table %s, entry %s.",
	     table_names[i], entry_name( i, j ) );
      imm->table_edit[0] = -1;
    } else {
      display_array( ch, "Tables", &table_names[0], &table_names[1],
		     MAX_TABLE, true, can_see );
    }
    return;
  }
  
  if( exact_match( argument, "new" ) ) {
    if( ( i = find_table( ch, argument, "Table not found.",
			  "Syntax: tedit new <table> <entry>" ) ) < 0 )
      return;
    if( !edit_table( ch, i ) )
      return;
    if( !*argument ) {
      send( ch, "You must specify a name for the new entry.\n\r" );
      return;
    }
    if( find_entry( ch, argument, i, 0 ) >= 0 ) {
      send( ch, "An entry already exists with that name.\n\r" );
      return;
    }
    if( table_max[i] == table_abs_max[i] ) {
      send( ch, "That table has no open slots.\n\r" );
      return;
    }
    init_entry( i, argument );
    imm->table_edit[0] = i;
    imm->table_edit[1] = table_max[i]-1;
    imm->textra_edit = 0;
    fsend( ch, "Table %s, entry %s added and you are now editing it.",
	   table_names[i], entry_name( i, imm->table_edit[1] ) );
    sort_table( ch, i );
    return;
  }
  
  if( exact_match( argument, "delete" ) ) {
    if( ( i = find_table( ch, argument, "Table not found." ) ) != -1 
	&& ( j = find_entry( ch, argument, i,
			     "Syntax: tedit delete <table> <name>" ) ) >= 0
	&& edit_table( ch, i ) ) 
      remove_entry( ch, i, j );
    return;
  }
  
  /*
  if( exact_match( argument, "sort" ) ) {
    if( ( i = find_table( ch, argument, "Table not found.",
			  "Which table do you wish to sort?" ) ) >= 0
	&& edit_table( ch, i ) )
      sort_table( ch, i );
    return;
  }
  */

  if( ( i = find_table( ch, argument, "Table not found." ) ) == -1 )
    return;
  
  if( !*argument ) {
    const int j = max( 0, i - MAX_PLYR_RACE );
    display_array( ch, table_names[i], 
		   (const char**) table_pntr( i,0,0 ), (const char**) table_pntr( i,0,1 ),
		   table_max[i], tedit_table[j].sort );
    page( ch, "\n\r%d entries.\n\r", table_max[i] );
    return;
  }

  if( ( j = find_entry( ch, argument, i ) ) == -1 )
    return;

  imm->table_edit[0] = i;
  imm->table_edit[1] = j;
  imm->textra_edit = 0;

  fsend( ch, "Tstat and tset now work on table %s, entry %s.",
	 table_names[i], entry_name( i, j ) );
 
  const int t = max( 0, i - MAX_PLYR_RACE );

  if( i != TABLE_TEDIT && tedit_table[t].lock ) {
    send( ch, "[ Warning: the %s table is locked. ]\n\r",
	  table_names[i] );
  }
} 


void do_tset( char_data* ch, const char *argument )
{
  char          arg  [ MAX_INPUT_LENGTH ];
  char          tmp  [ MAX_INPUT_LENGTH ];  
  void**       pntr;
  int          k, n;
  bool        error  = false;

  wizard_data *imm;
  if( !( imm = wizard( ch ) ) )
    return;

  const int i = imm->table_edit[0];
  const int j = imm->table_edit[1];

  if( i == -1 ) {
    send( ch, "You are not editing any table.\n\r" );
    return;
  }

  if( !*argument ) {
    do_tstat( ch, argument );
    return;
  }

  // In case table is locked...
  if( !edit_table( ch, i ) )
    return;

  argument = one_argument( argument, arg );

  int length = strlen( arg );
  const char *name;
  for( k = 0; ( name = table_field[i][k] ); ++k ) {
    if( *name == '*' )
      continue;
    if( *name == '+' )
      ++name;
    if( !strncasecmp( arg, name, length ) ) {
      break;
    }
  }
  
  if( !name ) {
    send( ch, "Syntax: tset <field> <value>\n\r" );
    return;
  }
  
  if( k == 0 && !*argument ) {
    send( ch, "You cannot set the %s field blank.\n\r", name );
    return;
  }

  pntr = (void**) table_pntr( i,k,j );

  switch( entry_type( i,k ) ) {
  case VAR_TEMP :
    send( ch, "That is a unsorted variable and may not be set.\n\r" );
    return;
    
  case VAR_SIZE : {
    class type_field size_field = {
      "size", MAX_SIZE, &size_name[0], &size_name[1], (int*) pntr, true };
    pntr = (void**)table_pntr( i,0,j );
    size_field.set( ch, (char*) *pntr, argument ); 
    return;
  }
  
  case VAR_STYPE : {
    class type_field stype_field = {
      "type", MAX_STYPE, &stype_name[0], &stype_name[1], (int*) pntr, true };
    pntr = (void**)table_pntr( i,0,j );
    stype_field.set( ch, (char*) *pntr, argument ); 
    return;
  }
  
  case VAR_ARG_TYPE : {
    class type_field arg_type_field = {
      "type", MAX_ARG, &arg_type_name[0], &arg_type_name[1], (int*) pntr, true };
    pntr = (void**)table_pntr( i,0,j );
    arg_type_field.set( ch, (char*) *pntr, argument ); 
    return;
  }
  
  case VAR_AFF_LOC : {
    class type_field loc_field = {
      "location", MAX_AFF_LOCATION, &affect_location[0], &affect_location[1],
      (int*) pntr, true };
    pntr = (void**)table_pntr( i,0,j );
    loc_field.set( ch, (char*) *pntr, argument ); 
    return;
  }

  case VAR_BOOL :
    set_bool( ch, argument, name, *(bool*)pntr ); 
    return;

  case VAR_ALIGN_FLAGS :
    abv_align_flags.set( ch, argument, name, (int*) pntr );
    return;     

  case VAR_CLASS_FLAGS :
    class_flags.set( ch, argument, name, (int*) pntr ); 
    return;
    
  case VAR_LANG_FLAGS :
    lang_flags.set( ch, argument, name, (int*) pntr );
    return;      

  case VAR_AFF_FLAGS :
    affect_flags.set( ch, argument, name, (int*) pntr );
    return;

  case VAR_PERM_FLAGS :
    permission_flags.set( ch, argument, name, (int*) pntr, imm->pcdata->pfile->permission );
    return;

  case VAR_LOC_FLAGS :
    location_flags.set( ch, argument, name, (int*) pntr );
    return;

  case VAR_SEX_FLAGS :
    sex_flags.set( ch, argument, name, (int*) pntr );
    return;

  case VAR_RACE_FLAGS :
    race_flags.set( ch, argument, name, (int*) pntr );
    return;

  case VAR_TERRAIN_FLAGS :
    terrain_flags.set( ch, argument, name, (int*) pntr ); 
    return;
    
  case VAR_POS : {
    class type_field pos_field = {
      "position", MAX_POSITION,
      &pos_name[0], &pos_name[1], (int*) pntr, true };
    pntr = (void**)table_pntr( i,0,j );
    pos_field.set( ch, (char*) *pntr, argument ); 
    return;
  }

  case VAR_SEX : {
    class type_field sex_field = {
      "sex", MAX_SEX-1,
      &sex_name[0], &sex_name[1], (int*) pntr, true };
    pntr = (void**)table_pntr( i,0,j );
    sex_field.set( ch, (char*) *pntr, argument ); 
    return;
  }

  case VAR_FORMULA :
    evaluate( argument, error );
    if( error ) {
      send( ch, "Expression fails to evaluate.\n\r" );
      return;
    }
    free_string( (char*) *pntr, MEM_TABLE );
    *(char**)pntr = alloc_string( argument, MEM_TABLE );
    snprintf( tmp, MAX_INPUT_LENGTH, "%s on %s %s set to:\n\r%s\n\r",
	      name, table_names[i], *(char**)table_pntr( i,0,j ), argument );
    break;
    
  case VAR_COMMAND:
    if( !*argument ) {
      display_commands( ch, "Command Functions" );
      return;
    } else if( !strcasecmp( argument, "none" ) ) {
      free_string( (char*) *pntr, MEM_TABLE );
      *(char**)pntr = empty_string;
      snprintf( tmp, MAX_INPUT_LENGTH, "%s on %s %s set to none.\n\r",
		name, table_names[i], *(char**)table_pntr( i,0,j ) );
      command_table[j].function = 0;
    } else {
      free_string( (char*) *pntr, MEM_TABLE );
      *(char**)pntr = alloc_string( argument, MEM_TABLE );
      snprintf( tmp, MAX_INPUT_LENGTH, "%s on %s %s set to:\n\r%s\n\r",
		name, table_names[i], *(char**)table_pntr( i,0,j ), argument );
      if( i == TABLE_COMMAND
	  && table_entry[ i-MAX_PLYR_RACE ][k].offset == &command_table[0].func_name ) {
	init_command( j );
	if( !command_table[j].function ) {
	  fsend( ch, "Warning: function \"%s\" not found!", argument );
	}
      }
    }
    break;
    
  case VAR_FUNCTION:
    if( !*argument ) {
      display_functions( ch, "Hardcode Functions" );
      return;
    } else if( !strcasecmp( argument, "none" ) ) {
      free_string( (char*) *pntr, MEM_TABLE );
      *(char**)pntr = empty_string;
      snprintf( tmp, MAX_INPUT_LENGTH, "%s on %s %s set to none.\n\r",
		name, table_names[i], *(char**)table_pntr( i,0,j ) );
      function_table[j].function = 0;
    } else {
      free_string( (char*) *pntr, MEM_TABLE );
      *(char**)pntr = alloc_string( argument, MEM_TABLE );
      snprintf( tmp, MAX_INPUT_LENGTH, "%s on %s %s set to:\n\r%s\n\r",
		name, table_names[i], *(char**)table_pntr( i,0,j ), argument );
      if( i == TABLE_FUNCTION
	  && table_entry[ i-MAX_PLYR_RACE ][k].offset == &function_table[0].func_name ) {
	init_function( j );
	if( !function_table[j].function ) {
	  fsend( ch, "Warning: function \"%s\" not found!", argument );
	}
      }
    }
    break;

  case VAR_BLANK :
  case VAR_CHAR :
    free_string( (char*) *pntr, MEM_TABLE );
    *(char**)pntr = alloc_string( argument, MEM_TABLE );
    pntr = (void**)table_pntr( i,0,j );
    snprintf( tmp, MAX_INPUT_LENGTH, "%s on %s %s set to:\n\r%s\n\r",
	      name, table_names[i], (char*) *pntr, argument );
    break;
    
  case VAR_INT :
    n            = atoi( argument );
    *(int *)pntr = n;
    pntr = (void**)table_pntr( i,0,j );
    snprintf( tmp, MAX_INPUT_LENGTH, "%s on %s %s set to %d.\n\r",
	      name, table_names[i], (char*) *pntr, n );
    break;
    
  case VAR_CENT :
    n            = (int) (100*atof( argument ));
    *(int *)pntr = n;
    pntr = (void**)table_pntr( i,0,j );
    snprintf( tmp, MAX_INPUT_LENGTH, "%s on %s %s set to %.2f.\n\r",
	      name, table_names[i], (char*) *pntr,
	      (double)n/100 );
    break;
    
  case VAR_CC : {
    class type_field cat_field = {
      "category", table_max[ TABLE_CMD_CAT ],
      &cmd_cat_table[0].name, &cmd_cat_table[1].name, (int*) pntr, true };
    pntr = (void**)table_pntr( i,0,j );
    cat_field.set( ch, (char*) *pntr, argument ); 
    return;
  } 
    
  case VAR_SKILL :
    {
      if( !*argument ) {
	display_skills( ch );
	return;
      }
      n = find_skill( argument );
      if( n == -1 && strcasecmp( argument, "none" ) ) {
	send( ch, "Unknown skill.\n\r" );
	return;
      }
      *(int *)pntr = n;
      pntr = (void**)table_pntr( i,0,j );
      snprintf( tmp, MAX_INPUT_LENGTH, "%s on %s %s set to %s.\n\r",
		name, table_names[i], (char*) *pntr,
		n == -1 ? "none" : skill_entry( n )->name );
    }
    break;

  case VAR_SPELL :
    {
      if( !*argument ) {
	display_skills( ch, SKILL_CAT_SPELL );
	return;
      }
      n = find_skill( argument, SKILL_CAT_SPELL );
      if( n == -1 && strcasecmp( argument, "none" ) ) {
	send( ch, "Unknown spell.\n\r" );
	return;
      }
      *(int *)pntr = n;
      pntr = (void**)table_pntr( i,0,j );
      snprintf( tmp, MAX_INPUT_LENGTH, "%s on %s %s set to %s.\n\r",
		name, table_names[i], (char*) *pntr,
		n == -1 ? "none" : skill_entry( n )->name );
    }
    break;

  case VAR_VARIABLE :
    {
      if( !*argument ) {
	display_array( ch, "Variables",
		       &variable_list[0].name, &variable_list[1].name,
		       -1, true );
	return;
      }
      const var_data *var = find_variable( argument );
      if( !var && strcasecmp( argument, "none" ) ) {
	send( ch, "Unknown variable.\n\r" );
	return;
      }
      *(const var_data **)pntr = var;
      pntr = (void**)table_pntr( i,0,j );
      snprintf( tmp, MAX_INPUT_LENGTH, "%s on %s %s set to %s.\n\r",
		name, table_names[i], (char*) *pntr,
		var ? var->name : "none" );
    }
    break;

  case VAR_SA :
    {
      // Find the shortest spell.affects name that completely matches.
      int x = 0;
      n = -1;
      if( ( length = strlen( argument ) ) ) {
	for( int e = 0; e < table_max[TABLE_SPELL_ACT]; ++e ) {
	  if( !strncasecmp( argument, spell_act_table[e].name, length ) ) {
	    int y = strlen( spell_act_table[e].name );
	    if( n < 0 || y < x ) {
	      n = e;
	      x = y;
	    }
	  }
	}
      }
      if( n == -1 ) {
	send( ch, "No spell action matching that name found.\n\r" );
	return;
      }     
      *(int *)pntr = n;
      pntr = (void**)table_pntr( i,0,j );
      snprintf( tmp, MAX_INPUT_LENGTH, "%s on %s %s set to %s.\n\r",
		name, table_names[i], (char*) *pntr,
		spell_act_table[n].name );
    }
    break;

    /*
  case VAR_SCAT : {
    class type_field scat_field = {
      "category", MAX_SKILL_CAT,
      &skill_cat_name[0],  &skill_cat_name[1], (int*) pntr, true };
    (void*) pntr = table_pntr( i,0,j );
    scat_field.set( ch, (char*) *pntr, argument ); 
    return;
  }
    */

  case VAR_OBJ :
    if( !strcasecmp( argument, "nothing" ) ) {
      send( ch, "Field set to nothing.\n\r" );
      *(int *)pntr     = 0;
      *((int *)pntr+1) = 1; 
      return;
    }
    {
      int n = 1;
      number_arg( argument, n );
      obj_data *obj;
      if( !( obj = one_object( ch, argument, "tset", &ch->contents ) ) ) {
	return;
      }
      *(int *)pntr     = obj->pIndexData->vnum;
      *((int *)pntr+1) = n;
      fsend( ch, "Object set to %s.", obj->pIndexData->Name( n ) );
    }
    return;
    
  case VAR_LEECH :
  case VAR_DICE : {
    class dice_field entry = {
      name, LEVEL_MOB, (int*) pntr };
    pntr = (void**)table_pntr( i,0,j );
    entry.set( ch, (char*) *pntr, argument );
    return;
  }
    
  case VAR_RACE : {
    class type_field race_field = {
      name, table_max[TABLE_RACE],
      &race_table[0].name,  &race_table[1].name, (int*) pntr, true };
    pntr = (void**)table_pntr( i,0,j );
    race_field.set( ch, (char*) *pntr, argument ); 
    return;
  }

  case CNST_CHAR :
  case VAR_PROG :
    send( ch, "That entry is not setable.\n\r" );
    return;

  case VAR_COLOR : {
    class type_field col_field = {
      "color", MAX_COLOR,
      &color_fields[0], &color_fields[1], (int*) pntr, true };
    pntr = (void**)table_pntr( i,0,j );
    col_field.set( ch, (char*) *pntr, argument ); 
    return;
  }
    
  case VAR_PERCENT :
    n            = atoi( argument );
    *(int *)pntr = n;
    pntr = (void**)table_pntr( i,0,j );
    snprintf( tmp, MAX_INPUT_LENGTH, "%s on %s %s set to %d%%.\n\r",
	      name, table_names[i], (char*) *pntr, n );
    break;
    
  case VAR_RELIGIONS :
    {
      if( !*argument ) {
	display_array( ch, "+Religions",
		       &religion_table[1].name, &religion_table[2].name,
		       table_max[ TABLE_RELIGION ] - 1 );
	return;
      }

      n = find_entry( ch, argument, TABLE_RELIGION );
      if( n < 0 )
	return;
      
      Array<int>* p = (Array<int>*) pntr;
      pntr = (void**)table_pntr( i,0,j );
      int x = p->find( n );
      if( x >= 0 ) {
	snprintf( tmp, MAX_INPUT_LENGTH, "%s on %s %s: removed %s.\n\r",
		  name, table_names[i], (char*) *pntr, religion_table[n].name );
	p->remove( x );
      } else {
	for( x = 0; x < p->size; ++x ) {
	  if( p->list[i] > n )
	    break;
	}
	snprintf( tmp, MAX_INPUT_LENGTH, "%s on %s %s: added %s.\n\r",
		  name, table_names[i], (char*) *pntr, religion_table[n].name );
	p->insert( n, x );
      }
    }
    break;
  }

  *tmp = toupper( *tmp );
  send( ch, tmp );  

  if( k == 0 ) {
    // Changed entry name, re-sort table if necessary.
    sort_table( ch, i );
  }
}


void do_tstat( char_data* ch, const char *)
{
  char                 tmp  [ FOUR_LINES ];
  wizard_data*         imm;
  if( !(imm = wizard( ch )) )
    return;
  obj_clss_data*  obj_clss;
  int                table  = imm->table_edit[0];
  int                entry  = imm->table_edit[1];
  int            col, k, n;
  void**              pntr;

  if( table == -1 ) {
    send( ch, "You are not editing any table.\n\r" );
    return;
  }

  page_underlined( ch, "Table %s, Entry %s\n\r",
		   table_names[table], entry_name( table, entry ) );
  
  const bool single = tables[table][0].single( );

  /*
    bool single = ( table != TABLE_SKILL
    && table != TABLE_CLSS
    && table != TABLE_PLYR_RACE
    && table != TABLE_NATION );
  */
  
  const char *name;

  for( k = col = 0; ( name = table_field[table][k] ); ++k ) {
    if( *name == '*' ) {
      continue;
    }
    if( *name == '+' ) {
      ++name;
      if( !single && col%2 == 1 ) {
	page( ch, "\n\r" );
	++col;
      }
    }

    pntr = (void**)table_pntr( table, k, entry );
    char *t = tmp + snprintf( tmp, FOUR_LINES, "%15s : ", name );

    switch( entry_type( table,k ) ) {
    case VAR_FORMULA :
    case VAR_CHAR :
    case CNST_CHAR :
    case VAR_COMMAND :
    case VAR_FUNCTION :
    case VAR_BLANK :
      strcpy( t, (char*) *pntr );
      break;

    case VAR_BOOL :
      sprintf( t, "%s", *(bool*)pntr ? "true" : "false" ); 
      break; 
   
    case VAR_TEMP :
    case VAR_INT :
      sprintf( t, "%d", (int) *pntr );
      break;
   
    case VAR_CENT :
      n = *(int *)pntr; 
      sprintf( t, "%.2f", (double)n/100.0 );
      break;
   
    case VAR_AFF_LOC :
      strcpy( t, affect_location[ *(int *)pntr ] );
      break;

    case VAR_SIZE :
      strcpy( t, size_name[ *(int *)pntr ] );
      break;

      /*
    case VAR_SCAT :
      strcpy( t, skill_cat_name[ *(int *)pntr ] );
      break;
      */

    case VAR_STYPE :
      strcpy( t, stype_name[ *(int *)pntr ] );
      break;

    case VAR_ARG_TYPE :
      strcpy( t, arg_type_name[ *(int *)pntr ] );
      break;

    case VAR_SKILL :
    case VAR_SPELL :
      sprintf( t, "%s", *(int *)pntr == -1 
	       ? "none" : skill_entry( *(int *)pntr )->name );
      break;

    case VAR_VARIABLE :
      {
	const var_data *var = *(const var_data **)pntr;
	sprintf( t, "%s", var ? var->name  : "none" );
      }
      break;

    case VAR_SA :
      if( *(int *)pntr >= 0 && *(int *)pntr < table_max[ TABLE_SPELL_ACT ] )   
	strcpy( t, spell_act_table[ *(int *)pntr].name );
      else
	sprintf( t, "%d (Bug)", *(int *)pntr );
      break;

    case VAR_CC :
      strcpy( t, cmd_cat_table[*(int *)pntr].name );
      break;

    case VAR_POS :
      strcpy( t, pos_name[ *(int *)pntr ] );
      break;

    case VAR_SEX :
      strcpy( t, sex_name[ *(int *)pntr ] );
      break;

    case VAR_OBJ :
      obj_clss = get_obj_index( *(int *)pntr );
      strcpy( t, obj_clss ?
	      obj_clss->Name( *((int *)pntr+1) ) : "nothing" );
      break;

    case VAR_AFF_FLAGS :
      affect_flags.sprint( t, (int*) pntr );
      break;

    case VAR_ALIGN_FLAGS :
      abv_align_flags.sprint( t, (int*) pntr );
      break;

    case VAR_CLASS_FLAGS :
      class_flags.sprint( t, (int*) pntr );
      break;

    case VAR_LANG_FLAGS :
      lang_flags.sprint( t, (int*) pntr );
      break;

    case VAR_PERM_FLAGS :
      permission_flags.sprint( t, (int*) pntr );
      break;

    case VAR_TERRAIN_FLAGS :
      terrain_flags.sprint( t, (int*) pntr );
      break;

    case VAR_LOC_FLAGS :
      if( *((int*) pntr) == 0 ) 
	strcpy( t, "anywhere" ); 
      else
	location_flags.sprint( t, (int*) pntr );
      break;

    case VAR_SEX_FLAGS :
      sex_flags.sprint( t, (int*) pntr );
      break;

    case VAR_RACE_FLAGS :
      race_flags.sprint( t, (int*) pntr );
      break;

    case VAR_DICE :
      sprintf_dice( t, *(int *)pntr );
      break; 

    case VAR_LEECH :
      sprintf_leech( t, *(int *)pntr );
      break;

    case VAR_RACE :
      strcpy( t, race_table[ *(int *)pntr ].name );
      break;

    case VAR_COLOR :
      strcpy( t, color_fields[ *(int *)pntr ] );
      break;

    case VAR_PERCENT :
      sprintf( t, "%d%%", (int) *pntr );
      break;

    case VAR_RELIGIONS:
      {
	Array<int>* p = (Array<int>*) pntr;
	if( p->is_empty( ) ) {
	  sprintf( t, "any" );
	} else {
	  const int x = p->size;
	  for( int i = 0; i < x; ++i ) {
	    if( i != 0 ) {
	      t += sprintf( t, ", " );
	    }
	    t += sprintf( t, religion_table[ p->list[i] ].name );
	  }
	}
      }
      break;

    case VAR_PROG :
      continue;
    }

    if( single ) {
      page( ch, "%s\n\r", tmp );
    } else if( strlen( tmp ) > 36 ) {
      page( ch, "%s  %s\n\r", col%2 == 1 ? "\n\r" : "", tmp );  
      col = 1;
    } else if( col%2 != 1 ) {
      page( ch, "  %-36s", tmp );
    } else {
      page( ch, "  %s\n\r", tmp );
    }

    ++col;
  }
  
  if( !single && col%2 == 1 )
    page( ch, "\n\r" );

  if( table == TABLE_SPELL_ACT ) { 
    bool found = false;
    
    page( ch, "\n\rUsed By:\n\r" );
    
    for( int i = 0; i < table_max[TABLE_SKILL_SPELL]; ++i )
      for( int j = 0; j < skill_spell_table[i].wait; j++ )
	if( skill_spell_table[i].action[j] == entry ) {
	  found = true;
	  page( ch, "  %s\n\r", skill_spell_table[i].name );
	  break;
	}
    
    if( !found ) 
      page( ch, "  nothing\n\r" );
  }

  Table_Data *t = table_addr( table, entry );

  if( Tprog_Data **p = t->program() ) {
    if( *p ) {
      page( ch, "\n\r[Code]\n\r%s\n\r", (*p)->Code( ) );
      show_extras( ch, (*p)->Extra_Descr( ) );
    } else {
      page( ch, "\n\r[Code]\n\r\n\r" );
    }
  }
}


// Support for rowhere command.
bool table_rowhere( char_data *ch, int index )
{
  const char *name;
  bool found = false;

  for( int i = 0; i < MAX_TABLE; ++i ) {
    for( int k = 0; ( name = table_field[i][k] ); ++k ) {
      if( entry_type( i, k ) == VAR_OBJ ) {
	if( *name == '+' || *name == '*' )
	  ++name;
	for( int j = 0; j < table_max[i]; ++j ) {
	  int vnum = *(int *)table_pntr( i, k, j );
	  if( vnum == index ) {
	    page( ch, "  in table %s, entry %s, field %s.\n\r",
		  table_names[i], entry_name( i, j ), table_field[i][k] );
	    found = true;
	  }
	}
      }
    }
  }

  return found;
}
