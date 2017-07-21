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
 * @file: lws2mql.h
 * @author: kris nyunt
 */

// The following ifdef block is the standard way of creating macros which make exporting
// from a DLL simpler. All files within this DLL are compiled with the LWS2MQL_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see
// LWS2MQL_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef LWS_EXPORTS
#define LWS_API extern __declspec(dllexport)
#else
#define LWS_API extern __declspec(dllimport)
#endif

/* maximum number of concurrently handled connections */
#define LWS_MAX_CONNECTIONS		256
/* maximum log buffer size */
#define LWS_LOG_SIZE			16384
/* maximum size of payload */
#define LWS_MAX_PAYLOAD			4096

//extern struct libwebsocket *_wsi[LWS_MAX_CONNECTIONS];
//extern struct libwebsocket_context *_context;
/* index to last allocated connection */
//extern int _wsid;

/* Exported functions */
LWS_API void lwsGetVersion(char *ver);
LWS_API int lwsCreate(char *err);
LWS_API int lwsDestroy(char *err);
LWS_API int lwsClientConnect(const char *address, int port, char *err);
LWS_API int lwsClientDisconnect(int wsid, char *err);
LWS_API int lwsSend(int wsid, char *buf, char *err);
LWS_API int lwsReceive(int wsid, char *buf, char *err);
LWS_API int lwsFlush(int wsid, char *err);
LWS_API int lwsIsAlive(int wsid, char *err);
LWS_API void lwsGetLog(char *buf, size_t size);
