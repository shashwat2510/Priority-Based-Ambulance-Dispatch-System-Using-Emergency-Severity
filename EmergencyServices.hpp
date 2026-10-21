#pragma once
#include "EmergencyService.hpp"
#include <cmath>
#include <limits>
#include <string>
#include <utility>

inline std::string toString(AmbulanceCapability capability) {
    return capability == AmbulanceCapability::ALS ? "ALS" : "BLS";
}

class AmbulanceService : public EmergencyService {
public:
    using EmergencyService::EmergencyService;

    AmbulanceService(std::string id, std::string name, std::string phone, bool available,
                     IncidentLocation location,
                     AmbulanceCapability capability = AmbulanceCapability::BLS)
        : EmergencyService(std::move(id), std::move(name), std::move(phone), available),
          location_(std::move(location)), hasLocation_(true), capability_(capability) {}

    EmergencyServiceType getType() const override { return EmergencyServiceType::AMBULANCE; }
    std::string getDisplayDetails() const override { return getName() + " (ground ambulance)"; }

    bool hasLocation() const { return hasLocation_; }
    const IncidentLocation& getLocation() const { return location_; }
    void setLocation(IncidentLocation location) {
        location_ = std::move(location);
        hasLocation_ = true;
    }

    AmbulanceCapability getCapability() const { return capability_; }
    void setCapability(AmbulanceCapability capability) { capability_ = capability; }
    bool isAlsCapable() const { return capability_ == AmbulanceCapability::ALS; }

    double distanceKmTo(const IncidentLocation& target) const {
        if (!hasLocation_) {
            return std::numeric_limits<double>::infinity();
        }
        const double pi = 3.14159265358979323846;
        const double earthRadiusKm = 6371.0;
        const double toRad = pi / 180.0;
        const double dLat = (target.latitude - location_.latitude) * toRad;
        const double dLon = (target.longitude - location_.longitude) * toRad;
        const double a = std::sin(dLat / 2) * std::sin(dLat / 2) +
                         std::cos(location_.latitude * toRad) * std::cos(target.latitude * toRad) *
                             std::sin(dLon / 2) * std::sin(dLon / 2);
        return earthRadiusKm * 2.0 * std::atan2(std::sqrt(a), std::sqrt(1.0 - a));
    }

private:
    IncidentLocation location_;
    bool hasLocation_ = false;
    AmbulanceCapability capability_ = AmbulanceCapability::BLS;
};

class AirAmbulanceService : public EmergencyService {
public:
    using EmergencyService::EmergencyService;
    EmergencyServiceType getType() const override { return EmergencyServiceType::AIR_AMBULANCE; }
    std::string getDisplayDetails() const override { return getName() + " (air ambulance)"; }
};
class PoliceService : public EmergencyService {
public:
    using EmergencyService::EmergencyService;
    EmergencyServiceType getType() const override { return EmergencyServiceType::POLICE; }
    std::string getDisplayDetails() const override { return getName() + " (police)"; }
};
class FireService : public EmergencyService {
public:
    using EmergencyService::EmergencyService;
    EmergencyServiceType getType() const override { return EmergencyServiceType::FIRE; }
    std::string getDisplayDetails() const override { return getName() + " (fire service)"; }
};
