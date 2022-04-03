# grpcconnect

实现a->b->c传输，因为有多个a或者多个c, 此模型像消息队列和观察者模式

## 流程

1. c启动时向b注册
2. a启动时向b发送消息
3. b收到a发送的流后，立即广播给所有c

[![](https://mermaid.ink/img/pako:eNqF0LEOwjAMBNBfiTyBRAdgy4AEbZkYEIykg9W4NFLTVK4zIODfCYKOCE83vBvOd6iDJdBwZRxadTiZXqXbXo4cbKyJt5XKss2jDn5gGkeyahQm9A-1mx07lCawn39Ku1-yuOSdo17yZfVHlpNc_ZP7Sa4rWIAn9uhs2nF_9wxIS54M6BQtNRg7MWD6Z6JxsChUWieBQTfYjbQAjBLOt74GLRxpQoXD9Bb_Vc8XBDtl0A)](https://mermaid-js.github.io/mermaid-live-editor/edit#pako:eNqF0LEOwjAMBNBfiTyBRAdgy4AEbZkYEIykg9W4NFLTVK4zIODfCYKOCE83vBvOd6iDJdBwZRxadTiZXqXbXo4cbKyJt5XKss2jDn5gGkeyahQm9A-1mx07lCawn39Ku1-yuOSdo17yZfVHlpNc_ZP7Sa4rWIAn9uhs2nF_9wxIS54M6BQtNRg7MWD6Z6JxsChUWieBQTfYjbQAjBLOt74GLRxpQoXD9Bb_Vc8XBDtl0A)

## 存在问题

* 网络带宽有限，故所有客户端和服务端都启用grpc自带的压缩功能`GRPC_COMPRESS_STREAM_GZIP`，对数据流压缩
* 由于是tcp模拟广播，所以存在多`c`时，靠后的客户端的延迟增加，如果使用tcp/ip的组播方式则不存在此问题
* 当从`b`->`c`段网络出现瓶颈，导致`a`发送的消息积压的情况，可以使用队列来缓存，但不是根本办法，提升软硬件才是最佳办法。
