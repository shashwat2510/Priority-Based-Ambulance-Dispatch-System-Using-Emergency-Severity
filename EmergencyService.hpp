#pragma once
#include "EmergencyRequest.hpp"
#include <string>
#include <utility>

class EmergencyService {
public:
    EmergencyService(std::string id, std::string name, std::string phone, bool available)
        : id_(std::move(id)), name_(std::move(name)), phone_(std::move(phone)), available_(available) {}
    virtual ~EmergencyService() = default;
    const std::string& getId() const { return id_; }
    const std::string& getName() const { return name_; }
    const std::string& getPhone() const { return phone_; }
    bool isAvailable() const { return available_; }
    void setAvailable(bool available) { available_ = available; }
    virtual EmergencyServiceType getType() const = 0;
    virtual std::string getDisplayDetails() const = 0;
private:
    std::string id_, name_, phone_;
    bool available_;
};
