#include <modules/config/config.hpp>
#include <modules/gui/gui.hpp>
#include <modules/gui/components/toggle.hpp>
#include <modules/hack/hack.hpp>

#include <Geode/modify/CustomListView.hpp>
#include <Geode/modify/LevelCell.hpp>

namespace eclipse::hacks::Global {
    class $hack(CompactEditorLevels) {
        void init() override {
            auto tab = gui::MenuTab::find("tab.global");
            tab->addToggle("global.compacteditorlevels")->handleKeybinds()->setDescription();
        }

        [[nodiscard]] const char* getId() const override { return "Compact Editor Levels"; }
    };

    class $hack(CompactProfileComments) {
        void init() override {
            auto tab = gui::MenuTab::find("tab.global");
            tab->addToggle("global.compactprofilecomments")->handleKeybinds()->setDescription();
        }

        [[nodiscard]] const char* getId() const override { return "Compact Profile Comments"; }
    };

    REGISTER_HACK(CompactEditorLevels);
    REGISTER_HACK(CompactProfileComments);

    class $modify(CompactLevelsCommentsCLVHook, CustomListView) {
        /*
        original code by cvolton, re-used for "my levels" list
        and now reused for profile comments yayy
        proof of consent of code reuse: https://discord.com/channels/911701438269386882/911702535373475870/1220117410988953762
        and now it finds a new home in eclipsemenu
        -- raydeeux
        */
        static CustomListView* create(
            cocos2d::CCArray* a, TableViewCellDelegate* b, float c, float d, int e, BoomListType f, float g
        ) {
            if (f == BoomListType::Level2 && config::get<bool>("global.compacteditorlevels", false))
                f = BoomListType::Level4; // Level4 = compact level view
            else if (f == BoomListType::Comment4 && config::get<bool>("global.compactprofilecomments", false))
                f = BoomListType::Comment2; // Comment2 = compact comment view
            return CustomListView::create(a, b, c, d, e, f, g);
        }
    };

    class $modify(CompactLevelsCommentsLCHook, LevelCell) {
        void onClick(CCObject* sender) {
            // get the "view" button to work with compact mode in "my levels"
            if (this->m_level->m_levelType == GJLevelType::Editor && config::get<bool>("global.compacteditorlevels", false))
                utils::get<cocos2d::CCDirector>()->replaceScene(
                    cocos2d::CCTransitionFade::create(0.5f, EditLevelLayer::scene(m_level))
                );
            else
                LevelCell::onClick(sender);
        }

        void loadLocalLevelCell() {
            LevelCell::loadLocalLevelCell();

            if (config::get<bool>("global.compacteditorlevels", false) && m_mainLayer) {
                m_mainLayer->setPositionY(-3.5f);
                if (const auto localLevelName = m_mainLayer->getChildByType<cocos2d::CCLabelBMFont>(0))
                    if (std::string(localLevelName->getString()) == std::string(m_level->m_levelName))
                        localLevelName->limitLabelWidth(200.f, .6f, .01f);
            }
        }
    };
}
