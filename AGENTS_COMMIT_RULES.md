---
enforced_by: agent
applies_to: git_commit
---

## Git Commit Rules

AI assistants **must strictly follow** this format when using `git commit`:

### Commit Message Format

```
<type>: <short description>

Co-Authored-By: Agent
```

### Rules

0. **No autonomous commit or push**: AI must never run `git commit`, `git push`, or any other git operation that alters the remote or commit history unless the user explicitly requests it

1. **Keep commit messages concise**:
   - Title: one-line short description (required)
   - Body: brief explanation when needed, but avoid over-description (optional)
   - Example 1:
     ```
     docs: add ColorConversion implementation docs

     Includes four-layer architecture, pipeline execution order, and usage examples
     ```
   - Example 2:
     ```
     fix: fix render pipeline memory leak
     ```
   - Example 3:
     ```
     refactor: refactor ECS component managers

     Improve component query performance, reduce cache misses
     ```

2. **Agent co-authorship**: must include `Co-Authored-By: Agent` as a separate paragraph

3. **Sign-off**: must use `git commit -s` to automatically add `Signed-off-by` trailer from git config

### Full Example

```bash
git commit -s -m "docs: add ColorConversion implementation docs

Co-Authored-By: Agent"
```

### Commit Type Prefixes

- `docs`: documentation updates
- `fix`: bug fixes
- `feat`: new features
- `refactor`: code refactoring
- `test`: test-related changes
- `perf`: performance improvements
- `style`: code formatting
- `chore`: build/toolchain updates
