/*
 * Copyright 2025 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

{% from 'art/api_kotlin_types.kt' import kotlin_annotation, kotlin_definition, kotlin_declaration, generate_simple_kdoc, add_kdoc_disclaimer, kotlin_member_optin, kotlin_class_optin, item_requires_optin, item_is_experimental, kotlin_parameter, kotlin_chain_parameter, kotlin_property, kotlin_chain_property with context %}
{{ add_kdoc_disclaimer() }}
package {{ kotlin_package }}

{% set ns = namespace(callback_count = 0, default_count = 0) %}
{%- set members = [] -%}
{%- for member in kotlin_record_members(structure.members, structure.name.get()) -%}
    {%- if member.name.camelCase().endswith('Callback') -%}
        {%- set ns.callback_count = ns.callback_count + 1 -%}
    {%- endif -%}
    {%- if kotlin_default(member) is not none -%}
        {%- set ns.default_count = ns.default_count + 1 -%}
    {%- endif -%}
    {%- do members.append(member) -%}
{%- endfor -%}

{%- set chain_children_list = chain_children[structure.name.get()] -%}
{%- for child in chain_children_list -%}
    {%- set ns.default_count = ns.default_count + 1 -%}
{%- endfor -%}

{%- set use_builder = ns.default_count > 3 -%}

{%- set class_is_experimental = item_is_experimental(structure) == 'True' -%}

{%- set stable_members = [] -%}
{%- set experimental_members = [] -%}
{%- for member in members -%}
    {%- if not class_is_experimental and item_requires_optin(member) == 'True' -%}
        {%- do experimental_members.append(member) -%}
    {%- else -%}
        {%- do stable_members.append(member) -%}
    {%- endif -%}
{%- endfor -%}

{%- set stable_chain_children = [] -%}
{%- set experimental_chain_children = [] -%}
{%- for child in chain_children_list -%}
    {%- if not class_is_experimental and item_requires_optin(child) == 'True' -%}
        {%- do experimental_chain_children.append(child) -%}
    {%- else -%}
        {%- do stable_chain_children.append(child) -%}
    {%- endif -%}
{%- endfor -%}

{%- set stable_ctor_has_defaults = namespace(val=false) -%}
{%- for member in stable_members -%}
    {%- if kotlin_default(member) is not none -%}
        {%- set stable_ctor_has_defaults.val = true -%}
        {%- break -%}
    {%- endif -%}
{%- endfor -%}
{%- if not stable_ctor_has_defaults.val -%}
    {%- if stable_chain_children -%}
        {%- set stable_ctor_has_defaults.val = true -%}
    {%- endif -%}
{%- endif -%}

//* Generating KDocs
{% set all_structs_info = kdocs.structs -%}
{%- set struct_info = all_structs_info.get(structure.name.get()) -%}
{%- set main_doc = struct_info.doc if struct_info else "" -%}
{%- if main_doc %}
    {{ generate_simple_kdoc(main_doc) }}
{% endif %}
{%- set experimental = kotlin_class_optin(structure) -%}
{%- if experimental %}
    {{ experimental }}
{% endif %}
{%- set has_experimental = (experimental_members or experimental_chain_children) and not class_is_experimental -%}
{%- if not has_experimental -%}
    public class {{ kotlin_name(structure) }}
        {%- if not use_builder %}
            {%- for member in members %}
                {% if kotlin_default(member) is not none %} @JvmOverloads constructor{% break %}{% endif %}
            {%- endfor %}
        {%- endif %}(
        {% for member in members %}
            //* Generating KDocs
            {% set member_doc = struct_info.members.get(member.name.get(), "") if struct_info and struct_info.members else "" %}
            {% if member_doc %}
                {{ generate_simple_kdoc(member_doc, indent_prefix = "    ", line_wrap_prefix = '\n     * ') }}
            {% endif %}
            {% if member.type.name.get() == "bool" %}@get:JvmName("is{{ member.name.CamelCase() }}") {% endif %}{{ kotlin_annotation(member) }} public var {{ member.name.camelCase() }}: {{ kotlin_definition(member) }},
        {% endfor %}
        {% for structure in chain_children_list %}
            //* Generating KDocs
            {% set chain_struct_doc = all_structs_info.get(structure.name.get()).doc if all_structs_info.get(structure.name.get()) else "" %}
            {% if chain_struct_doc %}
                {{ generate_simple_kdoc(chain_struct_doc, indent_prefix = "    ", line_wrap_prefix = '\n     * ') }}
            {% endif %}
            public var {{ structure.name.camelCase() }}: {{ kotlin_name(structure) }}? = null,
        {% endfor %}
    )
    {% if ns.callback_count > 1 or use_builder %}
        {
          {% if ns.callback_count > 1 %}
              {% for member in members %}
                  {% if member.name.camelCase().endswith('Callback') %}
                      {% set callback_name = member.name.camelCase() %}
                      {% set executor_name = callback_name + 'Executor' %}
                      {% set function_name = 'set' + member.name.CamelCase() %}
                      public fun {{ function_name }}(
                          {{ executor_name }}: java.util.concurrent.Executor,
                          {{ callback_name }}: {{ kotlin_definition(member) }}
                      ): Unit {
                          this.{{ executor_name }} = {{ executor_name }}
                          this.{{ callback_name }} = {{ callback_name }}
                      }
                  {% endif %}
              {% endfor %}
          {% endif %}

          {% if use_builder %}
                /**
                 * Builder for [{{ kotlin_name(structure) }}].
                 */
                public class Builder(
                    {% for member in members %}
                        {% if kotlin_default(member) is none %}
                            {{ kotlin_annotation(member) }} private val {{ member.name.camelCase() }}: {{ kotlin_declaration(member) }}{{ "," if not loop.last or ns.default_count > 0 }}
                        {% endif %}
                    {% endfor %}
                ) {
                    {% for member in members %}
                        {% if kotlin_default(member) is not none %}
                            {{ kotlin_annotation(member) }} private var {{ member.name.camelCase() }}: {{ kotlin_definition(member) }}
                        {% endif %}
                    {% endfor %}
                    {% for structure in chain_children_list %}
                        private var {{ structure.name.camelCase() }}: {{ kotlin_name(structure) }}? = null
                    {% endfor %}

                    {% for member in members %}
                        {% if kotlin_default(member) is not none %}
                            public fun set{{ member.name.CamelCase() }}({{ kotlin_annotation(member) }} {{ member.name.camelCase() }}: {{ kotlin_declaration(member) }}): Builder = apply {
                                this.{{ member.name.camelCase() }} = {{ member.name.camelCase() }}
                            }
                        {% endif %}
                    {% endfor %}
                    {% for structure in chain_children_list %}
                        public fun set{{ structure.name.CamelCase() }}({{ structure.name.camelCase() }}: {{ kotlin_name(structure) }}?): Builder = apply {
                            this.{{ structure.name.camelCase() }} = {{ structure.name.camelCase() }}
                        }
                    {% endfor %}

                    /**
                     * Builds the [{{ kotlin_name(structure) }}].
                     */
                    public fun build(): {{ kotlin_name(structure) }} = {{ kotlin_name(structure) }}(
                        {% for member in members %}
                            {{ member.name.camelCase() }} = {{ member.name.camelCase() }},
                        {% endfor %}
                        {% for structure in chain_children_list %}
                            {{ structure.name.camelCase() }} = {{ structure.name.camelCase() }},
                        {% endfor %}
                    )
                }
          {% endif %}
        }
    {% endif %}
{%- else -%}
    public class {{ kotlin_name(structure) }} {
        {% for member in members %}
            //* Generating KDocs
            {% set member_doc = struct_info.members.get(member.name.get(), "") if struct_info and struct_info.members else "" %}
            {% if member_doc %}
                {{ generate_simple_kdoc(member_doc, indent_prefix = "    ", line_wrap_prefix = '\n     * ') }}
            {% endif %}
            {{ kotlin_property(member, parent=structure) }}
        {% endfor %}
        {% for child in chain_children_list %}
            //* Generating KDocs
            {% set chain_struct_doc = all_structs_info.get(child.name.get()).doc if all_structs_info.get(child.name.get()) else "" %}
            {% if chain_struct_doc %}
                {{ generate_simple_kdoc(chain_struct_doc, indent_prefix = "    ", line_wrap_prefix = '\n     * ') }}
            {% endif %}
            {{ kotlin_chain_property(child, parent=structure) }}
        {% endfor %}

        {% set stable_params = [] -%}
        {%- for member in stable_members -%}
            {%- do stable_params.append(kotlin_parameter(member, parent=structure)) -%}
        {%- endfor -%}
        {%- for child in stable_chain_children -%}
            {%- do stable_params.append(kotlin_chain_parameter(child, parent=structure)) -%}
        {%- endfor -%}
        {%- if stable_ctor_has_defaults.val and not use_builder %}
            @JvmOverloads
        {% endif %}
        public constructor(
            {% for param in stable_params %}
                {{ param }}{{ "," if not loop.last }}
            {% endfor %}
        ) {
            {% for member in stable_members %}
                this.{{ member.name.camelCase() }} = {{ member.name.camelCase() }}
            {% endfor %}
            {% for child in stable_chain_children %}
                this.{{ child.name.camelCase() }} = {{ child.name.camelCase() }}
            {% endfor %}
        }

        {% if experimental_members or experimental_chain_children %}
            {% set exp_params = [] -%}
            {%- for member in stable_members -%}
                {%- do exp_params.append(kotlin_parameter(member, parent=structure)) -%}
            {%- endfor -%}
            {%- for child in stable_chain_children -%}
                {%- do exp_params.append(kotlin_chain_parameter(child, parent=structure)) -%}
            {%- endfor -%}
            {%- for member in experimental_members -%}
                {%- do exp_params.append(kotlin_parameter(member, parent=structure)) -%}
            {%- endfor -%}
            {%- for child in experimental_chain_children -%}
                {%- do exp_params.append(kotlin_chain_parameter(child, parent=structure)) -%}
            {%- endfor -%}
            {{"    "}}@ExperimentalWebGpuApi
            public constructor(
                {% for param in exp_params %}
                    {{ param }}{{ "," if not loop.last }}
                {% endfor %}
            ) {
                {% for member in stable_members %}
                    this.{{ member.name.camelCase() }} = {{ member.name.camelCase() }}
                {% endfor %}
                {% for child in stable_chain_children %}
                    this.{{ child.name.camelCase() }} = {{ child.name.camelCase() }}
                {% endfor %}
                {% for member in experimental_members %}
                    this.{{ member.name.camelCase() }} = {{ member.name.camelCase() }}
                {% endfor %}
                {% for child in experimental_chain_children %}
                    this.{{ child.name.camelCase() }} = {{ child.name.camelCase() }}
                {% endfor %}
            }
        {% endif %}
        {%- if ns.callback_count > 1 %}
            {% for member in members %}
                {% if member.name.camelCase().endswith('Callback') %}
                    {% set callback_name = member.name.camelCase() %}
                    {% set executor_name = callback_name + 'Executor' %}
                    {% set function_name = 'set' + member.name.CamelCase() %}

                    public fun {{ function_name }}(
                        {{ executor_name }}: java.util.concurrent.Executor,
                        {{ callback_name }}: {{ kotlin_definition(member) }}
                    ): Unit {
                        this.{{ executor_name }} = {{ executor_name }}
                        this.{{ callback_name }} = {{ callback_name }}
                    }
                {% endif %}
            {% endfor %}
        {% endif %}
        {%- if use_builder %}

            /**
             * Builder for [{{ kotlin_name(structure) }}].
             */
            public class Builder(
                {% for member in members %}
                    {% if kotlin_default(member) is none %}
                        {{ kotlin_annotation(member) }}{{ kotlin_member_optin(member, parent=structure) }} private val {{ member.name.camelCase() }}: {{ kotlin_declaration(member) }}{{ "," if not loop.last or ns.default_count > 0 }}
                    {% endif %}
                {% endfor %}
            ) {
                {% for member in members %}
                    {% if kotlin_default(member) is not none %}
                        {{ kotlin_annotation(member) }}{{ kotlin_member_optin(member, parent=structure) }} private var {{ member.name.camelCase() }}: {{ kotlin_definition(member) }}
                    {% endif %}
                {% endfor %}
                {% for child in chain_children_list %}
                    {{ kotlin_member_optin(child, parent=structure) }} private var {{ child.name.camelCase() }}: {{ kotlin_name(child) }}? = null
                {% endfor %}

                {% for member in members %}
                    {% if kotlin_default(member) is not none %}
                        {{ kotlin_member_optin(member, parent=structure) }} public fun set{{ member.name.CamelCase() }}({{ kotlin_annotation(member) }} {{ member.name.camelCase() }}: {{ kotlin_declaration(member) }}): Builder = apply {
                            this.{{ member.name.camelCase() }} = {{ member.name.camelCase() }}
                        }
                    {% endif %}
                {% endfor %}
                {% for child in chain_children_list %}
                    {{ kotlin_member_optin(child, parent=structure) }} public fun set{{ child.name.CamelCase() }}({{ child.name.camelCase() }}: {{ kotlin_name(child) }}?): Builder = apply {
                        this.{{ child.name.camelCase() }} = {{ child.name.camelCase() }}
                    }
                {% endfor %}

                /**
                 * Builds the [{{ kotlin_name(structure) }}].
                 */
                {% if (experimental_members or experimental_chain_children) and not class_is_experimental %}
                    @OptIn(ExperimentalWebGpuApi::class)
                {% endif %}
                public fun build(): {{ kotlin_name(structure) }} = {{ kotlin_name(structure) }}(
                    {% for member in stable_members %}
                        {{ member.name.camelCase() }} = {{ member.name.camelCase() }},
                    {% endfor %}
                    {% for child in stable_chain_children %}
                        {{ child.name.camelCase() }} = {{ child.name.camelCase() }},
                    {% endfor %}
                    {% for member in experimental_members %}
                        {{ member.name.camelCase() }} = {{ member.name.camelCase() }},
                    {% endfor %}
                    {% for child in experimental_chain_children %}
                        {{ child.name.camelCase() }} = {{ child.name.camelCase() }},
                    {% endfor %}
                )
            }
        {% endif %}
    }
{%- endif -%}