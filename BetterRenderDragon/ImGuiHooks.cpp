#include <windows.h>
#include <psapi.h>
#include <atlbase.h>
#include <initguid.h>
#include <d3d12.h>
#include <d3d11.h>

#include <string>

#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "imgui/backends/imgui_impl_dx12.h"
#include "imgui/backends/imgui_impl_dx11.h"
#include "imgui_impl_winrt.h"

#include "ImGuiHooks.h"
#include "HookAPI.h"
#include "Options.h"
#include "Util.h"
#include "Version.h"

//=======================================================================================================================================================================

IDXGIFactory2* Factory;
IUnknown* CoreWindow;
bool ImGuiInitialized = false;
bool IsChangingUIKey = false;
bool JustChangedKey = false;

std::string CPUName;
std::string GPUName;
std::string RendererType;

//=======================================================================================================================================================================

//std::atomic_bool running = true;
//DWORD WINAPI trySaveOptions(LPVOID lpThreadParameter) {
//	while (running) {
//		if (Options::dirty) {
//			Options::save();
//			Options::dirty = false;
//		}
//		Sleep(1000);
//	}
//	return 0;
//}

//=======================================================================================================================================================================

void updateOptions() {
	static float saveTimer = 0.0f;

	static bool showImGui = Options::showImGui;
	static bool performanceEnabled = Options::performanceEnabled;
	static bool vanilla2DeferredEnabled = Options::vanilla2DeferredEnabled;
	static bool deferredRenderingEnabled = Options::deferredRenderingEnabled;
	static bool limitShaderModel = Options::limitShaderModel;
	static bool disableRendererContextD3D12RTX = Options::disableRendererContextD3D12RTX;
	static bool materialBinLoaderEnabled = Options::materialBinLoaderEnabled;
	static bool redirectShaders = Options::redirectShaders;
	static bool customUniformsEnabled = Options::customUniformsEnabled;
	static bool windowSettingsEnabled = Options::windowSettingsEnabled;

	if (showImGui != Options::showImGui
		|| performanceEnabled != Options::performanceEnabled
		|| vanilla2DeferredEnabled != Options::vanilla2DeferredEnabled
		|| deferredRenderingEnabled != Options::deferredRenderingEnabled
		|| limitShaderModel != Options::limitShaderModel
		|| disableRendererContextD3D12RTX != Options::disableRendererContextD3D12RTX
		|| materialBinLoaderEnabled != Options::materialBinLoaderEnabled
		|| redirectShaders != Options::redirectShaders
		|| customUniformsEnabled != Options::customUniformsEnabled
		|| windowSettingsEnabled != Options::windowSettingsEnabled) {

		Options::dirty = true;
		saveTimer = 3.0f;

		showImGui = Options::showImGui;
		performanceEnabled = Options::performanceEnabled;
		vanilla2DeferredEnabled = Options::vanilla2DeferredEnabled;
		deferredRenderingEnabled = Options::deferredRenderingEnabled;
		limitShaderModel = Options::limitShaderModel;
		disableRendererContextD3D12RTX = Options::disableRendererContextD3D12RTX;
		materialBinLoaderEnabled = Options::materialBinLoaderEnabled;
		redirectShaders = Options::redirectShaders;
		customUniformsEnabled = Options::customUniformsEnabled;
		windowSettingsEnabled = Options::windowSettingsEnabled;
	}

	//TODO: Put it on a separate thread
	if (saveTimer > 0.0f) {
		saveTimer -= ImGui::GetIO().DeltaTime;
		if (saveTimer <= 0.0f) {
			saveTimer = 0.0f;
			if (Options::dirty) {
				Options::save();
				Options::dirty = false;
			}
		}
	}
}


void updateKeys() {
	static bool prevToggleImGui = false;
	static bool prevToggleDeferredRendering = false;

	bool toggleImGui = ImGui::IsKeyPressed((ImGuiKey)Options::uikey) && !JustChangedKey;
	if (!toggleImGui)
		JustChangedKey = false;
	if (toggleImGui && !prevToggleImGui)
		Options::showImGui = !Options::showImGui;
	prevToggleImGui = toggleImGui;

	bool toggleDR = ImGui::IsKeyDown(ImGuiKey_LeftCtrl) && ImGui::IsKeyDown(ImGuiKey_Semicolon);
	if (toggleDR && !prevToggleDeferredRendering)
		Options::deferredRenderingEnabled = !Options::deferredRenderingEnabled;
	prevToggleDeferredRendering = toggleDR;
}

void updateImGui() {
	//static bool showDemo = false;
	static bool showModuleManager = false;
	static bool showAbout = false;

	bool resetLayout = false;
	bool moduleManagerRequestFocus = false;
	bool aboutRequestFocus = false;

	updateKeys();
	updateOptions();

	ImGui::NewFrame();
	if (Options::showImGui) {
		auto& io = ImGui::GetIO();

		ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(ImVec2(300, 250), ImGuiCond_FirstUseEver);
		if (ImGui::Begin("BetterRenderDragon", &Options::showImGui, ImGuiWindowFlags_MenuBar)) {
			if (ImGui::BeginMenuBar()) {
				if (ImGui::BeginMenu("View")) {
					if (ImGui::MenuItem("Open Module Manager", NULL)) {
						showModuleManager = true;
						moduleManagerRequestFocus = true;
					}
					//if (ImGui::MenuItem("Open ImGui Demo Window", NULL)) {
					//	showDemo = true;
					//}
					ImGui::EndMenu();
				}
				if (ImGui::BeginMenu("Windows")) {
					if (ImGui::MenuItem("Reset window position and size", NULL))
						resetLayout = true;
					if (showModuleManager || showAbout/* || showDemo*/)
						ImGui::Separator();
					if (showModuleManager && ImGui::MenuItem("Module Manager", NULL))
						moduleManagerRequestFocus = true;
					if (showAbout && ImGui::MenuItem("About", NULL))
						aboutRequestFocus = true;
					//if (showDemo)
					//	ImGui::MenuItem("ImGui Demo Window", NULL);
					ImGui::EndMenu();
				}
				if (ImGui::BeginMenu("Help")) {
					if (ImGui::MenuItem("About", NULL)) {
						showAbout = true;
						aboutRequestFocus = true;
					}
					ImGui::EndMenu();
				}
				ImGui::EndMenuBar();
			}

			ImGui::Text("BetterRenderDragon %s", BetterRDVersion);
			ImGui::NewLine();

			if (Options::performanceEnabled && ImGui::CollapsingHeader("Performance", ImGuiTreeNodeFlags_DefaultOpen)) {
				ImGui::Indent();
				ImGui::Text("FPS: %.01f", io.Framerate);
				ImGui::Text("Frame Time: %.01fms", io.DeltaTime * 1000.0f);
				ImGui::Unindent();
			}

			if (Options::vanilla2DeferredEnabled && ImGui::CollapsingHeader("Vanilla2Deferred", ImGuiTreeNodeFlags_DefaultOpen)) {
				if (!Options::vanilla2DeferredAvailable)
					ImGui::BeginDisabled();
				ImGui::Indent();
				//ImGui::Checkbox("Enable Deferred Rendering", &Options::deferredRenderingEnabled);
				ImGui::Checkbox("Disable RTX (Requires restart)", &Options::disableRendererContextD3D12RTX);
				ImGui::Unindent();
				if (!Options::vanilla2DeferredAvailable)
					ImGui::EndDisabled();
			}

			if (Options::materialBinLoaderEnabled && ImGui::CollapsingHeader("MaterialBinLoader", ImGuiTreeNodeFlags_DefaultOpen)) {
				ImGui::Indent();
				ImGui::Checkbox("Load shaders from resource pack", &Options::redirectShaders);
				ImGui::Unindent();
			}

#if 0
			if (Options::windowSettingsEnabled && ImGui::CollapsingHeader("Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
				ImGui::Indent();
				if (ImGui::Button(IsChangingUIKey == false ? "Set Open UI Key" : "Cancel")) {
					IsChangingUIKey = !IsChangingUIKey;
				}
				if (IsChangingUIKey) {
					for (int i = ImGuiKey_NamedKey_BEGIN; i < ImGuiKey_COUNT; i++) {
						if (ImGui::IsKeyDown((ImGuiKey)i) && !ImGui::IsMouseKey((ImGuiKey)i)) {
							Options::uikey = i;
							IsChangingUIKey = false;
							ImGui::GetIO().AddKeyEvent((ImGuiKey)Options::uikey, false);
							JustChangedKey = true;
							break;
						}
					}
				}
				ImGui::Text("UI Key: %s", ImGui::GetKeyName((ImGuiKey)Options::uikey));
				ImGui::Unindent();
			}
#endif

			if (Options::customUniformsEnabled && ImGui::CollapsingHeader("CustomUniforms", ImGuiTreeNodeFlags_DefaultOpen)) {
				ImGui::Indent();
				ImGui::Unindent();
			}
			ImGui::NewLine();
		}
		ImGui::End();

		if (showModuleManager) {
			if (moduleManagerRequestFocus)
				ImGui::SetNextWindowFocus();
			ImGui::SetNextWindowSize(ImVec2(300, 130), ImGuiCond_FirstUseEver);
			if (ImGui::Begin("BetterRenderDragon - Module Manager", &showModuleManager)) {
				if (ImGui::BeginTable("modulesTable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
					ImGui::TableSetupColumn("Module");
					ImGui::TableSetupColumn("Enabled");
					ImGui::TableHeadersRow();

					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0); ImGui::Text("Performance");
					ImGui::TableSetColumnIndex(1); ImGui::Checkbox("##1", &Options::performanceEnabled);

					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0); ImGui::Text("Vanilla2Deferred");
					ImGui::TableSetColumnIndex(1); ImGui::Checkbox("##2", &Options::vanilla2DeferredEnabled);

					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0); ImGui::Text("MaterialBinLoader");
					ImGui::TableSetColumnIndex(1); ImGui::Checkbox("##3", &Options::materialBinLoaderEnabled);

#if 0
					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0); ImGui::Text("Settings");
					ImGui::TableSetColumnIndex(1); ImGui::Checkbox("##4", &Options::windowSettingsEnabled);
#endif

					//ImGui::TableNextRow();
					//ImGui::TableSetColumnIndex(0); ImGui::Text("CustomUniforms");
					//ImGui::TableSetColumnIndex(1); ImGui::Checkbox("", &Options::customUniformsEnabled);

					ImGui::EndTable();
				}
			}
			ImGui::End();
		}

		if (showAbout) {
			if (aboutRequestFocus)
				ImGui::SetNextWindowFocus();
			if (ImGui::Begin("BetterRenderDragon - About", &showAbout)) {
				ImGui::Text("BetterRenderDragon %s", BetterRDVersion);
				ImGui::Text("https://github.com/ddf8196/BetterRenderDragon");
			}
			ImGui::End();
		}

		//if (showDemo)
		//	ImGui::ShowDemoWindow(&showDemo);
	}
	ImGui::EndFrame();

	if (resetLayout) {
		ImGui::ClearWindowSettings("BetterRenderDragon");
		ImGui::ClearWindowSettings("Dear ImGui Demo");
	}
}

void initializeImgui() {
	ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO();
	io.IniFilename = nullptr;
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
}

//=======================================================================================================================================================================

namespace ImGuiD3D12 {
	CComPtr<ID3D12Device> Device;
	CComPtr<ID3D12DescriptorHeap> DescriptorHeapBackBuffers;
	CComPtr<ID3D12DescriptorHeap> DescriptorHeapImGuiRender;
	CComPtr<ID3D12CommandAllocator> CommandAllocator;
	CComPtr<ID3D12GraphicsCommandList> CommandList;

	ID3D12CommandQueue* CommandQueue;

	struct BackBufferContext {
		ID3D12Resource* Resource;
		D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHandle;
	};

	uint32_t BufferCount = 0;
	BackBufferContext* BufferContext;

	void CreateRT(IDXGISwapChain* swapChain) {
		const auto RTVDescriptorSize = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		D3D12_CPU_DESCRIPTOR_HANDLE RTVHandle = DescriptorHeapBackBuffers->GetCPUDescriptorHandleForHeapStart();

		for (uint32_t i = 0; i < BufferCount; i++) {
			ID3D12Resource* pBackBuffer = nullptr;
			BufferContext[i].DescriptorHandle = RTVHandle;
			swapChain->GetBuffer(i, IID_PPV_ARGS(&pBackBuffer));
			Device->CreateRenderTargetView(pBackBuffer, nullptr, RTVHandle);
			BufferContext[i].Resource = pBackBuffer;
			RTVHandle.ptr += RTVDescriptorSize;
		}
	}

	void ReleaseRT() {
		for (size_t i = 0; i < BufferCount; i++) {
			if (BufferContext[i].Resource) {
				BufferContext[i].Resource->Release();
				BufferContext[i].Resource = nullptr;
			}
		}
	}

	bool initializeImguiBackend(IDXGISwapChain* pSwapChain) {
		if (SUCCEEDED(pSwapChain->GetDevice(IID_ID3D12Device, (void**)&Device))) {
			bool dxr11 = false;
			D3D12_FEATURE_DATA_D3D12_OPTIONS5 options5 = { 0 };
			if (SUCCEEDED(Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &options5, sizeof(options5)))) {
				dxr11 = options5.RaytracingTier >= 11;
			}
			Options::vanilla2DeferredAvailable = true;
			Options::limitShaderModel = !dxr11;

			initializeImgui();

			DXGI_SWAP_CHAIN_DESC Desc;
			pSwapChain->GetDesc(&Desc);

			BufferCount = Desc.BufferCount;
			BufferContext = new BackBufferContext[BufferCount];

			D3D12_DESCRIPTOR_HEAP_DESC DescriptorImGuiRender = {};
			DescriptorImGuiRender.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
			DescriptorImGuiRender.NumDescriptors = BufferCount;
			DescriptorImGuiRender.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

			if (Device->CreateDescriptorHeap(&DescriptorImGuiRender, IID_PPV_ARGS(&DescriptorHeapImGuiRender)) != S_OK) {
				return false;
			}

			if (Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&CommandAllocator)) != S_OK) {
				return false;
			}

			if (Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, CommandAllocator, NULL, IID_PPV_ARGS(&CommandList)) != S_OK ||
				CommandList->Close() != S_OK) {
				return false;
			}

			D3D12_DESCRIPTOR_HEAP_DESC DescriptorBackBuffers;
			DescriptorBackBuffers.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
			DescriptorBackBuffers.NumDescriptors = BufferCount;
			DescriptorBackBuffers.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
			DescriptorBackBuffers.NodeMask = 1;

			if (Device->CreateDescriptorHeap(&DescriptorBackBuffers, IID_PPV_ARGS(&DescriptorHeapBackBuffers)) != S_OK) {
				return false;
			}

			CreateRT(pSwapChain);

			ImGui_ImplWinRT_Init(CoreWindow);
			ImGui_ImplDX12_Init(Device, BufferCount, DXGI_FORMAT_R8G8B8A8_UNORM, DescriptorHeapImGuiRender, DescriptorHeapImGuiRender->GetCPUDescriptorHandleForHeapStart(), DescriptorHeapImGuiRender->GetGPUDescriptorHandleForHeapStart());
			ImGui_ImplDX12_CreateDeviceObjects();
		}
		return true;
	}

	void renderImGui(IDXGISwapChain3* swapChain) {
		ImGui_ImplDX12_NewFrame();
		ImGui_ImplWinRT_NewFrame();

		updateImGui();

		BackBufferContext& CurrentBufferContext = BufferContext[swapChain->GetCurrentBackBufferIndex()];
		CommandAllocator->Reset();

		D3D12_RESOURCE_BARRIER Barrier;
		Barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		Barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		Barrier.Transition.pResource = CurrentBufferContext.Resource;
		Barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		Barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
		Barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;

		ID3D12DescriptorHeap* descriptorHeaps[1] = { DescriptorHeapImGuiRender.p };
		CommandList->Reset(CommandAllocator, nullptr);
		CommandList->ResourceBarrier(1, &Barrier);
		CommandList->OMSetRenderTargets(1, &CurrentBufferContext.DescriptorHandle, FALSE, nullptr);
		CommandList->SetDescriptorHeaps(1, descriptorHeaps);

		ImGui::Render();
		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), CommandList);

		ID3D12CommandList* commandLists[1] = { CommandList.p };
		Barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		Barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		CommandList->ResourceBarrier(1, &Barrier);
		CommandList->Close();
		CommandQueue->ExecuteCommandLists(1, commandLists);
	}

	PFN_IDXGISwapChain_Present Original_IDXGISwapChain_Present = nullptr;
	HRESULT STDMETHODCALLTYPE IDXGISwapChain_Present_Hook(IDXGISwapChain* This, UINT SyncInterval, UINT Flags) {
		CComPtr<IDXGISwapChain3> swapChain3;
		if (FAILED(This->QueryInterface<IDXGISwapChain3>(&swapChain3))) {
			return Original_IDXGISwapChain_Present(This, SyncInterval, Flags);
		}

		if (!ImGuiInitialized) {
			printf("Initializing ImGui on Direct3D 12\n");
			if (initializeImguiBackend(This)) {
				ImGuiInitialized = true;
			} else {
				printf("ImGui is not initialized\n");
				return Original_IDXGISwapChain_Present(This, SyncInterval, Flags);
			}
		}

		renderImGui(swapChain3);

		return Original_IDXGISwapChain_Present(This, SyncInterval, Flags);
	}

	PFN_IDXGISwapChain_ResizeBuffers Original_IDXGISwapChain_ResizeBuffers;
	HRESULT STDMETHODCALLTYPE IDXGISwapChain_ResizeBuffers_Hook(IDXGISwapChain* This, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags) {
		ReleaseRT();
		HRESULT hResult = Original_IDXGISwapChain_ResizeBuffers(This, BufferCount, Width, Height, NewFormat, SwapChainFlags);
		CreateRT(This);
		return hResult;
	}
}

//=======================================================================================================================================================================

namespace ImGuiD3D11 {
	ID3D11Device* Device;
	CComPtr<ID3D11DeviceContext> DeviceContext;

	uint32_t BufferCount = 0;
	ID3D11RenderTargetView** RenderTargetViews;

	void CreateRT(IDXGISwapChain* swapChain) {
		for (uint32_t i = 0; i < BufferCount; i++) {
			CComPtr<ID3D11Resource> backBuffer;
			swapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffer));

			ID3D11RenderTargetView* rtv;
			Device->CreateRenderTargetView(backBuffer, nullptr, &rtv);

			RenderTargetViews[i] = rtv;
		}
	}

	void ReleaseRT() {
		for (size_t i = 0; i < BufferCount; i++) {
			if (RenderTargetViews[i]) {
				RenderTargetViews[i]->Release();
				RenderTargetViews[i] = nullptr;
			}
		}
	}

	bool initializeImguiBackend(IDXGISwapChain* pSwapChain) {
		Options::vanilla2DeferredAvailable = false;
		Options::limitShaderModel = false;

		initializeImgui();

		DXGI_SWAP_CHAIN_DESC Desc;
		pSwapChain->GetDesc(&Desc);

		BufferCount = Desc.BufferCount;
		RenderTargetViews = new ID3D11RenderTargetView*[BufferCount];

		Device->GetImmediateContext(&DeviceContext);

		CreateRT(pSwapChain);

		ImGui_ImplWinRT_Init(CoreWindow);
		ImGui_ImplDX11_Init(Device, DeviceContext);
		ImGui_ImplDX11_CreateDeviceObjects();

		return true;
	}

	void renderImGui(IDXGISwapChain3* swapChain) {
		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWinRT_NewFrame();

		updateImGui();

		ID3D11RenderTargetView* currentRTV = RenderTargetViews[swapChain->GetCurrentBackBufferIndex()];
		DeviceContext->OMSetRenderTargets(1, &currentRTV, nullptr);

		ImGui::Render();
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
	}

	PFN_IDXGISwapChain_Present Original_IDXGISwapChain_Present = nullptr;
	HRESULT STDMETHODCALLTYPE IDXGISwapChain_Present_Hook(IDXGISwapChain* This, UINT SyncInterval, UINT Flags) {
		CComPtr<IDXGISwapChain3> swapChain3;
		if (FAILED(This->QueryInterface<IDXGISwapChain3>(&swapChain3))) {
			return Original_IDXGISwapChain_Present(This, SyncInterval, Flags);
		}

		if (ImGuiInitialized) {
			renderImGui(swapChain3);
		}

		return Original_IDXGISwapChain_Present(This, SyncInterval, Flags);
	}

	PFN_IDXGISwapChain_ResizeBuffers Original_IDXGISwapChain_ResizeBuffers;
	HRESULT STDMETHODCALLTYPE IDXGISwapChain_ResizeBuffers_Hook(IDXGISwapChain* This, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags) {
		ReleaseRT();
		HRESULT hResult = Original_IDXGISwapChain_ResizeBuffers(This, BufferCount, Width, Height, NewFormat, SwapChainFlags);
		CreateRT(This);
		return hResult;
	}
}

//=======================================================================================================================================================================

PFN_IDXGIFactory2_CreateSwapChainForCoreWindow Original_IDXGIFactory2_CreateSwapChainForCoreWindow;
HRESULT STDMETHODCALLTYPE IDXGIFactory2_CreateSwapChainForCoreWindow_Hook(IDXGIFactory2* This, IUnknown* pDevice, IUnknown* pWindow, const DXGI_SWAP_CHAIN_DESC1* pDesc, IDXGIOutput* pRestrictToOutput, IDXGISwapChain1** ppSwapChain) {
	if (pWindow)
		CoreWindow = pWindow;

	HRESULT hResult = Original_IDXGIFactory2_CreateSwapChainForCoreWindow(This, pDevice, pWindow, pDesc, pRestrictToOutput, ppSwapChain);
	if (SUCCEEDED(hResult)) {
		IDXGISwapChain1* swapChain = *ppSwapChain;
		CComPtr<ID3D12CommandQueue> d3d12CommandQueue;
		CComPtr<ID3D11Device> d3d11Device;
		if (SUCCEEDED(pDevice->QueryInterface(&d3d12CommandQueue))) {
			//Direct3D 12
			ImGuiD3D12::CommandQueue = (ID3D12CommandQueue*)pDevice;
			if (!ImGuiD3D12::Original_IDXGISwapChain_Present)
				ReplaceVtable(*(void**)swapChain, 8, (void**)&ImGuiD3D12::Original_IDXGISwapChain_Present, ImGuiD3D12::IDXGISwapChain_Present_Hook);
			if (!ImGuiD3D12::Original_IDXGISwapChain_ResizeBuffers)
				ReplaceVtable(*(void**)swapChain, 13, (void**)&ImGuiD3D12::Original_IDXGISwapChain_ResizeBuffers, ImGuiD3D12::IDXGISwapChain_ResizeBuffers_Hook);
			//When the graphics API used by RenderDragon is D3D12, this function will be called in a non-main thread and later IDXGISwapChain::Present will be called three times in the main thread, so initialize ImGui later in IDXGISwapChain::Present
		} else if (SUCCEEDED(pDevice->QueryInterface(&d3d11Device))) {
			//Direct3D 11
			ImGuiD3D11::Device = (ID3D11Device*)pDevice;
			if (!ImGuiD3D11::Original_IDXGISwapChain_Present)
				ReplaceVtable(*(void**)swapChain, 8, (void**)&ImGuiD3D11::Original_IDXGISwapChain_Present, ImGuiD3D11::IDXGISwapChain_Present_Hook);
			if (!ImGuiD3D11::Original_IDXGISwapChain_ResizeBuffers)
				ReplaceVtable(*(void**)swapChain, 13, (void**)&ImGuiD3D11::Original_IDXGISwapChain_ResizeBuffers, ImGuiD3D11::IDXGISwapChain_ResizeBuffers_Hook);
			// When the graphics API used by RenderDragon is D3D11, this function will be called in the main thread, and IDXGISwapChain::Present will all be called in a non-main thread, so initialize ImGui here immediately
			printf("Initializing ImGui on Direct3D 11\n");
			if (!(ImGuiInitialized = ImGuiD3D11::initializeImguiBackend(swapChain))) {
				printf("Failed to initialize ImGui on Direct3D 11\n");
			}
		} else {
			
		}
	}
	return hResult;
}

//=======================================================================================================================================================================

DeclareHook(CreateDXGIFactory1, HRESULT, REFIID riid, void** ppFactory) {
	HRESULT hResult = original(riid, ppFactory);
	if (IsEqualIID(IID_IDXGIFactory2, riid) && SUCCEEDED(hResult)) {
		printf("CreateDXGIFactory1 riid=IID_IDXGIFactory2\n");
		IDXGIFactory2* factory = (IDXGIFactory2*)*ppFactory;
		if (!Original_IDXGIFactory2_CreateSwapChainForCoreWindow)
			ReplaceVtable(*(void**)factory, 16, (void**)&Original_IDXGIFactory2_CreateSwapChainForCoreWindow, IDXGIFactory2_CreateSwapChainForCoreWindow_Hook);

		Factory = factory;
	}
	return hResult;
}

//=======================================================================================================================================================================

void ImGuiHooks_Init() {
	printf("%s\n", __FUNCTION__);

	HMODULE dxgiModule = GetModuleHandleA("dxgi.dll");
	if (!dxgiModule) {
		return;
	}

	FARPROC CreateDXGIFactory1 = GetProcAddress(dxgiModule, "CreateDXGIFactory1");
	if (!CreateDXGIFactory1)
		return;

	Hook(CreateDXGIFactory1, CreateDXGIFactory1);
}

//=======================================================================================================================================================================
