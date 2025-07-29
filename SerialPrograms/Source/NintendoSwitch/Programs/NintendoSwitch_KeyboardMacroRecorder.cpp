/*  Keyboard Macro Recorder
 *
 *  From: https://github.com/PokemonAutomation/
 *
 */


#include <QKeyEvent>
#include <QKeySequence>
#include <QFileDialog>
#include <QMessageBox>
#include <QApplication>
#include <QTimer>
#include <QFile>
#include <QTextStream>
#include "Common/Cpp/Exceptions.h"
#include "Common/Cpp/Json/JsonTools.h"
#include "Common/Cpp/Time.h"
#include "CommonFramework/Logging/Logger.h"
#include "CommonFramework/GlobalSettingsPanel.h"
#include "CommonFramework/Options/Environment/PerformanceOptions.h"
#include "Controllers/KeyboardInput/GlobalQtKeyMap.h"
#include "NintendoSwitch/Commands/NintendoSwitch_Commands_PushButtons.h"
#include "NintendoSwitch_KeyboardMacroRecorder.h"

using namespace std::chrono_literals;

namespace PokemonAutomation{
namespace NintendoSwitch{

// MacroRecordingKeyboardManager implementation
MacroRecordingKeyboardManager::MacroRecordingKeyboardManager(Logger& logger, KeyEventCallback press_callback, KeyEventCallback release_callback)
    : KeyboardInputController(logger, true)
    , m_press_callback(std::move(press_callback))
    , m_release_callback(std::move(release_callback))
{
    start();
}

MacroRecordingKeyboardManager::~MacroRecordingKeyboardManager() {
    stop();
}

std::unique_ptr<ControllerState> MacroRecordingKeyboardManager::make_state() const {
    return std::make_unique<ProControllerState>();
}

void MacroRecordingKeyboardManager::update_state(ControllerState& state, const std::set<uint32_t>& pressed_keys) {
    // This is called by the base class when keyboard state changes
    // We don't need to do anything here since we're just recording
    (void)state;
    (void)pressed_keys;
}

void MacroRecordingKeyboardManager::cancel_all_commands() {
    // No commands to cancel in recording mode
}

void MacroRecordingKeyboardManager::replace_on_next_command() {
    // No commands to replace in recording mode
}

void MacroRecordingKeyboardManager::send_state(const ControllerState& state) {
    // No state to send in recording mode
    (void)state;
}

// Override the key event methods to call our callbacks
void MacroRecordingKeyboardManager::on_key_press(const QKeyEvent& event) {
    if (m_press_callback) {
        m_press_callback(event);
    }
    KeyboardInputController::on_key_press(event);
}

void MacroRecordingKeyboardManager::on_key_release(const QKeyEvent& event) {
    if (m_release_callback) {
        m_release_callback(event);
    }
    KeyboardInputController::on_key_release(event);
}



KeyboardMacroRecorder_Descriptor::KeyboardMacroRecorder_Descriptor()
    : SingleSwitchProgramDescriptor(
        "NintendoSwitch:KeyboardMacroRecorder",
        "Nintendo Switch", "Keyboard Macro Recorder",
        "ComputerControl/blob/master/Wiki/Programs/NintendoSwitch/KeyboardMacroRecorder.md",
        "Record keyboard input and generate TurboMacro files",
        FeedbackType::NONE,
        AllowCommandsWhenRunning::DISABLE_COMMANDS,
        {ControllerFeature::NintendoSwitch_ProController}
    )
{}

KeyboardMacroRecorder::KeyboardMacroRecorder()
    : OUTPUT_FILENAME(
        "<b>Output Filename:</b>",
        LockMode::UNLOCK_WHILE_RUNNING,
        "recorded_macro.json",
        ""
    )
    , AUTO_SAVE(
        "<b>Auto-save on stop:</b>",
        LockMode::UNLOCK_WHILE_RUNNING,
        true
    )
    , RECORDING_TIMEOUT(
        "<b>Recording Timeout:</b><br>Maximum time to record (0 = no limit)",
        LockMode::UNLOCK_WHILE_RUNNING,
        0ms, 0ms, 3600000ms,  // 0ms to 1 hour
        "300000 ms"  // 5 minutes default
    )
    , MAX_ACTIONS(
        "<b>Maximum Actions:</b><br>Maximum number of actions to record (0 = no limit)",
        LockMode::UNLOCK_WHILE_RUNNING,
        1000, 0, 10000,
        0
    )
{
    PA_ADD_OPTION(OUTPUT_FILENAME);
    PA_ADD_OPTION(AUTO_SAVE);
    PA_ADD_OPTION(RECORDING_TIMEOUT);
    PA_ADD_OPTION(MAX_ACTIONS);
    
    // Initialize keyboard mappings based on default ProController mappings
    initialize_keyboard_mappings();
    initialize_joystick_mappings();
}

void KeyboardMacroRecorder::initialize_keyboard_mappings() {
    // Button mappings based on default ProController keyboard mappings
    m_keyboard_mappings = {
        {Qt::Key_Slash, TurboMacroAction::Y, "Y Button"},
        {Qt::Key_Question, TurboMacroAction::Y, "Y Button"},
        {Qt::Key_Shift, TurboMacroAction::B, "B Button"},
        {Qt::Key_Control, TurboMacroAction::B, "B Button"},
        {Qt::Key_Enter, TurboMacroAction::A, "A Button"},
        {Qt::Key_Return, TurboMacroAction::A, "A Button"},
        {Qt::Key_Apostrophe, TurboMacroAction::X, "X Button"},
        {Qt::Key_QuoteDbl, TurboMacroAction::X, "X Button"},
        {Qt::Key_Q, TurboMacroAction::L, "L Button"},
        {Qt::Key_E, TurboMacroAction::R, "R Button"},
        {Qt::Key_R, TurboMacroAction::ZL, "ZL Button"},
        {Qt::Key_BraceRight, TurboMacroAction::R, "R Button"},
        {Qt::Key_BracketRight, TurboMacroAction::R, "R Button"},
        {Qt::Key_Backslash, TurboMacroAction::ZR, "ZR Button"},
        {Qt::Key_Bar, TurboMacroAction::ZR, "ZR Button"},
        {Qt::Key_V, TurboMacroAction::GL, "GL Button"},
        {Qt::Key_Period, TurboMacroAction::GR, "GR Button"},
        {Qt::Key_Greater, TurboMacroAction::GR, "GR Button"},
        {Qt::Key_Minus, TurboMacroAction::MINUS, "MINUS Button"},
        {Qt::Key_Underscore, TurboMacroAction::MINUS, "MINUS Button"},
        {Qt::Key_Plus, TurboMacroAction::PLUS, "PLUS Button"},
        {Qt::Key_Equal, TurboMacroAction::PLUS, "PLUS Button"},
        {Qt::Key_C, TurboMacroAction::LEFT_JOY_CLICK, "Left Joystick Click"},
        {Qt::Key_0, TurboMacroAction::RIGHT_JOY_CLICK, "Right Joystick Click"},
        {Qt::Key_Home, TurboMacroAction::HOME, "HOME Button"},
        {Qt::Key_Escape, TurboMacroAction::HOME, "HOME Button"},
        {Qt::Key_H, TurboMacroAction::HOME, "HOME Button"},
        {Qt::Key_Insert, TurboMacroAction::CAPTURE, "CAPTURE Button"},
        {Qt::Key_Y, TurboMacroAction::A, "A+R Button (CFW)"}, // Special case for A+R
    };
}

void KeyboardMacroRecorder::initialize_joystick_mappings() {
    // D-pad mappings
    m_joystick_mappings = {
        {Qt::Key_8, false, 0, -1, "Dpad Up"},
        {Qt::Key_9, false, 1, -1, "Dpad Up+Right"},
        {Qt::Key_6, false, 1, 0, "Dpad Right"},
        {Qt::Key_3, false, 1, 1, "Dpad Down+Right"},
        {Qt::Key_2, false, 0, 1, "Dpad Down"},
        {Qt::Key_1, false, -1, 1, "Dpad Down+Left"},
        {Qt::Key_4, false, -1, 0, "Dpad Left"},
        {Qt::Key_7, false, -1, -1, "Dpad Up+Left"},
    };
    
    // Left joystick mappings
    m_joystick_mappings.insert(m_joystick_mappings.end(), {
        {Qt::Key_W, true, 0, -1, "Left Joystick Up"},
        {Qt::Key_A, true, -1, 0, "Left Joystick Left"},
        {Qt::Key_S, true, 0, 1, "Left Joystick Down"},
        {Qt::Key_D, true, 1, 0, "Left Joystick Right"},
    });
    
    // Right joystick mappings
    m_joystick_mappings.insert(m_joystick_mappings.end(), {
        {Qt::Key_Up, false, 0, -1, "Right Joystick Up"},
        {Qt::Key_Right, false, 1, 0, "Right Joystick Right"},
        {Qt::Key_Down, false, 0, 1, "Right Joystick Down"},
        {Qt::Key_Left, false, -1, 0, "Right Joystick Left"},
    });
}

void KeyboardMacroRecorder::program(SingleSwitchProgramEnvironment& env, ProControllerContext& context) {
    env.console.log("Keyboard Macro Recorder Starting...", COLOR_BLUE);
    env.console.log("Press keys to record actions. The program will capture:", COLOR_CYAN);
    env.console.log("- Button presses and releases", COLOR_CYAN);
    env.console.log("- Joystick movements", COLOR_CYAN);
    env.console.log("- Timing between actions", COLOR_CYAN);
    env.console.log("", COLOR_WHITE);
    env.console.log("Note: This program works with the existing keyboard input system.", COLOR_CYAN);
    env.console.log("Make sure the Nintendo Switch controller is connected and keyboard input is enabled.", COLOR_CYAN);
    env.console.log("", COLOR_WHITE);
    
    // Set up keyboard manager with callbacks
    m_keyboard_manager = std::make_unique<MacroRecordingKeyboardManager>(
        env.console,
        [this](const QKeyEvent& event) { this->on_key_press(event); },
        [this](const QKeyEvent& event) { this->on_key_release(event); }
    );
    
    env.console.log("Keyboard input system initialized successfully.", COLOR_GREEN);
    
    // Start recording
    start_recording();
    
    // Main recording loop
    WallClock start_time = current_time();
    
    while (true) {
        // Check for timeout
        if (RECORDING_TIMEOUT > 0ms && current_time() - start_time > RECORDING_TIMEOUT) {
            env.console.log("Recording timeout reached. Stopping...", COLOR_YELLOW);
            break;
        }
        
        // Check for maximum actions
        if (MAX_ACTIONS > 0 && m_total_actions >= MAX_ACTIONS) {
            env.console.log("Maximum actions reached. Stopping...", COLOR_YELLOW);
            break;
        }
        
        // Check if recording should stop
        if (m_stop_recording.load()) {
            break;
        }
        
        // Small delay to prevent busy waiting
        std::this_thread::sleep_for(10ms);
    }
    
    // Stop recording
    stop_recording();
    
    // Clean up keyboard manager
    m_keyboard_manager.reset();
    
    // Generate and save macro
    if (!m_recorded_actions.empty()) {
        env.console.log("Generating TurboMacro file...", COLOR_BLUE);
        
        try {
            JsonArray macro_json = generate_json_macro();
            std::string filename = OUTPUT_FILENAME;
            
            if (AUTO_SAVE) {
                save_macro_to_file(filename);
                env.console.log("Macro saved to: " + filename, COLOR_GREEN);
            } else {
                // Show the JSON in console for manual copy
                env.console.log("Generated Macro JSON:", COLOR_GREEN);
                env.console.log(macro_json.dump(), COLOR_WHITE);
            }
            
            // Print statistics
            env.console.log("Recording Statistics:", COLOR_CYAN);
            env.console.log("- Total Actions: " + std::to_string(m_total_actions), COLOR_WHITE);
            env.console.log("- Button Actions: " + std::to_string(m_button_actions), COLOR_WHITE);
            env.console.log("- Joystick Actions: " + std::to_string(m_joystick_actions), COLOR_WHITE);
            env.console.log("- Wait Actions: " + std::to_string(m_wait_actions), COLOR_WHITE);
            
        } catch (const Exception& e) {
            env.console.log("Error generating macro: " + e.message(), COLOR_RED);
        }
    } else {
        env.console.log("No actions recorded.", COLOR_YELLOW);
    }
    
    env.console.log("Keyboard Macro Recorder finished.", COLOR_BLUE);
}

void KeyboardMacroRecorder::start_recording() {
    std::lock_guard<std::mutex> lock(m_recording_mutex);
    
    if (m_recording.load()) {
        return; // Already recording
    }
    
    m_recording.store(true);
    m_stop_recording.store(false);
    m_recording_start_time = current_time();
    m_last_action_time = m_recording_start_time;
    
    // Clear previous recording
    clear_recording();
}

void KeyboardMacroRecorder::stop_recording() {
    std::lock_guard<std::mutex> lock(m_recording_mutex);
    
    if (!m_recording.load()) {
        return; // Not recording
    }
    
    m_recording.store(false);
    m_stop_recording.store(true);
    
    // Finalize any pending joystick actions
    finalize_joystick_actions();
}

void KeyboardMacroRecorder::clear_recording() {
    std::lock_guard<std::mutex> lock(m_actions_mutex);
    
    m_recorded_actions.clear();
    m_pressed_keys.clear();
    m_joystick_pressed_keys.clear();
    reset_joystick_state();
    
    m_total_actions = 0;
    m_button_actions = 0;
    m_joystick_actions = 0;
    m_wait_actions = 0;
}

void KeyboardMacroRecorder::on_key_press(const QKeyEvent& event) {
    if (!m_recording.load()) {
        return;
    }
    
    Qt::Key key = static_cast<Qt::Key>(event.key());
    WallClock now = current_time();
    
    // Check if this is a joystick key
    if (is_joystick_key(key)) {
        add_joystick_action(event, true);
    } else {
        add_button_action(event, true);
    }
    
    m_last_action_time = now;
}

void KeyboardMacroRecorder::on_key_release(const QKeyEvent& event) {
    if (!m_recording.load()) {
        return;
    }
    
    Qt::Key key = static_cast<Qt::Key>(event.key());
    WallClock now = current_time();
    
    // Check if this is a joystick key
    if (is_joystick_key(key)) {
        add_joystick_action(event, false);
    } else {
        add_button_action(event, false);
    }
    
    m_last_action_time = now;
}



void KeyboardMacroRecorder::add_button_action(const QKeyEvent& event, bool is_press) {
    Qt::Key key = static_cast<Qt::Key>(event.key());
    WallClock now = current_time();
    
    // Find the mapping for this key
    TurboMacroAction action = map_key_to_action(key);
    if (action == TurboMacroAction::NO_ACTION) {
        return; // No mapping found
    }
    
    RecordedAction recorded_action;
    recorded_action.timestamp = now;
    recorded_action.action_name = get_action_string(action);
    
    if (is_press) {
        recorded_action.type = RecordedAction::Type::BUTTON_PRESS;
        m_pressed_keys[key] = now;
    } else {
        recorded_action.type = RecordedAction::Type::BUTTON_RELEASE;
        
        // Calculate duration if we have a press time
        auto press_iter = m_pressed_keys.find(key);
        if (press_iter != m_pressed_keys.end()) {
            recorded_action.duration = std::chrono::duration_cast<Milliseconds>(now - press_iter->second);
            m_pressed_keys.erase(press_iter);
        }
    }
    
    // Add to recorded actions
    {
        std::lock_guard<std::mutex> lock(m_actions_mutex);
        m_recorded_actions.push_back(recorded_action);
        m_total_actions++;
        m_button_actions++;
    }
}

void KeyboardMacroRecorder::add_joystick_action(const QKeyEvent& event, bool is_press) {
    Qt::Key key = static_cast<Qt::Key>(event.key());
    WallClock now = current_time();
    
    JoystickMapping* mapping = get_joystick_mapping(key);
    if (!mapping) {
        return; // No mapping found
    }
    
    update_joystick_state(key, is_press);
    
    // Record the joystick movement
    RecordedAction recorded_action;
    recorded_action.timestamp = now;
    recorded_action.type = RecordedAction::Type::JOYSTICK_MOVE;
    
    if (mapping->is_left_joystick) {
        recorded_action.action_name = "left-joystick";
        recorded_action.x_axis = m_current_left_x;
        recorded_action.y_axis = m_current_left_y;
    } else {
        recorded_action.action_name = "right-joystick";
        recorded_action.x_axis = m_current_right_x;
        recorded_action.y_axis = m_current_right_y;
    }
    
    // Add to recorded actions
    {
        std::lock_guard<std::mutex> lock(m_actions_mutex);
        m_recorded_actions.push_back(recorded_action);
        m_total_actions++;
        m_joystick_actions++;
    }
}

void KeyboardMacroRecorder::add_wait_action(Milliseconds duration) {
    if (duration <= 0ms) {
        return;
    }
    
    RecordedAction recorded_action;
    recorded_action.type = RecordedAction::Type::WAIT;
    recorded_action.timestamp = current_time();
    recorded_action.action_name = "wait";
    recorded_action.wait_duration = duration;
    
    {
        std::lock_guard<std::mutex> lock(m_actions_mutex);
        m_recorded_actions.push_back(recorded_action);
        m_total_actions++;
        m_wait_actions++;
    }
}

TurboMacroAction KeyboardMacroRecorder::map_key_to_action(Qt::Key key) {
    for (const auto& mapping : m_keyboard_mappings) {
        if (mapping.key == key) {
            return mapping.action;
        }
    }
    return TurboMacroAction::NO_ACTION;
}

std::string KeyboardMacroRecorder::get_action_string(TurboMacroAction action) {
    switch (action) {
    case TurboMacroAction::A: return "button-A";
    case TurboMacroAction::B: return "button-B";
    case TurboMacroAction::X: return "button-X";
    case TurboMacroAction::Y: return "button-Y";
    case TurboMacroAction::L: return "button-L";
    case TurboMacroAction::R: return "button-R";
    case TurboMacroAction::ZL: return "button-ZL";
    case TurboMacroAction::ZR: return "button-ZR";
    case TurboMacroAction::PLUS: return "button-plus";
    case TurboMacroAction::MINUS: return "button-minus";
    case TurboMacroAction::LEFT_JOY_CLICK: return "left-joy-click";
    case TurboMacroAction::RIGHT_JOY_CLICK: return "right-joy-click";
    case TurboMacroAction::DPADLEFT: return "dpad-left";
    case TurboMacroAction::DPADRIGHT: return "dpad-right";
    case TurboMacroAction::DPADUP: return "dpad-up";
    case TurboMacroAction::DPADDOWN: return "dpad-down";
    case TurboMacroAction::HOME: return "button-home";
    case TurboMacroAction::CAPTURE: return "button-capture";
    case TurboMacroAction::GL: return "button-gl";
    case TurboMacroAction::GR: return "button-gr";
    default: return "no-action";
    }
}

bool KeyboardMacroRecorder::is_joystick_key(Qt::Key key) {
    return get_joystick_mapping(key) != nullptr;
}

JoystickMapping* KeyboardMacroRecorder::get_joystick_mapping(Qt::Key key) {
    for (auto& mapping : m_joystick_mappings) {
        if (mapping.key == key) {
            return &mapping;
        }
    }
    return nullptr;
}

void KeyboardMacroRecorder::update_joystick_state(Qt::Key key, bool is_press) {
    JoystickMapping* mapping = get_joystick_mapping(key);
    if (!mapping) {
        return;
    }
    
    if (is_press) {
        m_joystick_pressed_keys[key] = current_time();
    } else {
        m_joystick_pressed_keys.erase(key);
    }
    
    update_current_joystick_state();
}

void KeyboardMacroRecorder::update_current_joystick_state() {
    // Reset to neutral position
    m_current_left_x = 128;
    m_current_left_y = 128;
    m_current_right_x = 128;
    m_current_right_y = 128;
    
    // Apply all currently pressed joystick keys
    for (const auto& [key, timestamp] : m_joystick_pressed_keys) {
        JoystickMapping* mapping = get_joystick_mapping(key);
        if (!mapping) {
            continue;
        }
        
        if (mapping->is_left_joystick) {
            m_current_left_x = std::clamp(m_current_left_x + mapping->x_delta * 128, 0, 255);
            m_current_left_y = std::clamp(m_current_left_y + mapping->y_delta * 128, 0, 255);
        } else {
            m_current_right_x = std::clamp(m_current_right_x + mapping->x_delta * 128, 0, 255);
            m_current_right_y = std::clamp(m_current_right_y + mapping->y_delta * 128, 0, 255);
        }
    }
}

void KeyboardMacroRecorder::reset_joystick_state() {
    m_current_left_x = 128;
    m_current_left_y = 128;
    m_current_right_x = 128;
    m_current_right_y = 128;
}

void KeyboardMacroRecorder::finalize_joystick_actions() {
    // Add final joystick positions if any are still pressed
    if (!m_joystick_pressed_keys.empty()) {
        update_current_joystick_state();
        
        // Add final joystick state
        if (m_current_left_x != 128 || m_current_left_y != 128) {
            RecordedAction action;
            action.type = RecordedAction::Type::JOYSTICK_MOVE;
            action.timestamp = current_time();
            action.action_name = "left-joystick";
            action.x_axis = m_current_left_x;
            action.y_axis = m_current_left_y;
            
            std::lock_guard<std::mutex> lock(m_actions_mutex);
            m_recorded_actions.push_back(action);
        }
        
        if (m_current_right_x != 128 || m_current_right_y != 128) {
            RecordedAction action;
            action.type = RecordedAction::Type::JOYSTICK_MOVE;
            action.timestamp = current_time();
            action.action_name = "right-joystick";
            action.x_axis = m_current_right_x;
            action.y_axis = m_current_right_y;
            
            std::lock_guard<std::mutex> lock(m_actions_mutex);
            m_recorded_actions.push_back(action);
        }
    }
}



JsonArray KeyboardMacroRecorder::generate_json_macro() {
    JsonArray macro_array;
    
    std::lock_guard<std::mutex> lock(m_actions_mutex);
    
    for (const auto& action : m_recorded_actions) {
        JsonObject action_obj;
        
        switch (action.type) {
        case RecordedAction::Type::BUTTON_PRESS:
        case RecordedAction::Type::BUTTON_RELEASE:
            action_obj["Action"] = action.action_name;
            if (action.duration > 0ms) {
                action_obj["HoldMs"] = std::to_string(action.duration.count()) + " ms";
            }
            if (action.release_duration > 0ms) {
                action_obj["ReleaseMs"] = std::to_string(action.release_duration.count()) + " ms";
            }
            break;
            
        case RecordedAction::Type::JOYSTICK_MOVE:
            action_obj["Action"] = action.action_name;
            action_obj["MoveDirectionX"] = (int)action.x_axis;
            action_obj["MoveDirectionY"] = (int)action.y_axis;
            if (action.duration > 0ms) {
                action_obj["HoldMs"] = std::to_string(action.duration.count()) + " ms";
            }
            if (action.release_duration > 0ms) {
                action_obj["ReleaseMs"] = std::to_string(action.release_duration.count()) + " ms";
            }
            break;
            
        case RecordedAction::Type::WAIT:
            action_obj["Action"] = "wait";
            if (action.wait_duration > 0ms) {
                action_obj["WaitMs"] = std::to_string(action.wait_duration.count()) + " ms";
            }
            break;
        }
        
        macro_array.push_back(action_obj);
    }
    
    return macro_array;
}

void KeyboardMacroRecorder::save_macro_to_file(const std::string& filename) {
    JsonArray macro_json = generate_json_macro();
    std::string json_string = macro_json.dump();
    
    QFile file(QString::fromStdString(filename));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        throw Exception("Failed to open file for writing: " + filename);
    }
    
    QTextStream out(&file);
    out << QString::fromStdString(json_string);
    file.close();
}

}
} 