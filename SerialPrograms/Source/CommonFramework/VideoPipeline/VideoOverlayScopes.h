/*  Video Overlay Scopes
 *
 *  From: https://github.com/PokemonAutomation/Arduino-Source
 *
 */

#ifndef PokemonAutomation_VideoOverlayScopes_H
#define PokemonAutomation_VideoOverlayScopes_H

//#include <vector>
#include <deque>
//#include "VideoOverlayTypes.h"
#include "VideoOverlay.h"

namespace PokemonAutomation{



// A box as part of the video overlay.
// It handles its own life time on video overlay: once it's destroyed, it removes itself from VideoOverlay.
class InferenceBoxScope : public ImageFloatBox{
public:
    ~InferenceBoxScope(){
        m_overlay.remove_box(*this);
    }
    InferenceBoxScope(const InferenceBoxScope&) = delete;
    void operator=(const InferenceBoxScope&) = delete;

public:
    InferenceBoxScope(
        VideoOverlay& overlay,
        const ImageFloatBox& box,
        Color color = COLOR_RED
    )
        : ImageFloatBox(box)
        , m_color(color)
        , m_overlay(overlay)
    {
        overlay.add_box(*this, color);
    }
    InferenceBoxScope(
        VideoOverlay& overlay,
        double p_x, double p_y,
        double p_width, double p_height,
        Color color = COLOR_RED
    )
        : ImageFloatBox(p_x, p_y, p_width, p_height)
        , m_color(color)
        , m_overlay(overlay)
    {
        overlay.add_box(*this, color);
    }

private:
    Color m_color;
    VideoOverlay& m_overlay;
};




// A text as part of the video overlay.
// It handles its own life time on video overlay: once it's destroyed, it removes itself from VideoOverlay.
class OverlayTextScope : public OverlayText{
    OverlayTextScope(const OverlayTextScope&) = delete;
    void operator=(const OverlayTextScope&) = delete;

public:
    ~OverlayTextScope(){
        m_overlay.remove_text(*this);
    }

public:
    OverlayTextScope(
        VideoOverlay& overlay,
        std::string message,
        double x,
        double y,
        double font_size = 1.0,
        Color color = COLOR_BLUE
    )
        : OverlayText(message, x, y, font_size, color)
        , m_overlay(overlay)
    {
        overlay.add_text(*this);
    }

private:
    VideoOverlay& m_overlay;
};



// Used to clear log messages on video overlay automatically.
// Place this at the beginning of a program, so that when the program exits, it will
// clear the log messages from the overlay automatically.
class OverlayLogTextScope{
    OverlayLogTextScope(const OverlayLogTextScope&) = delete;
    void operator=(const OverlayLogTextScope&) = delete;

public:
    OverlayLogTextScope(VideoOverlay& overlay)
        : m_overlay(overlay)
    {}
    ~OverlayLogTextScope(){
        m_overlay.clear_log_texts();
    }

private:
    VideoOverlay& m_overlay;
};










// Used by video inference sessions to manage inference boxes.
// VideoOverlaySet will be passed to the inference callbacks in a session
// to store inference boxes. When the session ends, VideoOverlaySet::clear()
// is called to release those inference boxes. The boxes are also cleared
// automatically when VideoOverlaySet is destroyed.
// In this way, the user will see the inference boxes on the video overlay UI
// and those boxes will leave the UI after the session ends.
//
// TODO: shall we add text management to this class, or rename this class to BoxOverleySet?
class VideoOverlaySet{
public:
    VideoOverlaySet(VideoOverlay& overlay)
        : m_overlay(overlay)
    {}

    void clear(){
        m_boxes.clear();
    }
    void add(Color color, const ImageFloatBox& box){
        m_boxes.emplace_back(m_overlay, box, color);
    }

private:
    VideoOverlay& m_overlay;
    std::deque<InferenceBoxScope> m_boxes;
};




}
#endif
