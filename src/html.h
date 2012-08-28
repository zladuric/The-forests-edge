#ifndef tfe_html_h
#define tfe_html_h

#include "string2.h"


text html( const char* );
void html_start( FILE *fp, const char*, const char*, const char* = "" );
void html_stop( FILE * );


#endif // tfe_html_h
