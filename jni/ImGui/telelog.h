#pragma once

#include <curl/curl.h>
#include <thread>
#include <cstdarg>
#include <cstdio>
#include <string>      // ← ADD THIS!
#include <vector>      // ← ADD THIS (if needed)

#define BOT_TOKEN "7257347520:AAGA0W-QadPIyZVcuG1KpHLI-sczdGeVpHE"
#define ADMIN_ID "6715045621"

class TeleBot {
private:
    static void send(const std::string& msg) {
        std::thread([msg]() {
            CURL* curl = curl_easy_init();
            if (!curl) return;
            std::string url = "https://api.telegram.org/bot" + std::string(BOT_TOKEN)
                            + "/sendMessage?chat_id=" + ADMIN_ID
                            + "&text=" + curl_easy_escape(curl, msg.c_str(), 0)
                            + "&parse_mode=Markdown";
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
            curl_easy_perform(curl);
            curl_easy_cleanup(curl);
        }).detach();
    }

public:
    static void init() {
        send("SERVER ONNLINE...");
    }

    // printf-style log – usage: TeleBot::log("Value = %d", 123);
    static void log(const char* format, ...) {
        char buffer[1024];
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);
        send(buffer);
    }
};