#include "../include/Session.h"

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
        delete this;
    }
}

void Session::handle_write(const boost::system::error_code& error) {
    if (!error) {
        memset(_data, 0, max_length);
        _socket.async_read_some(boost::asio::buffer(_data, max_length), std::bind(&Session::handle_read,
            this, placeholders::_1, placeholders::_2));
    }
    else {
        delete this;
    }
}

Server::Server(boost::asio::io_context& ioc, short port) :_ioc(ioc),
_acceptor(ioc, tcp::endpoint(tcp::v4(), port)) {
    start_accept();
}

void Server::start_accept() {
    Session* new_session = new Session(_ioc);
    _acceptor.async_accept(new_session->Socket(),
        std::bind(&Server::handle_accept, this, new_session, placeholders::_1));
}
void Server::handle_accept(Session* new_session, const boost::system::error_code& error) {
    if (!error) {
        new_session->Start();
    }
    else {
        delete new_session;
    }

    start_accept();
}


