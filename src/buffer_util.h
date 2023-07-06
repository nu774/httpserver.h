#ifndef HS_BUFFER_UTIL_H
#define HS_BUFFER_UTIL_H

#include <stdlib.h>
#include <string.h>

#ifndef HTTPSERVER_IMPL
#include "common.h"
#endif

static inline void _hs_buffer_free(struct hsh_buffer_s *buffer,
                                   int64_t *memused) {
  if (buffer->buf) {
    free(buffer->buf);
    *memused -= buffer->capacity;
    buffer->buf = NULL;
  }
}

static inline void _hs_buffer_discard_until(struct hsh_buffer_s *buffer, int pos) {
  if (pos > buffer->index)
    pos = buffer->index;
  if (buffer->buf && pos > 0) {
    memmove(buffer->buf, buffer->buf + pos, buffer->length - pos);
    buffer->length -= pos;
    buffer->index -= pos;
  }
}

#endif
