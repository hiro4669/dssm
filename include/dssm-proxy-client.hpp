#ifndef _DSSM_PROXY_CLIENT_HPP_
#define _DSSM_PROXY_CLIENT_HPP_

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "libssm.h"
#include "dssm-def.hpp"

/**
 * Dummy Class
 */
class DSSMDummy
{
};

typedef enum {
	TCP_CONNECT,
	UDP_CONNECT,
} Connect_type;

class PConnector
{
private:
	struct sockaddr_in server;
	struct sockaddr_in dserver;
	int sock;  //Proxyとの通信用ソケット
	SSM_open_mode openMode;

	char *tbuf;

	const char *streamName;
	int streamId;
	SSM_sid sid;			///< sid
	void *mData;			///< データのポインタ
	uint64_t mDataSize;		///< データ構造体のサイズ
	void *mProperty;		///< プロパティのポインタ
	uint64_t mPropertySize; ///< プロパティサイズ
	void *mFullData;
	ssmtime *timecontrol; ///< for get real time
	const char *ipaddr;
	bool isVerbose;
	bool isBlocking;
	uint32_t thrdMsgLen;
	uint32_t dssmMsgLen;

	Connect_type conType;
	//double saveTime;
	//double cycle;
	

	void deserializeMessage(ssm_msg *msg, char *buf);

	bool serializeTMessage(thrd_msg *tmsg, char **buf);
	bool deserializeTMessage(thrd_msg *tmsg, char **buf);

	bool createRemoteSSM(const char *name, int stream_id, uint64_t ssm_size, ssmTimeT life, ssmTimeT cycle);
	bool setPropertyRemoteSSM(const char *name, int sensor_uid, const void *adata, uint64_t size);
	bool getPropertyRemoteSSM(const char *name, int sensor_uid, const void *adata);

	bool sendData(const char *data, uint64_t size);
	bool recvData(); // read data recv

    // for msg_queue 
    pid_t my_pid;
    int msq_id;
    bool open_msgque();

protected:

	double saveTime;
	double cycle;
	double life;

	int dsock; //DComとの通信用ソケット
	uint64_t mFullDataSize;
	void writeInt(char **p, int v);
	void writeLong(char **p, uint64_t v);
	void writeDouble(char **p, double v);
	void writeRawData(char **p, char *d, int len);

	int readInt(char **p);
	uint64_t readLong(char **p);
	double readDouble(char **p);
	void readRawData(char **p, char *d, int len);

	/* For Re-Connect when dssm-proxy get down*/
	void reconnwrite();
	void reconnread();
	void reconnreadbuf();
	bool ping();

public:
	SSM_tid timeId; // データのTimeID (SSM_tid == int)
	ssmTimeT time;	// = 0;  // データのタイムスタンプ (ssmTimeT == double)

	PConnector();
	~PConnector();
	PConnector(const char *streamName, int streamId = -1, const char *ipAddress = "127.0.0.1");

	void initPConnector();

	bool connectToServer(const char *serverName, int port);
	bool sendMsgToServer(int cmd_type, ssm_msg *msg);
	bool recvMsgFromServer(ssm_msg *msg, char *buf);

	bool sendTMsg(thrd_msg *tmsg);
	bool recvTMsg(thrd_msg *tmsg);

	bool TCPconnectToDataServer(const char *serverName, int port);
	bool UDPconnectToDataServer(const char *serverName, int port);

	bool isOpen();
	void *getData();

	bool initRemote();
	bool initDSSM();
	void setStream(const char *streamName, int streamId);
	void setBuffer(void *data, uint64_t dataSize, void *property,
			uint64_t propertySize, void *fulldata);
	void setIpAddress(const char *address);
	bool create(const char *streamName, int streamId, double saveTime,
			double cycle);
	bool create(double saveTime = 0.0, double cycle = 0.0);
	bool open(SSM_open_mode openMode = SSM_READ);
	bool open(const char *streamNane, int streamId, SSM_open_mode openMode);
	bool setProperty();
	bool getProperty();
	void setOffset(ssmTimeT offset);
	bool createDataCon();
	bool TCPcreateDataCon();
	bool UDPcreateDataCon();

	bool release();
	bool terminate();

	/* getter */
	const char *getStreamName() const;
	const char *getSensorName() const;
	bool isUpdate();
	void setVerbose(bool verbose);
	void setBlocking(bool isBlocking);
	int getStreamId() const;
	int getSensorId() const;
	SSM_sid getSSMId();
	void *data();
	uint64_t dataSize();
	void *property();
	uint64_t propertySize();
	uint64_t sharedSize();

	SSM_tid getTID_top(SSM_sid sid);
	SSM_tid getTID_top();
	SSM_tid getTID_bottom(SSM_sid sid);
	SSM_tid getTID_bottom();
	SSM_tid getTID(SSM_sid sid, ssmTimeT ytime);

	double timettof(struct timespec t); // 使わないかも
	static ssmTimeT getRealTime();		// 現在時刻の取得

	bool write(ssmTimeT time = gettimeSSM());					   // write bulkdata with time
	bool read(SSM_tid tmid = -1, READ_packet_type type = TIME_ID); // read
	bool readNew();												   // 最新であり、前回読み込んだデータと違うデータのときに読み込む
	bool readNext(int dt);										   // 前回読み込んだデータの次のデータを読み込む dt -> 移動量
	bool readBack(int dt);										   // 前回のデータの1つ(以上)前のデータを読み込む dt -> 移動量
	bool readLast();											   // 最新データの読み込み
	bool readTime(ssmTimeT t);									   // 時間指定, packet control

	
	void setConnectType(Connect_type conType);	 // UDPかTCPか

	virtual void readyRingBuf( ssmTimeT life = 0.0, ssmTimeT cycle = 0.0 ) = 0;

};
#endif
