#~ gcc [-c|-S|-E] [-std=standard]
    #~ [-g] [-pg] [-Olevel]
    #~ [-Wwarn...] [-pedantic]
    #~ [-Idir...] [-Ldir...]
    #~ [-Dmacro[=defn]...] [-Umacro]
    #~ [-foption...] [-mmachine-option...]
    #~ [-o outfile] [@file] infile...

CATCH_SIGNAL=${CATCH_SIGNAL:=1}
echo "CATCH_SIGNAL is set to $CATCH_SIGNAL"
USR_LIB=${USR_LIB:="/usr/lib"}
echo "USR_LIB is set to $USR_LIB"
POSTGRESQL_LIB=${POSTGRESQL_LIB:="/usr/include/postgresql"}
echo "POSTGRESQL_LIB is set to $POSTGRESQL_LIB"
MSG_SALT_PATH=${MSG_SALT_PATH:="/home/kia84/Documents/dcs/salt"}
echo "MSG_SALT_PATH is set to $MSG_SALT_PATH"
echo

cd ../cache/        &&
./compile_sqlite.sh &&
./compile_cache.sh  &&
cd ../hash/         &&
./compile_hash.sh   &&
cd ../server/sync   &&
./compile_sync.sh   &&
cd ..               &&
echo 'Compile and link server and all components...' &&
gcc -std=gnu99 -O0 -Wall -Werror -pedantic -I$POSTGRESQL_LIB -L$USR_LIB -DCATCH_SIGNAL=$CATCH_SIGNAL -DMSG_SALT_PATH=`echo "\"$MSG_SALT_PATH\""` -o dcs_server.exe server.c ../strxtoi.c ../md5/message_digest.c ../md5/md5c.c ../cache/sqlite3.o ../cache/cache.o ../hash/hash.o ../hash/hashlittle.o sync/sync.o -ldl -lpq -lzmq -pthread
