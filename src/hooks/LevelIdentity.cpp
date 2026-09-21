#include <Geode/Geode.hpp>
#include <Geode/modify/LevelCell.hpp>
#include <Geode/modify/LevelInfoLayer.hpp>

#include "../DemonList.hpp"
#include "../FACEITLevel.hpp"

using namespace geode::prelude;
using namespace faceit;

namespace
{
    FACEITLevel *badgeOf(CCNode *root)
    {
        return typeinfo_cast<FACEITLevel *>(root->getChildByIDRecursive("level"_spr));
    }

    void applyDemonList(CCNode *root, GJGameLevel *level, bool ask)
    {
        if (!level)
            return;

        auto badge = badgeOf(root);
        if (!badge)
            return;

        auto const levelID = level->m_levelID.value();
        badge->setLevelID(levelID);

        if (!ask)
            return;

        badge->playIntro();

        if (badge->getPlacement() > 0)
            return;

        demonlist::lookup(levelID, [badge = Ref<FACEITLevel>(badge)](int placement)
                          {
            if (placement <= 0 || !badge->getParent())
                return;

            badge->applyPlacement(placement);
            badge->playIntro(); });
    }
}

class $modify(FACEITLevelCell, LevelCell)
{
    void loadFromLevel(GJGameLevel *level)
    {
        LevelCell::loadFromLevel(level);
        applyDemonList(this, level, false);
    }
};

class $modify(FACEITLevelInfoLayer, LevelInfoLayer)
{
    bool init(GJGameLevel *level, bool challenge)
    {
        if (!LevelInfoLayer::init(level, challenge))
            return false;

        applyDemonList(this, level, true);
        return true;
    }

    void levelDownloadFinished(GJGameLevel *level)
    {
        auto const badge = badgeOf(this);
        auto const before = badge ? badge->getLevel() : UNRATED_LEVEL;

        LevelInfoLayer::levelDownloadFinished(level);

        if (auto const current = badgeOf(this); current && current->getLevel() != before)
            applyDemonList(this, level, true);
    }
};
