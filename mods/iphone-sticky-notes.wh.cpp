// ==WindhawkMod==
// @id              iphone-sticky-notes
// @name            iPhone Sticky Notes
// @description     Desktop sticky notes with a modern iPhone liquid-glass look, editable text, colors, drag, autosave, and a small add-note button.
// @version         1.0
// @author          iphone-sticky-notes contributors
// @github          https://github.com/iphone-sticky-notes
// @include         explorer.exe
// @compilerOptions -lgdi32 -luser32 -lshell32 -lmsimg32
// @license         MIT
// ==/WindhawkMod==

// ==WindhawkModReadme==
/*
# iPhone Sticky Notes

Adds editable sticky notes to the Windows desktop from Windhawk. Install this file as a
Windhawk mod, enable it for Explorer, and use the floating **+** button to create notes.

Features:
- Modern iPhone-inspired liquid-glass cards and translucent floating add button.
- Type directly inside each note.
- Drag notes by their header.
- Cycle colors with the palette button.
- Autosaves note position, color, and text in `%APPDATA%\\WindhawkStickyNotes\\notes.ini`.
*/
// ==/WindhawkModReadme==

#include <windows.h>
#include <windowsx.h>
#include <shlobj.h>

#include <string>
#include <vector>

constexpr int kNoteWidth = 280;
constexpr int kNoteHeight = 260;
constexpr int kHeaderHeight = 38;
constexpr UINT_PTR kAutosaveTimer = 42;
constexpr UINT kSaveMessage = WM_APP + 25;

struct NoteWindow {
    HWND hwnd = nullptr;
    HWND edit = nullptr;
    COLORREF color = RGB(255, 230, 109);
    int id = 0;
    HBRUSH brush = nullptr;
};

HINSTANCE g_instance;
HWND g_launcher = nullptr;
HANDLE g_thread = nullptr;
DWORD g_threadId = 0;
bool g_running = true;
int g_nextId = 1;
std::vector<NoteWindow*> g_notes;
const COLORREF g_colors[] = {
    RGB(255, 230, 109), RGB(149, 225, 211), RGB(243, 139, 168),
    RGB(174, 214, 241), RGB(255, 204, 188), RGB(210, 180, 222)
};

std::wstring GetStoragePath() {
    wchar_t appData[MAX_PATH]{};
    if (FAILED(SHGetFolderPathW(nullptr, CSIDL_APPDATA, nullptr, SHGFP_TYPE_CURRENT, appData))) {
        lstrcpyW(appData, L".");
    }
    std::wstring dir = std::wstring(appData) + L"\\WindhawkStickyNotes";
    CreateDirectoryW(dir.c_str(), nullptr);
    return dir + L"\\notes.ini";
}

std::wstring EscapeIni(std::wstring value) {
    std::wstring out;
    for (wchar_t ch : value) {
        if (ch == L'\r') continue;
        if (ch == L'\n') out += L"\\n";
        else if (ch == L'\\') out += L"\\\\";
        else out += ch;
    }
    return out;
}

std::wstring UnescapeIni(const wchar_t* value) {
    std::wstring out;
    for (const wchar_t* p = value; *p; ++p) {
        if (*p == L'\\' && p[1]) {
            ++p;
            out += (*p == L'n') ? L'\n' : *p;
        } else {
            out += *p;
        }
    }
    return out;
}

void SaveNotes() {
    std::wstring path = GetStoragePath();
    DeleteFileW(path.c_str());
    wchar_t count[16];
    wsprintfW(count, L"%u", static_cast<unsigned>(g_notes.size()));
    WritePrivateProfileStringW(L"meta", L"count", count, path.c_str());

    for (size_t i = 0; i < g_notes.size(); ++i) {
        NoteWindow* note = g_notes[i];
        wchar_t section[32];
        wsprintfW(section, L"note%u", static_cast<unsigned>(i));
        RECT rc{};
        GetWindowRect(note->hwnd, &rc);
        wchar_t number[32];
        wsprintfW(number, L"%ld", rc.left);
        WritePrivateProfileStringW(section, L"x", number, path.c_str());
        wsprintfW(number, L"%ld", rc.top);
        WritePrivateProfileStringW(section, L"y", number, path.c_str());
        wsprintfW(number, L"%u", note->color);
        WritePrivateProfileStringW(section, L"color", number, path.c_str());

        int len = GetWindowTextLengthW(note->edit);
        std::vector<wchar_t> buffer(static_cast<size_t>(len) + 1);
        GetWindowTextW(note->edit, buffer.data(), len + 1);
        WritePrivateProfileStringW(section, L"text", EscapeIni(buffer.data()).c_str(), path.c_str());
    }
}


void FillVerticalGradient(HDC hdc, const RECT& rect, COLORREF top, COLORREF bottom) {
    TRIVERTEX vertices[2] = {
        {rect.left, rect.top,
         static_cast<COLOR16>(GetRValue(top) << 8),
         static_cast<COLOR16>(GetGValue(top) << 8),
         static_cast<COLOR16>(GetBValue(top) << 8), 0xffff},
        {rect.right, rect.bottom,
         static_cast<COLOR16>(GetRValue(bottom) << 8),
         static_cast<COLOR16>(GetGValue(bottom) << 8),
         static_cast<COLOR16>(GetBValue(bottom) << 8), 0xffff}
    };
    GRADIENT_RECT gradientRect{0, 1};
    GradientFill(hdc, vertices, 2, &gradientRect, 1, GRADIENT_FILL_RECT_V);
}

COLORREF BlendColor(COLORREF first, COLORREF second, int percentSecond) {
    int percentFirst = 100 - percentSecond;
    return RGB(
        (GetRValue(first) * percentFirst + GetRValue(second) * percentSecond) / 100,
        (GetGValue(first) * percentFirst + GetGValue(second) * percentSecond) / 100,
        (GetBValue(first) * percentFirst + GetBValue(second) * percentSecond) / 100);
}

void DrawLiquidGlassCard(HDC hdc, NoteWindow* note) {
    RECT card{0, 0, kNoteWidth, kNoteHeight};
    COLORREF softBody = BlendColor(note->color, RGB(255, 255, 255), 36);
    COLORREF softBottom = BlendColor(note->color, RGB(210, 232, 255), 28);
    FillVerticalGradient(hdc, card, softBody, softBottom);

    HPEN borderPen = CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
    HGDIOBJ oldPen = SelectObject(hdc, borderPen);
    HGDIOBJ oldBrush = SelectObject(hdc, GetStockObject(HOLLOW_BRUSH));
    RoundRect(hdc, 1, 1, kNoteWidth - 1, kNoteHeight - 1, 28, 28);
    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(borderPen);

    RECT topGlow{8, 7, kNoteWidth - 8, 72};
    FillVerticalGradient(hdc, topGlow, RGB(255, 255, 255), BlendColor(note->color, RGB(255, 255, 255), 70));

    HBRUSH shine = CreateSolidBrush(RGB(255, 255, 255));
    RECT shineLine{20, 12, kNoteWidth - 52, 16};
    FillRect(hdc, &shineLine, shine);
    RECT shineDot{22, 25, 82, 31};
    FillRect(hdc, &shineDot, shine);
    DeleteObject(shine);
}

void ApplyEditColor(NoteWindow* note) {
    if (note->brush) {
        DeleteObject(note->brush);
    }
    note->brush = CreateSolidBrush(BlendColor(note->color, RGB(255, 255, 255), 30));
    InvalidateRect(note->hwnd, nullptr, TRUE);
    InvalidateRect(note->edit, nullptr, TRUE);
}

LRESULT CALLBACK NoteProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    NoteWindow* note = reinterpret_cast<NoteWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    switch (msg) {
    case WM_NCCREATE:
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(((CREATESTRUCTW*)lParam)->lpCreateParams));
        return TRUE;
    case WM_CREATE: {
        note = reinterpret_cast<NoteWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        note->edit = CreateWindowExW(0, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL,
                                    14, kHeaderHeight, kNoteWidth - 28, kNoteHeight - kHeaderHeight - 14,
                                    hwnd, reinterpret_cast<HMENU>(1001), g_instance, nullptr);
        SendMessageW(note->edit, WM_SETFONT, reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)), TRUE);
        ApplyEditColor(note);
        HRGN region = CreateRoundRectRgn(0, 0, kNoteWidth + 1, kNoteHeight + 1, 28, 28);
        SetWindowRgn(hwnd, region, TRUE);
        SetTimer(hwnd, kAutosaveTimer, 2500, nullptr);
        return 0;
    }
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLOREDIT:
        SetBkColor(reinterpret_cast<HDC>(wParam), BlendColor(note->color, RGB(255, 255, 255), 30));
        SetTextColor(reinterpret_cast<HDC>(wParam), RGB(35, 35, 38));
        if (!note->brush) {
            note->brush = CreateSolidBrush(BlendColor(note->color, RGB(255, 255, 255), 30));
        }
        return reinterpret_cast<LRESULT>(note->brush);
    case WM_LBUTTONDOWN:
        if (GET_Y_LPARAM(lParam) < kHeaderHeight && GET_X_LPARAM(lParam) < kNoteWidth - 68) {
            SendMessageW(hwnd, WM_NCLBUTTONDOWN, HTCAPTION, 0);
        }
        return 0;
    case WM_COMMAND:
        if (HIWORD(wParam) == EN_CHANGE) PostMessageW(hwnd, kSaveMessage, 0, 0);
        return 0;
    case WM_TIMER:
    case kSaveMessage:
        SaveNotes();
        return 0;
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        DrawLiquidGlassCard(hdc, note);
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(70, 70, 76));
        RECT title{14, 0, kNoteWidth - 74, kHeaderHeight};
        DrawTextW(hdc, L"Liquid Note", -1, &title, DT_VCENTER | DT_SINGLELINE);
        RECT colorButton{kNoteWidth - 62, 8, kNoteWidth - 38, 30};
        HBRUSH colorBrush = CreateSolidBrush(note->color);
        FillRect(hdc, &colorButton, colorBrush);
        Rectangle(hdc, colorButton.left, colorButton.top, colorButton.right, colorButton.bottom);
        DeleteObject(colorBrush);
        RECT closeButton{kNoteWidth - 32, 6, kNoteWidth - 8, 32};
        DrawTextW(hdc, L"x", -1, &closeButton, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_LBUTTONUP:
        if (GET_Y_LPARAM(lParam) < kHeaderHeight && GET_X_LPARAM(lParam) > kNoteWidth - 36) {
            DestroyWindow(hwnd);
        } else if (GET_Y_LPARAM(lParam) < kHeaderHeight && GET_X_LPARAM(lParam) > kNoteWidth - 68) {
            note->color = g_colors[(note->id + GetTickCount()) % (sizeof(g_colors) / sizeof(g_colors[0]))];
            ApplyEditColor(note);
            SaveNotes();
        }
        return 0;
    case WM_RBUTTONUP:
        note->color = g_colors[(note->id + GetTickCount()) % (sizeof(g_colors) / sizeof(g_colors[0]))];
        ApplyEditColor(note);
        SaveNotes();
        return 0;
    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;
    case WM_DESTROY:
        for (auto it = g_notes.begin(); it != g_notes.end(); ++it) {
            if ((*it)->hwnd == hwnd) {
                if ((*it)->brush) DeleteObject((*it)->brush);
                delete *it;
                g_notes.erase(it);
                break;
            }
        }
        SaveNotes();
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

void CreateNote(int x, int y, COLORREF color, const std::wstring& text) {
    auto* note = new NoteWindow();
    note->color = color;
    note->id = g_nextId++;
    note->hwnd = CreateWindowExW(WS_EX_TOOLWINDOW | WS_EX_TOPMOST | WS_EX_LAYERED, L"WhIphoneStickyNote", L"iPhone Sticky Note",
                                WS_POPUP | WS_VISIBLE, x, y, kNoteWidth, kNoteHeight, nullptr, nullptr, g_instance, note);
    SetLayeredWindowAttributes(note->hwnd, 0, 245, LWA_ALPHA);
    SetWindowTextW(note->edit, text.c_str());
    g_notes.push_back(note);
}

void LoadNotes() {
    std::wstring path = GetStoragePath();
    int count = GetPrivateProfileIntW(L"meta", L"count", 0, path.c_str());
    if (count <= 0) {
        CreateNote(120, 120, g_colors[0], L"Напишите заметку…");
        return;
    }
    for (int i = 0; i < count; ++i) {
        wchar_t section[32], text[8192]{};
        wsprintfW(section, L"note%d", i);
        int x = GetPrivateProfileIntW(section, L"x", 120 + i * 30, path.c_str());
        int y = GetPrivateProfileIntW(section, L"y", 120 + i * 30, path.c_str());
        COLORREF color = GetPrivateProfileIntW(section, L"color", g_colors[i % 6], path.c_str());
        GetPrivateProfileStringW(section, L"text", L"", text, 8192, path.c_str());
        CreateNote(x, y, color, UnescapeIni(text));
    }
}

LRESULT CALLBACK LauncherProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_LBUTTONUP:
        CreateNote(160 + static_cast<int>(g_notes.size()) * 24, 160 + static_cast<int>(g_notes.size()) * 24,
                   g_colors[g_notes.size() % (sizeof(g_colors) / sizeof(g_colors[0]))], L"");
        SaveNotes();
        return 0;
    case WM_RBUTTONDOWN:
        SendMessageW(hwnd, WM_NCLBUTTONDOWN, HTCAPTION, 0);
        return 0;
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rc{0, 0, 54, 54};
        FillVerticalGradient(hdc, rc, RGB(98, 181, 255), RGB(0, 122, 255));
        HPEN ring = CreatePen(PS_SOLID, 1, RGB(220, 242, 255));
        HGDIOBJ oldPen = SelectObject(hdc, ring);
        HGDIOBJ oldBrush = SelectObject(hdc, GetStockObject(HOLLOW_BRUSH));
        RoundRect(hdc, 1, 1, 53, 53, 26, 26);
        SelectObject(hdc, oldBrush);
        SelectObject(hdc, oldPen);
        DeleteObject(ring);
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(255, 255, 255));
        DrawTextW(hdc, L"+", -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        EndPaint(hwnd, &ps);
        return 0;
    }
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

DWORD WINAPI UiThread(LPVOID) {
    WNDCLASSW noteClass{};
    noteClass.hInstance = g_instance;
    noteClass.lpszClassName = L"WhIphoneStickyNote";
    noteClass.lpfnWndProc = NoteProc;
    noteClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    RegisterClassW(&noteClass);

    WNDCLASSW launcherClass{};
    launcherClass.hInstance = g_instance;
    launcherClass.lpszClassName = L"WhIphoneStickyLauncher";
    launcherClass.lpfnWndProc = LauncherProc;
    launcherClass.hCursor = LoadCursor(nullptr, IDC_HAND);
    RegisterClassW(&launcherClass);

    g_launcher = CreateWindowExW(WS_EX_TOOLWINDOW | WS_EX_TOPMOST | WS_EX_LAYERED, L"WhIphoneStickyLauncher", L"Add iPhone Sticky Note",
                                WS_POPUP | WS_VISIBLE, 60, 160, 54, 54, nullptr, nullptr, g_instance, nullptr);
    SetLayeredWindowAttributes(g_launcher, 0, 232, LWA_ALPHA);
    HRGN launcherRegion = CreateRoundRectRgn(0, 0, 55, 55, 28, 28);
    SetWindowRgn(g_launcher, launcherRegion, TRUE);
    LoadNotes();

    MSG msg;
    while (g_running && GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    SaveNotes();
    return 0;
}

BOOL Wh_ModInit() {
    g_instance = GetModuleHandleW(nullptr);
    g_running = true;
    g_thread = CreateThread(nullptr, 0, UiThread, nullptr, 0, &g_threadId);
    return g_thread != nullptr;
}

void Wh_ModUninit() {
    SaveNotes();
    for (NoteWindow* note : g_notes) {
        if (note->hwnd) PostMessageW(note->hwnd, WM_CLOSE, 0, 0);
    }
    if (g_launcher) PostMessageW(g_launcher, WM_CLOSE, 0, 0);
    g_running = false;
    if (g_threadId) PostThreadMessageW(g_threadId, WM_QUIT, 0, 0);
    if (g_thread) {
        WaitForSingleObject(g_thread, 2000);
        CloseHandle(g_thread);
    }
}
