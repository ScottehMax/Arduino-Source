/*  Encounter Tracker
 *
 *  From: https://github.com/PokemonAutomation/Arduino-Source
 *
 */

#ifndef PokemonAutomation_PokemonSwSh_EncounterTracker_H
#define PokemonAutomation_PokemonSwSh_EncounterTracker_H

#include "CommonFramework/Tools/ConsoleHandle.h"
#include "PokemonSwSh/Inference/ShinyDetection/PokemonSwSh_ShinyEncounterDetector.h"
#include "PokemonSwSh_EncounterStats.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonSwSh{


class StandardEncounterTracker{
public:
    StandardEncounterTracker(
        EncounterStats& stats,
        ConsoleHandle& console,
        bool require_square,
        uint16_t exit_battle_time
    );

    virtual bool run_away();

    bool process_result(ShinyEncounterDetector::Detection detection);

protected:
    EncounterStats& m_stats;
    ConsoleHandle& m_console;
    bool m_require_square;
    uint16_t m_exit_battle_time;
};



}
}
}
#endif