# This script preprocesses our C++ code and generates fodder for Doxygen which makes pretty documentation
# for our Lua scripters out there

use strict;            # Require vars to be declared!
use File::Basename;
use List::Util 'first';

# Need to be in the doc directory
chdir "doc" || die "Could not change to doc folder: $!";

# Relative path where intermediate outputs will be written
my $outpath = "temp-doxygen/";

# Create it if it doesn't exist
if (! -d $outpath) {
   mkdir($outpath) || die "Could not create folder $outpath: $!";
}

# These are items we collect and build up over all pages, and they will get written to a special .h file at the end
my @mainpage = ();
my @enums = ();

# Iterate over all cpp folders in our working dir converting Doxygen comments to Luadoc comments
my @files = <../zap/*>;

push(@files, "./luadoc_static_text.txt");

# Loop through all the files we found above...
foreach my $file (@files) {

   next unless( $file =~ m|\.cpp$| || $file =~ m|\.h$|);    # Skip all but .cpp and .h files

   open my $IN, "<", $file || die "Could not open $file for reading: $!";

   my $writeFile = 0;

   # Various modes we could be in
   my $collectingMethods = 0;
   # my $collectingLongDescr = 0;
   my $collectingLongComment = 0;
   my $collectingMainPage = 0;
   my $collectingEnum = 0;

   my $enumColumn;
   my $enumIgnoreColumn;
   my $enumName;

   my @methods = ();
   my @comments = ();
   my %classes = ();        # Will be a hash of arrays

   my $shortClassDescr = "";
   my $longClassDescr  = "";

   my %alreadyWroteHeader = ();

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
         if( $line =~ m|METHOD\( *CLASS, *(.+?) *,| ) {                 # Signals class declaration... methods will follow
            my $method = $1;
            push(@methods, $method);
            next;
         }

         if( $line =~ m|GENERATE_LUA_METHODS_TABLE\( *(.+?) *,| ) {     # Signals we have all methods for this class, gives us class name; now generate code
            my $class = $1;

            foreach my $method (@methods) {
               push(@{$classes{$class}}, "void $method() { }\n");
            }

            @methods = ();
            $collectingMethods = 0;
            next;
         }
      }


      if( $line =~ m|/\*\*| ) {              # /** signals the beginning of a long comment block we need to pay attention to
         $collectingLongComment = 1;
         push(@comments, "/*!\n");
         next;
      }

      if( $collectingLongComment ) {

         # Look for closing */ to terminate our long comment
         if( $line =~ m|\*/| ) {
            push(@comments, "*/\n");
            $collectingLongComment = 0;
            $collectingMainPage = 0;
            next;
         }

         # $line =~ s|^\ *\* *||;  # Strip off leading *s and spaces

         if( $line =~ m|\@mainpage\s| ) {
            push(@mainpage, $line);
            $collectingMainPage = 1;
            next;
         }

         # Check for some special custom tags...

         # Handle Lua enum defs: "@luaenum ObjType(2)" or "@luaenum ObjType(2,1)"
         #                            $1        $2           $3  <== $3 will not appear in all lines
         if( $line =~ m|\@luaenum\s+(\w+)\s*\((\d+)\s*,?\s*(\d+)?\)| ) {
            $collectingEnum = 1;
            $enumName = $1;
            $enumColumn = $2;
            $enumIgnoreColumn = $3 eq "" ? -1 : $3;

            push(@enums, "/**\n\@defgroup $enumName"."Enum $enumName\n");

            next;
         }

         if( $line =~ m|\@luafunc\s+(.*)$| ) {     # Line looks like:  * @luafunc  retval BfObject::getClassID(p1, p2); retval and p1/p2 are optional
            push(@comments, " \\fn $1\n");

            #               $1      $2     $3    $4     ($1 grabs extra spaces, trimmed below)
            $line =~ m|\s(\w+\s+)?(\w+)::(.+?)\((.*)\)|;    # Grab retval, class, method, and args from $line
            my $retval = $1 eq "" ? "void" : $1;                              # Retval is optional, use void if omitted            
            my $class  = $2  || die "Couldn't get class name from $line\n";   # Must have a class
            my $method = $3  || die "Couldn't get method name from $line\n";  # Must have a method
            my $args   = $4;                                                  # Args are optional

            $retval =~ s|\s+$||;     # Trim any trailing spaces from $retval


            # Find the original class definition and delete it
            my $index = first { ${$classes{$class}}[$_] eq "void $method() { }\n" } 0..$#{$classes{$class}};
            if($index ne "") {
               splice(@{$classes{$class}}, $index, 1);       # Delete element at $index
            }


            chomp($line);     # Remove trailing \n

            # Add our new sig to the list
            push(@{$classes{$class}}, "$retval $method($args) { /* From '$line' */ }\n");

            next;
         }

         if( $line =~ m|\@luaclass\s+(\w+)\s*$| ) {       # Description of a class defined in a header file
            push(@comments, " \\class $1\n");
            next;
         }

         if( $line =~ m|\@luavclass\s+(\w+)\s*$| ) {      # Description of a virtual class, not defined in any C++ code
            my $class = $1;
            push(@comments, " \\class $class\n");

            push(@{$classes{$class}}, "class $1 {\n");
            push(@{$classes{$class}}, "public:\n");
            next;
         }

         if( $line =~ m|\@descr\s+(.*)$| ) {
            push(@comments, "\n $1\n");
            next;
         }

         # Otherwise keep the line unaltered and put it in the appropriate array
         if($collectingMainPage) {
            push(@mainpage, $line);
         } elsif($collectingEnum) {
            push(@enums, $line);
         } else {
            push(@comments, $line);
         }

         next;
      }

      # Starting with an enum def that looks like this:
      # /**
      #  * @luaenum Weapon(2[,n])  <=== 2 refers to 0-based index of column containing Lua enum name, n refers to column specifying whether to include this item
      #  * The Weapon enum can be used to represent a weapon in some functions.
      #  */
      #  #define WEAPON_ITEM_TABLE \
      #    WEAPON_ITEM(WeaponPhaser,     "Phaser",      "Phaser",     100,   500,   500,  600, 1000, 0.21f,  0,       false, ProjectilePhaser ) \
      #    WEAPON_ITEM(WeaponBounce,     "Bouncer",     "Bouncer",    100,  1800,  1800,  540, 1500, 0.15f,  0.5f,    false, ProjectileBounce ) \
      #  
      #
      # Make this:
      # /** @defgroup WeaponEnum Weapon
      #  *  The Weapons enum has values for each type of weapon in Bitfighter.
      #  *  @{
      #  *  @section Weapon
      #  * __Weapon__
      #  * * %Weapon.Phaser
      #  * * %Weapon.Bouncer
      #  @} 

      if($collectingEnum) {
         # If we get here we presume the @luaenum comment has been closed, and the next #define we see will begin the enum itself
         # Enum will continue until we hit a line with no trailing \
         if( $line =~ m|#\s*define| ) {
            push(@enums, "\@\{\n");
            push(@enums, "__Values__\n");
            $collectingEnum = 2;
            next;
         }

         next if $collectingEnum == 1;    # Skip lines until we hit a #define

         # Skip blank lines, or those that look like they are starting with a comment
         unless( $line =~ m|^\s*$| or $line =~ m|^\s*//| or $line =~ m|\s*/\*| ) {
            my @words = split(/,/, $line);   # Line looks like this:  WEAPON_ITEM(WeaponTriple,     "Triple",      "Triple",    ...

            # Skip items marked as not to be shared with Lua... see #define TYPE_NUMBER_TABLE for example
            next if($enumIgnoreColumn != -1 && $words[$enumIgnoreColumn] eq "false");     

            my $enumval = $words[$enumColumn];
            $enumval =~ s|[\s"\)\\]*||g;         # Strip out quotes and whitespace and other junk

            push(@enums, " * * \%" . $enumName . "." . $enumval . "\n");    # Produces:  * * %Weapon.Triple
            # no next here, always want to do the termination check below
         }


         if( $line !~ m|\\$| ) {          # Line has no terminating \, it's the last of its kind!
            push(@enums, "\@\}\n");       # Close doxygen group block
            push(@enums, "*/\n\n");       # Close comment

            $collectingEnum = 0;          # This enum is complete!
         }

         next;
      }
   }

   # If we added any lines to keepers, write it out... otherwise skip it!
   if($writeFile)
   {
      my ($name, $path, $suffix) = fileparse($file, (".cpp", ".h"));

      # Write the simulated .h file
      my $outfile = $outpath . basename($name) . ".h";
      open my $OUT, '>', $outfile || die "Can't open $outfile for writing: $!";

      print $OUT "// This file was generated automatically from the C++ source to feed doxygen.  It will be overwritten.\n\n\n";

      foreach my $key ( keys %classes ) {
         print $OUT @{$classes{$key}};    # Main body of class
         print $OUT "}; // $key\n";       # Close the class
      }

      print $OUT @comments;
      
      close $OUT;           
   }
   close $IN;
}


# Finally, write our main page data
my $outfile = $outpath . "main_page_content.h";
open my $OUT, '>', $outfile || die "Can't open $outfile for writing: $!";

print $OUT "// This file was generated automatically from the C++ source to feed doxygen.  It will be overwritten.\n\n\n";

print $OUT "/**\n";
print $OUT @mainpage;
print $OUT "*/\n";

print $OUT @enums;

close $OUT;           

