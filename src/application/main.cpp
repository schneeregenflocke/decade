#include <exception>
#include <iostream>

#include "decade_app.hpp"

// The last net: everything below reports its own failures, but a constructor on
// the way up (the ICU backend, for one) can still throw. An exception leaving
// main means std::terminate and no word about why.
int main(int argc, char** argv) {
  try {
    return application::RunDecadeApp(argc, argv);
  } catch (const std::exception& error) {
    std::cerr << "decade: " << error.what() << '\n';
    return 1;
  }
}
