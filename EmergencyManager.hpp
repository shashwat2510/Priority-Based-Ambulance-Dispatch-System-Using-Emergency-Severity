#pragma once
#include "../model/EmergencyContact.hpp"
#include "../model/EmergencyRequest.hpp"
#include "../model/EmergencyService.hpp"
#include <memory>
#include <cstddef>
#include <queue>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// Ordering for std::priority_queue (top() = next request to dispatch):
//   1. higher EmergencyPriority first (CRITICAL > HIGH > NORMAL > LOW),
//   2. then earlier creation first (getCreationSequence(), FIFO within a level),
//   3. then smaller id as a final deterministic fallback.
// Requires the revised EmergencyRequest.hpp (getCreationSequence()).
struct HigherEmergencyPriority {
    bool operator()(const EmergencyRequest& a, const EmergencyRequest& b) const {
        if (a.getPriority() != b.getPriority())
            return static_cast<int>(a.getPriority()) < static_cast<int>(b.getPriority());
        if (a.getCreationSequence() != b.getCreationSequence())
            return a.getCreationSequence() > b.getCreationSequence();
        return a.getId() > b.getId();
    }
};

class EmergencyManager {
public:
    bool createEmergencyRequest(const EmergencyRequest& request);
    const EmergencyRequest* findRequestById(const std::string& id) const;
    std::size_t prioritizeEmergencyRequests();
    bool updateRequestStatus(const std::string& id, EmergencyStatus status);
    bool processNextEmergencyRequest(EmergencyRequest& out);
    std::vector<EmergencyRequest> getRequestHistory(const std::string& userId) const;
    std::vector<EmergencyRequest> getAllRequests() const;
    bool addEmergencyContact(const EmergencyContact& contact);
    std::vector<EmergencyContact> getEmergencyContacts(const std::string& userId) const;
    void addEmergencyService(std::shared_ptr<EmergencyService> service);
    std::vector<std::shared_ptr<const EmergencyService>> findAvailableServices(EmergencyServiceType type) const;
    std::vector<std::shared_ptr<const EmergencyService>> getEmergencyServices() const;

    // NEW (basic severity classification): maps an incident description to an
    // existing EmergencyPriority. Pure function; does not touch manager state.
    static EmergencyPriority classifySeverity(const std::string& description);

    // NEW (basic ambulance selection): returns the best available ground
    // ambulance for the request, or nullptr if none is available. Uses only the
    // existing service list; no hospital or road-routing logic.
    std::shared_ptr<const EmergencyService> selectAmbulance(const EmergencyRequest& request) const;

    // NEW (dispatch): takes the highest-priority PENDING request that currently has a
    // suitable available service, marks the service unavailable, records the assignment
    // and moves the request to ASSIGNED. Requests that cannot be served stay PENDING and
    // queued. Returns false (and changes nothing) if no request can be dispatched.
    // processNextEmergencyRequest() is kept as-is and does NOT reserve a service.
    bool dispatchNextEmergencyRequest(EmergencyRequest& out,
                                      std::shared_ptr<const EmergencyService>& assignedService);

    // NEW: id of the service currently assigned to a request, or "" if none.
    std::string getAssignedServiceId(const std::string& requestId) const;

private:
    std::unordered_map<std::string, EmergencyRequest> itemMapById;
    std::queue<std::string> requestQueue;
    std::priority_queue<EmergencyRequest, std::vector<EmergencyRequest>, HigherEmergencyPriority> priorityQueue;
    std::unordered_map<std::string, EmergencyContact> contactsById_;
    std::vector<std::shared_ptr<EmergencyService>> services_;
    // NEW: request id -> service id for active assignments (released on COMPLETED/CANCELLED)
    std::unordered_map<std::string, std::string> assignedServiceByRequest_;
    // NEW: ids currently held in priorityQueue, so a request is never queued twice
    std::unordered_set<std::string> queuedIds_;
};
