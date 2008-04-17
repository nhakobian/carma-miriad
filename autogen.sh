#!/bin/bash

if test "x$LIBTOOLIZE" = x; then
  export LIBTOOLIZE=libtoolize
  # Determine if this is Mac OS X and react appropriately...
  if test -x /usr/bin/glibtoolize; then
    export LIBTOOLIZE=glibtoolize
  fi 
fi

echo "Initializing build scripts ..."
aclocal
autoheader
$LIBTOOLIZE --force --copy
automake --force --copy --add-missing --warnings=obsolete  --warnings=gnu --warnings=unsupported --warnings=no-syntax --warnings=no-portability
autoconf

(cd borrow/pgplot && ./autogen.sh)

echo "Now run 'configure' to generate a Makefile."
