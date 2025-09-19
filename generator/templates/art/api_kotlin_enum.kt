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

{% set file_docs = kdocs.bitflags if enum.category == 'bitmask' else kdocs.enums %}
{% set docstring = file_docs.get(enum.name.snake_case(), {}).doc %}
{% if docstring %}
    /**
     * {{ docstring | trim | wordwrap(80, break_long_words=False, break_on_hyphens=False, wrapstring = "\n * ") }}
     */
{% endif %}
@JvmInline
public value class {{ enum.name.CamelCase() }}(public val value: Int) {
    {% if enum.category == 'bitmask' %}
        public infix fun or(b: {{ enum.name.CamelCase() }}): {{ enum.name.CamelCase() }} ={{ ' ' }}
            {{- enum.name.CamelCase() }}(this.value or b.value)
    {% endif %}
    public companion object {
        {% for value in enum.values %}
        {% set enum_doc = file_docs.get(enum.name.get(), {}) %}
        {% set value_docstring = enum_doc.get('entries', {}).get(value.name.snake_case()) %}
        {% if value_docstring %}
                /**
                 * {{ value_docstring | trim | wordwrap(80, break_long_words=False, break_on_hyphens=False, wrapstring = "\n         * ")}}
                 */
        {% endif %}
            public val {{ as_ktName(value.name.CamelCase()) }}: {{ enum.name.CamelCase() }} ={{' '}}
                {{- enum.name.CamelCase() }}({{ '{:#010x}'.format(value.value) }})
        {% endfor %}
        internal val names: Map<Int, String> = mapOf(
            {% for value in enum.values %}
                {{ '{:#010x}'.format(value.value) }} to "{{ as_ktName(value.name.CamelCase()) }}",
            {% endfor %}
        )
    }
    override fun toString(): String = names[value]?:value.toString()
}
