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

import kotlin.coroutines.resume
import kotlin.coroutines.suspendCoroutine
{% from 'art/api_kotlin_types.kt' import kotlin_annotation, kotlin_declaration, kotlin_definition, check_if_doc_present, generate_kdoc with context %}

{% set all_callback_info = kdocs.callbacks %}
{% set all_objects = kdocs.objects %}
//* Provide an async wrapper for the 'callback info' type of async methods.
{% for obj in by_category['object'] %}
    {% for method in obj.methods if has_callbackInfoStruct(method) %}
        {% set callback_info = method.arguments[-1].type %}
        {% set callback_function = callback_info.members[-1].type %}
        {% set return_name = callback_function.name.chunks[:-1] | map('title') | join + 'Return' %}

        {# Generating KDocs #}
        {% set curr_callback_info = all_callback_info.get(callback_info.name.get()) %}
        {% set callback_doc = curr_callback_info.doc if curr_callback_info else "" %}
        {% set callback_args_doc = curr_callback_info.args if curr_callback_info else {} %}
        {% set callback_args = kotlin_record_members(callback_function.arguments) | list %}
        {% if check_if_doc_present(callback_doc, "", callback_args_doc, callback_args) == 'True' %}
            {{ generate_kdoc(callback_doc, "", callback_args_doc, callback_args, line_wrap_prefix = "\n * ") }}

        {%- endif %}
        //* We make a return class for every callback method so that it can be used inline
        //* (without callbacks) in a suspend (async) function.
        public class {{ return_name }}(
            {% for arg in kotlin_record_members(callback_function.arguments) %}
                {{ kotlin_annotation(arg) }} public val {{ as_varName(arg.name) }}: {{ kotlin_declaration(arg) }},
            {% endfor %}) {
            //* Required for destructuring declarations. These come for free in a 'data' class but
            //* we don't make it a data class because that can cause binary compatibility issues.
            {% for arg in kotlin_record_members(callback_function.arguments) %}
                public operator fun component{{ loop.index }}() : {{ kotlin_declaration(arg) }} =
                    {{- as_varName(arg.name) }}
            {% endfor %}
        }

        {# Generating KDocs #}
        {% set cur_object_info = all_objects.get(obj.name.get()) %}
        {% set method_info = cur_object_info.methods.get(method.name.snake_case()) if cur_object_info else None %}
        {% if not method_info %}
            {% set method_info = cur_object_info.methods.get(method.name.camelCase()) if cur_object_info else None %}
        {% endif %}
        {% set method_doc = method_info.doc if method_info and method_info.doc else "" %}
        {% set return_doc = method_info.returns_doc if method_info and method_info.returns_doc else "" %}
        {% set args_doc = method_info.args if method_info else {} %}
        {% set method_args = method.arguments[:-1] | list %}
        {% if check_if_doc_present(method_doc, return_doc, args_doc, method_args) != 'False' %}
            {{ generate_kdoc(method_doc, return_doc, args_doc, method_args, line_wrap_prefix = "\n * ") }}

        {%- endif %}
        //* Every method that is identified as using callbacks is given a helper method that wraps
        //* call with a suspend function.
        public suspend fun {{ obj.name.CamelCase() }}.{{ method.name.camelCase() }}(
            {%- for arg in method.arguments[:-1] %}
                {{ kotlin_annotation(arg) }}  {{ as_varName(arg.name) }}: {{ kotlin_definition(arg) }},
            {%- endfor %}): {{ return_name }} = suspendCoroutine {
                //* Result is handled by the async callback; this assignment satisfies @CheckReturnValue.
                val unused = {{ method.name.camelCase() }}(
                    {%- for arg in kotlin_record_members(method.arguments) %}
                        {%- if as_varName(arg.name) == 'callbackInfo' %}
                            {{- as_varName(arg.name).removesuffix("Info")}}
                        {%- else %}
                            {{- as_varName(arg.name) }}
                        {%- endif %}
                        {%- if loop.last %}
                            //* The final parameter of a callback method is always callback info.
                            //* We make this and include our generated callback.
                            {{- ' = ' }}
                        {%- else %}
                            //* Non-final parameters are whatever the client supplied.
                            {{- ', ' }}
                        {%- endif %}
                    {%- endfor %}{
                    {%- for arg in kotlin_record_members(callback_function.arguments) %}
                        {{- as_varName(arg.name) }},
                    {%- endfor %} -> it.resume({{ return_name }}(
                        //* We make an instance of the callback parameters -> return type wrapper.
                        {%- for arg in kotlin_record_members(callback_function.arguments) %}
                            {{- as_varName(arg.name) }} {{ ', ' }}
                        {%- endfor %})
                    )
                })
            }
    {% endfor %}
{% endfor %}

