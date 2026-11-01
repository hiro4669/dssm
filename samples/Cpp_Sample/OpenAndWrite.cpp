#include <iostream>

#include <signal.h>
#include <unistd.h>


#include <ssm.hpp>
#include "intSsm.h"

bool gShutOff = false;

void ctrlC(int aStatus)
{ 
	signal(SIGINT, NULL);
	gShutOff = true;
}
// Ctrl-C による正常終了を設定
inline void setSigInt(){ signal(SIGINT, ctrlC); } 


// メイン
int main(int aArgc, char **aArgv) {

    std::cout << "OpenAndWrite" << std::endl;

    SSMApi<intSsm_k> intSsm(SNAME_INT, 1);

    if (!initSSM()) return 0;

    if (!intSsm.open(SSM_WRITE)) {
//    if (!intSsm.open(SSM_READ)) {
        std::cerr << "Failed to open SSM stream." << std::endl;
        endSSM();
        return 1;
    }

    setSigInt();

    std::cout << "start." << std::endl;

    int cnt = 0;

    while (!gShutOff) {
        intSsm.data.num = cnt;
        if (!intSsm.write()) {
            std::cerr << "Failed to write to SSM stream." << std::endl;
        } else {
            std::cout << "Wrote NUM = " << intSsm.data.num << std::endl;
        }
        cnt++;
        sleepSSM(1);
    }

    intSsm.release();
    endSSM();
    std::cout << "end." << std::endl;

    return 0;
}

