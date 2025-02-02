#pragma once
#include "Maths.h"

#include <cassert>
#include <cmath>

namespace dae
{
	namespace BRDF
	{
		/**
		 * \param kd Diffuse Reflection Coefficient
		 * \param cd Diffuse Color
		 * \return Lambert Diffuse Color
		 */
		inline static ColorRGB Lambert(float kd, const ColorRGB& cd)
		{
			//todo: W3

			ColorRGB perfectDiffuseReflectant{cd * kd};
			return (perfectDiffuseReflectant / static_cast<float>(PI));
		}

		inline static ColorRGB Lambert(const ColorRGB& kd, const ColorRGB& cd)
		{
			//todo: W3
			ColorRGB perfectDiffuseReflectant{ cd * kd };
			return (perfectDiffuseReflectant / static_cast<float>(PI));
		}

		/**
		 * \brief todo
		 * \param ks Specular Reflection Coefficient
		 * \param exp Phong Exponent
		 * \param l Incoming (incident) Light Direction
		 * \param v View Direction
		 * \param n Normal of the Surface
		 * \return Phong Specular Color
		 */
		inline static ColorRGB Phong(float ks, float exp, const Vector3& l, const Vector3& v, const Vector3& n)
		{
			//todo: W3
			Vector3 reflection{ Vector3::Reflect(l,n)};
			float cos{ Vector3::Dot(reflection, v) };
			float phong = ks * pow(cos, exp);

			if (cos < 0) return ColorRGB{};

			return ColorRGB(phong, phong, phong);
		}

		/**
		 * \brief BRDF Fresnel Function >> Schlick
		 * \param h Normalized Halfvector between View and Light directions
		 * \param v Normalized View direction
		 * \param f0 Base reflectivity of a surface based on IOR (Indices Of Refrection), this is different for Dielectrics (Non-Metal) and Conductors (Metal)
		 * \return
		 */
		inline static ColorRGB FresnelFunction_Schlick(const Vector3& h, const Vector3& v, const ColorRGB& f0)
		{
			//todo: W3
			const float base{ 1 - Vector3::Dot(h, v) };
			return f0 + (ColorRGB{1.f,1.f,1.f} - f0) * (base * base * base * base * base);
		}

		/**
		 * \brief BRDF NormalDistribution >> Trowbridge-Reitz GGX (UE4 implemetation - squared(roughness))
		 * \param n Surface normal
		 * \param h Normalized half vector
		 * \param roughness Roughness of the material
		 * \return BRDF Normal Distribution Term using Trowbridge-Reitz GGX
		 */
		inline static float NormalDistribution_GGX(const Vector3& n, const Vector3& h, float roughness)
		{
			//todo: W3
			const float dotProduct{ Vector3::Dot(n, h) };
			const float alpha{ roughness * roughness };
			const float base{ dotProduct * dotProduct * (alpha * alpha - 1) + 1 };

			return (alpha * alpha) / (static_cast<float>(PI) * base * base);
		}


		/**
		 * \brief BRDF Geometry Function >> Schlick GGX (Direct Lighting + UE4 implementation - squared(roughness))
		 * \param n Normal of the surface
		 * \param v Normalized view direction
		 * \param roughness Roughness of the material
		 * \return BRDF Geometry Term using SchlickGGX
		 */
		inline static float GeometryFunction_SchlickGGX(const Vector3& n, const Vector3& v, float roughness)
		{
			//todo: W3
			const float normalViewDot = Vector3::Dot(n, v);

			if (normalViewDot < 0.0f) return 0.0f;

			const float roughnessSquared{ roughness * roughness };
			const float ggxAlpha{ roughnessSquared + 1 };
			const float k = ggxAlpha * ggxAlpha / 8;

			return normalViewDot / (normalViewDot * (1 - k) + k);
		}

		/**
		 * \brief BRDF Geometry Function >> Smith (Direct Lighting)
		 * \param n Normal of the surface
		 * \param v Normalized view direction
		 * \param l Normalized light direction
		 * \param roughness Roughness of the material
		 * \return BRDF Geometry Term using Smith (> SchlickGGX(n,v,roughness) * SchlickGGX(n,l,roughness))
		 */
		inline static float GeometryFunction_Smith(const Vector3& n, const Vector3& v, const Vector3& l, float roughness)
		{
			//todo: W3
			return GeometryFunction_SchlickGGX(n, v, roughness) * GeometryFunction_SchlickGGX(n, l, roughness);
		}

	}
}