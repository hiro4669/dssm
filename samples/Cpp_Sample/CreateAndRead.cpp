#include <iostream>
#include <signal.h>

#include <ssm.hpp>
#include "intSsm.h"

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

int main(int aArgc, char **aArgv) {

    std::cout << "Hello World\n" << std::endl;

    SSMApi<intSsm_k> intSsm(SNAME_INT, 1);

    if (!initSSM()) return 0;

    if (!intSsm.create(5.0, 1.0)) return 1;

    std::cout << "init success\n" << std::endl;

    while (!gShutOff) {
        if (intSsm.readNew()) {
            std::cout << "NUM = " << intSsm.data.num << std::endl;
        }
        std::cout << "hoge" << std::endl;
        sleepSSM(1);
    }

    intSsm.release();
    endSSM();
    std::cerr << "end." << std::endl;


    return 0;
}
