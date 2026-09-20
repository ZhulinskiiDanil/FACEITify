#include <Geode/Geode.hpp>

#include "DemonList.hpp"

using namespace geode::prelude;

$on_mod(Loaded)
{
    faceit::demonlist::prefetch();
}
