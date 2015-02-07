AUTOMAKE_OPTIONS = foreign  
bin_PROGRAMS = camsvr  
camsvr_SOURCES = CameraServer.cpp include/H264CameraCapture.h include/H264CameraFramedSource.hpp include/H264CameraServerMediaSubsession.hpp src/H264CameraCapture.c src/H264CameraFramedSource.cpp src/H264CameraServerMediaSubsession.cpp

if IS_DEBUG
AM_CFLAGS = -g3 -O0 -DIS_DEBUG
AM_CXXFLAGS = -g3 -O0 -DIS_DEBUG
else
AM_CFLAGS = -O3
AM_CXXFLAGS = -O3
endif
