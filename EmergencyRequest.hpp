#pragma once
#include <atomic>
#include <chrono>
#include <cstdint>
#include <string>
#include <utility>

struct IncidentLocation {
    std::string locationId;
    std::string address;
    double latitude = 0.0;
    double longitude = 0.0;
};

enum class EmergencyStatus { PENDING, ASSIGNED, ACCEPTED, EN_ROUTE, COMPLETED, CANCELLED };
enum class EmergencyPriority { LOW = 1, NORMAL = 2, HIGH = 3, CRITICAL = 4 };
enum class EmergencyServiceType { AMBULANCE, AIR_AMBULANCE, POLICE, FIRE };
enum class AmbulanceCapability { BLS, ALS };
enum class CapabilityRequirement { ANY, BLS_ONLY, ALS_ONLY };

class EmergencyRequest {
public:
    EmergencyRequest(std::string id, std::string userId, IncidentLocation location,
                     EmergencyServiceType serviceType, EmergencyPriority priority,
                     std::string description = "", std::string patientName = "",
                     CapabilityRequirement capabilityRequirement = CapabilityRequirement::ANY)
        : id_(std::move(id)), userId_(std::move(userId)), location_(std::move(location)),
          serviceType_(serviceType), priority_(priority), description_(std::move(description)),
          patientName_(std::move(patientName)), capabilityRequirement_(capabilityRequirement) {}

    
    const std::string& getId() const { return id_; }
    const std::string& getUserId() const { return userId_; }
    const IncidentLocation& getLocation() const { return location_; }
    EmergencyServiceType getServiceType() const { return serviceType_; }
    EmergencyPriority getPriority() const { return priority_; }
    EmergencyStatus getStatus() const { return status_; }
    const std::string& getDescription() const { return description_; }
    const std::string& getPatientName() const { return patientName_; }
    CapabilityRequirement getCapabilityRequirement() const { return capabilityRequirement_; }

   
    void setStatus(EmergencyStatus status) { status_ = status; }


    void setPriority(EmergencyPriority priority) { priority_ = priority; }


    std::uint64_t getCreationSequence() const { return creationSequence_; }
    std::chrono::system_clock::time_point getCreatedAt() const { return createdAt_; }

private:
    
    static std::uint64_t nextCreationSequence() {
        static std::atomic<std::uint64_t> counter{0};
        return ++counter;
    }

    std::string id_, userId_;
    IncidentLocation location_;
    EmergencyServiceType serviceType_;
    EmergencyPriority priority_;
    std::string description_;
    std::string patientName_;
    CapabilityRequirement capabilityRequirement_ = CapabilityRequirement::ANY;
    EmergencyStatus status_ = EmergencyStatus::PENDING;
    std::uint64_t creationSequence_ = nextCreationSequence();
    std::chrono::system_clock::time_point createdAt_ = std::chrono::system_clock::now();
};

struct EmergencyRequestDispatchOrder {
    bool operator()(const EmergencyRequest& a, const EmergencyRequest& b) const {
        if (a.getPriority() != b.getPriority()) {
            return static_cast<int>(a.getPriority()) < static_cast<int>(b.getPriority());
        }
        return a.getCreationSequence() > b.getCreationSequence();
    }
};
