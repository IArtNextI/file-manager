cmake_minimum_required(VERSION 3.13)

project(FileManager)

option(SANITIZERS "Build with sanitizers" ON)

if (SANITIZERS)
    add_compile_options(-fsanitize=address -fsanitize=undefined -fsanitize=leak -fno-sanitize-recover)
    add_link_options(-fsanitize=address -fsanitize=undefined -fsanitize=leak -fno-sanitize-recover)
endif()

add_subdirectory(Libs)

add_executable(manager
    main.c
)

target_link_libraries(manager PRIVATE
    terminal
)