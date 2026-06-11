// AUTO-GENERATED FILE. DO NOT EDIT.
#include <dawn/dawn_proc.h>
#include <dawn/dawn_proc_table.h>
#include <string.h>

void InitializeDawnProcsFromWebView(WGPUProcGetProcAddress getProcAddress);

void InitializeDawnProcsFromWebView(WGPUProcGetProcAddress getProcAddress) {
    if (!getProcAddress) {
        dawnProcSetProcs(nullptr);
        return;
    }

    DawnProcTable newProcs = {};
    const uint8_t* libVersion = dawnProcGetVersion();
    memcpy(newProcs.version, libVersion, 20);
    newProcs.getProcAddress = getProcAddress;

    {% for function in by_category["function"] %}
        {% if function.name.get() != "get proc address" %}
            newProcs.{{as_varName(function.name)}} = reinterpret_cast<{{as_cProc(None, function.name)}}>(getProcAddress({"{{as_cMethod(None, function.name)}}", sizeof("{{as_cMethod(None, function.name)}}") - 1}));
        {% endif %}
    {% endfor %}

    {% for (type, methods) in c_methods_sorted_by_parent %}
        {% for method in methods %}
            newProcs.{{as_varName(type.name, method.name)}} = reinterpret_cast<{{as_cProc(type.name, method.name)}}>(getProcAddress({"{{as_cMethod(type.name, method.name)}}", sizeof("{{as_cMethod(type.name, method.name)}}") - 1}));
        {% endfor %}
    {% endfor %}

    dawnProcSetProcs(&newProcs);
}
