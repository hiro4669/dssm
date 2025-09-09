
/*
 * writeのテスト. intSsmを使って, 1秒に1回カウントアップ変数を書き込む.
 */

// c++系
#include <iostream>
#include <iomanip>
#include <string>
// c系
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
// その他
#include <ssm.hpp>
// データ構造
#include "intSsm.h"
// クライアント側
//#include "dssm-proxy-client-child.hpp"
#include "dssm.hpp"
// おまじない
using namespace std;
// 終了するかどうかの判定用変数
bool gShutOff = false;
// シグナルハンドラー
// この関数を直接呼び出すことはない
void ctrlC(int aStatus)
{
	signal(SIGINT, NULL);
	gShutOff = true;
}
// Ctrl-C による正常終了を設定
inline void setSigInt(){ signal(SIGINT, ctrlC); }

typedef struct {
    int ival;
    double dval;
    char cval[32];
} param;


int main(int aArgc, char **aArgv) {
	/*
	 * 変数の宣言
	 * PConnectorClient<data型, property型> 変数名(ssm登録名, ssm登録番号, 接続するproxy(server)のipAddress);
	 * property型は省略可能、省略するとpropertyにアクセスできなきなくなるだけ
	 * ssm登録番号は省略可能、省略すると0に設定される
	 * ssm登録名は./intSsm.hに#define SNAME_INT "intSsm"と定義
	 * data型とproperty型は ./intSsm.h に定義
	 * 指定しているIPはループバックアドレス(自分自身)
	 */
	//DSSMApi<intSsm_k, props_p> con(SNAME_INT, 1);
	DSSMApi<intSsm_k, props_p, param> con(SNAME_INT, 1);

    // set broadcast data
    con.br_data.ival = 123;
    con.br_data.dval = 456.789;
    strncpy(con.br_data.cval, "Hello DSSM", 32);
    void* data = malloc(sizeof(param));
    memset(data, 0, sizeof(param));
    memcpy(data, (char*)con.pbr_data, sizeof(param));

	con.initRemote();

    // send broadcast to proxy server
    con.sendBroadcast();

	// 共有メモリにSSMで領域を確保
	// create 失敗するとfalseを返す
	// con.create( センサデータ保持時間(sec), おおよそのデータ更新周期(sec) )
	if (!con.create(5.0, 1.0)) {
		// ssm-coordinatorから切断
		con.terminate();
		return 1;
	}

	// propertyにデータをセット
    memset(con.property.name, 0, NAME_SIZE);
	con.property.dnum = 1.5;
	con.property.num = 5;
    strncpy(con.property.name, "HELLO DSSM", 10);
    con.property.name[10] = '\0';


	// セットしたデータがメモリに書き込まれる
	con.setProperty();

	// データの送受信路を開く
	// これがないとデータが送信できない
	//con.setConnectType(TCP_CONNECT);
	con.setConnectType(UDP_CONNECT);
	if (!con.createDataCon()) {
	//if (!con.UDPcreateDataCon()) {
		con.terminate();
		return 1;
	}

	// 安全に終了できるように設定
	setSigInt();

	// 書き込む変数
	int cnt = 0;
	// 1秒に1回インクリメント
	while (!gShutOff) {
	  // con.wdata->データ型 で書き込み
	  	cnt+=1;
		con.wdata->num = cnt;
		printf("wrote %d\n", cnt);
		// 引数なしだと現在時刻を書き込み
		con.write();

		// SSM時間に合わせたsleep...だが，speedを1以外に変更できないので引数がそのまま停止時間になる
		sleepSSM(1);
	}	
	// coordinatorからの切断
	con.terminate();

}
