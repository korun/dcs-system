echo 'Compile SQLite to sqlite.o ...'
gcc -c -std=c99 -O3 -Wall -Werror -pedantic -o sqlite.o sqlite3.c
