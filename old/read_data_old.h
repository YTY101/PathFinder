#ifndef READ_DATA_OLD
#define READ_DATA_OLD
#include<iostream>
#include<string>
#include<vector>
#include<unordered_map>
#include<memory>
using namespace std;

// 定义 Node 结构体
struct Node {
    string id;
    double lat;
    double lon;
    vector<pair<string, string>> tags;  // 用来存储键值对 (k, v)

    void print() const {
        cout << "Node ID: " << id << ", Latitude: " << lat << ", Longitude: " << lon << endl;
        for (const auto& tag : tags) {
            cout << "  Tag: " << tag.first << " = " << tag.second << endl;
        }
    }
};

// 定义 Way 结构体
struct Way {
    string id;
    vector<string> node_refs;  // 存储节点的 ID
    vector<pair<string, string>> tags;  // 用来存储键值对 (k, v)

    void print() const {
        cout << "Way ID: " << id << ", Nodes: ";
        for (const string node_id : node_refs) {
            cout << node_id << " ";
        }
        cout << endl;
        for (const auto& tag : tags) {
            cout << "  Tag: " << tag.first << " = " << tag.second << endl;
        }
    }
};

// 四叉树节点
class QuadTreeNode {
public:
    double min_lat, max_lat, min_lon, max_lon;
    std::vector<Way> ways;
    std::unique_ptr<QuadTreeNode> children[4]; // 四个子节点
    // QuadTreeNode* children[4]; // 四个子节点

    QuadTreeNode(double minLat, double maxLat, double minLon, double maxLon)
        : min_lat(minLat), max_lat(maxLat), min_lon(minLon), max_lon(maxLon) {}

    bool insert(const Way& way, unordered_map<string, Node>& nodes);
    void subdivide();

    bool contains(double min_lat, double max_lat, double min_lon, double max_lon) {
        // 检查当前节点的范围是否完全包含给定范围
        return (min_lat >= this->min_lat && max_lat <= this->max_lat &&
                min_lon >= this->min_lon && max_lon <= this->max_lon);
    }
    void query_min_containing(double min_lat, double max_lat, double min_lon, double max_lon, QuadTreeNode*& found_node) {
        // 如果当前节点不包含给定范围，则返回
        if (!contains(min_lat, max_lat, min_lon, max_lon)) {
            return;
        }

        // 更新找到的节点为当前节点
        found_node = this;

        // 递归检查子节点
        if (children[0]) {
            for (auto& child : children) {
                // 先检查子节点的可能性
                child->query_min_containing(min_lat, max_lat, min_lon, max_lon, found_node);
            }
        }
    }

};

// 四叉树
class QuadTree {
public:
    QuadTreeNode* root;

    QuadTree(double min_lat, double max_lat, double min_lon, double max_lon) {
        root = new QuadTreeNode(min_lat, max_lat, min_lon, max_lon);
    }

    void insert(const Way& way, unordered_map<string, Node>& nodes) {
        root->insert(way, nodes);
    }

    QuadTreeNode* query_min_containing(double min_lat, double max_lat, double min_lon, double max_lon) {
        QuadTreeNode* found_node = nullptr;
        root->query_min_containing(min_lat, max_lat, min_lon, max_lon, found_node);
        return found_node;
    }
    
};



void parseOSM(const string& filename, unordered_map<string, Node>& nodes, unordered_map<string, Way>& ways, QuadTree& quad_tree_ways);



#endif