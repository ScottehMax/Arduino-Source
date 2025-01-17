/*  Max Lair Detect PP
 *
 *  From: https://github.com/PokemonAutomation/Arduino-Source
 *
 */

#include "Common/Qt/StringToolsQt.h"
#include "CommonFramework/ImageTypes/ImageRGB32.h"
#include "CommonFramework/ImageTools/ImageStats.h"
#include "CommonFramework/ImageTools/SolidColorTest.h"
#include "CommonFramework/ImageTools/ImageFilter.h"
#include "CommonFramework/OCR/OCR_StringNormalization.h"
#include "CommonFramework/OCR/OCR_RawOCR.h"
#include "PokemonSwSh_MaxLair_Detect_HPPP.h"

#include <iostream>
using std::cout;
using std::endl;

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonSwSh{
namespace MaxLairInternal{



double read_hp_bar_internal(const ImageViewRGB32& image){
    size_t width = image.width();
    size_t height = image.height();
    size_t area = width * height;

    ImageStats stats;
    double bar = 0.5;
    for (size_t c = 0;; c++){
        stats = image_stats(extract_box_reference(image, ImageFloatBox(0.0, 0.0, bar, 1.0)));
        double max_color = 0;
        max_color = std::max(max_color, stats.average.r);
        max_color = std::max(max_color, stats.average.g);
        max_color = std::max(max_color, stats.average.b);
        if (max_color > 128 && stats.stddev.sum() < 50){
            break;
        }
        bar *= 0.5;
        if (c > 12){
            stats = image_stats(extract_box_reference(image, ImageFloatBox(0.0, 0.0, 1.0, 1.0)));
//            cout << stats.average << stats.stddev << endl;
//            image.save("test.png");
            return stats.average.sum() < 384 && stats.stddev.sum() < 80
                ? 0.0
                : -1;
        }
    }

    Color color = stats.average.round();
    int bar_R = color.red();
    int bar_G = color.green();
    int bar_B = color.blue();

    int bar_area = 0;
    for (size_t r = 0; r < height; r++){
        for (size_t c = 0; c < width; c++){
            Color pixel(image.pixel(c, r));
            int R = pixel.red() - bar_R;
            int G = pixel.green() - bar_G;
            int B = pixel.blue() - bar_B;
            if (R*R + G*G + B*B < 100*100){
                bar_area++;
            }
        }
    }

    return std::min((double)bar_area / area, bar * 4);
}
double read_hp_bar(const ImageViewRGB32& image){
    //  Try reading just the upper half first.
    double hp = read_hp_bar_internal(extract_box_reference(image, ImageFloatBox(0.5, 0.0, 0.5, 1.0)));
    if (hp > 0){
        return (1.0 + hp) * 0.5;
    }

    //  Now try the bottom half.
    return read_hp_bar_internal(image);
}

double read_hp_bar(Logger& logger, const ImageViewRGB32& image){
    double hp = read_hp_bar(image);
//    static int c = 0;
//    image.save("test-" + std::to_string(c++) + ".png");
    if (hp < 0){
        logger.log("HP Read: ?", COLOR_RED);
    }else{
        logger.log("HP Read: " + std::to_string(100 * hp) + "%", COLOR_BLUE);
    }
    return hp;
}
Health read_in_battle_hp_box(Logger& logger, const ImageViewRGB32& sprite, const ImageViewRGB32& hp_bar){
    ImageStats stats = image_stats(sprite);
//    cout << stats.average << stats.stddev << endl;
    if (is_solid(stats, {0., 0.389943, 0.610057})){
        logger.log("HP Read: Dead", COLOR_BLUE);
        return {0, 1};
    }
    double hp = read_hp_bar(logger, hp_bar);
    if (hp == 0){
        return {0, -1};
    }
    return {hp, 0};
}






int8_t parse_pp(const std::string& current, const std::string& total){
//    cout << current << " " << total << endl;
    if (total[0] == '0'){
        return -1;
    }
    uint32_t num_current = atoi(current.c_str());
    uint32_t num_total = atoi(total.c_str());
    if (num_current > num_total){
        return -1;
    }
    if (num_current > 64 || num_total > 64){
        return -1;
    }
    if (num_total == 1){
        return (int8_t)num_current;
    }
    if (num_total % 5 != 0){
        return -1;
    }
    return (int8_t)num_current;
}
int8_t parse_pp(const std::string& str){
    //  Clean up and split into tokens deliminated by '/'.
    std::vector<std::string> tokens;
    tokens.emplace_back();
    for (char ch : str){
        if (ch <= 32){
            continue;
        }
        if ('0' <= ch && ch <= '9'){
            tokens.back() += ch;
            continue;
        }
        if (ch == 'o' || ch == 'O'){
            tokens.back() += '0';
            continue;
        }
        if (ch == '/'){
            tokens.emplace_back();
            continue;
        }
    }

    if (tokens.size() > 2){
        return -1;
    }
    if (tokens.size() == 2){
        return parse_pp(tokens[0], tokens[1]);
    }

    const std::string& value = tokens[0];
    if (value.empty()){
        return 0;
    }

    for (size_t c = value.size(); c > 1;){
        char ch = value[--c];
        if (ch != '1' && ch != '7'){
            continue;
        }
        int8_t val = parse_pp(
            value.substr(0, c),
            value.substr(c + 1)
        );
        if (val >= 0){
            return val;
        }
        val = parse_pp(
            value.substr(0, c),
            value.substr(c)
        );
        if (val >= 0){
            return val;
        }
    }

    return -1;
}

int8_t read_pp_text(Logger& logger, const ImageViewRGB32& image){
    if (image.width() == 0 || image.height() == 0){
        return -1;
    }

    std::vector<std::pair<uint32_t, uint32_t>> filters{
        {0xff000000, 0xff404040},
        {0xff808080, 0xffffffff},
    };
    bool ok = false;
    ImageRGB32 processed;
//    cout << "============" << endl;
    for (const auto& item : filters){
        size_t text_pixels;
        processed = to_blackwhite_rgb32_range(text_pixels, image, item.first, item.second, false);
        double text_ratio = 1.0 - (double)text_pixels / (image.width() * image.height());
//        cout << "text_ratio = " << text_ratio << endl;
        if (0.02 <= text_ratio && text_ratio <= 0.50){
            ok = true;
            break;
        }
    }
    if (!ok){
        logger.log("OCR Result: Invalid text ratio.", COLOR_RED);
        return -1;
    }


    std::string ocr_text = OCR::ocr_read(Language::English, processed);

    ocr_text = to_utf8(OCR::run_character_reductions(to_utf32(ocr_text)));
    int8_t pp = parse_pp(ocr_text);

    std::string str;
    str += "OCR Result: \"";
    for (char ch : ocr_text){
        if (ch != '\r' && ch != '\n'){
            str += ch;
        }
    }
    str += "\" -> (";
    str += pp < 0 ? "? PP)" : std::to_string((int)pp) + " PP)";
    logger.log(str, pp < 0 ? COLOR_RED : COLOR_BLUE);

    return pp;
}






























}
}
}
}
