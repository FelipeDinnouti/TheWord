#ifndef EVENT_BUS_H
#define EVENT_BUS_H

#include <any>
#include <functional>
#include <memory>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace theword::event {

class EventBus {
public:
    template<typename T>
    using Slot = std::function<void(const T&)>;

    template<typename T>
    void On(Slot<T> slot) {
        auto& entries = slots_[std::type_index(typeid(T))];
        entries.push_back(std::make_shared<std::function<void(const std::any&)>>(
            [s = std::move(slot)](const std::any& e) {
                s(std::any_cast<const T&>(e));
            }
        ));
    }

    template<typename T>
    void Emit(const T& event) {
        auto it = slots_.find(std::type_index(typeid(T)));
        if (it != slots_.end()) {
            for (auto& slot : it->second) {
                (*slot)(std::any(event));
            }
        }
    }

private:
    std::unordered_map<
        std::type_index,
        std::vector<std::shared_ptr<std::function<void(const std::any&)>>>
    > slots_;
};

} // namespace theword::event
#endif
