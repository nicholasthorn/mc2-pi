# Two processes talking over gos_net's real backend on this machine, on any
# platform: cmake -DNET_PING=<path> -P two_process.cmake
#
# execute_process runs its commands at the same time, as a pipeline. The pipe carries
# nothing that matters: the server reports on stderr, and the client on stdout.
#
# A port of this run's own: WSL is one machine shared by every session.

string(RANDOM LENGTH 4 ALPHABET 0123456789 suffix)
math(EXPR port "20000 + ${suffix}")
execute_process(
    COMMAND "${NET_PING}" server ${port} 40
    COMMAND "${NET_PING}" client 127.0.0.1 ${port} 3
    RESULTS_VARIABLE results
    TIMEOUT 60)
message(STATUS "server, client on port ${port}: ${results}")
if(NOT results STREQUAL "0;0")
    message(FATAL_ERROR "two-process run failed: ${results}")
endif()
