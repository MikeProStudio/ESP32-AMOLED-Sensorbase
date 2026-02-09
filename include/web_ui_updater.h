#ifndef WEB_UI_UPDATER_H
#define WEB_UI_UPDATER_H

#include <Arduino.h>

/**
 * @brief Service zur Aktualisierung der Web-UI von GitHub
 */
class WebUIUpdater {
public:
    static void checkForUpdates();
    static bool performUpdate(const String& url);
    static String getLocalVersion();
    
private:
    static const char* GITHUB_VERSION_URL;
    static const char* GITHUB_ASSET_URL;
};

#endif // WEB_UI_UPDATER_H
