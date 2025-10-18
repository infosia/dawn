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
package {{ kotlin_package }}

import dalvik.annotation.optimization.FastNative
import java.nio.ByteBuffer
{% for method in obj.methods if has_callbackInfoStruct(method) %}
    import java.util.concurrent.Executor
    {% break %}
{% endfor %}
{% from 'art/api_kotlin_types.kt' import kotlin_annotation, kotlin_declaration, kotlin_definition, has_doc, has_any_arg_doc with context %}

{% set all_objects = kdocs.objects%}
{% set object_docs = all_objects.get(obj.name.get()) %}
{% set doc_str = object_docs.doc if object_docs else "" %}
{% if doc_str | trim %}
    /**
     * {{ doc_str | trim | wordwrap(80, break_long_words=False, break_on_hyphens=False, wrapstring = '\n * ') }}
     */
{% endif %}
public class {{ obj.name.CamelCase() }}(public val handle: Long): AutoCloseable {
    {% set all_method_docs = object_docs.methods if object_docs else {} %}
    {% for method in obj.methods if include_method(method) %}
        {% set method_docs = all_method_docs.get(method.name.snake_case()) %}
        {% set doc_str = method_docs.doc if method_docs else "" %}
        {% set return_str = method_docs.returns_doc if method_docs else "" %}
        {% set all_args = method_docs.args if method_docs else [] %}
        {% if has_doc(doc_str, return_str, all_args, kotlin_record_members(method.arguments)) == 'True' %}
            {% set has_arg_doc = has_any_arg_doc(all_args, kotlin_record_members(method.arguments)) %}
            /**
            {%- if doc_str | trim %}

                 * {{ doc_str | trim | wordwrap(80, break_long_words=False, break_on_hyphens=False, wrapstring = "\n     * ") }}
            {%- endif %}
            {%- for arg in kotlin_record_members(method.arguments) %}
                {% set camel_name = as_varName(arg.name) %}
                {% set snake_name = arg.name.snake_case() %}
                {% set arg_name = camel_name if camel_name in all_args else (snake_name if snake_name in all_args else None) %}
                {% set arg_doc = all_args.get(arg_name) | trim if arg_name else "" %}
                {%- if has_arg_doc == 'True' and arg_doc %}

                     * @param {{ as_varName(arg.name) + " " +  arg_doc | wordwrap(90, break_long_words=False, break_on_hyphens=False, wrapstring = "\n     * ") }}
                {%- endif %}
            {%- endfor %}
            {%- if return_str | trim %}

                 * @return {{ return_str | trim | wordwrap(80, break_long_words=False, break_on_hyphens=False, wrapstring = "\n     * ") }}
            {%- endif %}

             */
        {% endif %}
        @FastNative
        @JvmName("{{ method.name.camelCase() }}")
        {{ kotlin_annotation(kotlin_return(method)) }} public external fun {{ method.name.camelCase() }}(
        //* TODO(b/341923892): rework async methods to use futures.
        {%- for arg in kotlin_record_members(method.arguments) %}
            {%- if as_varName(arg.name) == 'callback' %}
                {{arg.name.camelCase()}}Executor: Executor = Executor(Runnable::run),{{' '}}
            {%- endif %}
            {{- kotlin_annotation(arg) }} {{ as_varName(arg.name) }}: {{ kotlin_definition(arg) }},{{ ' ' }}
        {%- endfor -%}): {{ kotlin_declaration(kotlin_return(method)) }}

        {% if method.name.chunks[0] == 'get' and not method.arguments %}
            //* For the Kotlin getter, strip word 'get' from name and convert the remainder to
            //* camelCase() (lower case first word). E.g. "get foo bar" translated to fooBar.
            {% set name = method.name.chunks[1] + method.name.chunks[2:] | map('title') | join %}
            @get:JvmName("{{ name }}")
            public val {{ name }}: {{ kotlin_declaration(kotlin_return(method)) }} get() = {{ method.name.camelCase() }}()

        {% endif %}
    {% endfor %}
    external override fun close()

    //* By default, the equals() function implements referential equality.
    //* see: https://kotlinlang.org/docs/equality.html#structural-equality
    //* A structural comparison of the wrapper object is equivalent to a referential comparison of
    //* the wrapped object.
    override fun equals(other: Any?): Boolean =
        other is {{ obj.name.CamelCase() }} && other.handle == handle
    override fun hashCode(): Int = handle.hashCode()
}
