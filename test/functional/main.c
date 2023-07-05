#define HTTPSERVER_IMPL
#include "../../build/src/httpserver.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

#define RESPONSE "Hello, World!"

ssize_t hs_test_write(int fd, char const *data, size_t size) {
  return write(fd, data, size);
}

int request_target_is(struct http_request_s* request, char const * target) {
  http_string_t url = http_request_target(request);
  int len = strlen(target);
  return len == url.len && memcmp(url.buf, target, url.len) == 0;
}

int chunk_count = 0;

void chunk_cb(struct http_request_s* request) {
  chunk_count++;
  struct http_response_s* response = http_response_init();
  http_response_body(response, RESPONSE, sizeof(RESPONSE) - 1);
  if (chunk_count < 3) {
    http_respond_chunk(request, response, chunk_cb);
  } else {
    http_response_header(response, "Foo-Header", "bar");
    http_respond_chunk_end(request, response);
  }
}

void chunked_echo_write_cb(struct http_request_s* request);

void chunked_echo_cb_1(struct http_request_s* request) {
  http_string_t str = http_request_chunk(request);
  struct http_response_s* response = http_response_init();
  if (str.len > 0) {
    http_response_body(response, str.buf, str.len);
    http_respond_chunk(request, response, chunked_echo_write_cb);
  } else {
    http_respond_chunk_end(request, response);
  }
}

void chunked_echo_write_cb(struct http_request_s* request) {
  http_request_read_chunk(request, chunked_echo_cb_1);
}

void chunked_echo_cb(struct http_request_s* request) {
  http_string_t str = http_request_chunk(request);
  struct http_response_s* response = http_response_init();
  http_string_t content_type = http_request_header(request, "Content-Type");
  if (content_type.len > 0) {
    char buf[256];
    snprintf(buf, sizeof buf, "%.*s", content_type.len, content_type.buf);
    http_response_header(response, "Content-Type", buf);
  } else {
    http_response_header(response, "Content-Type", "application/octet-stream");
  }
  if (str.len > 0) {
    http_response_body(response, str.buf, str.len);
    http_respond_chunk(request, response, chunked_echo_write_cb);
  } else {
    http_respond(request, response);
  }
}

struct http_server_s* poll_server;

void handle_request(struct http_request_s* request) {
  chunk_count = 0;
  http_request_connection(request, HTTP_AUTOMATIC);
  struct http_response_s* response = http_response_init();
  http_response_status(response, 200);
  if (request_target_is(request, "/echo")
   || request_target_is(request, "/chunked-req")
   || request_target_is(request, "/large")) {
    http_request_read_chunk(request, chunked_echo_cb);
    return;
  } else if (request_target_is(request, "/host")) {
    http_string_t ua = http_request_header(request, "Host");
    http_response_header(response, "Content-Type", "text/plain");
    http_response_body(response, ua.buf, ua.len);
  } else if (request_target_is(request, "/poll")) {
    while (http_server_poll(poll_server) > 0);
    http_response_header(response, "Content-Type", "text/plain");
    http_response_body(response, RESPONSE, sizeof(RESPONSE) - 1);
  } else if (request_target_is(request, "/empty")) {
    // No Body
  } else if (request_target_is(request, "/chunked")) {
    http_response_header(response, "Content-Type", "text/plain");
    http_response_body(response, RESPONSE, sizeof(RESPONSE) - 1);
    http_respond_chunk(request, response, chunk_cb);
    return;
  } else if (request_target_is(request, "/headers")) {
    int iter = 0, i = 0;
    http_string_t key, val;
    char buf[512];
    while (http_request_iterate_headers(request, &key, &val, &iter)) {
      i += snprintf(buf + i, 512 - i, "%.*s: %.*s\n", key.len, key.buf, val.len, val.buf);
    }
    http_response_header(response, "Content-Type", "text/plain");
    http_response_body(response, buf, i);
    return http_respond(request, response);
  } else {
    http_response_header(response, "Content-Type", "text/plain");
    http_response_body(response, RESPONSE, sizeof(RESPONSE) - 1);
  }
  http_respond(request, response);
}

struct http_server_s* server;

void handle_sigterm(int signum) {
  (void)signum;
  close(http_server_loop(server));
  close(http_server_loop(poll_server));
}

int main() {
  signal(SIGINT, handle_sigterm);
  signal(SIGTERM, handle_sigterm);
  server = http_server_init(8080, handle_request);
  poll_server = http_server_init(8081, handle_request);
  http_server_listen_poll(poll_server);
  http_server_listen(server);
  free(server);
  free(poll_server);
}
