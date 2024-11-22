#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <tinyxml2.h>
#include <memory>
#include "read_data_old.h"
#define MAX_WAYS_CAPACITY 3 // 最大容量
using namespace tinyxml2;
using namespace std;


bool QuadTreeNode::insert(const Way& way, unordered_map<string, Node>& nodes) {
    // 检查是否有节点在当前节点范围内
    bool has_node_in_range = false;
    
    for (const auto& ref : way.node_refs) {
        // 假设通过引用查找节点的经纬度
        const Node& node = nodes[ref]; // 获取 Node 信息
        // 检查节点是否在范围内
        if (node.lat >= min_lat && node.lat <= max_lat &&
            node.lon >= min_lon && node.lon <= max_lon) {
            has_node_in_range = true; // 找到一个在范围内的节点
            break; // 找到后可以直接退出循环
        }
    }

    // 如果没有节点在当前节点范围内，则不插入
    if (!has_node_in_range) {
        return false; 
    }

    // 如果当前节点没有子节点且还未达到最大容量
    if (ways.size() < MAX_WAYS_CAPACITY) {
        ways.push_back(way);
        return true;
    }

    // 否则，进行细分
    if (!children[0]) {
        subdivide();
    }

    // 递归插入到子节点
    for (auto& child : children) {
        if (child->insert(way, nodes)) {
            return true;
        }
    }

    return false; // 插入失败
}

void QuadTreeNode::subdivide() {
    // 计算子节点的边界，将当前节点细分为四个子节点
    double mid_lat = (min_lat + max_lat) / 2;
    double mid_lon = (min_lon + max_lon) / 2;

    children[0] = std::make_unique<QuadTreeNode>(min_lat, mid_lat, min_lon, mid_lon);
    children[1] = std::make_unique<QuadTreeNode>(min_lat, mid_lat, mid_lon, max_lon);
    children[2] = std::make_unique<QuadTreeNode>(mid_lat, max_lat, min_lon, mid_lon);
    children[3] = std::make_unique<QuadTreeNode>(mid_lat, max_lat, mid_lon, max_lon);
    // children[0] = new QuadTreeNode(min_lat, mid_lat, min_lon, mid_lon);
    // children[1] = new QuadTreeNode(min_lat, mid_lat, mid_lon, max_lon);
    // children[2] = new QuadTreeNode(mid_lat, max_lat, min_lon, mid_lon);
    // children[3] = new QuadTreeNode(mid_lat, max_lat, mid_lon, max_lon);
}



// 解析 OSM 文件的函数
void parseOSM(const string& filename, unordered_map<string, Node>& nodes, unordered_map<string, Way>& ways, QuadTree& quad_tree_ways) {
    XMLDocument doc;
    XMLError eResult = doc.LoadFile(filename.c_str());
    if (eResult != XML_SUCCESS) {
        cout << "Error loading OSM file!" << endl;
        cout << eResult << endl;
        return;
    }

    // 解析 Node
    XMLNode* pRoot = doc.FirstChildElement("osm");
    XMLElement* pNodeElement = pRoot->FirstChildElement("node");
    while (pNodeElement) {
        Node node;
        node.id = pNodeElement->Attribute("id"); // 读取为string
        node.lat = pNodeElement->DoubleAttribute("lat");
        node.lon = pNodeElement->DoubleAttribute("lon");

        // 解析 Node 的 Tag
        XMLElement* pTagElement = pNodeElement->FirstChildElement("tag");
        while (pTagElement) {
            string key = pTagElement->Attribute("k");
            string value = pTagElement->Attribute("v");
            node.tags.push_back(make_pair(key, value));
            pTagElement = pTagElement->NextSiblingElement("tag");
        }

        nodes[node.id] = node; // 将 Node 存储到 unordered_map 中
        pNodeElement = pNodeElement->NextSiblingElement("node");
    }

    // 解析 Way
    XMLElement* pWayElement = pRoot->FirstChildElement("way");
    while (pWayElement) {
        Way way;
        way.id = pWayElement->Attribute("id"); // 读取为string

        // 解析 Way 的节点引用
        XMLElement* pNdElement = pWayElement->FirstChildElement("nd");
        while (pNdElement) {
            string ref = pNdElement->Attribute("ref"); // 存储为string
            way.node_refs.push_back(ref);
            pNdElement = pNdElement->NextSiblingElement("nd");
        }

        // 解析 Way 的 Tag
        XMLElement* pWayTagElement = pWayElement->FirstChildElement("tag");
        while (pWayTagElement) {
            string key = pWayTagElement->Attribute("k");
            string value = pWayTagElement->Attribute("v");
            way.tags.push_back(make_pair(key, value));
            pWayTagElement = pWayTagElement->NextSiblingElement("tag");
        }

        ways[way.id] = way; // 将 Way 存储到 unordered_map 中
        quad_tree_ways.insert(way, nodes); // 将 Way 存储到 QuadTree 中
        pWayElement = pWayElement->NextSiblingElement("way");
    }

    // 打印读取的 Node 和 Way（可选）
    /*
    for (const auto& pair : nodes) {
        pair.second.print();
    }
    for (const auto& pair : ways) {
        pair.second.print();
    }
    */
}

// int main() {
//     vector<Node> nodes;
//     vector<Way> ways;
//     // 解析 OSM 文件（确保路径正确）
//     parseOSM("data/map", nodes, ways);
//     for (const Node& node : nodes) {
//         node.print();
//     }
//     for (const Way& way : ways) {
//         way.print();
//     }
//     return 0;
// }
