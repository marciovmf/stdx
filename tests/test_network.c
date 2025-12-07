#define X_IMPL_TEST
#include <stdx_test.h>
#define X_IMPL_NETWORK
#include <stdx_network.h>
#include <string.h>
#include <stdio.h>

#ifdef _WIN32
typedef long ssize_t;
#endif

#define TEST_ENABLE_OUTPUT 0

int test_tcp_socket() {
  XSocket sock = x_net_socket_tcp4();
  ASSERT_TRUE(x_net_socket_is_valid(sock));
  x_net_close(sock);
  return 0;
}

int test_udp_socket() {
  XSocket sock = x_net_socket_udp4();
  ASSERT_TRUE(x_net_socket_is_valid(sock));
  x_net_close(sock);
  return 0;
}

int test_address_resolution() {
  XAddress addr;
  char str[128];

  ASSERT_TRUE(x_net_resolve("localhost", "80", X_NET_AF_IPV4, &addr));
  ASSERT_TRUE(x_net_format_address(&addr, str, sizeof(str)) > 0);
#if TEST_ENABLE_OUTPUT
  printf("Resolved 'localost' address: %s\n", str);
#endif
  ASSERT_TRUE(x_net_dns_resolve("google.com", X_NET_AF_IPV4, &addr));
  ASSERT_TRUE(x_net_format_address(&addr, str, sizeof(str)) > 0);
#if TEST_ENABLE_OUTPUT
  printf("Resolved 'google.com' address: %s\n", str);
#endif

  return 0;
}

int test_bind_listen_accept() {
  XSocket listener = x_net_socket_tcp4();
  XAddress addr;
  ASSERT_TRUE(x_net_socket_is_valid(listener));
  ASSERT_TRUE(x_net_resolve("127.0.0.1", "12345", X_NET_AF_IPV4, &addr));
  ASSERT_TRUE(x_net_bind(listener, &addr));
  ASSERT_TRUE(x_net_listen(listener, 5));
  x_net_close(listener);
  return 0;
}

int test_udp_send_recv() {
  XSocket sender = x_net_socket_udp4();
  XSocket receiver = x_net_socket_udp4();
  XAddress addr;
  ASSERT_TRUE(x_net_resolve("127.0.0.1", "23456", X_NET_AF_IPV4, &addr));
  ASSERT_TRUE(x_net_bind(receiver, &addr));

  const char* msg = "Hello, UDP!";
  ASSERT_TRUE(x_net_sendto(sender, msg, strlen(msg), &addr) > 0);

  char buf[128];
  XAddress recv_addr;
  size_t len = x_net_recvfrom(receiver, buf, sizeof(buf), &recv_addr);
  ASSERT_TRUE(len > 0);
  buf[len] = 0;
#if TEST_ENABLE_OUTPUT
  printf("Received: %s\n", buf);
#endif

  x_net_close(sender);
  x_net_close(receiver);
  return 0;
}

int test_multicast_ipv4()
{
  XSocket sock = x_net_socket_udp4();
  ASSERT_TRUE(x_net_socket_is_valid(sock));
  ASSERT_TRUE(x_net_enable_broadcast(sock, true));
  ASSERT_TRUE(x_net_join_multicast_ipv4(sock, "224.0.0.1"));
  ASSERT_TRUE(x_net_leave_multicast_ipv4(sock, "224.0.0.1"));
  x_net_close(sock);
  return 0;
}

int test_multicast_ipv6()
{
  // NOTE(marciovmf): For this test we use the firt available network adapter. Might not work on all systems.
  // TODO(marciovmf): Improve this test so it finds an adapter that can actually join an IPV6 multicast group
  XNetAdapterInfo info = {0};
  XNetAdapter adapter = {0};
  ASSERT_TRUE(x_net_list_adapters(&adapter, 1));
  ASSERT_TRUE(x_net_get_adapter_info(adapter.name, &info));
  unsigned int ifindex = info.ifindex;

  XSocket sock = x_net_socket_udp6();
  ASSERT_TRUE(x_net_socket_is_valid(sock));
  ASSERT_TRUE(x_net_join_multicast_ipv6(sock, "ff01::1", ifindex));
  ASSERT_TRUE(x_net_leave_multicast_ipv6(sock, "ff01::1", ifindex));
  x_net_close(sock);
  return 0;
}

int test_get_adapters_info(void)
{
  int num_adapters = x_net_get_adapter_count();
  ASSERT_TRUE(num_adapters > 0);

  XNetAdapter* adapters = (XNetAdapter*) malloc(sizeof(XNetAdapter)* num_adapters);
  ASSERT_TRUE(x_net_list_adapters(adapters, num_adapters));
  for (int i = 0; i < num_adapters; i++)
  {
    XNetAdapter* adapter_name = &adapters[i];
    XNetAdapterInfo info = {0};
    ASSERT_TRUE(x_net_get_adapter_info(adapter_name->name, &info));
#if TEST_ENABLE_OUTPUT
    printf("\tAdapter name='%s':%s, mac=%s, ipv4=%s, ipv6=%s, mtu=%d, is_wireless=%d, ifindex=%d, speed_bps=%zu\n",
        info.name,
        info.description,
        info.mac,
        info.ipv4,
        info.ipv6,
        info.mtu,
        info.is_wireless,
        info.ifindex,
        info.speed_bps
        );
#endif
  }
  return 0;
}

int main()
{
  ASSERT_TRUE(x_net_init());

  STDXTestCase tests[] =
  {
    X_TEST(test_get_adapters_info),
    X_TEST(test_tcp_socket),
    X_TEST(test_udp_socket),
    X_TEST(test_bind_listen_accept),
    X_TEST(test_address_resolution),
    X_TEST(test_udp_send_recv),
    X_TEST(test_multicast_ipv4),
    X_TEST(test_multicast_ipv6),
  };

  int result = stdx_run_tests(tests, sizeof(tests)/sizeof(tests[0]));
  x_net_shutdown();
  return result;
}
