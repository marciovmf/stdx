/*
 * STDX - Cross-Platform Networking API
 * Part of the STDX General Purpose C Library by marciovmf
 * https://github.com/marciovmf/stdx
 *
 * Provides a high-level, cross-platform socket networking API for C:
 *   - Unified socket creation and management for IPv4/IPv6 (TCP/UDP)
 *   - Support for non-blocking I/O via select/poll
 *   - Address resolution and string formatting utilities
 *   - DNS resolution (hostname to IP)
 *   - Multicast and broadcast support
 *   - Network adapter enumeration and querying
 *   - Portable error handling with human-readable messages
 *
 * All functions use consistent types and naming (XSocket, XAddress, etc.).
 *
 * To compile the implementation, define:
 *     #define STDX_IMPLEMENTATION_NETWORK
 * in **one** source file before including this header.
 *
 * On windows, this library links with iphlpapi.lib and ws2_32.lib
 *
 * Author: marciovmf
 * License: MIT
 * Usage: #include "stdx_network.h"
 */

#ifndef XNET_H
#define XNET_H

#ifdef __cplusplus
extern "C"
{
#endif

#define STDX_NETWORK_VERSION_MAJOR 1
#define STDX_NETWORK_VERSION_MINOR 0
#define STDX_NETWORK_VERSION_PATCH 0

#define STDX_IO_VERSION (STDX_NETWORK_VERSION_MAJOR * 10000 + STDX_NETWORK_VERSION_MINOR * 100 + STDX_NETWORK_VERSION_PATCH)

#include <stdint.h>
#include <stdint.h>
#include <stdbool.h>

#if defined(_WIN32)
#include <winsock2.h>
#include <ws2tcpip.h>
  typedef SOCKET XSocket;
  typedef int32_t socklen_t;
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
  typedef int32_t XSocket;
#define INVALID_SOCKET (-1)
#endif

  typedef struct
  {
    int32_t family;
    socklen_t addrlen;
    struct sockaddr_storage addr;
  } XAddress;

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
    int8_t mac[18];      // MAC address XX:XX:XX:XX:XX:XX
    int8_t ipv4[16];     // IPv4 address
    int8_t ipv6[46];     // IPv6 address
    uint64_t speed_bps;// Link speed in bits per second (if known)
    bool is_wireless;  //
    int32_t mtu;           // MTU size
    uint32_t ifindex;
  } XNetAdapterInfo;

  // Core
  bool x_net_init(void);                                       // Initialize the networking subsystem.
  void x_net_shutdown(void);                                    // Clean up the networking subsystem.
  bool x_net_socket_is_valid(XSocket sock);                    // Check if a socket is valid.
  void x_net_close(XSocket sock);                               // Close a socket.
  int32_t  x_net_set_nonblocking(XSocket sock, int32_t nonblocking);    // Enable or disable non-blocking mode.

  // Socket Creation
  XSocket x_net_socket(XAddressFamily family, XSocketType type); // Create a socket with specified family and type.
  XSocket x_net_socket_tcp4(void);                               // Create an IPv4 TCP socket.
  XSocket x_net_socket_tcp6(void);                               // Create an IPv6 TCP socket.
  XSocket x_net_socket_udp4(void);                               // Create an IPv4 UDP socket.
  XSocket x_net_socket_udp6(void);                               // Create an IPv6 UDP socket.

  // Bind, Listen, Connect
  bool x_net_bind(XSocket sock, const XAddress* addr);          // Bind socket to a specific address.
  bool x_net_bind_any(XSocket sock, XAddressFamily family, uint16_t port); // Bind socket to any local address of a given family.
  bool x_net_bind_any_udp(XSocket sock);                        // Bind UDP socket to any IPv4 address.
  bool x_net_bind_any_udp6(XSocket sock);                       // Bind UDP socket to any IPv6 address.
  bool x_net_listen(XSocket sock, int32_t backlog);                 // Mark socket as listening for connections.
  XSocket x_net_accept(XSocket sock, XAddress* out_addr);       // Accept an incoming connection.
  int32_t x_net_connect(XSocket sock, const XAddress* addr);        // Connect to a remote address.

  // Send/Receive
  size_t x_net_send(XSocket sock, const void* buf, size_t len);     // Send data on a connected socket.
  size_t x_net_recv(XSocket sock, void* buf, size_t len);           // Receive data from a connected socket.
  size_t x_net_sendto(XSocket sock, const void* buf, size_t len, const XAddress* addr); // Send data to a specific address.
  size_t x_net_recvfrom(XSocket sock, void* buf, size_t len, XAddress* out_addr);      // Receive data from a socket and get sender's address.

  // Polling
  int32_t   x_net_select(XSocket* read_sockets, int32_t read_count, int32_t timeout_ms); // Wait for readability on multiple sockets.
  int32_t   x_net_poll(XSocket sock, int32_t events, int32_t timeout_ms);   // Wait for specific events on a socket.

  // Address Utilities
  bool  x_net_resolve(const int8_t* host, const int8_t* port, XAddressFamily family, XAddress* out_addr); // Resolve hostname and port to address.
  int32_t   x_net_parse_ip(XAddressFamily family, const int8_t* ip, void* out_addr); // Parse IP string into raw address.
  int32_t   x_net_format_address(const XAddress* addr, char* out_str, int32_t maxlen); // Format address as a string.

  void  x_net_address_clear(XAddress* addr); // Clear the XAddress struct (zero it out).
  void  x_net_address_any(XAddress* out_addr, int32_t family, uint16_t port); // Create "any" address for binding.
  int32_t   x_net_address_from_ip_port(const int8_t* ip, uint16_t port, XAddress* out_addr); // Parse IP and port into XAddress.
  int32_t   x_net_address_equal(const XAddress* a, const XAddress* b); // Compare two XAddress values.
  int32_t   x_net_address_to_string(const XAddress* addr, char* buf, int32_t buf_len); // Format XAddress to string (IP:port).

  // DNS
  int32_t   x_net_dns_resolve(const int8_t* hostname, XAddressFamily family, XAddress* out_addr); // Resolve hostname to address.

  // Multicast
  bool  x_net_join_multicast_ipv4(XSocket sock, const int8_t* group); // Join an IPv4 multicast group.
  bool  x_net_leave_multicast_ipv4(XSocket sock, const int8_t* group); // Leave an IPv4 multicast group.
  bool  x_net_join_multicast_ipv6(XSocket sock, const int8_t* multicast_ip, uint32_t ifindex); // Join an IPv6 multicast group.
  bool  x_net_leave_multicast_ipv6(XSocket sock, const int8_t* multicast_ip, uint32_t ifindex); // Leave an IPv6 multicast group.
  bool  x_net_join_multicast_ipv4_addr(XSocket sock, const XAddress* group_addr); // Join an IPv4 multicast group (using address struct).
  bool  x_net_leave_multicast_ipv4_addr(XSocket sock, const XAddress* group_addr); // Leave an IPv4 multicast group (using address struct).

  // Broadcast
  bool  x_net_enable_broadcast(XSocket sock, bool enable); // Enable or disable broadcast on a socket.

  // Adapter information
  int32_t   x_net_get_adapter_count(void); // Returns the number of network adapters on the system, or -1 on error.
  int32_t   x_net_list_adapters(XNetAdapter* out_adapters, int32_t max_adapters); // List all available network adapters. Returns the number found, or -1 on error.
  bool  x_net_get_adapter_info(const int8_t* name, XNetAdapterInfo* out_info); // Get detailed information about an adapter by name. Returns 0 on success, -1 on error.

  // Error handling
  int32_t   x_net_get_last_error(void); // Returns the last network error code for the current thread/process. On Windows, this returns WSAGetLastError(). On POSIX, it returns errno.
  int32_t   x_net_get_last_error_message(char* buf, int32_t buf_len);   // Writes a human-readable error message for the last network error into `buf`. Returns 0 on success, or -1 on failure (e.g., buffer too small).


#ifdef STDX_IMPLEMENTATION_NETWORK

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef _WIN32
  // Windows includes
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <windows.h>
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")

#else
  // Linux/Unix includes
#include <ifaddrs.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <netpacket/packet.h>
#endif

#define X_NET_POLLIN  0x01
#define X_NET_POLLOUT 0x02

  static bool x_net_initialized = false;

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

    IP_ADAPTER_ADDRESSES* adapters = (IP_ADAPTER_ADDRESSES*)malloc(size);
    if (!adapters) return -1;

    ret = GetAdaptersAddresses(AF_UNSPEC, 0, NULL, adapters, &size);
    if (ret != NO_ERROR)
    {
      free(adapters);
      return -1;
    }

    int32_t count = 0;
    IP_ADAPTER_ADDRESSES* adapter = adapters;
    while (adapter)
    {
      count++;
      adapter = adapter->Next;
    }
    free(adapters);
    return count;
  }

  int32_t x_net_list_adapters_win32(XNetAdapter* out_adapters, int32_t max_adapters)
  {
    ULONG size = 0;
    DWORD ret = GetAdaptersAddresses(AF_UNSPEC, 0, NULL, NULL, &size);
    if (ret != ERROR_BUFFER_OVERFLOW) return -1;

    IP_ADAPTER_ADDRESSES* adapters = (IP_ADAPTER_ADDRESSES*)malloc(size);
    if (!adapters) return -1;

    ret = GetAdaptersAddresses(AF_UNSPEC, 0, NULL, adapters, &size);
    if (ret != NO_ERROR)
    {
      free(adapters);
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
    free(adapters);
    return count;
  }

  int32_t x_net_get_adapter_info_win32(const int8_t* name, XNetAdapterInfo* out_info)
  {
    if (!name || !out_info) return -1;

    ULONG size = 0;
    DWORD ret = GetAdaptersAddresses(AF_UNSPEC, 0, NULL, NULL, &size);
    if (ret != ERROR_BUFFER_OVERFLOW) return -1;

    IP_ADAPTER_ADDRESSES* adapters = (IP_ADAPTER_ADDRESSES*)malloc(size);
    if (!adapters) return -1;

    ret = GetAdaptersAddresses(AF_UNSPEC, 0, NULL, adapters, &size);
    if (ret != NO_ERROR)
    {
      free(adapters);
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
        free(adapters);
        return 0;
      }
      adapter = adapter->Next;
    }

    free(adapters);
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

    freeifaddrs(ifaddr);
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

    freeifaddrs(ifaddr);
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
      freeifaddrs(ifaddr);
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

#endif // STDX_IMPLEMENTATION_NETWORK

#ifdef __cplusplus
}
#endif

#endif // XNET_H
