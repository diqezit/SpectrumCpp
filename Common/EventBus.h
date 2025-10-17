// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// EventBus.h: A simple pub/sub system for decoupling components.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
#ifndef SPECTRUM_CPP_EVENT_BUS_H
#define SPECTRUM_CPP_EVENT_BUS_H

#include "Common.h"

namespace Spectrum {

    class EventBus {
    public:
        using EventHandler = std::function<void()>;

        void Subscribe(InputAction action, EventHandler handler) {
            m_subscribers[action].push_back(std::move(handler));
        }

        void Publish(InputAction action) {
            if (m_subscribers.count(action)) {
                for (const auto& handler : m_subscribers[action]) {
                    if (handler) {
                        handler();
                    }
                }
            }
        }

    private:
        std::map<InputAction, std::vector<EventHandler>> m_subscribers;
    };

}

#endif