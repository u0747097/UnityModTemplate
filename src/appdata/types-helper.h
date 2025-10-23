#pragma once

#pragma region Class Helpers

// Used to directly declare a class that can be resolved from its module and class name
#define UNITY_CLASS_DECL(MODULE, CLASS_NAME) \
private:\
    inline static constexpr const char* ModuleName = MODULE; \
    inline static constexpr const char* ClassName = CLASS_NAME; \
public: \
    inline static const char* getClassName() { return ClassName; } \
    inline static UnityResolve::Class* getClass() { \
        static UnityResolve::Class* c = nullptr; \
        if (!c) c = app::getClass(MODULE, CLASS_NAME); \
        return c; \
	}

// Used to declare a class that can be resolved from a field name in another class`
#define UNITY_CLASS_DECL_FROM_FIELD_NAME(MODULE, CONTAINER_CLASS_NAME, FIELD_NAME) \
private: \
    inline static constexpr const char* ModuleName = MODULE; \
    inline static constexpr const char* ContainerClassName = CONTAINER_CLASS_NAME; \
    inline static constexpr const char* FieldName = FIELD_NAME; \
    inline static const char* ClassName; \
public: \
    inline static const char* getClassName() { return ClassName; } \
    inline static UnityResolve::Class* getClass() { \
        static UnityResolve::Class* cachedClass = nullptr; \
        if (!cachedClass) { \
            cachedClass = app::findClassFromField(ModuleName, ContainerClassName, FieldName); \
            ClassName = cachedClass ? cachedClass->name.c_str() : ""; \
        } \
        return cachedClass; \
    }

// Used to declare a class that can be resolved from a set of field names
#define UNITY_CLASS_DECL_FROM_FIELDS(MODULE, MIN_MATCHES, ... ) \
private: \
    inline static constexpr const char* ModuleName = MODULE; \
    inline static const std::vector<std::string> RequiredFields = { __VA_ARGS__ }; \
    inline static const char* ClassName; \
public: \
    inline static const char* getClassName() { return ClassName; } \
	inline static UnityResolve::Class* getClass() { \
		static UnityResolve::Class* cachedClass = nullptr; \
		if (!cachedClass) { \
			cachedClass = app::findClassByFields(ModuleName, RequiredFields, MIN_MATCHES); \
			ClassName = cachedClass ? cachedClass->name.c_str() : ""; \
		} \
		return cachedClass; \
	}

#pragma endregion

#pragma region Method Helpers

// Used to initialize a method pointer for a class that's declared with UNITY_CLASS_DECL
#define UNITY_METHOD(RETURN_TYPE, METHOD_NAME, ...) \
private: \
    inline static UnityResolve::Method* METHOD_NAME##_method_ptr = nullptr; \
    inline static UnityResolve::MethodPointer<RETURN_TYPE, __VA_ARGS__> METHOD_NAME##_func_ptr{}; \
    inline static bool METHOD_NAME##_initialized = false; \
    static void METHOD_NAME##_private_init() \
    { \
        if (!METHOD_NAME##_initialized) { \
            METHOD_NAME##_method_ptr = app::getMethod(getClass(), #METHOD_NAME); \
            if (METHOD_NAME##_method_ptr) METHOD_NAME##_func_ptr = METHOD_NAME##_method_ptr->Cast<RETURN_TYPE, __VA_ARGS__>(); \
            METHOD_NAME##_initialized = true; \
        } \
    } \
public: \
    static UnityResolve::MethodPointer<RETURN_TYPE, __VA_ARGS__> METHOD_NAME() { \
        METHOD_NAME##_private_init(); \
        return METHOD_NAME##_func_ptr; \
    } \
    static FunctionHook<RETURN_TYPE(__VA_ARGS__)>& METHOD_NAME##_Hook() { \
        static FunctionHook<RETURN_TYPE(__VA_ARGS__)> hook = []() { \
            METHOD_NAME##_private_init(); \
            if (METHOD_NAME##_method_ptr) { \
                void* raw = *static_cast<void**>(METHOD_NAME##_method_ptr->address); \
                auto funcRaw = reinterpret_cast<RETURN_TYPE(*)(__VA_ARGS__)>(raw); \
                return FunctionHook<RETURN_TYPE(__VA_ARGS__)>(funcRaw); \
            } \
            return FunctionHook<RETURN_TYPE(__VA_ARGS__)>(); \
        }(); \
        return hook; \
    }

// Resolves a method that comes AFTER a known method in the same class
#define UNITY_METHOD_AFTER(RETURN_TYPE, METHOD_NAME, AFTER_NAME, ...) \
private: \
    inline static UnityResolve::Method* METHOD_NAME##_method_ptr = nullptr; \
    inline static UnityResolve::MethodPointer<RETURN_TYPE, __VA_ARGS__> METHOD_NAME##_func_ptr{}; \
    inline static bool METHOD_NAME##_initialized = false; \
    static void METHOD_NAME##_private_init() \
    { \
        if (!METHOD_NAME##_initialized) { \
            METHOD_NAME##_method_ptr = app::getMethod(getClass(), #METHOD_NAME); \
            if (METHOD_NAME##_method_ptr) METHOD_NAME##_func_ptr = METHOD_NAME##_method_ptr->Cast<RETURN_TYPE, __VA_ARGS__>(); \
            METHOD_NAME##_initialized = true; \
        } \
    } \
public: \
    static UnityResolve::MethodPointer<RETURN_TYPE, __VA_ARGS__> METHOD_NAME() { \
        METHOD_NAME##_private_init(); \
        return METHOD_NAME##_func_ptr; \
    } \
    static FunctionHook<RETURN_TYPE(__VA_ARGS__)>& METHOD_NAME##_Hook() { \
        static FunctionHook<RETURN_TYPE(__VA_ARGS__)> hook = []() { \
            METHOD_NAME##_private_init(); \
            if (METHOD_NAME##_method_ptr) { \
                void* raw = *static_cast<void**>(METHOD_NAME##_method_ptr->address); \
                auto funcRaw = reinterpret_cast<RETURN_TYPE(*)(__VA_ARGS__)>(raw); \
                return FunctionHook<RETURN_TYPE(__VA_ARGS__)>(funcRaw); \
            } \
            return FunctionHook<RETURN_TYPE(__VA_ARGS__)>(); \
        }(); \
        return hook; \
    }
    // static FunctionHook& METHOD_NAME##_Hook() \
    // { \
    //     static std::unique_ptr<FunctionHook> hook; \
    //     static bool initialized = false; \
    //     if (!initialized) { \
    //         METHOD_NAME##_private_init(); \
    //         if (METHOD_NAME##_method_ptr) { \
    //             hook = std::make_unique<FunctionHook>(); \
    //             void* raw = *static_cast<void**>(METHOD_NAME##_method_ptr->address); \
    //             const auto funcRaw = reinterpret_cast<RETURN_TYPE(*)(__VA_ARGS__)>(raw); \
    //             hook->target(funcRaw); \
    //         } \
    //         initialized = true; \
    //     } \
    //     return *hook; \
    // }

// Resolves a method that comes BEFORE a known method in the same class
#define UNITY_METHOD_BEFORE(RETURN_TYPE, METHOD_NAME, BEFORE_NAME, ...) \
private: \
    inline static UnityResolve::Method* METHOD_NAME##_method_ptr = nullptr; \
    inline static UnityResolve::MethodPointer<RETURN_TYPE, __VA_ARGS__> METHOD_NAME##_func_ptr{}; \
    inline static bool METHOD_NAME##_initialized = false; \
    static void METHOD_NAME##_private_init() \
    { \
        if (!METHOD_NAME##_initialized) { \
            METHOD_NAME##_method_ptr = app::getMethod(getClass(), #METHOD_NAME); \
            if (METHOD_NAME##_method_ptr) METHOD_NAME##_func_ptr = METHOD_NAME##_method_ptr->Cast<RETURN_TYPE, __VA_ARGS__>(); \
            METHOD_NAME##_initialized = true; \
        } \
    } \
public: \
    static UnityResolve::MethodPointer<RETURN_TYPE, __VA_ARGS__> METHOD_NAME() { \
        METHOD_NAME##_private_init(); \
        return METHOD_NAME##_func_ptr; \
    } \
    static FunctionHook<RETURN_TYPE(__VA_ARGS__)>& METHOD_NAME##_Hook() { \
        static FunctionHook<RETURN_TYPE(__VA_ARGS__)> hook = []() { \
            METHOD_NAME##_private_init(); \
            if (METHOD_NAME##_method_ptr) { \
                void* raw = *static_cast<void**>(METHOD_NAME##_method_ptr->address); \
                auto funcRaw = reinterpret_cast<RETURN_TYPE(*)(__VA_ARGS__)>(raw); \
                return FunctionHook<RETURN_TYPE(__VA_ARGS__)>(funcRaw); \
            } \
            return FunctionHook<RETURN_TYPE(__VA_ARGS__)>(); \
        }(); \
        return hook; \
    }

// Resolves a method BETWEEN two known method names in the same class
#define UNITY_METHOD_BETWEEN(RETURN_TYPE, METHOD_NAME, AFTER_NAME, BEFORE_NAME, ...) \
private: \
    inline static UnityResolve::Method* METHOD_NAME##_method_ptr = nullptr; \
    inline static UnityResolve::MethodPointer<RETURN_TYPE, __VA_ARGS__> METHOD_NAME##_func_ptr{}; \
    inline static bool METHOD_NAME##_initialized = false; \
    static void METHOD_NAME##_private_init() \
    { \
        if (!METHOD_NAME##_initialized) { \
            METHOD_NAME##_method_ptr = app::getMethod(getClass(), #METHOD_NAME); \
            if (METHOD_NAME##_method_ptr) METHOD_NAME##_func_ptr = METHOD_NAME##_method_ptr->Cast<RETURN_TYPE, __VA_ARGS__>(); \
            METHOD_NAME##_initialized = true; \
        } \
    } \
public: \
    static UnityResolve::MethodPointer<RETURN_TYPE, __VA_ARGS__> METHOD_NAME() { \
        METHOD_NAME##_private_init(); \
        return METHOD_NAME##_func_ptr; \
    } \
    static FunctionHook<RETURN_TYPE(__VA_ARGS__)>& METHOD_NAME##_Hook() { \
        static FunctionHook<RETURN_TYPE(__VA_ARGS__)> hook = []() { \
            METHOD_NAME##_private_init(); \
            if (METHOD_NAME##_method_ptr) { \
                void* raw = *static_cast<void**>(METHOD_NAME##_method_ptr->address); \
                auto funcRaw = reinterpret_cast<RETURN_TYPE(*)(__VA_ARGS__)>(raw); \
                return FunctionHook<RETURN_TYPE(__VA_ARGS__)>(funcRaw); \
            } \
            return FunctionHook<RETURN_TYPE(__VA_ARGS__)>(); \
        }(); \
        return hook; \
    }

// Alternatively, you can call METHOD()(arguments) directly, but it doesn't look as nice
#define UNITY_CALL(METHOD_ACCESSOR, ...) METHOD_ACCESSOR()(__VA_ARGS__);

#pragma endregion

#pragma region Field Helpers

// Used to declare a field in a Unity class with its field offset
#define UNITY_FIELD(FIELD_TYPE, FIELD_NAME, FIELD_OFFSET) \
    inline FIELD_TYPE FIELD_NAME() { \
        return *reinterpret_cast<FIELD_TYPE*>(reinterpret_cast<uintptr_t>(this) + FIELD_OFFSET); \
	} \
	inline void FIELD_NAME(FIELD_TYPE value) const { \
	    *reinterpret_cast<FIELD_TYPE*>(reinterpret_cast<uintptr_t>(this) + FIELD_OFFSET) = value; \
	}

// Used to declare a field in a Unity class with its field offset, but resolves the offset dynamically
#define UNITY_FIELD_FROM(FIELD_TYPE, FRIENDLY_NAME, OBFUSCATED_NAME) \
    inline FIELD_TYPE FRIENDLY_NAME() { \
        static int offset = -1; \
        if (offset == -1) { \
            auto klass = getClass(); \
            if (!klass) { \
                LOG_ERROR("Cannot resolve field '" OBFUSCATED_NAME "' – getClass() must be defined"); \
            } else { \
                offset = app::getFieldOffset(klass, OBFUSCATED_NAME); \
                if (offset == -1) { \
                    LOG_ERROR("Field '" OBFUSCATED_NAME "' not found in class '{}'", klass->name.c_str()); \
                } \
            } \
        } \
        return *reinterpret_cast<FIELD_TYPE*>(reinterpret_cast<uintptr_t>(this) + offset); \
    } \
    inline void FRIENDLY_NAME(FIELD_TYPE value) const { \
        static int offset = -1; \
        if (offset == -1) { \
            auto klass = getClass(); \
            if (!klass) { \
                LOG_ERROR("Cannot resolve field '" OBFUSCATED_NAME "' – getClass() must be defined"); \
            } else { \
                offset = app::getFieldOffset(klass, OBFUSCATED_NAME); \
                if (offset == -1) { \
                    LOG_ERROR("Field '" OBFUSCATED_NAME "' not found in class '{}'", klass->name.c_str()); \
                } \
            } \
        } \
        *reinterpret_cast<FIELD_TYPE*>(reinterpret_cast<uintptr_t>(this) + offset) = value; \
    }

#pragma endregion
