gcc -o libtxt.o -c txt-to-nano.c
gcc --shared libtxt.o -o libtxt.so
rm libtxt.o
