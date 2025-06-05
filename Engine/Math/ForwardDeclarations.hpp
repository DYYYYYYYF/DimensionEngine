#pragma once

// Math forward declarations
template<typename T> struct DAPI TVector2_Base;
template<typename T> struct DAPI TVector3_Base;
template<typename T> struct DAPI TVector3_16;
template<typename T> struct DAPI TVector4_Base;
template<typename T> struct DAPI TVector4_SIMD;
template<typename T> struct DAPI TQuaternion;
template<typename T> struct DAPI TMatrix4;
template<typename T> struct DAPI TExtents2D;
template<typename T> struct DAPI TExtents3D;
template<typename T> struct DAPI TVertex3;
template<typename T> struct DAPI TVertex2;

template<typename T> using TVector2 = TVector2_Base<T>;
template<typename T> using TVector3 = TVector3_Base<T>;
template<typename T> using TVector4 = TVector4_Base<T>;
template<typename T> using TVector4d = TVector4_SIMD<T>;

using Vector2		=	TVector2_Base<float>;
using Vector2f		=	TVector2_Base<float>;
using Vector		=	TVector3_Base<float>;
using Vector3		=	TVector3_Base<float>;
using Vector3f		=	TVector3_Base<float>;
using Vector4		=	TVector4_Base<float>;
using Vector4f		=	TVector4_Base<float>;
using Quaternion	=	TQuaternion<float>;
using Matrix4		=	TMatrix4<float>;
using Extents2D		=	TExtents2D<float>;
using Extents3D		=	TExtents3D<float>;
using Vertex		=	TVertex3<float>;
using Vertex2D		=	TVertex2<float>;

using Vector2d = TVector2_Base<double>;
using Vector3d = TVector3_Base<double>;
using Vector4d = TVector4_SIMD<double>;
using Quaterniond = TQuaternion<double>;
using Matrix4d = TMatrix4<double>;
using Vertexd = TVertex3<double>;