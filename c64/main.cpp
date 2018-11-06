#include <Windows.h>

#include <ios>
#include <fstream>
#include <iostream>

#include "cpu.h"
#include "vic.h"
#include "cia.h"

void rom_load(const char* filename, char** bytes) {
	std::ifstream image(filename, std::ios::binary);
	if (image.is_open()) {
		image.seekg(0, std::ios::end);
		std::streamoff length = image.tellg();

		*bytes = new char[(size_t)length];

		image.seekg(0, std::ios::beg);
		image.read(*bytes, length);
	}
}

void ram_dump(const char* filename) {
	extern char* ram;
	std::ofstream dump(filename, std::ios::binary);
	if (dump.is_open()) {
		dump.write(ram, 65536);
	}
}

bool g_stop = false;

const char* windowClassName = "C64WINDOW";

BITMAPINFO bmpinfo;

LRESULT CALLBACK wndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	if (msg == WM_CLOSE) {
		PostQuitMessage(0);
	}
	else if (msg == WM_PAINT) {
		PAINTSTRUCT ps;
		HDC dc = BeginPaint(hWnd, &ps);
		if (dc) {
			RECT rect;
			GetClientRect(hWnd, &rect);
			StretchDIBits(dc, 0, 0, rect.right - rect.left, rect.bottom - rect.top, 0, 0, vic->getFrameWidth(), vic->getFrameHeight(), vic->getFrameBuffer(), &bmpinfo, DIB_RGB_COLORS, SRCCOPY);
			//SetDIBitsToDevice(dc, 0, 0, vic->getFrameWidth(), vic->getFrameHeight(), 0, 0, 0, vic->getFrameHeight(), vic->getFrameBuffer(), &bmpinfo, DIB_RGB_COLORS);

			EndPaint(hWnd, &ps);
		}
	}
	else if (msg == WM_CHAR) {

	}
	return DefWindowProc(hWnd, msg, wParam, lParam);
}

HWND createMainWindow(HINSTANCE hInstance) {

	WNDCLASS wc = { 0 };
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.hInstance = hInstance;
	wc.lpfnWndProc = wndProc;
	wc.lpszClassName = windowClassName;

	RegisterClass(&wc);

	HWND hWnd = 0;
	hWnd = CreateWindow(windowClassName, "C64 Output", WS_OVERLAPPEDWINDOW, 0, 0, 336, 238, 0, 0, hInstance, 0);

	ShowWindow(hWnd, SW_SHOW);

	return hWnd;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {

	HWND hWnd = createMainWindow(hInstance);

	extern char* ram;
	extern char* vram;
	extern char* kernal;
	extern char* basic;
	extern char* charset;
	extern char* ioread;
	extern char* iowrite;

	rom_load("images/kernal.bin", &kernal);
	rom_load("images/basic.bin", &basic);
	rom_load("images/charset.bin", &charset);

	ram = new char[65536];
	vram = new char[1024];
	ioread = new char[4096];
	iowrite = new char[4096];

	cpu_init();

	vic = new Vic();
	cia = new Cia(vic);

	memset(&bmpinfo, 0, sizeof(bmpinfo));
	bmpinfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmpinfo.bmiHeader.biWidth = vic->getFrameWidth();
	bmpinfo.bmiHeader.biHeight = vic->getFrameHeight();
	bmpinfo.bmiHeader.biPlanes = 1;
	bmpinfo.bmiHeader.biBitCount = 32;
	bmpinfo.bmiHeader.biCompression = BI_RGB;

	MSG msg;
	do {
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else {
			// run cia (both)
			cia->tick();

			// run vic
			vic->tick();

			// run cpu
			cpu_tick();

			if (vic->frameStarted()) {
				RedrawWindow(hWnd, 0, 0, RDW_INVALIDATE | RDW_UPDATENOW);
			}
		}
	} while (msg.message != WM_QUIT);

	ram_dump("ram.dump");

	delete cia;
	delete vic;

	return 0;
}