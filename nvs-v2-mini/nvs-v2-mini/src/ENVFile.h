#ifndef ENV_FILE_H
#define ENV_FILE_H

#include <Arduino.h>
#include <LittleFS.h>
#include <map>

class ENVFile {
private:
    std::map<String, String> _env;
    bool _loaded = false;
    
    void _load() {
        _loaded = true;
        File file = LittleFS.open(".env", "r");
        if (!file) {
            return;
        }
        while (file.available()) {
            String d = file.readStringUntil('\n');
            if (d.length() == 0) continue;
            int pos = d.indexOf('=');
            if (pos == -1) continue;
            String key = d.substring(0, pos);
            String value = d.substring(pos + 1);
            _env[key] = value;
        }
        file.close();
    }

    bool _has(String key) {
        if (!_loaded) _load();
        return _env.count(key) > 0;
    }

public:
    ENVFile() = default;

    void begin() {
#ifdef ESP32
        LittleFS.begin(true);
#else
        LittleFS.begin();
#endif
        _load();
    }

    String getString(String key, String defaultValue = "") {
        if (!_has(key)) return defaultValue;
        return _env[key];
    }

    long getInt(String key, long defaultValue = 0) {
        if (!_has(key)) return defaultValue;
        return _env[key].toInt();
    }

    float getFloat(String key) {
        if (!_has(key)) return 0;
        return _env[key].toFloat();
    }

    double getDouble(String key) {
        if (!_has(key)) return 0;
        return _env[key].toDouble();
    }

    void set(String key, String value) {
        _env[key] = value;
        save();
    }

    void set(String key, long value) {
        _env[key] = String(value);
        save();
    }

    void set(String key, float value) {
        _env[key] = String(value);
        save();
    }

    void set(String key, double value) {
        _env[key] = String(value);
        save();
    }

    void save() {
        File file = LittleFS.open(".env", "w");
        if (!file) {
            return;
        }
        for (auto &e : _env) {
            file.printf("%s=%s\n", e.first.c_str(), e.second.c_str());
        }
        file.close();
    }
};

#endif // ENV_FILE_H