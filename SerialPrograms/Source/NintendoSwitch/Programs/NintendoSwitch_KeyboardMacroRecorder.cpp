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
#include "Common/Cpp/Exceptions.h"
#include "NintendoSwitch/Options/TurboMacroTable.h"
#include "NintendoSwitch/NintendoSwitch_Settings.h"
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
        AllowCommandsWhenRunning::ENABLE_COMMANDS,
        {ControllerFeature::NintendoSwitch_ProController}
    )
{}

KeyboardMacroRecorder::KeyboardMacroRecorder()
    : OUTPUT_FILENAME(
        false,
        "<b>Output Filename:</b><br>Name of the JSON file to save the recorded macro.",
        LockMode::UNLOCK_WHILE_RUNNING,
        "recorded_macro.json",
        ""
    )
    , DEFAULT_HOLD_TIME(
        "<b>Default Hold Time:</b><br>Default time to hold buttons when recording.",
        LockMode::UNLOCK_WHILE_RUNNING,
        0ms, Milliseconds::max(),
        "100 ms"
    )
    , DEFAULT_RELEASE_TIME(
        "<b>Default Release Time:</b><br>Default time between button presses.",
        LockMode::UNLOCK_WHILE_RUNNING,
        0ms, Milliseconds::max(),
        "50 ms"
    )
    , m_is_recording(false)
    , m_first_run(true)
{
    PA_ADD_OPTION(OUTPUT_FILENAME);
    PA_ADD_OPTION(DEFAULT_HOLD_TIME);
    PA_ADD_OPTION(DEFAULT_RELEASE_TIME);
    // Initialize keyboard mapping based on default Pro Controller mappings
    initialize_keyboard_mapping();
}

KeyboardMacroRecorder::~KeyboardMacroRecorder(){
    // Note: Cleanup is handled in the program method when ProgramCancelledException is caught
    // This ensures the recording is saved immediately when the user clicks "Stop Program"
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
    if (m_first_run){
        env.console.log("Keyboard Macro Recorder started.");
        env.console.log("This program records keyboard input and converts it to TurboMacro JSON format.");
        env.console.log("Recording will start automatically and continue until you click 'Stop Program'.");
        env.console.log("The macro will be saved when the program stops.");
        m_first_run = false;
    }
    
    // Start recording immediately
    env.console.log("Starting recording...");
    start_recording();
    
    // Register with the keyboard input system
    ProController& controller = static_cast<ProController&>(context.controller());
    controller.add_keyboard_callback(static_cast<KeyboardEventCallback*>(this));
    
    env.console.log("Recording started. Press keys to record your macro.");
    env.console.log("Click 'Stop Program' to stop recording and save the macro.");
    
    // Run continuously until Stop Program is called
    try {
        while (true){
            // Check if the program should stop
            context.throw_if_cancelled();
            
            // Small delay to prevent busy waiting
            context.wait_for(std::chrono::milliseconds(100));
        }
    } catch (const ProgramCancelledException&) {
        // Program was stopped by user - save the recording
        env.console.log("Program stopped by user. Saving recording...");
        
        // Unregister from the keyboard input system
        controller.remove_keyboard_callback(static_cast<KeyboardEventCallback*>(this));
        
        stop_recording();
        save_recording();
        env.console.log("Recording saved successfully.");
        throw; // Re-throw to let the framework handle it
    }
}

void KeyboardMacroRecorder::start_recording(){
    m_is_recording = true;
    m_recording_start_time = current_time();
    m_recorded_events.clear();
    m_pressed_keys.clear();
}

void KeyboardMacroRecorder::stop_recording(){
    m_is_recording = false;
    
    // Release any still-pressed keys with proper timing
    WallClock stop_time = current_time();
    for (const auto& pair : m_pressed_keys){
        Qt::Key key = pair.first;
        WallClock press_time = pair.second;
        Milliseconds actual_hold_time = std::chrono::duration_cast<Milliseconds>(stop_time - press_time);
        
        // Update the hold time in the existing press event
        for (auto& recorded_event : m_recorded_events) {
            if (recorded_event.key == key && recorded_event.is_press && recorded_event.timestamp == press_time) {
                recorded_event.hold_time = actual_hold_time;
                break;
            }
        }
        
        // Add a release event
        RecordedEvent release_event;
        release_event.timestamp = stop_time;
        release_event.key = key;
        release_event.is_press = false;
        release_event.action = key_to_action(key);
        release_event.hold_time = Milliseconds::zero();
        release_event.release_time = Milliseconds::zero();
        get_joystick_values(key, release_event.x_axis, release_event.y_axis);
        
        m_recorded_events.push_back(release_event);
    }
    m_pressed_keys.clear();
}





void KeyboardMacroRecorder::save_recording(){
    if (m_recorded_events.empty()){
        return;
    }
    
    save_macro_to_json();
    
    // Get the full path for logging
    std::string filename = std::string(OUTPUT_FILENAME);
    if (filename.empty()){
        filename = "recorded_macro.json";
    }
    std::filesystem::path current_path = std::filesystem::current_path();
    std::filesystem::path full_path = current_path / filename;
    
    std::cout << "Macro saved to: " << full_path.string() << std::endl;
    std::cout << "Recorded " << m_recorded_events.size() << " events." << std::endl;
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
    
    // Log the key press for debugging
    std::cout << "Key pressed: " << get_key_name(key) << " -> " << action_to_string(press_event.action) << std::endl;
}

void KeyboardMacroRecorder::on_key_release(const QKeyEvent& event){
    if (!m_is_recording){
        return;
    }
    
    Qt::Key key = (Qt::Key)event.key();
    
    // Find the corresponding press event and update its hold time
    auto press_it = m_pressed_keys.find(key);
    if (press_it != m_pressed_keys.end()) {
        WallClock press_time = press_it->second;
        WallClock release_time = current_time();
        Milliseconds actual_hold_time = std::chrono::duration_cast<Milliseconds>(release_time - press_time);
        
        // Update the hold time in the press event
        for (auto& recorded_event : m_recorded_events) {
            if (recorded_event.key == key && recorded_event.is_press && recorded_event.timestamp == press_time) {
                recorded_event.hold_time = actual_hold_time;
                break;
            }
        }
    }
    
    // Record the key release (but don't add timing info to release events)
    RecordedEvent release_event;
    release_event.timestamp = current_time();
    release_event.key = key;
    release_event.is_press = false;
    release_event.action = key_to_action(key);
    release_event.hold_time = Milliseconds::zero(); // No timing for release events
    release_event.release_time = Milliseconds::zero(); // No timing for release events
    get_joystick_values(key, release_event.x_axis, release_event.y_axis);
    
    m_recorded_events.push_back(release_event);
    m_pressed_keys.erase(key);
    
    // Log the key release for debugging
    std::cout << "Key released: " << get_key_name(key) << " -> " << action_to_string(release_event.action) << std::endl;
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
    
    WallClock last_action_time = m_recording_start_time;
    const Milliseconds MIN_WAIT_TIME = 50ms; // Minimum gap to consider as a wait
    
    for (const RecordedEvent& event : m_recorded_events){
        if (event.action == TurboMacroAction::NO_ACTION){
            continue;
        }
        
        // Check if we need to add a wait action
        if (event.timestamp > last_action_time) {
            Milliseconds gap = std::chrono::duration_cast<Milliseconds>(event.timestamp - last_action_time);
            if (gap >= MIN_WAIT_TIME) {
                JsonObject wait_obj;
                wait_obj["Action"] = "wait";
                wait_obj["WaitMs"] = gap.count();
                macro_array.push_back(JsonValue(std::move(wait_obj)));
            }
        }
        
        // Only process press events for actions (skip release events)
        if (!event.is_press) {
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
        
        // Set timing parameters only for press events
        if (event.hold_time > Milliseconds::zero()) {
            action_obj["HoldMs"] = event.hold_time.count();
        }
        if (event.release_time > Milliseconds::zero()) {
            action_obj["ReleaseMs"] = event.release_time.count();
        }
        
        macro_array.push_back(JsonValue(std::move(action_obj)));
        last_action_time = event.timestamp;
    }
    
    return macro_array;
}

std::string KeyboardMacroRecorder::get_key_name(Qt::Key key){
    QKeySequence seq(key);
    return seq.toString().toStdString();
}

std::string KeyboardMacroRecorder::action_to_string(TurboMacroAction action){
    switch (action){
    case TurboMacroAction::NO_ACTION: return "No Action";
    case TurboMacroAction::LEFT_JOYSTICK: return "Left Joystick";
    case TurboMacroAction::RIGHT_JOYSTICK: return "Right Joystick";
    case TurboMacroAction::LEFT_JOY_CLICK: return "Left Joy Click";
    case TurboMacroAction::RIGHT_JOY_CLICK: return "Right Joy Click";
    case TurboMacroAction::B: return "B";
    case TurboMacroAction::A: return "A";
    case TurboMacroAction::Y: return "Y";
    case TurboMacroAction::X: return "X";
    case TurboMacroAction::R: return "R";
    case TurboMacroAction::L: return "L";
    case TurboMacroAction::ZR: return "ZR";
    case TurboMacroAction::ZL: return "ZL";
    case TurboMacroAction::PLUS: return "PLUS";
    case TurboMacroAction::MINUS: return "MINUS";
    case TurboMacroAction::DPADLEFT: return "DPad Left";
    case TurboMacroAction::DPADRIGHT: return "DPad Right";
    case TurboMacroAction::DPADUP: return "DPad Up";
    case TurboMacroAction::DPADDOWN: return "DPad Down";
    case TurboMacroAction::WAIT: return "Wait";
    default: return "Unknown";
    }
}

}
}
