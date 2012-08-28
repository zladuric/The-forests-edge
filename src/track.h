#ifndef tfe_track_h
#define tfe_track_h


class track_data
{
public:
  track_data*   next;
  int           race;
  int           to_dir;
  long          decay_time;

  track_data( );
  ~track_data( );
};


#endif // tfe_track_h
