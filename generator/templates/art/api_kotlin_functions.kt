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

{% from 'art/api_kotlin_types.kt' import kotlin_annotation, kotlin_declaration, kotlin_definition with context %}
{% set all_functions = kdocs.functions %}
{% for function in by_category['function'] if include_method(function) %}
    {% set curr_function = all_functions.get(function.name.get()) %}
    {% set function_doc_str = curr_function.doc if curr_function else "" %}
    {% set return_str = curr_function.returns_doc if curr_function else "" %}
    {% set all_args = curr_function.args if curr_function else [] %}
    {% set has_any_arg_doc = namespace(found=false) %}
    {%- for arg in function.arguments %}
        {% set camel_name = as_varName(arg.name) %}
        {% set snake_name = arg.name.snake_case() %}
        {% set arg_name = camel_name if camel_name in all_args else (snake_name if snake_name in all_args else None) %}
        {%- if arg_name and all_args.get(arg_name) %}
            {% set has_any_arg_doc.found = true %}
            {% break %}
        {%- endif %}
    {%- endfor %}
    {% if function_doc_str or return_str or has_any_arg_doc.found %}
        /**
        {%- if function_doc_str %}

             * {{ function_doc_str | wordwrap(80, break_long_words=False, break_on_hyphens=False, wrapstring = "\n * ") }}
        {%- endif %}

        {%- for arg in function.arguments %}
            {% set camel_name = as_varName(arg.name) %}
            {% set snake_name = arg.name.snake_case() %}
            {% set arg_name = camel_name if camel_name in all_args else (snake_name if snake_name in all_args else None) %}
            {%- if has_any_arg_doc.found and all_args.get(arg_name) %}

                 * @param {{ as_varName(arg.name) + " " +  all_args.get(arg_name) | trim | wordwrap(90, break_long_words=False, break_on_hyphens=False, wrapstring = "\n * ") }}
            {%- endif %}
        {%- endfor %}

        {%- if return_str %}

             * @return {{ return_str | trim | wordwrap(80, break_long_words=False, break_on_hyphens=False, wrapstring = "\n* ") }}
        {%- endif %}

         */
    {% endif %}
    @FastNative
    {{ kotlin_annotation(kotlin_return(function)) }} public external fun {{ function.name.camelCase() }}(
        {%- for arg in function.arguments -%}
            {{- kotlin_annotation(arg) }} {{ as_varName(arg.name) }}: {{ kotlin_definition(arg) }},{{' '}}
        {%- endfor %}): {{ kotlin_declaration(kotlin_return(function)) }}
{% endfor %}
