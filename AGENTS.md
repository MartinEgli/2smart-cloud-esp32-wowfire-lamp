## Granular Git Flow Policy

Use `granular-git-flow` for repository hygiene whenever work in this project
creates or changes files that should be versioned.

This project policy is the always-on trigger layer. The skill is the execution
method. Git hooks may still be used for technical checks, but they do not
replace the agent instruction.

Required behaviour:

- Inspect `git status --short --branch`, the current branch, staged changes,
  unstaged changes, untracked files, and remotes before committing.
- Use Git Flow branch names where practical: `feature/<topic>`, `fix/<topic>`,
  `release/<version>`, or `hotfix/<topic>`.
- Stage only files that belong to the current logical change.
- Prefer one commit per logical change.
- Run available validation before committing when the change is testable.
- Commit with concise Conventional Commit messages.
- Push after each completed commit when a remote exists.
- Leave unrelated user changes untouched and report them.
- Do not force-push, hard reset, clean, or discard work without explicit user
  instruction.

Useful initialization prompt:

```text
Use granular-git-flow init-project for this repo.
```
