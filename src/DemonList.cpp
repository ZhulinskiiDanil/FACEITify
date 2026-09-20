#include "DemonList.hpp"

#include <Geode/utils/file.hpp>
#include <Geode/utils/web.hpp>

#include <chrono>
#include <unordered_map>
#include <vector>

using namespace geode::prelude;

namespace
{
    constexpr char const *LIST_URL = "https://api.demonlist.org/level/classic/list";
    constexpr char const *LEVEL_URL = "https://api.demonlist.org/level/classic/get";
    constexpr char const *USER_AGENT = "faceitify (Geode mod)";

    constexpr auto CACHE_LIFETIME = std::chrono::hours(1);
    constexpr auto REQUEST_TIMEOUT = std::chrono::seconds(15);

    constexpr auto LIST_TIMEOUT = std::chrono::seconds(60);
    constexpr auto RETRY_DELAY = std::chrono::minutes(10);

    struct Cache
    {
        // { levelID: placement }; I love JS >:), gugu-gaga
        std::unordered_map<int, int> placements;

        bool complete = false;
        std::unordered_map<int, std::vector<std::function<void(int)>>> pending;

        std::chrono::system_clock::time_point fetched{};
        std::chrono::system_clock::time_point attempted{};

        bool read = false;
        bool fetching = false;
    };

    Cache &cache()
    {
        static Cache instance;
        return instance;
    }

    std::filesystem::path cachePath()
    {
        return Mod::get()->getSaveDir() / "demonlist-top.json";
    }

    web::WebRequest request(std::chrono::seconds timeout = REQUEST_TIMEOUT)
    {
        auto request = web::WebRequest();
        request.userAgent(USER_AGENT);
        request.timeout(timeout);

        return request;
    }

    void readCacheFile()
    {
        auto &c = cache();
        c.read = true;

        auto contents = file::readString(cachePath());
        if (!contents)
            return;

        auto parsed = matjson::parse(contents.unwrap());
        if (!parsed)
            return;

        auto root = parsed.unwrap();
        if (!root.contains("fetched") || !root.contains("levels"))
            return;

        auto const seconds = root["fetched"].asInt().unwrapOr(0);

        if (seconds > 0)
            c.fetched = std::chrono::system_clock::time_point{std::chrono::seconds{seconds}};

        for (auto &entry : root["levels"])
        {
            auto const key = entry.getKey();
            if (!key)
                continue;

            auto const id = utils::numFromString<int>(*key).unwrapOr(0);
            auto const placement = entry.asInt().unwrapOr(0);
            if (id > 0 && placement > 0)
            {
                c.placements[id] = static_cast<int>(placement);
                c.complete = true;
            }
        }
    }

    void writeCacheFile()
    {
        auto &c = cache();

        auto levels = matjson::Value::object();
        for (auto const &[id, placement] : c.placements)
        {
            if (placement > 0)
                levels.set(std::to_string(id), placement);
        }

        auto root = matjson::Value::object();
        root.set("fetched", std::chrono::duration_cast<std::chrono::seconds>(c.fetched.time_since_epoch()).count());
        root.set("levels", levels);

        auto written = file::writeString(cachePath(), root.dump(matjson::NO_INDENTATION));
        if (!written)
            log::warn("could not save the demon list cache: {}", std::move(written).unwrapErr());
    }

    void fetchList()
    {
        auto &c = cache();
        c.fetching = true;
        c.attempted = std::chrono::system_clock::now();

        auto bulk = request(LIST_TIMEOUT);

        async::spawn(bulk.get(LIST_URL), [](web::WebResponse response)
                     {
            auto &c = cache();
            c.fetching = false;

            if (!response.ok())
            {
                log::warn(
                    "demon list request failed with code {}, not retrying for {} minutes",
                    response.code(), RETRY_DELAY.count()
                );
                return;
            }

            auto json = response.json();
            if (!json)
            {
                log::warn("demon list sent something that is not JSON");
                return;
            }

            auto body = json.unwrap();
            if (!body.contains("data") || !body["data"].contains("levels"))
            {
                log::warn("demon list sent JSON without data.levels");
                return;
            }

            int found = 0;
            for (auto &level : body["data"]["levels"])
            {
                if (!level.contains("ingame_id") || !level.contains("placement"))
                    continue;

                auto const id = level["ingame_id"].asInt().unwrapOr(0);
                auto const placement = level["placement"].asInt().unwrapOr(0);
                if (id > 0 && placement > 0)
                {
                    c.placements[static_cast<int>(id)] = static_cast<int>(placement);
                    ++found;
                }
            }

            if (!found)
                return;

            c.complete = true;

            c.fetched = std::chrono::system_clock::now();
            writeCacheFile();

            log::info("demon list: {} placements cached", found); });
    }

    void ensureFresh()
    {
        auto &c = cache();
        if (!c.read)
            readCacheFile();

        auto const now = std::chrono::system_clock::now();
        if (c.fetching || now - c.fetched <= CACHE_LIFETIME || now - c.attempted <= RETRY_DELAY)
            return;

        fetchList();
    }
}

namespace faceit::demonlist
{
    int placementOf(int levelID)
    {
        ensureFresh();

        auto const &placements = cache().placements;
        auto const found = placements.find(levelID);

        return found == placements.end() ? 0 : found->second;
    }

    void lookup(int levelID, std::function<void(int placement)> callback)
    {
        ensureFresh();

        auto &c = cache();
        if (auto const known = c.placements.find(levelID); known != c.placements.end())
        {
            callback(known->second);
            return;
        }

        if (c.complete)
        {
            callback(0);
            return; // Whole list in hand and the level isn't in it. Nothing to ask
        }

        auto &waiting = c.pending[levelID];
        waiting.push_back(std::move(callback));

        if (waiting.size() > 1)
            return;

        auto single = request();
        single.param("ingame_id", levelID);

        async::spawn(single.get(LEVEL_URL), [levelID](web::WebResponse response)
                     {
            auto &c = cache();

            auto waiting = std::move(c.pending[levelID]);
            c.pending.erase(levelID);

            auto const code = response.code();
            if (code != 200 && code != 404)
            {
                log::warn("demon list lookup for {} failed with code {}", levelID, code);
                return;
            }

            int placement = 0;
            if (code == 200)
            {
                auto json = response.json();
                if (!json)
                    return;

                auto body = json.unwrap();
                if (!body.contains("data") || !body["data"].contains("placement"))
                    return;

                placement = static_cast<int>(body["data"]["placement"].asInt().unwrapOr(0));
            }

            c.placements[levelID] = placement;

            for (auto &callback : waiting)
                callback(placement); });
    }

    void prefetch()
    {
        ensureFresh();
    }
}
