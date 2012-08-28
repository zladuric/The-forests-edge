#ifndef tfe_struct_h
#define tfe_struct_h

/*
 *   DEFINED TYPES
 */


class action_data;
class affect_data;
class alias_data;
class area_data;
class auction_data;
class cast_data;
class char_data;
typedef class   Clan_Data          clan_data;
typedef class   Command_Data       command_data;
typedef class   Command_Queue      command_queue;
typedef class   Const_Data         const_data;
typedef class   Custom_Data        custom_data;
typedef class   Descr_Data         descr_data;
typedef class   Dice_Data          dice_data;
typedef class   Enemy_Data         enemy_data;
class event_data;
typedef class   Exit_Data          exit_data;
typedef class   Extra_Data         extra_data;
typedef class   Clan_Data          clan_data;
typedef class   Help_Data          help_data;
//typedef class   Index_Data         index_data;
typedef class   Info_Data          info_data;
//typedef class   Link_Data          link_data;
typedef class   List_Data          list_data;
typedef class   Mob_Data           mob_data;
typedef class   Mprog_Data         mprog_data;
typedef class   Note_Data          note_data;
typedef class   Obj_Data           obj_data;
typedef class   Obj_Clss_Data      obj_clss_data;
typedef class   Oprog_Data         oprog_data;
typedef class   Path_Data          path_data;
typedef class   Pc_Data            pc_data;
typedef class   Pfile_Data         pfile_data;
class player_data;
typedef class   Recipe_Data        recipe_data;
typedef class   Reput_Data         reput_data;
typedef class   Request_Data       request_data;
typedef class   Reset_Data         reset_data;
typedef class   Room_Data          room_data;
typedef class   Rtable_Data        rtable_data;
typedef class   Share_Data         share_data;
typedef class   Shop_Data          shop_data;
typedef class   Skill_Type         skill_type;
typedef class   Species_Data       species_data;
typedef class   Tell_Data          tell_data;
//typedef class   Text_Data          text_data;
class thing_data;
typedef class   Time_Data          time_data;
typedef class   Term_Type          term_type;
typedef class   Title_Array        title_array;
typedef class   Title_Data         title_data;
typedef class   Trainer_Data       trainer_data;
typedef class   Var_Data           var_data;
class visible_data;
typedef class   Wizard_Data        wizard_data;


/*
 *   BASIC HEADERS
 */


#include "ctype.h"
#include "math.h"
#include "string.h"

#include "macros.h"

#include "bit.h"
#include "memory.h"
#include "string2.h"


/* 
 *   CLASS TEMPLATES
 */


#include "array.h"


class action_array: public Array<action_data*> {};
class affect_array: public Array<affect_data*> {};
class alias_array: public Array<alias_data*> {};
class auction_array: public Array<auction_data*> {};
class char_array: public Array<char_data*> {};
class event_array: public Array<event_data*> {};
class exit_array: public Array<Exit_Data*> {};
class obj_array: public Array<Obj_Data*> {};
class pfile_array: public Array<Pfile_Data*> {};
class request_array: public Array<Request_Data*> {};
class room_array: public Array<Room_Data*> {};
class extra_array: public Array<Extra_Data*> {};
class visible_array: public Array<visible_data*> {};
class string_array: public Array<const char *> {};
class thing_array: public Array<thing_data*> {};


/*
 *   FUNCTION TYPE
 */


typedef void         do_func     ( char_data*, const char *argument );
typedef bool         spell_func  ( char_data*, char_data*, void*, int, int );
typedef bool         path_func   ( char_data*, thing_data*, const char*,
				   int = 0, int = 0,
				   int = 100, int = 50 );
typedef bool         set_func    ( char_data*, const char *, const char*& );
typedef thing_data*  thing_func  ( thing_data*, char_data*, thing_data* = 0 );
typedef void         event_func  ( event_data* );


class Info_Data
{
public:
  info_data *next;
  const char *string1;
  const char *string2;
  int none;
  int level;
  int type;
  clan_data *clan;
  pfile_data *pfile;
  time_t when;
  
  Info_Data( const char *s1, const char *s2,
	     int n, int l, int t,
	     clan_data *c, pfile_data *p );
  ~Info_Data( );
};


class Save_Data
{
public:
  Save_Data( )
    : fixed( false ) { }
  virtual ~Save_Data( );

  obj_array save_list;
  bool fixed;

  virtual void Save ( bool = true ) = 0;
};


class save_array: public Array<Save_Data *>
{
public:
  void Setup( Save_Data* );
  void Save( );
};


class Help_Data
{
public:
  const char*      name;
  const char*      text;
  const char*  immortal;
  int       level  [ 2 ];
  int    category;
  time_t when;
  const char *by;

  Help_Data( );
  ~Help_Data( );
  
  friend const char* name( help_data* help ) {
    return help->name;
  }
};


extern int max_help;
extern help_data **help_list;


extern save_array saves;


#include "general.h"
#include "number.h"
#include "update.h"
#include "affect.h"
#include "file.h"
#include "thing.h"
#include "program.h"
#include "object.h"

#include "flag.h"

#include "account.h"
#include "weather.h"
#include "imm.h"
#include "code.h"
#include "table.h"
#include "abil.h"
#include "dictionary.h"

#include "event.h"
#include "interp.h"

#include "quest.h"
#include "char.h"
#include "pos.h"
#include "wear.h"

#include "network.h"
#include "option.h"

#include "color.h"
#include "player.h"

#include "auction.h"
#include "cards.h"
#include "channel.h"
#include "clan.h"
#include "group.h"
#include "liquid.h"
#include "list.h"
#include "look.h"
#include "magic.h"
#include "move.h"
#include "note.h"
#include "library.h"
#include "mob.h"
#include "reset.h"
#include "reput.h"
#include "room.h"
#include "shop.h"
#include "skill.h"
#include "thief.h"
#include "weight.h"

#include "output.h"
#include "log.h"
#include "set.h"
#include "fight.h"


/*
 *   GENERAL CLASSES/STRUCTURES
 */


/*
 *   GLOBAL CONSTANTS
 */


extern  const   char   *act_name [];
extern  const   char   *action_trigger [];
extern  const   char   *affect_location [];
extern  const   char   *anti_flags [];
extern  const   char   *coin_name [];
extern  const   int    coin_value [];
extern  const   int    coin_vnum  [];
extern  const   char   *cont_flag_name [];
extern  const   char   *consume_flags [];
extern  const   int    default_weight [];
extern  const   char   *dflag_name [];
extern  const   char   *dflag_name [];
extern  const   char   *gem_quality [];
extern  const   char   *item_type_name [];
extern  const   char   *oflag_ident [];
extern  const   char   *oflag_name [];
extern  const   char   *mprog_trigger [];
extern  const   char   *mprog_value [ MAX_MPROG_TRIGGER ];
extern  const   char   *oprog_trigger [ MAX_OPROG_TRIGGER ];
extern  const   char   *plr_name [];
extern  const   char   *pos_name [];
extern  const   char   *rflag_name [];
extern  const   char   *restriction_flags [];
extern  const   char   *restrict_ident [];
extern  const   char   *size_name [];
extern  const   char   *sex_name [];
extern  const   char   *hand_name [];
extern  const   char   *size_flags []; 
extern  const   char   *trap_flags [];
extern  const   char   *where_name []; 

extern int max_pfile;

extern pfile_data** pfile_list;

extern const char *const bad_command;

extern string_array extract_strings;
extern event_array extract_events;

extern char *tz;

extern const char *const find_one;
extern const char *const find_zero;
extern const char *const find_keyword;
extern const char *const find_few;


/*
 *   GLOBAL FUNCTIONS
 */


do_func do_abilities;
do_func do_accounts;
do_func do_acode;
do_func do_adata;
do_func do_adminchan;
do_func do_advance;
do_func do_aedit;
do_func do_affects;
do_func do_aflag;
do_func do_alias;
do_func do_allegiance;
do_func do_appearance;
do_func do_appraise;
do_func do_approve;
do_func do_areas;
do_func do_aset;
do_func do_ask;
do_func do_assist;
do_func do_astat;
do_func do_at;
do_func do_atalk;
do_func do_auction;
do_func do_avatar;
do_func do_backstab;
do_func do_balance;
do_func do_bamfin;
do_func do_bamfout;
do_func do_ban;
do_func do_bandage;
do_func do_bash;
do_func do_befriend;
do_func do_beep;
do_func do_butcher;
do_func do_define;
do_func do_berserk;
do_func do_bid;
do_func do_bite;
do_func do_build;
do_func do_buildchan;
do_func do_bugs;
do_func do_bury;
do_func do_buy;
do_func do_calculate;
do_func do_camouflage;
do_func do_camp;
do_func do_cast;
do_func do_cedit;
do_func do_cflag;
do_func do_changes; 
do_func do_chant;
do_func do_chat;
do_func do_charge;
do_func do_clans;
do_func do_climb;
do_func do_close;
do_func do_color;
do_func do_commands;
do_func do_compare;
do_func do_compile;
do_func do_consent;
do_func do_consider;
do_func do_constants;
do_func do_cook;
do_func do_cover;
do_func do_ctell;
do_func do_custom;
do_func do_cwhere;
do_func do_deal;
do_func do_descript;
do_func do_dedit;
do_func do_delete;
do_func do_deny;
do_func do_deposit;
do_func do_dflag;
do_func do_dictionary;
do_func do_dig;
do_func do_disguise;
do_func do_dip;
do_func do_disarm;
do_func do_disconnect;
do_func do_dismount;
do_func do_dock;
do_func do_down;
do_func do_drink;
do_func do_drop;
do_func do_dset;
do_func do_dstat;
do_func do_east;
do_func do_eat;
do_func do_echo;
do_func do_edit;
do_func do_emote;
do_func do_empty;
do_func do_energize;
do_func do_enter;
do_func do_equipment;
do_func do_exits;
do_func do_exp;
do_func do_extract;
do_func do_fill;
do_func do_filter;
do_func do_flee;
do_func do_focus;
do_func do_follow;
do_func do_forage;
do_func do_force;
do_func do_freeze;
do_func do_functions;
do_func do_fwhere;
do_func do_garrote;
do_func do_get;
do_func do_give;
do_func do_glance;
do_func do_god;
do_func do_gossip;
do_func do_goto;
do_func do_guard;
do_func do_group;
do_func do_gtell;
do_func do_hands;
do_func do_hbug;
do_func do_hdesc;
do_func do_hedit;
do_func do_help;
do_func do_heist;
do_func do_hfind;
do_func do_hide;
do_func do_high;
do_func do_holylight;
do_func do_homepage;
do_func do_hset;
do_func do_hstat;
do_func do_identity;
do_func do_identify;
do_func do_iflag;
do_func do_ignite;
do_func do_immtalk;
do_func do_imprison;
do_func do_index;
do_func do_info;
do_func do_inspect;
do_func do_introduce;
do_func do_inventory;
do_func do_invis;
do_func do_journal;
do_func do_junk;
do_func do_keywords;
do_func do_kick;
do_func do_kill;
do_func do_knock;
do_func do_label;
do_func do_lag;
do_func do_language;
do_func do_last;
do_func do_ledit;
do_func do_leech;
do_func do_level;
do_func do_list;
do_func do_load;
do_func do_lock;
do_func do_log;
do_func do_look;
do_func do_lost;
do_func do_lset;
do_func do_lstat;
do_func do_mail;
do_func do_map;
do_func do_marmor;
do_func do_mbug;
do_func do_mdesc;
do_func do_medit;
do_func do_meditate;
do_func do_melee;
do_func do_melt;
do_func do_memory;
do_func do_message;
do_func do_mextra;
do_func do_mfind;
do_func do_mflag;
do_func do_miflag;
do_func do_miset;
do_func do_mistat;
do_func do_mload;
do_func do_mlog;
do_func do_motd;
do_func do_mount;
do_func do_move;
do_func do_movement;
do_func do_mpcode;
do_func do_mpdata;
do_func do_mpedit;
do_func do_mpflag;
do_func do_mpset;
do_func do_mpstat;
do_func do_mreset;
do_func do_mset;
do_func do_mskill;
do_func do_mstat;
do_func do_mwhere;
do_func do_name;
do_func do_newbie;
do_func do_noemote;
do_func do_north;
do_func do_noshout;
do_func do_notell;
do_func do_notes;
do_func do_obug;
do_func do_odesc;
do_func do_oedit;
do_func do_oextra;
do_func do_ofind;
do_func do_oflag;
do_func do_oiflag;
do_func do_oiset;
do_func do_oistat;
do_func do_oload;
do_func do_olog;
do_func do_ooc;
do_func do_opcode;
do_func do_opdata;
do_func do_opedit;
do_func do_opflag;
do_func do_opset;
do_func do_opstat;
do_func do_open;
do_func do_options;
do_func do_order;
do_func do_oset;
do_func do_ostat;
do_func do_owhere;
do_func do_pardon;
do_func do_password;
do_func do_path;
do_func do_pbug;
do_func do_peace;
do_func do_peek;
do_func do_perform;
do_func do_pets;
do_func do_pfind;
do_func do_pick;
do_func do_play;
do_func do_polymorph;
do_func do_practice;
do_func do_pray;
do_func do_privileges;
do_func do_prompt;
do_func do_prepare;
do_func do_ps;
do_func do_pull;
do_func do_purchase;
do_func do_push;
do_func do_punch;
do_func do_purge;
do_func do_put;
do_func do_qbug;
do_func do_qedit;
do_func do_qflag;
do_func do_qlook;
do_func do_qremove;
do_func do_qset;
do_func do_qstat;
do_func do_quaff;
do_func do_quests;
do_func do_queue;
do_func do_quit;
do_func do_qwhere;
do_func do_qwho;
do_func do_rbug;
do_func do_read;
do_func do_reboot;
do_func do_recho;
do_func do_recite;
do_func do_recognize;
do_func do_rdesc;
do_func do_redit;
do_func do_reimburse;
do_func do_relations;
do_func do_religion;
do_func do_rename;
do_func do_repair;
do_func do_reputation;
do_func do_remove;
do_func do_rent;
do_func do_reply;
do_func do_request;
do_func do_rescue;
do_func do_reset;
do_func do_rest;
do_func do_restore;
do_func do_return;
do_func do_review;
do_func do_rfind;
do_func do_rflag;
do_func do_rlog;
do_func do_rmwhere;
do_func do_roomlist;
do_func do_rowhere;
do_func do_rset;
do_func do_rstat;
do_func do_rtable;
do_func do_rtwhere;
do_func do_sacrifice;
do_func do_save;
do_func do_say;
do_func do_scan;
do_func do_score;
do_func do_search;
do_func do_sell;
do_func do_shcustom;
do_func do_shedit;
do_func do_shflag;
do_func do_shoot;
do_func do_shout;
do_func do_shutdown;
do_func do_skin;
do_func do_sit;
do_func do_skill;
do_func do_slay;
do_func do_sleep;
do_func do_slookup;
do_func do_smoke;
do_func do_sneak;
do_func do_snoop;
do_func do_socials;
do_func do_source;
do_func do_south;
do_func do_speed;
do_func do_spell;
do_func do_spin_kick;
do_func do_split;
do_func do_sset;
do_func do_stand;
do_func do_statistics;
do_func do_status;
do_func do_steal;
do_func do_switch;
do_func do_system;
do_func do_tcode;
do_func do_tdata;
do_func do_tedit;
do_func do_tell;
do_func do_testing;
do_func do_tfind;
do_func do_throw;
do_func do_time;
do_func do_title;
do_func do_to;
do_func do_train;
do_func do_transfer;
do_func do_track;
do_func do_trust;
do_func do_tset;
do_func do_tstat;
do_func do_typo;
do_func do_unlock;
do_func do_untrap;
do_func do_up;
do_func do_users;
do_func do_value;
do_func do_vote; 
do_func do_wait;
do_func do_wake;
do_func do_wanted;
do_func do_wear;
do_func do_weather;
do_func do_west;
do_func do_where;
do_func do_whistle;
do_func do_whisper;
do_func do_who;
do_func do_whois;
do_func do_wimpy;
do_func do_withdraw;
do_func do_godlock;
do_func do_weather;
do_func do_wizlock;
do_func do_write;
do_func do_yell;
do_func do_zap;


void   boot_db            ( );
void   count_objects      ( );

bool   is_entangled       ( char_data*, const char* = 0 );
bool   is_silenced        ( char_data*, const char* = 0 );
bool   is_paralyzed       ( char_data*, const char* = 0 );
bool   is_choking         ( char_data*, const char* = 0 );
bool   is_confused        ( char_data*, const char* = 0 );

bool   are_allied         ( char_data*, char_data* );
bool   is_aggressive      ( char_data*, char_data* );
bool   is_berserk         ( char_data*, const char* = 0 );
bool   is_focusing        ( char_data*, const char* = 0 );
bool   is_confused_pet    ( char_data* );
bool   is_drowning        ( char_data*, const char* = 0 );
bool   is_humanoid        ( char_data* );
bool   is_familiar        ( char_data* );
bool   is_mob             ( char_data* );
bool   is_number          ( const char *arg );
bool   is_parrying        ( char_data*, const char* = 0 );
bool   isperiod           ( char letter );
bool   is_same_group      ( const char_data*, const char_data* ); 
bool   load_char          ( link_data*, const char*, const char* );
bool   not_player         ( char_data* );
bool   pet_help           ( char_data* );
bool   player_in_room     ( room_data* );

char   *coin_phrase       ( char_data* );
char   *coin_phrase       ( int* );
char   *reputation_name   ( int );
const char   *one_line           ( const char*, char* );
const char   *edit_string        ( char_data*, const char *, const char *, int, bool );

mob_data      *find_keeper       ( char_data* );
player_data   *rand_player       ( room_data*, char_data* = 0 );
char_data     *random_pers       ( room_data*, char_data* = 0 );

const char*  number_suffix      ( int );
const char*  damage_color       ( char_data*,  char_data*, char_data* );
const char   *condition_word    ( char_data *ch );
const char   *condition_prep    ( char_data *ch );

int    creation_points    ( player_data* );
int    dice_string        ( char *string );
int    get_money          ( char_data *ch );
int    get_reputation     ( char_data *ch, int race );
int    leech_regen        ( char_data* );
int    pack_int           ( int* );
long long xp_compute      ( char_data* );

int    *unpack_int        ( int ); 

room_data *get_temple     ( char_data* );
room_data *get_portal     ( char_data* );

void   advance_level      ( char_data*, bool );
void   area_update        ( );
bool   backstab           ( char_data*, char_data* );
void   burn_web           ( char_data*, const char* );
void   calc_resist        ( player_data* );
bool   can_bash           ( char_data *ch, char_data *victim, bool msg = true );
bool   can_disarm         ( char_data *ch, char_data *victim, obj_data *wield, bool msg = true );
bool   can_track          ( room_data *room );
void   cant_message       ( char_data* ch, const char *act, const char *cond );
bool   carnivore          ( char_data* );
bool   charge             ( char_data *ch, char_data *victim );
bool   check_social       ( char_data*, const char*, const char*, thing_data *target = 0 );
void   check_string       ( char_data *ch, char *string );
void   check_mount        ( char_data* );
void   clear_queue        ( char_data* );
void   clear_queue        ( obj_data*, obj_data* = 0 );
void   clear_pager        ( char_data *ch );
void   confused_char      ( char_data* );
void   death              ( char_data*, char_data*, const char* );
void   disconnect         ( char_data*, char_data* = 0 );
void   enchant_object     ( obj_data* ); 
void   extract_corpse     ( obj_data* );
const char *fake_name     ( bool );
int    find_command       ( const char_data*, const char*, bool = false );
help_data *find_help      ( char_data*, const char* );
unsigned format           ( char*, const char*,
			    bool = false, int = 0, const char_data* = 0,
			    bool = true );
int    format_tell        ( char*, const char*, const char_data* = 0 );
void   gain_drunk         ( char_data*, int );
bool   garrote            ( char_data*, char_data* );
int    haggle             ( char_data*, int, bool = true );
void   help_link          ( link_data*, const char* );
void   high_mob           ( char_data*, const char* );
void   init_memory        ( );
bool   interpret          ( char_data *ch, const char *argument );
bool   leave_shadows      ( char_data * );
void   load               ( FILE*, reset_data*& );
void   load_tables        ( );
bool   look_same          ( char_data*, thing_data*, thing_data*, bool = false );
bool   look_same          ( char_data*, obj_data*, obj_data*, bool = false );
bool   look_same          ( char_data*, char_data*, char_data*, bool = false );
void   lose_level         ( char_data *ch, bool );
void   make_tracks        ( char_data*, room_data*, int );
void   modify_reputation  ( char_data*, char_data*, int );
bool   obj_clss_arg       ( char_data*, const char*&,
			    obj_clss_data*, obj_data*, char*, int& );
void   show_resists       ( char_data*, char_data*, bool = true );
bool   plague             ( char_data *ch, int duration, bool consume );
bool   poison             ( char_data *ch, int level, int duration, bool consume );
obj_data *potion_container ( obj_data*, int = 1 );
void   process_tell       ( char_data*, char_data*, const char * );
void   raw_goto           ( wizard_data *, room_data *, room_data * );
bool   recite             ( char_data*, const char*, obj_data* );
void   request_message    ( player_data* );
bool   save_html_players  ( );
void   save_mail          ( pfile_data* );
void   save_mobs          ( );
void   save_objects       ( );
void   save_table         ( int );
void   save_tables        ( );
bool   search_code        ( char_data*, search_type&, void* );
void   set_alloy          ( obj_data*, int );
void   set_title          ( char_data*, char* );
void   show_map           ( char_data*, room_data*, int, int, bool = false, bool = false );
void   show_who           ( char_data*, bool = false, bool = false );
void   skin               ( char_data*, obj_data*, bool = false );
void   summon_help        ( char_data*, char_data*, int = 15, int = 50 );
void   unseat             ( char_data* );
void   update_handler     ( );
void   update_links       ( );
void   update_pos         ( char_data* );
void   update_queue       ( queue_data *& );
void   update_score       ( char_data* );
bool   vegetarian         ( char_data* );
void   water_shock        ( char_data*, char_data*, int );
void   write              ( FILE*, action_data* );
void   write              ( FILE*, reset_data* );
void   zero_exp           ( species_data* );


#endif // tfe_struct_h
