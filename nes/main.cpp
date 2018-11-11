#include "cpu.h"
#include "ppu.h"
#include "pak.h"
#include "bus.h"
#include "apu.h"

#include <ios>
#include <fstream>
#include <iostream>

bool g_stop = false;
bool g_debug_read = false;

constexpr uint32_t rgba(unsigned char r, unsigned char g, unsigned char b, unsigned char a) {
	return (b)+(g << 8) + (r << 16) + (a << 24);
}

#ifdef _WIN32

#include <Windows.h>
bool g_slow = false;

void selectRom(HWND hWnd) {
	OPENFILENAME ofn;       // common dialog box structure
	char szFile[260];       // buffer for file name

	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hWnd;
	ofn.lpstrFile = szFile;
	// Set lpstrFile[0] to '\0' so that GetOpenFileName does not 
	// use the contents of szFile to initialize itself.
	ofn.lpstrFile[0] = '\0';
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFilter = "All\0*.*\0NES Rom\0*.NES\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

	// Display the Open dialog box. 

	if (GetOpenFileName(&ofn) == TRUE) {
		pak_load(ofn.lpstrFile);
		ppu_init();
		cpu_init();
	}
}

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
			StretchDIBits(dc, 0, 0, rect.right - rect.left, rect.bottom - rect.top, 0, 0, ppu_frame_width, ppu_frame_height, ppu_frame_buffer, &bmpinfo, DIB_RGB_COLORS, SRCCOPY);
			EndPaint(hWnd, &ps);
		}
	}
	else if (msg == WM_CHAR) {
		if (wParam == '1') {
			ppu_mode = 1;
			SetWindowText(hWnd, "NES");
		}
		else if (wParam == '2') {
			ppu_mode = 2;
			SetWindowText(hWnd, "Memory");
		}
		else if (wParam == '3') {
			ppu_mode = 3;
			SetWindowText(hWnd, "Chr Rom");
		}
		else if (wParam == 't') {
			g_slow = !g_slow;
		}
		else if (wParam == 'r') {
			ppu_init();
			cpu_init();
		}
		else if (wParam == 'l') {
			selectRom(hWnd);
		}
	}
	else if (msg == WM_KEYUP) {
		if (wParam == VK_F1 || wParam == VK_F2 || wParam == VK_F3 || wParam == VK_F4) {
			int zoom = wParam == VK_F1 ? 1 : (wParam == VK_F2 ? 2 : (wParam == VK_F3 ? 3 : 4));
			RECT rect;
			rect.left = 0;
			rect.top = 0;
			rect.right = ppu_frame_width * zoom;
			rect.bottom = ppu_frame_height * zoom;
			AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);
			MoveWindow(hWnd, 0, 0, rect.right - rect.left, rect.bottom - rect.top, FALSE);
		}
		else if (wParam == 'A') apu_input_a = 0;
		else if (wParam == 'S') apu_input_b = 0;
		else if (wParam == 'Y') apu_input_select = 0;
		else if (wParam == 'X') apu_input_start = 0;
		else if (wParam == VK_UP) apu_input_up = 0;
		else if (wParam == VK_DOWN) apu_input_down = 0;
		else if (wParam == VK_LEFT) apu_input_left = 0;
		else if (wParam == VK_RIGHT) apu_input_right = 0;
	}
	else if (msg == WM_KEYDOWN) {
		if (wParam == 'A') apu_input_a = 1;
		else if (wParam == 'S') apu_input_b = 1;
		else if (wParam == 'Y') apu_input_select = 1;
		else if (wParam == 'X') apu_input_start = 1;
		else if (wParam == VK_UP) apu_input_up = 1;
		else if (wParam == VK_DOWN) apu_input_down = 1;
		else if (wParam == VK_LEFT) apu_input_left = 1;
		else if (wParam == VK_RIGHT) apu_input_right = 1;
	}
	return DefWindowProc(hWnd, msg, wParam, lParam);
}

HWND createMainWindow(HINSTANCE hInstance) {

	memset(&bmpinfo, 0, sizeof(bmpinfo));
	bmpinfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmpinfo.bmiHeader.biWidth = ppu_frame_width;
	bmpinfo.bmiHeader.biHeight = -ppu_frame_height;
	bmpinfo.bmiHeader.biPlanes = 1;
	bmpinfo.bmiHeader.biBitCount = 32;
	bmpinfo.bmiHeader.biCompression = BI_RGB;

	WNDCLASS wc = { 0 };
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.hInstance = hInstance;
	wc.lpfnWndProc = wndProc;
	wc.lpszClassName = windowClassName;

	RegisterClass(&wc);

	HWND hWnd = 0;
	hWnd = CreateWindow(windowClassName, "NES", WS_OVERLAPPEDWINDOW, 0, 0, 800, 600, 0, 0, hInstance, 0);

	RECT rect;
	rect.left = 0;
	rect.top = 0;
	rect.right = ppu_frame_width * 4;
	rect.bottom = ppu_frame_height * 4;
	AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);
	MoveWindow(hWnd, 0, 0, rect.right - rect.left, rect.bottom - rect.top, FALSE);

	ShowWindow(hWnd, SW_SHOW);

	return hWnd;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {

	HWND hWnd = createMainWindow(hInstance);
	selectRom(hWnd);

	HWAVEOUT wo;
	WAVEFORMATEX wfx{ 0 };
	wfx.cbSize = 0;
	wfx.wFormatTag = WAVE_FORMAT_PCM;
	wfx.nChannels = 1;
	wfx.nSamplesPerSec = 44100;
	wfx.nBlockAlign = 2;
	wfx.wBitsPerSample = 16;
	wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign * wfx.nChannels;
	auto res = waveOutOpen(&wo, WAVE_MAPPER, &wfx, (DWORD_PTR)hWnd, 0, CALLBACK_WINDOW);

	int numsamples = 2000;
	WAVEHDR wh1;
	WAVEHDR wh2;
	int16_t* buffer1;
	int16_t* buffer2;

	buffer1 = new int16_t[numsamples];
	buffer2 = new int16_t[numsamples];

	memset(&wh1, 0, sizeof(WAVEHDR));
	wh1.lpData = (char*)buffer1;
	wh1.dwBufferLength = numsamples * 2;
	
	memset(&wh2, 0, sizeof(WAVEHDR));
	wh2.lpData = (char*)buffer2;
	wh2.dwBufferLength = numsamples * 2;

	waveOutPrepareHeader(wo, &wh1, sizeof(WAVEHDR));
	waveOutWrite(wo, &wh1, sizeof(WAVEHDR));

	waveOutPrepareHeader(wo, &wh2, sizeof(WAVEHDR));
	waveOutWrite(wo, &wh2, sizeof(WAVEHDR));

	LARGE_INTEGER freq;
	QueryPerformanceFrequency(&freq);

	int tick = 0;
	int t = 0;
	LARGE_INTEGER nextVsync{ 0 };
	MSG msg;
	do {
		LARGE_INTEGER counter;
		QueryPerformanceCounter(&counter);

		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);

			if (msg.message == MM_WOM_DONE) {
				if ((WAVEHDR*)msg.lParam == &wh1) {
					apu_gen_audio(buffer1, numsamples);
					res = waveOutWrite(wo, &wh1, sizeof(WAVEHDR));
				}
				else {
					apu_gen_audio(buffer2, numsamples);
					waveOutWrite(wo, &wh2, sizeof(WAVEHDR));
				}
			}
		}
		else if (!ppu_vsync) {
			// run cpu
			cpu_tick();
			ppu_tick();
			apu_tick();

			if (ppu_mode == 2) {
				g_debug_read = true;
				for (int i = 0; i < ppu_frame_width * ppu_frame_height; ++i) {
					unsigned char v = bus_read(i)*0.125;
					uint32_t color;

					if (i == cpu_pc) color = rgba(0, 255, 0, 255);
					else if (i == bus_last_read) color = rgba(0, 0, 255, 255);
					else if (i == bus_last_write) color = rgba(255, 0, 0, 255);
					else if (i < 0x2000) color = rgba(v, v, v, 255);
					else if (i < 0x4000) color = rgba(v, 0, 0, 255);
					else if (i < 0x4020) color = rgba(0, v, 0, 255);
					else if (i < 0x8000) color = rgba(0, 0, v, 255);
					else color = rgba(v, v, v, 255);

					ppu_frame_buffer[i] = color;
				}
				g_debug_read = false;
				RedrawWindow(hWnd, 0, 0, RDW_INVALIDATE | RDW_UPDATENOW);
			}
		}
		else if (counter.QuadPart > nextVsync.QuadPart) {
			nextVsync.QuadPart = counter.QuadPart + freq.QuadPart / 60;
			RedrawWindow(hWnd, 0, 0, RDW_INVALIDATE | RDW_UPDATENOW);
			ppu_vsync = false;
		}
	} while (msg.message != WM_QUIT);

	return 0;
}

#else

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>

void updateScreen(SDL_Renderer* renderer, SDL_Texture* framebuffer) {
	void* pixels = nullptr;
	int pitch = ppu_frame_width;
	if (SDL_LockTexture(framebuffer, nullptr, &pixels, &pitch)) {
		std::cerr << SDL_GetError() << "\n";
		exit(1);
	}
	
	memcpy(pixels, ppu_frame_buffer, ppu_frame_width * ppu_frame_height * 4);
	SDL_UnlockTexture(framebuffer);
	SDL_RenderCopy(renderer, framebuffer, NULL, NULL);
	SDL_RenderPresent(renderer);
}

int main(int argc, char** argv) {
	if (argc < 2) {
		std::cerr << "Missing ROM file argument.\n";
		return 1;
	}

	pak_load(argv[1]);
	ppu_init();
	cpu_init();
	
	if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO)) {
		std::cerr << "Error initializing SDL.\n";
		return 1;
	}

	auto window = SDL_CreateWindow("NES", 0, 0, ppu_frame_width, ppu_frame_height, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
	if (!window) {
		std::cerr << SDL_GetError();
		return 1;
	}

	auto renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	if (!renderer) {
		std::cerr << SDL_GetError();
		return 1;
	}

	auto framebuffer = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, ppu_frame_width, ppu_frame_height);
	if (!framebuffer) {
		std::cerr << SDL_GetError();
		return 1;
	}

	while(!g_stop) {
		//Handle events on queue
		SDL_Event e;
		if (SDL_PollEvent(&e) != 0) {
			//User requests quit
			switch (e.type) {
				case SDL_QUIT:
					g_stop = true;
					break;

				case SDL_KEYDOWN:
					if (e.key.keysym.sym == SDLK_UP) apu_input_up = 1;
					else if (e.key.keysym.sym == SDLK_DOWN) apu_input_down = 1;
					else if (e.key.keysym.sym == SDLK_LEFT) apu_input_left = 1;
					else if (e.key.keysym.sym == SDLK_RIGHT) apu_input_right = 1;
					else if (e.key.keysym.sym == SDLK_a) apu_input_a = 1;
					else if (e.key.keysym.sym == SDLK_s) apu_input_b = 1;
					else if (e.key.keysym.sym == SDLK_y) apu_input_select = 1;
					else if (e.key.keysym.sym == SDLK_x) apu_input_start = 1;
					break;

				case SDL_KEYUP:
					if (e.key.keysym.sym == SDLK_UP) apu_input_up = 0;
					else if (e.key.keysym.sym == SDLK_DOWN) apu_input_down = 0;
					else if (e.key.keysym.sym == SDLK_LEFT) apu_input_left = 0;
					else if (e.key.keysym.sym == SDLK_RIGHT) apu_input_right = 0;
					else if (e.key.keysym.sym == SDLK_a) apu_input_a = 0;
					else if (e.key.keysym.sym == SDLK_s) apu_input_b = 0;
					else if (e.key.keysym.sym == SDLK_y) apu_input_select = 0;
					else if (e.key.keysym.sym == SDLK_x) apu_input_start = 0;
					else if (e.key.keysym.sym == SDLK_1) { ppu_mode = 1; SDL_SetWindowTitle(window, "NES"); }
					else if (e.key.keysym.sym == SDLK_2) { ppu_mode = 2; SDL_SetWindowTitle(window, "Memory"); }
					else if (e.key.keysym.sym == SDLK_3) { ppu_mode = 3; SDL_SetWindowTitle(window, "Chr Rom"); }
					else if (e.key.keysym.sym == SDLK_r) { ppu_init(); cpu_init(); }
					break;
			}
		}
		else {
			cpu_tick();
			ppu_tick();

			if (ppu_mode == 2) {
				g_debug_read = true;
				for (int i = 0x0; i < ppu_frame_width * ppu_frame_height; ++i) {
					unsigned char v = bus_read(i)*0.125;
					uint32_t color;

					if (i == cpu_pc) color = rgba(0, 255, 0, 255);
					else if (i == bus_last_read) color = rgba(0, 0, 255, 255);
					else if (i == bus_last_write) color = rgba(255, 0, 0, 255);
					else if (i < 0x2000) color = rgba(v, v, v, 255);
					else if (i < 0x4000) color = rgba(v, 0, 0, 255);
					else if (i < 0x4020) color = rgba(0, v, 0, 255);
					else if (i < 0x8000) color = rgba(0, 0, v, 255);
					else color = rgba(v, v, v, 255);

					ppu_frame_buffer[i] = color;
				}
				g_debug_read = false;
				updateScreen(renderer, framebuffer);
			}
			else if (ppu_vsync) {
				updateScreen(renderer, framebuffer);
			}
		}
	}
	
	SDL_DestroyTexture(framebuffer);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}

#endif