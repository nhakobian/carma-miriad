/*
 *  test out a bunch of things the C version of mirlib does
 *
 *  gcc -g -Imirlib -o testmirlib testmirlib.c mirlib/libmir.a -lm
 * or:
 *  gcc -g -I$MIRINC -I$MIRSUBS -o testmirlib testmirlib.c -L$MIRLIB -lmir -lm
 *  [except it seems to need pgplot now, and it should not !!! ]
 *  gcc -g -I$MIRINC -I$MIRSUBS -o testmirlib testmirlib.c $MIRLIB/libmir.a -lm
 *  [prevents the  pgplot problem]
 *
 *  Note: for LFS you also need to add extra compile flags:
 *     set lfs=(-D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE)
 *     gcc -g $lfs -I$MIRINC -I$MIRSUBS -o testmirlib testmirlib.c $MIRLIB/libmir.a -lm
 *
 * History:
 *
 *      
 *      aug-2006:     some changes for MIR5
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "maxdimc.h"
#include "miriad.h"

#define check(iostat) if(iostat)bugno_c('f',iostat)


void test_hio(char *name1)
{
  int t1, i1,iostat;
  double pi = 3.14159265358979323846;
  float g = 9.8;
  int d = 28;
  int8 d8 = 28;

  fprintf(stderr,"test_hio: %s\n",name1);

  hopen_c(&t1, name1, "old", &iostat);
  if (iostat==0) {
    fprintf(stderr,"Deleting previous dataset %s\n",name1);
    hrm_c(t1);
  } else
    fprintf(stderr,"Creating new dataset %s\n",name1);

  hopen_c(&t1, name1, "new", &iostat);                check(iostat);

  wrhdd_c(t1,"a_double",pi);
  wrhdr_c(t1,"a_real",g);
  wrhdi_c(t1,"a_int",d);
  wrhdl_c(t1,"a_int8",d8);

  haccess_c(t1,&i1,"a_item","write",&iostat);         check(iostat);
  hdaccess_c(i1,&iostat);                             check(iostat);

#if 0
  hdelete_c(t1,"a",&iostat);                          check(iostat);
#endif
  hclose_c(t1);

}

void test_xyio(char *fname, int nx, int ny, int nz)
{
  int t1, i,j,k,iostat;
  float data[MAXDIM];
  int axes[3];

  fprintf(stderr,"test_xyio: %s nx,ny,nz=%d %d %d\n",fname,nx,ny,nz);

  /* delete old one , if exists */
  hopen_c(&t1, fname, "old", &iostat);
  if (iostat==0) hrm_c(t1);

  axes[0] = nx;
  axes[1] = ny;
  axes[2] = nz;

  xyopen_c(&t1,fname,"new",3,axes);
  for (k=1; k<=nz; k++) {
    xysetpl_c(t1,1,&k);
    for (j=1; j<=ny; j++)
      xywrite_c(t1,j,data);
  }
  xyclose_c(t1);


}

void test_uvio(char *fname, int nc, int nw, int nr)
{
  int t1, i,iostat;
  double preamble[5];
  float data[2*MAXCHAN];
  int  flags[MAXCHAN];

  fprintf(stderr,"test_uvio: %s nc,nw,nr=%d %d %d\n",fname,nc,nw,nr);

  /* delete old one , if exists */
  hopen_c(&t1, fname, "old", &iostat);
  if (iostat==0) hrm_c(t1);

  uvopen_c(&t1, fname, "new");

  for (i=0; i<nr; i++) {

    uvputvr_c(t1,H_BYTE,"abyte",(char *)data, 1);
    uvputvr_c(t1,H_INT2,"aint2",(char *)data, 1);
    uvputvr_c(t1,H_INT, "aint4",(char *)data, 1);
    uvputvr_c(t1,H_REAL,"areal",(char *)data, 1);
    uvputvr_c(t1,H_DBLE,"adble",(char *)data, 1);
    uvputvr_c(t1,H_CMPLX,"acmplx",(char *)data, 1);
    
    uvwrite_c(t1,preamble,data,flags,nc);
    uvwwrite_c(t1,data,flags,nw);
  }
  uvclose_c(t1);
}


void test_sizes(void)
{
  printf("sizeof(short)     = %d\n",sizeof(short));
  printf("sizeof(int)       = %d\n",sizeof(int));
  printf("sizeof(long)      = %d\n",sizeof(long));
  printf("sizeof(long long) = %d\n",sizeof(long long));
  printf("sizeof(size_t)    = %d\n",sizeof(size_t));
  printf("sizeof(off_t)     = %d\n",sizeof(off_t));
  printf("sizeof(void *)    = %d\n",sizeof(void *));
}

int main(int argc, char *argv[])
{
  int n1, n2, n3;
  char *buf;
  fprintf(stderr,"Testing MIRLIB:\n");
  if (argc==1) {
    fprintf(stderr,"Command line options\n");
    fprintf(stderr," h                              hio test on test1.mir \n");
    fprintf(stderr," x [nx ny nz]                   xyio write test on test1.xy\n");
    fprintf(stderr," u [nc nw nr]                   uvio write test on test1.uv\n");
    fprintf(stderr," m                              malloc loop until full memory [sic]\n");
    return 1;
  }

  switch (*argv[1]) {
  case 'h': 
    test_hio("test1.mir");
    break;
  case 'x':
    if (argc>2) {
      n1 = atoi(argv[2]);
      n2 = atoi(argv[3]);
      n3 = atoi(argv[4]);
    } else {
      n1 = n2 = n3 = 64;
    }
    test_xyio("test1.xy", n1,n2,n3);
    break;
  case 'u':
    if (argc>2) {
      n1 = atoi(argv[2]);
      n2 = atoi(argv[3]);
      n3 = atoi(argv[4]);
    } else {
      n1 = 1024;
      n2 = 16;
      n3 = 40000;
    }
    test_uvio("test1.uv", n1,n2,n3);
    break;
  case 'm':
    fprintf(stderr,"Malloc loop until memory full, incrementing by %d ... ",BUFSIZE);  
    sleep(1);
    fprintf(stderr,"... go!\n");
    do {
      buf = malloc(BUFSIZE);
    } while (buf);
    break;
  case 's':
    test_sizes();
    break;
  default:
    break;
  }
  return 0;
}


