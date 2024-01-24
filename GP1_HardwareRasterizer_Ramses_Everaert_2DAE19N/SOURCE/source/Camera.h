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

		float nearPlane{0.5f};
		float farPlane{1000.f};

		float aspectRatio{};

		float rotationSpeed{ 0.01f };
		float moveSpeed{ 5.f };
		float dragSpeed{ 0.5f };

		Matrix projectionMatrix{};
		Matrix viewMatrix{};

		void Initialize(float _aspectratio, float _fovAngle = 90.f, Vector3 _origin = {0.f,0.f,0.f}, float _near = 0.5f, float _far = 1000.f)
		{
			fovAngle = _fovAngle;
			fov = tanf((fovAngle * TO_RADIANS) / 2.f);

			aspectRatio = _aspectratio;
			origin = _origin;

			nearPlane = _near;
			farPlane = _far;
		}

		void CalculateViewMatrix()
		{
			Matrix rotationMatrix = Matrix::CreateRotationX(totalPitch) * Matrix::CreateRotationY(totalYaw);
			forward = rotationMatrix.GetAxisZ();
			right = rotationMatrix.GetAxisX();
			up = rotationMatrix.GetAxisY();

			// Use the target to create the look-at matrix.
			viewMatrix = Matrix::CreateLookAtLH(origin, origin + forward, Vector3::UnitY);

			//DirectX Implementation => https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dxmatrixlookatlh
		}

		 
		void CalculateProjectionMatrix()
		{
			projectionMatrix =  Matrix::CreatePerspectiveFovLH(fov, aspectRatio, nearPlane, farPlane);
			//DirectX Implementation => https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dxmatrixperspectivefovlh
		}

		void Update(const Timer* pTimer)
		{
			//Camera Update Logic
			HandleInputs(pTimer);

			//Update Matrices
			CalculateViewMatrix();
			CalculateProjectionMatrix(); //Try to optimize this - should only be called once or when fov/aspectRatio changes

		}

		void HandleInputs(const Timer* pTimer)
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

			float currentMoveSpeed = moveSpeed;
			if (pKeyboardState[SDL_SCANCODE_LSHIFT])
			{
				currentMoveSpeed *= 2.f; 
			}

			origin += zDirection  * currentMoveSpeed * forward * dt;
			origin += xDirection  * currentMoveSpeed * right * dt;
			origin += yDirection  * currentMoveSpeed * up * dt;
		}

		void HandleMouseInput(float dt)
		{
			int mouseX, mouseY;
			const uint32_t mouseState = SDL_GetRelativeMouseState(&mouseX, &mouseY);

			bool isLeftMousePressed{ static_cast<bool>(mouseState & SDL_BUTTON(1)) && !static_cast<bool>(mouseState & SDL_BUTTON(3)) }; // prevent double mouse button click
			bool isRightMousePressed{ static_cast<bool>(mouseState & SDL_BUTTON(3)) && !static_cast<bool>(mouseState & SDL_BUTTON(1)) }; // Prevent double mouse button click
			bool areBothMouseButtonsPressed{ static_cast<bool>(mouseState & SDL_BUTTON(1)) && static_cast<bool>(mouseState & SDL_BUTTON(3)) };

			SDL_SetRelativeMouseMode(static_cast<SDL_bool>(isLeftMousePressed || isRightMousePressed)); // https://metacpan.org/pod/SDL2::mouse

			if (areBothMouseButtonsPressed)
			{
				origin -= mouseY * dragSpeed * Vector3::UnitY;
			}
			else if (isRightMousePressed)
			{
				totalYaw += mouseX * rotationSpeed;
				totalPitch -= mouseY * rotationSpeed;
			}
			else if (isLeftMousePressed)
			{
				origin -= mouseY * dragSpeed * forward;
				totalYaw += mouseX * rotationSpeed;
			}
		}

		Matrix GetViewMatrix() const
		{
			return viewMatrix;
		}

		Matrix GetProjectionMatrix() const
		{
			return projectionMatrix;
		}

	};
}
