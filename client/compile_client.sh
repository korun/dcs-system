#~ gcc [-c|-S|-E] [-std=standard]
    #~ [-g] [-pg] [-Olevel]
    #~ [-Wwarn...] [-pedantic]
    #~ [-Idir...] [-Ldir...]
    #~ [-Dmacro[=defn]...] [-Umacro]
    #~ [-foption...] [-mmachine-option...]
    #~ [-o outfile] [@file] infile...

MSG_SALT_PATH=${MSG_SALT_PATH:="/home/kia84/Documents/dcs/salt"}
echo "MSG_SALT_PATH is set to $MSG_SALT_PATH"

echo 'Compile and link client and all components...' &&
gcc -std=c99 -O0 -Wall -Werror -pedantic -DMSG_SALT_PATH=`echo "\"$MSG_SALT_PATH\""` -o dcs_client.exe client.c gethwdata.c ../strxtoi.c ../md5/md5c.c ../md5/message_digest.c -lzmq -lrt
