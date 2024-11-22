#include <iostream>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <nlohmann/json.hpp>
#include "findpath.cpp"
namespace asio = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;

// 使用nlohmann/json命名空间
using json = nlohmann::json;

// 简单的处理HTTP请求并响应JSON的函数
void handle_request(beast::tcp_stream& stream, http::request<http::string_body>& req) {
    try {
        // 解析传入的JSON
        json request_data = json::parse(req.body());

        // 打印接收到的JSON
        std::cout << "Received JSON: " << request_data.dump(4) << std::endl;

        // 创建响应JSON
        json response_data = {
            {"status", "success"},
            {"received_data", request_data}
        };

        // 将响应数据序列化为字符串
        std::string response_body = response_data.dump();

        // 创建HTTP响应
        http::response<http::string_body> res{
            http::status::ok, req.version()};
        res.set(http::field::server, "SimpleServer");
        res.set(http::field::content_type, "application/json");
        res.content_length(response_body.size());
        res.body() = std::move(response_body);

        // 发送响应
        http::write(stream, res);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        // 如果发生异常，返回错误响应
        http::response<http::string_body> res{
            http::status::bad_request, 11};
        res.set(http::field::server, "SimpleServer");
        res.set(http::field::content_type, "application/json");
        res.body() = "{\"status\":\"error\",\"message\":\"Invalid JSON\"}";
        http::write(stream, res);
    }
}

// 服务器入口
void start_server(asio::io_context& ioc, asio::ip::tcp::endpoint endpoint) {
    try {
        beast::tcp_stream stream(ioc);

        // 监听端口
        asio::ip::tcp::acceptor acceptor(ioc, endpoint);
        std::cout << "Server is running on " << endpoint << std::endl;

        for (;;) {
            // 等待客户端连接
            acceptor.accept(stream.socket());

            // 读取HTTP请求
            beast::flat_buffer buffer;
            http::request<http::string_body> req;
            http::read(stream, buffer, req);

            // 处理请求
            handle_request(stream, req);

            // 关闭连接
            stream.socket().shutdown(asio::ip::tcp::socket::shutdown_send);
        }

    } catch (const std::exception& e) {
        std::cerr << "Server Error: " << e.what() << std::endl;
    }
}

int main() {
    try {
        asio::io_context ioc;
        asio::ip::tcp::endpoint endpoint(asio::ip::tcp::v4(), 8080); // 监听8080端口
        start_server(ioc, endpoint);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    return 0;
}
