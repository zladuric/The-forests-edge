/*
 *   GAME PARAMETERS
 */

#ifndef __DEFINE_H__
#define __DEFINE_H__

//#define MAX_INTEGER                      64000
//#define M_PI            3.14159265358979323846
//#define M_E              2.7182818284590452354

#define DEFAULT_PORT		2000

#define MAX_STRING_LENGTH       4096
#define MAX_INPUT_LENGTH         320
#define ONE_LINE                  80
#define TWO_LINES                160
#define THREE_LINES              240
#define FOUR_LINES               320
#define FIVE_LINES               400
#define SIX_LINES                480
#define EIGHT_LINES              640
  
#define MAX_PFILE               5000

#define NOWHERE                   -1
#define UNLEARNT                   0

#define START_YEAR               100
#define BANK_WEIGHT            50000
#define BANK_BONUS             25000
#define AUCTION_TIME              30
#define MAX_ROOM               99999
#define MAX_SPELL_LEVEL           30

#define DAMAGE_LIMIT            1500

#define MAX_CMD_QUEUE            200
#define MIN_DELAY                  5


/*
 *  ROOM & MOB NUMBERS OF INTEREST
 */

#define MOB_BLANK                 133
#define MOB_CLAY_GOLEM            842


/*
 *   DEFINITIONS
 */


#define MAX_PLYR_RACE              13
#define MAX_SPELL_WAIT              5
#define MAX_ARMOR                   5
#define MAX_PALETTE                 6
#define MAX_CFLAG                  32
#define MAX_INGRED                  5
#define MAX_SPECIES             50000
//#define MAX_TRAIN                  16
#define MAX_OBJ_INDEX           50000 
//#define MAX_FAKE_MOB               29
#define MAX_AUCTION_BID       1000000
#define MAX_ATTACK                  5

#define ATT_SPELL                   1
#define ATT_YELL_HELP               2   


#define COPPER                      0
#define SILVER                      1 
#define GOLD                        2
#define PLATINUM                    3
#define MAX_COIN                    4


#define COND_ALCOHOL                0
#define COND_FULL                   1
#define COND_THIRST                 2
#define COND_DRUNK                  3
#define MAX_COND                    4


#define CONT_CLOSEABLE              0
#define CONT_PICKPROOF              1
#define CONT_CLOSED                 2
#define CONT_LOCKED                 3
#define CONT_HOLDING                4
#define MAX_CONT                    5

#define CONSUME_POISON              0
#define CONSUME_PLAGUE              1
#define MAX_CONSUME                 2

#define GEM_RANDOM                  -1
#define GEM_NONE                    0
#define GEM_UNCUT                   1
#define GEM_FRACTURED               2
#define GEM_CHIPPED                 3
#define GEM_BLEMISHED               4
#define GEM_SCRATCHED               5
#define GEM_FLAWLESS                6
#define MAX_GEM                     7

#define FORMAT_NORMAL               0
#define FORMAT_BOLD                 1
#define FORMAT_REVERSE              2
#define FORMAT_UNDERLINE            3
#define FORMAT_RED                  4
#define FORMAT_GREEN                5
#define FORMAT_YELLOW               6
#define FORMAT_BLUE                 7
#define FORMAT_MAGENTA              8
#define FORMAT_CYAN                 9
#define FORMAT_WHITE               10
#define FORMAT_B_RED               11
#define FORMAT_B_GREEN             12
#define FORMAT_B_YELLOW            13
#define FORMAT_B_BLUE              14
#define FORMAT_B_MAGENTA           15
#define FORMAT_B_CYAN              16
#define FORMAT_B_WHITE             17
#define FORMAT_BL_RED              18
#define FORMAT_BL_GREEN            19 
#define FORMAT_BL_YELLOW           20
#define FORMAT_BL_BLUE             21
#define FORMAT_BL_MAGENTA          22
#define FORMAT_BL_CYAN             23
#define FORMAT_BL_WHITE            24
#define FORMAT_BB_RED                 25
#define FORMAT_BB_GREEN               26
#define FORMAT_BB_YELLOW              27
#define FORMAT_BB_BLUE                28
#define FORMAT_BB_MAGENTA             29
#define FORMAT_BB_CYAN                30
#define FORMAT_BB_WHITE               31
#define FORMAT_H_RED                  32
#define FORMAT_H_GREEN                33
#define FORMAT_H_YELLOW               34
#define FORMAT_H_BLUE                 35
#define FORMAT_H_MAGENTA              36
#define FORMAT_H_CYAN                 37
#define FORMAT_H_WHITE                38
#define MAX_FORMAT                    39


#define RACE_HUMAN                  0
#define RACE_ELF                    1
#define RACE_GNOME                  2
#define RACE_DWARF                  3    
#define RACE_HALFLING               4
#define RACE_ENT                    5
#define RACE_CENTAUR                6
#define RACE_LIZARD                 7
#define RACE_OGRE                   8
#define RACE_TROLL                  9
#define RACE_ORC                   10
#define RACE_GOBLIN                11
#define RACE_VYAN                  12
#define RACE_UNDEAD                28
#define RACE_UNKNOWN               30
#define RACE_PLANT                 31
#define RACE_GOLEM                 33
#define RACE_ELEMENTAL             63

#define REP_ATTACKED                0
#define REP_KILLED                  1
#define REP_STOLE_FROM              2
#define REP_HEIST                   3

#define TRAP_POISON                 0
#define TRAP_BLIND                  1
#define MAX_TRAP                    2

/*
#define QUEST_SACRIFICE             0
#define QUEST_KILL                  1
#define QUEST_GIVE                  2
#define QUEST_PRAY                  3
#define MAX_QUEST_TYPE              4
*/


#define RES_DEXTERITY              -1
#define RES_MAGIC                   0
#define RES_FIRE                    1
#define RES_COLD                    2
#define RES_SHOCK                   3
#define RES_MIND                    4
#define RES_ACID                    5
#define RES_POISON                  6
#define MAX_RESIST                  7


#define SEX_NEUTRAL                 0
#define SEX_MALE                    1
#define SEX_FEMALE                  2
#define SEX_HERMAPHRODITE           3
#define SEX_RANDOM                  4
#define MAX_SEX                     5


#define HAND_NONE                   0
#define HAND_LEFT                   1
#define HAND_RIGHT                  2
#define HAND_BOTH                   3
#define HAND_RANDOM                 4
#define MAX_HAND                    5
 

#define SIZE_ANT                    0
#define SIZE_RAT                    1
#define SIZE_DOG                    2
#define SIZE_GNOME                  3
#define SIZE_HUMAN                  4
#define SIZE_OGRE                   5
#define SIZE_HORSE                  6
#define SIZE_GIANT                  7
#define SIZE_ELEPHANT               8
#define SIZE_DINOSAUR               9
#define MAX_SIZE                   10


/*
#define SKY_CLOUDLESS               0
#define SKY_CLOUDY                  1
#define SKY_RAINING                 2
#define SKY_LIGHTNING               3
*/


#define STAT_PET                    0
#define STAT_IN_GROUP               1
#define STAT_FAMILIAR               2
#define STAT_TWO_HAND               3
#define STAT_FOLLOWER               4
#define STAT_NO_SNOOP               5 
#define STAT_FLEE_FROM              6
#define STAT_GARROTING              7
#define STAT_TAMED                  8
#define STAT_BERSERK                9
#define STAT_SNUCK                 10
#define STAT_SENTINEL              11
#define STAT_REPLY_LOCK            12
#define STAT_AGGR_ALL              13
#define STAT_AGGR_GOOD             14
#define STAT_AGGR_EVIL             15
#define STAT_ORDERED               16
#define STAT_HIDING                17
#define STAT_SNEAKING              18
#define STAT_WIMPY                 19
#define STAT_RESPOND               20
#define STAT_FORCED                21
#define STAT_CAMOUFLAGED           22
#define STAT_AGGR_LAWFUL           23
#define STAT_AGGR_CHAOTIC          24
#define STAT_STUNNED               25
#define STAT_LEADER                26
#define STAT_NOFOLLOW              27
#define STAT_WAITING               28
#define STAT_STOOD                 29
#define STAT_HOLD_POS              30
#define STAT_GROUP_LOOTER          31
#define STAT_COVER_TRACKS          32
#define STAT_FOCUS                 33
#define STAT_MAX                   34
#define STATUS_INTS                 2


#define COOK_RAW                    0
#define COOK_COOKED                 1
#define COOK_BURNT                  2
#define MAX_COOK                    3


/*
 *   THING TYPES
 */


#define THING_DATA     0
#define EXIT_DATA      1
#define EXTRA_DATA     2
#define ROOM_DATA      3
#define OBJ_DATA       4
#define CHAR_DATA      5
#define MOB_DATA       6
#define PLAYER_DATA    7
#define WIZARD_DATA    8
#define AUCTION_DATA   9
#define SHOP_DATA     10


/*
 *   DEFINED FUNCTIONS
 */

#define IS_AWAKE( ch )        ( ch->position > POS_SLEEPING ) 

#define MAX_CACHE( ch )       ( 1 * ch->Level( ) )


#endif /* __DEFINE_H__ */
