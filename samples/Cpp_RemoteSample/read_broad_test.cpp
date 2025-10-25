/*
 *	Read Broadccast Test Program
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

int main() {

    std::cout << "Read Broad Test Program" << std::endl;
	DSSMApi<intSsm_k, props_p, param> con(SNAME_INT, 1);

    con.initRemote();
    
    sleepSSM(1); // wait for init
    if (con.receiveBroadcast()) {
        printf("ival = %d\n", con.br_data.ival);
        printf("dval = %f\n", con.br_data.dval);
        printf("cval = %s\n", con.br_data.cval);;
    }


    


    return 0;
}
