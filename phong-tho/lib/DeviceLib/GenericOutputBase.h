#ifndef GENERIC_OUTPUT_BASE_H
#define GENERIC_OUTPUT_BASE_H

#include <Arduino.h>

#if __has_include(<PCF8574.h>)
#include <PCF8574.h>
#ifndef USE_PCF8574
#define USE_PCF8574
#endif // USE_PCF8574
#endif // __has_include_next(<PCF8574.h>)

#if __has_include(<espnow-node.h>)
#ifndef USE_ESPNOW_NODE
#define USE_ESPNOW_NODE
#include <espnow-node.h>
#endif // USE_ESPNOW_NODE
#endif // __has_include_next(<espnow-node.h>)

namespace stdGenericOutput {
    class GenericOutputBase;
}


class stdGenericOutput::GenericOutputBase {

public:

    GenericOutputBase() = default;

    /**
     * @brief Construct a new GenericOutputBase object
     * 
     * @param pin pin number
     * @param activeState LOW or HIGH. Default is LOW
     */
    explicit GenericOutputBase(uint8_t pin, bool activeState = LOW);

#if defined(USE_PCF8574)
    /**
     * @brief Construct a new GenericOutputBase object
     *
     * @param pcf8574 PCF8574 object
     * @param pin pin number
     * @param activeState LOW or HIGH. Default is LOW
     */
    explicit GenericOutputBase(PCF8574& pcf8574, uint8_t pin, bool activeState = LOW);
#endif

    /**
     * @brief Destroy the GenericOutputBase object
     */
    ~GenericOutputBase();

    /**
     * @brief Set powe to ON
     * 
     */
    virtual void on();

    /**
     * @brief Set power to OFF
     * 
     */
    virtual void off();

    /**
     * @brief Toggle power
     * 
     */
    void toggle();

    /**
     * @brief Set the power state
     * @param state
     */
    void setState(bool state);

    /**
     * @brief Set the power state from string "ON" or "OFF"
     * 
     * @param state 
     */
    void setState(const String& state);

    /**
     * @brief Set the Active State object
     * 
     * @param activeState 
     */
    void setActiveState(bool activeState);

    /**
     * @brief Get the Active State object
     * 
     * @return true 
     * @return false 
     */
    bool getActiveState() const;

    /**
     * @brief Get current state of the device
     * 
     * @return true when ON
     * @return false when OFF
     */
    bool getState() const;

    /**
     * @brief Get current state of the device as string "ON" or "OFF"
     *
     * @return String
     
     */
    String getStateString() const;

    /**
     * @brief Set callback function to be called when power is on
     *
     * @param onPowerOn
     */
    void onPowerOn(std::function<void()> onPowerOn) {
        _onPowerOn = std::move(onPowerOn);
    }

    /**
     * @brief Set callback function to be called when power is off
     *
     * @param onPowerOff
     */
    void onPowerOff(std::function<void()> onPowerOff) {
        _onPowerOff = std::move(onPowerOff);
    }

    /**
     * @brief Set callback function to be called when power is changed
     *
     * @param onPowerChanged
     */
    void onPowerChanged(std::function<void()> onPowerChanged) {
        _onPowerChanged = std::move(onPowerChanged);
    }

#ifdef USE_ESPNOW_NODE
    String getStateBoolString() const {
        return _state ? "true" : "false";
    }

    void attachESPNOW(const String& propName, ENNodeInfo *nodeInfo) {
        _propName = propName;
        _nodeInfo = nodeInfo;
        nodeInfo->addProp(propName, getStateBoolString(),
            [this](){
                return getStateBoolString();
            },
            [this](const String& value){
                if (value == "true") {
                    on();
                } else if (value == "false") {
                    off();
                } else if (value == "toggle") {
                    toggle();
                    schedule_function([this](){
                        Node.sendSyncProp(_propName, getStateBoolString());
                    });
                } else {
                    return false;
                }
                return true;
            });
    }
#endif

protected:
    uint8_t _pin;
    bool _activeState;
    bool _state;
    std::function<void()> _onPowerOn = nullptr;
    std::function<void()> _onPowerOff = nullptr;
    std::function<void()> _onPowerChanged = nullptr;

#if defined(USE_PCF8574)
    PCF8574* _pcf8574 = nullptr;
#endif

#ifdef USE_ESPNOW_NODE
    String _propName;
    ENNodeInfo* _nodeInfo = nullptr;
    // ENNode* _node = nullptr;
#endif

    /**
     * @brief digitalWrite wrapper
     */
    void _write();
};

#endif // GENERIC_OUTPUT_BASE_H