#ifndef _DSSM_DEF_HPP_
#define _DSSM_DEF_HPP_

#include <cstdint>
//** DSSMのための定義を追加 ******************************************* ----->
#define SERVER_PORT 8080	   /* サーバ用PORT */
#define SERVER_IP 0x00000000UL /* サーバ用待ち受けIP */

#define BR_PORT 12345          /* ブロードキャスト待受ポート */
#define MAXRECVSTRING 256      /* ブロードキャストメッセージの最大長 */

#define PRQ_KEY 0x3293

#define DMSG_CMD      1000
#define DMSG_RES      1001
#define DMSG_MAX_SIZE 2048

/* proxy-clientで使うコマンド群 */
typedef enum {
	WRITE_MODE = 1,  // 書き込み時
	READ_MODE = 2,   // 読み出し時
	PROXY_INIT = 3,   // 初期
	BUFFER_MODE = 4
} PROXY_open_mode;

/* ssm-proxy-clientからパケットを送る際の判断信号 */
typedef enum {
	TIME_ID = 0, // send timeid
	REAL_TIME, // send real time
	READ_NEXT, // read next
	SSM_ID, // request ssm id
	TID_REQ, // request time id
	TOP_TID_REQ, // request timeid top
	BOTTOM_TID_REQ, // request timeid bottom
	PACKET_FAILED, // falied
	
	WRITE_PACKET, // writemode packet
	
	TMC_RES,
	TMC_FAIL
} READ_packet_type;

/* SSMObserverコマンドメッセージ */
typedef struct {
	uint64_t msg_type;  /// 宛先
	uint64_t res_type;	/// 返信用
	uint32_t cmd_type;	/// コマンドの種類
	pid_t pid;          /// プロセスID
	uint64_t msg_size;  /// bodyのサイズ
	// char body[];        /// データ & パディング
} ssm_obsv_msg;

/* Threadでやり取りするメッセージ */
typedef struct {
	uint64_t msg_type;
	uint64_t res_type;
	int32_t tid;
	ssmTimeT time;   //
} thrd_msg;

typedef struct {
    long msg_type;
    long res_type;
    int  cmd_type;
    uint8_t name[DMSG_MAX_SIZE];
} dssm_msg;  


enum {
    DMC_NULL = 0,
    DMC_BR_START,
    DMC_BR_STOP,
};
#define DMSG_SIZE (sizeof(dssm_msg) - sizeof(long))


// <-------- DSSMのための定義を追加


///**
 //* @brief Observer間で送信するメッセージ
 //*/
//typedef enum {
	//OBSV_INIT = 0,			 /// 新しいSubscriberを追加
	//OBSV_SUBSCRIBE,      /// Subscriberを追加
	//OBSV_ADD_STREAM,     /// Streamを追加。
	//OBSV_START,          /// Subscriberのスタート
	//OBSV_ADD_CONDITION,  /// 新しい条件を追加
	//OBSV_NOTIFY,         /// 条件が満たされたことを通知
	//OBSV_DELETE,
	//OBSV_RES,
	//OBSV_FAIL,
//} OBSV_msg_type;

///**
 //* @brief Observerに送信する条件
 //*/
//typedef enum {
	//OBSV_COND_LATEST = 0, /// 最新のデータを取得
	//OBSV_COND_TIME,       /// 指定した時刻のデータを取得
	//OBSV_COND_TRIGGER,    /// トリガー。トリガーで指定したストリームがコマンドの条件を満たしたとき、他のストリームもデータを読み出す。
	//OBSV_COND_NO_TRIGGER, /// トリガーではない
//} OBSV_cond_type;

#endif
