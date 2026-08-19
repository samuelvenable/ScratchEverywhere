#ifndef LIBRETRO
#include "image.hpp"
#include "translation.hpp"
#include <log.hpp>
#ifdef ENABLE_MENU
#include <menus/mainMenu.hpp>
#endif
#include <string>
#include <cstdlib>
#include <inspector.hpp>
#include <render.hpp>
#include <runtime.hpp>
#include <unzip.hpp>

#ifdef ENABLE_AUDIO
#include <audio.hpp>
#endif

#ifdef __SWITCH__
#include <switch.h>
#endif

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten_browser_file.h>
#include <filesystem.hpp>
#endif

#if defined(SE_USE_LIBRARY_BUILD)
#if defined(_WIN32) || defined(_WIN64) || defined(__APPLE__) || (defined(__linux__) && !defined(__ANDROID__) && !defined(WEBOS)) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__DragonFly__) || (defined(__sun) && defined(__SVR4))
#include <libdlgmod/libdlgmod.h>
#if !defined(USE_LIBDLGMOD)
#define USE_LIBDLGMOD
#endif
#include <cctype>
#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#elif defined(__APPLE__)
#include <AppKit/AppKit.h>
#elif __has_include(<X11/Xlib.h>) && __has_include(<X11/Xutil.h>)
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#endif
#endif
static ScriptThread monitorDisplayThread;
#endif

static void exitApp() {
    Render::deInit();
    OS::deinit();
}

#if defined(SE_USE_LIBRARY_BUILD)
#if defined(USE_LIBDLGMOD)
static void scratchEverywhereCleanUp() {
    Scratch::cleanupScratchProject();
    Render::deInit();
    OS::deinit();
    exit(0);
}
#if defined(_WIN32) || defined(_WIN64)
static WNDPROC OriginalWndProc = nullptr;
LRESULT CALLBACK CustomWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_SYSCOMMAND:
        if ((wParam & 0xFFF0) == SC_CLOSE) {
            scratchEverywhereCleanUp();
            return 0;
        }
        break;
    case WM_CLOSE:
        scratchEverywhereCleanUp();
        return 0;
        break;
    }
    return CallWindowProc(OriginalWndProc, hwnd, msg, wParam, lParam);
}
#elif defined(__APPLE__)
@interface WindowDelegate : NSObject <NSWindowDelegate>
@end
@implementation WindowDelegate
- (BOOL)windowShouldClose:(id)sender {
    scratchEverywhereCleanUp();
	return YES;
}
@end
#endif
#endif
#if defined(_WIN32) || defined(_WIN64)
extern "C" __declspec(dllexport) void scratch_everywhere_destroy() {
#else
extern "C" __attribute__((visibility("default"))) void scratch_everywhere_destroy() {
#endif
    scratchEverywhereCleanUp();
}
#if defined(_WIN32) || defined(_WIN64)
extern "C" __declspec(dllexport) const char *scratch_everywhere_step() {
#else
extern "C" __attribute__((visibility("default"))) const char *scratch_everywhere_step() {
#endif
    static char buffer[4];
    std::pair<bool, bool> code = Scratch::stepScratchProject(monitorDisplayThread);
    const int first  = ((code.first)  ? 1 : 0);
    const int second = ((code.second) ? 1 : 0);
    snprintf(buffer, sizeof(buffer), "%d:%d", first, second);
    return static_cast<const char *>buffer;
}
#if defined(USE_LIBDLGMOD)
#if !defined(_WIN32) && !defined(_WIN64) && !defined(__APPLE__)
static int XErrorHandlerImpl(Display *display, XErrorEvent *event) {
  return 0;
}
static int XIOErrorHandlerImpl(Display *display) {
  return 0;
}
#endif
#endif
static void scratchEverywhereEmbedInParentWindow(std::string window) {
#if defined(USE_LIBDLGMOD)
#if defined(_WIN32) || defined(_WIN64)
	HWND scratch_everywhere_window = (HWND)(void *)strtoull(widget_get_owner(), nullptr, 10);
	HWND scratch_everywhere_parent_window = (HWND)(void *)strtoull(window.c_str(), nullptr, 10);
    if (IsIconic(scratch_everywhere_parent_window)) ShowWindow(scratch_everywhere_parent_window, SW_RESTORE);
	SetWindowLongPtrW(scratch_everywhere_window, GWLP_HWNDPARENT, (LONG_PTR)(void *)scratch_everywhere_parent_window);
	RECT rect; GetClientRect(scratch_everywhere_parent_window, &rect); MoveWindow(scratch_everywhere_window, 0, 0, (rect.right - rect.left), (rect.bottom - rect.top), TRUE);
    SetWindowLongPtrW(scratch_everywhere_parent_window, GWL_STYLE, (GetWindowLongPtrW(scratch_everywhere_parent_window, GWL_STYLE) | WS_CLIPCHILDREN | WS_CLIPSIBLINGS) & ~(WS_THICKFRAME | WS_MAXIMIZEBOX));
	SetWindowLongPtrW(scratch_everywhere_window, GWL_STYLE, (GetWindowLongPtrW(scratch_everywhere_window, GWL_STYLE) | WS_POPUP) & ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU));
	SetWindowLongPtrW(scratch_everywhere_window, GWL_EXSTYLE, GetWindowLongPtrW(scratch_everywhere_window, GWL_EXSTYLE) & ~(WS_EX_DLGMODALFRAME | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE));
    SetWindowPos(scratch_everywhere_parent_window, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED); 
	SetWindowPos(scratch_everywhere_window, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
	OriginalWndProc = (WNDPROC)SetWindowLongPtrW(scratch_everywhere_parent_window, GWLP_WNDPROC, (LONG_PTR)CustomWndProc);
	SetParent(scratch_everywhere_window, scratch_everywhere_parent_window);
#elif defined(__APPLE__)
	// On macOS the OS is so locked-down that this only works for windows belonging to the same process:
	NSWindow *scratch_everywhere_window = (NSWindow *)(void *)strtoull(widget_get_owner(), nullptr, 10);
	NSWindow *scratch_everywhere_parent_window = (NSWindow *)(void *)strtoull(window.c_str(), nullptr, 10);
	[scratch_everywhere_parent_window addChildWindow:scratch_everywhere_window ordered:NSWindowAbove];
	[scratch_everywhere_window setStyleMask:NSWindowStyleMaskBorderless]; NSEvent *event = [NSEvent mouseEventWithType:NSEventTypeLeftMouseDown location:
	NSMakePoint(scratch_everywhere_window.frame.size.width / 2, scratch_everywhere_window.frame.size.height / 2) modifierFlags:0 timestamp:0 windowNumber:
	[scratch_everywhere_window windowNumber] context:nullptr eventNumber:0 clickCount:1 pressure:1.0]; [scratch_everywhere_window sendEvent:event];
	NSPoint origin = scratch_everywhere_parent_window.frame.origin; NSSize size = scratch_everywhere_parent_window.contentView.bounds.size; 
	[scratch_everywhere_window setFrame:NSMakeRect(origin.x, origin.y, size.width, size.height) display:YES animate:NO];
	[[scratch_everywhere_parent_window standardWindowButton:NSWindowZoomButton] setEnabled:NO];
	scratch_everywhere_parent_window.styleMask &= ~NSWindowStyleMaskResizable;
	WindowDelegate *delegate = [[WindowDelegate alloc] init];
	[scratch_everywhere_parent_window setDelegate:delegate];
#elif __has_include(<X11/Xlib.h>) && __has_include(<X11/Xutil.h>)
    // Error handlers force ignored failures on Wayland, thus avoiding segfaults:
  	XSetErrorHandler(XErrorHandlerImpl); XSetIOErrorHandler(XIOErrorHandlerImpl); 
    Display *display = XOpenDisplay(nullptr); Window scratch_everywhere_window = 
    (Window)strtoul(widget_get_owner(), nullptr, 10); Window scratch_everywhere_parent_window = 
    (Window)strtoul(window.c_str(), nullptr, 10); XSetTransientForHint(display, scratch_everywhere_window, 
    scratch_everywhere_parent_window); XReparentWindow(display, scratch_everywhere_window, 
    scratch_everywhere_parent_window, 0, 0); XWindowAttributes attr; XGetWindowAttributes(display, 
    scratch_everywhere_parent_window, &attr); XResizeWindow(display, scratch_everywhere_window, attr.width, 
    attr.height); XSizeHints *sh = XAllocSizeHints(); sh->flags = PMinSize | PMaxSize; sh->min_width = 
    sh->max_width = attr.width; sh->min_height = sh->max_height = attr.height; XSetWMNormalHints(display, 
    scratch_everywhere_parent_window, sh); XFree(sh); XCloseDisplay(display);
    // FIXME: This is too much work to rewrite in Wayland (might do later)...
#endif
#endif
}
#endif

static bool initApp(int width, int height, bool resizable, std::string title) {
    return Scratch::initializeRuntime(width, height, resizable, title);
}

bool activateMainMenu() {
#ifdef ENABLE_MENU
    MainMenu *menu = new MainMenu();
    if (Unzip::filePath.empty()) MenuManager::changeMenu(menu);

    while (Render::appShouldRun()) {
        MenuManager::render();

        if (MenuManager::isProjectLoaded != 0) {
            if (MenuManager::isProjectLoaded == -1) return false;
            MenuManager::isProjectLoaded = 0;
            return true;
        }

#ifdef __EMSCRIPTEN__
        emscripten_sleep(0);
#endif
#ifdef ENABLE_INSPECTOR
        Inspector::processCommands();
#endif
    }
#endif
    return false;
}

void mainLoop() {
    Scratch::startScratchProject();

    if (Scratch::nextProject) {
        Log::log(Unzip::filePath);
        if (Unzip::load()) {
            goto skipCheck;
        }

        if (Unzip::projectOpened != -3) {
            exitApp();
            exit(0);
        }

#if defined(ENABLE_MENU)
        if (!activateMainMenu()) {
            exitApp();
            exit(0);
        }
#endif

    skipCheck:
        return;
    }

    Unzip::filePath = "";
    Scratch::nextProject = false;
    Scratch::dataNextProject = Value();
#if defined(ENABLE_MENU)
    if (OS::toExit || !activateMainMenu()) {
#else
    if (OS::toExit) {
#endif
        exitApp();
        exit(0);
    }
}

#if !defined(SE_USE_LIBRARY_BUILD)
#if defined(WINDOWING_SDL1) || defined(WINDOWING_SDL2)
#include <SDL.h>

extern "C" int main(int argc, char **argv) {
#else
int main(int argc, char **argv) {
#endif
#else
#if defined(_WIN32) || defined(_WIN64)
extern "C" __declspec(dllexport) const char *scratch_everywhere_create(const char *sb3, const char *win) {
#else
extern "C" __attribute__((visibility("default"))) const char *scratch_everywhere_create(const char *sb3, const char *win) {
#endif
#endif
#if defined(SE_USE_LIBRARY_BUILD) && defined(USE_LIBDLGMOD)
    if (!initApp(640, 480, false, "Scratch Everywhere!")) {
#else
    if (!initApp(-1, -1, true, "Scratch Everywhere!")) {
#endif
#if !defined(SE_USE_LIBRARY_BUILD)
        exitApp();
        return 1;
#endif
    }

    srand(time(nullptr));

    bool enableInspector = false;
#if !defined(SE_USE_LIBRARY_BUILD)
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--inspector") {
            enableInspector = true;
        } else if (Unzip::filePath.empty()) {
#if defined(__PC__)
            Unzip::filePath = arg;
#endif
        }
    }
#endif

#ifdef ENABLE_INSPECTOR
    if (enableInspector) Inspector::init();
#endif

#if defined(__EMSCRIPTEN__)
    if (argc > 1) {
        while (!FileSystem::fileExists("/romfs/project.sb3")) {
            if (!Render::appShouldRun()) {
                exitApp();
                exit(0);
            }
            emscripten_sleep(0);
        }
    }
#endif

    if (!Unzip::load()) {
        if (Unzip::projectOpened == -3) {
#ifdef __EMSCRIPTEN__
            bool uploadComplete = false;
            emscripten_browser_file::upload(".sb3", [](std::string const &filename, std::string const &mime_type, std::string_view buffer, void *userdata) {
                *(bool *)userdata = true;
                if (!FileSystem::fileExists(OS::getScratchFolderLocation())) FileSystem::createDirectory(OS::getScratchFolderLocation());
                std::ofstream f(OS::getScratchFolderLocation() + filename);
                f << buffer;
                f.close();
                Unzip::filePath = OS::getScratchFolderLocation() + filename;
                Unzip::load(); // TODO: Error handling
            },
                                            &uploadComplete);
            while (Render::appShouldRun() && !uploadComplete)
                emscripten_sleep(0);
#else
#if defined(ENABLE_MENU)
            if (!activateMainMenu()) {
                exitApp();
                return 0;
            }
#endif
#endif
        } else {
#if !defined(SE_USE_LIBRARY_BUILD)
            exitApp();
            return 0;
#endif
        }
    }

#if defined(SE_USE_LIBRARY_BUILD)
    Unzip::filePath = sb3;
    Unzip::load();
    Scratch::initializeScratchProject();
#if defined(USE_LIBDLGMOD)
	if (!win.empty() && !win.compare("0") && isdigit(win[0])) {
		scratchEverywhereEmbedInParentWindow(win);
	}
#endif
#if defined(USE_LIBDLGMOD)
	return widget_get_owner();
#else
	return "0";
#endif
#else
#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(mainLoop, 0, 1);
#else
    while (true)
        mainLoop();
#endif
    exitApp();
    return 0;
#endif
}
#endif
