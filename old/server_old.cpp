#include <iostream>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <nlohmann/json.hpp>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include "server_old.h"
#include "read_data_old.h"
namespace asio = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;

// 使用nlohmann/json命名空间
using json = nlohmann::json;

unordered_map<string, Node> nodes;
unordered_map<string, Way> ways;
QuadTree quad_tree_ways(31.1029000, 31.3290000, 121.2897000, 121.6969000);

std::mutex node_mutex; // 用于保护共享数据
bool LOG = false;

void handle_request(beast::tcp_stream& stream, http::request<http::string_body>& req) {
    try {
        if (req.method() == http::verb::options) {
            // 处理 OPTIONS 请求（预检请求）
            http::response<http::string_body> res{http::status::ok, req.version()};
            res.set(http::field::server, "SimpleServer");
            res.set(http::field::access_control_allow_origin, "*");
            res.set(http::field::access_control_allow_methods, "GET, POST, OPTIONS");
            res.set(http::field::access_control_allow_headers, "Content-Type");
            res.set(http::field::access_control_max_age, "3600");
            res.content_length(0); // 无需返回内容
            http::write(stream, res);
            return;
        }



        // 解析传入的JSON
        if(LOG) std::cout<<"GOT IT: "<<req.body()<<" END"<<std::endl;
        json request_data = json::parse(req.body());

        // 打印接收到的JSON
        if(LOG) std::cout << "Received JSON: " << request_data.dump(4) << std::endl;
        
        // 创建响应JSON
        json response_data;

        if(request_data.contains("task") && request_data["task"]=="render"){
            response_data = handle_render(request_data);
        }else{
        if(LOG)     std::cout<<"Unknown Request"<<std::endl;
            response_data = {
                {"status", "success"},
                {"received_data", request_data}
            };
        }

        

        // 将响应数据序列化为字符串
        std::string response_body = response_data.dump();

        // 创建HTTP响应
        http::response<http::string_body> res{
            http::status::ok, req.version()};
        res.set(http::field::server, "SimpleServer");
        res.set(http::field::content_type, "application/json");
        res.set(http::field::access_control_allow_origin, "*");
        res.set(http::field::access_control_allow_methods, "GET, POST, OPTIONS"); // 允许的方法
        res.set(http::field::access_control_allow_headers, "Content-Type"); // 允许的头部
        res.set(http::field::access_control_max_age, "3600"); // 预检请求的缓存时间

        res.content_length(response_body.size());
        res.body() = std::move(response_body);

        // std::cout<<"Response: "<<response_body<<std::endl;

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

void collect_ways(QuadTreeNode* node, json& ways_data, const unordered_map<string, Node>& nodes) {
    std::unordered_set<std::string> existing_ids; // 用于存储已存在的 way id

    // 先记录 ways_data 中的所有 id 到哈希表
    for (const auto& existing_way : ways_data) {
        existing_ids.insert(existing_way["id"].get<std::string>()); // 确保提取为 std::string
    }

    // 遍历当前节点的 ways
    for (const auto& way : node->ways) {
        // 检查 ways_data 中是否已经有相同 id 的 way
        if (existing_ids.find(way.id) != existing_ids.end()) {
            continue; // 如果已存在，跳过插入
        }

        json nodes_in_way = json::array(); // 用来存储节点属性

        // 收集该 way 所有节点的信息
        for (const auto& ref : way.node_refs) {
            const Node& node = nodes.at(ref); // 使用 at() 进行安全访问
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

        // 将新加入的 id 也加入到哈希表中
        existing_ids.insert(way.id); // 确保 way.id 是 std::string
    }

    // 递归遍历所有子节点
    for (const auto& child : node->children) {
        if (child) { // 确保子节点存在
            collect_ways(child.get(), ways_data, nodes); // 使用 get() 获取普通指针
        }
    }
}

json handle_render(json request_data){
    double min_lat = request_data["southWest"]["lat"];
    double min_lon = request_data["southWest"]["lng"];
    double max_lat = request_data["northEast"]["lat"];
    double max_lon = request_data["northEast"]["lng"];
    
    json nodes_data = json::array();
    json ways_data = json::array();

    // 遍历返回node数据（没必要）
    // for (const auto& [id, node] : nodes) {
    //     if (node.lat >= min_lat && node.lat <= max_lat &&
    //         node.lon >= min_lon && node.lon <= max_lon) {
    //         nodes_data.push_back({
    //             {"id", node.id},
    //             {"lat", node.lat},
    //             {"lng", node.lon}
    //         });
    //     }
    // }

    // // 遍历路径，选择在经纬度范围内的路径
    // for (const auto& [id, way] : ways) {
    //     bool way_in_range = false; // 标志变量，判断路径是否在范围内

    //     for (const auto& node_id : way.node_refs) {
    //         const Node& node = nodes[node_id];
    //         if (node.lat >= min_lat && node.lat <= max_lat &&
    //             node.lon >= min_lon && node.lon <= max_lon) {
    //             way_in_range = true; // 路径中有节点在范围内
    //             break;
    //         }
    //     }

    //     if (way_in_range) {
    //         json nodes_in_way = json::array(); // 使用字典来存储 Node 的属性
    //         for (const auto& ref : way.node_refs) {
    //             const auto& node = nodes[ref];
    //             json node_json;
    //             node_json["id"] = node.id;
    //             node_json["lat"] = node.lat;
    //             node_json["lng"] = node.lon;
    //             nodes_in_way.push_back(node_json);
    //         }
    //         json tags_of_way;
    //         for(const auto& tag : way.tags){
    //             tags_of_way[tag.first] = tag.second;
    //         }

    //         ways_data.push_back({
    //             {"id", way.id},
    //             {"nodes", nodes_in_way}, // 使用字典而不是 Node 对象
    //             {"tags", tags_of_way}
    //         });
    //         // std::cout << "Way inside range: " << way.id << std::endl; // 调试信息
    //     }

    // }

    

    QuadTreeNode* containing_node = quad_tree_ways.query_min_containing(min_lat, max_lat, min_lon, max_lon);

    if (containing_node) {
        // // 根据找到的最小子节点中的 Ways 进行数据填充
        // for (const auto& way : containing_node->ways) {
        //     json nodes_in_way = json::array(); // 用来存储节点属性

        //     for (const auto& ref : way.node_refs) {
        //         const Node& node = nodes[ref];
        //         nodes_in_way.push_back({
        //             {"id", node.id},
        //             {"lat", node.lat},
        //             {"lng", node.lon}
        //         });
        //     }
            
        //     json tags_of_way;
        //     for (const auto& tag : way.tags) {
        //         tags_of_way[tag.first] = tag.second;
        //     }

        //     ways_data.push_back({
        //         {"id", way.id},
        //         {"nodes", nodes_in_way},
        //         {"tags", tags_of_way}
        //     });
        // }
        collect_ways(containing_node, ways_data, nodes);
    }else{
        return {
            {"status", "exceeded"},
            {"nodes_data", ""},
            {"ways_data", ""}
        };
    }
    
    return {
        {"status", "success"},
        {"nodes_data", nodes_data},
        {"ways_data", ways_data}
    };
}


// // 服务器入口
// void start_server(asio::io_context& ioc, asio::ip::tcp::endpoint endpoint) {
//     //加载数据
//     // parseOSM("data/map/map.osm", nodes, ways);
//     // parseOSM("data/mid_map/map.osm", nodes, ways);
//     std::cout << "Loading data..." << std::endl;
//     parseOSM("data/big_map/map.osm", nodes, ways, quad_tree_ways);
//     std::cout << "Data loaded." << std::endl;

//     try {
//         beast::tcp_stream stream(ioc);

//         // 监听端口
//         asio::ip::tcp::acceptor acceptor(ioc, endpoint);
//         std::cout << "Server is running on " << endpoint << std::endl;

//         for (;;) {
//             // 等待客户端连接
//             acceptor.accept(stream.socket());
//             if(LOG) std::cout<<"Waiting..."<<std::endl;
//             // 读取HTTP请求
//             beast::flat_buffer buffer;
//             http::request<http::string_body> req;
//             http::read(stream, buffer, req);

//             // 处理请求
//             handle_request(stream, req);
//             if(LOG) std::cout<<"Handled 1 Req"<<std::endl;
//             // 关闭连接
//             stream.socket().shutdown(asio::ip::tcp::socket::shutdown_send);
//             stream.socket().close();
//             if(LOG) std::cout<<"Shutted"<<std::endl;
//         }

//     } catch (const std::exception& e) {
//         std::cerr << "Server Error: " << e.what() << std::endl;
//     }
// }

void start_server(asio::io_context& ioc, asio::ip::tcp::endpoint endpoint) {
    std::cout << "Loading data..." << std::endl;
    parseOSM("data/big_map/map.osm", nodes, ways, quad_tree_ways);
    std::cout << "Data loaded." << std::endl;

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

        // 将处理请求的逻辑放在新的线程中
        std::thread([s = std::move(stream), req]() mutable {
            handle_request(s, req);
        }).detach(); // 使用 detach 让其独立于主线程执行
    }
}