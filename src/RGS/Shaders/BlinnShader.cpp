#include "BlinnShader.h"

namespace RGS {

	void BlinnVertexShader(BlinnVaryings& varyings, const BlinnVertex& vertex, const BlinnUniforms& uniforms)
	{
		varyings.ClipPos = uniforms.MVP * vertex.ModelPos;
		varyings.TexCoord = vertex.TexCoord;
		varyings.WorldPos = uniforms.Model * vertex.ModelPos;
		varyings.WorldNormal = uniforms.ModelNormalToWorld * Vec4{ vertex.ModelNormal , 0.0f };
	}
	//片元着色器
	Vec4 BlinnFragmentShader(bool& discard, const BlinnVaryings& varyings, const BlinnUniforms& uniforms)
	{
        discard = false;

        const Vec3& cameraPos = uniforms.CameraPos;
        const Vec3& lightPos = uniforms.LightPos;
        const Vec3& worldPos = varyings.WorldPos;
        Vec3 worldNormal = Normalize(varyings.WorldNormal);
        //视线方向
        Vec3 viewDir = Normalize(cameraPos - worldPos);
        //光线方向
        Vec3 lightDir = Normalize(lightPos - worldPos);
        //半程向量
        Vec3 halfDir = Normalize(lightDir + viewDir);

        Vec3 ambient = uniforms.LightAmbient;
        Vec3 specularStrength{ 1.0f, 1.0f, 1.0f };
        Vec3 diffColor{ 1.0f, 1.0f, 1.0f };
        if (uniforms.Diffuse && uniforms.Specular)
        {
            const Vec2& texCoord = varyings.TexCoord;
            diffColor = uniforms.Diffuse->Sample(texCoord);
            ambient = ambient * diffColor;
            specularStrength = uniforms.Specular->Sample(texCoord);
        }


        //worldNormal的方向是从平面指向外侧的
        Vec3 diffuse = std::max(0.0f, Dot(worldNormal, lightDir)) * uniforms.LightDiffuse*diffColor;
        //uniforms.Shininess用高光度控制边缘半径
        Vec3 specular = (float)pow(std::max(0.0f, Dot(halfDir, worldNormal)), uniforms.Shininess) * uniforms.LightSpecular* specularStrength;

        Vec3 result = ambient + diffuse + specular;

        return { result, 1.0f };
	}
}