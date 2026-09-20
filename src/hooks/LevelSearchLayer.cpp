#include <Geode/Geode.hpp>
#include <Geode/modify/LevelSearchLayer.hpp>

#include "../DifficultyFace.hpp"

using namespace geode::prelude;
using namespace faceit;

class $modify(FACEITSearchLayer, LevelSearchLayer)
{
    bool init(int type)
    {
        if (!LevelSearchLayer::init(type))
            return false;

        this->replaceDifficultyFilters();
        return true;
    }

    // Toggling a filter puts the vanilla face back
    void toggleDifficultyNum(int diff, bool enabled)
    {
        LevelSearchLayer::toggleDifficultyNum(diff, enabled);
        this->replaceDifficultyFilters();
    }

    // Picking a demon type swaps the filter's face for that type's, which also
    // changes which level the button means
    void demonFilterSelectClosed(int filter)
    {
        LevelSearchLayer::demonFilterSelectClosed(filter);
        this->replaceDifficultyFilters();
    }

    void replaceDifficultyFilters()
    {
        // TODO: assumes checkDiff is keyed the same way as getDifficultyFrame.
        auto const isSelected = [this](int difficulty)
        {
            return this->checkDiff(difficulty);
        };

        auto filters = m_difficultySprites;
        for (unsigned int i = 0; filters && i < filters->count(); ++i)
            replaceDifficultyFace(typeinfo_cast<CCNode *>(filters->objectAtIndex(i)), isSelected);

        replaceDifficultyFace(m_lastDifficultySprite, isSelected);
    }
};
