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

#include "../Math/Color.h"
#include "../Math/Rect.h"
#include "../Graphics/GraphicsDefs.h"
#include "../Graphics/Material.h"

namespace Urho3D
{

class PixelShader;
class Graphics;
class Matrix3x4;
class Texture;
class UIElement;

static const unsigned UI_VERTEX_SIZE = 6;

/**
 * @class UIBatch
 * @brief 表示 UI 渲染的绘制调用。包含了渲染一个 UI 元素所需的所有信息，如混合模式、裁剪矩形、纹理等。
 * 该类提供了多种方法来添加不同类型的四边形到渲染批次中，并且支持批次合并操作。
 */
class URHO3D_API UIBatch
{
public:
    /// 用默认值构造 UIBatch 对象。
    UIBatch();
    /**
     * @brief 构造 UIBatch 对象。
     * @param element 此批次所代表的 UI 元素。
     * @param blendMode 混合模式。
     * @param scissor 裁剪矩形。
     * @param texture 纹理。
     * @param vertexData 顶点数据指针。
     */
    UIBatch(UIElement* element, BlendMode blendMode, const IntRect& scissor, Texture* texture, PODVector<float>* vertexData);

    /**
     * @brief 为批次设置新的颜色，会覆盖渐变效果。
     * @param color 要设置的颜色。
     * @param overrideAlpha 是否覆盖透明度。
     */
    void SetColor(const Color& color, bool overrideAlpha = false);
    /// 恢复 UI 元素的默认颜色。
    void SetDefaultColor();
    /**
     * @brief 添加一个四边形到批次中。
     * @param x 四边形的 x 坐标。
     * @param y 四边形的 y 坐标。
     * @param width 四边形的宽度。
     * @param height 四边形的高度。
     * @param texOffsetX 纹理的 x 偏移量。
     * @param texOffsetY 纹理的 y 偏移量。
     * @param texWidth 纹理的宽度，默认为 0。
     * @param texHeight 纹理的高度，默认为 0。
     */
    void AddQuad(float x, float y, float width, float height, int texOffsetX, int texOffsetY, int texWidth = 0, int texHeight = 0);
    /**
     * @brief 使用变换矩阵添加一个四边形到批次中。
     * @param transform 变换矩阵。
     * @param x 四边形的 x 坐标。
     * @param y 四边形的 y 坐标。
     * @param width 四边形的宽度。
     * @param height 四边形的高度。
     * @param texOffsetX 纹理的 x 偏移量。
     * @param texOffsetY 纹理的 y 偏移量。
     * @param texWidth 纹理的宽度，默认为 0。
     * @param texHeight 纹理的高度，默认为 0。
     */
    void AddQuad(const Matrix3x4& transform, int x, int y, int width, int height, int texOffsetX, int texOffsetY, int texWidth = 0,
        int texHeight = 0);
    /**
     * @brief 添加一个使用平铺纹理的四边形到批次中。
     * @param x 四边形的 x 坐标。
     * @param y 四边形的 y 坐标。
     * @param width 四边形的宽度。
     * @param height 四边形的高度。
     * @param texOffsetX 纹理的 x 偏移量。
     * @param texOffsetY 纹理的 y 偏移量。
     * @param texWidth 纹理的宽度。
     * @param texHeight 纹理的高度。
     * @param tiled 是否使用平铺纹理。
     */
    void AddQuad(int x, int y, int width, int height, int texOffsetX, int texOffsetY, int texWidth, int texHeight, bool tiled);
    /**
     * @brief 添加一个具有自由点和 UV 坐标的四边形到批次中。使用当前颜色，不使用渐变。
     * 点应按顺时针顺序指定。
     * @param transform 变换矩阵。
     * @param a 四边形的第一个点。
     * @param b 四边形的第二个点。
     * @param c 四边形的第三个点。
     * @param d 四边形的第四个点。
     * @param texA 第一个点的纹理坐标。
     * @param texB 第二个点的纹理坐标。
     * @param texC 第三个点的纹理坐标。
     * @param texD 第四个点的纹理坐标。
     */
    void AddQuad(const Matrix3x4& transform, const IntVector2& a, const IntVector2& b, const IntVector2& c, const IntVector2& d,
        const IntVector2& texA, const IntVector2& texB, const IntVector2& texC, const IntVector2& texD);
    /**
     * @brief 添加一个具有自由点、UV 坐标和颜色的四边形到批次中。
     * 点应按顺时针顺序指定。
     * @param transform 变换矩阵。
     * @param a 四边形的第一个点。
     * @param b 四边形的第二个点。
     * @param c 四边形的第三个点。
     * @param d 四边形的第四个点。
     * @param texA 第一个点的纹理坐标。
     * @param texB 第二个点的纹理坐标。
     * @param texC 第三个点的纹理坐标。
     * @param texD 第四个点的纹理坐标。
     * @param colA 第一个点的颜色。
     * @param colB 第二个点的颜色。
     * @param colC 第三个点的颜色。
     * @param colD 第四个点的颜色。
     */
    void AddQuad(const Matrix3x4& transform, const IntVector2& a, const IntVector2& b, const IntVector2& c, const IntVector2& d,
        const IntVector2& texA, const IntVector2& texB, const IntVector2& texC, const IntVector2& texD, const Color& colA,
        const Color& colB, const Color& colC, const Color& colD);
    /**
     * @brief 尝试将另一个批次合并到当前批次中。
     * @param batch 要合并的批次。
     * @return 如果合并成功返回 true，否则返回 false。
     */
    bool Merge(const UIBatch& batch);
    /**
     * @brief 返回 UI 元素的插值颜色。
     * @param x 计算颜色的 x 坐标。
     * @param y 计算颜色的 y 坐标。
     * @return 插值后的颜色。
     */
    unsigned GetInterpolatedColor(float x, float y);

    /**
     * @brief 添加或合并一个批次到批次列表中。
     * @param batch 要添加或合并的批次。
     * @param batches 批次列表。
     */
    static void AddOrMerge(const UIBatch& batch, PODVector<UIBatch>& batches);

    /// 此批次所代表的 UI 元素。
    UIElement* element_{};
    /// 混合模式。
    BlendMode blendMode_{BLEND_REPLACE};
    /// 裁剪矩形。
    IntRect scissor_;
    /// 纹理。
    Texture* texture_{};
    /// 纹理的逆尺寸。
    Vector2 invTextureSize_{Vector2::ONE};
    /// 顶点数据指针。
    PODVector<float>* vertexData_{};
    /// 顶点数据的起始索引。
    unsigned vertexStart_{};
    /// 顶点数据的结束索引。
    unsigned vertexEnd_{};
    /// 当前颜色，默认根据元素计算。
    unsigned color_{};
    /// 渐变标志。
    bool useGradient_{};
    /// 自定义材质。
    Material* customMaterial_{};

    /// 用于像素完美渲染的位置调整向量，由 UI 初始化。
    static Vector3 posAdjust;
};

}
