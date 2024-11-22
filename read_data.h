#ifndef READ_DATA
#define READ_DATA
#include<iostream>
#include<string>
#include<vector>
#include<unordered_map>
#include<memory>
#include<utility>
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


struct Neighbor {
    std::string node_id;
    double distance;
};

// 定义 CHUNK 结构体，用于存储 Node 和 Way
struct Chunk {
    vector<Node> nodes;
    vector<Way> ways;
};




string getChunkKey(double lat, double lon);
std::pair<string, int> getTargetChunk(string chunk_id, int depth, unordered_map<string, bool>& visited);

bool isNodeInChunk(const Node& node, double min_lat, double max_lat, double min_lon, double max_lon);
void parseOSM(const string& filename, unordered_map<string, Node>& nodes, unordered_map<string, Way>& ways, unordered_map<string, Chunk>& chunk_map);

double calculateDistance(const Node& node1, const Node& node2);


#endif