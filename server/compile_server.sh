#~ gcc [-c|-S|-E] [-std=standard]
    #~ [-g] [-pg] [-Olevel]
    #~ [-Wwarn...] [-pedantic]
    #~ [-Idir...] [-Ldir...]
    #~ [-Dmacro[=defn]...] [-Umacro]
    #~ [-foption...] [-mmachine-option...]
    #~ [-o outfile] [@file] infile...

cd ../cache/        &&
./compile_sqlite.sh &&
./compile_cache.sh  &&
cd ../hash/         &&
./compile_hash.sh   &&
cd ../server/sync   &&
./compile_sync.sh   &&
cd ..               &&
echo 'Compile and link server and all components...' &&
gcc -std=gnu99 -O0 -Wall -Werror -pedantic -I/usr/include/postgresql -L/usr/lib -DCATCH_SIGNAL=1 -o dcs_server.exe server.c ../strxtoi.c ../md5/message_digest.c ../md5/md5c.c ../cache/sqlite3.o ../cache/cache.o ../hash/hash.o ../hash/hashlittle.o sync/sync.o -ldl -lpq -lzmq -pthread
