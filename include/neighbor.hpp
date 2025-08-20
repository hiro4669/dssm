#ifndef __NEIGHBOR_HPP__
#define __NEIGHBOR_HPP__

#include <stdio.h>
#include <string>
#include <map>
#include <arpa/inet.h>
#include <netinet/in.h>


#define IPINFO_SIZE 21

/* For Time */
#include <time.h>

class Neighbor {
    private:
    std::string ip_addr;
    struct timespec ltime;
    int port;

    public:
    Neighbor(){}
    Neighbor(std::string ip_addr, int port) {
        this->ip_addr = ip_addr;
        this->port = port;
        this->updateTime();
    }
    ~Neighbor(){}

    std::string getIpAddress();
    int getPort();
    void showInfo();
    void updateTime();
};


class NeighborManager {
    private:
    std::map<std::string, Neighbor> nmap;

    public:
    NeighborManager() {
        // for test
        Neighbor nb1("10.0.0.1", 8080);
        Neighbor nb2("192.168.0.1", 12000);
        this->nmap[nb1.getIpAddress()] = nb1;
        this->nmap[nb2.getIpAddress()] = nb2;
        // end for test

    }
    ~NeighborManager() {}
    void add(Neighbor nb);
    bool find(Neighbor nb);
    size_t count();
    bool serialize(char* buffer);
};


/***   For Neighbor class    ***/
inline void Neighbor::updateTime() {
    clock_gettime(CLOCK_REALTIME, &this->ltime);
}

inline void Neighbor::showInfo() {
    printf("ipaddr:port = %s : %d (%ld.%ld)\n", 
        this->ip_addr.c_str(), this->port, this->ltime.tv_sec, this->ltime.tv_nsec);
}

inline std::string Neighbor::getIpAddress() {
    return this->ip_addr;
}

inline int Neighbor::getPort() {
    return this->port;
}

/***  NeighborManager ***/
inline void NeighborManager::add(Neighbor nb) {    
    this->nmap[nb.getIpAddress()] = nb;
}


inline size_t NeighborManager::count() {
    //return this->nblist.size();
    return this->nmap.size();
}

inline bool NeighborManager::find(Neighbor nb) {
    decltype(this->nmap)::iterator it = this->nmap.find(nb.getIpAddress());
    if (it != this->nmap.end()) {
        it->second.updateTime();
        it->second.showInfo();        
        return true;
    } 
    return false;
}

inline bool NeighborManager::serialize(char* buffer) {
    //std::cout << "serialize" << std::endl;
    int idx = 2;
    for (auto it : this->nmap) {
        Neighbor nb = it.second;
        std::string ip_addr_str = nb.getIpAddress();
        std::string port_str = std::to_string(nb.getPort());        
        //std::cout << port_str << std::endl;
        memcpy(&buffer[idx], ip_addr_str.c_str(), ip_addr_str.length());
        idx += 16;
        memcpy(&buffer[idx], port_str.c_str(), port_str.length());
        idx += 5;
    }

    int size = this->nmap.size();
    buffer[0] = (size >> 8) & 0xff;
    buffer[1] = size & 0xff;

    /*
    for (int i = 0; i < size * IPINFO_SIZE + 2; ++i) {
        if (i % 16 == 0) printf("\n");
        printf("%02x ", buffer[i]);
    }
    printf("\n");
    */
    

    return true;
}
#endif