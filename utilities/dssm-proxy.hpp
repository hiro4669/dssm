#ifndef _DSSM_PROXY_HPP_
#define _DSSM_PROXY_HPP_

#include "libssm.h"
#include "ssm.hpp"
#include <netinet/in.h>
#include "Thread.hpp"
#include "dssm-def.hpp"

#include <vector>
#include <ifaddrs.h>

#define BUFFER_SIZE 1024	   /* バッファバイト数 */

/* クライアントからの接続を待つサーバの情報を表現する構造体 */
struct TCPSERVER_INFO
{
	int wait_socket;		 /* サーバ待ち受け用ソケット */
	sockaddr_in server_addr; /* サーバ待ち受け用アドレス */
};

/* クライアントとの接続に関する情報を保存する構造体 */
struct TCPCLIENT_INFO
{
	int data_socket;		 /* クライアントとの通信用ソケット */
	sockaddr_in client_addr; /* クライアントのアドレス */
};

struct BUFFERDATA
{
	SSM_tid tid;
	time_t	time;
};

/*  ブロードキャスト送信用  */
struct BROADCAST_SENDINFO {
    uint16_t port;
    const char *ipaddr;
    char *msg;
    unsigned int msg_len;
    int sd;
    struct sockaddr_in addr;
    int permission;
};

/*  ブロードキャスト受信用  */
struct BROADCAST_RECVINFO {
    uint16_t port;
    int sd;
    struct sockaddr_in addr;
};

class ProxyServer;

class DataCommunicator :  public Thread
{
private:
	TCPSERVER_INFO server;
	TCPCLIENT_INFO client;

public:
	char *mData;
	uint64_t mDataSize;
	uint64_t ssmHeaderSize;
	uint64_t mFullDataSize;
	PROXY_open_mode mType;
	uint32_t thrdMsgLen;

	char *buf;

	bool isTCP;

	SSMApiBase *pstream;
	ProxyServer *proxy;

	bool sopen();
	bool rwait();
	bool UDPrwait();
	bool UDPsopen();
	bool sclose();

	bool receiveTMsg(thrd_msg *tmsg);
	bool sendTMsg(thrd_msg *tmsg);

	bool deserializeTmsg(thrd_msg *tmsg);
	bool serializeTmsg(thrd_msg *tmsg);

	void handleData();
	void handleRead();
	void handleBuffer();

	bool sendBulkData(char *buf, uint64_t size);
	bool receiveData();

public:
	DataCommunicator() = delete;
	DataCommunicator(uint16_t nport, char *mData, uint64_t d_size, uint64_t h_size,
					 SSMApiBase *pstream, PROXY_open_mode type, ProxyServer *proxy, bool isTCP = true);
	~DataCommunicator();

	void *run(void *args);
	void handleBufferRead();
};

class ProxyServer
{
private:
	TCPSERVER_INFO server;
	TCPCLIENT_INFO client;
	uint16_t nport; // センサデータ受信用のポート番号.子プロセスが生成されるたびにインクリメントしていく

	char *mData;			// データ用
	uint64_t mDataSize;		// データサイズ
	uint64_t ssmHeaderSize;	// データにつけるヘッダのサイズ
	uint64_t mFullDataSize; // mDataSize + ヘッダのサイズ
	char *mProperty;
	uint64_t mPropertySize;
	PROXY_open_mode mType;

	DataCommunicator *com;

	SSMApiBase stream; // real stream
	uint32_t dssmMsgLen;

	bool open();
	bool wait();

	void setSSMType(PROXY_open_mode mode);
	int receiveMsg(ssm_msg *msg, char *buf);
	int sendMsg(int cmd_type, ssm_msg *msg);

	void setupSigHandler();
	static void catchSignal(int signo);
	std::vector<pid_t> pids;

	/* for broadcast sending */
	int sbr_init(BROADCAST_SENDINFO *binfo, const char *ipaddr, const int port);
	int set_sbr_info(BROADCAST_SENDINFO *binfo);
	void send_br_msg(BROADCAST_SENDINFO *binfo, char *msg, int msg_len);
	void sbr_close(BROADCAST_SENDINFO *binfo);
	uint16_t create_msg(char* buffer, std::string ipaddr_str, int port);	
	void send_notification();	


	/* for broadcast receiving */
	int set_rbr_info(BROADCAST_RECVINFO *binfo);
	void rbr_close(BROADCAST_RECVINFO *binfo);
	std::pair<std::string , std::string> recv_br_msg(BROADCAST_RECVINFO *binfo);
	std::pair<std::string , std::string> parse_data(char* buf, int msg_len);
	void receive_notification();
	

	


public:
	ProxyServer();
	~ProxyServer();
	bool init();
	bool run(bool notify = false);
	bool server_close();
	bool client_close();
	void handleCommand();
	bool shutdown_children();

	int readInt(char **p);
	uint64_t readLong(char **p);
	double readDouble(char **p);
	void readRawData(char **p, char *d, int len);

	void writeInt(char **p, int v);
	void writeLong(char **p, uint64_t v);
	void writeDouble(char **p, double v);
	void writeRawData(char **p, char *d, int len);

	void deserializeMessage(ssm_msg *msg, char *buf);
	
};

/* get ip address information */
static std::pair<std::string, std::string> get_interface_info() {
    struct ifaddrs *addrs = nullptr;
    getifaddrs(&addrs);
    char buffer[INET_ADDRSTRLEN]={0,0,0,0};
    uint32_t br_addr = 0;
    uint32_t ip_addr = 0;
    sa_family_t address_family = 0;
    for (auto p = addrs; p != nullptr; p=p->ifa_next) {
		if (p->ifa_addr == nullptr) continue;

        address_family = p->ifa_addr->sa_family;
        if (address_family != AF_INET) continue; // if not ipv4 skip            
        
		uint32_t ip_addr_tmp = ((struct sockaddr_in*)p->ifa_addr)->sin_addr.s_addr;        
        if ((ip_addr_tmp & 0xff) == 127) continue;
        if ((ip_addr_tmp & 0xff) == 169) continue;
        
        ip_addr = ip_addr_tmp;
        
        uint32_t netmask = ((struct sockaddr_in*)p->ifa_netmask)->sin_addr.s_addr;
        uint32_t net_addr = ip_addr & netmask;
        br_addr = net_addr | ~netmask;
            
    }
    
    if (br_addr > 0) {        
        std::string br_addr_str = inet_ntop(AF_INET, &br_addr, buffer, INET_ADDRSTRLEN);
        std::string ip_addr_str = inet_ntop(AF_INET, &ip_addr, buffer, INET_ADDRSTRLEN);
        return {ip_addr_str, br_addr_str};
    }
    return {"",""};
    
}

#endif
