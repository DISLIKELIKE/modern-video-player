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

