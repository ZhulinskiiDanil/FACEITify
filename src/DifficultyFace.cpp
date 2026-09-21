#include "DifficultyFace.hpp"

#include <limits>
#include <utility>
#include <vector>

#include "FACEITLevel.hpp"

using namespace geode::prelude;

namespace
{
    std::vector<std::pair<CCRect, int>> const &difficultyFrames()
    {
        static auto const frames = []
        {
            std::vector<std::pair<CCRect, int>> result;
            auto cache = CCSpriteFrameCache::sharedSpriteFrameCache();

            for (int difficulty = faceit::AUTO_LEVEL; difficulty <= faceit::MAX_LEVEL; ++difficulty)
                for (auto name : {GJDifficultyName::Short, GJDifficultyName::Long})
                {
                    auto const frameName = GJDifficultySprite::getDifficultyFrame(difficulty, name);
                    if (auto frame = cache->spriteFrameByName(frameName.c_str()))
                        result.emplace_back(frame->getRect(), difficulty);
                }

            return result;
        }();

        return frames;
    }

    constexpr int NO_DIFFICULTY = std::numeric_limits<int>::min();

    int difficultyOfSprite(CCSprite *sprite)
    {
        auto const rect = sprite->getTextureRect();
        for (auto const &[frameRect, difficulty] : difficultyFrames())
            if (frameRect.equals(rect))
                return difficulty;

        return NO_DIFFICULTY;
    }

    CCSprite *findDifficultySprite(CCNode *node)
    {
        if (auto sprite = typeinfo_cast<CCSprite *>(node))
            if (difficultyOfSprite(sprite) != NO_DIFFICULTY || sprite->getChildByID("level"_spr))
                return sprite;

        auto children = node->getChildren();
        for (unsigned int i = 0; children && i < children->count(); ++i)
        {
            auto child = typeinfo_cast<CCNode *>(children->objectAtIndex(i));
            if (!child)
                continue;

            if (auto found = findDifficultySprite(child))
                return found;
        }

        return nullptr;
    }
}

namespace faceit
{
    void replaceDifficultyFace(CCNode *node, std::function<bool(int difficulty)> const &isSelected)
    {
        if (!node)
            return;

        auto sprite = findDifficultySprite(node);
        if (!sprite)
            return;

        auto const difficulty = difficultyOfSprite(sprite);

        sprite->setTextureRect(CCRectZero);

        auto badge = typeinfo_cast<FACEITLevel *>(sprite->getChildByID("level"_spr));
        if (!badge)
        {
            badge = FACEITLevel::create(levelFromDifficulty(difficulty), iconSize());
            if (!badge)
                return;

            badge->setID("level"_spr);
            sprite->addChild(badge);
        }
        else if (difficulty != NO_DIFFICULTY)
        {
            badge->loadFromLevel(levelFromDifficulty(difficulty));
        }

        if (difficulty != NO_DIFFICULTY)
            badge->setTag(difficulty);

        if (isSelected)
            badge->setSelected(isSelected(badge->getTag()));

        badge->setPosition({0.f, 0.f});
    }
}
