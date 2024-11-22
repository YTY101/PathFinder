#include <iostream>
#include <tinyxml2.h>

using namespace tinyxml2;

int main() {
    // 创建 XMLDocument 对象
    XMLDocument doc;
    
    // 加载 XML 文件
    XMLError eResult = doc.LoadFile("test.xml");
    
    if (eResult != XML_SUCCESS) {
        std::cout << "Error loading XML file!" << std::endl;
        return -1;
    }

    // 获取根元素
    XMLElement* root = doc.RootElement();
    
    if (root == nullptr) {
        std::cout << "Error getting root element!" << std::endl;
        return -1;
    }

    // 获取子元素
    XMLElement* element = root->FirstChildElement("element");
    
    if (element == nullptr) {
        std::cout << "Error finding element!" << std::endl;
        return -1;
    }

    // 获取并打印属性值和文本内容
    const char* id = element->Attribute("id");
    const char* text = element->GetText();

    std::cout << "Element ID: " << id << std::endl;
    std::cout << "Element Text: " << text << std::endl;

    return 0;
}
