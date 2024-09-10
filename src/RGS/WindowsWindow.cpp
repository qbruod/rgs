#include "RGS/Base.h"
#include "RGS/Window.h"
#include "RGS/WindowsWindow.h"

#include <Windows.h>
#include <iostream>

#define RGS_WINDOW_ENTRY_NAME  "Entry"
#define RGS_WINDOW_CLASS_NAME  "Class"

namespace RGS {
	bool WindowsWindow::s_Inited = false;

	void WindowsWindow::Init()
	{
		ASSERT(!s_Inited);
		Register();
		s_Inited = true;
	}

	void WindowsWindow::Terminate()
	{
		ASSERT(s_Inited);
		Unregister();
		s_Inited = false;
	}

	void WindowsWindow::Register()
	{
        ATOM atom;
        WNDCLASS wc = { 0 };
        wc.cbClsExtra = 0;
        wc.cbWndExtra = 0;
        wc.hbrBackground = (HBRUSH)(WHITE_BRUSH);  // 背景色
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);  // 默认光标
        wc.hIcon = LoadIcon(NULL, IDI_APPLICATION); // 默认图标
        wc.hInstance = GetModuleHandle(NULL);
        wc.lpfnWndProc = WindowsWindow::WndProc;   // 窗口处理函数
        wc.lpszClassName = RGS_WINDOW_CLASS_NAME;  // 窗口类名
        wc.style = CS_HREDRAW | CS_VREDRAW;        // 拉伸时重绘
        wc.lpszMenuName = NULL;                    // 不要菜单

        atom = RegisterClass(&wc);  // 注册窗口
        if (atom == 0) {
            DWORD error = GetLastError();
            std::cerr << "RegisterClass failed with error: " << error << std::endl;
        }
        ASSERT(atom != 0);
	}

    void WindowsWindow::Unregister()
    {
        UnregisterClass(RGS_WINDOW_CLASS_NAME, GetModuleHandle(NULL));
    }

    LRESULT CALLBACK WindowsWindow::WndProc(const HWND hWnd, const UINT msgID, const WPARAM wParam, const LPARAM lParam)
    {
        WindowsWindow* window = (WindowsWindow*)GetProp(hWnd, RGS_WINDOW_ENTRY_NAME);
        if (window == nullptr)
            return DefWindowProc(hWnd, msgID, wParam, lParam);

        switch (msgID)
        {
        case WM_DESTROY:
            window->m_Closed = true;
            return 0;
        case WM_KEYDOWN:
            KeyPressImpl(window, wParam, RGS_PRESS);
            return 0;
        case WM_KEYUP:
            KeyPressImpl(window, wParam, RGS_RELEASE);
            return 0;
        }
        return DefWindowProc(hWnd, msgID, wParam, lParam);
    }

    void WindowsWindow::Show() const
    {
        HDC windowDC = GetDC(m_Handle);
        //把在内存中画好的数据m_MemoryDC拷贝到电脑屏幕windowDC上
        BitBlt(windowDC, 0, 0, m_Width, m_Height, m_MemoryDC, 0, 0, SRCCOPY);   
        ShowWindow(m_Handle, SW_SHOW);
        ReleaseDC(m_Handle, windowDC);
    }

    WindowsWindow::WindowsWindow(const std::string title, const int width, const int height)
        :Window(title, width, height)
    {
        ASSERT((s_Inited), "未初始化，尝试 RGS::WindowsWindow::Init()");

        DWORD style = WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
        RECT rect;
        rect.left = 0;
        rect.top = 0;
        rect.bottom = (long)height;
        rect.right = (long)width;       //设置样式、大小
        AdjustWindowRect(&rect, style, false);
        m_Handle = CreateWindow(RGS_WINDOW_CLASS_NAME, m_Title.c_str(), style,
            CW_USEDEFAULT, 0, rect.right - rect.left, rect.bottom - rect.top,
            NULL, NULL, GetModuleHandle(NULL), NULL);
        if (m_Handle == nullptr) {
            DWORD error = GetLastError();
            std::cerr << "CreateWindow failed with error: " << error << std::endl;
        }
        ASSERT(m_Handle != nullptr);
        m_Closed = false;       //成功打开窗口

        SetProp(m_Handle, RGS_WINDOW_ENTRY_NAME, this);     //给m_Handle窗口粘贴附加属性
                          //粘贴属性名           粘贴属性值

        HDC windowDC = GetDC(m_Handle);     //获取设备上下文（获取m_Handle这个窗口设备的）
        m_MemoryDC = CreateCompatibleDC(windowDC);  // 创建与窗口设备上下文兼容的内存设备上下文m_MemoryDC

        BITMAPINFOHEADER biHeader = {};
        HBITMAP newBitmap;
        HBITMAP oldBitmap;

        biHeader.biSize = sizeof(BITMAPINFOHEADER);
        biHeader.biWidth = ((long)m_Width);
        biHeader.biHeight = -((long)m_Height);
        biHeader.biPlanes = 1;
        biHeader.biBitCount = 24;       //每个像素设置24个位
        biHeader.biCompression = BI_RGB;

        // 分配空间
        // https://learn.microsoft.com/zh-cn/windows/win32/api/wingdi/nf-wingdi-createdibsection
        //创建位图(为了在内存中存储图像数据，并在需要时将这些图像渲染到窗口上)
        newBitmap = CreateDIBSection(m_MemoryDC, (BITMAPINFO*)&biHeader, DIB_RGB_COLORS, (void**)&m_Buffer, nullptr, 0);
        ASSERT(newBitmap != nullptr);
        constexpr int channelCount = 3;
        int size = m_Width * m_Height * channelCount * sizeof(unsigned char);
        memset(m_Buffer, 0, size);  //把窗口画成黑色
        oldBitmap = (HBITMAP)SelectObject(m_MemoryDC, newBitmap);

        DeleteObject(oldBitmap);    //删除旧位图
        ReleaseDC(m_Handle, windowDC);      //释放上下文windowDC

        Show();
    }

    WindowsWindow::~WindowsWindow()
    {
        ShowWindow(m_Handle, SW_HIDE);      //隐藏窗口
        RemoveProp(m_Handle, RGS_WINDOW_ENTRY_NAME);        //删除m_Handle绑定的键值对（RGS_WINDOW_ENTRY_NAME，this）
        DeleteDC(m_MemoryDC);
        DestroyWindow(m_Handle);
    }

    void  WindowsWindow::PollInputEvents()
    {
        MSG message;
        while (PeekMessage(&message, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&message);
            DispatchMessage(&message);
        }
        //GetMessage 的场景: 适用于标准应用程序中，
        //主要关注消息处理，等待用户输入或系统消息。
        //PeekMessage 的场景 : 适用于需要定期更新应用程序状态
        //如动画或游戏）的场景，同时确保应用程序对用户输入做出及时响应。
    }

    void WindowsWindow::KeyPressImpl(WindowsWindow* window, const WPARAM wParam, const char state)
    {
        if (wParam >= '0' && wParam <= '9')
        {
            window->m_Keys[wParam] = state;
            return;
        }

        if (wParam >= 'A' && wParam <= 'Z')
        {
            window->m_Keys[wParam] = state;
            return;
        }

        switch (wParam)
        {
        case VK_SPACE:
            window->m_Keys[RGS_KEY_SPACE] = state;
            break;
        case VK_SHIFT:
            window->m_Keys[RGS_KEY_LEFT_SHIFT] = state;
            window->m_Keys[RGS_KEY_RIGHT_SHIFT] = state;
            break;
        default:
            break;
        }
    }

    void WindowsWindow::DrawFramebuffer(const Framebuffer& framebuffer)
    {
        const int fWidth = framebuffer.GetWidth();
        const int fHeight = framebuffer.GetHeight();
        const int width = m_Width < fWidth ? m_Width : fWidth;
        const int height = m_Height < fHeight ? m_Height : fHeight;
        //取小值，尽可能显示画面
        for (int i = 0; i < height; i++)
        {
            for (int j = 0;j < width;j++)
            {
                //因为Window的原点在左上角，framebuffer的原点在左下角
                //所以需要翻转RGB显示
                //window（也就是m_buffer）中的RGB是反的，也就是BGR
                constexpr int channelCount = 3;
                constexpr int rChannel = 2;
                constexpr int gChannel = 1;
                constexpr int bChannel = 0;

                Vec3 color = framebuffer.GetColor(j, fHeight - 1 - i);
                const int pixStart = (i * m_Width + j) * channelCount;
                const int rIndex = pixStart + rChannel;
                const int gIndex = pixStart + gChannel;
                const int bIndex = pixStart + bChannel;

                m_Buffer[rIndex] = Float2UChar(color.X);
                m_Buffer[gIndex] = Float2UChar(color.Y);
                m_Buffer[bIndex] = Float2UChar(color.Z);
            }
        }
        Show();
    }

}