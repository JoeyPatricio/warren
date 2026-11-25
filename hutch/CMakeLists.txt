add_executable(hutch main.cpp queue.cpp server.cpp wake.cpp)
target_compile_options(hutch PRIVATE -Wall -Wextra)
target_link_libraries(hutch PRIVATE sqlite3)
