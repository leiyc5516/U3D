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

#include "../Precompiled.h"

#include "../Core/Context.h"
#include "../Graphics/Texture2D.h"
#include "../Resource/ResourceCache.h"
#include "../UI/BorderImage.h"

#include "../DebugNew.h"

namespace Urho3D
{

extern const char* blendModeNames[];
extern const char* UI_CATEGORY;

BorderImage::BorderImage(Context* context) :
    UIElement(context),
    imageRect_(IntRect::ZERO),
    border_(IntRect::ZERO),
    imageBorder_(IntRect::ZERO),
    hoverOffset_(IntVector2::ZERO),
    disabledOffset_(IntVector2::ZERO),
    blendMode_(BLEND_REPLACE),
    tiled_(false)
{
}

BorderImage::~BorderImage() = default;

void BorderImage::RegisterObject(Context* context)
{
    context->RegisterFactory<BorderImage>(UI_CATEGORY);

    URHO3D_COPY_BASE_ATTRIBUTES(UIElement);
    URHO3D_MIXED_ACCESSOR_ATTRIBUTE("Texture", GetTextureAttr, SetTextureAttr, ResourceRef, ResourceRef(Texture2D::GetTypeStatic()),
        AM_FILE);
    URHO3D_ACCESSOR_ATTRIBUTE("Image Rect", GetImageRect, SetImageRect, IntRect, IntRect::ZERO, AM_FILE);
    URHO3D_ACCESSOR_ATTRIBUTE("Border", GetBorder, SetBorder, IntRect, IntRect::ZERO, AM_FILE);
    URHO3D_ACCESSOR_ATTRIBUTE("Image Border", GetImageBorder, SetImageBorder, IntRect, IntRect::ZERO, AM_FILE);
    URHO3D_ACCESSOR_ATTRIBUTE("Hover Image Offset", GetHoverOffset, SetHoverOffset, IntVector2, IntVector2::ZERO, AM_FILE);
    URHO3D_ACCESSOR_ATTRIBUTE("Disabled Image Offset", GetDisabledOffset, SetDisabledOffset, IntVector2, IntVector2::ZERO, AM_FILE);
    URHO3D_ACCESSOR_ATTRIBUTE("Tiled", IsTiled, SetTiled, bool, false, AM_FILE);
    URHO3D_ENUM_ACCESSOR_ATTRIBUTE("Blend Mode", GetBlendMode, SetBlendMode, BlendMode, blendModeNames, 0, AM_FILE);
    URHO3D_MIXED_ACCESSOR_ATTRIBUTE("Material", GetMaterialAttr, SetMaterialAttr, ResourceRef, ResourceRef(Material::GetTypeStatic()),
        AM_FILE);
}

void BorderImage::GetBatches(PODVector<UIBatch>& batches, PODVector<float>& vertexData, const IntRect& currentScissor)
{
    if (enabled_)
        GetBatches(batches, vertexData, currentScissor, (hovering_ || selected_ || HasFocus()) ? hoverOffset_ : IntVector2::ZERO);
    else
        GetBatches(batches, vertexData, currentScissor, disabledOffset_);
}

void BorderImage::SetTexture(Texture* texture)
{
    texture_ = texture;
    if (imageRect_ == IntRect::ZERO)
        SetFullImageRect();
}

void BorderImage::SetImageRect(const IntRect& rect)
{
    if (rect != IntRect::ZERO)
        imageRect_ = rect;
}

void BorderImage::SetFullImageRect()
{
    if (texture_)
        SetImageRect(IntRect(0, 0, texture_->GetWidth(), texture_->GetHeight()));
}

void BorderImage::SetBorder(const IntRect& rect)
{
    border_.left_ = Max(rect.left_, 0);
    border_.top_ = Max(rect.top_, 0);
    border_.right_ = Max(rect.right_, 0);
    border_.bottom_ = Max(rect.bottom_, 0);
}

void BorderImage::SetImageBorder(const IntRect& rect)
{
    imageBorder_.left_ = Max(rect.left_, 0);
    imageBorder_.top_ = Max(rect.top_, 0);
    imageBorder_.right_ = Max(rect.right_, 0);
    imageBorder_.bottom_ = Max(rect.bottom_, 0);
}

void BorderImage::SetHoverOffset(const IntVector2& offset)
{
    hoverOffset_ = offset;
}

void BorderImage::SetHoverOffset(int x, int y)
{
    hoverOffset_ = IntVector2(x, y);
}

void BorderImage::SetDisabledOffset(const IntVector2& offset)
{
    disabledOffset_ = offset;
}

void BorderImage::SetDisabledOffset(int x, int y)
{
    disabledOffset_ = IntVector2(x, y);
}

void BorderImage::SetBlendMode(BlendMode mode)
{
    blendMode_ = mode;
}

void BorderImage::SetTiled(bool enable)
{
    tiled_ = enable;
}

/**
 * @brief 获取边框图像的渲染批次数据
 * 
 * 此函数根据边框图像的当前状态（不透明度、颜色等）计算渲染批次，并将其添加到批次列表中。
 * 它会考虑边框、图像边框、偏移量等因素，将图像分割为多个部分进行渲染。
 * 
 * @param batches 用于存储渲染批次的向量
 * @param vertexData 用于存储顶点数据的向量
 * @param currentScissor 当前的裁剪矩形，用于确定哪些区域需要渲染
 * @param offset 图像的偏移量，通常由按钮的状态（如悬停、禁用）决定
 */
void BorderImage::GetBatches(PODVector<UIBatch>& batches, PODVector<float>& vertexData, const IntRect& currentScissor,
    const IntVector2& offset)
{
    // 标记图像是否完全不透明
    bool allOpaque = true;
    // 检查图像的不透明度和四个角的颜色透明度，如果有任何一个小于 1.0f，则图像不是完全不透明
    if (GetDerivedOpacity() < 1.0f || colors_[C_TOPLEFT].a_ < 1.0f || colors_[C_TOPRIGHT].a_ < 1.0f ||
        colors_[C_BOTTOMLEFT].a_ < 1.0f || colors_[C_BOTTOMRIGHT].a_ < 1.0f)
        allOpaque = false;

    // 创建一个新的渲染批次，根据混合模式和不透明度设置最终的混合模式
    UIBatch
        batch(this, blendMode_ == BLEND_REPLACE && !allOpaque ? BLEND_ALPHA : blendMode_, currentScissor, texture_, &vertexData);

    // 如果设置了材质，则将其应用到渲染批次中
    if (material_)
        batch.customMaterial_ = material_;

    // 计算内部矩形的纹理边框，如果图像边框未设置，则使用边框值
    const IntRect& uvBorder = (imageBorder_ == IntRect::ZERO) ? border_ : imageBorder_;
    // 获取缩进宽度
    int x = GetIndentWidth();
    // 获取图像的尺寸，并减去缩进宽度
    IntVector2 size = GetSize();
    size.x_ -= x;
    // 计算内部矩形的尺寸
    IntVector2 innerSize(
        Max(size.x_ - border_.left_ - border_.right_, 0),
        Max(size.y_ - border_.top_ - border_.bottom_, 0));
    // 计算内部矩形的纹理尺寸
    IntVector2 innerUvSize(
        Max(imageRect_.right_ - imageRect_.left_ - uvBorder.left_ - uvBorder.right_, 0),
        Max(imageRect_.bottom_ - imageRect_.top_ - uvBorder.top_ - uvBorder.bottom_, 0));

    // 计算纹理的左上角坐标，并加上偏移量
    IntVector2 uvTopLeft(imageRect_.left_, imageRect_.top_);
    uvTopLeft += offset;

    // 处理顶部区域
    if (border_.top_)
    {
        // 处理顶部左侧区域
        if (border_.left_)
            batch.AddQuad(x, 0, border_.left_, border_.top_, uvTopLeft.x_, uvTopLeft.y_, uvBorder.left_, uvBorder.top_);
        // 处理顶部中间区域
        if (innerSize.x_)
            batch.AddQuad(x + border_.left_, 0, innerSize.x_, border_.top_, uvTopLeft.x_ + uvBorder.left_, uvTopLeft.y_,
                innerUvSize.x_, uvBorder.top_, tiled_);
        // 处理顶部右侧区域
        if (border_.right_)
            batch.AddQuad(x + border_.left_ + innerSize.x_, 0, border_.right_, border_.top_,
                uvTopLeft.x_ + uvBorder.left_ + innerUvSize.x_, uvTopLeft.y_, uvBorder.right_, uvBorder.top_);
    }
    // 处理中间区域
    if (innerSize.y_)
    {
        // 处理中间左侧区域
        if (border_.left_)
            batch.AddQuad(x, border_.top_, border_.left_, innerSize.y_, uvTopLeft.x_, uvTopLeft.y_ + uvBorder.top_,
                uvBorder.left_, innerUvSize.y_, tiled_);
        // 处理中间中间区域
        if (innerSize.x_)
            batch.AddQuad(x + border_.left_, border_.top_, innerSize.x_, innerSize.y_, uvTopLeft.x_ + uvBorder.left_,
                uvTopLeft.y_ + uvBorder.top_, innerUvSize.x_, innerUvSize.y_, tiled_);
        // 处理中间右侧区域
        if (border_.right_)
            batch.AddQuad(x + border_.left_ + innerSize.x_, border_.top_, border_.right_, innerSize.y_,
                uvTopLeft.x_ + uvBorder.left_ + innerUvSize.x_, uvTopLeft.y_ + uvBorder.top_, uvBorder.right_, innerUvSize.y_,
                tiled_);
    }
    // 处理底部区域
    if (border_.bottom_)
    {
        // 处理底部左侧区域
        if (border_.left_)
            batch.AddQuad(x, border_.top_ + innerSize.y_, border_.left_, border_.bottom_, uvTopLeft.x_,
                uvTopLeft.y_ + uvBorder.top_ + innerUvSize.y_, uvBorder.left_, uvBorder.bottom_);
        // 处理底部中间区域
        if (innerSize.x_)
            batch.AddQuad(x + border_.left_, border_.top_ + innerSize.y_, innerSize.x_, border_.bottom_,
                uvTopLeft.x_ + uvBorder.left_, uvTopLeft.y_ + uvBorder.top_ + innerUvSize.y_, innerUvSize.x_, uvBorder.bottom_,
                tiled_);
        // 处理底部右侧区域
        if (border_.right_)
            batch.AddQuad(x + border_.left_ + innerSize.x_, border_.top_ + innerSize.y_, border_.right_, border_.bottom_,
                uvTopLeft.x_ + uvBorder.left_ + innerUvSize.x_, uvTopLeft.y_ + uvBorder.top_ + innerUvSize.y_, uvBorder.right_,
                uvBorder.bottom_);
    }

    // 将新的渲染批次添加到批次列表中，如果可以合并则进行合并
    UIBatch::AddOrMerge(batch, batches);

    // 重置鼠标悬停状态，为下一帧做准备
    hovering_ = false;
}

void BorderImage::SetTextureAttr(const ResourceRef& value)
{
    auto* cache = GetSubsystem<ResourceCache>();
    SetTexture(cache->GetResource<Texture2D>(value.name_));
}

ResourceRef BorderImage::GetTextureAttr() const
{
    return GetResourceRef(texture_, Texture2D::GetTypeStatic());
}

void BorderImage::SetMaterialAttr(const ResourceRef& value)
{
    auto* cache = GetSubsystem<ResourceCache>();
    SetMaterial(cache->GetResource<Material>(value.name_));
}

ResourceRef BorderImage::GetMaterialAttr() const
{
    return GetResourceRef(material_, Material::GetTypeStatic());
}

void BorderImage::SetMaterial(Material* material)
{
    material_ = material;
}

Material* BorderImage::GetMaterial() const
{
    return material_;
}

}
