#pragma once

#ifdef _WIN32
    #define QEVORYX_PLATFORM_WINDOWS 1
    #define QEVORYX_PLATFORM_LINUX 0
#else
    #define QEVORYX_PLATFORM_WINDOWS 0
    #define QEVORYX_PLATFORM_LINUX 1
#endif

// ── Windows headers ──
#if QEVORYX_PLATFORM_WINDOWS
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <winsock2.h>
    #include <windows.h>
    #include <ws2tcpip.h>
    #include <iphlpapi.h>
    #include <io.h>
    #include <process.h>
    #ifdef _MSC_VER
        #pragma comment(lib, "Ws2_32.lib")
        #pragma comment(lib, "Iphlpapi.lib")
    #endif

    // Windows doesn't have close() for sockets — use closesocket()
    #define QEVORYX_CLOSESOCK closesocket

    // Windows doesn't have geteuid — always 0 (admin check done differently)
    #define QEVORYX_GETEUID() 0

    // Windows doesn't have raw IPPROTO_RAW in the same way
    // Use IPPROTO_IP with IP_HDRINCL
    #ifndef IPPROTO_RAW
        #define IPPROTO_RAW 47
    #endif

    // inet_pton/inet_ntop available in Vista+
    #if !defined(inet_pton)
        #include <ws2tcpip.h>
    #endif

// ── Linux headers ──
#else
    #include <unistd.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <ifaddrs.h>
    #include <net/if.h>
    #include <sys/types.h>

    #define QEVORYX_CLOSESOCK close
    #define QEVORYX_GETEUID() geteuid()
#endif
