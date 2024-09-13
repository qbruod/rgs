//顶点着色器的实现
#pragma once
#include "ShaderBase.h"

#include "RGS/Maths.h"
#include "RGS/Texture.h"

#include <memory>
#include <iostream>

namespace RGS {

    struct BlinnVertex : public VertexBase
    {
        Vec3 ModelNormal;
        Vec2 TexCoord = { 0.0f,0.0f };


    };

    struct BlinnVaryings : public VaryingsBase
    {
        //获取点在世界空间坐标WorldPos和对应点的法线WorldNormal
        Vec3 WorldPos;
        Vec3 WorldNormal;
        Vec2 TexCoord;
    };

    struct BlinnUniforms : public UniformsBase
    {
        Mat4 Model;
        Mat4 ModelNormalToWorld;
        //光的位置
        Vec3 LightPos{ 0.0f, 1.0f, 2.0f };
        //环境光(Ambient)
        Vec3 LightAmbient{ 0.2f, 0.2f, 0.2f };
        //漫反射(Diffuse)颜色
        Vec3 LightDiffuse{ 0.5f, 0.5f, 0.5f };
        //镜面(Specular)光照颜色
        Vec3 LightSpecular{ 1.0f, 1.0f, 1.0f };
        //物体固有色
        Vec3 ObjectColor{ 1.0f, 1.0f, 1.0f };
        //眼睛位置
        Vec3 CameraPos;
        //高光度
        float Shininess = 2.0f;

        //漫反射纹理
        Texture* Diffuse = nullptr;
        //镜面纹理
        Texture* Specular = nullptr;
    };

    //通过传入模型空间顶点和变换矩阵得到裁剪空间顶点值
    void BlinnVertexShader(BlinnVaryings& varyings, const BlinnVertex& vertex, const BlinnUniforms& uniforms);

    Vec4 BlinnFragmentShader(bool& discard, const BlinnVaryings& varyings, const BlinnUniforms& uniforms);
}