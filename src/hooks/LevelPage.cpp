#include <Geode/Geode.hpp>
#include <Geode/modify/LevelPage.hpp>

#include <algorithm>
#include <cmath>

#include "../FACEITLevel.hpp"

using namespace geode::prelude;
using namespace faceit;

namespace
{
    GJDifficulty difficultyOf(GJGameLevel *level)
    {
        if (!level)
            return GJDifficulty::NA;

        if (level->m_autoLevel)
            return GJDifficulty::Auto;

        if (level->m_demon.value() <= 0)
            return level->m_difficulty;

        switch (level->m_demonDifficulty)
        {
        case 3:
            return GJDifficulty::DemonEasy;
        case 4:
            return GJDifficulty::DemonMedium;
        case 5:
            return GJDifficulty::DemonInsane;
        case 6:
            return GJDifficulty::DemonExtreme;
        default:
            return GJDifficulty::Demon;
        }
    }

    CCSprite *faceOf(LevelPage *page)
    {
        if (auto found = typeinfo_cast<CCSprite *>(page->getChildByIDRecursive("difficulty-sprite")))
            return found;

        return page->m_difficultySprite;
    }

    // The frames differ in size by difficulty, so every page takes the size of
    // the one its family measures by, the way the faces elsewhere do
    float badgeSizeFor(CCSize const &face)
    {
        auto const side = std::min(face.width, face.height);

        auto const shortSide = iconSize(GJDifficultyName::Short);
        auto const longSide = iconSize(GJDifficultyName::Long);

        if (side <= 0.f)
            return shortSide;

        return std::fabs(side - shortSide) <= std::fabs(side - longSide) ? shortSide : longSide;
    }

    void replaceFace(LevelPage *page, GJGameLevel *level)
    {
        auto face = faceOf(page);
        if (!face)
            return;

        if (typeinfo_cast<GJDifficultySprite *>(face))
            return;

        auto const size = face->getContentSize();
        auto const anchor = face->getAnchorPoint();

        auto const badgeSize = badgeSizeFor(size);

        CCPoint const middle{
            size.width * (.5f - anchor.x),
            size.height * (.5f - anchor.y)};

        face->setTextureRect(CCRectZero);

        auto const skill = levelFromDifficulty(difficultyOf(level));

        auto badge = typeinfo_cast<FACEITLevel *>(face->getChildByID("level"_spr));
        if (!badge)
        {
            badge = FACEITLevel::create(skill, badgeSize);
            if (!badge)
                return;

            badge->setID("level"_spr);
            face->addChild(badge);
        }
        else
        {
            badge->loadFromLevel(skill);
        }

        badge->setPosition(middle);
    }
}

class $modify(FACEITLevelPage, LevelPage)
{
    bool init(GJGameLevel *level)
    {
        if (!LevelPage::init(level))
            return false;

        replaceFace(this, level);
        return true;
    }

    void updateDynamicPage(GJGameLevel *level)
    {
        LevelPage::updateDynamicPage(level);
        replaceFace(this, level);
    }
};
