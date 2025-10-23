#pragma once

#define DECL_SINGLETON(CLASS_NAME)                          \
	public: \
		static CLASS_NAME* getInstance() { \
			if (!s_instance) \
				s_instance = new CLASS_NAME(); \
			return s_instance; \
		} \
		static bool isInitialized() { return s_instance != nullptr; } \
		CLASS_NAME(const CLASS_NAME&) = delete; \
		CLASS_NAME(CLASS_NAME&&) = delete; \
		CLASS_NAME& operator=(const CLASS_NAME&) = delete; \
		CLASS_NAME& operator=(CLASS_NAME&&) = delete; \
	protected: \
        struct SingletonSetter { \
            SingletonSetter(CLASS_NAME* ptr) { \
                if (!s_instance) s_instance = ptr; \
            } \
        }; \
        SingletonSetter m_singletonSetter{static_cast<CLASS_NAME*>(this)}; \
	private: \
		inline static CLASS_NAME* s_instance = nullptr;
