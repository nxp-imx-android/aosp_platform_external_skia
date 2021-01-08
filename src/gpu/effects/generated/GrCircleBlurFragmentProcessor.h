/*
 * Copyright 2018 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/**************************************************************************************************
 *** This file was autogenerated from GrCircleBlurFragmentProcessor.fp; do not modify.
 **************************************************************************************************/
#ifndef GrCircleBlurFragmentProcessor_DEFINED
#define GrCircleBlurFragmentProcessor_DEFINED

#include "include/core/SkM44.h"
#include "include/core/SkTypes.h"

#include "src/gpu/effects/GrTextureEffect.h"

#include "src/gpu/GrFragmentProcessor.h"

class GrCircleBlurFragmentProcessor : public GrFragmentProcessor {
public:
    static std::unique_ptr<GrFragmentProcessor> Make(std::unique_ptr<GrFragmentProcessor> inputFP,
                                                     GrRecordingContext*,
                                                     const SkRect& circle,
                                                     float sigma);
    GrCircleBlurFragmentProcessor(const GrCircleBlurFragmentProcessor& src);
    std::unique_ptr<GrFragmentProcessor> clone() const override;
    const char* name() const override { return "CircleBlurFragmentProcessor"; }
    SkRect circleRect;
    float solidRadius;
    float textureRadius;

private:
    GrCircleBlurFragmentProcessor(std::unique_ptr<GrFragmentProcessor> inputFP,
                                  SkRect circleRect,
                                  float solidRadius,
                                  float textureRadius,
                                  std::unique_ptr<GrFragmentProcessor> blurProfile)
            : INHERITED(kGrCircleBlurFragmentProcessor_ClassID,
                        (OptimizationFlags)(inputFP ? ProcessorOptimizationFlags(inputFP.get())
                                                    : kAll_OptimizationFlags) &
                                kCompatibleWithCoverageAsAlpha_OptimizationFlag)
            , circleRect(circleRect)
            , solidRadius(solidRadius)
            , textureRadius(textureRadius) {
        this->registerChild(std::move(inputFP), SkSL::SampleUsage::PassThrough());
        this->registerChild(std::move(blurProfile), SkSL::SampleUsage::Explicit());
    }
    GrGLSLFragmentProcessor* onCreateGLSLInstance() const override;
    void onGetGLSLProcessorKey(const GrShaderCaps&, GrProcessorKeyBuilder*) const override;
    bool onIsEqual(const GrFragmentProcessor&) const override;
#if GR_TEST_UTILS
    SkString onDumpInfo() const override;
#endif
    GR_DECLARE_FRAGMENT_PROCESSOR_TEST
    using INHERITED = GrFragmentProcessor;
};
#endif
