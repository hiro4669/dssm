#include <iostream>
#include <unistd.h>
#include <signal.h>
#include <ssm.h>
#include "intSsm.h"
#include "dssm.hpp"


bool gShutOff = false;

void ctrlC(int aStatus) {
    signal(SIGINT, NULL);
    gShutOff = true;
}

inline void setSigInt() { signal(SIGINT, ctrlC); }

int main() {

    std::cout << "OpenAndWriteProxyTest" << std::endl;

    DSSMApi<intSsm_k, props_p> con(SNAME_INT, 1);

    con.initRemote();

    if (!con.open(SSM_WRITE)) {
//    if (!con.open(SSM_READ)) {
        std::cout << "Open Fail" << std::endl;
        con.terminate();
        return 1;
    }

    std::cout << "Open OK" << std::endl;

    con.setConnectType(UDP_CONNECT);
    if (!con.createDataCon()) {
        std::cout << "Create Data Connection Fail" << std::endl;
        con.terminate();
        return 1;
    }

    setSigInt();

    while (!gShutOff) {
        con.wdata->num++;
        if (!con.write()) {
            std::cout << "Write Fail" << std::endl;
        } else {
            std::cout << "Wrote NUM = " << con.wdata->num << std::endl;
        }
        sleepSSM(1);
    }
    
    
    con.terminate();

    return 0;
}
