#~ gcc [-c|-S|-E] [-std=standard]
    #~ [-g] [-pg] [-Olevel]
    #~ [-Wwarn...] [-pedantic]
    #~ [-Idir...] [-Ldir...]
    #~ [-Dmacro[=defn]...] [-Umacro]
    #~ [-foption...] [-mmachine-option...]
    #~ [-o outfile] [@file] infile...

./compile_md5.sh
echo 'Compile md5.c md5_driver...'
gcc -Ofast -Wall -Werror -pedantic -DMD=5 -o md5driver.exe mddriver.c md5sum.o
echo 'Remove .o files...'
rm md5sum.o
