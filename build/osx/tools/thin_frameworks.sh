#!/bin/bash
#
# Thins all of our frameworks according to the given architecture and
# cleans up any header files to reduce size

if [ "x$3" = "x" ]; then
  echo "Usage: $0 LIPO_COMMAND ARCHITECTURE APP_DIR"
  exit 1
fi

lipo="$1"
arch="$2"
appdir="$3"

echo "=> Thinning out installed frameworks for: $arch"

echo "==> Stripping unwanted architectures"

# Go to the frameworks directory
pushd "${appdir}/Contents/Frameworks/" 1>/dev/null

# Do the strip!
for fw_dir in `ls -d *.framework`
do
  fw_name="${fw_dir%\.framework}"
  
  echo "--> $fw_name"
  
  pushd "$fw_dir/Versions/A" 1>/dev/null
  cp "$fw_name" "$fw_name".orig
  "$lipo" "$fw_name".orig -extract ${arch} -output "$fw_name"
  rm "$fw_name".orig
  popd 1>/dev/null
done

echo "==> Removing headers"

# Now clean up the headers
find . -type f -name '*.h' -exec rm -f {} \;

popd 1>/dev/null
