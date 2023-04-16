#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <zlib.h>

typedef struct ranges {
    long long int start;
    long long int end;
    struct ranges* next;
} ranges_t;

void* init_ranges() {
    ranges_t* range = malloc(sizeof* range);

    if (range == NULL) return NULL;

    range->start = -1;
    range->end = -1;

    return range;
}

void free_ranges(ranges_t* ranges) {
    while (ranges) {
        ranges_t* next = ranges->next;

        free(ranges);

        ranges = next;
    }
}

ranges_t* parse_bytes(char* str, size_t length) {
    int result = -1;
    int max = 2147483647;
    long long int start_finded = 0;
    size_t start_position = 0;
    ranges_t* ranges = NULL;
    ranges_t* last_range = NULL;

    for (size_t i = 0; i <= length; i++) {
        long long int end = -1;

        if (isdigit(str[i])) continue;
        else if (str[i] == '-') {
            if (last_range && last_range->end == -1) goto failed;
            if (last_range && last_range->start == -1) goto failed;

            int start = -1;

            start_finded = 1;
            if (i > start_position) {
                if (i - start_position > 10) goto failed;

                str[i] = 0;
                start = atoll(&str[start_position]);
                str[i] = '-';

                if (start > max) goto failed;

                if (last_range && last_range->start > start) goto failed;

                if (last_range && last_range->start > -1 && last_range->end >= start) {
                    start_position = i + 1;
                    continue;
                }
            }

            ranges_t* range = init_ranges();
            if (range == NULL) goto failed;

            if (ranges == NULL) ranges = range;

            if (last_range != NULL) {
                last_range->next = range;
            }
            last_range = range;

            if (i > start_position) {
                range->start = start;
            }

            start_position = i + 1;
        }
        else if (str[i] == ',') {
            if (i > start_position) {
                if (i - start_position > 10) goto failed;

                str[i] = 0;
                end = atoll(&str[start_position]);
                str[i] = ',';

                if (end > max) goto failed;
                if (end < last_range->start) goto failed;
                if (start_finded == 0) goto failed;

                if (last_range->end <= end) {
                    last_range->end = end;
                }
            }

            start_finded = 0;
            start_position = i + 1;
        }
        else if (str[i] == 0) {
            if (i > start_position) {
                if (i - start_position > 10) goto failed;

                end = atoll(&str[start_position]);

                if (end > max) goto failed;
                if (end < last_range->start) goto failed;
                if (start_finded == 0) goto failed;

                if (last_range->end <= end) {
                    last_range->end = end;
                }
            }
            else if (last_range && start_finded) {
                last_range->end = -1;
            }
        }
        else if (str[i] == ' ') {
            if (!(i > 0 && str[i - 1] == ',')) goto failed;
            start_position = i + 1;
        }
        else goto failed;
    }

    result = 0;

    failed:

    if (result == -1) {
        free_ranges(ranges);
        return NULL;
    }

    return ranges;
}

int main() {
    char* range = malloc(200);
    strcpy(range, "bytes=1-3,2-13,13-21,501-450000,450001-");

    if (!(range[0] == 'b' && range[1] == 'y' && range[2] == 't' && range[3] == 'e' && range[4] == 's' && range[5] == '=')) return 0;

    ranges_t* ranges = parse_bytes(&range[6], strlen(&range[6]));

    if (ranges == NULL) return 0;



    return 0;
}