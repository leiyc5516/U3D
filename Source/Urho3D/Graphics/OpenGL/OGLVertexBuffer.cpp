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

#include "../../Precompiled.h"

#include "../../Graphics/Graphics.h"
#include "../../Graphics/GraphicsImpl.h"
#include "../../Graphics/VertexBuffer.h"
#include "../../IO/Log.h"

#include "../../DebugNew.h"

namespace Urho3D
{

void VertexBuffer::OnDeviceLost()
{
    if (object_.name_ && !graphics_->IsDeviceLost())
        glDeleteBuffers(1, &object_.name_);

    GPUObject::OnDeviceLost();
}

void VertexBuffer::OnDeviceReset()
{
    if (!object_.name_)
    {
        Create();
        dataLost_ = !UpdateToGPU();
    }
    else if (dataPending_)
        dataLost_ = !UpdateToGPU();

    dataPending_ = false;
}

void VertexBuffer::Release()
{
    Unlock();

    if (object_.name_)
    {
        if (!graphics_)
            return;

        if (!graphics_->IsDeviceLost())
        {
            for (unsigned i = 0; i < MAX_VERTEX_STREAMS; ++i)
            {
                if (graphics_->GetVertexBuffer(i) == this)
                    graphics_->SetVertexBuffer(nullptr);
            }

            graphics_->SetVBO(0);
            glDeleteBuffers(1, &object_.name_);
        }

        object_.name_ = 0;
    }
}

/**
 * @brief 设置顶点缓冲区的数据
 * @param data 指向顶点数据的指针
 * @return 成功返回true，失败返回false
 * 
 * 该函数用于将顶点数据设置到顶点缓冲区中。主要功能包括：
 * 1. 检查数据指针有效性
 * 2. 检查顶点大小是否已定义
 * 3. 更新影子数据(如果存在)
 * 4. 将数据上传到GPU缓冲区
 */
bool VertexBuffer::SetData(const void* data)
{
    // 检查数据指针是否为空
    if (!data)
    {
        URHO3D_LOGERROR("Null pointer for vertex buffer data");
        return false;
    }

    // 检查顶点大小是否已定义
    if (!vertexSize_)
    {
        URHO3D_LOGERROR("Vertex elements not defined, can not set vertex buffer data");
        return false;
    }

    // 如果存在影子数据且数据指针不同，则拷贝数据到影子缓冲区
    if (shadowData_ && data != shadowData_.Get())
        memcpy(shadowData_.Get(), data, vertexCount_ * (size_t)vertexSize_);

    // 如果缓冲区对象存在
    if (object_.name_)
    {
        // 检查设备是否丢失
        if (!graphics_->IsDeviceLost())
        {
            // 绑定缓冲区并上传数据
            graphics_->SetVBO(object_.name_);
            glBufferData(GL_ARRAY_BUFFER, vertexCount_ * (size_t)vertexSize_, data, dynamic_ ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);
        }
        else
        {
            // 设备丢失时标记数据待处理
            URHO3D_LOGWARNING("Vertex buffer data assignment while device is lost");
            dataPending_ = true;
        }
    }

    // 重置数据丢失标志并返回成功
    dataLost_ = false;
    return true;
}

bool VertexBuffer::SetDataRange(const void* data, unsigned start, unsigned count, bool discard)
{
    if (start == 0 && count == vertexCount_)
        return SetData(data);

    if (!data)
    {
        URHO3D_LOGERROR("Null pointer for vertex buffer data");
        return false;
    }

    if (!vertexSize_)
    {
        URHO3D_LOGERROR("Vertex elements not defined, can not set vertex buffer data");
        return false;
    }

    if (start + count > vertexCount_)
    {
        URHO3D_LOGERROR("Illegal range for setting new vertex buffer data");
        return false;
    }

    if (!count)
        return true;

    if (shadowData_ && shadowData_.Get() + start * vertexSize_ != data)
        memcpy(shadowData_.Get() + start * vertexSize_, data, count * (size_t)vertexSize_);

    if (object_.name_)
    {
        if (!graphics_->IsDeviceLost())
        {
            graphics_->SetVBO(object_.name_);
            if (!discard || start != 0)
                glBufferSubData(GL_ARRAY_BUFFER, start * (size_t)vertexSize_, count * vertexSize_, data);
            else
                glBufferData(GL_ARRAY_BUFFER, count * (size_t)vertexSize_, data, dynamic_ ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);
        }
        else
        {
            URHO3D_LOGWARNING("Vertex buffer data assignment while device is lost");
            dataPending_ = true;
        }
    }

    return true;
}

void* VertexBuffer::Lock(unsigned start, unsigned count, bool discard)
{
    if (lockState_ != LOCK_NONE)
    {
        URHO3D_LOGERROR("Vertex buffer already locked");
        return nullptr;
    }

    if (!vertexSize_)
    {
        URHO3D_LOGERROR("Vertex elements not defined, can not lock vertex buffer");
        return nullptr;
    }

    if (start + count > vertexCount_)
    {
        URHO3D_LOGERROR("Illegal range for locking vertex buffer");
        return nullptr;
    }

    if (!count)
        return nullptr;

    lockStart_ = start;
    lockCount_ = count;
    discardLock_ = discard;

    if (shadowData_)
    {
        lockState_ = LOCK_SHADOW;
        return shadowData_.Get() + start * vertexSize_;
    }
    else if (graphics_)
    {
        lockState_ = LOCK_SCRATCH;
        lockScratchData_ = graphics_->ReserveScratchBuffer(count * vertexSize_);
        return lockScratchData_;
    }
    else
        return nullptr;
}

void VertexBuffer::Unlock()
{
    switch (lockState_)
    {
    case LOCK_SHADOW:
        SetDataRange(shadowData_.Get() + lockStart_ * vertexSize_, lockStart_, lockCount_, discardLock_);
        lockState_ = LOCK_NONE;
        break;

    case LOCK_SCRATCH:
        SetDataRange(lockScratchData_, lockStart_, lockCount_, discardLock_);
        if (graphics_)
            graphics_->FreeScratchBuffer(lockScratchData_);
        lockScratchData_ = nullptr;
        lockState_ = LOCK_NONE;
        break;

    default:
        break;
    }
}

/**
 * @brief 创建OpenGL顶点缓冲区对象
 * @return 创建成功返回true，失败返回false
 * 
 * 该函数负责创建实际的OpenGL顶点缓冲区对象(VBO)，主要功能包括：
 * 1. 检查顶点数量和元素掩码是否有效
 * 2. 检查图形设备状态
 * 3. 生成新的缓冲区对象名称
 * 4. 绑定缓冲区并分配存储空间
 */
bool VertexBuffer::Create()
{
    // 检查顶点数量和元素掩码是否有效
    if (!vertexCount_ || !elementMask_)
    {
        Release();  // 无效参数时释放可能存在的资源
        return true; // 返回true表示"无操作"成功
    }

    // 检查图形子系统是否存在
    if (graphics_)
    {
        // 检查设备是否丢失
        if (graphics_->IsDeviceLost())
        {
            URHO3D_LOGWARNING("Vertex buffer creation while device is lost");
            return true; // 设备丢失时返回true(延迟创建)
        }

        // 生成新的OpenGL缓冲区对象
        if (!object_.name_)
            glGenBuffers(1, &object_.name_);
        if (!object_.name_)
        {
            URHO3D_LOGERROR("Failed to create vertex buffer");
            return false; // 生成失败返回false
        }

        // 绑定缓冲区并分配存储空间
        graphics_->SetVBO(object_.name_);
        glBufferData(GL_ARRAY_BUFFER, vertexCount_ * (size_t)vertexSize_, nullptr, dynamic_ ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);
    }

    return true; // 创建成功
}

bool VertexBuffer::UpdateToGPU()
{
    if (object_.name_ && shadowData_)
        return SetData(shadowData_.Get());
    else
        return false;
}

void* VertexBuffer::MapBuffer(unsigned start, unsigned count, bool discard)
{
    // Never called on OpenGL
    return nullptr;
}

void VertexBuffer::UnmapBuffer()
{
    // Never called on OpenGL
}

}
