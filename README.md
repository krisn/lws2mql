# lws2mql
Websocket client library for MetaTrader Trading Platform. Supports MQL4 and MQL5. Lets the MetaTrader Scripts, Expert Advisors, or Indicators connect to a Websocket server. This is based on the libwebsocket library.

## How To
Since we are generating a Windows DLL for MetaTrader, it is easier to build on a Windows environment or at least using Wine.

### Build

Change to `src` folder

`C:\> cd ${git-repo-dir}\src`

Compile with GCC

`C:\> gcc -c -DLWS_EXPORTS lws2mql.c`

Generate the .dll file also linking in `lib\libwebsockets.dll`

`C:\> gcc -shared -o lws2mql.dll lws2mql.o -L..\lib\ -lwebsockets`

### Deploy
Copy the files under `MQL5\Include` and `MQL5\Libraries` folders to the relevant places under your MetaTrader 5 installation folder. Libraries are the pre-built DLLs or you can use the ones you've built yourself. Includes are the MQL header files that will provide easy interfacing with the DLLs.

## Example

In your .mq5 source file, include the `Websocket.mqh` file and make connection, send/receive data as the example below.
```c
#include "Websocket.mqh"

CWebsocket ws;

int OnInit()
  {
    ws.Init();
    ConnectWebsocket();

    //--- succeed
    return(INIT_SUCCEEDED);
  }

void OnDeinit(const int reason)
  {

    if(ws.ClientDisconnect()<0) PrintFormat("[%s] Websocket %i disconnect error: %s",__FUNCTION__,ws.GetHandle(),ws.GetError());
    ws.Deinit();

  }

int OnCalculate(const int rates_total,
                const int prev_calculated,
                const datetime &time[],
                const double &open[],
                const double &high[],
                const double &low[],
                const double &close[],
                const long &tick_volume[],
                const long &volume[],
                const int &spread[])
  {
    if (Broadcast(ws)>-1) PrintFormat("Signal %i broadcast OK via socket %i",i,ws.GetHandle());
    else PrintFormat("Signal %i broadcast FAILED on socket %i, Error: %s",i,ws.GetHandle(),ws.GetError());
  }

int Broadcast(CWebsocket& _socket, int _channel_id, string _message)
  {
    payload = ""
     "{"
       "\"type\":\"signal\","
       "\"payload\": {"
         "\"channel_id\":"+(string)_channel_id+","
         "\"message\":\""+_message+"\""
       "}"
     "}";

    return (_socket.Send(payload));
  }

```
