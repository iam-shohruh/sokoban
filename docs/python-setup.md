# Python setup

## Requirements

- Python 3.12

Check your version:
```
python3 --version
```

If you don't have Python 3.12, download it from [python.org](https://www.python.org/downloads/).

---

## Create a virtual environment

From the root of the repo:
```
python3 -m venv .venv
```

This creates a `.venv/` folder that holds an isolated Python installation for this project. It is already in `.gitignore` — do not commit it.

---

## Activate it

Every time you open a new terminal, activate the environment before doing anything else.

**Mac / Linux**
```
source .venv/bin/activate
```

**Windows**
```
.venv\Scripts\activate
```

Your terminal prompt will change to show `(.venv)` when it is active.

---

## Install dependencies

```
pip install -r requirements.txt
```

Run this once after creating the environment, and again any time `requirements.txt` changes (e.g. after pulling someone else's changes that added a new package).

---

## Add a new dependency

```
pip install <package>
pip freeze > requirements.txt
```

Commit the updated `requirements.txt` so everyone else gets the same package.

---

## Deactivate

```
deactivate
```
