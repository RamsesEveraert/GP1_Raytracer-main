#pragma once
#include <cassert>
#include <SDL_keyboard.h>
#include <SDL_mouse.h>

#include "Math.h"
#include "Timer.h"

namespace dae
{
	struct Camera
	{
		Camera() = default;

		Camera(const Vector3& _origin, float _fovAngle):
			origin{_origin},
			fovAngle{_fovAngle}
			
		{
			SetFOV(_fovAngle);
		}


		Vector3 origin{};
		float fovAngle{90.f};

		Vector3 forward{Vector3::UnitZ};
		Vector3 up{Vector3::UnitY};
		Vector3 right{Vector3::UnitX};

		float totalPitch{0.f};
		float totalYaw{0.f};

		Matrix cameraToWorld{};
		float fov{};

		void SetFOV(float fovAngle)
		{
			fov = tanf(TO_RADIANS * fovAngle * 0.5f);
		}


		Matrix CalculateCameraToWorld()
		{
			//todo: W2 
			
			//auto translationMatrix = Matrix::CreateTranslation(origin);
			//auto rotationMatrix = Matrix::CreateRotationX(totalPitch) * Matrix::CreateRotationY(totalYaw);

			cameraToWorld = Matrix::CreateRotationX(totalPitch) * Matrix::CreateRotationY(totalYaw) * Matrix::CreateTranslation(origin);
			return cameraToWorld;

		}


		void Update(Timer* pTimer)
		{
			const float deltaTime = pTimer->GetElapsed();

			//Keyboard Input
			const uint8_t* pKeyboardState = SDL_GetKeyboardState(nullptr);

			//Mouse Input
			int mouseX{}, mouseY{};
			const uint32_t mouseState = SDL_GetRelativeMouseState(&mouseX, &mouseY);

			bool needRecalculate{ false };

			Vector3 translationDelta = HandleKeyboardInput(pKeyboardState, deltaTime, cameraToWorld, forward, up);
			if (translationDelta != Vector3{})
				needRecalculate = true;

			HandleMouseInput(mouseX, mouseY, mouseState, totalYaw, totalPitch, origin, deltaTime, forward);

			if (needRecalculate)
				CalculateCameraToWorld();

			origin += translationDelta;
		}

		Vector3 HandleKeyboardInput(const uint8_t* pKeyboardState, float deltaTime, const Matrix& cameraToWorld, const Vector3& forward, const Vector3& up)
		{
			const float translateSpeed{ 10.f };
			int8_t zDirection{ pKeyboardState[SDL_SCANCODE_W] - pKeyboardState[SDL_SCANCODE_S] };
			int8_t xDirection{ pKeyboardState[SDL_SCANCODE_D] - pKeyboardState[SDL_SCANCODE_A] };
			int8_t yDirection{ pKeyboardState[SDL_SCANCODE_E] - pKeyboardState[SDL_SCANCODE_Q] };

			if (xDirection || yDirection || zDirection)
			{
				Vector3 localSpaceForward = cameraToWorld.TransformVector(forward).Normalized();
				Vector3 localSpaceRight = Vector3::Cross(up, localSpaceForward).Normalized();
				Vector3 localSpaceUp = Vector3::Cross(localSpaceForward, localSpaceRight);

				Vector3 translationDelta{};
				translationDelta += zDirection * localSpaceForward;
				translationDelta += xDirection * localSpaceRight;
				translationDelta += yDirection * localSpaceUp;

				return translationDelta * translateSpeed * deltaTime;
			}
			return Vector3{};
		}
		void HandleMouseInput(int mouseX, int mouseY, uint32_t mouseState, float& totalYaw, float& totalPitch, Vector3& origin, float deltaTime, const Vector3& forward)
		{
			const float rotateSpeed{ .5f };
			const float translateSpeed{ 10.f };

			bool isLeftMousePressed{ static_cast<bool>(mouseState & SDL_BUTTON(1)) && !static_cast<bool>(mouseState & SDL_BUTTON(3)) };
			bool isRightMousePressed{ static_cast<bool>(mouseState & SDL_BUTTON(3)) && !static_cast<bool>(mouseState & SDL_BUTTON(1)) };
			bool areBothMouseButtonsPressed{ static_cast<bool>(mouseState & SDL_BUTTON(1)) && static_cast<bool>(mouseState & SDL_BUTTON(3)) };

			bool isMouseMoving = mouseX != 0 || mouseY != 0;

			if ((isLeftMousePressed || isRightMousePressed) && isMouseMoving)
			{
				if (isLeftMousePressed)
				{
					origin += translateSpeed * mouseY * forward * deltaTime;
					totalYaw += rotateSpeed * mouseX * deltaTime;
				}

				if (isRightMousePressed)
				{
					totalPitch += rotateSpeed * mouseY * deltaTime;
					if (!isLeftMousePressed)
						totalYaw += rotateSpeed * mouseX * deltaTime;
				}
			}

			if (areBothMouseButtonsPressed && mouseY != 0)
			{
				Vector3 localSpaceUp = Vector3::Cross(cameraToWorld.TransformVector(forward).Normalized(), Vector3::Cross(up, cameraToWorld.TransformVector(forward)).Normalized());
				origin += translateSpeed * mouseY * localSpaceUp * deltaTime;
			}
		}


	};
}
