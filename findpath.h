#ifndef FINDPATH
#define FINDPATH
#include <nlohmann/json.hpp>
using json = nlohmann::json;

json aStarSearch(const std::string& start_id, const std::string& end_id);

#endif