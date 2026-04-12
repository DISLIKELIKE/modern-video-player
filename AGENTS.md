# cx - Semantic Code Navigation

When `cx` is available in the project, prefer it over reading files directly.

## Escalation hierarchy: directory overview -> file overview -> symbols -> definition / references -> read

- **Explore a directory** -> `cx overview <dir>` (~20 tokens per entry)
- **Understand a file's structure** -> `cx overview <file>` (~200 tokens)
- **Find symbols across the project** -> `cx symbols [--kind K] [--name GLOB] [--file PATH]`
- **Read a specific function/type** -> `cx definition --name <name>` (~500 tokens)
- **Find all usages of a symbol** -> `cx references --name <name>` shows every usage with enclosing function and context
- **Check blast radius before refactoring** -> `cx references --name <name> --unique` shows one row per dependent function
- **Fall back to Read tool** only when you need the full file or surrounding context beyond the symbol body

## When to use cx instead of Read

- **Exploring a new codebase** - start with `cx overview .` to see top-level structure, then drill into subdirectories. Cheaper than `ls` + reading files.
- **Before reading a file** - run `cx overview` first. You often don't need the full file.
- **Before editing a function** - `cx definition --name X` gives you the exact text for Edit tool's `old_string` without reading the whole file.
- **Before refactoring** - `cx references --name X --unique` shows which functions depend on X (one row per caller). Use without `--unique` to see every usage with context lines.
- **Understanding how a symbol is used** - `cx references --name X` shows each usage site with the enclosing function and the source line, so you can see if it's called, used as a type, imported, etc.
- **Exploring a codebase** - use `cx symbols` to find what you need across files, then `cx definition` to read specific symbols. Avoid reading file after file.
- **After context compression** - if you previously read a file but the content was compressed out, use `cx overview` to re-orient and `cx definition` for the specific symbols you need. Don't re-read the full file.

## Quick reference

```
cx overview PATH                                    file or directory table of contents
cx overview DIR --full                              directory overview with signatures
cx symbols [--kind K] [--name GLOB] [--file PATH]   search symbols project-wide
cx definition --name NAME [--from PATH] [--kind K]  get a function/type body
cx references --name NAME [--file PATH] [--unique]   find all usages (--unique: one per caller)
cx lang list                                         show supported languages
cx lang add LANG [LANG...]                           install language grammars
```

Short aliases: `cx o`, `cx s`, `cx d`, `cx r`

Symbol kinds: fn, method, struct, enum, trait, type, const, class, interface, module, event

Check signatures for `pub`/`export` to identify public API without reading the file.

## Missing grammars

If cx reports a missing grammar (e.g. `cx: rust grammar not installed`), install it with `cx lang add rust`. Run `cx lang list` to see what's installed.

# RTK - Token-Optimized Shell

When `rtk` is available in the project, prefer it for shell commands that would otherwise emit large output.

- **Prefer RTK built-ins** -> `rtk read`, `rtk find`, `rtk grep`, `rtk diff`, `rtk summary`
- **Prefer RTK wrappers for noisy tools** -> `rtk git ...`, `rtk cargo ...`, `rtk pytest`, `rtk go test`, `rtk docker ...`, `rtk kubectl ...`
- **Windows note** -> prefer RTK's built-in subcommands over `rtk ls` when using PowerShell, because `ls` is a shell alias rather than an external binary
- **Fall back to raw commands** when full unfiltered output is required or when RTK does not support the command well

# Working Agreements

## Core Principles

- 优先做能修复 `root cause` 的最小正确改动。
- 先遵循 `repository` 现有 conventions，再考虑新增 abstraction。
- 仅在消除实际重复或提供明确 extension point 时引入 abstraction 或 design pattern。
- 默认保持改动范围收敛；只有为正确完成任务，才扩大重构范围。

## Repo-grounded Workflow

- 先调查，再判断。涉及 codebase、`repository`、file、function、API、config 或 `schema` 时，先读取相关实现和上下文，再下结论。
- 用户点名具体文件时，先读取该文件，再回答与其相关的判断、建议或修改。
- 仅基于已读取、已检查的信息发言；涉及行为、效果或修复结果的结论时，仅基于已验证的信息说明结论边界。对未打开的文件、未确认的实现或未复现的问题，明确说明不确定性，不做推测性断言。

## Change Boundaries

- 涉及 production dependency、`schema`、secrets 或 `CI/release workflow` 的变更前，先询问。
- 行为变化时，若附近已有 tests 或 documentation，优先更新最近相关项，使变更与现有覆盖保持一致。
- 遇到阻塞时，仅在低风险且不会制造半完成状态时，推进仍可独立完成的最小范围；无法继续时，明确说明阻塞原因、影响范围和未完成条件，不提前宣称完成。

## Validation and Reporting

- 优先验证最小受影响范围；无法进行完整 `validation` 时，明确说明已验证内容、未验证部分和剩余风险。
- 完成工作时，报告实际变更、验证结果、关键假设、决策依据、剩余缺口，以及仍需用户确认或后续处理的事项。
