/*  ShinyHuntUnattended-Regigigas2
 *
 *  From: https://github.com/PokemonAutomation/Arduino-Source
 *
 */

#ifndef PokemonAutomation_PokemonSwSh_ShinyHuntUnattendedRegigigas2_H
#define PokemonAutomation_PokemonSwSh_ShinyHuntUnattendedRegigigas2_H

#include "CommonFramework/Options/SectionDivider.h"
#include "CommonFramework/Options/SimpleInteger.h"
#include "NintendoSwitch/Options/TimeExpression.h"
#include "NintendoSwitch/Options/StartInGripMenu.h"
#include "NintendoSwitch/Framework/SingleSwitchProgram.h"
#include "PokemonSwSh/Options/PokemonSwSh_DateToucher.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonSwSh{


class ShinyHuntUnattendedRegigigas2_Descriptor : public RunnableSwitchProgramDescriptor{
public:
    ShinyHuntUnattendedRegigigas2_Descriptor();
};



class ShinyHuntUnattendedRegigigas2 : public SingleSwitchProgramInstance{
public:
    ShinyHuntUnattendedRegigigas2(const ShinyHuntUnattendedRegigigas2_Descriptor& descriptor);

    virtual void program(SingleSwitchProgramEnvironment& env) override;

private:
    StartInGripOrGame START_IN_GRIP_MENU;
    TouchDateInterval TOUCH_DATE_INTERVAL;

    SimpleInteger<uint8_t> REVERSAL_PP;
    TimeExpression<uint16_t> START_TO_ATTACK_DELAY;
    SectionDivider m_advanced_options;
    TimeExpression<uint16_t> ATTACK_TO_CATCH_DELAY;
    TimeExpression<uint16_t> CATCH_TO_OVERWORLD_DELAY;
};

}
}
}
#endif
