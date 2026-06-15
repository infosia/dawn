#include <dawn/native/DawnNative.h>

extern "C" __attribute__((visibility("default"))) const DawnProcTable* getBundledDawnProcs() {
    return &dawn::native::GetProcs();
}
