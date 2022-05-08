/*  Image Boxes
 *
 *  From: https://github.com/PokemonAutomation/Arduino-Source
 *
 */

#include <algorithm>
#include <QImage>
#include "Common/Cpp/Exceptions.h"
#include "Common/Cpp/Color.h"
#include "Kernels/Waterfill/Kernels_Waterfill_Types.h"
#include "ImageBoxes.h"

#include <iostream>
using std::cout;
using std::endl;

namespace PokemonAutomation{


ImagePixelBox::ImagePixelBox(size_t p_min_x, size_t p_min_y, size_t p_max_x, size_t p_max_y)
    : min_x((pxint_t)p_min_x), min_y((pxint_t)p_min_y)
    , max_x((pxint_t)p_max_x), max_y((pxint_t)p_max_y)
{
    if (min_x != (int64_t)p_min_x || max_x != (int64_t)p_max_x){
        throw InternalProgramError(
            nullptr, PA_CURRENT_FUNCTION,
            "Pixel Overflow: x = (" + std::to_string(p_min_x) + "," + std::to_string(p_max_x) + ")"
        );
    }
    if (min_y != (int64_t)p_min_y || max_y != (int64_t)p_max_y){
        throw InternalProgramError(
            nullptr, PA_CURRENT_FUNCTION,
            "Pixel Overflow: y = (" + std::to_string(p_min_y) + "," + std::to_string(p_max_y) + ")"
        );
    }
}
ImagePixelBox::ImagePixelBox(const Kernels::Waterfill::WaterfillObject& object)
    : ImagePixelBox(object.min_x, object.min_y, object.max_x, object.max_y)
{}


void ImagePixelBox::merge_with(const ImagePixelBox& box){
    if (box.area() == 0){
        return;
    }
    if (this->area() == 0){
        *this = box;
    }
    min_x = std::min(min_x, box.min_x);
    min_y = std::min(min_y, box.min_y);
    max_x = std::max(max_x, box.max_x);
    max_y = std::max(max_y, box.max_y);
}

bool ImagePixelBox::overlap(const ImagePixelBox& box) const{
    return !(box.min_x >= max_x || box.max_x <= min_x || box.min_y >= max_y || box.max_y <= min_y);
}

size_t ImagePixelBox::overlap_with(const ImagePixelBox& box) const{
    pxint_t min_x = std::max(this->min_x, box.min_x);
    pxint_t max_x = std::min(this->max_x, box.max_x);
    if (min_x >= max_x){
        return 0;
    }
    pxint_t min_y = std::max(this->min_y, box.min_y);
    pxint_t max_y = std::min(this->max_y, box.max_y);
    if (min_y >= max_y){
        return 0;
    }
    return (size_t)(max_x - min_x) * (size_t)(max_y - min_y);
}

bool ImagePixelBox::inside(pxint_t x, pxint_t y) const{
    return x > min_x && x < max_x && y > min_y && y < max_y;
}

void ImagePixelBox::clip(size_t image_width, size_t image_height){
    min_x = std::max(0, min_x);
    min_y = std::max(0, min_y);
    max_x = std::min(max_x, image_width > 0 ? (int)image_width-1 : 0);
    max_y = std::min(max_y, image_height > 0 ? (int)image_height-1 : 0);
}

size_t ImagePixelBox::distance_x(const ImagePixelBox& box) const{
    pxint_t min_x = std::max(this->min_x, box.min_x);
    pxint_t max_x = std::min(this->max_x, box.max_x);
    if (min_x >= max_x){
        return min_x - max_x;
    }
    return 0;
}

size_t ImagePixelBox::distance_y(const ImagePixelBox& box) const{
    pxint_t min_y = std::max(this->min_y, box.min_y);
    pxint_t max_y = std::min(this->max_y, box.max_y);
    if (min_y >= max_y){
        return min_y - max_y;
    }
    return 0;
}



ConstImageRef extract_box_reference(const ConstImageRef& image, const ImagePixelBox& box){
    return image.sub_image(box.min_x, box.min_y, box.width(), box.height());
}
ConstImageRef extract_box_reference(const ConstImageRef& image, const ImageFloatBox& box){
    size_t min_x = (size_t)(image.width() * box.x + 0.5);
    size_t min_y = (size_t)(image.height() * box.y + 0.5);
    size_t width = (size_t)(image.width() * box.width + 0.5);
    size_t height = (size_t)(image.height() * box.height + 0.5);
    return image.sub_image(min_x, min_y, width, height);
}
ImageRef extract_box_reference(const ImageRef& image, const ImagePixelBox& box){
    return image.sub_image(box.min_x, box.min_y, box.width(), box.height());
}
ImageRef extract_box_reference(const ImageRef& image, const ImageFloatBox& box){
    size_t min_x = (size_t)(image.width() * box.x + 0.5);
    size_t min_y = (size_t)(image.height() * box.y + 0.5);
    size_t width = (size_t)(image.width() * box.width + 0.5);
    size_t height = (size_t)(image.height() * box.height + 0.5);
    return image.sub_image(min_x, min_y, width, height);
}

ConstImageRef extract_box_reference(const QImage& image, const ImagePixelBox& box){
    return extract_box_reference(ConstImageRef(image), box);
}
ConstImageRef extract_box_reference(const QImage& image, const ImageFloatBox& box){
    return extract_box_reference(ConstImageRef(image), box);
}
ImageRef extract_box_reference(QImage& image, const ImagePixelBox& box){
    return extract_box_reference(ImageRef(image), box);
}
ImageRef extract_box_reference(QImage& image, const ImageFloatBox& box){
    return extract_box_reference(ImageRef(image), box);
}
ConstImageRef extract_box_reference(const ConstImageRef& image, const ImageFloatBox& box, int offset_x, int offset_y){
    ptrdiff_t min_x = (ptrdiff_t)(image.width() * box.x + 0.5) + offset_x;
    ptrdiff_t min_y = (ptrdiff_t)(image.height() * box.y + 0.5) + offset_y;
    ptrdiff_t width = (ptrdiff_t)(image.width() * box.width + 0.5);
    ptrdiff_t height = (ptrdiff_t)(image.height() * box.height + 0.5);

    if (min_x < 0){
        width += min_x;
        min_x = 0;
        width = std::max<ptrdiff_t>(width, 0);
    }
    if (min_y < 0){
        height += min_y;
        min_y = 0;
        height = std::max<ptrdiff_t>(height, 0);
    }

    return image.sub_image(min_x, min_y, width, height);
}


QImage extract_box_copy(const QImage& image, const ImagePixelBox& box){
    return image.copy(box.min_x, box.min_y, box.width(), box.height());
}
QImage extract_box_copy(const QImage& image, const ImageFloatBox& box){
    return image.copy(
        (pxint_t)(image.width() * box.x + 0.5),
        (pxint_t)(image.height() * box.y + 0.5),
        (pxint_t)(image.width() * box.width + 0.5),
        (pxint_t)(image.height() * box.height + 0.5)
    );
}
QImage extract_box(const QImage& image, const ImageFloatBox& box, int offset_x, int offset_y){
    return image.copy(
        (pxint_t)(image.width() * box.x + 0.5) + offset_x,
        (pxint_t)(image.height() * box.y + 0.5) + offset_y,
        (pxint_t)(image.width() * box.width + 0.5),
        (pxint_t)(image.height() * box.height + 0.5)
    );
}


ImageFloatBox translate_to_parent(
    const ConstImageRef& original_image,
    const ImageFloatBox& inference_box,
    const ImagePixelBox& box
){
    double width = original_image.width();
    double height = original_image.height();
    pxint_t box_x = (pxint_t)(width * inference_box.x + 0.5);
    pxint_t box_y = (pxint_t)(height * inference_box.y + 0.5);
    return ImageFloatBox(
        (box_x + box.min_x) / width,
        (box_y + box.min_y) / height,
        (box.max_x - box.min_x) / width,
        (box.max_y - box.min_y) / height
    );
}


ImagePixelBox floatbox_to_pixelbox(size_t width, size_t height, const ImageFloatBox& float_box){
    return ImagePixelBox(
        (pxint_t)(width * float_box.x + 0.5),
        (pxint_t)(height * float_box.y + 0.5),
        (pxint_t)(width * (float_box.x + float_box.width) + 0.5),
        (pxint_t)(height * (float_box.y + float_box.height) + 0.5)
    );
}
ImageFloatBox pixelbox_to_floatbox(size_t width, size_t height, const ImagePixelBox& pixel_box){
    double image_inverse_width = 1. / (double)width;
    double image_inverse_height = 1. / (double)height;
    return ImageFloatBox(
        pixel_box.min_x * image_inverse_width,
        pixel_box.min_y * image_inverse_height,
        pixel_box.width() * image_inverse_width,
        pixel_box.height() * image_inverse_height
    );
}
ImageFloatBox pixelbox_to_floatbox(const QImage& image, const ImagePixelBox& pixel_box){
    return pixelbox_to_floatbox(image.width(), image.height(), pixel_box);
}


ImagePixelBox extract_object_from_inner_feature(
    const ImagePixelBox& inner_relative_to_image,
    const ImageFloatBox& inner_relative_to_object
){
    double scale_x = inner_relative_to_image.width() / inner_relative_to_object.width;
    double scale_y = inner_relative_to_image.height() / inner_relative_to_object.height;

    double shift_x = inner_relative_to_image.min_x - inner_relative_to_object.x * scale_x;
    double shift_y = inner_relative_to_image.min_y - inner_relative_to_object.y * scale_y;

    return ImagePixelBox(
        (pxint_t)(shift_x + 0.5),
        (pxint_t)(shift_y + 0.5),
        (pxint_t)(shift_x + scale_x + 0.5),
        (pxint_t)(shift_y + scale_y + 0.5)
    );
}
QImage extract_object_from_inner_feature(
    const QImage& image,
    const ImagePixelBox& inner_relative_to_image,
    const ImageFloatBox& inner_relative_to_object
){
    return extract_box_copy(
        image,
        extract_object_from_inner_feature(
            inner_relative_to_image,
            inner_relative_to_object
        )
    );
}


void draw_box(QImage& image, const ImagePixelBox& pixel_box, uint32_t color, size_t thickness){
    if (thickness == 0 || image.width() <= 0 || image.height() <= 0){
        return;
    }

    Color c(color);
    auto clamp_x = [&](pxint_t x){
        return std::min(std::max(x, 0), image.width()-1);
    };
    auto clamp_y = [&](pxint_t y){
        return std::min(std::max(y, 0), image.height()-1);
    };

    auto draw_solid_rect = [&](pxint_t start_x, pxint_t start_y, pxint_t end_x, pxint_t end_y){
        start_x = clamp_x(start_x);
        end_x = clamp_x(end_x);
        start_y = clamp_y(start_y);
        end_y = clamp_y(end_y);
        if (start_x > end_x){
            std::swap(start_x, end_x);
        }
        if (start_y > end_y){
            std::swap(start_y, end_y);
        }
        QColor qColor(c.r(), c.g(), c.b(), c.a());
        for (pxint_t y = start_y; y <= end_y; ++y){
            for (pxint_t x = start_x; x <= end_x; ++x){
                image.setPixelColor(x, y, qColor);
            }
        }
    };

    pxint_t lo = ((pxint_t)thickness - 1) / 2; // lower offset
    pxint_t uo = (pxint_t)thickness - lo - 1; // upper offset
    
    // draw the upper horizontal line
    draw_solid_rect(pixel_box.min_x-lo, pixel_box.min_y-lo, pixel_box.max_x+uo, pixel_box.min_y+uo);
    // draw the lower horizontal line
    draw_solid_rect(pixel_box.min_x-lo, pixel_box.max_y-lo, pixel_box.max_x+uo, pixel_box.max_y+uo);
    // draw the left vertical line
    draw_solid_rect(pixel_box.min_x-lo, pixel_box.min_y-lo, pixel_box.min_x+uo, pixel_box.max_y+uo);
    // draw the right vertical line
    draw_solid_rect(pixel_box.max_x-lo, pixel_box.min_y-lo, pixel_box.max_x+uo, pixel_box.max_y+uo);
}


}
