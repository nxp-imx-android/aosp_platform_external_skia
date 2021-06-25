

/**************************************************************************************************
 *** This file was autogenerated from GrDSLFPTest_Ternary.fp; do not modify.
 **************************************************************************************************/
/* TODO(skia:11854): DSLCPPCodeGenerator is currently a work in progress. */
#include "GrDSLFPTest_Ternary.h"

#include "src/core/SkUtils.h"
#include "src/gpu/GrTexture.h"
#include "src/gpu/glsl/GrGLSLFragmentProcessor.h"
#include "src/gpu/glsl/GrGLSLFragmentShaderBuilder.h"
#include "src/gpu/glsl/GrGLSLProgramBuilder.h"
#include "src/sksl/SkSLCPP.h"
#include "src/sksl/SkSLUtil.h"
#include "src/sksl/dsl/priv/DSLFPs.h"
#include "src/sksl/dsl/priv/DSLWriter.h"

class GrGLSLDSLFPTest_Ternary : public GrGLSLFragmentProcessor {
public:
    GrGLSLDSLFPTest_Ternary() {}
    void emitCode(EmitArgs& args) override {
        [[maybe_unused]] const GrDSLFPTest_Ternary& _outer = args.fFp.cast<GrDSLFPTest_Ternary>();

        using namespace SkSL::dsl;
        StartFragmentProcessor(this, &args);
[[maybe_unused]] const auto& primaryColors = _outer.primaryColors;
Var _primaryColors(kConst_Modifier, DSLType(kBool_Type), "primaryColors", Bool(!!(primaryColors)));
Declare(_primaryColors);
Var _colorGreen;
if (primaryColors) {
    Var(kUniform_Modifier, DSLType(kHalf4_Type), "colorGreen").swap(_colorGreen);
    colorGreenVar = VarUniformHandle(_colorGreen);
    DeclareGlobal(_colorGreen);
} else {
    Var(kConst_Modifier, DSLType(kHalf4_Type), "colorGreen", Half4(0)).swap(_colorGreen);
    Declare(_colorGreen);
}
Var _colorRed;
if (primaryColors) {
    Var(kUniform_Modifier, DSLType(kHalf4_Type), "colorRed").swap(_colorRed);
    colorRedVar = VarUniformHandle(_colorRed);
    DeclareGlobal(_colorRed);
} else {
    Var(kConst_Modifier, DSLType(kHalf4_Type), "colorRed", Half4(0)).swap(_colorRed);
    Declare(_colorRed);
}
Var _colorOrange;
if (!primaryColors) {
    Var(kUniform_Modifier, DSLType(kHalf4_Type), "colorOrange").swap(_colorOrange);
    colorOrangeVar = VarUniformHandle(_colorOrange);
    DeclareGlobal(_colorOrange);
} else {
    Var(kConst_Modifier, DSLType(kHalf4_Type), "colorOrange", Half4(0)).swap(_colorOrange);
    Declare(_colorOrange);
}
Var _colorPurple;
if (!primaryColors) {
    Var(kUniform_Modifier, DSLType(kHalf4_Type), "colorPurple").swap(_colorPurple);
    colorPurpleVar = VarUniformHandle(_colorPurple);
    DeclareGlobal(_colorPurple);
} else {
    Var(kConst_Modifier, DSLType(kHalf4_Type), "colorPurple", Half4(0)).swap(_colorPurple);
    Declare(_colorPurple);
}
Var _green(kNo_Modifier, DSLType(kHalf4_Type), "green", Select(_primaryColors, /*If True:*/ _colorGreen, /*If False:*/ _colorOrange));
Var _red(kNo_Modifier, DSLType(kHalf4_Type), "red", Select(_primaryColors, /*If True:*/ _colorRed, /*If False:*/ _colorPurple));
Var _t(kNo_Modifier, DSLType(kBool_Type), "t", true);
Var _f(kNo_Modifier, DSLType(kBool_Type), "f", false);
Declare(_green);
Declare(_red);
Declare(_t);
Declare(_f);
Return(Half4(Select(_t, /*If True:*/ _green.x(), /*If False:*/ _red.x()), Select(_f, /*If True:*/ _red.y(), /*If False:*/ _green.y()), Select(_green.y() == _red.x(), /*If True:*/ _green.z(), /*If False:*/ _red.x()), Select(_green.w() != _red.w(), /*If True:*/ _red.y(), /*If False:*/ _green.w())));
        EndFragmentProcessor();
    }
private:
    void onSetData(const GrGLSLProgramDataManager& pdman, const GrFragmentProcessor& _proc) override {
        const GrDSLFPTest_Ternary& _outer = _proc.cast<GrDSLFPTest_Ternary>();
        {
        if (colorGreenVar.isValid()) {
            pdman.set4fv(colorGreenVar, 1, (_outer.colorGreen).vec());
        }
        if (colorRedVar.isValid()) {
            pdman.set4fv(colorRedVar, 1, (_outer.colorRed).vec());
        }
        if (colorOrangeVar.isValid()) {
            pdman.set4fv(colorOrangeVar, 1, (_outer.colorOrange).vec());
        }
        if (colorPurpleVar.isValid()) {
            pdman.set4fv(colorPurpleVar, 1, (_outer.colorPurple).vec());
        }
        }
    }
    UniformHandle colorGreenVar;
    UniformHandle colorRedVar;
    UniformHandle colorOrangeVar;
    UniformHandle colorPurpleVar;
};
std::unique_ptr<GrGLSLFragmentProcessor> GrDSLFPTest_Ternary::onMakeProgramImpl() const {
    return std::make_unique<GrGLSLDSLFPTest_Ternary>();
}
void GrDSLFPTest_Ternary::onGetGLSLProcessorKey(const GrShaderCaps& caps, GrProcessorKeyBuilder* b) const {
    b->addBool(primaryColors, "primaryColors");
}
bool GrDSLFPTest_Ternary::onIsEqual(const GrFragmentProcessor& other) const {
    const GrDSLFPTest_Ternary& that = other.cast<GrDSLFPTest_Ternary>();
    (void) that;
    if (primaryColors != that.primaryColors) return false;
    if (colorGreen != that.colorGreen) return false;
    if (colorRed != that.colorRed) return false;
    if (colorOrange != that.colorOrange) return false;
    if (colorPurple != that.colorPurple) return false;
    return true;
}
GrDSLFPTest_Ternary::GrDSLFPTest_Ternary(const GrDSLFPTest_Ternary& src)
: INHERITED(kGrDSLFPTest_Ternary_ClassID, src.optimizationFlags())
, primaryColors(src.primaryColors)
, colorGreen(src.colorGreen)
, colorRed(src.colorRed)
, colorOrange(src.colorOrange)
, colorPurple(src.colorPurple) {
        this->cloneAndRegisterAllChildProcessors(src);
}
std::unique_ptr<GrFragmentProcessor> GrDSLFPTest_Ternary::clone() const {
    return std::make_unique<GrDSLFPTest_Ternary>(*this);
}
#if GR_TEST_UTILS
SkString GrDSLFPTest_Ternary::onDumpInfo() const {
    return SkStringPrintf("(primaryColors=%d, colorGreen=half4(%f, %f, %f, %f), colorRed=half4(%f, %f, %f, %f), colorOrange=half4(%f, %f, %f, %f), colorPurple=half4(%f, %f, %f, %f))", !!(primaryColors), colorGreen.fR, colorGreen.fG, colorGreen.fB, colorGreen.fA, colorRed.fR, colorRed.fG, colorRed.fB, colorRed.fA, colorOrange.fR, colorOrange.fG, colorOrange.fB, colorOrange.fA, colorPurple.fR, colorPurple.fG, colorPurple.fB, colorPurple.fA);
}
#endif
