//+------------------------------------------------------------------+
//|                                                      lws2mql.mqh |
//|                                        Copyleft 2014, Kris Nyunt |
//|                                                   kris@nyunt.net |
//+------------------------------------------------------------------+
#property copyright "Copyleft 2014, Kris Nyunt"
#property link      "kris@nyunt.net"
#property version   "1.00"

#import "lws2mql.dll"

void lwsGetVersion(char& ver[]);
int lwsCreate(char& err[]);
int lwsDestroy(char& err[]);
int lwsClientConnect(char& address[], int port, char& err[]);
int lwsClientDisconnect(int wsid, char& err[]);
int lwsSend(int wsid, char& buf[], char& err[]);
int lwsReceive(int wsid, char& buf[], char& err[]);
int lwsFlush(int wsid, char& err[]);
int lwsIsAlive(int wsid, char& err[]);
void lwsGetLog(char& buf[], int size);

#import

/* maximum number of concurrently handled connections */
#define LWS_MAX_CONNECTIONS      256
/* maximum log buffer size */
#define LWS_LOG_SIZE             16384
/* maximum size of payload */
#define LWS_MAX_PAYLOAD          4096