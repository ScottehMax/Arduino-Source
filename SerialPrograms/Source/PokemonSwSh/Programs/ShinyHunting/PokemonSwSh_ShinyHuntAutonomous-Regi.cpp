/*  ShinyHuntAutonomous-Regi
 *
 *  From: https://github.com/PokemonAutomation/Arduino-Source
 *
 */

#include "Common/Clientside/PrettyPrint.h"
#include "Common/SwitchFramework/FrameworkSettings.h"
#include "Common/SwitchFramework/Switch_PushButtons.h"
#include "Common/PokemonSwSh/PokemonSettings.h"
#include "Common/PokemonSwSh/PokemonSwShGameEntry.h"
#include "Common/PokemonSwSh/PokemonSwShDateSpam.h"
#include "PokemonSwSh/Inference/ShinyDetection/PokemonSwSh_ShinyEncounterDetector.h"
#include "PokemonSwSh_EncounterTracker.h"
#include "PokemonSwSh_ShinyHunt-Regi.h"
#include "PokemonSwSh_ShinyHuntAutonomous-Regi.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonSwSh{

ShinyHuntAutonomousRegi::ShinyHuntAutonomousRegi()
    : SingleSwitchProgram(
        FeedbackType::REQUIRED, PABotBaseLevel::PABOTBASE_12KB,
        "Shiny Hunt Autonomous - Regi",
        "SerialPrograms/ShinyHuntAutonomous-Regi.md",
        "Automatically hunt for shiny Regi using video feedback."
    )
    , REQUIRE_SQUARE(
        "<b>Require Square:</b><br>Stop only for a square shiny. Run from star shinies.",
        false
    )
    , m_advanced_options(
        "<font size=4><b>Advanced Options:</b> You should not need to touch anything below here.</font>"
    )
    , EXIT_BATTLE_MASH_TIME(
        "<b>Exit Battle Time:</b><br>After running, wait this long to return to overworld.",
        "6 * TICKS_PER_SECOND"
    )
    , TRANSITION_DELAY(
        "<b>Transition Delay:</b><br>Time to enter/exit the building.",
        "5 * TICKS_PER_SECOND"
    )
    , TOUCH_DATE_INTERVAL(
        "<b>Rollover Prevention:</b><br>Prevent a den from rolling over by periodically touching the date. If set to zero, this feature is disabled.",
        "4 * 3600 * TICKS_PER_SECOND"
    )
{
    m_options.emplace_back(&REGI_NAME, "REGI_NAME");
    m_options.emplace_back(&REQUIRE_SQUARE, "REQUIRE_SQUARE");
    m_options.emplace_back(&m_advanced_options, "");
    m_options.emplace_back(&EXIT_BATTLE_MASH_TIME, "EXIT_BATTLE_MASH_TIME");
    m_options.emplace_back(&TRANSITION_DELAY, "TRANSITION_DELAY");
    m_options.emplace_back(&TOUCH_DATE_INTERVAL, "TOUCH_DATE_INTERVAL");
}




std::string ShinyHuntAutonomousRegi::Stats::stats() const{
    std::string str;
    str += str_encounters();
    str += " - Light Resets: " + tostr_u_commas(m_light_resets);
    str += str_shinies();
    return str;
}


void ShinyHuntAutonomousRegi::program(SingleSwitchProgramEnvironment& env) const{
    grip_menu_connect_go_home();
    resume_game_back_out(TOLERATE_SYSTEM_UPDATE_MENU_FAST, 200);

    Stats stats;
    StandardEncounterTracker tracker(stats, env.console, REQUIRE_SQUARE, EXIT_BATTLE_MASH_TIME);

    uint32_t last_touch = system_clock() - TOUCH_DATE_INTERVAL;
    bool error = false;
    while (true){
        stats.log_stats(env, env.logger);

        move_to_corner(env, error, TRANSITION_DELAY);
        if (error){
            stats.m_light_resets++;
            stats.log_stats(env, env.logger);
            error = false;
        }

        //  Touch the date.
        if (TOUCH_DATE_INTERVAL > 0 && system_clock() - last_touch >= TOUCH_DATE_INTERVAL){
            env.logger.log("Touching date to prevent rollover.");
            pbf_press_button(BUTTON_HOME, 10, GAME_TO_HOME_DELAY_SAFE);
            touch_date_from_home(SETTINGS_TO_HOME_DELAY);
            resume_game_no_interact(TOLERATE_SYSTEM_UPDATE_MENU_FAST);
            last_touch += TOUCH_DATE_INTERVAL;
        }

        //  Do the light puzzle.
        run_regi_light_puzzle(env, REGI_NAME, stats.encounters());

        //  Start the encounter.
        pbf_mash_button(BUTTON_A, 5 * TICKS_PER_SECOND);
        env.console.botbase().wait_for_all_requests();

        //  Detect shiny.
        ShinyEncounterDetector::Detection detection;
        {
            ShinyEncounterDetector detector(
                env.console, env.logger,
                ShinyEncounterDetector::REGULAR_BATTLE,
                std::chrono::seconds(30)
            );
            detection = detector.detect(env);
        }

        if (tracker.process_result(detection)){
            break;
        }
        if (detection == ShinyEncounterDetector::NO_BATTLE_MENU){
            pbf_mash_button(BUTTON_B, TICKS_PER_SECOND);
            tracker.run_away();
            error = true;
        }
    }

    stats.log_stats(env, env.logger);

    pbf_press_button(BUTTON_HOME, 10, GAME_TO_HOME_DELAY_SAFE);

    end_program_callback();
    end_program_loop();
}



}
}
}

