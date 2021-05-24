//
// Created by liuzikai on 5/22/21.
//

#include "ArmorSelector.h"
#include <algorithm>

namespace meta {

ArmorSelector::ArmorInfo ArmorSelector::select(std::vector<ArmorInfo> armors) {

    assert(!armors.empty());

    // Select the armor with minimal distance
    std::sort(armors.begin(), armors.end(), DistanceLess());

    return armors.front();
}

}