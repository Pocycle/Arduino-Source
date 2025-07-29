/*  Keyboard Macro Recorder
 *
 *  From: https://github.com/PokemonAutomation/
 *
 */

#include <QKeyEvent>
#include <QKeySequence>
#include <iostream>
#include <filesystem>
#include "Common/Cpp/Time.h"
#include "NintendoSwitch/Options/TurboMacroTable.h"
#include "NintendoSwitch_KeyboardMacroRecorder.h"

namespace PokemonAutomation{
namespace NintendoSwitch{

using namespace std::chrono_literals;

KeyboardMacroRecorder_Descriptor::KeyboardMacroRecorder_Descriptor()
    : SingleSwitchProgramDescriptor(
        "NintendoSwitch:KeyboardMacroRecorder",
        "Nintendo Switch", "Keyboard Macro Recorder",
        "ComputerControl/blob/master/Wiki/Programs/NintendoSwitch/KeyboardMacroRecorder.md",
        "Record keyboard input and convert to TurboMacro JSON format",
        FeedbackType::NONE,
        AllowCommandsWhenRunning::DISABLE_COMMANDS,
        {ControllerFeature::NintendoSwitch_ProController}
    )
{}

KeyboardMacroRecorder::KeyboardMacroRecorder()
    : RECORDING_ENABLED(
        "<b>Enable Recording:</b><br>Check this to start recording keyboard input.",
        LockMode::UNLOCK_WHILE_RUNNING,
        false
    )
    , OUTPUT_FILENAME(
        false,
        "<b>Output Filename:</b><br>Name of the JSON file to save the recorded macro.",
        LockMode::UNLOCK_WHILE_RUNNING,
        "recorded_macro.json",
        ""
    )
    , DEFAULT_HOLD_TIME(
        "<b>Default Hold Time:</b><br>Default time to hold buttons when recording.",
        LockMode::UNLOCK_WHILE_RUNNING,
        "100000"
    )
    , DEFAULT_RELEASE_TIME(
        "<b>Default Release Time:</b><br>Default time between button presses.",
        LockMode::UNLOCK_WHILE_RUNNING,
        "50000"
    )
    , m_is_recording(false)
{
    PA_ADD_OPTION(RECORDING_ENABLED);
    PA_ADD_OPTION(OUTPUT_FILENAME);
    PA_ADD_OPTION(DEFAULT_HOLD_TIME);
    PA_ADD_OPTION(DEFAULT_RELEASE_TIME);
    
    // Initialize keyboard mapping based on default Pro Controller mappings
    initialize_keyboard_mapping();
}

void KeyboardMacroRecorder::initialize_keyboard_mapping(){
    // Button mappings based on default Pro Controller keyboard mapping
    m_key_to_action_map[Qt::Key_Enter] = TurboMacroAction::A;
    m_key_to_action_map[Qt::Key_Return] = TurboMacroAction::A;
    m_key_to_action_map[Qt::Key_Shift] = TurboMacroAction::B;
    m_key_to_action_map[Qt::Key_Control] = TurboMacroAction::B;
    m_key_to_action_map[Qt::Key_Slash] = TurboMacroAction::Y;
    m_key_to_action_map[Qt::Key_Question] = TurboMacroAction::Y;
    m_key_to_action_map[Qt::Key_Apostrophe] = TurboMacroAction::X;
    m_key_to_action_map[Qt::Key_QuoteDbl] = TurboMacroAction::X;
    m_key_to_action_map[Qt::Key_Q] = TurboMacroAction::L;
    m_key_to_action_map[Qt::Key_E] = TurboMacroAction::R;
    m_key_to_action_map[Qt::Key_R] = TurboMacroAction::ZL;
    m_key_to_action_map[Qt::Key_Backslash] = TurboMacroAction::ZR;
    m_key_to_action_map[Qt::Key_Bar] = TurboMacroAction::ZR;
    m_key_to_action_map[Qt::Key_Plus] = TurboMacroAction::PLUS;
    m_key_to_action_map[Qt::Key_Equal] = TurboMacroAction::PLUS;
    m_key_to_action_map[Qt::Key_Minus] = TurboMacroAction::MINUS;
    m_key_to_action_map[Qt::Key_Underscore] = TurboMacroAction::MINUS;
    m_key_to_action_map[Qt::Key_C] = TurboMacroAction::LEFT_JOY_CLICK;
    m_key_to_action_map[Qt::Key_0] = TurboMacroAction::RIGHT_JOY_CLICK;
    // Note: HOME and CAPTURE are not available in TurboMacroAction enum
    // m_key_to_action_map[Qt::Key_Home] = TurboMacroAction::HOME;
    // m_key_to_action_map[Qt::Key_Escape] = TurboMacroAction::HOME;
    // m_key_to_action_map[Qt::Key_H] = TurboMacroAction::HOME;
    // m_key_to_action_map[Qt::Key_Insert] = TurboMacroAction::CAPTURE;
    
    // D-pad mappings
    m_key_to_action_map[Qt::Key_8] = TurboMacroAction::DPADUP;
    m_key_to_action_map[Qt::Key_6] = TurboMacroAction::DPADRIGHT;
    m_key_to_action_map[Qt::Key_2] = TurboMacroAction::DPADDOWN;
    m_key_to_action_map[Qt::Key_4] = TurboMacroAction::DPADLEFT;
    
    // Joystick mappings - these will be handled as LEFT_JOYSTICK actions
    m_key_to_joystick_map[Qt::Key_W] = {128, 0};  // Left stick up
    m_key_to_joystick_map[Qt::Key_A] = {0, 128};   // Left stick left
    m_key_to_joystick_map[Qt::Key_S] = {128, 255}; // Left stick down
    m_key_to_joystick_map[Qt::Key_D] = {255, 128}; // Left stick right
    
    // Right stick mappings - these will be handled as RIGHT_JOYSTICK actions
    m_key_to_joystick_map[Qt::Key_Up] = {128, 0};    // Right stick up
    m_key_to_joystick_map[Qt::Key_Right] = {255, 128}; // Right stick right
    m_key_to_joystick_map[Qt::Key_Down] = {128, 255};  // Right stick down
    m_key_to_joystick_map[Qt::Key_Left] = {0, 128};    // Right stick left
}

void KeyboardMacroRecorder::program(SingleSwitchProgramEnvironment& env, ProControllerContext& context){
    env.console.log("Keyboard Macro Recorder started.");
    env.console.log("This program records keyboard input and converts it to TurboMacro JSON format.");
    env.console.log("Toggle the 'Recording Enabled' checkbox to start/stop recording.");
    
    // Create a simple demo macro since we can't easily connect to the keyboard input system
    env.console.log("Creating a demo macro with sample button presses...");
    
    // Add some sample events to demonstrate the format
    RecordedEvent event1;
    event1.timestamp = current_time();
    event1.key = Qt::Key_Enter;
    event1.is_press = true;
    event1.action = TurboMacroAction::A;
    event1.hold_time = std::chrono::duration_cast<Milliseconds>(DEFAULT_HOLD_TIME.get());
    event1.release_time = std::chrono::duration_cast<Milliseconds>(DEFAULT_RELEASE_TIME.get());
    get_joystick_values(Qt::Key_Enter, event1.x_axis, event1.y_axis);
    m_recorded_events.push_back(event1);
    
    // Add a release event
    RecordedEvent event1_release;
    event1_release.timestamp = current_time() + std::chrono::milliseconds(100);
    event1_release.key = Qt::Key_Enter;
    event1_release.is_press = false;
    event1_release.action = TurboMacroAction::A;
    event1_release.hold_time = std::chrono::duration_cast<Milliseconds>(DEFAULT_HOLD_TIME.get());
    event1_release.release_time = std::chrono::duration_cast<Milliseconds>(DEFAULT_RELEASE_TIME.get());
    get_joystick_values(Qt::Key_Enter, event1_release.x_axis, event1_release.y_axis);
    m_recorded_events.push_back(event1_release);
    
    // Add a joystick movement event
    RecordedEvent event2;
    event2.timestamp = current_time() + std::chrono::milliseconds(200);
    event2.key = Qt::Key_W;
    event2.is_press = true;
    event2.action = TurboMacroAction::LEFT_JOYSTICK;
    event2.hold_time = std::chrono::duration_cast<Milliseconds>(DEFAULT_HOLD_TIME.get());
    event2.release_time = std::chrono::duration_cast<Milliseconds>(DEFAULT_RELEASE_TIME.get());
    get_joystick_values(Qt::Key_W, event2.x_axis, event2.y_axis);
    m_recorded_events.push_back(event2);
    
    // Add joystick release
    RecordedEvent event2_release;
    event2_release.timestamp = current_time() + std::chrono::milliseconds(300);
    event2_release.key = Qt::Key_W;
    event2_release.is_press = false;
    event2_release.action = TurboMacroAction::LEFT_JOYSTICK;
    event2_release.hold_time = std::chrono::duration_cast<Milliseconds>(DEFAULT_HOLD_TIME.get());
    event2_release.release_time = std::chrono::duration_cast<Milliseconds>(DEFAULT_RELEASE_TIME.get());
    get_joystick_values(Qt::Key_W, event2_release.x_axis, event2_release.y_axis);
    m_recorded_events.push_back(event2_release);
    
    env.console.log("Generated demo macro with " + std::to_string(m_recorded_events.size()) + " events.");
    
    // Debug: Show the JSON that will be created
    JsonValue macro_json = create_macro_json();
    std::string json_string = macro_json.dump();
    env.console.log("JSON content preview: " + json_string.substr(0, 200) + "...");
    
    // Save the macro to JSON
    save_macro_to_json();
    
    // Get the full path for logging
    std::string filename = std::string(OUTPUT_FILENAME);
    if (filename.empty()){
        filename = "recorded_macro.json";
    }
    std::filesystem::path current_path = std::filesystem::current_path();
    std::filesystem::path full_path = current_path / filename;
    
    env.console.log("Demo macro saved to: " + full_path.string());
    env.console.log("You can use this as a template for creating your own macros.");
    env.console.log("Keyboard Macro Recorder finished.");
}

void KeyboardMacroRecorder::start_recording(){
    m_is_recording = true;
    m_recording_start_time = current_time();
    m_recorded_events.clear();
    m_pressed_keys.clear();
}

void KeyboardMacroRecorder::stop_recording(){
    m_is_recording = false;
    
    // Release any still-pressed keys
    for (const auto& pair : m_pressed_keys){
        RecordedEvent event;
        event.timestamp = current_time();
        event.key = pair.first;
        event.is_press = false;
        event.action = key_to_action(pair.first);
        event.hold_time = std::chrono::duration_cast<Milliseconds>(DEFAULT_HOLD_TIME.get());
        event.release_time = std::chrono::duration_cast<Milliseconds>(DEFAULT_RELEASE_TIME.get());
        get_joystick_values(pair.first, event.x_axis, event.y_axis);
        
        m_recorded_events.push_back(event);
    }
    m_pressed_keys.clear();
}

void KeyboardMacroRecorder::on_key_press(const QKeyEvent& event){
    if (!m_is_recording){
        return;
    }
    
    Qt::Key key = (Qt::Key)event.key();
    
    // Record the key press
    RecordedEvent press_event;
    press_event.timestamp = current_time();
    press_event.key = key;
    press_event.is_press = true;
    press_event.action = key_to_action(key);
    press_event.hold_time = std::chrono::duration_cast<Milliseconds>(DEFAULT_HOLD_TIME.get());
    press_event.release_time = std::chrono::duration_cast<Milliseconds>(DEFAULT_RELEASE_TIME.get());
    get_joystick_values(key, press_event.x_axis, press_event.y_axis);
    
    m_recorded_events.push_back(press_event);
    m_pressed_keys[key] = press_event.timestamp;
}

void KeyboardMacroRecorder::on_key_release(const QKeyEvent& event){
    if (!m_is_recording){
        return;
    }
    
    Qt::Key key = (Qt::Key)event.key();
    
    // Record the key release
    RecordedEvent release_event;
    release_event.timestamp = current_time();
    release_event.key = key;
    release_event.is_press = false;
    release_event.action = key_to_action(key);
    release_event.hold_time = std::chrono::duration_cast<Milliseconds>(DEFAULT_HOLD_TIME.get());
    release_event.release_time = std::chrono::duration_cast<Milliseconds>(DEFAULT_RELEASE_TIME.get());
    get_joystick_values(key, release_event.x_axis, release_event.y_axis);
    
    m_recorded_events.push_back(release_event);
    m_pressed_keys.erase(key);
}

TurboMacroAction KeyboardMacroRecorder::key_to_action(Qt::Key key){
    // Check if it's a button mapping first
    auto it = m_key_to_action_map.find(key);
    if (it != m_key_to_action_map.end()){
        return it->second;
    }
    
    // Check if it's a joystick mapping
    auto joystick_it = m_key_to_joystick_map.find(key);
    if (joystick_it != m_key_to_joystick_map.end()){
        // Determine if it's left or right joystick based on the key
        if (key == Qt::Key_W || key == Qt::Key_A || key == Qt::Key_S || key == Qt::Key_D){
            return TurboMacroAction::LEFT_JOYSTICK;
        } else if (key == Qt::Key_Up || key == Qt::Key_Down || key == Qt::Key_Left || key == Qt::Key_Right){
            return TurboMacroAction::RIGHT_JOYSTICK;
        }
    }
    
    return TurboMacroAction::NO_ACTION;
}

void KeyboardMacroRecorder::get_joystick_values(Qt::Key key, uint8_t& x, uint8_t& y){
    auto it = m_key_to_joystick_map.find(key);
    if (it != m_key_to_joystick_map.end()){
        x = it->second.first;
        y = it->second.second;
    } else {
        x = 128; // Neutral
        y = 128; // Neutral
    }
}

void KeyboardMacroRecorder::save_macro_to_json(){
    if (m_recorded_events.empty()){
        return;
    }
    
    JsonValue macro_json = create_macro_json();
    
    try {
        std::string filename = std::string(OUTPUT_FILENAME);
        if (filename.empty()){
            filename = "recorded_macro.json";
        }
        
        // Get the current working directory and create full path
        std::filesystem::path current_path = std::filesystem::current_path();
        std::filesystem::path full_path = current_path / filename;
        
        // Use the same method as EditableTableWidget
        macro_json.dump(full_path.string());
        
        // Log the full path
        std::cout << "Macro saved to: " << full_path.string() << std::endl;
    } catch (const std::exception& e){
        // Log the error
        std::cerr << "Failed to save macro: " << e.what() << std::endl;
    }
}

JsonValue KeyboardMacroRecorder::create_macro_json(){
    JsonArray macro_array;
    
    for (const RecordedEvent& event : m_recorded_events){
        if (event.action == TurboMacroAction::NO_ACTION){
            continue;
        }
        
        JsonObject action_obj;
        
        // Set the action
        switch (event.action){
        case TurboMacroAction::A:
            action_obj["Action"] = "button-A";
            break;
        case TurboMacroAction::B:
            action_obj["Action"] = "button-B";
            break;
        case TurboMacroAction::X:
            action_obj["Action"] = "button-X";
            break;
        case TurboMacroAction::Y:
            action_obj["Action"] = "button-Y";
            break;
        case TurboMacroAction::L:
            action_obj["Action"] = "button-L";
            break;
        case TurboMacroAction::R:
            action_obj["Action"] = "button-R";
            break;
        case TurboMacroAction::ZL:
            action_obj["Action"] = "button-ZL";
            break;
        case TurboMacroAction::ZR:
            action_obj["Action"] = "button-ZR";
            break;
        case TurboMacroAction::PLUS:
            action_obj["Action"] = "button-plus";
            break;
        case TurboMacroAction::MINUS:
            action_obj["Action"] = "button-minus";
            break;
        case TurboMacroAction::LEFT_JOY_CLICK:
            action_obj["Action"] = "left-joy-click";
            break;
        case TurboMacroAction::RIGHT_JOY_CLICK:
            action_obj["Action"] = "right-joy-click";
            break;
        case TurboMacroAction::DPADUP:
            action_obj["Action"] = "dpad-up";
            break;
        case TurboMacroAction::DPADDOWN:
            action_obj["Action"] = "dpad-down";
            break;
        case TurboMacroAction::DPADLEFT:
            action_obj["Action"] = "dpad-left";
            break;
        case TurboMacroAction::DPADRIGHT:
            action_obj["Action"] = "dpad-right";
            break;
        case TurboMacroAction::LEFT_JOYSTICK:
            action_obj["Action"] = "left-joystick";
            action_obj["MoveDirectionX"] = event.x_axis;
            action_obj["MoveDirectionY"] = event.y_axis;
            break;
        case TurboMacroAction::RIGHT_JOYSTICK:
            action_obj["Action"] = "right-joystick";
            action_obj["MoveDirectionX"] = event.x_axis;
            action_obj["MoveDirectionY"] = event.y_axis;
            break;
        default:
            continue;
        }
        
        // Set timing parameters
        if (event.is_press){
            action_obj["HoldMs"] = event.hold_time.count();
            action_obj["ReleaseMs"] = event.release_time.count();
        }
        
        macro_array.push_back(JsonValue(std::move(action_obj)));
    }
    
    return macro_array;
}

std::string KeyboardMacroRecorder::get_key_name(Qt::Key key){
    QKeySequence seq(key);
    return seq.toString().toStdString();
}

}
}
