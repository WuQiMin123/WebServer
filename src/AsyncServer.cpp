#include <iostream>
#include "boost/asio.hpp"
#include "Session.cpp"


int main(){
    try {
        boost::asio::io_context ioc;
        Server server(ioc, 8080);
        ioc.run();
    }
    catch (std::exception& e) {
        std::cerr << "Exception:" << e.what() << std::endl;
    }
    return 0;
}