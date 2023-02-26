add_library(terminal
    src/terminal.c
)

target_include_directories(terminal PUBLIC include)

target_link_libraries(terminal PUBLIC
    vector
)