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
		float aspectRatio{};
		float fovAngle{ 45.f };
		float previousfovAngle{ fovAngle };
		float FOV{};
		float rotSpeed{ 3 };
		float movSpeed{ 100 };
		float nearPlane{ .1f };
		float farPlane{100.f};

		Vector3 forward{Vector3::UnitZ};
		Vector3 up{Vector3::UnitY};
		Vector3 right{Vector3::UnitX};

		float totalPitch{};
		float totalYaw{};

		Matrix invViewMatrix{};
		Matrix viewMatrix{};
		Matrix projectionMatrix{};

		void Initialize(float _fovAngle = 90.f, Vector3 _origin = {0.f,0.f,0.f},float AspectRatio = 9/3)
		{
			fovAngle = _fovAngle;
			FOV = tan((fovAngle * (PI / 180)) / 2);

			origin = _origin;

			aspectRatio = AspectRatio;
		}

		void CalculateViewMatrix()
		{
			right = Vector3::Cross(Vector3::UnitY, forward).Normalized();
			up = Vector3::Cross(forward, right).Normalized();

			invViewMatrix = Matrix(right, up, forward, origin);

			viewMatrix = Matrix::Inverse(invViewMatrix); 
		}


		void CalculateProjectionMatrix()
		{
			projectionMatrix = Matrix::CreatePerspectiveFovLH(FOV,aspectRatio , nearPlane, farPlane);
			//DirectX Implementation => https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dxmatrixperspectivefovlh
		}

		const float* GetOrigin() const
		{
			return reinterpret_cast<const float*>(&origin);
		}

		void Update(Timer* pTimer)
		{
			SDL_SetRelativeMouseMode(SDL_TRUE);

			if (fovAngle != previousfovAngle)
			{
				FOV = tan((fovAngle * (PI / 180)) / 2);

				previousfovAngle = fovAngle;
			}

			const float deltaTime = pTimer->GetElapsed();

			//Keyboard Input
			const uint8_t* pKeyboardState = SDL_GetKeyboardState(nullptr);

			//Mouse Input
			int mouseX{}, mouseY{};
			const uint32_t mouseState = SDL_GetRelativeMouseState(&mouseX, &mouseY);

			Vector3 forwardVec = forward.Normalized() * movSpeed;

			Vector3 rightVector = right * movSpeed ;

			Vector3 upVector = up * movSpeed;

			// Process mouse movements (relative to the last frame)
			if (mouseState & SDL_BUTTON(SDL_BUTTON_RIGHT) && mouseState & SDL_BUTTON(SDL_BUTTON_LEFT))
			{
				if (mouseY > 0)
				{
					origin -= upVector * 3 * deltaTime;
				}

				if (mouseY < 0)
				{
					origin += upVector * 3 * deltaTime;
				}
			}

			else if (mouseState & SDL_BUTTON(SDL_BUTTON_RIGHT))
			{
				if (mouseY > 0)
				{
					totalPitch += rotSpeed * deltaTime;
				}

				if (mouseY < 0)
				{
					totalPitch -= rotSpeed * deltaTime;
				}
				
				if (mouseX > 0)
				{
					totalYaw += rotSpeed * deltaTime;
				}

				if (mouseX < 0)
				{
					totalYaw -= rotSpeed * deltaTime;
				}
			}

			else if (mouseState & SDL_BUTTON(SDL_BUTTON_LEFT))
			{

				if (mouseY > 0)
				{
					origin -= (forwardVec * 3) * deltaTime;
				}

				if (mouseY < 0)
				{
					origin += (forwardVec * 3) * deltaTime;
				}

				if (mouseX > 0)
				{
					totalYaw += rotSpeed * deltaTime;
				}

				if (mouseX < 0)
				{
					totalYaw -= rotSpeed * deltaTime;
				}
				
			}

			if (pKeyboardState[SDL_SCANCODE_W])
			{
				origin += forwardVec * deltaTime;
			}

			if (pKeyboardState[SDL_SCANCODE_S])
			{
				origin -= forwardVec * deltaTime;
			}

			if (pKeyboardState[SDL_SCANCODE_D])
			{
				origin += rightVector * deltaTime;
			}

			if (pKeyboardState[SDL_SCANCODE_A])
			{
				origin -= rightVector * deltaTime;
			}

			if (pKeyboardState[SDL_SCANCODE_SPACE])
			{
				origin += upVector * deltaTime;
			}

			if (pKeyboardState[SDL_SCANCODE_LSHIFT])
			{
				movSpeed = 200;
			}

			else
			{
				movSpeed = 100;
			}

			Matrix totalRotation = Matrix::CreateRotation(Vector3(-totalPitch, totalYaw, 0));
			forward = totalRotation.TransformVector(Vector3::UnitZ);
			Camera::forward.Normalize();

			//Update Matrices
			CalculateViewMatrix();
			CalculateProjectionMatrix(); //Try to optimize this - should only be called once or when fov/aspectRatio changes
		}

	};
}
