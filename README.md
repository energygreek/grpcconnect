# grpcconnect

实现a->b->c传输，因为有多个a或者多个c, 此模型像消息队列和观察者模式

## 流程

1. c启动时向b注册
2. a启动时向b发送消息
3. b收到a发送的流后，立即广播给所有c

[![](https://mermaid.ink/img/pako:eNqF0LEOwjAMBNBfiTyBRAdgy4AEbZkYEIykg9W4NFLTVK4zIODfCYKOCE83vBvOd6iDJdBwZRxadTiZXqXbXo4cbKyJt5XKss2jDn5gGkeyahQm9A-1mx07lCawn39Ku1-yuOSdo17yZfVHlpNc_ZP7Sa4rWIAn9uhs2nF_9wxIS54M6BQtNRg7MWD6Z6JxsChUWieBQTfYjbQAjBLOt74GLRxpQoXD9Bb_Vc8XBDtl0A)](https://mermaid-js.github.io/mermaid-live-editor/edit#pako:eNqF0LEOwjAMBNBfiTyBRAdgy4AEbZkYEIykg9W4NFLTVK4zIODfCYKOCE83vBvOd6iDJdBwZRxadTiZXqXbXo4cbKyJt5XKss2jDn5gGkeyahQm9A-1mx07lCawn39Ku1-yuOSdo17yZfVHlpNc_ZP7Sa4rWIAn9uhs2nF_9wxIS54M6BQtNRg7MWD6Z6JxsChUWieBQTfYjbQAjBLOt74GLRxpQoXD9Bb_Vc8XBDtl0A)


## 实现

异步模式将消息分别存入每个Consumer的队列，各自异步发送。 同步模式则收到后立即同步转发给所有Consumer

### 异步转发模式

b(AsyncPlatform)实现从a(Producer)接收消息功能、接受c(Consumer)订阅功能

1. c发送订阅请求到b
2. a发送期货消息到b
3. b在收到a的消息后转存到c的发送队列
4. c异步发送队列中的消息

### 同步转发模式

同步实现需要Plateform提前知道转发对象c，实现从a(Producer)接收消息功能、向c发送消息功能

1. a将消息发送给b, b调用客户端发送功能，将消息发送给所有c

## 存在问题

* 网络带宽有限，故所有客户端和服务端都启用grpc自带的压缩功能`GRPC_COMPRESS_STREAM_GZIP`，对数据流压缩
* 由于是tcp模拟广播，所以存在多`c`时，靠后的客户端的延迟增加，如果使用tcp/ip的组播方式则不存在此问题
* 当从`b`->`c`段网络出现瓶颈，导致`a`发送的消息积压的情况，可以使用队列来缓存，但不是根本办法，提升软硬件才是最佳办法
* 所有的发送期货数据都采用grpc自带压缩，这样会在plateform端进行一次解压和压缩，浪费cpu资源，可以优化为用外置压缩方式，对于grpc透明

## 使用

// 异步转发模式

1. 启动Platform
```
./build/bin/Platform
```
2. 启动多个Consumer
```
./build/bin/Platform
```
```
./build/bin/Platform
```

3. 启动Producer发送Future
```
grpcconnect > ./build/bin/Producer
producer received: we finish
```

所有Consumer收到
```
reciever new future future-100
```

// 同步转发模式
1. 启动2个Consumer
```
./build/bin/SyncConsumer 50052
./build/bin/SyncConsumer 50053
```
2. 启动Platform， 传入2个Consumer的地址
```
./build/bin/SyncPlatform 50052 50053 
```
3. 启动Producer发送Future
```
grpcconnect > ./build/bin/SyncProducer
producer received: we finish
```

所有Consumer收到
```
reciever new future future-100
```