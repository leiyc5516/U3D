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

// This file contains VertexBuffer code common to all graphics APIs.

#include "../Precompiled.h"

#include "../Graphics/Graphics.h"
#include "../Graphics/VertexBuffer.h"
#include "../Math/MathDefs.h"

#include "../DebugNew.h"

namespace Urho3D
{

VertexBuffer::VertexBuffer(Context* context, bool forceHeadless) :
    Object(context),
    GPUObject(forceHeadless ? nullptr : GetSubsystem<Graphics>())
{
    UpdateOffsets();

    // Force shadowing mode if graphics subsystem does not exist
    if (!graphics_)
        shadowed_ = true;
}

VertexBuffer::~VertexBuffer()
{
    Release();
}

void VertexBuffer::SetShadowed(bool enable)
{
    // If no graphics subsystem, can not disable shadowing
    if (!graphics_)
        enable = true;

    if (enable != shadowed_)
    {
        if (enable && vertexSize_ && vertexCount_)
            shadowData_ = new unsigned char[vertexCount_ * vertexSize_];
        else
            shadowData_.Reset();

        shadowed_ = enable;
    }
}

/**
 * @brief 设置顶点缓冲区的大小和属性
 * @param vertexCount 顶点数量
 * @param elementMask 顶点属性掩码，表示包含哪些顶点元素
 * @param dynamic 是否为动态缓冲区
 * @return 成功返回true，失败返回false
 * 
 * 该函数是设置顶点缓冲区大小的便捷方法，通过顶点属性掩码来指定顶点格式。
 * 内部会调用GetElements()将掩码转换为顶点元素列表，再调用另一个SetSize()重载。
 */
bool VertexBuffer::SetSize(unsigned vertexCount, unsigned elementMask, bool dynamic)
{
    return SetSize(vertexCount, GetElements(elementMask), dynamic);
}

/**
 * @brief 设置顶点缓冲区的大小和属性
 * @param vertexCount 顶点数量
 * @param elements 顶点元素描述数组
 * @param dynamic 是否为动态缓冲区
 * @return 成功返回true，失败返回false
 * 
 * 该函数用于设置顶点缓冲区的详细参数，包括：
 * 1. 先解锁当前缓冲区
 * 2. 更新顶点数量、元素格式和动态标志
 * 3. 重新计算各元素偏移量
 * 4. 根据是否需要影子数据分配内存
 * 5. 最后创建实际的GPU缓冲区
 */
bool VertexBuffer::SetSize(unsigned vertexCount, const PODVector<VertexElement>& elements, bool dynamic)
{
    // 解锁当前缓冲区
    Unlock();

    // 设置顶点数量、元素格式和动态标志
    vertexCount_ = vertexCount;
    elements_ = elements;
    dynamic_ = dynamic;

    // 更新各顶点元素的偏移量
    UpdateOffsets();

    // 如果需要影子数据且参数有效，则分配影子缓冲区
    if (shadowed_ && vertexCount_ && vertexSize_)
        shadowData_ = new unsigned char[vertexCount_ * vertexSize_];
    else
        shadowData_.Reset();

    // 创建实际的GPU缓冲区
    return Create();
}

/**
 * @brief 更新顶点元素的偏移量并计算相关属性
 * 
 * 该函数用于：
 * 1. 计算每个顶点元素的偏移量
 * 2. 生成顶点格式的唯一哈希值
 * 3. 设置顶点元素的掩码
 * 4. 计算顶点总大小
 */
void VertexBuffer::UpdateOffsets()
{
    unsigned elementOffset = 0;  // 当前元素偏移量
    elementHash_ = 0;           // 顶点格式哈希值
    elementMask_ = MASK_NONE;   // 顶点元素掩码

    // 遍历所有顶点元素
    for (PODVector<VertexElement>::Iterator i = elements_.Begin(); i != elements_.End(); ++i)
    {
        // 设置当前元素的偏移量
        i->offset_ = elementOffset;
        // 累加偏移量
        elementOffset += ELEMENT_TYPESIZES[i->type_];
        // 计算顶点格式哈希值
        elementHash_ <<= 6;
        elementHash_ += (((int)i->type_ + 1) * ((int)i->semantic_ + 1) + i->index_);

        // 检查是否为传统顶点元素
        for (unsigned j = 0; j < MAX_LEGACY_VERTEX_ELEMENTS; ++j)
        {
            const VertexElement& legacy = LEGACY_VERTEXELEMENTS[j];
            if (i->type_ == legacy.type_ && i->semantic_ == legacy.semantic_ && i->index_ == legacy.index_)
                elementMask_ |= VertexMaskFlags(1u << j);  // 设置对应的掩码位
        }
    }

    // 计算顶点总大小
    vertexSize_ = elementOffset;
}

const VertexElement* VertexBuffer::GetElement(VertexElementSemantic semantic, unsigned char index) const
{
    for (PODVector<VertexElement>::ConstIterator i = elements_.Begin(); i != elements_.End(); ++i)
    {
        if (i->semantic_ == semantic && i->index_ == index)
            return &(*i);
    }

    return nullptr;
}

const VertexElement* VertexBuffer::GetElement(VertexElementType type, VertexElementSemantic semantic, unsigned char index) const
{
    for (PODVector<VertexElement>::ConstIterator i = elements_.Begin(); i != elements_.End(); ++i)
    {
        if (i->type_ == type && i->semantic_ == semantic && i->index_ == index)
            return &(*i);
    }

    return nullptr;
}

const VertexElement* VertexBuffer::GetElement(const PODVector<VertexElement>& elements, VertexElementType type, VertexElementSemantic semantic, unsigned char index)
{
    for (PODVector<VertexElement>::ConstIterator i = elements.Begin(); i != elements.End(); ++i)
    {
        if (i->type_ == type && i->semantic_ == semantic && i->index_ == index)
            return &(*i);
    }

    return nullptr;
}

bool VertexBuffer::HasElement(const PODVector<VertexElement>& elements, VertexElementType type, VertexElementSemantic semantic, unsigned char index)
{
    return GetElement(elements, type, semantic, index) != nullptr;
}

unsigned VertexBuffer::GetElementOffset(const PODVector<VertexElement>& elements, VertexElementType type, VertexElementSemantic semantic, unsigned char index)
{
    const VertexElement* element = GetElement(elements, type, semantic, index);
    return element ? element->offset_ : M_MAX_UNSIGNED;
}

PODVector<VertexElement> VertexBuffer::GetElements(unsigned elementMask)
{
    PODVector<VertexElement> ret;

    for (unsigned i = 0; i < MAX_LEGACY_VERTEX_ELEMENTS; ++i)
    {
        if (elementMask & (1u << i))
            ret.Push(LEGACY_VERTEXELEMENTS[i]);
    }

    return ret;
}

unsigned VertexBuffer::GetVertexSize(const PODVector<VertexElement>& elements)
{
    unsigned size = 0;

    for (unsigned i = 0; i < elements.Size(); ++i)
        size += ELEMENT_TYPESIZES[elements[i].type_];

    return size;
}

unsigned VertexBuffer::GetVertexSize(unsigned elementMask)
{
    unsigned size = 0;

    for (unsigned i = 0; i < MAX_LEGACY_VERTEX_ELEMENTS; ++i)
    {
        if (elementMask & (1u << i))
            size += ELEMENT_TYPESIZES[LEGACY_VERTEXELEMENTS[i].type_];
    }

    return size;
}

void VertexBuffer::UpdateOffsets(PODVector<VertexElement>& elements)
{
    unsigned elementOffset = 0;

    for (PODVector<VertexElement>::Iterator i = elements.Begin(); i != elements.End(); ++i)
    {
        i->offset_ = elementOffset;
        elementOffset += ELEMENT_TYPESIZES[i->type_];
    }
}

}
