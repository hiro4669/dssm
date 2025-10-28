### Distributed Streaming data Sharing Manager, netowrk extension of SSM



#### Directory structrure
- Makefile   make file
- README     This file
- src        sources
- include    include files
- utilities  lssm,ssmlogger,ssmmultiplayer

#### Install
- ./configure
- make
- sudo make install


#### Executable files
- ssm-coordinator     SSM
- dssm-proxy
- lsssm               print ssm list
- ssm-logger          logging ssm data
- ssm-advance-player  play ssm log data
- ssm-monitor         monitoring ssm
- ssm-graph           print ssm dot file
- ssm-date            print ssm time


#### Samples
You can run two types of example: executing local, using network

#### Local Example
1. Run `ssm-coordinator`
1. Run `samples/Cpp_Sample/ssmWriteSample`
1. Run `samples/Cpp_Sample/ssmReadSample`

#### Using network Example
1. Run `ssm-coordinator`
1. Run `dssm-proxy`
1. Run `samples/Cpp_RemoteSample/write_proxy_test`
1. Run `samples/Cpp_RemoteSample/read_proxy_test`

If you want to use buffered features (recommended if you use WiFi)
1. Run `samples/Cpp_RemoteSample/read_buffer_test`


#### Broadcast Example
If you want to use broadcast function, you can exchange any typed data using template. Also, by default, broadcast function exchanges IP Address and Port Number. If you specify the type as template, you can exchange the data in addition to IP Address and Port Number.

Example:
For sending
```
typedef struct {
    int ival;
    double dval;
    char cval[32];
} param;

int main(int aArgc, char **aArgv) {
    // third type is the data exchanged using broadcast
	DSSMApi<intSsm_k, props_p, param> con(SNAME_INT, 1);
    .....
    // invoke this method if you exchange your own data
    con.sendBroadcast()
```
For receiving
Broadcast function exchange all information and store it on DSSMProxy.
To get the information, you can use `con.reeiveBroadcast()` method.
This method returns a vector of DSSMApi::BrInfo as `std::vector<DSSMApi<...>::BrInfo`

You can see how to use it via `samples/CppRemoteSample/read_broad_test.cpp


