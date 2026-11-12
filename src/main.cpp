#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/modify/PlayerObject.hpp>
#include <Geode/binding/PlayLayer.hpp>
#include <Geode/binding/PauseLayer.hpp>
#include <Geode/binding/PlayerObject.hpp>
#include <Geode/binding/GJGameLevel.hpp>
#include <Geode/binding/ButtonSprite.hpp>
#include <Geode/binding/CCMenuItemSpriteExtra.hpp>

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

using namespace geode::prelude;

namespace geomind {
    struct InputEvent {
        uint64_t frame = 0;
        uint8_t button = 1;
        bool down = false;
    };

    struct Route {
        uint32_t levelID = 0;
        std::string levelName;
        std::vector<InputEvent> events;
    };

    static Route g_route;
    static uint64_t g_frame = 0;
    static size_t g_playIndex = 0;
    static bool g_recording = false;
    static bool g_playing = false;
    static float g_bestPercent = 0.f;

    static std::filesystem::path routePath() {
        return Mod::get()->getSaveDir() / "last-route.gma";
    }

    static void resetAttemptState() {
        g_frame = 0;
        g_playIndex = 0;
        g_bestPercent = 0.f;
    }

    static void beginRecording() {
        auto pl = PlayLayer::get();
        if (!pl || !pl->m_level) {
            Notification::create("Enter a level before recording", NotificationIcon::Error)->show();
            return;
        }
        g_route = {};
        g_route.levelID = static_cast<uint32_t>(pl->m_level->m_levelID);
        g_route.levelName = pl->m_level->m_levelName;
        resetAttemptState();
        g_playing = false;
        g_recording = true;
        Notification::create("GeoMind: recording inputs", NotificationIcon::Info)->show();
        log::info("GeoMind: recording started for {}", g_route.levelName);
    }

    static bool saveRoute() {
        if (g_route.events.empty()) return false;
        try {
            auto path = routePath();
            std::filesystem::create_directories(path.parent_path());
            matjson::Value root = matjson::Value::object();
            root["format"] = "GeoMindRoute";
            root["version"] = 1;
            root["levelID"] = static_cast<int64_t>(g_route.levelID);
            root["levelName"] = g_route.levelName;
            root["frameUnit"] = "PlayLayer update";
            matjson::Value events = matjson::Value::array();
            for (const auto& event : g_route.events) {
                matjson::Value item = matjson::Value::object();
                item["frame"] = static_cast<int64_t>(event.frame);
                item["button"] = static_cast<int>(event.button);
                item["down"] = event.down;
                events.push(item);
            }
            root["events"] = events;
            std::ofstream file(path, std::ios::binary);
            if (!file) return false;
            file << root.dump(2);
            log::info("GeoMind: saved {} input events to {}", g_route.events.size(), path.string());
            return true;
        } catch (const std::exception& e) {
            log::error("GeoMind: save failed: {}", e.what());
            return false;
        }
    }

    static bool loadRoute() {
        try {
            std::ifstream file(routePath(), std::ios::binary);
            if (!file) return false;
            std::string source((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            auto parsed = matjson::parse(source);
            if (!parsed) return false;
            auto root = parsed.unwrap();
            if (root["format"].asString().unwrapOr("") != "GeoMindRoute") return false;
            if (root["version"].asInt().unwrapOr(0) != 1) return false;

            Route loaded;
            loaded.levelID = static_cast<uint32_t>(root["levelID"].asInt().unwrapOr(0));
            loaded.levelName = root["levelName"].asString().unwrapOr("Unknown level");
            if (!root["events"].isArray()) return false;

            for (const auto& item : root["events"]) {
                InputEvent event;
                event.frame = static_cast<uint64_t>(std::max<int64_t>(0, item["frame"].asInt().unwrapOr(0)));
                event.button = static_cast<uint8_t>(std::clamp<int64_t>(item["button"].asInt().unwrapOr(1), 0, 255));
                event.down = item["down"].asBool().unwrapOr(false);
                loaded.events.push_back(event);
            }
            std::stable_sort(loaded.events.begin(), loaded.events.end(),
                [](const InputEvent& a, const InputEvent& b) { return a.frame < b.frame; });
            g_route = std::move(loaded);
            return !g_route.events.empty();
        } catch (const std::exception& e) {
            log::error("GeoMind: load failed: {}", e.what());
            return false;
        }
    }

    static void stopPlayback() {
        if (!g_playing) return;
        g_playing = false;
        g_playIndex = 0;
        if (auto pl = PlayLayer::get(); pl && pl->m_player1) {
            pl->m_player1->releaseAllButtons();
        }
        Notification::create("GeoMind: replay stopped", NotificationIcon::Info)->show();
    }

    static void startPlayback() {
        auto pl = PlayLayer::get();
        if (!pl || !pl->m_level) {
            Notification::create("Enter a level before replaying", NotificationIcon::Error)->show();
            return;
        }
        if (!loadRoute()) {
            Notification::create("No saved GeoMind route found", NotificationIcon::Error)->show();
            return;
        }
        if (g_route.levelID != static_cast<uint32_t>(pl->m_level->m_levelID)) {
            Notification::create("Saved route belongs to a different level", NotificationIcon::Error)->show();
            return;
        }
        if (g_route.events.empty()) return;

        g_recording = false;
        resetAttemptState();
        g_playing = true;
        Notification::create("GeoMind: replay armed — resume to play", NotificationIcon::Success)->show();
        log::info("GeoMind: replay armed with {} events", g_route.events.size());
    }

    static void recordEvent(PlayerObject* player, PlayerButton button, bool down) {
        if (!g_recording || g_playing) return;
        auto pl = PlayLayer::get();
        if (!pl || player != pl->m_player1) return;
        g_route.events.push_back({
            g_frame,
            static_cast<uint8_t>(button),
            down
        });
    }

    static void applyReplayFrame(PlayLayer* layer) {
        if (!g_playing || !layer || !layer->m_player1) return;
        while (g_playIndex < g_route.events.size() &&
               g_route.events[g_playIndex].frame <= g_frame) {
            const auto event = g_route.events[g_playIndex++];
            auto button = static_cast<PlayerButton>(event.button);
            if (event.down) layer->m_player1->pushButton(button);
            else layer->m_player1->releaseButton(button);
        }
        if (g_playIndex >= g_route.events.size()) {
            g_playing = false;
            layer->m_player1->releaseAllButtons();
            Notification::create("GeoMind: replay finished", NotificationIcon::Success)->show();
        }
    }
}

// Input hooks: capture the actual game input calls and replay them through the
// same PlayerObject interface. Playback events are ignored by the recorder.
class $modify(GeoMindPlayer, PlayerObject) {
    bool pushButton(PlayerButton button) {
        bool result = PlayerObject::pushButton(button);
        geomind::recordEvent(this, button, true);
        return result;
    }

    bool releaseButton(PlayerButton button) {
        bool result = PlayerObject::releaseButton(button);
        geomind::recordEvent(this, button, false);
        return result;
    }
};

class $modify(GeoMindPlayLayer, PlayLayer) {
    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        if (!PlayLayer::init(level, useReplay, dontCreateObjects)) return false;
        geomind::g_recording = false;
        geomind::g_playing = false;
        geomind::resetAttemptState();
        if (level) {
            log::info("GeoMind: loaded level '{}'", level->m_levelName);
        }
        return true;
    }

    void update(float dt) {
        PlayLayer::update(dt);
        if (geomind::g_recording && m_level) {
            geomind::g_bestPercent = std::max(geomind::g_bestPercent, getCurrentPercent());
        }
        geomind::applyReplayFrame(this);
        ++geomind::g_frame;
    }

    void destroyPlayer(PlayerObject* player, GameObject* object) {
        PlayLayer::destroyPlayer(player, object);
        if (geomind::g_playing) {
            geomind::g_playing = false;
            geomind::g_playIndex = 0;
            log::info("GeoMind: replay died at {:.2f}%", getCurrentPercent());
        }
    }

    void onQuit() {
        if (geomind::g_recording) {
            geomind::g_recording = false;
            if (geomind::saveRoute()) {
                Notification::create("GeoMind route auto-saved", NotificationIcon::Success)->show();
            }
        }
        geomind::stopPlayback();
        PlayLayer::onQuit();
    }
};

// Pause-menu controls keep the workflow usable without external scripts.
class $modify(GeoMindPauseLayer, PauseLayer) {
    void customSetup() {
        PauseLayer::customSetup();

        auto size = CCDirector::sharedDirector()->getWinSize();
        auto menu = CCMenu::create();
        menu->setPosition({ size.width - 105.f, size.height - 42.f });

        auto addButton = [&](const char* title, const char* sprite, float y,
                             SEL_MenuHandler handler) {
            auto visual = ButtonSprite::create(title, "bigFont.fnt", sprite, 0.65f);
            visual->setScale(0.42f);
            auto button = CCMenuItemSpriteExtra::create(visual, this, handler);
            button->setPosition({ 0.f, y });
            menu->addChild(button);
        };

        addButton(geomind::g_recording ? "Stop + Save" : "Record Route",
                  geomind::g_recording ? "GJ_button_06.png" : "GJ_button_01.png",
                  0.f, menu_selector(GeoMindPauseLayer::onRecord));
        addButton("Replay Route", "GJ_button_04.png", -34.f,
                  menu_selector(GeoMindPauseLayer::onReplay));
        addButton("Stop Replay", "GJ_button_06.png", -68.f,
                  menu_selector(GeoMindPauseLayer::onStopReplay));

        auto statusText = fmt::format("Events: {} | Best: {:.1f}%",
            geomind::g_route.events.size(), geomind::g_bestPercent);
        auto status = CCLabelBMFont::create(statusText.c_str(), "chatFont.fnt");
        status->setScale(0.42f);
        status->setAnchorPoint({ 1.f, 0.5f });
        status->setPosition({ -3.f, -98.f });
        menu->addChild(status);

        this->addChild(menu, 50);
    }

    void onRecord(CCObject*) {
        if (geomind::g_recording) {
            geomind::g_recording = false;
            if (geomind::saveRoute()) {
                Notification::create("GeoMind route saved", NotificationIcon::Success)->show();
            } else {
                Notification::create("Route save failed", NotificationIcon::Error)->show();
            }
        } else {
            geomind::beginRecording();
        }
    }

    void onReplay(CCObject*) {
        geomind::startPlayback();
    }

    void onStopReplay(CCObject*) {
        geomind::stopPlayback();
    }
};

class $modify(GeoMindMenuLayer, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;
        auto menu = this->getChildByID("bottom-menu");
        if (!menu) return true;

        auto sprite = ButtonSprite::create("GeoMind AI", "goldFont.fnt", "GJ_button_01.png", 0.75f);
        sprite->setScale(0.55f);
        auto button = CCMenuItemSpriteExtra::create(sprite, this,
            menu_selector(GeoMindMenuLayer::onOpen));
        button->setID("geomind-open-button");
        menu->addChild(button);
        menu->updateLayout();
        return true;
    }

    void onOpen(CCObject*) {
        FLAlertLayer::create(
            "GeoMind AI",
            "Classic Mode Route Lab<cr><cr>"
            "This build adds route input recording and replay.<cr>"
            "Open a level, pause, then use the GeoMind controls.<cr><cr>"
            "Autonomous search and XDBot export are still in development.",
            "OK"
        )->show();
    }
};

$on_mod(Loaded) {
    log::info("GeoMind AI loaded — classic mode route capture/replay prototype.");
}
