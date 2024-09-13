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

	enum class DepthFuncType
	{
		LESS,
		LEQUAL,
		ALWAYS,
	};

	//顶点着色器与片元着色器
	template<typename vertex_t,typename uniforms_t,typename varyings_t>
	struct Program
	{
		//EnableDepthTest：是否开启深度测试
		//EnableWriteDepth：是否写入深度测试
		//EnableBlend:是否开启混合
		bool EnableDepthTest = true;
		bool EnableWriteDepth = true;
		bool EnableBlend = false;
		bool EnableDoubleSided = false;

		DepthFuncType DepthFunc = DepthFuncType::LESS;

		//using定义函数指针类型void (*)(varyings_t&, const vertex_t&, const uniforms_t&)的别名Vertex_shader_t
		//此函数的签名为void，varyings_t，const vertex_t，const uniforms_t
		using vertex_shader_t = void (*)(varyings_t&, const vertex_t&, const uniforms_t&);
		//像调用函数一样调用VertexShader。如：VertexShader(varying, vertex, uniforms);
		//这句话意思等价于void (*VertexShader)(varyings_t&, const vertex_t&, const uniforms_t&);
		vertex_shader_t VertexShader;

		//discard标识是否需要丢弃当前像素点
		//用Vec4类型存储color值
		using fragment_shader_t = Vec4(*)(bool& discard, const varyings_t&, const uniforms_t&);
		fragment_shader_t FragmentShader;

		Program(const vertex_shader_t vertexShader, const fragment_shader_t fragmentShader)
			: VertexShader(vertexShader), FragmentShader(fragmentShader) {}
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


		static bool IsVertexVisible(const Vec4& clipPos);
		static bool IsInsidePlane(const Vec4& clipPos, const Plane plane);


		//利用相似三角形求出比例
		static float GetIntersectRatio(const Vec4& prev, const Vec4& curr, const Plane plane);


		//求平面与线段交点坐标
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


		//齐次变换(用XYZ值分别除模型距摄像机的距离w，除完后令W=1/w)也叫透视除法
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


		//视口变换  把坐标范围从-1到1变为0到1
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


		//包围盒
		struct BoundingBox { int MinX, MaxX, MinY, MaxY; };
		//获取包围盒
		static BoundingBox GetBoundingBox(const Vec4(&fragCoords)[3], const int width, const int height);
		//得到p点权重
		static void CalculateWeights(float(&screenWeights)[3], float(&weights)[3], const Vec4(&fragCoords)[3], const Vec2& screenPoint);
		//判断p点是否在三角形内部
		static bool IsInsideTriangle(float(&weights)[3]);
		//三角形内部点p的颜色混合方法
		template <typename varyings_t>
		static void LerpVaryings(varyings_t& out,
								const varyings_t(&varyings)[3],
								const float(&weights)[3],
								const int width,
								const int height)
		{
			//裁剪空间权重
			out.ClipPos = varyings[0].ClipPos * weights[0] +
				varyings[1].ClipPos * weights[1] +
				varyings[2].ClipPos * weights[2];
			//Ncd空间的权重
			out.NdcPos = out.ClipPos / out.ClipPos.W;
			out.NdcPos.W = 1.0f / out.ClipPos.W;
			//视口空间的权重
			out.FragPos.X = ((out.NdcPos.X + 1.0f) * 0.5f * width);
			out.FragPos.Y = ((out.NdcPos.Y + 1.0f) * 0.5f * height);
			out.FragPos.Z = (out.NdcPos.Z + 1.0f) * 0.5f;
			out.FragPos.W = out.NdcPos.W;

			constexpr uint32_t floatOffset = sizeof(Vec4) * 3 / sizeof(float);
			constexpr uint32_t floatNum = sizeof(varyings_t) / sizeof(float);
			float* v0 = (float*)&varyings[0];
			float* v1 = (float*)&varyings[1];
			float* v2 = (float*)&varyings[2];
			float* outFloat = (float*)&out;

			for (int i = floatOffset; i < (int)floatNum; i++)
			{
				outFloat[i] = v0[i] * weights[0] + v1[i] * weights[1] + v2[i] * weights[2];
			}
		}
		//像素处理  uniforms：可以存顶点着色器和片元着色器需要的参数
		template<typename vertex_t, typename uniforms_t, typename varyings_t>
		static void ProcessPixel(Framebuffer& framebuffer,
									const int x,
									const int y,
									const Program<vertex_t, uniforms_t, varyings_t>& program,
									const varyings_t& varyings,
									const uniforms_t& uniforms)
		{
			//像素着色
			bool discard = false;
			Vec4 color{ 0.0f, 0.0f, 0.0f, 0.0f };
			color = program.FragmentShader(discard, varyings, uniforms);
			if (discard)
			{
				return;
			}
			color.X = Clamp(color.X, 0.0f, 1.0f);
			color.Y = Clamp(color.Y , 0.0f, 1.0f);
			color.Z = Clamp(color.Z , 0.0f, 1.0f);
			color.W = Clamp(color.W, 0.0f, 1.0f);

			//混合  alpha：表示透明度
			if (program.EnableBlend)
			{
				Vec3 dstColor = framebuffer.GetColor(x, y);
				Vec3 srcColor = color;
				float alpha = color.W;
				// 使用线性插值 (Lerp) 计算混合后的颜色
				// Lerp 公式：混合颜色 = (1 - alpha) * 目标颜色 + alpha * 源颜色
				color = { Lerp(dstColor, srcColor, alpha), 1.0f };
				framebuffer.SetColor(x, y, color);
			}
			else
			{
				framebuffer.SetColor(x, y, color);
			}

			if (program.EnableWriteDepth)
			{
				float depth = varyings.FragPos.Z;
				framebuffer.SetDepth(x, y, depth);
			}
		}

		//判断图案是否在背面
		static bool IsBackFacing(const Vec4& a, const Vec4& b, const Vec4& c);
		//判断当前深度与帧缓存上深度之间的关系
		static bool PassDepthTest(const float writeDepth, const float fDepth, const DepthFuncType depthFunc);
		//framebuffer：要画的帧缓存    program：存了片段着色器会在光栅化中起作用
		template<typename vertex_t, typename uniforms_t, typename varyings_t>
		static void RasterizeTriangle(Framebuffer& framebuffer,
									  const Program<vertex_t, uniforms_t, varyings_t>& program,
									  const varyings_t(&varyings)[3],
									  const uniforms_t& uniforms)
		{
			//背面剔除
			//program.EnableDoubleSided判断是否开启双面
			if (!program.EnableDoubleSided)
			{
				bool isBackFacing = false;
				isBackFacing = IsBackFacing(varyings[0].NdcPos, varyings[1].NdcPos, varyings[2].NdcPos);
				if (isBackFacing)
				{
					return;
				}
			}

			int width = framebuffer.GetWidth();
			int height = framebuffer.GetHeight();
			//包围盒
			Vec4 fragCoords[3];
			fragCoords[0] = varyings[0].FragPos;
			fragCoords[1] = varyings[1].FragPos;
			fragCoords[2] = varyings[2].FragPos;
			BoundingBox bBox = GetBoundingBox(fragCoords, width, height);

			for (int y = bBox.MinY; y <= bBox.MaxY; y++)
			{
				for (int x = bBox.MinX; x <= bBox.MaxX; x++)
				{
					/* Varyings Setup */
					float screenWeights[3];
					//weights：权重
					float weights[3];
					//每个像素格的中心点坐标screenPoint，也叫p点
					Vec2 screenPoint{ (float)x + 0.5f, (float)y + 0.5f };

					//得到p点权重
					CalculateWeights(screenWeights, weights, fragCoords, screenPoint);
					//判断p点是否在三角形内部
					if (!IsInsideTriangle(weights))
						continue;

					varyings_t pixVaryings;
					LerpVaryings(pixVaryings, varyings, weights, width, height);

					//深度测试
					if (program.EnableDepthTest)
					{
						float depth = pixVaryings.FragPos.Z;
						float fDepth = framebuffer.GetDepth(x, y);
						DepthFuncType depthFunc = program.DepthFunc;
						if (!PassDepthTest(depth, fDepth, depthFunc))
						{
							continue;
						}
					}

					//像素处理
					ProcessPixel(framebuffer, x, y, program, pixVaryings, uniforms);
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

			//(透视投影->裁剪->)齐次除法->NCD->视口变换->屏幕空间
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