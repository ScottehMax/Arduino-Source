/*  Selected Region Detector
 *
 *  From: https://github.com/PokemonAutomation/Arduino-Source
 *
 */

#include <QImage>

#include "CommonFramework/ImageTools/ImageStats.h"
#include "CommonFramework/ImageTools/ImageBoxes.h"
#include "CommonFramework/ImageTools/ImageFilter.h"
#include "PokemonLA_PokemonMapSpriteReader.h"
#include "PokemonLA/Resources/PokemonLA_AvailablePokemon.h"
#include "PokemonLA/Resources/PokemonLA_PokemonIcons.h"

#include "Common/Cpp/Exceptions.h"
#include "Common/Qt/ImageOpener.h"
#include "Common/Qt/QtJsonTools.h"
#include "CommonFramework/ImageMatch/ImageCropper.h"
#include "CommonFramework/ImageMatch/ImageDiff.h"
#include "CommonFramework/Globals.h"

#include <map>
#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include <cfloat>
#include <cmath>
#include <array>
using std::cout;
using std::endl;

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonLA{

namespace {

using FeatureType = double;
using FeatureVector = std::vector<FeatureType>;

using ImageMatch::ExactImageDictionaryMatcher;

// defined locally stored data for matching MMO sprites
struct MMOSpriteMatchingData {
    ExactImageDictionaryMatcher color_matcher;
    ExactImageDictionaryMatcher gradient_matcher;
    // sprite slug -> features
    std::map<std::string, FeatureVector> features;

    MMOSpriteMatchingData(
        ExactImageDictionaryMatcher c_matcher,
        ExactImageDictionaryMatcher g_matcher,
        std::map<std::string, FeatureVector> f
    )
        : color_matcher(std::move(c_matcher))
        , gradient_matcher(std::move(g_matcher))
        , features(f) {}
};



FeatureType feature_distance(const FeatureVector& a, const FeatureVector& b){
    if (a.size() != b.size()){
        cout << "Error, feature size mismatch " << a.size() << " " << b.size() << endl;
        throw std::runtime_error("feature size mismatch");
    }
    FeatureType sum = 0.0f;
    for(size_t i = 0; i < a.size(); i++){
        FeatureType d = a[i] - b[i];
        sum += d*d;
    }

    return sum;
}

std::string feature_to_str(const FeatureVector& a){
    std::ostringstream os;
    os << "[";
    for(size_t i = 0; i < a.size(); i++){
        if (i != 0){
            os << ", ";
        }
        os << a[i];
    }
    os << "]";
    return os.str();
}

void run_Sobel_gradient_filter(const ConstImageRef& image, std::function<void(int x, int y, int sum_x[3], int sum_y[3])> proces_gradient){
    const int width = image.width();
    const int height = image.height();
    // Kernel for computing gradient along x axis
    const int kx[3][3] = {
        {-1, 0, 1},
        {-2, 0, 2},
        {-1, 0, 1},
    };
    // kernel for gradient along y axis
    const int ky[3][3] = {
        { 1,  2,  1},
        { 0,  0,  0},
        {-1, -2, -1},
    };
    const int ksz = 3; // kernel size
    const int x_end = width - ksz + 1;
    const int y_end = height - ksz + 1;

    for(int y = 0; y < y_end; y++){
        for(int x = 0; x < x_end; x++){
            int sum_x[3] = {0, 0, 0};
            int sum_y[3] = {0, 0, 0};
            bool has_alpha_pixel = false;
            for(int sy = 0; sy < 3; sy++){
                for(int sx = 0; sx < 3; sx++){
                    uint32_t p = image.pixel(x + sx, y + sy);
                    int alpha = p >> 24;
                    if (alpha < 128){
                        // We don't compute gradient when there is a pixel in the kernel
                        // scope that is transparent
                        has_alpha_pixel = true;
                        break;
                    }
                    for(int ch = 0; ch < 3; ch++){ // rgb channel
                        int shift = ch * 8;
                        int c = (uint32_t(0xff) & p >> shift);

                        sum_x[ch] += c * kx[sy][sx];
                        sum_y[ch] += c * ky[sy][sx];
                    }
                }
                if (has_alpha_pixel){
                    break;
                }
            } // end of kernel operation
            if (has_alpha_pixel){
                continue;
            }

            proces_gradient(x+1, y+1, sum_x, sum_y);
        }
    }
}

QImage compute_image_gradient(const ConstImageRef& image){
    QImage result(image.width(), image.height(), QImage::Format::Format_ARGB32);
    result.fill(QColor(0,0,0,0));
    ImageRef result_ref(result);

    run_Sobel_gradient_filter(image, [&](int x, int y, int sum_x[3], int sum_y[3]){
        int gx = (sum_x[0] + sum_x[1] + sum_x[2] + 1) / 3;
        int gy = (sum_y[0] + sum_y[1] + sum_y[2] + 1) / 3;

        // if (gx*gx + gy*gy <= 2000){
        //     return;
        // }

        int gxc = std::min(std::abs(gx), 255);
        int gyc = std::min(std::abs(gy), 255);

        result_ref.pixel(x, y) = combine_rgb(gxc, gyc, 0);
    });

    return result;
}

FeatureVector compute_gradient_histogram(const ConstImageRef& image){
    const int num_angle_divisions = 8;
    double division_angle = 2. * M_PI / num_angle_divisions;
    double inverse_division_angle = 1.0 / division_angle;

    std::array<int, num_angle_divisions> bin = {0};

    int num_grad = 0;

    run_Sobel_gradient_filter(image, [&](int x, int y, int sum_x[3], int sum_y[3]){
        int gx = sum_x[0] + sum_x[1] + sum_x[2];
        int gy = sum_y[0] + sum_y[1] + sum_y[2];
        if (gx*gx + gy*gy <= 2000){
            return;
        }
        num_grad++;

        // if (count == 0){
        //     // cout << "gxy " << sum_x[0] << " " << sum_x[1] << " " << sum_x[2] << ", " <<
        //     //  sum_y[0] << " " << sum_y[1] << " " << sum_y[2] << endl;
        //     int gxc = std::min(std::abs(gx), 255);
        //     int gyc = std::min(std::abs(gy), 255);

        //     output_image->setPixelColor(x, y, QColor(gxc, gyc, 0));
        // }

        double angle = std::atan2(gy, gx); // range in -pi, pi
        int bin_idx = int((angle + M_PI) * inverse_division_angle);
        // clamp bin to [0, 11]
        bin_idx = std::min(std::max(bin_idx, 0), num_angle_divisions-1);
        bin[bin_idx]++;
    });

    FeatureVector result(num_angle_divisions);
    for(size_t i = 0; i < num_angle_divisions; i++){
        result[i] = bin[i] / (FeatureType)num_grad;
    }

    return result;
}

} // end anonymous namespace

FeatureVector compute_feature(const ConstImageRef& input_image){
    QImage image = input_image.to_qimage().convertedTo(QImage::Format::Format_ARGB32);
    ImageRef image_ref(image);
    int width = image.width();
    int height = image.height();

    // Set pixel outside the sprite circle to transparent:
    float r = (width + height) / 4.0;
    float center_x  = (width-1) / 2.0f;
    float center_y = (height-1) / 2.0f;
    float r2 = r * r;
    for(int y = 0; y < height; y++){
        for(int x = 0; x < width; x++){
            if ((x-center_x)*(x-center_x) + (y-center_y)*(y-center_y) >= r2){
                image_ref.pixel(x, y) = 0;
            }
        }
    }    

    // Divide the image into 4 areas, compute average color on each.
    // Note: we skip the upper right area because that area may overlap with
    // the berry or bonus wave symbol.
    const int num_divisions = 2;
    const float portion = 1.0 / (float)num_divisions;
    FeatureVector result;
    for(int i = 0; i < num_divisions; i++){
        for(int j = 0; j < num_divisions; j++){
            if (i == 1 && j == 0){
                continue; // skip the berry / bonus wave overlapping area
            }
            ImageFloatBox box{i*portion, j*portion, portion, portion};
            auto sub_image = extract_box_reference(image, box);

            ImageStats stats = image_stats(sub_image);

            result.push_back(stats.average.r);
            result.push_back(stats.average.g);
            result.push_back(stats.average.b);
        }
    }

    return result;
}

void load_and_visit_MMO_sprite(std::function<void(const std::string& slug, QImage& sprite)> visit_sprit){
    const char* sprite_path = "PokemonLA/MMOSprites.png";
    const char* json_path = "PokemonLA/MMOSprites.json";
    QImage sprites = open_image(RESOURCE_PATH() + sprite_path);
    QJsonObject json = read_json_file(
        RESOURCE_PATH() + json_path
    ).object();

    int width = json.find("spriteWidth")->toInt();
    int height = json.find("spriteHeight")->toInt();
    if (width <= 0){
        throw FileException(nullptr, PA_CURRENT_FUNCTION, "Invalid width.", (RESOURCE_PATH() + json_path).toStdString());
    }
    if (height <= 0){
        throw FileException(nullptr, PA_CURRENT_FUNCTION, "Invalid height.", (RESOURCE_PATH() + json_path).toStdString());
    }

    QJsonObject locations = json.find("spriteLocations")->toObject();
    for (auto iter = locations.begin(); iter != locations.end(); ++iter){
        // cout << "sprite " << count << endl;

        std::string slug = iter.key().toStdString();
        QJsonObject obj = iter.value().toObject();
        int y = obj.find("top")->toInt();
        int x = obj.find("left")->toInt();
        QImage sprite = sprites.copy(x, y, width, height);

        sprite = sprite.scaled(50, 50);

        visit_sprit(slug, sprite);
    }
}

MMOSpriteMatchingData build_MMO_sprite_matching_data(){
    ImageMatch::WeightedExactImageMatcher::InverseStddevWeight color_stddev_weight;
    color_stddev_weight.stddev_coefficient = 0.004;
    // stddev_weight.stddev_coefficient = 0.1;
    color_stddev_weight.offset = 1.0;
    ExactImageDictionaryMatcher color_matcher(color_stddev_weight);

    ImageMatch::WeightedExactImageMatcher::InverseStddevWeight gradient_stddev_weight;
    gradient_stddev_weight.stddev_coefficient = 0.000;
    // stddev_weight.stddev_coefficient = 0.1;
    gradient_stddev_weight.offset = 1.0;
    ExactImageDictionaryMatcher gradient_matcher(gradient_stddev_weight);

    std::map<std::string, FeatureVector> features;

    load_and_visit_MMO_sprite([&](const std::string& slug, QImage& sprite){
        color_matcher.add(slug, sprite);
        QImage sprite_gradient = compute_image_gradient(sprite);
        gradient_matcher.add(slug, sprite_gradient);

        features.emplace(slug, compute_feature(sprite));
    });

    return MMOSpriteMatchingData(
        std::move(color_matcher),
        std::move(gradient_matcher),
        std::move(features)
    );
}

const MMOSpriteMatchingData& MMO_SPRITE_MATCHING_DATA(){
    const static auto& sprite_matching_data = build_MMO_sprite_matching_data();

    return sprite_matching_data;
}


std::multimap<double, std::string> match_pokemon_map_sprite(const ConstImageRef& image){
    const FeatureVector& image_feature = compute_feature(image);


    const std::map<std::string, FeatureVector>& features = MMO_SPRITE_MATCHING_DATA().features;

    // cout << "input image feature: " << feature_to_str(image_feature) << endl;

    // FeatureType closest_dist = FLT_MAX;
    // std::string closest_slug = "";

    std::multimap<double, std::string> result;

    for(const auto& p : features){
        const std::string& slug = p.first;
        const FeatureVector& feature = p.second;
        const FeatureType dist = feature_distance(image_feature, feature);
        result.emplace(dist, slug);
    }

    // cout << "Closest feature distance " << closest_dist << ", slug " << closest_slug << endl;
    // cout << feature_to_str(features.find(closest_slug)->second) << endl;

    return result;
}






const ExactImageDictionaryMatcher& get_MMO_sprite_matcher(){
    const static auto matcher = MMO_SPRITE_MATCHING_DATA().color_matcher;
    return matcher;
}

const ExactImageDictionaryMatcher& get_MMO_sprite_gradient_matcher(){
    const static auto matcher = MMO_SPRITE_MATCHING_DATA().gradient_matcher;
    return matcher;
}

// For a sprite on the screenshot, create gradient image of it
QImage compute_MMO_sprite_gradient(const ConstImageRef& image){
    QImage result = compute_image_gradient(image);
    ImageRef result_ref(result);
    
    int width = image.width();
    int height = image.height();
    float r = (width + height) / 4.0;
    float center_x  = (width-1) / 2.0f;
    float center_y = (height-1) / 2.0f;
    // -r/8 to remove some boundary areas
    float dist2_th = (r - r/8) * (r - r/8);
    for(int y = 0; y < height; y++){
        for(int x = 0; x < width; x++){
            if ((x-center_x)*(x-center_x) + (y-center_y)*(y-center_y) >= dist2_th){
                // gradients outside of the sprite circle is set to zero
                result_ref.pixel(x, y) = combine_argb(0,0,0,0);
            }
        }
    }    
    return result;
}











float compute_MMO_sprite_gradient_distance(const ConstImageRef& gradient_template, const ConstImageRef& gradient){
    int tempt_width = gradient_template.width();
    int tempt_height = gradient_template.height();

    FloatPixel image_brightness = ImageMatch::pixel_average(gradient, gradient_template);
    FloatPixel scale = image_brightness /  image_stats(gradient_template).average;

//    cout << image_brightness << m_stats.average << scale << endl;

    if (std::isnan(scale.r)) scale.r = 1.0;
    if (std::isnan(scale.g)) scale.g = 1.0;
    if (std::isnan(scale.b)) scale.b = 1.0;
    scale.bound(0.8, 1.2);

    QImage scaled_template = gradient_template.to_qimage();
    ImageMatch::scale_brightness(scaled_template, scale);


    static int count = 0;
    QImage output(gradient.width(), gradient.height(), QImage::Format::Format_ARGB32);
    output.fill(QColor(0,0,0,0));

    // cout << "Size check " << tempt_width << " x " << tempt_height << ",  " <<
    // gradient.width() << " x " << gradient.height() << endl;

    float score = 0.0f;
    int max_offset = 2;

// #define USE_IMAGE_LEVEL_TRANSLATION
// #define USE_PIXEL_LEVEL_TRANSLATION
#define USE_BLOCK_LEVEL_TRANSLATION

#ifdef USE_IMAGE_LEVEL_TRANSLATION
    score = FLT_MAX;
    for(int oy = -max_offset; oy <= max_offset; oy++){ // offset_y
        for(int ox = -max_offset; ox <= max_offset; ox++){ // offset_x

            float match_score = 0.0;
            int num_gradients = 0;
            for(int y = 0; y < gradient.height(); y++){
                for(int x = 0; x < gradient.width(); x++){
                    uint32_t g = gradient.pixel(x, y);
                    uint32_t gx = uint32_t(0xff) & (g >> 16);
                    uint32_t gy = uint32_t(0xff) & (g >> 8);
                    uint32_t alpha = g >> 24;
                    if (alpha < 128){
                        continue;
                    }

                    int my = y + oy; // moved y
                    int mx = x + ox; // moved x
                    if (mx < 0 || mx >= tempt_width || my < 0 || my >= tempt_height){
                        continue;
                    }
                    // int dist_x = std::abs(ox);
                    // int dist_y = std::abs(oy);
                    // int dist2 = dist_x * dist_x + dist_y * dist_y;
                    uint32_t t_g = scaled_template.pixel(mx, my);
                    uint32_t t_a = t_g >> 24;
                    if (t_a < 128){
                        continue;
                    }

                    num_gradients++;

                    uint32_t t_gx = uint32_t(0xff) & (t_g >> 16);
                    uint32_t t_gy = uint32_t(0xff) & (t_g >> 8);
                    float pixel_score = std::max(t_gx, gx) * (gx - t_gx) * (gx - t_gx) + std::max(t_gy, gy) * (gy - t_gy) * (gy - t_gy);
                    pixel_score /= 255;
                    match_score += pixel_score;

                    output.setPixelColor(x, y, QColor(
                        std::min((int)std::sqrt(gx*gx+gy*gy),255),
                        std::min((int)std::sqrt(t_gx*t_gx+t_gy*t_gy), 255),
                        0
                    ));
                }
            }

            match_score = std::sqrt(match_score / num_gradients);
            if (match_score < score){
                score = match_score;
            }
        }
    }
#endif

#ifdef USE_PIXEL_LEVEL_TRANSLATION
    score = 0;
    int num_gradients = 0;
    for(int y = 0; y < gradient.height(); y++){
        for(int x = 0; x < gradient.width(); x++){
            uint32_t g = gradient.pixel(x, y);
            uint32_t gx = uint32_t(0xff) & (g >> 16);
            uint32_t gy = uint32_t(0xff) & (g >> 8);
            uint32_t alpha = g >> 24;
            if (alpha < 128){
                continue;
            }

            float min_pixel_score = FLT_MAX;
            for(int oy = -max_offset; oy <= max_offset; oy++){ // offset_y
                for(int ox = -max_offset; ox <= max_offset; ox++){ // offset_x
                    int my = y + oy; // moved y
                    int mx = x + ox; // moved x
                    if (mx < 0 || mx >= tempt_width || my < 0 || my >= tempt_height){
                        continue;
                    }
                    // int dist_x = std::abs(ox);
                    // int dist_y = std::abs(oy);
                    // int dist2 = dist_x * dist_x + dist_y * dist_y;
                    uint32_t t_g = scaled_template.pixel(mx, my);
                    uint32_t t_a = t_g >> 24;
                    if (t_a < 128){
                        continue;
                    }

                    uint32_t t_gx = uint32_t(0xff) & (t_g >> 16);
                    uint32_t t_gy = uint32_t(0xff) & (t_g >> 8);
                    float pixel_score = std::max(t_gx, gx) * (gx - t_gx) * (gx - t_gx) + std::max(t_gy, gy) * (gy - t_gy) * (gy - t_gy);
                    pixel_score /= 255;
                    if (pixel_score < min_pixel_score){
                        min_pixel_score = pixel_score;
                    }

                    if (ox == 0 && oy == 0){
                        output.setPixelColor(x, y, QColor(
                            std::min((int)std::sqrt(gx*gx+gy*gy),255),
                            std::min((int)std::sqrt(t_gx*t_gx+t_gy*t_gy), 255),
                            0
                        ));
                    }
                }
            } // end offset

            if (min_pixel_score < FLT_MAX){
                score += min_pixel_score;
                num_gradients++;
            }
        }
    }
    score = std::sqrt(score / num_gradients);
#endif

#ifdef USE_BLOCK_LEVEL_TRANSLATION
    int block_radius = 5;

    score = 0;
    int num_gradients = 0;

    auto compute_pixel_dist = [](uint32_t t_g, uint32_t g){
        uint32_t gx = uint32_t(0xff) & (g >> 16);
        uint32_t gy = uint32_t(0xff) & (g >> 8);
        uint32_t t_gx = uint32_t(0xff) & (t_g >> 16);
        uint32_t t_gy = uint32_t(0xff) & (t_g >> 8);
        float pixel_score = std::max(t_gx, gx) * (gx - t_gx) * (gx - t_gx) + std::max(t_gy, gy) * (gy - t_gy) * (gy - t_gy);
        pixel_score /= 255;
        return pixel_score;
    };

    auto is_transparent = [](uint32_t g){
        return (g >> 24) < 128;
    };

    for(int y = 0; y < gradient.height(); y++){
        for(int x = 0; x < gradient.width(); x++){
            uint32_t g = gradient.pixel(x, y);
            if (is_transparent(g)){
                continue;
            }

            float min_block_score = FLT_MAX;
            for(int oy = -max_offset; oy <= max_offset; oy++){ // offset_y
                for(int ox = -max_offset; ox <= max_offset; ox++){ // offset_x

                    float block_score = 0.0;
                    int block_size = 0;
                    for(int by = y - block_radius; by <= y + block_radius; by++){
                        for(int bx = x - block_radius; bx <= x + block_radius; bx++){
                            if (bx < 0 || bx >= gradient.width() || by < 0 || by >= gradient.height()){
                                continue;
                            }

                            uint32_t bg = gradient.pixel(bx, by);
                            if (is_transparent(bg)){
                                continue;
                            }

                            int ty = by + oy; // template y
                            int tx = bx + ox; // template x
                            if (tx < 0 || tx >= tempt_width || ty < 0 || ty >= tempt_height){
                                continue;
                            }
                            uint32_t t_bg = gradient_template.pixel(tx, ty);
                            if (is_transparent(t_bg)){
                                continue;
                            }

                            block_score += compute_pixel_dist(t_bg, bg);
                            block_size++;
                        }
                    }
                    block_score = block_score / block_size;
                    min_block_score = std::min(min_block_score, block_score);
                }
            } // end offset

            if (min_block_score < FLT_MAX){
                score += min_block_score;
                num_gradients++;
            }
        }
    }
    score = std::sqrt(score / num_gradients);
#endif

    // output.save("test_distance_alignment_" + QString::number(count) + ".png");
    count++;

    return score;
}

}
}
}
