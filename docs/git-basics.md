# Git basics

## Core concepts

- **Repository (repo):** the project folder tracked by Git
- **Commit:** a saved snapshot of your changes
- **Branch:** an independent line of work that won't affect `main` until merged
- **Remote:** the copy of the repo on GitHub (`origin`)

---

## First-time setup

Tell Git who you are (do this once on your machine):
```
git config --global user.name "Your Name"
git config --global user.email "you@example.com"
```

---

## Daily commands

**See what changed:**
```
git status
```

**See the actual diff:**
```
git diff
```

**Stage changes for a commit:**
```
git add <file>        # stage a specific file
git add .             # stage everything in the current directory
```

**Commit staged changes:**
```
git commit -m "short description of what you did"
```

Write commit messages in the imperative: *"add parser"*, not *"added parser"* or *"adding parser"*.

**Push your branch to GitHub:**
```
git push
```

First push on a new branch:
```
git push -u origin <branch-name>
```

**Pull latest changes from GitHub:**
```
git pull
```

---

## Branches

**See all branches:**
```
git branch
```

**Create and switch to a new branch:**
```
git checkout -b <branch-name>
```

**Switch to an existing branch:**
```
git checkout <branch-name>
```

**Never commit directly to `main`.** Always work on a branch and open a PR.

---

## Syncing with main

Before starting new work, make sure your local `main` is up to date:
```
git checkout main
git pull
git checkout -b <your-new-branch>
```

If `main` moved while you were working on your branch:
```
git checkout main
git pull
git checkout <your-branch>
git merge main
```

Resolve any conflicts, then commit.

---

## Undoing things

**Unstage a file (before committing):**
```
git restore --staged <file>
```

**Discard local changes to a file:**
```
git restore <file>
```

**Undo the last commit (keeps your changes):**
```
git reset --soft HEAD~1
```
