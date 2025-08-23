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
inline void setSigInt(){ signal(SIGINT, ctrlC); }

int main() {
	/*
	 * 変数の宣言
	 * PConnectorClient<data型, property型> 変数名(ssm登録名, ssm登録番号, 接続するproxy(server)のipAddress);
	 * property型は省略可能、省略するとpropertyにアクセスできなきなくなるだけ
	 * ssm登録番号は省略可能、省略すると0に設定される
	 * ssm登録名は./intSsm.hに#define SNAME_INT "intSsm"と定義
	 * data型とproperty型は ./intSsm.h に定義
	 * 指定しているIPはループバックアドレス(自分自身)
	 */
//	DSSMApi<intSsm_k, doubleProperty_p> con(SNAME_INT, 1);
	DSSMApi<intSsm_k, props_p> con(SNAME_INT, 1);

	// dssm関連の初期化
//	con.initDSSM();
	con.initRemote( );

	// 共有メモリにすでにある領域を開く
	// 失敗するとfalseを返す
	if (!con.open(SSM_READ)) {
		// terminate()でssm-coordinatorとの接続を切断する
		con.terminate();
		return 1;
	}

	// 指定したプロパティを取得
	con.getProperty();

	// プロパティは 変数名.property.データ でアクセス
	printf("property dnum: -> %f\n", con.property.dnum);
	printf("property num :  -> %d\n", con.property.num);
	printf("property name:  -> %s\n", con.property.name);

	// データ通信路を開く
	// これをしないとデータを取得できない
	//con.setConnectType(TCP_CONNECT);
	con.setConnectType(UDP_CONNECT);
	if (!con.createDataCon()) {
	//if (!con.TCPcreateDataCon()) {
		con.terminate();
		return 1;
	}

	// 安全に終了できるように設定
	setSigInt();
	double ttime = 0.0;
	printf("%f",ttime);
	while (!gShutOff) {

		// 最新のデータを取得
		/*
		if (con.readNew()) {
			printf("\n");
			printf("now -> %f\n", con.time);
			printf("now timeId -> %d\n", con.timeId);
			cout << "NUM = " << con.data.num << endl;
		}
		*/
		

		// 1秒前のデータを取得
		/*
		if (con.readTime(con.time - 1)) {
			printf("\n");
			printf("before 1 sec -> %f\n", con.time);
			cout << "old NUM = " << con.data.num << endl;
		}
		*/

		// 次の指定したtimeIDのデータを取得
		if (con.readNext(5)) {
			printf("\n");
			printf("readNext: timeId -> %d\n", con.timeId);
			cout << "readNext: NUM = " << con.data.num << endl;
		}

		sleepSSM(1);
	}

	// プログラム終了後は切断
	con.terminate();
}
