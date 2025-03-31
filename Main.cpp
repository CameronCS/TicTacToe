//C Standard Library Imports
#include <Windows.h>
#include <vector>
#include <wchar.h>

#pragma region Macros

// Macro Definitions
#define SELECT_TEXT L"Select"
#define NO_TEXT L""
#define BTN_WIDTH 100
#define BTN_HEIGHT 100
#define PLAYER_LABEL_STATIC_TEXT L"Turn: "
#define PADDING 50
#define FONT_NAME L"Fira Code"

#pragma endregion

#pragma region Globals
// Global Variables
const wchar_t CLASSNAME[] = L"TicTacToe";
HINSTANCE ghInst;
HWND hwndWindow;
HWND hwndPlayer;
bool player1 = true;
std::vector<HWND> *slots = new std::vector<HWND>();
// Supporing Methods for slots
HWND get_slot(int idx) { return slots->at(idx); }
// Win Proc
LRESULT __stdcall WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Hbrush for btn colours
HBRUSH hBrushRed = CreateSolidBrush(RGB(255, 0, 0));
HBRUSH hBrushBlue = CreateSolidBrush(RGB(0, 0, 255));

#pragma endregion

#pragma region Function Declaration
// Method Delcarations with respective Namespaces
namespace comp {
	ATOM register_class();
	int create_hwndWindow();
	int create_player_label();
	int init_slots();
	void get_player_str(TCHAR *str, size_t size, bool is_player_one);

	namespace helper {
		int get_button_pos(int col);
	}
}

namespace game {
	bool check_game_over();
	bool three_in_a_row(HWND slot_1, HWND slot_2, HWND slot_3);
	bool check_horizontal_win();
	bool check_vertical_win();
	bool check_diagonal_win(bool set_one);
	bool check_player_win();
	void change_player_label();
	void reset_game_state();
	void quit_game(bool x_pressed);
	void change_text(int sender);
}

namespace draw {
	void draw_vertical_lines(HDC hdc);
	void draw_horizontal_lines(HDC hdc);
}

#pragma endregion

int __stdcall wWinMain(_In_ HINSTANCE hInst, _In_opt_ HINSTANCE hPrevInst, _In_ PWSTR lpszCmdLine, _In_ int nCmdShow) {
	ghInst = hInst;

	if (!comp::register_class()) {
		MessageBox(NULL, L"Error Registering the class closing application", L"TicTacToe", MB_ICONERROR);
		return 1;
	}

	if (comp::create_hwndWindow() == 0) {
		MessageBox(NULL, L"Error creating the 'Main Window' closing application", L"TicTacToe", MB_ICONERROR);
		return 1;
	}

	if (comp::create_player_label() == 0) {
		MessageBox(NULL, L"Error creating the 'Player Label' closing application", L"TicTacToe", MB_ICONERROR);
		return 1;
	}

	if (comp::init_slots() == 0) {
		return 1;
	}

	ShowWindow(hwndWindow, nCmdShow);

	MSG msg = {};
	while (GetMessage(&msg, NULL, 0, 0) > 0) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return 0;
}

LRESULT __stdcall WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {	
	switch (msg) {
		case WM_DESTROY: {
			game::quit_game(false);
			DeleteObject(hBrushRed);
			DeleteObject(hBrushBlue);
			break;
		}

		case WM_CTLCOLORBTN: {
			HDC hdcButton = (HDC)wParam;
			HWND hButton = (HWND)lParam;

			// Retrieve the brush for this button
			HBRUSH hBrush = (HBRUSH)GetProp(hButton, L"BgColor");
			if (hBrush) {
				SetBkMode(hdcButton, TRANSPARENT);
				return (LRESULT)hBrush;
			}
			break;
		}

		case WM_PAINT: {
			PAINTSTRUCT ps = {};
			HDC hdc = BeginPaint(hwnd, &ps);

			HPEN hPen = CreatePen(PS_SOLID, 3, RGB(0, 0, 0));
			HGDIOBJ oldPen = SelectObject(hdc, hPen);

			draw::draw_vertical_lines(hdc);
			draw::draw_horizontal_lines(hdc);

			SelectObject(hdc, oldPen);
			DeleteObject(hPen);

			EndPaint(hwnd, &ps);
			break;
		}

		case WM_COMMAND: {
			int button_id = LOWORD(wParam);
			game::change_text(button_id);
			break;
		}

		default: {
			return DefWindowProc(hwnd, msg, wParam, lParam);
		}
	}
	return DefWindowProc(hwnd, msg, wParam, lParam);
}

#pragma region Comp

ATOM comp::register_class() {
	WNDCLASS __class = {};
	__class.hInstance = ghInst;
	__class.lpszClassName = CLASSNAME;
	__class.lpfnWndProc = WndProc;
	__class.hCursor = LoadCursor(NULL, IDC_ARROW);
	__class.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	__class.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

	return RegisterClassW(&__class);
}

int comp::create_hwndWindow() {
	hwndWindow = CreateWindowEx(
		0, CLASSNAME, L"TicTacToe", WS_OVERLAPPEDWINDOW ^ WS_THICKFRAME ^ WS_MAXIMIZEBOX,
		CW_USEDEFAULT, CW_USEDEFAULT, 700, 700,
		NULL, NULL, ghInst, NULL
	);

	HFONT hFont = CreateFont(
		20, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
		DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
		DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, FONT_NAME);
	SendMessage(hwndWindow, WM_SETFONT, (WPARAM)hFont, TRUE);

	return hwndWindow == NULL ? 0 : 1;
}

int comp::create_player_label() {
	hwndPlayer = CreateWindow(
		L"STATIC", L"Turn: Player 1",
		WS_CHILD | WS_VISIBLE,
		275, 50, 150, 25,
		hwndWindow, NULL, ghInst, NULL
	);

	HFONT hFont = CreateFont(
		20, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
		DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
		DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, FONT_NAME);
	SendMessage(hwndPlayer, WM_SETFONT, (WPARAM)hFont, TRUE);

	return hwndPlayer == NULL ? 0 : 1;
}

int comp::init_slots() {
	short __idx = 0;
	for (int row = 0; row < 3; row++) {
		for (int col = 0; col < 3; col++) {
			int __x = comp::helper::get_button_pos(col);
			int __y = comp::helper::get_button_pos(row);
			HWND btn = CreateWindow(
				L"BUTTON", SELECT_TEXT,
				WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
				__x, __y, BTN_WIDTH, BTN_HEIGHT,
				hwndWindow, (HMENU)(__idx++), ghInst, NULL
			);
			slots->push_back(btn);

			if (btn == NULL) {
				TCHAR idx_as_str[5] = {};
				swprintf_s(idx_as_str, 5, L"%d", (__idx - 1));

				TCHAR button_error_message[255] = {};
				wcscpy_s(button_error_message, L"Error creating the 'Slot Button' @ index [");
				wcscat_s(button_error_message, idx_as_str);
				wcscat_s(button_error_message, L"] Closing Application");

				MessageBox(NULL, button_error_message, L"TicTacToe", MB_ICONERROR);
				return 0;
			}

			HFONT hFont = CreateFont(
				20, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
				DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
				DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, FONT_NAME);
			SendMessage(btn, WM_SETFONT, (WPARAM)hFont, TRUE);
		}
	}
	return 1;
}


void comp::get_player_str(TCHAR *str, size_t size, bool is_player_one) {
	if (is_player_one) {
		wcscpy_s(str, size, L"Player 1");
	} else {
		wcscpy_s(str, size, L"Player 2");
	}
}


int comp::helper::get_button_pos(int col) {
	switch (col) {
		case 0: return PADDING + BTN_WIDTH;
		case 1: return get_button_pos(0) + PADDING + BTN_WIDTH;
		case 2: return get_button_pos(1) + PADDING + BTN_WIDTH;
		default: break;
	}
	return 0;
}

#pragma endregion

#pragma region Game

bool game::check_game_over() {
	bool has_select = false;
	for (int i = 0; i < 9; i++) {
		HWND btn = slots->at(i);
		TCHAR txt[10] = {};
		GetWindowTextW(btn, txt, 10);
		if (wcscmp(txt, SELECT_TEXT) == 0) {
			has_select = true;
		}
	}
	return !has_select;
}

bool game::three_in_a_row(HWND slot_1, HWND slot_2, HWND slot_3) {
	constexpr int slot_text_size = 10;

	TCHAR slot_1_text[slot_text_size] = {};
	GetWindowText(slot_1, slot_1_text, slot_text_size);

	TCHAR slot_2_text[slot_text_size] = {};
	GetWindowText(slot_2, slot_2_text, slot_text_size);

	TCHAR slot_3_text[slot_text_size] = {};
	GetWindowText(slot_3, slot_3_text, slot_text_size);

	if (wcscmp(slot_1_text, L"Select") == 0) {
		return false;
	}

	if (wcscmp(slot_2_text, L"Select") == 0) {
		return false;
	}

	if (wcscmp(slot_3_text, L"Select") == 0) {
		return false;
	}

	if (wcscmp(slot_1_text, slot_2_text) != 0) {
		return false;
	}

	if (wcscmp(slot_1_text, slot_3_text) != 0) {
		return false;
	}

	return true;
}

bool game::check_horizontal_win() {
	/*
	Horizontal Winning Sets
	123 -> 012
	456 -> 345
	789 -> 678
	*/
	bool vertical_win = false;
	for (int i = 0; i < 7; i += 3) {
		if (!vertical_win) {
			HWND slot_1 = get_slot(i);
			HWND slot_2 = get_slot(i + 1);
			HWND slot_3 = get_slot(i + 2);
			vertical_win = three_in_a_row(slot_1, slot_2, slot_3);
		}
	}
	return vertical_win;
}

bool game::check_vertical_win() {
	/*
	Vertical Winning Sets
	147 -> 036
	258 -> 147
	369 -> 258
	*/
	bool horizontal_win = false;
	for (int i = 0; i < 3; i++) {
		if (!horizontal_win) {
			HWND slot_1 = get_slot(i);
			HWND slot_2 = get_slot(i + 3);
			HWND slot_3 = get_slot(i + 6);
			horizontal_win = three_in_a_row(slot_1, slot_2, slot_3);
		}
	}
	return horizontal_win;
}

bool game::check_diagonal_win(bool set_one) {
	/*
	Diagonal Winning Sets
	159 -> 048 [Set One]
	357 -> 246 [Set Two]
*/
	if (set_one) {
		HWND slot_1 = get_slot(0);
		HWND slot_2 = get_slot(4);
		HWND slot_3 = get_slot(8);
		return three_in_a_row(slot_1, slot_2, slot_3);
	} else {
		HWND slot_1 = get_slot(2);
		HWND slot_2 = get_slot(4);
		HWND slot_3 = get_slot(6);
		return three_in_a_row(slot_1, slot_2, slot_3);
	}
}


//return check_horizontal_win() || check_vertical_win() || check_diagonal_win(true) || check_diagonal_win(false);
bool game::check_player_win() {
	if (check_horizontal_win()) {
		return true;
	}

	if (check_vertical_win()) {
		return true;
	}

	if (check_diagonal_win(true)) {
		return true;
	}

	if (check_diagonal_win(false)) {
		return true;
	}

	return false;
}

void game::change_player_label() {
	TCHAR current_player[10] = {};
	if (player1) {
		wcscpy_s(current_player, 10, L"Player 1");
	} else {
		wcscpy_s(current_player, 10, L"Player 2");
	}

	TCHAR player_string[255] = PLAYER_LABEL_STATIC_TEXT;
	wcscat_s(player_string, current_player);
	SetWindowText(hwndPlayer, player_string);
}

void game::reset_game_state() {
	for (int i = 0; i < 9; i++) {
		HWND btn = get_slot(i);
		SetWindowText(btn, L"Select");
		HFONT hFont = CreateFont(
			20, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
			DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
			DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, FONT_NAME);
		SendMessage(btn, WM_SETFONT, (WPARAM)hFont, TRUE);
	}
	player1 = true;
	game::change_player_label();
}


void game::quit_game(bool x_pressed) {
	if (x_pressed) {
		MessageBox(hwndWindow, L"Game is Closing, Thank you for playing!", L"Quitting Game", MB_ICONINFORMATION);
	}
	delete slots;
	PostQuitMessage(0);
}


void game::change_text(int sender) {
	HWND btn = get_slot(sender);

	TCHAR current[10] = {};
	GetWindowText(btn, current, 10);

	if (wcscmp(current, SELECT_TEXT) == 0) {

		TCHAR symbol[2] = {};
		if (player1) {
			// Set button symbol to "X"
			wcscpy_s(symbol, L"X");

			// Save red brush for background coloring
			SetProp(btn, L"BgColor", hBrushRed);

			// Set Font Size to 40
			HFONT hFont = CreateFont(
				40, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
				DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
				DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, FONT_NAME);
			SendMessage(btn, WM_SETFONT, (WPARAM)hFont, TRUE);
		} else {
			// Set button symbol to "O"
			wcscpy_s(symbol, L"O");

			// Save blue brush for background coloring
			SetProp(btn, L"BgColor", hBrushBlue);

			// Set Font Size to 40
			HFONT hFont = CreateFont(
				40, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
				DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
				DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, FONT_NAME);
			SendMessage(btn, WM_SETFONT, (WPARAM)hFont, TRUE);
		}

		SetWindowText(btn, symbol);
		InvalidateRect(btn, NULL, TRUE);
		// SendMessageW(btn, WM_CTLCOLORBTN, NULL, (LPARAM)btn);

		if (game::check_player_win()) {
			TCHAR winning_player[255] = {};
			comp::get_player_str(winning_player, 255, player1);

			TCHAR winning_text[255] = L" Wins\nDo you wish to play again?";
			wcscat_s(winning_player, winning_text);

			int mb_result = MessageBox(hwndWindow, winning_player, L"Game Over!", MB_ICONINFORMATION | MB_YESNO);

			if (mb_result == IDYES) {
				reset_game_state();
				return;
			} else game::quit_game(false);
		}

		if (game::check_game_over()) {
			int mb_result = MessageBox(hwndWindow, L"Game Over!\nNo one won! :(\nDo you wish to play again", L"Game Over", MB_ICONINFORMATION | MB_YESNO);
			if (mb_result == IDYES) {
				reset_game_state();
				return;
			} else game::quit_game(false);
		}

		player1 = !player1;
		game::change_player_label();
	} else {
		MessageBox(hwndWindow, L"Position Taken", L"TicTacToe Error", MB_ICONWARNING);
	}
}

#pragma endregion

#pragma region Draw

void draw::draw_vertical_lines(HDC hdc) {
	// Begin Draw Vertical Lines
	MoveToEx(hdc, 125, 125, NULL);
	LineTo(hdc, 125, 575);

	MoveToEx(hdc, 275, 125, NULL);
	LineTo(hdc, 275, 575);

	MoveToEx(hdc, 425, 125, NULL);
	LineTo(hdc, 425, 575);

	MoveToEx(hdc, 575, 125, NULL);
	LineTo(hdc, 575, 575);
	// End Draw Vertical Lines
}
void draw::draw_horizontal_lines(HDC hdc) {
	// Begin Draw Horizontal Lines
	MoveToEx(hdc, 125, 125, NULL);
	LineTo(hdc, 575, 125);

	MoveToEx(hdc, 125, 275, NULL);
	LineTo(hdc, 575, 275);

	MoveToEx(hdc, 125, 425, NULL);
	LineTo(hdc, 575, 425);

	MoveToEx(hdc, 125, 575, NULL);
	LineTo(hdc, 575, 575);
	// End Draw Horizontal Lines
}

#pragma endregion