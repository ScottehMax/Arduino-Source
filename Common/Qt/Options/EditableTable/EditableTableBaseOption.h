/*  Editable Table Option
 *
 *  From: https://github.com/PokemonAutomation/Arduino-Source
 *
 */

#ifndef PokemonAutomation_EditableTableBaseOption_H
#define PokemonAutomation_EditableTableBaseOption_H

#include <memory>
#include <vector>
#include <QString>

class QJsonValue;
class QWidget;

namespace PokemonAutomation{

class JsonValue2;


class EditableTableRow{
public:
    virtual ~EditableTableRow() = default;

    virtual void load_json(const JsonValue2& json) = 0;
    virtual JsonValue2 to_json() const = 0;

    virtual std::unique_ptr<EditableTableRow> clone() const = 0;

    virtual QString check_validity() const{ return QString(); }
    virtual std::vector<QWidget*> make_widgets(QWidget& parent) = 0;

private:
    friend class EditableTableBaseWidget;
    size_t m_index;
};

class EditableTableFactory{
public:
    virtual QStringList make_header() const = 0;
    virtual std::unique_ptr<EditableTableRow> make_row() const = 0;
};



class EditableTableBaseOption{
public:
    EditableTableBaseOption(
        QString label, const EditableTableFactory& factory,
        std::vector<std::unique_ptr<EditableTableRow>> default_value = {}
    );

    void load_default(const JsonValue2& json);
    void load_current(const JsonValue2& json);
    JsonValue2 write_default() const;
    JsonValue2 write_current() const;

    size_t size() const{ return m_current.size(); }
    const EditableTableRow& operator[](size_t index) const;

    QString check_validity() const;
    void restore_defaults();

public:
    void add_row(size_t index, std::unique_ptr<EditableTableRow> row);
    void remove_row(size_t index);

private:
    std::vector<std::unique_ptr<EditableTableRow>> load_json(const JsonValue2& json);
    JsonValue2 to_json(const std::vector<std::unique_ptr<EditableTableRow>>& table) const;

private:
    friend class EditableTableBaseWidget;
    QString m_label;
    const EditableTableFactory& m_factory;
    std::vector<std::unique_ptr<EditableTableRow>> m_default;
    std::vector<std::unique_ptr<EditableTableRow>> m_current;
};




}
#endif
