c************************************************************************
	program xpeak
	implicit none
c
c= XPEAK - Calculate peak brightness in a miriad cube
c& sek
c: image analysis
c+
c	XPEAK calculates the peak brightness image for a Miriad cube. 
c	Currently only the peak along the third axis is calculated for 
c	3 dimensional images.
c	To obtain the peaks for other axes use the task REORDER to reorder
c	the axes.
c@ in
c	The input image. No default.
c@ region
c	The region of the input image to be used. See documentation for region
c	on how to specify this. Only the bounding box is supported.
c@ out
c	The peak image. No default.
c@ clip
c	Two values. Exclude pixels with values in the range clip(1) to clip(2).
c	If only one value is given, then exclude -abs(clip) to abs(clip).
c--
c   Other things to be improved in this code:
c	- Really should be included within MOMENT. Also work on axis 1.
c	  Could be tidied up considerably.
c
c	- when items such as cdeltX, crvalX are missing, assume reasonable
c	  defaults. I recently fixed the ctypeX problem.
c	  (X = the moment axis)
c
c  History:
c    26may89 Robert Loushin  original version - v-axis of vxy image only.
c    25jun90 mchw  Major update and rework to allow for other image axes.
c			Fixed a bug for 2nd and higher moments.
c    13nov90 mchw  Added suport for xyv images, and pixel blanking.
c			Made 2nd moment work and removed higher moments.
c    08feb91 mchw  Two values for clip range, and version in history.
c    25feb91 mjs   Changed references of itoa to itoaf.
c    01aug91 rjs   Use boxSet(...'s')
c    05aug91 alr/pjt Changed clip from include to exclude range.
c			Write out dummy 3rd. axis.
c    06aug91 mchw	Merge in above changes.
c    11jul92 pjt   Fixed rather serious flagging bug when axis=1
c    26nov92 nebk  Add btype
c     4jan93 pjt   Fixed rather serious flagging bug when axis=3
c     5feb93 rjs   Use memalloc.
c     5apr93 pjt   Since rjs is walkabouting, I had to fix that amazing bug:
c		   indexor's (i0,j0) in moment3 wrong if blc,trc used
c    04jan96 nebk  Write header descriptors as double precision
c    21may96 sek   Reborn as XPEAK.
c    26nov96 rjs   Increase length of input and output filenames.
c    02jul97 rjs   cellscal change.
c    23jul97 rjs   added pbtype.
c------------------------------------------------------------------------
	include 'maxdim.h'
 	character version*(*)
	parameter(version='version 1.0 26-Nov-96')
	integer maxnax,maxboxes,maxruns,naxis
	parameter(maxnax=3,maxboxes=2048)
	parameter(maxruns=3*maxdim)
	integer boxes(maxboxes)
	integer i,j,lin,lout,nsize(maxnax),blc(maxnax),trc(maxnax)
	integer size(maxnax),axis
	real blo,bhi,clip(2)
	character in*64, out*64
	character line*72
	integer mom
c
c  External
c
	logical keyprsnt
c
c Get inputs.
c
	call output('Xpeak: '//version)
	call keyini
	call keya('in',in,' ')
	call BoxInput('region',in,boxes,maxboxes)
	call keya('out',out,' ')
	mom = 0
c	call keyi('mom',mom,0)
	axis = 3
c	call keyi('axis',axis,3)
	call keyr('clip',clip(1),0.)
	if(keyprsnt('clip'))then
	  call keyr('clip',clip(2),0.)
	else
	  clip(2) = abs(clip(1))
	  clip(1) = -clip(2)
	endif
	call keyfin
c
c Check inputs.
c
	if(in .eq. ' ') call bug('f','No input specified. (in=)')
	if(out .eq. ' ') call bug('f','No output specified. (out=)')
	if(mom.lt.-1 .or. mom.gt.2)
     *	   call bug('f','moment must be between -1 and 2')
	if(clip(2).lt.clip(1)) call bug('f','clip range out of order')
	call xyopen(lin,in,'old',maxnax,nsize)
	call rdhdi(lin,'naxis',naxis,0)
	naxis = min(naxis,maxnax)
	if(nsize(1).gt.maxdim)call bug('f','Input file too big for me')
	if(axis.ne.1.and.axis.ne.3) then
	  call output('Reorder axes to take moment of axis 2')
	  call bug('f','axis 2 is not implemented.')
	endif
c
c  Determine the min and max value.
c
	call ImMinMax(lIn,naxis,nsize,blo,bhi)
	if(blo.eq.bhi)then
	  call xyclose(lIn)
	  write(line,'(''All pixels are '',1pg10.3)')blo
	  call output(line)
	  stop
	endif
c
c  Set up the region of interest. Warn if not rectangle.
c
c	call BoxMask(lIn,boxes,maxboxes)	- not yet used
	call BoxSet(boxes,maxnax,nsize,'s')
	call BoxInfo(boxes,maxnax,blc,trc)
c
c  Open output image and fill in its header.
c
	j = 0
	do i = 1,naxis
	  if(blc(i).lt.1) blc(i) = 1
          if(trc(i).gt.nsize(i)) trc(i) = nsize(i)
	  if(i.ne.axis) then
	    j = j + 1
	    size(j) = trc(i) - blc(i) + 1
	  endif
	enddo
	size(naxis) = 1
	call xyopen(lOut,out,'new',naxis,size)
	call header(lIn,lOut,naxis,blc,trc,mom,axis)
c
c  Calculate moment.
c
	call makemom(lIn,lOut,naxis,blc,trc,mom,axis,clip)
c
c  Update history and close files.
c
	call Hisopen(lOut,'append')
        call HisWrite(lOut,'XPEAK: '//version)
	call HisInput(lOut,'XPEAK')
	call HisClose(lOut)
	call xyclose(lIn)
	call xyclose(lOut)
	end
c************************************************************************
	subroutine header(lIn,lOut,naxis,blc,trc,mom,axis)
	implicit none
	integer lin,lOut,naxis,blc(naxis),trc(naxis),mom,axis
c
c  Copy keywords to output file.
c
c  Inputs:
c    lIn,lOut	Handle of input and output files.
c    naxis	The number of input axes.
c    blc,trc	The corners of the input image.
c    mom	The moment to be calculated.
c    axis	The axis for which the moment is calculated.
c------------------------------------------------------------------------
	integer nkeys, nckeys,i,j,k
	parameter (nkeys=23, nckeys=4)
	character keyw(nkeys)*9, ckeyw(nckeys)*5, itoaf*1, cin*1, cout*1
	character atemp*9,ctype*10
	double precision rtemp,cdelt,crval,crpix,idx
	logical hdprsnt
c
c  Be careful that nkeys and nckeys match the number of keywords.
c	bunit is a special case for this task.
c
	data keyw/   'bmaj    ','bmin    ','bpa     ','rms     ',
     *    'obstime ','epoch   ','history ','instrume',
     *	  'ltype   ','lstart  ','lwidth  ','lstep   ','niters  ',
     *	  'object  ','observer','obsra   ','obsdec  ','pbfwhm  ',
     *    'restfreq','telescop','vobs    ','pbtype  ','cellscal'/
c
c  Keyword values which must be changed as they are passed from in to out.
c
	data ckeyw/'ctype','cdelt','crval','crota'/
c
c  Copy across unchanged header keywords.
c
	do i = 1,nkeys
	  call hdcopy(lin,lout,keyw(i))
	enddo
c
c  Handle the keywords which must be moved to another axis.
c
	j = 0
	do i = 1,naxis
	  if(i.ne.axis) then
	    j = j + 1
	    cin = itoaf(i)
	    cout = itoaf(j)
	    atemp = ckeyw(1)//cin
	    if(hdprsnt(lin,atemp)) then
	      call rdhda(lin,ckeyw(1)//cin,atemp,' ')
	      call wrhda(lout,ckeyw(1)//cout,atemp)
	    endif
	    do k = 2,nckeys
	      atemp = ckeyw(k)//cin
	      if(hdprsnt(lin,atemp)) then
	        call rdhdd(lin,ckeyw(k)//cin,rtemp,0.0d0)
	        call wrhdd(lout,ckeyw(k)//cout,rtemp)
	      endif
	    enddo
c
c  Special cases: the crpixes will change if the user uses a subcube.
c
	    if(hdprsnt(lin,'crpix'//cin)) then
	      call rdhdd(lin,'crpix'//cin,rtemp,0.0d0)
	      rtemp = rtemp - dble(blc(i)) + 1
	      call wrhdd(lout,'crpix'//cout,rtemp)
	    endif
	  endif
	enddo
c
c  bunit is usually a pain.  For this program it is relatively simple.
c
	if(mom.eq.-1 .or. mom.eq.0) then
	  if(hdprsnt(lin,'bunit')) then
            call rdhda(lin,'bunit',atemp,' ')
            call wrhda(lout,'bunit',atemp)
	  endif
          call wrbtype(lout,'intensity')
	else 
	  if(mom.eq.1.or.mom.eq.2) then
            atemp = 'KM/S'
            call wrhda(lout,'bunit',atemp)
            call wrbtype(lout,'velocity')
	  else
            cin = itoaf(mom)
            atemp = 'KM/S**'//cin
            call wrhda(lout,'bunit',atemp)
            call wrbtype(lout,'velocity_dispersion')
	  endif
	endif
c
c  Write out additional information about the ``third'' dummy axis.
c  also the third dimension is 1 (naxis3=1), the axes are labeled
c  as much as possible from the input cube
c
        cin = itoaf(axis)
        idx = (blc(axis)+trc(axis))/2.0
	call rdhdd(lin,'crpix'//cin, crpix, 1.0d0)
	call rdhdd(lin,'cdelt'//cin, cdelt, 1.0d0)
	call rdhdd(lin,'crval'//cin, crval, 0.0d0)
	call rdhda(lin,'ctype'//cin, ctype, ' ')
	crval = crval + (idx-crpix)*cdelt 
	cdelt = cdelt * (trc(axis)-blc(axis)+1)
	call wrhdd(lout,'crpix3', 1.d0)
	call wrhdd(lout,'cdelt3', cdelt)
	call wrhdd(lout,'crval3', crval)
	call wrhda(lout,'ctype3', ctype)
	end
c********1*********2*********3*********4*********5*********6*********7*c
	subroutine makemom(lIn,lOut,naxis,blc,trc,mom,axis,clip)
	implicit none
	integer lin,lOut,naxis,blc(naxis),trc(naxis),mom,axis
	real clip(2)
c
c  Calculate the scale factor and channel offset for moments.
c
c  Inputs:
c    lIn,lOut	Handle of input and output files.
c    naxis	The number of input axes.
c    blc,trc	The corners of the input image.
c    mom	The moment to be calculated. Default = 0.
c    axis	The axis for which the moment is calculated.
c    clip	Pixel values in range clip(1) to clip(2) are excluded.
c------------------------------------------------------------------------
	include 'maxdim.h'
	real scale,restfreq,offset
	real crpix,cdelt,crval
	integer n1,n2,pSum,pFlags
	character ctype*9,cin*1
c
c  Dynamic memory junk.
c
	real ref(MAXBUF)
	logical lref(MAXBUF)
	equivalence (ref,lref)
	common ref
c
c  Externals.
c
	logical hdprsnt
	character itoaf*1
c
c Calculate the appropriate scale factor to convert from channels to km/sec.
c
	cin = itoaf(axis)
	call rdhda(lIn,'ctype'//cin,ctype,' ')
	if(ctype(1:4).ne.'VELO' .and. ctype(1:4).ne.'FELO' .and.
     *						ctype(1:4).ne.'FREQ')
     *        call bug('w','axis must be VELO, FELO, or FREQ.')
	call rdhdr(lin,'cdelt'//cin,cdelt,0.0)
	if(cdelt.eq. 0.0) call bug('f','cdelt is 0 or not present.')
c
	if(mom .eq. -1) then
	  scale = 1.0 / real(trc(axis)-blc(axis)+1)
	else if(mom .eq. 0) then
	  scale = 1.
	else
	  scale = cdelt
	  if(ctype(1:4).eq.'FREQ') then
            call rdhdr(lin,'restfreq',restfreq,0.)
            if(restfreq .eq. 0.)
     *	      call bug('f','restfreq not present in header.')
            scale = cdelt * 2.99793e5 / restfreq
          endif
	endif
c
c  Get offset in channels.
c
	if(hdprsnt(lin,'crpix'//cin)
     *			 .and. hdprsnt(lin,'crval'//cin)) then
	  call rdhdr(lin,'crpix'//cin,crpix,0.0)
	  call rdhdr(lin,'crval'//cin,crval,0.0)
	  offset = crpix - crval/cdelt
	else
	  call bug('f','crpix, or crval not in header.')
	endif
c
c  Compute the moment.
c
	if(axis.eq.1)then
	  call moment1(lIn,lOut,naxis,blc,trc,mom,scale,offset,clip)
	else if(axis.eq.2)then
	  call bug('f','axis 2 is not implemented.')
	else if(axis.eq.3)then
	  n1 = trc(1) - blc(1) + 1
	  n2 = trc(2) - blc(2) + 1
	  call memalloc(pSum,3*n1*n2,'r')
	  call memalloc(pFlags,n1*n2,'l')
	  call moment3(lIn,lOut,naxis,blc,trc,mom,scale,offset,clip,
     *	    ref(pSum),lref(pFlags),n1,n2)
	  call memfree(pFlags,n1*n2,'l')
	  call memfree(pSum,3*n1*n2,'r')
	endif
c
	end
c********1*********2*********3*********4*********5*********6*********7*c
	subroutine moment1(lIn,lOut,naxis,blc,trc,mom,scale,offset,clip)
	implicit none
	integer lIn,lOut,naxis,blc(naxis),trc(naxis),mom
	real scale,offset,clip(2)
c
c  Calculate the moment for axis 1.
c
c  Inputs:
c    lIn,lOut	Handle of input and output files.
c    naxis	The number of input axes.
c    blc,trc	The corners of the input image.
c    mom	The moment to be calculated. Default = 0.
c    scale	Scale factor to convert from channels to km/s
c    offset	Offset in channels.
c    clip	Pixels with values in range clip(1:2) are excluded.
c------------------------------------------------------------------------
	include 'maxdim.h'
	real buf(maxdim),sum(maxdim),sum1(maxdim),sum2(maxdim)
	real chan,chan2,flux,sigsq
	integer i,j,k
	logical flags(maxdim), outflags(maxdim)
c
	do k = blc(3),trc(3)
	  call xysetpl(lIn,1,k)
	  do j = blc(2),trc(2)
	    call xyread(lIn,j,buf)
	    call xyflgrd(lIn,j,flags)
	    sum(j) = 0.0
	    sum1(j) = 0.0
	    sum2(j) = 0.0
c
c  Loop through the velocity channels and accumulate the moments.
c
	    do i = blc(1),trc(1)
	      if(flags(i).and.
     *		 (buf(i).le.clip(1).or.buf(i).ge.clip(2)) ) then
		chan = (i-offset)*scale
		chan2 = chan*chan
			     sum(j) = sum(j) + buf(i)
	        if(mom.ge.1) sum1(j) = sum1(j) + buf(i)*chan
		if(mom.eq.2) sum2(j) = sum2(j) + buf(i)*chan2
	      endif
	    enddo
c
c  Normalize and scale the moments.
c
	    flux = sum(j)
	    if(flux.ne.0.) then
	      if(mom.eq.-1) sum(j) = sum(j) * scale
	      if(mom.ge.1) sum1(j) = sum1(j)/flux
	      outflags(j) = .true.
	      if(mom.eq.2) then
		sigsq = sum2(j)/flux - sum1(j)*sum1(j)
		if(sigsq.gt.0) then
		    sum2(j) = sqrt(sigsq)
		else
		  sum2(j) = 0.
		  flags(j) = .false.
		endif
	      endif
	    else
	      sum1(j) = 0.
	      sum2(j) = 0.
	      outflags(j) = .false.
            endif
	  enddo
c
c  Now write out the moment map.
c
	  if(mom.le.0) call xywrite(lOut,k-blc(3)+1,sum(blc(2)))
	  if(mom.eq.1) call xywrite(lOut,k-blc(3)+1,sum1(blc(2)))
	  if(mom.eq.2) call xywrite(lOut,k-blc(3)+1,sum2(blc(2)))
	  call xyflgwr(lOut,k-blc(3)+1,outflags(blc(2)))
	enddo
	end
c********1*********2*********3*********4*********5*********6*********7*c
	subroutine moment3(lIn,lOut,naxis,blc,trc,mom,scale,offset,clip,
     *	  sum,outflag,n1,n2)
c
	implicit none
	integer lIn,lOut,naxis,blc(naxis),trc(naxis),mom,n1,n2
	real scale,offset,clip(2),sum(n1,n2,3)
	logical outflag(n1,n2)
c
c  Calculate the moments for axis 3.
c
c  Inputs:
c    lIn,lOut	Handle of input and output files.
c    naxis	The number of input axes.
c    blc,trc	The corners of the input image.
c    mom	The moment to be calculated. Default = 0.
c    scale	Scale factor to convert from channels to km/s
c    offset	Offset in channels.
c    clip	Pixels with value in range clip(1),clip(2) are excluded.
c  Scratch:
c    sum	Used to accumulate the moments.
c    outflag	Flagging info for the output array.
c------------------------------------------------------------------------
	include 'maxdim.h'
	real buf(maxdim),flux
	real chan,chan2,sigsq
	integer i,j,k,i0,j0
	logical flags(maxdim),good
c
c  Check consistency.
c
	if(trc(2)-blc(2)+1.ne.n2.or.
     *	   trc(1)-blc(1)+1.ne.n1)call bug('f',
     *	  'Dimension inconsistency in MOMENT3')
c
c  Zero the array to accumulate the moments.
c
	do k=1,3
	  do j = 1,n2
	    do i = 1,n1
	      sum(i,j,k) = 0.0
	    enddo
	  enddo
	enddo
c
c  Loop through the velocity channels.
c
	do k = blc(3),trc(3)
	  call xysetpl(lIn,1,k)
	  chan = (k-offset)*scale
	  chan2 = chan*chan
c
c  Accumulate the moments.
c
	  j0 = 1
	  do j = blc(2),trc(2)
	    call xyread(lIn,j,buf)
	    call xyflgrd(lIn,j,flags)
	    i0 = 1
	    do i = blc(1),trc(1)
	      good=flags(i).and.(buf(i).le.clip(1).or.buf(i).ge.clip(2))
	      if(good) then
                       if(buf(i).GT.sum(i0,j0,1)) then
			                          sum(i0,j0,1) = buf(i)
                       endif
         	if(mom.ge.1) sum(i0,j0,2) = sum(i0,j0,2) + buf(i)*chan
		if(mom.ge.2) sum(i0,j0,3) = sum(i0,j0,3) + buf(i)*chan2
	      endif
	      i0 = i0 + 1
	    enddo
	    j0 = j0 + 1
	  enddo
	enddo
c
c  Normalize and scale the moments.
c
	do j = 1,n2
	  do i = 1,n1
	    flux = sum(i,j,1)
	    if(flux.ne.0.) then
	      if(mom.eq.-1) sum(i,j,1) = sum(i,j,1) * scale
	      if(mom.ge.1) sum(i,j,2) = sum(i,j,2)/flux
	      outflag(i,j) = .true.
	      if(mom.eq.2) then
	        sigsq = sum(i,j,3)/flux - sum(i,j,2)*sum(i,j,2)
	        if(sigsq.gt.0.) then
	          sum(i,j,3) = sqrt(sigsq)
	        else
	          sum(i,j,3) = 0.
	          outflag(i,j) = .false.
	        endif
	      endif
	    else
	      sum(i,j,2) = 0.
	      sum(i,j,3) = 0.
	      outflag(i,j) = .false.
	    endif
	  enddo
	enddo
c
c  Now write out the moment map.
c
	k = max(mom+1,1)
	do j = 1,n2
	  do i = 1,n1
	    buf(i) = sum(i,j,k)
	    flags(i) = outflag(i,j)
	  enddo

	  call xywrite(lOut,j,buf)
	  call xyflgwr(lOut,j,flags)
	enddo
c
	end






