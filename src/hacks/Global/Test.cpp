  #include <modules/gui/gui.hpp>
#include <modules/hack/hack.hpp>
#include <modules/config/config.hpp>

#include <Geode/modify/GJBaseGameLayer.hpp>


class Noclip : public hack::Hack {
        void init() override {
            auto tab = gui::MenuTab::find("Global");
            tab->addToggle("Test Mod", "global.testmod")->setDescription("Test Mod")->handleKeybinds();
        }

        [[nodiscard]] bool isCheating() override { return config::get<bool>("global.testmod", false); }
        [[nodiscard]] const char* getId() const override { return "Test Mod"; }
    };
