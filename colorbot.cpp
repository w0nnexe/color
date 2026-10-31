#include "colorbot.hpp"
#include <chrono>
#include <cmath>
#include <algorithm>
#include <random>
#include <vector>
#pragma comment(lib, "user32.lib")

namespace {
    // Standard HSV: H 0-360, S/V 0-100. Purple is the only supported color.
    constexpr HSV_RANGE kPurpleRange{ 275.0f, 320.0f, 35.0f, 100.0f, 85.0f, 100.0f };
}

ColorBot::~ColorBot() {
    Stop();
}

void ColorBot::Start() {
    if (running) return;
    running = true;
    worker = std::thread(&ColorBot::Loop, this);
}

void ColorBot::Stop() {
    running = false;
    if (worker.joinable()) worker.join();
}

int ColorBot::GetRandomInt(int min, int max) {
    if (min >= max) return min;
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> distr(min, max);
    return distr(gen);
}

void ColorBot::RGBtoHSV(int r, int g, int b, float& h, float& s, float& v) {
    float rf = r / 255.0f;
    float gf = g / 255.0f;
    float bf = b / 255.0f;

    float maxc = (std::max)({ rf, gf, bf });
    float minc = (std::min)({ rf, gf, bf });
    float delta = maxc - minc;

    v = maxc * 100.0f;

    if (maxc < 0.00001f) { s = 0.0f; h = 0.0f; return; }
    s = (delta / maxc) * 100.0f;

    if (delta < 0.00001f) { h = 0.0f; return; }

    if (maxc == rf)      h = 60.0f * fmodf((gf - bf) / delta, 6.0f);
    else if (maxc == gf) h = 60.0f * (((bf - rf) / delta) + 2.0f);
    else                 h = 60.0f * (((rf - gf) / delta) + 4.0f);

    if (h < 0.0f) h += 360.0f;
}

bool ColorBot::MatchesHSV(float h, float s, float v, const HSV_RANGE& range) {
    return h >= range.h_low && h <= range.h_high &&
           s >= range.s_low && s <= range.s_high &&
           v >= range.v_low && v <= range.v_high;
}

// ----------------------------------------------------------------------------
// BroadcastSystemMessage dispatches input to application windows.
// BSF_IGNORECURRENTTASK prevents echoing back into our own message loop.
// ----------------------------------------------------------------------------
static bool BroadcastMessageInject(UINT msg, WPARAM w, LPARAM l) {
    DWORD recipients = BSM_APPLICATIONS;
    LONG r = BroadcastSystemMessageW(
        BSF_IGNORECURRENTTASK | BSF_POSTMESSAGE | BSF_ALLOWSFW,
        &recipients, msg, w, l);
    return r > 0;
}

void ColorBot::Loop() {
    auto lastActionTime = std::chrono::steady_clock::now();

    while (running) {
        if (!config.enabled) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        int screenWidth = GetSystemMetrics(SM_CXSCREEN);
        int screenHeight = GetSystemMetrics(SM_CYSCREEN);

        RECT bounds = CaptureArea::Bounds(screenWidth, screenHeight,
            config.capture_x.load(), config.capture_y.load());
        int activeW = bounds.right - bounds.left;
        int activeH = bounds.bottom - bounds.top;
        int captureX = bounds.left;
        int captureY = bounds.top;

        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastActionTime).count();
        if (elapsed < config.cooldown_ms) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }

        HDC hScreenDC = nullptr;
        HDC hMemoryDC = nullptr;
        HBITMAP hBitmap = nullptr;
        HBITMAP hOldBitmap = nullptr;
        std::vector<RGBQUAD> pixels;
        bool havePixels = false;

        hScreenDC = GetDC(NULL);
        if (!hScreenDC) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }
        hMemoryDC = CreateCompatibleDC(hScreenDC);
        if (hMemoryDC) hBitmap = CreateCompatibleBitmap(hScreenDC, activeW, activeH);
        if (hBitmap) hOldBitmap = (HBITMAP)SelectObject(hMemoryDC, hBitmap);
        if (hOldBitmap && hOldBitmap != HGDI_ERROR) {
            BITMAPINFOHEADER bi = {};
            bi.biSize = sizeof(BITMAPINFOHEADER);
            bi.biWidth = activeW;
            bi.biHeight = -activeH;
            bi.biPlanes = 1;
            bi.biBitCount = 32;
            bi.biCompression = BI_RGB;

            pixels.resize(activeW * activeH);
            bool copied = BitBlt(hMemoryDC, 0, 0, activeW, activeH, hScreenDC,
                captureX, captureY, SRCCOPY) != FALSE;
            SelectObject(hMemoryDC, hOldBitmap);
            havePixels = copied && GetDIBits(hMemoryDC, hBitmap, 0, activeH, pixels.data(),
                (BITMAPINFO*)&bi, DIB_RGB_COLORS) == activeH;
        }

        if (hBitmap) DeleteObject(hBitmap);
        if (hMemoryDC) DeleteDC(hMemoryDC);
        if (hScreenDC) ReleaseDC(NULL, hScreenDC);

        if (!havePixels || pixels.empty()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
            continue;
        }

        bool targetFound = false;
        int tol = config.tolerance.load();
        HSV_RANGE range = kPurpleRange;
        range.s_low = (std::max)(0.0f, range.s_low - static_cast<float>(tol));
        range.s_high = (std::min)(100.0f, range.s_high + static_cast<float>(tol));
        range.v_low = (std::max)(0.0f, range.v_low - static_cast<float>(tol));
        range.v_high = (std::min)(100.0f, range.v_high + static_cast<float>(tol));

        for (const auto& pixel : pixels) {
            float h, s, v;
            RGBtoHSV(pixel.rgbRed, pixel.rgbGreen, pixel.rgbBlue, h, s, v);

            if (MatchesHSV(h, s, v, range)) { targetFound = true; break; }
        }

        if (targetFound) {
            HWND targetWindow = GetForegroundWindow();
            if (targetWindow) {
                int delay = GetRandomInt(config.delay_min_ms, config.delay_max_ms);
                std::this_thread::sleep_for(std::chrono::milliseconds(delay));

                UINT vkey = config.shoot_vkey.load();
                UINT scanCode = MapVirtualKeyA(vkey, MAPVK_VK_TO_VSC);
                bool isMouse = (vkey == VK_LBUTTON || vkey == VK_RBUTTON || vkey == VK_MBUTTON ||
                                vkey == VK_XBUTTON1 || vkey == VK_XBUTTON2);

                int holdDelay = GetRandomInt(15, 30);

                POINT curPos; GetCursorPos(&curPos);
                LPARAM mouseLParam = MAKELPARAM(curPos.x, curPos.y);
                LPARAM kdLParam = (LPARAM)(0x00000001 | (scanCode << 16));
                LPARAM kuLParam = (LPARAM)(0xC0000001 | (scanCode << 16));
                UINT mDown = 0, mUp = 0;
                WPARAM mDownW = 0, mUpW = 0;
                if (isMouse) {
                    switch (vkey) {
                    case VK_LBUTTON:  mDown = WM_LBUTTONDOWN; mUp = WM_LBUTTONUP;  mDownW = MK_LBUTTON; mUpW = 0; break;
                    case VK_RBUTTON:  mDown = WM_RBUTTONDOWN; mUp = WM_RBUTTONUP;  mDownW = MK_RBUTTON; mUpW = 0; break;
                    case VK_MBUTTON:  mDown = WM_MBUTTONDOWN; mUp = WM_MBUTTONUP;  mDownW = MK_MBUTTON; mUpW = 0; break;
                    case VK_XBUTTON1: mDown = WM_XBUTTONDOWN; mUp = WM_XBUTTONUP;  mDownW = MAKEWPARAM(MK_XBUTTON1, XBUTTON1); mUpW = MAKEWPARAM(0, XBUTTON1); break;
                    case VK_XBUTTON2: mDown = WM_XBUTTONDOWN; mUp = WM_XBUTTONUP;  mDownW = MAKEWPARAM(MK_XBUTTON2, XBUTTON2); mUpW = MAKEWPARAM(0, XBUTTON2); break;
                    default: break;
                    }
                }

                if (isMouse) {
                    BroadcastMessageInject(mDown, mDownW, mouseLParam);
                    std::this_thread::sleep_for(std::chrono::milliseconds(holdDelay));
                    BroadcastMessageInject(mUp, mUpW, mouseLParam);
                } else {
                    BroadcastMessageInject(WM_KEYDOWN, vkey, kdLParam);
                    std::this_thread::sleep_for(std::chrono::milliseconds(holdDelay));
                    BroadcastMessageInject(WM_KEYUP, vkey, kuLParam);
                }
            }
            lastActionTime = std::chrono::steady_clock::now();
        }
    }
}

