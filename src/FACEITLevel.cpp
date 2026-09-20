#include "FACEITLevel.hpp"

#include "DemonList.hpp"

#include <algorithm>
#include <cmath>

using namespace geode::prelude;

namespace
{
    constexpr float INNER_DISC_RATIO = .55f;

    constexpr float ARC_INNER_RATIO = 77.f / 256.f;
    constexpr float ARC_OUTER_RATIO = 103.f / 256.f;
    constexpr float OUTLINE_INNER_RATIO = 104.f / 256.f;
    constexpr float OUTLINE_OUTER_RATIO = 127.f / 256.f;

    constexpr float ARC_BLOCKED_DEGREES = .225f * 360.f;
    constexpr float ARC_START_DEGREES = ARC_BLOCKED_DEGREES / 2.f;
    constexpr float ARC_SWEEP_DEGREES = 360.f - ARC_BLOCKED_DEGREES;
    constexpr float ARC_STEP_DEGREES = 3.f;

    constexpr cocos2d::ccColor3B ARC_TRACK_COLOR = {49, 49, 52};
    constexpr cocos2d::ccColor3B OUTLINE_COLOR = {255, 255, 255};
    constexpr cocos2d::ccColor3B CHALLENGER_TEXT_COLOR = {255, 255, 255};

    cocos2d::CCPoint pointOnArc(float radius, float degrees)
    {
        auto const angle = (-90.f - degrees) * 3.14159265f / 180.f;
        return {radius * std::cos(angle), radius * std::sin(angle)};
    }

    // A ring segment
    void drawArc(cocos2d::CCDrawNode *node, float innerRadius, float outerRadius,
                 float startDegrees, float sweepDegrees, cocos2d::ccColor3B color, GLubyte opacity)
    {
        if (sweepDegrees <= 0.f)
            return;

        auto fill = cocos2d::ccc4FFromccc3B(color);
        fill.a = static_cast<float>(opacity) / 255.f;
        auto const steps = std::max(1, static_cast<int>(std::ceil(sweepDegrees / ARC_STEP_DEGREES)));
        auto const step = sweepDegrees / static_cast<float>(steps);

        for (int i = 0; i < steps; ++i)
        {
            auto const from = startDegrees + step * static_cast<float>(i);
            auto const to = from + step;

            cocos2d::CCPoint quad[4] = {
                pointOnArc(innerRadius, from),
                pointOnArc(outerRadius, from),
                pointOnArc(outerRadius, to),
                pointOnArc(innerRadius, to),
            };
            node->drawPolygon(quad, 4, fill, 0.f, fill);
        }
    }

    // 1 white, 2-3 green, 4-7 yellow, 8-9 orange, 10 red
    cocos2d::ccColor3B colorForLevel(int level)
    {
        if (level >= 10)
            return {254, 31, 0};
        if (level >= 8)
            return {255, 99, 9};
        if (level >= 4)
            return {255, 200, 0};
        if (level >= 2)
            return {28, 228, 0};
        if (level >= faceit::MIN_LEVEL)
            return {238, 238, 238};

        // Unrated
        return {154, 154, 154};
    }
}

namespace faceit
{
    int levelFromDifficulty(GJDifficulty difficulty)
    {
        switch (difficulty)
        {
        case GJDifficulty::Auto:
            return 1;
        case GJDifficulty::Easy:
            return 1;
        case GJDifficulty::Normal:
            return 2;
        case GJDifficulty::Hard:
            return 3;
        case GJDifficulty::Harder:
            return 4;
        case GJDifficulty::Insane:
            return 5;
        case GJDifficulty::DemonEasy:
            return 6;
        case GJDifficulty::DemonMedium:
            return 7;
        // A plain demon is a hard demon.
        case GJDifficulty::Demon:
            return 8;
        case GJDifficulty::DemonInsane:
            return 9;
        case GJDifficulty::DemonExtreme:
            return 10;
        default:
            return UNRATED_LEVEL;
        }
    }

    float iconSize(GJDifficultyName name)
    {
        auto const frameName = GJDifficultySprite::getDifficultyFrame(static_cast<int>(GJDifficulty::Easy), name);
        auto frame = CCSpriteFrameCache::sharedSpriteFrameCache()->spriteFrameByName(frameName.c_str());
        if (!frame)
            return FACEITLevel::DEFAULT_SIZE;

        auto const size = frame->getOriginalSize();
        return std::min(size.width, size.height);
    }

    int levelFromDifficulty(int difficulty)
    {
        return levelFromDifficulty(static_cast<GJDifficulty>(difficulty));
    }

    FACEITLevel *FACEITLevel::create(int level, float size)
    {
        auto ret = new FACEITLevel();
        if (ret->init(level, size))
        {
            ret->autorelease();
            return ret;
        }
        delete ret;
        return nullptr;
    }

    FACEITLevel *FACEITLevel::createForDifficulty(GJDifficulty difficulty, float size)
    {
        return FACEITLevel::create(levelFromDifficulty(difficulty), size);
    }

    bool FACEITLevel::init(int level, float size)
    {
        if (!CCNodeRGBA::init())
            return false;

        this->setContentSize({size, size});
        this->setAnchorPoint({.5f, .5f});
        this->setCascadeOpacityEnabled(true);

        return this->loadFromLevel(level);
    }

    bool FACEITLevel::loadFromLevel(int level)
    {
        m_difficultyLevel = std::clamp(level, UNRATED_LEVEL, MAX_LEVEL);

        return this->rebuild();
    }

    void FACEITLevel::setLevelID(int levelID)
    {
        if (m_levelID == levelID)
            return;

        m_levelID = levelID;
        this->applyPlacement(demonlist::placementOf(levelID));
    }

    bool FACEITLevel::applyPlacement(int placement)
    {
        m_placement = std::max(placement, 0);

        return this->rebuild();
    }

    bool FACEITLevel::rebuild()
    {
        m_level = m_placement > 0 ? MAX_LEVEL : m_difficultyLevel;
        auto const challenger = m_placement > 0 && m_placement <= demonlist::CHALLENGER_PLACES;

        auto const size = this->getContentSize();
        auto const center = CCPoint{size.width / 2.f, size.height / 2.f};

        if (!m_arc)
        {
            m_arc = CCDrawNode::create();
            this->addChild(m_arc, 0);
        }
        m_arc->setVisible(!challenger); // the Challenger badge has no gap to fill

        auto const templateName = challenger ? "faceitChallenger.png"_spr : "faceitLevelTemplate.png"_spr;
        if (m_badge && m_template != templateName)
        {
            m_badge->removeFromParent();
            m_badge = nullptr;
        }

        if (!m_badge)
        {
            m_badge = CCSprite::create(templateName);
            if (!m_badge)
                return false;

            m_template = templateName;
            m_badge->setAnchorPoint({.5f, .5f});
            m_badge->setPosition(center);
            m_badge->setScale(size.width / m_badge->getContentSize().width);
            this->addChild(m_badge, 1);
        }

        if (!m_outline)
        {
            m_outline = CCDrawNode::create();
            this->addChild(m_outline, 3);
        }

        if (!m_label)
        {
            m_label = Label::create("", "gjFont17.fnt");
            m_label->setAnchorPoint({.5f, .5f});
            m_label->setPosition(center);
            this->addChild(m_label, 2);
        }

        if (challenger)
            m_label->setText(std::to_string(m_placement));
        else
            m_label->setText(m_level == UNRATED_LEVEL ? "?" : std::to_string(m_level));

        m_label->setColor(challenger ? CHALLENGER_TEXT_COLOR : colorForLevel(m_level));
        m_label->setLimitLabelSize(
            {size.width * INNER_DISC_RATIO, size.height * INNER_DISC_RATIO}, 0.5f, .1f);

        // // TODO: too clever by half, reaching both of those through setOpacity
        // this->setOpacity(this->getOpacity());

        this->updateArc();
        this->updateOutline();

        return true;
    }

    void FACEITLevel::updateArc()
    {
        if (!m_arc)
            return;

        auto const size = this->getContentSize();
        auto const opacity = this->getDisplayedOpacity();
        auto const innerRadius = size.width * ARC_INNER_RATIO;
        auto const outerRadius = size.width * ARC_OUTER_RATIO;

        m_arc->setPosition({size.width / 2.f, size.height / 2.f});
        m_arc->clear();

        // The whole run first, then the part the level has reached over it
        drawArc(
            m_arc,
            innerRadius,
            outerRadius,
            ARC_START_DEGREES,
            ARC_SWEEP_DEGREES,
            ARC_TRACK_COLOR,
            opacity);
        drawArc(
            m_arc, innerRadius, outerRadius, ARC_START_DEGREES,
            ARC_SWEEP_DEGREES * static_cast<float>(m_level) / static_cast<float>(MAX_LEVEL),
            colorForLevel(m_level), opacity);
    }

    void FACEITLevel::setSelected(bool selected)
    {
        if (m_selected == selected)
            return;

        m_selected = selected;
        this->updateOutline();
    }

    bool FACEITLevel::isSelected() const
    {
        return m_selected;
    }

    void FACEITLevel::updateOutline()
    {
        if (!m_outline)
            return;

        auto const size = this->getContentSize();

        m_outline->setPosition({size.width / 2.f, size.height / 2.f});
        m_outline->clear();

        if (!m_selected)
            return;

        drawArc(
            m_outline, size.width * OUTLINE_INNER_RATIO, size.width * OUTLINE_OUTER_RATIO,
            0.f, 360.f, OUTLINE_COLOR, this->getDisplayedOpacity());
    }

    int FACEITLevel::getLevel() const
    {
        return m_level;
    }

    int FACEITLevel::getPlacement() const
    {
        return m_placement;
    }
}
