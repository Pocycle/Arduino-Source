/*  Keyboard Macro Recorder
 *
 *  From: https://github.com/PokemonAutomation/
 *
 */

#ifndef PokemonAutomation_NintendoSwitch_KeyboardMacroRecorder_H
#define PokemonAutomation_NintendoSwitch_KeyboardMacroRecorder_H

#include "NintendoSwitch/NintendoSwitch_SingleSwitchProgram.h"
#include "Common/Cpp/Options/SimpleIntegerOption.h"
#include "Common/Cpp/Options/StringOption.h"
#include "Common/Cpp/Options/BooleanCheckBoxOption.h"
#include "Common/Cpp/Options/TimeDurationOption.h"
#include "Common/Cpp/Json/JsonValue.h"
#include "Common/Cpp/Json/JsonArray.h"
#include "Common/Cpp/Json/JsonObject.h"
#include "Common/Cpp/Time.h"
#include "Controllers/KeyboardInput/KeyboardInput.h"
#include "NintendoSwitch/Controllers/NintendoSwitch_ControllerState.h"
#include "NintendoSwitch/Options/TurboMacroTable.h"

#include <vector>
#include <map>
#include <deque>
#include <mutex>
#include <atomic>
#include <thread>
#include <condition_variable>
#include <functional>

namespace PokemonAutomation{
namespace NintendoSwitch{

// Forward declarations
class KeyboardMacroRecorder_Descriptor;
class KeyboardMacroRecorder;

// Recorded action structure
struct RecordedAction {
    enum class Type {
        BUTTON_PRESS,
        BUTTON_RELEASE,
        JOYSTICK_MOVE,
        WAIT
    };

    Type type;
    WallClock timestamp;
    std::string action_name;
    uint8_t x_axis = 128;
    uint8_t y_axis = 128;
    Milliseconds duration = 0ms;
    Milliseconds release_duration = 0ms;
    Milliseconds wait_duration = 0ms;
};

// Keyboard to TurboMacro action mapping
struct KeyboardMapping {
    Qt::Key key;
    TurboMacroAction action;
    std::string description;
    
    KeyboardMapping(Qt::Key k, TurboMacroAction a, std::string desc)
        : key(k), action(a), description(std::move(desc)) {}
};

// Joystick mapping for continuous input
struct JoystickMapping {
    Qt::Key key;
    bool is_left_joystick;
    int8_t x_delta;
    int8_t y_delta;
    std::string description;
    
    JoystickMapping(Qt::Key k, bool left, int8_t x, int8_t y, std::string desc)
        : key(k), is_left_joystick(left), x_delta(x), y_delta(y), description(std::move(desc)) {}
};

class KeyboardMacroRecorder_Descriptor : public SingleSwitchProgramDescriptor{
public:
    KeyboardMacroRecorder_Descriptor();
};

// Macro recording keyboard manager that extends the existing system
class MacroRecordingKeyboardManager : public KeyboardInputController {
public:
    using KeyEventCallback = std::function<void(const QKeyEvent&)>;
    
    MacroRecordingKeyboardManager(Logger& logger, KeyEventCallback press_callback, KeyEventCallback release_callback);
    virtual ~MacroRecordingKeyboardManager();

    virtual std::unique_ptr<ControllerState> make_state() const override;
    virtual void update_state(ControllerState& state, const std::set<uint32_t>& pressed_keys) override;
    virtual void cancel_all_commands() override;
    virtual void replace_on_next_command() override;
    virtual void send_state(const ControllerState& state) override;

    // Override key event methods to capture events
    void on_key_press(const QKeyEvent& event) override;
    void on_key_release(const QKeyEvent& event) override;

private:
    KeyEventCallback m_press_callback;
    KeyEventCallback m_release_callback;
};

class KeyboardMacroRecorder : public SingleSwitchProgramInstance{
public:
    KeyboardMacroRecorder();

    virtual void program(SingleSwitchProgramEnvironment& env, ProControllerContext& context) override;

private:
    // Recording control
    void start_recording();
    void stop_recording();
    void clear_recording();
    
    // Input handling
    void on_key_press(const QKeyEvent& event);
    void on_key_release(const QKeyEvent& event);
    
    // Action processing
    void add_button_action(const QKeyEvent& event, bool is_press);
    void add_joystick_action(const QKeyEvent& event, bool is_press);
    void add_wait_action(Milliseconds duration);
    
    // Macro generation
    std::vector<TurboMacroRow> convert_to_turbo_macro();
    JsonArray generate_json_macro();
    void save_macro_to_file(const std::string& filename);
    
    // Utility functions
    TurboMacroAction map_key_to_action(Qt::Key key);
    std::string get_action_string(TurboMacroAction action);
    bool is_joystick_key(Qt::Key key);
    JoystickMapping* get_joystick_mapping(Qt::Key key);
    void update_joystick_state(Qt::Key key, bool is_press);
    void finalize_joystick_actions();
    
    // State tracking
    void update_current_joystick_state();
    void reset_joystick_state();

private:
    // UI Options
    StringOption OUTPUT_FILENAME;
    BooleanCheckBoxOption AUTO_SAVE;
    TimeDurationOption RECORDING_TIMEOUT;
    SimpleIntegerOption<uint32_t> MAX_ACTIONS;
    
    // Recording state
    std::atomic<bool> m_recording{false};
    std::atomic<bool> m_stop_recording{false};
    std::mutex m_recording_mutex;
    std::condition_variable m_recording_cv;
    
    // Recorded data
    std::deque<RecordedAction> m_recorded_actions;
    std::mutex m_actions_mutex;
    
    // Current state tracking
    std::map<Qt::Key, WallClock> m_pressed_keys;
    std::map<Qt::Key, WallClock> m_joystick_pressed_keys;
    uint8_t m_current_left_x = 128;
    uint8_t m_current_left_y = 128;
    uint8_t m_current_right_x = 128;
    uint8_t m_current_right_y = 128;
    
    // Keyboard mappings
    std::vector<KeyboardMapping> m_keyboard_mappings;
    std::vector<JoystickMapping> m_joystick_mappings;
    
    // Keyboard manager
    std::unique_ptr<MacroRecordingKeyboardManager> m_keyboard_manager;
    
    // Timing
    WallClock m_recording_start_time;
    WallClock m_last_action_time;
    
    // Statistics
    uint32_t m_total_actions = 0;
    uint32_t m_button_actions = 0;
    uint32_t m_joystick_actions = 0;
    uint32_t m_wait_actions = 0;
};

}
}
#endif 