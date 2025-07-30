/*  Keyboard Macro Recorder
 *
 *  From: https://github.com/PokemonAutomation/
 *
 */

#include <iostream>
#include <filesystem>
#include <fstream>
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
        "Nintendo Switch",
        "Keyboard Macro Recorder",
        "ComputerControl/blob/master/Wiki/Programs/NintendoSwitch/KeyboardMacroRecorder.md",
        "Record controller commands and convert to TurboMacro JSON format",
        FeedbackType::NONE,
        AllowCommandsWhenRunning::ENABLE_COMMANDS,
        {ControllerFeature::NintendoSwitch_ProController},
        FasterIfTickPrecise::NOT_FASTER,
        false
    )
{}

KeyboardMacroRecorder::KeyboardMacroRecorder()
    : OUTPUT_FILENAME(
        false,
        "<b>Output Filename:</b><br>Save the recorded macro to this file.",
        LockMode::UNLOCK_WHILE_RUNNING,
        "macro.json",
        "",
        false
    )
    , m_is_recording(false)
    , m_first_run(true)
{
}

KeyboardMacroRecorder::~KeyboardMacroRecorder(){
    // Ensure recording is stopped and saved when the program is destroyed
    if (m_is_recording){
        stop_recording();
        save_recording();
    }
}

void KeyboardMacroRecorder::program(SingleSwitchProgramEnvironment& env, ProControllerContext& context){
    if (m_first_run){
        env.console.log("Controller Macro Recorder started.");
        env.console.log("This program records controller commands and converts them to TurboMacro JSON format.");
        env.console.log("Recording will start automatically and continue until you click 'Stop Program'.");
        env.console.log("The macro will be saved when the program stops.");
        m_first_run = false;
    }
    
    // Start recording immediately
    env.console.log("Starting recording...");
    start_recording();
    
    // Register with the controller system
    ProController& controller = static_cast<ProController&>(context.controller());
    controller.add_controller_callback(static_cast<ControllerEventCallback*>(this));
    
    env.console.log("Recording started. Use the controller to record your macro.");
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
        
        // Unregister from the controller system
        controller.remove_controller_callback(static_cast<ControllerEventCallback*>(this));
        
        stop_recording();
        save_recording();
        env.console.log("Recording saved successfully.");
        throw; // Re-throw to let the framework handle it
    } catch (...) {
        // Any other exception - still try to save
        if (m_is_recording) {
            env.console.log("Program stopped due to error. Attempting to save recording...");
            
            // Unregister from the controller system
            controller.remove_controller_callback(static_cast<ControllerEventCallback*>(this));
            
            stop_recording();
            save_recording();
        }
        throw; // Re-throw the original exception
    }
}

void KeyboardMacroRecorder::start_recording(){
    m_is_recording = true;
    m_recording_start_time = current_time();
    m_recorded_events.clear();
}

void KeyboardMacroRecorder::stop_recording(){
    m_is_recording = false;
    m_recording_stop_time = current_time();
}

void KeyboardMacroRecorder::save_recording(){
    if (m_recorded_events.empty()){
        std::cout << "No events recorded. Nothing to save." << std::endl;
        return;
    }
    
    save_macro_to_json();
}

void KeyboardMacroRecorder::save_macro_to_json(){
    JsonValue macro_json = create_macro_json();
    
    std::string filename = OUTPUT_FILENAME;
    if (filename.empty()){
        filename = "macro.json";
    }
    
    try {
        std::ofstream file(filename);
        if (!file.is_open()){
            throw FileException(nullptr, PA_CURRENT_FUNCTION, "Unable to open file: " + filename, filename);
        }
        file << macro_json.dump(4);
        file.close();
        
        std::cout << "Macro saved to: " + filename << std::endl;
    } catch (const std::exception& e){
        throw FileException(nullptr, PA_CURRENT_FUNCTION, "Failed to save macro: " + std::string(e.what()), filename);
    }
}

void KeyboardMacroRecorder::on_controller_command_start(
    Button button,
    DpadPosition position,
    uint8_t left_x, uint8_t left_y,
    uint8_t right_x, uint8_t right_y,
    Milliseconds duration
){
    if (!m_is_recording){
        return;
    }
    
    RecordedEvent event;
    event.timestamp = current_time();
    event.is_start = true;
    event.button = button;
    event.position = position;
    event.left_x = left_x;
    event.left_y = left_y;
    event.right_x = right_x;
    event.right_y = right_y;
    event.duration = duration;
    
    m_recorded_events.push_back(event);
    
    // Log the command start for debugging
    std::cout << "Command start: " << button_to_string(button) 
              << ", dpad(" << dpad_to_string(position) << ")"
              << ", LJ(" << (int)left_x << "," << (int)left_y << ")"
              << ", RJ(" << (int)right_x << "," << (int)right_y << ")"
              << ", duration=" << duration.count() << "ms" << std::endl;
}

void KeyboardMacroRecorder::on_controller_command_end(
    Button button,
    DpadPosition position,
    uint8_t left_x, uint8_t left_y,
    uint8_t right_x, uint8_t right_y
){
    if (!m_is_recording){
        return;
    }
    
    RecordedEvent event;
    event.timestamp = current_time();
    event.is_start = false;
    event.button = button;
    event.position = position;
    event.left_x = left_x;
    event.left_y = left_y;
    event.right_x = right_x;
    event.right_y = right_y;
    event.duration = Milliseconds::zero();
    
    m_recorded_events.push_back(event);
    
    // Log the command end for debugging
    std::cout << "Command end: " << button_to_string(button) 
              << ", dpad(" << dpad_to_string(position) << ")"
              << ", LJ(" << (int)left_x << "," << (int)left_y << ")"
              << ", RJ(" << (int)right_x << "," << (int)right_y << ")" << std::endl;
}

JsonValue KeyboardMacroRecorder::create_macro_json(){
    JsonObject macro;
    
    // Add metadata
    macro["version"] = "1.0";
    macro["description"] = "Recorded controller macro";
    macro["recorded_at"] = std::to_string(m_recording_start_time.time_since_epoch().count());
    macro["duration_ms"] = std::to_string((m_recording_stop_time - m_recording_start_time).count());
    
    // Convert recorded events to TurboMacro format
    JsonArray actions;
    
    for (const auto& event : m_recorded_events){
        if (!event.is_start){
            continue; // Skip end events for now, we'll handle them differently
        }
        
        JsonObject action;
        
        // Convert button to TurboMacro action
        if (event.button != BUTTON_NONE){
            action["action"] = button_to_turbo_macro_action(event.button);
            if (event.duration > Milliseconds::zero()){
                action["duration"] = std::to_string(event.duration.count());
            }
        }
        
        // Convert dpad position
        if (event.position != DpadPosition::DPAD_NONE){
            action["dpad"] = dpad_to_turbo_macro_dpad(event.position);
            if (event.duration > Milliseconds::zero()){
                action["duration"] = std::to_string(event.duration.count());
            }
        }
        
        // Convert joystick positions
        if (event.left_x != 128 || event.left_y != 128){
            JsonObject left_stick;
            left_stick["x"] = std::to_string(event.left_x);
            left_stick["y"] = std::to_string(event.left_y);
            action["left_stick"] = JsonValue(std::move(left_stick));
            if (event.duration > Milliseconds::zero()){
                action["duration"] = std::to_string(event.duration.count());
            }
        }
        
        if (event.right_x != 128 || event.right_y != 128){
            JsonObject right_stick;
            right_stick["x"] = std::to_string(event.right_x);
            right_stick["y"] = std::to_string(event.right_y);
            action["right_stick"] = JsonValue(std::move(right_stick));
            if (event.duration > Milliseconds::zero()){
                action["duration"] = std::to_string(event.duration.count());
            }
        }
        
        if (!action.empty()){
            actions.push_back(JsonValue(std::move(action)));
        }
    }
    
    macro["actions"] = JsonValue(std::move(actions));
    return macro;
}

std::string KeyboardMacroRecorder::button_to_string(Button button){
    switch (button){
    case BUTTON_NONE: return "None";
    case BUTTON_A: return "A";
    case BUTTON_B: return "B";
    case BUTTON_X: return "X";
    case BUTTON_Y: return "Y";
    case BUTTON_L: return "L";
    case BUTTON_R: return "R";
    case BUTTON_ZL: return "ZL";
    case BUTTON_ZR: return "ZR";
    case BUTTON_PLUS: return "PLUS";
    case BUTTON_MINUS: return "MINUS";
    case BUTTON_LCLICK: return "LCLICK";
    case BUTTON_RCLICK: return "RCLICK";
    case BUTTON_HOME: return "HOME";
    case BUTTON_CAPTURE: return "CAPTURE";
    default: return "Unknown";
    }
}

std::string KeyboardMacroRecorder::dpad_to_string(DpadPosition position){
    switch (position){
    case DPAD_NONE: return "None";
    case DPAD_UP: return "Up";
    case DPAD_UP_RIGHT: return "Up+Right";
    case DPAD_RIGHT: return "Right";
    case DPAD_DOWN_RIGHT: return "Down+Right";
    case DPAD_DOWN: return "Down";
    case DPAD_DOWN_LEFT: return "Down+Left";
    case DPAD_LEFT: return "Left";
    case DPAD_UP_LEFT: return "Up+Left";
    default: return "Unknown";
    }
}

std::string KeyboardMacroRecorder::button_to_turbo_macro_action(Button button){
    switch (button){
    case BUTTON_A: return "A";
    case BUTTON_B: return "B";
    case BUTTON_X: return "X";
    case BUTTON_Y: return "Y";
    case BUTTON_L: return "L";
    case BUTTON_R: return "R";
    case BUTTON_ZL: return "ZL";
    case BUTTON_ZR: return "ZR";
    case BUTTON_PLUS: return "PLUS";
    case BUTTON_MINUS: return "MINUS";
    case BUTTON_LCLICK: return "LEFT_JOY_CLICK";
    case BUTTON_RCLICK: return "RIGHT_JOY_CLICK";
    case BUTTON_HOME: return "HOME";
    case BUTTON_CAPTURE: return "CAPTURE";
    default: return "";
    }
}

std::string KeyboardMacroRecorder::dpad_to_turbo_macro_dpad(DpadPosition position){
    switch (position){
    case DPAD_UP: return "UP";
    case DPAD_RIGHT: return "RIGHT";
    case DPAD_DOWN: return "DOWN";
    case DPAD_LEFT: return "LEFT";
    default: return "";
    }
}

}
}
