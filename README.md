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

客户端在async_write过程中断开，触发handle_write中的错误处理，执行delete this。
与此同时或之后，底层的TCP连接关闭会导致所有未完成的异步操作（例如之前发起的async_read_some）被取消，触发handle_read中的错误处理，再次执行delete this。

void Session::Start(){
	memset(_data, 0, max_length);
	_socket.async_read_some(boost::asio::buffer(_data, max_length),     
		std::bind(&Session::handle_read, this, placeholders::_1,
			placeholders::_2)
	);      
}


void Session::handle_read(const boost::system::error_code& error, size_t bytes_transfered) { 

    if (!error) {
        cout << "server receive data is " << _data << endl;    
        boost::asio::async_write(_socket, boost::asio::buffer(_data, bytes_transfered),     
            std::bind(&Session::handle_write, this, placeholders::_1));         
    }
    else {
        delete this;         // <--- 第二处析构
    }
}

void Session::handle_write(const boost::system::error_code& error) {
    if (!error) {
        memset(_data, 0, max_length);
        _socket.async_read_some(boost::asio::buffer(_data, max_length), std::bind(&Session::handle_read,
            this, placeholders::_1, placeholders::_2));
    }
    else {   
        delete this;        // <--- 第一处析构
    }
}


==========================================================================================
优化1：通过C11智能指针构造成一个伪闭包的状态延长session的生命周期。防止出现触发session读回调函数时此时session的内存已经被回收了的情况
WebServer/
├── include/
│   ├── CSession.h     # CSession 类声明
│   └── CServer.h      # CServer 类声明
├── src/
│   ├── AsyncServer.cpp # 主函数入口
│   ├── CSession.cpp   # CSession 类实现
│   └── CServer.cpp    # CServer 类实现

CSession 类
功能：管理单个客户端连接的读写，并支持 UUID 唯一会话标识

使用 shared_from_this() 管理自身生命周期，确保异步操作期间对象不被释放。
支持异步读写：
async_read_some → HandleRead
async_write → HandleWrite
使用 _send_que 队列和锁（_send_lock）保障发送线程安全。
每个会话有唯一 UUID。
核心方法：
Start()：开始监听客户端数据
Send()：将消息加入发送队列并异步发送
HandleRead()：处理收到的数据并回传
HandleWrite()：发送完成后继续发下一条或清理连接

CServer 类
功能：监听端口、接受新连接并管理所有会话

使用 map<string, shared_ptr<CSession>> _sessions 存储活跃会话
提供 ClearSession(uuid) 方法清除指定会话
每次 accept 创建一个新 CSession 实例
核心方法：
StartAccept()：启动异步等待新连接
HandleAccept()：接受连接后启动会话并加入 session map

整体流程：
启动 CServer → 监听端口 → 客户端连接 → 创建 CSession
                                        ↓
                                加入 _sessions 管理
                                        ↓
                            CSession Start() → 异步读取 → HandleRead → Send → HandleWrite → 循环

优化点：
错误处理时不再直接 delete this，而是通过 _server->ClearSession(_uuid) 统一管理销毁逻辑。只有当Session从map中移除后，Session才会被释放。
（但是这么做还是会有崩溃问题，因为第一次在session写回调函数中移除session，session的引用计数就为0了，调用了session的析构函数，这样在触发session读回调函数时此时session的内存已经被回收了自然会出现崩溃的问题。
解决这个问题可以利用智能指针引用计数和bind的特性，实现一个伪闭包的机制延长session的生命周期。）

使用 shared_ptr 管理 CSession 对象的生命周期。

所有异步操作的回调函数中绑定 shared_from_this()（即 _self_shared），确保对象在异步操作完成前不会被销毁。
