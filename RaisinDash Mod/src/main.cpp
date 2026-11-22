#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/ProfilePage.hpp>
#include <Geode/utils/web.hpp>
#include <Geode/loader/Event.hpp>

using namespace geode::prelude;

static bool g_checkedOnBoot = false;

void setEclipseEnabled(bool allow) {
    auto eclipse = Loader::get()->getLoadedMod("eclipse.eclipse-menu");
    if (!eclipse) return;

    if (allow) {
        if (!eclipse->isEnabled()) {
            (void)eclipse->enable();
            log::info("Moderator verified: Eclipse Menu enabled.");
        }
    } else {
        if (eclipse->isEnabled()) {
            (void)eclipse->disable();
            log::info("Non-moderator account: Eclipse Menu disabled.");
        }
    }
}

// Parse the ":49:<modBadge>:" field from Geometry Dash's getGJUserInfo20 response
int parseModBadgeFromResponse(std::string const& body) {
    // GD server responses are key:value pairs separated by ':'
    std::vector<std::string> parts;
    std::stringstream ss(body);
    std::string item;
    while (std::getline(ss, item, ':')) {
        parts.push_back(item);
    }

    for (size_t i = 0; i + 1 < parts.size(); i += 2) {
        if (parts[i] == "49") {
            return numFromString<int>(parts[i + 1]).unwrapOr(0);
        }
    }
    return 0;
}

void checkModeratorStatusFromServer() {
    auto accManager = GJAccountManager::sharedState();
    if (!accManager || accManager->m_accountID <= 0) {
        // Not logged in yet -> keep Eclipse disabled
        setEclipseEnabled(false);
        return;
    }

    // Replace this URL with your exact Glowhost database URL!
    std::string serverUrl = "https://glowhost.us/lvruowey";

    web::WebRequest req;
    req.bodyString(fmt::format(
        "targetAccountID={}&secret=Wmfd2893gb7",
        accManager->m_accountID
    ));
    req.header("Content-Type", "application/x-www-form-urlencoded");

    req.post(serverUrl).listen([](web::WebResponse* res) {
        if (res && res->ok()) {
            std::string text = res->string().unwrapOr("");
            int badge = parseModBadgeFromResponse(text);
            setEclipseEnabled(badge > 0);
        } else {
            // Fail closed if server check fails
            setEclipseEnabled(false);
        }
    });
}

class $modify(BootCheckMenuLayer, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;

        if (!g_checkedOnBoot) {
            g_checkedOnBoot = true;
            // Start disabled until server confirms mod status
            setEclipseEnabled(false);
            checkModeratorStatusFromServer();
        }
        return true;
    }
};

class $modify(ModCheckProfilePage, ProfilePage) {
    void loadPageFromUserInfo(GJUserScore* score) {
        ProfilePage::loadPageFromUserInfo(score);

        auto accManager = GJAccountManager::sharedState();
        if (score && accManager && score->m_accountID == accManager->m_accountID) {
            setEclipseEnabled(score->m_modBadge > 0);
        }
    }
};
#include <Geode/modify/LevelSelectLayer.hpp>

void hideNonModButtons(CCNode* layer, bool isMod) {
    if (!layer || isMod) return;

    // List every button Node ID you want hidden from regular players
    std::vector<std::string> hiddenIDs = {
        "geode.loader/geode-button",
        "km7dev.gdps-switcher/gdps-switcher-button",
        "user95401.main-levels-editor/edit-button"
    };

    for (auto const& id : hiddenIDs) {
        if (auto btn = layer->getChildByIDRecursive(id)) {
            btn->setVisible(false);
        }
    }
}
