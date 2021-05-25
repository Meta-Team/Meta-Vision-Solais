//
// Created by liuzikai on 3/6/21.
//

#include "ParamSetManager.h"
#include <iostream>
#include <pugixml.hpp>
#include <google/protobuf/util/json_util.h>

namespace meta {

ParamSetManager::ParamSetManager()
        : paramSetRoot(fs::path(PARAM_SET_ROOT) / "params"){

}

void ParamSetManager::reloadParamSetList() {
    paramsSetNames.clear();

    if (!fs::is_directory(paramSetRoot) || !fs::exists(paramSetRoot / "default.json")) {
        fs::create_directories(paramSetRoot);

        // Create default parameter file
        ParamSet params;
        params.set_camera_id(0);
        params.set_image_width(1280);
        params.set_image_height(720);
        params.set_fps(120);
        params.set_allocated_gamma(allocToggledDouble(false));

        params.set_enemy_color(ParamSet::BLUE);
        params.set_brightness_threshold(155);
        params.set_color_threshold_mode(ParamSet::RB_CHANNELS);
        params.set_allocated_hsv_red_hue(allocDoubleRange(150, 30));  // across the 0 (180) point
        params.set_allocated_hsv_blue_hue(allocDoubleRange(90, 150));
        params.set_rb_channel_threshold(55);
        params.set_allocated_color_dilate(allocToggledInt(true, 6));

        params.set_contour_fit_function(ParamSet::ELLIPSE);
        params.set_allocated_contour_pixel_count(allocToggledDouble(true, 15));
        params.set_allocated_contour_min_area(allocToggledDouble(false, 3));
        params.set_allocated_long_edge_min_length(allocToggledInt(true, 30));
        params.set_allocated_light_aspect_ratio(allocToggledDoubleRange(true, 2, 30));
        params.set_allocated_light_max_rotation(allocToggledDouble(true, 15));

        params.set_allocated_light_length_max_ratio(allocToggledDouble(true, 1.5));
        params.set_allocated_light_x_dist_over_l(allocToggledDoubleRange(false, 1, 3));
        params.set_allocated_light_y_dist_over_l(allocToggledDoubleRange(false, 0, 1));
        params.set_allocated_light_angle_max_diff(allocToggledDouble(true, 10));
        params.set_allocated_armor_aspect_ratio(allocToggledDoubleRange(true, 1.25, 5));

        saveParamSetToJson(params, paramSetRoot / "default.json");
    }

    for (const auto &entry : fs::directory_iterator(paramSetRoot)) {
        if (strcasecmp(entry.path().extension().c_str(), ".json") == 0) {
            paramsSetNames.emplace_back(entry.path().stem().string());
        }
    }

    curParamSetName = "default";  // switch to default
}

ParamSet ParamSetManager::loadCurrentParamSet() const {
    ParamSet p;
    auto filename = paramSetRoot / (curParamSetName + ".json");

    std::ifstream file(filename.string());
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    JsonStringToMessage(content, &p, google::protobuf::util::JsonParseOptions());

    return p;
}

void ParamSetManager::saveCurrentParamSet(const ParamSet &p) {
    saveParamSetToJson(p, paramSetRoot / (curParamSetName + ".json"));
}

void ParamSetManager::saveParamSetToJson(const ParamSet &p, const fs::path &filename) {
    std::string content;

    google::protobuf::util::JsonPrintOptions options;
    options.add_whitespace = true;
    options.always_print_primitive_fields = true;
    options.preserve_proto_field_names = true;
    MessageToJsonString(p, &content, options);

    std::ofstream(filename.string()) << content;
}

}