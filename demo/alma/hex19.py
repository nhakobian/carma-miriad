#!/usr/bin/env python

# hex19.py

print "   ---  ALMA Mosaicing (Cas A model)   ---   "
print "   mchw. 12mar03 version"

# History:
#  june 02 mchw. ALMA script.
#  15aug02 mchw. CARMA version edited from ALMA script.
#  23aug02 mchw. calculate region from source size.
#  20sep02 mchw. Re-import CARMA improvements for ALMA.
#  25sep02 mchw. Re-import improvements from hex7.csh to hex19.csh
#  26sep02 mchw. Increase imsize from 129 to 257.
#  12mar03 mchw. convert to PYTHON.
#  13mar03 pjt   more conversion to PYTHON, now at 200ft

import sys, os, time, string, math
version='2003-03-12'

# command line arguments that can be changed...
keyval = {
    "config"  : "config1",           # antenna config file (without the .ant extension)
    "dec"     : "-30",               # declination (can be a real number)
    "cell"    : "0.04",              # scale size.
    "nchan"   : "1",                 # number of channels
    "method"  : "mosmem",            # mosmem, joint, or default
    "center"  : "hex19_12",          # file with pointing offsets (uvgen format)
    "VERSION" : "1.0 mchw"           # VERSION id for the user interface
    }

# -----------------------------------------------------------------------------
#
#                                       parse command line (must be 'key=val')
quit = 0
for arg in sys.argv[1:]:
    i=string.find(arg,"=")
    if arg == "--help":
        quit=1
    elif i > 0:
        key = arg[0:i]
        val = arg[i+1:]
        print arg,i,key
        if keyval.has_key(key):
            keyval[key] = val
        else:
            print "### Warning: keyword in %s not understood" % arg
    else:
        print "### Warning: argument %s not understood" % arg
        

#                                            report current defaults
print "Current command line defaults are:"
for k in keyval.keys():
    print k + '=' + keyval[k]
if quit:
    os._exit(0)
# -----------------------------------------------------------------------------

#
# define all variables, now in their proper type, for this script
#

log = 'mfs.log'
logger = '>> %s 2>&1' % log                        # blank this is you want logging to the screen
#logger = ''
config  = keyval['config']
dec     = string.atof(keyval['dec'])
cell    = string.atof(keyval['cell'])
nchan   = string.atoi(keyval['nchan'])
method  = keyval['method']
center  = keyval['center']

harange = '-1,1,0.013'
select  = '-shadow\(12\)'
freq    = 230.0
imsize  = 257                    # avoid 2**N, image size 2**N + 1 is good.
npoint  = 20              #  should be #entries in the @center file + 1

mir = os.environ['MIR']

# -----------------------------------------------------------------------------


# MIRIAD
#   remove a possibly existing miriad dataset (no wildcards)
def zap(file):
    if os.path.isdir(file):
        os.system('rm -rf ' + file)
#   remove all datasets (wildcards now allowed)
def zap_all(files):
    os.system('rm -rf ' + files)

# MIRIAD
#   execute a miriad 'command' (a list of strings) and accumulate a log 
def miriad(command,log=0):
    if (log != 0):
        mycmd = cmd(command) + '> %s.log 2>&1' % command[0]
    else:
        mycmd = cmd(command) + logger;
    return os.system(mycmd)

# MIRIAD
#   convert a list of strings to a command string
def cmd(cmdlist):
    str = cmdlist[0] 
    for arg in cmdlist[1:]:
        str = str + ' ' +  arg
    print str
    return str

# MIRIAD
#   save CPU info in predefined global slots
def timer(slot):
    global timet, timec
    timec[slot] = time.clock()
    timet[slot] = time.time()

# MIRIAD
#   create a string to print human readable CPU info between slots 
def cpulen(a,b):
    st = '%.3f ' % (timet[b]-timet[a])
    sc = '%.3f ' % (timec[b]-timec[a])
    return sc + st

# MIRIAD
#   run a command, grep through the output for a word and return a 0-indexed word
#   should use popen()
def grepcmd(cmd,word,index=0):
    log = 'tmp.log'
    os.system(cmd + ' > %s' % log)
    f = open(log,"r")
    v = f.read()
    va=string.split(v,"\n")
    f.close()
    for s in va:
        if string.find(s,word)>=0:
            sa=string.split(s)
            # print 'Match ' + word + '=>' + sa[index]
            return sa[index]
    print 'No match on' + word
    return "no-match"

# MIRIAD
#   grep through a logfile for a word and return a 0-indexed word
def greplog(log,word,index=0):
    f = open(log,"r")
    v = f.read()
    va=string.split(v,"\n")
    f.close()
    for s in va:
        if string.find(s,word)>=0:
            sa=string.split(s)
            # print 'Match ' + word + '=>' + sa[index]
            return sa[index]
    print 'No match on' + word
    return "no-match"

#----------------------

       
#   get the (as a string) value of an item in a dataset
def itemize1(data,item):
    log = 'tmp.log'
    cmd = [
        'itemize',
        'in=%s/%s' % (data, item),
        'log=%s' % log
        ]
    miriad(cmd)
    f = open(log,"r")
    v = string.split(f.readline())
    f.close()
    return v[2]

# -----------------------------------------------------------------------------


#   copy a file from source (s) to destination (d)
def copy_data(s,d):
    zap(d)
    os.system("cp -r %s %s" % (s,d))


#  should this be
#    units=None
#    if units is None:
#        bla
#    else:
#        bla

def puthd(map,item,value,units=0):
    cmd = [
        'puthd',
        'in=%s/%s' % (map,item),
        ]
    if (units == 0):
        cmd.append("value=%s" % value)
    else:
        cmd.append("value=%s,%s" % (value,units))
    return cmd

def demos(map,vis,out):
    cmd = [
        'demos',
        'map=%s' % map,
        'vis=%s' % vis,
        'out=%s' % out,
        ]
    zap_all(out+"*")
    return cmd;

def invert(vis,map,beam,imsize,select):
    cmd = [
     'invert',
     'vis=%s' % vis,
     'map=%s' % map,
     'beam=%s' % beam,
     'imsize=%d' % imsize,
     'select=%s' % select,
     'sup=0',
     'options=mosaic,double',
     ]
    zap(map)
    zap(beam)
    return cmd

def cgdisp(map):
    cmd = [
        'cgdisp',
        'map=%s' % map,
        'device=/xs',
        'labtyp=arcsec',
        'range=0,0,lin,8'
        ]
    return cmd;

def uvmodel(vis,model,out):
    cmd = [
        'uvmodel',
        'vis=%s' % vis,
        'model=%s' % model,
        'out=%s' % out,
        'options=add,selradec'
        ]
    zap(out)
    return cmd;

def implot(map,region='quarter'):
    cmd = [
        'implot',
        'in=' + map,
        'device=/xs',
        'units=s',
        'conflag=l',
        'conargs=1.4',
        'region=%s' % region
        ]
    return cmd

def imlist(map):
    cmd = [
        'imlist',
        'in=' + map,
        'options=mosaic'
        ]
    return cmd


def imgen(map,out,pbfwhm):
    cmd = [
        'imgen',
        'in=%s' % map,
        'out=%s' % out,
        'object=gaussian',
        'factor=0',
        'spar=1,0,0,%g,%g' % (pbfwhm,pbfwhm)
        ]
    zap(out)
    return cmd


def mosmem(map,beam,out,region,flux=0,default=0):
    cmd = [
        'mosmem',
        'map=%s' % map,
        'beam=%s' % beam,
        'out=%s' % out,
        'region=%s' % region,
        'rmsfac=200,1',
#        'niters=200'
        'niters=2'
        ]
    if flux != 0:
        cmd.append('flux=%g' % flux)
    if default != 0:
        cmd.appenx('default=%s' % default)
    zap(out)
    return cmd

def restor(map,beam,model,out):
    cmd = [
        'restor',
        'model=%s' %  model,
        'map=%s' % map,
        'beam=%s' % beam, 
        'out=%s' % out
        ]
    zap(out)
    return cmd

def regrid(map,tin,out):
    cmd = [
        'regrid',
        'in=%s' % map,
        'tin=%s' % tin, 
        'out=%s' % out,
        'axes=1,2'
        ]
    zap(out)
    return cmd

def convol(map,out,b1,b2,pa):
    cmd = [
        'convol',
        'map=%s' % map,
        'out=%s' % out,
        'fwhm=%g,%g' % (b1,b2),
        'pa=%g' % pa 
        ]
    zap(out)
    return cmd

def imframe(map,out):
    cmd = [
        'imframe',
        'in=%s' % map,
        'out=%s' % out,
        'frame=-1024,1024,-1024,1024'
        ]
    zap(out)
    return cmd

def uvgen(ant,dec,harange,freq,nchan,out,center):
    cmd = [
        'uvgen',
        'ant=%s' % ant,
        'baseunit=-3.33564',
        'radec=23:23:25.803,%g' % dec,
        'lat=-23.02',
        'harange=%s' % harange,
        'source=$MIRCAT/point.source',
        'telescop=alma',
        'systemp=40',
        #        'pnoise=30',
        'jyperk=40',
        'freq=%g' % freq,
        'corr=%d,1,0,8000' % nchan,
        'out=%s' % out,
        'center=@%s' % center
        ]
    zap(out)
    return cmd

def imdiff(in1,in2,resid):
    cmd = [
        'imdiff',
        'in1=%s' % in1,
        'in2=%s' % in2,
        'resid=%s' % resid,
        'options=nox,noy,noex'
        ]
    zap(resid)
    return cmd

def histo(map,region):
    cmd = [
        'histo',
        'in=%s' % map,
        'region=%s' % region
        ]
    return cmd


# -----------------------------------------------------------------------------
# Nyquist sample rate for each pointing.
# calc '6/(pi*250)*12'
cells  = 500*cell
region = "arcsec,box\(%.2f,-%.2f,-%.2f,%.2f\)" % (cells,cells,cells,cells)

ant    = config + '.ant'
uv     = config + '.uv'
casc   = "casc.vla"                        # the archived VLA map (miriad format)

demos1 = "%s.cas.%g.demos" % (config,cell)
base1  = "%s.%g.cas.%g" % (config,dec,cell)
base2  = "single.%g.cas.%g" % (dec,cell)

map1   = base1 + ".mp"
beam1  = base1 + ".bm"
map2   = base2 + ".map"
beam2  = base2 + ".beam"
mem    = base1 + ".mem"
cm     = base1 + ".cm"
mp     = base1 + '.mp'
res    = base1 + '.resid'
conv   = base1 + '.conv'


print "   ---  ALMA Mosaicing (Cas A model)   ---   " 
print " config  = $config"            
print " dec     = $dec"               
print " scale   = $cell"              
print " harange = $harange  hours"    
print " select  = $select"            
print " freq    = $freq"              
print " nchan   = $nchan"             
print " imsize  = $imsize"            
print " region  = $region"            
print " "                             
print "   ---  TIMING   ---   "       
print "START: `date`"

print "Generate mosaic grid"
#  lambda/2*antdiam (arcsec)
print 300/freq/2/12e3*2e5

print "Using %s mosaic listing" % center


if method == "mosmem":
    print "Generate uv-data. Tsys=40K, bandwidth=8 GHz " 
    miriad(uvgen(ant,dec,harange,freq,nchan,uv,center))
    os.system('uvindex vis=%s' % uv)
    
    print "Scale model size from pixel 0.4 to %g arcsec" % cell
    # with 0.4 arcsec pixel size Cas A is about 320 arcsec diameter; image size 1024 == 409.6 arcsec
    # scale model size. eg. cell=0.1 arcsec -> 80 arcsec cell=.01 -> 8 arcsec diameter
    # rm -r single.$dec.cas.$cell
    # cp -r casc.vla single.$dec.cas.$cell


    copy_data(casc,base2)

    miriad(puthd(base2,'crval2',dec,units='dms'))
    miriad(puthd(base2,'crval3',freq))
    miriad(puthd(base2,'cdelt1',-cell,units='arcsec'))
    miriad(puthd(base2,'cdelt2',cell,units='arcsec'))
    
    print "Make model images for each pointing center" 
    
    miriad(demos(base2,uv,demos1))

    print "Make model uv-data using VLA image of Cas A as a model (the model has the VLA primary beam)"
    
    for i in range(1,npoint):
        miriad(cgdisp(demos1+"%d"%i))
        vis_i = base1+".uv%d"%i
        miriad(uvmodel(uv,demos1+"%d"%i,vis_i))
        if i==1:
            vis_all=vis_i
        else:
            vis_all=vis_all + ',' + vis_i
            
        print "UVMODEL: add the model to the noisy sampled uv-data"
            
        miriad(invert(vis_all, base1+".mp", base1+".bm", imsize, select))
            
        print "INVERT: "
        
        miriad(implot(base1+'.mp',region=region))
        miriad(imlist(base1+'.mp'))
        
            
        print "Make single dish image and beam"
        
        pbfwhm = string.atof(grepcmd("pbplot telescop=alma freq=%g" % freq, "FWHM", 2)) * 60.0

        print "Single dish FWHM = %g arcsec at %g GHz" % (pbfwhm,freq)

        miriad(imframe(base2,base2+".bigger"))
        miriad(convol(base2+".bigger",base2+".bigger.map",pbfwhm,pbfwhm,0.0))
        miriad(regrid(base2+".bigger.map",base1+'.mp',base2+".map"))
        
        # implot in=single.$dec.cas.$cell.map units=s device=/xs conflag=l conargs=2

        miriad(imgen(base2+".map",base2+".beam",pbfwhm))
        miriad(implot(base2+".map"))

        # huh???
        miriad(puthd(base2+".map",'rms','7.32'))
        

if method=='mosmem':
    print " MOSMEM Interferometer only" 
    print " MOSMEM Interferometer only with niters=200 flux=732.063 rmsfac=200." 
    miriad(mosmem(map1,beam1,mem,region,flux=732.063))
elif method=='joint':
    print "Joint deconvolution of interferometer and single dish data"
    print "Joint deconvolution of interferometer and single dish data ; niters=200 rmsfac=200,1"
    miriad(mosmem(map1+','+map2,beam1+','+beam2,mem,region))
elif method=='default':
    print "MOSMEM with default single dish image" 
    print "MOSMEM with default single dish image; niters=200 rmsfac=200" 
    miriad(mosmem(map1,beam1,mem,region))
else:
    print "Unknown method " + method


# restor map=$config.$dec.cas.$cell.mp beam=$config.$dec.cas.$cell.bm out=$config.$dec.cas.$cell.cm model=$config.$dec.cas.$cell.mem

miriad(restor(map1,beam1,mem,cm))

# implot device=/xs units=s in=$config.$dec.cas.$cell.cm region=$region

print "convolve the model by the beam and subtract from the deconvolved image"
b1 = string.atof(grepcmd('prthd in=%s' % cm, 'Beam', 2))
b2 = string.atof(grepcmd('prthd in=%s' % cm, 'Beam', 4))
b3 = string.atof(grepcmd('prthd in=%s' % cm, 'Position', 2)) 

miriad(convol(base2,base1+'.conv',b1,b2,b3))
miriad(implot(base1+'.conv',region=region))

print "regrid the convolved model to the deconvolved image template"

miriad(regrid(base1+".conv",base1+".cm",base1+'.regrid'))
miriad(implot(base1+'.regrid',region=region))

#  cgdisp range=0,0,lin,8 in=$config.$dec.cas.$cell.conv device=$config.$dec.cas.$cell.conv.gif/gif labtyp=arcsec,arcsec options=beambl,wedge  region=$region


miriad(imdiff(base1+'.cm',base1+'.regrid',base1+'.resid'))

miriad(implot(base1+'.resid',region=region))
miriad(histo(base1+'.resid',region=region))

print "FINISH:"

# ================================================================================

print "print out results - summarize rms and beam sidelobe levels"
print "   ---  RESULTS   ---   "

#   extract information, the hard way

#   BUG: doesn't look like 'mp' has rms???
#rms    = string.atof(itemize1(mp,'rms')) * 1000
rms = -1
srms   = string.atof(grepcmd('histo in=%s' % res, 'Rms', 3)) 
smax   = string.atof(grepcmd('histo in=%s' % res, 'Maximum', 2))
smin   = string.atof(grepcmd('histo in=%s' % res, 'Minimum', 2))
Model_Flux = string.atof(grepcmd('histo in=%s region=%s' % (conv,region),'Flux',5))
Model_Peak = string.atof(grepcmd('histo in=%s region=%s' % (conv,region),'Maximum',2))
Flux       = string.atof(grepcmd('histo in=%s region=%s' % (cm,region),'Flux',5))
Peak       = string.atof(grepcmd('histo in=%s region=%s' % (cm,region),'Maximum',2))
Fidelity   = Peak/srms 

print " Config  DEC  HA[hrs]    Beam[arcsec] scale Model_Flux,Peak  Image_Flux,Peak Residual:Rms,Max,Min[Jy] Fidelity"
print " %s %g %s %.3f %g %g %g %.3f %.3f %.3f %.3f %.3f %.3f %.3f %.3f" % (config,dec,harange,rms,b1,b2,
                                                              cell,Model_Flux,Model_Peak,Flux,Peak,srms,smax,smin,Fidelity)

#mv timing hex19.$config.$dec.$harange.$nchan.$imsize
#cat $config.$dec.$harange.$nchan.$imsize
#cat casa.results
#enscript -r casa.results
