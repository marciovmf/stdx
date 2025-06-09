#include <stdx_common.h>
#define STDX_IMPLEMENTATION_STRING
#define STDX_IMPLEMENTATION_NETWORK
#define STDX_IMPLEMENTATION_FILESYSTEM
#define STDX_IMPLEMENTATION_THREAD
#define STDX_IMPLEMENTATION_IO
#define STDX_IMPLEMENTATION_LOG

#include <stdx_string.h>
#include <stdx_network.h>
#include <stdx_filesystem.h>
#include <stdx_thread.h>
#include <stdx_io.h>
#include <stdx_log.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PORT 8080
#define DOC_ROOT "C:\\docs"
#define BUFFER_SIZE 8192


#define HTML_ERROR_PAGE(NUMBER, MSG) \
"<!DOCTYPE html>\n" \
"<html lang=\"en\">\n" \
"<head>\n" \
"  <meta charset=\"UTF-8\">\n" \
"  <title>Error " #NUMBER "</title>\n" \
"  <style>\n" \
"    body { font-family: sans-serif; background: #fff3f3; color: #990000; display: flex; align-items: center; justify-content: center; height: 100vh; margin: 0; text-align: center; }\n" \
"    .box { border: 2px dashed #990000; padding: 2em; border-radius: 8px; background: #fff; box-shadow: 0 0 10px #ddd; }\n" \
"    h1 { margin: 0 0 0.5em; }\n" \
"    p { margin: 0; font-size: 1.1em; }\n" \
"  </style>\n" \
"</head>\n" \
"<body>\n" \
"  <div class=\"box\">\n" \
"    <h1>Error " #NUMBER "</h1>\n" \
"    <p>" MSG "</p>\n" \
"  </div>\n" \
"</body>\n" \
"</html>\n"

const char* get_mime_type(const char* path)
{
  const char* ext = strrchr(path, '.');
  if (!ext) return "application/octet-stream";
  ext++;
  if (strcmp(ext, "html") == 0) return "text/html";
  if (strcmp(ext, "css") == 0) return "text/css";
  if (strcmp(ext, "js") == 0) return "application/javascript";
  if (strcmp(ext, "png") == 0) return "image/png";
  if (strcmp(ext, "jpg") == 0 || strcmp(ext, "jpeg") == 0) return "image/jpeg";
  if (strcmp(ext, "gif") == 0) return "image/gif";
  if (strcmp(ext, "txt") == 0) return "text/plain";
  return "application/octet-stream";
}

void send_response(XSocket client, int status, const char* content_type, const char* body, size_t body_len)
{
  char header[1024];
  int len = snprintf(header, sizeof(header),
      "HTTP/1.1 %d OK\r\n"
      "Content-Type: %s\r\n"
      "Content-Length: %zu\r\n"
      "Connection: close\r\n"
      "\r\n",
      status, content_type, body_len);

  x_net_send(client, header, len);
  x_net_send(client, body, body_len);
}

void send_file_response(XSocket client, const char* filepath)
{

  if (!x_fs_path_is_file_cstr(filepath))
  {
    const char *not_found = HTML_ERROR_PAGE(404, "The page you are looking for was not found.");
    send_response(client, 404, "text/plain", not_found, strlen(not_found));
    return;
  }

  size_t size = 0;
  const char* buffer = x_io_read_text(filepath, &size, NULL);

  if (!buffer)
  {
    const char* err = "500 Internal Server Error";
    send_response(client, 500, "text/plain", err, strlen(err));
    return;
  }

  const char* mime_type = get_mime_type(filepath);
  send_response(client, 200, mime_type, buffer, size);
  free((void*) buffer);
}

bool is_path_safe(const char* base_path, const char* requested_path)
{
  //  XFSPath docroot;
  //  x_fs_path(&docroot, base_path);
  //  if (x_fs_path_common_prefix(base_path, requested_path, &docroot) < docroot.length)
  //    return false;
  return true;
}

void send_directory_listing(XSocket client, const char* dirpath, const char* url_path)
{
  XFSDireEntry entry;
  XFSDireHandle* handle = x_fs_find_first_file(dirpath, &entry);
  if (!handle)
  {
    const char* err = "500 Internal Server Error";
    send_response(client, 500, "text/plain", err, strlen(err));
    return;
  }

  char html[BUFFER_SIZE];
  size_t offset = snprintf(html, sizeof(html),
      "<html><body><h1>Index of %s</h1><ul>", url_path);

  do
  {
    if (strcmp(entry.name, ".") == 0 || strcmp(entry.name, "..") == 0) continue;
    offset += snprintf(html + offset, sizeof(html) - offset,
        "<li><a href=\"%s/%s\">%s</a></li>", url_path, entry.name, entry.name);
  } while (x_fs_find_next_file(handle, &entry));

  x_fs_find_close(handle);

  offset += snprintf(html + offset, sizeof(html) - offset, "</ul></body></html>");
  send_response(client, 200, "text/html", html, offset);
}

void handle_directory_request(XSocket client, const char* dirpath, const char* url_path)
{
  char index_path[MAX_PATH];
  snprintf(index_path, sizeof(index_path), "%s\\index.html", dirpath);
  if (x_fs_path_exists_cstr(index_path) && x_fs_path_is_file_cstr(index_path))
  {
    send_file_response(client, index_path);
  } else
  {
    send_directory_listing(client, dirpath, url_path);
  }
}

void handle_client(XSocket client)
{
  char buffer[BUFFER_SIZE] =
  {0};
  size_t received = x_net_recv(client, buffer, sizeof(buffer) - 1);
  x_log_info("Received %zu bytes: %.*s", received, (int)received, buffer);

  if (received <= 0)
  {
    x_log_error("No data received.\n");
    return;
  }

  // identify HTTP method
  char method[8], path[512];
  sscanf(buffer, "%7s %511s", method, path);

  if (strcmp(method, "GET") != 0)
  {
    const char* not_implemented = "501 Not Implemented";
    send_response(client, 501, "text/plain", not_implemented, strlen(not_implemented));
    return;
  }

  // compose the document full path
  XFSPath fullpath;
  x_fs_path(&fullpath, DOC_ROOT, path);
  x_fs_path_normalize(&fullpath);

  if (!is_path_safe(DOC_ROOT, x_fs_path_cstr(&fullpath)))
  {
    const char* forbidden = "403 Forbidden";
    send_response(client, 403, "text/html", forbidden, strlen(forbidden));
    return;
  }

  if (!x_fs_path_exists(&fullpath))
  {
    const char *not_found = HTML_ERROR_PAGE(404, "The page you are looking for was not found.");
    send_response(client, 404, "text/html", not_found, strlen(not_found));
    return;
  }

  if (x_fs_path_is_directory(&fullpath))
  {
    handle_directory_request(client, x_fs_path_cstr(&fullpath), path);
  }
  else
  {
    send_file_response(client, x_fs_path_cstr(&fullpath));
  }
}

static void* client_thread(void* arg)
{
  XSocket client = *(XSocket*)arg;
  free(arg);
  handle_client(client);
  x_net_close(client);
  return NULL;
}

int main()
{
  if (!x_net_init())
  {
    x_log_fatal("Failed to initialize networking.");
    return 1;
  }

  XSocket server = x_net_socket_tcp4();
  if (!x_net_socket_is_valid(server))
  {
    fprintf(stderr, "Failed to create socket.");
    return 1;
  }

  if (!x_net_bind_any(server, X_NET_AF_IPV4, PORT) || !x_net_listen(server, 10))
  {
    fprintf(stderr, "Failed to bind/listen on port %d.", PORT);
    return 1;
  }

  x_log_info("Serving HTTP on port %d from %s...", PORT, DOC_ROOT);

  while (1)
  {
    XAddress client_addr;
    XSocket client = x_net_accept(server, &client_addr);
    if (!x_net_socket_is_valid(client)) continue;

    XSocket* client_ptr = malloc(sizeof(XSocket));
    *client_ptr = client;

    XThread *t;
    if (x_thread_create(&t, client_thread, client_ptr) != 0)
    {
      x_log_error("Failed to Accept client");
      free(client_ptr);
    }
    else
    {
      x_thread_join(t);
      x_thread_destroy(t);
    }
  }

  x_net_close(server);
  x_net_shutdown();
  return 0;
}
