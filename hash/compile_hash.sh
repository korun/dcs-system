#~ gcc [-c|-S|-E] [-std=standard]
    #~ [-g] [-pg] [-Olevel]
    #~ [-Wwarn...] [-pedantic]
    #~ [-Idir...] [-Ldir...]
    #~ [-Dmacro[=defn]...] [-Umacro]
    #~ [-foption...] [-mmachine-option...]
    #~ [-o outfile] [@file] infile...

echo 'Compile hash to hash.o ...'
gcc -c -ansi -Ofast -Wall -Werror -pedantic -DAUTO_EXPAND=1 -o hash.o hash.c
echo 'Compile hashlittle to hashlittle.o ...'
gcc -c -ansi -Ofast -Wall -Werror -pedantic -o hashlittle.o hashlittle.c
