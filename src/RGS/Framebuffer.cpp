#pragma once
#include "RGS/Base.h"
#include "RGS/Framebuffer.h"

namespace RGS {
	Framebuffer::Framebuffer(const int width, const int height)
		:m_Width(width), m_Height(height)
	{
		ASSERT((width > 0) && (height>0));
		m_PixelSize = m_Width * m_Height;
		m_ColorBuffer = new Vec3[m_PixelSize]();//用于存储颜色信息
		//分配一个大小为m_PixelSize的动态数组（因为程序运行前m_PixelSize
		//大小并不可知）。每个元素都是一个Vec3对象（RGB）
		m_DepthBuffer = new float[m_PixelSize]();
		Clear({ 0.0f,0.0f,0.0f });
		ClearDepth(1.0f);
	}

	Framebuffer::~Framebuffer()
	{
		delete[] m_ColorBuffer;
		delete[] m_DepthBuffer;
		m_ColorBuffer = nullptr;
		m_DepthBuffer = nullptr;//避免空悬指针
		//当你使用 delete[] 释放动态数组的内存时，
		//指针仍然指向原来的内存地址，但该内存已经被释放，
		//不能再合法地访问
	}
	
	void Framebuffer::SetColor(const int x, const int y, const Vec3& color)
	{
		if ((x < 0) || (x >= m_Width) || (y < 0) || (y >= m_Height))
		{
			ASSERT(false);
			return;
		}
		else
		{
			int index = GetPixelIndex(x, y);
			m_ColorBuffer[index] = color;
		}
	}

	Vec3 Framebuffer::GetColor(const int x, const int y) const
	{
		int index = GetPixelIndex(x, y);
		if (index < m_PixelSize && index >= 0)
		{
			return m_ColorBuffer[index];
		}
		else
		{
			ASSERT(false);
			return { 0.0f,0.0f,0.0f };
		}
	}

	void Framebuffer::SetDepth(const int x, const int y, const float depth)
	{
		int index = GetPixelIndex(x, y);
		if (index < m_PixelSize && index >= 0)
		{
			m_DepthBuffer[index] = depth;
		}
		else
		{
			ASSERT(false);
		}
	}

	float Framebuffer::GetDepth(const int x, const int y)const
	{
		int index = GetPixelIndex(x, y);
		if (index < m_PixelSize && index >= 0)
		{
			return m_DepthBuffer[index];
		}
		else
		{
			ASSERT(false);
		}
	}

	void Framebuffer::Clear(const Vec3& color)
	{
		for (int i = 0;i < m_PixelSize;i++)
		{
			m_ColorBuffer[i] = color;
		}
	}

	void Framebuffer::ClearDepth(float depth)
	{
		for (int i = 0;i < m_PixelSize;i++)
		{
			m_DepthBuffer[i] = depth;
		}
	}

	
}