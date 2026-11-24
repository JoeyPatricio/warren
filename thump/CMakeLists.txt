add_executable(thump main.cpp client.cpp commands/wake.cpp)
target_compile_options(thump PRIVATE -Wall -Wextra)
install(TARGETS thump)
