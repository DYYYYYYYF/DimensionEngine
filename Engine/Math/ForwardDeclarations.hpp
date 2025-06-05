#pragma once

// Math forward declarations
template<typename T> struct DAPI TVector2;
template<typename T> struct DAPI TVector3;
template<typename T> struct DAPI TVector4;
template<typename T> struct DAPI TQuaternion;
template<typename T> struct DAPI TMatrix4;
template<typename T> struct DAPI TExtents2D;
template<typename T> struct DAPI TExtents3D;
template<typename T> struct DAPI TVertex3;
template<typename T> struct DAPI TVertex2;

using Vector2		=	TVector2<float>;
using Vector2f		=	TVector2<float>;
using Vector		=	TVector3<float>;
using Vector3		=	TVector3<float>;
using Vector3f		=	TVector3<float>;
using Vector4		=	TVector4<float>;
using Vector4f		=	TVector4<float>;
using Quaternion	=	TQuaternion<float>;
using Matrix4		=	TMatrix4<float>;
using Extents2D		=	TExtents2D<float>;
using Extents3D		=	TExtents3D<float>;
using Vertex		=	TVertex3<float>;
using Vertex2D		=	TVertex2<float>;

using Vector2d = TVector2<double>;
using Vector3d = TVector3<double>;
using Vector4d = TVector4<double>;
using Quaterniond = TQuaternion<double>;
using Matrix4d = TMatrix4<double>;
using Vertexd = TVertex3<double>;