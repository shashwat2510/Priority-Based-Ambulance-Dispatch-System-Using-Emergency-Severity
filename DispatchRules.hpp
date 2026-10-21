#pragma once
// Small, header-only module holding the basic dispatch rules so that they do not
// live inside EmergencyManager.cpp. It has no state and no I/O.
// Not included here on purpose: hospital recommendation, road routing, database.
#include "../model/EmergencyRequest.hpp"
#include "../model/EmergencyService.hpp"
#include "../model/EmergencyServices.hpp"
#include <cctype>
#include <initializer_list>
#include <limits>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

namespace DispatchRules {

inline std::string toLower(std::string text) {
    for (char& c : text) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return text;
}

inline bool containsAny(const std::string& text, std::initializer_list<const char*> keywords) {
    for (const char* keyword : keywords)
        if (text.find(keyword) != std::string::npos) return true;
    return false;
}

// Rule-based severity classification from free text. Safety-biased: the highest
// matching level wins, and an empty or unrecognised description is NORMAL (not LOW).
// Limitation: plain keyword matching, so negations ("not bleeding") are not understood.
inline EmergencyPriority classifySeverity(const std::string& description) {
    const std::string text = toLower(description);
    if (containsAny(text, {"unconscious", "unresponsive", "not breathing", "no pulse",
                           "cardiac arrest", "heart attack", "severe bleeding",
                           "heavy bleeding", "stroke", "choking", "drowning",
                           "anaphyla"}))
        return EmergencyPriority::CRITICAL;
    if (containsAny(text, {"chest pain", "difficulty breathing", "shortness of breath",
                           "seizure", "head injury", "bleeding", "poison", "overdose",
                           "burn", "labour", "labor", "road accident", "crash",
                           "severe pain", "fracture"}))
        return EmergencyPriority::HIGH;
    if (containsAny(text, {"minor", "mild", "sprain", "bruise", "non-urgent", "checkup"}))
        return EmergencyPriority::LOW;
    return EmergencyPriority::NORMAL;
}

// CRITICAL incidents require an ALS unit; every other level accepts ALS or BLS.
inline bool isCapabilityCompatible(EmergencyPriority priority, AmbulanceCapability capability) {
    return priority != EmergencyPriority::CRITICAL || capability == AmbulanceCapability::ALS;
}

// Tie-break only (after distance): keep ALS units free for incidents that need them.
inline int capabilityRank(EmergencyPriority priority, AmbulanceCapability capability) {
    const bool wantsAls = static_cast<int>(priority) >= static_cast<int>(EmergencyPriority::HIGH);
    return (capability == AmbulanceCapability::ALS) == wantsAls ? 0 : 1;
}

// Chooses a service for the request or returns nullptr. Pure: nothing is modified.
//  - Only available services of the requested type are considered; `busyIds` lets the
//    caller exclude services with an active assignment even if their flag was reset.
//  - Ground ambulances: capability must be compatible; the nearest (straight-line
//    distance) wins, then capability rank, then smallest id for determinism.
//    Ambulances with no known location sort after those with one.
//  - Other service types: smallest id wins (deterministic).
inline std::shared_ptr<EmergencyService>
selectService(const std::vector<std::shared_ptr<EmergencyService>>& services,
              const EmergencyRequest& request,
              const std::unordered_set<std::string>& busyIds) {
    std::shared_ptr<EmergencyService> best;
    int bestRank = 0;
    double bestDistance = std::numeric_limits<double>::infinity();

    for (const auto& service : services) {
        if (!service || !service->isAvailable() ||
            service->getType() != request.getServiceType() ||
            busyIds.count(service->getId()))
            continue;

        int rank = 0;
        double distance = std::numeric_limits<double>::infinity();
        if (request.getServiceType() == EmergencyServiceType::AMBULANCE) {
            AmbulanceCapability capability = AmbulanceCapability::BLS;
            if (const auto* ambulance = dynamic_cast<const AmbulanceService*>(service.get())) {
                capability = ambulance->getCapability();
                distance = ambulance->distanceKmTo(request.getLocation());
            }
            const auto requirement = request.getCapabilityRequirement();
            if (requirement == CapabilityRequirement::BLS_ONLY && capability != AmbulanceCapability::BLS) continue;
            if (requirement == CapabilityRequirement::ALS_ONLY && capability != AmbulanceCapability::ALS) continue;
            if (requirement == CapabilityRequirement::ANY &&
                !isCapabilityCompatible(request.getPriority(), capability)) continue;
            rank = capabilityRank(request.getPriority(), capability);
        }

        const bool better =
            !best || distance < bestDistance ||
            (distance == bestDistance &&
             (rank < bestRank || (rank == bestRank && service->getId() < best->getId())));
        if (better) {
            best = service;
            bestRank = rank;
            bestDistance = distance;
        }
    }
    return best;
}

}  // namespace DispatchRules
