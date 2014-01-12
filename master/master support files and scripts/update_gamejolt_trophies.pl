#!/usr/bin/perl
#
# Script to cycle through all achievements and, for those players who have provided Game Jolt credentials,
# will notify Game Jolt that the achievement has been achieved.
# 
# Note that this requires the Game Jolt id to be entered into the achievement table in the database.
#

use strict;
use DBI;
use Digest::MD5 qw(md5_hex);

my $game_id = "20546";

# First, get the info we need from the master.ini file
my $iniFile = "/home/master/bitfighter/master/master.ini";
open (INI, "$iniFile") || die "Can't open $iniFile: $!\n";

my %ini;
my $section;

while (<INI>) {
   chomp;
   if(/^\s*\[(\w+)\].*/) {      # Search for [section]
       $section = $1;
   }

   elsif(/^\W*(\w+)=?(\w+)\W*(#.*)?$/) {   # Search for key=value
      my $key = $1;
      my $value = $2;

      # Store them in a hash
      $ini{$section}{$key} = $value;
   }
}
close (INI);

# If GameJolt integration is disabled, we can quit now
my $useGameJolt = $ini{"GameJolt"}{"UseGameJolt"};
if(lc($useGameJolt) eq "no") { exit 0; }

# Things we'll need:
my $secret = $ini{"GameJolt"}{"GameJoltSecret"};

my $dbhost     = $ini{"stats"}{"stats_database_addr"};
my $dbname     = $ini{"stats"}{"stats_database_name"};
my $dbusername = $ini{"stats"}{"stats_database_username"};
my $dbpassword = $ini{"stats"}{"stats_database_password"};


# Now retrieve the various achievements from the database

my $dbh   = DBI->connect("DBI:mysql:$dbname", $dbusername, $dbpassword) || die "Connection Error: $DBI::errstr\n";
my $sql   = <<END_SQL;

SELECT a.gamejolt_id, d.pf_gj_user_name, d.pf_gj_user_token
FROM bf_stats.achievements AS a
LEFT JOIN bf_stats.player_achievements AS pa      ON pa.achievement_id = a.id
LEFT JOIN bf_phpbb.phpbb_users AS u               ON u.username = pa.player_name
LEFT JOIN bf_phpbb.phpbb_profile_fields_data AS d ON d.user_id = u.user_id
WHERE d.pf_gj_user_name  IS NOT NULL AND
      d.pf_gj_user_token IS NOT NULL

END_SQL

my $query = $dbh->prepare($sql);
$query->execute || die "SQL Error: $DBI::errstr\n";

# Loop through the achievements
while(my @row = $query->fetchrow_array) 
{
   my($trophy_id, $username, $user_token) = @row;
   my $url = "http://gamejolt.com/api/game/v1/trophies/add-achieved?game_id=$game_id\&username=$username\&user_token=$user_token\&trophy_id=$trophy_id";
   my $signature = md5_hex($url, $secret);
   
   # Add the signature we calculated
   $url .= "\&signature=$signature";

   system("curl", "-s", "$url");
} 

