#ifndef HOME_TITAN99_CODE_DECADE_SRC_PACKAGES_PAGE_CONFIG_HPP
#define HOME_TITAN99_CODE_DECADE_SRC_PACKAGES_PAGE_CONFIG_HPP

#include <array>

#include <sigslot/signal.hpp>

#include <boost/serialization/access.hpp>
#include <boost/serialization/array.hpp>
#include <boost/serialization/nvp.hpp>
#include <boost/serialization/split_member.hpp>

struct PageSetupConfig {
  std::array<float, 2> size;
  std::array<float, 4> margins;
  int orientation;

private:
  friend class boost::serialization::access;
  template <class Archive> void serialize(Archive &ar, const unsigned int /*version*/)
  {
    ar &BOOST_SERIALIZATION_NVP(size);
    ar &BOOST_SERIALIZATION_NVP(margins);
    ar &BOOST_SERIALIZATION_NVP(orientation);
  }
};

class PageSetupStore {
public:
  void ReceivePageSetup(const PageSetupConfig &page_setup_config)
  {
    this->page_setup_config = page_setup_config;
    SendPageSetup();
  }

  void SendPageSetup() { signal_page_setup_config(page_setup_config); }

  sigslot::signal<const PageSetupConfig &> signal_page_setup_config;

  PageSetupConfig page_setup_config;

private:
  friend class boost::serialization::access;
  template <class Archive> void save(Archive &ar, const unsigned int /*version*/) const
  {
    ar &BOOST_SERIALIZATION_NVP(page_setup_config);
  }
  template <class Archive> void load(Archive &ar, const unsigned int /*version*/)
  {
    ar &BOOST_SERIALIZATION_NVP(page_setup_config);
    signal_page_setup_config(page_setup_config);
  }
  BOOST_SERIALIZATION_SPLIT_MEMBER()
};
#endif // HOME_TITAN99_CODE_DECADE_SRC_PACKAGES_PAGE_CONFIG_HPP
