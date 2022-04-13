#ifndef FD_PREP_H
#define FD_PREP_H

#include <catch2/catch.hpp>
#include <iostream>
#include <assert.h>

#include "tp/circuits.h"

namespace tp {
  class FDPreprocessing {
  public:
    FDPreprocessing(Circuit circuit) : mCircuit(circuit) {};


    void _DummyPrep() {
      mCircuit._DummyPrep();
    }

  private:
    Circuit mCircuit;
  };


  

} // namespace tp

#endif  // FD_PREP_H
