#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.63])
AC_INIT(camera-server,0.1,zergwangj@163.com)
AM_INIT_AUTOMAKE(camera-server,0.1)
AC_CONFIG_SRCDIR([CameraServer.cpp])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC

# Checks for libraries.
AC_CHECK_LIB(v4l2,v4l2_open,,AC_MSG_WARN(v4l2.a may be required for capture video))
AC_CHECK_LIB(x264,x264_predict_16x16_init)
AC_CHECK_LIB(avutil,av_malloc,,AC_MSG_WARN(libavutil.a may be required for MPEG streaming))
AC_CHECK_LIB(avcodec,avcodec_register_all,,AC_MSG_WARN(libavcodec.a is required for MPEG streaming))
AC_CHECK_LIB(avformat,av_new_stream,,AC_MSG_WARN(libavformat.a is required for MPEG streaming))
AC_CHECK_LIB(avdevice,avdevice_register_all,,AC_MSG_WARN(libavdevice.a may be required for MPEG streaming))
AC_CHECK_LIB(swscale,sws_scale,,,-lswscale)

# Checks for header files.
AC_CHECK_HEADERS([libavcodec/avcodec.h libavformat/avformat.h libswscale/swscale.h libavdevice/avdevice.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_UINT8_T

# Checks for library functions.
AC_CHECK_FUNCS([memset])

# Enable debug compiler flags. 
AC_ARG_ENABLE(debug,
AS_HELP_STRING([--enable-debug],
               [enable debugging, default: no]),
[case "${enableval}" in
             yes) debug=true ;;
             no)  debug=false ;;
             *)   AC_MSG_ERROR([bad value ${enableval} for --enable-debug]) ;;
esac],
[debug=false])

AM_CONDITIONAL(DEBUG, test x"$debug" = x"true")

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
