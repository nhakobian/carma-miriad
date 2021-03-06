/*****************************************************************************
* 
*			  NCSA HDF version 3.10r2
*				Sept 20, 1990
*
* NCSA HDF Version 3.10r2 source code and documentation are in the public
* domain.  Specifically, we give to the public domain all rights for future
* licensing of the source code, all resale rights, and all publishing rights.
* 
* We ask, but do not require, that the following message be included in all
* derived works:
* 
* Portions developed at the National Center for Supercomputing Applications at
* the University of Illinois at Urbana-Champaign.
* 
* THE UNIVERSITY OF ILLINOIS GIVES NO WARRANTY, EXPRESSED OR IMPLIED, FOR THE
* SOFTWARE AND/OR DOCUMENTATION PROVIDED, INCLUDING, WITHOUT LIMITATION,
* WARRANTY OF MERCHANTABILITY AND WARRANTY OF FITNESS FOR A PARTICULAR PURPOSE
* 
*****************************************************************************/

#ifdef RCSID
static char RcsId[] = "@(#)$Revision$";
#endif
/*
$Header$

$Log$
Revision 1.1  1990/09/28 21:50:12  teuben
Initial revision

 * Revision 3.1  90/07/02  10:11:41  clow
 * some cosmetic modifications
 * 
*/

/*    dfseq
*  Sequencer for NCSA Hierarchical Data Format files
*
*  Display images on the Sun-3 console or on a remote ICR terminal 
*  in sequence, suitable for onscreen animation.  Can expand the image size
*  on the fly. Can be adapted to Sun 2's easily
*
*  This version now supports displaying an image in a window on a Silicon
*  Graphics Iris 4D workstation.
*
*  National Center for Supercomputing Applications
*  University of Illinois, Urbana-Champaign
*
*  by Tim Krauskopf
*  first version of seq 7/11/86
*  modifications        2/20/87
*  re-write             6/21/88
*  Iris 4D support      6/24/89  Mike Krogh
*  Line padding		9/25/89  Chin-Chau Low
*  In-between versions re-written by Gaige B. Paulsen and Swami Natarajan
*
*  This program is in the public domain
*
*/
#include "df.h"             /* HDF includes */

#ifdef SUN
#include <pixrect/pixrect_hs.h>
#include <memory.h>
#define  DEVICE  "/dev/fb"
#define NEEDPAD			/* Needs padding to byte bioundary */
#define BYTEBOUND 4		/* byte boundary of a word on Suns */
				/* should be multiple of  2 on Sun3 */
				/* and 4 on Sun4 */
#endif

#ifdef IRIS4
#include <gl.h>
#include <device.h>
#endif


#ifndef IRIS4
#define  SCRX    1152
#define  SCRY    900
#endif

#ifdef IRIS4
#define  SCRX    1280
#define  SCRY    1024
#endif


#define  LUSAGE   printf("\nUsage: %s [-s] [-l] [-e exp] [-p xwhere ywhere] file1 file2 . . .\n",argv[0])
#define  RUSAGE   printf("\nUsage: %s [-s] [-l] [-e exp] file1 file2 . . .\n",argv[0])
#define USAGE   if (remote) { RUSAGE; } else { LUSAGE; }



int 
    oldcf=0,                /* old value of compression flag */
    oldx=0,oldy=0,          /* old values of xdim and ydim */
    coldx=0,coldy=0,        /* old values of xdim and ydim for CI8s */
    xdim=0,ydim=0,          /* size of image on disk */
    xwhere=0,ywhere=0,      /* where to put it on the screen */
    step=0,                 /* single step? */
    remote=0,               /* should use ICR for remote display */
    large=0,                /* should make images as large as possible */
    center=1,               /* should center the images */
    startpic=1,             /* for parameter counts */
    oldxs=0,oldys=0,        /* old sizes */
    xsize=0,ysize=0,        /* what final size on screen, after blow-up */
    xfact=1,yfact=1;        /* what factor for blowing up the picture */

#ifdef NEEDPAD
int xpad;
#endif

#ifdef SUN
struct pixrect *screen,*img=NULL;
struct mpr_data *stuff;
#endif

#ifdef IRIS4
int GID;
short *img=NULL;
int WINDOW_OPEN=0;
int idev,qvalue;
int attached=1;
#endif


char 
    rgb[768],                   /* storage for a palette */
    *wherebig=NULL,             /* where to store small image */
    *wheresmall=NULL;           /* where to store image-related stuff */

/*************************************************************************/
/*  dfseq
*   Load the images from the specified files.
*   Create the internal storage if needed.
*
*   Search pattern for files:
*       RIG images in file
*       RI8 and CI8 images in file
*/
main(argc,argv)
    int argc;
    char *argv[];
    {
    int i,filearg;

/*
*  Check to see if we are displaying on a local console or on a
*  remote ICR.  This program does double duty.
*/
    i = strlen(argv[0]);
    if (strncmp("hdfseq",argv[0]+i-6,6))
        remote=1;

#ifdef SUN
/*
*  Check to see if we have the display console and open it.
*/
    if (!remote)
        screen = pr_open(DEVICE);
#endif
 


/*
*  Are there enough parameters?  Give user information on calling.
*/
    if (argc < 2) {
        printf("%s,  version: 1.0   date: December 1, 1988\n",argv[0]);
                if (remote) {
        printf(" This utility displays sequences of raster-8 images\n");
                printf(" from HDF files on a remote device using NCSA's\n");
        printf(" Interactive Color Raster protocol.\n\n");
                }
                if (!remote) {
#ifdef SUN
                printf(" This utility displays sequences of raster-8 images\n");
                printf(" from HDF files on the Sun console running Sunview.\n");
#endif
#ifdef IRIS4
                printf(" This utility displays sequences of raster-8 images\n");
                printf(" from HDF files on the Iris console running 4Sight.\n");
#endif
        printf("\n");
                }
        USAGE;
        puts(" -l              make image as large as possible");
        puts(" -e expansion    expand image by a certain factor");
        if (!remote) puts(" -p xloc yloc    position on screen");
        puts(" -s              step through images");
        exit(1);
    }

/*
*  work on parms
*/
    for (i=1; i<argc; i++) {                /* look at each parm */
        if (*argv[i] == '-') 
            switch ( *(argv[i]+1)) {
                case 'p':                   /* special position on screen */
                    center=0;               /* don't center */
                    xwhere = atoi(argv[++i]);
                    ywhere = atoi(argv[++i]);
                    if (xwhere > SCRX || ywhere > SCRY) {
                        puts("\n Invalid position ");
                        USAGE;
                        exit(1);
                    }
                    startpic+=3;
                    break;
                case 'e':
                    xfact = yfact = atoi(argv[++i]);
                    if (xfact < 1) {
                        xfact = 1;
                        yfact = 1;
                    }
                    startpic+=2;
                    break;
                case 'l':                   /* large pic */
                    large=1;
                    startpic++;
                    break;
                case 's':                   /* step through pics */
                    step = 1;
                    startpic++;
                    break;
                default:
                    USAGE;
                    exit(1);
            }
    }


/*
*  Process each of the files in the command string.
*
*/
    for (filearg = startpic; filearg < argc; filearg++)
        showpic(argv[filearg]);


#ifdef SUN
    if (!remote)
        pr_close(screen);
#endif
#ifdef IRIS4
    if ((!remote) && (img)) /* make sure we have an image */
        while(1) {
            idev=qread(&qvalue);
            if (idev==REDRAW) {
                reshapeviewport();
                if (img != NULL) {
                    rectwrite(0,0,(xdim-1),(ydim-1),img);
                    swapbuffers();
                }
            }
        }
#endif

    exit(0);
}

/*************************************************************************/
/*  getspace
*  Provide enough space in the space pointers for the operations
*  to all work.
*/
getspace()
    {

/*  
*  Don't allocate anything if the image is the same size as before.
*/
    if (oldx != xdim || oldy != ydim) {
        oldx = xdim; oldy = ydim; 

        if (wheresmall)
            free(wheresmall);

        if (NULL == (wheresmall = malloc(xdim*ydim))) {
            puts(" Cannot allocate memory, fatal error");
            exit(1);
        }

    }
    return(0);
}


#ifdef SUN
/*
*  Allocate the space for the pixrect if displaying locally
*/
getpix()
    {
/*
*  If local expansion is desired, allocate the space for it in the
*  pixrect.  Allocation will take place because xsize and ysize will
*  be set before calling this routine.
*
*  only re-allocate if the image changes size.
*/
    if (!remote && (oldxs != xsize || oldys != ysize)) {
        oldxs = xsize ; oldys = ysize;
        if (img)
            pr_destroy(img);

        img = mem_create(xsize,ysize,8);        /* to size of image */
#ifndef NEEDPAD
        if (xfact > 1 || yfact > 1) {
#endif
#ifdef NEEDPAD
	if (xfact > 1 || yfact > 1 || xpad) {
#endif
            stuff = mpr_d(img);
            wherebig = (char *)stuff->md_image;     /* pointer inside pixrect */
        }
        else {
            stuff = mpr_d(img);
            wheresmall = (char *)stuff->md_image;
        }
    }
    return(0);
}
#endif


#ifdef IRIS4
/*
*  Allocate the space for the image if displaying locally
*/
getpix()
{
/*
*  Allocation will take place because xsize and ysize will
*  be set before calling this routine.
*
*/
    if (!remote && (oldxs != xsize || oldys != ysize)) {
        oldxs = xsize ; oldys = ysize;
        if (img)
            free(img);
        if ((img = (short *)malloc(xdim*ydim*sizeof(short))) == NULL) {
            puts(" Cannot allocate memory, fatal error");
            exit(1);
        } 

    }
    return(0);
}
#endif



/*************************************************************************/
/*  largeset
*  Set up the xfact, yfact, xsize and ysize for expanding the image
*  locally.  
*
*/
largeset()
    {

    if (large) {
        xfact = SCRX/xdim;      /* how much blow-up can we do? */
        yfact = SCRY/ydim;      /* calculate expansion factor  */
        if (xfact > yfact)
            xfact = yfact;
        else
            yfact = xfact;
    }

    xsize = xfact*xdim;         /* re-calculate actual pixel dimensions */
    ysize = yfact*ydim;

#ifdef NEEDPAD			/* add padding for byte boundary */
    xpad = BYTEBOUND - xsize % BYTEBOUND;
    if (xpad == BYTEBOUND) xpad = 0;
    else xsize += xpad;
#ifdef DEBUG
    printf("xpad %d\n",xpad);
#endif /*DEBUG*/
    return(xfact > 1 || yfact > 1 || xpad > 0);	/* is expansion necessary? */
#endif /*NEEDPAD*/

    return(xfact > 1 || yfact > 1); /* is expansion necessary? */
}


/*************************************************************************/
/*  showpic
*  For each file, search for the components that we can display.
*  Display them according to the remote flag on or off.
*
*/
showpic(filename)
    char *filename;
    {
    int ispal;


    oldx = xdim; oldy = ydim;       /* save old values */

/*
*  Search for all RIGs in this file.
*  Process each RIG and display.
*/
    while (1) {
        if (DFR8getdims(filename, &xdim, &ydim, &ispal)<0)
            break;      /* all RIGs processed */
#ifdef DEBUG
	printf("xdim %d ydim %d\n",xdim,ydim);
#endif /*DEBUG*/
#ifdef SUN
        if (!remote) {
            if (largeset())         /* set expansion needs */
                getspace();         /* get local space for pre-expansion */
            getpix();               /* allocate pixrect */
        }
#endif
#ifdef IRIS4
        if (!remote) {
            largeset();         /* set expansion needs */
            getspace();         /* get local space for pre-expansion */
            getpix();               /* allocate memory */
        }
#endif

        if (remote)
            getspace();         /* get space for image in mem */
    

/*
*  Try to successfully load the palette and image from the file
*/
        if (!DFR8getimage(filename, wheresmall, xdim, ydim, rgb)) {
            if (remote) 
                rimage(ispal);      /* display remote image with [palette] */
#ifdef SUN
            else
                piximage(ispal);    /* display image on Sun with [palette] */
#endif
#ifdef IRIS4
            else
                piximage(ispal);    /* display image on Iris with [palette] */
#endif
        }
        else puts(" Error loading image");

    }

    return(0);

}



#ifdef IRIS4
/***********************************************************************/
/*  piximage
*  On the Iris console, display the image as the parameters specify.
*  Handles centering (center)
*  Uses xwhere and ywhere, xsize and ysize.
*  Performs expansion if xfact or yfact > 1
*  Takes the palette from the rgb[] if asked to.
*  Will pause if step=1.
*/
piximage(usepal)
int usepal;
{
    char *pp;
    int r,g,b;
    int j,j1,j2,k;


/*
*  compute centering values, based on new size
*/
    if (center) {
        xwhere = (SCRX-xsize)/2;
        ywhere = (SCRY-ysize)/2;
        if (xwhere < 0) 
            xwhere = 0;
        if (ywhere < 0)
            ywhere = 0;
    }

    if (!WINDOW_OPEN) {
        WINDOW_OPEN=1;
        if (step) {
            printf("Press <enter> to step through images or");
            printf(" 'Q' to quit.\n");
        }
        prefposition(xwhere,(xwhere+xsize),ywhere,(ywhere+ysize));
        GID = winopen("hdfseq");  /* open the window */
        shademodel(FLAT);             /* don't worry about shading */
        doublebuffer();
        multimap();
        gconfig();
        setmap(4);
        color(BLACK);
        clear();
        qdevice(REDRAW);
        if (step) {
            qdevice(QKEY);
            qdevice(RETKEY);
            qdevice(INPUTCHANGE);
        }
    }

/*
* Process window movement events.
*/
    while (qtest()) {
        idev=qread(&qvalue);
        if (idev==REDRAW) {
            reshapeviewport();
            if (img != NULL) {
                rectwrite(0,0,(xdim-1),(ydim-1),img);
                swapbuffers();
            }
        }
        if (idev==INPUTCHANGE)
            attached=qvalue;
    }

/*
*  Do the image expansion, if called for.
*/
    if (xfact > 1 || yfact > 1)
        rectzoom((float)xfact,(float)yfact);  /* let the iris scale it */

/*
*  Set the display palette to the new palette.
*/

    if (usepal) {
        pp = rgb;
        for (j=0; j<256; j++) {
            r = (int)(*pp++);
            g = (int)(*pp++);
            b = (int)(*pp++);
            mapcolor(j,r,g,b);  /* change the system palette */
        }
    }

/*
* Convert image data to iris pixel format
* flip it upside down and convert to short integers.
*/

    for (j=0; j<ydim; j++) {
        j1 = (ydim-j-1)*xdim;
        j2 = j*xdim;
        for (k=0; k<xdim; k++)
            *(img+j1+k) = (short)(*(wheresmall+j2+k));
    }

    rectwrite(0,0,(xdim-1),(ydim-1),img);
    swapbuffers();

    if (step) {
        while(1) {
            idev=qread(&qvalue);
            if (idev==REDRAW) {
                reshapeviewport();
                if (img != NULL) {
                    rectwrite(0,0,(xdim-1),(ydim-1),img);
                    swapbuffers();
                }
            }
            if (idev==QKEY)
                exit(0);
            if (idev==RETKEY)
                break;
            if (idev==INPUTCHANGE)
                attached=qvalue;
        }
    }

}
#endif



#ifdef SUN
/***********************************************************************/
/*  piximage
*  On the Sun console, display the image as the parameters specify.
*  Handles centering (center)
*  Uses xwhere and ywhere, xsize and ysize.
*  Performs expansion if xfact or yfact > 1
*  Takes the palette from the rgb[] if asked to.
*  Will pause if step=1.
*/
piximage(usepal)
    int usepal;
    {
    char r[256],g[256],b[256],*pp;
    int j;

/*
*  compute centering values, based on new size
*/
    if (center) {
        xwhere = (SCRX-xsize)/2;
        ywhere = (SCRY-ysize)/2;
        if (xwhere < 0) 
            xwhere = 0;
        if (ywhere < 0)
            ywhere = 0;
    }


/*
*  Do the image expansion, if called for.
*  The creative pointering makes sure that wherebig and wheresmall are
*  always set to the correct pointers.  img is always the target pixrect.
*/
#ifndef NEEDPAD
    if (xfact > 1 || yfact > 1)
        bigimg(wherebig,wheresmall);
#endif
#ifdef NEEDPAD
    if (xfact > 1 || yfact > 1 || xpad)	/* also do the padding */
	bigimg(wherebig, wheresmall);
#endif

/*
*  Set the display palette to the new palette.
*/
    if (usepal) {
        pp = rgb;                   /* array of rgbrgbrgbrgb */
        for (j=0; j<256; j++) {
                r[j] = *pp++;
                g[j] = *pp++;
                b[j] = *pp++;
        }

        pr_putcolormap(screen, 2, 254, &r[2], &g[2], &b[2]);
    }

/*
*  Display the image using pixrects
*/
    pr_rop(screen,xwhere,ywhere,xsize,ysize,PIX_SRC,img,0,0);

    if (step) {
        printf("Press return to continue, 'q' return to quit");
        if ('q' == getchar())
            exit(0);
    }


}

#endif

/*****************************************************************************/
/*  rimage
*  Remote display of the image using the ICR.
*  Just print the codes to stdout using the protocol.
*/
rimage(usepal)
    int usepal;
    {
    int i,j,newxsize;
    char *space,*thisline,*thischar;
    register unsigned char c;

/*
*  Open the window with the W command
*/

(void)printf("\033^W;%d;%d;%d;%d;0;rseq^",xwhere,ywhere,xdim*xfact,ydim*yfact);

/*
*  If a palette should be used, send it with the M command.
*/
    if (usepal) {
        (void)printf("\033^M;0;256;768;rseq^");  /* start map */

        thischar = rgb;
        for (j=0; j<768; j++) {
            c = *thischar++;
            if (c > 31 && c < 123) {
                putchar(c);
            }
            else {
                putchar((c>>6)+123);
                putchar((c & 0x3f) + 32);
            }
        }
    }

/*
*  Send the data for the image with RLE encoding for efficiency.
*  Encode each line and send it.
*/
    space = malloc(ydim+100);
    thisline = wheresmall;

    for (i = 0; i < ydim; i++) {
        newxsize = rleit(thisline,space,xdim);
        thisline += xdim;               /* increment to next line */

        (void)printf("\033^R;0;%d;%d;%d;rseq^",i*xfact,xfact,newxsize); 

        thischar = space;
        for (j = 0; j < newxsize; j++) {

/***********************************************************************/
/*  Encoding of bytes:
*
*  123 precedes #'s 0-63
*  124 precedes #'s 64-127
*  125 precedes #'s 128-191
*  126 precedes #'s 192-255
*  overall:  realchar = (specialchar - 123)*64 + (char-32)
*            specialchar = r div 64 + 123
*            char = r mod 64 + 32
*/
/***********************************************************************/

            c = *thischar++;      /* get byte to send */

            if (c > 31 && c < 123) {
                putchar(c);
            }
            else {
                putchar((c>>6)+123);
                putchar((c & 0x3f) + 32);
            }
        }
    }

/*
*  pause for the user
*/
    if (step) {
        printf("Press return to continue, 'q' return to quit");
        if ('q' == getchar())
            exit(0);
    }

    free(space);
    return(0);
}


#ifdef SUN
/*****************************************************************************/
/* expandimg
* copy an image memory to memory, expanding byte by byte to get a larger image.
*  no aliasing, just byte replication
*/
bigimg(targ,src)
    unsigned char *targ,*src;
    {
    register i,j,line;
    register unsigned char *p,*q,*oldq;

    p = src;
    q = targ;
    for (line = 0; line < ydim; line++) {
/*        p = src+line*xdim;*/
/*        oldq = q = targ+line*xsize*yfact;*/
	oldq = q;

        for (i=0; i<xdim; i++,p++)
            for (j=0; j<xfact; j++)
                *q++ = *p;

#ifdef NEEDPAD
	for (i = 0; i < xpad; i++)
	    *q++ = 0;		/* pad the line */
#endif /*NEEDPAD*/

        for (i=1; i<yfact ; i++) {
            DFmovmem(oldq,q,xsize);             /* make one copy of the line */
            q += xsize;
        }

    }
    return(0);
}
#endif



/********************************************************************/
/*  rleit
*  compress the data to go out with a simple run-length encoded scheme.
*
*/
int rleit(buf,bufto,len)
    int len;
    char *buf,*bufto;
    {
    register char *p,*q,*cfoll,*clead;
    char *begp;
    int i;

    p = buf;
    cfoll = bufto;              /* place to copy to */
    clead = cfoll + 1;
    
    begp = p;
    while (len > 0) {           /* encode stuff until gone */
        
        q = p + 1;
        i = len-1;
        while (*p == *q && i+120 > len && i) {
            q++;
            i--;
        }
        
        if (q > p + 2) {        /* three in a row */
            if (p > begp) {
                *cfoll = p - begp;
                cfoll = clead;
            }
            *cfoll++ = 128 | (q-p);     /* len of seq */
            *cfoll++ = *p;      /* char of seq */
            len -= q-p;         /* subtract len of seq */
            p = q;
            clead = cfoll+1;
            begp = p;
        }
        else {
            *clead++ = *p++;    /* copy one char */
            len--;
            if (p > begp + 120) {
                *cfoll = p - begp;
                cfoll = clead++;
                begp = p;
            }
        }
        
    }
/*
*  fill in last bytecount
*/
    if (p > begp) 
        *cfoll = (p - begp);
    else
        clead--;                    /* don't need count position */
    
    return((int)(clead - bufto));   /* how many stored as encoded */
}

