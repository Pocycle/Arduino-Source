/*  Turbo Macro
 *
 *  From: https://github.com/PokemonAutomation/
 *
 */

 #ifndef PokemonAutomation_NintendoSwitch_KeyboardMacroRecorder_H
 #define PokemonAutomation_NintendoSwitch_KeyboardMacroRecorder_H
 
 #include "NintendoSwitch/NintendoSwitch_SingleSwitchProgram.h"
 #include "Common/Cpp/Options/SimpleIntegerOption.h"
 #include "NintendoSwitch/Options/TurboMacroTable.h"
 
 namespace PokemonAutomation{
 namespace NintendoSwitch{
 
 
 class KeyboardMacroRecorder_Descriptor : public SingleSwitchProgramDescriptor{
 public:
     KeyboardMacroRecorder_Descriptor();
 };
 
 
 class KeyboardMacroRecorder : public SingleSwitchProgramInstance{
 public:
     KeyboardMacroRecorder();
 
     virtual void program(SingleSwitchProgramEnvironment& env, ProControllerContext& context) override;
 
 private:
     void run_macro(SingleSwitchProgramEnvironment& env, ProControllerContext& context);
     void execute_action(
         VideoStream& stream, ProControllerContext& context,
         const TurboMacroRow& row
     );
 
 private:
     SimpleIntegerOption<uint32_t> LOOP;
     TurboMacroTable MACRO;
 };
 
 
 
 }
 }
 #endif // PokemonAutomation_NintendoSwitch_KeyboardMacroRecorder_H
 
