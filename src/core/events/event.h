#pragma once

template <typename... Args>
class EventConnection
{
public:
    using DisconnectFunc = std::function<void()>;

    EventConnection() = default;

    explicit EventConnection(DisconnectFunc disconnectFunc)
        : m_disconnectFunc(std::move(disconnectFunc))
    {
    }

    ~EventConnection()
    {
        disconnect();
    }

    EventConnection(const EventConnection&) = delete;
    EventConnection& operator=(const EventConnection&) = delete;

    EventConnection(EventConnection&& other) noexcept
        : m_disconnectFunc(std::move(other.m_disconnectFunc))
    {
        other.m_disconnectFunc = nullptr;
    }

    EventConnection& operator=(EventConnection&& other) noexcept
    {
        if (this != &other)
        {
            disconnect();
            m_disconnectFunc = std::move(other.m_disconnectFunc);
            other.m_disconnectFunc = nullptr;
        }
        return *this;
    }

    void disconnect()
    {
        if (m_disconnectFunc)
        {
            m_disconnectFunc();
            m_disconnectFunc = nullptr;
        }
    }

    bool isConnected() const
    {
        return m_disconnectFunc != nullptr;
    }

private:
    DisconnectFunc m_disconnectFunc;
};

template <typename... Args>
class Event
{
public:
    using Connection = EventConnection<Args...>;
    using Handler = std::function<void(Args...)>;

private:
    struct HandlerSlot
    {
        std::shared_ptr<Handler> handler;
        std::atomic<bool> active{true};
        size_t id;

        HandlerSlot(Handler h, size_t slot_id)
            : handler(std::make_shared<Handler>(std::move(h)))
            , id(slot_id)
        {
        }

        // Disable copy, enable move
        HandlerSlot(const HandlerSlot&) = delete;
        HandlerSlot& operator=(const HandlerSlot&) = delete;

        HandlerSlot(HandlerSlot&& other) noexcept
            : handler(std::move(other.handler))
            , active(other.active.load())
            , id(other.id)
        {
        }

        HandlerSlot& operator=(HandlerSlot&& other) noexcept
        {
            if (this != &other)
            {
                handler = std::move(other.handler);
                active.store(other.active.load());
                id = other.id;
            }
            return *this;
        }
    };

    mutable std::mutex m_mutex;
    std::vector<HandlerSlot> m_handlers;
    std::atomic<size_t> m_nextId{1};

public:
    Event() = default;

    // Disable copy, enable move
    Event(const Event&) = delete;
    Event& operator=(const Event&) = delete;
    Event(Event&&) = default;
    Event& operator=(Event&&) = default;

    template <typename Callable>
    _NODISCARD Connection connect(Callable&& callable)
    {
        static_assert(std::is_invocable_v<Callable, Args...>,
                      "Callable must be invocable with the event's argument types");

        std::lock_guard lock(m_mutex);

        size_t id = m_nextId++;

        // Convert callable to std::function
        Handler handler = std::forward<Callable>(callable);
        m_handlers.emplace_back(std::move(handler), id);

        // Return connection that can disconnect this specific handler
        return Connection([this, id]
        {
            disconnectHandler(id);
        });
    }

    // Connect a free function
    template <auto FuncPtr>
    _NODISCARD Connection connect()
    {
        return connect(FuncPtr);
    }

    // Connect a member function
    template <auto MemPtr, typename T>
    _NODISCARD Connection connect(T* instance)
    {
        return connect([instance](Args... args)
        {
            (instance->*MemPtr)(args...);
        });
    }

    // template <auto FuncPtr>
    // void addListener()
    // {
    //     this->connect<FuncPtr>();
    // }
    //
    // template <typename T, auto MemPtr>
    // void addListener(T* instance)
    // {
    //     this->template connect<MemPtr>(instance);
    // }
    //
    // template <typename Callable>
    // void addListener(Callable&& callable)
    // {
    //     this->connect(std::forward<Callable>(callable));
    // }

    template <typename... UArgs>
    void operator()(UArgs&&... args)
    {
        // Create a copy of active handlers to avoid holding lock during execution
        std::vector<std::shared_ptr<Handler>> activeHandlers;

        {
            std::lock_guard lock(m_mutex);
            activeHandlers.reserve(m_handlers.size());

            for (const auto& slot : m_handlers)
            {
                if (slot.active.load())
                {
                    activeHandlers.push_back(slot.handler);
                }
            }
        }

        // Execute handlers without holding the lock
        for (const auto& handler : activeHandlers)
        {
            try
            {
                (*handler)(std::forward<UArgs>(args)...);
            }
            catch (const std::exception& e)
            {
                LOG_ERROR("Event handler exception: {}", e.what());
            }
            catch (...)
            {
                LOG_ERROR("Unknown event handler exception");
            }
        }
    }

    void clear()
    {
        std::lock_guard lock(m_mutex);
        for (auto& slot : m_handlers)
        {
            slot.active.store(false);
        }
        m_handlers.clear();
    }

    _NODISCARD bool empty() const
    {
        std::lock_guard lock(m_mutex);
        return m_handlers.empty();
    }

private:
    void disconnectHandler(size_t id)
    {
        std::lock_guard lock(m_mutex);

        for (auto it = m_handlers.begin(); it != m_handlers.end(); ++it)
        {
            if (it->id == id)
            {
                it->active.store(false);
                m_handlers.erase(it);
                break;
            }
        }
    }
};

// For single threaded use/no locking
template <typename... Args>
class FastEvent
{
public:
    using Connection = EventConnection<Args...>;
    using Handler = std::function<void(Args...)>;

private:
    struct HandlerSlot
    {
        Handler handler;
        size_t id;

        HandlerSlot(Handler h, size_t slotId)
            : handler(std::move(h))
            , id(slotId)
        {
        }

        // Disable copy, enable move
        HandlerSlot(const HandlerSlot&) = delete;
        HandlerSlot& operator=(const HandlerSlot&) = delete;

        HandlerSlot(HandlerSlot&& other) noexcept
            : handler(std::move(other.handler))
            , id(other.id)
        {
        }

        HandlerSlot& operator=(HandlerSlot&& other) noexcept
        {
            if (this != &other)
            {
                handler = std::move(other.handler);
                id = other.id;
            }
            return *this;
        }
    };

    std::vector<HandlerSlot> m_handlers;
    size_t m_nextId{1};

public:
    template <typename Callable>
    _NODISCARD Connection connect(Callable&& callable)
    {
        static_assert(std::is_invocable_v<Callable, Args...>,
                      "Callable must be invocable with the event's argument types");

        size_t id = m_nextId++;
        Handler handler = std::forward<Callable>(callable);
        m_handlers.emplace_back(std::move(handler), id);

        return Connection([this, id]()
        {
            disconnectHandler(id);
        });
    }

    template <auto FuncPtr>
    _NODISCARD Connection connect()
    {
        return connect(FuncPtr);
    }

    template <auto MemPtr, typename T>
    _NODISCARD Connection connect(T* instance)
    {
        return connect([instance](Args... args)
        {
            (instance->*MemPtr)(args...);
        });
    }

    template <typename... UArgs>
    void operator()(UArgs&&... args)
    {
        // Execute handlers directly
        for (const auto& slot : m_handlers)
        {
            try
            {
                slot.handler(std::forward<UArgs>(args)...);
            }
            catch (const std::exception& e)
            {
                LOG_ERROR("Event handler exception: {}", e.what());
            }
            catch (...)
            {
                LOG_ERROR("Unknown event handler exception");
            }
        }
    }

    void clear()
    {
        m_handlers.clear();
    }

    _NODISCARD bool empty() const
    {
        return m_handlers.empty();
    }

private:
    void disconnectHandler(size_t id)
    {
        m_handlers.erase(
            std::remove_if(m_handlers.begin(), m_handlers.end(),
                           [id](const HandlerSlot& slot) { return slot.id == id; }),
            m_handlers.end()
            );
    }
};
