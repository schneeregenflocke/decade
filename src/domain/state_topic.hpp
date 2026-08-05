#ifndef STATE_TOPIC_HPP
#define STATE_TOPIC_HPP

#include <sigslot/signal.hpp>

namespace domain {

// The channel a store publishes its state on — the port in the sense of ports
// and adapters. A store gets its topic injected by reference and calls it,
// instead of owning a signal of its own; who owns the topic is the outer layer's
// decision (the EventBus in the application).
//
// Why not the event bus itself: the domain may know nothing from the
// application. A topic is merely a typed signal — the domain thereby still hangs
// on sigslot alone and knows neither the bus nor its remaining topics. The
// coupling stays minimal along the way: a store sees exactly the one channel it
// serves.
template <typename Value>
using StateTopic = sigslot::signal<const Value&>;

}  // namespace domain

#endif  // STATE_TOPIC_HPP
