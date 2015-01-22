AUTOMAKE_OPTIONS = foreign  
bin_PROGRAMS = camsvr  
camsvr_SOURCES = CameraServer.cpp include/H264CameraCapture.h include/H264CameraFramedSource.hpp include/H264CameraServerMediaSubsession.hpp src/H264CameraCapture.c src/H264CameraFramedSource.cpp src/H264CameraServerMediaSubsession.cpp

if DEBUG
AM_CFLAGS = -g3 -O0
AM_CXXFLAGS = -g3 -O0
else
AM_CFLAGS = -O2
AM_CXXFLAGS = -O2
endif
