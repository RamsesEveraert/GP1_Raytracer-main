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


			if (discriminant >= 0)
			{
				float sqrtDiscriminant = sqrtf(discriminant);

				// Calculate both intersection points
				float t1 = (-b - sqrtDiscriminant) / (2 * a);

				if (t1 >= ray.min && t1 <= ray.max)
				{

					if (!ignoreHitRecord && t1 < hitRecord.t)
					{
						hitRecord.t = t1;
						hitRecord.didHit = true;
						hitRecord.origin = ray.origin + t1 * ray.direction;
						hitRecord.normal = (hitRecord.origin - sphere.origin).Normalized();
						hitRecord.materialIndex = sphere.materialIndex;
					}

					return true;
				}
				float t2 = (-b + sqrtDiscriminant) / (2 * a);
				if (t2 >= ray.min && t2 <= ray.max)
				{
					if (!ignoreHitRecord && t2 < hitRecord.t)
					{
						hitRecord.t = t2;
						hitRecord.didHit = true;
						hitRecord.origin = ray.origin + t2 * ray.direction;
						hitRecord.normal = (hitRecord.origin - sphere.origin).Normalized();
						hitRecord.materialIndex = sphere.materialIndex;
					}

					return true;
				}
				
			}

			return false;
		
		}

		inline bool HitTest_Sphere(const Sphere& sphere, const Ray& ray)
		{
			HitRecord temp{};
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
			const Vector3 edge1 = triangle.v1 - triangle.v0;
			const Vector3 edge2 = triangle.v2 - triangle.v0;

			const Vector3 perpendicularVector = Vector3::Cross(ray.direction, edge2);
			const float determinant = Vector3::Dot(edge1, perpendicularVector);

			// Check if ray is parallel to the triangle
			if (dae::AreEqual(determinant, 0.f))
				return false;

			const float inverseDet = 1.0f / determinant;
			const Vector3 distanceToTriangle = ray.origin - triangle.v0;
			const float u = inverseDet * Vector3::Dot(distanceToTriangle, perpendicularVector);

			if (u < 0.0f || u > 1.0f)
				return false;

			const Vector3 crossVec = Vector3::Cross(distanceToTriangle, edge1);
			const float v = inverseDet * Vector3::Dot(ray.direction, crossVec);

			if (v < 0.0f || u + v > 1.0f)
				return false;

			// intersection distance along the ray's direction
			const float t = inverseDet * Vector3::Dot(edge2, crossVec);

			// intersection is in the ray's positive direction
			if (t <= FLT_EPSILON)
				return false;

			if (!ignoreHitRecord && t < hitRecord.t)
			{
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
		inline bool HitTest_TriangleMesh(const TriangleMesh& mesh, const Ray& ray, HitRecord& hitRecord, bool ignoreHitRecord = false)
		{
			//todo W5
			assert(false && "No Implemented Yet!");
			return false;
		}

		inline bool HitTest_TriangleMesh(const TriangleMesh& mesh, const Ray& ray)
		{
			HitRecord temp{};
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