cmake_minimum_required(VERSION 3.10)
project(code C)

set(CMAKE_C_STANDARD 99)

add_executable(code src/sender.c)
add_executable(codee src/receiver.c)
#add_executable(pliz src/socket/read_write_loop_receiver.c)
#add_executable(encor src/socket/read_write_loop_sender.c)