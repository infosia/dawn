# Wire path diagram (with symbols)

- Client side (`src/dawn/wire/client`):
  - Core: `WireClient`, `ObjectBase`, typed client wrappers (e.g., `Buffer.cpp`, `RenderPassEncoder.cpp`).
  - Serialization: `SerializeCommand(...)` via `WireClient` and `ChunkedCommandSerializer`.
  - Memory transfer: `ClientMemoryTransferService` used by objects like `Buffer::Create()` for map pathways.

- Server side (`src/dawn/wire/server`):
  - Core: `WireServer`, `server::Server`/`Server*` shims per object.
  - Dispatch: Command handlers deserialize and forward to dawn_native objects.

- Transport:
  - Shared ring-buffer abstraction in wire; configurable to IPC or in-proc queues in embedders.

- Callbacks/events:
  - Server enqueues callbacks (map completion, error scopes, device loss) → serialized to client → surfaced to app/test harness.

Tests and mocks
- Mocks under `src/dawn/wire/server/*_mock.cpp` and usage in `src/dawn/tests` demonstrate expectations.

Validation split
- Most semantic validation occurs in dawn_native; the wire validates framing and object lifetimes/IDs.
