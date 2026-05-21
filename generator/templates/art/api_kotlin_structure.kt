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

{% from 'art/api_kotlin_types.kt' import kotlin_annotation, kotlin_definition, kotlin_declaration, generate_simple_kdoc, add_kdoc_disclaimer, kotlin_member_optin, kotlin_class_optin, item_requires_optin with context %}
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

//* Partition members: experimental members with defaults are placed in the class body.
//* This prevents stable constructors from requiring opt-in annotations.
{%- set constructor_members = [] -%}
{%- set body_members = [] -%}
{%- for member in members -%}
    {%- if item_requires_optin(member) == 'True' and kotlin_default(member) is not none -%}
        {%- do body_members.append(member) -%}
    {%- else -%}
        {%- do constructor_members.append(member) -%}
    {%- endif -%}
{%- endfor -%}

//* Partition chained children: experimental ones go to the body to keep constructor stable.
{%- set constructor_chain_children = [] -%}
{%- set body_chain_children = [] -%}
{%- for child in chain_children_list -%}
    {%- if item_requires_optin(child) == 'True' -%}
        {%- do body_chain_children.append(child) -%}
    {%- else -%}
        {%- do constructor_chain_children.append(child) -%}
    {%- endif -%}
{%- endfor -%}

//* Check if the constructor has any members or chain children that require opt-in.
{%- set ctor_requires_optin = namespace(val=false) -%}
{%- for member in constructor_members -%}
    {%- if item_requires_optin(member) == 'True' -%}
        {%- set ctor_requires_optin.val = true -%}
        {%- break -%}
    {%- endif -%}
{%- endfor -%}
{%- if not ctor_requires_optin.val -%}
    {%- for child in constructor_chain_children -%}
        {%- if item_requires_optin(child) == 'True' -%}
            {%- set ctor_requires_optin.val = true -%}
            {%- break -%}
        {%- endif -%}
    {%- endfor -%}
{%- endif -%}

//* Annotate the constructor if it requires opt-in and the whole class is not already experimental.
{%- set needs_experimental_ctor = ctor_requires_optin.val and "art_experimental" not in structure.json_data.get("tags", []) -%}
//* Request @JvmOverloads if we're not using a builder and any constructor member has a default value.
{%- set needs_jvm_overloads = namespace(val=false) -%}
{%- if not use_builder -%}
    {%- for member in constructor_members -%}
        {%- if kotlin_default(member) is not none -%}
            {%- set needs_jvm_overloads.val = true -%}
            {%- break -%}
        {%- endif -%}
    {%- endfor -%}
{%- endif -%}

//* Generating KDocs
{% set all_structs_info = kdocs.structs %}
{% set struct_info = all_structs_info.get(structure.name.get()) %}
{% set main_doc = struct_info.doc if struct_info else "" %}
{% if main_doc %}
    {{ generate_simple_kdoc(main_doc) }}
{% endif %}
{% set experimental = kotlin_class_optin(structure) -%}
{% if experimental %}
    //* If this entire WebGPU structure is experimental, the entire class is annotated.
    //* This propagates the opt-in requirement to any code referencing this structure type.
    {{ experimental }}
{% endif %}
public class {{ kotlin_name(structure) }}
    //* If the primary constructor contains mandatory experimental parameters, the constructor is annotated.
    //* This ensures stable constructors remain warning-free while shielding experimental parameters.
    {%- if needs_experimental_ctor %} @ExperimentalWebGpuApi{% endif %}
    {%- if needs_jvm_overloads.val %} @JvmOverloads{% endif %}
    {%- if needs_experimental_ctor or needs_jvm_overloads.val %} constructor{% endif %}(
    {% for member in constructor_members %}
        //* Generating KDocs
        {% set member_doc = struct_info.members.get(member.name.get(), "") if struct_info and struct_info.members else "" %}
        {% if member_doc %}
            {{ generate_simple_kdoc(member_doc, indent_prefix = "    ", line_wrap_prefix = '\n     * ') }}
        {% endif %}
        {% if member.type.name.get() == "bool" %}@get:JvmName("is{{ member.name.CamelCase() }}") {% endif %}{{ kotlin_annotation(member) }}{{ kotlin_member_optin(member, parent=structure) }} public var {{ member.name.camelCase() }}: {{ kotlin_definition(member) }},
    {% endfor %}
    {% for child in constructor_chain_children %}
        //* Generating KDocs
        {% set chain_struct_doc = all_structs_info.get(child.name.get()).doc if all_structs_info.get(child.name.get()) else "" %}
        {% if chain_struct_doc %}
            {{ generate_simple_kdoc(chain_struct_doc, indent_prefix = "    ", line_wrap_prefix = '\n     * ') }}
        {% endif %}
        {{ kotlin_member_optin(child, parent=structure) }} public var {{ child.name.camelCase() }}: {{ kotlin_name(child) }}? = null,
    {% endfor %}
)
{% if ns.callback_count > 1 or use_builder or body_members or body_chain_children %}
    {
      //* Isolating optional experimental properties and chained children in the class body
      //* ensures stable constructor compatibility and keeps standard usage warning-free.
      {% for member in body_members %}
          //* Generating KDocs
          {% set member_doc = struct_info.members.get(member.name.get(), "") if struct_info and struct_info.members else "" %}
          {% if member_doc %}
              {{ generate_simple_kdoc(member_doc, indent_prefix = "      ", line_wrap_prefix = '\n       * ') }}
          {% endif %}
            {{ kotlin_member_optin(member, parent=structure) }} public var {{ member.name.camelCase() }}: {{ kotlin_definition(member) }}
      {% endfor %}
      {% for child in body_chain_children %}
          //* Generating KDocs
          {% set chain_struct_doc = all_structs_info.get(child.name.get()).doc if all_structs_info.get(child.name.get()) else "" %}
          {% if chain_struct_doc %}
              {{ generate_simple_kdoc(chain_struct_doc, indent_prefix = "      ", line_wrap_prefix = '\n       * ') }}
          {% endif %}
            {{ kotlin_member_optin(child, parent=structure) }} public var {{ child.name.camelCase() }}: {{ kotlin_name(child) }}? = null
      {% endfor %}

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
                //* Internally opts-in to ExperimentalWebGpuApi for the generated assembly in the build() method.
                //* This allows the builder to map experimental body properties cleanly using .also { ... }
                //* without leaking warnings to the stable caller of build() (who already opted in at the setters).
                {% if item_requires_optin(structure, chain_children) == 'True' and "art_experimental" not in structure.json_data.get("tags", []) %}
                    @kotlin.OptIn(ExperimentalWebGpuApi::class)
                {% endif %}
                public fun build(): {{ kotlin_name(structure) }} = {{ kotlin_name(structure) }}(
                    {% for member in constructor_members %}
                        {{ member.name.camelCase() }} = {{ member.name.camelCase() }},
                    {% endfor %}
                    {% for child in constructor_chain_children %}
                        {{ child.name.camelCase() }} = {{ child.name.camelCase() }},
                    {% endfor %}
                )
                {%- if body_members or body_chain_children -%}
                .also { options ->
                    {% for member in body_members %}
                        options.{{ member.name.camelCase() }} = this.{{ member.name.camelCase() }}
                    {% endfor %}
                    {% for child in body_chain_children %}
                        options.{{ child.name.camelCase() }} = this.{{ child.name.camelCase() }}
                    {% endfor %}
                }
                {%- endif -%}
            }
      {% endif %}
    }
{% endif %}