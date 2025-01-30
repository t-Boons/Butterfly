#pragma once
#include "Butterfly.hpp"
#include "GLFW/glfw3.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_dx12.h"
#include "imgui/imgui_impl_glfw.h"

class ImGUIRenderer
{
public:
    void Init(Butterfly::Window* window)
    {
        using namespace Butterfly;

        m_window = window;

        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; 
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        ImGui_ImplGlfw_InitForOther(window->GLFWWindow(), true);

        ImGui_ImplDX12_InitInfo init_info;
        init_info.Device = DX12API()->Device();
        init_info.NumFramesInFlight = 1;
        init_info.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
        init_info.SrvDescriptorHeap = DX12API()->DescriptorAllocatorSrvCbvUav()->Heap().Get();
        init_info.CommandQueue = DX12API()->Queue(QueueType::Direct)->D3D12Queue();
        init_info.LegacySingleSrvCpuDescriptor = DX12API()->DescriptorAllocatorSrvCbvUav()->Heap()->GetCPUDescriptorHandleForHeapStart();
        init_info.LegacySingleSrvGpuDescriptor = DX12API()->DescriptorAllocatorSrvCbvUav()->Heap()->GetGPUDescriptorHandleForHeapStart();
        ImGui_ImplDX12_Init(&init_info);
    }

    void BeginFrame() 
    {
        ImGui_ImplDX12_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
    }

    void EndFrame(Butterfly::DX12CommandList& list, Butterfly::BFTexture & rt)
    {
        using namespace Butterfly;

        // Rendering
        ImGui::Render();
        ID3D12DescriptorHeap* heaps[] = { DX12API()->DescriptorAllocatorSrvCbvUav()->Heap().Get()};
        list.List()->SetDescriptorHeaps(_countof(heaps), heaps);
        GraphicsCommands::SetRenderTargets(list, { &rt}, nullptr);
        ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), list.List());
    }

    ~ImGUIRenderer()
    {
        ImGui_ImplDX12_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }

private:
    Butterfly::Window* m_window;
};