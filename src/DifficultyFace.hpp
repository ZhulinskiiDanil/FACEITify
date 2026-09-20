#pragma once

#include <Geode/Geode.hpp>

#include <functional>

namespace faceit
{
    void replaceDifficultyFace(
        cocos2d::CCNode *node,
        std::function<bool(int difficulty)> const &isSelected = {});
}
