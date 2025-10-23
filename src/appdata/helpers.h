#pragma once

#include <UnityResolve.hpp>

namespace app
{
    /**
     * @brief Retrieves a Unity class by its module and class name.
     * @param module The name of the Unity module (e.g., "Assembly-CSharp.dll", "mscorlib.dll"). 
     * @param className The name of the class within that module (e.g., "PlayerController", "GameManager").
     * @return A pointer to the UnityResolve::Class object if found, or nullptr if not found.
     */
    inline UnityResolve::Class* getClass(const std::string& module, const std::string& className)
    {
        auto unityModule = UnityResolve::Get(module);
        if (!unityModule)
        {
            LOG_ERROR("Module '{}' not found", module.c_str());
            return nullptr;
        }

        auto klass = unityModule->Get(className);
        if (!klass)
        {
            LOG_ERROR("Class '{}' not found in module '{}'", className.c_str(), module.c_str());
            return nullptr;
        }

        return klass;
    }

    /**
     * @brief Resolves an obfuscated class by scanning a known class for a specific field name.
     *
     * This is useful when the target class is obfuscated but is used as a field type in a stable class.
     * For example, class CharacterComponent has a field <Character>k__BackingField
     * Which is of type Character, but the Character class itself is obfuscated. 
     * 
     * @param module The name of the module (e.g., "Assembly-CSharp.dll").
     * @param containerClass The known, non-obfuscated class that contains the field (e.g., "CharacterComponent").
     * @param fieldName The EXACT name of the field referencing the obfuscated class (e.g., "<Character>k__BackingField").
     * @return A pointer to the obfuscated UnityResolve::Class if found, otherwise nullptr.
     */
    inline UnityResolve::Class* findClassFromField(const std::string& module, const std::string& containerClass,
                                                   const std::string_view fieldName)
    {
        for (const auto containerClassPtr = getClass(module, containerClass); const auto* field : containerClassPtr->
             fields)
        {
            if (!field || !field->type || field->name != fieldName) continue;

            const std::string& fullTypeName = field->type->name;

            // fullTypeName example: "MX.Logic.BattleEntities.ObfuscatedClassName"
            return getClass(module,
                            fullTypeName.substr(fullTypeName.find_last_of('.') + 1));
        }

        return nullptr;
    }

    /**
     * @brief Finds a Unity class by matching multiple known field names for fast identification of obfuscated classes.
     * @param module The name of the Unity module (e.g., "Assembly-CSharp.dll").
     * @param requiredFields Vector of field names that must exist in the target class.
     * @param minMatches Minimum number of fields that must match (default: all fields must match).
     * @return A pointer to the obfuscated UnityResolve::Class if found, otherwise nullptr.
     */
    inline UnityResolve::Class* findClassByFields(const std::string& module,
                                                  const std::vector<std::string>& requiredFields,
                                                  size_t minMatches = 0)
    {
        if (requiredFields.empty())
        {
            LOG_ERROR("No field name provided for class search");
            return nullptr;
        }

        if (minMatches == 0) minMatches = requiredFields.size();
        if (minMatches > requiredFields.size()) minMatches = requiredFields.size();

        auto unityModule = UnityResolve::Get(module);
        if (!unityModule)
        {
            LOG_ERROR("Module '{}' not found", module.c_str());
            return nullptr;
        }

        std::vector<UnityResolve::Class*> matchingClasses;

        for (const auto& klass : unityModule->classes)
        {
            if (!klass || klass->fields.empty()) continue;

            size_t matchCount = 0;

            if (klass->fields.size() < minMatches) continue;

            std::unordered_set<std::string> classFieldNames;
            classFieldNames.reserve(klass->fields.size());

            for (const auto& field : klass->fields)
            {
                if (field && !field->name.empty())
                {
                    classFieldNames.insert(field->name);
                }
            }

            for (const auto& requiredField : requiredFields)
            {
                if (classFieldNames.contains(requiredField))
                {
                    matchCount++;
                }
            }

            if (matchCount >= minMatches)
            {
                matchingClasses.push_back(klass);
            }
        }

        if (matchingClasses.empty())
        {
            LOG_ERROR("No class found in module '{}' with required fields", module.c_str());
            return nullptr;
        }

        if (matchingClasses.size() == 1)
        {
            // LOG_DEBUG("Found class '{}' in module '{}' with required fields", matchingClasses[0]->name.c_str(),
            // 		  module.c_str());
            return matchingClasses[0];
        }

        LOG_ERROR("{} classes found in module '{}' with required fields", matchingClasses.size(), module.c_str());
        for (const auto& klass : matchingClasses)
        {
            LOG_ERROR("Class '{}' with fields: ", klass->name.c_str());
        }

        return nullptr;
    }

    /**
     * @brief Finds the method immediately after a given method in a Unity class.
     * @param module The name of the Unity module (e.g., "Assembly-CSharp.dll", "mscorlib.dll").
     * @param className The name of the class within that module (e.g., "PlayerController", "GameManager"). 
     * @param methodName The name of the method (e.g., "Start", "Update").
     * @return A pointer to the UnityResolve::Method object if found, or nullptr if not found.
     */
    inline UnityResolve::Method* getMethod(const std::string& module,
                                           const std::string& className,
                                           const std::string& methodName)
    {
        auto klass = getClass(module, className);
        if (!klass)
        {
            LOG_ERROR("Failed to get class '{}' from module '{}'", className.c_str(), module.c_str());
            return nullptr;
        }

        auto method = klass->Get<UnityResolve::Method>(methodName);
        if (!method)
        {
            LOG_ERROR("Method '{}' not found in class '{}' of module '{}'", methodName.c_str(), className.c_str(),
                      module.c_str());
            return nullptr;
        }

        return method;
    }

    /**
     * @brief Retrieves a method from a Unity class by name.
     * @param klass Pointer to UnityResolve::Class.
     * @param methodName Name of the method to find.
     * @return Pointer to UnityResolve::Method, or nullptr on failure.
     */
    inline UnityResolve::Method* getMethod(UnityResolve::Class* klass, const std::string& methodName)
    {
        if (!klass)
        {
            LOG_ERROR("Unity class is null");
            return nullptr;
        }

        auto method = klass->Get<UnityResolve::Method>(methodName);
        if (!method)
        {
            LOG_ERROR("Method '{}' not found in class '{}'", methodName.c_str(), klass->name.c_str());
            return nullptr;
        }

        return method;
    }

    /**
     * @brief Gets the raw method address from module/class/method strings.
     * @param module Module name.
     * @param className Class name.
     * @param methodName Method name.
     * @return Function pointer address, or nullptr on failure.
     */
    inline void* getMethodAddress(const std::string& module,
                                  const std::string& className,
                                  const std::string& methodName)
    {
        auto method = getMethod(module, className, methodName);
        if (!method)
        {
            LOG_ERROR("Failed to get method '{}' from class '{}' in module '{}'", methodName.c_str(), className.c_str(),
                      module.c_str());
            return nullptr;
        }

        return *static_cast<void**>(method->address);
    }

    /**
     * @brief Gets a raw method address from a Unity class and method name.
     * @param klass Pointer to UnityResolve::Class.
     * @param methodName Name of the method.
     * @return Function pointer address, or nullptr on failure.
     */
    inline void* getMethodAddress(UnityResolve::Class* klass, const std::string& methodName)
    {
        if (!klass)
        {
            LOG_ERROR("Unity class is null");
            return nullptr;
        }

        auto method = getMethod(klass, methodName);
        if (!method)
        {
            LOG_ERROR("Failed to get method '{}' from class '{}'", methodName.c_str(), klass->name.c_str());
            return nullptr;
        }

        return *static_cast<void**>(method->address);
    }

    /**
     * @brief Gets the raw address of an already resolved Unity method.
     * @param method Pointer to UnityResolve::Method.
     * @return Raw function pointer, or nullptr if method is null.
     */
    inline void* getMethodAddress(UnityResolve::Method* method)
    {
        if (!method)
        {
            LOG_ERROR("Unity method is null");
            return nullptr;
        }

        return *static_cast<void**>(method->address);
    }

    /**
     * @brief Finds the method that appears immediately after a specified method.
     * @param klass Pointer to UnityResolve::Class.
     * @param afterMethodName Name of the anchor method.
     * @return Pointer to the next method in order, or nullptr if not found.
     */
    inline UnityResolve::Method* findMethodAfter(const UnityResolve::Class* klass,
                                                 const std::string_view afterMethodName)
    {
        bool found = false;

        for (const auto& method : klass->methods)
        {
            if (!method) continue;

            if (found) return method;

            if (method->name == afterMethodName) found = true;
        }

        return nullptr;
    }

    /**
     * @brief Finds the method that appears immediately before a specified method.
     * @param klass Pointer to UnityResolve::Class.
     * @param beforeMethodName Name of the anchor method.
     * @return Pointer to the method that appears before it, or nullptr if not found.
     */
    inline UnityResolve::Method* findMethodBefore(const UnityResolve::Class* klass,
                                                  const std::string_view beforeMethodName)
    {
        UnityResolve::Method* prev = nullptr;

        for (const auto& method : klass->methods)
        {
            if (!method) continue;

            if (method->name == beforeMethodName) return prev;

            prev = method;
        }

        return nullptr;
    }

    /**
     * @brief Finds a method that appears between two known methods.
     * @param klass Pointer to UnityResolve::Class.
     * @param afterMethodName Name of the method that comes before the target.
     * @param beforeMethodName Optional name of the method that should follow the target.
     * @return The method found between the two, or nullptr on failure.
     */
    inline UnityResolve::Method* findMethodBetween(const UnityResolve::Class* klass,
                                                   const std::string_view afterMethodName,
                                                   const std::optional<std::string_view>& beforeMethodName =
                                                       std::nullopt)
    {
        bool found = false;

        for (const auto& method : klass->methods)
        {
            if (!method) continue;

            std::string_view name = method->name;

            if (found)
            {
                if (beforeMethodName && name == *beforeMethodName) break;

                return method;
            }

            if (name == afterMethodName) found = true;
        }

        return nullptr;
    }

    /**
     * @brief Retrieves a field from a Unity class by its name and returns its Field pointer and offset.
     * @param klass Pointer to UnityResolve::Class.
     * @param fieldName Name of the field to find.
     * @return A pair containing the UnityResolve::Field pointer and its offset.
     */
    inline std::pair<UnityResolve::Field*, int> getFieldFromClass(const UnityResolve::Class* klass,
                                                                  const std::string& fieldName)
    {
        if (!klass)
        {
            LOG_ERROR("Unity class is null");
            return {nullptr, -1};
        }

        for (const auto& field : klass->fields)
        {
            if (field && field->name == fieldName)
            {
                return {field, field->offset};
            }
        }

        LOG_ERROR("Field '{}' not found in class '{}'", fieldName.c_str(), klass->name.c_str());
        return {nullptr, -1};
    }

    /**
     * @brief Gets the offset of a field in a Unity class by its name.
     * @param klass Pointer to UnityResolve::Class.
     * @param fieldName Name of the field to find.
     * @return The offset of the field in decimal, or -1 if not found.
     */
    inline int getFieldOffset(const UnityResolve::Class* klass, const std::string& fieldName)
    {
        if (auto [field, offset] = getFieldFromClass(klass, fieldName); field)
        {
            return offset;
        }

        LOG_ERROR("Failed to get offset for field '{}' in class '{}'", fieldName.c_str(), klass->name.c_str());
        return -1;
    }
}
