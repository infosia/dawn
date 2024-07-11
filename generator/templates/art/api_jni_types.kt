//* Copyright 2024 The Dawn & Tint Authors
//*
//* Redistribution and use in source and binary forms, with or without
//* modification, are permitted provided that the following conditions are met:
//*
//* 1. Redistributions of source code must retain the above copyright notice, this
//*    list of conditions and the following disclaimer.
//*
//* 2. Redistributions in binary form must reproduce the above copyright notice,
//*    this list of conditions and the following disclaimer in the documentation
//*    and/or other materials provided with the distribution.
//*
//* 3. Neither the name of the copyright holder nor the names of its
//*    contributors may be used to endorse or promote products derived from
//*    this software without specific prior written permission.
//*
//* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
//* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
//* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
//* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
//* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
//* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
//* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
//* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
//* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
//* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

{% macro arg_to_jni_type(arg) %}
    {% if arg.length == 'strlen' %}
        jstring
    {% elif arg.length and arg.length != 'constant' %}
        {% if arg.type.category in ['bitmask', 'enum', 'function pointer', 'object', 'structure'] %}
            jobjectArray
        {% elif arg.type.name.get() == 'void' %}
            jobject
        {% elif arg.type.name.get() == 'uint32_t' %}
            jintArray
        {% else %}
            {{ unreachable_code() }}
        {% endif %}
    {% else %}
        {{ to_jni_type(arg.type) }}
    {% endif %}
{% endmacro %}

{% macro to_jni_type(type) %}
    {% if type.category in ['function pointer', 'object', 'structure'] %}
        jobject
    {% elif type.category in ['bitmask', 'enum'] %}
        jint
    {% else %}
        {{ jni_primitives[type.name.get()] }}
    {% endif %}
{% endmacro %}

{% macro jni_signature(member) %}
    {%- if member.length == 'strlen' -%}
        Ljava/lang/String;
    {%- elif member.length and member.length != 'constant' -%}
        {%- if member.type.category in ['bitmask', 'enum', 'function pointer', 'object', 'structure'] -%}
            //*  JvmInline does not inline bitmask/enums in arrays.
            [L{{ jni_name(member.type) }};
        {%- elif member.type.name.get() == 'uint32_t' -%}
            [I
        {%- else -%}
            {{ unreachable_code() }}
        {%- endif -%}
    {%- elif member.type.category in ['function pointer', 'object', 'structure'] -%}
        L{{ jni_name(member.type) }};
    {%- elif member.type.name.get() in ['int', 'int64_t', 'uint64_t', 'size_t', 'void'] -%}  {# remove void #}
        J
    {%- elif member.type.name.get() in ['int', 'int32_t', 'uint32_t'] or member.type.category in ['bitmask', 'enum'] -%}
        //*  JvmInline makes lone bitmask/enums appear as integer to JNI.
        I
    {%- elif member.type.name.get() in ['int16_t', 'uint16_t'] -%}
        S
    {%- elif member.type.name.get() == 'double' -%}
        D
    {%- elif member.type.name.get() == 'float' -%}
        F
    {%- elif member.type.name.get() == 'bool' -%}
        Z
    {%- else -%}
        {{ unreachable_code() }}
    {%- endif -%}
{% endmacro %}

{% macro jni_convert(name, v, size, member) %}
    {% if member.type.name.get() in ['void const *', 'void *'] %}
        auto {{ name }} = toByteBuffer(env, {{ v }}, {{ size }});
    {% elif size %}
        //* Native container converted to a Kotlin container.
        jobjectArray {{ name }};
        {
            jclass clz2 = env->FindClass("{{ jni_name(member.type) }}");
            {{ name }} = env->NewObjectArray({{ size }}, clz2, 0);
            {% if member.type.category == 'object' %}
                jmethodID constructor = env->GetMethodID(clz2, "<init>", "(I)V");
            {% endif %}
            for (int idx = 0; idx != {{ size }}; idx++) {
                jobject element;
                {% if member.type.category == 'object' %}
                    element = env->NewObject(clz2, constructor, {{ v }}[idx]);
                {% else %}
                    // should do structure
                {% endif %}
                env->SetObjectArrayElement({{ name }}, idx, element);
            }
        }
    {% elif member.type.category == 'object' %}
        jobject {{ name }} =
                env->NewObject(env->FindClass("{{ jni_name(member.type) }}"),
                        env->GetMethodID(env->FindClass("{{ jni_name(member.type) }}"),
                                "<init>", "(J)V"),
                    reinterpret_cast<jlong>({{ v }}));
    {% elif member.type.category == 'structure' %}
        jobject {{ name }} =
        {% if member.annotation in ['*', 'const*'] %}
            Convert(env, {{ v }});
        {% else %}
            Convert(env, &{{ v }});
        {% endif %}
    {% elif member.type.name.get() in ['int64_t', 'uint64_t', 'size_t'] %}
        jlong {{ name }} = static_cast<jint>({{ v }});
    {% elif member.type.name.get() in ['int', 'int32_t', 'uint32_t'] or member.type.category in ['bitmask', 'enum'] %}
        jint {{ name }} = static_cast<jint>({{ v }});
    {% elif member.type.name.get() in ['int16_t', 'uint16_t'] %}
        jshort {{ name }} = {{ v }};
    {% elif member.type.name.get() == 'double' %}
        jdouble {{ name }} = {{ v }};
    {% elif member.type.name.get() == 'float' %}
        jfloat {{ name }} = {{ v }};
    {% elif member.type.name.get() == 'bool' %}
        bool {{ name }} = {{ v }};
    {% elif member.type.name.get() == 'char' %}
        jstring {{ name }} = env->NewStringUTF({{ v }});
    {% elif member.type.category == 'object' %}
        jclass returnClass = env->FindClass("{{ jni_name(member.type) }}");
        auto constructor = env->GetMethodID(returnClass, "<init>", "(J)V");
        auto {{ name }} = env->NewObject(returnClass, constructor, reinterpret_cast<jlong>({{ v }}));
    {% else %}
        auto {{ name }} = {{ v }};
    {% endif %}
{% endmacro %}

