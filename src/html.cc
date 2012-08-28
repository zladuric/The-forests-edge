#include <stdio.h>
#include <ctype.h>
#include "html.h"


extern  const char *color_key;


static const char *const html_colors[] = {
  0,
  "<span style=\"font-weight:bold;\">",
  "<span style=\"text-decoration:underline;\">",
  "<span style=\"color:black;\">",
  "<span style=\"color:red;\">",
  "<span style=\"font-weight:bold; color:red;\">",
  "<span style=\"color:green;\">",
  "<span style=\"font-weight:bold; color:green;\">",
  "<span style=\"color:yellow;\">",
  "<span style=\"font-weight:bold; color:yellow;\">",
  "<span style=\"color:blue;\">",
  "<span style=\"font-weight:bold; color:blue;\">",
  "<span style=\"color:magenta;\">",
  "<span style=\"font-weight:bold; color:magenta;\">",
  "<span style=\"color:cyan;\">",
  "<span style=\"font-weight:bold; color:cyan;\">",
  "<span style=\"color:#a9a9a9;\">",
  "<span style=\"font-weight:bold; color:white;\">",
  "<span style=\"text-decoration:underline;\">",
  "<span style=\"font-weight:bold;\">"
};


text html( const char *tfe )
{
  text html = "<pre>\n";

  unsigned spans = 0;

  while( char c = *tfe++ ) {
    if( c == '\n' ) {
      html += "\n";
      continue;
    }
    if( c == '\r' ) {
      continue;
    }
    if( c == '<' ) {
      html += "&lt;";
      continue;
    }
    if( c == '>' ) {
      html += "&gt;";
      continue;
    }
    if( c == '&' ) {
      html += "&amp;";
      continue;
    }
    if( c == '\"' ) {
      html += "&quot;";
      continue;
    }
    if( c == '@' ) {
     c = *tfe++;
     if( c == '@' || c == 0 ) {
       html += '@';
       continue;
     }
     if( c == 'I' ) {
       html += "  ";
       continue;
     }
     if( c == 'n' ) {
       while( spans != 0 ) {
	 html += "</span>";
	 --spans;
       }
       continue;
     }
     for( int i = 1; color_key[i]; ++i ) {
       if( color_key[i] == c ) {
	 while( spans != 0 ) {
	   html += "</span>";
	   --spans;
	 }
	 html += html_colors[i];
	 ++spans;
	 break;
       }
     }
     continue;
    }
    html += c;
  }

  html += "</pre>\n";

  return html;
}


static const char *const html_fg = "ivory";
static const char *const html_bg = "#305a7a";
static const char *const html_link = "#33ccff";
static const char *const html_vlink = "#66cccc";
static const char *const html_alink = "#33ccff";


void html_start( FILE *fp,
		 const char *title,
		 const char *header,
		 const char *dir )
{
  fprintf( fp, "<head>\n\
  <meta http-equiv=\"content-type\" content=\"text/html; charset=ISO-8859-1\">\n\
  <title>%s</title>\n\
</head>\n", title );

  fprintf( fp, "<body style=\"color: %s; background-color: %s;\"\n\
  link=\"%s\" vlink=\"%s\" alink=\"%s\">\n",
	   html_fg, html_bg,
	   html_link, html_vlink, html_alink );


  fprintf( fp, "<table style=\"width: 100%%; text-align: left;\" border=\"0\" cellspacing=\"0\"\
  cellpadding=\"0\">\
  <tbody>\
    <tr>\
      <td style=\"background-color: rgb(163, 191, 147);\"> <img\
 src=\"%sscary_forest.jpg\" title=\"\" alt=\"\"\
 style=\"width: 224px; height: 150px;\" width=\"224\" height=\"150\"><br>\
      </td>\
      <td\
 style=\"background-color: rgb(163, 191, 147); text-align: center; vertical-align: middle;\"><big\
 style=\"color: ivory;\"><big><big><big><span\
 style=\"font-style: italic;\">The Forest's Edge</span></big></big></big></big><br>\
      </td>\
      <td style=\"background-color: rgb(163, 191, 147);\"><img\
 src=\"%spath.jpg\" title=\"\" alt=\"\" style=\"width: 200px; height: 150px;\"\
 align=\"right\"> </td>\
    </tr>\
  </tbody>\
</table>\
<br><br>", dir, dir );

  fprintf( fp, "<table cellpadding=\"0\" cellspacing=\"0\" border=\"0\"\
 style=\"text-align: left; width: 100%%;\"><tbody>\n\
    <tr><td><br></td></tr>\n\
    <tr><td style=\"vertical-align: top; width: 150px;\">\n\
      <a href=\"%sindex.html\">Home</a><br><br>\n\
      <a href=\"telnet://tfe.genesismuds.com:2000\">Connect</a><br><br>\n\
      <a href=\"%splayers.html\">Player Directory</a><br><br>\n\
      <a href=\"%swho.cgi\">Who's Online</a><br><br>\n\
      <a href=\"%shelp/index.html\">Help Pages</a><br><br>\n\
      <a href=\"%sclans/index.html\">Player Clans</a><br><br>\n\
      <a href=\"%sresources.html\"> Resources</a><br><br>\n\
      <a href=\"%scredits.html\">Credits</a><br>\n\
      </td>\n",
	   dir, dir, dir, dir, dir, dir, dir );

  fprintf( fp, " <td style=\"vertical-align: top;\">\
      <div align=\"left\"><big><big>%s</big></big><br>\
      </div>\
      <br>\n", header );

  fprintf( fp, "<hr>\n" );
}


void html_stop( FILE *fp )
{
  fprintf( fp, "<hr>\n" );
  fprintf( fp, "</body>\n" );
  fprintf( fp, "</html>\n" );
  fclose( fp );
}
