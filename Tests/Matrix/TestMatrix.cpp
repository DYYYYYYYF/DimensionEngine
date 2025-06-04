#include <Math/MathTypes.hpp>

#include <iostream>
using namespace std;

void TestMatrix() {

	GLOG(Log::eInfo, "Vector2<float> size: %ld", sizeof(TVector2<float>));
	GLOG(Log::eInfo, "Vector3<float> size: %ld", sizeof(TVector3<float>));
	GLOG(Log::eInfo, "Vector4<float> size: %ld", sizeof(TVector4<float>));
	GLOG(Log::eInfo, "TMatrix4<float> size: %ld", sizeof(TMatrix4<float>));

	GLOG(Log::eInfo, "Vector2<double> size: %ld", sizeof(TVector2<double>));
	GLOG(Log::eInfo, "Vector3<double> size: %ld", sizeof(TVector3<double>));
	GLOG(Log::eInfo, "Vector4<double> size: %ld", sizeof(TVector4<double>));
	GLOG(Log::eInfo, "TMatrix4<double> size: %ld", sizeof(TMatrix4<double>));

	Matrix4 Translation = {
		1, 0, 0, 1,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1
	};

	Quaternion QRatation(Vector(0.0f, 90.0f, 0.0f));
	Matrix4 Rotation = QRatation.ToRotationMatrix();
	Matrix4 Rotation1 = Matrix4::EulerXYZ(0.0f, Deg2Rad(90.0f), 0.0f);
	Quaternion QRotation1 = MatrixToQuat(Rotation1);

	Matrix4 Scale = {
		1, 0, 0, 0,
		0, 2, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1
	};

	cout << "Quaternion: " << QRatation << endl;
	cout << "Quaternion1: " << QRotation1 << endl;
	cout << "Rotation: \n" << Rotation << endl;
	cout << "Rotation1: \n" << Rotation1 << endl;

	Matrix4 Transform = Translation.Multiply(Rotation.Multiply(Scale));
	cout << "Multiply: \n" << Transform << endl;

	Vector Vec1(-1, 1, 0);
	Vector4 Vec4(0, 1, 0, 1);
	cout << "Vector3 right multiply Matrix: \n" << Vec1 * Transform;
	cout << "Matrix left multiply Vector3: \n" << Transform * Vec1;

	cout << "Vector4 right multiply Matrix: \n" << Vec4 * Transform;
	cout << "Matrix left multiply Vector4: \n" << Transform * Vec4;

	Vector4 Vec4_2(1, 0, 0, 1);
	cout << "Vector4 add: " << Vec4 + Vec4_2;
	cout << "Vector4 Sub: " << Vec4 - Vec4_2;
	cout << "Vector4 Mul: " << Vec4 * Vec4_2;
	cout << "Vector4 Div: " << Vec4 / Vec4_2;

	cout << "Vector4 length: " << Vec4.Length() << endl;
	cout << "Vector4 normalized: " << Vec4.Normalize() << endl;
	cout << "Vector4 length: " << Vec4.Length() << endl;

	Matrix4 Mat3 = {
		1, 0, 0, 2,
		0, 1, 0, 2,
		0, 0, 1, 2,
		0, 0, 0, 1
	};

	Matrix4 Mat4 = {
		1, 0, 0,  0,
		0, 1, 0, -2,
		0, 0, 1,  0,
		0, 0, 0,  1
	};

	cout << "Matrix Multiply Matrix:\n" << Mat3.Multiply(Mat4) << endl;
}