// Eclipse
// Copyright (C) 2025 ProunceDev
// MIT License

#include "eclipseEditBox.h"

wchar_t shiftChar(wchar_t ch) {
	switch(ch) {
		case L'1': return L'!';
		case L'2': return L'@';
		case L'3': return L'#';
		case L'4': return L'$';
		case L'5': return L'%';
		case L'6': return L'^';
		case L'7': return L'&';
		case L'8': return L'*';
		case L'9': return L'(';
		case L'0': return L')';
		case L'-': return L'_';
		case L'=': return L'+';
		case L'[': return L'{';
		case L']': return L'}';
		case L'\\': return L'|';
		case L';': return L':';
		case L'\'': return L'"';
		case L',': return L'<';
		case L'.': return L'>';
		case L'/': return L'?';
		case L'`': return L'~';
		default: return ch;
	}
}

unsigned int EclipseEditBox::nowMs()
{
    using namespace std::chrono;
    return (unsigned int)duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

EclipseEditBox::EclipseEditBox(gui::IGUIEnvironment* env)
{
    buffer = L"";
    caret = 0;
    selection_a = selection_b = 0;
    selAnchor = -1;
    rebuildLayoutNeeded = true;
    scrollY = 0;
    caretVisible = true;
    pushUndo();
	if (env)
		Operator = env->getOSOperator();

	if (Operator)
		Operator->grab();
}

void EclipseEditBox::pushUndo()
{
    UndoState s{ buffer, caret, selection_a, selection_b };
    if (undoStack.empty() || undoStack.back().text != s.text || undoStack.back().caret != s.caret) {
        undoStack.push_back(std::move(s));
        if (undoStack.size() > maxUndo) undoStack.pop_front();
        redoStack.clear();
    }
}

void EclipseEditBox::rebuildLogicalIndex()
{
    logicalLineStartIndex.clear();
    logicalLineStartIndex.push_back(0);
    for (size_t i = 0; i < buffer.size(); ++i) {
        if (buffer[i] == L'\n') {
            logicalLineStartIndex.push_back(i + 1);
        }
    }
    if (logicalLineStartIndex.empty()) logicalLineStartIndex.push_back(0);
}

void EclipseEditBox::rebuildLayoutIfNeeded(gui::IGUIFont* font)
{
    if (!rebuildLayoutNeeded && font == lastFont) return;
    lastFont = font;
    // compute line height
    if (font) {
        lineHeight = font->getDimension(L"A").Height + lineSpacing;
    } else {
        lineHeight = 16 + lineSpacing;
    }
	gutterWidth = font ? font->getDimension(L"00").Width + 8 : 32;
    computeWrappedLines(font);
    rebuildLayoutNeeded = false;
}

void EclipseEditBox::computeWrappedLines(gui::IGUIFont* font)
{
    wrappedLines.clear();
    rebuildLogicalIndex();

    if (!font) {
        // fallback: treat entire buffer as single wrapped line
        wrappedLines.push_back({0, 0, buffer});
        return;
    }

    // content width (exclude padding and gutter)
    s32 contentW = editorRect.getWidth() - gutterWidth - padding*2;
    if (contentW < 10) contentW = 10;

    // for each logical line, perform greedy word wrap
    size_t numLogLines = logicalLineStartIndex.size();
    for (size_t li = 0; li < numLogLines; ++li) {
        size_t startAbs = logicalLineStartIndex[li];
        size_t endAbs = (li + 1 < numLogLines) ? logicalLineStartIndex[li+1] - 1 : buffer.size();
        // build logical line string
        std::wstring logical;
        if (endAbs > startAbs) logical.assign(&buffer[startAbs], endAbs - startAbs);
        else logical.clear();

        if (logical.empty()) {
            wrappedLines.push_back({li, 0, L""});
            continue;
        }

        // split logical into words (keeping separators)
        std::vector<std::wstring> tokens;
        std::wstring cur;
        bool curIsWord = isWordChar(logical[0]);
        for (wchar_t ch : logical) {
            bool isW = isWordChar(ch);
            if (tokens.empty() && cur.empty()) { curIsWord = isW; cur.push_back(ch); continue; }
            if (isW == curIsWord) { cur.push_back(ch); }
            else { tokens.push_back(cur); cur.clear(); cur.push_back(ch); curIsWord = isW; }
        }
        if (!cur.empty()) tokens.push_back(cur);

        // now greedily pack tokens into wrapped lines
        size_t tokenIndexChar = 0;
        for (size_t t = 0; t < tokens.size();) {
            // try to fit tokens[t..k] into line
            size_t k = t;
            std::wstring trial;
            bool continuedEarly = false;
            while (k < tokens.size()) {
                std::wstring trialNext = trial + tokens[k];
                core::dimension2du dim = font->getDimension(trialNext.c_str());
                if ((int)dim.Width <= contentW) {
                    trial = trialNext;
                    ++k;
                } else {
                    // if nothing yet in trial and single token exceeds width, break token by char
                    if (trial.empty()) {
                        // take as many chars from tokens[k] as fit
                        std::wstring &tok = tokens[k];
                        size_t take = 0;
                        for (size_t c = 1; c <= tok.size(); ++c) {
                            core::dimension2du d2 = font->getDimension(tok.substr(0,c).c_str());
                            if ((int)d2.Width <= contentW) take = c;
                            else break;
                        }
                        if (take == 0) take = 1; // forcibly at least 1 char
                        std::wstring piece = tok.substr(0, take);
                        wrappedLines.push_back({li, tokenIndexChar, piece});
                        // mutate token to remove piece
                        tok.erase(0, take);
                        if (tok.empty()) ++k;
                        tokenIndexChar += take;
                        t = k;
                        continuedEarly = true;
                        break;
                    } else {
                        break;
                    }
                }
            }
            if (continuedEarly) continue;
            // fitted tokens [t..k-1]
            wrappedLines.push_back({li, tokenIndexChar, trial});
            // advance tokenIndexChar by length of those tokens
            size_t sumlen = 0;
            for (size_t p = t; p < k; ++p) sumlen += tokens[p].size();
            tokenIndexChar += sumlen;
            t = k;
        }
    }
}

void EclipseEditBox::absIndexToLogicalLineCol(size_t absIndex, size_t& outLine, size_t& outCol) const
{
    // clamp
    size_t idx = std::min(absIndex, buffer.size());
    // find logical line
    outLine = 0;
    for (size_t i = 0; i < logicalLineStartIndex.size(); ++i) {
        if (i+1 == logicalLineStartIndex.size() || logicalLineStartIndex[i+1] > idx) {
            outLine = i;
            break;
        }
    }
    outCol = idx - logicalLineStartIndex[outLine];
}

size_t EclipseEditBox::logicalLineColToAbsIndex(size_t line, size_t col) const
{
    if (logicalLineStartIndex.empty()) return 0;
    size_t l = std::min(line, logicalLineStartIndex.size() - 1);
    size_t c = col;
    // clamp col to end of line
    size_t lineStart = logicalLineStartIndex[l];
    size_t lineEnd = (l + 1 < logicalLineStartIndex.size()) ? logicalLineStartIndex[l+1] - 1 : buffer.size();
    size_t lineLen = (lineEnd > lineStart) ? (lineEnd - lineStart) : 0;
    if (c > lineLen) c = lineLen;
    return lineStart + c;
}

size_t EclipseEditBox::displayRowXToAbsIndex(gui::IGUIFont* font, int dispRow, int xInTextArea) const
{
    if (wrappedLines.empty()) return 0;
    int row = std::max(0, std::min(dispRow, (int)wrappedLines.size()-1));
    const WrappedLine& wl = wrappedLines[row];
    // measure char widths to find column
    size_t bestCol = wl.startChar;
    for (size_t i = 0; i <= wl.text.size(); ++i) {
        std::wstring sub = wl.text.substr(0, i);
        core::dimension2du d = font->getDimension(sub.c_str());
        if ((int)d.Width >= xInTextArea) { bestCol = wl.startChar + i; break; }
        bestCol = wl.startChar + i;
    }
    // convert to absolute index
    size_t absIdx = logicalLineStartIndex[wl.logicalLine] + bestCol;
    return absIdx;
}

void EclipseEditBox::absIndexToDisplayRowX(gui::IGUIFont* font, size_t absIndex, int& outRow, int& outX, int& outY) const
{
    // find logical line & col
    size_t li, col;
    absIndexToLogicalLineCol(absIndex, li, col);
    // find first wrapped row for logical line and scan
    for (size_t r = 0; r < wrappedLines.size(); ++r) {
        if (wrappedLines[r].logicalLine == li) {
            const WrappedLine &wl = wrappedLines[r];
            if (col >= wl.startChar && col <= wl.startChar + wl.text.size()) {
                outRow = (int)r;
                std::wstring sub = wl.text.substr(0, col - wl.startChar);
                outX = (int)font->getDimension(sub.c_str()).Width;
                outY = (int)r * (lineHeight);
                return;
            }
        }
    }
    // fallback: put at end
    outRow = (int)wrappedLines.size() - 1;
    outX = (int)font->getDimension(wrappedLines.back().text.c_str()).Width;
    outY = (int)outRow * (lineHeight);
}

void EclipseEditBox::insertAtCaret(const std::wstring& s)
{
    pushUndo();
    // delete selection if any
    size_t a = std::min(selection_a, selection_b);
    size_t b = std::max(selection_a, selection_b);
    if (a != b) {
        deleteRange(a, b);
        caret = a;
    }
    // insert s at caret
    buffer.insert(buffer.begin() + caret, s.begin(), s.end());
    caret += s.size();
    selection_a = selection_b = caret;
    rebuildLayoutNeeded = true;
}

void EclipseEditBox::deleteRange(size_t a, size_t b)
{
    if (b <= a) return;
    buffer.erase(buffer.begin() + a, buffer.begin() + b);
    caret = a;
    selection_a = selection_b = caret;
    rebuildLayoutNeeded = true;
}

std::wstring EclipseEditBox::substring(size_t a, size_t b) const
{
    if (b <= a) return L"";
    return std::wstring(buffer.begin() + a, buffer.begin() + b);
}

bool EclipseEditBox::isWordChar(wchar_t c)
{
    // treat alnum or underscore as word char; others are separators
    if ((c >= L'0' && c <= L'9') || (c >= L'A' && c <= L'Z') || (c >= L'a' && c <= L'z') || c == L'_') return true;
    // treat other unicode letters as word char
    return iswalpha(c) != 0;
}

void EclipseEditBox::moveCaretLeft(bool ctrl, bool shift)
{
    size_t newCaret = caret;
    if (ctrl) {
        if (caret == 0) newCaret = 0;
        else {
            // move to previous word boundary (within same logical line or previous line end)
            size_t li, col; absIndexToLogicalLineCol(caret, li, col);
            if (col == 0) {
                if (li == 0) newCaret = 0;
                else {
                    size_t prevLineStart = logicalLineStartIndex[li-1];
                    size_t prevLineLen = (logicalLineStartIndex[li] - prevLineStart);
                    newCaret = prevLineStart + prevLineLen;
                }
            } else {
                // scan left skipping spaces then word chars
                std::wstring ln;
                size_t start = logicalLineStartIndex[li];
                size_t end = (li+1 < logicalLineStartIndex.size()) ? logicalLineStartIndex[li+1]-1 : buffer.size();
                if (end > start) ln.assign(&buffer[start], end - start);
                else ln.clear();
                size_t pos = col;
                while (pos > 0 && iswspace(ln[pos-1])) pos--;
                while (pos > 0 && isWordChar(ln[pos-1])) pos--;
                newCaret = start + pos;
            }
        }
    } else {
        newCaret = (caret == 0 ? 0 : caret - 1);
    }
    if (shift) {
        if (selAnchor == -1) selAnchor = caret;
        caret = newCaret;
        selection_a = selAnchor;
        selection_b = caret;
    } else {
        caret = newCaret;
        selAnchor = -1;
        selection_a = selection_b = caret;
    }
    rebuildLayoutNeeded = true;
}

void EclipseEditBox::moveCaretRight(bool ctrl, bool shift)
{
    size_t newCaret = caret;
    if (ctrl) {
        if (caret >= buffer.size()) newCaret = buffer.size();
        else {
            size_t li, col; absIndexToLogicalLineCol(caret, li, col);
            size_t start = logicalLineStartIndex[li];
            size_t end = (li+1 < logicalLineStartIndex.size()) ? logicalLineStartIndex[li+1]-1 : buffer.size();
            std::wstring ln;
            if (end > start) ln.assign(&buffer[start], end - start);
            else ln.clear();
            size_t pos = col;
            size_t n = ln.size();
            while (pos < n && iswspace(ln[pos])) pos++;
            while (pos < n && isWordChar(ln[pos])) pos++;
            newCaret = start + pos;
            if (newCaret == caret) newCaret = std::min(buffer.size(), caret + 1);
        }
    } else {
        newCaret = std::min(buffer.size(), caret + 1);
    }
    if (shift) {
        if (selAnchor == -1) selAnchor = caret;
        caret = newCaret;
        selection_a = selAnchor;
        selection_b = caret;
    } else {
        caret = newCaret;
        selAnchor = -1;
        selection_a = selection_b = caret;
    }
    rebuildLayoutNeeded = true;
}

void EclipseEditBox::moveCaretUp(bool shift)
{
    rebuildLayoutNeeded = true;
    // get display row & x
    int row, xpix, ypix;
    absIndexToDisplayRowX(lastFont, caret, row, xpix, ypix);
    int target = std::max(0, row - 1);
    // find wrapped line target and compute abs index
    size_t idx = displayRowXToAbsIndex(lastFont, target, xpix);
    if (shift) {
        if (selAnchor == -1) selAnchor = caret;
        caret = idx;
        selection_a = selAnchor; selection_b = caret;
    } else {
        caret = idx;
        selAnchor = -1;
        selection_a = selection_b = caret;
    }
    ensureCaretVisible(lastFont);
}

void EclipseEditBox::moveCaretDown(bool shift)
{
    rebuildLayoutNeeded = true;
    int row, xpix, ypix;
    absIndexToDisplayRowX(lastFont, caret, row, xpix, ypix);
    int target = std::min((int)wrappedLines.size()-1, row + 1);
    size_t idx = displayRowXToAbsIndex(lastFont, target, xpix);
    if (shift) {
        if (selAnchor == -1) selAnchor = caret;
        caret = idx;
        selection_a = selAnchor; selection_b = caret;
    } else {
        caret = idx;
        selAnchor = -1;
        selection_a = selection_b = caret;
    }
    ensureCaretVisible(lastFont);
}

void EclipseEditBox::moveCaretHome(bool ctrl, bool shift)
{
    size_t newCaret = 0;
    if (ctrl) newCaret = 0;
    else {
        size_t li, col; absIndexToLogicalLineCol(caret, li, col);
        newCaret = logicalLineStartIndex[li];
    }
    if (shift) {
        if (selAnchor == -1) selAnchor = caret;
        caret = newCaret;
        selection_a = selAnchor; selection_b = caret;
    } else {
        caret = newCaret; selAnchor = -1; selection_a = selection_b = caret;
    }
    ensureCaretVisible(lastFont);
}

void EclipseEditBox::moveCaretEnd(bool ctrl, bool shift)
{
    size_t newCaret = buffer.size();
    if (!ctrl) {
        size_t li, col; absIndexToLogicalLineCol(caret, li, col);
        size_t end = (li+1 < logicalLineStartIndex.size()) ? logicalLineStartIndex[li+1]-1 : buffer.size();
        newCaret = end;
    }
    if (shift) {
        if (selAnchor == -1) selAnchor = caret;
        caret = newCaret;
        selection_a = selAnchor; selection_b = caret;
    } else {
        caret = newCaret; selAnchor = -1; selection_a = selection_b = caret;
    }
    ensureCaretVisible(lastFont);
}

void EclipseEditBox::deleteWordBack()
{
    size_t a = std::min(selection_a, selection_b);
    size_t b = std::max(selection_a, selection_b);
    if (a != b) {
        pushUndo(); deleteRange(a,b); rebuildLayoutNeeded = true; return;
    }
    if (caret == 0) return;
    size_t li, col; absIndexToLogicalLineCol(caret, li, col);
    size_t start = logicalLineStartIndex[li];
    size_t lineLen = ((li+1 < logicalLineStartIndex.size()) ? logicalLineStartIndex[li+1]-1 : buffer.size()) - start;
    std::wstring ln;
    if (lineLen > 0) ln.assign(&buffer[start], lineLen);
    else ln.clear();
    size_t pos = col;
    while (pos > 0 && iswspace(ln[pos-1])) pos--;
    while (pos > 0 && isWordChar(ln[pos-1])) pos--;
    size_t newAbs = start + pos;
    pushUndo();
    deleteRange(newAbs, caret);
    caret = newAbs;
    selection_a = selection_b = caret;
    rebuildLayoutNeeded = true;
}

void EclipseEditBox::deleteWordForward()
{
    size_t a = std::min(selection_a, selection_b);
    size_t b = std::max(selection_a, selection_b);
    if (a != b) { pushUndo(); deleteRange(a,b); rebuildLayoutNeeded = true; return; }
    if (caret >= buffer.size()) return;
    size_t li, col; absIndexToLogicalLineCol(caret, li, col);
    size_t start = logicalLineStartIndex[li];
    size_t end = (li+1 < logicalLineStartIndex.size()) ? logicalLineStartIndex[li+1]-1 : buffer.size();
    std::wstring ln;
    if (end > start) ln.assign(&buffer[start], end - start);
    else ln.clear();
    size_t pos = col;
    size_t n = ln.size();
    while (pos < n && iswspace(ln[pos])) pos++;
    while (pos < n && isWordChar(ln[pos])) pos++;
    size_t newAbs = start + pos;
    pushUndo();
    deleteRange(caret, newAbs);
    selection_a = selection_b = caret;
    rebuildLayoutNeeded = true;
}

void EclipseEditBox::ensureCaretVisible(gui::IGUIFont* font)
{
    if (!font) return;
    rebuildLayoutIfNeeded(font);
    int row, xpix, ypix;
    absIndexToDisplayRowX(font, caret, row, xpix, ypix);
    int contentH = editorRect.getHeight() - padding*2;
    int top = scrollY;
    int bottom = scrollY + contentH - lineHeight;
    int targetY = ypix;
    if (targetY < top) scrollY = std::max(0, targetY);
    else if (targetY > bottom) {
        int maxScroll = std::max<int>(0, (int)wrappedLines.size() * lineHeight - contentH);
        scrollY = std::min<int>(maxScroll, targetY - contentH + lineHeight);
    }
}

void EclipseEditBox::draw(video::IVideoDriver* driver, float dtime, gui::IGUIFont* font, const core::rect<s32>& clipRect, ColorTheme theme, s32 border_width)
{
    caretTimer += dtime * 1000.0f;
    if (caretTimer >= (float)caretBlinkMs) {
        caretTimer = 0.0f;
        caretVisible = !caretVisible;
    }

    lastFont = font;
    rebuildLayoutIfNeeded(font);

    // basic box with border
    if (driver) {
        driver->draw2DRoundedRectangle(editorRect, theme.background_bottom, border_width, &clipRect);
        // draw gutter
        core::rect<s32> gutterRect(
			editorRect.UpperLeftCorner.X, 
			editorRect.UpperLeftCorner.Y,
            editorRect.UpperLeftCorner.X + gutterWidth, 
			editorRect.LowerRightCorner.Y
		);

        driver->draw2DRoundedRectangle(gutterRect, theme.background_top, border_width, &clipRect);

        // border
        video::SColor bcol = active ? theme.primary : theme.border;
        driver->draw2DRoundedRectangleOutline(editorRect, bcol, border_width, border_width, &clipRect);
    }

    // draw the wrapped text and selection
    drawTextContent(driver, font, clipRect, theme, border_width);
}

void EclipseEditBox::drawTextContent(video::IVideoDriver* driver, gui::IGUIFont* font, const core::rect<s32>& clipRect, ColorTheme theme, s32 border_width)
{
    if (!font || !driver) return;
    // content rect (left/top of text)
    core::rect<s32> contentRect(
        editorRect.UpperLeftCorner.X + gutterWidth + padding,
        editorRect.UpperLeftCorner.Y + padding,
        editorRect.LowerRightCorner.X - padding,
        editorRect.LowerRightCorner.Y - padding
    );

	// Create text clip rect from editor rect and clip rect
	core::rect<s32> textClipRect(
		core::max_(editorRect.UpperLeftCorner.X + border_width, clipRect.UpperLeftCorner.X),
		core::max_(editorRect.UpperLeftCorner.Y + border_width, clipRect.UpperLeftCorner.Y),
		core::min_(editorRect.LowerRightCorner.X - border_width, clipRect.LowerRightCorner.X),
		core::min_(editorRect.LowerRightCorner.Y - border_width, clipRect.LowerRightCorner.Y)
	);

    int firstVisibleRow = scrollY / lineHeight;
    int rowsVisible = (contentRect.getHeight() / lineHeight) + 2;
    int lastVisibleRow = std::min((int)wrappedLines.size()-1, firstVisibleRow + rowsVisible);

    // draw each visible row
    int y = contentRect.UpperLeftCorner.Y - (scrollY % lineHeight);
    for (int r = firstVisibleRow; r <= lastVisibleRow; ++r) {
        if (r < 0 || r >= (int)wrappedLines.size()) break;
        const WrappedLine& wl = wrappedLines[r];
        size_t absStart = logicalLineStartIndex[wl.logicalLine] + wl.startChar;
        size_t absEnd = absStart + wl.text.size();

        size_t selA = std::min(selection_a, selection_b);
        size_t selB = std::max(selection_a, selection_b);

        if (selA < selB && !(selB <= absStart || selA >= absEnd)) {
            size_t localA = (selA <= absStart) ? 0 : (selA - absStart);
            size_t localB = (selB >= absEnd) ? (absEnd - absStart) : (selB - absStart);
            std::wstring leftPart = wl.text.substr(0, localA);
            std::wstring selPart = wl.text.substr(localA, localB - localA);
            int selX = contentRect.UpperLeftCorner.X + (int)font->getDimension(leftPart.c_str()).Width;
            int selW = (int)font->getDimension(selPart.c_str()).Width;
            core::rect<s32> selRect(selX, y, selX + selW, y + lineHeight);

            // draw selection rectangle
            driver->draw2DRectangle(theme.secondary, selRect, &textClipRect);
        }

        // draw text
        core::rect<s32> targetRect(contentRect.UpperLeftCorner.X, y, contentRect.LowerRightCorner.X, y + lineHeight);
        font->draw(wl.text.c_str(), targetRect, theme.text, false, false, &textClipRect);

        // draw gutter number on first wrapped line of logical line
        if (wl.startChar == 0) {
            std::wstring num = std::to_wstring((long long)wl.logicalLine + 1);
            core::rect<s32> gutterTextRect(editorRect.UpperLeftCorner.X + 4, y, editorRect.UpperLeftCorner.X + gutterWidth - 6, y + lineHeight);
            font->draw(num.c_str(), gutterTextRect, theme.text_muted, false, false, &textClipRect);
        }
        y += lineHeight;
    }

    // caret
    if (active && caretVisible) {
        int row, caretX, caretY;
        absIndexToDisplayRowX(font, caret, row, caretX, caretY);
        // only draw if in visible window
        if (row >= firstVisibleRow && row <= lastVisibleRow) {
            int screenY = contentRect.UpperLeftCorner.Y + (row - firstVisibleRow) * lineHeight - (scrollY % lineHeight);
            int screenX = contentRect.UpperLeftCorner.X + caretX;
            core::rect<s32> caretRect(screenX, screenY, screenX + (s32)caretWidth, screenY + lineHeight);
            driver->draw2DRectangle(theme.text, caretRect, &textClipRect);
        }
    }
}

bool EclipseEditBox::handleEvent(const SEvent& event)
{
	if (!active && event.EventType != EET_MOUSE_INPUT_EVENT)
		return false;
	
    bool consumed = false;

    rebuildLayoutNeeded = true;

    if (event.EventType == EET_MOUSE_INPUT_EVENT) {
        const auto& m = event.MouseInput;

        if (m.Event == EMIE_LMOUSE_PRESSED_DOWN) {
            core::position2di p(m.X, m.Y);
            mouseDownPos = p;           
            pendingFocus = editorRect.isPointInside(p); 
            pendingClick = pendingFocus;
            dragging = false;
            clickMovedTooFar = false;
        } 
        else if (m.Event == EMIE_MOUSE_MOVED) {
            if (pendingClick) {
                core::position2di current(m.X, m.Y);
                if ((current - mouseDownPos).getLengthSQ() > 10){
					clickMovedTooFar = true;
				}
            }
            if (dragging) {
				int localX = m.X - (editorRect.UpperLeftCorner.X + gutterWidth + padding);
				int localY = m.Y - (editorRect.UpperLeftCorner.Y + padding) + scrollY;

				float rowF = (float)localY / lineHeight;
				int targetRow = (int)(rowF + 0.5f);
				targetRow = std::max(0, std::min((int)wrappedLines.size()-1, targetRow));

				size_t newAbs = displayRowXToAbsIndex(lastFont, targetRow, localX);

				if (selAnchor == -1) selAnchor = caret;
				caret = newAbs;
				selection_a = selAnchor;
				selection_b = caret;

				ensureCaretVisible(lastFont);
				consumed = true;
			}

        } 
        else if (m.Event == EMIE_LMOUSE_LEFT_UP) {
            core::position2di p(m.X, m.Y);

            if (pendingClick && !clickMovedTooFar && editorRect.isPointInside(p)) {
                // Activate only on release, if mouse didn't move too far
                active = true;

                // click count (multi-click)
                u32 now = nowMs();
                if (now - lastClickTime < 400) ++clickCount; else clickCount = 1;
                lastClickTime = now;

                // compute click position relative to text area
                int localX = m.X - (editorRect.UpperLeftCorner.X + gutterWidth + padding);
                int localY = m.Y - (editorRect.UpperLeftCorner.Y + padding) + scrollY;
                int row = localY / lineHeight;
                row = std::max(0, std::min((int)wrappedLines.size()-1, row));
                size_t newAbs = displayRowXToAbsIndex(lastFont, row, localX);

                if (clickCount == 1) {
                    selAnchor = -1;
                    caret = newAbs;
                    selection_a = selection_b = caret;
                    dragging = true;
                } else if (clickCount == 2) {
                    // double click: select word
                    size_t li, col; absIndexToLogicalLineCol(newAbs, li, col);
                    size_t start = logicalLineStartIndex[li];
                    size_t end = (li+1 < logicalLineStartIndex.size()) ? logicalLineStartIndex[li+1]-1 : buffer.size();
                    std::wstring lineStr;
                    if (end > start) lineStr.assign(&buffer[start], end - start);
                    else lineStr.clear();
                    if (lineStr.empty()) { selAnchor = -1; caret = start; selection_a = selection_b = caret; }
                    else {
                        size_t pos = col;
                        if (pos >= lineStr.size()) pos = lineStr.size() ? lineStr.size()-1 : 0;
                        // expand to word bounds
                        size_t L = pos;
                        while (L > 0 && isWordChar(lineStr[L-1])) L--;
                        size_t R = pos;
                        while (R < lineStr.size() && isWordChar(lineStr[R])) R++;
                        size_t a = start + L;
                        size_t b = start + R;
                        selAnchor = a;
                        caret = b;
                        selection_a = a; selection_b = b;
                    }
                } else {
                    // triple click: select logical line
                    size_t li, col; absIndexToLogicalLineCol(newAbs, li, col);
                    size_t a = logicalLineStartIndex[li];
                    size_t b = (li+1 < logicalLineStartIndex.size()) ? logicalLineStartIndex[li+1]-1 : buffer.size();
                    selAnchor = a;
                    caret = b;
                    selection_a = a; selection_b = b;
                }
                ensureCaretVisible(lastFont);
                // Don't consume so that parent properly releases dragging
            }

            pendingClick = false;
            dragging = false;
        } 
        else if (m.Event == EMIE_MOUSE_WHEEL) {
            int delta = (int)m.Wheel;
            int contentH = editorRect.getHeight() - padding*2;
            int step = lineHeight * 3;
            int maxScroll = std::max<int>(0, (int)wrappedLines.size()*lineHeight - contentH);
            scrollY = std::max(0, std::min(maxScroll, scrollY - delta * step));
            consumed = true;
        }
    } 
    else if (event.EventType == EET_KEY_INPUT_EVENT) {
        const auto& k = event.KeyInput;
        if (!k.PressedDown) return consumed;
        bool ctrl = k.Control;
        bool shift = k.Shift;

        // navigation / shortcuts
        if (k.Key == KEY_LEFT) { moveCaretLeft(ctrl, shift); consumed = true; }
        else if (k.Key == KEY_RIGHT) { moveCaretRight(ctrl, shift); consumed = true; }
        else if (k.Key == KEY_UP) { moveCaretUp(shift); consumed = true; }
        else if (k.Key == KEY_DOWN) { moveCaretDown(shift); consumed = true; }
        else if (k.Key == KEY_HOME) { moveCaretHome(ctrl, shift); consumed = true; }
        else if (k.Key == KEY_END) { moveCaretEnd(ctrl, shift); consumed = true; }
        else if (k.Key == KEY_PRIOR) { // PageUp
            int contentH = editorRect.getHeight() - padding*2;
            scrollY = std::max(0, scrollY - contentH);
            consumed = true;
			ensureCaretVisible(lastFont);
        }
        else if (k.Key == KEY_NEXT) { // PageDown
            int contentH = editorRect.getHeight() - padding*2;
            int maxScroll = std::max<int>(0, (int)wrappedLines.size()*lineHeight - contentH);
            scrollY = std::min(maxScroll, scrollY + contentH);
            consumed = true;
			ensureCaretVisible(lastFont);
        }
        else if (ctrl && (k.Key == KEY_KEY_A)) { // Ctrl+A
            selAnchor = 0; caret = buffer.size(); selection_a = 0; selection_b = caret; consumed = true;
        }
        else if (ctrl && (k.Key == KEY_KEY_C)) { // Ctrl+C
            size_t a = std::min(selection_a, selection_b), b = std::max(selection_a, selection_b);
            if (a < b) {
                if (Operator) {
                    std::string utf8 = wide_to_utf8(substring(a,b));
                    Operator->copyToClipboard(utf8.c_str());
                }
            }
            consumed = true;
        }
        else if (ctrl && (k.Key == KEY_KEY_X)) { // Ctrl+X
            size_t a = std::min(selection_a, selection_b), b = std::max(selection_a, selection_b);
            if (a < b) {
                if (clipboardSet) clipboardSet(substring(a,b));
                pushUndo();
                deleteRange(a,b);
                consumed = true;
            }
			ensureCaretVisible(lastFont);
        }
        else if (ctrl && (k.Key == KEY_KEY_V)) { // Ctrl+V
			const c8 *p = Operator->getTextFromClipboard();
			std::wstring s = utf8_to_wide(p ? p : "");
			if (!s.empty()) { insertAtCaret(s); consumed = true; }
			ensureCaretVisible(lastFont);
        }
        else if (ctrl && (k.Key == KEY_KEY_Z)) { // Ctrl+Z
            if (undoStack.size() > 1) {
                redoStack.push_back({buffer, caret, selection_a, selection_b});
                UndoState last = undoStack.back();
                undoStack.pop_back();
                if (!undoStack.empty()) {
                    UndoState prev = undoStack.back();
                    buffer = prev.text; caret = prev.caret; selection_a = prev.sa; selection_b = prev.sb;
                    rebuildLayoutNeeded = true;
                }
            }
            consumed = true;
        }
        else if (ctrl && (k.Key == KEY_KEY_Y)) { // Ctrl+Y
            if (!redoStack.empty()) {
                UndoState s = redoStack.back(); redoStack.pop_back();
                pushUndo();
                buffer = s.text; caret = s.caret; selection_a = s.sa; selection_b = s.sb;
                rebuildLayoutNeeded = true;
            }
            consumed = true;
			ensureCaretVisible(lastFont);
        }
        else if ((k.Key == KEY_BACK)) { // Backspace / Ctrl+Backspace
            if (ctrl) deleteWordBack();
            else {
                size_t a = std::min(selection_a, selection_b), b = std::max(selection_a, selection_b);
                if (a < b) { pushUndo(); deleteRange(a,b); consumed = true; }
                else if (caret > 0) { pushUndo(); deleteRange(caret-1, caret); consumed = true; }
            }
			ensureCaretVisible(lastFont);
        }
        else if (k.Key == KEY_DELETE) { // Delete / Ctrl+Delete
            if (ctrl) deleteWordForward();
            else {
                size_t a = std::min(selection_a, selection_b), b = std::max(selection_a, selection_b);
                if (a < b) { pushUndo(); deleteRange(a,b); consumed = true; }
                else if (caret < buffer.size()) { pushUndo(); deleteRange(caret, caret+1); consumed = true; }
            }
			ensureCaretVisible(lastFont);
        }
        else if (k.Key == KEY_RETURN) {
            insertAtCaret(std::wstring(1, L'\n')); consumed = true;
			ensureCaretVisible(lastFont);
        }
        else if (k.Key == KEY_TAB) {
            std::wstring tab(tabSpaces, L' ');
            insertAtCaret(tab); consumed = true;
			ensureCaretVisible(lastFont);
        }
		else if (k.Key == KEY_ESCAPE) {
			if (active) {
				selection_a = selection_b = caret;
				selAnchor = -1;
				active = false;
				consumed = true;
			}
		}
        else {
			wchar_t ch = (wchar_t)k.Char;
			if (ch != 0 && !k.Control) {
				if (shift) {
					// Convert letters to uppercase
					if (iswalpha(ch)) ch = towupper(ch);
					else ch = shiftChar(ch); // Convert symbols
				}
				std::wstring s(1, ch);
				insertAtCaret(s);
				consumed = true;
			}
        }

        caretVisible = true;
        caretTimer = 0.0f;
    }

    return consumed;
}
