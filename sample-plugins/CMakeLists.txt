add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/../file-manager-plugins/libtxt.so
    COMMAND gcc -c -o ${CMAKE_CURRENT_SOURCE_DIR}/libtxt.o ${CMAKE_CURRENT_SOURCE_DIR}/txt-to-nano.c
    COMMAND gcc -o ${CMAKE_CURRENT_SOURCE_DIR}/libtxt.so --shared ${CMAKE_CURRENT_SOURCE_DIR}/libtxt.o
    COMMAND rm ${CMAKE_CURRENT_SOURCE_DIR}/libtxt.o
    COMMAND mkdir -p ${CMAKE_CURRENT_BINARY_DIR}/../file-manager-plugins/
    COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/libtxt.so ${CMAKE_CURRENT_BINARY_DIR}/../file-manager-plugins/
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/txt-to-nano.c
)

add_custom_target(plugins ALL
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/../file-manager-plugins/libtxt.so)
