PLATFORM=$1
VERSION=$2
USER=$3

USAGE="build <win32|macosx|linux|source> <version> [user]"

if [ "$PLATFORM" != "win32" -a "$PLATFORM" != "macosx" -a "$PLATFORM" != "linux" -a "$PLATFORM" != "source" ]; then
   echo "$USAGE"
   exit 1
fi

if [ "$VERSION" = "" ]; then
   echo "$USAGE"
   exit 1
fi

if [ "$PLATFORM" = "source" ]; then

if [ "$USER" = "" ]; then
echo User must be specified for source build!
exit 1
fi

rm -rf tnl
cvs -d:ext:"$USER"@cvs.sourceforge.net:/cvsroot/opentnl export -r HEAD tnl
tar -czf tnl-"$VERSION"-source.tar.gz tnl
zip -r tnl-"$VERSION"-source.zip tnl

fi


DIR=zap-"$VERSION"

echo Writing to dir "$DIR"

rm -rf "$DIR"
mkdir "$DIR"

cp README.txt "$DIR"
mkdir "$DIR"/sfx
mkdir "$DIR"/levels

cp sfx/*.wav "$DIR"/sfx
cp levels/*.txt "$DIR"/levels

if [ "$PLATFORM" = "win32" ]; then
   ./installer/upx.exe --best --crp-ms=999999 --nrv2d -o "$DIR"/glut32.dll glut32.dll
   ./installer/upx.exe --best --crp-ms=999999 --nrv2d -o "$DIR"/zap.exe zap.exe
   cp OpenAL32.dll "$DIR"
   rm "$DIR"-win32.zip
   ./installer/makensis-bz2.exe /Dversion="$VERSION" installer/zap.nsi
fi

if [ "$PLATFORM" = "macosx" ]; then
   cp ZAP "$DIR"
   strip "$DIR"/ZAP
   rm "$DIR"-macosx.tar.gz
   tar -czf "$DIR"-macosx.tar.gz "$DIR"
fi

if [ "$PLATFORM" = "linux" ]; then
   cp zap "$DIR"
   cp zapded "$DIR"
   strip "$DIR"/zap
   strip "$DIR"/zapded
   rm "$DIR"-x86linux.tar.gz
   tar -czf "$DIR"-x86linux.tar.gz "$DIR"
fi

