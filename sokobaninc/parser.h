#ifndef PARSER_H
#define PARSER_H

/*
 * Parses raw Sokoban level datasets into JSON (mirrors Python parser.py)
 *
 * Supported formats (auto-detected):
 *   sok      – single .sok file containing multiple levels (SokobanYASC format)
 *   xsokoban – directory of screen.N files, one level each
 *
 * Grid tile chars: # wall, $ box, * box-on-goal, . goal, @ player, + player-on-goal, space floor.
 */

/* ------------------------------------------------------------------ */
/* Public API (mirrors Python parse_dataset / main)                    */
/* ------------------------------------------------------------------ */

/*
 * Parses the dataset at data/raw/<dataset_name>/ and writes
 * data/parsed/<dataset_name>/levels.json.
 * Returns 0 on success, non-zero on error.
 */
int parse_dataset(const char *dataset_name);

/* Entry point when run as a standalone program */
int parser_main(int argc, char *argv[]);

/* ------------------------------------------------------------------ */
/* Internal helpers (exposed for testing)                              */
/* ------------------------------------------------------------------ */

typedef enum { FORMAT_SOK, FORMAT_XSOKOBAN, FORMAT_UNKNOWN } DatasetFormat;

DatasetFormat detect_format(const char *dataset_path);

#endif /* PARSER_H */