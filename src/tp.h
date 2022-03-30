#ifndef TP_H
#define TP_H

#include <vector>

#include "scl.h"

namespace tp {
  using FF = scl::FF<61>;
  using Shr = FF;
  using Poly = scl::details::EvPolynomial<FF>;
  using Vec = scl::Vec<FF>;

  template<typename T>
  using vec = std::vector<T>;

}

#endif  // TP_H
