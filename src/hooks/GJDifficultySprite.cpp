#include <Geode/Geode.hpp>
#include <Geode/modify/GJDifficultySprite.hpp>

#include <algorithm>

#include "../FACEITLevel.hpp"

using namespace geode::prelude;
using namespace faceit;

namespace
{
    CCPoint const BADGE_OFFSET{0.f, 6.f};
}

// Level cells, level info, search results, profiles, leaderboards
class $modify(FACEITDifficultySprite, GJDifficultySprite)
{
    struct Fields
    {
        int m_difficulty = 0;
        GJDifficultyName m_name = GJDifficultyName::Short;
    };

    static GJDifficultySprite *create(int difficulty, GJDifficultyName name)
    {
        auto ret = GJDifficultySprite::create(difficulty, name);
        if (ret)
            static_cast<FACEITDifficultySprite *>(ret)->applyFACEITLevel(difficulty, name);
        return ret;
    }

    void updateDifficultyFrame(int difficulty, GJDifficultyName name)
    {
        GJDifficultySprite::updateDifficultyFrame(difficulty, name);
        this->applyFACEITLevel(difficulty, name);
    }

    void updateFeatureState(GJFeatureState state)
    {
        GJDifficultySprite::updateFeatureState(state);
        this->applyFACEITLevel(m_fields->m_difficulty, m_fields->m_name);
    }

    void applyFACEITLevel(int difficulty, GJDifficultyName name)
    {
        m_fields->m_difficulty = difficulty;
        m_fields->m_name = name;

        this->setCascadeOpacityEnabled(false);
        this->setCascadeColorEnabled(false);
        this->setOpacity(0);

        auto const level = levelFromDifficulty(difficulty);

        if (auto existing = typeinfo_cast<FACEITLevel *>(this->getChildByID("level"_spr)))
        {
            existing->loadFromLevel(level);
            existing->updateAnchoredPosition(Anchor::Center, BADGE_OFFSET);
            return;
        }

        auto nodeSize = iconSize(name);
        if (nodeSize <= 0.f)
            nodeSize = FACEITLevel::DEFAULT_SIZE; // no frame to measure yet

        auto node = FACEITLevel::create(level, nodeSize);
        // Resources missing: better the vanilla face than nothing.
        if (!node)
        {
            this->setOpacity(255);
            return;
        }
        node->setID("level"_spr);

        this->addChildAtPosition(node, Anchor::Center, BADGE_OFFSET);
    }
};
