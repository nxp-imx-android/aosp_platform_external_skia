

/**************************************************************************************************
 *** This file was autogenerated from GrSectionCpp.fp; do not modify.
 **************************************************************************************************/
#include "GrSectionCpp.h"

cpp section#include "src/core/SkUtils.h"
#include "src/gpu/GrTexture.h"
#include "src/gpu/glsl/GrGLSLFragmentProcessor.h"
#include "src/gpu/glsl/GrGLSLFragmentShaderBuilder.h"
#include "src/gpu/glsl/GrGLSLProgramBuilder.h"
#include "src/sksl/SkSLCPP.h"
#include "src/sksl/SkSLUtil.h"
class GrGLSLSectionCpp : public GrGLSLFragmentProcessor {
public:
    GrGLSLSectionCpp() {}
    void emitCode(EmitArgs& args) override {
        GrGLSLFPFragmentBuilder* fragBuilder = args.fFragBuilder;
        const GrSectionCpp& _outer = args.fFp.cast<GrSectionCpp>();
        (void) _outer;
        fragBuilder->codeAppendf(
R"SkSL(return half4(1.0);
)SkSL"
);
    }
private:
    void onSetData(const GrGLSLProgramDataManager& pdman, const GrFragmentProcessor& _proc) override {
    }
};
std::unique_ptr<GrGLSLFragmentProcessor> GrSectionCpp::onMakeProgramImpl() const {
    return std::make_unique<GrGLSLSectionCpp>();
}
void GrSectionCpp::onGetGLSLProcessorKey(const GrShaderCaps& caps, GrProcessorKeyBuilder* b) const {
}
bool GrSectionCpp::onIsEqual(const GrFragmentProcessor& other) const {
    const GrSectionCpp& that = other.cast<GrSectionCpp>();
    (void) that;
    return true;
}
GrSectionCpp::GrSectionCpp(const GrSectionCpp& src)
: INHERITED(kGrSectionCpp_ClassID, src.optimizationFlags()) {
        this->cloneAndRegisterAllChildProcessors(src);
}
std::unique_ptr<GrFragmentProcessor> GrSectionCpp::clone() const {
    return std::make_unique<GrSectionCpp>(*this);
}
#if GR_TEST_UTILS
SkString GrSectionCpp::onDumpInfo() const {
    return SkString();
}
#endif
