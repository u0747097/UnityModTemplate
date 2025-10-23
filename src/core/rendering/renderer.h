#pragma once
#include "backend/renderer_backend.h"
#include "utils/dx_utils.h"

class Renderer
{
public:
    static Renderer& getInstance();

    // Initialize the renderer with automatic backend detection
    bool initialize();

    // Initialize with specific backend
    bool initialize(RenderAPI api);

    // Shutdown the renderer
    void shutdown();

    // Check if initialized
    bool isInitialized() const { return m_initialized; }

    // Get current backend
    IRendererBackend* getBackend() const { return m_backend.get(); }

    // Get current render API
    RenderAPI getRenderAPI() const { return m_currentAPI; }

    // Handle window resize
    void onResize(int width, int height);

private:
    Renderer() = default;
    ~Renderer();

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    bool m_initialized = false;
    RenderAPI m_currentAPI = RenderAPI::Unknown;
    std::unique_ptr<IRendererBackend> m_backend;

    static std::unique_ptr<IRendererBackend> createBackend(RenderAPI api);
};
