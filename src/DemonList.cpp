#include "DemonList.hpp"

#include <Geode/utils/file.hpp>
#include <Geode/utils/web.hpp>

#include <chrono>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

using namespace geode::prelude;
using faceit::demonlist::Source;

namespace
{
    constexpr char const *USER_AGENT = "faceitify (Geode mod)";

    constexpr auto CACHE_LIFETIME = std::chrono::hours(1);
    constexpr auto REQUEST_TIMEOUT = std::chrono::seconds(15);

    constexpr auto LIST_TIMEOUT = std::chrono::seconds(60);
    constexpr auto RETRY_DELAY = std::chrono::minutes(10);

    constexpr char const *DEMONLIST_LIST_URL = "https://api.demonlist.org/level/classic/list";
    constexpr char const *DEMONLIST_LEVEL_URL = "https://api.demonlist.org/level/classic/get";

    constexpr char const *POINTERCRATE_LIST_URL = "https://pointercrate.com/api/v2/demons/listed/";
    constexpr int POINTERCRATE_PAGE_SIZE = 100;
    constexpr int POINTERCRATE_PAGES = 2;

    constexpr char const *AREDL_LIST_URL = "https://api.aredl.net/v2/api/aredl/levels";
    constexpr char const *AREDL_LEVEL_URL = "https://api.aredl.net/v2/api/aredl/levels/";

    using Placements = std::unordered_map<int, int>;

    struct Cache
    {
        Placements placements;

        bool complete = false;
        std::unordered_map<int, std::vector<std::function<void(int)>>> pending;

        std::vector<std::pair<int, std::function<void(int)>>> waiting;

        std::chrono::system_clock::time_point fetched{};
        std::chrono::system_clock::time_point attempted{};

        bool read = false;
        bool fetching = false;
    };

    Cache &cache(Source source)
    {
        static std::map<Source, Cache> instances;
        return instances[source];
    }

    std::chrono::system_clock::time_point now()
    {
        return std::chrono::system_clock::now();
    }

    Source sourceFromSetting()
    {
        auto const value = Mod::get()->getSettingValue<std::string>("list");

        if (value == "Pointercrate")
            return Source::Pointercrate;
        if (value == "AREDL")
            return Source::AREDL;
        if (value == "Off")
            return Source::Off;

        return Source::DemonListOrg;
    }

    Source &currentSource()
    {
        static Source source = sourceFromSetting();
        return source;
    }

    std::filesystem::path cachePath(Source source)
    {
        char const *file = "demonlist-top.json";

        if (source == Source::Pointercrate)
            file = "pointercrate-top.json";
        else if (source == Source::AREDL)
            file = "aredl-top.json";

        return Mod::get()->getSaveDir() / file;
    }

    web::WebRequest request(std::chrono::seconds timeout = REQUEST_TIMEOUT)
    {
        auto request = web::WebRequest();
        request.userAgent(USER_AGENT);
        request.timeout(timeout);

        return request;
    }

    void readCacheFile(Source source)
    {
        auto &c = cache(source);
        c.read = true;

        auto contents = file::readString(cachePath(source));
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

    void writeCacheFile(Source source)
    {
        auto &c = cache(source);

        auto levels = matjson::Value::object();
        for (auto const &[id, placement] : c.placements)
        {
            if (placement > 0)
                levels.set(std::to_string(id), placement);
        }

        auto root = matjson::Value::object();
        root.set("fetched", std::chrono::duration_cast<std::chrono::seconds>(c.fetched.time_since_epoch()).count());
        root.set("levels", levels);

        (void)file::writeString(cachePath(source), root.dump(matjson::NO_INDENTATION));
    }

    // Only two of the lists answer about a single level
    bool hasLevelEndpoint(Source source)
    {
        return source == Source::DemonListOrg || source == Source::AREDL;
    }

    void askLevel(Source source, int levelID, std::function<void(int)> callback);

    void flushWaiting(Cache &c)
    {
        auto waiting = std::move(c.waiting);
        c.waiting.clear();

        for (auto &[levelID, callback] : waiting)
        {
            auto const found = c.placements.find(levelID);
            callback(found == c.placements.end() ? 0 : found->second);
        }
    }

    int takePlacements(Placements &into, matjson::Value const &levels,
                       char const *idKey, char const *placementKey)
    {
        if (!levels.isArray())
            return 0;

        int found = 0;
        for (auto &level : levels)
        {
            if (!level.contains(idKey) || !level.contains(placementKey))
                continue;

            auto const id = level[idKey].asInt().unwrapOr(0);
            auto const placement = level[placementKey].asInt().unwrapOr(0);
            if (id > 0 && placement > 0)
            {
                into[static_cast<int>(id)] = static_cast<int>(placement);
                ++found;
            }
        }

        return found;
    }

    int parseDemonListOrg(Placements &into, matjson::Value const &body)
    {
        if (!body.contains("data") || !body["data"].contains("levels"))
            return 0;

        return takePlacements(into, body["data"]["levels"], "ingame_id", "placement");
    }

    int parsePointercrate(Placements &into, matjson::Value const &body)
    {
        return takePlacements(into, body, "level_id", "position");
    }

    int parseAREDL(Placements &into, matjson::Value const &body)
    {
        return takePlacements(into, body, "level_id", "position");
    }

    void listArrived(Source source, Placements placements)
    {
        auto &c = cache(source);

        c.placements = std::move(placements);
        c.complete = true;
        c.fetched = now();

        writeCacheFile(source);
        flushWaiting(c);
    }

    void listFailed(Source source)
    {
        auto &c = cache(source);
        if (!hasLevelEndpoint(source))
        {
            flushWaiting(c);
            return;
        }

        auto waiting = std::move(c.waiting);
        c.waiting.clear();

        for (auto &[levelID, callback] : waiting)
            askLevel(source, levelID, std::move(callback));
    }

    using Parser = int (*)(Placements &, matjson::Value const &);

    void fetchWholeList(Source source, std::string url, web::WebRequest bulk, Parser parse)
    {
        auto &c = cache(source);
        c.fetching = true;
        c.attempted = now();

        async::spawn(bulk.get(url), [source, parse](web::WebResponse response)
                     {
            auto &c = cache(source);
            c.fetching = false;

            if (!response.ok())
            {
                listFailed(source);
                return;
            }

            auto json = response.json();
            if (!json)
            {
                listFailed(source);
                return;
            }

            Placements placements;
            if (!parse(placements, json.unwrap()))
            {
                listFailed(source);
                return;
            }

            listArrived(source, std::move(placements)); });
    }

    void fetchPointercratePage(int page, std::shared_ptr<Placements> placements);

    void fetchPointercrate()
    {
        auto &c = cache(Source::Pointercrate);
        c.fetching = true;
        c.attempted = now();

        fetchPointercratePage(0, std::make_shared<Placements>());
    }

    void fetchPointercratePage(int page, std::shared_ptr<Placements> placements)
    {
        auto bulk = request(LIST_TIMEOUT);
        bulk.param("limit", POINTERCRATE_PAGE_SIZE);
        if (page > 0)
            bulk.param("after", page * POINTERCRATE_PAGE_SIZE);

        async::spawn(bulk.get(POINTERCRATE_LIST_URL), [page, placements](web::WebResponse response)
                     {
            auto &c = cache(Source::Pointercrate);

            if (!response.ok())
            {
                c.fetching = false;
                listFailed(Source::Pointercrate);
                return;
            }

            auto json = response.json();
            if (!json)
            {
                c.fetching = false;
                listFailed(Source::Pointercrate);
                return;
            }

            auto const found = parsePointercrate(*placements, json.unwrap());

            // A short page is the end of the list, whatever the page count says
            if (found >= POINTERCRATE_PAGE_SIZE && page + 1 < POINTERCRATE_PAGES)
            {
                fetchPointercratePage(page + 1, placements);
                return;
            }

            c.fetching = false;

            if (placements->empty())
            {
                listFailed(Source::Pointercrate);
                return;
            }

            listArrived(Source::Pointercrate, std::move(*placements)); });
    }

    void fetchList(Source source)
    {
        switch (source)
        {
        case Source::Pointercrate:
            fetchPointercrate();
            return;

        case Source::AREDL:
        {
            auto bulk = request(LIST_TIMEOUT);
            bulk.param("exclude_pending", "true");
            bulk.param("exclude_removed", "true");

            fetchWholeList(source, AREDL_LIST_URL, std::move(bulk), &parseAREDL);
            return;
        }

        case Source::DemonListOrg:
            fetchWholeList(source, DEMONLIST_LIST_URL, request(LIST_TIMEOUT), &parseDemonListOrg);
            return;

        default:
            return;
        }
    }

    void ensureFresh(Source source)
    {
        if (source == Source::Off)
            return;

        auto &c = cache(source);
        if (!c.read)
            readCacheFile(source);

        auto const moment = now();
        if (c.fetching || moment - c.fetched <= CACHE_LIFETIME || moment - c.attempted <= RETRY_DELAY)
            return;

        fetchList(source);
    }

    void lookupOne(Source source, int levelID)
    {
        auto single = request();

        auto url = std::string{AREDL_LEVEL_URL} + std::to_string(levelID);
        if (source == Source::DemonListOrg)
        {
            url = DEMONLIST_LEVEL_URL;
            single.param("ingame_id", levelID);
        }

        async::spawn(single.get(url), [source, levelID](web::WebResponse response)
                     {
            auto &c = cache(source);

            auto waiting = std::move(c.pending[levelID]);
            c.pending.erase(levelID);

            auto const code = response.code();
            if (code != 200 && code != 404)
                return;

            int placement = 0;
            if (code == 200)
            {
                auto json = response.json();
                if (!json)
                    return;

                auto body = json.unwrap();

                auto const &level = body.contains("data") ? body["data"] : body;
                auto const key = source == Source::AREDL ? "position" : "placement";

                if (!level.contains(key))
                    return;

                placement = static_cast<int>(level[key].asInt().unwrapOr(0));
            }

            c.placements[levelID] = placement;

            for (auto &callback : waiting)
                callback(placement); });
    }

    void askLevel(Source source, int levelID, std::function<void(int)> callback)
    {
        auto &waiting = cache(source).pending[levelID];
        waiting.push_back(std::move(callback));

        if (waiting.size() > 1)
            return;

        lookupOne(source, levelID);
    }
}

namespace faceit::demonlist
{
    Source source()
    {
        return currentSource();
    }

    int placementOf(int levelID)
    {
        auto const source = currentSource();
        if (source == Source::Off)
            return 0;

        ensureFresh(source);

        auto const &placements = cache(source).placements;
        auto const found = placements.find(levelID);

        return found == placements.end() ? 0 : found->second;
    }

    void lookup(int levelID, std::function<void(int placement)> callback)
    {
        auto const source = currentSource();
        if (source == Source::Off)
        {
            callback(0);
            return;
        }

        ensureFresh(source);

        auto &c = cache(source);
        if (auto const known = c.placements.find(levelID); known != c.placements.end())
        {
            callback(known->second);
            return;
        }

        if (c.complete)
        {
            callback(0);
            return;
        }

        if (c.fetching)
        {
            c.waiting.emplace_back(levelID, std::move(callback));
            return;
        }

        if (!hasLevelEndpoint(source))
        {
            callback(0);
            return;
        }

        askLevel(source, levelID, std::move(callback));
    }

    void prefetch()
    {
        ensureFresh(currentSource());
    }

    void sourceChanged()
    {
        auto const next = sourceFromSetting();
        if (next == currentSource())
            return;

        currentSource() = next;
        ensureFresh(next);
    }
}
