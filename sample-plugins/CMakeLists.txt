add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/../file-manager-plugins/libtxt.so
    COMMAND gcc -c -o libtxt.o txt-to-vim.c
    COMMAND gcc -o libtxt.so --shared libtxt.o
    COMMAND rm libtxt.o
    COMMAND mkdir -p ${CMAKE_CURRENT_BINARY_DIR}/../file-manager-plugins/
    COMMAND cp libtxt.so ${CMAKE_CURRENT_BINARY_DIR}/../file-manager-plugins/
    DEPENDS txt-to-vim.c
)
