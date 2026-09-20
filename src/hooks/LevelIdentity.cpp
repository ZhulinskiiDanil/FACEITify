#include <Geode/Geode.hpp>
#include <Geode/modify/LevelCell.hpp>
#include <Geode/modify/LevelInfoLayer.hpp>

#include "../DemonList.hpp"
#include "../FACEITLevel.hpp"

using namespace geode::prelude;
using namespace faceit;

namespace
{
    void applyDemonList(CCNode *root, GJGameLevel *level, bool ask)
    {
        if (!level)
            return;

        auto badge = typeinfo_cast<FACEITLevel *>(root->getChildByIDRecursive("level"_spr));
        if (!badge)
            return;

        auto const levelID = level->m_levelID.value();
        badge->setLevelID(levelID);

        if (!ask || badge->getPlacement() > 0)
            return;

        demonlist::lookup(levelID, [badge = Ref<FACEITLevel>(badge)](int placement)
                          {
            if (placement > 0 && badge->getParent())
                badge->applyPlacement(placement); });
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
};
