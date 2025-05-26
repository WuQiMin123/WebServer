#pragma once
#include <memory>
#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>  // 添加此行以支持 tcp::socket
#include <iostream>

using namespace std;

// 可选：添加命名空间别名简化书写
namespace asio = boost::asio;
using tcp = boost::asio::ip::tcp;

class Session {
public:
    Session(boost::asio::io_context& ioc) : _socket(ioc) {
    }

    tcp::socket& Socket() {  // 使用完整命名空间或别名
        return _socket;
    }

    void Start();

private:
    void handle_read(const boost::system::error_code& error, size_t bytes_transferred);
    void handle_write(const boost::system::error_code& error);

    tcp::socket _socket;            // 接受客户端连接的 socket
    enum { max_length = 1024 };
    char _data[max_length];         // 存储接收数据的缓冲区
};

class Server {
public:
    Server(boost::asio::io_context& ioc, short port);

private:
    void start_accept();
    void handle_accept(Session* new_session, const boost::system::error_code& error);

    boost::asio::io_context& _ioc;
    tcp::acceptor _acceptor;        // 使用 tcp::acceptor
};