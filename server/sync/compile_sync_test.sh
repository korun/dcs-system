#~ gcc [-c|-S|-E] [-std=standard]
    #~ [-g] [-pg] [-Olevel]
    #~ [-Wwarn...] [-pedantic]
    #~ [-Idir...] [-Ldir...]
    #~ [-Dmacro[=defn]...] [-Umacro]
    #~ [-foption...] [-mmachine-option...]
    #~ [-o outfile] [@file] infile...
./compile_sync.sh &&
echo 'Compile sync_test...' &&
gcc -std=c99 -Ofast -Wall -Werror -pedantic -I/usr/include/postgresql/ -L/usr/lib/ -o sync.exe sync.o sync_test.c ../../cache/cache.c ../../hash/hash.o ../../hash/hashlittle.o -lpq
