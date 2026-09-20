#pragma once

#include <Geode/Geode.hpp>
#include <Geode/ui/Label.hpp>

namespace faceit
{
    // Skill levels 1...10, 0 unrated
    constexpr int UNRATED_LEVEL = 0;
    constexpr int MIN_LEVEL = 1;
    constexpr int MAX_LEVEL = 10;

    float iconSize(GJDifficultyName name = GJDifficultyName::Short);

    int levelFromDifficulty(GJDifficulty difficulty);
    int levelFromDifficulty(int difficulty);

    class FACEITLevel : public cocos2d::CCNodeRGBA
    {
    public:
        static constexpr float DEFAULT_SIZE = 30.f;

        static FACEITLevel *create(int level, float size = DEFAULT_SIZE);
        static FACEITLevel *createForDifficulty(GJDifficulty difficulty, float size = DEFAULT_SIZE);

        // Both false if the badge sprite failed to load
        bool loadFromLevel(int level);
        bool applyPlacement(int placement);

        void setLevelID(int levelID);

        void setSelected(bool selected);
        bool isSelected() const;

        int getLevel() const;
        int getPlacement() const;

        void setOpacity(GLubyte opacity) override;

    protected:
        bool init(int level, float size);
        bool rebuild();
        void updateArc();
        void updateOutline();

        int m_difficultyLevel = UNRATED_LEVEL; // what GD says
        int m_placement = 0;                   // what the demon list says
        int m_level = UNRATED_LEVEL;           // what the two come out as
        int m_levelID = 0;
        bool m_selected = false;
        std::string m_template;
        cocos2d::CCDrawNode *m_arc = nullptr;
        cocos2d::CCDrawNode *m_outline = nullptr;
        cocos2d::CCSprite *m_badge = nullptr;
        geode::Label *m_label = nullptr;
    };
}
