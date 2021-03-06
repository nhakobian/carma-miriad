#
#   example of how to define your own compilers
#   then source this file and use 
#	install.miriad generic=1
#
#   Big caveat:   pgplot doesn't work well this way,
#      especially if compilers are not using just their basename,
#      but derived names such gas gcc-4.2; you will have to hack
#      by symlinking to the appropriate $MIR/borrow/pgplot/sys_NAME/X_Y.conf
#      file.

setenv CC  /usr/local/bin/gcc-4.2
setenv CXX /usr/local/bin/g++
setenv F77 /usr/local/bin/gfortran

