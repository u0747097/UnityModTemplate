#pragma once

#include "logger.h"

// TODO: This needs work
#define SAFE_EXECUTE(code) \
    try { \
        code \
    } catch (const std::exception& e) { \
        LOG_ERROR("[EXCEPTION] {}:{} in {} - {}", \
                  __FILE__, __LINE__, __FUNCTION__, e.what()); \
    } catch (...) { \
        LOG_ERROR("[EXCEPTION] {}:{} in {} - Unknown error", \
                  __FILE__, __LINE__, __FUNCTION__); \
    }

#define SAFE_BEGIN \
    try {

#define SAFE_END \
    } catch (const std::exception& e) { \
        LOG_ERROR("[EXCEPTION] {}:{} in {} - {}", \
                  __FILE__, __LINE__, __FUNCTION__, e.what()); \
    } catch (...) { \
        LOG_ERROR("[EXCEPTION] {}:{} in {} - Unknown error", \
                  __FILE__, __LINE__, __FUNCTION__); \
    }