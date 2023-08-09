#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#if defined(__DJGPP__)
#include <sys/farptr.h>
#include <dos.h>
#include <dpmi.h>
#include <go32.h>
#include <conio.h>
#endif

static int
find_index(char **haystack, int num_columns, const char *needle)
{
    for (int idx=0; idx<num_columns; ++idx) {
        if (strcmp(haystack[idx], needle) == 0) {
            return idx;
        }
    }

    return -1;
}

int main(int argc, char *argv[])
{
#if defined(__DJGPP__)
    int their_ds, their_offset;
    if (argc != 2) {
        printf("Cannot run directly\n");
        return 1;
    }

    int res = sscanf(argv[1], "%x:%x", &their_ds, &their_offset);
    if (res != 2) {
        printf("Error with arg\n");
        return 1;
    }
#endif

    char *header_line = NULL;
    char *header_parts[64];
    int num_columns = 0;

    char **values[128];
    int odx = 0;

    int name_idx = -1, author_idx = -1, run_idx = -1, endscreen_idx = -1,
        visible_idx = -1;

    FILE *fp = fopen("DEMO2023/games.csv", "rb");
    while (!feof(fp)) {
        char tmp[512];

        if (fgets(tmp, sizeof(tmp), fp) != NULL) {
            char *end = tmp + strlen(tmp) - 1;
            while (end > tmp && (*end == '\r' || *end == '\n')) {
                *end-- = '\0';
            }

            if (header_line == NULL) {
                header_line = strdup(tmp);
                //printf("Header: >%s<\n", header_line);

                char *part = strtok(header_line, ";");
                int idx = 0;
                while (part != NULL) {
                    header_parts[idx] = part;
                    //printf("Part: >%s<\n", header_parts[idx]);

                    ++idx;
                    ++num_columns;
                    part = strtok(NULL, ";");
                }

                name_idx = find_index(header_parts, num_columns, "Name");
                author_idx = find_index(header_parts, num_columns, "Author");
                run_idx = find_index(header_parts, num_columns, "Run");
                endscreen_idx = find_index(header_parts, num_columns, "EndScreen");
                visible_idx = find_index(header_parts, num_columns, "Visible");
            } else {
                values[odx] = (char **)malloc(sizeof(char *) * num_columns);

                char *part = strtok(tmp, ";");
                int idx = 0;
                while (part != NULL) {
                    //printf(">%s< => >%s<\n", header_parts[idx], part);
                    values[odx][idx] = strdup(part);

                    // fix up paths
                    if (idx == run_idx) {
                        char *cur = values[odx][idx];
                        while (*cur) {
                            if (*cur == '\\') {
                                *cur = '/';
                            }
                            ++cur;
                        }
                    }

                    ++idx;
                    part = strtok(NULL, ";");
                }

                // only if visible
                if (values[odx][visible_idx][0] == 'Y') {
                    ++odx;
                }
            }
        }
    }

    char *result = NULL;
    while (1) {
#if defined(__DJGPP__)
        clrscr();
#else
        printf("%c[2J%c[H", 27, 27); fflush(stdout);
#endif
        printf("[DOS Game Jam Demo Disc, Your Choice]\n");
        for (int i=0; i<odx; ++i) {
            printf(" [%2d] %-33s", i + 1, values[i][name_idx]);
            ++i;
            if (i < odx) {
                printf(" [%2d] %-33s\n", i + 1, values[i][name_idx]);
            } else {
                printf("\n");
            }
        }

        printf("Your choice (0 to exit): "); fflush(stdout);
        int choice = -1;
        if (fscanf(stdin, "%d", &choice) == 1) {
            if (choice < 0 || choice > odx) {
                continue;
            }

            if (choice == 0) {
                result = "exit";
            } else {
                char *tmp = values[choice-1][run_idx];
                int have_endscreen = strcmp(values[choice-1][endscreen_idx], "Y") == 0;

                result = (char *)malloc(1 + strlen("demo2023/") + strlen(tmp) + 1);
                sprintf(result, "%cdemo2023/%s", have_endscreen ? '@' : ' ', tmp);
#if !defined(__DJGPP__)
                printf("Got: %d [run=%s]\n", choice, result);
#endif
            }
            break;
        }
    }

#if defined(__DJGPP__)
    while (*result) {
        _farpokeb(_dos_ds, their_ds * 16 + their_offset, *result);
        ++their_offset;
        ++result;
    }
    _farpokeb(_dos_ds, their_ds * 16 + their_offset, '\0');
#endif

    // TODO: could free stuff

    return 0;
}
