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

#include "../Graphics/Graphics.h"
#include "../Graphics/Texture.h"
#include "../UI/UIElement.h"

#include "../DebugNew.h"

namespace Urho3D
{

Vector3 UIBatch::posAdjust(0.0f, 0.0f, 0.0f);

UIBatch::UIBatch()
{
    SetDefaultColor();
}

/**
 * @brief UIBatch 类的带参数构造函数。
 * 
 * 该构造函数用于初始化 UIBatch 对象，接收 UI 元素、混合模式、裁剪矩形、纹理和顶点数据指针作为参数。
 * 它会根据传入的参数设置对象的成员变量，并计算纹理的逆尺寸。同时，调用 `SetDefaultColor` 函数设置默认颜色。
 * 
 * @param element 此批次所代表的 UI 元素。
 * @param blendMode 混合模式，用于控制像素的混合方式。
 * @param scissor 裁剪矩形，定义了渲染区域。
 * @param texture 纹理对象，用于渲染 UI 元素。
 * @param vertexData 指向顶点数据向量的指针，存储四边形的顶点信息。
 */
UIBatch::UIBatch(UIElement* element, BlendMode blendMode, const IntRect& scissor, Texture* texture, PODVector<float>* vertexData) :     // NOLINT(modernize-pass-by-value)
    // 初始化此批次所代表的 UI 元素
    element_(element),
    // 初始化混合模式
    blendMode_(blendMode),
    // 初始化裁剪矩形
    scissor_(scissor),
    // 初始化纹理对象
    texture_(texture),
    // 计算并初始化纹理的逆尺寸。如果纹理存在，则计算其宽度和高度的倒数；否则，使用默认值 (1.0f, 1.0f)
    invTextureSize_(texture ? Vector2(1.0f / (float)texture->GetWidth(), 1.0f / (float)texture->GetHeight()) : Vector2::ONE),
    // 初始化顶点数据指针
    vertexData_(vertexData),
    // 初始化顶点数据的起始索引，设置为当前顶点数据向量的大小
    vertexStart_(vertexData->Size()),
    // 初始化顶点数据的结束索引，设置为当前顶点数据向量的大小
    vertexEnd_(vertexData->Size())
{
    // 调用 SetDefaultColor 函数设置默认颜色
    SetDefaultColor();
}

/**
 * @brief 设置 UIBatch 的颜色。
 * 
 * 该函数用于设置 UIBatch 的颜色，并根据 `overrideAlpha` 参数决定是否覆盖原有的透明度。
 * 如果没有关联的 UI 元素，则强制覆盖透明度。
 * 
 * @param color 要设置的颜色。
 * @param overrideAlpha 是否覆盖原有的透明度。如果为 true，则直接使用传入颜色的透明度；
 *                      如果为 false，则将传入颜色的透明度乘以 UI 元素的派生不透明度。
 */
void UIBatch::SetColor(const Color& color, bool overrideAlpha)
{
    // 如果没有关联的 UI 元素，强制覆盖透明度，因为没有元素就无法获取派生不透明度
    if (!element_)
        overrideAlpha = true;

    // 设置颜色渐变标志为 false，表示不使用颜色渐变
    useGradient_ = false;
    // 根据 overrideAlpha 参数决定最终的颜色值
    // 如果 overrideAlpha 为 true，直接使用传入颜色的无符号整数表示
    // 如果 overrideAlpha 为 false，将传入颜色的透明度乘以 UI 元素的派生不透明度后，再转换为无符号整数表示
    color_ =
        overrideAlpha ? color.ToUInt() : Color(color.r_, color.g_, color.b_, color.a_ * element_->GetDerivedOpacity()).ToUInt();
}

/**
 * @brief 设置 UIBatch 的默认颜色。
 * 
 * 该函数根据是否存在关联的 UI 元素，设置 UIBatch 的颜色和颜色渐变标志。
 * 如果存在关联的 UI 元素，则使用该元素的派生颜色，并根据元素是否有颜色渐变设置标志。
 * 如果不存在关联的 UI 元素，则将颜色设置为白色（完全不透明），并将颜色渐变标志设置为 false。
 */
void UIBatch::SetDefaultColor()
{
    // 检查是否存在关联的 UI 元素
    if (element_)
    {
        // 如果存在关联的 UI 元素，将 UIBatch 的颜色设置为该元素的派生颜色的无符号整数表示
        color_ = element_->GetDerivedColor().ToUInt();
        // 根据 UI 元素是否有颜色渐变，设置 UIBatch 的颜色渐变标志
        useGradient_ = element_->HasColorGradient();
    }
    else
    {
        // 如果不存在关联的 UI 元素，将 UIBatch 的颜色设置为白色（完全不透明）
        color_ = 0xffffffff;
        // 将 UIBatch 的颜色渐变标志设置为 false，表示没有颜色渐变
        useGradient_ = false;
    }
}

/**
 * @brief 向顶点数据中添加一个四边形的顶点信息。
 * 
 * 该函数会根据传入的四边形位置、尺寸以及纹理偏移等信息，计算四边形各顶点的位置、颜色和纹理坐标，
 * 并将这些信息添加到顶点数据向量中。如果颜色没有渐变且透明度为 0，则不会添加该四边形。
 * 
 * @param x 四边形在屏幕空间中的左上角 x 坐标。
 * @param y 四边形在屏幕空间中的左上角 y 坐标。
 * @param width 四边形的宽度。
 * @param height 四边形的高度。
 * @param texOffsetX 纹理偏移的 x 坐标。
 * @param texOffsetY 纹理偏移的 y 坐标。
 * @param texWidth 纹理的宽度。若为 0，则使用四边形的宽度。
 * @param texHeight 纹理的高度。若为 0，则使用四边形的高度。
 */
void UIBatch::AddQuad(float x, float y, float width, float height, int texOffsetX, int texOffsetY, int texWidth, int texHeight)
{
    // 定义四边形四个顶点的颜色值
    unsigned topLeftColor, topRightColor, bottomLeftColor, bottomRightColor;

    // 检查是否使用颜色渐变
    if (!useGradient_)
    {
        // 如果 alpha 通道为 0，说明完全透明，不会渲染任何内容，因此不添加该四边形
        if (!(color_ & 0xff000000))
            return;

        // 若不使用颜色渐变，四个顶点的颜色都设置为统一颜色
        topLeftColor = color_;
        topRightColor = color_;
        bottomLeftColor = color_;
        bottomRightColor = color_;
    }
    else
    {
        // 若使用颜色渐变，根据顶点位置插值计算每个顶点的颜色
        topLeftColor = GetInterpolatedColor(x, y);
        topRightColor = GetInterpolatedColor(x + width, y);
        bottomLeftColor = GetInterpolatedColor(x, y + height);
        bottomRightColor = GetInterpolatedColor(x + width, y + height);
    }

    // 获取元素在屏幕上的位置
    const IntVector2& screenPos = element_->GetScreenPosition();

    // 计算四边形在屏幕空间中的实际坐标
    float left = x + screenPos.x_ - posAdjust.x_;
    float right = left + width;
    // 原代码此处可能有误，应该减去 posAdjust.y_ 而不是 posAdjust.x_
    float top = y + screenPos.y_ - posAdjust.y_; 
    float bottom = top + height;

    // 计算四边形各顶点对应的纹理坐标
    float leftUV = texOffsetX * invTextureSize_.x_;
    float topUV = texOffsetY * invTextureSize_.y_;
    float rightUV = (texOffsetX + (texWidth ? texWidth : width)) * invTextureSize_.x_;
    float bottomUV = (texOffsetY + (texHeight ? texHeight : height)) * invTextureSize_.y_;

    // 记录当前顶点数据向量的大小，作为新顶点数据的起始位置
    unsigned begin = vertexData_->Size();
    // 为新的四边形顶点数据预留空间，一个四边形由两个三角形组成，共 6 个顶点
    vertexData_->Resize(begin + 6 * UI_VERTEX_SIZE);
    // 获取指向新顶点数据起始位置的指针
    float* dest = &(vertexData_->At(begin));
    // 更新顶点数据的结束位置
    vertexEnd_ = vertexData_->Size();

    // 添加第一个三角形的第一个顶点（左上角）
    dest[0] = left;
    dest[1] = top;
    dest[2] = 0.0f;
    ((unsigned&)dest[3]) = topLeftColor;
    dest[4] = leftUV;
    dest[5] = topUV;

    // 添加第一个三角形的第二个顶点（右上角）
    dest[6] = right;
    dest[7] = top;
    dest[8] = 0.0f;
    ((unsigned&)dest[9]) = topRightColor;
    dest[10] = rightUV;
    dest[11] = topUV;

    // 添加第一个三角形的第三个顶点（左下角）
    dest[12] = left;
    dest[13] = bottom;
    dest[14] = 0.0f;
    ((unsigned&)dest[15]) = bottomLeftColor;
    dest[16] = leftUV;
    dest[17] = bottomUV;

    // 添加第二个三角形的第一个顶点（右上角）
    dest[18] = right;
    dest[19] = top;
    dest[20] = 0.0f;
    ((unsigned&)dest[21]) = topRightColor;
    dest[22] = rightUV;
    dest[23] = topUV;

    // 添加第二个三角形的第二个顶点（右下角）
    dest[24] = right;
    dest[25] = bottom;
    dest[26] = 0.0f;
    ((unsigned&)dest[27]) = bottomRightColor;
    dest[28] = rightUV;
    dest[29] = bottomUV;

    // 添加第二个三角形的第三个顶点（左下角）
    dest[30] = left;
    dest[31] = bottom;
    dest[32] = 0.0f;
    ((unsigned&)dest[33]) = bottomLeftColor;
    dest[34] = leftUV;
    dest[35] = bottomUV;
}

/**
 * @brief 向顶点数据中添加一个四边形，支持使用变换矩阵进行顶点变换。
 * 
 * 该函数会根据传入的变换矩阵、四边形的位置和尺寸、纹理偏移等信息，
 * 计算四边形各顶点的位置、颜色和纹理坐标，并将这些信息添加到顶点数据向量中。
 * 如果颜色没有渐变且透明度为 0，则不会添加该四边形。
 * 
 * @param transform 用于变换顶点位置的 3x4 矩阵。
 * @param x 四边形在屏幕空间中的左上角 x 坐标。
 * @param y 四边形在屏幕空间中的左上角 y 坐标。
 * @param width 四边形的宽度。
 * @param height 四边形的高度。
 * @param texOffsetX 纹理偏移的 x 坐标。
 * @param texOffsetY 纹理偏移的 y 坐标。
 * @param texWidth 纹理的宽度。若为 0，则使用四边形的宽度。
 * @param texHeight 纹理的高度。若为 0，则使用四边形的高度。
 */
void UIBatch::AddQuad(const Matrix3x4& transform, int x, int y, int width, int height, int texOffsetX, int texOffsetY,
    int texWidth, int texHeight)
{
    // 定义四边形四个顶点的颜色值
    unsigned topLeftColor, topRightColor, bottomLeftColor, bottomRightColor;

    // 检查是否使用颜色渐变
    if (!useGradient_)
    {
        // 如果 alpha 通道为 0，说明完全透明，不会渲染任何内容，因此不添加该四边形
        if (!(color_ & 0xff000000))
            return;

        // 若不使用颜色渐变，四个顶点的颜色都设置为统一颜色
        topLeftColor = color_;
        topRightColor = color_;
        bottomLeftColor = color_;
        bottomRightColor = color_;
    }
    else
    {
        // 若使用颜色渐变，根据顶点位置插值计算每个顶点的颜色
        topLeftColor = GetInterpolatedColor(x, y);
        topRightColor = GetInterpolatedColor(x + width, y);
        bottomLeftColor = GetInterpolatedColor(x, y + height);
        bottomRightColor = GetInterpolatedColor(x + width, y + height);
    }

    // 使用变换矩阵对四边形的四个顶点进行变换，并减去位置调整量
    Vector3 v1 = (transform * Vector3((float)x, (float)y, 0.0f)) - posAdjust;
    Vector3 v2 = (transform * Vector3((float)x + (float)width, (float)y, 0.0f)) - posAdjust;
    Vector3 v3 = (transform * Vector3((float)x, (float)y + (float)height, 0.0f)) - posAdjust;
    Vector3 v4 = (transform * Vector3((float)x + (float)width, (float)y + (float)height, 0.0f)) - posAdjust;

    // 计算四边形各顶点对应的纹理坐标
    float leftUV = ((float)texOffsetX) * invTextureSize_.x_;
    float topUV = ((float)texOffsetY) * invTextureSize_.y_;
    float rightUV = ((float)(texOffsetX + (texWidth ? texWidth : width))) * invTextureSize_.x_;
    float bottomUV = ((float)(texOffsetY + (texHeight ? texHeight : height))) * invTextureSize_.y_;

    // 记录当前顶点数据向量的大小，作为新顶点数据的起始位置
    unsigned begin = vertexData_->Size();
    // 为新的四边形顶点数据预留空间，一个四边形由两个三角形组成，共 6 个顶点
    vertexData_->Resize(begin + 6 * UI_VERTEX_SIZE);
    // 获取指向新顶点数据起始位置的指针
    float* dest = &(vertexData_->At(begin));
    // 更新顶点数据的结束位置
    vertexEnd_ = vertexData_->Size();

    // 添加第一个三角形的第一个顶点（左上角）
    dest[0] = v1.x_;
    dest[1] = v1.y_;
    dest[2] = 0.0f;
    ((unsigned&)dest[3]) = topLeftColor;
    dest[4] = leftUV;
    dest[5] = topUV;

    // 添加第一个三角形的第二个顶点（右上角）
    dest[6] = v2.x_;
    dest[7] = v2.y_;
    dest[8] = 0.0f;
    ((unsigned&)dest[9]) = topRightColor;
    dest[10] = rightUV;
    dest[11] = topUV;

    // 添加第一个三角形的第三个顶点（左下角）
    dest[12] = v3.x_;
    dest[13] = v3.y_;
    dest[14] = 0.0f;
    ((unsigned&)dest[15]) = bottomLeftColor;
    dest[16] = leftUV;
    dest[17] = bottomUV;

    // 添加第二个三角形的第一个顶点（右上角）
    dest[18] = v2.x_;
    dest[19] = v2.y_;
    dest[20] = 0.0f;
    ((unsigned&)dest[21]) = topRightColor;
    dest[22] = rightUV;
    dest[23] = topUV;

    // 添加第二个三角形的第二个顶点（右下角）
    dest[24] = v4.x_;
    dest[25] = v4.y_;
    dest[26] = 0.0f;
    ((unsigned&)dest[27]) = bottomRightColor;
    dest[28] = rightUV;
    dest[29] = bottomUV;

    // 添加第二个三角形的第三个顶点（左下角）
    dest[30] = v3.x_;
    dest[31] = v3.y_;
    dest[32] = 0.0f;
    ((unsigned&)dest[33]) = bottomLeftColor;
    dest[34] = leftUV;
    dest[35] = bottomUV;
}

/**
 * @brief 向顶点数据中添加一个四边形，支持平铺模式。
 * 
 * 此函数会根据传入的参数添加一个四边形到顶点数据中。如果启用了平铺模式，
 * 会将四边形分割成多个小四边形进行填充。如果没有颜色渐变且透明度为 0，
 * 则不会添加四边形。
 * 
 * @param x 四边形在屏幕空间中的左上角 x 坐标。
 * @param y 四边形在屏幕空间中的左上角 y 坐标。
 * @param width 四边形的宽度。
 * @param height 四边形的高度。
 * @param texOffsetX 纹理偏移的 x 坐标。
 * @param texOffsetY 纹理偏移的 y 坐标。
 * @param texWidth 纹理的宽度。
 * @param texHeight 纹理的高度。
 * @param tiled 是否启用平铺模式。如果为 true，则会将四边形分割成多个小四边形进行填充。
 */
void UIBatch::AddQuad(int x, int y, int width, int height, int texOffsetX, int texOffsetY, int texWidth, int texHeight, bool tiled)
{
    // 检查是否既没有颜色渐变，又透明度为 0。如果是，则不添加四边形
    if (!(element_->HasColorGradient() || element_->GetDerivedColor().ToUInt() & 0xff000000))
        return; // No gradient and alpha is 0, so do not add the quad

    // 如果不启用平铺模式，直接调用另一个 AddQuad 函数添加四边形
    if (!tiled)
    {
        AddQuad(x, y, width, height, texOffsetX, texOffsetY, texWidth, texHeight);
        return;
    }

    // 定义平铺的起始位置和尺寸变量
    int tileX = 0;
    int tileY = 0;
    int tileW = 0;
    int tileH = 0;

    // 垂直方向遍历，直到覆盖整个四边形的高度
    while (tileY < height)
    {
        // 每次垂直移动后，重置水平方向的起始位置
        tileX = 0;
        // 计算当前行的平铺高度，避免超出四边形的高度
        tileH = Min(height - tileY, texHeight);

        // 水平方向遍历，直到覆盖当前行的宽度
        while (tileX < width)
        {
            // 计算当前列的平铺宽度，避免超出四边形的宽度
            tileW = Min(width - tileX, texWidth);

            // 调用另一个 AddQuad 函数添加当前小四边形
            AddQuad(x + tileX, y + tileY, tileW, tileH, texOffsetX, texOffsetY, tileW, tileH);

            // 水平移动到下一个平铺位置
            tileX += tileW;
        }

        // 垂直移动到下一行
        tileY += tileH;
    }
}

void UIBatch::AddQuad(const Matrix3x4& transform, const IntVector2& a, const IntVector2& b, const IntVector2& c, const IntVector2& d,
    const IntVector2& texA, const IntVector2& texB, const IntVector2& texC, const IntVector2& texD)
{
    Vector3 v1 = (transform * Vector3((float)a.x_, (float)a.y_, 0.0f)) - posAdjust;
    Vector3 v2 = (transform * Vector3((float)b.x_, (float)b.y_, 0.0f)) - posAdjust;
    Vector3 v3 = (transform * Vector3((float)c.x_, (float)c.y_, 0.0f)) - posAdjust;
    Vector3 v4 = (transform * Vector3((float)d.x_, (float)d.y_, 0.0f)) - posAdjust;

    Vector2 uv1((float)texA.x_ * invTextureSize_.x_, (float)texA.y_ * invTextureSize_.y_);
    Vector2 uv2((float)texB.x_ * invTextureSize_.x_, (float)texB.y_ * invTextureSize_.y_);
    Vector2 uv3((float)texC.x_ * invTextureSize_.x_, (float)texC.y_ * invTextureSize_.y_);
    Vector2 uv4((float)texD.x_ * invTextureSize_.x_, (float)texD.y_ * invTextureSize_.y_);

    unsigned begin = vertexData_->Size();
    vertexData_->Resize(begin + 6 * UI_VERTEX_SIZE);
    float* dest = &(vertexData_->At(begin));
    vertexEnd_ = vertexData_->Size();

    dest[0] = v1.x_;
    dest[1] = v1.y_;
    dest[2] = 0.0f;
    ((unsigned&)dest[3]) = color_;
    dest[4] = uv1.x_;
    dest[5] = uv1.y_;

    dest[6] = v2.x_;
    dest[7] = v2.y_;
    dest[8] = 0.0f;
    ((unsigned&)dest[9]) = color_;
    dest[10] = uv2.x_;
    dest[11] = uv2.y_;

    dest[12] = v3.x_;
    dest[13] = v3.y_;
    dest[14] = 0.0f;
    ((unsigned&)dest[15]) = color_;
    dest[16] = uv3.x_;
    dest[17] = uv3.y_;

    dest[18] = v1.x_;
    dest[19] = v1.y_;
    dest[20] = 0.0f;
    ((unsigned&)dest[21]) = color_;
    dest[22] = uv1.x_;
    dest[23] = uv1.y_;

    dest[24] = v3.x_;
    dest[25] = v3.y_;
    dest[26] = 0.0f;
    ((unsigned&)dest[27]) = color_;
    dest[28] = uv3.x_;
    dest[29] = uv3.y_;

    dest[30] = v4.x_;
    dest[31] = v4.y_;
    dest[32] = 0.0f;
    ((unsigned&)dest[33]) = color_;
    dest[34] = uv4.x_;
    dest[35] = uv4.y_;
}

void UIBatch::AddQuad(const Matrix3x4& transform, const IntVector2& a, const IntVector2& b, const IntVector2& c, const IntVector2& d,
    const IntVector2& texA, const IntVector2& texB, const IntVector2& texC, const IntVector2& texD, const Color& colA,
    const Color& colB, const Color& colC, const Color& colD)
{
    Vector3 v1 = (transform * Vector3((float)a.x_, (float)a.y_, 0.0f)) - posAdjust;
    Vector3 v2 = (transform * Vector3((float)b.x_, (float)b.y_, 0.0f)) - posAdjust;
    Vector3 v3 = (transform * Vector3((float)c.x_, (float)c.y_, 0.0f)) - posAdjust;
    Vector3 v4 = (transform * Vector3((float)d.x_, (float)d.y_, 0.0f)) - posAdjust;

    Vector2 uv1((float)texA.x_ * invTextureSize_.x_, (float)texA.y_ * invTextureSize_.y_);
    Vector2 uv2((float)texB.x_ * invTextureSize_.x_, (float)texB.y_ * invTextureSize_.y_);
    Vector2 uv3((float)texC.x_ * invTextureSize_.x_, (float)texC.y_ * invTextureSize_.y_);
    Vector2 uv4((float)texD.x_ * invTextureSize_.x_, (float)texD.y_ * invTextureSize_.y_);

    unsigned c1 = colA.ToUInt();
    unsigned c2 = colB.ToUInt();
    unsigned c3 = colC.ToUInt();
    unsigned c4 = colD.ToUInt();

    unsigned begin = vertexData_->Size();
    vertexData_->Resize(begin + 6 * UI_VERTEX_SIZE);
    float* dest = &(vertexData_->At(begin));
    vertexEnd_ = vertexData_->Size();

    dest[0] = v1.x_;
    dest[1] = v1.y_;
    dest[2] = 0.0f;
    ((unsigned&)dest[3]) = c1;
    dest[4] = uv1.x_;
    dest[5] = uv1.y_;

    dest[6] = v2.x_;
    dest[7] = v2.y_;
    dest[8] = 0.0f;
    ((unsigned&)dest[9]) = c2;
    dest[10] = uv2.x_;
    dest[11] = uv2.y_;

    dest[12] = v3.x_;
    dest[13] = v3.y_;
    dest[14] = 0.0f;
    ((unsigned&)dest[15]) = c3;
    dest[16] = uv3.x_;
    dest[17] = uv3.y_;

    dest[18] = v1.x_;
    dest[19] = v1.y_;
    dest[20] = 0.0f;
    ((unsigned&)dest[21]) = c1;
    dest[22] = uv1.x_;
    dest[23] = uv1.y_;

    dest[24] = v3.x_;
    dest[25] = v3.y_;
    dest[26] = 0.0f;
    ((unsigned&)dest[27]) = c3;
    dest[28] = uv3.x_;
    dest[29] = uv3.y_;

    dest[30] = v4.x_;
    dest[31] = v4.y_;
    dest[32] = 0.0f;
    ((unsigned&)dest[33]) = c4;
    dest[34] = uv4.x_;
    dest[35] = uv4.y_;
}

/**
 * @brief 尝试将另一个 UIBatch 对象合并到当前对象中。
 * 
 * 该函数会检查传入的 UIBatch 对象是否可以与当前对象合并。
 * 只有在混合模式、裁剪矩形、纹理、顶点数据指针相同，并且传入批次的起始顶点位置
 * 等于当前批次的结束顶点位置时，才可以进行合并。如果可以合并，
 * 则更新当前批次的结束顶点位置。
 * 
 * @param batch 要尝试合并的另一个 UIBatch 对象。
 * @return 如果合并成功返回 true，否则返回 false。
 */
bool UIBatch::Merge(const UIBatch& batch)
{
    // 检查多个条件，判断是否可以合并两个 UIBatch 对象
    // 如果混合模式不同，说明渲染方式不同，不能合并
    // 如果裁剪矩形不同，说明渲染区域不同，不能合并
    // 如果纹理不同，说明使用的纹理资源不同，不能合并
    // 如果顶点数据指针不同，说明存储顶点数据的位置不同，不能合并
    // 如果传入批次的起始顶点位置不等于当前批次的结束顶点位置，数据不连续，不能合并
    if (batch.blendMode_ != blendMode_ ||
        batch.scissor_ != scissor_ ||
        batch.texture_ != texture_ ||
        batch.vertexData_ != vertexData_ ||
        batch.vertexStart_ != vertexEnd_)
        return false;

    // 如果上述条件都满足，则可以合并两个 UIBatch 对象
    // 更新当前批次的结束顶点位置为传入批次的结束顶点位置
    vertexEnd_ = batch.vertexEnd_;
    return true;
}

/**
 * @brief 计算并返回指定位置的插值颜色。
 * 
 * 该函数根据传入的 x 和 y 坐标，在 UI 元素的四个角颜色之间进行插值计算，
 * 得到指定位置的颜色值。如果 UI 元素的宽度和高度都不为 0，则进行二维插值；
 * 否则，直接返回左上角的颜色。最后，将颜色的透明度乘以元素的派生不透明度。
 * 
 * @param x 要计算颜色的位置的 x 坐标，相对于 UI 元素的左上角。
 * @param y 要计算颜色的位置的 y 坐标，相对于 UI 元素的左上角。
 * @return 计算得到的颜色的无符号整数表示。
 */
unsigned UIBatch::GetInterpolatedColor(float x, float y)
{
    // 获取 UI 元素的尺寸
    const IntVector2& size = element_->GetSize();

    // 检查 UI 元素的宽度和高度是否都不为 0
    if (size.x_ && size.y_)
    {
        // 计算 x 和 y 方向的插值因子，并将其限制在 0 到 1 之间
        float cLerpX = Clamp(x / (float)size.x_, 0.0f, 1.0f);
        float cLerpY = Clamp(y / (float)size.y_, 0.0f, 1.0f);

        // 在顶部的两个角颜色之间进行线性插值，得到顶部的插值颜色
        Color topColor = element_->GetColor(C_TOPLEFT).Lerp(element_->GetColor(C_TOPRIGHT), cLerpX);
        // 在底部的两个角颜色之间进行线性插值，得到底部的插值颜色
        Color bottomColor = element_->GetColor(C_BOTTOMLEFT).Lerp(element_->GetColor(C_BOTTOMRIGHT), cLerpX);
        // 在顶部和底部的插值颜色之间进行线性插值，得到最终的插值颜色
        Color color = topColor.Lerp(bottomColor, cLerpY);
        // 将最终颜色的透明度乘以元素的派生不透明度
        color.a_ *= element_->GetDerivedOpacity();
        // 将最终颜色转换为无符号整数表示并返回
        return color.ToUInt();
    }
    else
    {
        // 如果 UI 元素的宽度或高度为 0，直接获取左上角的颜色
        Color color = element_->GetColor(C_TOPLEFT);
        // 将左上角颜色的透明度乘以元素的派生不透明度
        color.a_ *= element_->GetDerivedOpacity();
        // 将左上角颜色转换为无符号整数表示并返回
        return color.ToUInt();
    }
}

/**
 * @brief 将一个 UIBatch 对象添加到批次向量中，或者尝试与现有批次合并。
 * 
 * 该函数会检查传入的 UIBatch 对象是否包含有效的顶点数据。如果包含，
 * 会尝试将其与批次向量中的最后一个批次合并。如果合并失败，则将该批次
 * 直接添加到批次向量的末尾。
 * 
 * @param batch 要添加或合并的 UIBatch 对象。
 * @param batches 存储 UIBatch 对象的向量。
 */
void UIBatch::AddOrMerge(const UIBatch& batch, PODVector<UIBatch>& batches)
{
    // 检查传入的批次是否包含有效的顶点数据。如果顶点结束位置等于顶点起始位置，
    // 说明该批次没有顶点数据，直接返回，不进行添加或合并操作。
    if (batch.vertexEnd_ == batch.vertexStart_)
        return;

    // 检查批次向量是否不为空，并且尝试将传入的批次与向量中的最后一个批次合并。
    // 如果合并成功，说明两个批次可以合并为一个，直接返回，不再进行添加操作。
    if (!batches.Empty() && batches.Back().Merge(batch))
        return;

    // 如果上述条件都不满足，说明无法合并，将传入的批次添加到批次向量的末尾。
    batches.Push(batch);
}

}
