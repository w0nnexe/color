#pragma once
#include <windows.h>
#include <atomic>
#include <thread>

namespace CaptureArea {
    inline constexpr int kMinSize = 1;
    inline constexpr int kMaxSize = 20;
    inline constexpr int kDefaultSize = 9;

    inline int ClampSize(int size) {
        return size < kMinSize ? kMinSize : (size > kMaxSize ? kMaxSize : size);
    }

    inline RECT Bounds(int screenWidth, int screenHeight, int width, int height) {
        width = ClampSize(width);
        height = ClampSize(height);
        int left = (screenWidth - width) / 2;
        int top = (screenHeight - height) / 2;
        return RECT{ left, top, left + width, top + height };
    }
}

struct HSV_RANGE {
    float h_low, h_high;
    float s_low, s_high;
    float v_low, v_high;
};

struct BotConfig {
    std::atomic<bool> enabled{ false };
    std::atomic<int> tolerance{ 25 };
    std::atomic<int> cooldown_ms{ 400 };
    std::atomic<int> delay_min_ms{ 30 };
    std::atomic<int> delay_max_ms{ 70 };
    std::atomic<int> shoot_vkey{ 0x4F }; // Default 'O'
    std::atomic<int> toggle_vkey{ 0x05 }; // Default MB4
    std::atomic<int> capture_x{ CaptureArea::kDefaultSize };
    std::atomic<int> capture_y{ CaptureArea::kDefaultSize };
    std::atomic<bool> show_capture_area{ false };
};

class ColorBot {
public:
    ColorBot() = default;
    ~ColorBot();

    void Start();
    void Stop();

    BotConfig& GetConfig() { return config; }

private:
    void Loop();
    static void RGBtoHSV(int r, int g, int b, float& h, float& s, float& v);
    static bool MatchesHSV(float h, float s, float v, const HSV_RANGE& range);
    static int GetRandomInt(int min, int max);

    BotConfig config;
    std::atomic<bool> running{ false };
    std::thread worker;
};
