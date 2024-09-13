//顶点着色器的实现
#pragma once
#include "ShaderBase.h"

#include "Math.h"

namespace RGS {

    struct BlinnVertex : public VertexBase
    {
    };

    struct BlinnVaryings : public VaryingsBase
    {
    };

    struct BlinnUniforms : public UniformsBase
    {
    };

    //通过传入模型空间顶点和变换矩阵得到裁剪空间顶点值
    void BlinnVertexShader(BlinnVaryings& varyings, const BlinnVertex& vertex, const BlinnUniforms& uniforms);

}