# Sokoban

A Sokoban solver for Intro2AI course at YTU.

## Level format

Levels are represented as a 2D grid of ASCII characters:

| Char | Meaning          |
|------|------------------|
| `#`  | Wall             |
| ` `  | Floor            |
| `@`  | Player           |
| `+`  | Player on goal   |
| `$`  | Box              |
| `*`  | Box on goal      |
| `.`  | Goal             |

A valid level has exactly one player (`@` or `+`) and equal numbers of boxes and goals.

## Parser

`src/sokoban/parser.py` converts raw level files from `data/raw/<dataset>/` into a
structured JSON file at `data/parsed/<dataset>/levels.json`.

### Usage

```
python3 -m src.sokoban.parser <dataset>
```

`<dataset>` must be a directory name inside `data/raw/`. Currently available:

```
python3 -m src.sokoban.parser microban    # 155 levels
python3 -m src.sokoban.parser xsokoban   # 90 levels
```

### Output schema

```json
{
  "dataset": "microban",
  "format": "sok",
  "collection_meta": {
    "set": "Microban",
    "copyright": "...",
    "email": "...",
    "homepage": "..."
  },
  "levels": [
    {
      "id": 1,
      "title": "Microban 1",
      "author": "David W Skinner",
      "comment": null,
      "source_file": "DavidWSkinner Microban.sok",
      "width": 6,
      "height": 7,
      "grid": [
        "####",
        "# .#",
        "#  ###",
        "#*@  #",
        "#  $ #",
        "#  ###",
        "####"
      ]
    }
  ]
}
```

`grid` rows are stored as raw strings and may be shorter than `width` — pad with spaces on load if uniform-width arrays are needed.

### Supported formats

| Format     | Detection                          | Example dataset |
|------------|------------------------------------|-----------------|
| `sok`      | Directory contains a `.sok` file   | microban        |
| `xsokoban` | Directory contains `screen.N` files | xsokoban        |

To add a new dataset: place its raw files in `data/raw/<name>/`, add a parser function in `parser.py`, and register a detection rule in `_detect_format`.
