/*
 *	ネットワークを介したread機能のテスト
 *	1秒に1回書き込まれるデータを取ってきて表示する．
 */

// c++系
#include <iostream>
#include <iomanip>
// c系
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
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
inline void setSigInt() { signal(SIGINT, ctrlC); }

int main()
{
	/*
	 * 変数の宣言
	 * PConnectorClient<data型, property型> 変数名(ssm登録名, ssm登録番号, 接続するproxy(server)のipAddress);
	 * property型は省略可能、省略するとpropertyにアクセスできなきなくなるだけ
	 * ssm登録番号は省略可能、省略すると0に設定される
	 * ssm登録名は./intSsm.hに#define SNAME_INT "intSsm"と定義
	 * data型とproperty型は ./intSsm.h に定義
	 * 指定しているIPはループバックアドレス(自分自身)
	 */
	//PConnectorClient<intSsm_k, doubleProperty_p> con(SNAME_INT, 1, "127.0.0.1");
//	PConnectorClient<intSsm_k, doubleProperty_p> con(SNAME_INT, 1);
	DSSMApi<intSsm_k, doubleProperty_p> con(SNAME_INT, 1);

	// dssm関連の初期化
//	con.initDSSM();
	con.initRemote( );

	// 共有メモリにすでにある領域を開く
	// 失敗するとfalseを返す
  // SSM_EXCLUSIVEでバッファを使用した読み込みを指定
//	if (!con.open(SSM_EXCLUSIVE))
	if (!con.open(SSM_READ_BUFFER))
	{
		// terminate()でssm-coordinatorとの接続を切断する
		con.terminate();
		return 1;
	}
	// 指定したプロパティを取得
	con.getProperty();

	// プロパティは 変数名.property.データ でアクセス
	printf("property -> %f\n", con.property.dnum);

	// データ通信路を開く
	// これをしないとデータを取得できない
	// UDPcreateDataConでUDP通信によるデータ通信
	// createDataConを用いることで従来のTCP通信によるデータ通信が行われる
	//con.setConnectType(UDP_CONNECT);
	con.setConnectType(TCP_CONNECT);
	if (!con.createDataCon()) {
	//if (!con.UDPcreateDataCon()) {
		con.terminate();
		return 1;
	}
  //データを読み込むためのバッファを作成
//	con.readyRingBuf(16);
	con.readyRingBuf( 5.0, 1.0 );
	//con.bulkReadThreadMain();
	// 安全に終了できるように設定
	setSigInt();
	double ttime;
	while (!gShutOff)
	{
				
		// 最新のデータを取得
		if (con.readNewBuf())
		{
			cout << "=================" << endl;
			cout << "NOW: " << con.time << endl;
			cout << "NUM: " << con.data.num << endl;
		}

		// 1秒前のデータを取得
		if (con.readTimeBuf(con.time - 1))
		{
			cout << "=================" << endl;
			cout << "1 SEC OLD: " << con.time << endl;
			cout << "OLDNUM: " << con.data.num << endl;
		}
		sleepSSM(1);
	}

	// プログラム終了後は切断
	con.terminate();
}
