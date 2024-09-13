#include "RGS/Application.h"

#include "RGS/Base.h"
#include "RGS/Window.h"
#include "RGS/Framebuffer.h"
#include "RGS/Maths.h"
#include "RGS/Shaders/BlinnShader.h"
#include "RGS/Renderer.h"


#include <iostream>
#include <string>
#include <chrono>
#include <vector>
#include <fstream>
#include <stdio.h>

namespace RGS
{

	Application::Application(std::string name, const int width, const int height)
		:m_Name(name),m_Width(width),m_Height(height)			//把函数得到的值传给未知数m_Name等
	{
		Init();
	}

	Application::~Application()
	{
		Terminate();
	}

	void Application::Init()
	{
		Window::Init();
		m_Window = Window::Create(m_Name, m_Width, m_Height);
		m_LastFrameTime = std::chrono::high_resolution_clock::now();

		LoadMesh("assets\\sphere.obj");

		m_Uniforms.Diffuse = new Texture("assets\\plywood_diff_2k.jpg");
		m_Uniforms.Specular = new Texture("assets\\container2_specular.png");
	}

	void Application::Terminate()
	{
		delete m_Uniforms.Diffuse;
		delete m_Uniforms.Specular;

		delete m_Window;
		Window::Terminate();
	}

	void Application::Run()
	{
		while (!m_Window->Closed())
		{
			auto nowFrameTime = std::chrono::steady_clock::now();
			//duration持续时长
			auto duration = std::chrono::duration_cast<std::chrono::microseconds>(nowFrameTime - m_LastFrameTime);
			//时长转换为以秒为单位
			float deltaTime = duration.count() * 0.001f * 0.001f;
			//m_LastFrameTime为上一帧时间
			m_LastFrameTime = nowFrameTime;

			OnUpdate(deltaTime);

			Window::PollInputEvents();
		}
	}

	void Application::LoadMesh(const char* fileName)
	{
		std::ifstream file(fileName);
		ASSERT(file);

		std::vector<Vec3> positions;
		std::vector<Vec2> texCoords;
		std::vector<Vec3> normals;
		std::vector<int> posIndices;
		std::vector<int> texIndices;
		std::vector<int> normalIndices;

		std::string line;
		while (!file.eof())
		{
			std::getline(file, line);
			int items = -1;
			if (line.find("v ") == 0)                /* 位置 */
			{
				Vec3 position;
				//把得到的位置信息传到position
				//items找到了几个值，正常是3个，因为XYZ
				items = sscanf(line.c_str(), "v %f %f %f",
					&position.X, &position.Y, &position.Z);
				ASSERT(items == 3);
				positions.push_back(position);
			}
			else if (line.find("vt ") == 0)          /* 纹理 */
			{
				Vec2 texcoord;
				items = sscanf(line.c_str(), "vt %f %f",
					&texcoord.X, &texcoord.Y);
				ASSERT(items == 2);
				texCoords.push_back(texcoord);
			}
			else if (line.find("vn ") == 0)          /* 法线 */
			{
				Vec3 normal;
				items = sscanf(line.c_str(), "vn %f %f %f",
					&normal.X, &normal.Y, &normal.Z);
				ASSERT(items == 3);
				normals.push_back(normal);
			}
			else if (line.find("f ") == 0)           /* 朝向 */
			{
				int pIndices[3], uvIndices[3], nIndices[3];
				items = sscanf(line.c_str(), "f %d/%d/%d %d/%d/%d %d/%d/%d",
					&pIndices[0], &uvIndices[0], &nIndices[0],
					&pIndices[1], &uvIndices[1], &nIndices[1],
					&pIndices[2], &uvIndices[2], &nIndices[2]);
				ASSERT(items == 9);
				for (int i = 0; i < 3; i++)
				{
					posIndices.push_back(pIndices[i] - 1);
					texIndices.push_back(uvIndices[i] - 1);
					normalIndices.push_back(nIndices[i] - 1);
				}
			}
		}
		file.close();

		//有几个三角形
		int triNum = posIndices.size() / 3;
		for (int i = 0; i < triNum; i++)
		{
			Triangle<BlinnVertex> triangle;
			for (int j = 0; j < 3; j++)
			{
				int index = 3 * i + j;
				int posIndex = posIndices[index];
				int texIndex = texIndices[index];
				int nIndex = normalIndices[index];
				triangle[j].ModelPos = { positions[posIndex] , 1.0f };
				triangle[j].TexCoord = texCoords[texIndex];
				triangle[j].ModelNormal = normals[nIndex];
			}
			m_Mesh.emplace_back(triangle);
		}


		
	}


	void Application::OnCameraUpdate(float time)
	{
		//控制相机移动速度
		constexpr float speed = 3.0f;
		//按空格向上走
		if (m_Window->GetKey(RGS_KEY_SPACE) == RGS_PRESS)
			m_Camera.Pos = m_Camera.Pos + speed * time * m_Camera.Up;
		if (m_Window->GetKey(RGS_KEY_LEFT_SHIFT) == RGS_PRESS)
			m_Camera.Pos = m_Camera.Pos - speed * time * m_Camera.Up;
		if (m_Window->GetKey(RGS_KEY_D) == RGS_PRESS)
			m_Camera.Pos = m_Camera.Pos + speed * time * m_Camera.Right;
		if (m_Window->GetKey(RGS_KEY_A) == RGS_PRESS)
			m_Camera.Pos = m_Camera.Pos - speed * time * m_Camera.Right;
		if (m_Window->GetKey(RGS_KEY_W) == RGS_PRESS)
			m_Camera.Pos = m_Camera.Pos + speed * time * m_Camera.Dir;
		if (m_Window->GetKey(RGS_KEY_S) == RGS_PRESS)
			m_Camera.Pos = m_Camera.Pos - speed * time * m_Camera.Dir;

		//旋转
		constexpr float rotateSpeed = 1.0f;
		//旋转矩阵
		Mat4 rotation = Mat4Identity();
		if (m_Window->GetKey(RGS_KEY_Q) == RGS_PRESS)
			rotation = Mat4RotateY(time * rotateSpeed);
		if (m_Window->GetKey(RGS_KEY_E) == RGS_PRESS)
			rotation = Mat4RotateY(-time * rotateSpeed);
		//矩阵×视线方向
		m_Camera.Dir = rotation * m_Camera.Dir;
		//归一化
		m_Camera.Dir = { Normalize(m_Camera.Dir), 0.0f };
		//矩阵×右手方向
		m_Camera.Right = rotation * m_Camera.Right;
		m_Camera.Right = { Normalize(m_Camera.Right), 0.0f };
	}



	void Application::OnUpdate(float time)
	{
		OnCameraUpdate(time);

		Framebuffer framebuffer(m_Width, m_Height);
		Program program(BlinnVertexShader,BlinnFragmentShader);

		Mat4 view = Mat4LookAt(m_Camera.Pos, m_Camera.Pos + m_Camera.Dir, {0.0f,1.0f,0.0f});
		Mat4 proj = Mat4Perspective(90.0f / 360.0f * 2.0f * PI, m_Camera.Aspect, 0.1f, 10.0f);

		Mat4 model = Mat4Identity();
		m_Uniforms.MVP = proj * view * model;
		m_Uniforms.CameraPos = m_Camera.Pos;
		m_Uniforms.Model = model;
		m_Uniforms.ModelNormalToWorld = Mat4Identity();

		//高光度随时间变化
		m_Uniforms.Shininess *= std::pow(2, time * 1.1f);
		if (m_Uniforms.Shininess > 256.0f)
			m_Uniforms.Shininess = 2.0f;

		for (auto tri : m_Mesh)
		{
			Renderer::Draw(framebuffer, program, tri, m_Uniforms);
		}

		m_Window->DrawFramebuffer(framebuffer);

	}
}