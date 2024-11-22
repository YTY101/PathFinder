#ifndef SERVER_OLD
#define SERVER_OLD
#include <iostream>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <nlohmann/json.hpp>

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;

using json = nlohmann::json;

void handle_request(beast::tcp_stream& stream, http::request<http::string_body>& req);
void start_server(asio::io_context& ioc, asio::ip::tcp::endpoint endpoint);
json handle_render(json request_data);


#endif