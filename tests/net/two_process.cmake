# Two processes talking over gos_net's real backend on this machine, on any
# platform: cmake -DNET_PING=<path> -P two_process.cmake
#
# execute_process runs its commands at the same time, as a pipeline. The pipe carries
# nothing that matters: the server reports on stderr, and the client on stdout.

execute_process(
    COMMAND "${NET_PING}" server 28200 40
    COMMAND "${NET_PING}" client 127.0.0.1 28200 3
    RESULTS_VARIABLE results
    TIMEOUT 60)
message(STATUS "server, client: ${results}")
if(NOT results STREQUAL "0;0")
    message(FATAL_ERROR "two-process run failed: ${results}")
endif()
