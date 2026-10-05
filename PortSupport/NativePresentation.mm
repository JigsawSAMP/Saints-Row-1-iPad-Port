#import <UIKit/UIKit.h>
#include "NativeGame.h"
#include "NativePresentation.h"
#include <rex/runtime.h>
#include <rex/system/gpu_plugin.h>
#include <rex/ui/window_sdl.h>
#include <rex/ui/windowed_app_context_sdl.h>
#include <SDL3/SDL.h>
#define SDL_MAIN_HANDLED
#include <SDL3/SDL_main.h>
namespace {
UIView* hostView;
UIView* renderView;
UIWindow* sdlWindow;
UIWindow* appWindow;
void AttachRenderView() {
    UIView* next = sdlWindow.rootViewController.view;
    if (!next || !hostView.window) return;
    if (renderView != next) {
        [renderView removeFromSuperview];
        renderView = next;
    }
    if (renderView.superview != hostView) {
        [renderView removeFromSuperview];
        renderView.frame = hostView.bounds;
        renderView.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
        [hostView addSubview:renderView];
    }
    sdlWindow.hidden = YES;
    if (!appWindow.isKeyWindow) [appWindow makeKeyAndVisible];
}
std::unique_ptr<rex::ui::SDLWindowedAppContext> context;
std::unique_ptr<rex::ui::Window> window;
}
extern "C" void SRNativeAttachView(void* view) { hostView = (__bridge UIView*)view; }
extern "C" void SRNativePumpEvents() {
    if (renderView && hostView.window) {
        renderView.frame = hostView.bounds;
    }
    if (context) context->PumpEvents();
    // This is a controller-only app: no SDL text-input session may summon
    // the iPad keyboard when game/controller events are processed.
    if (window) {
        window->SetTextInputActive(false);
        SDL_StopTextInput(static_cast<SDL_Window*>(window->GetNativeWindowHandle()));
    }
    AttachRenderView();
}
std::unique_ptr<rex::system::IGraphicsSystem> SRPrepareGraphics() {
    __block rex::system::IGraphicsSystem* result = nullptr;
    dispatch_sync(dispatch_get_main_queue(), ^{
        SDL_SetMainReady();
        context = std::make_unique<rex::ui::SDLWindowedAppContext>();
        if (!context->Initialize()) return;
        appWindow = hostView.window;
        window = rex::ui::Window::Create(*context, "Saints Row", 1280, 720);
        if (!window->Open()) return;
        auto* native = static_cast<SDL_Window*>(window->GetNativeWindowHandle());
        sdlWindow = (__bridge UIWindow*)SDL_GetPointerProperty(SDL_GetWindowProperties(native), SDL_PROP_WINDOW_UIKIT_WINDOW_POINTER, nullptr);
        AttachRenderView();
        auto graphics = rex::system::LoadGpuPlugin("xenos");
        if (!graphics || XFAILED(graphics->SetupPresentation(context.get()))) return;
        result = graphics.release();
    });
    return std::unique_ptr<rex::system::IGraphicsSystem>(result);
}
void SRConnectPresenter(rex::Runtime* runtime) {
    dispatch_sync(dispatch_get_main_queue(), ^{
        runtime->set_display_window(window.get());
        window->SetPresenter(runtime->graphics_system()->presenter());
        AttachRenderView();
    });
}
