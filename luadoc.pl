# Based on https://github.com/bobbens/naev/blame/master/docs/luadoc.sh
# Converted to Perl for cross-platform compatibility
# This script preprocesses our C++ code and generates fodder for luadoc which makes pretty documentation.

use strict;            # Require vars to be declared!
use File::Basename;

# Need to be in the doc directory
chdir "doc" || die "Could not change to doc folder: $!";

# Create doc/lua/ directory
my $luadir = "lua";      
if (! -d $luadir) {
   mkdir($luadir) || die "Could not create folder $luadir: $!";
}


# Iterate over all cpp folders in our working dir converting Doxygen comments to Luadoc comments
my @files = <../zap/*.cpp>;
foreach my $file (@files) {

   open my $IN, "<", $file || die "Could not open $file for reading: $!";
   my @lines = <$IN>;
   close $IN;

   # All lines from $file are now in @lines

   my @keepers = ("-- This file was generated automatically from the C++ sourcecode to feed Luadoc.  It will be overwritten.\n");

   # Visit each line converting Doxygen comments as we go
   for my $line (@lines) {
       # Convert Doxygen /** to Luadoc ---
      $line =~ s|^ */\*\* *$|---| && push(@keepers, $line) && next;  # s|^ */\*\* *$|---|p

      # Convert special tags to Lua expressions, which is what Luadoc can parse
      # Lines after @luafunc & @luamod will be ignored by Luadoc
      # Doxygen comments that do not contain any of these tags have no impact on the Luadoc output
                                                                               # Original naev sed commands
      $line =~ s|^ *\* *\@luafunc|function|             && push(@keepers, $line) && next;  # s|^ *\* *@luafunc|function|p

      # $line =~ s|^ *\* *\@luamod *\(.*\)|module "\1"|   && push(@keepers, $line) && next;  # s|^ *\* *@luamod *\(.*\)|module "\1"|p
      # Rename some tags:      
      $line =~ s|^ *\* *\@brief|-- \@description|       && push(@keepers, $line) && next;  # s|^ *\* *@brief|-- @description|p
      $line =~ s|^ *\* *\@luasee|-- \@see|              && push(@keepers, $line) && next;  # s|^ *\* *@luasee|-- @see|p
      $line =~ s|^ *\* *\@luaparam|-- \@param|          && push(@keepers, $line) && next;  # s|^ *\* *@luaparam|-- @param|p
      $line =~ s|^ *\* *\@luareturn|-- \@return|        && push(@keepers, $line) && next;  # 
      $line =~ s|^ *\* *\@usage|-- \@usage|             && push(@keepers, $line) && next;  # s|^ *\* *@usage|-- @usage|p
      $line =~ s|^ *\* *\@description|-- \@description| && push(@keepers, $line) && next;  # s|^ *\* *@description|-- @description|p
      $line =~ s|^ *\* *\@name|-- \@name|               && push(@keepers, $line) && next;  # s|^ *\* *@name|-- @name|p
      $line =~ s|^ *\* *\@class|-- \@class|             && push(@keepers, $line) && next;  # s|^ *\* *@class|-- @class|p
      $line =~ s|^ *\* *\@field|-- \@field|             && push(@keepers, $line) && next;  # s|^ *\* *@field|-- @field|p
      $line =~ s|^ *\* *\@release|-- \@release|         && push(@keepers, $line) && next;  # s|^ *\* *@release|-- @release|p
       
      # Custom tags:
      $line =~ s|^ *\* *\@code|-- <pre>|                && push(@keepers, $line) && next;  # s|^ *\* *@code|-- <pre>|p
      $line =~ s|^ *\* *\@endcode|-- </pre>|            && push(@keepers, $line) && next;  # s|^ *\* *@endcode|-- </pre>|p
    
      # Remove other tags:
      # $line =~ \|^ *\* *@.*|                          && push(@keepers, $line) && next;  # \|^ *\* *@.*|d
    
      # Insert newline between comments, replace */ with \n:
      # $line =~ \|^ *\*/| && push(@keepers, $line) && next; # \|^ *\*/|c

      # Keep other comments, replace * with --
      $line =~ s|^ *\*|--|                              && push(@keepers, $line) && next; # s|^ *\*|--|p

      # Lines not matched above will not be copied into @keepers
      #push(@keepers, $line);
   }

   # If we added any lines to keepers, write it out... otherwise skip it!
   if($#keepers > 1)
   {
      # Output lines we want to keep into a file
      my $outfile = "lua/" . basename($file) . ".luadoc";

      print "Saving $outfile +++ ...\n";
      
      open my $OUT, '>', $outfile || die "Can't open $outfile for writing: $!";
      print $OUT @keepers;

      close $OUT;
   }
}

# Pick the correct luadoc command; OS dependent
my$luadoc;
if($^O eq 'MSWin32') {
   $luadoc = "luadoc_start";
} else {
   $luadoc = "luadoc";
}


# Run Luadoc, put HTML files into lua/ dir
chdir "lua";
#system($luadoc . ' --nofiles --taglet "bitfighter-taglet" -t templates/ *.luadoc');
system($luadoc . ' --nofiles  -t templates/ *.luadoc');
# rm *.luadoc
