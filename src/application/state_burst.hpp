#ifndef STATE_BURST_HPP
#define STATE_BURST_HPP

#include "../domain/state_topic.hpp"

namespace application {

// Brackets a burst of state changes: it opens on construction and closes on
// destruction, so the two halves cannot drift apart and an exception in
// between still closes it.
//
// Consumers that would rebuild per change hold that rebuild while the bracket
// stands and do it once at the end. Whoever ignores the topic keeps working
// exactly as before — the bracket is an offer, not a protocol.
class StateBurst {
 public:
  explicit StateBurst(domain::StateTopic<bool>& topic);

  ~StateBurst();

  StateBurst(const StateBurst&) = delete;
  StateBurst& operator=(const StateBurst&) = delete;
  StateBurst(StateBurst&&) = delete;
  StateBurst& operator=(StateBurst&&) = delete;

 private:
  domain::StateTopic<bool>& topic_;
};

}  // namespace application

#endif  // STATE_BURST_HPP
