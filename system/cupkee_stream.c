/* GPLv2 License
 *
 * Copyright (C) 2016-2018 Lixing Ding <ding.lixing@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 **/

#include <cupkee.h>

static inline int stream_is_readable(cupkee_stream_t *s) {
    return s && (s->flags & CUPKEE_STREAM_FL_READABLE);
}

static inline int stream_is_writable(cupkee_stream_t *s) {
    return s && (s->flags & CUPKEE_STREAM_FL_WRITABLE);
}

static inline int stream_rx_request(cupkee_stream_t *s, size_t n) {
    return s->_read(s, n, NULL);
}

static inline int stream_tx_request(cupkee_stream_t *s) {
    return s->_write(s, 0, NULL);
}

int cupkee_stream_init(
   cupkee_stream_t *s, int id,
   size_t rx_buf_size, size_t tx_buf_size,
   int (*_read)(cupkee_stream_t *s, size_t n, void *),
   int (*_write)(cupkee_stream_t *s, size_t n, const void *)
)
{
    uint8_t flags = 0;

    if (!s || id < 0) {
        return -CUPKEE_EINVAL;
    }

    memset(s, 0, sizeof(cupkee_stream_t));
    if (rx_buf_size && _read) {
        s->_read = _read;
        s->rx_buf_size = rx_buf_size;
        flags |= CUPKEE_STREAM_FL_READABLE;
        cupkee_buffer_alloc(&s->rx_buf, rx_buf_size);
    }

    if (tx_buf_size && _write) {
        s->_write = _write;
        s->tx_buf_size = tx_buf_size;
        flags |= CUPKEE_STREAM_FL_WRITABLE;
        cupkee_buffer_alloc(&s->tx_buf, tx_buf_size);
    }
    s->id = id;
    s->rx_state = CUPKEE_STREAM_STATE_IDLE;

    s->flags = flags;
    return 0;
}

int cupkee_stream_deinit(cupkee_stream_t *s)
{
    if (s) {
        s->id = -1;

        cupkee_buffer_deinit(&s->rx_buf);
        cupkee_buffer_deinit(&s->tx_buf);
    }
    return 0;
}

void cupkee_stream_listen(cupkee_stream_t *s, int event)
{
    if (s) {
        switch (event) {
        case CUPKEE_EVENT_ERROR: s->flags |= CUPKEE_STREAM_FL_NOTIFY_ERROR; break;
        case CUPKEE_EVENT_DATA:  s->flags |= CUPKEE_STREAM_FL_NOTIFY_DATA;  stream_rx_request(s, s->rx_buf_size); break;
        case CUPKEE_EVENT_DRAIN: s->flags |= CUPKEE_STREAM_FL_NOTIFY_DRAIN; break;
        default: break;
        }
    }
}

void cupkee_stream_ignore(cupkee_stream_t *s, int event)
{
    if (s) {
        switch (event) {
        case CUPKEE_EVENT_ERROR: s->flags &= ~CUPKEE_STREAM_FL_NOTIFY_ERROR; break;
        case CUPKEE_EVENT_DATA:  s->flags &= ~CUPKEE_STREAM_FL_NOTIFY_DATA;  break;
        case CUPKEE_EVENT_DRAIN: s->flags &= ~CUPKEE_STREAM_FL_NOTIFY_DRAIN; break;
        default: break;
        }
    }
}

int cupkee_stream_readable(cupkee_stream_t *s)
{
    return stream_is_readable(s) ? cupkee_buffer_length(&s->rx_buf) : 0;
}

int cupkee_stream_writable(cupkee_stream_t *s)
{
    return stream_is_writable(s) ? cupkee_buffer_space(&s->tx_buf) : 0;
}

int cupkee_stream_push(cupkee_stream_t *s, size_t n, const void *data)
{

    if (!stream_is_readable(s) || !n || !data) {
        return 0;
    } else {
        cupkee_buffer_t *buf = &s->rx_buf;
        int cnt = cupkee_buffer_give(buf, n, data);

        if (s->flags & CUPKEE_STREAM_FL_NOTIFY_DATA && cupkee_buffer_length(buf) > s->rx_buf_size / 2) {
            cupkee_object_event_post(s->id, CUPKEE_EVENT_DATA);
        }
        s->last_push = _cupkee_systicks;

        return cnt;
    }
}

void cupkee_stream_sync(cupkee_stream_t *s, uint32_t systicks)
{
    if (s->flags & CUPKEE_STREAM_FL_NOTIFY_DATA
        && !cupkee_buffer_is_empty(&s->rx_buf)
        && (systicks - s->last_push) > 20) {
        cupkee_object_event_post(s->id, CUPKEE_EVENT_DATA);
    }
}

int cupkee_stream_pull(cupkee_stream_t *s, size_t n, void *data)
{
    if (stream_is_writable(s) && n && data) {
        int cnt = cupkee_buffer_take(&s->tx_buf, n, data);

        if (cnt > 0 && cupkee_buffer_is_empty(&s->tx_buf) && s->flags & CUPKEE_STREAM_FL_NOTIFY_DRAIN) {
            cupkee_object_event_post(s->id, CUPKEE_EVENT_DRAIN);
        }

        return cnt;
    }
    return 0;
}

int cupkee_stream_unshift(cupkee_stream_t *s, uint8_t data)
{
    if (!stream_is_readable(s)) {
        return -CUPKEE_EINVAL;
    }

    return cupkee_buffer_unshift(&s->rx_buf, data);
}

void cupkee_stream_pause(cupkee_stream_t *s)
{
    if (stream_is_readable(s)) {
        s->rx_state = CUPKEE_STREAM_STATE_PAUSED;
    }
}

void cupkee_stream_resume(cupkee_stream_t *s)
{
    if (stream_is_readable(s) && s->rx_state != CUPKEE_STREAM_STATE_FLOWING) {
        stream_rx_request(s, cupkee_buffer_space(&s->rx_buf));
        s->rx_state = CUPKEE_STREAM_STATE_FLOWING;
    }
}

int cupkee_stream_read(cupkee_stream_t *s, size_t n, void *buf)
{
    size_t max;

    if (!stream_is_readable(s) || !buf) {
        return -CUPKEE_EINVAL;
    }

    if (s->rx_state == CUPKEE_STREAM_STATE_IDLE) {
        s->rx_state = CUPKEE_STREAM_STATE_PAUSED;
    }

    if ((max = cupkee_buffer_length(&s->rx_buf)) < n) {
        stream_rx_request(s, n - max);
    }

    return cupkee_buffer_take(&s->rx_buf, n, buf);
}

int cupkee_stream_write(cupkee_stream_t *s, size_t n, const void *data)
{
    int retv;

    if (!stream_is_writable(s) || !data) {
        return -CUPKEE_EINVAL;
    }

    retv = cupkee_buffer_give(&s->tx_buf, n, data);
    if (retv == (int) cupkee_buffer_length(&s->tx_buf)) {
        stream_tx_request(s);
    }

    return retv;
}

int cupkee_stream_read_sync(cupkee_stream_t *s, size_t n, void *buf)
{
    if (!stream_is_readable(s) || !buf) {
        return -CUPKEE_EINVAL;
    }

    return s->_read(s, n, buf);
}

int cupkee_stream_write_sync(cupkee_stream_t *s, size_t n, const void *data)
{
    if (!stream_is_writable(s) || !data) {
        return -CUPKEE_EINVAL;
    }

    return s->_write(s, n, data);
}

