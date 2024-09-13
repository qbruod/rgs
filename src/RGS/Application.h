#pragma once
#include <string>
#include "RGS/Window.h"
#include "RGS/InputCodes.h"

namespace RGS
{
	class Application
	{
	public:
		Application(const std::string name, const int width, const int height);
		~Application();

		void Run();
	private:
		void Init();
		void Terminate();

		void OnUpdate();

	private:
		std::string m_Name;
		int m_Width, m_Height;

		Window* m_Window;
	};
}