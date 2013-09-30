./compile_hash.sh

echo 'Compile and link test_hash...'
gcc -ansi -Ofast -Wall -Werror -pedantic -o hash.exe test_hash.c hash.o hashlittle.o

echo 'Removing .o files...'
rm hash.o hashlittle.o
