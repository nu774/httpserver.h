add_executable(
  unit-test-runner
  munit.c test_parser.c test_read_socket.c test_write_socket.c main.c
)

include(CheckFunctionExists)
check_function_exists(epoll_wait EPOLL)
check_function_exists(kqueue KQUEUE)

if(KQUEUE)
  target_compile_definitions(unit-test-runner PRIVATE "KQUEUE")
endif()

if(EPOLL)
  target_compile_definitions(unit-test-runner PRIVATE "EPOLL")
endif()

target_link_libraries(unit-test-runner PUBLIC httpsrv)

add_custom_command(TARGET unit-test-runner POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy
                   ${CMAKE_SOURCE_DIR}/test/unit/read_socket.txt
                   ${CMAKE_CURRENT_BINARY_DIR}/read_socket.txt)
