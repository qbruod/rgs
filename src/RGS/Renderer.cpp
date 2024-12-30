#include "Renderer.h"
#include <algorithm>


namespace RGS {
	bool Renderer::IsVertexVisible(const Vec4& clipPos)
	{
		//判断顶点是否在视锥体内只需要判断X/Y/Z的绝对值÷W是否小于1
		return std::fabs(clipPos.X) <= clipPos.W && std::fabs(clipPos.Y) <= clipPos.W && std::fabs(clipPos.Z) <= clipPos.W;
	}


	 bool Renderer::IsInsidePlane(const Vec4& clipPos, const Plane plane)
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
	float Renderer::GetIntersectRatio(const Vec4& prev, const Vec4& curr, const Plane plane)
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

	//获取包围盒
	Renderer::BoundingBox Renderer::GetBoundingBox(const Vec4(&fragCoords)[3],
													const int width,
													const int height)
	{
		auto xList = { fragCoords[0].X, fragCoords[1].X, fragCoords[2].X };
		auto yList = { fragCoords[0].Y, fragCoords[1].Y, fragCoords[2].Y };

		float minX = std::min<float>(xList);
		float maxX = std::max<float>(xList);
		float minY = std::min<float>(yList);
		float maxY = std::max<float>(yList);

		//把xy的最值压缩到屏幕可见范围内
		minX = Clamp(minX, 0.0f, (float)(width - 1));
		maxX = Clamp(maxX, 0.0f, (float)(width - 1));
		minY = Clamp(minY, 0.0f, (float)(height - 1));
		maxY = Clamp(maxY, 0.0f, (float)(height - 1));

		//去Min的下底，取Max的上底。如floor(1.5)=1,ceil(2.5)=3
		BoundingBox bBox;
		bBox.MinX = std::floor(minX);
		bBox.MinY = std::floor(minY);
		bBox.MaxX = std::ceil(maxX);
		bBox.MaxY = std::ceil(maxY);

		return bBox;
	}

	//这里的权重是根据真实图形坐标求出的权重，不是经过透视除法后的
	void Renderer::CalculateWeights(float(&screenWeights)[3],
										float(&weights)[3],
										const Vec4(&fragCoords)[3],
										const Vec2& screenPoint)
	{
		Vec2 ab = fragCoords[1] - fragCoords[0];
		Vec2 ac = fragCoords[2] - fragCoords[0];
		Vec2 ap = screenPoint - fragCoords[0];
		float factor = 1.0f / (ab.X * ac.Y - ab.Y * ac.X);
		float s = (ac.Y * ap.X - ac.X * ap.Y) * factor;
		float t = (ab.X * ap.Y - ab.Y * ap.X) * factor;
		//screenWeights为各点的w',fragCoords.W为1/w(w为点到原点在z轴的距离)
		screenWeights[0] = 1 - s - t;
		screenWeights[1] = s;
		screenWeights[2] = t;

		float w0 = fragCoords[0].W * screenWeights[0];
		float w1 = fragCoords[1].W * screenWeights[1];
		float w2 = fragCoords[2].W * screenWeights[2];
		//normalizer为wp
		float normalizer = 1.0f / (w0 + w1 + w2);
		//weights为真实权重比例
		weights[0] = w0 * normalizer;
		weights[1] = w1 * normalizer;
		weights[2] = w2 * normalizer;
	}

	bool Renderer::IsInsideTriangle(float(&weights)[3])
	{
		//不写>=0是因为系统有误差
		//所以通过判断权重是否>=一个很小的负数来判断p点是否在三角形内部
		return weights[0] >= -EPSILON && weights[1] >= -EPSILON && weights[2] >= -EPSILON;
	}

	bool Renderer::IsBackFacing(const Vec4& a, const Vec4& b, const Vec4& c)
	{
		// 逆时针为正面（可见）
		//用叉乘判断面方向是否朝向摄像机
		float signedArea = a.X * b.Y - a.Y * b.X +
			b.X * c.Y - b.Y * c.X +
			c.X * a.Y - c.Y * a.X;
		return signedArea <= 0;
	}

	bool Renderer::PassDepthTest(const float writeDepth, const float fDepth, const DepthFuncType depthFunc)
	{
		switch (depthFunc)
		{
		case DepthFuncType::LESS:
			return fDepth - writeDepth > EPSILON;
		case DepthFuncType::LEQUAL:
			return fDepth - writeDepth >= -EPSILON;
		case DepthFuncType::ALWAYS:
			return true;
		default:
			return false;
		}
	}

}