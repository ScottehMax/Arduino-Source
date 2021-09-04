/*  String Select
 *
 *  From: https://github.com/PokemonAutomation/Arduino-Source
 *
 */

#ifndef PokemonAutomation_StringSelect_H
#define PokemonAutomation_StringSelect_H

#include <QComboBox>
#include "ConfigOption.h"

namespace PokemonAutomation{


class StringSelect : public ConfigOption{
public:
    StringSelect(
        QString label,
        const std::vector<QString>& cases,
        const QString& default_case
    );
    StringSelect(
        QString label,
        std::vector<std::pair<QString, QIcon>> cases,
        const QString& default_case
    );

    operator size_t() const{ return m_current; }
    operator const QString&() const{ return m_case_list[m_current].first; }

    virtual void load_json(const QJsonValue& json) override;
    virtual QJsonValue to_json() const override;

    virtual void restore_defaults() override;

    virtual ConfigOptionUI* make_ui(QWidget& parent) override;

private:
    friend class StringSelectUI;
    QString m_label;
    std::vector<std::pair<QString, QIcon>> m_case_list;
    std::map<QString, size_t> m_case_map;
    size_t m_default;
    size_t m_current;
};



class StringSelectUI : public ConfigOptionUI, public QWidget{
public:
    StringSelectUI(QWidget& parent, StringSelect& value);
    virtual QWidget* widget() override{ return this; }
    virtual void restore_defaults() override;

private:
    StringSelect& m_value;
    QComboBox* m_box;
    bool m_updating = false;
};



}
#endif
