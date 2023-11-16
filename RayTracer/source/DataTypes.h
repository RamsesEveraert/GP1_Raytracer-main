#pragma once
#include <cassert>

#include "Math.h"
#include <vector>
#include <array>

namespace dae
{
#pragma region GEOMETRY
	struct Sphere
	{
		Vector3 origin{};
		float radius{};

		unsigned char materialIndex{ 0 };
	};

	struct Plane
	{
		Vector3 origin{};
		Vector3 normal{};

		unsigned char materialIndex{ 0 };
	};

	enum class TriangleCullMode
	{
		FrontFaceCulling,
		BackFaceCulling,
		NoCulling
	};

	struct Triangle
	{
		Triangle() = default;
		Triangle(const Vector3& _v0, const Vector3& _v1, const Vector3& _v2, const Vector3& _normal) :
			v0{ _v0 }, v1{ _v1 }, v2{ _v2 }, normal{ _normal } {}

		Triangle(const Vector3& _v0, const Vector3& _v1, const Vector3& _v2) :
			v0{ _v0 }, v1{ _v1 }, v2{ _v2 }
		{
			const Vector3 edgeV0V1 = v1 - v0;
			const Vector3 edgeV0V2 = v2 - v0;
			normal = Vector3::Cross(edgeV0V1, edgeV0V2).Normalized();
		}

		Vector3 v0{};
		Vector3 v1{};
		Vector3 v2{};

		Vector3 normal{};

		TriangleCullMode cullMode{};
		unsigned char materialIndex{};
	};

	struct AABB {
		Vector3 minAABB;
		Vector3 maxAABB;

		AABB() = default;
		AABB(const Vector3& min, const Vector3& max) : minAABB(min), maxAABB(max) {}

		// Expands the bounding box to include the given point
		void Encapsulate(const Vector3& point) 
		{
			minAABB = Vector3::Min(minAABB, point);
			maxAABB = Vector3::Max(maxAABB, point);
		}

		// Creates an AABB that encapsulates a collection of points
		static AABB FromPoints(const std::vector<Vector3>& points) 
		{
			if (points.empty()) return {};

			// Initialize AABB using the first point
			AABB aabb(points[0], points[0]);
			for (const auto& point : points) {
				aabb.Encapsulate(point);
			}
			return aabb;
		}

		AABB Transformed(const Matrix& transform) const 
		{
			// Transformes position of each corner
			std::array<Vector3, 8> corners = 
			{ // https://stackoverflow.com/questions/4424579/stdvector-versus-stdarray-in-c
				transform.TransformPoint(minAABB),
				transform.TransformPoint({minAABB.x, minAABB.y, maxAABB.z}),
				transform.TransformPoint({minAABB.x, maxAABB.y, minAABB.z}),
				transform.TransformPoint({minAABB.x, maxAABB.y, maxAABB.z}),
				transform.TransformPoint({maxAABB.x, minAABB.y, minAABB.z}),
				transform.TransformPoint({maxAABB.x, minAABB.y, maxAABB.z}),
				transform.TransformPoint({maxAABB.x, maxAABB.y, minAABB.z}),
				transform.TransformPoint(maxAABB)
			};

			AABB transformedAABB = AABB(corners[0], corners[0]);
			for (const auto& corner : corners) 
			{
				transformedAABB.Encapsulate(corner);  // Expand the AABB to include the transformed corners
			}
			return transformedAABB;
		}
	};


	struct TriangleMesh
	{
		TriangleMesh() = default;
		TriangleMesh(const std::vector<Vector3>& _positions, const std::vector<int>& _indices, TriangleCullMode _cullMode) :
			positions(_positions), indices(_indices), cullMode(_cullMode)
		{
			//Calculate Normals
			CalculateNormals();

			//Update Transforms
			UpdateTransforms();
		}

		TriangleMesh(const std::vector<Vector3>& _positions, const std::vector<int>& _indices, const std::vector<Vector3>& _normals, TriangleCullMode _cullMode) :
			positions(_positions), indices(_indices), normals(_normals), cullMode(_cullMode)
		{
			UpdateTransforms();
		}

		std::vector<Vector3> positions{};
		std::vector<Vector3> normals{};
		std::vector<int> indices{};
		unsigned char materialIndex{};

		TriangleCullMode cullMode{ TriangleCullMode::BackFaceCulling };

		Matrix rotationTransform{};
		Matrix translationTransform{};
		Matrix scaleTransform{};

		AABB aabb;
		AABB transformedAABB;

		std::vector<Vector3> transformedPositions{};
		std::vector<Vector3> transformedNormals{};

		void Translate(const Vector3& translation)
		{
			translationTransform = Matrix::CreateTranslation(translation);
		}

		void RotateY(float yaw)
		{
			rotationTransform = Matrix::CreateRotationY(yaw);
		}

		void Scale(const Vector3& scale)
		{
			scaleTransform = Matrix::CreateScale(scale);
		}

		void AppendTriangle(const Triangle& triangle, bool ignoreTransformUpdate = false)
		{
			int startIndex = static_cast<int>(positions.size());

			positions.emplace_back(triangle.v0);
			positions.emplace_back(triangle.v1);
			positions.emplace_back(triangle.v2);

			indices.emplace_back(startIndex);
			indices.emplace_back(++startIndex);
			indices.emplace_back(++startIndex);

			normals.emplace_back(triangle.normal);

			//Not ideal, but making sure all vertices are updated
			if (!ignoreTransformUpdate)
			{
				UpdateTransforms();
			}
				
		}

		void CalculateNormals()
		{
			size_t meshIndices = indices.size();

			normals.clear();
			normals.reserve(meshIndices / 3);

			for (size_t offset = 0; offset < meshIndices; offset += 3)
			{
				Vector3 v0 = positions[indices[offset]];
				Vector3 v1 = positions[indices[offset + 1]];
				Vector3 v2 = positions[indices[offset + 2]];

				normals.emplace_back(Vector3::Cross(v1 - v0, v2 - v0).Normalized());
			}


		}

		void UpdateAABB() 
		{
			aabb = AABB::FromPoints(positions);
		}

		void UpdateTransforms() 
		{
			Matrix transform = scaleTransform * rotationTransform * translationTransform;

			transformedPositions.clear();
			transformedNormals.clear();
			transformedPositions.reserve(positions.size());
			transformedNormals.reserve(normals.size());

			for (const auto& position : positions) {
				transformedPositions.emplace_back(transform.TransformPoint(position));
			}

			for (const auto& normal : normals) {
				transformedNormals.emplace_back(transform.TransformVector(normal).Normalized());
			}

			transformedAABB = aabb.Transformed(transform);
		}
	};
#pragma endregion
#pragma region LIGHT
	enum class LightType
	{
		Point,
		Directional
	};

	struct Light
	{
		Vector3 origin{};
		Vector3 direction{};
		ColorRGB color{};
		float intensity{};

		LightType type{};
	};
#pragma endregion
#pragma region MISC
	struct Ray
	{
		Vector3 origin{};
		Vector3 direction{};

		float min{ 0.0001f };
		float max{ FLT_MAX };
	};

	struct HitRecord
	{
		Vector3 origin{};
		Vector3 normal{};
		float t = FLT_MAX;

		bool didHit{ false };
		unsigned char materialIndex{ 0 };
	};
#pragma endregion
}