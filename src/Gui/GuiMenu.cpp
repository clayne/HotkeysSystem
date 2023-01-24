#include "GuiMenu.h"
#include "Data.h"
#include "Draw.h"
#include "Config.h"
#include "EquipsetManager.h"
#include "Equipment.h"
#include "Translate.h"
#include "WidgetHandler.h"

#include <filesystem>

#include <imgui.h>
#include <imgui_internal.h>
#include "extern/imgui_impl_dx11.h"
#include "extern/imgui_stdlib.h"

GuiMenu::GuiMenu() {
    ConfigHandler::GetSingleton()->LoadConfig();
    EquipmentManager::GetSingleton()->Load();
    logger::info("GuiMenu initialized!");
}

void GuiMenu::DisableInput(bool _status) {
    auto control = RE::ControlMap::GetSingleton();
    if (!control) return;

    control->ignoreKeyboardMouse = _status;
}

void GuiMenu::Toggle(std::optional<bool> enabled = std::nullopt) {
    auto dataHandler = DataHandler::GetSingleton();
    if (!dataHandler) return;

    auto drawHelper = DrawHelper::GetSingleton();
    if (!drawHelper) return;

    auto widgetHandler = WidgetHandler::GetSingleton();
    if (!widgetHandler) return;

    auto& io = ImGui::GetIO();
    io.ClearInputCharacters();
    io.ClearInputKeys();
    show = enabled.value_or(!show);
    DisableInput(show);
    if (show) {
        dataHandler->Init();
        widgetHandler->CloseExpireTimer();
        widgetHandler->SetMenuAlpha(100);
    } else {
        drawHelper->NotifyReload(true);
        dataHandler->Clear();
        widgetHandler->ProcessFadeOut();
    }
}

void GuiMenu::DrawMain() {
    if (!imgui_inited) return;

    auto ui = RE::UI::GetSingleton();
    if (!ui) return;

    auto config = ConfigHandler::GetSingleton();
    if (!config) return;

    auto ts = Translator::GetSingleton();
    if (!ts) return;

    auto manager = EquipsetManager::GetSingleton();
    if (!manager) return;

    auto dataHandler = DataHandler::GetSingleton();
    if (!dataHandler) return;

    auto drawHelper = DrawHelper::GetSingleton();
    if (!drawHelper) return;

    auto equipment = EquipmentManager::GetSingleton();
    if (!equipment) return;

    auto& io = ImGui::GetIO();
    if (ImGui::IsKeyPressed(config->Gui.hotkey) && !ui->IsMenuOpen(RE::MainMenu::MENU_NAME) &&
        !ui->IsMenuOpen(RE::LoadingMenu::MENU_NAME)) {
        Toggle();
    }
    //if (ImGui::IsKeyPressed(config->Gui.hotkey)) {
    //    Toggle();
    //}
    io.MouseDrawCursor = show;
    io.BackendFlags |= ImGuiBackendFlags_HasGamepad;

    if (!show) return;

    if (font) ImGui::PushFont(font);
    auto& style = ImGui::GetStyle();
    switch (static_cast<Config::GuiStyle>(config->Gui.style)) {
        case Config::GuiStyle::DARK: ImGui::StyleColorsDark(); break;
        case Config::GuiStyle::LIGHT: ImGui::StyleColorsLight(); break;
        case Config::GuiStyle::CLASSIC: ImGui::StyleColorsClassic(); break;
    }
    style.WindowRounding = config->Gui.rounding;
    style.ChildRounding = config->Gui.rounding;
    style.FrameRounding = config->Gui.rounding;
    style.PopupRounding = config->Gui.rounding;
    style.ScrollbarRounding = config->Gui.rounding;
    style.GrabRounding = config->Gui.rounding;
    style.WindowBorderSize = config->Gui.windowBorder ? 1.0f : 0.0f;
    style.FrameBorderSize = config->Gui.frameBorder ? 1.0f : 0.0f;
    style.ScaleAllSizes(config->Gui.fontScaling);

    // Reload data whenever user changes 'Favorited only' option.
    static bool ShouldReloadData = false;
    if (ShouldReloadData != config->Settings.favorOnly) {
        ShouldReloadData = config->Settings.favorOnly;
        dataHandler->Init();
    }

    ImGui::SetNextWindowSize({620, 575}, ImGuiCond_Once);
    if (ImGui::Begin(fmt::format("UI-Integrated Hotkeys System {}", SKSE::PluginDeclaration::GetSingleton()->GetVersion().string()).c_str(),
                     &show,
                     ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_MenuBar)) {

        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu(C_TRANSLATE("_MENUBAR_FILE"))) {
                ImGui::MenuItem(C_TRANSLATE("_TAB_EQUIPSETS"), NULL, false, false);
                ImGui::Separator();
                if (ImGui::MenuItem(C_TRANSLATE("_MENUBAR_SAVE"))) {
                    manager->ExportEquipsets();
                }
                if (ImGui::MenuItem(C_TRANSLATE("_MENUBAR_LOAD"))) {
                    manager->RemoveAllWidget();
                    manager->RemoveAll();
                    manager->ImportEquipsets();
                    manager->SyncSortOrder();
                    manager->CreateAllWidget();
                }
                ImGui::MenuItem("##BLANK", NULL, false, false);
                ImGui::MenuItem(C_TRANSLATE("_TAB_EQUIPMENT"), NULL, false, false);
                ImGui::Separator();
                if (ImGui::MenuItem(C_TRANSLATE("_MENUBAR_SAVE"))) {
                    equipment->Save();
                }
                if (ImGui::MenuItem(C_TRANSLATE("_MENUBAR_LOAD"))) {
                    equipment->RemoveAllArmorWidget();
                    equipment->RemoveAllWeaponWidget();
                    equipment->RemoveAllShoutWidget();
                    equipment->Load();
                    equipment->CreateAllArmorWidget();
                    equipment->CreateAllWeaponWidget();
                    equipment->CreateAllShoutWidget();
                }
                ImGui::MenuItem("##BLANK", NULL, false, false);
                ImGui::MenuItem(C_TRANSLATE("_TAB_CONFIG"), NULL, false, false);
                ImGui::Separator();
                if (ImGui::MenuItem(C_TRANSLATE("_MENUBAR_SAVE"))) {
                    config->SaveConfig();
                }
                if (ImGui::MenuItem(C_TRANSLATE("_MENUBAR_LOAD"))) {
                    config->LoadConfig();
                    ts->Load();
                    dataHandler->Init();
                    GuiMenu::NotifyFontReload();
                }
                
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }

        ImGui::BeginChild("main", {0.f, -ImGui::GetFontSize() - 2.f});

        if (ImGui::BeginTabBar("##")) {
            if (ImGui::BeginTabItem(C_TRANSLATE("_TAB_EQUIPSETS"))) {
                if (ImGui::Button(C_TRANSLATE("_NEW"), ImVec2(90, 0))) {
                    ImGui::SetNextWindowSize({200, 230}, ImGuiCond_Once);
                    ImGui::OpenPopup(C_TRANSLATE("_SELECT_NEW_POPUP"));
                }
                ImGui::Separator();

                ImVec2 center = ImGui::GetMainViewport()->GetCenter();
                ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

                if (ImGui::BeginPopupModal(C_TRANSLATE("_SELECT_NEW_POPUP"), NULL)) {
                    bool shouldClose = false;
                    ImVec2 size = {-FLT_MIN, 40.0f};

                    if (ImGui::Button(C_TRANSLATE("_SELECT_NEW_NORMAL"), size)) {
                        drawHelper->NotifyReload(true);
                        ImGui::OpenPopup(C_TRANSLATE("_SELECT_NEW_OPEN_NORMAL"));
                    }
                    ImGui::SetNextWindowSize({540, 590}, ImGuiCond_Once);
                    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
                    if (ImGui::BeginPopupModal(C_TRANSLATE("_SELECT_NEW_OPEN_NORMAL"), NULL)) {
                        shouldClose = Draw::CreateNormal();
                        ImGui::EndPopup();
                    }

                    if (ImGui::Button(C_TRANSLATE("_SELECT_NEW_POTION"), size)) {
                        drawHelper->NotifyReload(true);
                        ImGui::OpenPopup(C_TRANSLATE("_SELECT_NEW_OPEN_POTION"));
                    }
                    ImGui::SetNextWindowSize({540, 590}, ImGuiCond_Once);
                    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
                    if (ImGui::BeginPopupModal(C_TRANSLATE("_SELECT_NEW_OPEN_POTION"), NULL)) {
                        shouldClose = Draw::CreatePotion();
                        ImGui::EndPopup();
                    }

                    if (ImGui::Button(C_TRANSLATE("_SELECT_NEW_CYCLE"), size)) {
                        drawHelper->NotifyReload(true);
                        ImGui::OpenPopup(C_TRANSLATE("_SELECT_NEW_OPEN_CYCLE"));
                    }
                    ImGui::SetNextWindowSize({540, 590}, ImGuiCond_Once);
                    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
                    if (ImGui::BeginPopupModal(C_TRANSLATE("_SELECT_NEW_OPEN_CYCLE"), NULL)) {
                        shouldClose = Draw::CreateCycle();
                        ImGui::EndPopup();
                    }

                    ImGui::Text("\n\n");
                    ImGui::Separator();
                    if (ImGui::Button(C_TRANSLATE("_CANCEL"), {-FLT_MIN, 25.0f})) {
                        ImGui::CloseCurrentPopup();
                    }
                    if (shouldClose) {
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::EndPopup();
                }

                auto sort = static_cast<Config::SortType>(config->Settings.sort);
                std::vector<Equipset*> equipsetVec;
                if (sort == Config::SortType::CREATEASC) {
                    equipsetVec = manager->equipsetVec;
                } else if (sort == Config::SortType::CREATEDESC) {
                    auto compare = [](Equipset* _first, Equipset* _second) {
                        if (!_first || !_second) return false;

                        return _first->order > _second->order;
                    };
                    equipsetVec = manager->equipsetVec;
                    std::sort(equipsetVec.begin(), equipsetVec.end(), compare);
                } else if (sort == Config::SortType::NAMEASC) {
                    auto compare = [](Equipset* _first, Equipset* _second) {
                        if (!_first || !_second) return false;

                        return _first->name < _second->name;
                    };
                    equipsetVec = manager->equipsetVec;
                    std::sort(equipsetVec.begin(), equipsetVec.end(), compare);
                } else if (sort == Config::SortType::NAMEDESC) {
                    auto compare = [](Equipset* _first, Equipset* _second) {
                        if (!_first || !_second) return false;

                        return _first->name > _second->name;
                    };
                    equipsetVec = manager->equipsetVec;
                    std::sort(equipsetVec.begin(), equipsetVec.end(), compare);
                }
                for (int i = 0; i < equipsetVec.size(); i++) {
                    auto equipset = equipsetVec[i];

                    if (current_opened.load() != i) {
                        ImGui::SetNextItemOpen(false);
                    }

                    if (ImGui::CollapsingHeader(equipset->name.c_str())) {
                        ImGui::Indent();
                        ImGui::PushID(i);

                        if (equipset->type == Equipset::TYPE::NORMAL) {
                            Draw::ShowNormal(static_cast<NormalSet*>(equipset), i);
                        } else if (equipset->type == Equipset::TYPE::POTION) {
                            Draw::ShowPotion(static_cast<PotionSet*>(equipset), i);
                        } else if (equipset->type == Equipset::TYPE::CYCLE) {
                            Draw::ShowCycle(static_cast<CycleSet*>(equipset), i);
                        }

                        ImGui::PopID();
                        ImGui::Unindent();
                    }
                }

                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem(C_TRANSLATE("_TAB_EQUIPMENT"))) {
                DrawEquipment();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem(C_TRANSLATE("_TAB_CONFIG"))) {
                DrawConfig();
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
        
        ImGui::EndChild();
    } else {
        Toggle(false);
    }
    ImGui::End();

    if (font) ImGui::PopFont();
}

void GuiMenu::LoadFont() {
    if (!reload_font.load()) return;
    reload_font.store(false);

    auto& io = ImGui::GetIO();
    auto config = ConfigHandler::GetSingleton();
    std::filesystem::path path = "Data/SKSE/Plugins/UIHS/Fonts/" + config->Gui.fontPath;
    if (std::filesystem::is_regular_file(path) && ((path.extension() == ".ttf") || (path.extension() == ".otf"))) {
        ImVector<ImWchar> ranges;
        ImFontGlyphRangesBuilder builder;
        builder.AddRanges(io.Fonts->GetGlyphRangesDefault());
        if (config->Gui.language == (int)Config::LangType::CHINESE) {
            builder.AddRanges(io.Fonts->GetGlyphRangesChineseFull());
        } else if (config->Gui.language == (int)Config::LangType::JAPANESE) {
            builder.AddRanges(io.Fonts->GetGlyphRangesJapanese());
        } else if (config->Gui.language == (int)Config::LangType::KOREAN) {
            builder.AddRanges(io.Fonts->GetGlyphRangesKorean());
        } else if (config->Gui.language == (int)Config::LangType::RUSSIAN) {
            builder.AddRanges(io.Fonts->GetGlyphRangesCyrillic());
        } else if (config->Gui.language == (int)Config::LangType::THAI) {
            builder.AddRanges(io.Fonts->GetGlyphRangesThai());
        } else if (config->Gui.language == (int)Config::LangType::VIETNAMESE) {
            builder.AddRanges(io.Fonts->GetGlyphRangesVietnamese());
        }
        builder.BuildRanges(&ranges);

        io.Fonts->Clear();
        io.Fonts->AddFontDefault();
        font = io.Fonts->AddFontFromFileTTF(path.string().c_str(), config->Gui.fontSize, NULL, ranges.Data);
        if (io.Fonts->Build()) {
            ImGui_ImplDX11_ReCreateFontsTexture();
            logger::info("Font loaded.");
            return;
        } else
            logger::error("Failed to build font {}", config->Gui.fontPath);
    } else {
        font = nullptr;
        logger::info("{} is not a font file", config->Gui.fontPath);
    }

    io.Fonts->Clear();
    io.Fonts->AddFontDefault();
    assert(io.Fonts->Build());

    ImGui_ImplDX11_ReCreateFontsTexture();

    logger::info("Font loaded.");
}

void GuiMenu::DrawEquipment() {
    auto config = ConfigHandler::GetSingleton();
    if (!config) return;

    auto ts = Translator::GetSingleton();
    if (!ts) return;

    auto equipment = EquipmentManager::GetSingleton();
    if (!equipment) return;

    ImGui::InvisibleButton("##Invisible", ImVec2(-110, 25));
    ImGui::SameLine();
    if (ImGui::Button(C_TRANSLATE("_RELOAD_WIDGET"), ImVec2(100, 25))) {
        equipment->RemoveAllArmorWidget();
        equipment->RemoveAllWeaponWidget();
        equipment->RemoveAllShoutWidget();
        equipment->CreateAllArmorWidget();
        equipment->CreateAllWeaponWidget();
        equipment->CreateAllShoutWidget();
    }

    if (ImGui::CollapsingHeader(C_TRANSLATE("_TAB_EQUIPMENT_ARMOR"))) {
        ImGui::Indent();

        for (int i = 0; i < 32; i++) {
            auto treeName = fmt::format("Slot{}", i + 30);
            if (ImGui::TreeNode(treeName.c_str())) {
                ImGui::PushID(i);

                ImGui::BeginChild("LeftRegion", ImVec2(ImGui::GetContentRegionAvail().x * 0.5f, 110.0f), false);

                ImGui::SetNextItemOpen(true, ImGuiCond_Once);
                if (ImGui::TreeNode(C_TRANSLATE("_WIDGET_ICON"))) {
                    ImGui::Checkbox(C_TRANSLATE("_WIDGET_ENABLE"), &equipment->armor[i].widgetIcon.enable);
                    Draw::SliderInt(C_TRANSLATE("_WIDGET_OFFSETX"), &equipment->armor[i].widgetIcon.offsetX, Config::icon_smin,
                                    Config::icon_smax, "%d", ImGuiSliderFlags_AlwaysClamp);
                    Draw::SliderInt(C_TRANSLATE("_WIDGET_OFFSETY"), &equipment->armor[i].widgetIcon.offsetY, Config::icon_smin,
                                    Config::icon_smax, "%d", ImGuiSliderFlags_AlwaysClamp);

                    ImGui::TreePop();
                }
                ImGui::EndChild();
                ImGui::SameLine();
                ImGui::BeginChild("RightRegion", ImVec2(0.0f, 110.0f), false);

                ImGui::SetNextItemOpen(true, ImGuiCond_Once);
                if (ImGui::TreeNode(C_TRANSLATE("_WIDGET_NAME"))) {
                    std::vector<std::string> align_items = {TRANSLATE("_ALIGN_LEFT"),
                                                            TRANSLATE("_ALIGN_RIGHT"),
                                                            TRANSLATE("_ALIGN_CENTER")};

                    ImGui::Checkbox(C_TRANSLATE("_WIDGET_ENABLE"), &equipment->armor[i].widgetName.enable);
                    uint32_t* align = reinterpret_cast<uint32_t*>(&equipment->armor[i].widgetName.align);
                    Draw::Combo(align_items, align, C_TRANSLATE("_WIDGET_ALIGN"));
                    Draw::SliderInt(C_TRANSLATE("_WIDGET_OFFSETX"), &equipment->armor[i].widgetName.offsetX, Config::text_smin,
                                    Config::text_smax, "%d", ImGuiSliderFlags_AlwaysClamp);
                    Draw::SliderInt(C_TRANSLATE("_WIDGET_OFFSETY"), &equipment->armor[i].widgetName.offsetY, Config::text_smin,
                                    Config::text_smax, "%d", ImGuiSliderFlags_AlwaysClamp);

                    ImGui::TreePop();
                }
                ImGui::EndChild();

                ImGui::Separator();

                ImGui::PopID();
                ImGui::TreePop();
            }
        }

        ImGui::Unindent();
    }

    if (ImGui::CollapsingHeader(C_TRANSLATE("_TAB_EQUIPMENT_WEAPON"))) {
        ImGui::Indent();

        if (ImGui::TreeNode(C_TRANSLATE("_WEAPON_LEFTHAND"))) {
            ImGui::BeginChild("LeftRegion", ImVec2(ImGui::GetContentRegionAvail().x * 0.5f, 110.0f), false);

            ImGui::SetNextItemOpen(true, ImGuiCond_Once);
            if (ImGui::TreeNode(C_TRANSLATE("_WIDGET_ICON"))) {
                ImGui::Checkbox(C_TRANSLATE("_WIDGET_ENABLE"), &equipment->lefthand.widgetIcon.enable);
                Draw::SliderInt(C_TRANSLATE("_WIDGET_OFFSETX"), &equipment->lefthand.widgetIcon.offsetX, Config::icon_smin,
                                Config::icon_smax, "%d", ImGuiSliderFlags_AlwaysClamp);
                Draw::SliderInt(C_TRANSLATE("_WIDGET_OFFSETY"), &equipment->lefthand.widgetIcon.offsetY, Config::icon_smin,
                                Config::icon_smax, "%d", ImGuiSliderFlags_AlwaysClamp);

                ImGui::TreePop();
            }
            ImGui::EndChild();
            ImGui::SameLine();
            ImGui::BeginChild("RightRegion", ImVec2(0.0f, 110.0f), false);

            ImGui::SetNextItemOpen(true, ImGuiCond_Once);
            if (ImGui::TreeNode(C_TRANSLATE("_WIDGET_NAME"))) {
                std::vector<std::string> align_items = {TRANSLATE("_ALIGN_LEFT"),
                                                        TRANSLATE("_ALIGN_RIGHT"),
                                                        TRANSLATE("_ALIGN_CENTER")};

                ImGui::Checkbox(C_TRANSLATE("_WIDGET_ENABLE"), &equipment->lefthand.widgetName.enable);
                uint32_t* align = reinterpret_cast<uint32_t*>(&equipment->lefthand.widgetName.align);
                Draw::Combo(align_items, align, C_TRANSLATE("_WIDGET_ALIGN"));
                Draw::SliderInt(C_TRANSLATE("_WIDGET_OFFSETX"), &equipment->lefthand.widgetName.offsetX, Config::text_smin,
                                Config::text_smax, "%d", ImGuiSliderFlags_AlwaysClamp);
                Draw::SliderInt(C_TRANSLATE("_WIDGET_OFFSETY"), &equipment->lefthand.widgetName.offsetY, Config::text_smin,
                                Config::text_smax, "%d", ImGuiSliderFlags_AlwaysClamp);

                ImGui::TreePop();
            }
            ImGui::EndChild();

            ImGui::Separator();

            ImGui::TreePop();
        }
        if (ImGui::TreeNode(C_TRANSLATE("_WEAPON_RIGHTHAND"))) {
            ImGui::BeginChild("LeftRegion", ImVec2(ImGui::GetContentRegionAvail().x * 0.5f, 110.0f), false);

            ImGui::SetNextItemOpen(true, ImGuiCond_Once);
            if (ImGui::TreeNode(C_TRANSLATE("_WIDGET_ICON"))) {
                ImGui::Checkbox(C_TRANSLATE("_WIDGET_ENABLE"), &equipment->righthand.widgetIcon.enable);
                Draw::SliderInt(C_TRANSLATE("_WIDGET_OFFSETX"), &equipment->righthand.widgetIcon.offsetX, Config::icon_smin,
                                Config::icon_smax, "%d", ImGuiSliderFlags_AlwaysClamp);
                Draw::SliderInt(C_TRANSLATE("_WIDGET_OFFSETY"), &equipment->righthand.widgetIcon.offsetY, Config::icon_smin,
                                Config::icon_smax, "%d", ImGuiSliderFlags_AlwaysClamp);

                ImGui::TreePop();
            }
            ImGui::EndChild();
            ImGui::SameLine();
            ImGui::BeginChild("RightRegion", ImVec2(0.0f, 110.0f), false);

            ImGui::SetNextItemOpen(true, ImGuiCond_Once);
            if (ImGui::TreeNode(C_TRANSLATE("_WIDGET_NAME"))) {
                std::vector<std::string> align_items = {TRANSLATE("_ALIGN_LEFT"),
                                                        TRANSLATE("_ALIGN_RIGHT"),
                                                        TRANSLATE("_ALIGN_CENTER")};

                ImGui::Checkbox(C_TRANSLATE("_WIDGET_ENABLE"), &equipment->righthand.widgetName.enable);
                uint32_t* align = reinterpret_cast<uint32_t*>(&equipment->righthand.widgetName.align);
                Draw::Combo(align_items, align, C_TRANSLATE("_WIDGET_ALIGN"));
                Draw::SliderInt(C_TRANSLATE("_WIDGET_OFFSETX"), &equipment->righthand.widgetName.offsetX, Config::text_smin,
                                Config::text_smax, "%d", ImGuiSliderFlags_AlwaysClamp);
                Draw::SliderInt(C_TRANSLATE("_WIDGET_OFFSETY"), &equipment->righthand.widgetName.offsetY, Config::text_smin,
                                Config::text_smax, "%d", ImGuiSliderFlags_AlwaysClamp);

                ImGui::TreePop();
            }
            ImGui::EndChild();

            ImGui::Separator();

            ImGui::TreePop();
        }

        ImGui::Unindent();
    }

    if (ImGui::CollapsingHeader(C_TRANSLATE("_TAB_EQUIPMENT_SHOUT"))) {
        ImGui::Indent();

        ImGui::BeginChild("LeftRegion", ImVec2(ImGui::GetContentRegionAvail().x * 0.5f, 110.0f), false);

        ImGui::SetNextItemOpen(true, ImGuiCond_Once);
        if (ImGui::TreeNode(C_TRANSLATE("_WIDGET_ICON"))) {
            ImGui::Checkbox(C_TRANSLATE("_WIDGET_ENABLE"), &equipment->shout.widgetIcon.enable);
            Draw::SliderInt(C_TRANSLATE("_WIDGET_OFFSETX"), &equipment->shout.widgetIcon.offsetX, Config::icon_smin, Config::icon_smax,
                            "%d", ImGuiSliderFlags_AlwaysClamp);
            Draw::SliderInt(C_TRANSLATE("_WIDGET_OFFSETY"), &equipment->shout.widgetIcon.offsetY, Config::icon_smin, Config::icon_smax,
                            "%d", ImGuiSliderFlags_AlwaysClamp);

            ImGui::TreePop();
        }
        ImGui::EndChild();
        ImGui::SameLine();
        ImGui::BeginChild("RightRegion", ImVec2(0.0f, 110.0f), false);

        ImGui::SetNextItemOpen(true, ImGuiCond_Once);
        if (ImGui::TreeNode(C_TRANSLATE("_WIDGET_NAME"))) {
            std::vector<std::string> align_items = {TRANSLATE("_ALIGN_LEFT"),
                                                    TRANSLATE("_ALIGN_RIGHT"),
                                                    TRANSLATE("_ALIGN_CENTER")};

            ImGui::Checkbox(C_TRANSLATE("_WIDGET_ENABLE"), &equipment->shout.widgetName.enable);
            uint32_t* align = reinterpret_cast<uint32_t*>(&equipment->shout.widgetName.align);
            Draw::Combo(align_items, align, C_TRANSLATE("_WIDGET_ALIGN"));
            Draw::SliderInt(C_TRANSLATE("_WIDGET_OFFSETX"), &equipment->shout.widgetName.offsetX, Config::text_smin, Config::text_smax,
                            "%d", ImGuiSliderFlags_AlwaysClamp);
            Draw::SliderInt(C_TRANSLATE("_WIDGET_OFFSETY"), &equipment->shout.widgetName.offsetY, Config::text_smin, Config::text_smax,
                            "%d", ImGuiSliderFlags_AlwaysClamp);

            ImGui::TreePop();
        }
        ImGui::EndChild();

        ImGui::Unindent();
    }
}

void GuiMenu::DrawConfig() {
    auto config = ConfigHandler::GetSingleton();
    if (!config) return;

    auto ts = Translator::GetSingleton();
    if (!ts) return;

    ImGui::SetNextItemOpen(true, ImGuiCond_Once);
    if (ImGui::CollapsingHeader(C_TRANSLATE("_TAB_CONFIG_WIDGET"))) {
        ImGui::BeginChild("LeftRegion", ImVec2(ImGui::GetContentRegionAvail().x * 0.5f, 175.0f), false);
        ImGui::SetNextItemOpen(true, ImGuiCond_Once);
        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
        if (ImGui::TreeNode(C_TRANSLATE("_TAB_CONFIG_WIDGET_GENERAL"))) {
            Draw::Combo(config->fontVec, &config->Widget.General.font, C_TRANSLATE("_TAB_CONFIG_WIDGET_GENERAL_FONT"));

            std::vector<std::string> displayVec = {TRANSLATE("_DISPLAYMODE_ALWAYS"),
                                                   TRANSLATE("_DISPLAYMODE_INCOMBAT")};
            Draw::Combo(displayVec, &config->Widget.General.displayMode,
                        C_TRANSLATE("_TAB_CONFIG_WIDGET_GENERAL_DISPLAY"));

            std::vector<std::string> animVec = {TRANSLATE("_ANIMATIONTYPE_FADE"),
                                                TRANSLATE("_ANIMATIONTYPE_INSTANT")};
            Draw::Combo(animVec, &config->Widget.General.animType, C_TRANSLATE("_TAB_CONFIG_WIDGET_GENERAL_ANIM"));

            auto msg = "%.1f" + TRANSLATE("_TIMESECOND");
            Draw::SliderFloat(C_TRANSLATE("_TAB_CONFIG_WIDGET_GENERAL_ANIMDELAY"), &config->Widget.General.animDelay,
                              1.0f, 5.0f, msg.c_str(), ImGuiSliderFlags_AlwaysClamp);

            ImGui::TreePop();
        }
        ImGui::PopItemWidth();
        ImGui::EndChild();
        ImGui::SameLine();
        ImGui::BeginChild("RightRegion", ImVec2(0.0f, 175.0f), false);
        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
        if (ImGui::TreeNode(C_TRANSLATE("_TAB_EQUIPSETS"))) {
            Draw::ComboIcon(&config->Widget.Equipset.bgType, C_TRANSLATE("_BACKGROUND_TYPE"));
            Draw::SliderInt(C_TRANSLATE("_BACKGROUND_SIZE"), &config->Widget.Equipset.bgSize, 0, 200, "%d%%",
                            ImGuiSliderFlags_AlwaysClamp);
            Draw::SliderInt(C_TRANSLATE("_BACKGROUND_ALPHA"), &config->Widget.Equipset.bgAlpha, 0, 100, "%d%%",
                            ImGuiSliderFlags_AlwaysClamp);
            Draw::SliderInt(C_TRANSLATE("_WIDGET_SIZE"), &config->Widget.Equipset.widgetSize, 0, 200, "%d%%",
                            ImGuiSliderFlags_AlwaysClamp);
            Draw::SliderInt(C_TRANSLATE("_FONT_SIZE"), &config->Widget.Equipset.fontSize, 0, 200, "%d%%",
                            ImGuiSliderFlags_AlwaysClamp);
            ImGui::Checkbox(C_TRANSLATE("_FONT_SHADOW"), &config->Widget.Equipset.fontShadow);
            ImGui::TreePop();
        }
        ImGui::SetNextItemOpen(true, ImGuiCond_Once);
        if (ImGui::TreeNode(C_TRANSLATE("_TAB_EQUIPMENT"))) {
            if (ImGui::TreeNode(C_TRANSLATE("_TAB_EQUIPMENT_ARMOR"))) {
                Draw::ComboIcon(&config->Widget.Equipment.Armor.bgType, C_TRANSLATE("_BACKGROUND_TYPE"));
                Draw::SliderInt(C_TRANSLATE("_BACKGROUND_SIZE"), &config->Widget.Equipment.Armor.bgSize, 0, 200, "%d%%",
                                ImGuiSliderFlags_AlwaysClamp);
                Draw::SliderInt(C_TRANSLATE("_BACKGROUND_ALPHA"), &config->Widget.Equipment.Armor.bgAlpha, 0, 100,
                                "%d%%", ImGuiSliderFlags_AlwaysClamp);
                Draw::SliderInt(C_TRANSLATE("_WIDGET_SIZE"), &config->Widget.Equipment.Armor.widgetSize, 0, 200, "%d%%",
                                ImGuiSliderFlags_AlwaysClamp);
                Draw::SliderInt(C_TRANSLATE("_FONT_SIZE"), &config->Widget.Equipment.Armor.fontSize, 0, 200, "%d%%",
                                ImGuiSliderFlags_AlwaysClamp);
                ImGui::Checkbox(C_TRANSLATE("_FONT_SHADOW"), &config->Widget.Equipment.Armor.fontShadow);
                ImGui::TreePop();
            }
            if (ImGui::TreeNode(C_TRANSLATE("_TAB_EQUIPMENT_WEAPON"))) {
                Draw::ComboIcon(&config->Widget.Equipment.Weapon.bgType, C_TRANSLATE("_BACKGROUND_TYPE"));
                Draw::SliderInt(C_TRANSLATE("_BACKGROUND_SIZE"), &config->Widget.Equipment.Weapon.bgSize, 0, 200,
                                "%d%%", ImGuiSliderFlags_AlwaysClamp);
                Draw::SliderInt(C_TRANSLATE("_BACKGROUND_ALPHA"), &config->Widget.Equipment.Weapon.bgAlpha, 0, 100, "%d%%",
                                ImGuiSliderFlags_AlwaysClamp);
                Draw::SliderInt(C_TRANSLATE("_WIDGET_SIZE"), &config->Widget.Equipment.Weapon.widgetSize, 0, 200, "%d%%",
                                ImGuiSliderFlags_AlwaysClamp);
                Draw::SliderInt(C_TRANSLATE("_FONT_SIZE"), &config->Widget.Equipment.Weapon.fontSize, 0, 200, "%d%%",
                                ImGuiSliderFlags_AlwaysClamp);
                ImGui::Checkbox(C_TRANSLATE("_FONT_SHADOW"), &config->Widget.Equipment.Weapon.fontShadow);
                ImGui::TreePop();
            }
            if (ImGui::TreeNode(C_TRANSLATE("_TAB_EQUIPMENT_SHOUT"))) {
                Draw::ComboIcon(&config->Widget.Equipment.Shout.bgType, C_TRANSLATE("_BACKGROUND_TYPE"));
                Draw::SliderInt(C_TRANSLATE("_BACKGROUND_SIZE"), &config->Widget.Equipment.Shout.bgSize, 0, 200, "%d%%",
                                ImGuiSliderFlags_AlwaysClamp);
                Draw::SliderInt(C_TRANSLATE("_BACKGROUND_ALPHA"), &config->Widget.Equipment.Shout.bgAlpha, 0, 100, "%d%%",
                                ImGuiSliderFlags_AlwaysClamp);
                Draw::SliderInt(C_TRANSLATE("_WIDGET_SIZE"), &config->Widget.Equipment.Shout.widgetSize, 0, 200, "%d%%",
                                ImGuiSliderFlags_AlwaysClamp);
                Draw::SliderInt(C_TRANSLATE("_FONT_SIZE"), &config->Widget.Equipment.Shout.fontSize, 0, 200, "%d%%",
                                ImGuiSliderFlags_AlwaysClamp);
                ImGui::Checkbox(C_TRANSLATE("_FONT_SHADOW"), &config->Widget.Equipment.Shout.fontShadow);
                ImGui::TreePop();
            }
            ImGui::TreePop();
        }
        ImGui::PopItemWidth();
        ImGui::EndChild();
        ImGui::InvisibleButton("##Invisible", ImVec2(-110, 25));
        ImGui::SameLine();
        if (ImGui::Button(C_TRANSLATE("_RELOAD_WIDGET"), ImVec2(100, 25))) {
            auto equipment = EquipmentManager::GetSingleton();
            if (!equipment) return;

            auto equipset = EquipsetManager::GetSingleton();
            if (!equipset) return;

            equipset->RemoveAllWidget();
            equipment->RemoveAllArmorWidget();
            equipment->RemoveAllWeaponWidget();
            equipment->RemoveAllShoutWidget();
            equipment->CreateAllArmorWidget();
            equipment->CreateAllWeaponWidget();
            equipment->CreateAllShoutWidget();
            equipset->CreateAllWidget();
        }
    }

    ImGui::SetNextItemOpen(true, ImGuiCond_Once);
    if (ImGui::CollapsingHeader(C_TRANSLATE("_TAB_CONFIG_SETTINGS"))) {
        ImGui::Indent();
        ImGui::BeginChild("LeftRegion2", ImVec2(ImGui::GetContentRegionAvail().x * 0.5f, 70.0f), false);
        Draw::InputButton(&config->Settings.modifier1, "Modifier1", TRANSLATE("_EDIT"), TRANSLATE("_MODIFIER1"));
        Draw::InputButton(&config->Settings.modifier2, "Modifier2", TRANSLATE("_EDIT"), TRANSLATE("_MODIFIER2"));
        Draw::InputButton(&config->Settings.modifier3, "Modifier3", TRANSLATE("_EDIT"), TRANSLATE("_MODIFIER3"));
        ImGui::EndChild();
        ImGui::SameLine();
        ImGui::BeginChild("RightRegion2", ImVec2(0.0f, 70.0f), false);
        {
            std::vector<std::string> items;
            items.push_back(TRANSLATE("_TAB_CONFIG_SETTINGS_SORT_CREATEASC"));
            items.push_back(TRANSLATE("_TAB_CONFIG_SETTINGS_SORT_CREATEDESC"));
            items.push_back(TRANSLATE("_TAB_CONFIG_SETTINGS_SORT_NAMEASC"));
            items.push_back(TRANSLATE("_TAB_CONFIG_SETTINGS_SORT_NAMEDESC"));
            Draw::Combo(items, &config->Settings.sort, C_TRANSLATE("_TAB_CONFIG_SETTINGS_SORTORDER"));
        }
        ImGui::Checkbox(C_TRANSLATE("_TAB_CONFIG_SETTINGS_FAVOR"), &config->Settings.favorOnly);
        ImGui::EndChild();

        ImGui::Unindent();
    }

    ImGui::SetNextItemOpen(true, ImGuiCond_Once);
    if (ImGui::CollapsingHeader(C_TRANSLATE("_TAB_CONFIG_GUI"))) {
        ImGui::Indent();

        ImGui::BeginChild("LeftRegion3", ImVec2(ImGui::GetContentRegionAvail().x * 0.5f, 70.0f), false);

        auto hotkey = reinterpret_cast<uint32_t*>(&config->Gui.hotkey);
        if (hotkey) {
            Draw::InputButton(hotkey, "GuiHotkey", TRANSLATE("_EDIT"), TRANSLATE("_TAB_CONFIG_GUI_HOTKEY"));
        }
        ImGui::Checkbox(C_TRANSLATE("_TAB_CONFIG_GUI_WINDOWBORDER"), &config->Gui.windowBorder);
        ImGui::Checkbox(C_TRANSLATE("_TAB_CONFIG_GUI_FRAMEBORDER"), &config->Gui.frameBorder);
        ImGui::EndChild();

        ImGui::SameLine();
        ImGui::BeginChild("RightRegion3", ImVec2(0.0f, 70.0f), false);
        {
            std::vector<std::string> items = {"Chinese", "Czech", "English",
                                              "French", "German", "Italian",
                                              "Japanese", "Korean", "Polish",
                                              "Russian", "Spanish", "Thai",
                                              "Vietnamese"};
            Draw::Combo(items, &config->Gui.language, TRANSLATE("_TAB_CONFIG_GUI_LANGUAGE"));

            static uint32_t language = config->Gui.language;
            if (language != config->Gui.language) {
                language = config->Gui.language;
                reload_font.store(true);
                Translator::GetSingleton()->Load();
                DataHandler::GetSingleton()->Init();
            }
        }
        {
            std::vector<std::string> items = {TRANSLATE("_STYLE_DARK"),
                                              TRANSLATE("_STYLE_LIGHT"),
                                              TRANSLATE("_STYLE_CLASSIC")};
            Draw::Combo(items, &config->Gui.style, TRANSLATE("_TAB_CONFIG_GUI_STYLE"));
        }
        Draw::SliderFloat(TRANSLATE("_TAB_CONFIG_GUI_ROUNDING"), &config->Gui.rounding, 0.0f, 12.0f, "%.1f", ImGuiSliderFlags_AlwaysClamp);
        ImGui::EndChild();

        ImGui::Unindent();
    }
}