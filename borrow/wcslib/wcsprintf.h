/*============================================================================

  WCSLIB 4.7 - an implementation of the FITS WCS standard.
  Copyright (C) 1995-2011, Mark Calabretta

  This file is part of WCSLIB.

  WCSLIB is free software: you can redistribute it and/or modify it under the
  terms of the GNU Lesser General Public License as published by the Free
  Software Foundation, either version 3 of the License, or (at your option)
  any later version.

  WCSLIB is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
  FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
  more details.

  You should have received a copy of the GNU Lesser General Public License
  along with WCSLIB.  If not, see <http://www.gnu.org/licenses/>.

  Correspondence concerning WCSLIB may be directed to:
    Internet email: mcalabre@atnf.csiro.au
    Postal address: Dr. Mark Calabretta
                    Australia Telescope National Facility, CSIRO
                    PO Box 76
                    Epping NSW 1710
                    AUSTRALIA

  Author: Mark Calabretta, Australia Telescope National Facility
  http://www.atnf.csiro.au/~mcalabre/index.html
  $Id$
*=============================================================================
*
* WCSLIB 4.7 - C routines that implement the FITS World Coordinate System
* (WCS) standard.
*
* Summary of the wcsprintf routines
* ------------------------------
* These routines allow diagnostic output from celprt(), linprt(), prjprt(),
* spcprt(), tabprt(), and wcsprt() to be redirected to a file or captured in
* a string buffer.  These routines use wcsprintf() for output.
*
*
* wcsprintf() - Print function used by WCSLIB diagnostic routines
* ---------------------------------------------------------------
* wcsprintf() is used by the celprt(), linprt(), prjprt(), spcprt(), tabprt(),
* and wcsprt() routines.  Its output may be redirected to a file or string
* buffer via wcsprintf_set().
*
* Given:
*   format    char*     Format string, passed to one of the printf(3) family
*                       of stdio library functions.
*   ...       mixed     Argument list matching format, as per printf(3).
*
* Function return value:
*             int       Number of bytes written.
*
*
* wcsprintf_set() - Set output disposition for wcsprintf()
* --------------------------------------------------------
* wcsprintf_set() sets the output disposition for wcsprintf() which is used by
* the celprt(), linprt(), prjprt(), spcprt(), tabprt(), and wcsprt() routines.
*
* Given:
*   wcsout    FILE*     Pointer to an output stream that has been opened for
*                       writing, e.g. by the fopen() stdio library function,
*                       or one of the predefined stdio output streams - stdout
*                       and stderr.  If zero (NULL), output is written to an
*                       internally-allocated string buffer, the address of
*                       which may be obtained by wcsprintf_buf().
*
* Function return value:
*             int       Status return value:
*                         0: Success.
*
*
* wcsprintf_buf() - Get the address of the internal string buffer
* ---------------------------------------------------------------
* wcsprintf_buf() returns the address of the internal string buffer created
* when wcsprintf_set() is invoked with its FILE* argument set to zero.
*
* Function return value:
*             const char *
*                       Address of the internal string buffer.  The user may
*                       free this buffer by calling wcsprintf_set() with a
*                       valid FILE*, e.g. stdout.  The free() stdlib library
*                       function must NOT be invoked on this const pointer.
*
*===========================================================================*/

#ifndef WCSLIB_WCSPRINTF
#define WCSLIB_WCSPRINTF

#ifdef __cplusplus
extern "C" {
#endif

int wcsprintf_set(FILE *wcsout);
int wcsprintf(const char *format, ...);
const char *wcsprintf_buf(void);

#ifdef __cplusplus
}
#endif

#endif /* WCSLIB_WCSPRINTF */
