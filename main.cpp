#include "service/EmergencyManager.hpp"
#include "service/ProjectDatabase.hpp"
#include "service/DispatchRules.hpp"
#include <iostream>
#include <iomanip>
#include <limits>
#include <string>
#include <memory>

namespace {
std::string readLine(const std::string& prompt) {
    std::cout << prompt;
    std::string value;
    std::getline(std::cin, value);
    return value;
}
bool readChoice(int& choice) {
    std::string input;
    if (!std::getline(std::cin, input)) return false;
    try {
        std::size_t used = 0;
        int parsed = std::stoi(input, &used);
        if (used != input.size()) return false;
        choice = parsed;
        return true;
    } catch (...) { return false; }
}
void showLocations() {
    const auto& locations = ProjectDatabase::locations();
    std::cout << "\nPredefined incident locations (SIMULATED project data):\n";
    for (std::size_t i = 0; i < locations.size(); ++i)
        std::cout << "  " << (i + 1) << ". " << locations[i].name << '\n';
}
void showAmbulances(const EmergencyManager& manager) {
    std::cout << "\nPredefined fleet (SIMULATED project data):\n";
    for (const auto& service : manager.getEmergencyServices()) {
        if (service->getType() != EmergencyServiceType::AMBULANCE) continue;
        const auto* ambulance = dynamic_cast<const AmbulanceService*>(service.get());
        std::cout << "  " << service->getId() << " | " << service->getName()
                  << " | " << (ambulance ? toString(ambulance->getCapability()) : "Unknown")
                  << " | " << (service->isAvailable() ? "Available" : "Dispatched");
        if (ambulance && ambulance->hasLocation())
            std::cout << " | base: " << ambulance->getLocation().address;
        std::cout << '\n';
    }
}
void registerEmergency(EmergencyManager& manager, unsigned long long& requestCounter) {
    std::cout << "\n--- Register emergency ---\n";
    const std::string patientName = readLine("Patient/requester name: ");
    if (patientName.find_first_not_of(" \t\r\n") == std::string::npos) {
        std::cout << "Name cannot be empty. Request not registered.\n";
        return;
    }

    showLocations();
    std::cout << "Select location number: ";
    int locChoice = 0;
    if (!readChoice(locChoice) || locChoice < 1 ||
        static_cast<std::size_t>(locChoice) > ProjectDatabase::locations().size()) {
        std::cout << "Invalid location selection. Request not registered.\n";
        return;
    }
    const auto& loc = ProjectDatabase::locations()[static_cast<std::size_t>(locChoice - 1)];

    std::cout << "\nSimulated symptom categories (not medical advice):\n";
    const auto& rules = ProjectDatabase::symptomRules();
    for (std::size_t i = 0; i < rules.size(); ++i)
        std::cout << "  " << (i + 1) << ". " << rules[i].label << '\n';
    std::cout << "  " << (rules.size() + 1) << ". Enter symptom description\n";
    std::cout << "Choose category: ";
    int symptomChoice = 0;
    if (!readChoice(symptomChoice) || symptomChoice < 1 ||
        static_cast<std::size_t>(symptomChoice) > rules.size() + 1) {
        std::cout << "Invalid symptom selection. Request not registered.\n";
        return;
    }

    std::string symptoms;
    int level = 0;
    if (static_cast<std::size_t>(symptomChoice) <= rules.size()) {
        const auto& selected = rules[static_cast<std::size_t>(symptomChoice - 1)];
        symptoms = selected.label;
        level = selected.level;
    } else {
        symptoms = readLine("Describe symptoms using supported keywords: ");
        if (symptoms.find_first_not_of(" \t\r\n") == std::string::npos) {
            std::cout << "Symptom description cannot be empty. Request not registered.\n";
            return;
        }
        level = ProjectDatabase::classifyLevel(symptoms);
        if (level == 0) {
            std::cout << "Symptoms were not matched to a supported rule. No request was registered; "
                         "please seek professional emergency guidance if this is a real emergency.\n";
            return;
        }
    }

    const auto priority = ProjectDatabase::priorityForLevel(level);
    const auto capability = ProjectDatabase::capabilityForLevel(level);
    const std::string requestId = "ER-" + std::to_string(++requestCounter);
    const std::string requesterId = "USER-" + std::to_string(requestCounter);
    EmergencyRequest request(requestId, requesterId, ProjectDatabase::incidentLocation(loc),
        EmergencyServiceType::AMBULANCE, priority, symptoms, patientName, capability);

    if (!manager.createEmergencyRequest(request)) {
        std::cout << "Could not register the request due to validation or duplicate ID.\n";
        return;
    }

    EmergencyRequest dispatched("placeholder", "placeholder", IncidentLocation{},
        EmergencyServiceType::AMBULANCE, EmergencyPriority::LOW);
    std::shared_ptr<const EmergencyService> service;
    std::cout << "\nRequest registered: " << requestId << '\n'
              << "Patient/requester: " << patientName << '\n'
              << "Location: " << loc.name << '\n'
              << "Symptoms: " << symptoms << '\n'
              << "Detected severity: " << ProjectDatabase::levelLabel(level) << '\n'
              << "Required capability: " << (capability == CapabilityRequirement::ALS_ONLY ? "ALS" : "BLS") << '\n';

    if (!manager.dispatchNextEmergencyRequest(dispatched, service)) {
        std::cout << "Dispatch status: PENDING - no compatible available ambulance at this time.\n";
        return;
    }

    if (dispatched.getId() != requestId) {
        std::cout << "Dispatch status for " << requestId
                  << ": PENDING - an earlier queued request (" << dispatched.getId()
                  << ") was dispatched first.\\n";
        return;
    }

    const auto* ambulance = dynamic_cast<const AmbulanceService*>(service.get());
    std::cout << std::fixed << std::setprecision(2)
              << "Assigned ambulance ID: " << service->getId() << '\n'
              << "Assigned ambulance: " << service->getName() << '\n'
              << "Ambulance capability: " << (ambulance ? toString(ambulance->getCapability()) : "Unknown") << '\n';
    if (ambulance)
        std::cout << "Estimated straight-line distance: "
                  << ambulance->distanceKmTo(ProjectDatabase::incidentLocation(loc))
                  << " km\n";
    std::cout << "Dispatch status: ASSIGNED\n";
}
void showHistory(const EmergencyManager& manager) {
    const auto requests = manager.getAllRequests();
    std::cout << "\n--- Emergency history ---\n";
    if (requests.empty()) { std::cout << "No emergency requests registered yet.\n"; return; }
    for (const auto& request : requests) {
        std::cout << request.getId() << " | " << request.getPatientName()
                  << " | " << request.getLocation().address
                  << " | priority " << static_cast<int>(request.getPriority())
                  << " | " << request.getDescription()
                  << " | status " << static_cast<int>(request.getStatus());
        const auto assigned = manager.getAssignedServiceId(request.getId());
        if (!assigned.empty()) std::cout << " | ambulance " << assigned;
        std::cout << '\n';
    }
}
}

int main() {
    EmergencyManager manager;
    ProjectDatabase::loadFleet(manager);
    unsigned long long requestCounter = 0;

    std::cout 
              << " Priority-Based Ambulance Dispatch System\n"
              << " Academic prototype - simulated fleet and locations\n"
              << " Symptom rules are NOT a clinically validated triage tool\n"
              << " if u cant register please call 112 our personnel will help you\n";
    while (true) {
        std::cout << "\n1. Register emergency\n2. View emergency history\n"
                     "3. View ambulance information\n4. Exit\nChoose an option: ";
        int choice = 0;
        if (!readChoice(choice)) {
            if (std::cin.eof()) break;
            std::cout << "Invalid menu option. Enter 1-4.\n";
            continue;
        }
        switch (choice) {
            case 1: registerEmergency(manager, requestCounter); break;
            case 2: showHistory(manager); break;
            case 3: showAmbulances(manager); break;
            case 4: std::cout << "Exiting. Stay safe.\n"; return 0;
            default: std::cout << "Invalid menu option. Enter 1-4.\n";
        }
    }
    return 0;
}
