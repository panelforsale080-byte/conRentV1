#include "Tool/EspTab.h"

#include "Il2cpp/Il2cpp.h"
#include "Il2cpp/il2cpp-class.h"
#include "Includes/Logger.h"
#include "imgui/imgui.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace
{
    struct EspVec3
    {
        float x = 0.f;
        float y = 0.f;
        float z = 0.f;

        static float Distance(const EspVec3 &a, const EspVec3 &b)
        {
            const float dx = a.x - b.x;
            const float dy = a.y - b.y;
            const float dz = a.z - b.z;
            return std::sqrt(dx * dx + dy * dy + dz * dz);
        }
    };

    struct EspFeature
    {
        bool box = false;
        bool line = false;
        bool distance = false;
        bool health = false;
        bool name = false;
        bool skillCd = false;
        bool spellCd = false;
        bool spell = false;
        bool monsterBox = false;
        bool monsterName = false;
        bool monsterHealth = false;
    };

    EspFeature g_feature;

    struct EspOffsets
    {
        bool ready = false;
        uintptr_t cameraGetMain = 0;
        uintptr_t cameraWorldToScreen = 0;
        uintptr_t battleBridge = 0;
        uintptr_t battleManager = 0;
        uintptr_t getBStartBattle = 0;
        uintptr_t logicBattleManager = 0;
        uintptr_t getPlayerLogic = 0;
        uintptr_t getCoolTime = 0;
        uintptr_t localPlayerShow = 0x48;
        uintptr_t dicPlayerShow = 0x60;
        uintptr_t dicMonsterShow = 0x68;
        uintptr_t lastPosition = 0x3F4;
        uintptr_t entityId = 0x18C;
        uintptr_t entityDeath = 0xC5;
        uintptr_t sameCamp = 0x2A9;
        uintptr_t hp = 0x1A4;
        uintptr_t hpMax = 0x1A8;
        uintptr_t ownSkillComp = 0x108;
        uintptr_t skillComp = 0x4F0;
        uintptr_t coolDownComp = 0xB0;
        uintptr_t dicCoolInfo = 0x18;
        uintptr_t skillList = 0x10;
        uintptr_t skillUseTypeList = 0x18;
        uintptr_t tranId = 0x88;
        uintptr_t summonSkillId = 0x984;
    };

    EspOffsets g_off;

    const int kMonsterIds[] = {2002, 2003, 2004, 2005, 2006, 2008, 2009, 2011, 2012, 2013, 2056, 2059, 2072};

    bool isMonsterId(int id)
    {
        return std::find(std::begin(kMonsterIds), std::end(kMonsterIds), id) != std::end(kMonsterIds);
    }

    const char *monsterName(int id)
    {
        switch (id)
        {
        case 2002:
            return "Lord";
        case 2003:
            return "Turtle";
        case 2004:
            return "Fiend";
        case 2005:
            return "Serpent";
        case 2006:
            return "Scaled Lizard";
        case 2008:
        case 2059:
            return "Crammer";
        case 2009:
            return "Rockursa";
        case 2011:
        case 2013:
            return "Crab";
        case 2012:
            return "Serpent Kids";
        case 2056:
        case 2072:
            return "Lithowanderer";
        default:
            return "Monster";
        }
    }

    Il2CppClass *findClass(const char *name)
    {
        return Il2cpp::FindClass(name);
    }

    uintptr_t methodPtr(Il2CppClass *klass, const char *name, int args)
    {
        if (!klass)
            return 0;
        auto *method = klass->getMethod(name, args);
        return method ? reinterpret_cast<uintptr_t>(method->methodPointer) : 0;
    }

    uintptr_t fieldOff(Il2CppClass *klass, const char *name)
    {
        if (!klass)
            return 0;
        auto *field = klass->getField(name);
        return field ? field->getOffset() : 0;
    }

    uintptr_t staticFieldValue(Il2CppClass *klass, const char *name)
    {
        if (!klass)
            return 0;
        auto *field = klass->getField(name);
        if (!field)
            return 0;
        return field->getStaticValue<uintptr_t>();
    }

    bool resolveOffsets()
    {
        if (g_off.ready)
            return true;
        if (!Il2cpp::IsRuntimeReady())
            return false;

        auto *camera = findClass("UnityEngine.Camera");
        auto *battleData = findClass("BattleData");
        auto *battleBridge = findClass("BattleBridge");
        auto *battleManager = findClass("BattleManager");
        auto *showEntity = findClass("ShowEntity");
        auto *showPlayer = findClass("ShowPlayer");
        auto *logicBattle = findClass("LogicBattleManager");
        auto *logicFighter = findClass("Battle.LogicFighter");
        auto *logicSkill = findClass("Battle.LogicSkillComp");
        auto *coolDownComp = findClass("Battle.CoolDownComp");
        auto *coolDownData = findClass("Battle.CoolDownData");
        auto *ownSkill = findClass("Battle.ShowOwnSkillComp");
        auto *showSkill = findClass("ShowSkillData");

        g_off.cameraGetMain = methodPtr(camera, "get_main", 0);
        g_off.cameraWorldToScreen = methodPtr(camera, "WorldToScreenPoint", 1);
        g_off.getBStartBattle = methodPtr(battleBridge, "get_bStartBattle", 0);
        g_off.getPlayerLogic = methodPtr(logicBattle, "GetPlayerLogic", 1);
        g_off.getCoolTime = methodPtr(coolDownData, "GetCoolTime", 0);

        g_off.battleBridge = staticFieldValue(battleData, "m_BattleBridge");
        g_off.battleManager = staticFieldValue(battleManager, "Instance");
        g_off.logicBattleManager = staticFieldValue(logicBattle, "Instance");

        if (auto v = fieldOff(battleManager, "m_LocalPlayerShow"))
            g_off.localPlayerShow = v;
        if (auto v = fieldOff(battleManager, "m_dicPlayerShow"))
            g_off.dicPlayerShow = v;
        if (auto v = fieldOff(battleManager, "m_dicMonsterShow"))
            g_off.dicMonsterShow = v;
        if (auto v = fieldOff(showEntity, "<LastPosition>k__BackingField"))
            g_off.lastPosition = v;
        if (auto v = fieldOff(showEntity, "m_ID"))
            g_off.entityId = v;
        if (auto v = fieldOff(showEntity, "m_bDeath"))
            g_off.entityDeath = v;
        if (auto v = fieldOff(showEntity, "m_bSameCampType"))
            g_off.sameCamp = v;
        if (auto v = fieldOff(showEntity, "m_Hp"))
            g_off.hp = v;
        if (auto v = fieldOff(showEntity, "m_HpMax"))
            g_off.hpMax = v;
        if (auto v = fieldOff(showEntity, "m_OwnSkillComp"))
            g_off.ownSkillComp = v;
        if (auto v = fieldOff(logicFighter, "m_SkillComp"))
            g_off.skillComp = v;
        if (auto v = fieldOff(logicSkill, "m_CoolDownComp"))
            g_off.coolDownComp = v;
        if (auto v = fieldOff(coolDownComp, "m_DicCoolInfo"))
            g_off.dicCoolInfo = v;
        if (auto v = fieldOff(ownSkill, "m_SkillList"))
            g_off.skillList = v;
        if (auto v = fieldOff(ownSkill, "skillUseTypeList"))
            g_off.skillUseTypeList = v;
        if (auto v = fieldOff(showSkill, "m_TranID"))
            g_off.tranId = v;
        if (auto v = fieldOff(showPlayer, "m_iSummonSkillId"))
            g_off.summonSkillId = v;

        g_off.ready = g_off.cameraGetMain && g_off.cameraWorldToScreen && g_off.battleManager;
        return g_off.ready;
    }

    bool readIntPtrDict(uintptr_t dict, std::vector<std::pair<int, uintptr_t>> &out)
    {
        out.clear();
        if (!dict)
            return false;

        const uintptr_t entries = *reinterpret_cast<uintptr_t *>(dict + 0x18);
        const int count = *reinterpret_cast<int *>(dict + 0x20);
        if (!entries || count <= 0 || count > 256)
            return false;

        const int maxLength = *reinterpret_cast<int *>(entries + 0x18);
        if (maxLength <= 0 || maxLength > 512)
            return false;

        const uintptr_t data = entries + 0x20;
        const int n = maxLength;
        for (int i = 0; i < n; ++i)
        {
            const uintptr_t entry = data + static_cast<uintptr_t>(i) * 24;
            const int hash = *reinterpret_cast<int *>(entry);
            if (hash < 0)
                continue;
            const int key = *reinterpret_cast<int *>(entry + 8);
            const uintptr_t value = *reinterpret_cast<uintptr_t *>(entry + 16);
            if (!value)
                continue;
            out.emplace_back(key, value);
            if (static_cast<int>(out.size()) >= count)
                break;
        }
        return !out.empty();
    }

    bool readPtrList(uintptr_t list, std::vector<uintptr_t> &out)
    {
        out.clear();
        if (!list)
            return false;
        const uintptr_t items = *reinterpret_cast<uintptr_t *>(list + 0x10);
        const int size = *reinterpret_cast<int *>(list + 0x18);
        if (!items || size <= 0 || size > 32)
            return false;
        const uintptr_t data = items + 0x20;
        out.reserve(static_cast<size_t>(size));
        for (int i = 0; i < size; ++i)
            out.push_back(*reinterpret_cast<uintptr_t *>(data + static_cast<uintptr_t>(i) * 8));
        return true;
    }

    ImVec2 worldToScreen(const EspVec3 &pos, float screenWidth, float screenHeight)
    {
        if (!g_off.cameraGetMain || !g_off.cameraWorldToScreen)
            return ImVec2(-1.f, -1.f);

        auto getMain = reinterpret_cast<void *(*)()>(g_off.cameraGetMain);
        void *camera = getMain();
        if (!camera)
            return ImVec2(-1.f, -1.f);

        auto w2s = reinterpret_cast<EspVec3 (*)(void *, EspVec3)>(g_off.cameraWorldToScreen);
        const EspVec3 screen = w2s(camera, pos);
        if (screen.z > 0.f)
            return ImVec2(screen.x, screenHeight - screen.y);
        return ImVec2(screenWidth - screen.x, screen.y);
    }

    ImU32 healthColor(int hp, int hpMax)
    {
        if (hpMax <= 0)
            return IM_COL32(180, 45, 45, 255);
        if (hp < hpMax * 0.3f)
            return IM_COL32(180, 45, 45, 255);
        if (hp <= hpMax * 0.6f)
            return IM_COL32(180, 180, 45, 255);
        return IM_COL32(0, 255, 0, 255);
    }

    struct CooldownValues
    {
        int skillCount = 0;
        int skill1 = 0;
        int skill2 = 0;
        int skill3 = 0;
        int skill4 = 0;
        int spell = 0;
    };

    CooldownValues readCooldown(int playerKey, uintptr_t entity)
    {
        CooldownValues cd;
        if (!g_off.getPlayerLogic || !g_off.logicBattleManager || !entity)
            return cd;

        auto getLogic = reinterpret_cast<uintptr_t (*)(void *, uint32_t)>(g_off.getPlayerLogic);
        const uintptr_t logic = getLogic(reinterpret_cast<void *>(g_off.logicBattleManager), static_cast<uint32_t>(playerKey));
        if (!logic)
            return cd;

        const uintptr_t skillComp = *reinterpret_cast<uintptr_t *>(logic + g_off.skillComp);
        if (!skillComp)
            return cd;
        const uintptr_t coolDownComp = *reinterpret_cast<uintptr_t *>(skillComp + g_off.coolDownComp);
        const uintptr_t ownSkill = *reinterpret_cast<uintptr_t *>(entity + g_off.ownSkillComp);
        if (!coolDownComp || !ownSkill)
            return cd;

        std::vector<uintptr_t> skills;
        if (!readPtrList(*reinterpret_cast<uintptr_t *>(ownSkill + g_off.skillList), skills))
            return cd;
        cd.skillCount = static_cast<int>(skills.size());
        if (cd.skillCount > 4)
            cd.skillCount = 4;

        std::vector<std::pair<int, uintptr_t>> cool;
        if (!readIntPtrDict(*reinterpret_cast<uintptr_t *>(coolDownComp + g_off.dicCoolInfo), cool))
            return cd;

        const int summonId = *reinterpret_cast<int *>(entity + g_off.summonSkillId);
        auto getCool = reinterpret_cast<int (*)(void *)>(g_off.getCoolTime);

        for (int i = 0; i < cd.skillCount; ++i)
        {
            const uintptr_t skill = skills[static_cast<size_t>(i)];
            if (!skill)
                continue;
            const int tranId = *reinterpret_cast<int *>(skill + g_off.tranId);
            for (const auto &kv : cool)
            {
                if (!kv.second || !getCool)
                    continue;
                int coolTime = getCool(reinterpret_cast<void *>(kv.second)) / 1000;
                if (coolTime < 0)
                    coolTime = 0;
                if (kv.first == summonId)
                    cd.spell = coolTime;
                if (kv.first == tranId)
                {
                    if (i == 0)
                        cd.skill1 = coolTime;
                    else if (i == 1)
                        cd.skill2 = coolTime;
                    else if (i == 2)
                        cd.skill3 = coolTime;
                    else
                        cd.skill4 = coolTime;
                }
            }
        }
        return cd;
    }

    void drawEntityBox(ImDrawList *draw, const ImVec2 &boxStart, const ImVec2 &boxEnd, ImU32 color, float rounding)
    {
        draw->AddRect(boxStart, boxEnd, color, rounding, 0, 1.6f);
    }
} // namespace

void EspTab::Draw()
{
    ImGui::TextDisabled("Own-app overlay. Reads this process only.");
    if (!Il2cpp::IsRuntimeReady())
    {
        ImGui::TextColored(ImVec4(1.f, 0.4f, 0.4f, 1.f), "IL2CPP runtime is not ready");
        return;
    }

    if (ImGui::Button("Resolve classes"))
    {
        g_off.ready = false;
        resolveOffsets();
    }
    ImGui::SameLine();
    ImGui::Text(g_off.ready ? "resolved" : "not resolved");

    ImGui::SeparatorText("Enemy");
    ImGui::Checkbox("Box", &g_feature.box);
    ImGui::SameLine();
    ImGui::Checkbox("Line", &g_feature.line);
    ImGui::SameLine();
    ImGui::Checkbox("Health", &g_feature.health);
    ImGui::Checkbox("Name", &g_feature.name);
    ImGui::SameLine();
    ImGui::Checkbox("Distance", &g_feature.distance);
    ImGui::SameLine();
    ImGui::Checkbox("Spell ID", &g_feature.spell);
    ImGui::Checkbox("Skill CD", &g_feature.skillCd);
    ImGui::SameLine();
    ImGui::Checkbox("Spell CD", &g_feature.spellCd);

    ImGui::SeparatorText("Monster");
    ImGui::Checkbox("Box##m", &g_feature.monsterBox);
    ImGui::SameLine();
    ImGui::Checkbox("Name##m", &g_feature.monsterName);
    ImGui::SameLine();
    ImGui::Checkbox("Health##m", &g_feature.monsterHealth);
}

void EspTab::DrawOverlay(ImDrawList *draw, float screenWidth, float screenHeight)
{
    if (!draw)
        return;
    try
    {
    const bool any = g_feature.box || g_feature.line || g_feature.health || g_feature.name || g_feature.distance ||
                     g_feature.spell || g_feature.skillCd || g_feature.spellCd || g_feature.monsterBox ||
                     g_feature.monsterName || g_feature.monsterHealth;
    if (!any)
        return;
    if (!Il2cpp::IsRuntimeReady() || !resolveOffsets())
        return;

    if (!g_off.battleManager)
        g_off.battleManager = staticFieldValue(findClass("BattleManager"), "Instance");
    if (!g_off.battleBridge)
        g_off.battleBridge = staticFieldValue(findClass("BattleData"), "m_BattleBridge");
    if (!g_off.logicBattleManager)
        g_off.logicBattleManager = staticFieldValue(findClass("LogicBattleManager"), "Instance");

    if (!g_off.battleManager)
        return;
    if (g_off.battleBridge && g_off.getBStartBattle)
    {
        auto started = reinterpret_cast<bool (*)(void *)>(g_off.getBStartBattle);
        if (!started(reinterpret_cast<void *>(g_off.battleBridge)))
            return;
    }

    const uintptr_t localPlayer = *reinterpret_cast<uintptr_t *>(g_off.battleManager + g_off.localPlayerShow);
    if (!localPlayer)
        return;

    const EspVec3 selfPos = *reinterpret_cast<EspVec3 *>(localPlayer + g_off.lastPosition);
    const ImVec2 selfScreen = worldToScreen(selfPos, screenWidth, screenHeight);

    if (g_feature.monsterBox || g_feature.monsterName || g_feature.monsterHealth)
    {
        std::vector<std::pair<int, uintptr_t>> monsters;
        const uintptr_t monsterDict = *reinterpret_cast<uintptr_t *>(g_off.battleManager + g_off.dicMonsterShow);
        if (readIntPtrDict(monsterDict, monsters))
        {
            for (const auto &kv : monsters)
            {
                const uintptr_t v = kv.second;
                if (!v)
                    continue;
                if (*reinterpret_cast<bool *>(v + g_off.entityDeath))
                    continue;
                if (*reinterpret_cast<bool *>(v + g_off.sameCamp))
                    continue;
                const int id = *reinterpret_cast<int *>(v + g_off.entityId);
                if (!isMonsterId(id))
                    continue;

                const int hp = *reinterpret_cast<int *>(v + g_off.hp);
                const int hpMax = *reinterpret_cast<int *>(v + g_off.hpMax);
                const EspVec3 pos = *reinterpret_cast<EspVec3 *>(v + g_off.lastPosition);
                const ImVec2 root = worldToScreen(pos, screenWidth, screenHeight);
                const float boxHeight = std::fabs(screenHeight / 6.8f);
                const float boxWidth = boxHeight * 0.55f;
                const ImVec2 boxStart{root.x - boxWidth * 0.5f, root.y - boxHeight};
                const ImVec2 boxEnd{boxStart.x + boxWidth, boxStart.y + boxHeight};

                if (g_feature.monsterBox)
                    drawEntityBox(draw, boxStart, boxEnd, IM_COL32(255, 255, 255, 200), 6.f);
                if (g_feature.monsterName)
                {
                    const char *name = monsterName(id);
                    const ImVec2 textSize = ImGui::CalcTextSize(name);
                    draw->AddText(ImVec2(root.x - textSize.x * 0.5f, boxStart.y - textSize.y - 4.f),
                                  IM_COL32(255, 200, 100, 255), name);
                }
                if (g_feature.monsterHealth && hpMax > 0)
                {
                    const float pct = std::clamp(static_cast<float>(hp) / static_cast<float>(hpMax), 0.f, 1.f);
                    const ImVec2 barStart{boxStart.x, boxEnd.y + 3.f};
                    const ImVec2 barEnd{boxEnd.x, barStart.y + 5.f};
                    draw->AddRectFilled(barStart, barEnd, IM_COL32(30, 30, 30, 220), 2.5f);
                    draw->AddRectFilled(barStart, ImVec2(barStart.x + (barEnd.x - barStart.x) * pct, barEnd.y),
                                        healthColor(hp, hpMax), 2.5f);
                }
            }
        }
    }

    std::vector<std::pair<int, uintptr_t>> players;
    const uintptr_t playerDict = *reinterpret_cast<uintptr_t *>(g_off.battleManager + g_off.dicPlayerShow);
    if (!readIntPtrDict(playerDict, players))
        return;

    for (const auto &kv : players)
    {
        const uintptr_t v = kv.second;
        if (!v)
            continue;
        if (*reinterpret_cast<bool *>(v + g_off.entityDeath))
            continue;
        if (*reinterpret_cast<bool *>(v + g_off.sameCamp))
            continue;

        const int hp = *reinterpret_cast<int *>(v + g_off.hp);
        const int hpMax = *reinterpret_cast<int *>(v + g_off.hpMax);
        const int id = *reinterpret_cast<int *>(v + g_off.entityId);
        const EspVec3 pos = *reinterpret_cast<EspVec3 *>(v + g_off.lastPosition);
        const ImVec2 root = worldToScreen(pos, screenWidth, screenHeight);
        const float boxHeight = std::fabs(screenHeight / 6.5f);
        const float boxWidth = boxHeight * 0.48f;
        const ImVec2 boxStart{root.x - boxWidth * 0.5f, root.y - boxHeight};
        const ImVec2 boxEnd{boxStart.x + boxWidth, boxStart.y + boxHeight};
        const float centerX = boxStart.x + boxWidth * 0.5f;

        if (g_feature.box)
            drawEntityBox(draw, boxStart, boxEnd, IM_COL32(255, 255, 255, 230), 8.f);
        if (g_feature.line)
            draw->AddLine(selfScreen, root, IM_COL32(255, 255, 255, 180), 1.5f);
        if (g_feature.health && hpMax > 0)
        {
            const float pct = std::clamp(static_cast<float>(hp) / static_cast<float>(hpMax), 0.f, 1.f);
            const ImVec2 barStart{boxStart.x, boxEnd.y + 3.f};
            const ImVec2 barEnd{boxEnd.x, barStart.y + 4.f};
            draw->AddRectFilled(barStart, barEnd, IM_COL32(30, 30, 30, 220), 2.5f);
            draw->AddRectFilled(barStart, ImVec2(barStart.x + boxWidth * pct, barEnd.y), healthColor(hp, hpMax), 2.5f);
            const std::string hpText = std::to_string(hp);
            draw->AddText(ImVec2(barEnd.x + 4.f, barStart.y - 2.f), IM_COL32(255, 255, 255, 220), hpText.c_str());
        }
        if (g_feature.name)
        {
            const std::string name = "ID " + std::to_string(id);
            const ImVec2 textSize = ImGui::CalcTextSize(name.c_str());
            draw->AddText(ImVec2(centerX - textSize.x * 0.5f, boxStart.y - textSize.y - 4.f),
                          IM_COL32(255, 255, 255, 255), name.c_str());
        }
        if (g_feature.spell)
        {
            const int summonId = *reinterpret_cast<int *>(v + g_off.summonSkillId);
            const std::string spell = "Spell " + std::to_string(summonId);
            const ImVec2 textSize = ImGui::CalcTextSize(spell.c_str());
            draw->AddText(ImVec2(centerX - textSize.x * 0.5f, boxEnd.y + 20.f), IM_COL32(135, 206, 250, 255),
                          spell.c_str());
        }
        if (g_feature.distance)
        {
            const std::string dist = std::to_string(static_cast<int>(EspVec3::Distance(selfPos, pos))) + "m";
            const ImVec2 textSize = ImGui::CalcTextSize(dist.c_str());
            draw->AddText(ImVec2(centerX - textSize.x * 0.5f, boxEnd.y + 34.f), IM_COL32(200, 200, 200, 220),
                          dist.c_str());
        }
        if (g_feature.skillCd || g_feature.spellCd)
        {
            const auto cd = readCooldown(kv.first, v);
            ImVec2 skillPos{boxEnd.x + 10.f, boxStart.y + 5.f};
            int index = 0;
            if (g_feature.skillCd)
            {
                const int skills[] = {cd.skill1, cd.skill2, cd.skill3, cd.skill4};
                for (int s = 0; s < cd.skillCount; ++s)
                {
                    const ImVec2 c{skillPos.x, skillPos.y + 25.f * index};
                    draw->AddCircleFilled(c, 10.f, IM_COL32(20, 20, 25, 230));
                    if (skills[s] > 0)
                    {
                        const std::string t = std::to_string(skills[s]);
                        const ImVec2 sz = ImGui::CalcTextSize(t.c_str());
                        draw->AddText(ImVec2(c.x - sz.x * 0.5f, c.y - sz.y * 0.5f), IM_COL32(255, 100, 100, 255),
                                      t.c_str());
                    }
                    else
                    {
                        draw->AddCircle(c, 7.f, IM_COL32(100, 255, 100, 255), 0, 2.f);
                    }
                    ++index;
                }
            }
            if (g_feature.spellCd)
            {
                const ImVec2 c{skillPos.x, skillPos.y + 25.f * index};
                draw->AddCircleFilled(c, 10.f, IM_COL32(20, 20, 25, 230));
                if (cd.spell > 0)
                {
                    const std::string t = std::to_string(cd.spell);
                    const ImVec2 sz = ImGui::CalcTextSize(t.c_str());
                    draw->AddText(ImVec2(c.x - sz.x * 0.5f, c.y - sz.y * 0.5f), IM_COL32(100, 200, 255, 255),
                                  t.c_str());
                }
                else
                {
                    draw->AddCircle(c, 7.f, IM_COL32(100, 200, 255, 255), 0, 2.f);
                }
            }
        }
    }
    }
    catch (...)
    {
    }
}
