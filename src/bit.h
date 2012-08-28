#ifndef tfe_bit_h
#define tfe_bit_h


const char *on_off       ( const int*, int );
const char *true_false   ( const int*, int );
const char *yes_no       ( const int*, int );


/*
 *   BIT VECTOR FUNCTIONS
 */


bool is_set( int vector, int bit );
void switch_bit( int& vector, int bit );
void set_bit( int& vector, int bit );
void remove_bit( int& vector, int bit );
void assign_bit( int& vector, int bit, bool value );
  

/*
 *   BIT ARRAY FUNCTIONS
 */


bool is_set( const int *array, int bit );
void switch_bit( int *array, int bit );
void set_bit( int *array, int bit );
void remove_bit( int *array, int bit );
void assign_bit( int *array, int bit, bool value );
  

/*
 *   LEVEL FUNCTIONS
 */


void set_level( int* array, int bit, int level );
int level_setting( const int* array, int bit );
void set_level( int& vector, int bit, int level );
int level_setting( int vector, int bit );


#endif // tfe_bit_h
