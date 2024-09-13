#pragma once
#include "RGS/Maths.h"
#include <string>

namespace RGS {

    class Texture
    {
    public:
        //选择纹理图片
        Texture(const std::string& path);
        ~Texture();
        //采样 输入纹理坐标返回颜色
        Vec4 Sample(Vec2 texCoords) const;
    private:
        void Init();

    private:
        //m_Channels标识通道
        int m_Width, m_Height, m_Channels;
        std::string m_Path;
        Vec4* m_Data = nullptr;
    };

}