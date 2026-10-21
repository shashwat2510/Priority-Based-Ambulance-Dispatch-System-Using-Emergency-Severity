#pragma once
#include "../model/EmergencyServices.hpp"
#include "../model/EmergencyRequest.hpp"
#include <string>
#include <vector>
#include <memory>
#include <algorithm>
#include <cctype>

struct ProjectLocation {
    std::string id;
    std::string name;
    double latitude;
    double longitude;
};

struct SymptomRule {
    int level;
    std::string label;
    std::vector<std::string> keywords;
};

// All coordinates and units below are SIMULATED academic-demo data, not live fleet data.
class ProjectDatabase {
public:
    static const std::vector<ProjectLocation>& locations() {
        static const std::vector<ProjectLocation> data = {
            {"LOC1", "Clock Tower area (simulated)", 30.3256, 78.0413},
            {"LOC2", "ISBT area (simulated)", 30.2850, 78.0020},
            {"LOC3", "Prem Nagar area (simulated)", 30.3540, 77.9650},
            {"LOC4", "Rajpur Road area (simulated)", 30.3700, 78.0800},
            {"LOC5", "Dalanwala area (simulated)", 30.3330, 78.0600}
        };
        return data;
    }

    static const std::vector<SymptomRule>& symptomRules() {
        static const std::vector<SymptomRule> data = {
            {3, "Unconscious / not breathing / severe bleeding", {"unconscious", "unresponsive", "not breathing", "no pulse", "severe bleeding", "heavy bleeding", "cardiac arrest"}},
            {3, "Severe breathing difficulty / suspected stroke", {"difficulty breathing", "can't breathe", "cannot breathe", "stroke", "choking"}},
            {2, "Chest pain / accident / fracture / seizure", {"chest pain", "road accident", "crash", "fracture", "seizure", "head injury", "overdose", "poison"}},
            {1, "Minor sprain / bruise / mild symptoms", {"minor", "mild", "sprain", "bruise", "checkup"}}
        };
        return data;
    }

    static int classifyLevel(const std::string& input) {
        std::string text = input;
        std::transform(text.begin(), text.end(), text.begin(),
            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        for (const auto& rule : symptomRules())
            for (const auto& keyword : rule.keywords)
                if (text.find(keyword) != std::string::npos) return rule.level;
        return 0; // unknown input is rejected by the UI; never silently mark low severity
    }

    static std::string levelLabel(int level) {
        if (level == 1) return "Level 1";
        if (level == 2) return "Level 2";
        if (level == 3) return "Level 3";
        return "Unknown";
    }

    static EmergencyPriority priorityForLevel(int level) {
        if (level == 3) return EmergencyPriority::CRITICAL;
        if (level == 2) return EmergencyPriority::HIGH;
        return EmergencyPriority::LOW;
    }

    static CapabilityRequirement capabilityForLevel(int level) {
        return level == 3 ? CapabilityRequirement::ALS_ONLY : CapabilityRequirement::BLS_ONLY;
    }

    static IncidentLocation incidentLocation(const ProjectLocation& loc) {
        return {loc.id, loc.name, loc.latitude, loc.longitude};
    }

    static void loadFleet(EmergencyManager& manager) {
        manager.addEmergencyService(std::make_shared<AmbulanceService>(
            "AMB-BLS-01", "BLS Ambulance 01", "112", true,
            IncidentLocation{"BASE1", "Simulated base near Clock Tower", 30.3260, 78.0430}, AmbulanceCapability::BLS));
        manager.addEmergencyService(std::make_shared<AmbulanceService>(
            "AMB-BLS-02", "BLS Ambulance 02", "112", true,
            IncidentLocation{"BASE2", "Simulated base near ISBT", 30.2860, 78.0040}, AmbulanceCapability::BLS));
        manager.addEmergencyService(std::make_shared<AmbulanceService>(
            "AMB-ALS-01", "ALS Ambulance 01", "112", true,
            IncidentLocation{"BASE3", "Simulated base near Rajpur Road", 30.3680, 78.0780}, AmbulanceCapability::ALS));
        manager.addEmergencyService(std::make_shared<AmbulanceService>(
            "AMB-ALS-02", "ALS Ambulance 02", "112", true,
            IncidentLocation{"BASE4", "Simulated base near Prem Nagar", 30.3550, 77.9670}, AmbulanceCapability::ALS));
    }
};
