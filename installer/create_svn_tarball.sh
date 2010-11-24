#!/bin/bash
#
# Ugly script to create source tarball for bitfighter and upload to google code.
#

#
#  DO NOT CHECK THIS IN WITH YOUR ACCOUNT DETAILS IN IT!!!!
#

# hardcoded vars
# replace e-mail and svn password in between the quotes
gc_username="someemail@example.com"
gc_password="randomlygeneratedpasswordRJKoidRu802"
gc_upload_script_location="http://support.googlecode.com/svn/trunk/scripts/googlecode_upload.py"
gc_upload_script=`basename $gc_upload_script_location`

# Check for subversion
svn="`which svn`"

if [ -z $svn ]
then
    echo "This script requires subversion. Please install it."
    exit 1
fi

# Check for parameter svn path
if [ -z "$2" ] || [ "$1" == "-h" ] || [ "$1" == "--help" ] 
then
  echo "Usage: " `basename $0` " SUBVERSION_REPOSITORY_PATH BITFIGHTER_VERSION"
  echo
  echo "Example: `basename $0` http://bitfighter.googlecode.com/svn/tags/bitfighter-013e 013e"
  echo
  exit 0
fi

# Setup
export bold=`tput bold`
export off=`tput sgr0`

# Argument variables
repo_path="$1"

# Ask for version
version=""
while [ -z $answer ]
do
  echo -n "What is the release version? (e.g. 013f): "
  read answer
  if [ -n $answer ]; then
    version="$answer"
  fi
done

echo " => You chose version $version"

tarball_root="bitfighter-$version"

# Create temp dir
tmp_dir="`mktemp -d`"
echo " => tmp dir: $tmp_dir"
cd "$tmp_dir"

# Do the checkout
echo " => Checking out $repo_path"
echo "    (may take a minute)"
svn co "$repo_path" "$tarball_root" 1>/dev/null

if [ ! -d $tarball_root ]; then
  echo "Your source did not check out properly.  Check the path. Exiting"
  exit 1
fi

# Compress
echo " => Compressing $tarball_root.tar.gz"
tar cfz "$tarball_root.tar.gz" --exclude=.svn "$tarball_root"

if [ $? -ne 0 ]; then
  echo "Tar did not compress properly.  Exiting"
  exit 1
fi

# Download the upload script on the fly
echo " => Downloading Google Code upload script, $gc_upload_script"
wget -q "$gc_upload_script_location"
chmod +x "$gc_upload_script"

if [ ! -f $gc_upload_script ]; then
  echo "The Google Code upload script was not downloaded correctly.  Check the URL. Exiting"
  exit 1
fi

# Upload to google code
echo " => Uploading to Google Code (may take a minute)"
./$gc_upload_script --summary="bitfighter $version source archive" --project=bitfighter --user="$gc_username" --password="$gc_password" $tarball_root.tar.gz

if [ $? -ne 0 ]; then
  echo "There was a failure in uploading.  Exiting"
  exit 1
fi

# Clean up my mess
echo " => Cleaning up"
rm -rf "$tmp_dir"

echo "Done!"

exit 0