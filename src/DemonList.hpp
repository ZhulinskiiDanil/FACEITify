#pragma once

#include <Geode/Geode.hpp>

#include <functional>

namespace faceit
{
    namespace demonlist
    {
        constexpr int CHALLENGER_PLACES = 100;

        int placementOf(int levelID);

        // Asks about one level and remembers the answer
        void lookup(int levelID, std::function<void(int placement)> callback);

        void prefetch();
    }
}
