/*  Controller Event Callback
 *
 *  From: https://github.com/PokemonAutomation/
 *
 *  This interface provides callbacks for tracking actual controller state changes
 *  rather than keyboard events. This is the source of truth for when controller
 *  commands are actually executed.
 *
 */

#ifndef PokemonAutomation_Controllers_ControllerEventCallback_H
#define PokemonAutomation_Controllers_ControllerEventCallback_H

#include "Common/Cpp/Time.h"

namespace PokemonAutomation{

// Forward declarations - these are the actual types used in Nintendo Switch controllers
namespace NintendoSwitch{
    enum Button : uint32_t;
    enum DpadPosition : uint8_t;
}

// Controller event callback interface for tracking actual controller state changes
class ControllerEventCallback{
public:
    virtual ~ControllerEventCallback() = default;
    
    // Called when controller actually starts executing a command
    virtual void on_controller_command_start(
        NintendoSwitch::Button button,
        NintendoSwitch::DpadPosition position,
        uint8_t left_x, uint8_t left_y,
        uint8_t right_x, uint8_t right_y,
        Milliseconds duration
    ) = 0;
    
    // Called when controller finishes executing a command
    virtual void on_controller_command_end(
        NintendoSwitch::Button button,
        NintendoSwitch::DpadPosition position,
        uint8_t left_x, uint8_t left_y,
        uint8_t right_x, uint8_t right_y
    ) = 0;
};

}
#endif 