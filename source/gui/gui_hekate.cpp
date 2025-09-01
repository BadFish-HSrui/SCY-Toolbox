#include "gui_hekate.hpp"
#include "button.hpp"

#include <stdio.h>
#include <dirent.h>
#include <sstream>
#include <fstream>
#include <utility>
#include "SimpleIniParser.hpp"

#include "list_selector.hpp"
#include "ams_bpc.h"

#define IRAM_PAYLOAD_MAX_SIZE 0x24000
static u8 g_reboot_payload[IRAM_PAYLOAD_MAX_SIZE];

enum NyxUMSType {
    NYX_UMS_SD_CARD = 0,
    NYX_UMS_EMMC_BOOT0,
    NYX_UMS_EMMC_BOOT1,
    NYX_UMS_EMMC_GPP,
    NYX_UMS_EMUMMC_BOOT0,
    NYX_UMS_EMUMMC_BOOT1,
    NYX_UMS_EMUMMC_GPP
};

GuiHekate::GuiHekate() : Gui() {
    canReboot = true;
    restrictedMode = true;
    errorMessage = "";
    Result rc = 0;

    if (canReboot && R_FAILED(rc = spsmInitialize())) {
        canReboot = false;
    }

    initializeForRestrictedMode();
    
    endInit();
}

void GuiHekate::initializeForRestrictedMode() {
    auto shutdownButton = new Button();
    shutdownButton->usableCondition = [this]() -> bool { return canReboot; };
    shutdownButton->volume = {Gui::g_framebuffer_width - 800, 80};
    shutdownButton->position = {100, Gui::g_framebuffer_height / 2 - shutdownButton->volume.second / 2};
    shutdownButton->adjacentButton[ADJ_RIGHT] = 1;
    shutdownButton->drawAction = [shutdownButton](Gui *gui, u16 x, u16 y, bool *isActivated) {
        gui->drawRectangled(x, y, shutdownButton->volume.first, shutdownButton->volume.second, currTheme.submenuButtonColor);
        gui->drawTextAligned(font20, x + shutdownButton->volume.first / 2, y + shutdownButton->volume.second / 2 + 10, currTheme.textColor, "关机", ALIGNED_CENTER);
    };
    shutdownButton->inputAction = [&](u32 kdown, bool *isActivated) {
        if (kdown & HidNpadButton_A) {
            spsmShutdown(false);
        }
    };
    add(shutdownButton);

    auto rebootButton = new Button();
    rebootButton->usableCondition = [this]() -> bool { return canReboot; };
    rebootButton->volume = {Gui::g_framebuffer_width - 800, 80};
    rebootButton->position = {Gui::g_framebuffer_width - 100 - rebootButton->volume.first, Gui::g_framebuffer_height / 2 - rebootButton->volume.second / 2};
    rebootButton->adjacentButton[ADJ_LEFT] = 0;
    rebootButton->drawAction = [rebootButton](Gui *gui, u16 x, u16 y, bool *isActivated) {
        gui->drawRectangled(x, y, rebootButton->volume.first, rebootButton->volume.second, currTheme.submenuButtonColor);
        gui->drawTextAligned(font20, x + rebootButton->volume.first / 2, y + rebootButton->volume.second / 2 + 10, currTheme.textColor, "重启到payload", ALIGNED_CENTER);
    };
    rebootButton->inputAction = [&](u32 kdown, bool *isActivated) {
        if (kdown & HidNpadButton_A) {
            spsmShutdown(true);
        }
    };
    add(rebootButton);
}


GuiHekate::~GuiHekate() {
    if (serviceIsActive(amsBpcGetServiceSession())) {
        amsBpcExit();
    }

    if(!serviceIsActive(smGetServiceSession())) {
        smInitialize();
    }

    if (serviceIsActive(spsmGetServiceSession())) {
        spsmExit();
    }
}

void GuiHekate::draw() {
    Gui::beginDraw();

    Gui::drawRectangle(0, 0, Gui::g_framebuffer_width, Gui::g_framebuffer_height, currTheme.backgroundColor);
    Gui::drawRectangle((u32)((Gui::g_framebuffer_width - 1220) / 2), 87, 1220, 1, currTheme.textColor);
    Gui::drawRectangle((u32)((Gui::g_framebuffer_width - 1220) / 2), Gui::g_framebuffer_height - 73, 1220, 1, currTheme.textColor);
    Gui::drawTextAligned(fontIcons, 70, 68, currTheme.textColor, "\uE130", ALIGNED_LEFT);
    Gui::drawTextAligned(font24, 70, 58, currTheme.textColor, "        酸菜鱼工具箱", ALIGNED_LEFT);
    Gui::drawTextAligned(font20, Gui::g_framebuffer_width - 50, Gui::g_framebuffer_height - 25, currTheme.textColor, "\uE0E1 返回     \uE0E0 确认", ALIGNED_RIGHT);

    // 只显示受限模式的选项
    Gui::drawTextAligned(font24, Gui::g_framebuffer_width / 2, 150, currTheme.activatedColor, "系统电源选项", ALIGNED_CENTER);
    Gui::drawTextAligned(font20, Gui::g_framebuffer_width / 2, 200, currTheme.textColor, "请选择以下电源操作选项:", ALIGNED_CENTER);

    drawButtons();

    Gui::endDraw();
}

void GuiHekate::onInput(u32 kdown) {
    inputButtons(kdown);

    if (kdown & HidNpadButton_B)
        Gui::g_nextGui = GUI_MAIN;
}