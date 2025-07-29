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
#include <QKeyEvent>
#include <vector>
#include <map>
#include <chrono>

namespace PokemonAutomation{
namespace NintendoSwitch{

using namespace std::chrono_literals;

struct RecordedEvent{
    WallClock timestamp;
    Qt::Key key;
    bool is_press;
    TurboMacroAction action;
    uint8_t x_axis;
    uint8_t y_axis;
    Milliseconds hold_time;
    Milliseconds release_time;
};

class KeyboardMacroRecorder_Descriptor : public SingleSwitchProgramDescriptor{
public:
    KeyboardMacroRecorder_Descriptor();
};

class KeyboardMacroRecorder : public SingleSwitchProgramInstance{
public:
    KeyboardMacroRecorder();

    virtual void program(SingleSwitchProgramEnvironment& env, ProControllerContext& context) override;

private:
    void initialize_keyboard_mapping();
    void start_recording();
    void stop_recording();
    void save_macro_to_json();
    void on_key_press(const QKeyEvent& event);
    void on_key_release(const QKeyEvent& event);
    TurboMacroAction key_to_action(Qt::Key key);
    void get_joystick_values(Qt::Key key, uint8_t& x, uint8_t& y);
    JsonValue create_macro_json();
    std::string get_key_name(Qt::Key key);

private:
    BooleanCheckBoxOption RECORDING_ENABLED;
    StringOption OUTPUT_FILENAME;
    MillisecondsOption DEFAULT_HOLD_TIME;
    MillisecondsOption DEFAULT_RELEASE_TIME;
    
    std::vector<RecordedEvent> m_recorded_events;
    std::map<Qt::Key, WallClock> m_pressed_keys;
    bool m_is_recording;
    WallClock m_recording_start_time;
    
    // Keyboard mapping for conversion
    std::map<Qt::Key, TurboMacroAction> m_key_to_action_map;
    std::map<Qt::Key, std::pair<uint8_t, uint8_t>> m_key_to_joystick_map;
};

}
}
#endif // PokemonAutomation_NintendoSwitch_KeyboardMacroRecorder_H
 
