#pragma once
#include "RGS/Maths.h"
#include <string>

namespace RGS {
	//初始模型空间顶点坐标数据
	struct VertexBase
	{
		Vec4 ModelPos = { 0, 0, 0, 1 };

	};

	//数据输入到Vertex Shader后得到的裁剪空间位置数据
	struct  VaryingsBase
	{
		Vec4 ClipPos = { 0.0f,0.0f,0.0f,1.0f };
		//存储齐次变换后的值
		Vec4 NdcPos = { 0.0f,0.0f,0.0f,1.0f };
		//存储视口变换后的值
		Vec4 FragPos = { 0.0f,0.0f,0.0f,1.0f };
	};

	struct UniformsBase
	{
		Mat4 MVP;//模型空间->裁剪空间用到的所有的变换矩阵乘到一起叫MVP
		//Model->view->Projection,但是代码是左乘的
		//所以具体实现时写PVM*顶点坐标
		operator const std::string() const { return (std::string)MVP; }
	};
	
}
