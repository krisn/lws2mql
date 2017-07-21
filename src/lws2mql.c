/**
 * The MIT License (MIT)
 *
 * Copyright (c) Kris Nyunt
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/**
 * This file is part of LWS2MQL package
 *
 * @file: lws2mql.c
 * @author: kris nyunt
 */

#include "stdafx.h"
#include "../include/libwebsockets.h"
#include "lws2mql.h"

static struct libwebsocket *_wsi[LWS_MAX_CONNECTIONS];
static struct libwebsocket_context *_context;
/* index to last allocated socket */
static int _wsid=-1, working_wsid=-1;
static int established=0, terminate=0, written=0;
static char _msg[128];
static char _log_buf[LWS_LOG_SIZE];
static char *_read_buf[LWS_MAX_CONNECTIONS];
static char *_write_buf[LWS_MAX_CONNECTIONS];
FILE *fp ;

struct per_session_data__mql {
	unsigned char buf[LWS_SEND_BUFFER_PRE_PADDING + LWS_MAX_PAYLOAD + LWS_SEND_BUFFER_POST_PADDING];
	unsigned int len;
	unsigned int index;
};

struct wsi_client {
	int index;
	struct libwebsocket_context *_context;
	struct libwesocket *_wsi;
	//struct libwebsocket ** ptr;
	char *_read_buf;
	char *_write_buf;
	int established;
	int terminate;
	int written;
};

static struct wsi_client *wsi_clients[LWS_MAX_CONNECTIONS];

void _log(char *msg);

/* mql protocol */
static int
callback_fn(struct libwebsocket_context *context,
			struct libwebsocket *wsi,
			enum libwebsocket_callback_reasons reason,
			void *user, void *in, size_t len)
{
	unsigned char buf[LWS_SEND_BUFFER_PRE_PADDING + LWS_MAX_PAYLOAD + LWS_SEND_BUFFER_POST_PADDING];
	struct per_session_data__mql *psd = (struct per_session_data__mql *)user;
	int n, l, wsid=-1;

	//sprintf(_msg, "%s: %p reason %d", __func__, (void *)wsi, reason);
	//_log(_msg);

	for (n = 0; n <= _wsid; n++) {
		if (_wsi[n] == wsi) {
			wsid = n;			
		
			sprintf(_msg, "%s: incoming %p existing %p index %d",  __func__, (void *)wsi, (void *)_wsi[n], n);
			_log(_msg);
			break;
		}
	}
	//wsid = working_wsid;
	if (wsid<0) return 0;

	switch (reason) {
	case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
	case LWS_CALLBACK_CLOSED:
		sprintf(_msg, "%s: Client %d LWS_CALLBACK_CLOSED on %p", __func__, wsid, (void *)wsi);
		_log(_msg);
		/* remove closed guy and terminate */
		_wsi[wsid] = NULL;
		//free(_wsi[wsid]);
		free(_read_buf[wsid]);
		free(_write_buf[wsid]);
		terminate = 1;
		established = 0;
		working_wsid = -1;
		break;

	/* when the callback is used for client operations */
	case LWS_CALLBACK_CLIENT_ESTABLISHED:
		sprintf(_msg, "%s: Client %d LWS_CALLBACK_CLIENT_ESTABLISHED on %p", __func__, wsid, (void *)wsi);
		_log(_msg);
		established = 1;
		/* start the ball rolling */
		//libwebsocket_callback_on_writable(context, wsi);
		break;

	case LWS_CALLBACK_CLIENT_RECEIVE:
		((char *)in)[len] = '\0';
		if (len < LWS_MAX_PAYLOAD-strlen(_read_buf[wsid])) {
			strcat(_read_buf[wsid], (char *)in);
			int m = len < 80 ? len : 80-1;
			char tmp[80];
			// WARNING: Access violation error (read), if (char *)in length is less than number of chars specified to copy
			strncpy(&tmp[0], (char *)in, m);
			tmp[m+1] = '\0';
			sprintf(_msg, "%s: Client %d RX: %s", __func__, wsid, tmp);
			_log(_msg);
		} else {
			sprintf(_msg, "%s: Client %d RX larger than read buffer size. %d bytes (Avail %d of %d b)", __func__, wsid, len, LWS_MAX_PAYLOAD-strlen(_read_buf[wsid]), LWS_MAX_PAYLOAD);
			_log(_msg);
		}
		break;

	case LWS_CALLBACK_CLIENT_WRITEABLE:
		/* send our packet... */
		if (written) return 0;
		l = sprintf((char *)&buf[LWS_SEND_BUFFER_PRE_PADDING], "%s", _write_buf[wsid]);
		int m = l < 80 ? l : 80-1;
		char tmp[80];
		strncpy(&tmp[0], _write_buf[wsid], m);
		tmp[m+1] = '\0';
		sprintf(_msg, "%s: Client %d TX: %s..", __func__, wsid, tmp);
		_log(_msg);

		n = libwebsocket_write(wsi, &buf[LWS_SEND_BUFFER_PRE_PADDING], l, LWS_WRITE_TEXT);
		if (n < 0) {
			sprintf(_msg, "%s: Error %d writing to socket, hanging up.", __func__, n);
			_log(_msg);
			return -1;
		}
		if (n < (int)l ) { 
			sprintf(_msg, "%s: Partial write", __func__);
			_log(_msg);
			return -1;
		}
		written = 1;
		break;

	default:
		break;
	}

	return 0;
}

/* List of supported protocols and callbacks */
static struct libwebsocket_protocols protocols[] = {	
	/* first protocol must always be HTTP handler */
	{
		"default",		                        /* name */
		callback_fn,		                    /* callback */
		sizeof(struct per_session_data__mql),   /* per_session_data_size */
		0                                       /* rx_buffer_size */
	},
	{ NULL, NULL, 0, 0 }                        /* end */
};

/* Gets the libwebsockets version.
 *  ver - Buffer to hold the version string.
 * Returns:
 *	void
 */
LWS_API void lwsGetVersion(char *ver)
{
	sprintf(ver, "libwebsockets version %s", lws_get_library_version());
}

/* Creates a websocket context.
 *  num_layers - The total number of layers including the input and the output layer.
 *  l1num - number of neurons in 1st layer (inputs)
 *  l2num, l3num, l4num - number of neurons in hidden and output layers (depending on num_layers).
 * Returns:
 *	handler to ann, -1 on error
 */
LWS_API int lwsCreate(char *err)
{
	sprintf(err, "");
	//struct libwebsocket_context *context;
	struct lws_context_creation_info info;

	if (_context != NULL) {
		sprintf(err, "%s: Websocket context already exists", __func__);
		_log(err);
		return 0;
	}

	memset(&info, 0, sizeof info);
	/*
	 * create the websockets context.  This tracks open connections and
	 * knows how to route any traffic and which protocol version to use,
	 * and if each connection is client or server side.
	 *
	 * For this client-only demo, we tell it to not listen on any port.
	 */
	info.port = CONTEXT_PORT_NO_LISTEN;
	info.protocols = protocols;
#ifndef LWS_NO_EXTENSIONS
	info.extensions = libwebsocket_get_internal_extensions();
#endif
	info.gid = -1;
	info.uid = -1;

	_context = libwebsocket_create_context(&info);
	if (_context == NULL) {
		sprintf(err, "%s: Creating websocket context failed", __func__);
		_log(err);
		return -1;
	}

	sprintf(_msg, "%s: Log size %d bytes", __func__, sizeof(_log_buf));
	_log(_msg);
	return 0;
}

/* Destroys the websocket context
 *  _wsi - websocket handler returned by lwsCreate*
 * Returns:
 *  0 on success -1 on error
 * WARNING: the handlers cannot be reused if wsi!=(_wsi-1)
 * Other handlers are reusable only after the last wsi is destroyed.
 */
LWS_API int lwsDestroy(char *err)
{
	int n;
	sprintf(err, "");

	if (_context == NULL) {
		sprintf(err, "%s: No websocket context exists. Nothing to destroy!", __func__);
		_log(err);
		return -1;
	}

	/* client connections still exists */
	for (n = 0; n <= _wsid; n++)
		if (_wsi[n] != NULL) {
			sprintf(err, "%s: Websocket clients still exists (last %d). Cannot destroy!", __func__, _wsid);
			_log(err);
			return -1;
		}

	libwebsocket_context_destroy(_context);

	sprintf(_msg, "%s: Websocket context destroyed", __func__);
	_log(_msg);
	return 0;
}

/* Creates a client connection.
 *  address - The address.
 *  port - The port number.
 *  err - Error message.
 * Returns:
 *	handler to websocket, -1 on error
 */
LWS_API int lwsClientConnect(const char *address, int port, char *err)
{
	sprintf(err, "");
	int use_ssl = 0;
	int ietf_version = -1; /* latest */
	//struct libwebsocket *wsi;
	int n, s;

	if (_context == NULL) {
		sprintf(err, "%s: Cannot find websocket context. Please create first.", __func__);
		_log(err);
		return -1;
	}

	if (_wsid>=LWS_MAX_CONNECTIONS) {
		sprintf(err, "%s: Too many connections opened already.", __func__);
		_log(err);
		return -1;
	}

	/* allocate the handler for wsi */
	_wsid++;
	working_wsid = _wsid;

	_wsi[_wsid] = libwebsocket_client_connect(_context, address, port, use_ssl, 
				  "/", address, "origin", NULL, ietf_version);
	if (_wsi[_wsid] == NULL) {
		sprintf(err, "%s: Client failed to connect to %s:%u", __func__, address, port);
		_log(err);
		_wsid--;
		working_wsid = -1;
		return -1;
	}

	/* service loop */
	n = 0; s=3; written=1; terminate=0;
	while (n >= 0 && s >= 0 && !terminate && !established) {
		sprintf(_msg, "%s: Client %d loop %d", __func__, _wsid, s);
		_log(_msg);

		n = libwebsocket_service(_context, 10);
		if (n < 0) {
			sprintf(err, "%s: Error while servicing connection for client %d", __func__, _wsid);
			_log(err);
			_wsid--;
			working_wsid = -1;
			return -1;
		}
		s--;
	}

	if (established) {
		/* initialize read/write buffers for this socket connection */
		_write_buf[_wsid] = (char *) malloc(LWS_MAX_PAYLOAD);
		sprintf(_write_buf[_wsid], "");
		_read_buf[_wsid] = (char *) malloc(LWS_MAX_PAYLOAD);
		sprintf(_read_buf[_wsid], "");

		sprintf(_msg, "%s: Client %d connected to %s:%u", __func__, _wsid, address, port);
		_log(_msg);
	} else {
		sprintf(err, "%s: Client failed establishing connection to %s:%u", __func__, address, port);
		_log(err);
		_wsid--;
		working_wsid = -1;
		return -1;
	}

	return (_wsid);
}

/* Disconnects a client.
 *  wsid - Client's websocket id.
 *  err - Error buffer.
 * Returns:
 *	0 on success, -1 on error
 */
LWS_API int lwsClientDisconnect(int wsid, char *err)
{
	sprintf(err, "");
	int i, n, s;

	if (wsid<0 || wsid>_wsid || _wsi[wsid]==NULL) {
		sprintf(err, "%s: Websocket connection %d does not exist", __func__, wsid);
		return -1;
	}
	working_wsid = wsid;

	/* destroy and clear the pointers */
	//libwebsocket_close_and_free_session(_context, _wsi[wsid], LWS_CLOSE_STATUS_NORMAL);
	//_wsi[wsid] = NULL;
	//free(_wsi[wsid]);
	//free(_read_buf[wsid]);
	//free(_write_buf[wsid]);
	callback_fn(_context, _wsi[wsid], LWS_CALLBACK_CLOSED, NULL, NULL, 0);
	//_wsi[wsid])->protocol->callback(_context, _wsi[wsid], LWS_CALLBACK_CLOSED, _wsi[wsid]->user_space, NULL, 0);
	//_context->protocols[0].callback(_context, _wsi[wsid], LWS_CALLBACK_CLOSED, _wsi[wsid]->user_space, NULL, 0);

	/* lets reuse the handlers if last */
	if (wsid==_wsid) {
		_wsid--;

		/* look if we can recover any more handlers */
		for (i=_wsid; i>-1; i--) {
			if (_wsi[i]==NULL) {
				_wsid--;
			} else {
				break;
			}
		}
	}

	sprintf(_msg, "%s: Client %d disconnected", __func__, wsid);
	_log(_msg);
	return 0;
}

/* Sends data to a connected client.
 *  wsid - Client's websocket id.
 *  buf - Payload buffer.
 *  err - Error buffer.
 * Returns:
 *	handler to websocket, -1 on error
 */
LWS_API int lwsSend(int wsid, char *buf, char *err)
{
	sprintf(err, "");
	int n, s, len;

	if (wsid<0 || wsid>_wsid || _wsi[wsid]==NULL) {
		sprintf(err, "%s: Websocket connection %d does not exist", __func__, wsid);
		_log(err);
		return -1;
	}
	working_wsid = wsid;
	
	if (strlen(buf) < LWS_MAX_PAYLOAD) {
		strcpy(_write_buf[wsid], buf);
		//strncpy(&tmp[0], _write_buf[wsid], 80);
		//sprintf(_msg, "%s: Payload %d TX: %s.. (%d bytes)", __func__, wsid, tmp, strlen(_write_buf[wsid]));
		//_log(tmp);
	} else {
		sprintf(err, "%s: Payload exceeds limit. %d b (Avail %d b)", __func__, strlen(buf), LWS_MAX_PAYLOAD);
		_log(err);
		return -1;
	}

	/* service loop */
	n = 0; s=3; written=0; terminate=0;
	while (n >= 0 && s >= 0 && !terminate && !written) {
		sprintf(_msg, "%s: Client %d loop %d", __func__, wsid, s);
		_log(_msg);

		libwebsocket_callback_on_writable(_context, _wsi[wsid]);
		n = libwebsocket_service(_context, 10);
		if (n < 0) {
			sprintf(err, "%s: Error while servicing client %d", __func__, wsid);
			_log(err);
			return -1;
		}
		s--;
	}

	sprintf(_msg, "%s: Websocket %d serviced", __func__, wsid);
	_log(_msg);
	return wsid;
}

/* Receives data rom a connected client.
 *  wsid - Client's websocket id.
 *  buf - Payload buffer.
 *  err - Error buffer.
 * Returns:
 *	handler to websocket, -1 on error
 */
LWS_API int lwsReceive(int wsid, char *buf, char *err)
{
	sprintf(err, "");
	int n, s, len;

	if (wsid<0 || wsid>_wsid || _wsi[wsid]==NULL) {
		sprintf(err, "%s: Websocket connection %d does not exist", __func__, wsid);
		_log(err);
		return -1;
	}
	working_wsid = wsid;

	strcpy(buf, _read_buf[wsid]);
	sprintf(_read_buf[wsid], "");
	//sprintf(_msg, "%s: Previous payload: %s (%d bytes)", __func__, _read_buf, strlen(_read_buf));
	//_log(_msg);

	/* service loop */
	n = 0; s=3; written=1; terminate=0;
	while (n >= 0 && s >= 0 && !terminate) {
		//sprintf(_msg, "%s: Client %d loop %d", __func__, wsid, s);
		//_log(_msg);

		n = libwebsocket_service(_context, 10);
		if (n < 0) {
			sprintf(err, "%s: Error while servicing client %d", __func__, wsid);
			_log(err);
			return -1;
		}
		s--;
	}

	sprintf(_msg, "%s: Websocket %d serviced", __func__, wsid);
	_log(_msg);

	strcat(buf, _read_buf[wsid]);
	//sprintf(_msg, "%s: Payload: %s (%d bytes)", __func__, _read_buf, strlen(_read_buf));
	//_log(_msg);
	
	sprintf(_read_buf[wsid], "");
	return wsid;
}

/* Flushes the payload buffer of a wbsocket.
 *  wsid - WWebsocket id.
 *  err - Error buffer.
 * Returns:
 *	handler to websocket, -1 on error
 */
LWS_API int lwsFlush(int wsid, char *err)
{
	sprintf(err, "");

	if (wsid<0 || wsid>_wsid || _wsi[wsid]==NULL) {
		sprintf(err, "%s: Websocket connection %d does not exist", __func__, wsid);
		_log(err);
		return -1;
	}

	sprintf(_read_buf[wsid], "");
	sprintf(_write_buf[wsid], "");
	sprintf(_msg, "%s: Websocket %d buffers flushed", __func__, wsid);
	_log(_msg);
	return wsid;
}

/* Checks whether a websocket is alive.
 *  wsid - Websocket id.
 *  err - Error buffer.
 * Returns:
 *	handler to websocket, -1 on error
 */
LWS_API int lwsIsAlive(int wsid, char *err)
{
	sprintf(err, "");

	if (wsid<0 || wsid>_wsid || _wsi[wsid]==NULL) {
		sprintf(err, "%s: Websocket connection %d does not exist", __func__, wsid);
		_log(err);
		return -1;
	}

	sprintf(_msg, "%s: Websocket %d alive?", __func__, wsid);
	_log(_msg);
	return wsid;
}

/* Gets the websocket log data. Useful to debug on client side.
 *  buf - Data buffer.
 *  size - Buffer size.
 * Returns:
 *	void
 */
LWS_API void lwsGetLog(char *buf, size_t size)
{
	char tmp[80];
	sprintf(tmp, "Buf: %d bytes -> Avail: %d bytes", strlen(_log_buf), sizeof(buf));
	strcpy(buf, _log_buf);
	strcat(buf, tmp);
}

/* Helper function to log websocket activities or messages.
 *  msg - Log message.
 * Returns:
 *	void
 */
void _log(char *msg)
{
	char buf[128];
	unsigned long long now;

	time_t timer;
	struct tm* tm_info;

	time(&timer);
	tm_info = localtime(&timer);

	buf[0] = '\0';
	strftime(buf, 25, "[%Y-%m-%d %H:%M:%S]", tm_info);
	sprintf(buf, "%s %s\n", buf, msg);

	fp = fopen ("/var/log/lws2mql.log","a+");
	fprintf(fp, "%s", buf);
	fclose(fp);

	if (strlen(buf) < LWS_LOG_SIZE-strlen(_log_buf)) {
		strcat(_log_buf, buf);
	}
}

#if 0
// This is an example of an exported variable
LWS_API int nLWS2MQL=0;

// This is an example of an exported function.
LWS_API int fnLWS2MQL(void)
{
	return 42;
}

// This is the constructor of a class that has been exported.
// see lws2mql.h for the class definition
CLWS2MQL::CLWS2MQL()
{
	return;
}
#endif
