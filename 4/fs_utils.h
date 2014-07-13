#ifndef _ECE_FS_UTILS_
#define _ECE_FS_UTILS_

int list_size(void *head, void *(next)(void *)) {
    int size = 0;
    void *current = head;
    for(; current != NULL; current = next(current), size++);
    return size;
}

void list_delete(void **head, void *node, void *(next)(void *), void(set_next)(void *, void *)) {
    if (node == *head) {
        *head = next(head);
    } else {
        void *current = *head;
        for(; next(current) != node; current = next(current));
        set_next(current, next(node));
    }

    free(node);
}

void list_insert(void **head, void *new, void *(next)(void *), void(set_next)(void *, void *)) {
    if (*head == NULL) {
        *head = new;
        return;
    }

    void *current = *head;
    for(; next(current) != NULL; current = next(current));
    set_next(current, new);
}

char* concat(char *s1, char *s2) {
    size_t len1 = strlen(s1),
           len2 = strlen(s2);
    char *result = malloc(len1 + len2 + 1);
    memcpy(result, s1, len1);
    memcpy(result + len1, s2, len2 + 1);
    return result;
}

#endif