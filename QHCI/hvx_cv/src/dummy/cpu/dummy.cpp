#include "QhciBase.hpp"

#define FEATURE_NAME "dummy"

//Step0. Define reference code here

class DUMMY : public QhciFeatureBase {
public:
    using QhciFeatureBase::QhciFeatureBase;
    int Test(remote_handle64 handle) override {
        AEEResult nErr = 0;
        //Api teat
        {
            //Step1. Define param && allocate buffer

            //Step2. Random generate input data, if need to use real data, pls comment this and implement by yourself.

            //Step3. CPU execute

            //Step4. DSP execute

            //Step5. Compare CPU & DSP results

            //Step6. Free buffer
        }

        return nErr;
    }
};

static DUMMY* _feature = new DUMMY(FEATURE_NAME);
static bool init = [] {
    QhciTest::GetInstance().current_map_func()[FEATURE_NAME] = _feature;
    return true;
}();
#undef FEATURE_NAME
