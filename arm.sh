CC="arm-none-linux-gnueabi-gcc" CXX="arm-none-linux-gnueabi-g++" CFLAGS="-I./include -I../include -I/usr/local/include -I/usr/local/include/BasicUsageEnvironment -I/usr/local/include/UsageEnvironment -I/usr/local/include/groupsock -I/usr/local/include/liveMedia" CPPFLAGS="-I./include -I../include -I/usr/local/include -I/usr/local/include/BasicUsageEnvironment -I/usr/local/include/UsageEnvironment -I/usr/local/include/groupsock -I/usr/local/include/liveMedia" LDFLAGS="-L/usr/local/lib" LIBS="-lliveMedia -lgroupsock -lBasicUsageEnvironment -lUsageEnvironment -lv4l2 -lv4lconvert -lpthread -lrt -lm -lavutil -lavcodec -lavfilter -lavdevice -lavformat -lswscale -lpostproc -lswresample -lx264" ./configure --host=arm-none-linux-gnueabi --target=arm-none-linux-gnueabi

make
make install

