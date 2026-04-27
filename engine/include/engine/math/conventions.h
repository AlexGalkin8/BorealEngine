#pragma once

namespace math::conventions {

enum class MatrixLayout
{
    ColumnMajor
};

enum class VectorConvention
{
    ColumnVector
};

enum class Handedness
{
    LeftHanded,
    RightHanded
};

// D3D12/HLSL-oriented defaults
inline constexpr MatrixLayout kMatrixLayout = MatrixLayout::ColumnMajor;
inline constexpr VectorConvention kVectorConvention = VectorConvention::ColumnVector;
inline constexpr Handedness kHandedness = Handedness::LeftHanded;

} // namespace math::conventions
