#include "includes.h"
#include <string>
#include <sstream>

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

Present oPresent;
HWND window = NULL;
WNDPROC oWndProc;
ID3D11Device* pDevice = NULL;
ID3D11DeviceContext* pContext = NULL;
ID3D11RenderTargetView* mainRenderTargetView;

bool alive = false;
bool show = true;

float turnRateCap = 300;
float turnRateCapTarget = 300;
float LookUpRateCap = 300;
float LookUpRateCapTarget = 300;
bool longYellOnDeath = 0;

uintptr_t gEngine;
uintptr_t gameInstance;
uintptr_t localPlayers;
uintptr_t localPlayer;
uintptr_t playerController;
uintptr_t character;
uintptr_t mordhauExeBase = 0x7FF781840000;
uintptr_t gEngineOffset = 0x5926678;
uintptr_t gameInstanceOffset = 0xdf8;
uintptr_t localPlayersOffset = 0x38;
uintptr_t playerControllerOffset = 0x30;
uintptr_t characterOffset = 0x260;
uintptr_t turnRateCapOffset = 0x8c4;
uintptr_t turnRateCapTargetOffset = 0x8c8;
uintptr_t LookUpRateCapOffset = 0x8cc;
uintptr_t LookUpRateCapTargetOffset = 0x8d0;
uintptr_t longYellOnDeathOffset = 0xab0;



const char* uintptr_t_to_const_char_array(uintptr_t address) {
	
}

template<typename T> T RPM(uintptr_t address) {
	try { return *(T*)address; }
	catch (...) { return T(); }
}

template<typename T> void WPM(uintptr_t address, T value) {
	try { *(T*)address = value; }
	catch (...) { return; }
}

void InitImGui()
{
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags = ImGuiConfigFlags_NoMouseCursorChange;
	ImGui_ImplWin32_Init(window);
	ImGui_ImplDX11_Init(pDevice, pContext);
}

LRESULT __stdcall WndProc(const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	if (true && ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
		return true;

	return CallWindowProc(oWndProc, hWnd, uMsg, wParam, lParam);
}

bool init = false;
HRESULT __stdcall hkPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags)
{
	if (!init)
	{
		if (SUCCEEDED(pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)& pDevice)))
		{
			pDevice->GetImmediateContext(&pContext);
			DXGI_SWAP_CHAIN_DESC sd;
			pSwapChain->GetDesc(&sd);
			window = sd.OutputWindow;
			ID3D11Texture2D* pBackBuffer;
			pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)& pBackBuffer);
			pDevice->CreateRenderTargetView(pBackBuffer, NULL, &mainRenderTargetView);
			pBackBuffer->Release();
			oWndProc = (WNDPROC)SetWindowLongPtr(window, GWLP_WNDPROC, (LONG_PTR)WndProc);
			InitImGui();
			init = true;
		}

		else
			return oPresent(pSwapChain, SyncInterval, Flags);
	}

	if (GetAsyncKeyState(VK_HOME) & 1)
	{
		show = !show;
	}

	if (show) {
		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		ImGui::Begin("ImGui Window");
		gEngine = RPM<uintptr_t>(mordhauExeBase + gEngineOffset);
		std::stringstream ss;
		ss << std::hex << gEngine;
		std::string string = "0x" + ss.str();
		ImGui::LabelText("gEngineAddress", string.c_str());
		gameInstance = RPM<uintptr_t>(gEngine + gameInstanceOffset);
		localPlayers = RPM<uintptr_t>(gameInstance + localPlayersOffset);
		localPlayer = RPM<uintptr_t>(localPlayers);
		playerController = RPM<uintptr_t>(localPlayer + playerControllerOffset);
		character = RPM<uintptr_t>(playerController + characterOffset);
		
		ImGui::SliderFloat("turnRateCap", &turnRateCap, -1, 3000);
		ImGui::SliderFloat("turnRateCapTarget", &turnRateCapTarget, -1, 3000);
		ImGui::SliderFloat("LookUpRateCap", &LookUpRateCap, -1, 3000);
		ImGui::SliderFloat("LookUpRateCapTarget", &LookUpRateCapTarget, -1, 3000);
		ImGui::Checkbox("Long yell on death", &longYellOnDeath);

		ImGui::End();

		ImGui::Render();

		pContext->OMSetRenderTargets(1, &mainRenderTargetView, NULL);
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
	}
	WPM<float>(character + turnRateCapOffset, turnRateCap);
	WPM<float>(character + turnRateCapTargetOffset, turnRateCapTarget);
	WPM<float>(character + LookUpRateCapOffset, LookUpRateCap);
	WPM<float>(character + LookUpRateCapTargetOffset, LookUpRateCapTarget);
	WPM<bool>(character + longYellOnDeathOffset, longYellOnDeath);
	if (!alive) {
		kiero::shutdown();
		pDevice->Release();
		pContext->Release();
		pSwapChain->Release();
		oWndProc = (WNDPROC)SetWindowLongPtr(window, GWLP_WNDPROC, (LONG_PTR)(oWndProc));
		oPresent(pSwapChain, SyncInterval, Flags);
		return 0;
	}
	return oPresent(pSwapChain, SyncInterval, Flags);
}

DWORD WINAPI MainThread(LPVOID lpReserved)
{
	bool init_hook = false;
	do
	{
		if (kiero::init(kiero::RenderType::D3D11) == kiero::Status::Success)
		{
			kiero::bind(8, (void**)& oPresent, hkPresent);
			alive = true;
			Beep(1000, 200);

			init_hook = true;
		}
	} while (!init_hook);
	while (alive) {
		if (GetAsyncKeyState(VK_END) & 1) {
			Beep(1000, 200);
			alive = false;
			Sleep(1000);
			FreeLibraryAndExitThread((HMODULE)lpReserved, 0);
		}
	}
	return TRUE;
}

BOOL WINAPI DllMain(HMODULE hMod, DWORD dwReason, LPVOID lpReserved)
{
	switch (dwReason)
	{
	case DLL_PROCESS_ATTACH:
		DisableThreadLibraryCalls(hMod);
		CreateThread(nullptr, 0, MainThread, hMod, 0, nullptr);
		break;
	case DLL_PROCESS_DETACH:
		kiero::shutdown();
		break;
	}
	return TRUE;
}