#ifndef WEBSERVE_COMMON_HPP_
#define WEBSERVE_COMMON_HPP_

#include <arpa/inet.h>
#include <cstdlib>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define APP_NAME "Webserve"

// Socket abstractions
#define SOCKET int
#define ISVALIDSOCKET(s) ((s) >= 0)
#define CLOSESOCKET(s) close(s)
#define GETSOCKETERRNO() (errno)

// Memory macros scaled to size_t for networking buffers
#define KIB(x) (static_cast<size_t>(x) << 10)
#define MIB(x) (static_cast<size_t>(x) << 20)
#define GIB(x) (static_cast<size_t>(x) << 30)

#ifndef DEBUG
#define DEBUG 1
#endif

#endif // WEBSERVE_COMMON_HPP_
