

BUILD: Using vs2010 Pro:
$ nmake /f Makefile.mak ARCH=win32 # for the Windows 32 target

BUILD: Fedora:
yum install octave
yum install octave-devel.x86_64
sudo yum install octave-image.x86_64
sudo yum install transfig

git clone https://github.com/rxl194/vlfeat.git
cd ./vlfeat
sed -i 's/mex $(OCTAVE_MEX_FLAGS)/mex $(OCTAVE_MEX_FLAGS) $(OCTAVE_MEX_LDFLAGS)/' make/octave.mak
gmake MKOCTFILE=/usr/bin/mkoctfile

# run demo
mkdir -p doc/demo
cd toolbox
octave --persist --eval "vl_setup; vl_demo"

fresh install of Ubuntu 13.04:
# install required packages
sudo apt-get install octave octave-pkg-dev build-essential octave-image transfig
git clone https://github.com/rxl194/vlfeat.git

# apply patch
cd vlfeat-0.9.16
sed -i 's/mex $(OCTAVE_MEX_FLAGS)/mex $(OCTAVE_MEX_FLAGS) $(OCTAVE_MEX_LDFLAGS)/' make/octave.mak
make MKOCTFILE=/usr/bin/mkoctfile
