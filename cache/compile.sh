./compile_sqlite.sh
./compile_cache.sh &&

echo 'Compile and link cache_test...' &&
gcc -std=c99 -Ofast -Wall -Werror -pedantic -o cache.exe cache_test.c cache.o sqlite.o -lpthread -ldl &&

echo 'Removing .o files...' &&
rm cache.o
#~ rm sqlite.o
