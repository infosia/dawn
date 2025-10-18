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
{% from 'art/api_kotlin_types.kt' import kotlin_annotation, kotlin_declaration, kotlin_definition, has_doc, has_any_arg_doc with context %}

{% set callbackName = 'on' + function_pointer.name.chunks[:-1] | map('title') | join %}
{% set all_docs = kdocs.callbacks %}
{% set curr_callback = all_docs.get(function_pointer.name.get()) %}
{% set doc_str = curr_callback.doc if curr_callback else "" %}
{% set all_args =  curr_callback.args if curr_callback else [] %}

{% if has_doc(doc_str, "", all_args, kotlin_record_members(function_pointer.arguments)) == 'True' %}
    {% set has_arg_doc = has_any_arg_doc(all_args, kotlin_record_members(function_pointer.arguments)) %}
    /**
    {%- if doc_str | trim %}

         * {{ doc_str | trim | wordwrap(80, break_long_words=False, break_on_hyphens=False, wrapstring = "\n * ") }}
    {%- endif %}

    {%- for arg in kotlin_record_members(function_pointer.arguments) %}
        {% set camel_name = as_varName(arg.name) %}
        {% set snake_name = arg.name.snake_case() %}
        {% set arg_name = camel_name if camel_name in all_args else (snake_name if snake_name in all_args else None) %}
        {% set arg_doc = all_args.get(arg_name) | trim if arg_name else "" %}
        {%- if has_arg_doc == 'True' and arg_doc %}

             * @param {{ as_varName(arg.name) + " " +  arg_doc | wordwrap(90, break_long_words=False, break_on_hyphens=False, wrapstring = "\n * ") }}
        {%- endif %}
    {%- endfor %}

     */
{% endif %}
public fun interface {{ function_pointer.name.CamelCase() }} {
    @Suppress("INAPPLICABLE_JVM_NAME")  //* Required for @JvmName on global function.
    @JvmName("{{ callbackName }}")  //* Required to access Inline Value Class parameters via JNI.
    public fun {{ callbackName }}(
    {%- for arg in kotlin_record_members(function_pointer.arguments) -%}
        {{ kotlin_annotation(arg) }} {{ as_varName(arg.name) }}: {{ kotlin_declaration(arg) }},{{ ' ' }}
    {%- endfor -%});
}

{% set args_list = kotlin_record_members(function_pointer.arguments) | list %}

internal class {{ function_pointer.name.CamelCase() }}Runnable(
private val callback: {{ function_pointer.name.CamelCase() }},
{% for arg in args_list %}
    private val {{ as_varName(arg.name) }}: {{ kotlin_declaration(arg) }}{{ ','
    if not loop.last }}
{% endfor %}
) : Runnable {
    override fun run() {
        callback.{{ callbackName }}(
            {%- for arg in args_list -%}
            {{ as_varName(arg.name) }}{{ ', ' if not loop.last }}
            {%- endfor -%}
        )
    }
}
