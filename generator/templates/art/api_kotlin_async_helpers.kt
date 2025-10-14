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
{% from 'art/api_kotlin_types.kt' import kotlin_annotation, kotlin_declaration, kotlin_definition with context %}

{% set all_callback_info = kdocs.callbacks %}
{% set all_objects = kdocs.objects %}
//* Provide an async wrapper for the 'callback info' type of async methods.
{% for obj in by_category['object'] %}
    {% for method in obj.methods if has_callbackInfoStruct(method) %}
        {% set callback_info = method.arguments[-1].type %}
        {% set callback_function = callback_info.members[-1].type %}
        {% set return_name = callback_function.name.chunks[:-1] | map('title') | join + 'Return' %}

        {% set curr_callback_info = all_callback_info.get(callback_info.name.get()) %}
        {% set doc_str = curr_callback_info.doc if curr_callback_info else "" %}
        {% set all_args = curr_callback_info.args if curr_callback_info else [] %}
        {% set has_any_arg_doc = namespace(found=false) %}
        {%- for arg in kotlin_record_members(callback_function.arguments) %}
            {% set camel_name = as_varName(arg.name) %}
            {% set snake_name = arg.name.snake_case() %}
            {% set arg_name = camel_name if camel_name in all_args else (snake_name if snake_name in all_args else None) %}
            {%- if arg_name and all_args.get(arg_name) %}
                {% set has_any_arg_doc.found = true %}
                {% break %}
            {%- endif %}
        {%- endfor %}
        {% if doc_str or has_any_arg_doc.found %}
            /**
            {%- if doc_str %}

                 * {{ doc_str | trim | wordwrap(90, break_long_words=False, break_on_hyphens=False, wrapstring = "\n*  ") }}
            {%- endif %}

            {%- for arg in kotlin_record_members(callback_function.arguments) %}
                {% set camel_name = as_varName(arg.name) %}
                {% set arg_doc = curr_callback_info.args.get(arg.name.snake_case()) if curr_callback_info else "" %}
                {% set snake_name = arg.name.snake_case() %}
                {% set arg_name = camel_name if camel_name in all_args else (snake_name if snake_name in all_args else None) %}
                {%- if has_any_arg_doc.found and all_args.get(arg_name) %}

                     * @param {{ as_varName(arg.name) + " " +  all_args.get(arg_name) | trim | wordwrap(90, break_long_words=False, break_on_hyphens=False, wrapstring = "\n * ") }}
                {%- endif %}
            {%- endfor %}

             */
        {% endif %}
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

        {% set cur_object_info = all_objects.get(obj.name.get()) %}
        {% set method_info = cur_object_info.methods.get(method.name.snake_case()) if cur_object_info else None %}
        {% if not method_info %}
            {% set method_info = cur_object_info.methods.get(method.name.camelCase()) if cur_object_info else None %}
        {% endif %}
        {% set method_doc = method_info.doc if method_info and method_info.doc else "" %}
        {% set return_str = method_info.returns_doc if method_info and method_info.returns_doc else "" %}
        {% set method_args_docs = method_info.args if method_info else {} %}
        {% set has_any_arg_doc = namespace(found=false) %}
        {% for arg in method.arguments[:-1] %}
            {% if method_args_docs.get(arg.name.snake_case()) %}
                {% set has_any_arg_doc.found = true %}
                {% break %}
            {% endif %}
        {% endfor %}
        {% if method_doc or return_str or has_any_arg_doc.found %}
            /**
            {%- if method_doc %}

                 * {{ method_doc | trim | wordwrap(90, break_long_words=False, break_on_hyphens=False, wrapstring = "\n * ") }}
            {%- endif %}
            {%- for arg in method.arguments[:-1] %}
                {% set arg_doc = method_args_docs.get(arg.name.snake_case()) %}
                {%- if arg_doc %}

                     * @param {{ as_varName(arg.name) }} {{ arg_doc | trim | wordwrap(90, break_long_words=False, break_on_hyphens=False, wrapstring = "\n * ") }}
                {%- endif %}
            {%- endfor %}
            {%- if return_str %}

                     * @return {{ return_str | trim | wordwrap(90, break_long_words=False, break_on_hyphens=False, wrapstring = "\n * ") }}
            {%- endif %}

             */
        {% endif %}
        //* Every method that is identified as using callbacks is given a helper method that wraps
        //* call with a suspend function.
        public suspend fun {{ obj.name.CamelCase() }}.{{ method.name.camelCase() }}(
            {%- for arg in method.arguments[:-1] %}
                {{ kotlin_annotation(arg) }}  {{ as_varName(arg.name) }}: {{ kotlin_definition(arg) }},
            {%- endfor %}): {{ return_name }} = suspendCoroutine {
                //* Result is handled by the async callback; this assignment satisfies @CheckReturnValue.
                val unused = {{ method.name.camelCase() }}(
                    {%- for arg in method.arguments %}
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

