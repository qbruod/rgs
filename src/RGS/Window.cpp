#include "RGS/Window.h"
#include "RGS/WindowsWindow.h"
#include "RGS/Base.h"

namespace RGS
{
	Window::Window(const std::string title, const int width, const int height)
		:m_Title(title),m_Width(width),m_Height(height)
	{
		ASSERT((m_Width > 0) && (m_Height > 0));
		memset(m_Keys, RGS_RELEASE, RGS_KEY_MAX_COUNT);
	}

	void Window::Init()
	{
		WindowsWindow::Init();
	}

	void Window::Terminate()
	{
		WindowsWindow::Terminate();
	}

	Window* Window::Create(const std::string title, const int width, const int height)
	{
		return new WindowsWindow(title, width, height);
	}

	void Window::PollInputEvents()
	{
		WindowsWindow::PollInputEvents();
	}
	//在.cpp文件中写static说明该函数仅在当前文件可见
	//在.h文件中该函数仅在定义它的源文件（.c 或 .cpp 文件）中可见,
	//可以避免与其他文件的同名函数发生冲突
}