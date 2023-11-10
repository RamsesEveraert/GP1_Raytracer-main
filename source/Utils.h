#pragma once
#include <cassert>
#include <fstream>
#include "Math.h"
#include "DataTypes.h"

namespace dae
{
	namespace GeometryUtils
	{
#pragma region Sphere HitTest
		//SPHERE HIT-TESTS

		inline bool HitTest_Sphere(const Sphere& sphere, const Ray& ray, HitRecord& hitRecord, bool ignoreHitRecord = false)
		{
			//TODO w1
			Vector3 originRayToCenterCircle = ray.origin - sphere.origin;

			float a = Vector3::Dot(ray.direction, ray.direction);
			float b = 2 * Vector3::Dot(ray.direction, originRayToCenterCircle);
			float c = Vector3::Dot(originRayToCenterCircle, originRayToCenterCircle) - sphere.radius * sphere.radius;

			float discriminant = b * b - 4 * a * c;  // Discriminant

			if (discriminant <= 0.0f) return false;

			float sqrtDiscriminant = sqrtf(discriminant);
			float inv2a = 1.0f / (2 * a);

			float t = (-b - sqrtDiscriminant) * inv2a;

			if (t < ray.min || t > ray.max)
			{
				t = (-b + sqrtDiscriminant) * inv2a;

				if (t < ray.min || t > ray.max)	return false;
			}

			if (!ignoreHitRecord && t < hitRecord.t)
			{
				hitRecord.didHit = true;
				hitRecord.materialIndex = sphere.materialIndex;
				hitRecord.origin = ray.origin + ray.direction * t;
				hitRecord.normal = (hitRecord.origin - sphere.origin).Normalized();
				hitRecord.t = t;
			}

			return true;
		
		}

		inline bool HitTest_Sphere(const Sphere& sphere, const Ray& ray)
		{
			static HitRecord temp{};
			return HitTest_Sphere(sphere, ray, temp, true);
		}
#pragma endregion
#pragma region Plane HitTest
		//PLANE HIT-TESTS
		inline bool HitTest_Plane(const Plane& plane, const Ray& ray, HitRecord& hitRecord, bool ignoreHitRecord = false)
		{

			//TODO w1
			const float t = Vector3::Dot((plane.origin - ray.origin), plane.normal) / Vector3::Dot(ray.direction, plane.normal);

			if (ray.min <= t && t <= ray.max)
			{
				if (!ignoreHitRecord && t < hitRecord.t)
				{
					hitRecord.t = t;
					hitRecord.didHit = true;
					hitRecord.origin = ray.origin + t * ray.direction.Normalized();
					hitRecord.normal = plane.normal;
					hitRecord.materialIndex = plane.materialIndex;
				}
				return true;
			}

			return false;
		}

		inline bool HitTest_Plane(const Plane& plane, const Ray& ray)
		{
			HitRecord temp{};
			return HitTest_Plane(plane, ray, temp, true);
		}
#pragma endregion
#pragma region Triangle HitTest

		inline bool HitTest_Triangle(const Triangle& triangle, const Ray& ray, HitRecord& hitRecord, bool ignoreHitRecord = false)
		{

			// Get two edges of triangle
			Vector3 e1 = triangle.v1 - triangle.v0;
			Vector3 e2 = triangle.v2 - triangle.v0;

			// Get the vector perpendicular to ray direction and edge e2 L.H cross
			Vector3 h = Vector3::Cross(e2, ray.direction);

			// Calculate the dot product of edge e1 and vector h
			float a = Vector3::Dot(e1, h);

			// If triangle is parallel return
			if (dae::AreEqual(a, 0.f)) return false;

			float inv_a = 1.0f / a;

			auto cullMode = triangle.cullMode;

			// When computing shadows
			if (ignoreHitRecord)
			{
				//flip for shadows
				switch (cullMode)
				{
				case TriangleCullMode::FrontFaceCulling:
					cullMode = TriangleCullMode::BackFaceCulling;
					break;

				case TriangleCullMode::BackFaceCulling:
					cullMode = TriangleCullMode::FrontFaceCulling;
					break;
				}
			}

			if (cullMode == TriangleCullMode::FrontFaceCulling && a < 0.0f) return false;
			if (cullMode == TriangleCullMode::BackFaceCulling && a > 0.0f) return false;

			Vector3 s = ray.origin - triangle.v0;

			// barycentric coordinate u u
			float u = Vector3::Dot(s, h) * inv_a;			
			if (u < 0.0f || u > 1.0f) return false;

			Vector3 q = Vector3::Cross(e1, s);

			// barycentric coordinate v
			float v = Vector3::Dot(ray.direction, q) * inv_a;			
			if (v < 0.0f || u + v > 1.0f) return false;

			float t = Vector3::Dot(e2, q) * inv_a;
			if (t < ray.min || t > ray.max || t >= hitRecord.t) return false;

			// If intersections are not to be ignored, record the intersection details
			if (!ignoreHitRecord) {
				hitRecord.t = t;  
				hitRecord.didHit = true; 
				hitRecord.origin = ray.origin + t * ray.direction;
				hitRecord.normal = triangle.normal;
				hitRecord.materialIndex = triangle.materialIndex;
			}

			return true; 
		}

	inline bool HitTest_Triangle(const Triangle& triangle, const Ray& ray)
		{
			HitRecord temp{};
			return HitTest_Triangle(triangle, ray, temp, true);
		}
#pragma endregion
#pragma region TriangeMesh HitTest
		inline bool SlabTest_TriangleMesh(const TriangleMesh& mesh, const Ray& ray)
		{
			const auto& aabb = mesh.transformedAABB; 
			Vector3 invDir = {
				1.0f / ray.direction.x,
				1.0f / ray.direction.y,
				1.0f / ray.direction.z
			};

			// Perform slab tests
			float t0x = (aabb.min.x - ray.origin.x) * invDir.x;
			float t1x = (aabb.max.x - ray.origin.x) * invDir.x;
			float tmin = std::min(t0x, t1x);
			float tmax = std::max(t0x, t1x);

			float t0y = (aabb.min.y - ray.origin.y) * invDir.y;
			float t1y = (aabb.max.y - ray.origin.y) * invDir.y;
			tmin = std::max(tmin, std::min(t0y, t1y));
			tmax = std::min(tmax, std::max(t0y, t1y));

			float t0z = (aabb.min.z - ray.origin.z) * invDir.z;
			float t1z = (aabb.max.z - ray.origin.z) * invDir.z;
			tmin = std::max(tmin, std::min(t0z, t1z));
			tmax = std::min(tmax, std::max(t0z, t1z));

			// Check if there are intersections within the ray bounds
			return (tmax >= tmin) && (tmax >= 0) && (tmin < ray.max) && (tmin > ray.min);
		}



		inline bool HitTest_TriangleMesh(const TriangleMesh& mesh, const Ray& ray, HitRecord& hitRecord, bool ignoreHitRecord = false)
		{
			if (!SlabTest_TriangleMesh(mesh, ray)) return false;

			size_t triangleCount = mesh.indices.size() / 3;

			bool didHit = false;

			for (size_t i = 0; i < triangleCount; ++i)
			{
				size_t offset = i * 3;
				 
				Triangle triangle{
					mesh.transformedPositions[mesh.indices[offset]],
					mesh.transformedPositions[mesh.indices[offset + 1]],
					mesh.transformedPositions[mesh.indices[offset + 2]],
					mesh.transformedNormals[i]
				};

				triangle.materialIndex = mesh.materialIndex;
				triangle.cullMode = mesh.cullMode;

				if (HitTest_Triangle(triangle, ray, hitRecord, ignoreHitRecord))
				{
					didHit = true;
				}
			}

			return didHit;
		}

		inline bool HitTest_TriangleMesh(const TriangleMesh& mesh, const Ray& ray)
		{
			static HitRecord temp{};
			return HitTest_TriangleMesh(mesh, ray, temp, true);
		}

#pragma endregion
	}

	namespace LightUtils
	{
		//Direction from target to light
		inline Vector3 GetDirectionToLight(const Light& light, const Vector3 origin)
		{
			//todo W3

			if (light.type == LightType::Point) return (light.origin - origin);
			else if (light.type == LightType::Directional) return -light.direction;

			return Vector3{};
		}

		inline ColorRGB GetRadiance(const Light& light, const Vector3& target)
		{
			//todo W3
			if (light.type == LightType::Point) return light.color * (light.intensity / GetDirectionToLight(light, target).SqrMagnitude());
			else if (light.type == LightType::Directional) return light.color * light.intensity;;

			return ColorRGB{};
		}
	}

	namespace Utils
	{
		//Just parses vertices and indices
#pragma warning(push)
#pragma warning(disable : 4505) //Warning unreferenced local function
		static bool ParseOBJ(const std::string& filename, std::vector<Vector3>& positions, std::vector<Vector3>& normals, std::vector<int>& indices)
		{
			std::ifstream file(filename);
			if (!file)
				return false;

			std::string sCommand;
			// start a while iteration ending when the end of file is reached (ios::eof)
			while (!file.eof())
			{
				//read the first word of the string, use the >> operator (istream::operator>>) 
				file >> sCommand;
				//use conditional statements to process the different commands	
				if (sCommand == "#")
				{
					// Ignore Comment
				}
				else if (sCommand == "v")
				{
					//Vertex
					float x, y, z;
					file >> x >> y >> z;
					positions.push_back({ x, y, z });
				}
				else if (sCommand == "f")
				{
					float i0, i1, i2;
					file >> i0 >> i1 >> i2;

					indices.push_back((int)i0 - 1);
					indices.push_back((int)i1 - 1);
					indices.push_back((int)i2 - 1);
				}
				//read till end of line and ignore all remaining chars
				file.ignore(1000, '\n');

				if (file.eof()) 
					break;
			}

			//Precompute normals
			for (uint64_t index = 0; index < indices.size(); index += 3)
			{
				uint32_t i0 = indices[index];
				uint32_t i1 = indices[index + 1];
				uint32_t i2 = indices[index + 2];

				Vector3 edgeV0V1 = positions[i1] - positions[i0];
				Vector3 edgeV0V2 = positions[i2] - positions[i0];
				Vector3 normal = Vector3::Cross(edgeV0V1, edgeV0V2);

				if(isnan(normal.x))
				{
					int k = 0;
				}

				normal.Normalize();
				if (isnan(normal.x))
				{
					int k = 0;
				}

				normals.push_back(normal);
			}

			return true;
		}
#pragma warning(pop)
	}
}