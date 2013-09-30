#~ gcc [-c|-S|-E] [-std=standard]
    #~ [-g] [-pg] [-Olevel]
    #~ [-Wwarn...] [-pedantic]
    #~ [-Idir...] [-Ldir...]
    #~ [-Dmacro[=defn]...] [-Umacro]
    #~ [-foption...] [-mmachine-option...]
    #~ [-o outfile] [@file] infile...
echo 'Compile and link client and all components...' &&
gcc -std=c99 -O0 -Wall -Werror -pedantic -o dcs_client.exe client.c gethwdata.c ../strxtoi.c ../md5/md5c.c ../md5/message_digest.c -lzmq -lrt
