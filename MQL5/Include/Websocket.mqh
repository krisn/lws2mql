//+------------------------------------------------------------------+
//|                                                   Websockets.mqh |
//|                                        Copyleft 2014, Kris Nyunt |
//|                                                   kris@nyunt.net |
//+------------------------------------------------------------------+
#property copyright "Copyleft 2014, Kris Nyunt"
#property link      "kris@nyunt.net"
#property version   "1.00"

#include <lws2mql.mqh>

//+------------------------------------------------------------------+
//| Constants                                                        |
//+------------------------------------------------------------------+

//+------------------------------------------------------------------+
//| Global Variables                                                 |
//+------------------------------------------------------------------+

//+------------------------------------------------------------------+
//| Class Websocket                                                  |
//| Usage:                                                           |
//+------------------------------------------------------------------+
class CWebsocket
  {
private:
   //--- parameters
   string            m_address;
   int               m_port;
   string            m_version;
   int               m_wsid;                 // Websocket handle
   string            m_last_error;
   char              m_log_buf[LWS_LOG_SIZE];

public:
   //--- Constructor/Destructor ---//
                     CWebsocket();
                    ~CWebsocket();
   bool              Init(void);
   void              Deinit(void);

   //--- lws2mql library functions ---//
   int               ClientConnect(void);
   int               ClientConnect(string address, int port);
   int               ClientDisconnect(void);
   int               Send(string str);
   int               Send(char& buf[]);
   string            Receive(void);
   int               Receive(string& str);
   int               Flush(void);
   bool              IsAlive(void);
   int               GetHandle(void);
   string            GetVersion(void);
   string            GetError(void);
   void              GetLog(void);

protected:
   void              Delay(const uint value);
  };
//+------------------------------------------------------------------+
//| Constructor                                                      |
//+------------------------------------------------------------------+
CWebsocket::CWebsocket(void) : m_wsid(-1),
                               m_address("127.0.0.1"),
                               m_port(8080)
  {
  }
//+------------------------------------------------------------------+
//| Destructor                                                       |
//+------------------------------------------------------------------+
CWebsocket::~CWebsocket(void)
  {
  }
//+------------------------------------------------------------------+
//| Method Init                                                      |
//+------------------------------------------------------------------+
bool CWebsocket::Init(void)
  {
   m_version = GetVersion();
   char err[256];
   int ret = lwsCreate(err);
   if (ret < 0) 
     {
      m_last_error = CharArrayToString(err);
      return false;
     }
   else
      return true;
  }
//+------------------------------------------------------------------+
//| Method Deinit                                                    |
//+------------------------------------------------------------------+
void CWebsocket::Deinit(void)
  {
   char err[256];
   int ret = lwsDestroy(err);
   if (ret < 0) m_last_error = CharArrayToString(err);
  }
//+------------------------------------------------------------------+
//| Get the version number                                           |
//+------------------------------------------------------------------+
string CWebsocket::GetVersion(void)
  {
   char ver[40];
   lwsGetVersion(ver);
   m_version = CharArrayToString(ver);

   return (m_version);
  }
//+------------------------------------------------------------------+
//| Get the version number                                           |
//+------------------------------------------------------------------+
int CWebsocket::GetHandle(void)
  {
   return (m_wsid);
  }
//+------------------------------------------------------------------+
//| Method Deinit                                                    |
//+------------------------------------------------------------------+
int CWebsocket::ClientConnect(void)
  {
   if (StringLen(m_address) > 8 && m_port > 0) 
      return ClientConnect(m_address,m_port);
   else
      return -1;
  }
//+------------------------------------------------------------------+
//| Method Deinit                                                    |
//+------------------------------------------------------------------+
int CWebsocket::ClientConnect(string address,int port)
  {
   char err[256], addr[80];

   StringToCharArray(address, addr);
   m_wsid = lwsClientConnect(addr, port, err);
   if (m_wsid < 0) m_last_error = CharArrayToString(err);
   //else PrintFormat("Websocket handle %i connected to %s:%i", m_wsid, address, port);
   
   return m_wsid;
  }
//+------------------------------------------------------------------+
//| Method Deinit                                                    |
//+------------------------------------------------------------------+
int CWebsocket::ClientDisconnect(void)
  {
   if (m_wsid < 0) {
      m_last_error = "Please call ClientConnect() first to establish connection";
      return -1;
   }

   char err[256];
   int ret = lwsClientDisconnect(m_wsid, err);
   if (ret < 0) m_last_error = CharArrayToString(err);

   return ret;
  }
//+------------------------------------------------------------------+
//| Get the version number                                           |
//+------------------------------------------------------------------+
string CWebsocket::Receive(void)
  {
   if (m_wsid < 0) {
      m_last_error = "Please call ClientConnect() first to establish connection";
      return NULL;
   }

   char err[128], buf[4096];
   int ret = lwsReceive(m_wsid, buf, err);

   string str = "";
   if (ret < 0) m_last_error = CharArrayToString(err);
   else str = CharArrayToString(buf);
   
   return str;
  }
//+------------------------------------------------------------------+
//| Get the version number                                           |
//+------------------------------------------------------------------+
int CWebsocket::Receive(string& str)
  {
   if (m_wsid < 0) {
      m_last_error = "Please call ClientConnect() first to establish connection";
      return -1;
   }

   char err[128], buf[4096];
   int ret = lwsReceive(m_wsid, buf, err);

   str = "";
   if (ret < 0) m_last_error = CharArrayToString(err);
   else str = CharArrayToString(buf);
   
   return ret;
  }
//+------------------------------------------------------------------+
//| Get the version number                                           |
//+------------------------------------------------------------------+
int CWebsocket::Send(string str)
  {
   if (m_wsid < 0) {
      m_last_error = "Please call ClientConnect() first to establish connection";
      return -1;
   }

   char err[128], buf[4096];
   StringToCharArray(str, buf);

   int ret = lwsSend(m_wsid, buf, err);
   if (ret < 0) m_last_error = CharArrayToString(err);

   return ret;
  }
//+------------------------------------------------------------------+
//| Get the version number                                           |
//+------------------------------------------------------------------+
int CWebsocket::Send(char& buf[])
  {
   if (m_wsid < 0) {
      m_last_error = "Please call ClientConnect() first to establish connection";
      return -1;
   }

   char err[128];
   int ret = lwsSend(m_wsid, buf, err);
   if (ret < 0) m_last_error = CharArrayToString(err);

   return ret;
  }
//+------------------------------------------------------------------+
//| Method Deinit                                                    |
//+------------------------------------------------------------------+
int CWebsocket::Flush(void)
  {
   if (m_wsid < 0) {
      m_last_error = "Please call ClientConnect() first to establish connection";
      return -1;
   }

   char err[256];
   int ret = lwsFlush(m_wsid, err);
   if (ret < 0) m_last_error = CharArrayToString(err);

   return ret;
  }
//+------------------------------------------------------------------+
//| Method Deinit                                                    |
//+------------------------------------------------------------------+
bool CWebsocket::IsAlive(void)
  {
   if (m_wsid < 0) return false;

   char err[256];
   int ret = lwsIsAlive(m_wsid, err);
   if (ret < 0) 
     {
      m_last_error = CharArrayToString(err);
      return false;
     }
   else 
     {
      return true;
     }
  }
//+------------------------------------------------------------------+
//| Get the version number                                           |
//+------------------------------------------------------------------+
void CWebsocket::GetLog(void)
  {
   lwsGetLog(m_log_buf, sizeof(m_log_buf));
   Print(CharArrayToString(m_log_buf));
  }
//+------------------------------------------------------------------+
//| Get the version number                                           |
//+------------------------------------------------------------------+
string CWebsocket::GetError(void)
  {
   return m_last_error;
  }
//+------------------------------------------------------------------+