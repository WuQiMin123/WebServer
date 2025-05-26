这是一个仿照asio官网编写的异步服务器demo。

src/
├── AsyncServer.cpp     # 主函数
├── Session.h           # Session 和 Server 类声明
└── Session.cpp         # Session 和 Server 类实现

Session 类
作用：处理单个客户端连接的数据读写

Server 类
作用：监听端口，接受新连接并创建会话

Server start_accept()
        ↓
    新连接到达 → 创建 Session
        ↓
    handle_accept() → Session::Start()
        ↓
    async_read_some → handle_read → async_write → handle_write → async_read_some ...

其中存在隐患，就是当服务器即将发送数据前(调用async_write前)，此刻客户端中断，服务器此时调用async_write会触发发送回调函数，判断ec为非0进而执行delete this逻辑回收session。

但要注意的是客户端关闭后，在tcp层面会触发读就绪事件，服务器会触发读事件回调函数。在读事件回调函数中判断错误码ec为非0，进而再次执行delete操作，从而造成二次析构。