# Capture and Replay Implementation Plan

This document outlines the steps to implement the "Serialize the Queue" design for capture and replay.

1. **Define the Serialization Format**
    *   Create a new header, e.g., `@src/dawn/native/capture/Serialization.h`, to define the serialization primitives.
    *   This file should contain:
        *   A magic number and version for the file format (e.g., `DAWNCAP\1`).
        *   An `enum class ObjectType` for each serializable Dawn object type (`Device`, `Buffer`, `Texture`, `CommandBuffer`, etc.).
        *   An `enum class CaptureCommand` for each high-level action we capture (e.g., `QueueSubmit`, `WriteBuffer`, `WriteTexture`, `unmap`). 

2. **Add `@src/dawn/native/CaptureContext.h` and `@src/dawn/native/CaptureContext.cpp`**
    *   This class will be the central manager for an active capture session.
    *   **Responsibilities**:
        *   Hold a reference to the output stream (`std::ostream&`).
        *   Track serialized objects to avoid duplicates. A `std::unordered_set<const ObjectBase*>` would work.
        *   Assign unique, stable IDs to objects. Since Dawn objects don't have built-in IDs for this purpose, we'll generate them. A `std::unordered_map<const ObjectBase*, ObjectId>` can map live object pointers to `uint64_t` IDs. An `ObjectId` can be a simple `uint64_t`.
        *   Getting an ID for an object will check if the object already has an ID. If so it will return it. If not it will make an id, remember it, and return the new id.
        *   Provide low-level serialization methods for writing primitives to the stream, e.g., `WriteU64(uint64_t)`, `WriteString(const std::string&)`, `WriteData(const void* data, size_t size)`.
    *   **Core Method 2**: `Capture<T>(const T* object);`
        *   This ch
    *   **Core Method 1**: `ObjectId GetId<T>(const T* object);`
        *   This method checks if the object has already been given 
        *   If not, it assigns it a new `ObjectId`, and calls `object->Serialize(this)`, 

3. **Modify `@src/dawn/native/Device.h` and `@src/dawn/native/Device.cpp`**
    *   **`DeviceBase::StartCapture(std::ostream& stream)`**:
        *   Check if a `CaptureContext` already exists on the `Queue`. If so, generate a validation error.
        *   Create a `std::unique_ptr<CaptureContext>` and move it to the `Queue`.
        *   Write a file header to the stream (magic number, version).
        *   Crucially, serialize the device itself to begin the object graph: `captureContext->Capture(this);`
    *   **`DeviceBase::EndCapture()`**:
        *   Flush the stream. The caller owns the stream and is responsible for closing it.
        *   Reset the `std::unique_ptr<CaptureContext>` on the `Queue` to `nullptr`, ending the capture.

4. **Modify `QueueBase::SubmitInternal` in `@src/dawn/native/Queue.cpp`**
    *   Inside the function, before the `SubmitImpl` call, add the capture logic:
        ```cpp
        if (mCaptureContext) {
            // Write a command to the stream indicating a Queue::Submit call
            mCaptureContext->WriteU32(static_cast<uint32_t>(CaptureCommand::QueueSubmit));
            // Serialize each command buffer in the submission
            for (size_t i = 0; i < commandCount; ++i) {
                commands[i]->Serialize(*mCaptureContext);
            }
        }
        ```

5. **Implement `CommandBufferBase::Serialize(CaptureContext& context)`**
    *   This method will be called by the `CaptureContext` when a command buffer is first seen during a `Submit`.
    *   **Implementation**:
        1.  Call the base `ObjectBase::Serialize` implementation.
        2.  Write the `ObjectType::CommandBuffer` enum.
        3.  Serialize its label (`mEncoderLabel`).
        4.  Walk through its resource usage tracker (`mResourceUsages`). For each resource, ensure it is captured:
            ```cpp
            for (const auto& resource : mResourceUsages) {
                context.Capture(resource);
            }
            ```
        5.  Serialize the commands (`mCommands`). This is the most complex part.
            *   Iterate through the `CommandIterator`.
            *   For each command (e.g., `copyBufferToBuffer`, `copyTextureToTexture`), write a unique command identifier to the stream, followed by the command's specific data.
                Note that mCommands is only high level commands like `copyBufferToBuffer`, `beginRenderPass`, etc...
                Lower level commands from render passes (draws) and compute passes (dispatch) need more code
                and can be implemented later.
            *   For commands that reference other objects (like `copyBufferToBuffer`), write the `ObjectId` of the referenced object. The `CaptureContext` can provide the ID for a given object pointer.

6. **Create a `dawn_end2end_test` in `@src/dawn/tests/end2end/CaptureReplayTests.cpp`**
    *   This test validates the capture mechanism.
    *   **Steps**:
        1.  Create a `std::stringstream` to capture to.
        2.  Call `device.StartCapture(stream)`.
        3.  Create an empty command buffer, or one with a simple command like setting a pipeline.
        4.  Submit the command buffer.
        5.  Call `device.EndCapture()`.
        6.  Assert that no validation errors occurred.
        7.  Assert that the `stringstream` is not empty and contains the file header. This confirms that the submission was serialized.