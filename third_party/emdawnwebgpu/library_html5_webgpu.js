var LibraryHTML5WebGPU = {
  emscripten_webgpu_get_device__deps: ['$WebGPU','wgpuDeviceAddRef'],
  emscripten_webgpu_get_device: () => {
#if ASSERTIONS
    assert(Module['preinitializedWebGPUDevice']);
#endif
    if (WebGPU.preinitializedDeviceId === undefined) {
      WebGPU.preinitializedDeviceId = WebGPU.importJsDevice(Module['preinitializedWebGPUDevice']);
    }
    _wgpuDeviceAddRef(WebGPU.preinitializedDeviceId);
    return WebGPU.preinitializedDeviceId;
  },
};

mergeInto(LibraryManager.library, LibraryHTML5WebGPU);
