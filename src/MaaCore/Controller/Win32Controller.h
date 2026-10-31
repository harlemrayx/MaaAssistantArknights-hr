#pragma once

#ifdef _WIN32

#include <deque>
#include <filesystem>
#include <memory>
#include <mutex>
#include <string>

#include "MaaUtils/SafeWindows.hpp"

#include "Common/AsstMsg.h"
#include "ControllerAPI.h"
#include "InstHelper.h"
#include "Win32ControlUnitLoader.h"

namespace asst
{
class Assistant;

class Win32Controller : public ControllerAPI, private InstHelper
{
public:
    Win32Controller(const AsstCallback& callback, Assistant* inst);
    virtual ~Win32Controller() override;

    Win32Controller(const Win32Controller&) = delete;
    Win32Controller& operator=(const Win32Controller&) = delete;
    Win32Controller(Win32Controller&&) = delete;
    Win32Controller& operator=(Win32Controller&&) = delete;

    // 绑定到窗口（替代 connect）
    bool attach(
        void* hwnd,
        Win32ScreencapMethod screencap_method,
        Win32InputMethod mouse_method,
        Win32InputMethod keyboard_method);

    void restore_window_position();

public: // ControllerAPI 接口
    virtual bool connect(const std::string& adb_path, const std::string& address, const std::string& config) override;
    virtual bool inited() const noexcept override;

    virtual const std::string& get_uuid() const override;

    virtual size_t get_pipe_data_size() const noexcept override { return 0; }

    virtual size_t get_version() const noexcept override { return 0; }

    virtual bool screencap(cv::Mat& image_payload, bool allow_reconnect = false) override;
    virtual bool screencap_for_preview(cv::Mat& image_payload, bool allow_reconnect = false) override;

    virtual bool start_game(const std::string& client_type) override;
    virtual bool stop_game(const std::string& client_type) override;

    virtual bool click(const Point& p) override;
    virtual bool input(const std::string& text) override;
    virtual bool swipe(
        const Point& p1,
        const Point& p2,
        int duration = 0,
        SwipeExtraDirection extra_swipe = SwipeExtraDirection::None,
        double slope_in = 1,
        double slope_out = 1,
        bool with_pause = false) override;

    virtual bool inject_input_event(const InputEvent& event) override;

    virtual bool press_esc() override;
    virtual void set_main_screen_recognition(bool on) override;
    virtual ControlFeat::Feat support_features() const noexcept override;

    virtual std::pair<int, int> get_screen_res() const noexcept override;

private:
    bool screencap_impl(cv::Mat& image_payload, bool move_cursor);
    void callback(AsstMsg msg, const json::value& details);
    // 记录窗口当前位置，任务结束时恢复
    void save_window_position();

    // 游戏重启后窗口句柄会变化，以下用于重新找到并绑定游戏窗口
    // 记录当前窗口的标题、窗口类与所属进程路径
    void record_window_identity();
    // 窗口类与所属程序（取不到程序路径时比较标题）均与记录一致才算游戏窗口
    bool is_game_window(HWND hwnd) const;
    bool is_attached_window_alive() const;
    // 按记录的窗口特征查找游戏窗口，preferred_pid 非 0 时优先返回该进程的窗口
    HWND find_game_window(DWORD preferred_pid) const;
    // 启动游戏进程，成功时返回进程 id（经 UAC 启动时可能取不到，为 0）
    bool launch_game_process(DWORD& pid) const;
    // 等待新窗口出现且客户区尺寸稳定
    HWND wait_for_game_window(DWORD pid);
    // 沿用当前截图/输入方式绑定到新窗口，并通知界面窗口句柄已变化
    bool reattach_window(HWND hwnd, const std::string& why);
    // 截图失败且原窗口已失效时，尝试找回游戏窗口；找不到则报告断开
    bool recover_lost_window();

    // 创建并连接控制单元，成功后替换当前单元
    bool create_unit(void* hwnd);
    // 取当前控制单元的引用，重新绑定窗口时旧单元会在进行中的调用结束后再销毁
    std::shared_ptr<MaaFwControlUnitAPI> acquire_unit() const;

    // 封装 MaaWin32ControlUnit 的调用
    bool prepare_window_for_input();
    bool unit_screencap(cv::Mat& image);
    bool unit_click(int x, int y);
    bool unit_swipe(int x1, int y1, int x2, int y2, int duration);
    bool unit_touch_down(int contact, int x, int y, int pressure);
    bool unit_touch_move(int contact, int x, int y, int pressure);
    bool unit_touch_up(int contact);
    bool unit_input_text(const std::string& text);
    bool unit_key_down(int key);
    bool unit_key_up(int key);
    bool unit_click_key(int key);

private:
    AsstCallback m_callback = nullptr;
    std::unique_ptr<Win32ControlUnitLoader> m_loader;
    mutable std::mutex m_unit_mutex;
    std::shared_ptr<MaaFwControlUnitAPI> m_unit;
    void* m_hwnd = nullptr;

    std::wstring m_window_title;
    std::wstring m_window_class;
    std::filesystem::path m_game_exe_path;
    // 最近一次由 start_game 启动的游戏进程
    DWORD m_launched_pid = 0;

    bool m_inited = false;
    std::string m_uuid;
    std::pair<int, int> m_screen_size = { 0, 0 };

    Win32ScreencapMethod m_screencap_method = Win32Screencap::None;
    Win32InputMethod m_mouse_method = Win32Input::None;
    Win32InputMethod m_keyboard_method = Win32Input::None;

    std::deque<long long> m_screencap_cost;
    int m_screencap_times = 0;

    bool m_main_screen_recognition = false;
    RECT m_original_window_rect = { 0, 0, 0, 0 };
    bool m_window_rect_saved = false;
};
} // namespace asst

#endif // _WIN32
