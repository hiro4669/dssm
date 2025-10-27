#ifndef __NEIGHBOR_HPP__
#define __NEIGHBOR_HPP__

#include <stdio.h>
#include <string>
#include <cstring>
#include <map>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <vector>

#include "dssm-def.hpp"


#define IPINFO_SIZE 21

/* For Time */
#include <time.h>

class Neighbor {
    private:
    std::string ip_addr;
    struct timespec ltime;
    int port;
    uint16_t br_data_len;
    uint8_t br_data[BR_MAX_SIZE];

    public:
    Neighbor(){
        this->ip_addr = "127.0.0.1";
        this->port = 0;
        this->br_data_len = 0;
        std::memset(this->br_data, 0, BR_MAX_SIZE);
    }
    Neighbor(std::string ip_addr, int port, int br_data_len = 0, uint8_t* br_data = nullptr) {
        this->ip_addr = ip_addr;
        this->port = port;
        this->updateTime();
        this->br_data_len = br_data_len;
        if (br_data_len > 0 && br_data != nullptr) {
            std::memcpy(this->br_data, br_data, br_data_len);
        } 

    }
    ~Neighbor(){}

    std::string getIpAddress();
    int getPort();
    void showInfo();
    void updateTime();
    std::vector<uint8_t> serialize();
};


class NeighborManager {
    private:
    std::map<std::string, Neighbor> nmap;

    public:
    NeighborManager() {
        // for test
        /*
        Neighbor nb1("10.0.0.1", 8080);
        Neighbor nb2("192.168.0.1", 12000);
        this->nmap[nb1.getIpAddress()] = nb1;
        this->nmap[nb2.getIpAddress()] = nb2;
        */
        // end for test

    }
    ~NeighborManager() {}
    void add(Neighbor nb);
    bool find(Neighbor nb);
    size_t count();
    std::vector<uint8_t> serialize();
    Neighbor getFirst();
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

inline std::vector<uint8_t> Neighbor::serialize() {
    std::vector<uint8_t> buffer;
    std::string ip_addr_str = this->getIpAddress();
    std::string port_str = std::to_string(this->getPort());

    uint16_t ip_len = (uint16_t)ip_addr_str.length();
    uint16_t port_len = (uint16_t)port_str.length();


    buffer.insert(buffer.end(), reinterpret_cast<uint8_t*>(&ip_len), 
            reinterpret_cast<uint8_t*>(&ip_len) + sizeof(uint16_t));
    buffer.insert(buffer.end(), ip_addr_str.begin(), ip_addr_str.end());


    buffer.insert(buffer.end(), reinterpret_cast<uint8_t*>(&port_len), 
            reinterpret_cast<uint8_t*>(&port_len) + sizeof(uint16_t));
    buffer.insert(buffer.end(), port_str.begin(), port_str.end());


    buffer.insert(buffer.end(), reinterpret_cast<uint8_t*>(&br_data_len), 
            reinterpret_cast<uint8_t*>(&br_data_len) + sizeof(uint16_t));

    if (br_data_len > 0) {
        buffer.insert(buffer.end(), this->br_data, this->br_data + br_data_len);
    }

    /*
    printf("total len = %ld\n", buffer.size());
    printf("--------data begin\n");
    uint8_t* data = buffer.data();
    for (int i = 0; i < buffer.size(); ++i) {
        if (i % 16 == 0) printf("\n");
        printf("%02x ", data[i]);
    }
    printf("\n -------data end\n");
    */
    return buffer;
}

/***  NeighborManager ***/
inline void NeighborManager::add(Neighbor nb) {    
    this->nmap[nb.getIpAddress()] = nb;
}


inline Neighbor NeighborManager::getFirst() {
    if (this->nmap.size() > 0) {
        return this->nmap.begin()->second;
    } else {
        return Neighbor();
    }
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



inline std::vector<uint8_t> NeighborManager::serialize() {
    printf("serialize invoked\n");
    std::vector<uint8_t> buffer;
    for (auto it : this->nmap) {
        Neighbor nb = it.second;
        std::vector<uint8_t> v = nb.serialize();
        buffer.insert(buffer.end(), v.begin(), v.end());
    }

    uint16_t count = this->count();
    buffer.insert(buffer.begin(), reinterpret_cast<uint8_t*>(&count),
            reinterpret_cast<uint8_t*>(&count) + sizeof(uint16_t));

    uint32_t total_size = buffer.size() + sizeof(uint32_t);
    buffer.insert(buffer.begin(), reinterpret_cast<uint8_t*>(&total_size),
            reinterpret_cast<uint8_t*>(&total_size) + sizeof(uint32_t));


    /*
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
    */

    return buffer;
}
#endif
