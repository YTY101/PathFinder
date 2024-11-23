#ifndef SERVER
#define SERVER
#include <iostream>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <nlohmann/json.hpp>

#include "read_data.h"

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;


extern std::unordered_map<std::string, Node> nodes;
extern std::unordered_map<std::string, Way> ways;
extern std::unordered_map<std::string, Chunk> chunk_map;

extern std::unordered_map<std::string, std::vector<Neighbor>> neighbors;

using json = nlohmann::json;

json handle_render(json request_data);
json handle_path(json request_data);
json handle_select_path(json request_data);

void handle_request(beast::tcp_stream& stream, http::request<http::string_body>& req);

void start_server(asio::io_context& ioc, asio::ip::tcp::endpoint endpoint);



#endif