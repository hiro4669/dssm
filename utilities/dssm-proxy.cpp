#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <arpa/inet.h>

#include <thread>
#include <utility>
#include <iostream>
#include <vector>
#include <tuple>

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <netinet/in.h>
#include <netinet/tcp.h>

#include <errno.h>
#include <sys/fcntl.h>

#include "ssm-time.h"
#include "ssm.h"
//#include "printlog.hpp"
//#include "libssm.h"

#include "dssm-proxy.hpp"
#include "dssm-utility.hpp"

//#include "broadcast.hpp"


#define DEBUG 0

bool isBuffer = false;
extern pid_t my_pid; // for debug
socklen_t address_len = sizeof(struct sockaddr_in);

DataCommunicator::DataCommunicator(uint16_t nport, char *mData, uint64_t d_size,
								   uint64_t h_size, SSMApiBase *pstream, PROXY_open_mode type, bool isTCP) {
	this->mData = mData;
	this->mDataSize = d_size;
	this->ssmHeaderSize = h_size;
	this->mFullDataSize = d_size + h_size;
	this->thrdMsgLen = dssm::util::countThrdMsgLength();

	// streamはコピーされる.
	this->pstream = pstream;
	this->mType = type;
	this->proxy = proxy;
	this->isTCP = isTCP;

	this->buf = (char *)malloc(sizeof(thrd_msg));

	this->server.wait_socket = -1;
	this->server.server_addr.sin_family = AF_INET;
	this->server.server_addr.sin_addr.s_addr = htonl(SERVER_IP);
	this->server.server_addr.sin_port = htons(nport);

	if (isTCP ? !this->sopen() : !this->UDPsopen())
	{
		perror("sopen error\n");
	}
}

DataCommunicator::~DataCommunicator()
{
	this->sclose();
	if (this->buf)
		free(this->buf);
}
//データの受信
bool DataCommunicator::receiveData()
{
//	int len = 0;
	uint64_t len = 0;
	while ((len += recv(this->client.data_socket, &mData[len],
						mFullDataSize - len, 0)) != mFullDataSize)
		;
	return true;
}


int DataCommunicator::readInt(char **p) {
	uint8_t v1 = **p;
	(*p)++;
	uint8_t v2 = **p;
	(*p)++;
	uint8_t v3 = **p;
	(*p)++;
	uint8_t v4 = **p;
	(*p)++;

	int v = (int)(v1 << 24 | v2 << 16 | v3 << 8 | v4);
	return v;
}

uint64_t DataCommunicator::readLong(char **p) {
	uint8_t v1 = **p;
	(*p)++;
	uint8_t v2 = **p;
	(*p)++;
	uint8_t v3 = **p;
	(*p)++;
	uint8_t v4 = **p;
	(*p)++;
	uint8_t v5 = **p;
	(*p)++;
	uint8_t v6 = **p;
	(*p)++;
	uint8_t v7 = **p;
	(*p)++;
	uint8_t v8 = **p;
	(*p)++;

	uint64_t lv = (uint64_t)((uint64_t)v1 << 56 | (uint64_t)v2 << 48 | (uint64_t)v3 << 40 | (uint64_t)v4 << 32 | (uint64_t)v5 << 24 | (uint64_t)v6 << 16 | (uint64_t)v7 << 8 | (uint64_t)v8);
	return lv;
}

double DataCommunicator::readDouble(char **p) {
	char buf[8];
	for (int i = 0; i < 8; ++i, (*p)++)
	{
		buf[7 - i] = **p;
	}
	return *(double *)buf;
}

void DataCommunicator::writeInt(char **p, int v) {
	**p = (v >> 24) & 0xff;
	(*p)++;
	**p = (v >> 16) & 0xff;
	(*p)++;
	**p = (v >> 8) & 0xff;
	(*p)++;
	**p = (v >> 0) & 0xff;
	(*p)++;
}
void DataCommunicator::writeLong(char **p, uint64_t v) {
	**p = (v >> 56) & 0xff;
	(*p)++;
	**p = (v >> 48) & 0xff;
	(*p)++;
	**p = (v >> 40) & 0xff;
	(*p)++;
	**p = (v >> 32) & 0xff;
	(*p)++;
	this->writeInt(p, v);
}

void DataCommunicator::writeDouble(char **p, double v) {
	char *dp = (char *)&v;
	for (int i = 0; i < 8; ++i, (*p)++)
	{
		**p = dp[7 - i] & 0xff;
	}
}

bool DataCommunicator::deserializeTmsg(thrd_msg *tmsg)
{
	memset((char *)tmsg, 0, sizeof(thrd_msg));
	char *p = this->buf;
	tmsg->msg_type = this->readLong(&p);
	tmsg->res_type = this->readLong(&p);
	tmsg->tid = this->readInt(&p);
	tmsg->time = this->readDouble(&p);
	return true;
}

bool DataCommunicator::serializeTmsg(thrd_msg *tmsg)
{
	// memset((char*)tmsg, 0, sizeof(thrd_msg));
	char *p = this->buf;
	this->writeLong(&p, tmsg->msg_type);
	this->writeLong(&p, tmsg->res_type);
	this->writeInt(&p, tmsg->tid);
	this->writeDouble(&p, tmsg->time);
	return true;
}

bool DataCommunicator::sendTMsg(thrd_msg *tmsg)
{
	if (serializeTmsg(tmsg))
	{
		if (send(this->client.data_socket, this->buf, this->thrdMsgLen, 0) != -1)
		{
			return true;
		}
	}
	return false;
}
bool DataCommunicator::receiveTMsg(thrd_msg *tmsg)
{
	int len;
	if ((len = recv(this->client.data_socket, this->buf, this->thrdMsgLen, 0)) > 0)
	{
		return deserializeTmsg(tmsg);
	}

	return false;
}

void DataCommunicator::handleData()
{
	//char *p;
	ssmTimeT time;
	//printf("%p",p);
	//p = nullptr;
	while (true)
	{
		if (!receiveData())
		{
			fprintf(stderr, "receiveData Error happends\n");
			break;
		}
		//p = &mData[ssmHeaderSize];

		time = *(reinterpret_cast<ssmTimeT *>(mData));
		pstream->write(time);
#ifdef DEBUG
		// std::cout << "tid " << this->pstream->timeId << "\n";
		// dssm::util::hexdump(&mData[sizeof(ssmTimeT)], mDataSize);
		// std::cout << std::endl;
#endif
	}
	// pstream->showRawData();
}

//とりあえず新しいデータを送り続けるモードだけで考えてみる
void DataCommunicator::handleBuffer()
{
	std::cout << "Inside Handle Buffer" << std::endl;
	SSM_tid tid = -1;
	int count = 0;
	bool loop = true;
	while (loop)
	{
		//read TIME_ID
		if (pstream->isUpdate())
		{
			if (!pstream->read(tid))
			{
				fprintf(stderr, "[%s] SSMApi::read error.\n", pstream->getStreamName());
			}
			char *p = mData;
			proxy->writeInt(&p, pstream->timeId);
			proxy->writeDouble(&p, pstream->time);
			if (!sendBulkData(mData, mFullDataSize))
			{
				fprintf( stderr, "[%d] ", count );
				perror("send bulk Error");
				count ++;
//				if(count > 10){loop = false;}	// コメントアウト
				loop = false;
			}
		}
		else{usleepSSM(1000);}
	}
}

bool DataCommunicator::sendBulkData(char *buf, uint64_t size)
{
#ifdef DEBUG
	// dssm::util::hexdump(buf, size);
#endif
	if (send(this->client.data_socket, buf, size, 0) != -1)
	{
		return true;
	}
	return false;
}

void DataCommunicator::handleRead()
{
	thrd_msg tmsg;
	printf("handleRead\n");
	//printf("setBlocking True\n");
	//pstream->setBlocking(true);
	while (true)
	{
		if (receiveTMsg(&tmsg))
		{
			switch (tmsg.msg_type)
			{
			case TID_REQ:
			{
				tmsg.tid = getTID(pstream->getSSMId(), tmsg.time);
				tmsg.res_type = TMC_RES;
				sendTMsg(&tmsg);
				break;
			}
			case TOP_TID_REQ:
			{
				tmsg.tid = getTID_top(pstream->getSSMId());
				tmsg.res_type = TMC_RES;
				sendTMsg(&tmsg);
				break;
			}
			case BOTTOM_TID_REQ:
			{
				tmsg.tid = getTID_bottom(pstream->getSSMId());
				tmsg.res_type = TMC_RES;
				sendTMsg(&tmsg);
				break;
			}
			case READ_NEXT:
			{
				int dt = tmsg.tid;
				pstream->readNext(dt);
				tmsg.tid = pstream->timeId;
				tmsg.time = pstream->time;
				tmsg.res_type = TMC_RES;
				if (sendTMsg(&tmsg))
				{
					if (!sendBulkData(&mData[sizeof(ssmTimeT)], mDataSize))
					{
						perror("send bulk Error");
					}
				}
				break;
			}
			case TIME_ID:
			{
				SSM_tid req_tid = (SSM_tid)tmsg.tid;
				if (!pstream->read(req_tid))
				{
					fprintf(stderr, "[%s] SSMApi::read error.\n", pstream->getStreamName());
				}
				tmsg.tid = pstream->timeId;
				tmsg.time = pstream->time;
				tmsg.res_type = TMC_RES;
				if (sendTMsg(&tmsg))
				{
					if (!sendBulkData(&mData[sizeof(ssmTimeT)], mDataSize))
					{
						perror("send bulk Error");
					}
				}
				break;
			}
			case REAL_TIME:
			{
				ssmTimeT t = tmsg.time;
				if (!pstream->readTime(t))
				{
					fprintf(stderr, "[%s] SSMApi::readTime error.\n", pstream->getStreamName());
				}
				tmsg.tid = pstream->timeId;
				tmsg.time = pstream->time;
				tmsg.res_type = TMC_RES;
				if (sendTMsg(&tmsg))
				{
					if (!sendBulkData(&mData[sizeof(ssmTimeT)], mDataSize))
					{
						perror("send bulk Error");
					}
				}
				break;
			}
			default:
			{
				//thrd_msg* ptr = &tmsg;
				break;
			}
			}
		}
		else
		{
			break;
		}
	}
}

void *DataCommunicator::run(void *args)
{
	if (isTCP ? rwait() : UDPrwait())
	{
		switch (mType)
		{
		case WRITE_MODE:
		{
			handleData();
			break;
		}
		case READ_MODE:
		{
			handleRead();
			break;
		}
		case BUFFER_MODE:
		{
			fprintf(stderr, "handle BUFFER_MODE\n");
			handleBuffer();
			break;
		}
		default:
		{
			perror("no such mode");
		}
		}
	}
	return nullptr;
}

bool DataCommunicator::sopen()
{
	this->server.wait_socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	int flag = 1;
	//int ret = setsockopt(this->server.wait_socket, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(flag));
	int ret = setsockopt(this->server.wait_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&flag, sizeof(flag));
	if (ret == -1)
	{
		fprintf(stderr, "server setsocket");
		exit(1);
	}
	if (this->server.wait_socket == -1)
	{
		perror("open socket error");
		return false;
	}
	if (bind(this->server.wait_socket,
			 (struct sockaddr *)&this->server.server_addr,
			 sizeof(this->server.server_addr)) == -1)
	{
		perror("data com bind");
		return false;
	}
	if (listen(this->server.wait_socket, 5) == -1)
	{
		perror("data com open");
		return false;
	}

	return true;
}

bool DataCommunicator::sclose()
{
	if (this->client.data_socket != -1)
	{
		close(this->client.data_socket);
		this->client.data_socket = -1;
	}
	if (this->server.wait_socket != -1)
	{
		close(this->server.wait_socket);
		this->server.wait_socket = -1;
	}
	return true;
}

bool DataCommunicator::rwait()
{
	memset(&this->client, 0, sizeof(this->client));
	this->client.data_socket = -1;
	for (;;)
	{
		socklen_t client_addr_len = sizeof(this->client.client_addr);
		this->client.data_socket = accept(this->server.wait_socket,
										  (struct sockaddr *)&this->client.client_addr, &client_addr_len);
		if (this->client.data_socket != -1)
			break;
		if (errno == EINTR)
			continue;
		perror("server open accept");
		return false;
	}
	return true;
}

bool DataCommunicator::UDPsopen()
{	
	client.data_socket = socket(AF_INET, SOCK_DGRAM, 0);
	if (client.data_socket == -1)
	{
		perror("open socket error");
		return false;
	}
	//bindでポートが紐づけられる
	if (bind(client.data_socket,
			 (struct sockaddr *)&server.server_addr,
			 sizeof(this->server.server_addr)) == -1)
	{
		perror("data com bind");
		return false;
	}
	return true;
}

bool DataCommunicator::UDPrwait()
{
	/* UDPでsendを使うため，connectする必要がある．connectの前にrecvfromを呼ぶ必要がある */
	ssize_t len = recvfrom(client.data_socket, buf, sizeof(buf), 0,
			 (struct sockaddr *)&client.client_addr, &address_len);	
	if (len < 0) return false;
	connect(client.data_socket, (struct sockaddr *)&client.client_addr, address_len);	
	return true;
}

//==============================================================================================================================================================================================================================
//=====================================================================================================    PROXY SERVER    =====================================================================================================
//==============================================================================================================================================================================================================================




ProxyServer::ProxyServer()
{
	nport = SERVER_PORT;
	mData = NULL;
	mDataSize = 0;
	ssmHeaderSize = sizeof(ssmTimeT);
	mFullDataSize = 0;
	mProperty = NULL;
	mPropertySize = 0;
	com = nullptr;
	mType = WRITE_MODE;
	dssmMsgLen = dssm::util::countDssmMsgLength();
    msq_id = -1;
    is_check_msgque = 1;
    brdata_len = 0;
    memset(br_buffer, 0, DMSG_MAX_SIZE);
}

ProxyServer::~ProxyServer()
{
	this->server_close();
	free(mData);
	mData = NULL;
	delete com;
	com = nullptr;
}

bool ProxyServer::init()
{
	setupSigHandler();
	memset(&this->server, 0, sizeof(this->server));
	this->server.wait_socket = -1;
	this->server.server_addr.sin_family = AF_INET;
	this->server.server_addr.sin_addr.s_addr = htonl(SERVER_IP);
	this->server.server_addr.sin_port = htons(SERVER_PORT);

    this->open_msgque(); // initialize msg_queue

	return this->open();
}

bool ProxyServer::open_msgque()
{
    if (is_check_msgque) {
        //msq_id = msgget(PRQ_KEY, IPC_CREAT | IPC_EXCL | 0666);
        msq_id = msgget(PRQ_KEY, IPC_CREAT | 0666);
    } else {
        msq_id = msgget(PRQ_KEY, IPC_CREAT | 0666);
    }

    if (msq_id < 0) {
        perror("msgget");
        return false;
    }
	if( errno == EEXIST ) {
		fprintf( stderr, "ERROR : message queue is already exist.\n" );
		fprintf( stderr, "maybe dssm-proxy has started.\n" );
	}

    printf("msgque is ready\n");

    return true;
}

bool ProxyServer::open()
{
	this->server.wait_socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	int flag = 1;
	int ret = setsockopt(this->server.wait_socket, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(flag));
    ret = setsockopt(this->server.wait_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&flag, sizeof(flag));
	if (ret == -1)
	{
		perror("proxy setsockopt");
		exit(1);
	}

	if (this->server.wait_socket == -1)
	{
		perror("open socket error");
		return false;
	}
	if (bind(this->server.wait_socket,
			 (struct sockaddr *)&this->server.server_addr,
			 sizeof(this->server.server_addr)) == -1)
	{
		perror("server bind");
		return false;
	}
	if (listen(this->server.wait_socket, 5) == -1)
	{
		perror("server open");
		return false;
	}

	return true;
}

bool ProxyServer::wait()
{
	memset(&this->client, 0, sizeof(this->client));
	this->client.data_socket = -1;
	for (;;)
	{
		socklen_t client_addr_len = sizeof(this->client.client_addr);
		this->client.data_socket = accept(this->server.wait_socket,
										  (struct sockaddr *)&this->client.client_addr, &client_addr_len);
		if (this->client.data_socket != -1)
			break;
		if (errno == EINTR)
			continue;
		perror("server open accept");
		return false;
	}
	return true;
	fprintf(stderr, "hwait::End\n");
}

bool ProxyServer::server_close()
{
	if (this->server.wait_socket != -1)
	{
		close(this->server.wait_socket);
		this->server.wait_socket = -1;
	}
	return true;
}

bool ProxyServer::client_close()
{
	if (this->client.data_socket != -1)
	{
		close(this->client.data_socket);
		this->client.data_socket = -1;
	}
	return true;
}

int ProxyServer::readInt(char **p)
{
	uint8_t v1 = **p;
	(*p)++;
	uint8_t v2 = **p;
	(*p)++;
	uint8_t v3 = **p;
	(*p)++;
	uint8_t v4 = **p;
	(*p)++;

	int v = (int)(v1 << 24 | v2 << 16 | v3 << 8 | v4);
	return v;
}

uint64_t ProxyServer::readLong(char **p)
{
	uint8_t v1 = **p;
	(*p)++;
	uint8_t v2 = **p;
	(*p)++;
	uint8_t v3 = **p;
	(*p)++;
	uint8_t v4 = **p;
	(*p)++;
	uint8_t v5 = **p;
	(*p)++;
	uint8_t v6 = **p;
	(*p)++;
	uint8_t v7 = **p;
	(*p)++;
	uint8_t v8 = **p;
	(*p)++;

	uint64_t lv = (uint64_t)((uint64_t)v1 << 56 | (uint64_t)v2 << 48 | (uint64_t)v3 << 40 | (uint64_t)v4 << 32 | (uint64_t)v5 << 24 | (uint64_t)v6 << 16 | (uint64_t)v7 << 8 | (uint64_t)v8);
	return lv;
}

double ProxyServer::readDouble(char **p)
{
	char buf[8];
	for (int i = 0; i < 8; ++i, (*p)++)
	{
		buf[7 - i] = **p;
	}
	return *(double *)buf;
}

void ProxyServer::readRawData(char **p, char *d, int len)
{
	for (int i = 0; i < len; ++i, (*p)++)
	{
		d[i] = **p;
	}
}
//char **pにvを書き込む
void ProxyServer::writeInt(char **p, int v)
{
	**p = (v >> 24) & 0xff;
	(*p)++;
	**p = (v >> 16) & 0xff;
	(*p)++;
	**p = (v >> 8) & 0xff;
	(*p)++;
	**p = (v >> 0) & 0xff;
	(*p)++;
}
//char **pにvを書き込む
void ProxyServer::writeLong(char **p, uint64_t v)
{
	**p = (v >> 56) & 0xff;
	(*p)++;
	**p = (v >> 48) & 0xff;
	(*p)++;
	**p = (v >> 40) & 0xff;
	(*p)++;
	**p = (v >> 32) & 0xff;
	(*p)++;
	this->writeInt(p, v);
}

void ProxyServer::writeDouble(char **p, double v)
{
	char *dp = (char *)&v;
	for (int i = 0; i < 8; ++i, (*p)++)
	{
		**p = dp[7 - i] & 0xff;
	}
}

void ProxyServer::writeRawData(char **p, char *d, int len)
{
	for (int i = 0; i < len; ++i, (*p)++)
		**p = d[i];
}

void ProxyServer::deserializeMessage(ssm_msg *msg, char *buf)
{
	msg->msg_type = readLong(&buf);
	msg->res_type = readLong(&buf);
	msg->cmd_type = readInt(&buf);
	readRawData(&buf, msg->name, 32);
	msg->suid = readInt(&buf);
	msg->ssize = readLong(&buf);
	msg->hsize = readLong(&buf);
	msg->time = readDouble(&buf);
	msg->saveTime = readDouble(&buf);
}

int ProxyServer::receiveMsg(ssm_msg *msg, char *buf)
{
	int len = recv(this->client.data_socket, buf, this->dssmMsgLen, 0);
	if (len > 0)	
	{
		deserializeMessage(msg, buf);
	}
	return len;
}

int ProxyServer::sendMsg(int cmd_type, ssm_msg *msg)
{
	ssm_msg msgbuf;
//	uint64_t len;
	int len;
	char *buf, *p;
	if (msg == NULL)
	{
		msg = &msgbuf;
	}
	msg->cmd_type = cmd_type;
	buf = (char *)malloc(sizeof(ssm_msg));
	p = buf;
	writeLong(&p, msg->msg_type);
	writeLong(&p, msg->res_type);
	writeInt(&p, msg->cmd_type);
	writeRawData(&p, msg->name, 32);
	writeInt(&p, msg->suid);
	writeLong(&p, msg->ssize);
	writeLong(&p, msg->hsize);
	writeDouble(&p, msg->time);
	writeDouble(&p, msg->saveTime);

	if ((len = send(this->client.data_socket, buf, this->dssmMsgLen, 0)) == -1)
	{
		fprintf(stderr, "error happens\n");
	}

	free(buf);
	return len;
}

void ProxyServer::handleCommand()
{	
	ssm_msg msg;
	char *buf = (char *)malloc(sizeof(ssm_msg));
	while (true)
	{
		int len = receiveMsg(&msg, buf);
		if (len == 0) {
			break;
		}		
		switch (msg.cmd_type & 0x1f)
		{
		case MC_NULL:
		{
			//fprintf(stderr, "MC_NULL\n");
			// for ping
			break;
		}
		case MC_INITIALIZE:
		{
			if (!initSSM())
			{
				fprintf(stderr, "init ssm error in ssm-proxy\n");
				sendMsg(MC_FAIL, &msg);
				break;
			}
			else
			{
				sendMsg(MC_RES, &msg);
			}
			break;
		}
		case MC_CREATE:
		{
			setSSMType(WRITE_MODE);
			mDataSize = msg.ssize;
			mFullDataSize = mDataSize + sizeof(ssmTimeT);
			if (mData)
			{
				free(mData);
			}

			mData = (char *)malloc(mFullDataSize);

			if (mData == NULL)
			{
				fprintf(stderr, "fail to create mData\n");
				sendMsg(MC_FAIL, &msg);
			}
			else
			{
				stream.setDataBuffer(&mData[sizeof(ssmTimeT)], mDataSize);
				if (!stream.create(msg.name, msg.suid, msg.saveTime, msg.time))
				{
					sendMsg(MC_FAIL, &msg);
					break;
				}
				sendMsg(MC_RES, &msg);
			}

			break;
		}
		case MC_OPEN:
		{
			mDataSize = msg.ssize;
			if (mData)
			{
				free(mData);
			}

			SSM_open_mode openMode = (SSM_open_mode)(msg.cmd_type & SSM_MODE_MASK);

			switch (openMode) {
				case SSM_READ: {
					setSSMType(READ_MODE);
					break;
				}
				case SSM_WRITE: {
					setSSMType(WRITE_MODE);
					break;
				}				
				case SSM_READ_BUFFER: {
					//fprintf(stderr, "MC_BUFFER\n");
					ssmHeaderSize += sizeof(SSM_tid);
					setSSMType(BUFFER_MODE);
					break;
				}
				default: {
					fprintf(stderr, "unknown ssm_open_mode\n");
				}
			}
			mFullDataSize = mDataSize + ssmHeaderSize;
			mData = (char *)malloc(mFullDataSize);

			if (mData == NULL)
			{
				fprintf(stderr, "fail to create mData");
				if (openMode == SSM_READ_BUFFER) ssmHeaderSize -= sizeof(SSM_tid);
				sendMsg(MC_FAIL, &msg);
			}
			else
			{
				stream.setDataBuffer(&mData[ssmHeaderSize], mDataSize);
				if (!stream.open(msg.name, msg.suid))
				{
					fprintf(stderr, "stream open failed\n");
					if (openMode == SSM_READ_BUFFER) ssmHeaderSize -= sizeof(SSM_tid);
					endSSM();
					sendMsg(MC_FAIL, &msg);
				}
				else
				{
					sendMsg(MC_RES, &msg);
				}
			}
			break;
		}
		case MC_STREAM_PROPERTY_SET:
		{
			mPropertySize = msg.ssize;
			if (mProperty)
			{
				free(mProperty);
			}
			mProperty = (char *)malloc(mPropertySize);
			if (mProperty == NULL)
			{
				sendMsg(MC_FAIL, &msg);
				break;
			}
			stream.setPropertyBuffer(mProperty, mPropertySize);
			sendMsg(MC_RES, &msg);
			uint64_t len = 0;
			while ((len += recv(this->client.data_socket, &mProperty[len],
								mPropertySize - len, 0)) != mPropertySize)
				;

			if (len == mPropertySize)
			{
				if (!stream.setProperty())
				{
					sendMsg(MC_FAIL, &msg);
					break;
				}
				sendMsg(MC_RES, &msg);
			}
			else
			{
				sendMsg(MC_FAIL, &msg);
			}
			break;
		}
		case MC_STREAM_PROPERTY_GET:
		{
			if (mProperty)
			{
				free(mProperty);
			}
			mPropertySize = msg.ssize;
			mProperty = (char *)malloc(mPropertySize);
			if (mProperty == NULL)
			{
				sendMsg(MC_FAIL, &msg);
				break;
			}
			stream.setPropertyBuffer(mProperty, mPropertySize);

			if (!stream.getProperty())
			{
				fprintf(stderr, "can't get property on SSM\n");
				sendMsg(MC_FAIL, &msg);
				break;
			}

			sendMsg(MC_RES, &msg);
			if (send(this->client.data_socket, mProperty, mPropertySize, 0) == -1)
			{
				fprintf(stderr, "packet send error\n");
			}
			break;
		}
		case MC_OFFSET:
		{
			ssmTimeT offset = msg.time;
			settimeOffset(offset);
			sendMsg(MC_RES, &msg);
			break;
		}
		case MC_TCPCONNECTION:
		{
			msg.suid = nport;
			com = new DataCommunicator(nport, mData, mDataSize, ssmHeaderSize,
									   &stream, mType);
			com->start(nullptr);
			sendMsg(MC_RES, &msg);
			break;
		}
		case MC_UDPCONNECTION:
		{
			msg.suid = nport;
			com = new DataCommunicator(nport, mData, mDataSize, ssmHeaderSize,
									   &stream, mType, false);
			com->start(nullptr);
			sendMsg(MC_RES, &msg);
			break;
		}
		case MC_TERMINATE:
		{
			sendMsg(MC_RES, &msg);
			goto END_PROC;
			break;
		}
		default:
		{
			fprintf(stderr, "NOTICE : unknown msg %d", msg.cmd_type);
			break;
		}
		}
	}

END_PROC:
	free(buf);
	if (com)
	{
		com->wait();
	}
	free(com);
	endSSM();
}

void ProxyServer::setSSMType(PROXY_open_mode mode)
{
	mType = mode;
}

/* for broadcast receiving  */
int ProxyServer::set_rbr_info(BROADCAST_RECVINFO *binfo) {
	binfo->sd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (binfo -> sd < 0) {
		fprintf(stderr, "cannot open broadcast receive socket\n");
		return -1;
	}
	binfo->addr.sin_family = AF_INET;
	binfo->addr.sin_addr.s_addr = htonl(INADDR_ANY);
	binfo->addr.sin_port = htons(binfo->port);
	
	if (bind(binfo->sd, (struct sockaddr*)&binfo->addr, sizeof(binfo->addr)) < 0) {
		fprintf(stderr, "cannot bind receive socket\n");
		return -1;
	}
	return 0;
}

void ProxyServer::rbr_close(BROADCAST_RECVINFO *binfo) {
	if (binfo->sd != 0) close(binfo->sd);
}

std::tuple<std::string , std::string, uint16_t> ProxyServer::parse_data(char* buf, int msg_len) {
	if (msg_len < 0) {
        fprintf(stderr, "something happend in parse\n");
        return {"", "",0};
    }
    int idx = 0;
    uint16_t len = (uint16_t)(buf[idx] << 8 | buf[idx + 1]);
    if (len != msg_len) {
        fprintf(stderr, "something happend in parse\n");
        return {"", "", 0};
    }

    idx += 2;
    uint8_t ip_len = buf[idx++];
    std::string ip_addr_str(&buf[idx], 0,  ip_len);

    idx += ip_len;
    uint8_t port_len = buf[idx++];    

    std::string port_str(&buf[idx], 0, port_len);

    idx += port_len;
    return {ip_addr_str, port_str, (uint16_t)idx};
}

std::tuple<std::string, std::string, uint8_t*, uint16_t> ProxyServer::recv_br_msg(BROADCAST_RECVINFO *binfo) {
	char recv_msg[BR_MAX_SIZE];
    memset(recv_msg, 0, BR_MAX_SIZE);
	int msg_len = recvfrom(binfo->sd, recv_msg, BR_MAX_SIZE, 0, NULL, 0);

    std::tuple<std::string, std::string, uint16_t> info =  parse_data(recv_msg, msg_len);

    uint8_t* recv_data = (uint8_t*)malloc(msg_len - std::get<2>(info));
    uint16_t data_len = msg_len - std::get<2>(info);
    memcpy(recv_data, &recv_msg[std::get<2>(info)], msg_len - std::get<2>(info));
	return {std::get<0>(info), std::get<1>(info), recv_data, data_len};
}

void ProxyServer::receive_notification() {
	std::cout << "receive notification start" << std::endl;
	BROADCAST_RECVINFO binfo;
	memset(&binfo, 0, sizeof(BROADCAST_RECVINFO));
	binfo.port = BR_PORT;
	this->set_rbr_info(&binfo);

	while (true) {
		std::tuple<std::string, std::string, uint8_t*, uint16_t> hinfo = this->recv_br_msg(&binfo);
        std::string ip_addr_str = std::get<0>(hinfo);
        std::string port_str = std::get<1>(hinfo);
        uint8_t* data = std::get<2>(hinfo);
        uint16_t data_len = std::get<3>(hinfo);

        if (!ip_addr_str.empty() && !port_str.empty()) {
            uint16_t port = (uint16_t)std::stoi(port_str);
            Neighbor nbr = (data_len > 0) ? 
                Neighbor(ip_addr_str, port, data_len, data) : 
                Neighbor(ip_addr_str, port);
            
            neighbor_manager.add(nbr); // overwrite if it exists
        }
        free(data);
	}
}

/* end of broadcast receiving  */

/* for broadcast sending */
int ProxyServer::sbr_init(BROADCAST_SENDINFO *binfo, const char *ipaddr, const int port) {
	memset(binfo, 0, sizeof(BROADCAST_SENDINFO));
	binfo->ipaddr = ipaddr;
	binfo->port = port;
	binfo->permission = 1;
	return 0;
}

int ProxyServer::set_sbr_info(BROADCAST_SENDINFO *binfo) {
	if ((binfo->sd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
		fprintf(stderr, "cannot open broadcast send socket\n");
		return -1;
	}
	
	if(setsockopt(binfo->sd, SOL_SOCKET, SO_BROADCAST, (void*)&binfo->permission, 
		sizeof(binfo->permission)) != 0) {
		fprintf(stderr, "cannot open socket as broadcast\n");
		return -1;
	}
	binfo->addr.sin_family = AF_INET;
	binfo->addr.sin_addr.s_addr = inet_addr(binfo->ipaddr);
	binfo->addr.sin_port = htons(binfo->port);
	return 0;
}

void ProxyServer::send_br_msg(BROADCAST_SENDINFO *binfo, char *msg, int msg_len) {
	unsigned int sent_len = 0;
    binfo->msg = msg;
    binfo->msg_len = msg_len;
	sent_len = sendto(binfo->sd, binfo->msg, binfo->msg_len, 0, 
		(struct sockaddr*)&(binfo->addr), sizeof(binfo->addr));
	if (sent_len != binfo->msg_len) {
		fprintf(stderr, "sent message is too short\n");
	}
	return;
}
void ProxyServer::sbr_close(BROADCAST_SENDINFO *binfo) {
	if (binfo->sd != 0) close(binfo->sd);	
}

uint16_t ProxyServer::create_msg(char* buffer, std::string ipaddr_str, int port) {
	uint16_t len = 0;	
    std::string port_str = std::to_string(port);
    len += ipaddr_str.length();
    len += port_str.length();
    len += 4;

    {
        std::lock_guard<std::mutex> lock(mtx);
        len += brdata_len;
        int idx = 0;
        buffer[idx++] = (len >> 8) & 0xff;
        buffer[idx++] = (len >> 0) & 0xff;
        buffer[idx++] = ipaddr_str.length() & 0xff;
        memcpy(&buffer[idx], ipaddr_str.c_str(), ipaddr_str.length());
        idx += ipaddr_str.length();
        buffer[idx++] = port_str.length();    
        memcpy(&buffer[idx], port_str.c_str(), port_str.length());    

        idx += port_str.length();
        memcpy(&buffer[idx], br_buffer, brdata_len);
    }

	return len;
}



void ProxyServer::send_notification() {
	//std::cout << "send notification start" << std::endl;
	std::pair<std::string, std::string> ainfo = get_interface_info();
	if (ainfo.first.empty()) {
        fprintf(stderr, "something happend\n");
        return;
    }
	//std::cout << ainfo.first << std::endl;
	//std::cout << ainfo.second << std::endl;
	char buffer[BR_MAX_SIZE];
    //uint16_t _msg_len = create_msg(buffer, ainfo.first, SERVER_PORT); // specify self ip
    //create_msg(buffer, ainfo.first, SERVER_PORT); // specify self ip
    //uint16_t total_len = (uint16_t)(buffer[0] << 8 | buffer[1]);    

	BROADCAST_SENDINFO binfo;
	this->sbr_init(&binfo, ainfo.second.c_str(), BR_PORT);
	this->set_sbr_info(&binfo);


	while (true) {
		//std::cout << "send..." << std::endl;
        create_msg(buffer, ainfo.first, SERVER_PORT); // specify self ip
        uint16_t total_len = (uint16_t)(buffer[0] << 8 | buffer[1]);    
		this->send_br_msg(&binfo, buffer, total_len);
		sleep(5);
	}

	this->sbr_close(&binfo);
}
/* end of for send broadcast */


/* update br_message */
void ProxyServer::update_brdata(uint8_t* data, uint16_t len) {
    std::cout << "update brdata " << len << std::endl;
    std::lock_guard<std::mutex> lock(mtx);
    memcpy(br_buffer, data, len);
    brdata_len = len;
}




void ProxyServer::handle_msg() {
    dssm_msg dmsg;
    int len;
    
    printf("handle msg is invoked\n");
    while (true) {
        len = msgrcv(msq_id, &dmsg, DMSG_SIZE, DMSG_CMD, 0);
        if (len < 0) {
            if (errno == EINTR) continue;
            perror("msgrcv");
            break;
        }
        switch (dmsg.cmd_type) {
            case DMC_NULL: { // do nothing
                break;
            }
            case DMC_BR_START: {
                printf("br_start\n");
                Neighbor testnb = Neighbor("10.0.0.1", 9999);
                neighbor_manager.add(testnb);

                update_brdata((uint8_t*)dmsg.data, dmsg.data_len);
                dmsg.msg_type = dmsg.res_type;
                dmsg.cmd_type = DMSG_RES;
                dmsg.res_type = DMC_REP_OK;

                if (msgsnd(msq_id, &dmsg, DMSG_SIZE, 0) < 0) {
                    perror("msgsnd");
                }
                break;
            }
            case DMC_BR_RECEIVE: {
                //std::cout << "receive broadcast" << std::endl;
                dmsg.msg_type = dmsg.res_type;
                dmsg.cmd_type = DMSG_RES;
                dmsg.res_type = DMC_REP_FAIL;
                int count = neighbor_manager.count();

                if (count > 0) {
                    // for test
                    //Neighbor testnb = Neighbor("10.0.0.1", 9999);
                    //neighbor_manager.add(testnb);
                    // end for test
                    std::vector<uint8_t> buffer = neighbor_manager.serialize();

                    /*
                    for (int i = 0; i < buffer.size(); ++i) {
                        if (i % 16 == 0) printf("\n");
                        printf("%02x ", buffer[i]);
                    }
                    printf("\n");
                    */

                    uint8_t* data = buffer.data();
                    dmsg.data_len = (uint16_t)buffer.size();
                    memcpy(dmsg.data, data, dmsg.data_len);
                    dmsg.res_type = DMC_REP_OK;
                }

                if (msgsnd(msq_id, &dmsg, DMSG_SIZE, 0) < 0) {
                    perror("msgsnd");
                }
                break;
            }
            case DMC_BR_STOP: {
                break;
            }
            default: {
                printf("unknown msg %d\n", dmsg.cmd_type);
                break;
            }
        }
    }   

    printf("end of handle msg\n");
}




bool ProxyServer::run(bool notify)
{
	if (notify) {
		if (!initSSM()) {
			fprintf(stderr, "init ssm error in ssm-proxy run\n");
			return false;
		} 

		std::thread recv_noifth([this] () {
			this->receive_notification();
		});
		recv_noifth.detach();

		std::thread send_notith([this]() {			
			this->send_notification();
		});
		send_notith.detach();
	}

    std::thread handle_msgth([this]() {
        this->handle_msg();
    });
    handle_msgth.detach();


	while (wait())
	{
		++nport;
		pid_t child_pid = fork();
		if (child_pid == -1)
		{ // fork failed
			break;
		}
		else if (child_pid == 0)
		{ // child
			this->server_close();
			this->handleCommand();
			this->client_close();
			//fprintf(stderr, "end of process");
			exit(1);
		}
		else
		{ // parent
			this->client_close();
			this->pids.push_back(child_pid);
			//printf("child_pid = %d\n", child_pid);
		}
	}
	this->server_close();	
	return true;
}


void ProxyServer::setupSigHandler()
{
	//std::cout << "end" << std::endl;
	struct sigaction act;
	memset(&act, 0, sizeof(act));				/* sigaction構造体をとりあえずクリア */
	act.sa_handler = &ProxyServer::catchSignal; /* SIGCHLD発生時にcatch_SIGCHLD()を実行 */
	sigemptyset(&act.sa_mask);					/* catch_SIGCHLD()中の追加シグナルマスクなし */
	act.sa_flags = SA_NOCLDSTOP | SA_RESTART;
	sigaction(SIGCHLD, &act, NULL);

	
}

void ProxyServer::catchSignal(int signo)
{
	printf("catch signal\n");
	pid_t child_pid = 0;
	/* すべての終了している子プロセスに対してwaitpid()を呼ぶ */
	do
	{
		int child_ret;
		child_pid = waitpid(-1, &child_ret, WNOHANG);
		/* すべての終了している子プロセスへwaitpid()を呼ぶと
		 WNOHANGオプションによりwaitpid()は0を返す */
	} while (child_pid > 0);
}

bool ProxyServer::shutdown_children() {
	for (int i = 0; i < (int)this->pids.size(); ++i) {
		if (kill(this->pids.at(i), SIGKILL) == -1) {
			return false;
		}
	}
	this->pids.clear();
	return true;
}

static ProxyServer *pserver = nullptr;

static void signal_handler(int signum) {
	if (pserver != nullptr) {
		pserver->shutdown_children();
		delete pserver;
		pserver = nullptr;
	}
	exit(EXIT_SUCCESS);

}



int main(int argc, char* argv[])
{
	std::cout << std::endl;
	std::cout << " --------------------------------------------------\n";
	std::cout << " DSSM ( Distribute Streaming data Sharing Manager )\n";
	std::cout << " Ver. 1.3\n";
	std::cout << " broadcast option available (./dssm-proxy -b)\n";
	std::cout << " --------------------------------------------------\n\n";


	/* Use Broadcast Feature */
	bool use_broadcast = false;
	if (argc > 1 && std::string(argv[1]) == "-b") {
		std::cout << "use broadcast " << std::endl;
		use_broadcast = true;
	}
	

	struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_handler = signal_handler;
    sa.sa_flags |= SA_RESTART;
    sigaction(SIGINT, &sa, NULL);

	pserver = new ProxyServer();
	pserver->init();
	pserver->run(use_broadcast);	
	if (pserver != nullptr) {
		delete pserver;
	}

	return 0;
}
