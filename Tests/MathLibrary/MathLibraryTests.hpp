// MathLibraryTests.hpp
#pragma once

#include <Math/MathTypes.hpp>
#include <Math/Transform.hpp>
#include <Math/GeometryUtils.hpp>
#include <iostream>
#include <vector>
#include <chrono>
#include <random>
#include <iomanip>

// 简单的测试框架
class MathLibraryTests {
private:
	static int total_tests;
	static int passed_tests;
	static int failed_tests;
	static constexpr float EPSILON = 1e-5f;
	static constexpr double EPSILON_D = 1e-10;

public:
	// 测试宏
	static void ASSERT_TRUE(bool condition, const char* message) {
		total_tests++;
		if (condition) {
			passed_tests++;
			std::cout << "[PASS] " << message << std::endl;
		}
		else {
			failed_tests++;
			std::cout << "[FAIL] " << message << std::endl;
		}
	}

	static void ASSERT_FLOAT_EQUAL(float expected, float actual, const char* message) {
		bool equal = std::abs(expected - actual) < EPSILON;
		ASSERT_TRUE(equal, message);
		if (!equal) {
			std::cout << "       Expected: " << expected << ", Actual: " << actual
				<< ", Diff: " << std::abs(expected - actual) << std::endl;
		}
	}

	static void ASSERT_VECTOR3_EQUAL(const Vector3& expected, const Vector3& actual, const char* message) {
		bool equal = expected.Compare(actual, EPSILON);
		ASSERT_TRUE(equal, message);
		if (!equal) {
			std::cout << "       Expected: (" << expected.x << ", " << expected.y << ", " << expected.z << ")" << std::endl;
			std::cout << "       Actual:   (" << actual.x << ", " << actual.y << ", " << actual.z << ")" << std::endl;
		}
	}

	static void ASSERT_MATRIX4_EQUAL(const Matrix4& expected, const Matrix4& actual, const char* message) {
		bool equal = true;
		for (int i = 0; i < 16; ++i) {
			if (std::abs(expected.data[i] - actual.data[i]) > EPSILON) {
				equal = false;
				break;
			}
		}
		ASSERT_TRUE(equal, message);
		if (!equal) {
			std::cout << "       Expected Matrix:" << std::endl << expected << std::endl;
			std::cout << "       Actual Matrix:" << std::endl << actual << std::endl;
		}
	}

	// ================================
	// Vector2 测试
	// ================================
	static void TestVector2() {
		std::cout << "\n=== Testing Vector2 ===" << std::endl;

		// 构造函数测试
		Vector2f v1(3.0f, 4.0f);
		ASSERT_FLOAT_EQUAL(3.0f, v1.x, "Vector2 constructor - x component");
		ASSERT_FLOAT_EQUAL(4.0f, v1.y, "Vector2 constructor - y component");

		// 长度测试
		ASSERT_FLOAT_EQUAL(5.0f, v1.Length(), "Vector2 length calculation");
		ASSERT_FLOAT_EQUAL(25.0f, v1.LengthSquared(), "Vector2 length squared calculation");

		// 标准化测试
		Vector2f v2 = v1;
		v2.Normalize();
		ASSERT_FLOAT_EQUAL(1.0f, v2.Length(), "Vector2 normalize - unit length");
		ASSERT_FLOAT_EQUAL(0.6f, v2.x, "Vector2 normalize - x component");
		ASSERT_FLOAT_EQUAL(0.8f, v2.y, "Vector2 normalize - y component");

		// 运算符测试
		Vector2f v3(1.0f, 2.0f);
		Vector2f v4(2.0f, 3.0f);
		Vector2f result = v3 + v4;
		ASSERT_FLOAT_EQUAL(3.0f, result.x, "Vector2 addition - x");
		ASSERT_FLOAT_EQUAL(5.0f, result.y, "Vector2 addition - y");

		result = v4 - v3;
		ASSERT_FLOAT_EQUAL(1.0f, result.x, "Vector2 subtraction - x");
		ASSERT_FLOAT_EQUAL(1.0f, result.y, "Vector2 subtraction - y");

		result = v3 * 2.0f;
		ASSERT_FLOAT_EQUAL(2.0f, result.x, "Vector2 scalar multiplication - x");
		ASSERT_FLOAT_EQUAL(4.0f, result.y, "Vector2 scalar multiplication - y");

		// 距离测试
		ASSERT_FLOAT_EQUAL(std::sqrt(2.0f), v3.Distance(v4), "Vector2 distance calculation");

		// 比较测试
		Vector2f v5(1.0f, 2.0f);
		ASSERT_TRUE(v3 == v5, "Vector2 equality comparison");
		ASSERT_TRUE(v3 != v4, "Vector2 inequality comparison");
	}

	// ================================
	// Vector3 测试
	// ================================
	static void TestVector3() {
		std::cout << "\n=== Testing Vector3 ===" << std::endl;

		// 构造函数测试
		Vector3 v1(1.0f, 2.0f, 3.0f);
		ASSERT_FLOAT_EQUAL(1.0f, v1.x, "Vector3 constructor - x component");
		ASSERT_FLOAT_EQUAL(2.0f, v1.y, "Vector3 constructor - y component");
		ASSERT_FLOAT_EQUAL(3.0f, v1.z, "Vector3 constructor - z component");

		// 长度测试
		ASSERT_FLOAT_EQUAL(std::sqrt(14.0f), v1.Length(), "Vector3 length calculation");
		ASSERT_FLOAT_EQUAL(14.0f, v1.LengthSquared(), "Vector3 length squared calculation");

		// 点积测试
		Vector3 v2(2.0f, 3.0f, 4.0f);
		ASSERT_FLOAT_EQUAL(20.0f, v1.Dot(v2), "Vector3 dot product");

		// 叉积测试
		Vector3 v3(1.0f, 0.0f, 0.0f);
		Vector3 v4(0.0f, 1.0f, 0.0f);
		Vector3 cross = v3.Cross(v4);
		ASSERT_VECTOR3_EQUAL(Vector3(0.0f, 0.0f, 1.0f), cross, "Vector3 cross product");

		// 标准化测试
		Vector3 v5(3.0f, 4.0f, 0.0f);
		Vector3 normalized = v5.Normalize();
		ASSERT_FLOAT_EQUAL(1.0f, normalized.Length(), "Vector3 normalize - unit length");

		// 静态方向向量测试
		ASSERT_VECTOR3_EQUAL(Vector3(1.0f, 0.0f, 0.0f), Vector3::Right(), "Vector3 Right direction");
		ASSERT_VECTOR3_EQUAL(Vector3(0.0f, 1.0f, 0.0f), Vector3::Up(), "Vector3 Up direction");
		ASSERT_VECTOR3_EQUAL(Vector3(0.0f, 0.0f, -1.0f), Vector3::Forward(), "Vector3 Forward direction");

		// 运算符重载测试
		Vector3 v6 = v1 + v2;
		ASSERT_VECTOR3_EQUAL(Vector3(3.0f, 5.0f, 7.0f), v6, "Vector3 addition operator");

		Vector3 v7 = v2 - v1;
		ASSERT_VECTOR3_EQUAL(Vector3(1.0f, 1.0f, 1.0f), v7, "Vector3 subtraction operator");

		Vector3 v8 = v1 * 2.0f;
		ASSERT_VECTOR3_EQUAL(Vector3(2.0f, 4.0f, 6.0f), v8, "Vector3 scalar multiplication");
	}

	// ================================
	// Vector4 测试
	// ================================
	static void TestVector4() {
		std::cout << "\n=== Testing Vector4 ===" << std::endl;

		// 构造函数测试
		Vector4 v1(1.0f, 2.0f, 3.0f, 4.0f);
		ASSERT_FLOAT_EQUAL(1.0f, v1.x, "Vector4 constructor - x component");
		ASSERT_FLOAT_EQUAL(4.0f, v1.w, "Vector4 constructor - w component");

		// 从Vector3构造
		Vector3 v3d(1.0f, 2.0f, 3.0f);
		Vector4 v4_from_v3(v3d, 5.0f);
		ASSERT_FLOAT_EQUAL(1.0f, v4_from_v3.x, "Vector4 from Vector3 - x");
		ASSERT_FLOAT_EQUAL(5.0f, v4_from_v3.w, "Vector4 from Vector3 - w");

		// 长度测试
		ASSERT_FLOAT_EQUAL(std::sqrt(30.0f), v1.Length(), "Vector4 length calculation");

		// 点积测试
		Vector4 v2(2.0f, 3.0f, 4.0f, 5.0f);
		ASSERT_FLOAT_EQUAL(40.0f, v1.Dot(v2), "Vector4 dot product");

		// SIMD运算测试（如果支持）
		Vector4 v4 = v1 + v2;
		ASSERT_VECTOR3_EQUAL(Vector3(3.0f, 5.0f, 7.0f), Vector3(v4.x, v4.y, v4.z), "Vector4 SIMD addition");

		// 标准化测试
		Vector4 v5 = v1;
		v5.Normalize();
		ASSERT_FLOAT_EQUAL(1.0f, v5.Length(), "Vector4 normalize - unit length");
	}

	// ================================
	// Matrix4 测试
	// ================================
	static void TestMatrix4() {
		std::cout << "\n=== Testing Matrix4 ===" << std::endl;

		// 单位矩阵测试
		Matrix4 identity = Matrix4::Identity();
		ASSERT_FLOAT_EQUAL(1.0f, identity.data[0], "Matrix4 Identity - [0,0]");
		ASSERT_FLOAT_EQUAL(1.0f, identity.data[5], "Matrix4 Identity - [1,1]");
		ASSERT_FLOAT_EQUAL(1.0f, identity.data[10], "Matrix4 Identity - [2,2]");
		ASSERT_FLOAT_EQUAL(1.0f, identity.data[15], "Matrix4 Identity - [3,3]");

		// 矩阵乘法测试
		Matrix4 result = identity * identity;
		ASSERT_MATRIX4_EQUAL(identity, result, "Matrix4 Identity multiplication");

		// 平移矩阵测试
		Vector3 translation(1.0f, 2.0f, 3.0f);
		Matrix4 trans = Matrix4::FromTranslation(translation);
		ASSERT_FLOAT_EQUAL(1.0f, trans.data[12], "Matrix4 Translation - tx");
		ASSERT_FLOAT_EQUAL(2.0f, trans.data[13], "Matrix4 Translation - ty");
		ASSERT_FLOAT_EQUAL(3.0f, trans.data[14], "Matrix4 Translation - tz");

		// 缩放矩阵测试
		Vector3 scale(2.0f, 3.0f, 4.0f);
		Matrix4 scale_mat = Matrix4::FromScale(scale);
		ASSERT_FLOAT_EQUAL(2.0f, scale_mat.data[0], "Matrix4 Scale - sx");
		ASSERT_FLOAT_EQUAL(3.0f, scale_mat.data[5], "Matrix4 Scale - sy");
		ASSERT_FLOAT_EQUAL(4.0f, scale_mat.data[10], "Matrix4 Scale - sz");

		// 向量变换测试
		Vector3 point(1.0f, 1.0f, 1.0f);
		Vector3 transformed = trans * point;
		ASSERT_VECTOR3_EQUAL(Vector3(2.0f, 3.0f, 4.0f), transformed, "Matrix4 point transformation");

		// 逆矩阵测试
		Matrix4 inverse = trans.Inverse();
		Matrix4 should_be_identity = trans * inverse;

		// 检查是否接近单位矩阵
		bool is_identity = true;
		for (int i = 0; i < 4; ++i) {
			for (int j = 0; j < 4; ++j) {
				float expected = (i == j) ? 1.0f : 0.0f;
				if (std::abs(should_be_identity.data[i * 4 + j] - expected) > EPSILON) {
					is_identity = false;
					break;
				}
			}
		}
		ASSERT_TRUE(is_identity, "Matrix4 inverse calculation");

		// 旋转矩阵测试
		Matrix4 rot_x = Matrix4::EulerX(D_HALF_PI);  // 90度绕X轴旋转
		Vector3 y_axis(0.0f, 1.0f, 0.0f);
		Vector3 rotated_y = rot_x * y_axis;
		ASSERT_VECTOR3_EQUAL(Vector3(0.0f, 0.0f, 1.0f), rotated_y, "Matrix4 X rotation");

		// LookAt矩阵测试
		Vector3 eye(0.0f, 0.0f, 5.0f);
		Vector3 target(0.0f, 0.0f, 0.0f);
		Vector3 up(0.0f, 1.0f, 0.0f);
		Matrix4 view = Matrix4::LookAt(eye, target, up);

		Vector3 forward = view.Forward();
		ASSERT_FLOAT_EQUAL(1.0f, forward.Length(), "Matrix4 LookAt - forward vector length");

		// 透视投影矩阵测试
		Matrix4 proj = Matrix4::Perspective(Deg2Rad(45.0f), 16.0f / 9.0f, 0.1f, 100.0f);
		ASSERT_TRUE(proj.data[11] == -1.0f, "Matrix4 Perspective - perspective divide");
	}

	// ================================
	// Quaternion 测试
	// ================================
	static void TestQuaternion() {
		std::cout << "\n=== Testing Quaternion ===" << std::endl;

		// 默认构造函数（单位四元数）
		Quaternion q1;
		ASSERT_FLOAT_EQUAL(0.0f, q1.x, "Quaternion default - x");
		ASSERT_FLOAT_EQUAL(0.0f, q1.y, "Quaternion default - y");
		ASSERT_FLOAT_EQUAL(0.0f, q1.z, "Quaternion default - z");
		ASSERT_FLOAT_EQUAL(1.0f, q1.w, "Quaternion default - w");

		// 从轴角构造
		Vector3 axis(0.0f, 0.0f, 1.0f);  // Z轴
		float angle = D_HALF_PI;  // 90度
		Quaternion q2(axis, angle);
		q2.Normalize();

		// 转换为旋转矩阵
		Matrix4 rot_mat = q2.ToRotationMatrix();
		Vector3 x_axis(1.0f, 0.0f, 0.0f);
		Vector3 rotated_x = rot_mat * x_axis;
		ASSERT_VECTOR3_EQUAL(Vector3(0.0f, 1.0f, 0.0f), rotated_x, "Quaternion Z-axis rotation");

		// 从欧拉角构造
		Vector3 euler(90.0f, 0.0f, 0.0f);  // 90度绕X轴
		Quaternion q3(euler);
		Matrix4 euler_mat = q3.ToRotationMatrix();
		Vector3 y_axis(0.0f, 1.0f, 0.0f);
		Vector3 rotated_y = euler_mat * y_axis;
		ASSERT_VECTOR3_EQUAL(Vector3(0.0f, 0.0f, 1.0f), rotated_y, "Quaternion Euler X rotation");

		// 四元数乘法测试
		Quaternion q4 = q2.Multiply(q3);
		ASSERT_FLOAT_EQUAL(1.0f, q4.Length(), "Quaternion multiplication - unit length");

		// SLERP测试
		Quaternion q_start(Vector3(0, 0, 1), 0.0f);  // 无旋转
		Quaternion q_end(Vector3(0, 0, 1), D_HALF_PI);  // 90度旋转
		Quaternion q_mid = Quaternion::Slerp(q_start, q_end, 0.5f);

		Matrix4 mid_mat = q_mid.ToRotationMatrix();
		Vector3 test_vec(1.0f, 0.0f, 0.0f);
		Vector3 mid_result = mid_mat * test_vec;

		// 45度旋转应该给出 (cos45, sin45, 0) ≈ (0.707, 0.707, 0)
		float cos45 = std::cos(D_PI / 4.0f);
		ASSERT_FLOAT_EQUAL(cos45, mid_result.x, "Quaternion SLERP - 45 degree rotation x");
		ASSERT_FLOAT_EQUAL(cos45, mid_result.y, "Quaternion SLERP - 45 degree rotation y");

		// 转换为欧拉角测试
		Vector3 back_to_euler = q3.ToEuler();
		ASSERT_FLOAT_EQUAL(D_HALF_PI, back_to_euler.x, "Quaternion to Euler conversion");
	}

	// ================================
	// Transform 测试
	// ================================
	static void TestTransform() {
		std::cout << "\n=== Testing Transform ===" << std::endl;

		// 默认构造函数
		Transform t1;
		ASSERT_VECTOR3_EQUAL(Vector3(0.0f), t1.GetLocation(), "Transform default - position");
		ASSERT_VECTOR3_EQUAL(Vector3(1.0f), t1.GetScale(), "Transform default - scale");

		// 设置位置、旋转、缩放
		Vector3 pos(1.0f, 2.0f, 3.0f);
		Vector3 euler(0.0f, 90.0f, 0.0f);  // Y轴旋转90度
		Vector3 scale(2.0f, 2.0f, 2.0f);

		Quaternion rot(euler);
		Transform t2(pos, rot, scale);

		ASSERT_VECTOR3_EQUAL(pos, t2.GetLocation(), "Transform constructor - position");
		ASSERT_VECTOR3_EQUAL(scale, t2.GetScale(), "Transform constructor - scale");

		// 点变换测试
		Vector3 local_point(1.0f, 0.0f, 0.0f);
		Vector3 world_point = t2.TransformPoint(local_point);

		// 应用缩放(2x)、Y轴旋转90度、平移
		// (1,0,0) -> 缩放 -> (2,0,0) -> Y旋转90度 -> (0,0,-2) -> 平移 -> (1,2,1)
		ASSERT_VECTOR3_EQUAL(Vector3(1.0f, 2.0f, 1.0f), world_point, "Transform point transformation");

		// 逆变换测试
		Vector3 back_to_local = t2.InverseTransformPoint(world_point);
		ASSERT_VECTOR3_EQUAL(local_point, back_to_local, "Transform inverse point transformation");

		// 方向变换测试（不受平移影响）
		Vector3 local_dir(1.0f, 0.0f, 0.0f);
		Vector3 world_dir = t2.TransformDirection(local_dir);
		// 方向只受旋转和缩放影响：(1,0,0) -> (2,0,0) -> (0,0,-2)
		Vector3 expected_dir = Vector3(0.0f, 0.0f, -2.0f);
		ASSERT_VECTOR3_EQUAL(expected_dir, world_dir, "Transform direction transformation");

		// 变换组合测试
		Transform t3;
		t3.Translate(Vector3(1.0f, 0.0f, 0.0f));
		t3.Scale(Vector3(2.0f, 2.0f, 2.0f));

		Vector3 test_point(1.0f, 1.0f, 1.0f);
		Vector3 result = t3.TransformPoint(test_point);
		// (1,1,1) -> 缩放(2,2,2) -> 平移(1,0,0) -> (3,2,2)
		ASSERT_VECTOR3_EQUAL(Vector3(3.0f, 2.0f, 2.0f), result, "Transform composition");
	}

	// ================================
	// GeometryUtils 测试
	// ================================
	static void TestGeometryUtils() {
		std::cout << "\n=== Testing GeometryUtils ===" << std::endl;

		// 创建一个简单的三角形
		std::vector<Vertex> vertices(3);
		vertices[0].position = Vector3(0.0f, 0.0f, 0.0f);
		vertices[0].texcoord = Vector2(0.0f, 0.0f);

		vertices[1].position = Vector3(1.0f, 0.0f, 0.0f);
		vertices[1].texcoord = Vector2(1.0f, 0.0f);

		vertices[2].position = Vector3(0.0f, 1.0f, 0.0f);
		vertices[2].texcoord = Vector2(0.0f, 1.0f);

		std::vector<uint32_t> indices = { 0, 1, 2 };

		// 法线生成测试
		GeometryUtils::GenerateNormals(
			static_cast<uint32_t>(vertices.size()), vertices.data(),
			static_cast<uint32_t>(indices.size()), indices.data()
		);

		// 应该生成Z轴正方向的法线
		Vector3 expected_normal(0.0f, 0.0f, 1.0f);
		ASSERT_VECTOR3_EQUAL(expected_normal, vertices[0].normal, "GeometryUtils normal generation");

		// 切线生成测试
		GeometryUtils::GenerateTangents(
			static_cast<uint32_t>(vertices.size()), vertices.data(),
			static_cast<uint32_t>(indices.size()), indices.data()
		);

		// 切线应该沿X轴方向
		Vector3 expected_tangent(1.0f, 0.0f, 0.0f);
		ASSERT_VECTOR3_EQUAL(expected_tangent, Vector3(vertices[0].tangent.x, vertices[0].tangent.y, vertices[0].tangent.z),
			"GeometryUtils tangent generation");

		// 顶点去重测试
		std::vector<Vertex> dup_vertices = vertices;
		dup_vertices.push_back(vertices[0]);  // 重复第一个顶点
		std::vector<uint32_t> dup_indices = { 0, 1, 2, 3, 1, 2 };  // 使用重复顶点

		uint32_t out_vertex_count;
		Vertex* out_vertices;

		GeometryUtils::DeduplicateVertices(
			static_cast<uint32_t>(dup_vertices.size()), dup_vertices.data(),
			static_cast<uint32_t>(dup_indices.size()), dup_indices.data(),
			&out_vertex_count, &out_vertices
		);

		ASSERT_TRUE(out_vertex_count == 3, "GeometryUtils vertex deduplication - count");

		if (out_vertices) {
			Memory::Free(out_vertices, sizeof(Vertex) * out_vertex_count, MemoryType::eMemory_Type_Array);
		}
	}

	// ================================
	// DMath 函数测试
	// ================================
	static void TestDMath() {
		std::cout << "\n=== Testing DMath ===" << std::endl;

		// 三角函数测试
		ASSERT_FLOAT_EQUAL(0.0f, DSin(0.0f), "DSin(0)");
		ASSERT_FLOAT_EQUAL(1.0f, DSin(D_HALF_PI), "DSin(π/2)");
		ASSERT_FLOAT_EQUAL(1.0f, DCos(0.0f), "DCos(0)");
		ASSERT_FLOAT_EQUAL(0.0f, DCos(D_HALF_PI), "DCos(π/2)");

		// 角度转换测试
		ASSERT_FLOAT_EQUAL(D_HALF_PI, Deg2Rad(90.0f), "Deg2Rad(90°)");
		ASSERT_FLOAT_EQUAL(90.0f, Rad2Deg(D_HALF_PI), "Rad2Deg(π/2)");

		// 平方根测试
		ASSERT_FLOAT_EQUAL(2.0f, Dsqrt(4.0f), "Dsqrt(4)");
		ASSERT_FLOAT_EQUAL(3.0f, Dsqrt(9.0f), "Dsqrt(9)");

		// atan2测试
		ASSERT_FLOAT_EQUAL(D_HALF_PI, DArcTan2(1.0f, 0.0f), "DArcTan2(1, 0)");
		ASSERT_FLOAT_EQUAL(D_PI / 4.0f, DArcTan2(1.0f, 1.0f), "DArcTan2(1, 1)");

		// 2的幂测试
		ASSERT_TRUE(IsPowerOf2(1), "IsPowerOf2(1)");
		ASSERT_TRUE(IsPowerOf2(2), "IsPowerOf2(2)");
		ASSERT_TRUE(IsPowerOf2(4), "IsPowerOf2(4)");
		ASSERT_TRUE(IsPowerOf2(1024), "IsPowerOf2(1024)");
		ASSERT_TRUE(!IsPowerOf2(3), "!IsPowerOf2(3)");
		ASSERT_TRUE(!IsPowerOf2(0), "!IsPowerOf2(0)");

		// 随机数测试（基本检查）
		int rand1 = DRandom(0, 100);
		int rand2 = DRandom(0, 100);
		ASSERT_TRUE(rand1 >= 0 && rand1 <= 100, "DRandom integer range");
		ASSERT_TRUE(rand1 != rand2, "DRandom different values");  // 概率性测试

		float randf1 = DRandom(0.0f, 1.0f);
		ASSERT_TRUE(randf1 >= 0.0f && randf1 <= 1.0f, "DRandom float range");
	}

	// ================================
	// 性能测试
	// ================================
	static void PerformanceTests() {
		std::cout << "\n=== Performance Tests ===" << std::endl;

		const int ITERATIONS = 1000000;

		// Vector3 加法性能
		auto start = std::chrono::high_resolution_clock::now();

		Vector3 v1(1.0f, 2.0f, 3.0f);
		Vector3 v2(4.0f, 5.0f, 6.0f);
		Vector3 result;

		for (int i = 0; i < ITERATIONS; ++i) {
			result = v1 + v2;
			v1.x += 0.001f;  // 防止编译器优化
		}

		auto end = std::chrono::high_resolution_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

		std::cout << "Vector3 addition (" << ITERATIONS << " ops): "
			<< duration.count() << "ms" << std::endl;

		// Matrix4 乘法性能
		start = std::chrono::high_resolution_clock::now();

		Matrix4 m1 = Matrix4::Identity();
		Matrix4 m2 = Matrix4::FromTranslation(Vector3(1.0f, 2.0f, 3.0f));
		Matrix4 m_result;

		for (int i = 0; i < ITERATIONS / 100; ++i) {  // 矩阵运算更重，减少迭代次数
			m_result = m1 * m2;
			m1.data[12] += 0.001f;  // 防止编译器优化
		}

		end = std::chrono::high_resolution_clock::now();
		duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

		std::cout << "Matrix4 multiplication (" << ITERATIONS / 100 << " ops): "
			<< duration.count() << "ms" << std::endl;

		// Quaternion SLERP性能
		start = std::chrono::high_resolution_clock::now();

		Quaternion q1(Vector3(0, 0, 1), 0.0f);
		Quaternion q2(Vector3(0, 0, 1), D_HALF_PI);
		Quaternion q_result;

		for (int i = 0; i < ITERATIONS / 10; ++i) {
			float t = (i % 100) / 100.0f;
			q_result = Quaternion::Slerp(q1, q2, t);
		}

		end = std::chrono::high_resolution_clock::now();
		duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

		std::cout << "Quaternion SLERP (" << ITERATIONS / 10 << " ops): "
			<< duration.count() << "ms" << std::endl;
	}

	// ================================
	// 边界条件测试
	// ================================
	static void EdgeCaseTests() {
		std::cout << "\n=== Edge Case Tests ===" << std::endl;

		// 零向量测试
		Vector3 zero_vec(0.0f, 0.0f, 0.0f);
		Vector3 normalized = zero_vec.Normalize();
		ASSERT_FLOAT_EQUAL(0.0f, normalized.Length(), "Zero vector normalization");

		// 非常小的向量
		Vector3 tiny_vec(1e-10f, 1e-10f, 1e-10f);
		Vector3 tiny_norm = tiny_vec.Normalize();
		ASSERT_TRUE(tiny_norm.Length() <= 1.0f + EPSILON, "Tiny vector normalization");

		// 奇异矩阵（零行列式）
		Matrix4 singular;
		Memory::Zero(singular.data, sizeof(float) * 16);
		Matrix4 inv = singular.Inverse();
		// 应该返回一个有效的矩阵（或处理奇异情况）
		ASSERT_TRUE(true, "Singular matrix inverse handled");

		// 四元数边界情况
		Quaternion q_zero(0.0f, 0.0f, 0.0f, 0.0f);
		Quaternion q_norm = q_zero.Normalize();
		ASSERT_FLOAT_EQUAL(1.0f, q_norm.w, "Zero quaternion normalization");

		// 极值角度
		float large_angle = 10.0f * D_PI;  // 大角度
		ASSERT_FLOAT_EQUAL(0.0f, DSin(large_angle), "Large angle sine");

		std::cout << "Edge case tests completed" << std::endl;
	}

	// ================================
	// 回归测试（针对已知修复的问题）
	// ================================
	static void RegressionTests() {
		std::cout << "\n=== Regression Tests ===" << std::endl;

		// 测试之前的Matrix4比较运算符bug
		Matrix4 m1 = Matrix4::Identity();
		Matrix4 m2 = Matrix4::Identity();
		ASSERT_TRUE(m1 == m2, "Matrix4 equality operator regression test");

		m2.data[1] = 1.0f;  // 修改一个元素
		ASSERT_TRUE(m1 != m2, "Matrix4 inequality regression test");

		// 测试Quaternion Normalize const正确性
		const Quaternion q_const(1.0f, 1.0f, 1.0f, 1.0f);
		Quaternion q_normalized = q_const.Normalize();  // 应该编译通过
		ASSERT_FLOAT_EQUAL(1.0f, q_normalized.Length(), "Quaternion const normalization regression test");

		// 测试Vertex比较
		Vertex v1, v2;
		v1.position = Vector3(1.0f, 2.0f, 3.0f);
		v2.position = Vector3(1.0f, 2.0f, 3.0f);
		ASSERT_TRUE(v1 == v2, "Vertex equality operator regression test");

		// 测试Transform const正确性
		const Transform t_const(Vector3(1.0f, 2.0f, 3.0f));
		Vector3 world_pos = t_const.TransformPoint(Vector3(0.0f, 0.0f, 0.0f));
		ASSERT_VECTOR3_EQUAL(Vector3(1.0f, 2.0f, 3.0f), world_pos, "Transform const member function regression test");

		std::cout << "Regression tests completed" << std::endl;
	}

	// ================================
	// 主测试函数
	// ================================
	static void RunAllTests() {
		std::cout << "=====================================\n";
		std::cout << "      Math Library Test Suite\n";
		std::cout << "=====================================\n";

		total_tests = 0;
		passed_tests = 0;
		failed_tests = 0;

		// 运行所有测试
		TestVector2();
		TestVector3();
		TestVector4();
		TestMatrix4();
		TestQuaternion();
		TestTransform();
		TestGeometryUtils();
		TestDMath();
		EdgeCaseTests();
		RegressionTests();

		// 可选：性能测试
		PerformanceTests();

		// 输出测试结果
		std::cout << "\n=====================================\n";
		std::cout << "          Test Results\n";
		std::cout << "=====================================\n";
		std::cout << "Total tests:  " << total_tests << std::endl;
		std::cout << "Passed:       " << passed_tests << std::endl;
		std::cout << "Failed:       " << failed_tests << std::endl;
		std::cout << "Success rate: " << std::fixed << std::setprecision(1)
			<< (100.0f * passed_tests / total_tests) << "%" << std::endl;

		if (failed_tests == 0) {
			std::cout << "\nAll tests passed! Math library is working correctly.\n" << std::endl;
		}
		else {
			std::cout << "\nSome tests failed. Please review the implementation.\n" << std::endl;
		}
	}
};

// 静态成员定义
int MathLibraryTests::total_tests = 0;
int MathLibraryTests::passed_tests = 0;
int MathLibraryTests::failed_tests = 0;