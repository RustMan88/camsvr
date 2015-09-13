AUTOMAKE_OPTIONS = foreign  
bin_PROGRAMS = camsvr  
camsvr_SOURCES = CameraServer.cpp include/H264LiveCapture.h include/H264LiveCaptureThread.hpp include/H264LiveFramedSource.hpp include/H264LiveServerMediaSubsession.hpp include/ADTSLiveCapture.h include/ADTSLiveCaptureThread.hpp include/ADTSLiveFramedSource.hpp include/ADTSLiveServerMediaSubsession.hpp src/H264LiveCapture.c src/H264LiveCaptureThread.cpp src/H264LiveFramedSource.cpp src/H264LiveServerMediaSubsession.cpp src/ADTSLiveCapture.c src/ADTSLiveCaptureThread.cpp src/ADTSLiveFramedSource.cpp src/ADTSLiveServerMediaSubsession.cpp

if IS_DEBUG
AM_CFLAGS = -g3 -O0 -DIS_DEBUG
AM_CXXFLAGS = -g3 -O0 -DIS_DEBUG
else
AM_CFLAGS = -O3
AM_CXXFLAGS = -O3
endif
