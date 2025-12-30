/**
 *  STDX - Cross-Platform Networking API
 *  Part of the STDX General Purpose C Library by marciovmf
 *  License: MIT
 *  <https://github.com/marciovmf/stdx>
 
 * ## Overview
 *
 * - Provides a high-level, cross-platform socket networking API.
 * - On windows, this library links with *iphlpapi.lib* and *ws2_32.lib*
 * 
 * ## How to compile
 * To compile the implementation define `X_IMPL_NET`
 * in **one** source file before including this header.
 *
 * To customize how this module allocates memory, define
 * `X_NET_ALLOC` / `X_NET_FREE` before including.
 */

#ifndef X_NETWORK_H
#define X_NETWORK_H

#define X_NETWORK_VERSION_MAJOR 1
#define X_NETWORK_VERSION_MINOR 0
#define X_NETWORK_VERSION_PATCH 0
#define X_NETWORK_VERSION (X_NETWORK_VERSION_MAJOR * 10000 + X_NETWORK_VERSION_MINOR * 100 + X_NETWORK_VERSION_PATCH)

#include <stdint.h>
#include <stdint.h>
#include <stdbool.h>

#if defined(_WIN32)
#include <winsock2.h>
  typedef SOCKET XSocket;
  typedef int32_t socklen_t;
#else
  typedef int32_t XSocket;
#define INVALID_SOCKET (-1)
#endif

#ifdef __cplusplus
extern "C" {
#endif

  typedef struct XAddress XAddress;

  typedef enum
  {
    X_NET_AF_IPV4 = 1,
    X_NET_AF_IPV6 = 2,
  } XAddressFamily;

  typedef enum
  {
    X_NET_SOCK_STREAM = 1,
    X_NET_SOCK_DGRAM = 2,
  } XSocketType;

  typedef struct
  {
    int8_t name[128];
  } XNetAdapter;

  typedef struct
  {
    int8_t name[128];
    int8_t description[256];
    int8_t mac[18];       /* MAC address XX:XX:XX:XX:XX:XX */
    int8_t ipv4[16];      /* IPv4 address */
    int8_t ipv6[46];      /* IPv6 address */
    uint64_t speed_bps;   /* Link speed in bits per second (if known) */
    bool is_wireless;
    int32_t mtu;
    uint32_t ifindex;
  } XNetAdapterInfo;

/**
* @brief Initialize the networking subsystem.
* @return True on success, false on failure.
*/
bool    x_net_init(void);

/**
* @brief Shut down the networking subsystem and release associated resources.
* @return Nothing.
*/
void    x_net_shutdown(void);

/**
* @brief Check whether a socket handle represents a valid socket.
* @param sock Socket handle to validate.
* @return True if the socket is valid, false otherwise.
*/
bool    x_net_socket_is_valid(XSocket sock);

/**
* @brief Close a socket handle.
* @param sock Socket handle to close.
* @return Nothing.
*/
void    x_net_close(XSocket sock);

/**
* @brief Enable or disable non-blocking mode on a socket.
* @param sock Socket handle to configure.
* @param nonblocking Non-zero to enable non-blocking mode, 0 to disable it.
* @return 0 on success, or -1 on error.
*/
int32_t x_net_set_nonblocking(XSocket sock, int32_t nonblocking);

/**
* @brief Create a socket with the specified address family and socket type.
* @param family Address family (e.g., IPv4/IPv6).
* @param type Socket type (e.g., TCP/UDP).
* @return New socket handle, or an invalid socket value on failure.
*/
XSocket x_net_socket(XAddressFamily family, XSocketType type);

/**
* @brief Create an IPv4 TCP socket.
* @return New socket handle, or an invalid socket value on failure.
*/
XSocket x_net_socket_tcp4(void);

/**
* @brief Create an IPv6 TCP socket.
* @return New socket handle, or an invalid socket value on failure.
*/
XSocket x_net_socket_tcp6(void);

/**
* @brief Create an IPv4 UDP socket.
* @return New socket handle, or an invalid socket value on failure.
*/
XSocket x_net_socket_udp4(void);

/**
* @brief Create an IPv6 UDP socket.
* @return New socket handle, or an invalid socket value on failure.
*/
XSocket x_net_socket_udp6(void);

/**
* @brief Bind a socket to a specific local address.
* @param sock Socket handle to bind.
* @param addr Local address to bind to.
* @return True on success, false on failure.
*/
bool    x_net_bind(XSocket sock, const XAddress* addr);

/**
* @brief Bind a socket to any local address of the given family and port.
* @param sock Socket handle to bind.
* @param family Address family to bind (e.g., IPv4/IPv6).
* @param port Local port to bind to.
* @return True on success, false on failure.
*/
bool    x_net_bind_any(XSocket sock, XAddressFamily family, uint16_t port);

/**
* @brief Bind a UDP socket to any local IPv4 address (system-chosen port/address if applicable).
* @param sock UDP socket handle to bind.
* @return True on success, false on failure.
*/
bool    x_net_bind_any_udp(XSocket sock);

/**
* @brief Bind a UDP socket to any local IPv6 address (system-chosen port/address if applicable).
* @param sock UDP socket handle to bind.
* @return True on success, false on failure.
*/
bool    x_net_bind_any_udp6(XSocket sock);

/**
* @brief Mark a bound socket as listening for incoming connections.
* @param sock Socket handle.
* @param backlog Maximum pending connection queue length.
* @return True on success, false on failure.
*/
bool    x_net_listen(XSocket sock, int32_t backlog);

/**
* @brief Accept an incoming connection on a listening socket.
* @param sock Listening socket handle.
* @param out_addr Optional output address receiving the peer address (may be NULL if supported).
* @return New connected socket handle, or an invalid socket value on failure.
*/
XSocket x_net_accept(XSocket sock, XAddress* out_addr);

/**
* @brief Connect a socket to a remote address.
* @param sock Socket handle to connect.
* @param addr Remote address to connect to.
* @return 0 on success, -1 on error, or a platform-defined in-progress code for non-blocking sockets.
*/
int32_t x_net_connect(XSocket sock, const XAddress* addr);

/**
* @brief Send data on a connected socket.
* @param sock Connected socket handle.
* @param buf Data to send.
* @param len Number of bytes to send.
* @return Number of bytes sent (may be less than len), or 0 on error/connection closed depending on implementation.
*/
size_t  x_net_send(XSocket sock, const void* buf, size_t len);

/**
* @brief Receive data from a connected socket.
* @param sock Connected socket handle.
* @param buf Output buffer to receive data.
* @param len Maximum number of bytes to read.
* @return Number of bytes received, or 0 on connection closed/error depending on implementation.
*/
size_t  x_net_recv(XSocket sock, void* buf, size_t len);

/**
* @brief Send data to a specific address (datagram sockets).
* @param sock Socket handle.
* @param buf Data to send.
* @param len Number of bytes to send.
* @param addr Destination address.
* @return Number of bytes sent, or 0 on error depending on implementation.
*/
size_t  x_net_sendto(XSocket sock, const void* buf, size_t len, const XAddress* addr);

/**
* @brief Receive data from a socket and optionally retrieve the sender address.
* @param sock Socket handle.
* @param buf Output buffer to receive data.
* @param len Maximum number of bytes to read.
* @param out_addr Output address receiving the sender address.
* @return Number of bytes received, or 0 on error/no data depending on implementation.
*/
size_t  x_net_recvfrom(XSocket sock, void* buf, size_t len, XAddress* out_addr);

/**
* @brief Wait for readability on multiple sockets.
* @param read_sockets Array of sockets to wait on for readability.
* @param read_count Number of sockets in read_sockets.
* @param timeout_ms Timeout in milliseconds (-1 for infinite, 0 for non-blocking poll).
* @return Number of sockets ready for reading, 0 on timeout, or -1 on error.
*/
int32_t x_net_select(XSocket* read_sockets, int32_t read_count, int32_t timeout_ms);

/**
* @brief Wait for specific events on a socket.
* @param sock Socket handle.
* @param events Bitmask of events to wait for (implementation-defined).
* @param timeout_ms Timeout in milliseconds (-1 for infinite, 0 for non-blocking poll).
* @return Event bitmask of occurred events, 0 on timeout, or -1 on error.
*/
int32_t x_net_poll(XSocket sock, int32_t events, int32_t timeout_ms);

/**
* @brief Resolve a host and service/port into a network address.
* @param host Hostname or address string.
* @param port Service name or port string.
* @param family Address family to resolve (e.g., IPv4/IPv6/unspecified).
* @param out_addr Output address receiving the resolved result.
* @return True on success, false on failure.
*/
bool    x_net_resolve(const int8_t* host, const int8_t* port, XAddressFamily family, XAddress* out_addr);

/**
* @brief Parse an IP string into a raw binary address.
* @param family Address family (IPv4 or IPv6).
* @param ip IP address string.
* @param out_addr Output buffer receiving the raw address bytes.
* @return 0 on success, or -1 on failure.
*/
int32_t x_net_parse_ip(XAddressFamily family, const int8_t* ip, void* out_addr);

/**
* @brief Format an address as a human-readable string.
* @param addr Address to format.
* @param out_str Output buffer receiving the formatted string.
* @param maxlen Size of out_str in bytes/chars.
* @return Number of characters written (excluding terminator), or -1 on failure.
*/
int32_t x_net_format_address(const XAddress* addr, char* out_str, int32_t maxlen);

/**
* @brief Clear an XAddress structure (set to zero).
* @param addr Address structure to clear.
* @return Nothing.
*/
void    x_net_address_clear(XAddress* addr);

/**
* @brief Create an "any" address for binding on the given family and port.
* @param out_addr Output address to fill.
* @param family Address family (implementation expects a family code).
* @param port Port to set.
* @return Nothing.
*/
void    x_net_address_any(XAddress* out_addr, int32_t family, uint16_t port);

/**
* @brief Build an XAddress from an IP string and port.
* @param ip IP address string.
* @param port Port number.
* @param out_addr Output address to fill.
* @return 0 on success, or -1 on failure.
*/
int32_t x_net_address_from_ip_port(const int8_t* ip, uint16_t port, XAddress* out_addr);

/**
* @brief Compare two XAddress values for equality.
* @param a First address.
* @param b Second address.
* @return Non-zero if equal, 0 if not equal.
*/
int32_t x_net_address_equal(const XAddress* a, const XAddress* b);

/**
* @brief Convert an XAddress to a string representation (typically "IP:port").
* @param addr Address to format.
* @param buf Output buffer receiving the string.
* @param buf_len Size of buf in bytes/chars.
* @return Number of characters written (excluding terminator), or -1 on failure.
*/
int32_t x_net_address_to_string(const XAddress* addr, char* buf, int32_t buf_len);

/**
* @brief Resolve a DNS hostname to an address.
* @param hostname Hostname to resolve.
* @param family Address family to resolve (e.g., IPv4/IPv6).
* @param out_addr Output address receiving the resolved result.
* @return 0 on success, or -1 on failure.
*/
int32_t x_net_dns_resolve(const int8_t* hostname, XAddressFamily family, XAddress* out_addr);

/**
* @brief Join an IPv4 multicast group by textual group address.
* @param sock Socket handle.
* @param group Multicast group address string (e.g., "239.0.0.1").
* @return True on success, false on failure.
*/
bool    x_net_join_multicast_ipv4(XSocket sock, const int8_t* group);

/**
* @brief Leave an IPv4 multicast group by textual group address.
* @param sock Socket handle.
* @param group Multicast group address string.
* @return True on success, false on failure.
*/
bool    x_net_leave_multicast_ipv4(XSocket sock, const int8_t* group);

/**
* @brief Join an IPv6 multicast group.
* @param sock Socket handle.
* @param multicast_ip Multicast group IPv6 address string.
* @param ifindex Network interface index to join on.
* @return True on success, false on failure.
*/
bool    x_net_join_multicast_ipv6(XSocket sock, const int8_t* multicast_ip, uint32_t ifindex);

/**
* @brief Leave an IPv6 multicast group.
* @param sock Socket handle.
* @param multicast_ip Multicast group IPv6 address string.
* @param ifindex Network interface index to leave on.
* @return True on success, false on failure.
*/
bool    x_net_leave_multicast_ipv6(XSocket sock, const int8_t* multicast_ip, uint32_t ifindex);

/**
* @brief Join an IPv4 multicast group using an XAddress structure.
* @param sock Socket handle.
* @param group_addr Multicast group address.
* @return True on success, false on failure.
*/
bool    x_net_join_multicast_ipv4_addr(XSocket sock, const XAddress* group_addr);

/**
* @brief Leave an IPv4 multicast group using an XAddress structure.
* @param sock Socket handle.
* @param group_addr Multicast group address.
* @return True on success, false on failure.
*/
bool    x_net_leave_multicast_ipv4_addr(XSocket sock, const XAddress* group_addr);

/**
* @brief Enable or disable broadcast capability on a socket.
* @param sock Socket handle.
* @param enable True to enable broadcast, false to disable.
* @return True on success, false on failure.
*/
bool    x_net_enable_broadcast(XSocket sock, bool enable);

/**
* @brief Get the number of network adapters available on the system.
* @return Number of adapters, or -1 on error.
*/
int32_t x_net_get_adapter_count(void);

/**
* @brief List available network adapters.
* @param out_adapters Output array receiving adapter entries.
* @param max_adapters Maximum number of adapters to write to out_adapters.
* @return Number of adapters written, or -1 on error.
*/
int32_t x_net_list_adapters(XNetAdapter* out_adapters, int32_t max_adapters);

/**
* @brief Retrieve detailed information about a network adapter by name.
* @param name Adapter name identifier.
* @param out_info Output structure receiving detailed adapter information.
* @return True on success, false on failure.
*/
bool    x_net_get_adapter_info(const int8_t* name, XNetAdapterInfo* out_info);

/**
* @brief Get the last network error code for the current thread/process.
* @return Last error code (WSAGetLastError() on Windows, errno on POSIX).
*/
int32_t x_net_get_last_error(void);

/**
* @brief Write a human-readable message for the last network error into a buffer.
* @param buf Output buffer to receive the message string.
* @param buf_len Size of buf in bytes/chars.
* @return 0 on success, or -1 on failure (e.g., buffer too small).
*/
int32_t x_net_get_last_error_message(char* buf, int32_t buf_len);
#ifdef __cplusplus
}
#endif

#ifdef X_IMPL_NETWORK

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <windows.h>
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")
#else
#include <ifaddrs.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <netpacket/packet.h>
#include <netinet/ip.h>
#include <netdb.h>
#endif

#ifndef X_NET_ALLOC
/**
 * @brief Internal macro for allocating memory.
 * To override how this header allocates memory, define this macro with a
 * different implementation before including this header.
 * @param sz  The size of memory to alloc.
 */
#define X_NET_ALLOC(sz)        malloc(sz)
#endif


#ifndef X_NET_FREE
/**
 * @brief Internal macro for freeing memory.
 * To override how this header frees memory, define this macro with a
 * different implementation before including this header.
 * @param p  The address of memory region to free.
 */
#define X_NET_FREE(p)          free(p)
#endif

#define X_NET_POLLIN  0x01
#define X_NET_POLLOUT 0x02

#ifdef __cplusplus
extern "C" {
#endif

  static bool x_net_initialized = false;

  struct XAddress
  {
    int32_t family;
    socklen_t addrlen;
    struct sockaddr_storage addr;
  };

  bool x_net_init(void)
  {
#if defined(_WIN32)
    if (x_net_initialized) return true;
    WSADATA wsaData;
    int32_t r = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (r == 0) x_net_initialized = true;
    return true;
#else
    return false;
#endif
  }

  void x_net_shutdown(void)
  {
    if (x_net_initialized)
    {
#if defined(_WIN32)
      WSACleanup();
#endif
      x_net_initialized = false;
    }
  }

  void x_net_close(XSocket sock)
  {
#if defined(_WIN32)
    closesocket(sock);
#else
    close(sock);
#endif
  }

  bool x_net_socket_is_valid(XSocket sock)
  {
#if defined(_WIN32)
    return sock != INVALID_SOCKET;
#else
    return sock >= 0;
#endif
  }

  int32_t x_net_set_nonblocking(XSocket sock, int32_t nonblocking)
  {
#if defined(_WIN32)
    u_long mode = nonblocking ? 1 : 0;
    return ioctlsocket(sock, FIONBIO, &mode);
#else

    int32_t flags = fcntl(sock, F_GETFL, 0);
    if (flags < 0) return -1;
    if (nonblocking)
      flags |= O_NONBLOCK;
    else
      flags &= ~O_NONBLOCK;
    return fcntl(sock, F_SETFL, flags);
#endif
  }

  // Socket Creation
  static int32_t x_net_family_to_af(XAddressFamily family)
  {
    switch(family)
    {
      case X_NET_AF_IPV4: return AF_INET;
      case X_NET_AF_IPV6: return AF_INET6;
      default: return AF_UNSPEC;
    }
  }

  static int32_t x_net_type_to_socktype(XSocketType type)
  {
    switch(type)
    {
      case X_NET_SOCK_STREAM: return SOCK_STREAM;
      case X_NET_SOCK_DGRAM: return SOCK_DGRAM;
      default: return 0;
    }
  }

  XSocket x_net_socket(XAddressFamily family, XSocketType type)
  {
    int32_t af = x_net_family_to_af(family);
    int32_t st = x_net_type_to_socktype(type);
    XSocket s = socket(af, st, 0);
    return s;
  }

  XSocket x_net_socket_tcp4(void)
  {
    return x_net_socket(X_NET_AF_IPV4, X_NET_SOCK_STREAM);
  }

  XSocket x_net_socket_tcp6(void)
  {
    return x_net_socket(X_NET_AF_IPV6, X_NET_SOCK_STREAM);
  }

  XSocket x_net_socket_udp4(void)
  {
    return x_net_socket(X_NET_AF_IPV4, X_NET_SOCK_DGRAM);
  }

  XSocket x_net_socket_udp6(void)
  {
    return x_net_socket(X_NET_AF_IPV6, X_NET_SOCK_DGRAM);
  }

  bool x_net_bind(XSocket sock, const XAddress* addr)
  {
    return bind(sock, (struct sockaddr*)&addr->addr, addr->addrlen) == 0;
  }

  bool x_net_bind_any(XSocket sock, XAddressFamily family, uint16_t port)
  {
    XAddress addr;
    x_net_address_any(&addr, family, port);
    return x_net_bind(sock, &addr);
  }

  bool x_net_bind_any_udp(XSocket sock)
  {
    return x_net_bind_any(sock, X_NET_AF_IPV4, 0);
  }

  bool x_net_bind_any_udp6(XSocket sock)
  {
    return x_net_bind_any(sock, X_NET_AF_IPV6, 0);
  }

  bool x_net_listen(XSocket sock, int32_t backlog)
  {
    return listen(sock, backlog) == 0;
  }

  XSocket x_net_accept(XSocket sock, XAddress* out_addr)
  {
    socklen_t addrlen = sizeof(out_addr->addr);
    XSocket client = accept(sock, (struct sockaddr*)&out_addr->addr, &addrlen);
    if (client != INVALID_SOCKET)
    {
      out_addr->family = out_addr->addr.ss_family;
      out_addr->addrlen = addrlen;
    }
    return client;
  }

  int32_t x_net_connect(XSocket sock, const XAddress* addr)
  {
    return connect(sock, (const struct sockaddr*)&addr->addr, addr->addrlen);
  }

  size_t x_net_send(XSocket sock, const void* buf, size_t len)
  {
    size_t sent = 
      send(sock, (const int8_t*)buf,
          (int) len, 0);
    return sent;
  }

  size_t x_net_recv(XSocket sock, void* buf, size_t len)
  {
    size_t recvd = recv(sock, (char*)buf, (int) len, 0);
    return recvd;
  }

  size_t x_net_sendto(XSocket sock, const void* buf, size_t len, const XAddress* addr)
  {
    size_t sent = sendto(sock, (const int8_t*)buf, (int) len, 0, (const struct sockaddr*)&addr->addr, addr->addrlen);
    return sent;
  }

  size_t x_net_recvfrom(XSocket sock, void* buf, size_t len, XAddress* out_addr)
  {
    socklen_t addrlen = sizeof(out_addr->addr);
    size_t recvd = recvfrom(sock, (char*)buf, (int) len, 0, (struct sockaddr*)&out_addr->addr, &addrlen);
    if (recvd >= 0)
    {
      out_addr->family = out_addr->addr.ss_family;
      out_addr->addrlen = addrlen;
    }
    return recvd;
  }

  int32_t x_net_select(XSocket* read_sockets, int32_t read_count, int32_t timeout_ms)
  {
    if (read_count <= 0 || read_sockets == NULL) return -1;

    fd_set readfds;
    FD_ZERO(&readfds);

    XSocket maxfd = 0;
    for (int i = 0; i < read_count; i++)
    {
      FD_SET(read_sockets[i], &readfds);
      if (read_sockets[i] > maxfd) maxfd = read_sockets[i];
    }

    struct timeval tv;
    if (timeout_ms >= 0)
    {
      tv.tv_sec = timeout_ms / 1000;
      tv.tv_usec = (timeout_ms % 1000) * 1000;
    }

#if defined(_WIN32)
    int32_t r = select(0, &readfds, NULL, NULL, (timeout_ms >= 0) ? &tv : NULL);
#else
    int32_t r = select((int)(maxfd + 1), &readfds, NULL, NULL, (timeout_ms >= 0) ? &tv : NULL);
#endif

    if (r <= 0) return r;

    int32_t ready_count = 0;
    for (int i = 0; i < read_count; i++)
    {
      if (FD_ISSET(read_sockets[i], &readfds))
        read_sockets[ready_count++] = read_sockets[i];
    }
    return ready_count;
  }

  int32_t x_net_poll(XSocket sock, int32_t events, int32_t timeout_ms)
  {
#if defined(_WIN32)
    WSAPOLLFD pfd;
    pfd.fd = sock;
    pfd.events = 0;
    if (events & X_NET_POLLIN) pfd.events |= POLLRDNORM;
    if (events & X_NET_POLLOUT) pfd.events |= POLLWRNORM;
    int32_t ret = WSAPoll(&pfd, 1, timeout_ms);
    if (ret <= 0) return ret;
    int32_t result = 0;
    if (pfd.revents & POLLRDNORM) result |= X_NET_POLLIN;
    if (pfd.revents & POLLWRNORM) result |= X_NET_POLLOUT;
    return result;
#else
    struct pollfd pfd;
    pfd.fd = sock;
    pfd.events = 0;
    if (events & X_NET_POLLIN) pfd.events |= POLLIN;
    if (events & X_NET_POLLOUT) pfd.events |= POLLOUT;
    int32_t ret = poll(&pfd, 1, timeout_ms);
    if (ret <= 0) return ret;
    int32_t result = 0;
    if (pfd.revents & POLLIN) result |= X_NET_POLLIN;
    if (pfd.revents & POLLOUT) result |= X_NET_POLLOUT;
    return result;
#endif
  }

  void x_net_address_clear(XAddress* addr)
  {
    if (!addr) return;
    memset(addr, 0, sizeof(*addr));
  }

  void x_net_address_any(XAddress* out_addr, int32_t family, uint16_t port)
  {
    x_net_address_clear(out_addr);
    if (family == X_NET_AF_IPV4)
    {
      struct sockaddr_in* sa = (struct sockaddr_in*)&out_addr->addr;
      sa->sin_family = AF_INET;
      sa->sin_addr.s_addr = INADDR_ANY;
      sa->sin_port = htons(port);
      out_addr->family = AF_INET;
      out_addr->addrlen = sizeof(struct sockaddr_in);
    } else if (family == X_NET_AF_IPV6)
    {
      struct sockaddr_in6* sa6 = (struct sockaddr_in6*)&out_addr->addr;
      sa6->sin6_family = AF_INET6;
      sa6->sin6_addr = in6addr_any;
      sa6->sin6_port = htons(port);
      out_addr->family = AF_INET6;
      out_addr->addrlen = sizeof(struct sockaddr_in6);
    } else
    {
      x_net_address_clear(out_addr);
    }
  }

  int32_t x_net_address_from_ip_port(const int8_t* ip, uint16_t port, XAddress* out_addr)
  {
    if (!ip || !out_addr) return -1;
    x_net_address_clear(out_addr);

    struct addrinfo hints, *res = NULL;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC; // IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM;

    int8_t port_str[16];
    snprintf(port_str, sizeof(port_str), "%u", port);

    int32_t err = getaddrinfo(ip, port_str, &hints, &res);
    if (err != 0 || !res)
    {
      return -1;
    }

    memcpy(&out_addr->addr, res->ai_addr, res->ai_addrlen);
    out_addr->addrlen = (socklen_t)res->ai_addrlen;
    out_addr->family = res->ai_family;

    freeaddrinfo(res);
    return 0;
  }

  int32_t x_net_address_equal(const XAddress* a, const XAddress* b)
  {
    if (!a || !b) return 0;
    if (a->family != b->family) return 0;

    if (a->family == AF_INET)
    {
      struct sockaddr_in* sa1 = (struct sockaddr_in*)&a->addr;
      struct sockaddr_in* sa2 = (struct sockaddr_in*)&b->addr;
      return sa1->sin_port == sa2->sin_port &&
        sa1->sin_addr.s_addr == sa2->sin_addr.s_addr;
    } else if (a->family == AF_INET6)
    {
      struct sockaddr_in6* sa1 = (struct sockaddr_in6*)&a->addr;
      struct sockaddr_in6* sa2 = (struct sockaddr_in6*)&b->addr;
      return sa1->sin6_port == sa2->sin6_port &&
        memcmp(&sa1->sin6_addr, &sa2->sin6_addr, sizeof(struct in6_addr)) == 0;
    }
    return 0;
  }

  int32_t x_net_address_to_string(const XAddress* addr, char* buf, int32_t buf_len)
  {
    if (!addr || !buf || buf_len <= 0) return -1;
    int8_t ipstr[INET6_ADDRSTRLEN];
    uint16_t port = 0;

    if (addr->family == AF_INET)
    {
      struct sockaddr_in* sa = (struct sockaddr_in*)&addr->addr;
      inet_ntop(AF_INET, &sa->sin_addr, ipstr, sizeof(ipstr));
      port = ntohs(sa->sin_port);
      return snprintf(buf, buf_len, "%s:%u", ipstr, port);
    } else if (addr->family == AF_INET6)
    {
      struct sockaddr_in6* sa6 = (struct sockaddr_in6*)&addr->addr;
      inet_ntop(AF_INET6, &sa6->sin6_addr, ipstr, sizeof(ipstr));
      port = ntohs(sa6->sin6_port);
      return snprintf(buf, buf_len, "[%s]:%u", ipstr, port);
    }
    return -1;
  }

  bool x_net_resolve(const int8_t* host, const int8_t* port, XAddressFamily family, XAddress* out_addr)
  {
    if (!host || !port || !out_addr) return -1;
    struct addrinfo hints, *res = NULL;
    memset(&hints, 0, sizeof(hints));

    hints.ai_family = (family == X_NET_AF_IPV4) ? AF_INET :
      (family == X_NET_AF_IPV6) ? AF_INET6 : AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    int32_t err = getaddrinfo(host, port, &hints, &res);
    if (err != 0 || !res) return false;

    memcpy(&out_addr->addr, res->ai_addr, res->ai_addrlen);
    out_addr->addrlen = (socklen_t)res->ai_addrlen;
    out_addr->family = res->ai_family;

    freeaddrinfo(res);
    return true;
  }

  int32_t x_net_parse_ip(XAddressFamily family, const int8_t* ip, void* out_addr)
  {
    if (!ip || !out_addr) return -1;
    int32_t af = (family == X_NET_AF_IPV4) ? AF_INET : AF_INET6;
    return inet_pton(af, ip, out_addr) == 1 ? 0 : -1;
  }

  int32_t x_net_format_address(const XAddress* addr, char* out_str, int32_t maxlen)
  {
    return x_net_address_to_string(addr, out_str, maxlen) != -1;
  }

  int32_t x_net_dns_resolve(const int8_t* hostname, XAddressFamily family, XAddress* out_addr)
  {
    return x_net_resolve(hostname, "0", family, out_addr);
  }

  bool x_net_join_multicast_ipv4(XSocket sock, const int8_t* group)
  {
    struct ip_mreq mreq;
    if (inet_pton(AF_INET, group, &mreq.imr_multiaddr) != 1) return -1;
    mreq.imr_interface.s_addr = INADDR_ANY;
    return setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (const int8_t*)&mreq, sizeof(mreq)) == 0;
  }

  bool x_net_leave_multicast_ipv4(XSocket sock, const int8_t* group)
  {
    struct ip_mreq mreq;
    if (inet_pton(AF_INET, group, &mreq.imr_multiaddr) != 1) return -1;
    mreq.imr_interface.s_addr = INADDR_ANY;
    return setsockopt(sock, IPPROTO_IP, IP_DROP_MEMBERSHIP, (const int8_t*)&mreq, sizeof(mreq)) == 0;
  }

  bool x_net_join_multicast_ipv6(XSocket sock, const int8_t* multicast_ip, uint32_t ifindex)
  {
    struct ipv6_mreq mreq;
    if (inet_pton(AF_INET6, multicast_ip, &mreq.ipv6mr_multiaddr) != 1) return -1;
    mreq.ipv6mr_interface = ifindex;
    return setsockopt(sock, IPPROTO_IPV6, IPV6_JOIN_GROUP, (const int8_t*)&mreq, sizeof(mreq)) == 0;
  }

  bool x_net_leave_multicast_ipv6(XSocket sock, const int8_t* multicast_ip, uint32_t ifindex)
  {
    struct ipv6_mreq mreq;
    if (inet_pton(AF_INET6, multicast_ip, &mreq.ipv6mr_multiaddr) != 1) return -1;
    mreq.ipv6mr_interface = ifindex;
    return setsockopt(sock, IPPROTO_IPV6, IPV6_LEAVE_GROUP, (const int8_t*)&mreq, sizeof(mreq)) == 0;
  }

  bool x_net_join_multicast_ipv4_addr(XSocket sock, const XAddress* group_addr)
  {
    if (!group_addr || group_addr->family != AF_INET) return -1;
    struct ip_mreq mreq;
    mreq.imr_multiaddr = ((struct sockaddr_in*)&group_addr->addr)->sin_addr;
    mreq.imr_interface.s_addr = INADDR_ANY;
    return setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (const int8_t*)&mreq, sizeof(mreq)) == 0;
  }

  bool x_net_leave_multicast_ipv4_addr(XSocket sock, const XAddress* group_addr)
  {
    if (!group_addr || group_addr->family != AF_INET) return -1;
    struct ip_mreq mreq;
    mreq.imr_multiaddr = ((struct sockaddr_in*)&group_addr->addr)->sin_addr;
    mreq.imr_interface.s_addr = INADDR_ANY;
    return setsockopt(sock, IPPROTO_IP, IP_DROP_MEMBERSHIP, (const int8_t*)&mreq, sizeof(mreq)) == 0;
  }

  bool x_net_enable_broadcast(XSocket sock, bool enable)
  {
    int32_t val = enable ? 1 : 0;
    return setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (const int8_t*)&val, sizeof(val)) == 0;
  }

  // Adapter info functions: platform-specific stubs (return -1 to indicate not implemented)

#if defined(_WIN32)

  int32_t x_net_get_adapter_count_win32(void)
  {
    ULONG size = 0;
    DWORD ret = GetAdaptersAddresses(AF_UNSPEC, 0, NULL, NULL, &size);
    if (ret != ERROR_BUFFER_OVERFLOW) return -1;

    IP_ADAPTER_ADDRESSES* adapters = (IP_ADAPTER_ADDRESSES*)X_NET_ALLOC(size);
    if (!adapters) return -1;

    ret = GetAdaptersAddresses(AF_UNSPEC, 0, NULL, adapters, &size);
    if (ret != NO_ERROR)
    {
      X_NET_FREE(adapters);
      return -1;
    }

    int32_t count = 0;
    IP_ADAPTER_ADDRESSES* adapter = adapters;
    while (adapter)
    {
      count++;
      adapter = adapter->Next;
    }
    X_NET_FREE(adapters);
    return count;
  }

  int32_t x_net_list_adapters_win32(XNetAdapter* out_adapters, int32_t max_adapters)
  {
    ULONG size = 0;
    DWORD ret = GetAdaptersAddresses(AF_UNSPEC, 0, NULL, NULL, &size);
    if (ret != ERROR_BUFFER_OVERFLOW) return -1;

    IP_ADAPTER_ADDRESSES* adapters = (IP_ADAPTER_ADDRESSES*)X_NET_ALLOC(size);
    if (!adapters) return -1;

    ret = GetAdaptersAddresses(AF_UNSPEC, 0, NULL, adapters, &size);
    if (ret != NO_ERROR)
    {
      X_NET_FREE(adapters);
      return -1;
    }

    int32_t count = 0;
    IP_ADAPTER_ADDRESSES* adapter = adapters;
    while (adapter && count < max_adapters)
    {
      strncpy(out_adapters[count].name, adapter->AdapterName, sizeof(out_adapters[count].name) - 1);
      out_adapters[count].name[sizeof(out_adapters[count].name) - 1] = '\0';
      count++;
      adapter = adapter->Next;
    }
    X_NET_FREE(adapters);
    return count;
  }

  int32_t x_net_get_adapter_info_win32(const int8_t* name, XNetAdapterInfo* out_info)
  {
    if (!name || !out_info) return -1;

    ULONG size = 0;
    DWORD ret = GetAdaptersAddresses(AF_UNSPEC, 0, NULL, NULL, &size);
    if (ret != ERROR_BUFFER_OVERFLOW) return -1;

    IP_ADAPTER_ADDRESSES* adapters = (IP_ADAPTER_ADDRESSES*)X_NET_ALLOC(size);
    if (!adapters) return -1;

    ret = GetAdaptersAddresses(AF_UNSPEC, 0, NULL, adapters, &size);
    if (ret != NO_ERROR)
    {
      X_NET_FREE(adapters);
      return -1;
    }

    IP_ADAPTER_ADDRESSES* adapter = adapters;
    while (adapter)
    {
      if (strcmp(adapter->AdapterName, name) == 0)
      {
        // Copy adapter name and description
        strncpy(out_info->name, adapter->AdapterName, sizeof(out_info->name) - 1);
        out_info->name[sizeof(out_info->name) - 1] = '\0';
        out_info->is_wireless = (adapter->IfType == IF_TYPE_IEEE80211) ? 1 : 0;
        out_info->mtu = (int)adapter->Mtu;
        out_info->ifindex = (int)adapter->IfIndex;
        out_info->speed_bps = adapter->TransmitLinkSpeed;
        // Convert description from WCHAR to char
        int32_t desc_len = WideCharToMultiByte(CP_UTF8, 0, adapter->Description, -1, out_info->description, sizeof(out_info->description), NULL, NULL);
        if (desc_len == 0) out_info->description[0] = '\0';

        // MAC address
        if (adapter->PhysicalAddressLength == 6)
        {
          snprintf(out_info->mac, sizeof(out_info->mac), "%02X:%02X:%02X:%02X:%02X:%02X",
              adapter->PhysicalAddress[0], adapter->PhysicalAddress[1],
              adapter->PhysicalAddress[2], adapter->PhysicalAddress[3],
              adapter->PhysicalAddress[4], adapter->PhysicalAddress[5]);
        } else
        {
          out_info->mac[0] = '\0';
        }

        // Initialize IP strings empty
        out_info->ipv4[0] = '\0';
        out_info->ipv6[0] = '\0';

        IP_ADAPTER_UNICAST_ADDRESS* addr = adapter->FirstUnicastAddress;
        while (addr)
        {
          SOCKADDR* sockaddr = addr->Address.lpSockaddr;
          int8_t buf[INET6_ADDRSTRLEN] =
          {0};
          if (sockaddr->sa_family == AF_INET)
          {
            struct sockaddr_in* sa_in = (struct sockaddr_in*)sockaddr;
            inet_ntop(AF_INET, &(sa_in->sin_addr), buf, sizeof(buf));
            strncpy(out_info->ipv4, buf, sizeof(out_info->ipv4) - 1);
            out_info->ipv4[sizeof(out_info->ipv4) - 1] = '\0';
          } else if (sockaddr->sa_family == AF_INET6)
          {
            struct sockaddr_in6* sa_in6 = (struct sockaddr_in6*)sockaddr;
            inet_ntop(AF_INET6, &(sa_in6->sin6_addr), buf, sizeof(buf));
            strncpy(out_info->ipv6, buf, sizeof(out_info->ipv6) - 1);
            out_info->ipv6[sizeof(out_info->ipv6) - 1] = '\0';
          }
          addr = addr->Next;
        }
        X_NET_FREE(adapters);
        return 0;
      }
      adapter = adapter->Next;
    }

    X_NET_FREE(adapters);
    return -1; // Not found
  }

#else

  int32_t x_net_get_adapter_count_posix(void)
  {
    struct ifaddrs* ifaddr;
    if (getifaddrs(&ifaddr) == -1) return -1;

    int32_t count = 0;
    struct ifaddrs* ifa = ifaddr;
    int8_t last_name[IFNAMSIZ] =
    {0};
    while (ifa)
    {
      if (ifa->ifa_name && strcmp(last_name, ifa->ifa_name) != 0)
      {
        strncpy(last_name, ifa->ifa_name, IFNAMSIZ-1);
        count++;
      }
      ifa = ifa->ifa_next;
    }

    X_NET_FREEifaddrs(ifaddr);
    return count;
  }

  int32_t x_net_list_adapters_posix(XNetAdapter* out_adapters, int32_t max_adapters)
  {
    struct ifaddrs* ifaddr;
    if (getifaddrs(&ifaddr) == -1) return -1;

    int32_t count = 0;
    int8_t last_name[IFNAMSIZ] =
    {0};

    struct ifaddrs* ifa = ifaddr;
    while (ifa && count < max_adapters)
    {
      if (ifa->ifa_name && strcmp(last_name, ifa->ifa_name) != 0)
      {
        strncpy(last_name, ifa->ifa_name, IFNAMSIZ - 1);
        strncpy(out_adapters[count].name, ifa->ifa_name, sizeof(out_adapters[count].name) - 1);
        out_adapters[count].name[sizeof(out_adapters[count].name) - 1] = '\0';
        count++;
      }
      ifa = ifa->ifa_next;
    }

    X_NET_FREEifaddrs(ifaddr);
    return count;
  }

  int32_t x_net_get_adapter_info_posix(const int8_t* name, XNetAdapterInfo* out_info)
  {
    if (!name || !out_info) return -1;

    struct ifaddrs* ifaddr;
    if (getifaddrs(&ifaddr) == -1) return -1;

    memset(out_info, 0, sizeof(*out_info));
    strncpy(out_info->name, name, sizeof(out_info->name) - 1);

    // MAC address retrieval via SIOCGIFHWADDR (socket ioctl)
    int32_t sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0)
    {
      X_NET_FREEifaddrs(ifaddr);
      return -1;
    }

    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, name, IFNAMSIZ - 1);
    if (ioctl(sock, SIOCGIFHWADDR, &ifr) == 0)
    {
      uint8_t* mac = (uint8_t*)ifr.ifr_hwaddr.sa_data;
      snprintf(out_info->mac, sizeof(out_info->mac), "%02X:%02X:%02X:%02X:%02X:%02X",
          mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    } else
    {
      out_info->mac[0] = '\0';
    }
    close(sock);

    // IP addresses
    struct ifaddrs* ifa = ifaddr;
    while (ifa)
    {
      if (ifa->ifa_name && strcmp(ifa->ifa_name, name) == 0 && ifa->ifa_addr)
      {
        int32_t family = ifa->ifa_addr->sa_family;
        int8_t buf[INET6_ADDRSTRLEN] =
        {0};
        out_info->ifindex = ifa->IfIndex;
        out_info->is_wireless = (ifa->IfType == IF_TYPE_IEEE80211) ? 1 : 0;
        out_info->mtu = (int) ifa->Mtu;
        out_info->speed_bps = ifa->TransmitLinkSpeed;

        if (family == AF_INET)
        {
          struct sockaddr_in* sa = (struct sockaddr_in*)ifa->ifa_addr;
          inet_ntop(AF_INET, &(sa->sin_addr), buf, sizeof(buf));
          strncpy(out_info->ipv4, buf, sizeof(out_info->ipv4) - 1);
          out_info->ipv4[sizeof(out_info->ipv4) - 1] = '\0';
        } else if (family == AF_INET6)
        {
          struct sockaddr_in6* sa6 = (struct sockaddr_in6*)ifa->ifa_addr;
          inet_ntop(AF_INET6, &(sa6->sin6_addr), buf, sizeof(buf));
          strncpy(out_info->ipv6, buf, sizeof(out_info->ipv6) - 1);
          out_info->ipv6[sizeof(out_info->ipv6) - 1] = '\0';
        }
      }
      ifa = ifa->ifa_next;
    }

    freeifaddrs(ifaddr);
    return 0;
  }
#endif

  int32_t x_net_get_adapter_count(void)
  {
#if defined(_WIN32)
    return x_net_get_adapter_count_win32();
#else
    return x_net_get_adapter_count_posix();
#endif
  }

  int32_t x_net_list_adapters(XNetAdapter* out_adapters, int32_t max_adapters)
  {
#if defined(_WIN32)
    return x_net_list_adapters_win32(out_adapters, max_adapters);
#else
    return x_net_list_adapters_posix(out_adapters, max_adapters);
#endif
  }

  bool x_net_get_adapter_info(const int8_t* name, XNetAdapterInfo* out_info)
  {
#if defined(_WIN32)
    return x_net_get_adapter_info_win32(name, out_info) == 0;
#else
    return x_net_get_adapter_info_posix(name, out_info) == 0;
#endif
  }

  int32_t x_net_get_last_error(void)
  {
#if defined(_WIN32)
    return WSAGetLastError();
#else
    return errno;
#endif
  }

  int32_t x_net_get_last_error_message(char* buf, int32_t buf_len)
  {
    if (!buf || buf_len <= 0) return -1;

#ifdef _WIN32
    DWORD err = WSAGetLastError();
    if (FormatMessageA(
          FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
          NULL,
          err,
          MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
          buf,
          (DWORD)buf_len,
          NULL) == 0)

    {
      snprintf(buf, buf_len, "Unknown Windows error %lu", err);
      return -1;
    }
    // Strip trailing newline from FormatMessage
    size_t len = strlen(buf);
    if (len > 1 && buf[len - 1] == '\n') buf[len - 1] = '\0';
    if (len > 2 && buf[len - 2] == '\r') buf[len - 2] = '\0';
    return 0;

#else
    int32_t err = errno;
    const int8_t* msg = strerror(err);
    if (!msg)
    {
      snprintf(buf, buf_len, "Unknown error %d", err);
      return -1;
    }
    strncpy(buf, msg, buf_len - 1);
    buf[buf_len - 1] = '\0';
    return 0;
#endif
  }

#ifdef __cplusplus
}
#endif

#endif // X_IMPL_NETWORK
#endif // X_NETWORK_H
