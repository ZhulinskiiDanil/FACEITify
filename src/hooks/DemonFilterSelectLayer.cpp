#include <Geode/Geode.hpp>
#include <Geode/modify/DemonFilterSelectLayer.hpp>

#include "../DifficultyFace.hpp"

using namespace geode::prelude;
using namespace faceit;

namespace
{
    bool isDemonTag(int tag)
    {
        return tag == 0 || (tag >= 6 && tag <= 10);
    }

    int demonTagOf(CCNode *node)
    {
        return node && isDemonTag(node->getTag()) ? node->getTag() : -1;
    }
}

class $modify(FACEITDemonFilterSelectLayer, DemonFilterSelectLayer)
{
    bool init()
    {
        if (!DemonFilterSelectLayer::init())
            return false;

        this->replaceDemonFaces();
        return true;
    }

    // Picking a type redraws the faces
    void selectRating(CCObject *sender)
    {
        DemonFilterSelectLayer::selectRating(sender);
        this->replaceDemonFaces();
    }

    void replaceDemonFaces()
    {
        auto demons = m_demons;

        for (unsigned int i = 0; demons && i < demons->count(); ++i)
        {
            auto node = typeinfo_cast<CCNode *>(demons->objectAtIndex(i));
            if (!node)
                continue;

            auto const tag = demonTagOf(node);

            replaceDifficultyFace(node, [this, tag](int)
                                  { return tag >= 0 && tag == m_currentDemon; });
        }
    }
};
