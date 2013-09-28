gcc -c -O2 -Wall -pedantic -o getpci.o getpci.c &&
gcc getpci.o /usr/local/lib/libpci.a -lz -lresolv -o getpci &&
rm getpci.o
