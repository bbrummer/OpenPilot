################################################################################
#!/bin/bash
################################################################################
#
# This script compiles GDAL and other dependencies needed by OSG and OSGEarth
#
# A good source for building with mingw :
#    http://www.gaia-gis.it/gaia-sins/mingw_how_to.html
#
################################################################################

# .\OpenPilotTools\msys\bin\sh --login -i -c /d/Projects/OpenPilot/make/3rdparty/osg_win.sh

################################################################################
# Environment
################################################################################

#WORKING_DIR=$PWD
WORKING_DIR=/C/OP/TP-WIN-JOB1/make/3rdparty
echo Working dir $WORKING_DIR

ROOT_DIR=$WORKING_DIR/../..

DOWNLOAD_DIR=$ROOT_DIR/downloads/osg
SOURCE_DIR=$ROOT_DIR/3rdparty/osg_dependencies
BUILD_DIR=$ROOT_DIR/build/3rdparty/osg_dependencies

HOST=mingw32

DOWNLOAD_FILES=1

umount /mingw
mount C:/Apps/tools/qt-5.4.0/Tools/mingw491_32 /mingw
mount

echo Path $PATH

ls /mingw
ls /C/Apps/tools/qt-5.4.0/Tools/mingw491_32

################################################################################
# wget
################################################################################

if [ "$DOWNLOAD_FILES" -eq 1 ]; then

mkdir -p $DOWNLOAD_DIR/

wget -P $DOWNLOAD_DIR/ http://pkgconfig.freedesktop.org/releases/pkg-config-0.28.tar.gz
wget -P $DOWNLOAD_DIR/ http://zlib.net/zlib-1.2.8.tar.gz
wget -P $DOWNLOAD_DIR/ http://www.ijg.org/files/jpegsrc.v9a.tar.gz
wget -P $DOWNLOAD_DIR/ http://sourceforge.net/projects/libpng/files/libpng16/1.6.16/libpng-1.6.16.tar.gz
wget -P $DOWNLOAD_DIR/ ftp://ftp.remotesensing.org/pub/libtiff/tiff-4.0.3.tar.gz
wget -P $DOWNLOAD_DIR/ http://sourceforge.net/projects/freetype/files/freetype2/2.5.3/freetype-2.5.3.tar.gz
wget -P $DOWNLOAD_DIR/ http://curl.haxx.se/download/curl-7.38.0.tar.gz
wget -P $DOWNLOAD_DIR/ http://download.osgeo.org/proj/proj-4.8.0.tar.gz
wget -P $DOWNLOAD_DIR/ http://download.osgeo.org/proj/proj-datumgrid-1.5.tar.gz
wget -P $DOWNLOAD_DIR/ http://download.osgeo.org/geos/geos-3.3.8.tar.bz2
wget -P $DOWNLOAD_DIR/ http://download.osgeo.org/gdal/1.10.1/gdal-1.10.1.tar.gz

fi

################################################################################
# wget
################################################################################

# list of libraries to build
# other candidates include bzip2, libxml2, gif, geotiff, ssl, gl...
BUILD_PKGCONFIG=1
BUILD_ZLIB=1
BUILD_LIBJPEG=1
BUILD_LIBPNG=1
BUILD_LIBTIFF=1
BUILD_FREETYPE=1
BUILD_OPENSSL=2
BUILD_CURL=1
BUILD_PROJ4=1
BUILD_GEOS=1
BUILD_GDAL=1

# TODO
#  libcurl needs to be built with zlib and ssl support
#  gdal does not seem to link with shared proj4
#  document libraries (required by, requires)

mkdir -p $SOURCE_DIR/
mkdir -p $BUILD_DIR/bin/
mkdir -p $BUILD_DIR/include/
mkdir -p $BUILD_DIR/lib/

# make sure all libraries see each others
export PATH=$BUILD_DIR/bin:$PATH
export CPATH=$BUILD_DIR/include
export LIBRARY_PATH=$BUILD_DIR/lib
export PKG_CONFIG_PATH=$BUILD_DIR/lib/pkgconfig

# reduce binary sizes by removing the -g option (not picked up by all libraries)
export CFLAGS=-O2
export CXXFLAGS=-O2

################################################################################
# pkg-config
# required by libcurl, gdal, osg, osgearth
################################################################################

if [ "$BUILD_PKGCONFIG" -eq 1 ]; then

echo "****************************************"
echo "Building pkg-config..."
echo "****************************************"

cd $SOURCE_DIR
tar xzf $DOWNLOAD_DIR/pkg-config-0.28.tar.gz -C .
cd pkg-config-0.28

./configure --prefix=$BUILD_DIR --build=$HOST \
  --with-internal-glib
make
make install

fi

################################################################################
# ZLIB
# required by libcurl, gdal, osg, osgearth
################################################################################

if [ "$BUILD_ZLIB" -eq 1 ]; then

echo "****************************************"
echo "Building zlib..."
echo "****************************************"

cd $SOURCE_DIR
tar xzf $DOWNLOAD_DIR/zlib-1.2.8.tar.gz -C .
cd zlib-1.2.8

make -f win32/Makefile.gcc clean
make -f win32/Makefile.gcc

cp -f zlib1.dll $BUILD_DIR/bin/
cp -f zconf.h $BUILD_DIR/include/
cp -f zlib.h $BUILD_DIR/include/
cp -f libz.a $BUILD_DIR/lib/
cp -f libz.dll.a $BUILD_DIR/lib/

fi

################################################################################
# LIBJPEG
# required by gdal, osg, osgearth
################################################################################

if [ "$BUILD_LIBJPEG" -eq 1 ]; then

echo "****************************************"
echo "Building libjpeg..."
echo "****************************************"

cd $SOURCE_DIR
tar xzf $DOWNLOAD_DIR/jpegsrc.v9a.tar.gz -C .
cd jpeg-9a

./configure --prefix=$BUILD_DIR --build=$HOST
make
make install-strip

fi

################################################################################
# LIBPNG
# required by gdal, osg, osgearth
################################################################################

if [ "$BUILD_LIBPNG" -eq 1 ]; then

echo "****************************************"
echo "Building libpng..."
echo "****************************************"

cd $SOURCE_DIR
tar xzf $DOWNLOAD_DIR/libpng-1.6.16.tar.gz -C .
cd libpng-1.6.16

./configure --prefix=$BUILD_DIR --build=$HOST
make
make install-strip

fi

################################################################################
# LIBTIFF
# reqires zlib
# required by gdal, osg, osgearth
################################################################################

if [ "$BUILD_LIBTIFF" -eq 1 ]; then

echo "****************************************"
echo "Building libtiff..."
echo "****************************************"

cd $SOURCE_DIR
tar xzf $DOWNLOAD_DIR/tiff-4.0.3.tar.gz -C .
cd tiff-4.0.3

./configure --prefix=$BUILD_DIR --build=$HOST
make
make install-strip

fi

################################################################################
# FreeType
################################################################################

if [ "$BUILD_FREETYPE" -eq 1 ]; then

echo "****************************************"
echo "Building FreeType..."
echo "****************************************"

cd $SOURCE_DIR
tar xzf $DOWNLOAD_DIR/freetype-2.5.3.tar.gz -C .
cd freetype-2.5.3

#unzip -q $DOWNLOAD_DIR/freetype-2.3.5-1-src.zip src/freetype/2.3.5/freetype-2.3.5/* -d $TEMP/freetype-2.3.5
#cp -r $TEMP/freetype-2.3.5/src/freetype/2.3.5/freetype-2.3.5 ./
#rm -rf $TEMP/freetype-2.3.5
#cd freetype-2.3.5

./configure --prefix=$BUILD_DIR
make
make install

# hack for osg
#mv $BUILD_DIR/include/freetype2 $BUILD_DIR/include/freetype

fi

################################################################################
# OpenSSL
################################################################################

if [ "$BUILD_OPENSSL" -eq 1 ]; then

echo "****************************************"
echo "Building OpenSSL..."
echo "****************************************"

cd $SOURCE_DIR
tar xzf $DOWNLOAD_DIR/curl-7.38.0.tar.gz -C .
cd curl-7.38.0

./configure --prefix=$BUILD_DIR --build=$HOST
make
make install

fi

################################################################################
# cURL
# required by gdal, osgearth
################################################################################

if [ "$BUILD_CURL" -eq 1 ]; then

echo "****************************************"
echo "Building cURL..."
echo "****************************************"

cd $SOURCE_DIR
tar xzf $DOWNLOAD_DIR/curl-7.38.0.tar.gz -C .
cd curl-7.38.0

# see http://curl.haxx.se/docs/install.html
#set ZLIB_PATH=c:\zlib-1.2.8
#set OPENSSL_PATH=c:\openssl-0.9.8y
#set LIBSSH2_PATH=c:\libssh2-1.4.3
#mingw32-make mingw32-zlib' to build with Zlib support;
#mingw32-make mingw32-ssl-zlib' to build with SSL and Zlib enabled;
#mingw32-make mingw32-ssh2-ssl-zlib' to build with SSH2, SSL, Zlib;
#mingw32-make mingw32-ssh2-ssl-sspi-zlib' to build with SSH2, SSL, Zlib

#mingw32-make mingw32

#cp -f src/curl.exe $BUILD_DIR/bin/
#cp -rf include/curl $BUILD_DIR/include/
#cp -f lib/libcurl.dll $BUILD_DIR/bin/
#cp -f lib/libcurldll.a $BUILD_DIR/lib/
#cp -f lib/libcurl.a $BUILD_DIR/lib/

./configure --prefix=$BUILD_DIR --build=$HOST \
  --enable-shared=yes --with-zlib=$BUILD_DIR
make
make install-strip

fi

################################################################################
# PROJ.4
# required by osgearth
################################################################################

if [ "$BUILD_PROJ4" -eq 1 ]; then

echo "****************************************"
echo "Building PROJ.4..."
echo "****************************************"

cd $SOURCE_DIR
tar -xzf $DOWNLOAD_DIR/proj-4.8.0.tar.gz -C .
tar -xzf $DOWNLOAD_DIR/proj-datumgrid-1.5.tar.gz -C proj-4.8.0/nad/
cd proj-4.8.0

./configure --prefix=$BUILD_DIR --build=$HOST \
  --enable-static=no --enable-shared=yes
make
make install

fi

################################################################################
# GEOS
# required by gdal
################################################################################

if [ "$BUILD_GEOS" -eq 1 ]; then

echo "****************************************"
echo "Building GEOS..."
echo "****************************************"

cd $SOURCE_DIR
tar xjf $DOWNLOAD_DIR/geos-3.3.8.tar.bz2 -C .
cd geos-3.3.8

# TODO why --disable-inline?
./configure --prefix=$BUILD_DIR --build=$HOST \
  --enable-static=no --enable-shared=yes --disable-inline
make
make install

fi

################################################################################
# GDAL
# requires zlib, libcurl, libpng, libjpeg, libtiff, geos
# required by osgearth
################################################################################

if [ "$BUILD_GDAL" -eq 1 ]; then

echo "****************************************"
echo "Building GDAL..."
echo "****************************************"

cd $SOURCE_DIR
tar xzf $DOWNLOAD_DIR/gdal-1.10.1.tar.gz -C .
cd gdal-1.10.1

# fix GNUmakefile as described here http://trac.osgeo.org/gdal/wiki/BuildingWithMinGW
patch < $WORKING_DIR/gdal_GNUmakefile_fix.diff

./configure --prefix=$BUILD_DIR --build=$HOST \
  --without-python --without-libtool \
  --with-xerces=no \
  --with-libz=$BUILD_DIR --with-curl=$BUILD_DIR \
  --with-png=$BUILD_DIR --with-jpeg=$BUILD_DIR --with-libtiff=$BUILD_DIR \
  --with-geos=$BUILD_DIR/bin/geos-config
make
make install

#  --with-xerces=no --with-libz=$BUILD_DIR  \
#  --with-png=$BUILD_DIR --with-jpeg=$BUILD_DIR --with-curl=$BUILD_DIR
#  --with-libtiff=$BUILD_DIR --with-geos=$BUILD_DIR/bin/geos-config

#		$(CONFIGURE) --prefix=$BUILD_DIR --build=$(HOST) --enable-static=no --enable-shared=yes --without-python --without-libtool --with-libz=$(DEP_INSTALL_DIR) --with-libtiff=$(DEP_INSTALL_DIR) --with-png=$(DEP_INSTALL_DIR) --with-jpeg=$(DEP_INSTALL_DIR) && \


#  --enable-static=no --enable-shared=yes


#./configure --prefix=$WORKSPACE/dist --enable-shared=yes --enable-static=yes --with-python=no --with-xerces=no --with-expat=$WORKSPACE/dist --with-expat-lib=-L$WORKSPACE/dist/lib --with-curl=no --with-sqlite3=$WORKSPACE/dist --with-odbc=no --with-mysql=no --with-oci=no --with-pg=$WORKSPACE/dist/bin/pg_config --with-geos=no --with-libz=internal --with-png=internal --with-libtiff=internal --with-geotiff=internal --with-jpeg=internal --with-gif=internal --with-jasper=no --with-mrsid=no --with-mrsid_lidar=no --with-ecw=no --with-pcraster=internal --with-xml2=no --with-threads=yes --host=x86_64-w64-mingw32 --build=x86_64-w64-mingw32 --without-libtool --enable-fast-install  
#./configure --prefix=$WORKSPACE/dist --enable-shared=yes --enable-static=no --with-python=no --with-xerces=no --with-expat=$WORKSPACE/dist --with-curl=no --with-sqlite3=$WORKSPACE/dist --with-odbc=no --with-mysql=no --with-oci=no --with-pg=$WORKSPACE/dist/bin/pg_config --with-geos=yes --with-libz=internal --with-png=internal --with-libtiff=internal --with-geotiff=internal --with-jpeg=internal --with-gif=internal --with-jasper=no --with-mrsid=no --with-mrsid_lidar=no --with-ecw=no --with-pcraster=internal --with-threads=yes --host=$HOST --build=$HOST --without-libtool --enable-fast-install --with-expat-lib=-L$WORKSPACE/dist/lib

fi

################################################################################
