// The Pip-Boy's actions on a pad (docs/swf_framework.md#pipboy-pad). Every pad button is the Pip-Boy's own but View
// (read from the game's Pip-Boy movies: A, B, X, Y, the shoulders, triggers, stick clicks and D-pad all act), so
// View, released while the Pip-Boy is open and a mod uses Repair or Ammo, calls CRUI_PipboyActions.Open, which opens
// our action menu (Interface/CRUI_PipboyActions.swf) over the Pip-Boy. The menu lists the actions in use; once one is
// picked it hands the D-pad, A and B to the open list as the Pip-Boy's own keys (PipboyKey), and closes as the list
// does. On top, the menu takes the pad, so the inventory page under it doesn't also act.
#include "condition.h"

#include <cstdint>
#include <string>

namespace crui {

namespace {

const char *PIPBOY = "PipboyMenu", *ACTIONS = "CRUI_PipboyActions";

// View on a pad, as MenuControls passes it to each input handler.
class PadHandler : public RE::BSInputEventUser {
public:
    bool ShouldHandleEvent(const RE::InputEvent *event) override {
        const auto *button = event ? event->As<RE::ButtonEvent>() : nullptr;
        return button && button->GetBSButtonCode() == RE::BS_BUTTON_CODE::kBack && button->QReleased();
    }

    // View released: open the action menu while the Pip-Boy is open, a mod uses one of its actions and the menu
    // isn't open already.
    void OnButtonEvent(const RE::ButtonEvent *) override {
        RE::UI *ui = RE::UI::GetSingleton();
        if (!ui || !ui->GetMenuOpen(PIPBOY) || ui->GetMenuOpen(ACTIONS) || (!RepairInUse() && !AmmoInUse())) return;
        RE::GameVM *game = RE::GameVM::GetSingleton();
        auto vm = game ? game->GetVM() : nullptr;
        RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> none;
        bool sent = vm && vm->DispatchStaticCall(ACTIONS, "Open", none);
        REX::DEBUG("Pip-Boy: View on the pad, {}", sent ? "opening the action menu" : "the action menu can't open");
    }
};
PadHandler g_padHandler;

// An action row {text, key}: its words, and the Pip-Boy key that opens its list.
Value Action(Movie *movie, const char *text, OurKey key) {
    Value row;
    movie->CreateObject(&row);
    row.SetMember("text", String(movie, text));
    row.SetMember("key", std::int32_t(KeyCode(key)));
    return row;
}

// PipboyActions(): the actions in use, [{text, key}]: Repair while a mod tracks an item, Ammo while one gives ammo
// lists.
class ActionsNative : public FunctionHandler {
public:
    void Call(const Params &params) override {
        params.movie->CreateArray(params.retVal);
        if (RepairInUse()) params.retVal->PushBack(Action(params.movie, "$CRUI_REPAIR", OurKey::Repair));
        if (AmmoInUse()) params.retVal->PushBack(Action(params.movie, "$CRUI_AMMO", OurKey::Ammo));
    }
};

// PipboyPress(key): Pip-Boy key `key` (a Flash key code) on the open Pip-Boy, as if typed there; whether a list is
// open after it.
class PressNative : public FunctionHandler {
public:
    void Call(const Params &params) override {
        Movie *pipboy = MenuMovie(PIPBOY);
        if (pipboy && params.argCount > 0) PipboyKey(pipboy, int(Num(params.args[0])));
        *params.retVal = RepairListOpen() || AmmoListOpen();
    }
};

// PipboyListOpen(): whether the repair or ammo list is open.
class ListOpenNative : public FunctionHandler {
public:
    void Call(const Params &params) override {
        *params.retVal = RepairListOpen() || AmmoListOpen();
    }
};

// PipboyActionsDone(): close the action menu.
class DoneNative : public FunctionHandler {
public:
    void Call(const Params &) override {
        if (RE::UIMessageQueue *queue = RE::UIMessageQueue::GetSingleton())
            queue->AddMessage(ACTIONS, RE::UI_MESSAGE_TYPE::kHide);
    }
};

}  // namespace

void InstallPad() {
    static bool installed = false;
    RE::MenuControls *controls = installed ? nullptr : RE::MenuControls::GetSingleton();
    if (controls) controls->RegisterHandler(&g_padHandler);
    installed = installed || controls;
    REX::INFO("Pip-Boy on a pad: {}", installed ? "View opens the action menu" : "no input handler");
}

void RegisterPad(Movie *movie, Value *plugin) {
    RegisterFunction<ActionsNative>(plugin, movie, "PipboyActions");
    RegisterFunction<PressNative>(plugin, movie, "PipboyPress");
    RegisterFunction<ListOpenNative>(plugin, movie, "PipboyListOpen");
    RegisterFunction<DoneNative>(plugin, movie, "PipboyActionsDone");
}

}  // namespace crui
