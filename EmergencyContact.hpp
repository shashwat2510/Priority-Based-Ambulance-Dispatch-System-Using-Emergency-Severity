#pragma once
#include <string>
#include <utility>

class EmergencyContact {
public:
    EmergencyContact(std::string id, std::string userId, std::string name,
                     std::string phone, std::string relationship)
        : id_(std::move(id)), userId_(std::move(userId)), name_(std::move(name)),
          phone_(std::move(phone)), relationship_(std::move(relationship)) {}
    const std::string& getId() const { return id_; }
    const std::string& getUserId() const { return userId_; }
    const std::string& getName() const { return name_; }
    const std::string& getPhone() const { return phone_; }
    const std::string& getRelationship() const { return relationship_; }
private:
    std::string id_, userId_, name_, phone_, relationship_;
};
