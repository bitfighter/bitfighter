# Based on https://github.com/bobbens/naev/blame/master/docs/luadoc.sh
# Converted to Perl for cross-platform compatibility
# This script preprocesses our C++ code and generates fodder for luadoc which makes pretty documentation.

use strict;            # Require vars to be declared!
use File::Basename;

# Need to be in the doc directory
chdir "doc" || die "Could not change to doc folder: $!";

# Create doc/lua/ directory -- this should always exist
my $luadir = "lua";      
if (! -d $luadir) {
   mkdir($luadir) || die "Could not create folder $luadir: $!";
}


# Iterate over all cpp folders in our working dir converting Doxygen comments to Luadoc comments
# my @files = <../zap/*.cpp>;
my @files = ("../zap/BfObject.cpp", "../zap/teleporter.cpp");
foreach my $file (@files) {

   open my $IN, "<", $file || die "Could not open $file for reading: $!";

   my $writeFile = 0;

   # Various modes we could be in
   my $collectingMethods = 0;
   # my $collectingLongDescr = 0;
   my $collectingLongComment = 0;

   my @methods = ();
   my @comments = ();
   my %classes = ();        # Will be a hash of arrays

   my $shortClassDescr = "";
   my $longClassDescr  = "";

   # Visit each line of the source cpp file
   foreach my $line (<$IN>) {

      # Lua base classes
      if( $line =~ m|REGISTER_LUA_CLASS\( *(.+?) *\)| ) {
         my $class = $1;
         unshift(@{$classes{$class}}, "$shortClassDescr\n$longClassDescr\nclass $class { \n public:\n");   # unshift adds element to beginning of array
         $writeFile = 1;
         next;
      }

      # Lua subclasses
      if( $line =~ m|REGISTER_LUA_SUBCLASS\( *(.+?) *, *(.+?) *\)| ) {
         my $class = $1;
         my $parent = $2;
         unshift(@{$classes{$class}}, "$shortClassDescr\n$longClassDescr\nclass $class : $parent { \n public:\n");    
         $writeFile = 1;
         $shortClassDescr = "";
         $longClassDescr = "";
         next;
      }

      if( $line =~ m|define +LUA_METHODS\(CLASS, *METHOD\)| ) {
         $collectingMethods = 1;
         next;
      }

      # if( $line =~ m|\@luaclass (.*)$| ) {
      #    $shortClassDescr = "//! $1\n";
      #    next;
      # }

      # if( $line =~ m|\@descr (.*)$| ) {
      #    $longClassDescr = "//! $1\n";
      #    $collectingLongDescr = 1;
      #    next;
      # }

      # if( $collectingLongDescr ) {
      #    $line =~ s|^\s+||; $line =~ s|\s+$||;     # Trim whitespace from $line

      #    # Did we hit a blank line?
      #    if( $line eq "" ) {
      #       $longClassDescr .= "\n";
      #       $collectingLongDescr = 0;
      #       next;
      #    }

      #    # # Strip various comment decorators from line
      #    $line =~ s|^\?*+||;
      #    $line =~ s|^\/+||;

      #    $longClassDescr .= "//! $line\n";
      #    next;
      # }

      if( $collectingMethods ) {
         if( $line =~ m|METHOD\( *CLASS, *(.+?) *,| ) {
            my $method = $1;
            push(@methods, $method);
            next;
         }

         if( $line =~ m|GENERATE_LUA_METHODS_TABLE\( *(.+?) *,| ) {
            my $class = $1;

            foreach my $method (@methods) {
               push(@{$classes{$class}}, "void $method() { }\n");
            }

            @methods = ();
            $collectingMethods = 0;
            next;
         }
      }

      if( $line =~ m|/\*\*| ) {
         $collectingLongComment = 1;
         push(@comments, "/*!\n");
         next;
      }

      if( $collectingLongComment ) {

         # Look for closing */ to terminate our long comment
         if( $line =~ m|\*/| ) {
            push(@comments, "*/\n");
            $collectingLongComment = 0;
            next;
         }

         $line =~ s|^\ *\* *||;  # Strip off leading *s and spaces

         if( $line =~ m|\@luafunc +(.*)$| ) {
            push(@comments, " \\fn $1\n");
            next;
         }

         if( $line =~ m|\@luaclass +(.*)$| ) {
            push(@comments, " \\class $1\n");
            next;
         }

         if( $line =~ m|\@brief +(.*)$| ) {
            push(@comments, " \\brief $1\n");
            next;
         }

         if( $line =~ m|\@param +(.*)$| ) {
            push(@comments, " \\param $1\n");
            next;
         }

         if( $line =~ m|\@descr +(.*)$| ) {
            push(@comments, "\n $1\n");
            next;
         }


         # otherwise...
         push(@comments, $line);
      }


      # # Convert Doxygen /** to Luadoc ---
      # $line =~ s|^ */\*\* *$|---| && push(@keepers, $line) && next;  # s|^ */\*\* *$|---|p

      # # Convert special tags to Lua expressions, which is what Luadoc can parse
      # # Lines after @luafunc & @luamod will be ignored by Luadoc
      # # Doxygen comments that do not contain any of these tags have no impact on the Luadoc output
      #                                                                          # Original naev sed commands
      # $line =~ s|^ *\* *\@luafunc|function|             && push(@keepers, $line) && next;  # s|^ *\* *@luafunc|function|p

      # $line =~ s|^ *\* *\@luamod *(.*)|module "\1"|     && push(@keepers, $line) && next;  # s|^ *\* *@luamod *\(.*\)|module "\1"|p

      # # Rename some tags:      
      # $line =~ s|^ *\* *\@brief|-- \@description|       && push(@keepers, $line) && next;  # s|^ *\* *@brief|-- @description|p
      # $line =~ s|^ *\* *\@luasee|-- \@see|              && push(@keepers, $line) && next;  # s|^ *\* *@luasee|-- @see|p
      # $line =~ s|^ *\* *\@luaparam|-- \@param|          && push(@keepers, $line) && next;  # s|^ *\* *@luaparam|-- @param|p
      # $line =~ s|^ *\* *\@luareturn|-- \@return|        && push(@keepers, $line) && next;  # 
      # $line =~ s|^ *\* *\@usage|-- \@usage|             && push(@keepers, $line) && next;  # s|^ *\* *@usage|-- @usage|p
      # $line =~ s|^ *\* *\@description|-- \@description| && push(@keepers, $line) && next;  # s|^ *\* *@description|-- @description|p
      # $line =~ s|^ *\* *\@name|-- \@name|               && push(@keepers, $line) && next;  # s|^ *\* *@name|-- @name|p
      # $line =~ s|^ *\* *\@class|-- \@class|             && push(@keepers, $line) && next;  # s|^ *\* *@class|-- @class|p
      # $line =~ s|^ *\* *\@field|-- \@field|             && push(@keepers, $line) && next;  # s|^ *\* *@field|-- @field|p
      # $line =~ s|^ *\* *\@release|-- \@release|         && push(@keepers, $line) && next;  # s|^ *\* *@release|-- @release|p
       
      # # Custom tags:
      # $line =~ s|^ *\* *\@code|-- <pre>|                && push(@keepers, $line) && next;  # s|^ *\* *@code|-- <pre>|p
      # $line =~ s|^ *\* *\@endcode|-- </pre>|            && push(@keepers, $line) && next;  # s|^ *\* *@endcode|-- </pre>|p
    
      # # Remove other tags:
      # # $line =~ \|^ *\* *@.*|                          && push(@keepers, $line) && next;  # \|^ *\* *@.*|d
    
      # # Insert newline between comments, replace */ with \n:
      # # $line =~ \|^ *\*/| && push(@keepers, $line) && next; # \|^ *\*/|c

      # # Keep other comments, replace * with --
      # $line =~ s|^ *\*|--|                              && push(@keepers, $line) && next; # s|^ *\*|--|p

      # # Lines not matched above will not be copied into @keepers
      # #push(@keepers, $line);
   }

   # If we added any lines to keepers, write it out... otherwise skip it!
   if($writeFile)
   {
      my ($name, $path, $suffix) = fileparse($file, (".cpp"));
      my $outpath = "lua/";

      # Write the simulated .h file
      my $outfile = $outpath . basename($name) . ".h";
      open my $OUT, '>', $outfile || die "Can't open $outfile for writing: $!";

      print $OUT "// This file was generated automatically from the C++ source to feed doxygen.  It will be overwritten.\n\n\n";

      foreach my $key ( keys %classes ) {
         print $OUT @{$classes{$key}};          # Main body of class
         print $OUT "};\n";      # Close the class
      }

      print $OUT @comments;
      
      close $OUT;           
   }
   close $IN;
}
