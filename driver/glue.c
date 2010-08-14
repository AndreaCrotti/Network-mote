#include "glue.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>

/**
 * Implementation of glue_t::setHandler. Installs/Removes a handler for the given (fd,type) tuple.
 *
 * @param fd The file descriptor to install/remove the handler for.
 * @param type The kind of action you want to wait for (read/write/error). See fdglue_handle_type_t.
 * @param hnd The handler you want to install. (Ignored if you remove an entry)
 * @param action What you want to do? Replace, remove or append?
 */
void _fdglue_t_set_handler(fdglue_t* this, int fd, fdglue_handle_type_t const type, fdglue_handler_t const hnd, fdglue_handler_replace_t const action, char** const active) {
    assert(this);
    // change/remove an item the list
    if (action != FDGHR_APPEND) {
        fdglue_handlerlist_t dummy;
        dummy.fd = -1;
        dummy.next = this->handlers;
        //   dummy  ->  handlers  ->  ...  ->  NULL
        struct fdglue_handlerlist_t* it,* last = &dummy;
        for (it = dummy.next; it; it = it->next) {
            if (it->fd == fd && it->type == type) {
                switch (action) {
                    case FDGHR_REPLACE:
                        it->hnd = hnd;
                        break;
                    case FDGHR_REMOVE:
                        last->next = it->next;
                        free(it);
                        it = last;
                        break;
                    default: {}
                }
            }
            last = it;
        }
        it = last;
        assert(it);
        this->handlers = dummy.next;
    } else {
        // put a new item into the list
        fdglue_handlerlist_t* p;
        assert(DYNAMIC_MEMORY);
        // item for the new element
        p = malloc(sizeof(fdglue_handlerlist_t));
        p->fd = fd;
        p->type = type;
        p->hnd = hnd;
        // allow easy external access to this item (to toggle it without removing it)
        if (active)
          *active = &(p->active);
        p->active = 1;
        p->next = this->handlers;
        this->handlers = p;
        if (fd > this->nfds)
            this->nfds = fd;
    }
}

/**
 * Main function to start listening on all file descriptors.
 * 
 * @param this The object this function is to called with.
 * @param timeout Amount of seconds until we will return even if no fd is writable/readable
 * @param us Amount of micro-seconds (added to the seconds)
 */
void _fdglue_t_listen(fdglue_t* this, unsigned timeout, unsigned us) {
    assert(this);
    static fd_set rd, wr, er;
    // fdmap tells us the 'struct fd_set' object depending on wether we want
    // to read/write/error
    static fd_set* fdmap[FDGHT_SIZE];
    fdmap[FDGHT_READ] = &rd;
    fdmap[FDGHT_WRITE] = &wr;
    fdmap[FDGHT_ERROR] = &er;
    // make sure every object is 'empty'
    FD_ZERO(&rd);
    FD_ZERO(&wr);
    FD_ZERO(&er);
    struct fdglue_handlerlist_t* it;
    for (it = this->handlers; it; it = it->next) {
        // it->active can be pointed to from outside the class to temporarily deactivate a certain descriptor
        if (it->active) {
            // add all fds we are supposed to listen to to the appropriate object
            FD_SET(it->fd,fdmap[it->type]);
        }
    }
    struct timeval tv = {.tv_sec = timeout, .tv_usec = us};
    // more magic
    if (-1 != select(this->nfds+1, &rd, &wr, &er, &tv)) {
        for (it = this->handlers; it; it = it->next) {
            if (it->active && FD_ISSET(it->fd,fdmap[it->type])) {
                // there was something on that file descriptor, so call the handler
                it->hnd.handle(&(it->hnd));
            }
        }
    }
}

/**
 *  Create a new fdglue object.
 */
fdglue_t* fdglue(fdglue_t* this) {
    CTOR(this);
    this->handlers = NULL;
    this->nfds = -1;
    this->set_handler = _fdglue_t_set_handler;
    this->listen = _fdglue_t_listen;
    return this;
}
