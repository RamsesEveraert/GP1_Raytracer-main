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
		}


		Vector3 origin{};
		float fovAngle{90.f};

		Vector3 forward{Vector3::UnitZ};
		Vector3 up{Vector3::UnitY};
		Vector3 right{Vector3::UnitX};

		float totalPitch{0.f};
		float totalYaw{0.f};

		Matrix cameraToWorld{};


		Matrix CalculateCameraToWorld()
		{
			//todo: W2 
			
			auto translationMatrix = Matrix::CreateTranslation(origin);
			auto rotationMatrix = Matrix::CreateRotationX(totalPitch) * Matrix::CreateRotationY(totalYaw);

			cameraToWorld = rotationMatrix * translationMatrix;
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

			//todo: W2

			const float translateSpeed{ 10.f };
			const float rotateSpeed{ .5f };

			// Calculate movement directions based on keyboard input
			int8_t zDirection{ pKeyboardState[SDL_SCANCODE_W] - pKeyboardState[SDL_SCANCODE_S] };
			int8_t xDirection{ pKeyboardState[SDL_SCANCODE_D] - pKeyboardState[SDL_SCANCODE_A] };
			int8_t yDirection{ pKeyboardState[SDL_SCANCODE_E] - pKeyboardState[SDL_SCANCODE_Q] };

			bool isTranslating{ xDirection != 0 || yDirection != 0 || zDirection != 0 };

			Vector3 localSpaceForward = cameraToWorld.TransformVector(forward).Normalized();
			Vector3 localSpaceRight = Vector3::Cross(up, localSpaceForward).Normalized();
			Vector3 localSpaceUp = Vector3::Cross(localSpaceForward, localSpaceRight);

			if (isTranslating)
			{
				origin += zDirection * localSpaceForward * translateSpeed * deltaTime;
				origin += xDirection * localSpaceRight * translateSpeed * deltaTime;
				origin += yDirection * localSpaceUp * translateSpeed * deltaTime;
			}

			
			// https://wiki.libsdl.org/SDL2/SDL_GetRelativeMouseState

			bool isLeftMousePressed{ static_cast<bool>(mouseState & SDL_BUTTON(1)) && !static_cast<bool>(mouseState & SDL_BUTTON(3)) };
			bool isRightMousePressed{ static_cast<bool>(mouseState & SDL_BUTTON(3)) && !static_cast<bool>(mouseState & SDL_BUTTON(1)) };
			bool areBothMouseButtonsPressed{ static_cast<bool>(mouseState & SDL_BUTTON(1)) && static_cast<bool>(mouseState & SDL_BUTTON(3)) };

			bool isMouseMoving{ mouseX != 0.0f || mouseY != 0.0f };

			bool isDraggingUp{ areBothMouseButtonsPressed && mouseY != 0.0f };

			if (isLeftMousePressed && isMouseMoving)
			{
				// Move camera forward or backward and rotate based on mouse movement
				origin += mouseY * forward * translateSpeed * deltaTime;
				totalYaw += mouseX * rotateSpeed * deltaTime;
			}

			if (isRightMousePressed && isMouseMoving)
			{
				// rotate camera based on mouse movement
				totalPitch += mouseY * rotateSpeed * deltaTime;
				totalYaw += mouseX * rotateSpeed * deltaTime;
			}

			// Move camera up or down based on mouse movement
			if (isDraggingUp)
			{
				origin += mouseY * localSpaceUp * translateSpeed * deltaTime;
			}

			
			bool canRotate{(isLeftMousePressed && isMouseMoving) || (isRightMousePressed && isMouseMoving)};

			if (isTranslating || canRotate)
			{
				CalculateCameraToWorld();
			}
		}
	};
}
