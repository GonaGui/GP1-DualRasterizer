#include "pch.h"

#if defined(_DEBUG)
#include "vld.h"
#endif

#undef main
#include "Renderer.h"

#define ESC "\033["
#define YELLOW_TXT "33"
#define GREEN_TXT "32"
#define PURPLE_TXT "35"
#define CYAN_TXT "36"
#define RESET "\033[m"



using namespace dae;

void ShutDown(SDL_Window* pWindow)
{
	SDL_DestroyWindow(pWindow);
	SDL_Quit();
}

bool m_PrintFPS{};

int main(int argc, char* args[])
{
	//Unreferenced parameters
	(void)argc;
	(void)args;

	//Create window + surfaces
	SDL_Init(SDL_INIT_VIDEO);

	const uint32_t width = 640;
	const uint32_t height = 480;

	SDL_Window* pWindow = SDL_CreateWindow(
		"Dual Rasterizer - Gonçalo Guilherme/ 2DAE10",
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		width, height, 0);

	if (!pWindow)
		return 1;

	//Initialize "framework"
	const auto pTimer = new Timer();
	const auto pRenderer = new Renderer(pWindow);

	//CONSOLE MESSAGES

	std::cout << ESC << YELLOW_TXT << "m" << "[Key Bindings - SHARED]" << RESET << "\n";
	std::cout << ESC << YELLOW_TXT << "m" << "	[F1]  Toggle Rasterizer Mode(HARDWARE / SOFTWARE)	" << RESET << "\n";
	std::cout << ESC << YELLOW_TXT << "m" << "	[F3] Toggle FireFX(ON / OFF)" << RESET << "\n";
	std::cout << ESC << YELLOW_TXT << "m" << "	[F2]  Toggle Vehicle Rotation(ON / OFF)" << RESET << "\n";
	std::cout << ESC << YELLOW_TXT << "m" << "	[F9]  Cycle CullMode(BACK / FRONT / NONE)" << RESET << "\n";
	std::cout << ESC << YELLOW_TXT << "m" << "	[F10] Toggle Uniform ClearColor(ON / OFF)	" << RESET << "\n";
	std::cout << ESC << YELLOW_TXT << "m" << "	[F11] Toggle Print FPS(ON / OFF)" << RESET << "\n";
	std::cout << ESC << YELLOW_TXT << "m" << "	[F4] Exclusive (Point/Linear)" << RESET << "\n";

	std::cout << ESC << GREEN_TXT << "m" << "[Key Bindings - HARDWARE]" << RESET << "\n";
	std::cout << ESC << GREEN_TXT << "m" << "	[F4] Exclusive (ANISOTROPIC)" << RESET << "\n";

	std::cout << ESC << PURPLE_TXT << "m" << "[Key Bindings - SOFTWARE]" << RESET << "\n";
	std::cout << ESC << PURPLE_TXT << "m" << "	[F5] Cycle Shading Mode(COMBINED / OBSERVED_AREA / DIFFUSE / SPECULAR)"<< RESET << "\n";
	std::cout << ESC << PURPLE_TXT << "m" << "	[F6] Toggle NormalMap(ON / OFF)" << RESET << "\n";
	std::cout << ESC << PURPLE_TXT << "m" << "	[F7] Toggle DepthBuffer Visualization(ON / OFF)" << RESET << "\n";
	std::cout << ESC << PURPLE_TXT << "m" << "	[F8] Toggle BoundingBox Visualization(ON / OFF)" << RESET << "\n \n";

	std::cout << ESC << CYAN_TXT << "m" << "Extra Features: " << RESET << "\n";
	std::cout << ESC << CYAN_TXT << "m" << "	 Software transparency " << RESET << "\n";
	std::cout << ESC << CYAN_TXT << "m" << "	 Added Linear sampling (no anisotropic filtering) :(" << RESET << "\n \n \n";

	//Start loop
	pTimer->Start();
	float printTimer = 0.f;
	bool isLooping = true;
	while (isLooping)
	{
		//--------- Get input events ---------
		SDL_Event e;
		while (SDL_PollEvent(&e))
		{
			switch (e.type)
			{
			case SDL_QUIT:
				isLooping = false;
				break;
			case SDL_KEYUP:
				//Test for a key
				if (e.key.keysym.scancode == SDL_SCANCODE_F11)
				{
					m_PrintFPS = !m_PrintFPS;

					if (m_PrintFPS) std::cout << ESC << YELLOW_TXT << "m" << "(SHARED) " << "Printing FPS..." << RESET << "\n \n";

					else std::cout << ESC << YELLOW_TXT << "m" << "(SHARED) " << "...Stopping FPS print" << RESET << "\n \n";
				}
					

				pRenderer->ToggleOptions(e.key.keysym.scancode);
				break;
			default: ;
			}
		}

		//--------- Update ---------
		pRenderer->Update(pTimer);

		//--------- Render ---------
		pRenderer->Render();

		//--------- Timer ---------
		pTimer->Update();
		printTimer += pTimer->GetElapsed();
		if (printTimer >= 1.f)
		{
			if (m_PrintFPS)
			{
				printTimer = 0.f;
				std::cout << "dFPS: " << pTimer->GetdFPS() << std::endl;

			}
			
		}
	}
	pTimer->Stop();

	//Shutdown "framework"
	delete pRenderer;
	delete pTimer;

	ShutDown(pWindow);
	return 0;
}