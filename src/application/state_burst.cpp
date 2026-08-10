#include "state_burst.hpp"

#include "../domain/state_topics.hpp"

namespace application {

StateBurst::StateBurst(domain::StateBurstTopic& topic) : topic_(topic) {
  topic_.Publish(true);
}

StateBurst::~StateBurst() { topic_.Publish(false); }

}  // namespace application
