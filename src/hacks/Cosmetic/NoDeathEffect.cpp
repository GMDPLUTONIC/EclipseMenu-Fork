#include <modules/gui/gui.hpp>
#include <modules/hack/hack.hpp>
#include <modules/config/config.hpp>

#include <Geode/modify/PlayerObject.hpp>

namespace eclipse::hacks::Cosmetic {

    class NoDeathEffect : public hack::Hack {
        void init() override {
            auto tab = gui::MenuTab::find("Cosmetic");
            tab->addToggle("No Death Effect", "cosmetic.nodeatheffect")
                ->setDescription("Disables the player's death effect.")
                ->handleKeybinds();
        }

        [[nodiscard]] const char* getId() const override { return "No Death Effect"; }
    };

    REGISTER_HACK(NoDeathEffect)

    class $modify(NoDeathEffectPOHook, PlayerObject) {
        ALL_DELEGATES_AND_SAFE_PRIO("cosmetic.nodeatheffect")
        void playDeathEffect() {}
    };

}
