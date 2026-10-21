# Priority-Based Ambulance Dispatch System

C++17 academic prototype. The ambulance fleet, incident locations, and symptom rules are simulated sample data, not live operational data. The symptom rules are not a clinically validated triage protocol or medical advice.

## Project layout

- `main.cpp` — menu-driven interface.
- `model/EmergencyRequest.hpp` — request model, existing enums, patient name, and optional project-specific capability requirement.
- `model/EmergencyContact.hpp` — existing emergency-contact model.
- `model/EmergencyService.hpp` — base emergency service.
- `model/EmergencyServices.hpp` — ambulance/fire/police service classes, capability and distance helper.
- `service/EmergencyManager.hpp/.cpp` — request queue, assignments, status transitions, service registry.
- `service/DispatchRules.hpp` — severity classification and compatible nearest-service selection.
- `service/ProjectDatabase.hpp` — predefined simulated locations, symptom categories, and ambulance fleet.
- `test/test_dispatch.cpp` — existing tests plus added project-specific capability tests.

## Build and run on Windows (VS Code terminal with MinGW-w64 g++)

Open the terminal in the folder containing `main.cpp`.

Build the application:

```powershell
g++ -std=c++17 -Wall -Wextra -pedantic main.cpp service/EmergencyManager.cpp -o ambulance_app.exe
```

Run the application:

```powershell
.\ambulance_app.exe
```

Build the test executable separately:

```powershell
g++ -std=c++17 -Wall -Wextra -pedantic test/test_dispatch.cpp service/EmergencyManager.cpp -o test_dispatch.exe
```

Run tests:

```powershell
.\test_dispatch.exe
```

## Dispatch rules in this prototype

- Severity Level 1 -> BLS only.
- Severity Level 2 -> BLS only.
- Severity Level 3 -> ALS only.
- Among available ambulances with the required capability, the nearest based on Haversine straight-line distance is selected.
- If no compatible unit is available, the request remains pending.
- An assigned service becomes unavailable and is released when the request is completed or cancelled through the manager's status-transition API.
- Unknown free-text symptoms are not registered as a low-severity request; the user is asked to use a supported symptom category or supported keyword.

## Limitations

- Fleet bases, location coordinates, and symptom mappings are simulated.
- Distance is estimated straight-line distance, not road distance or ETA.
- The symptom keyword matcher is simplistic; it does not understand negation or provide clinical triage.
- The menu does not yet expose controls to accept, start, complete, or cancel a request; the existing manager API supports status updates.
- Request IDs are unique for one application run, but records are stored in memory only and are not persisted after exit.
