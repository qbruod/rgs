//创建帧缓存
#pragma once
#include "RGS/Maths.h"

namespace RGS {
	class Framebuffer
	{
    public:
        Framebuffer(const int width, const int height);
        ~Framebuffer();

        int GetWidth() const { return m_Width; }
        int GetHeight() const { return m_Height; }

        void SetColor(const int x, const int y, const Vec3& color);
        Vec3 GetColor(const int x, const int y) const;
        void SetDepth(const int x, const int y, const float depth);
        float GetDepth(const int x, const int y) const;

        void Clear(const Vec3& color = { 0.0f, 0.0f, 0.0f });
        //用输入的颜色清除画布
        void ClearDepth(float depth = 1.0f);
        //用1.0f这个深度值填充深度
    private:
        int GetPixelIndex(const int x, const int y) const { return y * m_Width + x; }
        //通过计算 y * m_Width + x 来确定像素在一维数组中的索引值(二维数组->一维数组位置）
    private:
        int m_Width = 800;
        int m_Height = 600;
        int m_PixelSize;
        float* m_DepthBuffer;
        Vec3* m_ColorBuffer;
	};
}