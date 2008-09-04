/*
 * HKUVLIST.C: hack miriad UV listing
 #
 * gcc -g -I$MIRINC -I$MIRSUBS -o hkuvplt hkuvplt.c $MIRLIB/libmir.a -lm libZeno.a -L/usr/local/pgplot -lcpgplot -lpgplot -lg2c -lX11 -lpng
 */

#include "maxdimc.h"
#include "miriad.h"
#include "stdinc.h"
#include "getparam.h"
#include "cpgplot.h"
#include "hkuvplt.h" 

#include <math.h>
#include <string.h>

/*
 * Default values for input parameters.
 */

string defv[] = {               ";HKUVPLT: hkuvplt test ",
    "vis=xxx.mir",              ";Input uv data",
    "mode=info",                ";Output mode: info, amp, uvdistance",
    "format=%g",                ";Output format used",
    "VERSION=1.0",              ";Peter Teuben   Sep 2008",
    NULL,
};

/*
 * Prototypes and variables for local procedures
 */

local void loaddefv(void);       /* load input params from command line */
local void loaddata(void);       /* load miriad data to hkmiriad structures */
local void listdata(void);       /* list data baseline based - very wide - */

local void showinfo(void);       /* show track information */

local void linkbrec(void);       /* link baselines and records */

local int *sepitems(string,int *,int *,char *);
                                 /* get items in line from terminal */
local int *listbase(char,int *,int, int *);/* list baseline numbers */
local void selsour(void);        /* select sources */
local int *sepsour(string,int *,char *); /* separate sources in line */
local char **splitline(string,char,int *); /* split line to items */
local int getantid(int);         /* get antenna id */
local int getbaseid(int);        /* get baseline id */


int mode = -1;
string amode;
string  fmt;
int nbad = 0;

/*
 * MAIN: toplevel routine
 */

int main(int argc, string argv[])
{
    int nit,*it,nb,*bl,i;
    char line[256],type,dum;
    float x1,y1,x2,y2;

    recordptr r;
    

    initparam(argv, defv);                       /* initialize param access  */
    headline = defv[0] + 1;                      /* headline                 */
    loaddefv();                                  /* parm from command line   */
    loaddata();                                  /* load data in hk struct.  */

    if (mode<0) {
      showinfo();                                  /* show track information   */
      return;
    }

    linkbrec();                                  /* link baseline and record */
    listdata();                                  /* list data */

    nit = 0; it = (int *) NULL;                  /* init. items              */
    sprintf(line,"t,%d",tra.ant[0]);             /* pick first antenna       */
    it = sepitems(line,it,&nit,&type);           /* separate items           */
    bl = listbase(type,it,nit,&nb);              /* itialize lists of bl,    */


    free(it);
    free(bl);
    free(pan);
}

/*
 * SELSOUR: select sources (mask others)
 */

local void selsour(void)
{
    int nm,*ma,i,k;
    char code,line[256];
    recordptr r;

    printf("\tSources:\n");
    for (i=0;i<tra.nsour;i++)
        printf("\t\t%2d: %s\n",i,tra.sour[i]);
    printf("\n");
    printf("Select sources: [A]ll/[S]elect/[M]ask/[U]nmask (e.g. \"a\",\"s,1,2\",\"m,2\",\"u,1,3\") =\n");
    scanf("%s",line);

    ma  = sepsour(line,&nm,&code);       /* separate items           */
    if (code == 'a') {
        for (r=rec; r<rec+tra.nrec; r++)
            (*r).flag &= ~MASK;          /* set maskbit to unmask    */
    } else if (code == 's') {
        for (r=rec; r<rec+tra.nrec; r++) {
            k = 0;
            for (i=0; i<nm; i++)
                if ((*r).souid == ma[i]) k++;
            if (k>0) {
                (*r).flag &= ~MASK;      /* set maskbit to unmask    */
            } else {
                (*r).flag |= MASK;       /* set maskbit to mask      */
            }
        }
    } else if (code == 'm') {
        for (r=rec; r<rec+tra.nrec; r++) {
            for (i=0; i<nm; i++) {
                if ((*r).souid == ma[i])
                    (*r).flag |= MASK;   /* set maskbit to mask      */
            }
        }
    } else if (code == 'u') {
        for (r=rec; r<rec+tra.nrec; r++) {
            for (i=0; i<nm; i++) {
                if ((*r).souid == ma[i])
                    (*r).flag &= ~MASK;  /* set maskbit to unmask    */
            }
        }
    }
    if (ma != (int *) NULL) free(ma);
}


/*
 * SEPSOUR: separate sources from line, check their presenses
 */

local int *sepsour(string line,int *nitem, char *type)
{
    int nelem,nit,*it;
    char **elem,**a;
    int i,j,k,n,b,a1,a2,s;

    elem = splitline(line,',',&nelem);          /* split line               */

    if (line[0] == 'a') {                       /* selected all             */

        *type = 'a';
        nit = 0;
        it = (int *) NULL;

    } else if (line[0] == 's') {

        *type = 's';
        nit = 0;
        for (i=1; i<nelem; i++) {
            s = atoi(elem[i]);                  /* count num of ant in line */
            if (s >= 0 && s < tra.nsour) nit++; /* if sour exists, count it */
        }
        it = (int *)allocate(nit * sizeof(int));/* alloc memory             */
        k = 0;
        for (i=1; i<nelem; i++) {
            s = atoi(elem[i]);                  /* this source              */
            if (s >= 0 && s < tra.nsour) { /* if exists in this track  */
                it[k] = s;
                k++;
            }
        }
    } else if (line[0] == 'm') {

        *type = 'm';
        nit = 0;
        for (i=1; i<nelem; i++) {
            s = atoi(elem[i]);                  /* count num of ant in line */
            if (s >= 0 && s < tra.nsour) nit++; /* if sour exists, count it */
        }
        it = (int *)allocate(nit * sizeof(int));/* alloc memory             */
        k = 0;
        for (i=1; i<nelem; i++) {
            s = atoi(elem[i]);                  /* this source              */
            if (s >= 0 && s < tra.nsour) { /* if exists in this track  */
                it[k] = s;
                k++;
            }
        }
    } else if (line[0] == 'u') {

        *type = 'u';
        nit = 0;
        for (i=1; i<nelem; i++) {
            s = atoi(elem[i]);                  /* count num of ant in line */
            if (s >= 0 && s < tra.nsour) nit++; /* if sour exists, count it */
        }
        it = (int *)allocate(nit * sizeof(int));/* alloc memory             */
        k = 0;
        for (i=1; i<nelem; i++) {
            s = atoi(elem[i]);                  /* this source              */
            if (s >= 0 && s < tra.nsour) { /* if exists in this track  */
                it[k] = s;
                k++;
            }
        }

    } else {
        it = (int *) NULL;
    } 

    free(elem);
    *nitem = nit;

    return(it);
}

/*
 * LOADDEFV: load variables defined in defv or command line
 */

local void loaddefv(void)
{
  vis = getparam("vis");                       /* get file names          */
  files = splitline(vis,',',&nfiles);          /* split file names        */
  amode = getparam("mode");

  if (*amode == 'a') 
    mode = 0;
  else if (*amode == 'u')
    mode = 1;
  else if (*amode == 'i')
    mode = -1;
  else {
    /* should give warning message */
    mode = 0;
  }
  fmt = getparam("format");
    
}


/*
 * LOADDATA: load data from miriad dataset to hkmiriad structures.
 */

local void loaddata(void)
{

#define MAXNAME 9

    int unit;
    int flg[MAXCHAN];
    float data[2*MAXCHAN];
    double pre[5],re,im,tmin;
    int nr,nc,ns,nb,na,b,bl[MAXBASE],a1,a2,an[MAXANT],nbytes;
    char s[MAXNAME],sour[MAXSOU][MAXNAME];
    recordptr r;
    int f,i,j,k,n,c;

    nr = 0;                                      /* initialize record, chan */
    nc = 0;                                      /* source, and bl counters */
    ns = 0;
    nb = 0;
    nbad = 0;
    tmin = 1.0e30;

    for (f=0; f<nfiles; f++) {                   /* read over all files     */
        uvopen_c(&unit,files[f],"old");          /* open miriad data        */
        n = 1;                                   /* initialize for loop     */
        while (1) {                              /* loop over records       */
            uvread_c(unit,pre,data,flg,MAXCHAN,&n); /* read data            */
	    if (n==0) break;
            c = 0;
            for (i=0; i<n; i++)
                if (flg[i] == 1) c++;            /* if not flagged,count it */
	    if (c==0) nbad++;

	    if (1) {
	        nr++;                            /* update record counter   */
                nc = MAX(nc,n);                  /* find max for chan count */
                tmin = MIN(tmin,pre[2]);         /* min time                */
                b = (int) pre[3];                /* get baseline number     */
                i = 0;                           /* search through bl list  */
                while (i<nb && bl[i] != b) i++;
                if (i == nb) {                   /* if not in list, add it  */
                    bl[nb] = b;
                    nb++;
                }
                uvgetvra_c(unit,"source",s,sizeof(s)); /* get source name   */
                i = 0;                           /* search thrgh sour list  */
                while (i<ns && !streq(sour[i],s)) i++;
                if (i == ns) {                   /* if not in list, add it  */
                    strcpy(sour[ns],s);
                    ns++;
                }
	    }
        }
        uvclose_c(unit);                         /* close miriad data       */
    } /* loop all files */
    na = 0;                                      /* make ant list from bl   */
    for (i=0; i<nb; i++) {                       /* loop over baselines     */
        a1 = bl[i] / 256;                        /* antennas in this bl     */
        a2 = bl[i] - 256*a1;
        k = 0;                                   /* search through ant list */
        while (k<na && an[k] != a1) k++;         /* if not in list yet,     */
        if (k == na) {                           /* add it.                 */
            an[na] = a1;
            na++;
        }
        k = 0;
        while (k<na && an[k] != a2) k++;
        if (k == na) {
            an[na] = a2;
            na++;
        }
    }
    nbytes = nr * sizeof(record);
    rec = (recordptr) allocate(nbytes);          /* mem for records     */
    j = 0;
    for (f=0; f<nfiles; f++) {                   /* read over all files     */
        uvopen_c(&unit,files[f],"old");          /* open miriad data        */
        n = 1;                                   /* initialize for loop     */
        while (n > 0) {                          /* loop over records       */
            uvread_c(unit,pre,data,flg,MAXCHAN,&n); /* read data            */
            c = 0;
            re = 0.0;
            im = 0.0;
            for (i=0; i<n; i++)                  /* average over channels   */
                if (flg[i] == 1) {
                    re += data[2*i  ];
                    im += data[2*i+1];
                    c++;
                }
	    r = rec + j;
	    
            if (c > 0) {
	      re /= (double) c;
	      im /= (double) c;
	      (*r).amp = sqrt(re*re + im*im);
	      (*r).pha = atan2(re,im) * 180.0/PI;
	    } else {
	      (*r).amp = -1.0;
	      (*r).pha = 0.0;
	    }
	    if (mode==1) /* uv distance */
	      (*r).amp = sqrt(pre[0]*pre[0]+pre[1]*pre[1]);
	    (*r).ut  = (float) (pre[2] - floor(tmin-0.5) -0.5)*24.0;
                                             /* calc ut from julian date*/
	    (*r).bl  = pre[3];               /* baseline number         */
	    (*r).next= NULL;                 /* init next pointer       */
	    uvgetvra_c(unit,"source",s,sizeof(s)); /* find source id    */
	    k = 0;
	    while (k<ns && !streq(sour[k],s) ) k++;
	    (*r).souid = k;
	    (*r).flag = NONE;                /* initialize flag         */
	    j++;
        }
        uvclose_c(unit);                         /* close miriad data       */
    } /* loop all files */
    
    tra.nrec  = nr;                              /* set par common in track */
    tra.nchan = nc;
    tra.nsour = ns;
    for (i=0; i<ns; i++) strcpy(tra.sour[i],sour[i]);
    tra.nbase = nb;
    for (i=0; i<nb; i++) tra.base[i] = bl[i];
    tra.nant = na;
    for (i=0; i<na;  i++) tra.ant[i] = an[i];

}

local void listdata(void) 
{
  int bl,a1,a2,i,j,nb,na,nr;
  recordptr r,bp[MAXANT][MAXANT];
    

  nb = tra.nbase;
  na = tra.nant;
  nr = tra.nrec;
  if (nr % nb) {
    fprintf(stderr,"Cannot handle a partially filled dataset yet: %d/%d\n",nr,nb);
    exit(1);
  }

  printf("# mode=%s\n",amode);
  printf("#BL: ");                         /* print baselines in header */
  for (i=0;i<tra.nbase;i++) {
    a1 = tra.base[i] / 256;
    a2 = tra.base[i] - 256*a1;
    printf("%d-%d ",a1,a2);
  }
  printf("\n");

  j = 0;
  for (r=rec; r<rec+nr; r++) {                /* loop over records        */
    bl = (*r).bl;                           /* find baseline number, and*/
    a1 = bl / 256;                          /* antenna numbers          */
    a2 = bl - 256*a1;
    if (j==0) {
      printf(fmt,(*r).ut);
      printf(" ");
    }
    printf(fmt,(*r).amp);
    printf(" ");
    j++;
    if (j==nb) {
      printf("\n");
      j=0;
    }
  }
}

/*
 * SHOWINFO: show track information
 */

local void showinfo(void)
{
    int i,a1,a2;
    char line[256];

    printf("\n\tNum of antennas    = %d",tra.nant);
    for (i=0;i<tra.nant;i++) {
        if (i % 15 == 0) printf("\n\t\t");
        printf("%d ",tra.ant[i]);
    }
    printf("\n");

    printf("\tNum of baselines   = %d",tra.nbase);
    for (i=0;i<tra.nbase;i++) {
        if (i % 8 == 0) printf("\n\t\t");
        a1 = tra.base[i] / 256;
        a2 = tra.base[i] - 256*a1;
        printf("%2d-%2d ",a1,a2);
    }
    printf("\n");

    printf("\tNum of sources     = %d\n\t\t",tra.nsour);
    for (i=0;i<tra.nsour;i++)
        printf("%s ",tra.sour[i]);
    printf("\n");

    printf("\tNum of records     = %d\n",tra.nrec);
    printf("\tNum of bad records = %d\n",nbad);
    printf("\tNum of times       = %d\n",tra.nrec/tra.nbase);

    printf("\n");

}

/*
 * SEPITEMS: separate items, i.e. ant/bass, from line, check their presenses
 */

local int *sepitems(string line,int *item, int *nitem, char *type)
{
    int nelem,nit,*it;
    char **elem,**a,t;
    int i,j,k,n,b,a1,a2,s;

    nit = *nitem;                               /* initialize param. return */
    it = item;                                  /* prev if invalid input    */
    t = *type;

    elem = splitline(line,',',&nelem);          /* split line               */
    if (line[0] == 't') {                       /* items are telescopes     */
        nit = 0;
        for (i=1; i<nelem; i++) {
            a1 = atoi(elem[i]);                 /* count num of ant in line */
            if (getantid(a1) >= 0) nit++;       /* if a1 exists, count it   */
        }
	if (nit > 0) {
	    t = 't';                            /* set item type            */
	    it = (int *)allocate(nit * sizeof(int)); /* alloc memory        */
	    k = 0;
	    for (i=1; i<nelem; i++) {
	        a1 = atoi(elem[i]);             /* this antenna             */
		if (getantid(a1) >= 0) {        /* if exists in track       */
		    it[k] = a1;
		    k++;
		}
	    }
	    if (item != (int *) NULL) free(item);
	} else {
	    nit = *nitem;
	}
    } else if (line[0] == 'b') {                /* items are baselines      */

        nit = 0;                                /* count num of baselines   */
        for (i=1; i<nelem; i++) {
            a = splitline(elem[i],'-',&n);
            if (n == 2) {
                a1 = atoi(a[0]);
                a2 = atoi(a[1]);
                if (a1 != a2) {
                    if (a1 < a2) b = 256*a1+a2;
                    if (a1 > a2) b = 256*a2+a1;
                    if (getbaseid(b) >= 0) nit++;/* if b exists, count it   */
                }
            }
            free(a);
        }
	if (nit > 0) {
	    t = 'b';
	    it = (int *) allocate(nit * sizeof(int)); /* alloc memory       */
	    k = 0;
	    for (i=1; i<nelem; i++) {
	        a = splitline(elem[i],'-',&n);
		if (n == 2) {
		    a1 = atoi(a[0]);
		    a2 = atoi(a[1]);
		    if (a1 != a2) {
		        if (a1 < a2) b = 256*a1+a2;
			if (a1 > a2) b = 256*a2+a1; /* this baseline        */
			if (getbaseid(b) >= 0) { /* if exist in track       */
			    it[k] = b;           /* add it to list          */
			    k++;
			}
		    }
		}
		free(a);
	    }
	    if (item != (int *) NULL) free(item);
	} else {
	    nit = *nitem;
	}
    } else if (line[0] == 'a') {                /* selected all             */
        t = 'a';
        nit = 0;
        it = (int *) NULL;
	if (item != (int *) NULL) free(item);
    }
    free(elem);

    *nitem = nit;
    *type = t;
    return(it);

}

/*
 * LISTBASE: list baseline numbers that include selected items
 */

local int *listbase(char type, int *a, int na, int *nb)
{
    int i,j,k,a1,a2,b,n,*bl;

    if (type == 't') {                          /* all bl with selected ant */
        n = 0;
        for (i=0; i<na; i++) {                  /* count num of baselines   */
            a1 = a[i];
            for (j=0; j<tra.nant; j++) {
                a2 = tra.ant[j];
                if (a1 != a2) {
                    if (a1 < a2) b = 256*a1+a2;
                    if (a1 > a2) b = 256*a2+a1;
                    if (getbaseid(b) >= 0) n++;
                }
            }
        }
        *nb = n;
        bl = (int *) allocate(*nb * sizeof(int));
        k = 0;
        for (i=0; i<na; i++) {                  /* store base in memory     */
            a1 = a[i];
            for (j=0; j<tra.nant; j++) {
                a2 = tra.ant[j];
                if (a1 != a2) {
                    if (a1 < a2) b = 256*a1+a2;
                    if (a1 > a2) b = 256*a2+a1;
                    if (getbaseid(b) >= 0) {
                        bl[k] = b;
                        k++;
                    }
                }
            }
        }
    } else if (type == 'b') {                   /* list all selected bls    */
        *nb = na;
        bl = (int *) allocate(*nb * sizeof(int));
        for (i=0; i<*nb; i++) bl[i] = a[i];
    } else if (type == 'a') {                   /* list all baselines       */
        *nb = tra.nbase;
        bl = (int *) allocate(*nb * sizeof(int));        
        for (i=0; i<*nb; i++) bl[i] = tra.base[i];
    } else {
      bl = (int *) NULL;
    }
    return(bl);                                 /* return bl list           */
}

/*
 * LINKBREC: link baselines and records
 */

local void linkbrec(void)
{
    int ma,bl,a1,a2,i,j;
    recordptr r,bp[MAXANT][MAXANT];

    ma = 0;
    for (i=0; i<tra.nant; i++) ma = MAX(ma,tra.ant[i]);
                                                /* find maximum ant number  */
    for (i=0;i<ma;i++)
        for (j=0;j<ma;j++) {                    /* initialize bl hashtable  */
            toprec[i][j] = (recordptr) NULL;
            bp[i][j] = toprec[i][j];
        }
        
    for (r=rec; r<rec+tra.nrec; r++) {          /* loop over records        */
        bl = (*r).bl;                           /* find baseline number, and*/
        a1 = bl / 256;                          /* antenna numbers          */
        a2 = bl - 256*a1;
        if (toprec[a1-1][a2-1] == NULL) {       /* first record in this bl, */
            toprec[a1-1][a2-1] = r;             /* link to it from the top  */
        } else {                                /* if not the first         */
            (*bp[a1-1][a2-1]).next = r;         /* link from prev. rec.     */
        }
        bp[a1-1][a2-1] = r;                     /* last record is updated   */
    }
}



/*
 * SPLITLINE: split line with comma separator to items
 */

local char **splitline(string line, char ch, int *nn)
{
    int n,i,ni;
    char *c,*str,*start,**arr;

    if (*line == (char) NULL) return(NULL);     /* return null if nothing   */
    str = strdup(line);                         /* copy line to temp. param */
    ni = 1;                                     /* initialize item counter  */
    for (c=str; *c!= (char) NULL; c++)          /* loop over characters     */
        if (*c == ch) {                         /* found a separator        */
            *c = '\0';                          /* replace it to string end */
            ni++;                               /* increase item counter    */
        }
    arr = (char **) allocate((ni+1) * sizeof(char **));
                                                /* alloc mem for output     */
    start = str;                                /* start from 1st character */
    for (i=0; i<ni; i++) {                      /* loop over items          */
        arr[i] = (char *) strdup(start);        /* copy string to the arary */
        while (*start != (char) NULL) start++;  /* find next string         */
        start++;
    }
    arr[ni] = 0;
    free(str);
    *nn = ni;
    return(arr);
}

/*
 * GETANTID: get antenna id
 */

local int getantid(int an)
{
    int i,k;

    i = -1;
    for (k=0; k<tra.nant; k++)
        if (tra.ant[k] == an) i = k;
    return(i);
}

/*
 * GETBASEID: get baseline id
 */

local int getbaseid(int bl)
{
    int i,k;

    i = -1;
    for (k=0; k<tra.nbase; k++)
        if (tra.base[k] == bl) i = k;
    return(i);
}
