

/**************************************************************************************************
 *** This file was autogenerated from GrMainCoords.fp; do not modify.
 **************************************************************************************************/
#include "GrMainCoords.h"

#include "src/core/SkUtils.h"
#include "src/gpu/GrTexture.h"
#include "src/gpu/glsl/GrGLSLFragmentProcessor.h"
#include "src/gpu/glsl/GrGLSLFragmentShaderBuilder.h"
#include "src/gpu/glsl/GrGLSLProgramBuilder.h"
#include "src/sksl/SkSLCPP.h"
#include "src/sksl/SkSLUtil.h"
class GrGLSLMainCoords : public GrGLSLFragmentProcessor {
public:
    GrGLSLMainCoords() {}
    void emitCode(EmitArgs& args) override {
        GrGLSLFPFragmentBuilder* fragBuilder = args.fFragBuilder;
        const GrMainCoords& _outer = args.fFp.cast<GrMainCoords>();
        (void) _outer;
        fragBuilder->codeAppendf(
R"SkSL(return half4(half2(%s), half2(%s));
)SkSL"
, args.fSampleCoord, args.fSampleCoord);
    }
private:
    void onSetData(const GrGLSLProgramDataManager& pdman, const GrFragmentProcessor& _proc) override {
    }
};
std::unique_ptr<GrGLSLFragmentProcessor> GrMainCoords::onMakeProgramImpl() const {
    return std::make_unique<GrGLSLMainCoords>();
}
void GrMainCoords::onGetGLSLProcessorKey(const GrShaderCaps& caps, GrProcessorKeyBuilder* b) const {
}
bool GrMainCoords::onIsEqual(const GrFragmentProcessor& other) const {
    const GrMainCoords& that = other.cast<GrMainCoords>();
    (void) that;
    return true;
}
GrMainCoords::GrMainCoords(const GrMainCoords& src)
: INHERITED(kGrMainCoords_ClassID, src.optimizationFlags()) {
        this->cloneAndRegisterAllChildProcessors(src);
    this->setUsesSampleCoordsDirectly();
}
std::unique_ptr<GrFragmentProcessor> GrMainCoords::clone() const {
    return std::make_unique<GrMainCoords>(*this);
}
#if GR_TEST_UTILS
SkString GrMainCoords::onDumpInfo() const {
    return SkString();
}
#endif
