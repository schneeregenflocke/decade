#ifndef STATE_TOPIC_HPP
#define STATE_TOPIC_HPP

#include <sigslot/signal.hpp>

namespace domain {

// Der Kanal, auf dem ein Store seinen Zustand veröffentlicht — der Port im
// Sinn von Ports & Adapters. Ein Store bekommt sein Topic per Referenz
// eingesetzt und ruft es auf, statt ein eigenes Signal zu besitzen; wer das
// Topic besitzt, entscheidet die äussere Schicht (in der Anwendung: EventBus).
//
// Warum nicht der EventBus selbst: die Domain darf nichts aus der Application
// kennen. Ein Topic ist nur ein typisiertes Signal — die Domain hängt damit
// weiterhin bloss an sigslot, kennt aber weder den Bus noch dessen übrige
// Themen. Nebenbei bleibt die Kopplung minimal: ein Store sieht genau den
// einen Kanal, den er bedient.
template <typename Value>
using StateTopic = sigslot::signal<const Value&>;

}  // namespace domain

#endif  // STATE_TOPIC_HPP
