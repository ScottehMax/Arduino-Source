/*  Video Overlay Widget
 *
 *  From: https://github.com/PokemonAutomation/Arduino-Source
 *
 */

#ifndef PokemonAutomation_VideoPipeline_VideoOverlayWidget_H
#define PokemonAutomation_VideoPipeline_VideoOverlayWidget_H

#include <map>
#include <QWidget>
#include "Common/Cpp/Concurrency/SpinLock.h"
#include "CommonFramework/VideoPipeline/VideoOverlaySession.h"

namespace PokemonAutomation{

struct OverlayText;

class VideoOverlayWidget : public QWidget, private VideoOverlaySession::Listener{
public:
    ~VideoOverlayWidget();
    VideoOverlayWidget(QWidget& parent, VideoOverlaySession& session);

    void set_enabled_boxes(bool visible) { m_enabled_boxes = visible;}
    void set_enabled_text(bool visible) { m_enabled_text = visible;}
    void set_enabled_log(bool visible) {m_enabled_log = visible;}
    void set_enabled_stats(bool visible) {m_enabled_stats = visible;}

private:
    //  Asynchronous changes to the overlays.

    virtual void update_boxes(const std::shared_ptr<const std::vector<VideoOverlaySession::Box>>& boxes) override;
    virtual void update_text(const std::shared_ptr<const std::vector<OverlayText>>& texts) override;
    virtual void update_log_text(const std::shared_ptr<const std::vector<OverlayText>>& texts) override;
    virtual void update_log_background(const std::shared_ptr<const std::vector<VideoOverlaySession::Box>>& bg_boxes) override;
    virtual void update_stats(const std::shared_ptr<const std::vector<OverlayStat>>& stats) override;

    virtual void resizeEvent(QResizeEvent* event) override;
    virtual void paintEvent(QPaintEvent*) override;

private:
    VideoOverlaySession& m_session;

    SpinLock m_lock;
    std::shared_ptr<const std::vector<VideoOverlaySession::Box>> m_boxes;
    std::shared_ptr<const std::vector<OverlayText>> m_texts;
    std::shared_ptr<const std::vector<OverlayText>> m_log_texts;
    std::shared_ptr<const std::vector<VideoOverlaySession::Box>> m_log_text_bg_boxes;
    std::shared_ptr<const std::vector<OverlayStat>> m_stats;

    bool m_enabled_boxes;
    bool m_enabled_text;
    bool m_enabled_log;
    bool m_enabled_stats;
};


}
#endif

