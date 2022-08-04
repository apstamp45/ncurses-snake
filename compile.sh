files=$(find . -name "*.c")
out="${PWD##*/}"
gcc -Wall -masm=intel -o $out $files $(cat cargs.txt)
gcc -g -Wall -masm=intel -o $out-debug $files $(cat cargs.txt)
