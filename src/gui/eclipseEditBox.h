// Eclipse
// Copyright (C) 2025 ProunceDev
// MIT License

#pragma once

#include <irrlicht.h>
#include <iostream>
#include <vector>
#include <codecvt> 
#include <locale> 
#include <IGUIEditBox.h>
#include <IGUIEnvironment.h>
#include <IVideoDriver.h>
#include <IGUIFont.h>
#include <chrono>
#include <cmath>

#include "client/color_theme.h"
#include "script/scripting_client.h"
#include "client/client.h"
#include "log.h"
#include "gui/modalMenu.h"
#include "settings.h"
#include "porting.h"
#include "filesys.h"
#include "scripting_mainmenu.h"
#include "client/texturesource.h"
#include "IOSOperator.h"

class EclipseEditBox
{
public:
    EclipseEditBox(gui::IGUIEnvironment* env);
    ~EclipseEditBox() = default;

    void setEditorRect(const core::rect<s32>& r) { editorRect = r; rebuildLayoutNeeded = true; }

    void draw(video::IVideoDriver* driver, float dtime, gui::IGUIFont* font, const core::rect<s32>& clipRect, ColorTheme theme, s32 border_width);

    bool handleEvent(const SEvent& event);

    void setTextUtf8(const std::string& utf8) { buffer = utf8_to_wide(utf8); rebuildLayoutNeeded = true; caret = 0; selection_a = selection_b = 0; }
	
	void loseFocus() { active = false; dragging = false; pendingClick = false; clickMovedTooFar = false; selection_a = selection_b = 0; }
    std::string getTextUtf8() const { return wide_to_utf8(buffer); }

    u32 caretWidth = 2;
    u32 caretBlinkMs = 530;
    s32 padding = 8;
    s32 gutterWidth = 48;
    s32 lineSpacing = 4;
    s32 tabSpaces = 4;

private:
    std::wstring buffer;
    core::rect<s32> editorRect;
    gui::IGUIFont* lastFont = nullptr;
	IOSOperator *Operator;

    size_t caret = 0;
    size_t selection_a = 0;
    size_t selection_b = 0;
    s32 selAnchor = -1;

    s32 scrollY = 0;

    struct WrappedLine { size_t logicalLine; size_t startChar; std::wstring text; };
    std::vector<WrappedLine> wrappedLines;
    std::vector<size_t> logicalLineStartIndex;

    bool rebuildLayoutNeeded = true;
    u32 lineHeight = 0;

    bool active = false;
    bool dragging = false;
    u32 lastClickTime = 0;
    int clickCount = 0;
    core::position2di mouseDownPos;
    bool pendingClick = false;
    bool pendingFocus = false;
    bool clickMovedTooFar = false;

    float caretTimer = 0.0f;
    bool caretVisible = true;

    struct UndoState { std::wstring text; size_t caret; size_t sa; size_t sb; };
    std::deque<UndoState> undoStack;
    std::deque<UndoState> redoStack;
    size_t maxUndo = 200;
    void pushUndo();

    std::function<void(const std::wstring&)> clipboardSet;
    std::function<std::wstring()> clipboardGet;

    static std::wstring utf8_to_wstring(const std::string& s);
    static std::string  wstring_to_utf8(const std::wstring& ws);

    void rebuildLogicalIndex();
    void rebuildLayoutIfNeeded(gui::IGUIFont* font);

    void computeWrappedLines(gui::IGUIFont* font);

    void absIndexToLogicalLineCol(size_t absIndex, size_t& outLine, size_t& outCol) const;
    size_t logicalLineColToAbsIndex(size_t line, size_t col) const;

    size_t displayRowXToAbsIndex(gui::IGUIFont* font, int dispRow, int xInTextArea) const;
    void absIndexToDisplayRowX(gui::IGUIFont* font, size_t absIndex, int& outRow, int& outX, int& outY) const;

    void insertAtCaret(const std::wstring& s);
    void deleteRange(size_t a, size_t b);
    std::wstring substring(size_t a, size_t b) const;

    void moveCaretLeft(bool ctrl, bool shift);
    void moveCaretRight(bool ctrl, bool shift);
    void moveCaretUp(bool shift);
    void moveCaretDown(bool shift);
    void moveCaretHome(bool ctrl, bool shift);
    void moveCaretEnd(bool ctrl, bool shift);

    void deleteWordBack();
    void deleteWordForward();

    void setSelection(size_t a, size_t b) { selection_a = a; selection_b = b; }
    void clearSelection() { selection_a = selection_b = caret; selAnchor = -1; }

    void ensureCaretVisible(gui::IGUIFont* font);

    static bool isWordChar(wchar_t c);
    static unsigned int nowMs();

    void drawTextContent(video::IVideoDriver* driver, gui::IGUIFont* font, const core::rect<s32>& clipRect, ColorTheme theme, s32 border_width);
};