#include "EmergencyManager.hpp"
#include "DispatchRules.hpp"
#include <cctype>
#include <algorithm>
#include <utility>

namespace {
bool validPhone(const std::string& phone) {
    int digits = 0;
    for (unsigned char c : phone) {
        if (std::isdigit(c)) ++digits;
        else if (c != '+' && c != '-' && c != ' ' && c != '(' && c != ')') return false;
    }
    return digits >= 7 && digits <= 15;
}
bool validTransition(EmergencyStatus from, EmergencyStatus to) {
    if (to == EmergencyStatus::CANCELLED)
        return from != EmergencyStatus::COMPLETED && from != EmergencyStatus::CANCELLED;
    switch (from) {
        case EmergencyStatus::PENDING: return to == EmergencyStatus::ASSIGNED;
        case EmergencyStatus::ASSIGNED: return to == EmergencyStatus::ACCEPTED;
        case EmergencyStatus::ACCEPTED: return to == EmergencyStatus::EN_ROUTE;
        case EmergencyStatus::EN_ROUTE: return to == EmergencyStatus::COMPLETED;
        case EmergencyStatus::COMPLETED:
        case EmergencyStatus::CANCELLED: return false;
    }
    return false;
}

// Pops queue entries until it finds a PENDING request for which canServe() is true.
// The main request map stays authoritative: queue entries whose request is missing or
// no longer PENDING are stale and are dropped. Requests that are PENDING but cannot be
// served right now are put back (each at most once, so no duplicates are created).
template <typename PriorityQueue, typename CanServe>
bool takeNextPending(PriorityQueue& queue, std::unordered_set<std::string>& queuedIds,
                     const std::unordered_map<std::string, EmergencyRequest>& requests,
                     CanServe canServe, std::string& idOut) {
    std::vector<EmergencyRequest> deferred;
    bool found = false;
    while (!queue.empty()) {
        EmergencyRequest next = queue.top();
        queue.pop();
        const auto it = requests.find(next.getId());
        if (it == requests.end() || it->second.getStatus() != EmergencyStatus::PENDING) {
            queuedIds.erase(next.getId());  // stale entry
            continue;
        }
        if (!canServe(it->second)) {
            deferred.push_back(std::move(next));  // still queued, try lower priorities
            continue;
        }
        queuedIds.erase(next.getId());
        idOut = next.getId();
        found = true;
        break;
    }
    for (auto& request : deferred) queue.push(std::move(request));
    return found;
}
}

bool EmergencyManager::createEmergencyRequest(const EmergencyRequest& request) {
    if (request.getId().empty() || request.getUserId().empty() ||
        request.getLocation().locationId.empty() || request.getLocation().address.empty() ||
        itemMapById.count(request.getId())) return false;
    itemMapById.emplace(request.getId(), request);
    requestQueue.push(request.getId());
    return true;
}

const EmergencyRequest* EmergencyManager::findRequestById(const std::string& id) const {
    auto it = itemMapById.find(id);
    return it == itemMapById.end() ? nullptr : &it->second;
}

std::size_t EmergencyManager::prioritizeEmergencyRequests() {
    std::size_t moved = 0;
    while (!requestQueue.empty()) {
        const std::string id = requestQueue.front();
        requestQueue.pop();
        const auto it = itemMapById.find(id);
        if (it != itemMapById.end() && it->second.getStatus() == EmergencyStatus::PENDING &&
            queuedIds_.insert(id).second) {
            priorityQueue.push(it->second);
            ++moved;
        }
    }
    return moved;
}

bool EmergencyManager::updateRequestStatus(const std::string& id, EmergencyStatus status) {
    auto it = itemMapById.find(id);
    if (it == itemMapById.end() || !validTransition(it->second.getStatus(), status)) return false;
    it->second.setStatus(status);
    if (status == EmergencyStatus::COMPLETED || status == EmergencyStatus::CANCELLED) {
        const auto assignment = assignedServiceByRequest_.find(id);
        if (assignment != assignedServiceByRequest_.end()) {
            for (auto& service : services_)
                if (service->getId() == assignment->second) service->setAvailable(true);
            assignedServiceByRequest_.erase(assignment);
        }
    }
    return true;
}

bool EmergencyManager::processNextEmergencyRequest(EmergencyRequest& out) {
    // Always merge newly created requests first so they take part in priority ordering.
    prioritizeEmergencyRequests();
    std::string id;
    if (!takeNextPending(priorityQueue, queuedIds_, itemMapById,
                         [](const EmergencyRequest&) { return true; }, id))
        return false;
    const auto it = itemMapById.find(id);
    it->second.setStatus(EmergencyStatus::ASSIGNED);
    out = it->second;
    return true;
}

bool EmergencyManager::dispatchNextEmergencyRequest(
    EmergencyRequest& out, std::shared_ptr<const EmergencyService>& assignedService) {
    prioritizeEmergencyRequests();

    std::unordered_set<std::string> busyIds;
    for (const auto& assignment : assignedServiceByRequest_) busyIds.insert(assignment.second);

    std::string id;
    const bool found = takeNextPending(
        priorityQueue, queuedIds_, itemMapById,
        [&](const EmergencyRequest& request) {
            return DispatchRules::selectService(services_, request, busyIds) != nullptr;
        },
        id);
    if (!found) return false;

    const auto it = itemMapById.find(id);
    const std::shared_ptr<EmergencyService> service =
        DispatchRules::selectService(services_, it->second, busyIds);
    if (!service) {  // cannot happen (selection is deterministic); keep state consistent anyway
        priorityQueue.push(it->second);
        queuedIds_.insert(id);
        return false;
    }
    service->setAvailable(false);
    assignedServiceByRequest_[id] = service->getId();
    it->second.setStatus(EmergencyStatus::ASSIGNED);
    out = it->second;
    assignedService = service;
    return true;
}

std::string EmergencyManager::getAssignedServiceId(const std::string& requestId) const {
    const auto it = assignedServiceByRequest_.find(requestId);
    return it == assignedServiceByRequest_.end() ? std::string() : it->second;
}

EmergencyPriority EmergencyManager::classifySeverity(const std::string& description) {
    return DispatchRules::classifySeverity(description);
}

std::shared_ptr<const EmergencyService>
EmergencyManager::selectAmbulance(const EmergencyRequest& request) const {
    if (request.getServiceType() != EmergencyServiceType::AMBULANCE) return nullptr;
    std::unordered_set<std::string> busyIds;
    for (const auto& assignment : assignedServiceByRequest_) busyIds.insert(assignment.second);
    return DispatchRules::selectService(services_, request, busyIds);
}

std::vector<EmergencyRequest> EmergencyManager::getRequestHistory(const std::string& userId) const {
    std::vector<EmergencyRequest> history;
    for (const auto& entry : itemMapById)
        if (entry.second.getUserId() == userId) history.push_back(entry.second);
    std::sort(history.begin(), history.end(), [](const EmergencyRequest& a, const EmergencyRequest& b) {
        return a.getId() < b.getId();
    });
    return history;
}

std::vector<EmergencyRequest> EmergencyManager::getAllRequests() const {
    std::vector<EmergencyRequest> history;
    for (const auto& entry : itemMapById) history.push_back(entry.second);
    std::sort(history.begin(), history.end(), [](const EmergencyRequest& a, const EmergencyRequest& b) {
        return a.getId() > b.getId();
    });
    return history;
}

bool EmergencyManager::addEmergencyContact(const EmergencyContact& contact) {
    if (contact.getId().empty() || contact.getUserId().empty() || contact.getName().empty() ||
        contact.getRelationship().empty() || !validPhone(contact.getPhone()) ||
        contactsById_.count(contact.getId())) return false;
    contactsById_.emplace(contact.getId(), contact);
    return true;
}

std::vector<EmergencyContact> EmergencyManager::getEmergencyContacts(const std::string& userId) const {
    std::vector<EmergencyContact> result;
    for (const auto& entry : contactsById_)
        if (entry.second.getUserId() == userId) result.push_back(entry.second);
    return result;
}

void EmergencyManager::addEmergencyService(std::shared_ptr<EmergencyService> service) {
    if (!service) return;
    for (const auto& existing : services_)
        if (existing->getId() == service->getId()) return;  // ids must be unique
    services_.push_back(std::move(service));
}

std::vector<std::shared_ptr<const EmergencyService>>
EmergencyManager::findAvailableServices(EmergencyServiceType type) const {
    std::vector<std::shared_ptr<const EmergencyService>> result;
    for (const auto& service : services_)
        if (service->isAvailable() && service->getType() == type) result.push_back(service);
    return result;
}

std::vector<std::shared_ptr<const EmergencyService>> EmergencyManager::getEmergencyServices() const {
    std::vector<std::shared_ptr<const EmergencyService>> result;
    for (const auto& service : services_) result.push_back(service);
    return result;
}
