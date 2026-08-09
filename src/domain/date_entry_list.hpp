#ifndef DATE_ENTRY_LIST_HPP
#define DATE_ENTRY_LIST_HPP

#include <cstddef>
#include <vector>

#include "date_entry.hpp"
#include "date_group.hpp"

// A value object: the entries of a project in canonical form. `Assign` discards
// null periods, sorts by begin and derives everything derived anew — the
// running number, the gap period to the next entry, the number within the group
// — and cuts groups that no longer exist.
//
// Separate from the store, because two holders need the same preparation but
// only one of them publishes: DateEntryStore publishes, DateEntryBars
// computes bars out of it. They used to share this through inheritance with
// protected access; composition manages without both.
class DateEntryList {
 public:
  void Assign(const std::vector<DateEntry>& incoming_date_entries);

  // Changing the groups re-clamps the stored entries: deleting a group must not
  // leave an entry pointing at it. The scene builder addresses its group nodes
  // by that index (`group_nodes.at(...)`), so a stale one takes the whole
  // rebuild down.
  void AssignDateGroups(const std::vector<DateGroup>& incoming_date_groups);

  [[nodiscard]] const std::vector<DateEntry>& Items() const;

  [[nodiscard]] bool IsEmpty() const;

  [[nodiscard]] std::size_t YearSpan() const;

  [[nodiscard]] int FirstYear() const;

  [[nodiscard]] int LastYear() const;

 private:
  void Sort();

  void AssignNumbers();

  void AssignInterIntervals();

  void AssignGroupNumbers();

  // The invariant every consumer relies on: an entry's group indexes a group
  // that exists. Both ends are guarded — a project file carries the number
  // unchecked, so a negative one arrives just as a too-large one does.
  //
  // Group 0 is the fallback because there is always one: DateGroupStore seeds a
  // `Default` group, and the groups panel refuses to delete row 0. Without a
  // single group the invariant has nothing to hold to, and no entry could be
  // drawn anyway.
  void ClampGroupsToKnownRange();

  std::vector<DateEntry> date_entries_;
  DateGroups date_groups_;
};

#endif  // DATE_ENTRY_LIST_HPP
