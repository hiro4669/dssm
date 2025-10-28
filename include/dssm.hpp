// テンプレートを利用した実装はヘッダに書くこと(リンカに怒られる)
/*
 * SSMの宣言に仕様する
 * ほとんどSSMApiと同じように使用できる
 */

#ifndef _DSSM_PROXY_CLIENT_CHILD_HPP_
#define _DSSM_PROXY_CLIENT_CHILD_HPP_

#include "dssm-proxy-client.hpp"
#include "dssm-utility.hpp"
#include <sys/poll.h>
#include <vector>
#include <iostream>
#include <thread>
#include <functional>
#include <cstring>
#include <vector>

#include "dssm-def.hpp"

//template <typename T, typename P = DSSMDummy>
template <typename T, typename P = DSSMDummy, typename U = DSSMDummy>
class DSSMApi : public PConnector
{
private:
	dssm::rbuffer::RingBuffer<T> ringBuf;
	struct ssmData
	{
		ssmTimeT time;
		T ssmRawData;
	};
	void initApi(const char *ipAddr)
	{
		fulldata = malloc(sizeof(T) + sizeof(ssmTimeT)); // メモリの開放はどうする？ -> とりあえずデストラクタで対応
		wdata = (T *)&(((char *)fulldata)[8]);
        pbr_data = &br_data;
		PConnector::setBuffer(&data, sizeof(T), &property, sizeof(P), fulldata);
		PConnector::setIpAddress(ipAddr);
	}

	void initApi()
	{
		fulldata = malloc(sizeof(T) + sizeof(ssmTimeT)); // メモリの開放はどうする？ -> とりあえずデストラクタで対応
		wdata = (T *)&(((char *)fulldata)[8]);
        pbr_data = &br_data;
		PConnector::setBuffer(&data, sizeof(T), &property, sizeof(P), fulldata);
	}


public:
	T data;
	T *wdata;
	P property;
	void *fulldata;

    U br_data;
    U *pbr_data;

    struct BrInfo {
        std::string ipaddr;
        std::string port;
        bool data_flg;
        U data;
    };

	DSSMApi() {
		initApi();
	}
	// 委譲
	DSSMApi(const char *streamName, int streamId = 0, const char *ipAddr = "127.0.0.1") : PConnector::PConnector(streamName, streamId) {
		initApi(ipAddr);
	}
	//デストラクタ
	~DSSMApi()
	{
		// std::cout << __PRETTY_FUNCTION__ << std::endl;
		free(fulldata);
//		free(wdata);
	}

    bool sendBroadcast() {
        dssm_msg msg;
        memset(msg.data, 0, DMSG_MAX_SIZE);
        msg.data_len = sizeof(U);
        memcpy(msg.data, (char*)pbr_data, sizeof(U));
        send_msg(DMC_BR_START, &msg);
        receive_msg(&msg);
        return true;
    }

    std::vector<BrInfo> receiveBroadcast() {
        BrInfo bInfo;
        std::vector<BrInfo> iList;

        dssm_msg msg;
        send_msg(DMC_BR_RECEIVE, NULL);
        receive_msg(&msg);

        if (msg.res_type != DMC_REP_OK) {
            fprintf(stderr, "msg cannot received correctly\n");
            return iList;
        }

        // received length will be used in the future
        //uint32_t len = (uint32_t)(msg.data[3] << 24 | msg.data[2] << 16 | msg.data[1] << 8 | msg.data[0]);
        /*
        printf("len = %d\n", len);
        printf("\n>>> Binary <<<\n");
        for (int i = 0; i < len; ++i) {
            if (i % 16 == 0) printf("\n");
            printf("%02x ", msg.data[i]);
        }
        printf("\n");
        */

        uint16_t count = (uint16_t)(msg.data[5] << 8 | msg.data[4]);
        int idx = 6;
        for (int i = 0; i < count; ++i) {
            int ip_len = (uint16_t)(msg.data[idx+1] << 8 | msg.data[idx]);
            idx += 2;
            std::string ipaddr_str((char*)&msg.data[idx], ip_len);
            idx += ip_len;
            int port_len = (uint16_t)(msg.data[idx+1] << 8 | msg.data[idx]);
            idx += 2;
            std::string port_str((char*)&msg.data[idx], port_len);
            idx += port_len;
            int data_len = (uint16_t)(msg.data[idx+1] << 8 | msg.data[idx]);
            idx += 2;

            bInfo.ipaddr = ipaddr_str;
            bInfo.port = port_str;
            bInfo.data_flg = false;
            if (data_len > 0) {
                memcpy(&bInfo.data, &msg.data[idx], data_len);
                bInfo.data_flg = true;
                idx += data_len;
            }
            iList.push_back(bInfo);
            printf("i = %d\n", i);
        }
        //printf("host count = %ld\n", iList.size());

        return iList;
    }

	// Hiroaki Fukuda
	void bulkReadThreadMain() {
		//fprintf(stderr, "bulkReadThreadMain Start\n");
		bool loop = true;
		std::vector<char> recvBuf(sizeof(T) + sizeof(ssmTimeT) + sizeof(SSM_tid));
		uint64_t total_size = sizeof(T) + sizeof(ssmTimeT) + sizeof(SSM_tid);
//		ssmData ssmRecvData;
		char* buffer = recvBuf.data();
		while (loop) {
			uint64_t len = 0;						
			while (true) {
				int rsize = recv(dsock, &buffer[len], total_size - len, 0);
				if (rsize == 0) goto exit;
				len += rsize;
				if (len == total_size) {
					break;
				}
			}			
			char *p = &buffer[0];
			SSM_tid tid = readInt(&p);
			ssmTimeT time = readDouble(&p);
			T toridata = *(reinterpret_cast<T *>(p));

			// for debug
			//fprintf(stderr, "[BUF(%d)] received tid=%d, time=%lf\n", ringBuf.getBufferSize(),tid, time);			
			ringBuf.writeBuffer(toridata, tid, time);
		}
exit:;
}

	// TAKUTO // not used 2023/08/12
	void rBufReadTask()
	{
		struct pollfd polldata;
		bool loop = true;
		int cnt = 0;
		polldata.events = POLLIN;
		polldata.revents = 0;
		polldata.fd = dsock;
		std::vector<char> recvBuf(sizeof(T) + sizeof(ssmTimeT) + sizeof(SSM_tid));
		ssmData ssmRecvData;
		//printf("cnt %d",cnt);
		//printf("size of ssm_tid = %d\n", sizeof(SSM_tid)); // 4
		//printf("size of ssmTimeT = %d\n", sizeof(ssmTimeT)); // 8
		while (loop)
		{
			int retv  = poll(&polldata, 1, 10);
			//fprintf(stderr, "retv = %d\n", retv);
			switch (retv)
			{
			case -1:
				perror("poll");
				fprintf(stderr, "poll= -1\n");				
				break;
			case 0:
				//fprintf(stderr, "poll= 0\n");
				break;
			default:
				ssize_t ret = recv(dsock, recvBuf.data(), mFullDataSize + 4, 0);
				fprintf(stderr, "sock ret = %d\n", (int)ret);
				char *p = recvBuf.data();
				SSM_tid tid = readInt(&p);
				ssmTimeT time = readDouble(&p);
				T toridata = *(reinterpret_cast<T *>(p));
				ringBuf.writeBuffer(toridata, tid, time);
				// int homeTID = readInt(&recvBuf[0]);
				/*
				ssmRecvData.time = readDouble(&recvBuf[4]);
				readRawData(&recvBuf[12], (char *)&ssmRecvData.ssmRawData, sizeof(T));
				std::cout << "received data " << std::endl;
				loop = false;
				ringBuf.writeBuffer(ssmRecvData.ssmRawData);
				*/
				break;
			};
		}
	}

	void readyRingBuf( ssmTimeT life = 0.0, ssmTimeT cycle = 0.0 )
	{
		if (life != 0.0) {
			this->life = life;
		}
		if (cycle != 0.0) {
			this->cycle = cycle;
		}

		if( this->life <= 0.0 ){
			fprintf( stderr, "DSSM ERROR : create : stream life time err.\n" );
		}
		if( this->cycle <= 0.0 ){
			fprintf( stderr, "DSSM ERROR : create : stream cycle err.\n" );
//			strcpy( err_msg, "arg error : cycle" );
			exit( EXIT_FAILURE );
		}
		if( this->life < this->cycle ){
			fprintf( stderr, "DSSM ERROR : create : stream saveTime MUST be larger than stream cycle.\n" );
//			strcpy( err_msg, "arg err : life, c" );
			exit( EXIT_FAILURE );
		}
		//スレッドスタート
		fprintf(stderr, "create table %f, %f\n", this->life, this->cycle);
		//int bufnum = calcSSM_table( this->life, this->cycle ) * sharedSize(  );
		int bufnum = calcSSM_table( this->life, this->cycle );		
		//int bufnum = calcSSM_table( life, cycle ) * sharedSize(  );
		std::cout << "Starting Ring Buffer (" << bufnum << "[byte])" << std::endl;
		// バッファのリセット
		ringBuf.reset();
		// 現在読み込んだtidのリセット
		timeId = -1;
		// リングバッファのリセット
		ringBuf.setBufferSize(bufnum);
		std::cout << "RBuf is ready" << std::endl;
		//std::thread readThread([this]{ rBufReadTask(); });

		std::thread readThread([this]{ bulkReadThreadMain(); });
		std::cout << "Created Thread." << std::endl;		
		readThread.detach();
	}

	/* read */
	bool readBuf(SSM_tid tid_in = -1)
	{	
		// for test
		if(!ping()) {			
			this->reconnreadbuf();
			//fprintf(stderr, "bufTid = %d\n", ringBuf.getTID_top());
		}
		switch (ringBuf.read(tid_in, this->data, this->timeId, this->time))
		{
		case 1: // Success
			break;
		case SSM_ERROR_FUTURE:
			std::cout << "SSM_ERROR_FUTURE" << std::endl;
			return false;			
		case SSM_ERROR_PAST:
			std::cout << "SSM_ERROR_PAST" << std::endl;
			//return read(tid_in);
			return false;
		case SSM_ERROR_NO_DATA:
			std::cout << "SSM_ERROR_NO_DATA" << std::endl;
			//return read(tid_in);
			return false;
		}
		return true;
	}

	/** @brief 最新であり、前回読み込んだデータとバッファの最新データが異なる時にデータを読む
	 * @return データを読み込めたときtrueを返す
	 */
	bool readNewBuf()
	{
		//fprintf(stderr, "[readNewBuf] tid_top = %d ctimeId = %d, write_p=%d\n", this->ringBuf.getTID_top(), this->timeId, this->ringBuf.getWriteP());
		if (this->timeId < this->ringBuf.getTID_top()) {						
			return (isOpen() ? readBuf(-1) : false);
		} else {
			if (!ping())  return readBuf(-1);
		}
		return false;
	}

	/** @brief 前回読み込んだデータの次のデータを読み込む
	 * @param[in] dt 進む量
	 * @return 指定されたdtだけTIDが進んだ次データを読み込めたときtrueを返す
	 *
	 */
	bool readNextBuf(int dt = 1)
	{
		int tid_in = this->timeId + dt;
		switch (ringBuf.read(tid_in, this->data, this->timeId, this->time))
		{
		case 1: // Success
			break;
		case SSM_ERROR_FUTURE:
			std::cout << "BUFFER ERROR FUTURE" << std::endl;
			return false;			
		case SSM_ERROR_PAST:
			std::cout << "BUFFER ERROR PAST" << std::endl;			
			return false;			
		case SSM_ERROR_NO_DATA:
			return false;
			break;
		}
		return true;
	}

	// 前回のデータの1つ(以上)前のデータを読み込む
	bool readBackBuf(int dt = 1)
	{
		return (dt <= timeId ? readBuf(timeId - dt) : false);
	}

	// 新しいデータを読み込む
	bool readLastBuf()
	{
		return readBuf(-1);
	}

	bool readTimeBuf(ssmTimeT ytime)
	{
		SSM_tid tid;

		/* 時刻テーブルからTIDを検索 */
		if (ytime <= 0)
		{ /* timeが負の時は最新データの読み込みとする */
			tid = -1;
		}
		else
		{
			switch (ringBuf.getTID(ytime, tid))
			{
			case 1: // 成功
				break;
			case SSM_ERROR_FUTURE:
				return readBuf(-1);
				//return read(tid);
				break;
			case SSM_ERROR_PAST:
				//return readTime(ytime);
				std::cout << "getTID ERROR PAST" << std::endl;
				return false;
				break;
			default:
				tid = -1;
				break;
			}
		}
		return readBuf(tid);
	}

	SSM_tid getTID_topBuf(SSM_sid sid){
		if(!ping()) {
			this->reconnreadbuf();
		}
		return ringBuf.getTID_top();
	}
};

template <typename S, typename V>
using DSSMApiWithProp = DSSMApi<S, DSSMDummy, V>;

#endif
