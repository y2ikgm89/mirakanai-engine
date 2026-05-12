// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/core/entity.hpp"

#include <cstddef>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <utility>
#include <vector>

namespace mirakana {

class Registry {
  public:
    Registry() = default;
    Registry(const Registry&) = delete;
    Registry& operator=(const Registry&) = delete;
    Registry(Registry&&) noexcept = default;
    Registry& operator=(Registry&&) noexcept = default;
    ~Registry() = default;

    [[nodiscard]] Entity create();
    void destroy(Entity entity);
    [[nodiscard]] bool alive(Entity entity) const;
    [[nodiscard]] std::size_t living_count() const noexcept;

    template <class T, class... Args> T& emplace(Entity entity, Args&&... args) {
        static_assert(std::is_object_v<T>, "components must be object types");
        ensure_alive(entity);
        auto& storage = component_storage<T>();
        auto [it, inserted] = storage.values.emplace(entity, T{std::forward<Args>(args)...});
        if (!inserted) {
            throw std::logic_error("component already exists on entity");
        }
        return it->second;
    }

    template <class T> [[nodiscard]] T* try_get(Entity entity) {
        if (!alive(entity)) {
            return nullptr;
        }
        auto* storage = find_storage<T>();
        if (storage == nullptr) {
            return nullptr;
        }
        auto it = storage->values.find(entity);
        return it == storage->values.end() ? nullptr : &it->second;
    }

    template <class T> [[nodiscard]] const T* try_get(Entity entity) const {
        if (!alive(entity)) {
            return nullptr;
        }
        const auto* storage = find_storage<T>();
        if (storage == nullptr) {
            return nullptr;
        }
        auto it = storage->values.find(entity);
        return it == storage->values.end() ? nullptr : &it->second;
    }

    template <class T> bool remove(Entity entity) {
        auto* storage = find_storage<T>();
        if (storage == nullptr) {
            return false;
        }
        return storage->values.erase(entity) > 0;
    }

  private:
    struct Slot {
        std::uint32_t generation{1};
        bool alive{false};
    };

    struct IComponentStorage {
        virtual ~IComponentStorage() = default;
        virtual void erase(Entity entity) = 0;
    };

    template <class T> struct ComponentStorage final : IComponentStorage {
        std::unordered_map<Entity, T, EntityHash> values;

        void erase(Entity entity) override {
            values.erase(entity);
        }
    };

    template <class T> ComponentStorage<T>& component_storage() {
        const std::type_index type = std::type_index(typeid(T));
        auto it = component_storages_.find(type);
        if (it == component_storages_.end()) {
            auto storage = std::make_unique<ComponentStorage<T>>();
            auto* raw = storage.get();
            component_storages_.emplace(type, std::move(storage));
            return *raw;
        }
        return *static_cast<ComponentStorage<T>*>(it->second.get());
    }

    template <class T> [[nodiscard]] ComponentStorage<T>* find_storage() {
        const std::type_index type = std::type_index(typeid(T));
        auto it = component_storages_.find(type);
        if (it == component_storages_.end()) {
            return nullptr;
        }
        return static_cast<ComponentStorage<T>*>(it->second.get());
    }

    template <class T> [[nodiscard]] const ComponentStorage<T>* find_storage() const {
        const std::type_index type = std::type_index(typeid(T));
        auto it = component_storages_.find(type);
        if (it == component_storages_.end()) {
            return nullptr;
        }
        return static_cast<const ComponentStorage<T>*>(it->second.get());
    }

    void ensure_alive(Entity entity) const;

    std::vector<Slot> slots_;
    std::vector<std::uint32_t> free_indices_;
    std::size_t living_count_{0};
    std::unordered_map<std::type_index, std::unique_ptr<IComponentStorage>> component_storages_;
};

} // namespace mirakana
