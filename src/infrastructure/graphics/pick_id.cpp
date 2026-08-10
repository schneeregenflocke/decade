#include "pick_id.hpp"

#include <string_view>

std::string_view PickKindName(PickId::Kind kind) {
  switch (kind) {
    case PickId::Kind::kBar:
      return "bar";
    case PickId::Kind::kTitle:
      return "title";
  }
  return "unknown";
}
