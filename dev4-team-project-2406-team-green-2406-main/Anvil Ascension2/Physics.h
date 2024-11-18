#pragma once

#include "gateware-main/Gateware.h"

namespace MathHelpers {
    inline GW::MATH::GVECTORF Cross(const GW::MATH::GVECTORF& a, const GW::MATH::GVECTORF& b) {
        GW::MATH::GVECTORF result;
        result.x = a.y * b.z - a.z * b.y;
        result.y = a.z * b.x - a.x * b.z;
        result.z = a.x * b.y - a.y * b.x;
        result.w = 0.0f; // Ensure w is set to 0 for vectors
        return result;
    }

    inline float Dot(const GW::MATH::GVECTORF& a, const GW::MATH::GVECTORF& b) {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }

    inline GW::MATH::GVECTORF Scale(const GW::MATH::GVECTORF& v, float scale) {
        GW::MATH::GVECTORF result;
        result.x = v.x * scale;
        result.y = v.y * scale;
        result.z = v.z * scale;
        result.w = v.w * scale; // Ensure w is scaled as well
        return result;
    }

    inline GW::MATH::GVECTORF Add(const GW::MATH::GVECTORF& a, const GW::MATH::GVECTORF& b) {
        GW::MATH::GVECTORF result;
        result.x = a.x + b.x;
        result.y = a.y + b.y;
        result.z = a.z + b.z;
        result.w = a.w + b.w; // Ensure w is added as well
        return result;
    }

    inline void AddInPlace(GW::MATH::GVECTORF& a, const GW::MATH::GVECTORF& b) {
        a.x += b.x;
        a.y += b.y;
        a.z += b.z;
        a.w += b.w; // Ensure w is added in place
    }

    inline GW::MATH::GVECTORF Negate(const GW::MATH::GVECTORF& v) {
        GW::MATH::GVECTORF result;
        result.x = -v.x;
        result.y = -v.y;
        result.z = -v.z;
        result.w = -v.w; // Ensure w is negated as well
        return result;
    }
}