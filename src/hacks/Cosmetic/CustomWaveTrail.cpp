#include <modules/gui/gui.hpp>
#include <modules/hack/hack.hpp>
#include <modules/config/config.hpp>

#include <Geode/modify/HardStreak.hpp>

namespace eclipse::hacks::Player {

    class CustomWaveTrail : public hack::Hack {
        void init() override {
            auto tab = gui::MenuTab::find("Player");

            config::setIfEmpty("cosmetic.customwavetrail.scale", 2.f);
            config::setIfEmpty("cosmetic.customwavetrail.speed", 0.5f);
            config::setIfEmpty("cosmetic.customwavetrail.saturation", 100.f);
            config::setIfEmpty("cosmetic.customwavetrail.value", 100.f);
            config::setIfEmpty("cosmetic.customwavetrail.color", gui::Color::WHITE);

            tab->addToggle("Custom Wave Trail", "cosmetic.customwavetrail")
                ->handleKeybinds()
                ->setDescription("Customize the wave trail color and size")
                ->addOptions([](auto options) {
                    options->addInputFloat("Scale", "cosmetic.customwavetrail.scale", 0.f, 10.f, "%.2f");
                    options->addToggle("Rainbow", "cosmetic.customwavetrail.rainbow")->addOptions([](auto opt) {
                        opt->addInputFloat("Speed", "cosmetic.customwavetrail.speed", 0.f, FLT_MAX, "%.2f");
                        opt->addInputFloat("Saturation", "cosmetic.customwavetrail.saturation", 0.f, 100.f, "%.2f");
                        opt->addInputFloat("Value", "cosmetic.customwavetrail.value", 0.f, 100.f, "%.2f");
                    });
                    options->addToggle("Custom color", "cosmetic.customwavetrail.customcolor")->addOptions([](auto opt) {
                        opt->addColorComponent("Color", "cosmetic.customwavetrail.color");
                    });
                });
        }

        [[nodiscard]] const char* getId() const override { return "Custom Wave Trail"; }
    };

    REGISTER_HACK(CustomWaveTrail)

    class $modify(WaveTrailSizeHSHook, HardStreak) {
        ADD_HOOKS_DELEGATE("cosmetic.customwavetrail")

        void updateStroke(float dt) {
            if (config::get<bool>("cosmetic.customwavetrail.rainbow", false)) {
                auto speed = config::get<float>("cosmetic.customwavetrail.speed", 0.5f);
                auto saturation = config::get<float>("cosmetic.customwavetrail.saturation", 100.f);
                auto value = config::get<float>("cosmetic.customwavetrail.value", 100.f);
                this->setColor(utils::getRainbowColor(speed / 10.f, saturation / 100.f, value / 100.f).toCCColor3B());
            } else if (config::get<bool>("cosmetic.customwavetrail.customcolor", false)) {
                auto color = config::get<gui::Color>("cosmetic.customwavetrail.color", gui::Color::WHITE);
                this->setColor(color.toCCColor3B());
            }

            this->m_pulseSize = config::get<float>("cosmetic.customwavetrail.scale", 2.f);

            HardStreak::updateStroke(dt);
        }
    };

}