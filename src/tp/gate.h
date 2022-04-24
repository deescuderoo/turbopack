#ifndef GATE_H
#define GATE_H

#include <vector>
#include <assert.h>

#include "tp.h"

namespace tp {

  class Gate {
  public:

    // Individual share of the mask (lambda...lambda) associated to
    // the output of this gate.
    FF GetShrLambda() { return mIndvShrLambdaC; };

    // Intended to be called by P1
    virtual FF GetMu() = 0;

    // Intended to be called by P1
    bool IsLearned() { return mLearned; };

    // For cleartext evaluation
    bool IsEvaluated() { return mEvaluated; }

    // For testing purposes. Sets mu to the given value
    void _SetDummyMu(FF mu) {
      mMu = mu;
      mLearned = true;
    }

    // To get indv_shr lambda 
    virtual FF GetIndvShrLambda() = 0; 

    // Setting ATLAS share
    void SetAtlasShare(FF share) {
      mAtlasShare = share;
      mAtlasSet = true;
    }

    // To get share when run with ATLAS
    virtual FF GetAtlasShare() = 0; 

    // To get lambda when having fake preprocessing
    virtual FF GetDummyLambda() = 0; 
    
    // Get the cleartext value associated to the output of this gate,
    // if computed already (cleartext evaluation)
    virtual FF GetClear() = 0;

    std::shared_ptr<Gate> GetLeft() { return mLeft; }
    std::shared_ptr<Gate> GetRight() { return mRight; }

    bool IsPadding() { return mIsPadding; }

  protected:
    // Bool that indicates whether P1 learned Mu already
    bool mLearned = false;

    // Sharing of (lambda ... lambda)
    FF mIndvShrLambdaC = FF(0);
    bool mIndvShrLambdaCSet = false;

    // ATLAS-related
    FF mAtlasShare;
    bool mAtlasSet = false;

    // mu = value - lambda
    // Learned by P1 in the online phase
    FF mMu;

    // Actual lambda. Used for debugging purposes
    FF mLambda;
    bool mLambdaSet = false;

    // Parents
    std::shared_ptr<Gate> mLeft;
    std::shared_ptr<Gate> mRight;

    // For cleartext evaluation
    // Bool that indicates whether the gate has been evaluated
    bool mEvaluated = false;
    // Evaluation of the output wire of the gate
    FF mClear;

    bool mIsPadding = false;


    friend class MultBatch;
  };  

  class AddGate : public Gate {
  public:
    AddGate(std::shared_ptr<Gate> left, std::shared_ptr<Gate> right) {
      mLeft = left;
      mRight = right;
      mIndvShrLambdaC = left->GetShrLambda() + right->GetShrLambda();
    }

    FF GetMu() {
      if ( !mLearned ) {
	mMu = mLeft->GetMu() + mRight->GetMu();
	mLearned = true;
      }
      return mMu;
    }

    FF GetClear() {
      if ( !mEvaluated ) {
	mClear = mLeft->GetClear() + mRight->GetClear();
	mEvaluated = true;
      }
      return mClear;
    }

    FF GetIndvShrLambda() {
      if ( !mIndvShrLambdaCSet ) {
	mIndvShrLambdaC = mLeft->GetIndvShrLambda() + mRight->GetIndvShrLambda();
	mIndvShrLambdaCSet = true;
      }
      return mIndvShrLambdaC;
    }    

    FF GetAtlasShare() {
      if ( !mAtlasSet ) {
	mAtlasShare = mLeft->GetAtlasShare() + mRight->GetAtlasShare();
	mAtlasSet = true;
      }
      return mAtlasShare;
    }    

    FF GetDummyLambda() {
      if ( !mLambdaSet ) {
	mLambda = mLeft->GetDummyLambda() + mRight->GetDummyLambda();
	mLambdaSet = true;
      }
      return mLambda;
    }    
  };
} // namespace tp

#endif  // GATE_H
