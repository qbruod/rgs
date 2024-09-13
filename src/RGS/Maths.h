#pragma once
#include <string>

#define PI 3.14159265359
#define EPSILON 1e-5f

namespace RGS {

    struct Vec2
    {
        float X, Y;
        
        constexpr Vec2()
            :X(0.0f),Y(0.0f){}
        constexpr Vec2(float x,float y)
            :X(x),Y(y){}
    };

    struct Vec3
    {
        float X, Y, Z;//相当于RGB

        // Learn constexpr: https://learn.microsoft.com/zh-cn/cpp/cpp/constexpr-cpp?view=msvc-170
        constexpr Vec3()
            : X(0.0f), Y(0.0f), Z(0.0f) {}//默认构造函数
        constexpr Vec3(float x, float y, float z)
            : X(x), Y(y), Z(z) {}
        //参数化构造函数，根据传入的xyz初始化XYZ
        operator Vec2() const { return{ X, Y }; }//operator类型转换操作符三维强制转换二维
    };

    struct Vec4
    {
        float X, Y, Z, W;

        constexpr Vec4()
            : X(0.0f), Y(0.0f), Z(0.0f), W(0.0f) {};
        constexpr Vec4(float x, float y, float z, float w)
            : X(x), Y(y), Z(z), W(w) {};
        constexpr Vec4(const Vec3& vec3, float w)
            : X(vec3.X), Y(vec3.Y), Z(vec3.Z), W(w) {};

        operator Vec2() const { return { X,Y }; }//同return Vec2(X, Y)
        operator Vec3() const { return { X,Y,Z }; }

        operator std::string() const
        {
            std::string res;
            res += "(";
            res += std::to_string(X);
            res += ",";
            res += std::to_string(Y);
            res += ",";
            res += std::to_string(Z);
            res += ",";
            res += std::to_string(W);
            res += ")";
            return res;
        }
    };

    //按行优先存储，第一列向量为[0][0],[1][0],[2][0],[3][0]《-》0,4,8,12
    struct Mat4
    {
        float M[4][4];

        Mat4()
        {
            for (int i = 0;i < 4;i++)
            {
                for (int j = 0;j < 4;j++)
                {
                    M[i][j] = 0.0f;
                }
            }
        }

        Mat4(const Vec4& v0, const Vec4& v1, const Vec4& v2, const Vec4& v3);

        operator const std::string() const
        {
            std::string res;
            res += "(";

            for (int i = 0;i < 4;i++)
            {
                for (int j = 0;j < 4;j++)
                {
                    res += std::to_string(M[i][j]);
                    res += (i == 3 && j == 3) ? ")" : ",";
                }
            }
            return res;
        }

        
    };

    float Dot(const Vec3& left, const Vec3& right);//矩阵点乘
    Vec3 Cross(const Vec3& left, const Vec3& right);//叉乘
    Vec3 Normalize(const Vec3& v);//归一化

    Vec3 operator+ (const Vec3& left, const Vec3& right);
    Vec3 operator- (const Vec3& left, const Vec3& right);
    Vec3 operator* (const float left, const Vec3& right);
    Vec3 operator* (const Vec3& left, const float right);
    Vec3 operator* (const Vec3& left, const Vec3& right);
    Vec3 operator/ (const Vec3& left, const float right);

    Vec4 operator+ (const Vec4& left, const Vec4& right);
    Vec4 operator- (const Vec4& left, const Vec4& right);
    Vec4 operator* (const float left, const Vec4& right);
    Vec4 operator* (const Vec4& left, const float right);
    Vec4 operator/ (const Vec4& left, const float right);

    Vec4 operator* (const Mat4& mat4,const Vec4& vec4);//变换矩阵*原始坐标=当前坐标
    Mat4 operator* (const Mat4& left, const Mat4& right);
    Mat4& operator*= (Mat4& left, const Mat4& right);

    Mat4 Mat4Identity();//创建单位阵
    Mat4 Mat4Scale(float sx, float sy, float sz);//按照x/y/z轴拉伸
    Mat4 Mat4Translate(float tx,float ty,float tz);//平移
    Mat4 Mat4RotateX(float angle);
    Mat4 Mat4RotateY(float angle);
    Mat4 Mat4RotateZ(float angle);
    //分别绕xyz轴旋转
    //计算顺序：变换矩阵n*变化矩阵n-1...*变化矩阵1*坐标，
    //其中变化矩阵1为对坐标进行的第一次变换，n为第n次

    //用LookAt函数构造摄像机坐标系(形参分别为相机的xyz轴，相机的位置eye)
    Mat4 Mat4LookAt(const Vec3& xAxis, const Vec3& yAxis, const Vec3& zAxis, const Vec3& eye);
    //重载LookAt，作用与上面相同 target是相机看向的位置
    Mat4 Mat4LookAt(const Vec3& eye,const Vec3& target,const Vec3& up);

    //透视投影 aspect=width/height
    Mat4 Mat4Perspective(float fovy, float aspect, float near, float far);

    unsigned char Float2UChar(const float f);
    float UChar2Float(const unsigned char c);

    float Lerp(const float start, const float end, const float t);

}