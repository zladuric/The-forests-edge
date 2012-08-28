#ifndef tfe_event_h
#define tfe_event_h


class event_data
{
public:
  event_func*    func;
  event_data*    loop;
  thing_data*   owner;
  int            time;
  void*       pointer; 

  event_data( event_func*, thing_data* );
  event_data( );

  ~event_data( );

  bool Is_Valid( ) const
  { return owner != 0; }
};


/* 
 *   ROUTINES
 */


#define QUEUE_LENGTH  150000


extern event_data*   event_queue  [ QUEUE_LENGTH ];
extern int            event_pntr;


void  extract        ( event_data* );
void  unlink         ( event_data* );
void  stop_events    ( thing_data*, event_func* = 0 );
event_data *find_event ( thing_data*, event_func* );

void         add_queue      ( event_data*, int );
int          modify_delay   ( char_data* ch, int delay );
void         set_delay      ( char_data*, int, bool = true );
void         set_min_delay  ( char_data*, int, bool = true );
const char*  name           ( event_data* );
int          time_till      ( event_data* );
void         event_update   ( );
void         delay_wander   ( event_data* );
void         set_update     ( char_data* );
void         set_regen      ( char_data* );
bool         behave         ( char_data*, exit_data* = 0, bool urgent = false );


event_func execute_wander;
event_func next_action;
event_func execute_leap;
event_func execute_drown;
event_func execute_fall;
event_func execute_path;
event_func update_affect;
event_func execute_decay;
event_func execute_junk;
//event_func execute_hunger;
//event_func execute_thirst;
//event_func execute_drunk;
event_func execute_update;
event_func execute_regen;

event_func execute_obj_fall;
event_func execute_obj_rise;
event_func execute_obj_float;
event_func execute_obj_sink;

event_func execute_mob_timer;


#endif // tfe_event_h
