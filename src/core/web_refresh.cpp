#include "core/web_refresh.h"

#include "core/config_manager.h"

namespace WebRefresh
{
    void appendMetaTag(String& html)
    {
        const uint16_t seconds =
            ConfigManager::get().webRefreshSeconds;

        if (seconds == 0)
        {
            return;
        }

        html += "<meta http-equiv=\"refresh\" content=\"";
        html += String(seconds);
        html += "\">\n";
    }

    String getStatusText()
    {
        const uint16_t seconds =
            ConfigManager::get().webRefreshSeconds;

        if (seconds == 0)
        {
            return "Automatische Aktualisierung ist ausgeschaltet.";
        }

        String result = "Die Seite wird alle ";
        result += String(seconds);
        result += " Sekunden aktualisiert.";
        return result;
    }
}
