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

#include "../Engine/Application.h"
#include "../IO/IOEvents.h"
#include "../IO/Log.h"

#if defined(IOS) || defined(TVOS)
#include "../Graphics/Graphics.h"
#include <SDL/SDL.h>
#endif

#include "../DebugNew.h"

namespace Urho3D
{

#if defined(IOS) || defined(TVOS) || defined(__EMSCRIPTEN__)
// Code for supporting SDL_iPhoneSetAnimationCallback() and emscripten_set_main_loop_arg()
#if defined(__EMSCRIPTEN__)
#include <emscripten/emscripten.h>
#endif
void RunFrame(void* data)
{
    static_cast<Engine*>(data)->RunFrame();
}
#endif

Application::Application(Context* context) :
    Object(context),
    exitCode_(EXIT_SUCCESS)
{
    engineParameters_ = Engine::ParseParameters(GetArguments());

    // Create the Engine, but do not initialize it yet. Subsystems except Graphics & Renderer are registered at this point
    engine_ = new Engine(context);

    // Subscribe to log messages so that can show errors if ErrorExit() is called with empty message
    SubscribeToEvent(E_LOGMESSAGE, URHO3D_HANDLER(Application, HandleLogMessage));
}

int Application::Run()
{
#if !defined(__GNUC__) || __EXCEPTIONS
    try  // 异常处理：非GCC编译器或启用了异常处理时使用try块
    {
#endif
        Setup();  // 执行应用初始化设置
        if (exitCode_)  // 如果初始化过程中设置了退出码
            return exitCode_;  // 直接返回错误码

        if (!engine_->Initialize(engineParameters_))  // 初始化引擎
        {
            ErrorExit();  // 初始化失败时调用错误处理
            return exitCode_;  // 返回失败状态码
        }

        Start();  // 调用应用启动逻辑
        if (exitCode_)  // 检查启动过程中是否出错
            return exitCode_;  // 返回错误码

        // 非iOS/tvOS/Emscripten平台使用阻塞式主循环
#if !defined(IOS) && !defined(TVOS) && !defined(__EMSCRIPTEN__)
        while (!engine_->IsExiting())  // 主循环：直到引擎请求退出
            engine_->RunFrame();  // 执行每帧逻辑

        Stop();  // 调用应用停止逻辑
        // iOS/tvOS平台使用动画回调机制，因此不支持手动调用Stop()
#else
#if defined(IOS) || defined(TVOS)
        // iOS/tvOS: 设置SDL动画回调
        SDL_iPhoneSetAnimationCallback(GetSubsystem<Graphics>()->GetWindow(), 1, &RunFrame, engine_);
#elif defined(__EMSCRIPTEN__)
        // Emscripten: 设置主循环回调
        emscripten_set_main_loop_arg(RunFrame, engine_, 0, 1);
#endif
#endif

        return exitCode_;  // 返回最终退出码
#if !defined(__GNUC__) || __EXCEPTIONS
    }
    catch (std::bad_alloc&)  // 捕获内存分配异常
    {
        ErrorDialog(GetTypeName(), "An out-of-memory error occurred. The application will now exit.");
        return EXIT_FAILURE;  // 返回内存不足错误码
    }
#endif
}

void Application::ErrorExit(const String& message)
{
    engine_->Exit(); // Close the rendering window
    exitCode_ = EXIT_FAILURE;

    if (!message.Length())
    {
        ErrorDialog(GetTypeName(), startupErrors_.Length() ? startupErrors_ :
            "Application has been terminated due to unexpected error.");
    }
    else
        ErrorDialog(GetTypeName(), message);
}

void Application::HandleLogMessage(StringHash eventType, VariantMap& eventData)
{
    using namespace LogMessage;

    if (eventData[P_LEVEL].GetInt() == LOG_ERROR)
    {
        // Strip the timestamp if necessary
        String error = eventData[P_MESSAGE].GetString();
        unsigned bracketPos = error.Find(']');
        if (bracketPos != String::NPOS)
            error = error.Substring(bracketPos + 2);

        startupErrors_ += error + "\n";
    }
}


}
