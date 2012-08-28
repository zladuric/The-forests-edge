#ifndef tfe_number_h
#define tfe_number_h


#define FUNC_EPSILON 1.0e-10


/*
 *   STRUCTURES
 */


void   add_percent_average ( int&, const int );
int    roll_dice           ( int, int );
void   damage              ( dice_data&, char_data*, obj_data* );
void   sprintf_dice        ( char*, int );
const char *dice_string    ( int );


class Dice_Data
{
public:
  int       number;    
  int       side;
  int       plus;

  Dice_Data( int value = 0 )
  {
    number = value & 0x3F;
    side   = ( value >> 6 ) & 0xFFF;
    plus   = ( value >> 18 ) & 0x3fff;
  }

  Dice_Data& operator=( const int& value ) {
    number =  value & 0x3F;
    side   = ( value >> 6 ) & 0xFFF;
    plus   = ( value >> 18 ) & 0x3fff;
    return *this;
  }
  
  operator int( ) {
    return number + ( side << 6 ) + ( plus << 18 );
  }
  
  int average( ) const {
    return number*(side+1)/2+plus;
  }
  
  int twice_average( ) const {
    return number*(side+1)+2*plus;
  }
  
  int roll( ) const {
    return roll_dice( number, side )+plus;
  }
};


/*
 *   VARIOUS INLINE MATH MACROS
 */


template < class T >
T sqr( T a )
{ 
  return( a*a );
}   


template < class T >
T cube ( T a )
{
  return( a*a*a );
} 


inline int max  ( int a, int b )  { return( a > b ? a : b );  }
inline int min  ( int a, int b )  { return( a < b ? a : b );  }
inline int sign ( int a )         { return( a > 0 ? 1 : -1 ); }


inline int range( int a, int b, int c )       
{
  return( b < a ? a : ( b > c ? c : b ) );
}


inline bool not_in_range( int i, int a, int b )
{
  return( i < a || i > b );
}


/*
 *   FUNCTIONS
 */


const char*   number_word     ( int, const char_data* = 0 );
const char*   ordinal_word    ( int, const char_data* = 0 );
int           number_range    ( int, int );
int           evaluate        ( const char *s1, bool&, int = 0, int = 0 );
void          atorange        ( const char*, int&, int& );
const char*   atos            ( int );
bool          renumber        ( int&, int, int );

const char*   int3            ( int, bool = false );
const char*   int4            ( int, bool = false );
const char*   int5            ( int, bool = false );
const char*   float3          ( int, bool = false );

double phi ( double );
double normal_prob( double, double = 0.0, double = 1.0, double = -3.100, double = 3.100 );
int norm_search( int, int, int, unsigned, unsigned );

//int norm ( int, int, int, int );
//unsigned norm2000 ( unsigned );
//unsigned norm2000_search ( unsigned, unsigned, unsigned );


#endif // tfe_number_h
