#include <vector>
#include <yjson.h>

std::vector<int> js2array(const YJson& json) {
  std::vector<int> result;
  for (auto& i: json.getArray()) {
    result.push_back(i.getValueInt());
  }
  return result;
}
