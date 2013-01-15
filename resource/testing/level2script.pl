#!perl
use strict;            # Require vars to be declared!
$SIG{__WARN__} = sub { die "Undef value: @_" if $_[0] =~ /undefined/ };


my $infile = 'C:\Users\Chris\AppData\Roaming\Bitfighter\levels/core.level';

open my $IN, "<", $infile || die "Could not open $infile for reading: $!";

foreach my $line (<$IN>) {
   $line =~ m/^\w*GameType /     && next;
   $line =~ m/^LevelName /       && next;
   $line =~ m/LevelDescription / && next;
   $line =~ m/LevelCredits /     && next;
   $line =~ m/GridSize /         && next;
   $line =~ m/Team /             && next;
   $line =~ m/Specials /         && next;
   $line =~ m/MinPlayers/        && next;
   $line =~ m/MaxPlayers/        && next;
   $line =~ m/^\s*$/             && next;

   my $gridsize = 255;

   my @words = split(' ', $line);
   shift @words;     # Remove item string

   if($line =~ m/BarrierMaker/) {
      my $width = shift @words;
      my @points;

      while(@words) {
         my $x = (shift @words) * $gridsize;
         my $y = (shift @words) * $gridsize;
         push(@points, "point.new($x,$y)");
      }

      print "levelgen:addItem(WallItem.new(",join(', ', @points), ",", $width, "))\n";
      next;
   }



   if($line =~ m/(GoalZone|LoadoutZone)/) {
      my $team = (shift @words);
      $team >= 0 && $team++;   # Stupid lua arrays
      my @points;

      while(@words) {
         my $x = (shift @words) * $gridsize;
         my $y = (shift @words) * $gridsize;
         push(@points, "point.new($x,$y)");
      }

      print "levelgen:addItem($1.new(". join(', ', @points), ", $team))\n";
      next;
   }


   if($line =~ m/(PolyWall)/) {
      my @points;

      while(@words) {
         my $x = (shift @words) * $gridsize;
         my $y = (shift @words) * $gridsize;
         push(@points, "point.new($x,$y)");
      }

      print "levelgen:addItem($1.new(". join(', ', @points), "))\n";
      next;
   }


   if($line =~ m/(FlagItem|Spawn|Turret)/) {
      my $team = (shift @words);
      $team >= 0 && $team++;   # Stupid lua arrays
      my $x = (shift @words) * $gridsize;
      my $y = (shift @words) * $gridsize;

      print "levelgen:addItem($1.new(point.new($x,$y), $team))\n";
      next;
   }



   if($line =~ m/(Core)/) {
      my $team = (shift @words);
      $team >= 0 && $team++;   # Stupid lua arrays

      my $health = (shift @words);
      my $x = (shift @words) * $gridsize;
      my $y = (shift @words) * $gridsize;
      
      print "levelgen:addItem(CoreItem.new(point.new($x,$y), $team, $health))\n";
      next;
   }



   if($line =~ m/(RepairItem|EnergyItem)/) {
      my $x = (shift @words) * $gridsize;
      my $y = (shift @words) * $gridsize;
      my $time = shift @words;
      print "levelgen:addItem($1.new(point.new($x,$y), $time))\n";
      next;
   }


   if($line =~ m/(ResourceItem|TestItem)/) {
      my $x = (shift @words) * $gridsize;
      my $y = (shift @words) * $gridsize;
      print "levelgen:addItem($1.new(point.new($x,$y)))\n";
      next;
   }
}

=pod

=cut


