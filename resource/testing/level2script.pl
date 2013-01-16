#!perl
use strict;            # Require vars to be declared!
$SIG{__WARN__} = sub { die "Undef value: @_" if $_[0] =~ /undefined/ };


my $infile = 'C:\Users\Chris\Documents\bf-trunk\exe\levels\nexus.level';

open my $IN, "<", $infile || die "Could not open $infile for reading: $!";

print "local g = nil\n";

foreach my $line (<$IN>) {
   $line =~ m/^\w*GameType /     && next;
   $line =~ m/^LevelName|Specials|LevelCredits|Script|LevelDescription/ && next;
   $line =~ m/LevelCredits /     && next;
   $line =~ m/GridSize /         && next;
   $line =~ m/Team /             && next;
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



   if($line =~ m/(HuntersNexusObject|NexusObject)/) {
      my @points;

      while(@words) {
         my $x = (shift @words) * $gridsize;
         my $y = (shift @words) * $gridsize;
         push(@points, "point.new($x,$y)");
      }

      print "levelgen:addItem(NexusZone.new(". join(', ', @points), "))\n";
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



   if($line =~ m/(FlagItem|^Spawn|Turret|ForceFieldProjector)/) {
      my $team = (shift @words);
      $team >= 0 && $team++;   # Stupid lua arrays
      my $x = (shift @words) * $gridsize;
      my $y = (shift @words) * $gridsize;

      print "levelgen:addItem($1.new(point.new($x,$y), $team))\n";
      next;
   }


   if($line =~ m/(FlagSpawn)/) {
      my $team = (shift @words);
      $team >= 0 && $team++;   # Stupid lua arrays
      my $x = (shift @words) * $gridsize;
      my $y = (shift @words) * $gridsize;
      my $time = shift @words;

      print "levelgen:addItem($1.new(point.new($x,$y), $team, $time))\n";
      next;
   }


   if($line =~ m/(AsteroidSpawn|CircleSpawn)/) {
      my $x = (shift @words) * $gridsize;
      my $y = (shift @words) * $gridsize;
      my $time = shift @words;

      print "levelgen:addItem($1.new(point.new($x,$y), $time))\n";
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


   if($line =~ m/(SpeedZone)/) {
      my $x1 = (shift @words) * $gridsize;
      my $y1 = (shift @words) * $gridsize;
      my $x2 = (shift @words) * $gridsize;
      my $y2 = (shift @words) * $gridsize;
      my $spd = shift @words;
      my $snap = (shift @words) eq "SnapEnabled";    

      print "g = $1.new(point.new($x1,$y1), point.new($x2,$y2), $spd)\n";
      if($snap) { print "   g:setSnapping(true)\n"; }
      print "   levelgen:addItem(g)\n";
      next;
   }


   if($line =~ m/(Teleporter)/) {
      my $x1 = (shift @words) * $gridsize;
      my $y1 = (shift @words) * $gridsize;
      my $x2 = (shift @words) * $gridsize;
      my $y2 = (shift @words) * $gridsize;

      print "levelgen:addItem($1.new(point.new($x1,$y1), point.new($x2,$y2)))\n";
      next;
   }


   if($line =~ m/(ResourceItem|TestItem)/) {
      my $x = (shift @words) * $gridsize;
      my $y = (shift @words) * $gridsize;
      print "levelgen:addItem($1.new(point.new($x,$y)))\n";
      next;
   }


   print "XXXXXXXXXXXXXXXXXXXXXXXX\n$line\n";
}

=pod

=cut


