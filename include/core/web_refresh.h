#pragma once

#include <Arduino.h>

namespace WebRefresh
{
    void appendMetaTag(String& html);
    String getStatusText();
}
