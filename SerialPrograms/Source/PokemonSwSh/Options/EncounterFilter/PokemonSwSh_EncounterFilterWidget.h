/*  Encounter Filter
 *
 *  From: https://github.com/PokemonAutomation/Arduino-Source
 *
 */

#ifndef PokemonAutomation_PokemonSwSh_EncounterFilterWidget_H
#define PokemonAutomation_PokemonSwSh_EncounterFilterWidget_H

#include <QComboBox>
#include "PokemonSwSh_EncounterFilterOption.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonSwSh{




class EncounterFilterWidget : public QWidget, public ConfigWidget{
public:
    EncounterFilterWidget(QWidget& parent, EncounterFilterOption& value);

    virtual void restore_defaults() override;
    virtual void update_ui() override;

private:
    EncounterFilterOption& m_value;

    QComboBox* m_shininess = nullptr;
    ConfigWidget* m_table = nullptr;
};



}
}
}
#endif
