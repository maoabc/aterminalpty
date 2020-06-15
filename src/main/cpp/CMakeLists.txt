cmake_minimum_required(VERSION 3.4.1)

find_library(log-lib log)

set(PTY_FILES forkpty.c pty_exec.c)
add_library(pty SHARED ${PTY_FILES})
target_link_libraries(pty ${log-lib})
