#include "Log.h"

namespace Log {

namespace detail {
  int NullBuffer::overflow(int c) { return c; }

  NullBuffer nullBuffer {};
  std::ostream nullStream(&nullBuffer);
} // namespace detail

std::ostream& log(const Level& decisionLevel) {
  if(decisionLevel >= level) {
    return std::cout;
  }

  return detail::nullStream;
}

std::ostream& log(const Particulars& particular) {
  if(particulars.count(particular) == 1) {
    return std::cout;
  } 

  return detail::nullStream;
}

Level level = Level::Trace;
std::set<Particulars> particulars {};

} // namespace Log