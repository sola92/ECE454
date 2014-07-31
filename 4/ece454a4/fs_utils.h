#ifndef _ECE_FS_UTILS_
#define _ECE_FS_UTILS_

/**
 * Returns size of a linked list.
 *
 * @param head of the linked list
 * @param next function that returns the successor of a node
 * @return     size of the linked list
 */
int list_size(void *head, void *(next)(void *)) {
    int size = 0;
    void *current = head;
    for(; current != NULL; current = next(current), size++);
    return size;
}

/**
 * Removes a node from a linked list.
 *
 * @param head     of the linked list
 * @param node     to be removed
 * @param next     function that returns the successor of a node
 * @param set_next function that connects two nodes
 */
void list_delete(void **head, void *node, void *(next)(void *), void(set_next)(void *, void *)) {
    if (node == *head) {
        *head = next(*head);
    } else {
        void *current = *head;
        for(; next(current) != node; current = next(current));
        set_next(current, next(node));
    }

    free(node);
}

/**
 * Insert a node into the given linked list.
 *
 * @param head     of the linked list
 * @param new      node sto be removed
 * @param next     function that returns the successor of a node
 * @param set_next function that connects two nodes
 */
void list_insert(void **head, void *new, void *(next)(void *), void(set_next)(void *, void *)) {
    if (*head == NULL) {
        *head = new;
        return;
    }

    void *current = *head;
    for(; next(current) != NULL; current = next(current));
    set_next(current, new);
}

/**
 * Concatenates char arrays. Appends second array to first.
 *
 * @param s1 first array
 * @param s2 second array
 * @return   concatenated array
 */
char* concat(char *s1, char *s2) {
    size_t len1 = strlen(s1),
           len2 = strlen(s2);
    char *result = malloc(len1 + len2 + 1);
    memcpy(result, s1, len1);
    memcpy(result + len1, s2, len2 + 1);
    return result;
}

#endif