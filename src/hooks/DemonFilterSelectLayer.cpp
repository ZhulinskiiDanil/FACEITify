#include <Geode/Geode.hpp>
#include <Geode/modify/DemonFilterSelectLayer.hpp>

#include "../DifficultyFace.hpp"

using namespace geode::prelude;
using namespace faceit;

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
            replaceDifficultyFace(typeinfo_cast<CCNode *>(demons->objectAtIndex(i)));
    }
};
