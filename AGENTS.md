# Project Instructions

- Agents/AIs MUST NEVER modify or rewrite this file. It is a read-only system constraint.


## Communication style
- You should address me as 'sir' in EACH response you sent to me.


## Git conventions
- Propose users to use dedicated branch for new features/new bug fix exercises. But do NOT create new branch without being explicitly told so.
- Keep commits atomic: one logical change per commit. If a commit needs two types, split it.
- Format: `<type>(<scope>): <subject>`
  - `<scope>` is the module or area affected. Omit it (along with the parentheses) for repo-wide changes.
  - `<subject>` is imperative mood, lowercase, no trailing period, ≤72 characters. "strip trailing whitespace", not "Stripped trailing whitespace."
  - Example: `fix(parser): strip trailing whitespace`
- Types:
  - `feat`: a new feature
  - `fix`: a bug fix
  - `docs`: documentation only
  - `style`: formatting, whitespace, semicolons; no change in meaning
  - `refactor`: restructuring that neither fixes a bug nor adds a feature
  - `perf`: a change that improves performance
  - `test`: adding or correcting tests
  - `build`: build system or dependencies
  - `ci`: CI configuration and scripts
  - `revert`: reverts a previous commit
  - `chore`: maintenance that fits none of the above and touches
    neither src nor tests
- Breaking changes: append `!` after the type/scope (`feat(api)!: ...`) and explain the break in the commit body.
- Explain *why* in the body when the reason isn't obvious from the diff. Skip the body for trivial commits.
- Don't mention the contributor identity in commit messages -- reviewers should evaluate the code on its own.
- Write short commit messages -- reviewers should try to understand the project mostly by reading code, not reading the messages; otherwise it defeats the purpose of a code review.
- Do not overwrite git commit history, commit history MUST be write-only.
  - If you need to reverse a change, use a new commit to offset.


## Code comments (incl. docstring)
- Do not add excessive comments. Comments become stale fast; try to express the intention with code. Only add comments when absolutely necessary (at most two lines, BEST with only a few words).
- Remove excessive comments in the code you modify.


## Negative traits suppression

### Code bloat
- When evaluating options, subject to correctness, readability, and coherence requirements, prefer brevity and minimal or negative net LOC growth. AVOID scope creep.
- When preparing a code-change plan/reviewing code change, include an evaluation of the following:
  - Code size: Will the change result in a net increase or decrease in lines of code (LOC)?
  - Coherence: Does the proposed change improve or harm the codebase’s overall coherence?

### Hallucination
- Treat recalled facts as potentially stale. When asked to check or verify something, consult the relevant current source: inspect repository files for codebase claims and authoritative external sources for external or time-sensitive claims.
- Clearly distinguish verified facts from inferences and unresolved uncertainty. If you believe a sentence is a fact, append the source right next to it.
- Use internal knowledge primarily for reasoning and synthesis, not as a substitute for available source verification.

### Over-eagerness
- Do NOT start an implementation exercise before you are clearly asked to do so.


## Visibility enhancements
- Use Claude Code's Edit or Codex's patch/edit mechanism for file changes so edited files remain visible.
- Right before each time you need to invoke a tool, print a ONE-LINE summary on the intention of the invocation. The line MUST start with "Plan" tag with the corresponding formatting.
- Explicit preference tracking: When you learn a new developer preference, workflow habit, or project rule, do not just keep it in your internal context. Explicitly append it to the PREFERENCE.md file and prepend it with your name (e.g., gemini-3.7-flash/gpt-5.6-sol/claude-opus-5.0) and date. Do NOT use another files for preference tracking, keep everything explicit and visible to devlopers
- All intermediate/throwaway files must only be written under `<project root>/tmp/` directory 


## Memory management
- File name and file path: Use `<model-name>.md` file at the `<project root>/MEMORY` (hereafter `memory`); replace `<model-name>` with your name (e.g., gemini-3.7-flash/gpt-5.6-sol/claude-opus-5.0). If the file does not exist, create one.
  - At Session Startup: Read `memory` to understand your current objectives, recent changes, and outstanding tasks.
  - At Session Compact/Wrap-up/Checkpoints: Update `memory` using file-editing tools (i.e., do NOT use other update approach that bypass git diff style audit trail).
  - Chronological Activity Log: Prepend a fresh entry here for every milestone or session wrap-up. You **must** prepend the exact date in `YYYY-MM-DD` format to the entry header and arrange them in reverse chronological order (newest entries at the top of this section).
  - Automatic Pruning: `memory` should contain historical entries up to 1 month, anything older than that shall be removed.
- Isolation: read only your own `memory`, there may be other `memory` files belonging to other models, do not read them.
- PREFERENCE.md consolidation: PREFERENCE.md must be no more than 1000 words. If it exceeds the limit, compact it and give the compacted part a new date stamp.