#include <Geode/Geode.hpp>

#include "DemonList.hpp"

using namespace geode::prelude;

$on_mod(Loaded)
{
    listenForSettingChanges<std::string>("list", [](std::string const &)
                                         { faceit::demonlist::sourceChanged(); });

    faceit::demonlist::prefetch();
}
