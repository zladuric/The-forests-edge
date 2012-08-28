#!/usr/bin/perl

use CGI ':standard';
use Socket;
use Sys::Hostname;

print header( -type => 'text/html' );

$iaddr = inet_aton( hostname() );
$port = 2001;
$mud_port = 2000;

$paddr = sockaddr_in( $port, $iaddr );
$proto = getprotobyname( "tcp" );

$dead = 0;

if( !socket( SOCK, PF_INET, SOCK_STREAM, $proto )
    || !connect( SOCK, $paddr ) ) {
  $dead = 1;
}

@lines = <SOCK>;

close SOCK;

print start_html( -title => 'Online TFE Players',
		  -BGCOLOR => '#305a7a',
		  -text => 'ivory',
		  -link => '#33ccff',
		  -vlink => '#66cccc',
		  -alink => '#33ccff' );

print "<table style=\"width: 100%; text-align: left;\" border=\"0\" cellspacing=\"0\"
 cellpadding=\"0\">
  <tbody>
    <tr>
      <td style=\"background-color: rgb(163, 191, 147);\"> <img
 src=\"scary_forest.jpg\" title=\"\" alt=\"\"
 style=\"width: 224px; height: 150px;\" width=\"224\" height=\"150\"><br>
      </td>
      <td
 style=\"background-color: rgb(163, 191, 147); text-align: center; vertical-align: middle;\"><big
 style=\"color: ivory;\"><big><big><big><span
 style=\"font-style: italic;\">The Forest's Edge</span></big></big></big></big><br>
      </td>
      <td style=\"background-color: rgb(163, 191, 147);\"><img
 src=\"path.jpg\" title=\"\" alt=\"\" style=\"width: 200px; height: 150px;\"
 align=\"right\"> </td>
    </tr>
  </tbody>
</table>
<br><br>";

print "<table cellpadding=\"0\" cellspacing=\"0\" border=\"0\"
 style=\"text-align: left; width: 100%;\">
  <tbody>
    <tr><td><br></td></tr>
    <tr><td style=\"vertical-align: top; width: 150px;\">
      <a href=\"index.html\">Home</a><br><br>
      <a href=\"telnet://tfe.genesismuds.com:2000\">Connect</a><br><br>
      <a href=\"players.html\">Player Directory</a><br><br>
      <a href=\"who.cgi\">Who's Online</a><br><br>
      <a href=\"help/index.html\">Help Pages</a><br><br>
      <a href=\"clans/index.html\">Player Clans</a><br><br>
      <a href=\"resources.html\"> Resources</a><br><br>
      <a href=\"credits.html\">Credits</a><br>
      </td>";

print "<td style=\"vertical-align: top;\">
      <div align=\"left\"><big><big>Online TFE Players</big></big><br>
      </div>
      <br>";


if( $dead ) {
  printf "<p>Unable to contact TFE server at %s:%d.</p>", hostname(), $mud_port;
} else {
  print "<pre>";
  foreach $line ( @lines ) {
    $line =~ s/\r//g;
    chop( $line );
    print ansi_to_html( "$line" );
    print "\n";
  }
  print "</pre>";
}

print "</tr></tbody></table>";

print end_html;

exit;


sub ansi_to_html
{
  my %ansi_on =
    ( 0  => '',
      1  => '<span style="font-weight:bold;">',
      4  => '<span style="text-decoration:underline;">',
      7  => '<span style="font-style:italic;">',
      30 => '<span style="color:black;">',
      31 => '<span style="color:red;">',
      32 => '<span style="color:green;">',
      33 => '<span style="color:yellow;">',
      34 => '<span style="color:blue;">',
      35 => '<span style="color:magenta;">',
      36 => '<span style="color:cyan;">',
      37 => '<span style="color:#a9a9a9;">'
      );

  my $line = $_[0];

  $line =~ s/\&/&amp;/g;
  $line =~ s/</&lt;/g;
  $line =~ s/>/&gt;/g;
  $line =~ s/\"/&quot;/g;

  $spans = 0;

  while( $line =~ /^([^\033]*)\033\[((\d+;)*\d+)m(.*)$/ ) {
    $line = $1;
    $end = $4;
    
    @escs = split( ';', $2 );

    foreach $code ( @escs ) {
      $line .= $ansi_on{ $code };
      if( $code == 0 ) {
	$line .= "</span>" x $spans;
	$spans = 0;
      } else {
	++$spans;
      }
    }

    $line .= $end;
  }

  return $line;
}
