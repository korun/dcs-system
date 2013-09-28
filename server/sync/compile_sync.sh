#~ gcc [-c|-S|-E] [-std=standard]
    #~ [-g] [-pg] [-Olevel]
    #~ [-Wwarn...] [-pedantic]
    #~ [-Idir...] [-Ldir...]
    #~ [-Dmacro[=defn]...] [-Umacro]
    #~ [-foption...] [-mmachine-option...]
    #~ [-o outfile] [@file] infile...
#~ -pedantic
echo 'Compile sync to sync.o ...'
gcc -c -std=c99 -Ofast -Wall -Werror -I/usr/include/postgresql/ -L/usr/lib/ -o sync.o sync.c -lpq
