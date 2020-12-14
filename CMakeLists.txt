cmake_minimum_required(VERSION 3.13)

project(uartmode)
add_subdirectory(libtoml)
add_executable(uartmode uartmode.c)
target_link_libraries(uartmode toml::toml)
install(TARGETS uartmode RUNTIME DESTINATION sbin)

project(rd_exbadc)
add_executable(rd_exbadc ads1115.h ads1115.c ads1115_example.c)
target_link_libraries(rd_exbadc gpiod)
install(TARGETS rd_exbadc RUNTIME DESTINATION bin)

project(can_read)
add_executable(can_read can_read.c)
install(TARGETS can_read RUNTIME DESTINATION sbin)

project(can_write)
add_executable(can_write can_write.c)
install(TARGETS can_write RUNTIME DESTINATION sbin)
