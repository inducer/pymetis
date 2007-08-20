#! /bin/sh

set -e

if test "$1" = ""; then
  echo "usage: $0 path-to-metis"
  exit 1
fi

rm src/gklib/*.{c,h}
cp $1/GKlib/trunk/*.{c,h} src/gklib/
rm src/metis/*.{c,h}
cp $1/include/*.h src/metis/
cp $1/libmetis/*.{c,h} src/metis/
rm src/metis/kfmetis.c

patch -p0 < metis.patch
