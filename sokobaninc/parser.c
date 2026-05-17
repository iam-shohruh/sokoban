#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdbool.h>

#ifdef _WIN32
#include <io.h>
#include <direct.h>
#define mkdir(path, mode) _mkdir(path)
#else
#include <sys/stat.h>
#include <sys/types.h>
#endif
/* ------------------------------------------------------------------ */
/* _GRID_CHARS  (mirrors Python _GRID_CHARS = frozenset(" #.$*@+"))   */
/* ------------------------------------------------------------------ */

static bool _is_grid_char(char c) {
    return c == ' ' || c == '#' || c == '.' || c == '$'
        || c == '*' || c == '@' || c == '+';
}

/* mirrors Python _is_grid_line */
static bool _is_grid_line(const char *line) {
    if (!line || !*line) return false;
    for (const char *p = line; *p; p++) {
        if (!_is_grid_char(*p)) return false;
    }
    return true;
}

/* ------------------------------------------------------------------ */
/* Utility: trim trailing \r\n                                         */
/* ------------------------------------------------------------------ */

static void rtrim(char *s) {
    int n = (int)strlen(s);
    while (n > 0 && (s[n-1] == '\r' || s[n-1] == '\n')) s[--n] = '\0';
}

/* ------------------------------------------------------------------ */
/* JSON escaping helper                                                 */
/* ------------------------------------------------------------------ */

static void json_write_string(FILE *f, const char *s) {
    if (!s) { fprintf(f, "null"); return; }
    fputc('"', f);
    for (; *s; s++) {
        if      (*s == '"')  fprintf(f, "\\\"");
        else if (*s == '\\') fprintf(f, "\\\\");
        else if (*s == '\n') fprintf(f, "\\n");
        else if (*s == '\r') fprintf(f, "\\r");
        else                 fputc(*s, f);
    }
    fputc('"', f);
}

/* ------------------------------------------------------------------ */
/* _parse_collection_header                                            */
/* mirrors Python: extract Key: Value pairs                            */
/* ------------------------------------------------------------------ */

typedef struct { char key[64]; char value[256]; } MetaEntry;

static int _parse_collection_header(char **lines, int nlines,
                                    MetaEntry *out, int max_out) {
    int count = 0;
    for (int i = 0; i < nlines && count < max_out; i++) {
        char *colon = strchr(lines[i], ':');
        if (!colon) continue;
        /* Key must start with A-Za-z */
        if (!isalpha((unsigned char)lines[i][0])) continue;
        int klen = (int)(colon - lines[i]);
        if (klen <= 0 || klen >= 64) continue;
        strncpy(out[count].key, lines[i], klen);
        out[count].key[klen] = '\0';
        /* lowercase key */
        for (int j = 0; j < klen; j++) out[count].key[j] = tolower(out[count].key[j]);
        /* value: trim leading space */
        const char *val = colon + 1;
        while (*val == ' ') val++;
        strncpy(out[count].value, val, sizeof(out[count].value)-1);
        out[count].value[sizeof(out[count].value)-1] = '\0';
        count++;
    }
    return count;
}

static const char *meta_get(const MetaEntry *meta, int nmeta, const char *key) {
    for (int i = 0; i < nmeta; i++) {
        if (strcmp(meta[i].key, key) == 0) return meta[i].value;
    }
    return NULL;
}

/* ------------------------------------------------------------------ */
/* _parse_sok                                                           */
/* mirrors Python _parse_sok()                                         */
/* ------------------------------------------------------------------ */

static int _parse_sok(const char *path, const char *dataset_name,
                      const char *out_path) {
    FILE *f = fopen(path, "rb");
    if (!f) { fprintf(stderr, "Cannot open '%s'\n", path); return 1; }
    fseek(f, 0, SEEK_END); long fsz = ftell(f); rewind(f);
    char *raw = (char *)malloc(fsz + 1);
    fread(raw, 1, fsz, f); raw[fsz] = '\0'; fclose(f);

    /* Split into lines */
    int   max_lines = 8192;
    char **lines    = (char **)malloc(max_lines * sizeof(char *));
    int   nlines    = 0;
    char *tok       = strtok(raw, "\n");
    while (tok && nlines < max_lines) {
        rtrim(tok);
        lines[nlines++] = tok;
        tok = strtok(NULL, "\n");
    }

    /* --- Split into blocks separated by blank lines (mirrors Python) --- */
    int   max_blocks = 512;
    char ***blocks   = (char ***)malloc(max_blocks * sizeof(char **));
    int   *block_sz  = (int *)calloc(max_blocks, sizeof(int));
    int   nblocks    = 0;
    char **cur_block = (char **)malloc(512 * sizeof(char *));
    int   cur_sz     = 0;

    for (int i = 0; i < nlines; i++) {
        if (lines[i][0] == '\0') {
            if (cur_sz > 0 && nblocks < max_blocks) {
                blocks[nblocks]   = (char **)malloc(cur_sz * sizeof(char *));
                block_sz[nblocks] = cur_sz;
                memcpy(blocks[nblocks], cur_block, cur_sz * sizeof(char *));
                nblocks++;
                cur_sz = 0;
            }
        } else {
            if (cur_sz < 512) cur_block[cur_sz++] = lines[i];
        }
    }
    if (cur_sz > 0 && nblocks < max_blocks) {
        blocks[nblocks]   = (char **)malloc(cur_sz * sizeof(char *));
        block_sz[nblocks] = cur_sz;
        memcpy(blocks[nblocks], cur_block, cur_sz * sizeof(char *));
        nblocks++;
    }
    free(cur_block);

    /* --- Separate header blocks from level blocks --- */
    char **header_lines = (char **)malloc(1024 * sizeof(char *));
    int    n_header     = 0;
    int    in_levels    = 0;
    int   *level_block_idx = (int *)malloc(max_blocks * sizeof(int));
    int    n_level_blocks  = 0;

    for (int b = 0; b < nblocks; b++) {
        if (!in_levels) {
            /* Python: if re.match(r'^\d+$', block[0].strip()): in_levels=True */
            bool all_digit = true;
            for (int c = 0; blocks[b][0][c]; c++) {
                if (!isdigit((unsigned char)blocks[b][0][c])) { all_digit = false; break; }
            }
            if (all_digit && blocks[b][0][0] != '\0') in_levels = 1;
        }
        if (in_levels) {
            level_block_idx[n_level_blocks++] = b;
        } else {
            for (int l = 0; l < block_sz[b] && n_header < 1024; l++)
                header_lines[n_header++] = blocks[b][l];
        }
    }

    MetaEntry collection_meta[32];
    int       n_cmeta = _parse_collection_header(header_lines, n_header,
                                                  collection_meta, 32);

    /* --- Open output JSON --- */
    FILE *out = fopen(out_path, "w");
    if (!out) { fprintf(stderr, "Cannot write '%s'\n", out_path); return 1; }

    fprintf(out, "{\n  \"dataset\": ");
    json_write_string(out, dataset_name);
    fprintf(out, ",\n  \"format\": \"sok\",\n  \"collection_meta\": {");
    for (int i = 0; i < n_cmeta; i++) {
        if (i) fprintf(out, ", ");
        json_write_string(out, collection_meta[i].key);
        fprintf(out, ": ");
        json_write_string(out, collection_meta[i].value);
    }
    fprintf(out, "},\n  \"levels\": [\n");

    /* --- Parse each level block --- */
    for (int lb = 0; lb < n_level_blocks; lb++) {
        int   b     = level_block_idx[lb];
        char **blk  = blocks[b];
        int   bsz   = block_sz[b];

        int level_id = atoi(blk[0]);

        char **grid_lines  = (char **)malloc(bsz * sizeof(char *)); int ngrid = 0;
        char **meta_lines2 = (char **)malloc(bsz * sizeof(char *)); int nmeta2 = 0;
        char  comment_buf[2048]; comment_buf[0] = '\0';
        bool  in_grid    = true;
        bool  in_comment = false;

        for (int l = 1; l < bsz; l++) {
            if (in_comment) {
                if (strncmp(blk[l], "Comment-End:", 12) == 0) in_comment = false;
                else {
                    strncat(comment_buf, blk[l], sizeof(comment_buf)-strlen(comment_buf)-2);
                    strncat(comment_buf, "\n", sizeof(comment_buf)-strlen(comment_buf)-1);
                }
            } else if (strncmp(blk[l], "Comment:", 8) == 0) {
                in_comment = true;
            } else if (in_grid && _is_grid_line(blk[l])) {
                grid_lines[ngrid++] = blk[l];
            } else {
                in_grid = false;
                meta_lines2[nmeta2++] = blk[l];
            }
        }

        MetaEntry per_level_meta[16];
        int       n_plmeta = _parse_collection_header(meta_lines2, nmeta2, per_level_meta, 16);
        const char *title  = meta_get(per_level_meta, n_plmeta, "title");
        const char *author = meta_get(per_level_meta, n_plmeta, "author");

        int width = 0;
        for (int g = 0; g < ngrid; g++) {
            int l = (int)strlen(grid_lines[g]);
            if (l > width) width = l;
        }
        int height = ngrid;

        if (lb) fprintf(out, ",\n");
        fprintf(out, "    {\n");
        fprintf(out, "      \"id\": %d,\n", level_id);
        fprintf(out, "      \"title\": "); json_write_string(out, title); fprintf(out, ",\n");
        fprintf(out, "      \"author\": "); json_write_string(out, author); fprintf(out, ",\n");
        fprintf(out, "      \"comment\": ");
        if (comment_buf[0]) json_write_string(out, comment_buf);
        else fprintf(out, "null");
        fprintf(out, ",\n");
        fprintf(out, "      \"source_file\": "); json_write_string(out, path); fprintf(out, ",\n");
        fprintf(out, "      \"width\": %d,\n", width);
        fprintf(out, "      \"height\": %d,\n", height);
        fprintf(out, "      \"grid\": [");
        for (int g = 0; g < ngrid; g++) {
            if (g) fprintf(out, ", ");
            json_write_string(out, grid_lines[g]);
        }
        fprintf(out, "]\n    }");

        free(grid_lines);
        free(meta_lines2);
    }

    fprintf(out, "\n  ]\n}\n");
    fclose(out);

    printf("Wrote %d levels → %s\n", n_level_blocks, out_path);

    /* cleanup */
    for (int b = 0; b < nblocks; b++) free(blocks[b]);
    free(blocks); free(block_sz); free(level_block_idx);
    free(header_lines); free(lines); free(raw);
    return 0;
}

/* ------------------------------------------------------------------ */
/* _parse_xsokoban                                                      */
/* mirrors Python _parse_xsokoban()                                    */
/* ------------------------------------------------------------------ */

static int cmp_screen_num(const void *a, const void *b) {
    const char *sa = *(const char **)a;
    const char *sb = *(const char **)b;
    /* Find suffix number after last '.' */
    const char *pa = strrchr(sa, '.'); int na = pa ? atoi(pa+1) : 0;
    const char *pb = strrchr(sb, '.'); int nb = pb ? atoi(pb+1) : 0;
    return na - nb;
}

static int _parse_xsokoban(const char *dataset_path, const char *dataset_name,
                            const char *out_path) {
    DIR *d = opendir(dataset_path);
    if (!d) { fprintf(stderr, "Cannot open dir '%s'\n", dataset_path); return 1; }

    /* Collect screen.N files */
    char **screen_files = (char **)malloc(512 * sizeof(char *));
    int    n_screens    = 0;
    struct dirent *ent;
    while ((ent = readdir(d)) != NULL) {
        if (strncmp(ent->d_name, "screen.", 7) == 0) {
            char full[1024];
            snprintf(full, sizeof(full), "%s/%s", dataset_path, ent->d_name);
            screen_files[n_screens++] = strdup(full);
        }
    }
    closedir(d);

    qsort(screen_files, n_screens, sizeof(char *), cmp_screen_num);

    FILE *out = fopen(out_path, "w");
    if (!out) { fprintf(stderr, "Cannot write '%s'\n", out_path); return 1; }

    fprintf(out, "{\n  \"dataset\": ");
    json_write_string(out, dataset_name);
    fprintf(out, ",\n  \"format\": \"xsokoban\",\n  \"collection_meta\": {},\n  \"levels\": [\n");

    for (int si = 0; si < n_screens; si++) {
        const char *fpath = screen_files[si];
        /* Extract numeric suffix */
        const char *dot = strrchr(fpath, '.');
        int level_id = dot ? atoi(dot+1) : si+1;

        FILE *gf = fopen(fpath, "r");
        if (!gf) continue;

        char  grid_buf[128][256]; int ngrid = 0;
        char  line[256];
        while (fgets(line, sizeof(line), gf) && ngrid < 128) {
            rtrim(line);
            strcpy(grid_buf[ngrid++], line);
        }
        fclose(gf);

        /* Drop trailing blank lines */
        while (ngrid > 0 && grid_buf[ngrid-1][0] == '\0') ngrid--;

        int width = 0;
        for (int g = 0; g < ngrid; g++) {
            int l = (int)strlen(grid_buf[g]);
            if (l > width) width = l;
        }

        if (si) fprintf(out, ",\n");
        fprintf(out, "    {\n");
        fprintf(out, "      \"id\": %d,\n", level_id);
        fprintf(out, "      \"title\": null,\n");
        fprintf(out, "      \"author\": null,\n");
        fprintf(out, "      \"comment\": null,\n");
        /* source_file: just filename */
        const char *fname = strrchr(fpath, '/');
        json_write_string(out, fname ? fname+1 : fpath);
        fprintf(out, ",\n");  /* will overwrite – fix: */
        /* actually rewrite properly */
        fseek(out, -2, SEEK_CUR);  /* back over ",\n" */
        fprintf(out, ",\n      \"source_file\": ");
        json_write_string(out, fname ? fname+1 : fpath);
        fprintf(out, ",\n");
        fprintf(out, "      \"width\": %d,\n", width);
        fprintf(out, "      \"height\": %d,\n", ngrid);
        fprintf(out, "      \"grid\": [");
        for (int g = 0; g < ngrid; g++) {
            if (g) fprintf(out, ", ");
            json_write_string(out, grid_buf[g]);
        }
        fprintf(out, "]\n    }");

        free(screen_files[si]);
    }

    fprintf(out, "\n  ]\n}\n");
    fclose(out);
    printf("Wrote %d levels → %s\n", n_screens, out_path);

    free(screen_files);
    return 0;
}

/* ------------------------------------------------------------------ */
/* detect_format  (mirrors Python _detect_format)                      */
/* ------------------------------------------------------------------ */

DatasetFormat detect_format(const char *dataset_path) {
    DIR *d = opendir(dataset_path);
    if (!d) return FORMAT_UNKNOWN;
    struct dirent *ent;
    bool has_sok = false, has_screen = false;
    while ((ent = readdir(d)) != NULL) {
        const char *name = ent->d_name;
        const char *dot  = strrchr(name, '.');
        if (dot && strcmp(dot, ".sok") == 0) has_sok = true;
        if (strncmp(name, "screen.", 7) == 0) has_screen = true;
    }
    closedir(d);
    if (has_sok)    return FORMAT_SOK;
    if (has_screen) return FORMAT_XSOKOBAN;
    return FORMAT_UNKNOWN;
}

/* ------------------------------------------------------------------ */
/* parse_dataset  (mirrors Python parse_dataset)                       */
/* ------------------------------------------------------------------ */

int parse_dataset(const char *dataset_name) {
    char raw_path[512], out_dir[512], out_path[512];
    snprintf(raw_path, sizeof(raw_path), "data/raw/%s",    dataset_name);
    snprintf(out_dir,  sizeof(out_dir),  "data/parsed/%s", dataset_name);
    snprintf(out_path, sizeof(out_path), "data/parsed/%s/levels.json", dataset_name);

    DatasetFormat fmt = detect_format(raw_path);
    if (fmt == FORMAT_UNKNOWN) {
        fprintf(stderr, "Cannot detect format for '%s'.\n", raw_path);
        return 1;
    }

    const char *fmt_name = (fmt == FORMAT_SOK) ? "sok" : "xsokoban";
    printf("Detected format: %s\n", fmt_name);

    /* Create output directory */
    mkdir(out_dir, 0755);

    if (fmt == FORMAT_SOK) {
        /* Find single .sok file */
        DIR *d = opendir(raw_path);
        if (!d) return 1;
        char sok_file[512]; sok_file[0] = '\0';
        int  n_sok = 0;
        struct dirent *ent;
        while ((ent = readdir(d)) != NULL) {
            const char *dot = strrchr(ent->d_name, '.');
            if (dot && strcmp(dot, ".sok") == 0) {
                snprintf(sok_file, sizeof(sok_file), "%s/%s", raw_path, ent->d_name);
                n_sok++;
            }
        }
        closedir(d);
        if (n_sok != 1) {
            fprintf(stderr, "Expected exactly one .sok file, found %d\n", n_sok);
            return 1;
        }
        return _parse_sok(sok_file, dataset_name, out_path);
    } else {
        return _parse_xsokoban(raw_path, dataset_name, out_path);
    }
}

/* ------------------------------------------------------------------ */
/* parser_main  (mirrors Python main / if __name__=="__main__")        */
/* ------------------------------------------------------------------ */

int parser_main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr,
            "Usage: sokoban_parser <dataset>\n"
            "  dataset: directory name within data/raw/ (e.g. microban, xsokoban)\n");
        return 1;
    }
    return parse_dataset(argv[1]);
}