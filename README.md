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

If you want to use buffered features (recommand if you use WiFi)
1. Run `samples/Cpp_RemoteSample/read_buffer_test`
