#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>

using namespace geode::prelude;

class $modify(GeoMindMenuLayer, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) {
            return false;
        }

        auto menu = this->getChildByID("bottom-menu");
        if (!menu) {
            log::warn("GeoMind AI: bottom menu was not found");
            return true;
        }

        auto buttonSprite = ButtonSprite::create("GeoMind", "goldFont.fnt", "GJ_button_01.png", 0.8f);
        auto button = CCMenuItemSpriteExtra::create(
            buttonSprite,
            this,
            menu_selector(GeoMindMenuLayer::onGeoMind)
        );
        button->setID("geomind-open-button");
        menu->addChild(button);
        menu->updateLayout();

        return true;
    }

    void onGeoMind(CCObject*) {
        FLAlertLayer::create(
            "GeoMind AI",
            "Classic mode only.<cr><cy>Prototype scaffold</c><cr><cr>"
            "The in-game interface is installed, but automated route search "
            "and XDBot macro export are not implemented yet.",
            "OK"
        )->show();
    }
};

$on_mod(Loaded) {
    log::info("GeoMind AI loaded.");
}
