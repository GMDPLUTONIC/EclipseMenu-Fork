#include <modules/gui/gui.hpp>
#include <modules/hack/hack.hpp>
#include <modules/config/config.hpp>

#include <Geode/modify/GJBaseGameLayer.hpp>

namespace eclipse::hacks::Cosmetic {

    void onHideParticles(bool state) {
        if (!state) return;

        bool customParticlesEnabled = !config::get<bool>("cosmetic.noparticles.nocustomparticles", false);
        bool miscParticlesEnabled = !config::get<bool>("cosmetic.noparticles.nomiscparticles", false);

        if (customParticlesEnabled && miscParticlesEnabled)
            config::set("cosmetic.noparticles", false);

        auto* gjbgl = GJBaseGameLayer::get();

        if (!gjbgl) return;

        for (const auto& [name, array] : geode::cocos::CCDictionaryExt<gd::string, cocos2d::CCArray*>{ gjbgl->m_particlesDict }) {
            for (const auto& particle : geode::cocos::CCArrayExt<cocos2d::CCParticleSystemQuad*>{ array }) {
                particle->setVisible(miscParticlesEnabled);
            }
        }

        for (const auto& [name, array] : geode::cocos::CCDictionaryExt<gd::string, cocos2d::CCArray*>{ gjbgl->m_claimedParticles }) {
            for (const auto& particle : geode::cocos::CCArrayExt<cocos2d::CCParticleSystemQuad*>{ array }) {
                particle->setVisible(customParticlesEnabled);
            }
        }

        for (const auto& particle : geode::cocos::CCArrayExt<cocos2d::CCParticleSystemQuad*>{ gjbgl->m_unclaimedParticles }) {
            particle->setVisible(customParticlesEnabled);
        }

        for (const auto& [name, array] : geode::cocos::CCDictionaryExt<gd::string, cocos2d::CCArray*>{ gjbgl->m_customParticles }) {
            for (const auto& particle : geode::cocos::CCArrayExt<cocos2d::CCParticleSystemQuad*>{ array }) {
                particle->setVisible(customParticlesEnabled);
            }
        }
    }

    class NoParticles : public hack::Hack {
        void init() override {
            auto tab = gui::MenuTab::find("Level");

            config::setIfEmpty("cosmetic.noparticles", false);
            config::setIfEmpty("cosmetic.noparticles.nomiscparticles", true);
            config::setIfEmpty("cosmetic.noparticles.nocustomparticles", false);

            tab->addToggle("No Particles", "cosmetic.noparticles")
                ->handleKeybinds()
                ->setDescription("Hides portal, coin, custom, etc particles in levels.")
                ->callback(onHideParticles)
                ->addOptions([](std::shared_ptr<gui::MenuTab> options) {
                    options->addToggle("No Misc. Particles", "cosmetic.noparticles.nomiscparticles")
                        ->setDescription("Includes portal, dash orb, coin, end wall particles, etc...");
                    options->addToggle("No Custom Particles", "cosmetic.noparticles.nocustomparticles")
                        ->setDescription("Includes particles created by the level author.");
                });
        }

        void update() override {
            onHideParticles(config::get<bool>("cosmetic.noparticles", false));
        }

        [[nodiscard]] const char* getId() const override { return "No Particles"; }
    };

    REGISTER_HACK(NoParticles)

    class $modify(NoParticlesBGLHook, GJBaseGameLayer) {
        ENABLE_SAFE_HOOKS_ALL()

        cocos2d::CCParticleSystemQuad* spawnParticle(char const* plist, int zOrder, cocos2d::tCCPositionType positionType, cocos2d::CCPoint position) {
            if (config::get<bool>("cosmetic.noparticles", false) && config::get<bool>("cosmetic.noparticles.nomiscparticles", false))
                return nullptr;

            return GJBaseGameLayer::spawnParticle(plist, zOrder, positionType, position);
        }
    };
}
