#ifndef OUT_GATE_H
#define OUT_GATE_H

#include <vector>
#include <assert.h>

#include "tp.h"
#include "gate.h"

namespace tp {
  class OutputGate : public Gate {
  public:
    OutputGate(std::shared_ptr<Gate> gate) {
      mLeft = gate;
      mRight = nullptr;
    };

    FF GetMu() {
      if ( !mLearned ) {
	mMu = mLeft->GetMu();
	mLearned = true;
      }
      return mMu;
    };

    FF GetClear() {
      if ( !mEvaluated ) {
	mClear = mLeft->GetClear();
	mEvaluated = true;
      }
      return mClear;
    };

    // TODO output protocol
    
  };
} // namespace tp

#endif  // OUT_GATE_H
