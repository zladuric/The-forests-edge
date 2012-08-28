#include <ctype.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include "define.h"
#include "struct.h"


char lib_buf [ LIB_BUF_LEN ];

const char *arg_type_name[MAX_ARG] = {
  "none", "any", "string", "integer", "null pointer",
  "character", "object", "room", "direction",
  "nation", "skill", "rflag",
  "stat", "class", "religion", "race", "thing",
  "char_affect", "obj_flag", "sex", "color", "wear_part",
  "group", "dflag", "exit", "wear_layer", "obj_type",
  "thing_list", "act_flag",
  "oprog_trigger", "mprog_trigger", "acode_trigger"
};


/*
 *   FUNCTION TABLE
 */


#define nn NONE
#define st STRING
#define ch CHARACTER
#define ob OBJECT
#define in INTEGER
#define rm ROOM
#define sk SKILL
#define sa STAT
#define di DIRECTION
#define rf RFLAG
#define na NATION
#define cl CLASS
#define re RELIGION
#define rc RACE
#define th THING
#define af AFFECT
#define of OFLAG
#define sx SEX
#define co COLOR
#define wp WEAR_PART
#define gr GROUP
#define df DFLAG
#define ex EXIT
#define wl WEAR_LAYER
#define ot OBJ_TYPE
#define tl THING_LIST
#define ma ACT_FLAG
#define ox OPROG_TRIG
#define mx MPROG_TRIG
#define ax ACODE_TRIG


const cfunc_type cfunc_list [] =
{
  { "acid_message",    &code_acid_message,  nn, { ch, in, st, ch, nn, nn } },
  { "acode",           &code_acode,         in, { rm, in, nn, nn, nn, nn } },
  { "act_area",        &code_act_area,      nn, { st, ch, ob, th, st, st } },
  { "act_neither",     &code_act_neither,   nn, { st, ch, ob, ch, st, st } },
  { "act_notchar",     &code_act_notchar,   nn, { st, ch, ob, th, st, st } },
  { "act_notvict",     &code_act_notvict,   nn, { st, ch, ob, ch, st, st } },
  { "act_room",        &code_act_room,      nn, { st, ch, ob, th, st, st } },
  { "act_tochar",      &code_act_tochar,    nn, { st, ch, ob, th, st, st } },
  { "append",          &code_append,        nn, { tl, th, nn, nn, nn, nn } },
  { "area",            &code_area,          in, { rm, nn, nn, nn, nn, nn } },
  { "assign_quest",    &code_assign_quest,  nn, { ch, in, nn, nn, nn, nn } },
  { "attack",          &code_attack,        in, { ch, ch, in, st, nn, nn } },
  { "attack_acid",     &code_attack_acid,   in, { ch, ch, in, st, nn, nn } },
  { "attack_backstab", &code_attack_backstab, in, { ch, ch, nn, nn, nn, nn } },
  { "attack_charge",   &code_attack_charge, in, { ch, ch, nn, nn, nn, nn } },
  { "attack_cold",     &code_attack_cold,   in, { ch, ch, in, st, nn, nn } },
  { "attack_fire",     &code_attack_fire,   in, { ch, ch, in, st, nn, nn } },
  { "attack_garrote",  &code_attack_garrote, in, { ch, ch, nn, nn, nn, nn } },
  { "attack_magic",    &code_attack_magic,  in, { ch, ch, in, st, nn, nn } },
  { "attack_mind",     &code_attack_mind,   in, { ch, ch, in, st, nn, nn } },
  { "attack_offhand",  &code_attack_offhand,in, { ch, ch, in, st, nn, nn } },
  { "attack_room",     &code_attack_room,   nn, { ch, in, st, nn, nn, nn } },
  { "attack_shock",    &code_attack_shock,  in, { ch, ch, in, st, nn, nn } },
  { "attack_sound",    &code_attack_sound,  in, { ch, ch, in, st, nn, nn } },
  { "attack_weapon",   &code_attack_weapon, in, { ch, ch, in, st, nn, nn } },
  { "beep",            &code_beep,          nn, { ch, nn, nn, nn, nn, nn } },
  { "beetle",          &code_beetle,        nn, { st, ch, ob, th, st, st } },
  { "bug",             &code_bug,           nn, { st, ch, ob, th, st, st } },
  { "burn_web",        &code_burn_web,      nn, { ch, st, nn, nn, nn, nn } },
  { "can_attack",      &code_can_attack,    in, { ch, ch, nn, nn, nn, nn } },
  { "can_bash",        &code_can_bash,      in, { ch, ch, nn, nn, nn, nn } },
  { "can_fly",         &code_can_fly,       in, { ch, nn, nn, nn, nn, nn } },
  { "can_hear",        &code_can_hear,      in, { ch, nn, nn, nn, nn, nn } },
  { "can_move",        &code_can_move,      in, { ch, nn, nn, nn, nn, nn } },
  { "can_see",         &code_can_see,       in, { ch, st, nn, nn, nn, nn } },
  { "can_see_char",    &code_can_see_char,  in, { ch, ch, nn, nn, nn, nn } },
  { "can_see_obj",     &code_can_see_obj,   in, { ch, ob, nn, nn, nn, nn } },
  { "can_see_room",    &code_can_see_room,  in, { ch, rm, nn, nn, nn, nn } },
  { "can_speak",       &code_can_speak,     in, { ch, nn, nn, nn, nn, nn } },
  { "cardinal",        &code_cardinal,      st, { in, ch, nn, nn, nn, nn } },
  { "cast_mana",       &code_cast_mana,     in, { sk, ch, nn, nn, nn, nn } },
  { "cast_spell",      &code_cast_spell,    in, { sk, ch, th, in, in, nn } },
  { "cflag",           &code_cflag,         in, { in, ch, nn, nn, nn, nn } },
  { "chair",           &code_chair,         ob, { ch, nn, nn, nn, nn, nn } },
  { "char_affected",   &code_char_affected, in, { ch, af, nn, nn, nn, nn } },
  { "char_in_room",    &code_char_in_room,  in, { ch, rm, nn, nn, nn, nn } },
  { "character",       &code_character,     ch, { th, nn, nn, nn, nn, nn } },
  { "check_skill",     &code_check_skill,   in, { ch, sk, in, nn, nn, nn } },
  { "clan_abbrev",     &code_clan_abbrev,   st, { ch, nn, nn, nn, nn, nn } },
  { "clan_name",       &code_clan_name,     st, { ch, nn, nn, nn, nn, nn } },
  { "class",           &code_class,         cl, { ch, nn, nn, nn, nn, nn } },
  { "close",           &code_close,         in, { th, di, nn, nn, nn, nn } },
  { "close",           &code2_close,        in, { ex, nn, nn, nn, nn, nn } },
  { "coin_value",      &code_coin_value,    in, { ob, nn, nn, nn, nn, nn } },
  { "cold_message",    &code_cold_message,  nn, { ch, in, st, ch, nn, nn } },
  { "contents",        &code_contents,      tl, { th, nn, nn, nn, nn, nn } },
  { "dam_acid",        &code_dam_acid,      in, { ch, ch, in, st, st, nn } },
  { "dam_cold",        &code_dam_cold,      in, { ch, ch, in, st, st, nn } },
  { "dam_fire",        &code_dam_fire,      in, { ch, ch, in, st, st, nn } },
  { "dam_magic",       &code_dam_magic,     in, { ch, ch, in, st, st, nn } },
  { "dam_message",     &code_dam_message,   nn, { ch, in, st, ch, nn, nn } },
  { "dam_mind",        &code_dam_mind,      in, { ch, ch, in, st, st, nn } },
  { "dam_physical",    &code_dam_physical,  in, { ch, ch, in, st, st, nn } },
  { "dam_shock",       &code_dam_shock,     in, { ch, ch, in, st, st, nn } },
  { "dam_sound",       &code_dam_sound,     in, { ch, ch, in, st, st, nn } },
  { "date_day",        &code_date_day,      in, { nn, nn, nn, nn, nn, nn } },
  { "date_first_year", &code_date_first_year, in, { nn, nn, nn, nn, nn, nn } },
  { "date_month",      &code_date_month,    in, { nn, nn, nn, nn, nn, nn } },
  { "date_weekday",    &code_date_weekday,  in, { nn, nn, nn, nn, nn, nn } },
  { "date_year",       &code_date_year,     in, { nn, nn, nn, nn, nn, nn } },
  { "days_in_month",   &code_days_in_month, in, { in, nn, nn, nn, nn, nn } },
  { "deep_water",      &code_deep_water,    in, { ch, rm, nn, nn, nn, nn } },
  { "default",         &code_default,       st, { st, nn, nn, nn, nn, nn } },
  { "dflag",           &code_dflag,         in, { df, ex, nn, nn, nn, nn } },
  { "dice",            &code_dice,          in, { in, in, in, nn, nn, nn } },
  { "dir_name",        &code_dir_name,      st, { di, nn, nn, nn, nn, nn } },
  { "direction",       &code_direction,     di, { ex, nn, nn, nn, nn, nn } },
  { "disembark",       &code_disembark,     nn, { ch, nn, nn, nn, nn, nn } },
  { "dismount",        &code_dismount,      nn, { ch, nn, nn, nn, nn, nn } },
  { "dispel_affect",   &code_dispel_affect, nn, { ch, af, nn, nn, nn, nn } },
  { "do_spell",        &code_do_spell,      in, { sk, ch, th, in, nn, nn } },
  { "doing_quest",     &code_doing_quest,   in, { ch, in, nn, nn, nn, nn } },
  { "done_quest",      &code_done_quest,    in, { ch, in, nn, nn, nn, nn } },
  { "door",            &code_door,          ex, { rm, di, nn, nn, nn, nn } },
  { "drain_exp",       &code_drain_exp,     nn, { ch, in, nn, nn, nn, nn } },
  { "drain_stat",      &code_drain_stat,    nn, { ch, sa, in, in, nn, nn } },
  { "drop",            &code_drop,          nn, { ch, tl, nn, nn, nn, nn } },
  { "embark",          &code_embark,        in, { ch, ob, nn, nn, nn, nn } },
  { "fail_quest",      &code_fail_quest,    nn, { ch, in, nn, nn, nn, nn } },
  { "failed_quest",    &code_failed_quest,  in, { ch, in, nn, nn, nn, nn } },
  { "find_player",     &code_find_player,   ch, { st, nn, nn, nn, nn, nn } },
  { "find_room",       &code_find_room,     rm, { in, nn, nn, nn, nn, nn } },
  { "find_skill",      &code_find_skill,    in, { ch, sk, nn, nn, nn, nn } },
  { "find_stat",       &code_find_stat,     in, { ch, sa, nn, nn, nn, nn } },
  { "fire_message",    &code_fire_message,  nn, { ch, in, st, ch, nn, nn } },
  { "first_word",      &code_first_word,    st, { st, nn, nn, nn, nn, nn } },
  { "followers",       &code_followers,     tl, { ch, nn, nn, nn, nn, nn } },
  { "get_desc",        &code_get_desc,      st, { th, st, nn, nn, nn, nn } },
  { "get_money",       &code_get_money,     in, { ch, nn, nn, nn, nn, nn } },
  { "get_quest",       &code_get_quest,     in, { ch, in, nn, nn, nn, nn } },
  { "get_religion",    &code_get_religion,  re, { ch, nn, nn, nn, nn, nn } },
  { "get_room",        &code_get_room,      rm, { ch, nn, nn, nn, nn, nn } },
  { "get_sex",         &code_get_sex,       sx, { ch, nn, nn, nn, nn, nn } },
  { "has_key",         &code_has_key,       ob, { ch, in, nn, nn, nn, nn } },
  { "has_obj",         &code_has_obj,       ob, { in, th, nn, nn, nn, nn } },
  { "has_obj_flag",    &code_has_obj_flag,  ob, { of, th, nn, nn, nn, nn } },
  { "has_prepared",    &code_has_prepared,  in, { ch, sk, nn, nn, nn, nn } },
  { "has_quest",       &code_has_quest,     in, { ch, in, nn, nn, nn, nn } },
  { "has_reset",       &code_has_reset,     in, { th, nn, nn, nn, nn, nn } },
  { "has_wear_part",   &code_has_wear_part, in, { ch, wp, nn, nn, nn, nn } },
  { "heal",            &code_heal,          in, { ch, in, nn, nn, nn, nn } },
  { "heal_victim",     &code_heal_victim,   in, { ch, ch, in, in, nn, nn } },
  { "inflict",         &code_inflict,       in, { ch, ch, in, st, nn, nn } },
  { "inflict_acid",    &code_inflict_acid,  in, { ch, ch, in, st, nn, nn } },
  { "inflict_cold",    &code_inflict_cold,  in, { ch, ch, in, st, nn, nn } },
  { "inflict_fire",    &code_inflict_fire,  in, { ch, ch, in, st, nn, nn } },
  { "inflict_magic",   &code_inflict_magic, in, { ch, ch, in, st, nn, nn } },
  { "inflict_mind",    &code_inflict_mind,  in, { ch, ch, in, st, nn, nn } },
  { "inflict_shock",   &code_inflict_shock, in, { ch, ch, in, st, nn, nn } },
  { "inflict_sound",   &code_inflict_sound, in, { ch, ch, in, st, nn, nn } },
  { "insert",          &code_insert,        nn, { tl, th, in, nn, nn, nn } },
  { "interpret",       &code_interpret,     nn, { ch, st, th, nn, nn, nn } },
  { "is_aggressive",   &code_is_aggressive, in, { ch, ch, ch, nn, nn, nn } },
  { "is_altered",      &code_is_altered,    in, { th, nn, nn, nn, nn, nn } },
  { "is_casting",      &code_is_casting,    in, { ch, nn, nn, nn, nn, nn } },
  { "is_confused_pet", &code_is_confused_pet, in, { ch, nn, nn, nn, nn, nn } },
  { "is_creature",     &code_is_creature,   in, { ch, nn, nn, nn, nn, nn } },
  { "is_drowning",     &code_is_drowning,   in, { ch, st, nn, nn, nn, nn } },
  { "is_empty",        &code_is_empty,      in, { tl, nn, nn, nn, nn, nn } },
  { "is_entangled",    &code_is_entangled,  in, { ch, st, nn, nn, nn, nn } },
  { "is_exhausted",    &code_is_exhausted,  in, { ch, in, nn, nn, nn, nn } },
  { "is_family",       &code_is_family,     in, { ch, rc, nn, nn, nn, nn } },
  { "is_fighting",     &code_is_fighting,   ch, { ch, st, nn, nn, nn, nn } },
  { "is_follower",     &code_is_follower,   ch, { ch, nn, nn, nn, nn, nn } },
  { "is_group",        &code_is_group,      in, { ch, gr, nn, nn, nn, nn } },
  { "is_locked",       &code2_is_locked,    in, { ex, nn, nn, nn, nn, nn } },
  { "is_locked",       &code_is_locked,     in, { th, di, nn, nn, nn, nn } },
  { "is_meditating",   &code_is_meditating, in, { ch, nn, nn, nn, nn, nn } },
  { "is_mob",          &code_is_mob,        in, { ch, nn, nn, nn, nn, nn } },
  { "is_mounted",      &code_is_mounted,    ch, { ch, st, nn, nn, nn, nn } },
  { "is_name",         &code_is_name,       in, { st, st, nn, nn, nn, nn } },
  { "is_not_player",   &code_is_not_player, in, { ch, nn, nn, nn, nn, nn } },
  { "is_open",         &code_is_open,       in, { th, di, nn, nn, nn, nn } },
  { "is_open",         &code2_is_open,      in, { ex, nn, nn, nn, nn, nn } },
  { "is_pet",          &code_is_pet,        ch, { ch, nn, nn, nn, nn, nn } },
  { "is_player",       &code_is_player,     in, { ch, nn, nn, nn, nn, nn } },
  { "is_race",         &code_is_race,       in, { ch, rc, nn, nn, nn, nn } },
  { "is_resting",      &code_is_resting,    in, { ch, nn, nn, nn, nn, nn } },
  { "is_searching",    &code_is_searching,  in, { ch, nn, nn, nn, nn, nn } },
  { "is_silenced",     &code_is_silenced,   in, { ch, st, nn, nn, nn, nn } },
  { "is_sleeping",     &code_is_sleeping,   in, { ch, nn, nn, nn, nn, nn } },
  { "is_standing",     &code_is_standing,   in, { ch, nn, nn, nn, nn, nn } },
  { "is_submerged",    &code_is_submerged,  in, { ch, rm, nn, nn, nn, nn } },
  { "item",            &code_item,          th, { tl, in, nn, nn, nn, nn } },
  { "junk",            &code_junk,          nn, { ch, tl, nn, nn, nn, nn } },
  { "junk_mob",        &code_junk_mob,      nn, { ch, nn, nn, nn, nn, nn } },
  { "junk_obj",        &code_junk_obj,      nn, { ob, in, nn, nn, nn, nn } },
  { "leader",          &code_leader,        ch, { ch, nn, nn, nn, nn, nn } },
  { "leads_to",        &code_leads_to,      rm, { ex, nn, nn, nn, nn, nn } },
  { "length",          &code_length,        in, { tl, nn, nn, nn, nn, nn } },
  { "light",           &code_light,         in, { th, nn, nn, nn, nn, nn } },
  { "list_name",       &code_list_name,     st, { tl, ch, st, nn, nn, nn } },
  { "lock",            &code_lock,          in, { th, di, nn, nn, nn, nn } },
  { "lock",            &code2_lock,         in, { ex, nn, nn, nn, nn, nn } },
  { "magic_message",   &code_magic_message, nn, { ch, in, st, ch, nn, nn } },
  { "mail",            &code_mail,          in, { st, st, st, st, nn, nn } },
  { "min_move",        &code_min_move,      in, { ch, nn, nn, nn, nn, nn } },
  { "mind_message",    &code_mind_message,  nn, { ch, in, st, ch, nn, nn } },
  { "mload",           &code_mload,         ch, { in, rm, nn, nn, nn, nn } },
  { "mob",             &code_mob,           ch, { th, nn, nn, nn, nn, nn } },
  { "mob_act",         &code_mob_act,       in, { ch, ma, nn, nn, nn, nn } },
  { "mob_in_room",     &code_mob_in_room,   ch, { in, rm, nn, nn, nn, nn } },
  { "modify_mana",     &code_modify_mana,   in, { ch, in, nn, nn, nn, nn } },
  { "modify_move",     &code_modify_move,   in, { ch, in, nn, nn, nn, nn } },
  { "moon_phase",      &code_moon_phase,    in, { nn, nn, nn, nn, nn, nn } },
  { "mount",           &code_mount,         ch, { ch, nn, nn, nn, nn, nn } },
  { "move_arrives",    &code_move_arrives,  st, { ch, nn, nn, nn, nn, nn } },
  { "move_leaves",     &code_move_leaves,   st, { ch, nn, nn, nn, nn, nn } },
  { "move_name",       &code_move_name,     st, { ch, nn, nn, nn, nn, nn } },
  { "mpcode",          &code_mpcode,        in, { ch, in, nn, nn, nn, nn } },
  { "mprog",           &code_mprog,         in, { in, in, nn, nn, nn, nn } },
  { "name",            &code_name,          st, { ch, th, nn, nn, nn, nn } },
  { "num_in_room",     &code_num_in_room,   in, { rm, nn, nn, nn, nn, nn } },
  { "num_mob",         &code_num_mob,       in, { in, rm, nn, nn, nn, nn } },
  { "number",          &code_number,        in, { ob, nn, nn, nn, nn, nn } },
  { "obj_affected",    &code_obj_affected,  in, { ob, af, nn, nn, nn, nn } },
  { "obj_condition",   &code_obj_condition, in, { ob, nn, nn, nn, nn, nn } },
  { "obj_cost",        &code_obj_cost,      in, { ob, nn, nn, nn, nn, nn } },
  { "obj_count",       &code_obj_count,     in, { in, nn, nn, nn, nn, nn } },
  { "obj_durability",  &code_obj_durability,in, { ob, nn, nn, nn, nn, nn } },
  { "obj_flag",        &code_obj_flag,      in, { ob, of, nn, nn, nn, nn } },
  { "obj_in_room",     &code_obj_in_room,   ob, { in, rm, nn, nn, nn, nn } },
  { "obj_level",       &code_obj_level,     in, { ob, nn, nn, nn, nn, nn } },
  { "obj_limit",       &code_obj_limit,     in, { in, nn, nn, nn, nn, nn } },
  { "obj_to_char",     &code_obj_to_char,   ob, { ob, ch, in, nn, nn, nn } },
  { "obj_to_container", &code_obj_to_container, ob, { ob, ob, in, nn, nn, nn } },
  { "obj_to_room",     &code_obj_to_room,   ob, { ob, rm, in, nn, nn, nn } },
  { "obj_type",        &code_obj_type,      ot, { ob, nn, nn, nn, nn, nn } },
  { "obj_value",       &code_obj_value,     in, { ob, in, nn, nn, nn, nn } },
  { "object",          &code_object,        ob, { th, nn, nn, nn, nn, nn } },
  { "offhand",         &code_offhand,       ob, { ch, nn, nn, nn, nn, nn } },
  { "oload",           &code_oload,         ob, { in, in, ch, nn, nn, nn } },
  { "opcode",          &code_opcode,        in, { ob, in, nn, nn, nn, nn } },
  { "open",            &code_open,          in, { th, di, nn, nn, nn, nn } },
  { "open",            &code2_open,         in, { ex, nn, nn, nn, nn, nn } },
  { "oprog",           &code_oprog,         in, { in, in, nn, nn, nn, nn } },
  { "ordinal",         &code_ordinal,       st, { in, ch, nn, nn, nn, nn } },
  { "owns",            &code_owns,          in, { ch, ob, nn, nn, nn, nn } },
  { "path",            &code_path,          nn, { ch, th, in, in, nn, nn } },
  { "path_end",        &code_path_end,      rm, { ch, nn, nn, nn, nn, nn } },
  { "path_next",       &code_path_next,     ex, { ch, nn, nn, nn, nn, nn } },
  { "path_retry",      &code_path_retry,    nn, { ch, nn, nn, nn, nn, nn } },
  { "path_target",     &code_path_target,   ch, { ch, nn, nn, nn, nn, nn } },
  { "plague",          &code_plague,        in, { ch, in, nn, nn, nn, nn } },
  { "player",          &code_player,        ch, { th, nn, nn, nn, nn, nn } },
  { "players_area",    &code_players_area,  in, { rm, nn, nn, nn, nn, nn } },
  { "players_room",    &code_players_room,  in, { rm, nn, nn, nn, nn, nn } },
  { "poison",          &code_poison,        in, { ch, in, nn, nn, nn, nn } },
  { "prepend",         &code_prepend,       nn, { tl, th, nn, nn, nn, nn } },
  { "race",            &code_race,          rc, { ch, nn, nn, nn, nn, nn } },
  { "rand_char",       &code_rand_char,     ch, { rm, ch, nn, nn, nn, nn } },
  { "rand_player",     &code_rand_player,   ch, { rm, ch, nn, nn, nn, nn } },
  { "random",          &code_random,        in, { in, in, nn, nn, nn, nn } },
  { "remove",          &code_remove,        th, { tl, in, nn, nn, nn, nn } },
  { "remove_cflag",    &code_remove_cflag,  nn, { in, ch, nn, nn, nn, nn } },
  { "remove_dflag",    &code_remove_dflag,  nn, { df, ex, nn, nn, nn, nn } },
  { "remove_obj_flag", &code_remove_obj_flag, ob, { ob, of, in, nn, nn, nn } },
  { "remove_quest",    &code_remove_quest,  nn, { ch, in, nn, nn, nn, nn } },
  { "remove_rflag",    &code_remove_rflag,  nn, { rf, rm, nn, nn, nn, nn } },
  { "replace_obj",     &code_replace_obj,   ob, { ob, in, in, ch, nn, nn } },
  { "reputation",      &code_reputation,    in, { ch, na, nn, nn, nn, nn } },
  { "reset_appear",    &code_reset_appear,  nn, { th, nn, nn, nn, nn, nn } },
  { "reverse",         &code_reverse,       ex, { ex, nn, nn, nn, nn, nn } },
  { "rflag",           &code_rflag,         in, { rf, rm, nn, nn, nn, nn } },
  { "rider",           &code_rider,         ch, { ch, nn, nn, nn, nn, nn } },
  { "room",            &code_room,          rm, { th, nn, nn, nn, nn, nn } },
  { "room_next",       &code_room_next,     rm, { rm, nn, nn, nn, nn, nn } },
  { "selected",        &code_selected,      in, { ob, nn, nn, nn, nn, nn } },
  { "send_not_char",   &code_send_not_char, nn, { st, ch, nn, nn, nn, nn } },
  { "send_to_area",    &code_send_to_area,  nn, { st, rm, in, nn, nn, nn } },
  { "send_to_char",    &code_send_to_char,  nn, { st, ch, nn, nn, nn, nn } },
  { "send_to_room",    &code_send_to_room,  nn, { st, rm, nn, nn, nn, nn } },
  { "set_adj_p",       &code_set_adj_p,     nn, { ch, st, nn, nn, nn, nn } },
  { "set_adj_s",       &code_set_adj_s,     nn, { ch, st, nn, nn, nn, nn } },
  { "set_after",       &code_set_after,     nn, { ob, st, nn, nn, nn, nn } },
  { "set_before",      &code_set_before,    nn, { ob, st, nn, nn, nn, nn } },
  { "set_cflag",       &code_set_cflag,     nn, { in, ch, nn, nn, nn, nn } },
  { "set_color",       &code_set_color,     nn, { ch, co, nn, nn, nn, nn } },
  { "set_cost",        &code_set_cost,      ob, { ob, in, in, nn, nn, nn } },
  { "set_delay",       &code_set_delay,     nn, { ch, in, nn, nn, nn, nn } },
  { "set_desc",        &code_set_desc,      nn, { ch, st, nn, nn, nn, nn } },
  { "set_dflag",       &code_set_dflag,     nn, { df, ex, nn, nn, nn, nn } },
  { "set_keywords",    &code_set_keywords,  nn, { ch, st, nn, nn, nn, nn } },
  { "set_light",       &code_set_light,     ob, { ob, in, in, nn, nn, nn } },
  { "set_long_p",      &code_set_long_p,    nn, { ch, st, nn, nn, nn, nn } },
  { "set_long_s",      &code_set_long_s,    nn, { ch, st, nn, nn, nn, nn } },
  { "set_mob_timer",   &code_set_mob_timer, nn, { ch, in, nn, nn, nn, nn } },
  { "set_mount",       &code_set_mount,     nn, { ch, ch, nn, nn, nn, nn } },
  { "set_name",        &code_set_name,      nn, { ch, st, nn, nn, nn, nn } },
  { "set_obj_flag",    &code_set_obj_flag,  ob, { ob, of, in, nn, nn, nn } },
  { "set_obj_timer",   &code_set_obj_timer, ob, { ob, in, nn, nn, nn, nn } },
  { "set_obj_value",   &code_set_obj_value, ob, { ob, in, in, in, nn, nn } },
  { "set_owner",       &code_set_owner,     ob, { ob, ch, nn, nn, nn, nn } },
  { "set_plural",      &code_set_plural,    nn, { th, st, nn, nn, nn, nn } },
  { "set_prefix_p",    &code_set_prefix_p,  nn, { ch, st, nn, nn, nn, nn } },
  { "set_prefix_s",    &code_set_prefix_s,  nn, { ch, st, nn, nn, nn, nn } },
  { "set_quest",       &code_set_quest,     nn, { ch, in, in, nn, nn, nn } },
  { "set_religion",    &code_set_religion,  nn, { ch, re, nn, nn, nn, nn } },
  { "set_resting",     &code_set_resting,   in, { ch, ob, nn, nn, nn, nn } },
  { "set_rflag",       &code_set_rflag,     nn, { rf, rm, nn, nn, nn, nn } },
  { "set_sentinel",    &code_set_sentinel,  in, { ch, in, nn, nn, nn, nn } },
  { "set_singular",    &code_set_singular,  nn, { th, st, nn, nn, nn, nn } },
  { "set_standing",    &code_set_standing,  in, { ch, nn, nn, nn, nn, nn } },
  { "set_victim",      &code_set_victim,    in, { ch, ch, nn, nn, nn, nn } },
  { "set_weight",      &code_set_weight,    ob, { ob, in, in, nn, nn, nn } },
  { "shield",          &code_shield,        ob, { ch, nn, nn, nn, nn, nn } },
  { "shock_message",   &code_shock_message, nn, { ch, in, st, ch, nn, nn } },
  { "show",            &code_show,          nn, { ch, rm, di, nn, nn, nn } }, 
  { "show",            &code2_show,         nn, { ch, ex, nn, nn, nn, nn } }, 
  { "show_map",        &code_show_map,      nn, { ch, rm, in, in, nn, nn } },
  { "size",            &code_size,          in, { th, nn, nn, nn, nn, nn } }, 
  { "skill",           &code_skill,         in, { sk, nn, nn, nn, nn, nn } },
  { "skip_word",       &code_skip_word,     st, { st, nn, nn, nn, nn, nn } },
  { "sound_message",   &code_sound_message, nn, { ch, in, st, ch, nn, nn } },
  { "spell_affect",    &code_spell_affect,  in, { ch, ch, sk, af, in, in } },
  { "spell_damage",    &code_spell_damage,  in, { sk, in, nn, nn, nn, nn } },
  { "spell_duration",  &code_spell_duration, in, { sk, in, nn, nn, nn, nn } },
  { "stop_camouflage", &code_stop_camouflage, in, { ch, nn, nn, nn, nn, nn } },
  { "stop_hide",       &code_stop_hide,     in, { ch, nn, nn, nn, nn, nn } },
  { "string",          &code_string,        st, { st, ch, ob, th, st, st } },
  { "summon",          &code_summon,        nn, { ch, ch, in, in, nn, nn } },
  { "take",            &code_take,          nn, { ch, tl, ob, nn, nn, nn } },
  { "tell",            &code_tell,          nn, { ch, ch, st, ob, st, st } },
  { "time_hour",       &code_time_hour,     in, { nn, nn, nn, nn, nn, nn } },
  { "time_minute",     &code_time_minute,   in, { nn, nn, nn, nn, nn, nn } },
  { "transfer",        &code_transfer,      in, { ch, rm, nn, nn, nn, nn } },
  { "transfer_all",    &code_transfer_all,  in, { rm, rm, nn, nn, nn, nn } },
  { "trigger_acode",   &code_trigger_acode, in, { in, ax, in, nn, nn, nn } },
  { "trigger_mprog",   &code_trigger_mprog, in, { in, mx, in, nn, nn, nn } },
  { "trigger_oprog",   &code_trigger_oprog, in, { in, ox, in, nn, nn, nn } },
  { "unlock",          &code_unlock,        in, { th, di, nn, nn, nn, nn } },
  { "unlock",          &code2_unlock,       in, { ex, nn, nn, nn, nn, nn } },
  { "update_quest",    &code_update_quest,  nn, { ch, in, nn, nn, nn, nn } },
  { "use_skill",       &code_use_skill,     nn, { ch, sk, nn, nn, nn, nn } },
  { "vnum",            &code_vnum,          in, { th, nn, nn, nn, nn, nn } },
  { "wait",            &code_wait,          nn, { in, nn, nn, nn, nn, nn } },
  { "water_logged",    &code_water_logged,  in, { rm, nn, nn, nn, nn, nn } },
  { "water_shock",     &code_water_shock,   nn, { ch, ch, in, nn, nn, nn } },
  { "wearing",         &code_wearing,       tl, { ch, nn, nn, nn, nn, nn } },
  { "wearing_obj",     &code_wearing_obj,   ob, { in, ch, nn, nn, nn, nn } },
  { "wearing_obj_flag",&code_wearing_obj_flag, ob, { of, ch, nn, nn, nn, nn } },
  { "weight",          &code_weight,        in, { th, nn, nn, nn, nn, nn } },
  { "where",           &code_where,         th, { th, nn, nn, nn, nn, nn } },
  { "wield",           &code_wield,         ob, { ch, nn, nn, nn, nn, nn } },
  { "word",            &code_word,          st, { st, in, nn, nn, nn, nn } },
  { "worn",            &code_worn,          ob, { ch, wp, wl, nn, nn, nn } },
  { "",                0,                   nn, { nn, nn, nn, nn, nn, nn } }
};


#undef nn
#undef st
#undef ch
#undef ob
#undef in
#undef rm
#undef sk
#undef sa
#undef di
#undef rf
#undef na
#undef cl
#undef re
#undef rc
#undef th
#undef af
#undef of
#undef sx
#undef co
#undef wp
#undef gr
#undef df
#undef ex
#undef wl
#undef ot
#undef tl


const void *code_beetle( const void **argument )
{
  const char *string = (const char *) argument[0];
  char_data *ch = (char_data*)(thing_data*) argument[1];
  obj_data *obj = (obj_data*)(thing_data*) argument[2];
  thing_data *thing  = (thing_data*) argument[3];
  const char *string1 = (const char*) argument[4];
  const char *string2 = (const char*) argument[5];
  
  act_print( lib_buf, string, ch, character( thing ), obj, object( thing ), string1, string2, 0, ch, false );
  beetle( lib_buf );

  return 0;
}


const void *code_bug( const void **argument )
{
  const char *string = (const char *) argument[0];
  char_data *ch = (char_data*)(thing_data*) argument[1];
  obj_data *obj = (obj_data*)(thing_data*) argument[2];
  thing_data *thing  = (thing_data*) argument[3];
  const char *string1 = (const char*) argument[4];
  const char *string2 = (const char*) argument[5];
  
  act_print( lib_buf, string, ch, character( thing ), obj, object( thing ), string1, string2, 0, ch, false );
  code_bug( lib_buf );
  
  return 0;
}


/*
 *   CAN_SEE ROUTINES
 */


const void *code_can_see( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];
  const char *string = (const char*) argument[1];

  if( !ch ) {
    code_bug( "Can_See: NULL character." );
    return 0;
  }
  
  return (void*) ch->Can_See( string );
}


const void *code_can_see_char( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];
  char_data *victim = (char_data*)(thing_data*) argument[1];

  if( !ch ) {
    code_bug( "Can_See_Char: NULL character." );
    return 0;
  }
  
  if( !victim ) {
    code_bug( "Can_See_Char: NULL victim." );
    return 0;
  }
  
  return (void*) victim->Seen( ch );
}


const void *code_can_see_obj( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];
  obj_data *obj = (obj_data*)(thing_data*) argument[1];

  if( !ch ) {
    code_bug( "Can_See_Room: NULL character." );
    return 0;
  }
  
  if( !obj ) {
    code_bug( "Can_See_Room: NULL object." );
    return 0;
  }
  
  return (void*) obj->Seen( ch );
}


const void *code_can_see_room( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];
  room_data *room = (room_data*)(thing_data*) argument[1];

  if( !ch ) {
    code_bug( "Can_See_Room: NULL character." );
    return 0;
  }
  
  if( !room ) {
    code_bug( "Can_See_Room: NULL room." );
    return 0;
  }
  
  return (void*) room->Seen( ch );
}



/*
 *   POSITION ROUTINES
 */

   
const void *code_is_mounted( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];
  const char *string = (const char*) argument[1];

  if( !ch ) {
    code_bug( "Is_Mounted: NULL character." );
    return (void *) false;
  }

  if( string )
    is_mounted( ch, string );

  return (void*) ch->mount;
}


const void *code_dismount( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];

  if( !ch ) {
    code_bug( "Dismount: NULL character." );
    return (void *) false;
  }

  dismount( ch, POS_STANDING );

  return 0;
}


const void *code_is_standing( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];

  if( !ch ) {
    code_bug( "Is_Standing: NULL character." );
    return (void *) false;
  }

  return (void*) ( ch->position == POS_STANDING );
}


const void *code_is_resting( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];

  if( !ch ) {
    code_bug( "Is_Resting: NULL character." );
    return (void *) false;
  }

  return (void*) ( ch->position == POS_RESTING );
}


const void *code_set_resting( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];
  obj_data *obj = (obj_data*)(thing_data*) argument[1];

  if( !ch ) {
    code_bug( "Set_Resting: NULL character." );
    return (void *) false;
  }

  if( obj ) {
    return (void*) sit( ch, obj, false );
  }

  return (void*) rest( ch, false );
}


const void *code_set_standing( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];

  if( !ch ) {
    code_bug( "Set_Standing: NULL character." );
    return (void *) false;
  }

  return (void*) stand( ch, false );
}


const void *code_is_meditating( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];

  if( !ch ) {
    code_bug( "Is_Meditating: NULL character." );
    return (void *) false;
  }

  return (void*) ( ch->position == POS_MEDITATING );
}


const void *code_is_sleeping( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];

  if( !ch ) {
    code_bug( "Is_Sleeping: NULL character." );
    return (void *) false;
  }

  return (void*) ( ch->position == POS_SLEEPING );
}


const void *code_is_fighting( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];
  const char *string = (const char*) argument[1];

  if( !ch ) {
    code_bug( "Is_Fighting: NULL character." );
    return 0;
  }

  if( !is_fighting( ch, string ) )
    return 0;

  return (void*) opponent( ch );
}


const void *code_is_pet( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];

  if( !ch ) {
    code_bug( "Is_Pet: NULL character." );
    return 0;
  }
  
  return (void*) ( ( !ch->pcdata && is_set( ch->status, STAT_PET ) )
		   ? ch->leader : 0 );
}


const void *code_is_player( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];

  if( !ch ) {
    code_bug( "Is_Player: NULL character." );
    return 0;
  }
  
  return (void*) ( ch->species == 0 );
}



const void *code_is_aggressive( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];
  char_data *victim1 = (char_data*)(thing_data*) argument[1];
  char_data *victim2 = (char_data*)(thing_data*) argument[2];

  if( !ch ) {
    code_bug( "Is_Aggressive: NULL character." );
    return 0;
  }

  if( !victim1 ) {
    code_bug( "Is_Aggressive: NULL character." );
    return 0;
  }

  return (void*) ( ch->aggressive.includes( victim1 )
	   || victim2 && ch->aggressive.includes( victim2 ) );
}


/*
 *   ROOM ROUTINES
 */


const void *code_rflag( const void **argument )
{
  const int flag = (int) argument[0];
  room_data *room = (room_data*)(thing_data*) argument[1]; 

  if( !room ) {
    code_bug( "Rflag: NULL room." );
    return 0;
  }

  return (void*) is_set( room->room_flags, flag );
}


const void *code_set_rflag( const void **argument )
{
  const int flag = (int) argument[0];
  room_data *room = (room_data*)(thing_data*) argument[1]; 

  if( !room ) {
    code_bug( "Set_Rflag: NULL room." );
    return 0;
  }

  set_bit( room->room_flags, flag );

  return 0;
}


const void *code_remove_rflag( const void **argument )
{
  const int flag = (int) argument[0];
  room_data *room = (room_data*)(thing_data*) argument[1]; 

  if( !room ) {
    code_bug( "Remove_Rflag: NULL room." );
    return 0;
  }

  remove_bit( room->room_flags, flag );

  return 0;
}


const void *code_get_room( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];

  if( !ch ) {
    code_bug( "Get_Room: NULL character." );
    return 0;
  }

  return ch->in_room;
}


/*
 *   CHARACTER ROUTINES
 */


const void *code_size( const void **argument )
{
  thing_data *th = (thing_data*) argument[0];

  if( !th ) {
    code_bug( "Size: Null argument." );
    return 0;
  }

  char_data *ch = dynamic_cast<char_data *>( th );
  room_data *room = dynamic_cast<room_data *>( th );

  if( ch ) {
    return (void*) ch->Size( );
  }

  if( room ) {
    return (void*) room->size;
  }

  code_bug( "Size: Argument is not a character or room." );
  return 0;
}


const void *code_weight( const void **argument )
{
  thing_data *thing = (thing_data*) argument[0];

  if( !thing ) {
    code_bug( "Weight: Null argument." );
    return 0;
  }

  return (void*) thing->Weight( );
}


const void *code_race( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];

  if( !ch ) {
    code_bug( "Race: Null character." );
    return 0;
  }

  return (void*) ch->shdata->race;
}


const void *code_class( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];

  if( !ch ) {
    code_bug( "Class: Null character." );
    return 0;
  }

  if( ch->species ) {
    code_bug( "Class: Non-Player character." );
    return 0;
  }
   
  return (void*) ch->pcdata->clss;
}


const void *code_get_sex( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];

  if( !ch ) {
    code_bug( "Get_Sex: Null character." );
    return 0;
  }

  return (void*) ch->sex;
}


/*
 *   CFLAG ROUTINES
 */


const void *code_cflag( const void **argument )
{
  const int flag = (int) argument[0];
  char_data *ch = (char_data*)(thing_data*) argument[1]; 

  if( !ch ) {
    code_bug( "Cflag: NULL character." );
    return 0;
  }
  
  if( flag < 0 || flag >= 32*MAX_CFLAG ) {
    code_bug( "Cflag: flag out of range." );
    return 0;
  }
  
  if( ch->species )
    return 0;

  return (void*) is_set( ch->pcdata->cflags, flag );
}


const void *code_set_cflag( const void **argument )
{
  const int flag  = (int) argument[0];
  char_data *ch = (char_data*)(thing_data*) argument[1]; 

  if( !ch ) {
    code_bug( "Set_Cflag: NULL character." );
    return 0;
  }

  if( flag < 0 || flag >= 32*MAX_CFLAG ) {
    code_bug( "Set_Cflag: flag out of range." );
    return 0;
  }

  if( !ch->species )
    set_bit( ch->pcdata->cflags, flag );

  return 0;
}


const void *code_remove_cflag( const void **argument )
{
  const int flag = (int) argument[0];
  char_data *ch = (char_data*)(thing_data*) argument[1]; 

  if( !ch ) {
    code_bug( "Remove_Cflag: NULL character." );
    return 0;
  }

  if( flag < 0 || flag >= 32*MAX_CFLAG ) {
    code_bug( "Remove_Cflag: flag out of range." );
    return 0;
  }

  if( !ch->species )
    remove_bit( ch->pcdata->cflags, flag );

  return 0;
}


/*
 *   CHARACTER STATUS ROUTINES
 */


const void *code_can_fly( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];

  if( !ch ) {
    code_bug( "Can_Fly: NULL Character." );
    return 0;
  }
  
  return (void*) ( ch->mount ? ch->mount->can_fly( ) : ch->can_fly( ) );
}


const void *code_is_creature( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];

  if( !ch ) {
    code_bug( "Is_Creature: NULL Character." );
    return 0;
  }
  
  return (void*) is_mob( ch );
}


const void *code_is_not_player( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];

  if( !ch ) {
    code_bug( "Is_Not_Player: NULL Character." );
    return 0;
  }
  
  return (void*) not_player( ch );
}


const void *code_is_confused_pet( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];

  if( !ch ) {
    code_bug( "Is_Confused_Pet: NULL Character." );
    return 0;
  }
  
  return (void*) is_confused_pet( ch );
}


const void *code_is_silenced( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];
  const char *string = (const char*) argument[1];

  if( !ch ) {
    code_bug( "Is_Silenced: NULL Character." );
    return 0;
  }
  
  return (void*) is_silenced( ch, string );
}


const void *code_is_entangled( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];
  const char *string = (const char*) argument[1];

  if( !ch ) {
    code_bug( "Is_Entangled: NULL Character." );
    return 0;
  }
  
  return (void*) is_entangled( ch, string );
}


const void *code_can_hear( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];

  if( !ch ) {
    code_bug( "Can_Hear: NULL Character." );
    return 0;
  }
  
  return (void*) ch->Can_Hear( );
}


const void *code_can_bash( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];
  char_data *victim = (char_data*)(thing_data*) argument[1];

  if( !victim ) {
    code_bug( "Can_Bash: NULL victim." );
    return 0;
  }
  
  if( victim->mount )
    victim = victim->mount;

  return (void*) can_bash( ch, victim, false );
}


/*
 *   UNCLASSIFIED 
 */


const void *code_area( const void **argument )
{
  room_data *room = (room_data*)(thing_data*) argument[0];

  if( !room ) {
    code_bug( "Area: NULL room." );
    return 0;
  }

  return (void *) room->area->room_first->vnum;
}


const void *code_room_next( const void **argument )
{
  room_data *room = (room_data*)(thing_data*) argument[0];

  if( !room ) {
    code_bug( "Room_Next: NULL room." );
    return 0;
  }

  return (void *) room->next;
}


const void *code_interpret( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];
  const char *string = (const char*) argument[1];
  thing_data *thing = (thing_data*) argument[2];

  char_data *victim = character( thing );
  obj_data *obj = object( thing );

  if( thing && !victim && !obj ) {
    code_bug( "Interpret: Bad target." );
    return 0;
  }

  if( ch && ch->Level( ) >= LEVEL_APPRENTICE ) {
    //      || victim && victim->Level( ) >= LEVEL_APPRENTICE ) {
    code_bug( "Interpret: Security violation." );
    return 0;
  }

  if( !string || !*string ) {
    code_bug( "Interpret: null string." );
    return 0;
  }

  if( ch ) {
    push();
    if( !thing )
      interpret( ch, string );
    else {
      // What a hack.
      char buf [ MAX_INPUT_LENGTH ];	// Cannot use lib_buf here; cmd may cause re-entry.
      snprintf( buf, MAX_INPUT_LENGTH, "%s %s", string, thing->Keywords( ch ) );
      for( char *s = buf + strlen( string ) + 1; *s; ++s ) {
	if( *s == ' ' )
	  *s = '.';
      }
      interpret( ch, buf );
    }
    pop();
  }

  return 0;
}


const void *code_send_not_char( const void **argument )
{
  const char *string = (const char*) argument[0];
  char_data *ch = (char_data*)(thing_data*) argument[1];

  if( !string ) {
    code_bug( "Send_not_char: null string." );
    return 0;
  }

  if( ch ) {
    for( int i = 0; i < *ch->array; ++i ) {
      char_data *rch = character( ch->array->list[i] );
      if( rch
	  && rch != ch
	  && rch->position > POS_SLEEPING ) {
	const size_t len = strlen( string );
	const bool has_lf = isspace( *( string + len - 1 ) );
	convert_to_ansi( rch, LIB_BUF_LEN, string, lib_buf );
	if( !has_lf ) {
	  strcat( lib_buf, "\n\r" );
	}
	send( rch, lib_buf );
      }
    }
  }
  
  return 0;
}


const void *code_send_to_char( const void **argument )
{
  const char *string = (const char*) argument[0];
  char_data *ch = (char_data*)(thing_data*) argument[1];

  if( !string ) {
    code_bug( "Send_to_char: null string." );
    return 0;
  }

  if( ch ) {
    const size_t len = strlen( string );
    const bool has_lf = isspace( *( string + len - 1 ) );
    convert_to_ansi( ch, LIB_BUF_LEN, string, lib_buf );
    if( !has_lf ) {
      strcat( lib_buf, "\n\r" );
    }
    send( ch, lib_buf );
  }
 
  return 0;
}


const void *code_send_to_room( const void **argument )
{
  const char *string = (const char*) argument[0];
  room_data *room = (room_data*)(thing_data*) argument[1];

  if( !string ) {
    code_bug( "Send_to_toom: null string." );
    return 0;
  }

  if( room ) {
    for( int i = 0; i < room->contents; i++ ) {
      char_data *rch;
      if( ( rch = character( room->contents[i] ) )
	  && rch->position > POS_SLEEPING ) {
	const size_t len = strlen( string );
	const bool has_lf = isspace( *( string + len - 1 ) );
	convert_to_ansi( rch, LIB_BUF_LEN, string, lib_buf );
	if( !has_lf ) {
	  strcat( lib_buf, "\n\r" );
	}
	send( rch, lib_buf );
      }
    }
  }
  
  return 0;
}


bool send_range( char_data *ch, thing_data *room, const char *string,
		 int = 0, int = 0, int = 100, int = 50 )
{
  if( ch->position > POS_SLEEPING
      && ch->in_room
      && ch->in_room != room ) {
    const size_t len = strlen( string );
    const bool has_lf = isspace( *( string + len - 1 ) );
    convert_to_ansi( ch, LIB_BUF_LEN, string, lib_buf );
    if( !has_lf ) {
      strcat( lib_buf, "\n\r" );
    }
    send( ch, lib_buf );
  }

  return false;
}


const void *code_send_to_area( const void **argument )
{
  const char *string = (const char*) argument[0];
  room_data *room = (room_data*)(thing_data*) argument[1];
  int range = (int) argument[2];

  if( range < 0 ) {
    code_bug( "Send_To_Area: Negative range." );
    return 0;
  }

  if( !string ) {
    code_bug( "Send_to_area: null string." );
    return 0;
  }

  if( room ) {
    if( range == 0 ) {
      area_data *area = room->area;
      for( room = area->room_first; room; room = room->next ) {
	for( int i = 0; i < room->contents; i++ ) {
	  char_data *rch;
	  if( ( rch = character( room->contents[i] ) )
	      && rch->position > POS_SLEEPING ) {
	    const size_t len = strlen( string );
	    const bool has_lf = isspace( *( string + len - 1 ) );
	    convert_to_ansi( rch, LIB_BUF_LEN, string, lib_buf );
	    if( !has_lf ) {
	      strcat( lib_buf, "\n\r" );
	    }
	    send( rch, lib_buf );
	  }
	}
      }
    } else {
      // Ranged.
      exec_range( room, range, send_range, string );
    }
  }
 
  return 0;
}


const void *code_act_room( const void **argument )
{
  const char *string = (const char*) argument[0];
  char_data *ch = (char_data*)(thing_data*) argument[1];
  obj_data *obj = (obj_data*)(thing_data*) argument[2];
  thing_data *thing = (thing_data*) argument[3];
  const char *string1 = (const char*) argument[4];
  const char *string2 = (const char*) argument[5];

  if( !string || !*string ) {
    code_bug( "Act_Room: null string." );
    return 0;
  }

  if( ch && ch->in_room )
    act_room( ch->in_room, string, ch, character( thing ), obj, object( thing ), 0, string1, string2 );

  return 0;
}


const void *code_act_neither( const void **argument )
{
  const char *string = (const char*) argument[0];
  char_data *ch = (char_data*)(thing_data*) argument[1];
  obj_data *obj = (obj_data*)(thing_data*) argument[2];
  thing_data *thing = (thing_data*) argument[3];
  const char *string1 = (const char*) argument[4];
  const char *string2 = (const char*) argument[5];
  
  if( !string || !*string ) {
    code_bug( "Act_Neither: null string." );
    return 0;
  }

  msg_type = MSG_STANDARD;
  act_neither( string, ch, character( thing ), obj, object( thing ), string1, string2 );

  return 0;
}


const void *code_act_tochar( const void **argument )
{
  const char *string = (const char*) argument[0];
  char_data *ch = (char_data*)(thing_data*) argument[1];
  obj_data *obj = (obj_data*)(thing_data*) argument[2];
  thing_data *thing = (thing_data*) argument[3];
  const char *string1 = (const char*) argument[4];
  const char *string2 = (const char*) argument[5];
  
  if( !string || !*string ) {
    code_bug( "Act_Tochar: null string." );
    return 0;
  }

  act( ch, string, ch, character( thing ), obj, object( thing ), 0, string1, string2 );

  return 0;
}


const void *code_act_area( const void **argument )
{
  const char *string = (const char*) argument[0];
  char_data *ch = (char_data*)(thing_data*) argument[1];
  obj_data *obj = (obj_data*)(thing_data*) argument[2];
  thing_data *thing = (thing_data*) argument[3];
  const char *string1 = (const char*) argument[4];
  const char *string2 = (const char*) argument[5];

  if( !ch ) {
    code_bug( "Act_Area: Character = null pointer." );
    return 0;
  }

  if( !string || !*string ) {
    code_bug( "Act_Area: null string." );
    return 0;
  }

  act_area( string, ch, character( thing ), obj, object( thing ), string1, string2 );

  return 0;
}


const void *code_act_notchar( const void **argument )
{
  const char *string = (const char*) argument[0];
  char_data *ch = (char_data*)(thing_data*) argument[1];
  obj_data *obj = (obj_data*)(thing_data*) argument[2];
  thing_data *thing = (thing_data*) argument[3];
  const char *string1 = (const char*) argument[4];
  const char *string2 = (const char*) argument[5];
  
  if( !string || !*string ) {
    code_bug( "Act_Notchar: null string." );
    return 0;
  }

  if( ch ) {
    msg_type = MSG_STANDARD;
    act_notchar( string, ch, character( thing ), obj, object( thing ), 0, string1, string2 );
  }

  return 0;
}


const void *code_act_notvict( const void **argument )
{
  const char *string = (const char*) argument[0];
  char_data *ch = (char_data*)(thing_data*) argument[1];
  obj_data *obj = (obj_data*)(thing_data*) argument[2];
  thing_data *thing = (thing_data*) argument[3];
  const char *string1 = (const char*) argument[4];
  const char *string2 = (const char*) argument[5];
  
  if( !string || !*string ) {
    code_bug( "Act_Notvict: null string." );
    return 0;
  }

  msg_type = MSG_STANDARD;
  act_notvict( string, ch, character( thing ), obj, object( thing ), string1, string2 );

  return 0;
}


const void *code_junk_mob( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];

  if( !ch )
    return 0;
 
  if( !ch->species ) {
    code_bug( "Junk_Mob: character is a player." );
    return 0;
  } 
  
  ch->Extract( );

  return 0;
}


const void *code_drain_stat( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];
  const int i = (int) argument[1]; 
  const int j = (int) argument[2];
  const int m = (int) argument[3];

  if( !ch ) {
    code_bug( "Drain_Stat: NULL character." );
    return 0;
  }

  if( i < 0 || i > 21 ) {
    code_bug( "Drain_Stat: Impossible field." );
    return 0;
  }

  if( i == 9 ) {
    // Prayer.
    if( ch->species )
      return 0;
    player_data *pc = (player_data*) ch;
    pc->prayer = range( 0, pc->prayer + m, 1000 );
    return 0;
  }

  if( i == 18 ) {
    // Hunger.
    gain_condition( ch, COND_FULL, m );
    return 0;
  }

  if( i == 19 ) {
    // Thirst.
    gain_condition( ch, COND_THIRST, m );
    return 0;
  }

  if( i == 20 ) {
    // Alcohol.
    ch->condition[ COND_ALCOHOL ] += m;
    if( ch->condition[ COND_ALCOHOL ] < 0 )
      ch->condition[ COND_ALCOHOL ] = 0;
    return 0;
  }

  if( i == 21 ) {
    // Drunk.
    gain_drunk( ch, m );
    return 0;
  }

  if( i > 4 || j <= 0 || m < -5 || m > 5 )
    return 0;
 
  const int loc[] = { APPLY_STR, APPLY_INT, APPLY_WIS, APPLY_DEX, APPLY_CON };

  affect_data affect; 
  affect.type      = AFF_NONE;
  affect.location  = loc[i];
  affect.modifier  = m;
  affect.duration  = j;
  affect.level     = 5;

  add_affect( ch, &affect );
  return 0;
}


const void *code_find_stat( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];
  int i = (int) argument[1]; 

  if( !ch ) {
    code_bug( "Find_Stat: NULL character." );
    return 0;
  }

  if( i < 0 || i > 21 ) {
    code_bug( "Find_Stat: Impossible field." );
    return 0;
  }

  if( ch->species ) {
    const int value[] = { ch->Strength( ), ch->Intelligence( ),
			  ch->Wisdom( ), ch->Dexterity( ), ch->Constitution( ),
			  ch->Level(), 0, 0, ch->shdata->alignment, 0, 0, 0,
			  ch->hit, ch->mana, ch->move,
			  ch->max_hit, ch->max_mana, ch->max_move,
			  50, 50, 0, 0
    };
    return (void*) value[i];

  } else {
    player_data *pc = (player_data*) ch;
    const int value[] = { ch->Strength( ), ch->Intelligence( ),
			  ch->Wisdom( ), ch->Dexterity( ), ch->Constitution( ),
			  ch->Level(), ch->pcdata->piety, ch->pcdata->clss,
			  ch->shdata->alignment, pc->prayer,
			  ch->pcdata->quest_pts, ch->pcdata->pfile->remort,
			  ch->hit, ch->mana, ch->move,
			  ch->max_hit, ch->max_mana, ch->max_move,
			  ch->condition[ COND_FULL ],
			  ch->condition[ COND_THIRST ],
			  ch->condition[ COND_ALCOHOL ],
			  ch->condition[ COND_DRUNK ]
    };
    return (void*) value[i];
  }
}


const void *code_random( const void **argument )
{
  const int n1 = (int) argument[0];
  const int n2 = (int) argument[1];

  return (void*) ( number_range( n1,n2 ) );
}


const void *code_dice( const void **argument )
{
  const int n1 = (int) argument[0];
  const int n2 = (int) argument[1];
  const int n3 = (int) argument[2];

  return (void*) ( roll_dice( n1,n2 )+n3 );
}


const void *code_find_skill( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];
  const int skill = (int) argument[1];

  if( !ch ) {
    code_bug( "Find_Skill: NULL character." );
    return 0;
  }
  
  return (void*) ch->get_skill( skill );
}


const void *code_use_skill( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];
  const int skill = (int) argument[1];

  if( !ch ) {
    code_bug( "Use_Skill: NULL character." );
    return 0;
  }
  
  ch->improve_skill( skill );

  return 0;
}


const void *code_check_skill( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];
  const int skill = (int) argument[1];
  const int chance = (int) argument[2];

  if( !ch ) {
    code_bug( "Check_Skill: NULL character." );
    return 0;
  }
  
  return (void*) ch->check_skill( skill, chance );
}


const void *code_summon( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];
  char_data *victim = (char_data*)(thing_data*) argument[1];
  int range = (int) argument[2];
  int delay = (int) argument[3];

  if( range == 0 ) {
    range = 15;
  } else if( range < 0 ) {
    code_bug( "Summon: negative range." );
    return 0;
  }

  if( delay == 0 ) {
    delay = 50;
  } else if( delay < 0 ) {
    code_bug( "Summon: negative delay." );
    return 0;
  }

  if( ch )
    summon_help( ch, victim, range, delay );

  return 0;
}


const void *code_reputation( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];
  const int nation = (int) argument[1];

  if( !ch ) {
    code_bug( "Reputation: Null character." );
    return 0;
  }
  
  return (void*) 50;
}


const void *code_find_room( const void **argument )
{
  const int vnum = (int) argument[0];

  room_data *room = get_room_index( vnum, false );

  if( !room ) {
    code_bug( "Find_Room: Non-existent room." );
    return 0;
  }
  
  return room;
}


const void *code_spell_damage( const void **argument )
{
  const int spell = (int) argument[0];
  const int level = (int) argument[1];

  if( spell < SPELL_FIRST
      || spell >= SPELL_FIRST+table_max[ TABLE_SKILL_SPELL ] ) {
    code_bug( "Spell_Damage: Invalid spell." );
    return 0;
  }
  
  if( level < UNLEARNT
      || level > MAX_SPELL_LEVEL ) {
    code_bug( "Spell_Damage: Invalid level %d.", level );
    return 0;
  }

  return (void*) spell_damage( spell, level );
}


const void *code_spell_duration( const void **argument )
{
  const int spell = (int) argument[0];
  const int level = (int) argument[1];

  if( spell < SPELL_FIRST
      || spell >= SPELL_FIRST+table_max[ TABLE_SKILL_SPELL ] ) {
    code_bug( "Spell_Duration: Invalid spell." );
    return 0;
  }
  
  if( level < UNLEARNT
      || level > MAX_SPELL_LEVEL ) {
    code_bug( "Spell_Duration: Invalid level %d.", level );
    return 0;
  }

  return (void*) duration( spell, level );
}


const void *code_is_casting( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];

  if( !ch ) {
    code_bug( "Is_Casting: NULL character." );
    return 0;
  }

  return ch->cast;
}


const void *code_has_prepared( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];
  int spell = (int) argument[1];

  if( !ch ) {
    code_bug( "Has_Prepared: NULL character." );
    return 0;
  }

  if( spell < SPELL_FIRST
      || spell >= SPELL_FIRST+table_max[ TABLE_SKILL_SPELL ] ) {
    code_bug( "Has_Prepared: Invalid spell." );
    return 0;
  }
  
  if( ch->species ) {
    return (void*) -1;
  }

  spell -= SPELL_FIRST;

  for( cast_data *prepare = ch->prepare; prepare; prepare = prepare->next ) {
    if( prepare->spell == spell ) {
      return (void*) prepare->times;
    }
  }
  
  return 0;
}


const void *code_cast_mana( const void **argument )
{
  int spell = (int) argument[0];
  char_data *ch = (char_data*)(thing_data*) argument[1];

  if( !ch ) {
    code_bug( "Cast_Mana: NULL caster." );
    return 0;
  }

  if( spell < SPELL_FIRST
      || spell >= SPELL_FIRST+table_max[ TABLE_SKILL_SPELL ] ) {
    code_bug( "Cast_Mana: Invalid spell." );
    return 0;
  }

  spell -= SPELL_FIRST;

  return (void*) mana_cost( ch, spell, "Cast_Mana" );
}


const void *code_cast_spell( const void **argument )
{
  int spell = (int) argument[0];
  char_data *ch = (char_data*)(thing_data*) argument[1];
  thing_data *target = (thing_data*) argument[2];
  int skill = (int) argument[3];
  int speed = (int) argument[4];

  if( !ch ) {
    code_bug( "Cast_Spell: NULL caster." );
    return 0;
  }

  if( ch->cast ) {
    code_bug( "Cast_Spell: Caster already busy casting." );
    return 0;
  }

  if( ch->position < POS_STANDING ) {
    pos_message( ch );
    return 0;
  }

  if( is_confused_pet( ch )
      || is_familiar( ch )
      || is_silenced( ch, "cast spells" )
      || is_entangled( ch, "cast spells" )
      || is_drowning( ch, "cast spells" ) )
    return 0;
  
  char_data *victim;
  obj_data *obj;
  char_array audience;

  if( !check_target( spell, ch, target, victim, obj, audience, "Cast_spell" ) )
    return 0;

  // Reverse do_spell( peaceful, ch ) -> do_spell( peaceful, null, ch ) for cast_spell.
  if( !ch ) {
    ch = victim;
  }
  if( victim ) {
    target = victim;
  }

  if( skill < 0 || skill > MAX_SPELL_LEVEL ) {
    code_bug( "Cast_Spell: Invalid level." );
    skill = 0;
  }

  // *** FIX ME: use MSKILL here.
  if( skill == 0 ) {
    skill = ch->get_skill( SPELL_FIRST+spell );
  }

  if( speed < 0 || speed > MAX_SPELL_LEVEL ) {
    code_bug( "Cast_Spell: Invalid speed." );
    speed = 0;
  }

  if( speed == 0 ) {
    speed = skill;
  }

  if( !allowed_location( ch, &skill_spell_table[spell].location,
			 "cast", skill_spell_table[spell].name ) )
    return 0;

  cast_data *cast = new cast_data;
  cast->spell = spell;
  cast->prepare = false;
  cast->wait = skill_spell_table[spell].prepare-1;
  cast->mana = 0;
  cast->target = target;
  cast->level = skill;
  cast->speed = speed;
  cast->audience = audience;
  
  if( !has_reagents( ch, cast ) ) {
    delete cast;
    return 0;
  }
  
  fsend( ch, "You begin casting %s.", skill_spell_table[spell].name );
  
  ch->cast = cast;

  const int delay = max( 5, 10 - speed/2 );

  set_delay( ch, delay );

  return (void*) true;
}


const void *code_do_spell( const void **argument )
{
  int spell = (int) argument[0];
  char_data *ch = (char_data*)(thing_data*) argument[1];
  thing_data *target = (thing_data*) argument[2];
  int skill = (int) argument[3];

  char_data *victim;
  obj_data *obj;
  char_array audience;

  if( !check_target( spell, ch, target, victim, obj, audience, "Do_spell" ) )
    return 0;

  if( skill < 0 || skill > MAX_SPELL_LEVEL ) {
    code_bug( "Do_Spell: Invalid level." );
    skill = 0;
  }

  // *** FIX ME: use MSKILL here.
  if( skill == 0 ) {
    skill = ch ? ch->get_skill( SPELL_FIRST+spell ) : 10;
  }

  if( ch ) {
    // Adapted from spell_update() in magic.cc.
    if( is_set( ch->in_room->room_flags, RFLAG_NO_MAGIC ) ) {
      return 0;
    }

    remove_bit( ch->status, STAT_WIMPY );
    strip_affect( ch, AFF_INVISIBLE );
    leave_shadows( ch );
  }

  push( );

  if( cast_triggers( spell, ch, target, victim, obj ) ) {
    pop( );
    return 0;
  }

  if( !react_spell( ch, victim, spell ) ) {
    pop( );
    return 0;
  }

  time_data start;
  gettimeofday( &start, 0 );
  
  if( Tprog_Data *tprog = skill_spell_table[spell].prog ) {
    clear_variables( );
    var_i = skill;
    var_j = 0; // duration 0
    var_ch = ch;
    var_room = ch ? ch->in_room : victim ? victim->in_room : 0;
    var_victim = victim;
    var_obj = object( target );
    var_exit = exit( target );
    var_list = (thing_array&) audience;
    const bool result = tprog->execute( );
    if( !result
	|| ch && !ch->Is_Valid( ) ) {
      pop( );
      finish_spell( start, spell );
      return 0;
    }
  }

  void *vo = ( skill_spell_table[spell].type != STYPE_SONG )
    ? (void*)target : (void*)&audience;

  const bool result = skill_spell_table[spell].function
    ? ( *skill_spell_table[spell].function )( ch, victim, vo, skill, 0 )
    : false;

  pop( );
  finish_spell( start, spell );
  
  return (void*) result;
}


const void *code_num_in_room( const void **argument )
{
  room_data *room = (room_data*)(thing_data*) argument[0];

  int num = 0;

  if( room )
    for( int i = 0; i < room->contents; i++ ) {
      if( char_data *rch = character( room->contents[i] ) ) {
	if( invis_level( rch ) < LEVEL_BUILDER ) {
	  ++num;
	}
      }
    }
  
  return (void*) num;
}  


const void *code_players_area( const void **argument )
{
  room_data *room = (room_data*)(thing_data*) argument[0];

  if( !room ) {
    code_bug( "Players_Area: NULL room." );
    return 0;
  }

  return (void*) room->area->nplayer;
}  


const void *code_players_room( const void **argument )
{
  room_data *room = (room_data*)(thing_data*) argument[0];

  int num = 0;  

  if( room ) {
    for( int i = 0; i < room->contents; i++ ) {
      if( player_data *pl = player( room->contents[i] ) ) {
	if( invis_level( pl ) < LEVEL_BUILDER ) {
	  ++num;
	}
      }
    }
  }

  return (void*) num;
}  


const void *code_num_mob( const void **argument )
{
  const int vnum = (int) argument[0];
  room_data *room = (room_data*)(thing_data*) argument[1];

  mob_data*    rch;
  int          num  = 0;  

  if( room )
    for( int i = 0; i < room->contents; i++ )
      if( ( rch = mob( room->contents[i] ) )
	  && rch->species->vnum == vnum )
        ++num;

  return (void*) num;
}  


const void *code_transfer( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];
  room_data *room = (room_data*)(thing_data*) argument[1];

  if( !room ) {
    code_bug( "Transfer: Null destination." );
    return (void*)false;
  }

  if( !ch )
    return (void*)false;

  if( ch->rider )
    ch = ch->rider;

  if( ch->Size( ) > room->size ) {
    code_bug( "Transfer: character too large for destination room." );
    return 0;
  }

  if( ch->mount
      && ch->mount->in_room == ch->in_room
      && ch->mount->Size( ) > room->size ) {
    code_bug( "Transfer: character's mount too large for destination room." );
    return 0;
  }

  // If it's a one-room transfer and there's a door, set room_pos.
  int pos = -1;
  for( int i = 0; i < ch->in_room->exits; ++i ) {
    exit_data *exit = ch->in_room->exits[i];
    if( exit->to_room == room ) {
      pos = dir_table[exit->direction].reverse;
      break;
    }
  }

  push();

  action_data *action;
  if( !trigger_leaving( ch, ch->in_room, 0, DIR_TRANSFER, action ) )
    return (void*) false;

  const bool mount = ch->mount && ch->mount->in_room == ch->in_room;

  if( mount ) {
    ch->mount->From( );
  }

  ch->From( );

  if( mount ) {
    ch->mount->To( room );
    ch->mount->room_position = pos;
  }

  ch->To( room );
  ch->room_position = pos;

 if( ch->position > POS_SLEEPING ) {
    send( ch, "\n\r" );
    show_room( ch, room, true, true );
  }

  if( ch->mount
      && ch->mount->position > POS_SLEEPING ) {
    send( ch->mount, "\n\r" );
    show_room( ch->mount, room, true, true );
  }

  trigger_entering( ch, room, DIR_TRANSFER, 0 );

  pop();

   return (void*)true;
}


const void *code_transfer_all( const void **argument )
{
  room_data *from = (room_data*)(thing_data*) argument[0];
  room_data *to = (room_data*)(thing_data*) argument[1];

  if( !from ) {
    code_bug( "Transfer_All: Null origin." );
    return 0;
  }

  if( !to ) {
    code_bug( "Transfer_All: Null destination." );
    return 0;
  }

  thing_array list = from->contents;
  char_array list2;
  bool first = true;
  int count = 0;
  int pos = -1;

  for( int i = 0; i < list; ++i ) {
    char_data *rch = character( list[i] );
    if( rch
	&& invis_level( rch ) < LEVEL_BUILDER
	&& rch->Size( ) <= to->size
	&& ( !rch->mount
	     || rch->mount->in_room != from
	     || rch->mount->Size( ) <= to->size ) ) {
      if( first ) {
	first = false;
	// If it's a one-room transfer and there's a door, set room_pos.
	for( int i = 0; i < from->exits; ++i ) {
	  exit_data *exit = from->exits[i];
	  if( exit->to_room == to ) {
	    pos = dir_table[exit->direction].reverse;
	    break;
	  }
	}
	push( );
      }
      action_data *action;
      if( !trigger_leaving( rch, from, 0, DIR_TRANSFER, action ) )
	continue;
      list2 += rch;
      rch->From( );
      rch->To( to );
      rch->room_position = pos;
      ++count;
    }
  }

  for( int i = 0; i < list2; ++i ) {
    char_data *rch = character( list2[i] );
    if( rch->position > POS_SLEEPING ) {
      send( rch, "\n\r" );
      show_room( rch, to, true, true );
    }

    trigger_entering( rch, to, DIR_TRANSFER, 0 );
  }

  if( !first )
    pop( );

  return (void*)count;
}


const void *code_mload( const void **argument )
{
  const int vnum = (int) argument[0];
  room_data *room = (room_data*)(thing_data*) argument[1];

  species_data*  species;

  if( !room || !( species = get_species( vnum ) ) ) {
    code_bug( "Mload: non-existent species or null room." );
    return 0;
  }

  push();

  mob_data *mob = new Mob_Data( species );
  mreset_mob( mob );
  mob->To( room );
  mob_setup( mob, room );

  pop();

  return mob;
}


const void *code_rand_char( const void **argument )
{
  room_data *room = (room_data*)(thing_data*) argument[0];
  char_data *ch = (char_data*)(thing_data*) argument[1];

  if( !room )
    return 0;

  char_data *rch = random_pers( room, ch );
  
  return rch;
}


const void *code_rand_player( const void **argument )
{
  room_data *room = (room_data*)(thing_data*) argument[0];
  char_data *ch = (char_data*)(thing_data*) argument[1];

  if( !room )
    return 0;

  char_data *rch = rand_player( room, ch );
  
  return rch;
}


const void *code_mob_in_room( const void **argument )
{
  const int vnum = (int) argument[0];
  room_data *room = (room_data*)(thing_data*) argument[1];

  char_data*   rch;

  if( !room ) {
    code_bug( "Mob_in_Room: Null room." );
    return 0;
  }

  for( int i = 0; i < room->contents; ++i ) {
    if( ( rch = mob( room->contents[i] ) )
	&& rch->species->vnum == vnum ) {
      rch->Show( 1 );
      return rch;
    }
  }
  
  return 0;
}


const void *code_coin_value( const void **argument )
{
  obj_data *obj = (obj_data*)(thing_data*) argument[0];

  if( !obj || obj->pIndexData->item_type != ITEM_MONEY )  
    return 0;

  for( int i = 0; i < MAX_COIN; i++ ) 
    if( obj->pIndexData->vnum == coin_vnum[i] ) 
      return (void*) ( coin_value[i]*obj->Number( ) );

  return 0;
}


const void *code_get_money( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];

  if( !ch ) {
    code_bug( "Get_Money: NULL character." );
    return 0;
  }

  return (void*) get_money( ch );
}


const void *code_remove_money( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];
  const int amount = (int) argument[1];
  const char *message = (const char*) argument[2];

  if( !ch ) {
    code_bug( "Remove_Money: NULL character." );
    return 0;
  }

  if( amount <= 0 ) {
    code_bug( "Remove_Money: Amount is <= 0." );
    return 0;
  }

  return (void*) remove_coins( ch, amount, message );
}


const void *code_plague( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];
  int duration = (int) argument[1];
 
  if( !ch ) {
    code_bug( "Plague: NULL character." );
    return 0;
  }

  if( duration < 0 ) {
    code_bug( "Plague: negative duration." );
    return 0;
  }

  if( duration == 0 )
    duration = number_range( 5, 12 );

  const bool worked = plague( ch, duration, false );

  // Plague level simply controls message sequence.
  if( worked
      && var_mob
      && var_mob->species
      && !is_set( var_mob->status, STAT_PET ) )
    var_mob->species->special += 20;

  return (void*) worked;
}


const void *code_poison( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];
  int duration = (int) argument[1];

  if( !ch ) {
    code_bug( "Poison: NULL character." );
    return 0;
  }
  
  if( duration < 0 ) {
    code_bug( "Poison: negative duration." );
    return 0;
  }

  if( duration == 0 )
    duration = 10;

  bool worked = poison( ch, 5, duration, false );

  if( worked
      && var_mob
      && var_mob->species
      && !is_set( var_mob->status, STAT_PET ) )
    var_mob->species->special += 10;

  return (void*) worked;
}


const void *code_heal( const void **argument )
{
  char_data* ch = (char_data*)(thing_data*) argument[0];
  const int i = (int) argument[1];

  if( !ch ) {
    code_bug( "Heal: NULL character." );
    return 0;
  }
  
  if( ch->hit > 0 && ch->hit + i <= 0 )
    return 0;

  ch->hit = min( ch->hit + i, ch->max_hit );

  update_pos( ch );
  update_max_move( ch );

  return (void*) true;
}


const void *code_modify_mana( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];
  const int i = (int) argument[1];

  if( !ch ) {
    code_bug( "Modify_Mana: NULL character." );
    return 0;
  }
  
  if( ch->mana+i < 0 ) {
    //    ch->mana = 0;
    return 0;
  }

  ch->mana = min( ch->mana + i, ch->max_mana );

  return (void*) true;
}


const void *code_drain_exp( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];
  const int i = (int) argument[1];

  if( !ch ) {
    code_bug( "Drain_Exp: NULL character." );
    return 0;
  }
  
  if( !ch->species ) {
    snprintf( lib_buf, LIB_BUF_LEN, "You %s %%d experience point%%s!\n\r",
	      i > 0 ? "lose" : "gain" );
    add_exp( ch, -i, lib_buf );
  }

  return 0;
}


/*
 *   FUNCTION CALLS
 */


const void *code_acode( const void **argument )
{
  room_data *room = (room_data*)(thing_data*) argument[0];
  const int i = (int) argument[1];

  if( !room ) {
    code_bug( "Acode: NULL room." );
    return 0;
  }

  int j = 1;

  for( action_data *action = room->action; action; action = action->next ) 
    if( j++ == i ) {
      push();
      const int result = action->execute( room );
      pop( result == -1 );
      return (void*)result;
    }
  
  code_bug( "Acode: action not found." );
  return 0;
}


const void *code_mpcode( const void **argument )
{
  char_data *mob = (char_data*)(thing_data*) argument[0];
  const int i = (int) argument[1];

  if( !mob || !mob->species ) {
    code_bug( "Mpcode: NULL mob or mob is a player." );
    return 0;
  }

  int j = 1;

  for( mprog_data *mprog = mob->species->mprog; mprog; mprog = mprog->next ) 
    if( j++ == i ) {
      push( );
      const int result = mprog->execute( mob );
      pop( result == -1 );
      return (void*)result;
    }

  code_bug( "Mpcode: mprog not found." );
  return 0;
}


const void *code_mprog( const void **argument )
{
  const int n = (int) argument[0];
  const int i = (int) argument[1];

  species_data *species = get_species( n );

  if( !species ) {
    code_bug( "Mprog: bad mob vnum." );
    return 0;
  }

  int j = 1;

  for( mprog_data *mprog = species->mprog; mprog; mprog = mprog->next ) 
    if( j++ == i ) {
      push( );
      const int result = mprog->execute( );
      pop( result == -1 );
      return (void*)result;
    }

  code_bug( "Mprog: mprog not found." );
  return 0;
}


const void *code_opcode( const void **argument )
{
  obj_data *obj = (obj_data*)(thing_data*) argument[0];
  const int i = (int) argument[1];

  if( !obj ) {
    code_bug( "Opcode: NULL obj." );
    return 0;
  }
 
  int j = 1;

  for( oprog_data *oprog = obj->pIndexData->oprog; oprog; oprog = oprog->next ) 
    if( j++ == i ) {
      push( );
      const int result = oprog->execute( obj );
      pop( result == -1 );
      return (void*)result;
    }

  code_bug( "Opcode: oprog not found." );
  return 0;
}


const void *code_oprog( const void **argument )
{
  const int n = (int) argument[0];
  const int i = (int) argument[1];

  obj_clss_data *clss = get_obj_index( n );

  if( !clss ) {
    code_bug( "Oprog: bad obj vnum." );
    return 0;
  }
 
  int j = 1;

  for( oprog_data *oprog = clss->oprog; oprog; oprog = oprog->next ) 
    if( j++ == i ) {
      push( );
      const int result = oprog->execute( );
      pop( result == -1 );
      return (void*)result;
    }

  code_bug( "Oprog: oprog not found." );
  return 0;
}


const void *code_wait( const void **argument )
{
  const int i = (int) argument[0];

  queue_data *queue = new queue_data;

  queue->room 	   = var_room;
  queue->mob  	   = var_mob;
  queue->ch   	   = var_ch;
  queue->rch   	   = var_rch;
  queue->victim	   = var_victim;
  queue->obj  	   = var_obj;
  queue->i    	   = var_i;
  queue->j    	   = var_j;
  queue->k    	   = var_k;
  queue->l    	   = var_l;
  queue->m    	   = var_m;
  queue->n    	   = var_n;
  queue->act_obj   = var_act_obj;
  queue->container = var_container;
  queue->vcmd      = alloc_string( var_cmd, MEM_QUEUE );
  queue->varg      = alloc_string( var_arg, MEM_QUEUE );
  queue->exit      = var_exit;
  queue->def       = var_def;
  queue->def_type  = var_def_type;
  queue->thing     = var_thing;
  queue->list      = var_list;

  queue->o_victim  = orig_victim;
  queue->o_ch      = orig_ch;

  queue->time = i;
  queue->program = curr_prog;
  //  queue->arg = curr_arg;
  //  queue->own = curr_own;

  if( i <= 0 ) {
    queue->next = now_list;
    now_list = queue;
    now_prog = true;
  } else {
    queue->next = queue_list;
    queue_list = queue;
    queue_prog = true;
  }

  end_prog = true;
  
  return 0;
}


const void *code_is_searching( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];

  if( !ch ) {
    code_bug( "Is_Searching: NULL character." );
    return 0;
  }
  
  return (void*) ( !ch->species
		   && is_set( ch->pcdata->pfile->flags, PLR_SEARCHING ) );
}


const void *code_is_follower( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];

  if( !ch ) {
    code_bug( "Is_Follower: NULL character." );
    return 0;
  }
  
  return (void*) ( is_set( ch->status, STAT_FOLLOWER )
		   ? ch->leader : 0 );
}
  

const void *code_char_in_room( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];
  room_data *room = (room_data*)(thing_data*) argument[1]; 

  return (void*) ( ch && ch->in_room == room );
}


const void *code_is_name( const void **argument )
{
  const char *str = (const char*) argument[0];
  const char *namelist = (const char*) argument[1];

  if( !str || !namelist )
    return 0;

  return (void*) is_name( str, namelist );
}


const void *code_get_religion( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];

  if( !ch ) {
    code_bug( "Get_Religion: NULL character." );
    return 0;
  }
  
  if( ch->species )
    return 0;

  return (void *) ch->pcdata->religion;
}  


const void *code_set_religion( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];
  int i = (int) argument[1];

  if( !ch ) {
    code_bug( "Set_Religion: NULL character." );
    return 0;
  }
  
  if( ch->species )
    return 0;

  if( i < 0 || i >= MAX_RELIGION ) {
    code_bug( "Set_Religion: religion value out of range." );
    return 0;
  } 

  ch->pcdata->religion = i;

  return 0;
}  


const void *code_tell( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];
  char_data *victim = (char_data*)(thing_data*) argument[1];
  const char *string = (const char*) argument[2];
  obj_data *obj = (obj_data*)(thing_data*) argument[3];
  const char *string1 = (const char*) argument[4];
  const char *string2 = (const char*) argument[5];

  // The use of victim->pcdata is correct here.
  if( !victim || !ch || !victim->pcdata )
    return 0;

  act_print( lib_buf, string, victim, ch, obj, 0, string1, string2, 0, victim );
  process_tell( ch, victim, lib_buf );

  return 0;
}


const void *code_name( const void **argument )
{
  char_data *ch = (char_data *)(thing_data*) argument[0];
  thing_data *thing = (thing_data*) argument[1];

  if( !thing ) {
    code_bug( "Name: NULL target." );
    return (void*) empty_string;
  }

  return (void*) thing->Name( ch, 1, false );
}


/*
const void *code_disarm( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];
  const int j = (int) argument[1];

  if( !ch ) {
    code_bug( "Disarm: NULL character or level." );
    return 0;
  }

  return (void*) ( ch->check_skill( SKILL_UNTRAP )
		   && number_range( 0, 20 ) > j );
}
*/


/*
 *   MOVEMENT ROUTINES
 */


const void *code_modify_move( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];
  const int i = (int) argument[1];

  if( !ch ) {
    code_bug( "Modify_Move: NULL character." );
    return 0;
  }
  
  if( ch->move+i < 0 )
    return 0;

  ch->move = min( ch->move + i, ch->max_move );
 
  return (void*) true;
}


const void *code_is_exhausted( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];
  const int i = (int) argument[1];

  if( !ch ) {
    code_bug( "Is_Exhausted: NULL character." );
    return 0;
  }

  if( ch->move < i ) {
    send( ch, "You are too exhausted.\n\r" );
    return (void*) true;
  }
  
  ch->move -= i;
 
  return 0;
}


const void *code_char_affected( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];
  const int aff = (int) argument[1];
  
  if( !ch ) {
    code_bug( "Char_Affected: NULL char." );
    return 0;
  }

  if( aff < 0
      || aff >= MAX_AFF_CHAR ) {
    code_bug( "Char_Affected: affect value out of range." );
    return 0;
  }

  if( aff == AFF_HIDE ) {
    return (void*) is_set( ch->status, STAT_HIDING );
  }

  if( aff == AFF_CAMOUFLAGE ) {
    return (void*) is_set( ch->status, STAT_CAMOUFLAGED );
  }

  return (void*) ch->is_affected( aff );
}


const void *code_set_name( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];
  const char *string = (const char*) argument[1];
 
  if( !ch ) {
    code_bug( "Set_Name: NULL char." );
    return 0;
  }

  if( !ch->species ) {
    code_bug( "Set_Name: attempt to change player appearance." );
    return 0;
  }

  if( ch->descr == ch->species->descr ) {
    ch->descr = new Descr_Data( *ch->species->descr );
  }

  free_string( ch->descr->name, MEM_DESCR );
  ch->descr->name = alloc_string( string, MEM_DESCR );

  return 0;
}


const void *code_set_keywords( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];
  const char *string = (const char*) argument[1];
 
  if( !ch ) {
    code_bug( "Set_Keywords: NULL char." );
    return 0;
  }

  if( !ch->species ) {
    code_bug( "Set_Keywords: attempt to change player appearance." );
    return 0;
  }

  if( ch->descr == ch->species->descr ) {
    ch->descr = new Descr_Data( *ch->species->descr );
  }

  free_string( ch->descr->keywords, MEM_DESCR );
  ch->descr->keywords = alloc_string( string, MEM_DESCR );

  return 0;
}


const void *code_set_singular( const void **argument )
{
  thing_data *th = (thing_data*) argument[0];
  const char *string = (const char*) argument[1];

  char_data *ch = dynamic_cast<char_data *>( th );
  obj_data *obj = dynamic_cast<obj_data *>( th );

  if( ch ) {
    if( !ch->species ) {
      code_bug( "Set_Singular: attempt to change player appearance." );
      return 0;
    }
    
    if( ch->descr == ch->species->descr ) {
      ch->descr = new Descr_Data( *ch->species->descr );
    }
    
    free_string( ch->descr->singular, MEM_DESCR );
    ch->descr->singular = alloc_string( string, MEM_DESCR );
  } else if( obj ) {
    if( obj->singular != obj->pIndexData->singular ) {
      free_string( obj->singular, MEM_OBJECT );
    }
    obj->singular = alloc_string( string, MEM_OBJECT );
  } else {
    code_bug( "Set_Singular: argument is not a valid character or object." );
  }

  return 0;
}


const void *code_set_plural( const void **argument )
{
  thing_data *th = (thing_data*) argument[0];
  const char *string = (const char*) argument[1];

  char_data *ch = dynamic_cast<char_data *>( th );
  obj_data *obj = dynamic_cast<obj_data *>( th );

  if( ch ) {
    if( !ch->species ) {
      code_bug( "Set_Plural: attempt to change player appearance." );
      return 0;
    }
    
    if( ch->descr == ch->species->descr ) {
      ch->descr = new Descr_Data( *ch->species->descr );
    }
    
    free_string( ch->descr->plural, MEM_DESCR );
    ch->descr->plural = alloc_string( string, MEM_DESCR );
  } else if( obj ) {
    if( obj->plural != obj->pIndexData->plural ) {
      free_string( obj->plural, MEM_OBJECT );
    }
    obj->plural = alloc_string( string, MEM_OBJECT );
  } else {
    code_bug( "Set_Plural: argument is not a valid character or object." );
  }

  return 0;
}


const void *code_set_long_s( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];
  const char *string = (const char*) argument[1];
 
  if( !ch ) {
    code_bug( "Set_Long_S: NULL char." );
    return 0;
  }

  if( !ch->species ) {
    code_bug( "Set_Long_S: attempt to change player appearance." );
    return 0;
  }

  if( ch->descr == ch->species->descr ) {
    ch->descr = new Descr_Data( *ch->species->descr );
  }

  free_string( ch->descr->long_s, MEM_DESCR );
  ch->descr->long_s = alloc_string( string, MEM_DESCR );

  return 0;
}


const void *code_set_adj_s( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];
  const char *string = (const char*) argument[1];
 
  if( !ch ) {
    code_bug( "Set_Adj_S: NULL char." );
    return 0;
  }

  if( !ch->species ) {
    code_bug( "Set_Adj_S: attempt to change player appearance." );
    return 0;
  }

  if( ch->descr == ch->species->descr ) {
    ch->descr = new Descr_Data( *ch->species->descr );
  }

  free_string( ch->descr->adj_s, MEM_DESCR );
  ch->descr->adj_s = alloc_string( string, MEM_DESCR );

  return 0;
}


const void *code_set_prefix_s( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];
  const char *string = (const char*) argument[1];
 
  if( !ch ) {
    code_bug( "Set_Prefix_S: NULL char." );
    return 0;
  }

  if( !ch->species ) {
    code_bug( "Set_Prefix_S: attempt to change player appearance." );
    return 0;
  }

  if( ch->descr == ch->species->descr ) {
    ch->descr = new Descr_Data( *ch->species->descr );
  }

  free_string( ch->descr->prefix_s, MEM_DESCR );
  ch->descr->prefix_s = alloc_string( string, MEM_DESCR );

  return 0;
}


const void *code_set_long_p( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];
  const char *string = (const char*) argument[1];
 
  if( !ch ) {
    code_bug( "Set_Long_P: NULL char." );
    return 0;
  }

  if( !ch->species ) {
    code_bug( "Set_Long_P: attempt to change player appearance." );
    return 0;
  }

  if( ch->descr == ch->species->descr ) {
    ch->descr = new Descr_Data( *ch->species->descr );
  }

  free_string( ch->descr->long_p, MEM_DESCR );
  ch->descr->long_p = alloc_string( string, MEM_DESCR );

  return 0;
}


const void *code_set_adj_p( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];
  const char *string = (const char*) argument[1];
 
  if( !ch ) {
    code_bug( "Set_Adj_P: NULL char." );
    return 0;
  }

  if( !ch->species ) {
    code_bug( "Set_Adj_P: attempt to change player appearance." );
    return 0;
  }

  if( ch->descr == ch->species->descr ) {
    ch->descr = new Descr_Data( *ch->species->descr );
  }

  free_string( ch->descr->adj_p, MEM_DESCR );
  ch->descr->adj_p = alloc_string( string, MEM_DESCR );

  return 0;
}


const void *code_set_prefix_p( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];
  const char *string = (const char*) argument[1];
 
  if( !ch ) {
    code_bug( "Set_Prefix_P: NULL char." );
    return 0;
  }

  if( !ch->species ) {
    code_bug( "Set_Prefix_P: attempt to change player appearance." );
    return 0;
  }

  if( ch->descr == ch->species->descr ) {
    ch->descr = new Descr_Data( *ch->species->descr );
  }

  free_string( ch->descr->prefix_p, MEM_DESCR );
  ch->descr->prefix_p = alloc_string( string, MEM_DESCR );

  return 0;
}


const void *code_set_desc( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];
  const char *string = (const char*) argument[1];
 
  if( !ch ) {
    code_bug( "Set_Desc: NULL char." );
    return 0;
  }

  if( !ch->species ) {
    code_bug( "Set_Desc: attempt to change player appearance." );
    return 0;
  }

  if( ch->descr == ch->species->descr ) {
    ch->descr = new Descr_Data( *ch->species->descr );
  }

  free_string( ch->descr->complete, MEM_DESCR );
  ch->descr->complete = alloc_string( string, MEM_DESCR );

  return 0;
}


const void *code_set_color( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];
  int color = (int) argument[1];
 
  if( !ch ) {
    code_bug( "Set_Color: NULL char." );
    return 0;
  }

  if( !ch->species ) {
    code_bug( "Set_Color: attempt to change player appearance." );
    return 0;
  }

  ch->color = color;

  return 0;
}


const void *code_reset_appear(  const void **argument )
{
  thing_data *th = (thing_data*) argument[0];

  char_data *ch = dynamic_cast<char_data *>( th );
  obj_data *obj = dynamic_cast<obj_data *>( th );

  if( ch ) {
    if( !ch->species ) {
      code_bug( "Reset_Appear: attempt to change player appearance." );
      return 0;
    }
    
    if( ch->descr == ch->species->descr ) {
      return 0;
    }
    
    delete ch->descr;
    
    ch->descr = ch->species->descr;
    ch->color = ch->species->color;
    
  } else if( obj ) {
    if( obj->singular != obj->pIndexData->singular ) {
      free_string( obj->singular, MEM_OBJECT );
      obj->singular = obj->pIndexData->singular;
    }
    if( obj->plural != obj->pIndexData->plural ) {
      free_string( obj->plural, MEM_OBJECT );
      obj->plural = obj->pIndexData->plural;
    }
    if( obj->before != obj->pIndexData->before ) {
      free_string( obj->before, MEM_OBJECT );
      obj->before = obj->pIndexData->before;
    }
    if( obj->after != obj->pIndexData->after ) {
      free_string( obj->after, MEM_OBJECT );
      obj->after = obj->pIndexData->after;
    }
  } else {
    code_bug( "Reset_Appear: argument is not a valid character or object." );
  }

  return 0;
}


const void *code_is_altered( const void **argument )
{
  thing_data *th = (thing_data*) argument[0];

  char_data *ch = dynamic_cast<char_data *>( th );
  obj_data *obj = dynamic_cast<obj_data *>( th );

  if( ch ) {
    if( !ch->species ) {
      code_bug( "Is_Altered: target is a player." );
      return 0;
    }
    
    return (void*) ( ch->descr != ch->species->descr );
  }

  if( obj ) {
    return (void*) ( obj->singular != obj->pIndexData->singular
		     || obj->plural != obj->pIndexData->plural
		     || obj->before != obj->pIndexData->before
		     || obj->after != obj->pIndexData->after );
  }

  code_bug( "Is_Altered: argument is not a valid character or object." );
  return 0;
}


const void *code_shield( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];

  if( !ch ) {
    code_bug( "Shield: NULL char." );
    return 0;
  }

  obj_data *wield, *secondary, *shield;
  get_wield( ch, wield, secondary, shield );

  return (void*) shield;
}


const void *code_wield( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];

  if( !ch ) {
    code_bug( "Wield: NULL char." );
    return 0;
  }

  obj_data *wield, *secondary, *shield;
  get_wield( ch, wield, secondary, shield );

  return (void*) wield;
}


const void *code_offhand( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];

  if( !ch ) {
    code_bug( "Offhand: NULL char." );
    return 0;
  }

  obj_data *wield, *secondary, *shield;
  get_wield( ch, wield, secondary, shield );

  return (void*) secondary;
}


const void *code_has_wear_part( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];
  const int part = (int) argument[1];

  if( !ch ) {
    code_bug( "Has_Wear_Part: NULL char." );
    return 0;
  }

  if( part < 0 || part >= MAX_WEAR ) {
    code_bug( "Has_Wear_Part: part out of range." );
    return 0;
  }

  if( ch->is_humanoid( ) )
    return (void*) ( part < MAX_WEAR_HUMANOID );

  return (void*) is_set( ch->species->wear_part, part );
}

const void *code_is_family( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];
  const int family = (int) argument[1];

  if( !ch ) {
    code_bug( "Is_Family: NULL char." );
    return 0;
  }

  if( family < 0 || family > table_max[ TABLE_RACE ] ) {
    code_bug( "Is_Family: invalid family." );
    return 0;
  }

  int race = ch->shdata->race;
  int check = race;

  while( true ) {
    if( check == family )
      return (void*) true;
    
    if( race_table[ check ].family == check )
      return (void* ) false;

    check = race_table[ check ].family;

    if( check == race ) {
      code_bug( "Is_Family: race table loop detected." );
      return 0;
    }
  }
}


const void *code_is_group( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];
  const int group = (int) argument[1];

  if( !ch ) {
    code_bug( "Is_Group: NULL char." );
    return 0;
  }

  if( !ch->species ) {
    //    code_bug( "Is_Group: char is not a mob." );
    return 0;
  }

  if( group < 0 || group > table_max[ TABLE_GROUP ] ) {
    code_bug( "Is_Group: invalid group." );
    return 0;
  }

  return (void*)( ch->species->group == group );
}


const void *code_is_race( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];
  const int race = (int) argument[1];

  if( !ch ) {
    code_bug( "Is_Race: NULL char." );
    return 0;
  }

  if( race < 0 || race > table_max[ TABLE_RACE ] ) {
    code_bug( "Is_Race: invalid race." );
    return 0;
  }

  return (void*)( ch->shdata->race == race );
}


const void *code_chair( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];

  if( !ch ) {
    code_bug( "Chair: NULL char." );
    return 0;
  }

  if( ch->pos_obj )
    ch->pos_obj->Show( 1 );

  return (void*) ch->pos_obj;
}


const void *code_mount( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];

  if( !ch ) {
    code_bug( "Mount: NULL char." );
    return 0;
  }

  if( ch->mount )
    ch->mount->Show( 1 );

  return (void*) ch->mount;
}


const void *code_rider( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];

  if( !ch ) {
    code_bug( "Rider: NULL char." );
    return 0;
  }

  if( ch->rider )
    ch->rider->Show( 1 );

  return (void*) ch->rider;
}


const void *code_leader( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];

  if( !ch ) {
    code_bug( "Leader: NULL char." );
    return 0;
  }

  if( ch->leader )
    ch->leader->Show( 1 );

  return (void*) ch->leader;
}


const void *code_string( const void **argument )
{
  const char *string = (const char*) argument[0];
  char_data *ch = (char_data*)(thing_data*) argument[1];
  obj_data *obj = (obj_data*)(thing_data*) argument[2];
  thing_data *thing  = (thing_data*) argument[3];
  const char *string1 = (const char*) argument[4];
  const char *string2 = (const char*) argument[5];

  act_print( lib_buf, string, ch, character( thing ), obj, object( thing ), string1, string2, 0, ch, false );

  const char *tmp = alloc_string( lib_buf, MEM_QUEUE );
  extract_strings += tmp;
  return (void*) tmp;
}


const void *code_set_delay( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];
  const int delay = (int) argument[1];

  if( !ch ) {
    code_bug( "Set_Delay: NULL char." );
    return 0;
  }

  if( delay < 0 ) {
    code_bug( "Set_Delay: Negative delay." );
    return 0;
  }

  stop_events( ch, execute_leap );

  set_delay( ch, delay );

  return 0;
}


const void *code_stop_camouflage( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];

  if( !ch ) {
    code_bug( "Stop_Camouflage: NULL char." );
    return 0;
  }

  if( !is_set( ch->status, STAT_CAMOUFLAGED )
      && !is_set( ch->status, STAT_HIDING ) )
    return 0;

  remove_bit( ch->status, STAT_CAMOUFLAGED );
  remove_bit( ch->status, STAT_HIDING );
  ch->seen_by.clear( );
  update_aggression( ch );

  return (void*)true;
}


const void *code_stop_hide( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];

  if( !ch ) {
    code_bug( "Stop_Hide: NULL char." );
    return 0;
  }

  if( !is_set( ch->status, STAT_HIDING ) )
    return 0;

  remove_bit( ch->status, STAT_HIDING );
  ch->seen_by.clear( );
  update_aggression( ch );

  return (void*)true;
}


const void *code_default( const void **argument )
{
  const char *string = (const char*)argument[0];

  if( !var_def ) {
    code_bug( "Default: No default messages." );
    return 0;
  }

  for( const default_data *def = var_def; *(def->name); ++def ) {
    if( def->type == var_def_type
	&& !strcasecmp( string, def->name ) ) {
      return (void*) def->msg;
    }
  }

  if( var_def_type >= 0 ) {
    for( const default_data *def = var_def; *(def->name); ++def ) {
      if( def->type < 0
	  && !strcasecmp( string, def->name ) ) {
	return (void*) def->msg;
      }
    }
  }

  code_bug( "Default: default message not found." );
  return 0;
}


const void *code_vnum( const void **argument )
{
  thing_data *thing = (thing_data*) argument[0];

  if( !thing ) {
    code_bug( "Vnum: null argument." );
    return 0;
  }

  if( room_data *room = Room( thing ) ) {
    return (void*) room->vnum;

  } else if ( obj_data *obj = object( thing ) ) {
    return (void*) obj->pIndexData->vnum;

  } else if ( char_data *ch = character( thing ) ) {
    if( ch->species ) {
      return (void*) ch->species->vnum;
    } else {
      return (void*) -1;
    }
  }

  code_bug( "Vnum: bad argument." );
  return 0;
}


const void *code_set_mount( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];
  char_data *victim = (char_data*)(thing_data*) argument[1];

  if( !ch ) {
    code_bug( "Set_Mount: null character." );
    return 0;
  }


  if( !victim ) {
    dismount( ch );
    return 0;
  }

  if( victim == ch ) {
    code_bug( "Set_Mount: ch == mount." );
    return 0;
  }

  if( player( victim ) ) {
    code_bug( "Set_Mount: player mount." );
    return 0;
  }

  if( victim->rider ) {
    code_bug( "Set_Mount: mount has a rider." );
    return 0;
  }
  
  if( victim->fighting ) {
    code_bug( "Set_Mount: mount is fighting." );
    return 0;
  }

  if( victim->position != POS_STANDING ) {
    code_bug( "Set_Mount: mount not standing." );
    return 0;
  }

  dismount( ch );

  //  strip_affect( ch, AFF_INVISIBLE );
  leave_shadows( ch );
  remove_bit( ch->status, STAT_SNEAKING );
  remove_bit( ch->status, STAT_COVER_TRACKS );

  //  strip_affect( victim, AFF_INVISIBLE );
  leave_shadows( victim );
  remove_bit( victim->status, STAT_SNEAKING );
  remove_bit( victim->status, STAT_COVER_TRACKS );

  if( !ch->species ) {
    remove_bit( ch->pcdata->pfile->flags, PLR_TRACK );
    remove_bit( ch->pcdata->pfile->flags, PLR_SEARCHING );
  }

  disrupt_spell( ch );

  ch->mount = victim;
  victim->rider = ch;

  return 0;
}


const void *code_dispel_affect( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];
  const int aff = (int) argument[1];
  
  if( !ch ) {
    code_bug( "Dispel_Affect: NULL char." );
    return 0;
  }

  if( aff < 0
      || aff >= MAX_AFF_CHAR ) {
    code_bug( "Dispel_Affect: affect value out of range." );
    return 0;
  }

  strip_affect( ch, aff );

  return 0;
}


const void *code_light( const void **argument )
{
  thing_data *thing = (thing_data*) argument[0];

  if( !thing ) {
    code_bug( "Light: Null argument." );
    return 0;
  }

  return (void*) thing->Light( );
}


const void *code_get_desc( const void **argument )
{
  thing_data *thing = (thing_data*) argument[0];
  const char *name = (const char *) argument[1];

  extra_array *extras = 0;

  if( !thing ) {
    extras = &curr_prog->Extra_Descr( );

  } else {
    
    obj_data *obj = object( thing );
    char_data *ch = character( thing );
    room_data *room = Room( thing );
    
    if( !obj && !ch && !room ) {
      code_bug( "Get_Desc: Bad argument." );
      return 0;
    }
    
    if( !name || !*name ) {
      if( obj ) {
	code_bug( "Get_Desc: objects have no default descriptions." );
	return 0;
      } else if( ch ) {
	return (void*) ch->descr->complete;
      } else {
	return (void*) room->Description( );
      }
    }
    
    if( obj ) {
      extras = &obj->pIndexData->extra_descr;
    } else if( ch ) {
      if( !ch->species ) {
	code_bug( "Get_Desc: character is a player." );
	return 0;
      }
      extras = &ch->species->extra_descr;
    } else {
      extras = &room->Extra_Descr( );
    }
  }

  for( int i = 0; i < *extras; ++i ) {
    if( is_name( name, extras->list[i]->keyword ) ) {
      return (void*) extras->list[i]->text;
    }
  }

  code_bug( "Get_Desc: no such extra \"%s\".", name );
  return 0;
}


const void *code_clan_name( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];

  if( !ch ) {
    code_bug( "Clan_Name: Null character." );
    return 0;
  }

  if( ch->species ) {
    code_bug( "Clan_Name: Not a player." );
    return 0;
  }

  if( !ch->pcdata->pfile ) {
    code_bug( "Clan_Name: No PFile." );
    return 0;
  }

  clan_data *clan = ch->pcdata->pfile->clan;

  if( !clan )
    return 0;

  return clan->name;
}


const void *code_clan_abbrev( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];

  if( !ch ) {
    code_bug( "Clan_Abbrev: Null character." );
    return 0;
  }

  if( ch->species ) {
    code_bug( "Clan_Abbrev: Not a player." );
    return 0;
  }

  if( !ch->pcdata->pfile ) {
    code_bug( "Clan_Abbrev: No PFile." );
    return 0;
  }

  clan_data *clan = ch->pcdata->pfile->clan;

  if( !clan )
    return 0;

  return clan->abbrev;
}


const void *code_first_word( const void **argument )
{
  const char *string = (const char*) argument[0];

  if( !string || !*string )
    return empty_string;

  skip_spaces( string );

  unsigned n = 0;
  for( const char *s = string; *s && !isspace( *s ); ++s ) {
    ++n;
  }

  n = min( n, MAX_STRING_LENGTH-1 );

  strncpy( lib_buf, string, n );
  lib_buf[n] = '\0';

  const char *word = alloc_string( lib_buf, MEM_QUEUE );
  extract_strings += word;
  return word;
}


const void *code_skip_word( const void **argument )
{
  const char *string = (const char*) argument[0];

  if( !string || !*string )
    return empty_string;

  skip_spaces( string );

  while( *string && !isspace( *string ) ) {
    ++string;
  }

  skip_spaces( string );

  return string;
}


const void *code_word( const void **argument )
{
  const char *string = (const char*) argument[0];
  const int num = (int) argument[1];

  if( !string || !*string || num < 0 )
    return empty_string;

  for( int i = 0; i < num; ++i ) {
    skip_spaces( string );
    
    while( *string && !isspace( *string ) ) {
      ++string;
    }

    if( !*string )
      return empty_string;
  }

  skip_spaces( string );

  unsigned n = 0;
  for( const char *s = string; *s && !isspace( *s ); ++s ) {
    ++n;
  }

  n = min( n, MAX_STRING_LENGTH-1 );

  strncpy( lib_buf, string, n );
  lib_buf[n] = '\0';

  const char *word = alloc_string( lib_buf, MEM_QUEUE );
  extract_strings += word;
  return word;
}


const void *code_character( const void **argument )
{
  thing_data *thing = (thing_data*) argument[0];

  return (void*) character( thing );
}


const void *code_mob( const void **argument )
{
  thing_data *thing = (thing_data*) argument[0];

  return (void*) mob( thing );
}


const void *code_player( const void **argument )
{
  thing_data *thing = (thing_data*) argument[0];

  return (void*) player( thing );
}


const void *code_room( const void **argument )
{
  thing_data *thing = (thing_data*) argument[0];

  return (void*) Room( thing );
}


const void *code_object( const void **argument )
{
  thing_data *thing = (thing_data*) argument[0];

  return (void*) object( thing );
}


const void *code_set_mob_timer( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];
  const int n = (int) argument[1];

  if( !ch ) {
    code_bug( "Set_mob_Timer: Null character." );
    return 0;
  }

  if( !mob( ch ) ) {
    code_bug( "Set_mob_Timer: Not a mob." );
    return 0;
  }

  if( n < 0 ) {
    code_bug( "Set_mob_Timer: Negative value." );
    return 0;
  }

  stop_events( ch, execute_mob_timer );

  if( n > 0 ) {
    add_queue( new event_data( execute_mob_timer, ch ), n*PULSE_MOBILE );
  }

  return 0;
}


const void *code_cardinal( const void **argument )
{
  const int n = (int) argument[0];
  char_data *ch = (char_data*)(thing_data*) argument[1];

  return number_word( n, ch );
}


const void *code_ordinal( const void **argument )
{
  const int n = (int) argument[0];
  char_data *ch = (char_data*)(thing_data*) argument[1];

  return ordinal_word( n, ch );
}


const void *code_set_sentinel( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];
  const int value = (int) argument[1];

  if( !ch ) {
    code_bug( "Set_Sentinel: character is null." );
    return 0;
  }

  if( !ch->species || ch->pcdata ) {
    code_bug( "Set_Sentinel: character is not a mob." );
    return 0;
  }

  if( is_set( ch->status, STAT_PET ) ) {
    code_bug( "Set_Sentinel: character is a pet." );
    return 0;
  }

  const bool old = is_set( ch->status, STAT_SENTINEL );

  if( value > 0 ) {
    assign_bit( ch->status, STAT_SENTINEL, value != 0 );
  }

  return (void*) old;
}


const void *code_mob_act( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];
  const int flag = (int) argument[1];

  if( !ch ) {
    code_bug( "Mob_Act: character is null." );
    return 0;
  }

  if( !ch->species ) {
    code_bug( "Mob_Act: character is not a mob." );
    return 0;
  }

  if( flag < 0 || flag >= MAX_ACT ) {
    code_bug( "Mob_Act: invalid flag." );
    return 0;
  }

  return (void*) is_set( ch->species->act_flags, flag );
}


static int min_move( char_data *ch )
{
  int move = ch->move;

  if( move == 0 )
    return 0;

  for( int i = 0; i < ch->followers; ++i ) {
    char_data *rch = ch->followers[i];
    if( rch->in_room == ch->in_room ) {
      move = min( move, min_move( rch ) );
    }
  }

  return move;
}


const void *code_min_move( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];

  if( !ch ) {
    code_bug( "Min_Move: character is null." );
    return 0;
  }
 
  return (void*) min_move( ch );
}


const void *code_is_mob( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];

  if( !ch ) {
    code_bug( "Is_Mob: character is null." );
    return 0;
  }

  if( !ch->species )
    return 0;

  if( is_set( ch->status, STAT_PET ) && !ch->leader->species )
    return 0;

  return (void*) true;
}


const void *code_can_move( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];

  if( !ch ) {
    code_bug( "Can_Move: character is null." );
    return 0;
  }

  return (void*) !( ch->position <= POS_SLEEPING
		    || is_entangled( ch )
		    || is_set( ch->status, STAT_STUNNED ) );
}


const void *code_can_speak( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];

  if( !ch ) {
    code_bug( "Can_Speak: character is null." );
    return 0;
  }
  
  return (void*) !( ch->position <= POS_SLEEPING
		    || is_silenced( ch ) );
}


const void *code_is_drowning( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];
  const char *string = (const char*) argument[1];

  if( !ch ) {
    code_bug( "Is_Drowning: character is null." );
    return 0;
  }
  
  return (void*) is_drowning( ch, string );
}


const void *code_water_logged( const void **argument )
{
  room_data *room = (room_data*)(thing_data*) argument[0];

  if( !room ) {
    code_bug( "Water_Logged: room is null." );
    return 0;
  }
  
  return (void*) water_logged( room );
}


const void *code_deep_water( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];
  room_data *room = (room_data*)(thing_data*) argument[1];

  if( !room && !ch ) {
    code_bug( "Deep_Water: room and character both null." );
    return 0;
  }
  
  return (void*) deep_water( ch, room );
}


const void *code_is_submerged( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];
  room_data *room = (room_data*)(thing_data*) argument[1];

  if( !room && !ch ) {
    code_bug( "Is_Submerged: room and character both null." );
    return 0;
  }
  
  return (void*) is_submerged( ch, room );
}


const void *code_show_map( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];
  room_data *room = (room_data*)(thing_data*) argument[1];
  int length = (int) argument[2];
  int width = (int) argument[3];

  if( !ch ) {
    code_bug( "Show_Map: character is null." );
    return 0;
  }

  if( !room )
    room = ch->in_room;

  if( length == 0 )
    length = 23;

  if( width == 0 )
    width = 59;

  // Dimensions must be one less than a multiple of 4.
  length = ( length/4 ) * 4 + 3;
  width = ( width/4 ) * 4 + 3;

  if( length < 1 || length > 59
      || width < 1 || width > 143 ) {
    code_bug( "Show_Map: bad map size." );
    return 0;
  }

  show_map( ch, room, length, width );

  return 0;
}


const void *code_has_reset( const void **argument )
{
  thing_data *thing = (thing_data*) argument[0];

  if( !thing ) {
    code_bug( "Has_Reset: thing is null." );
    return 0;
  }

  if( obj_data *obj = object( thing ) ) {
    return obj->reset;
  } else if( mob_data *npc = mob( thing ) ) {
    return npc->reset;
  }

  code_bug( "Has_Reset: thing is not an object or mob." );
  return 0;
}


const void *code_dir_name( const void **argument )
{
  const int dir = (int) argument[0];

  if( dir < 0 || dir > MAX_DIR_COMPASS ) {
    code_bug( "Dir_Name: bad direction." );
    return 0;
  }

  return dir_table[dir].name;
}


const void *code_move_name( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];

  if( !ch ) {
    code_bug( "Move_Name: character is null." );
    return 0;
  }

  if( ch->mount ) {
    return "ride";
  }

  int pos = ch->mod_position( );

  if( pos == POS_FLYING ) {
    return "fly";
  }

  if( pos == POS_FALLING ) {
    return "fall";
  }

  if( pos == POS_HOVERING ) {
    return "glide";
  }

  if( pos == POS_WADING ) {
    return "wade";
  }

  if( pos == POS_SWIMMING || pos == POS_DROWNING ) {
    return "swim";
  }

  return move_verb( ch, &Movement_Data::name );
}


const void *code_move_leaves( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];

  if( !ch ) {
    code_bug( "Move_Leaves: character is null." );
    return 0;
  }

  if( ch->mount ) {
    return "rides";
  }

  const int pos = ch->mod_position( );

  if( pos == POS_FLYING ) {
    return "flies";
  }

  if( pos == POS_FALLING ) {
    return "falls";
  }

  if( pos == POS_HOVERING ) {
    return "glides";
  }

  if( pos == POS_WADING ) {
    return "wades";
  }

  if( pos == POS_SWIMMING || pos == POS_DROWNING ) {
    return "swims";
  }
  return move_verb( ch, &Movement_Data::leave );
}


const void *code_move_arrives( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];

  if( !ch ) {
    code_bug( "Move_Arrives: character is null." );
    return 0;
  }

  if( ch->mount ) {
    return "rides in";
  }

  const int pos = ch->mod_position( );

  if( pos == POS_FLYING ) {
    return "flies in";
  }

  if( pos == POS_FALLING ) {
    return "falls in";
  }

  if( pos == POS_HOVERING ) {
    return "glides in";
  }

  if( pos == POS_WADING ) {
    return "wades in";
  }

  if( pos == POS_SWIMMING || pos == POS_DROWNING ) {
    return "swims in";
  }
  return move_verb( ch, &Movement_Data::arrive );
}


const void *code_find_player( const void **argument )
{
  const char *string = (const char*) argument[0];

  if( !string || !*string ) {
    code_bug( "Find_Player: empty or null name." );
    return 0;
  }

  for( int i = 0; i < player_list; ++i ) {
    player_data *pl = player_list[i];
    if( pl->In_Game( )
	&& invis_level( pl ) < LEVEL_BUILDER
	&& !strcasecmp( string, pl->descr->name ) ) {
      return pl;
    }
  }

  return 0;
}


const void *code_beep( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];

  if( !ch ) {
    code_bug( "Beep: character is null." );
    return 0;
  }

  send( ch, "\7" );

  return 0;
}


const void *code_spell_affect( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];
  char_data *victim = (char_data*)(thing_data*) argument[1];
  const int spell = (int) argument[2];
  const int aff = (int) argument[3];
  const int level = (int) argument[4];
  const int time = (int) argument[5];

  if( spell < SPELL_FIRST
      || spell >= SPELL_FIRST+table_max[ TABLE_SKILL_SPELL ] ) {
    code_bug( "Spell_Affect: Invalid spell." );
    return 0;
  }
  
  if( level < 0 || level > MAX_SPELL_LEVEL ) {
    code_bug( "Spell_Affect: Invalid level." );
    return 0;
  }

  return (void*) spell_affect( ch, victim, level, time, spell, aff );
}


const void *code_burn_web( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];
  const char *string = (const char*) argument[1];

  burn_web( ch, string );

  return 0;
}


const void *code_heal_victim( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];
  char_data *victim = (char_data*)(thing_data*) argument[1];
  const int hp = (int) argument[2];
  const int dur = (int) argument[3];

  if( !victim ) {
    code_bug( "Heal_Victim: null victim." );
    return 0;
  }

  if( hp < 0 ) {
    code_bug( "Heal_Victim: Negative hp." );
    return 0;
  }

  return (void*) heal_victim( ch, victim, hp, dur );
}


const void *code_water_shock( const void **argument )
{
  char_data *ch = (char_data*)(thing_data*) argument[0];
  char_data *victim = (char_data*)(thing_data*) argument[1];
  const int damage = (int) argument[2];

  if( damage < 0 ) {
    code_bug( "Water_Shock: Negative damage." );
    return 0;
  }

  water_shock( ch, victim, damage );

  return 0;
}


const void *code_trigger_oprog( const void **argument )
{
  const int vnum = (int) argument[0];
  const int trigger = (int) argument[1];
  const int min = (int) argument[2];

  const obj_clss_data *clss = get_obj_index( vnum );

  if( !clss ) {
    code_bug( "Trigger: Bad obj vnum %d.", vnum );
    return 0;
  }

  if( trigger < 0 || trigger >= MAX_OPROG_TRIGGER ) {
    code_bug( "Trigger: Bad trigger." );
    return 0;
  }

  int i = 1;
  for( oprog_data *oprog = clss->oprog; oprog; oprog = oprog->next ) {
    if( i > min && oprog->trigger == trigger )
      return (void*) i;
    ++i;
  }

  return 0;
}


const void *code_trigger_mprog( const void **argument )
{
  const int vnum = (int) argument[0];
  const int trigger = (int) argument[1];
  const int min = (int) argument[2];

  const species_data *species = get_species( vnum );

  if( !species ) {
    code_bug( "Trigger: Bad species vnum %d.", vnum );
    return 0;
  }

  if( trigger < 0 || trigger >= MAX_OPROG_TRIGGER ) {
    code_bug( "Trigger: Bad trigger." );
    return 0;
  }

  int i = 1;
  for( mprog_data *mprog = species->mprog; mprog; mprog = mprog->next ) {
    if( i > min && mprog->trigger == trigger )
      return (void*) i;
    ++i;
  }

  return 0;
}


const void *code_trigger_acode( const void **argument )
{
  const int vnum = (int) argument[0];
  const int trigger = (int) argument[1];
  const int min = (int) argument[2];

  const room_data *room = get_room_index( vnum );

  if( !room ) {
    code_bug( "Trigger: Bad room vnum %d.", vnum );
    return 0;
  }

  if( trigger < 0 || trigger >= MAX_OPROG_TRIGGER ) {
    code_bug( "Trigger: Bad trigger." );
    return 0;
  }

  int i = 1;
  for( action_data *action = room->action; action; action = action->next ) {
    if( i > min && action->trigger == trigger )
      return (void*) i;
    ++i;
  }

  return 0;
}

const void *code_mail( const void **argument )
{
  const char *from = (const char*) argument[0];
  const char *title = (const char*) argument[1];
  const char *message = (const char*) argument[2];
  const char *to = (const char*) argument[3];

  if( !to || !*to ) {
    code_bug( "Mail: empty or null recipient name." );
    return 0;
  }

  pfile_data *pfile = find_pfile_exact( to );

  if( !pfile ) {
    code_bug( "Mail: can't find recipient \"%s\".", to );
    return 0;
  }

  if( !message )
    message = empty_string;

  if( !from )
    from = empty_string;

  if( !title )
    title = empty_string;

  const size_t len = strlen( message );
  const bool has_lf = isspace( *( message + len - 1 ) );
  if( !has_lf ) {
    strcpy( lib_buf, message );
    strcat( lib_buf, "\n\r" );
    message = lib_buf;
  }

  note_data *note = new note_data;
  note->title     = alloc_string( title,    MEM_NOTE );
  note->message   = alloc_string( message,  MEM_NOTE );
  note->from      = alloc_string( from,     MEM_NOTE );
  note->noteboard = NOTE_PRIVATE;
  note->date = current_time;

  if( player_data *pl = find_player( pfile ) ) {
    append( pl->pcdata->mail, note );
    save_mail( pfile, pl->pcdata->mail );
    if( pl->link
	&& pl->link->connected == CON_PLAYING ) {
      if( *from ) {
	fsend( pl, "A mail daemon runs up and hands you a letter from %s.", from );
      } else {
	fsend( pl, "A mail daemon runs up and hands you a letter." );
      }
    }

  } else {
    note_data *mail = read_mail( pfile );
    append( mail, note );
    save_mail( pfile, mail );
    delete_list( mail );
  }

  return (void*) true;
}

const void *code_skill( const void **argument )
{
  const int skill = (int) argument[0];
  return (void*) skill;
}
