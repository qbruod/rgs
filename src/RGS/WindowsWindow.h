#pragma once
#include <string>
#include <Windows.h>
#include "RGS/Window.h"

namespace RGS
{
	class WindowsWindow :public Window
	{
	public:
		WindowsWindow(const std::string title, const int width, const int height);
		~WindowsWindow();

		virtual void Show() const override;
		virtual void DrawFramebuffer(const Framebuffer& framebuffer) override;

	public:
		static void Init();
		static void Terminate();

		static void PollInputEvents();	//分发事件的方法

	private:
		static void Register();
		static void Unregister();

		static LRESULT CALLBACK WndProc(const HWND hWnd,const UINT msgID,const WPARAM wParam,const LPARAM lParam);//窗口处理函数
		static void KeyPressImpl(WindowsWindow* window, const WPARAM wParam, const char state);
	private:
		HWND m_Handle;
		HDC m_MemoryDC;
		unsigned char* m_Buffer;
		static bool s_Inited;
	};


}