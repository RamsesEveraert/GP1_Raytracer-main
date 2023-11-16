#pragma once
#include <cassert>
#include <SDL_keyboard.h>
#include <SDL_mouse.h>

#include "Maths.h"
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
		float fov{ tanf((fovAngle * TO_RADIANS) / 2.f) };

		Vector3 forward{Vector3::UnitZ};
		Vector3 up{Vector3::UnitY};
		Vector3 right{Vector3::UnitX};

		float totalPitch{};
		float totalYaw{};

		float rotationSpeed{ 0.01f };
		float moveSpeed{ 10.f };
		float dragSpeed{ 5.0f };

		Matrix invViewMatrix{};
		Matrix viewMatrix{};

		void Initialize(float _fovAngle = 90.f, Vector3 _origin = {0.f,0.f,0.f})
		{
			fovAngle = _fovAngle;
			fov = tanf((fovAngle * TO_RADIANS) / 2.f);

			origin = _origin;
		}

		void CalculateViewMatrix()
		{
			//TODO W1
			//ONB => invViewMatrix
			//Inverse(ONB) => ViewMatrix

			//ViewMatrix => Matrix::CreateLookAtLH(...) [not implemented yet]
			//DirectX Implementation => https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dxmatrixlookatlh
		}

		void CalculateProjectionMatrix()
		{
			//TODO W3

			//ProjectionMatrix => Matrix::CreatePerspectiveFovLH(...) [not implemented yet]
			//DirectX Implementation => https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dxmatrixperspectivefovlh
		}

		void Update(Timer* pTimer)
		{
			

			//Camera Update Logic
			HandleInputs(pTimer);

			//Update Matrices
			CalculateViewMatrix();
			CalculateProjectionMatrix(); //Try to optimize this - should only be called once or when fov/aspectRatio changes

		}

		void HandleInputs(Timer* pTimer)
		{
			const float dt = pTimer->GetElapsed();
			HandleKeyboardInput(dt);
			HandleMouseInput(dt);
		}

		void HandleKeyboardInput(float dt)
		{
			const uint8_t* pKeyboardState = SDL_GetKeyboardState(nullptr);

			int8_t zDirection{ pKeyboardState[SDL_SCANCODE_W] - pKeyboardState[SDL_SCANCODE_S] };
			int8_t xDirection{ pKeyboardState[SDL_SCANCODE_D] - pKeyboardState[SDL_SCANCODE_A] };
			int8_t yDirection{ pKeyboardState[SDL_SCANCODE_E] - pKeyboardState[SDL_SCANCODE_Q] };

			origin += zDirection * moveSpeed * forward * dt;
			origin += xDirection * moveSpeed * right * dt;
			origin += yDirection * moveSpeed * up * dt;
		}

		void HandleMouseInput(float dt)
		{
			int mouseX, mouseY;
			const uint32_t mouseState = SDL_GetRelativeMouseState(&mouseX, &mouseY);

			//Mouse buttons
			bool isLeftMousePressed{ static_cast<bool>(mouseState & SDL_BUTTON(1)) && !static_cast<bool>(mouseState & SDL_BUTTON(3)) }; // prevent double mouse button click
			bool isRightMousePressed{ static_cast<bool>(mouseState & SDL_BUTTON(3)) && !static_cast<bool>(mouseState & SDL_BUTTON(1)) }; // Prevent double mouse button click
			bool areBothMouseButtonsPressed{ static_cast<bool>(mouseState & SDL_BUTTON(1)) && static_cast<bool>(mouseState & SDL_BUTTON(3)) };

			SDL_SetRelativeMouseMode(static_cast<SDL_bool>(isLeftMousePressed || isRightMousePressed)); // https://metacpan.org/pod/SDL2::mouse

			if (areBothMouseButtonsPressed)
			{
				origin -= mouseY * dragSpeed * Vector3::UnitY * dt;
			}
			else if (isRightMousePressed)
			{
				totalYaw += mouseX * rotationSpeed;
				totalPitch -= mouseY * rotationSpeed;
			}
			else if (isLeftMousePressed)
			{
				origin -= mouseY * dragSpeed * forward * dt;
				totalYaw += mouseX * rotationSpeed;
			}
		}

	};
}
