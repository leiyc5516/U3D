//
// Copyright (c) 2008-2022 the Urho3D project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#pragma once

#include "../Container/ArrayPtr.h"
#include "../Resource/Resource.h"

namespace Urho3D
{

class ShaderVariation;

/**
 * @class Shader
 * @brief 表示一个着色器资源，由多个着色器变体组成。
 * 
 * 该类继承自 Resource 类，负责加载、处理和管理着色器的源代码以及不同定义组合下的着色器变体。
 * 支持顶点着色器和像素着色器的处理，能够根据不同的定义获取对应的着色器变体。
 */
class URHO3D_API Shader : public Resource
{
    // 注册对象类型，用于 Urho3D 的反射系统
    URHO3D_OBJECT(Shader, Resource);

public:
    /**
     * @brief 构造函数。
     * 
     * @param context 指向上下文对象的指针，用于资源管理和系统访问。
     */
    explicit Shader(Context* context);
    /**
     * @brief 析构函数。
     * 
     * 覆盖基类的析构函数，确保正确释放资源。
     */
    ~Shader() override;
    /**
     * @brief 注册对象工厂。
     * 
     * 用于在 Urho3D 的反射系统中注册 Shader 类，以便动态创建对象。
     * @param context 指向上下文对象的指针，用于资源管理和系统访问。
     * @nobind 表示该函数不会绑定到脚本系统。
     */
    static void RegisterObject(Context* context);

    /**
     * @brief 从流中开始加载资源。
     * 
     * 此函数可能在工作线程中调用。如果加载成功则返回 true。
     * @param source 反序列化器对象，用于从流中读取数据。
     * @return 如果加载成功返回 true，否则返回 false。
     */
    bool BeginLoad(Deserializer& source) override;
    /**
     * @brief 完成资源加载。
     * 
     * 此函数总是在主线程中调用。如果加载成功则返回 true。
     * @return 如果加载成功返回 true，否则返回 false。
     */
    bool EndLoad() override;

    /**
     * @brief 根据着色器类型和定义字符串获取着色器变体。
     * 
     * 多个定义之间用空格分隔。
     * @param type 着色器类型，如顶点着色器或像素着色器。
     * @param defines 定义字符串，包含一个或多个用空格分隔的定义。
     * @return 指向 ShaderVariation 对象的指针，如果未找到则可能为 nullptr。
     */
    ShaderVariation* GetVariation(ShaderType type, const String& defines);
    /**
     * @brief 根据着色器类型和定义字符指针获取着色器变体。
     * 
     * 多个定义之间用空格分隔。
     * @param type 着色器类型，如顶点着色器或像素着色器。
     * @param defines 定义字符指针，包含一个或多个用空格分隔的定义。
     * @return 指向 ShaderVariation 对象的指针，如果未找到则可能为 nullptr。
     */
    ShaderVariation* GetVariation(ShaderType type, const char* defines);

    /**
     * @brief 返回顶点着色器或像素着色器的源代码。
     * 
     * 根据传入的着色器类型，返回对应的源代码。
     * @param type 着色器类型，如顶点着色器或像素着色器。
     * @return 对应着色器类型的源代码的常量引用。
     */
    const String& GetSourceCode(ShaderType type) const { return type == VS ? vsSourceCode_ : psSourceCode_; }

    /**
     * @brief 返回着色器代码及其包含文件的最新时间戳。
     * 
     * 时间戳可用于检查文件是否有更新。
     * @return 最新的时间戳。
     */
    unsigned GetTimeStamp() const { return timeStamp_; }

private:
    /**
     * @brief 处理源代码和包含文件。
     * 
     * 解析源代码并处理其中的包含文件。如果处理成功则返回 true。
     * @param code 用于存储处理后代码的字符串引用。
     * @param source 反序列化器对象，用于从流中读取源代码。
     * @return 如果处理成功返回 true，否则返回 false。
     */
    bool ProcessSource(String& code, Deserializer& source);
    /**
     * @brief 对定义进行排序并去除多余的空格。
     * 
     * 防止创建不必要的重复着色器变体。
     * @param defines 原始的定义字符串。
     * @return 经过规范化处理后的定义字符串。
     */
    String NormalizeDefines(const String& defines);
    /**
     * @brief 重新计算着色器占用的内存。
     * 
     * 更新着色器所使用的内存大小。
     */
    void RefreshMemoryUse();

    /// 适配顶点着色器的源代码
    String vsSourceCode_;
    /// 适配像素着色器的源代码
    String psSourceCode_;
    /// 顶点着色器变体的哈希映射，键为定义的哈希值
    HashMap<StringHash, SharedPtr<ShaderVariation> > vsVariations_;
    /// 像素着色器变体的哈希映射，键为定义的哈希值
    HashMap<StringHash, SharedPtr<ShaderVariation> > psVariations_;
    /// 源代码的时间戳
    unsigned timeStamp_;
    /// 到目前为止唯一变体的数量
    unsigned numVariations_;
};

}
