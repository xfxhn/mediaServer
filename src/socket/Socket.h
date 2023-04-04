#pragma once

#ifdef _WIN32
#include <winsock.h>
#pragma comment(lib, "ws2_32.lib")
#endif

#if defined(__linux__) || defined(__APPLE__)

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

#define SOCKET int

#endif