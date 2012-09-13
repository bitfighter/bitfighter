# This script preprocesses our C++ code and generates fodder for Doxygen which makes pretty documentation
# for our Lua scripters out there

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
my @files = <../zap/*.cpp>;
# my @files = ("../zap/BfObject.cpp", "../zap/teleporter.cpp");
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
         unshift(@{$classes{$class}}, "$shortClassDescr\n$longClassDescr\nclass $class : public $parent { \n public:\n");    
         $writeFile = 1;
         $shortClassDescr = "";
         $longClassDescr = "";
         next;
      }

      if( $line =~ m|define +LUA_METHODS\(CLASS, *METHOD\)| ) {
         $collectingMethods = 1;
         next;
      }

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

         if( $line =~ m|\@descr +(.*)$| ) {
            push(@comments, "\n $1\n");
            next;
         }

         # otherwise...
         push(@comments, $line);
      }
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
