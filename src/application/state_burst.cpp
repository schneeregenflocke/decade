#include "state_burst.hpp"

#include "../domain/state_topic.hpp"

namespace application {

StateBurst::StateBurst(domain::StateTopic<bool>& topic) : topic_(topic) {
  topic_(true);
}

StateBurst::~StateBurst() { topic_(false); }

}  // namespace application
