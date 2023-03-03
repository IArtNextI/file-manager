add_library(manager_lib
    src/manager.c
)

target_include_directories(manager_lib PUBLIC include)

target_link_libraries(manager_lib PUBLIC
    terminal
)