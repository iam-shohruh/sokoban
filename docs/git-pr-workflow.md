# Git PR workflow

This is the end-to-end flow for every piece of work: from picking up an issue to getting your code into `main`.

---

## 1. Pick up an issue

Go to the GitHub Issues tab, find your task, and assign it to yourself.

---

## 2. Start from a fresh main

```
git checkout main
git pull
```

---

## 3. Create a branch for the issue

Use the issue ID in the branch name so it's easy to trace:
```
git checkout -b issue-<id>-short-description
```

Example:
```
git checkout -b issue-7-bfs-solver
```

---

## 4. Do your work

Make changes, test them, then commit in small logical steps:
```
git add <files>
git commit -m "description of what this commit does"
```

Prefer multiple small commits over one giant commit.

---

## 5. Write a devlog entry

Before opening a PR, add a file at `devlog/<issue-id>.md` describing what you did.
The CI check will block the PR if this file is missing.

Use this as a loose template:
```markdown
## <date> — @<your-name>

**Done:** what you built or changed
**Decisions:** any choices you made and why
**Notes:** blockers, questions, links to relevant issues
```

Stage and commit it:
```
git add devlog/<issue-id>.md
git commit -m "add devlog entry for issue #<id>"
```

---

## 6. Push your branch

```
git push -u origin issue-<id>-short-description
```

---

## 7. Open a pull request

Go to the repo on GitHub. You will see a prompt to open a PR for your recently pushed branch.

In the PR description:
- Summarize what you did
- Write `closes #<id>` so the issue closes automatically when the PR is merged
- Tag a teammate to review

---

## 8. Address review comments

If a reviewer requests changes, make the edits locally, commit them, and push:
```
git add <files>
git commit -m "address review: ..."
git push
```

The PR updates automatically.

---

## 9. Merge

Once approved and CI passes, merge the PR on GitHub. Use **Squash and merge** if your branch has many messy commits, **Merge commit** if the commit history is clean and worth keeping.

Delete the branch after merging — GitHub offers a button for this.

---

## 10. Clean up locally

```
git checkout main
git pull
git branch -d issue-<id>-short-description
```
