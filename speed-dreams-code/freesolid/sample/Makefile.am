# Bypass extra crappy libtool linking

all: sample x_wing teapot torus ellipsoid walk depth

sample.o: sample.cpp
	$(CXX) $(AM_CXXFLAGS) -I@TOPDIR@/include -c $< -o $@
x_wing.o: x_wing.cpp
	$(CXX) $(AM_CXXFLAGS) -I@TOPDIR@/include -c $< -o $@
teapot.o: teapot.cpp
	$(CXX) $(AM_CXXFLAGS) -I@TOPDIR@/include -c $< -o $@
torus.o: torus.cpp
	$(CXX) $(AM_CXXFLAGS) -I@TOPDIR@/include @SDL_CFLAGS@ -c $< -o $@
ellipsoid.o: ellipsoid.cpp
	$(CXX) $(AM_CXXFLAGS) -I@TOPDIR@/include @SDL_CFLAGS@ -c $< -o $@
walk.o: walk.cpp
	$(CXX) $(AM_CXXFLAGS) -I@TOPDIR@/include @SDL_CFLAGS@ -c $< -o $@
depth.o: depth.cpp
	$(CXX) $(AM_CXXFLAGS) -I@TOPDIR@/include @SDL_CFLAGS@ -c $< -o $@

sample: sample.o
	$(CXX) $(AM_CXXFLAGS) $< -o $@ \
	-L@TOPDIR@/libsolid/.libs/ -lFreeSOLID $(AM_LDFLAGS)

x_wing: x_wing.o
	$(CXX) $(AM_CXXFLAGS) $< -o $@ \
	-L@TOPDIR@/libsolid/.libs/ -lFreeSOLID $(AM_LDFLAGS) 
teapot: teapot.o
	$(CXX) $(AM_CXXFLAGS) $< -o $@ \
	-L@TOPDIR@/libsolid/.libs/ -lFreeSOLID $(AM_LDFLAGS) 
torus: torus.o
	$(CXX) $(AM_CXXFLAGS) $< -o $@ \
	-L@TOPDIR@/libsolid/.libs/ -lFreeSOLID $(AM_LDFLAGS) @SDL_LIBS@ @GL_LIBS@
ellipsoid: ellipsoid.o
	$(CXX) $(AM_CXXFLAGS) $< -o $@ \
	-L@TOPDIR@/libsolid/.libs/ -lFreeSOLID $(AM_LDFLAGS) @SDL_LIBS@ @GL_LIBS@
walk: walk.o
	$(CXX) $(AM_CXXFLAGS) $< -o $@ \
	-L@TOPDIR@/libsolid/.libs/ -lFreeSOLID $(AM_LDFLAGS) @SDL_LIBS@ @GL_LIBS@
depth: depth.o
	$(CXX) $(AM_CXXFLAGS) $< -o $@ \
	-L@TOPDIR@/libsolid/.libs/ -lFreeSOLID $(AM_LDFLAGS) @SDL_LIBS@ @GL_LIBS@
clean:
	rm -rf *.o
	rm -rf sample
	rm -rf sample.exe
	rm -rf x_wing
	rm -rf x_wing.exe
	rm -rf teapot
	rm -rf teapot.exe
	rm -rf torus
	rm -rf torus.exe
	rm -rf ellipsoid
	rm -rf ellipsoid.exe
	rm -rf walk
	rm -rf walk.exe
	rm -rf depth
	rm -rf depth.exe

EXTRA_DIST = 	sample.cpp \
	     	x_wing.cpp \
	     	teapot.cpp \
		torus.cpp \
		ellipsoid.cpp \
		walk.cpp \
		sample.dsp \
		teapot.vrt \
		x_wing.vrt \
		world.vrt


