#pragma once
#include "colorbot.hpp"

class CaptureOverlay {
public:
    static constexpr int kPadding = 4;

    CaptureOverlay() = default;
    ~CaptureOverlay() { if (window_) DestroyWindow(window_); }
    CaptureOverlay(const CaptureOverlay&) = delete;
    CaptureOverlay& operator=(const CaptureOverlay&) = delete;

    HWND Window() const { return window_; }

    bool Update(const BotConfig& config) {
        if (!config.show_capture_area.load()) {
            if (window_ && visible_) ShowWindow(window_, SW_HIDE);
            visible_ = false;
            failed_ = false;
            return true;
        }
        if (failed_ || !EnsureCreated()) { failed_ = true; return false; }

        RECT bounds = CaptureArea::Bounds(GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN),
            config.capture_x.load(), config.capture_y.load());
        HWND foreground = GetForegroundWindow();
        bool changed = !EqualRect(&bounds, &bounds_);
        if (changed || !visible_ || foreground != foreground_) {
            bounds_ = bounds;
            if (!SetWindowPos(window_, HWND_TOPMOST, bounds.left - kPadding, bounds.top - kPadding,
                bounds.right - bounds.left + kPadding * 2, bounds.bottom - bounds.top + kPadding * 2,
                SWP_NOACTIVATE | SWP_SHOWWINDOW))
                return false;
            InvalidateRect(window_, nullptr, FALSE);
            UpdateWindow(window_);
            visible_ = true;
            foreground_ = foreground;
        }
        return true;
    }

private:
    HWND window_ = nullptr;
    HWND foreground_ = nullptr;
    RECT bounds_ = {};
    bool visible_ = false;
    bool failed_ = false;

    bool EnsureCreated() {
        if (window_) return true;
        HINSTANCE instance = GetModuleHandleW(nullptr);
        WNDCLASSEXW wc = {};
        wc.cbSize = sizeof(wc);
        wc.hInstance = instance;
        wc.lpfnWndProc = WindowProc;
        wc.lpszClassName = L"CB_CaptureAreaOverlay";
        if (!RegisterClassExW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) return false;

        // An independent, non-activating window keeps the outline visible when the menu is hidden.
        window_ = CreateWindowExW(WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST |
            WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE, wc.lpszClassName, L"", WS_POPUP,
            0, 0, 1, 1, nullptr, nullptr, instance, this);
        if (!window_) return false;
        // Exclude the visual guide from GDI capture so the bot samples the screen underneath it.
        // https://learn.microsoft.com/windows/win32/api/winuser/nf-winuser-setwindowdisplayaffinity
        if (!SetLayeredWindowAttributes(window_, RGB(0, 0, 0), 255, LWA_COLORKEY) ||
            !SetWindowDisplayAffinity(window_, WDA_EXCLUDEFROMCAPTURE)) {
            DestroyWindow(window_);
            window_ = nullptr;
            return false;
        }
        return true;
    }

    void Paint(HDC dc) const {
        RECT client;
        GetClientRect(window_, &client);
        FillRect(dc, &client, (HBRUSH)GetStockObject(BLACK_BRUSH));
        int width = bounds_.right - bounds_.left;
        int height = bounds_.bottom - bounds_.top;
        HGDIOBJ oldBrush = SelectObject(dc, GetStockObject(NULL_BRUSH));
        HPEN shadow = CreatePen(PS_SOLID, 3, RGB(28, 27, 38));
        HGDIOBJ oldPen = SelectObject(dc, shadow);
        if (width <= 2 || height <= 2)
            Rectangle(dc, kPadding, kPadding, kPadding + width, kPadding + height);
        else
            Ellipse(dc, kPadding, kPadding, kPadding + width, kPadding + height);
        HPEN outline = CreatePen(PS_SOLID, 1, RGB(172, 154, 255));
        SelectObject(dc, outline);
        if (width == 1 && height == 1)
            SetPixelV(dc, kPadding, kPadding, RGB(172, 154, 255));
        else if (width <= 2 || height <= 2)
            Rectangle(dc, kPadding, kPadding, kPadding + width, kPadding + height);
        else
            Ellipse(dc, kPadding, kPadding, kPadding + width, kPadding + height);
        SelectObject(dc, oldPen);
        SelectObject(dc, oldBrush);
        DeleteObject(outline);
        DeleteObject(shadow);
    }

    static LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
        auto* self = reinterpret_cast<CaptureOverlay*>(GetWindowLongPtrW(window, GWLP_USERDATA));
        if (message == WM_NCCREATE) {
            self = static_cast<CaptureOverlay*>(reinterpret_cast<CREATESTRUCTW*>(lParam)->lpCreateParams);
            SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
            self->window_ = window;
        }
        if (message == WM_NCHITTEST) return HTTRANSPARENT;
        if (message == WM_MOUSEACTIVATE) return MA_NOACTIVATE;
        if (message == WM_ERASEBKGND) return 1;
        if (message == WM_PAINT && self) {
            PAINTSTRUCT paint;
            HDC dc = BeginPaint(window, &paint);
            self->Paint(dc);
            EndPaint(window, &paint);
            return 0;
        }
        if (message == WM_PRINTCLIENT && self) {
            self->Paint(reinterpret_cast<HDC>(wParam));
            return 0;
        }
        if (message == WM_NCDESTROY && self) {
            self->window_ = nullptr;
            SetWindowLongPtrW(window, GWLP_USERDATA, 0);
        }
        return DefWindowProcW(window, message, wParam, lParam);
    }
};
