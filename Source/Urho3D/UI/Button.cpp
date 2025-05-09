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
#include "../Input/InputEvents.h"
#include "../UI/Button.h"
#include "../UI/UI.h"
#include "../UI/UIEvents.h"

#include "../DebugNew.h"

namespace Urho3D
{

extern const char* UI_CATEGORY;

Button::Button(Context* context) :
    BorderImage(context),
    pressedOffset_(IntVector2::ZERO),
    pressedChildOffset_(IntVector2::ZERO),
    repeatDelay_(1.0f),
    repeatRate_(0.0f),
    repeatTimer_(0.0f),
    pressed_(false)
{
    SetEnabled(true);
    focusMode_ = FM_FOCUSABLE;
}

Button::~Button() = default;

void Button::RegisterObject(Context* context)
{
    context->RegisterFactory<Button>(UI_CATEGORY);

    URHO3D_COPY_BASE_ATTRIBUTES(BorderImage);
    URHO3D_UPDATE_ATTRIBUTE_DEFAULT_VALUE("Is Enabled", true);
    URHO3D_UPDATE_ATTRIBUTE_DEFAULT_VALUE("Focus Mode", FM_FOCUSABLE);
    URHO3D_ACCESSOR_ATTRIBUTE("Pressed Image Offset", GetPressedOffset, SetPressedOffset, IntVector2, IntVector2::ZERO, AM_FILE);
    URHO3D_ACCESSOR_ATTRIBUTE("Pressed Child Offset", GetPressedChildOffset, SetPressedChildOffset, IntVector2, IntVector2::ZERO, AM_FILE);
    URHO3D_ACCESSOR_ATTRIBUTE("Repeat Delay", GetRepeatDelay, SetRepeatDelay, float, 1.0f, AM_FILE);
    URHO3D_ACCESSOR_ATTRIBUTE("Repeat Rate", GetRepeatRate, SetRepeatRate, float, 0.0f, AM_FILE);
}

void Button::Update(float timeStep)
{
    // 如果按钮处于按下状态但鼠标不再悬停，则重置按下状态
    if (!hovering_ && pressed_)
        SetPressed(false);

    // 处理按钮重复按下事件
    if (pressed_ && repeatRate_ > 0.0f)
    {
        // 更新重复计时器
        repeatTimer_ -= timeStep;
        
        // 检查是否达到重复触发时间间隔
        if (repeatTimer_ <= 0.0f)
        {
            // 重置计时器，基于重复频率
            repeatTimer_ += 1.0f / repeatRate_;

            // 发送按钮重复按下事件
            using namespace Pressed;
            VariantMap& eventData = GetEventDataMap();
            eventData[P_ELEMENT] = this;  // 设置事件源为当前按钮
            SendEvent(E_PRESSED, eventData);  // 发送按下事件
        }
    }
}

/**
 * @brief 获取按钮的渲染批次数据
 * 
 * 此函数根据按钮的当前状态（启用、悬停、按下、选中、禁用等）计算偏移量，
 * 然后调用基类 BorderImage 的 GetBatches 函数来收集渲染批次数据。
 * 
 * @param batches 用于存储渲染批次的向量
 * @param vertexData 用于存储顶点数据的向量
 * @param currentScissor 当前的裁剪矩形，用于确定哪些区域需要渲染
 */
void Button::GetBatches(PODVector<UIBatch>& batches, PODVector<float>& vertexData, const IntRect& currentScissor)
{
    // 初始化偏移量为零向量
    IntVector2 offset(IntVector2::ZERO);
    // 检查按钮是否启用
    if (enabled_)
    {
        // 如果鼠标悬停在按钮上或者按钮获得焦点，则添加悬停偏移量
        if (hovering_ || HasFocus())
            offset += hoverOffset_;
        // 如果按钮被按下或者被选中，则添加按下偏移量
        if (pressed_ || selected_)
            offset += pressedOffset_;
    }
    else
    {
        // 如果按钮被禁用，则添加禁用偏移量
        offset += disabledOffset_;
    }

    // 调用基类 BorderImage 的 GetBatches 函数，并传入计算好的偏移量
    BorderImage::GetBatches(batches, vertexData, currentScissor, offset);
}

void Button::OnClickBegin(const IntVector2& position, const IntVector2& screenPosition, MouseButton button, MouseButtonFlags buttons, QualifierFlags qualifiers,
    Cursor* cursor)
{
    if (button == MOUSEB_LEFT)
    {
        SetPressed(true);
        repeatTimer_ = repeatDelay_;
        hovering_ = true;

        using namespace Pressed;

        VariantMap& eventData = GetEventDataMap();
        eventData[P_ELEMENT] = this;
        SendEvent(E_PRESSED, eventData);
    }
}

void Button::OnClickEnd(const IntVector2& position, const IntVector2& screenPosition, MouseButton button, MouseButtonFlags buttons, QualifierFlags qualifiers,
    Cursor* cursor, UIElement* beginElement)
{
    if (pressed_ && button == MOUSEB_LEFT)
    {
        SetPressed(false);
        // If mouse was released on top of the element, consider it hovering on this frame yet (see issue #1453)
        if (IsInside(screenPosition, true))
            hovering_ = true;

        using namespace Released;

        VariantMap& eventData = GetEventDataMap();
        eventData[P_ELEMENT] = this;
        SendEvent(E_RELEASED, eventData);
    }
}

void Button::OnDragMove(const IntVector2& position, const IntVector2& screenPosition, const IntVector2& deltaPos, MouseButtonFlags buttons,
    QualifierFlags qualifiers, Cursor* cursor)
{
    SetPressed(true);
}

void Button::OnKey(Key key, MouseButtonFlags buttons, QualifierFlags qualifiers)
{
    if (HasFocus() && (key == KEY_RETURN || key == KEY_RETURN2 || key == KEY_KP_ENTER || key == KEY_SPACE))
    {
        // Simulate LMB click
        OnClickBegin(IntVector2(), IntVector2(), MOUSEB_LEFT, MOUSEB_NONE, QUAL_NONE, nullptr);
        OnClickEnd(IntVector2(), IntVector2(), MOUSEB_LEFT, MOUSEB_NONE, QUAL_NONE, nullptr, nullptr);
    }
}

void Button::SetPressedOffset(const IntVector2& offset)
{
    pressedOffset_ = offset;
}

void Button::SetPressedOffset(int x, int y)
{
    pressedOffset_ = IntVector2(x, y);
}

void Button::SetPressedChildOffset(const IntVector2& offset)
{
    pressedChildOffset_ = offset;
}

void Button::SetPressedChildOffset(int x, int y)
{
    pressedChildOffset_ = IntVector2(x, y);
}

void Button::SetRepeat(float delay, float rate)
{
    SetRepeatDelay(delay);
    SetRepeatRate(rate);
}

void Button::SetRepeatDelay(float delay)
{
    repeatDelay_ = Max(delay, 0.0f);
}

void Button::SetRepeatRate(float rate)
{
    repeatRate_ = Max(rate, 0.0f);
}

void Button::SetPressed(bool enable)
{
    pressed_ = enable;
    SetChildOffset(pressed_ ? pressedChildOffset_ : IntVector2::ZERO);
}

}
