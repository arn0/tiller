sudo /usr/bin/socat PTY,link=/dev/ttyS1,b38400,rawer,group-late=dialout,user-late=arno,mode=770 TCP4-LISTEN:3003,su=nobody,fork,range=192.168.20.1/24,reuseaddr