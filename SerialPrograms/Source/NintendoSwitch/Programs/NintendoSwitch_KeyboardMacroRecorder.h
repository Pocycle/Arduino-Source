/*  Keyboard Macro Recorder
 *
 *  From: https://github.com/PokemonAutomation/
 *
 */

#ifndef PokemonAutomation_NintendoSwitch_KeyboardMacroRecorder_H
#define PokemonAutomation_NintendoSwitch_KeyboardMacroRecorder_H

#include "NintendoSwitch/NintendoSwitch_SingleSwitchProgram.h"
#include "NintendoSwitch/Options/TurboMacroTable.h"
#include "Common/Cpp/Options/StringOption.h"
#include "Common/Cpp/Options/BooleanCheckBoxOption.h"
#include "Common/Cpp/Options/TimeDurationOption.h"
#include "Common/Cpp/Json/JsonValue.h"
#include "Common/Cpp/Json/JsonArray.h"
#include "Common/Cpp/Json/JsonObject.h"
#include "Controllers/ControllerEventCallback.h"
#include "NintendoSwitch/Controllers/NintendoSwitch_ControllerState.h"
#include "Common/Cpp/Time.h"
#include <QKeyEvent>
#include <cstdint>
#include <vector>
#include <map>
#include <chrono>

namespace PokemonAutomation{
namespace NintendoSwitch{

using namespace std::chrono_literals;

struct RecordedEvent{
    WallClock timestamp;
    bool is_start;
    Button button;
    DpadPosition position;
    uint8_t left_x;
    uint8_t left_y;
    uint8_t right_x;
    uint8_t right_y;
    Milliseconds duration;
};

class KeyboardMacroRecorder_Descriptor : public SingleSwitchProgramDescriptor{
public:
    KeyboardMacroRecorder_Descriptor();
};

class KeyboardMacroRecorder : public SingleSwitchProgramInstance, public ControllerEventCallback{
public:
    KeyboardMacroRecorder();
    virtual ~KeyboardMacroRecorder();

    virtual void program(SingleSwitchProgramEnvironment& env, ProControllerContext& context) override;
    
    // ControllerEventCallback interface
    virtual void on_controller_command_start(
        Button button,
        DpadPosition position,
        uint8_t left_x, uint8_t left_y,
        uint8_t right_x, uint8_t right_y,
        Milliseconds duration
    ) override;
    virtual void on_controller_command_end(
        Button button,
        DpadPosition position,
        uint8_t left_x, uint8_t left_y,
        uint8_t right_x, uint8_t right_y
    ) override;

private:
    void start_recording();
    void stop_recording();
    void save_recording();
    void save_macro_to_json();
    JsonValue create_macro_json();
    std::string button_to_string(Button button);
    std::string dpad_to_string(DpadPosition position);
    std::string button_to_turbo_macro_action(Button button);
    std::string dpad_to_turbo_macro_dpad(DpadPosition position);

private:
    StringOption OUTPUT_FILENAME;
    
    std::vector<RecordedEvent> m_recorded_events;
    bool m_is_recording;
    WallClock m_recording_start_time;
    WallClock m_recording_stop_time;
    bool m_first_run;
};

}
}
#endif // PokemonAutomation_NintendoSwitch_KeyboardMacroRecorder_H
 
