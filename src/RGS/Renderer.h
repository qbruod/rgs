#pragma once
#include "RGS/Framebuffer.h"
#include "RGS/Maths.h"
#include "Base.h"

#include <type_traits>
#include <cmath>

namespace RGS {

	template<typename vertex_t>
	struct Triangle
	{
		static_assert(std::is_base_of_v<VertexBase, vertex_t>, "vertex_t 必须继承自RGS::VertexBase");
		//三个顶点
		vertex_t Vertex[3];
		//非 const 版本 用于非常量对象，允许通过下标访问并修改对象。
		//const 版本 用于const类型的常量对象，保证只能访问，不能修改对象的内部数据。
		vertex_t& operator[](size_t i)
		{
			return Vertex[i];
		}

		const vertex_t& operator[](size_t i)const
		{
			return Vertex[i];
		}
		//构造函数使用默认版本
		Triangle() = default;
		
	};

	//因为不同顶点着色器输入的数据结构可能不一样，所以用模板
	template<typename vertex_t,typename uniforms_t,typename varyings_t>
	struct Program
	{
		//using定义函数指针类型void (*)(varyings_t&, const vertex_t&, const uniforms_t&)的别名Vertex_shader_t
		//此函数的签名为void，varyings_t，const vertex_t，const uniforms_t
		using vertex_shader_t = void (*)(varyings_t&, const vertex_t&, const uniforms_t&);

		//像调用函数一样调用VertexShader。如：VertexShader(varying, vertex, uniforms);
		//这句话意思等价于void (*VertexShader)(varyings_t&, const vertex_t&, const uniforms_t&);
		vertex_shader_t VertexShader;

		Program(const vertex_shader_t vertexShader)
			: VertexShader(vertexShader) {}
	};

	class Renderer
	{
	private:
		//三角形有3个顶点，裁剪空间的视锥体是一个6面体。3条边被6面体裁剪后最多变为9边形
		//这里只讨论凸多边形（内角小于180度）
		static constexpr int RGS_MAX_VARYINGS = 9;

	private:

		//枚举类型，枚举所有平面
		enum class Plane
		{
			POSITIVE_W,
			POSITIVE_X,
			NEGATIVE_X,
			POSITIVE_Y,
			NEGATIVE_Y,
			POSITIVE_Z,
			NEGATIVE_Z,
		};



		static bool IsVertexVisible(const Vec4& clipPos)
		{
			//判断顶点是否在视锥体内只需要判断X/Y/Z的绝对值÷W是否小于1
			return std::fabs(clipPos.X) <= clipPos.W && std::fabs(clipPos.Y) <= clipPos.W && std::fabs(clipPos.Z) <= clipPos.W;
		}


		static bool IsInsidePlane(const Vec4& clipPos, const Plane plane)
		{
			switch (plane)
			{
			//W一定为正，否则无意义
			case Plane::POSITIVE_W:
				return clipPos.W >= 0.0f;
			case Plane::POSITIVE_X:
				return clipPos.X <= +clipPos.W;
			case Plane::NEGATIVE_X:
				return clipPos.X >= -clipPos.W;
			case Plane::POSITIVE_Y:
				return clipPos.Y <= +clipPos.W;
			case Plane::NEGATIVE_Y:
				return clipPos.Y >= -clipPos.W;
			case Plane::POSITIVE_Z:
				return clipPos.Z <= +clipPos.W;
			case Plane::NEGATIVE_Z:
				return clipPos.Z >= -clipPos.W;
			default:
				ASSERT(false);
				return false;
			}
		}


		//利用相似三角形求出比例
		static float GetIntersectRatio(const Vec4& prev, const Vec4& curr, const Plane plane)
		{
			switch (plane) {
			case Plane::POSITIVE_W:
				return (prev.W - 0.0f) / (prev.W - curr.W);
			case Plane::POSITIVE_X:
				return (prev.W - prev.X) / ((prev.W - prev.X) - (curr.W - curr.X));
			case Plane::NEGATIVE_X:
				return (prev.W + prev.X) / ((prev.W + prev.X) - (curr.W + curr.X));
			case Plane::POSITIVE_Y:
				return (prev.W - prev.Y) / ((prev.W - prev.Y) - (curr.W - curr.Y));
			case Plane::NEGATIVE_Y:
				return (prev.W + prev.Y) / ((prev.W + prev.Y) - (curr.W + curr.Y));
			case Plane::POSITIVE_Z:
				return (prev.W - prev.Z) / ((prev.W - prev.Z) - (curr.W - curr.Z));
			case Plane::NEGATIVE_Z:
				return (prev.W + prev.Z) / ((prev.W + prev.Z) - (curr.W + curr.Z));
			default:
				ASSERT(false);
				return 0.0f;
			}
		}


		template <typename varyings_t>
		static void LerpVaryings(varyings_t& out, const varyings_t& start, const varyings_t& end, const float ratio)
		{
			constexpr uint32_t floatNum = sizeof(varyings_t) / sizeof(float);
			float* startFloat = (float*)&start;
			float* endFloat = (float*)&end;
			float* outFloat = (float*)&out;

			for (int i = 0;i < (int)floatNum;i++)
			{
				outFloat[i] = Lerp(startFloat[i], endFloat[i], ratio);
			}
		}



		//Varyings顶点数列，顶点按图形逆序存储
		//函数返回值是裁剪后的凸多边形有几个点（顶点数组中有几个点有效）
		template <typename varyings_t>
		static int ClipAgainstPlane(varyings_t(&outVaryings)[RGS_MAX_VARYINGS],
									const varyings_t(&inVaryings)[RGS_MAX_VARYINGS],
									const Plane plane,
									const int inVertexNum)
		{
			ASSERT(inVertexNum >= 3);

			int outVertexNum = 0;
			for (int i = 0; i < inVertexNum; i++)
			{
				int prevIndex = (inVertexNum - 1 + i) % inVertexNum;//currVaryings前一个点的序列值
				int currIndex = i;

				const varyings_t& prevVaryings = inVaryings[prevIndex];
				const varyings_t& currVaryings = inVaryings[currIndex];

				//判断prevInside，currInside分别在内还是在外
				const bool prevInside = IsInsidePlane(prevVaryings.ClipPos, plane);
				const bool currInside = IsInsidePlane(currVaryings.ClipPos, plane);

				if (currInside != prevInside)
				{
					//求出交点在prevVaryings和currVaryings所在线的比率
					float ratio = GetIntersectRatio(prevVaryings.ClipPos, currVaryings.ClipPos, plane);
					//得到焦点坐标并存到outVaryings
					LerpVaryings(outVaryings[outVertexNum], prevVaryings, currVaryings, ratio);
					//顶点总数+1
					outVertexNum++;
				}

				//当前点在平面内的话outVertexNum也+1
				if (currInside)
				{
					outVaryings[outVertexNum] = inVaryings[currIndex];
					outVertexNum++;
				}
			}

			ASSERT(outVertexNum <= RGS_MAX_VARYINGS);
			return outVertexNum;
		}


		//齐次除法(用XYZ值分别除模型距摄像机的距离w，除完后令W=1/w)
		template<typename varyings_t>
		static void CaculateNdcPos(varyings_t(&varyings)[RGS_MAX_VARYINGS], const int vertexNum)
		{
			for (int i = 0; i < vertexNum; i++)
			{
				float w = varyings[i].ClipPos.W;
				varyings[i].NdcPos = varyings[i].ClipPos / w;
				varyings[i].NdcPos.W = 1.0f / w;
			}
		}


		//视口变换
		template<typename varyings_t>
		static void CaculateFragPos(varyings_t(&varyings)[RGS_MAX_VARYINGS],
									const int vertexNum,
									const float width,
									const float height)
		{
			for (int i = 0; i < vertexNum; i++)
			{
				float x = ((varyings[i].NdcPos.X + 1.0f) * 0.5f * width);
				float y = ((varyings[i].NdcPos.Y + 1.0f) * 0.5f * height);
				float z = (varyings[i].NdcPos.Z + 1.0f) * 0.5f;
				float w = varyings[i].NdcPos.W;

				varyings[i].FragPos.X = x;
				varyings[i].FragPos.Y = y;
				varyings[i].FragPos.Z = z;
				varyings[i].FragPos.W = w;
			}
		}
		

		//裁剪的实现
		template<typename varyings_t>
		//因为Draw函数是静态，所以在其中调用的Clip函数也是静态
		static int Clip(varyings_t(&varyings)[RGS_MAX_VARYINGS])
		{
			//判断三角形的三个点是否在视锥体内，如都在就不用裁
			bool v0_Visible = IsVertexVisible(varyings[0].ClipPos);
			bool v1_Visible = IsVertexVisible(varyings[1].ClipPos);
			bool v2_Visible = IsVertexVisible(varyings[2].ClipPos);
			if (v0_Visible && v1_Visible && v2_Visible)
				return 3;

			int vertexNum = 3;
			
			//用n个平面对图形进行裁剪

			varyings_t varyings_[RGS_MAX_VARYINGS];//和varyings一起被称为双重缓存

			//得到varyings裁剪空间下的ClipPose坐标和坐标数vertexNum，让他去被Plane::POSITIVE_W平面裁
			//vertexNum为裁剪后varyings_中有几个点
			vertexNum = ClipAgainstPlane(varyings_, varyings, Plane::POSITIVE_W, vertexNum);//裁掉所有w为负的部分
			if (vertexNum == 0) return 0;
			vertexNum = ClipAgainstPlane(varyings, varyings_, Plane::POSITIVE_X, vertexNum);
			if (vertexNum == 0) return 0;
			vertexNum = ClipAgainstPlane(varyings_, varyings, Plane::NEGATIVE_X, vertexNum);
			if (vertexNum == 0) return 0;
			vertexNum = ClipAgainstPlane(varyings, varyings_, Plane::POSITIVE_Y, vertexNum);
			if (vertexNum == 0) return 0;
			vertexNum = ClipAgainstPlane(varyings_, varyings, Plane::NEGATIVE_Y, vertexNum);
			if (vertexNum == 0) return 0;
			vertexNum = ClipAgainstPlane(varyings, varyings_, Plane::POSITIVE_Z, vertexNum);
			if (vertexNum == 0) return 0;
			vertexNum = ClipAgainstPlane(varyings_, varyings, Plane::NEGATIVE_Z, vertexNum);
			if (vertexNum == 0) return 0;

			//将 varyings_ 中的数据复制到 varyings
			//并且复制的字节数为 varyings_ 所占用的内存大小
			//memcpy 执行的是字节级别的内存拷贝，因此适用于原始的字节序列或结构体等。
			memcpy(varyings, varyings_, sizeof(varyings_));

			return vertexNum;
		}


		//framebuffer：要画的帧缓存    program：存了片段着色器会在光栅化中起作用
		template<typename vertex_t, typename uniforms_t, typename varyings_t>
		static void RasterizeTriangle(Framebuffer& framebuffer,
									  const Program<vertex_t, uniforms_t, varyings_t>& program,
									  const varyings_t(&varyings)[3],
									  const uniforms_t& uniforms)
		{
			for (int i = 0;i < 3;i++)
			{
				int x = varyings[i].FragPos.X;
				int y = varyings[i].FragPos.Y;
				//以在 (x, y) 为中心的位置绘制一个 11×11 的正方形
				for (int j = -5;j < 6;j++)
				{
					for (int k = -5;k < 6;k++)
					{
						framebuffer.SetColor(x + j, y + k, { 1.0f,1.0f,1.0f });
					}
				}
			}
		}



	public:


		//framebuffer：指定要将图像画到哪张帧缓存上
		//program：指定顶点着色器
		//triangle：要画的三角形
		//uniforms：顶点着色器将顶点从model空间变到clip空间所需的所有变换矩阵的乘积
		template<typename vertex_t, typename uniforms_t, typename varyings_t>
		static void Draw(Framebuffer& framebuffer,
						const Program<vertex_t, uniforms_t, varyings_t>& program,
						const Triangle<vertex_t>& triangle,
						const uniforms_t& uniforms)
		{
			static_assert(std::is_base_of_v<VertexBase, vertex_t>, "vertex_t 必须继承自 RGS::VertexBase");
			static_assert(std::is_base_of_v<VaryingsBase, varyings_t>, "varyings_t 必须继承自 RGS::VaryingsBase");

			//顶点着色
			varyings_t varyings[RGS_MAX_VARYINGS];
			for (int i = 0; i < 3; i++)
			{
				program.VertexShader(varyings[i], triangle[i], uniforms);
			}

			/* Clipping */
			int vertexNum = Clip(varyings);

			//齐次除法->透视空间->视口变换->屏幕空间
			CaculateNdcPos(varyings, vertexNum);
			int fWidth = framebuffer.GetWidth();
			int fHeight = framebuffer.GetHeight();
			CaculateFragPos(varyings, vertexNum, (float)fWidth, (float)fHeight);


			/* Triangle Assembly & Rasterization */
			//三角形组合与光栅化
			for (int i = 0; i < vertexNum - 2; i++)
			{
				//按住i点取三角形
				varyings_t triVaryings[3];
				triVaryings[0] = varyings[0];
				triVaryings[1] = varyings[i + 1];
				triVaryings[2] = varyings[i + 2];

				RasterizeTriangle(framebuffer, program, triVaryings, uniforms);
			}
		}

		
		
	};
}