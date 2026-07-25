// ==WindhawkMod==
// @id              iphone-liquid-glass-sticky-notes
// @name            iPhone Liquid Glass Sticky Notes
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

constexpr int kNoteWidth = 300;
constexpr int kNoteHeight = 270;
constexpr int kHeaderHeight = 54;
constexpr int kLauncherWidth = 132;
constexpr int kLauncherHeight = 46;
constexpr UINT_PTR kAutosaveTimer = 42;
constexpr UINT kSaveMessage = WM_APP + 25;

struct NoteWindow {
    HWND hwnd = nullptr;
    HWND edit = nullptr;
    COLORREF color = RGB(255, 230, 109);
    int id = 0;
    HBRUSH brush = nullptr;
    HFONT font = nullptr;
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

enum AccentState {
    ACCENT_DISABLED = 0,
    ACCENT_ENABLE_GRADIENT = 1,
    ACCENT_ENABLE_TRANSPARENTGRADIENT = 2,
    ACCENT_ENABLE_BLURBEHIND = 3,
    ACCENT_ENABLE_ACRYLICBLURBEHIND = 4
};

struct AccentPolicy {
    int accentState;
    int accentFlags;
    int gradientColor;
    int animationId;
};

struct WindowCompositionAttributeData {
    int attribute;
    void* data;
    size_t sizeOfData;
};

void EnableLiquidGlassBlur(HWND hwnd) {
    HMODULE user32 = GetModuleHandleW(L"user32.dll");
    auto setWindowCompositionAttribute = reinterpret_cast<BOOL(WINAPI*)(HWND, WindowCompositionAttributeData*)>(
        GetProcAddress(user32, "SetWindowCompositionAttribute"));
    if (!setWindowCompositionAttribute) {
        return;
    }

    AccentPolicy policy{};
    policy.accentState = ACCENT_ENABLE_ACRYLICBLURBEHIND;
    policy.accentFlags = 2;
    policy.gradientColor = 0x99FFFFFF;

    WindowCompositionAttributeData data{};
    data.attribute = 19; // WCA_ACCENT_POLICY
    data.data = &policy;
    data.sizeOfData = sizeof(policy);
    setWindowCompositionAttribute(hwnd, &data);
}


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
    FillVerticalGradient(hdc, card, RGB(255, 255, 255), RGB(235, 244, 255));

    HBRUSH shadow = CreateSolidBrush(RGB(196, 206, 230));
    RECT shadowRect{7, kNoteHeight - 18, kNoteWidth - 7, kNoteHeight - 5};
    FillRect(hdc, &shadowRect, shadow);
    DeleteObject(shadow);

    HPEN borderPen = CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
    HGDIOBJ oldPen = SelectObject(hdc, borderPen);
    HGDIOBJ oldBrush = SelectObject(hdc, GetStockObject(HOLLOW_BRUSH));
    RoundRect(hdc, 1, 1, kNoteWidth - 1, kNoteHeight - 1, 30, 30);
    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(borderPen);

    RECT header{0, 0, kNoteWidth, kHeaderHeight};
    FillVerticalGradient(hdc, header, RGB(255, 255, 255), RGB(243, 248, 255));

    HBRUSH accent = CreateSolidBrush(note->color);
    RECT accentBar{18, kHeaderHeight - 8, kNoteWidth - 18, kHeaderHeight - 4};
    FillRect(hdc, &accentBar, accent);
    DeleteObject(accent);

    HBRUSH shine = CreateSolidBrush(RGB(255, 255, 255));
    RECT shineLine{22, 9, kNoteWidth - 68, 13};
    FillRect(hdc, &shineLine, shine);
    RECT shineDot{22, 20, 94, 25};
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
        note->edit = CreateWindowExW(0, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL,
                                    18, kHeaderHeight + 2, kNoteWidth - 36, kNoteHeight - kHeaderHeight - 22,
                                    hwnd, reinterpret_cast<HMENU>(1001), g_instance, nullptr);
        note->font = CreateFontW(-17, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                                 OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                                 DEFAULT_PITCH | FF_SWISS, L"Segoe UI Variable");
        SendMessageW(note->edit, WM_SETFONT, reinterpret_cast<WPARAM>(note->font), TRUE);
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
        SetTextColor(hdc, RGB(38, 40, 48));
        RECT title{20, 2, kNoteWidth - 92, 34};
        DrawTextW(hdc, L"Liquid Sticky", -1, &title, DT_VCENTER | DT_SINGLELINE);
        SetTextColor(hdc, RGB(110, 114, 124));
        RECT subtitle{20, 27, kNoteWidth - 92, 50};
        DrawTextW(hdc, L"Пишите здесь", -1, &subtitle, DT_VCENTER | DT_SINGLELINE);
        HBRUSH colorBrush = CreateSolidBrush(note->color);
        HGDIOBJ oldBrush = SelectObject(hdc, colorBrush);
        HGDIOBJ oldPen = SelectObject(hdc, GetStockObject(NULL_PEN));
        Ellipse(hdc, kNoteWidth - 72, 14, kNoteWidth - 46, 40);
        SelectObject(hdc, oldPen);
        SelectObject(hdc, oldBrush);
        DeleteObject(colorBrush);
        SetTextColor(hdc, RGB(80, 84, 96));
        RECT closeButton{kNoteWidth - 39, 10, kNoteWidth - 12, 38};
        DrawTextW(hdc, L"×", -1, &closeButton, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
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
                if ((*it)->font) DeleteObject((*it)->font);
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
    SetLayeredWindowAttributes(note->hwnd, 0, 222, LWA_ALPHA);
    EnableLiquidGlassBlur(note->hwnd);
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
        RECT rc{0, 0, kLauncherWidth, kLauncherHeight};
        FillVerticalGradient(hdc, rc, RGB(108, 190, 255), RGB(0, 122, 255));
        HPEN ring = CreatePen(PS_SOLID, 1, RGB(220, 242, 255));
        HGDIOBJ oldPen = SelectObject(hdc, ring);
        HGDIOBJ oldBrush = SelectObject(hdc, GetStockObject(HOLLOW_BRUSH));
        RoundRect(hdc, 1, 1, kLauncherWidth - 1, kLauncherHeight - 1, 24, 24);
        SelectObject(hdc, oldBrush);
        SelectObject(hdc, oldPen);
        DeleteObject(ring);
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(255, 255, 255));
        DrawTextW(hdc, L"+ Стикер", -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
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
                                WS_POPUP | WS_VISIBLE, 60, 160, kLauncherWidth, kLauncherHeight, nullptr, nullptr, g_instance, nullptr);
    SetLayeredWindowAttributes(g_launcher, 0, 232, LWA_ALPHA);
    EnableLiquidGlassBlur(g_launcher);
    HRGN launcherRegion = CreateRoundRectRgn(0, 0, kLauncherWidth + 1, kLauncherHeight + 1, 24, 24);
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
