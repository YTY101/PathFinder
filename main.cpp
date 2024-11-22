#include<iostream>
#include<unordered_map>
#include "read_data.h"
#include "server.h"
#include "findpath.h"

int main(){

    try {
        asio::io_context ioc;
        asio::ip::tcp::endpoint endpoint(asio::ip::tcp::v4(), 8080); // 监听8080端口
        start_server(ioc, endpoint);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }


    return 0;
}



