/*
 * Copyright 2020 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "imgui.h"
#include "include/core/SkBitmap.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkPath.h"
#include "include/utils/SkParsePath.h"
#include "samplecode/Sample.h"

#include "src/core/SkGeometry.h"

#include <stack>

namespace {

//////////////////////////////////////////////////////////////////////////////

constexpr inline SkPoint rotate90(const SkPoint& p) { return {p.fY, -p.fX}; }
inline SkPoint rotate180(const SkPoint& p) { return p * -1; }
inline bool isClockwise(const SkPoint& a, const SkPoint& b) { return a.cross(b) > 0; }

/** Version of setLength that asserts on failure to help catch edge cases */
SkPoint setLength(SkPoint p, float len) {
    if (!p.setLength(len)) {
        SkDebugf("Failed to set point length\n");
        SkASSERT(false);
    }
    return p;
}

constexpr uint64_t choose(uint64_t n, uint64_t k) {
    SkASSERT(n >= k);
    uint64_t result = 1;
    for (uint64_t i = 1; i <= k; i++) {
        result *= (n + 1 - i);
        result /= i;
    }
    return result;
}

//////////////////////////////////////////////////////////////////////////////

/**
 * A scalar (float-valued weights) Bezier curve of arbitrary degree.
 */
class ScalarBezCurve {
public:
    static constexpr int kDegreeInvalid = -1;

    /** Creates an empty curve with invalid degree. */
    ScalarBezCurve() : fDegree(kDegreeInvalid) {}

    /** Creates a curve of the specified degree with weights initialized to 0. */
    explicit ScalarBezCurve(int degree) : fDegree(degree) {
        SkASSERT(degree >= 0);
        fWeights.resize(degree + 1, {0});
    }

    /** Creates a curve of specified degree with the given weights. */
    ScalarBezCurve(int degree, const std::vector<float>& weights) : ScalarBezCurve(degree) {
        SkASSERT(degree >= 0);
        SkASSERT(weights.size() == (size_t)degree + 1);
        fWeights.insert(fWeights.begin(), weights.begin(), weights.end());
    }

    /** Returns the extreme-valued weight */
    float extremumWeight() const {
        float f = 0;
        int sign = 1;
        for (float w : fWeights) {
            if (std::abs(w) > f) {
                f = std::abs(w);
                sign = w >= 0 ? 1 : -1;
            }
        }
        return sign * f;
    }

    /** Evaluates the curve at t */
    float eval(float t) const { return Eval(*this, t); }

    /** Evaluates the curve at t */
    static float Eval(const ScalarBezCurve& curve, float t) {
        // Set up starting point of recursion (k=0)
        ScalarBezCurve result = curve;

        for (int k = 1; k <= curve.fDegree; k++) {
            // k is level of recursion, k-1 has previous level's values.
            for (int i = curve.fDegree; i >= k; i--) {
                result.fWeights[i] = result.fWeights[i - 1] * (1 - t) + result.fWeights[i] * t;
            }
        }

        return result.fWeights[curve.fDegree];
    }

    /** Splits this curve at t into two halves (of the same degree) */
    void split(float t, ScalarBezCurve* left, ScalarBezCurve* right) const {
        Split(*this, t, left, right);
    }

    /** Splits the curve at t into two halves (of the same degree) */
    static void Split(const ScalarBezCurve& curve,
                      float t,
                      ScalarBezCurve* left,
                      ScalarBezCurve* right) {
        // Set up starting point of recursion (k=0)
        const int degree = curve.fDegree;
        ScalarBezCurve result = curve;
        *left = ScalarBezCurve(degree);
        *right = ScalarBezCurve(degree);
        left->fWeights[0] = curve.fWeights[0];
        right->fWeights[degree] = curve.fWeights[degree];

        for (int k = 1; k <= degree; k++) {
            // k is level of recursion, k-1 has previous level's values.
            for (int i = degree; i >= k; i--) {
                result.fWeights[i] = result.fWeights[i - 1] * (1 - t) + result.fWeights[i] * t;
            }

            left->fWeights[k] = result.fWeights[k];
            right->fWeights[degree - k] = result.fWeights[degree];
        }
    }

    /**
     * Increases the degree of the curve to the given degree. Has no effect if the
     * degree is already equal to the given degree.
     *
     * This process is always exact (NB the reverse, degree reduction, is not exact).
     */
    void elevateDegree(int newDegree) {
        if (newDegree == fDegree) {
            return;
        }

        fWeights = ElevateDegree(*this, newDegree).fWeights;
        fDegree = newDegree;
    }

    /**
     * Increases the degree of the curve to the given degree. Has no effect if the
     * degree is already equal to the given degree.
     *
     * This process is always exact (NB the reverse, degree reduction, is not exact).
     */
    static ScalarBezCurve ElevateDegree(const ScalarBezCurve& curve, int newDegree) {
        SkASSERT(newDegree >= curve.degree());
        if (newDegree == curve.degree()) {
            return curve;
        }

        // From Farouki, Rajan, "Algorithms for polynomials in Bernstein form" 1988.
        ScalarBezCurve elevated(newDegree);
        const int r = newDegree - curve.fDegree;
        const int n = curve.fDegree;

        for (int i = 0; i <= n + r; i++) {
            elevated.fWeights[i] = 0;
            for (int j = std::max(0, i - r); j <= std::min(n, i); j++) {
                const float f =
                        (choose(n, j) * choose(r, i - j)) / static_cast<float>(choose(n + r, i));
                elevated.fWeights[i] += curve.fWeights[j] * f;
            }
        }

        return elevated;
    }

    /**
     * Returns the zero-set of this curve, which is a list of t values where the curve crosses 0.
     */
    std::vector<float> zeroSet() const { return ZeroSet(*this); }

    /**
     * Returns the zero-set of the curve, which is a list of t values where the curve crosses 0.
     */
    static std::vector<float> ZeroSet(const ScalarBezCurve& curve) {
        constexpr float kTol = 0.001f;
        std::vector<float> result;
        ZeroSetRec(curve, 0, 1, kTol, &result);
        return result;
    }

    /** Multiplies the curve's weights by a constant value */
    static ScalarBezCurve Mul(const ScalarBezCurve& curve, float f) {
        ScalarBezCurve result = curve;
        for (int k = 0; k <= curve.fDegree; k++) {
            result.fWeights[k] *= f;
        }
        return result;
    }

    /**
     * Multiplies the two curves and returns the result.
     *
     * Degree of resulting curve is the sum of the degrees of the input curves.
     */
    static ScalarBezCurve Mul(const ScalarBezCurve& a, const ScalarBezCurve& b) {
        // From G. Elber, "Free form surface analysis using a hybrid of symbolic and numeric
        // computation". PhD thesis, 1992. p.11.
        const int n = a.degree(), m = b.degree();
        const int newDegree = n + m;
        ScalarBezCurve result(newDegree);

        for (int k = 0; k <= newDegree; k++) {
            result.fWeights[k] = 0;
            for (int i = std::max(0, k - n); i <= std::min(k, m); i++) {
                const float f =
                        (choose(m, i) * choose(n, k - i)) / static_cast<float>(choose(m + n, k));
                result.fWeights[k] += a.fWeights[i] * b.fWeights[k - i] * f;
            }
        }

        return result;
    }

    /** Returns a^2 + b^2. This is a specialized method because the loops are easily fused. */
    static ScalarBezCurve AddSquares(const ScalarBezCurve& a, const ScalarBezCurve& b) {
        const int n = a.degree(), m = b.degree();
        const int newDegree = n + m;
        ScalarBezCurve result(newDegree);

        for (int k = 0; k <= newDegree; k++) {
            float aSq = 0, bSq = 0;
            for (int i = std::max(0, k - n); i <= std::min(k, m); i++) {
                const float f =
                        (choose(m, i) * choose(n, k - i)) / static_cast<float>(choose(m + n, k));
                aSq += a.fWeights[i] * a.fWeights[k - i] * f;
                bSq += b.fWeights[i] * b.fWeights[k - i] * f;
            }
            result.fWeights[k] = aSq + bSq;
        }

        return result;
    }

    /** Returns a - b. */
    static ScalarBezCurve Sub(const ScalarBezCurve& a, const ScalarBezCurve& b) {
        ScalarBezCurve result = a;
        result.sub(b);
        return result;
    }

    /** Subtracts the other curve from this curve */
    void sub(const ScalarBezCurve& other) {
        SkASSERT(other.fDegree == fDegree);
        for (int k = 0; k <= fDegree; k++) {
            fWeights[k] -= other.fWeights[k];
        }
    }

    /** Subtracts a constant from this curve */
    void sub(float f) {
        for (int k = 0; k <= fDegree; k++) {
            fWeights[k] -= f;
        }
    }

    /** Returns the curve degree */
    int degree() const { return fDegree; }

    /** Returns the curve weights */
    const std::vector<float>& weights() const { return fWeights; }

    float operator[](size_t i) const { return fWeights[i]; }
    float& operator[](size_t i) { return fWeights[i]; }

private:
    /** Recursive helper for ZeroSet */
    static void ZeroSetRec(const ScalarBezCurve& curve,
                           float tmin,
                           float tmax,
                           float tol,
                           std::vector<float>* result) {
        float lenP = 0;
        bool allPos = curve.fWeights[0] >= 0, allNeg = curve.fWeights[0] < 0;
        for (int i = 1; i <= curve.fDegree; i++) {
            lenP += std::abs(curve.fWeights[i] - curve.fWeights[i - 1]);
            allPos &= curve.fWeights[i] >= 0;
            allNeg &= curve.fWeights[i] < 0;
        }
        if (lenP <= tol) {
            result->push_back((tmin + tmax) * 0.5);
            return;
        } else if (allPos || allNeg) {
            // No zero crossings possible if the coefficients don't change sign (convex hull
            // property)
            return;
        } else if (SkScalarNearlyZero(tmax - tmin)) {
            return;
        } else {
            ScalarBezCurve left(curve.fDegree), right(curve.fDegree);
            Split(curve, 0.5f, &left, &right);

            const float tmid = (tmin + tmax) * 0.5;
            ZeroSetRec(left, tmin, tmid, tol, result);
            ZeroSetRec(right, tmid, tmax, tol, result);
        }
    }

    int fDegree;
    std::vector<float> fWeights;
};

// Several debug-only visualization helpers
namespace viz {
std::unique_ptr<ScalarBezCurve> outerErr;
SkPath outerFirstApprox;
}  // namespace viz

/**
 * Prototype variable-width path stroker.
 *
 * Takes as input a path to be stroked, and two distance functions (inside and outside).
 * Produces a fill path with the stroked path geometry.
 *
 * The algorithms in use here are from:
 *
 * G. Elber, E. Cohen. "Error bounded variable distance offset operator for free form curves and
 * surfaces." International Journal of Computational Geometry & Applications 1, no. 01 (1991)
 *
 * G. Elber. "Free form surface analysis using a hybrid of symbolic and numeric computation."
 * PhD diss., Dept. of Computer Science, University of Utah, 1992.
 */
class SkVarWidthStroker {
public:
    /**
     * Strokes the path with a fixed-width distance function. This produces a traditional stroked
     * path.
     */
    SkPath getFillPath(const SkPath& path, const SkPaint& paint) {
        return getFillPath(path, paint, identityVarWidth(paint.getStrokeWidth()),
                           identityVarWidth(paint.getStrokeWidth()));
    }

    /**
     * Strokes the given path using the two given distance functions for inner and outer offsets.
     */
    SkPath getFillPath(const SkPath& path,
                       const SkPaint& paint,
                       const ScalarBezCurve& varWidth,
                       const ScalarBezCurve& varWidthInner);

private:
    /** Helper struct referring to a single segment of an SkPath */
    struct PathSegment {
        SkPath::Verb fVerb;
        std::array<SkPoint, 4> fPoints;
    };

    /** Initialize stroker state */
    void initForPath(const SkPath& path, const SkPaint& paint);

    /** Strokes a line segment */
    void strokeLine(const PathSegment& line, bool needsMove);

    /** Strokes a quadratic segment */
    void strokeQuad(const PathSegment& quad, bool needsMove);

    /**
     * Strokes the given segment using the given distance function.
     *
     * Returns a list of quad segments that approximate the offset curve.
     * TODO: no reason this needs to return a vector of quads, can just append to the path
     */
    std::vector<PathSegment> strokeSegment(const PathSegment& seg,
                                           const ScalarBezCurve& distFnc) const;

    /** Adds an endcap to fOuter */
    enum class CapLocation { Start, End };
    void endcap(CapLocation loc);

    /** Adds a join between the two segments */
    void join(const PathSegment& prev, const PathSegment& curr);

    /** Appends path in reverse to result */
    static void appendPathReversed(const SkPath& path, SkPath* result);

    /** Returns the segment unit normal and unit tangent if not nullptr */
    static SkPoint unitNormal(const PathSegment& seg, float t, SkPoint* tangentOut);

    /** Returns the degree of a segment curve */
    static int segmentDegree(const PathSegment& seg);

    /** Splits a path segment at t */
    static void splitSegment(const PathSegment& seg, float t, PathSegment* segA, PathSegment* segB);

    /**
     * Returns a quadratic segment that approximates the given segment using the given distance
     * function.
     */
    static void approximateSegment(const PathSegment& seg,
                                   const ScalarBezCurve& distFnc,
                                   PathSegment* approxQuad);

    /** Returns a constant (deg 0) distance function for the given stroke width */
    static ScalarBezCurve identityVarWidth(float strokeWidth) {
        return ScalarBezCurve(0, {strokeWidth / 2.0f});
    }

    float fRadius;
    SkPaint::Cap fCap;
    SkPaint::Join fJoin;
    SkPath fInner, fOuter;
    ScalarBezCurve fVarWidth, fVarWidthInner;
    float fCurrT;
};

void SkVarWidthStroker::initForPath(const SkPath& path, const SkPaint& paint) {
    fRadius = paint.getStrokeWidth() / 2;
    fCap = paint.getStrokeCap();
    fJoin = paint.getStrokeJoin();
    fInner.rewind();
    fOuter.rewind();
    fCurrT = 0;
}

SkPath SkVarWidthStroker::getFillPath(const SkPath& path,
                                      const SkPaint& paint,
                                      const ScalarBezCurve& varWidth,
                                      const ScalarBezCurve& varWidthInner) {
    initForPath(path, paint);
    fVarWidth = varWidth;
    fVarWidthInner = varWidthInner;

    // Trace the inner and outer paths simultaneously. Inner will therefore be
    // recorded in reverse from how we trace the outline.
    SkPath::Iter it(path, false);
    PathSegment segment, prevSegment;
    bool firstSegment = true;
    while ((segment.fVerb = it.next(&segment.fPoints[0])) != SkPath::kDone_Verb) {
        // Join to the previous segment
        if (!firstSegment) {
            join(prevSegment, segment);
        }

        // Stroke the current segment
        switch (segment.fVerb) {
            case SkPath::kLine_Verb:
                strokeLine(segment, firstSegment);
                break;
            case SkPath::kQuad_Verb:
                strokeQuad(segment, firstSegment);
                break;
            case SkPath::kMove_Verb:
                // Don't care about multiple contours currently
                continue;
            default:
                SkDebugf("Unhandled path verb %d\n", segment.fVerb);
                SkASSERT(false);
                break;
        }

        std::swap(segment, prevSegment);
        firstSegment = false;
    }

    // Open contour => endcap at the end
    const bool isClosed = path.isLastContourClosed();
    if (isClosed) {
        SkDebugf("Unhandled closed contour\n");
        SkASSERT(false);
    } else {
        endcap(CapLocation::End);
    }

    // Walk inner path in reverse, appending to result
    appendPathReversed(fInner, &fOuter);
    endcap(CapLocation::Start);

    return fOuter;
}

void SkVarWidthStroker::strokeLine(const PathSegment& line, bool needsMove) {
    viz::outerErr.reset(nullptr);

    std::vector<PathSegment> outer = strokeSegment(line, fVarWidth);
    std::vector<PathSegment> inner = strokeSegment(line, ScalarBezCurve::Mul(fVarWidthInner, -1));

    if (needsMove) {
        fOuter.moveTo(outer.front().fPoints[0]);
        fInner.moveTo(inner.front().fPoints[0]);
    }

    for (const PathSegment& seg : outer) {
        fOuter.quadTo(seg.fPoints[1], seg.fPoints[2]);
    }

    for (const PathSegment& seg : inner) {
        fInner.quadTo(seg.fPoints[1], seg.fPoints[2]);
    }
}

void SkVarWidthStroker::strokeQuad(const PathSegment& quad, bool needsMove) {
    viz::outerErr.reset(nullptr);

    std::vector<PathSegment> outer = strokeSegment(quad, fVarWidth);
    std::vector<PathSegment> inner = strokeSegment(quad, ScalarBezCurve::Mul(fVarWidthInner, -1));

    if (needsMove) {
        fOuter.moveTo(outer.front().fPoints[0]);
        fInner.moveTo(inner.front().fPoints[0]);
    }

    for (const PathSegment& seg : outer) {
        fOuter.quadTo(seg.fPoints[1], seg.fPoints[2]);
    }

    for (const PathSegment& seg : inner) {
        fInner.quadTo(seg.fPoints[1], seg.fPoints[2]);
    }
}

std::vector<SkVarWidthStroker::PathSegment> SkVarWidthStroker::strokeSegment(
        const PathSegment& seg, const ScalarBezCurve& distFnc) const {
    // Work item for the recursive splitting stack.
    struct Item {
        PathSegment fSeg;
        ScalarBezCurve fDistFnc, fDistFncSqd;
        ScalarBezCurve fSegX, fSegY;

        Item(const PathSegment& seg,
             const ScalarBezCurve& distFnc,
             const ScalarBezCurve& distFncSqd)
                : fSeg(seg), fDistFnc(distFnc), fDistFncSqd(distFncSqd) {
            const int segDegree = segmentDegree(seg);
            fSegX = ScalarBezCurve(segDegree);
            fSegY = ScalarBezCurve(segDegree);
            for (int i = 0; i <= segDegree; i++) {
                fSegX[i] = seg.fPoints[i].fX;
                fSegY[i] = seg.fPoints[i].fY;
            }
        }
    };

    // Push the initial segment and distance function
    std::stack<Item> stack;
    stack.push(Item(seg, distFnc, ScalarBezCurve::Mul(distFnc, distFnc)));

    std::vector<PathSegment> result;
    constexpr int kMaxIters = 5000; /** TODO: this is completely arbitrary */
    int iter = 0;
    while (!stack.empty()) {
        if (iter++ >= kMaxIters) break;
        const Item item = stack.top();
        stack.pop();

        const ScalarBezCurve& distFnc = item.fDistFnc;
        ScalarBezCurve distFncSqd = item.fDistFncSqd;
        const float kTol = std::abs(0.5f * distFnc.extremumWeight());

        // Compute a quad that approximates stroke outline
        PathSegment quadApprox;
        approximateSegment(item.fSeg, distFnc, &quadApprox);
        ScalarBezCurve quadApproxX(2), quadApproxY(2);
        for (int i = 0; i < 3; i++) {
            quadApproxX[i] = quadApprox.fPoints[i].fX;
            quadApproxY[i] = quadApprox.fPoints[i].fY;
        }

        // Compute control polygon for the delta(t) curve. First must elevate to a common degree.
        const int deltaDegree = std::max(quadApproxX.degree(), item.fSegX.degree());
        ScalarBezCurve segX = item.fSegX, segY = item.fSegY;
        segX.elevateDegree(deltaDegree);
        segY.elevateDegree(deltaDegree);
        quadApproxX.elevateDegree(deltaDegree);
        quadApproxY.elevateDegree(deltaDegree);

        ScalarBezCurve deltaX = ScalarBezCurve::Sub(quadApproxX, segX);
        ScalarBezCurve deltaY = ScalarBezCurve::Sub(quadApproxY, segY);

        // Compute psi(t) = delta_x(t)^2 + delta_y(t)^2.
        ScalarBezCurve E = ScalarBezCurve::AddSquares(deltaX, deltaY);

        // Promote E and d(t)^2 to a common degree.
        const int commonDeg = std::max(distFncSqd.degree(), E.degree());
        distFncSqd.elevateDegree(commonDeg);
        E.elevateDegree(commonDeg);

        // Subtract dist squared curve from E, resulting in:
        //   eps(t) = delta_x(t)^2 + delta_y(t)^2 - d(t)^2
        E.sub(distFncSqd);

        // Purely for debugging/testing, save the first approximation and error function:
        if (viz::outerErr == nullptr) {
            using namespace viz;
            outerErr = std::make_unique<ScalarBezCurve>(E);
            outerFirstApprox.rewind();
            outerFirstApprox.moveTo(quadApprox.fPoints[0]);
            outerFirstApprox.quadTo(quadApprox.fPoints[1], quadApprox.fPoints[2]);
        }

        // Compute maxErr, which is just the max coefficient of eps (using convex hull property
        // of bez curves)
        float maxAbsErr = std::abs(E.extremumWeight());

        if (maxAbsErr > kTol) {
            PathSegment left, right;
            splitSegment(item.fSeg, 0.5f, &left, &right);

            ScalarBezCurve distFncL, distFncR;
            distFnc.split(0.5f, &distFncL, &distFncR);

            ScalarBezCurve distFncSqdL, distFncSqdR;
            distFncSqd.split(0.5f, &distFncSqdL, &distFncSqdR);

            stack.push(Item(right, distFncR, distFncSqdR));
            stack.push(Item(left, distFncL, distFncSqdL));
        } else {
            // Approximation is good enough.
            result.push_back(quadApprox);
        }
    }
    SkASSERT(!result.empty());
    return result;
}

void SkVarWidthStroker::endcap(CapLocation loc) {
    const auto buttCap = [this](CapLocation loc) {
        if (loc == CapLocation::Start) {
            // Back at the start of the path: just close the stroked outline
            fOuter.close();
        } else {
            // Inner last pt == first pt when appending in reverse
            SkPoint innerLastPt;
            fInner.getLastPt(&innerLastPt);
            fOuter.lineTo(innerLastPt);
        }
    };

    switch (fCap) {
        case SkPaint::kButt_Cap:
            buttCap(loc);
            break;
        default:
            SkDebugf("Unhandled endcap %d\n", fCap);
            buttCap(loc);
            break;
    }
}

void SkVarWidthStroker::join(const PathSegment& prev, const PathSegment& curr) {
    const auto miterJoin = [this](const PathSegment& prev, const PathSegment& curr) {
        // Common path endpoint of the two segments is the midpoint of the miter line.
        const SkPoint miterMidpt = curr.fPoints[0];

        SkPoint before = unitNormal(prev, 1, nullptr);
        SkPoint after = unitNormal(curr, 0, nullptr);

        // Check who's inside and who's outside.
        SkPath *outer = &fOuter, *inner = &fInner;
        if (!isClockwise(before, after)) {
            std::swap(inner, outer);
            before = rotate180(before);
            after = rotate180(after);
        }

        const float cosTheta = before.dot(after);
        if (SkScalarNearlyZero(1 - cosTheta)) {
            // Nearly identical normals: don't bother.
            return;
        }

        // Before and after have the same origin and magnitude, so before+after is the diagonal of
        // their rhombus. Origin of this vector is the midpoint of the miter line.
        SkPoint miterVec = before + after;

        // Note the relationship (draw a right triangle with the miter line as its hypoteneuse):
        //     sin(theta/2) = strokeWidth / miterLength
        // so miterLength = strokeWidth / sin(theta/2)
        // where miterLength is the length of the miter from outer point to inner corner.
        // miterVec's origin is the midpoint of the miter line, so we use strokeWidth/2.
        // Sqrt is just an application of half-angle identities.
        const float sinHalfTheta = sqrtf(0.5 * (1 + cosTheta));
        const float halfMiterLength = fRadius / sinHalfTheta;
        miterVec.setLength(halfMiterLength);  // TODO: miter length limit

        // Outer: connect to the miter point, and then to t=0 (on outside stroke) of next segment.
        const SkPoint dest = setLength(after, fRadius);
        outer->lineTo(miterMidpt + miterVec);
        outer->lineTo(miterMidpt + dest);

        // Connect to the miter midpoint (common path endpoint of the two segments),
        // and then to t=0 (on inside) of the next segment. This adds an interior "loop" of
        // geometry that handles edge cases where segment lengths are shorter than the
        // stroke width.
        inner->lineTo(miterMidpt);
        inner->lineTo(miterMidpt - dest);
    };

    switch (fJoin) {
        case SkPaint::kMiter_Join:
            miterJoin(prev, curr);
            break;
        default:
            SkDebugf("Unhandled join %d\n", fJoin);
            miterJoin(prev, curr);
            break;
    }
}

void SkVarWidthStroker::appendPathReversed(const SkPath& path, SkPath* result) {
    const int numVerbs = path.countVerbs();
    const int numPoints = path.countPoints();
    std::vector<uint8_t> verbs;
    std::vector<SkPoint> points;
    verbs.resize(numVerbs);
    points.resize(numPoints);
    path.getVerbs(verbs.data(), numVerbs);
    path.getPoints(points.data(), numPoints);

    for (int i = numVerbs - 1, j = numPoints; i >= 0; i--) {
        auto verb = static_cast<SkPath::Verb>(verbs[i]);
        switch (verb) {
            case SkPath::kLine_Verb: {
                j -= 1;
                SkASSERT(j >= 1);
                result->lineTo(points[j - 1]);
                break;
            }
            case SkPath::kQuad_Verb: {
                j -= 1;
                SkASSERT(j >= 2);
                result->quadTo(points[j - 1], points[j - 2]);
                j -= 1;
                break;
            }
            case SkPath::kMove_Verb:
                // Ignore
                break;
            default:
                SkASSERT(false);
                break;
        }
    }
}

int SkVarWidthStroker::segmentDegree(const PathSegment& seg) {
    static constexpr int lut[] = {
            -1,  // move,
            1,   // line
            2,   // quad
            -1,  // conic
            3,   // cubic
            -1   // done
    };
    const int deg = lut[static_cast<uint8_t>(seg.fVerb)];
    SkASSERT(deg > 0);
    return deg;
}

void SkVarWidthStroker::splitSegment(const PathSegment& seg,
                                     float t,
                                     PathSegment* segA,
                                     PathSegment* segB) {
    // TODO: although general, this is a pretty slow way to do this
    const int degree = segmentDegree(seg);
    ScalarBezCurve x(degree), y(degree);
    for (int i = 0; i <= degree; i++) {
        x[i] = seg.fPoints[i].fX;
        y[i] = seg.fPoints[i].fY;
    }

    ScalarBezCurve leftX(degree), rightX(degree), leftY(degree), rightY(degree);
    x.split(t, &leftX, &rightX);
    y.split(t, &leftY, &rightY);

    segA->fVerb = segB->fVerb = seg.fVerb;
    for (int i = 0; i <= degree; i++) {
        segA->fPoints[i] = {leftX[i], leftY[i]};
        segB->fPoints[i] = {rightX[i], rightY[i]};
    }
}

void SkVarWidthStroker::approximateSegment(const PathSegment& seg,
                                           const ScalarBezCurve& distFnc,
                                           PathSegment* approxQuad) {
    // This is a simple control polygon transformation.
    // From F. Yzerman. "Precise offsetting of quadratic Bezier curves". 2019.
    // TODO: detect and handle more degenerate cases (e.g. linear)
    // TODO: Tiller-Hanson works better in many cases but does not generalize well
    SkPoint tangentStart, tangentEnd;
    SkPoint offsetStart = unitNormal(seg, 0, &tangentStart);
    SkPoint offsetEnd = unitNormal(seg, 1, &tangentEnd);
    SkPoint offsetMid = offsetStart + offsetEnd;

    float radiusStart = distFnc.eval(0);
    float radiusMid = distFnc.eval(0.5f);
    float radiusEnd = distFnc.eval(1);

    offsetStart.setLength(radiusStart);
    offsetMid.setLength(radiusMid);
    offsetEnd.setLength(radiusEnd);

    SkPoint start, mid, end;
    switch (segmentDegree(seg)) {
        case 1:
            start = seg.fPoints[0];
            end = seg.fPoints[1];
            mid = (start + end) * 0.5f;
            break;
        case 2:
            start = seg.fPoints[0];
            mid = seg.fPoints[1];
            end = seg.fPoints[2];
            break;
        case 3:
            start = seg.fPoints[0];
            mid = (seg.fPoints[1] + seg.fPoints[2]) * 0.5f;
            end = seg.fPoints[3];
            break;
        default:
            SkDebugf("Unhandled degree for segment approximation");
            SkASSERT(false);
            break;
    }

    approxQuad->fPoints[0] = start + offsetStart;
    approxQuad->fPoints[1] = mid + offsetMid;
    approxQuad->fPoints[2] = end + offsetEnd;
}

SkPoint SkVarWidthStroker::unitNormal(const PathSegment& seg, float t, SkPoint* tangentOut) {
    switch (seg.fVerb) {
        case SkPath::kLine_Verb: {
            const SkPoint tangent = setLength(seg.fPoints[1] - seg.fPoints[0], 1);
            const SkPoint normal = rotate90(tangent);
            if (tangentOut) {
                *tangentOut = tangent;
            }
            return normal;
        }
        case SkPath::kQuad_Verb: {
            SkPoint tangent;
            if (t == 0) {
                tangent = seg.fPoints[1] - seg.fPoints[0];
            } else if (t == 1) {
                tangent = seg.fPoints[2] - seg.fPoints[1];
            } else {
                tangent = ((seg.fPoints[1] - seg.fPoints[0]) * (1 - t) +
                           (seg.fPoints[2] - seg.fPoints[1]) * t) *
                          2;
            }
            tangent.normalize();
            if (tangentOut) {
                *tangentOut = tangent;
            }
            return rotate90(tangent);
        }
        default:
            SkDebugf("Unhandled verb for unit normal %d\n", seg.fVerb);
            SkASSERT(false);
            return {};
    }
}

}  // namespace

//////////////////////////////////////////////////////////////////////////////

class VariableWidthStroker : public Sample {
public:
    VariableWidthStroker()
            : fShowHidden(true)
            , fShowSkeleton(true)
            , fShowStrokePoints(false)
            , fShowUI(false)
            , fDifferentInnerFunc(false)
            , fShowErrorCurve(false) {
        resetToDefaults();

        fPtsPaint.setAntiAlias(true);
        fPtsPaint.setStrokeWidth(10);
        fPtsPaint.setStrokeCap(SkPaint::kRound_Cap);

        fStrokePointsPaint.setAntiAlias(true);
        fStrokePointsPaint.setStrokeWidth(5);
        fStrokePointsPaint.setStrokeCap(SkPaint::kRound_Cap);

        fStrokePaint.setAntiAlias(true);
        fStrokePaint.setStyle(SkPaint::kStroke_Style);
        fStrokePaint.setColor(0x80FF0000);

        fNewFillPaint.setAntiAlias(true);
        fNewFillPaint.setColor(0x8000FF00);

        fHiddenPaint.setAntiAlias(true);
        fHiddenPaint.setStyle(SkPaint::kStroke_Style);
        fHiddenPaint.setColor(0xFF0000FF);

        fSkeletonPaint.setAntiAlias(true);
        fSkeletonPaint.setStyle(SkPaint::kStroke_Style);
        fSkeletonPaint.setColor(SK_ColorRED);
    }

private:
    /** Selectable menu item for choosing distance functions */
    struct DistFncMenuItem {
        std::string fName;
        int fDegree;
        bool fSelected;
        std::vector<float> fWeights;

        DistFncMenuItem(const std::string& name, int degree, bool selected) {
            fName = name;
            fDegree = degree;
            fSelected = selected;
            fWeights.resize(degree + 1, 1.0f);
        }
    };

    SkString name() override { return SkString("VariableWidthStroker"); }

    void onSizeChange() override {
        fWinSize = SkSize::Make(this->width(), this->height());
        INHERITED::onSizeChange();
    }

    bool onChar(SkUnichar uni) override {
        switch (uni) {
            case '0':
                this->toggle(fShowUI);
                return true;
            case '1':
                this->toggle(fShowSkeleton);
                return true;
            case '2':
                this->toggle(fShowHidden);
                return true;
            case '3':
                this->toggle(fShowStrokePoints);
                return true;
            case '4':
                this->toggle(fShowErrorCurve);
                return true;
            case 'x':
                resetToDefaults();
                return true;
            case '-':
                fWidth -= 5;
                return true;
            case '=':
                fWidth += 5;
                return true;
            default:
                break;
        }
        return false;
    }

    void toggle(bool& value) { value = !value; }

    void resetToDefaults() {
        fPathPts[0] = {300, 400};
        fPathPts[1] = {500, 400};
        fPathPts[2] = {700, 400};

        fWidth = 175;

        fDistFncs = fDefaultsDistFncs;
        fDistFncsInner = fDefaultsDistFncs;
    }

    void makePath(SkPath* path) {
        path->moveTo(fPathPts[0]);
        path->quadTo(fPathPts[1], fPathPts[2]);
    }

    static ScalarBezCurve makeDistFnc(const std::vector<DistFncMenuItem>& fncs, float strokeWidth) {
        const float radius = strokeWidth / 2;
        for (const auto& df : fncs) {
            if (df.fSelected) {
                return ScalarBezCurve::Mul(ScalarBezCurve(df.fDegree, df.fWeights), radius);
            }
        }
        SkASSERT(false);
        return ScalarBezCurve(0, {radius});
    }

    void onDrawContent(SkCanvas* canvas) override {
        canvas->drawColor(0xFFEEEEEE);

        SkPath path;
        this->makePath(&path);

        fStrokePaint.setStrokeWidth(fWidth);

        // Elber-Cohen stroker result
        ScalarBezCurve distFnc = makeDistFnc(fDistFncs, fWidth);
        ScalarBezCurve distFncInner =
                fDifferentInnerFunc ? makeDistFnc(fDistFncsInner, fWidth) : distFnc;
        SkVarWidthStroker stroker;
        SkPath fillPath = stroker.getFillPath(path, fStrokePaint, distFnc, distFncInner);
        fillPath.setFillType(SkPathFillType::kWinding);
        canvas->drawPath(fillPath, fNewFillPaint);

        if (fShowHidden) {
            canvas->drawPath(fillPath, fHiddenPaint);
        }

        if (fShowSkeleton) {
            canvas->drawPath(path, fSkeletonPaint);
            canvas->drawPoints(SkCanvas::kPoints_PointMode, fPathPts.size(), fPathPts.data(),
                               fPtsPaint);
        }

        if (fShowStrokePoints) {
            drawStrokePoints(canvas, fillPath);
        }

        if (fShowUI) {
            drawUI();
        }

        if (fShowErrorCurve && viz::outerErr != nullptr) {
            SkPaint firstApproxPaint;
            firstApproxPaint.setStrokeWidth(4);
            firstApproxPaint.setStyle(SkPaint::kStroke_Style);
            firstApproxPaint.setColor(SK_ColorRED);
            canvas->drawPath(viz::outerFirstApprox, firstApproxPaint);
            drawErrorCurve(canvas, *viz::outerErr);
        }
    }

    Sample::Click* onFindClickHandler(SkScalar x, SkScalar y, skui::ModifierKey modi) override {
        const SkScalar tol = 4;
        const SkRect r = SkRect::MakeXYWH(x - tol, y - tol, tol * 2, tol * 2);
        for (size_t i = 0; i < fPathPts.size(); ++i) {
            if (r.intersects(SkRect::MakeXYWH(fPathPts[i].fX, fPathPts[i].fY, 1, 1))) {
                return new Click([this, i](Click* c) {
                    fPathPts[i] = c->fCurr;
                    return true;
                });
            }
        }
        return nullptr;
    }

    void drawStrokePoints(SkCanvas* canvas, const SkPath& fillPath) {
        SkPath::Iter it(fillPath, false);
        SkPoint points[4];
        SkPath::Verb verb;
        std::vector<SkPoint> pointsVec, ctrlPts;
        while ((verb = it.next(&points[0])) != SkPath::kDone_Verb) {
            switch (verb) {
                case SkPath::kLine_Verb:
                    pointsVec.push_back(points[1]);
                    break;
                case SkPath::kQuad_Verb:
                    ctrlPts.push_back(points[1]);
                    pointsVec.push_back(points[2]);
                    break;
                case SkPath::kMove_Verb:
                    pointsVec.push_back(points[0]);
                    break;
                case SkPath::kClose_Verb:
                    break;
                default:
                    SkDebugf("Unhandled path verb %d for stroke points\n", verb);
                    SkASSERT(false);
                    break;
            }
        }

        canvas->drawPoints(SkCanvas::kPoints_PointMode, pointsVec.size(), pointsVec.data(),
                           fStrokePointsPaint);
        fStrokePointsPaint.setColor(SK_ColorBLUE);
        fStrokePointsPaint.setStrokeWidth(3);
        canvas->drawPoints(SkCanvas::kPoints_PointMode, ctrlPts.size(), ctrlPts.data(),
                           fStrokePointsPaint);
        fStrokePointsPaint.setColor(SK_ColorBLACK);
        fStrokePointsPaint.setStrokeWidth(5);
    }

    void drawErrorCurve(SkCanvas* canvas, const ScalarBezCurve& E) {
        const float winW = fWinSize.width() * 0.75f, winH = fWinSize.height() * 0.25f;
        const float padding = 25;
        const SkRect box = SkRect::MakeXYWH(padding, fWinSize.height() - winH - padding,
                                            winW - 2 * padding, winH);
        constexpr int nsegs = 100;
        constexpr float dt = 1.0f / nsegs;
        constexpr float dx = 10.0f;
        const int deg = E.degree();
        SkPath path;
        for (int i = 0; i < nsegs; i++) {
            const float tmin = i * dt, tmax = (i + 1) * dt;
            ScalarBezCurve left(deg), right(deg);
            E.split(tmax, &left, &right);
            const float tRel = tmin / tmax;
            ScalarBezCurve rl(deg), rr(deg);
            left.split(tRel, &rl, &rr);

            const float x = i * dx;
            if (i == 0) {
                path.moveTo(x, -rr[0]);
            }
            path.lineTo(x + dx, -rr[deg]);
        }

        SkPaint paint;
        paint.setStyle(SkPaint::kStroke_Style);
        paint.setAntiAlias(true);
        paint.setStrokeWidth(0);
        paint.setColor(SK_ColorRED);
        const SkRect pathBounds = path.computeTightBounds();
        constexpr float yAxisMax = 8000;
        const float sx = box.width() / pathBounds.width();
        const float sy = box.height() / (2 * yAxisMax);
        canvas->save();
        canvas->translate(box.left(), box.top() + box.height() / 2);
        canvas->scale(sx, sy);
        canvas->drawPath(path, paint);

        SkPath axes;
        axes.moveTo(0, 0);
        axes.lineTo(pathBounds.width(), 0);
        axes.moveTo(0, -yAxisMax);
        axes.lineTo(0, yAxisMax);
        paint.setColor(SK_ColorBLACK);
        paint.setAntiAlias(false);
        canvas->drawPath(axes, paint);

        canvas->restore();
    }

    void drawUI() {
        static constexpr auto kUIOpacity = 0.35f;
        static constexpr float kUIWidth = 200.0f, kUIHeight = 400.0f;
        ImGui::SetNextWindowBgAlpha(kUIOpacity);
        if (ImGui::Begin("E-C Controls", nullptr,
                         ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoResize |
                                 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
                                 ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav)) {
            const SkRect uiArea = SkRect::MakeXYWH(10, 10, kUIWidth, kUIHeight);
            ImGui::SetWindowPos(ImVec2(uiArea.x(), uiArea.y()));
            ImGui::SetWindowSize(ImVec2(uiArea.width(), uiArea.height()));

            const auto drawControls = [](std::vector<DistFncMenuItem>& distFncs,
                                         const std::string& menuPfx,
                                         const std::string& ptPfx) {
                std::string degreeMenuLabel = menuPfx + ": ";
                for (const auto& df : distFncs) {
                    if (df.fSelected) {
                        degreeMenuLabel += df.fName;
                        break;
                    }
                }
                if (ImGui::BeginMenu(degreeMenuLabel.c_str())) {
                    for (size_t i = 0; i < distFncs.size(); i++) {
                        if (ImGui::MenuItem(distFncs[i].fName.c_str(), nullptr,
                                            distFncs[i].fSelected)) {
                            for (size_t j = 0; j < distFncs.size(); j++) {
                                distFncs[j].fSelected = j == i;
                            }
                        }
                    }
                    ImGui::EndMenu();
                }

                for (auto& df : distFncs) {
                    if (df.fSelected) {
                        for (int i = 0; i <= df.fDegree; i++) {
                            const std::string label = ptPfx + std::to_string(i);
                            ImGui::SliderFloat(label.c_str(), &(df.fWeights[i]), 0, 1);
                        }
                    }
                }
            };

            drawControls(fDistFncs, "Degree", "P");

            if (ImGui::CollapsingHeader("Demo part 2", true)) {
                fDifferentInnerFunc = true;
                drawControls(fDistFncsInner, "Degree (inner)", "Q");
            } else {
                fDifferentInnerFunc = false;
            }
        }
        ImGui::End();
    }

    bool fShowHidden, fShowSkeleton, fShowStrokePoints, fShowUI, fDifferentInnerFunc,
            fShowErrorCurve;
    float fWidth = 175;
    SkPaint fPtsPaint, fStrokePaint, fNewFillPaint, fHiddenPaint, fSkeletonPaint,
            fStrokePointsPaint;
    static constexpr int kNPts = 3;
    std::array<SkPoint, kNPts> fPathPts;
    SkSize fWinSize;
    const std::vector<DistFncMenuItem> fDefaultsDistFncs = {
            DistFncMenuItem("Linear", 1, true), DistFncMenuItem("Quadratic", 2, false),
            DistFncMenuItem("Cubic", 3, false), DistFncMenuItem("One Louder (11)", 11, false),
            DistFncMenuItem("30?!", 30, false)};
    std::vector<DistFncMenuItem> fDistFncs = fDefaultsDistFncs;
    std::vector<DistFncMenuItem> fDistFncsInner = fDefaultsDistFncs;

    using INHERITED = Sample;
};

DEF_SAMPLE(return new VariableWidthStroker;)