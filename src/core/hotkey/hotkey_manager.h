#pragma once

namespace config
{
    class HotkeyField;
}

class HotkeyManager
{
public:
    static HotkeyManager& getInstance();

    void registerField(config::HotkeyField* field);
    void unregisterField(config::HotkeyField* field);
    void updateHotkey(config::HotkeyField* field, int oldVk, int newVk);

    void beginCapture(config::HotkeyField* field) { m_currentCapture = field; }
    bool isCapturing() const { return m_currentCapture != nullptr; }
    config::HotkeyField* getCurrentCapture() const { return m_currentCapture; }

    void setCaptured(int vk);
    void cancelCapture();

    bool processKey(int vk);

    static const std::vector<std::pair<const char*, int>>& keyList()
    {
        static std::vector<std::pair<const char*, int>> list = {
            {"None", 0},

            {"F1", VK_F1}, {"F2", VK_F2}, {"F3", VK_F3}, {"F4", VK_F4},
            {"F5", VK_F5}, {"F6", VK_F6}, {"F7", VK_F7}, {"F8", VK_F8},
            {"F9", VK_F9}, {"F10", VK_F10}, {"F11", VK_F11}, {"F12", VK_F12},

            {"INSERT", VK_INSERT}, {"DELETE", VK_DELETE},
            {"HOME", VK_HOME}, {"END", VK_END},
            {"PAGE UP", VK_PRIOR}, {"PAGE DOWN", VK_NEXT},
            {"UP", VK_UP}, {"DOWN", VK_DOWN}, {"LEFT", VK_LEFT}, {"RIGHT", VK_RIGHT},

            {"TAB", VK_TAB}, {"CAPS", VK_CAPITAL}, {"SHIFT", VK_SHIFT},
            {"CTRL", VK_CONTROL}, {"ALT", VK_MENU}, {"SPACE", VK_SPACE},
            {"BACKSPACE", VK_BACK}, {"ENTER", VK_RETURN}, {"ESC", VK_ESCAPE},

            {"1", '1'}, {"2", '2'}, {"3", '3'}, {"4", '4'}, {"5", '5'},
            {"6", '6'}, {"7", '7'}, {"8", '8'}, {"9", '9'}, {"0", '0'},
            {"A", 'A'}, {"B", 'B'}, {"C", 'C'}, {"D", 'D'}, {"E", 'E'},
            {"F", 'F'}, {"G", 'G'}, {"H", 'H'}, {"I", 'I'}, {"J", 'J'},
            {"K", 'K'}, {"L", 'L'}, {"M", 'M'}, {"N", 'N'}, {"O", 'O'},
            {"P", 'P'}, {"Q", 'Q'}, {"R", 'R'}, {"S", 'S'}, {"T", 'T'},
            {"U", 'U'}, {"V", 'V'}, {"W", 'W'}, {"X", 'X'}, {"Y", 'Y'}, {"Z", 'Z'},

            {"-", VK_OEM_MINUS}, {"=", VK_OEM_PLUS}, {"[", VK_OEM_4}, {"]", VK_OEM_6},
            {"\\", VK_OEM_5}, {";", VK_OEM_1}, {"'", VK_OEM_7},
            {",", VK_OEM_COMMA}, {".", VK_OEM_PERIOD}, {"/", VK_OEM_2},

            {"NUM 0", VK_NUMPAD0}, {"NUM 1", VK_NUMPAD1}, {"NUM 2", VK_NUMPAD2},
            {"NUM 3", VK_NUMPAD3}, {"NUM 4", VK_NUMPAD4}, {"NUM 5", VK_NUMPAD5},
            {"NUM 6", VK_NUMPAD6}, {"NUM 7", VK_NUMPAD7}, {"NUM 8", VK_NUMPAD8},
            {"NUM 9", VK_NUMPAD9}, {"NUM *", VK_MULTIPLY}, {"NUM /", VK_DIVIDE},
            {"NUM +", VK_ADD}, {"NUM -", VK_SUBTRACT}, {"NUM .", VK_DECIMAL}, {"NUM LOCK", VK_NUMLOCK},

            {"PRINT SCREEN", VK_SNAPSHOT}, {"SCROLL LOCK", VK_SCROLL}, {"PAUSE", VK_PAUSE}, {"MENU", VK_APPS},

            {"LEFT WIN", VK_LWIN}, {"RIGHT WIN", VK_RWIN}
        };
        return list;
    }

    static const char* keyName(int vk)
    {
        for (const auto& pair : keyList())
        {
            if (pair.second == vk)
            {
                return pair.first;
            }
        }

        return "Unknown";
    }

private:
    std::vector<config::HotkeyField*> m_fields;
    std::unordered_map<int, std::vector<config::HotkeyField*>> m_vkToFields;
    config::HotkeyField* m_currentCapture = nullptr;
};
