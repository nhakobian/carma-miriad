%!PS
/TM 820 def
/BM -70 def
/LM -20 def
/RM 630 def
/newline
 {currentpoint 13 sub
  exch pop LM
  exch moveto} def
/nlIfNec
 {currentpoint pop RM gt
   {newline}if} def
/done?
 {currentpoint exch pop
  BM lt} def
/fillpage
 {/strg exch def
  { {pop pop nlIfNec} strg kshow
     done? {exit} if
  }loop
}def
/Times-Bold findfont 10 scalefont setfont
LM TM moveto
.7 setgray
( Miriad ) fillpage
/Times-Roman findfont 50 scalefont setfont
RM LM sub
(Miriad)stringwidth pop sub
2 div 500 moveto
0 setgray
(Miriad)show
/Times-Roman findfont 20 scalefont setfont
RM LM sub (Multichannel Image Reconstruction,) stringwidth pop sub
2 div 460 moveto
(Multichannel Image Reconstruction,)show
RM LM sub (Image Analysis and Display) stringwidth pop sub
2 div 430 moveto
(Image Analysis and Display)show
/Times-Roman findfont 50 scalefont setfont
RM LM sub (Users Guide) stringwidth pop sub
2 div 360 moveto
(Users Guide)show
/Times-Roman findfont 20 scalefont setfont
RM LM sub (Bob Sault and Neil Killeen) stringwidth pop sub
2 div 130 moveto
(Bob Sault and Neil Killeen) show
RM LM sub (20 May 2008) stringwidth pop sub
2 div 100 moveto
(20 May 2008) show
RM LM sub (See http://www.atnf.csiro.au/computing/software/miriad)
stringwidth pop sub 2 div 20 sub 50 moveto
(See http://www.atnf.csiro.au/computing/software/miriad) show
