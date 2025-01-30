#include <modules/config/config.hpp>
#include <modules/gui/gui.hpp>
#include <modules/gui/components/float-toggle.hpp>
#include <modules/hack/hack.hpp>

#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/LevelEditorLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>

#include <sinaps.hpp>

#ifndef GEODE_IS_ARM_MAC

constexpr float MIN_TPS = 0.f;
constexpr float MAX_TPS = 100000.f;

#ifdef GEODE_IS_MACOS
#define REQUIRE_PATCH
#endif

namespace eclipse::hacks::Global {
    class $hack(TPSBypass) {
    public:
        void init() override {
            config::setIfEmpty("global.tpsbypass", 240.f);

            #ifdef REQUIRE_PATCH
            auto res = setupPatches();
            if (!res) {
                geode::log::error("TPSBypass: {}", res.unwrapErr());
                return;
            }
            #endif

            auto tab = gui::MenuTab::find("tab.global");
            tab->addFloatToggle("global.tpsbypass", MIN_TPS, MAX_TPS, "%.2f TPS")
               ->setDescription()
               ->handleKeybinds();
        }

        #ifdef REQUIRE_PATCH
        geode::Result<> setupPatches() {
            auto base = reinterpret_cast<uint8_t*>(geode::base::get());
            auto moduleSize = utils::getBaseSize();
            geode::log::info("TPSBypass: base = 0x{:X}", (uintptr_t)base);
            geode::log::info("TPSBypass: moduleSize = 0x{:X}", moduleSize);

            using namespace sinaps::mask;

            #ifdef GEODE_IS_INTEL_MAC
            // on x86, we need to replace two values that are used in GJBaseGameLayer::getModifiedDelta
            // pretty simple, just find original values and replace them with raw bytes
            auto floatAddr = sinaps::find<dword<0x3B888889>>(base, moduleSize);
            auto doubleAddr = sinaps::find<qword<0x3F71111120000000>>(base, moduleSize);

            if (floatAddr == -1 || doubleAddr == -1) {
                return geode::Err(fmt::format("failed to find addresses: float = 0x{:X}, double = 0x{:X}", floatAddr, doubleAddr));
            }

            auto patch1Res = geode::Mod::get()->patch(
                reinterpret_cast<void*>(floatAddr),
                geode::toBytes(config::get<"global.tpsbypass", float>(240.f))
            );
            if (!patch1Res) return geode::Err(fmt::format("failed to patch float address: {}", patch1Res.unwrapErr()));
            auto patch1 = patch1Res.unwrap();
            (void) patch1->disable();

            auto patch2Res = geode::Mod::get()->patch(
                reinterpret_cast<void*>(doubleAddr),
                geode::toBytes<double>(config::get<"global.tpsbypass", float>(240.f))
            );
            if (!patch2Res) return geode::Err(fmt::format("failed to patch double address: {}", patch2Res.unwrapErr()));
            auto patch2 = patch2Res.unwrap();
            (void) patch2->disable();

            const auto setState = [patch1, patch2](bool state) {
                if (state) {
                    (void) patch1->enable();
                    (void) patch2->enable();
                } else {
                    (void) patch1->disable();
                    (void) patch2->disable();
                }
            };

            const auto setValue = [patch1, patch2](float value) {
                (void) patch1->updateBytes(geode::toBytes(value));
                (void) patch2->updateBytes(geode::toBytes<double>(value));
            };
            #elif defined(GEODE_IS_ARM_MAC)
            // on ARM, it's a bit tricky, since the value is not stored as a constant.
            //
            // assembly in question:
            // ; Load 0.0041667f
            // 29 11 91 52        movz w9, #0x8889
            // 09 71 A7 72        movk w9, #0x3b88, lsl #16
            // ...
            // ; Load 0.00416666688
            // 09 00 A4 D2        movz x9, #0x2000, lsl #16
            // 29 22 C2 F2        movk x9, #0x1111, lsl #32
            // 29 EE E7 F2        movk x9, #0x3f71, lsl #48

            // TODO: implement ARM patching
            #endif

            // update bytes on value change
            config::addDelegate("global.tpsbypass", [setValue] {
                setValue(config::get<"global.tpsbypass", float>(240.f));
            });

            // update patches on enable/disable
            config::addDelegate("global.tpsbypass.toggle", [setState] {
                setState(config::get<"global.tpsbypass.toggle", bool>());
            });

            // apply initial state
            setState(config::get<"global.tpsbypass.toggle", bool>());
        }
        #endif

        [[nodiscard]] const char* getId() const override { return "Physics Bypass"; }
        [[nodiscard]] int32_t getPriority() const override { return -14; }

        [[nodiscard]] bool isCheating() const override {
            auto toggle = config::get<"global.tpsbypass.toggle", bool>();
            auto tps = config::get<"global.tpsbypass", float>(240.f);
            return toggle && tps != 240.f;
        }
    };

    REGISTER_HACK(TPSBypass)

    class $modify(TPSBypassGJBGLHook, GJBaseGameLayer) {
        ALL_DELEGATES_AND_SAFE_PRIO("global.tpsbypass.toggle")

        struct Fields {
            double m_extraDelta = 0.0;
            float m_realDelta = 0.0;
            bool m_shouldHide = false;
            bool m_isEditor = utils::get<LevelEditorLayer>() != nullptr;
            bool m_shouldBreak = false;
        };

        float getCustomDelta(float dt, float tps, bool applyExtraDelta = true) {
            auto spt = 1.f / tps;

            if (applyExtraDelta && m_resumeTimer > 0) {
                --m_resumeTimer;
                dt = 0.f;
            }

            auto totalDelta = dt + m_extraDelta;
            auto timestep = std::min(m_gameState.m_timeWarp, 1.f) * spt;
            auto steps = std::round(totalDelta / timestep);
            auto newDelta = steps * timestep;
            if (applyExtraDelta) m_extraDelta = totalDelta - newDelta;
            return static_cast<float>(newDelta);
        }

        #ifndef REQUIRE_PATCH
        float getModifiedDelta(float dt) {
            return getCustomDelta(dt, config::get<"global.tpsbypass", float>(240.f));
        }
        #endif

        bool shouldContinue(const Fields* fields) const {
            if (!fields->m_isEditor) return true;

            // in editor, player hitbox is removed from section when it dies,
            // so we need to check if it's still there
            return !fields->m_shouldBreak;
        }

        void update(float dt) override {
            auto fields = m_fields.self();
            fields->m_extraDelta += dt;
            fields->m_shouldBreak = false;

            // store current frame delta for later use in updateVisibility
            fields->m_realDelta = getCustomDelta(dt, 240.f, false);

            auto newTPS = config::get<"global.tpsbypass", float>(240.f);
            auto newDelta = 1.0 / newTPS;

            if (fields->m_extraDelta >= newDelta) {
                // call original update several times, until the extra delta is less than the new delta
                size_t steps = fields->m_extraDelta / newDelta;
                fields->m_extraDelta -= steps * newDelta;

                auto start = utils::getTimestamp();
                auto ms = dt * 1000;
                fields->m_shouldHide = true;
                while (steps > 1 && shouldContinue(fields)) {
                    GJBaseGameLayer::update(newDelta);
                    auto end = utils::getTimestamp();
                    // if the update took too long, break out of the loop
                    if (end - start > ms) break;
                    --steps;
                }
                fields->m_shouldHide = false;

                if (shouldContinue(fields)) {
                    // call one last time with the remaining delta
                    GJBaseGameLayer::update(newDelta * steps);
                }
            }
        }
    };

    inline TPSBypassGJBGLHook::Fields* getFields(GJBaseGameLayer* self) {
        return reinterpret_cast<TPSBypassGJBGLHook*>(self)->m_fields.self();
    }

    // Skip some functions to make the game run faster during extra updates period

    class $modify(TPSBypassPLHook, PlayLayer) {
        ALL_DELEGATES_AND_SAFE_PRIO("global.tpsbypass.toggle")

        // PlayLayer postUpdate handles practice mode checkpoints, labels and also calls updateVisibility
        void postUpdate(float dt) override {
            auto fields = getFields(this);
            if (fields->m_shouldHide) return;
            PlayLayer::postUpdate(fields->m_realDelta);
        }

        // we also would like to fix the percentage calculation, which uses constant 240 TPS to determine the progress
        int calculationFix() {
            auto timestamp = m_level->m_timestamp;
            auto currentProgress = m_gameState.m_currentProgress;
            // this is only an issue for 2.2+ levels (with TPS greater than 240)
            if (timestamp > 0 && config::get<"global.tpsbypass", float>(240.f) > 240.f) {
                // recalculate m_currentProgress based on the actual time passed
                auto progress = utils::getActualProgress(this);
                m_gameState.m_currentProgress = timestamp * progress / 100.f;
            }
            return currentProgress;
        }

        void updateProgressbar() {
            auto currentProgress = calculationFix();
            PlayLayer::updateProgressbar();
            m_gameState.m_currentProgress = currentProgress;
        }

        void destroyPlayer(PlayerObject* player, GameObject* object) override {
            auto currentProgress = calculationFix();
            PlayLayer::destroyPlayer(player, object);
            m_gameState.m_currentProgress = currentProgress;
        }

        void levelComplete() {
            // levelComplete uses m_gameState.m_unkUint2 to store the timestamp
            // also we can't rely on m_level->m_timestamp, because it might not be updated yet
            auto oldTimestamp = m_gameState.m_unkUint2;
            if (config::get<"global.tpsbypass", float>(240.f) > 240.f) {
                auto ticks = static_cast<uint32_t>(std::round(m_gameState.m_levelTime * 240));
                m_gameState.m_unkUint2 = ticks;
            }
            PlayLayer::levelComplete();
            m_gameState.m_unkUint2 = oldTimestamp;
        }
    };

    class $modify(TPSBypassLELHook, LevelEditorLayer) {
        ALL_DELEGATES_AND_SAFE_PRIO("global.tpsbypass.toggle")

        // Editor postUpdate handles the exit of playback mode and player trail drawing and calls updateVisibility
        void postUpdate(float dt) override {
            // editor disables playback mode in postUpdate, so we should call the function if we're dead
            auto fields = getFields(this);
            if (fields->m_shouldHide && !m_player1->m_maybeIsColliding && !m_player2->m_maybeIsColliding) return;
            // m_maybeIsColliding will be reset in our inner update call next iteration,
            // so we need to store it here to check if we should break out of the loop
            fields->m_shouldBreak = m_player1->m_maybeIsColliding;
            LevelEditorLayer::postUpdate(fields->m_realDelta);
        }
    };
}
#endif
