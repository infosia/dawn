# Gemini Directives

- Do not run unit tests or formatting scripts.
- If a script needs to be run, ask the user to run it directly. This is the default behavior.
- The user can request to run specific scripts in special cases.
- This does not apply to searching, grepping, opening, editing, or creating new files.
- To see the full changes in a branch, use 'git upstream-diff' to see the history of all changes across many commits.

## Code review directives
- When asked to do a code review, spell check and fix all obvious spelling errors.
- Please check the code for 'dan style'. Dan style is to favor conditional early returns/continues rather than excessively deep nested conditionals. For example, instead of `if (condition) { ... }`, prefer `if (!condition) { return; } ...`. This reduces nesting and improves readability.