/*  Video Display
 *
 *  From: https://github.com/PokemonAutomation/Arduino-Source
 *
 */

#include <QResizeEvent>
#include <QVBoxLayout>
#include "VideoDisplayWidget.h"
#include "VideoDisplayWindow.h"

//#include <iostream>
//using std::cout;
//using std::endl;

namespace PokemonAutomation{



WidgetStackFixedAspectRatio::WidgetStackFixedAspectRatio(QWidget& parent, SizePolicy size_policy, double aspect_ratio)
    : QWidget(&parent)
    , m_size_policy(size_policy)
    , m_aspect_ratio(aspect_ratio)
{
    m_detached_internal = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(m_detached_internal);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setAlignment(Qt::AlignCenter);
    m_stack_holder = new QWidget(m_detached_internal);
    layout->addWidget(m_stack_holder);
}

void WidgetStackFixedAspectRatio::add_widget(QWidget& widget){
    widget.setParent(m_stack_holder);
    m_widgets.insert(&widget);
}
void WidgetStackFixedAspectRatio::set_size_policy(SizePolicy size_policy){
    m_size_policy = size_policy;
    clear_fixed_dimensions();
    m_recent_widths.clear();
    update_size(this->size());
}
void WidgetStackFixedAspectRatio::set_aspect_ratio(double aspect_ratio){
//    cout << "WidgetStackFixedAspectRatio::set_aspect_ratio(): " << aspect_ratio << endl;
    m_aspect_ratio = aspect_ratio;
    m_recent_widths.clear();
    update_size(this->size());
}

void WidgetStackFixedAspectRatio::clear_fixed_dimensions(){
    this->setMinimumSize(QSize(0, 0));
    this->setMaximumSize(QSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX));
}
void WidgetStackFixedAspectRatio::resize_to_box(QSize enclosing_box){
    int width, height;

    double box_ratio = (double)enclosing_box.width() / enclosing_box.height();
    if (m_aspect_ratio < box_ratio){
        height = enclosing_box.height();
        width = (int)(height * m_aspect_ratio);
    }else{
        width = enclosing_box.width();
        height = (int)(width / m_aspect_ratio);
    }

    QSize size(width, height);

    m_detached_internal->setFixedSize(enclosing_box);
    m_stack_holder->setFixedSize(size);
    for (QWidget* widget : m_widgets){
        widget->setFixedSize(size);
    }
}
void WidgetStackFixedAspectRatio::resize_to_width(int width){
//    cout << "WidgetStackFixedAspectRatio::resize_to_width(): " << width << endl;

    int height = (int)(width / m_aspect_ratio);

    QSize size(width, height);

    //  Safeguard against a resizing loop where the UI bounces between larger
    //  height with scroll bar and lower height with no scroll bar.
    auto iter = m_recent_widths.find(width);
    if (iter != m_recent_widths.end() && std::abs(width - m_stack_holder->width()) < 50){
//        cout << "Supressing potential infinite resizing loop." << endl;
        return;
    }

    m_width_history.push_back(width);
    m_recent_widths.insert(width);
    if (m_width_history.size() > 10){
        m_recent_widths.erase(m_width_history[0]);
        m_width_history.pop_front();
    }

    this->setFixedHeight(height);
    m_detached_internal->setFixedSize(size);
    m_stack_holder->setFixedSize(size);
    for (QWidget* widget : m_widgets){
        widget->setFixedSize(size);
    }
}
void WidgetStackFixedAspectRatio::update_size(QSize size){
    switch (m_size_policy){
    case EXPAND_TO_BOX:
        resize_to_box(size);
        return;
    case ADJUST_HEIGHT_TO_WIDTH:
        resize_to_width(size.width());
        return;
    }
}
void WidgetStackFixedAspectRatio::resizeEvent(QResizeEvent* event){
//    cout << "WidgetStackFixedAspectRatio::resizeEvent(): " << event->size().width() << " x " << event->size().height() << endl;
    update_size(event->size());
}




VideoDisplayWidget::VideoDisplayWidget(
    QWidget& parent, size_t id,
    CommandReceiver& command_receiver,
    CameraSession& camera,
    VideoOverlaySession& overlay
)
    : WidgetStackFixedAspectRatio(parent, WidgetStackFixedAspectRatio::ADJUST_HEIGHT_TO_WIDTH)
    , m_id(id)
    , m_command_receiver(command_receiver)
    , m_video(camera.make_QtWidget(this))
    , m_overlay(new VideoOverlayWidget(*this, overlay))
    , m_popped_out(false)
{
    this->add_widget(*m_video);
    this->add_widget(*m_overlay);

    Resolution resolution = m_video->camera().current_resolution();
    if (resolution){
        set_aspect_ratio(resolution.aspect_ratio());
    }

    m_overlay->setVisible(true);
    m_overlay->setHidden(false);
    m_overlay->raise();
}



void VideoDisplayWidget::mouseDoubleClickEvent(QMouseEvent* event){
//    if (!PreloadSettings::instance().DEVELOPER_MODE){
//        return;
//    }
    // If this widget is not already inside a VideoDisplayWindow, move it
    // into a VideoDisplayWindow
    if (!m_popped_out){
        move_to_new_window();
    }else{
        QWidget::mouseDoubleClickEvent(event);
    }
}

void VideoDisplayWidget::move_to_new_window(){
    // The constructor of VideoDisplayWindow handles the transfer of this VideoDisplayWidget to the new window.
    // The constructor also displays the window.
    // So there is nothing else to do in VideoDisplayWidget::move_to_new_window() besides building VideoDisplayWindow.
    m_popped_out = true;
//    this->setMinimumHeight(0);
//    this->setMaximumHeight(QWIDGETSIZE_MAX);
    this->set_size_policy(EXPAND_TO_BOX);
    new VideoDisplayWindow(this);
}
void VideoDisplayWidget::move_back_from_window(){
    m_popped_out = false;
    this->set_size_policy(ADJUST_HEIGHT_TO_WIDTH);
}



}
