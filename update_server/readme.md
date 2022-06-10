### 环境安装(ubuntu)

+ 服务端

  + 参考https://github.com/stallion5632/apisix-nginx-module 编译openresty，openresty使用官方的1.19.9.1版本(https://openresty.org/download/openresty-1.19.9.1.tar.gz)；
  + 需安装nginx-upload-module[https://github.com/hongzhidao/nginx-upload-module] 模块实现文件断点续传，因而，configure脚本为：`./configure --add-module=../apisix-nginx-module/src/meta  --add-module=../nginx-upload-module-master`
+ 客户端
  + 依赖boost库1.66版本以上，cpr（cpr依赖curl）；
  + 使用gcc5.4以上版本；
+ 程序运行方法

### 运行
+ 服务端

  + 运行服务方法 ./start.sh

  + http服务日志 logs/http.log，tcp服务日志logs/stream.log
  + 查询数据库数据
    + 查询某一个实例: localhost:9988/sys/v1/service_api?filename=libunqlite.so&type=client1
    + 全部查询实例，去除参数： localhost:9988/sys/v1/service_api
+ 客户端

  直接运行./edge_client

### http服务和tcp服务任务下发
+ 使用内置的共享内存，http下发消息至tcp服务，然后推送至tcp客户端

### tcp通讯协议：

| 消息       | 长度(BYTES)                               | 结构              | 描述                                         |
| ---------- | ----------------------------------------- | ----------------- | -------------------------------------------- |
| **Header** | 4                                         | SyncBytes         | 识别码，表示不同设备之间的交互               |
| **Header** | 4                                         | FullMessageLength | 数据总长度，包括SyncBytes和FullMessageLength |
| **Header** | 4                                         | MeaageType        | 消息类型（消息号)，如登录消息或发送消息消息  |
| **Header** | 4                                         | StringBody        | 消息字符流长度                               |
| **Body**   | StringBodyLength                          | StringBody        | 消息体长度，可能为0                          |
| **Body**   | FullMessageLength - 16 - StringBodyLength | BinaryData        | 二进制数据，长度可能为0                      |

注: 消息头总共**16**个字节（包括SyncBytes和FullMessageLength），消息体长度StringBodyLength是已知的，而二进制数据长度计算得到（FullMessageLength - 16 - StringBodyLength）。

