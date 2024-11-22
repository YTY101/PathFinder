#include <iostream>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <nlohmann/json.hpp>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <fstream>
#include <limits>
#include "server.h"
#include "read_data.h"
#include "findpath.h"
namespace asio = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;

// 使用nlohmann/json命名空间
using json = nlohmann::json;

unordered_map<string, Node> nodes;
unordered_map<string, Way> ways;
unordered_map<string, Chunk> chunk_map;

std::mutex node_mutex; // 用于保护共享数据
bool LOG = false;

void handle_request(beast::tcp_stream& stream, http::request<http::string_body>& req) {
    try {
        if (req.method() == http::verb::options) {
            // 处理 OPTIONS 请求（预检请求）
            http::response<http::string_body> res{http::status::ok, req.version()};
            res.set(http::field::server, "SimpleServer");
            res.set(http::field::access_control_allow_origin, "*");
            // res.set(http::field::access_control_allow_methods, "GET, POST, OPTIONS");
            // res.set(http::field::access_control_allow_headers, "Content-Type");
            res.set(http::field::access_control_max_age, "3600");
            res.set(http::field::access_control_allow_methods, "*");
            res.set(http::field::access_control_allow_headers, "*");
            // res.set(http::field::access_control_max_age, "100000");
            res.content_length(0); // 无需返回内容
            http::write(stream, res);
            return;
        }

        // 解析传入的JSON
        if(LOG) std::cout<<"GOT IT: "<<req.body()<<" END"<<std::endl;
        json request_data = json::parse(req.body());

        // 打印接收到的JSON
        // if(LOG) std::cout << "Received JSON: " << request_data.dump(4) << std::endl;
        // std::cout << "Received JSON: " << request_data.dump(4) << std::endl;

        // 创建响应JSON
        json response_data;

        if(request_data.contains("task") && request_data["task"]=="render"){
            response_data = handle_render(request_data);
        }else if(request_data.contains("task") && request_data["task"]=="path"){
            response_data = handle_path(request_data);
        }else if(request_data.contains("task") && request_data["task"]=="select_path"){
            response_data = handle_select_path(request_data);
        }else{
        if(LOG)     std::cout<<"Unknown Request"<<std::endl;
            response_data = {
                {"status", "success"},
                {"received_data", request_data}
            };
        }

        

        // 将响应数据序列化为字符串
        std::string response_body = response_data.dump();
        // std::cout<<"Response: "<<response_body<<std::endl<<"END"<<std::endl;;

        // 创建HTTP响应
        http::response<http::string_body> res{
            http::status::ok, req.version()};
        res.set(http::field::server, "SimpleServer");
        res.set(http::field::content_type, "application/json");
        res.set(http::field::access_control_allow_origin, "*");
        // res.set(http::field::access_control_allow_methods, "GET, POST, OPTIONS"); // 允许的方法
        // res.set(http::field::access_control_allow_headers, "Content-Type"); // 允许的头部
        res.set(http::field::access_control_allow_methods, "*"); // 允许的方法
        res.set(http::field::access_control_allow_headers, "*"); // 允许的头部
        // res.set(http::field::access_control_max_age, "3600"); // 预检请求的缓存时间

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


json handle_render(json request_data) {
    string chunk_id = request_data["chunkId"];
    
    json nodes_data = json::array();
    json ways_data = json::array();

    std::cout<<chunk_id<<std::endl;

    // 检查指定的 chunk 是否存在
    if (chunk_map.find(chunk_id) != chunk_map.end()) {
        const Chunk& chunk = chunk_map[chunk_id];

        // 遍历节点并填充数据
        for (const auto& node : chunk.nodes) {
            nodes_data.push_back({
                {"id", node.id},
                {"lat", node.lat},
                {"lng", node.lon}
            });
        }
        
        // 遍历路径并填充数据
        for (const auto& way : chunk.ways) {
            json nodes_in_way = json::array(); // 用来存储节点属性
            
            for (const auto& ref : way.node_refs) {
                const auto& node = nodes[ref];
                nodes_in_way.push_back({
                    {"id", node.id},
                    {"lat", node.lat},
                    {"lng", node.lon}
                });
            }
            
            json tags_of_way;
            for (const auto& tag : way.tags) {
                tags_of_way[tag.first] = tag.second;
            }

            ways_data.push_back({
                {"id", way.id},
                {"nodes", nodes_in_way},
                {"tags", tags_of_way}
            });
        }

        return {
            {"status", "success"},
            {"nodes_data", nodes_data},
            {"ways_data", ways_data}
        };
    } else {
        return {
            {"status", "error"},
            {"message", "Chunk not found."},
            {"nodes_data", json::array()},
            {"ways_data", json::array()}
        };
    }
}

json handle_path(json request_data){
    string start_node_id = request_data["startNodeId"];
    string end_node_id = request_data["endNodeId"];
    return aStarSearch(start_node_id, end_node_id); 
}

string find_closest_node(double lat, double lon, const std::string& chunk_id) {
    // 默认最小距离和最近节点
    double min_distance = std::numeric_limits<double>::max();
    string closest_node_id = "";

    // 确保该 chunk 存在并且包含节点
    if (chunk_map.find(chunk_id) != chunk_map.end()) {
        const Chunk& chunk = chunk_map[chunk_id];

        // 遍历 chunk 中的所有节点
        for (const auto& node : chunk.nodes) {
            // 计算当前节点到给定经纬度的距离
            double distance = std::sqrt(std::pow(node.lat - lat, 2) + std::pow(node.lon - lon, 2));

            // 更新最小距离和最近节点
            if (distance < min_distance) {
                min_distance = distance;
                closest_node_id = node.id;
            }
        }
    }

    // 返回找到的最近节点
    return closest_node_id;
}

json handle_select_path(json request_data) {
    double start_lat = request_data["startLat"];
    double start_lon = request_data["startLon"];
    double end_lat = request_data["endLat"];
    double end_lon = request_data["endLon"];

    // 获取起点和终点所在的 Chunk ID
    string start_chunk_id = getChunkKey(start_lat, start_lon);
    string end_chunk_id = getChunkKey(end_lat, end_lon);

    unordered_map<string, bool> visited;
    visited.clear();
    string target_start_chunk_id = getTargetChunk(start_chunk_id, 0, visited).first;
    visited.clear();
    string target_end_chunk_id = getTargetChunk(end_chunk_id, 0, visited).first; 

    string closest_start_node_id = find_closest_node(start_lat, start_lon, target_start_chunk_id);
    string closest_end_node_id = find_closest_node(end_lat, end_lon, target_end_chunk_id);


    return aStarSearch(closest_start_node_id, closest_end_node_id); 
}


void start_server(asio::io_context& ioc, asio::ip::tcp::endpoint endpoint) {
    std::cout << "Loading data..." << std::endl;
    parseOSM("data/big_map/map.osm", nodes, ways, chunk_map);
    std::cout << "Data loaded." << std::endl;

    // 打印有效的 Chunk ID 并写入到文件
    ofstream chunksFile("chunks.txt"); // 打开一个名为 chunks.txt 的文件
    if (chunksFile.is_open()) {
        for (const auto& pair : chunk_map) {
            chunksFile << pair.first << endl; // 将 Chunk ID 写入文件
        }
        chunksFile.close(); // 关闭文件
        cout << "有效的 Chunk ID 已写入到 chunks.txt 文件中。" << endl;
    } else {
        cout << "无法打开文件 chunks.txt 进行写入。" << endl;
    }

    beast::tcp_stream stream(ioc);

    // 监听端口
    asio::ip::tcp::acceptor acceptor(ioc, endpoint);
    std::cout << "Server is running on " << endpoint << std::endl;

    for (;;) {
        // 等待客户端连接
        acceptor.accept(stream.socket());
        if (LOG) std::cout << "Waiting..." << std::endl;

        // 读取HTTP请求
        beast::flat_buffer buffer;
        http::request<http::string_body> req;
        http::read(stream, buffer, req);
         // 创建HTTP响应对象
        http::response<http::string_body> res{http::status::ok, req.version()};
        // 将处理请求的逻辑放在新的线程中
        std::thread([s = std::move(stream), req]() mutable {
            handle_request(s, req);
        }).detach(); // 使用 detach 让其独立于主线程执行
    }
}