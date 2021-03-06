/* This file is part of remsh
 * Copyright 2009, 2010, 2010 Dustin J. Mitchell
 * See COPYING for license information
 */

#ifndef REMSH_H
#define REMSH_H

#include <sys/types.h>

/*
 * Xport layer
 */

struct remsh_xport;

struct remsh_xport_vtable {
    /* write to the transport; returns -1 on error, or 0 on success */
    int (* write)(struct remsh_xport *self, void *buf, ssize_t len);

    /* read from the transport; returns -1 on error, 0 on EOF, and a positive
     * number on success */
    ssize_t (* read)(struct remsh_xport *self, void *buf, ssize_t len);

    /* close the connection; returns -1 on failure, 0 on success; this also
     * frees the remsh_xport object, so SELF is invalid after this method
     * returns without error. */
    int (* close)(struct remsh_xport *self);
};

typedef struct remsh_xport {
    /* pointer to functions for this instance */
    struct remsh_xport_vtable *v;

    /* error message from most recent failure */
    char *errmsg;
} remsh_xport;

/* "method" macros */
#define remsh_xport_write(o, b, l) ((o)->v->write((o),(b),(l)))
#define remsh_xport_read(o, b, l) ((o)->v->read((o),(b),(l)))
#define remsh_xport_close(o) ((o)->v->close((o)))

/* Constructor for a transport mechanism wrapped around a simple file
 * descriptor */
remsh_xport *remsh_fd_xport_new(int fd);

/*
 * Wire Layer
 */

/* A simple array of keys and values; terminate the array with an element
 * containing a NULL key */
typedef struct remsh_box {
    unsigned char key_len; /* calculated with strlen if zero */
    char *key;
    unsigned short int val_len; /* required */
    char *val;
} remsh_box;

/* opaque type */
typedef struct remsh_wire remsh_wire;

/* Constructor for remsh wires */
remsh_wire *remsh_wire_new(remsh_xport *xport);

/* Close and destroy the object; returns -1 on error, 0 on success.  The
 * object is gone and cannot be used after a successful return. */
int remsh_wire_close(remsh_wire *wire);

/* Send a box containing they keys and values in BOX; returns -1 on error, or 0
 * on success. */
int remsh_wire_send_box(remsh_wire *wire, remsh_box *box);

/* Read a box.  Returns -1 on error and 0 on success.  *BOX is NULL on EOF, and
 * otherwise points to an internally-allocated key/value array representing the
 * returned box.  The box remains valid until the next call to this method. */
int remsh_wire_read_box(remsh_wire *wire, remsh_box **box);

/* Extract values from BOX.  The EXTRACT array specifies the keys of interest;
 * on return, any keys which were found in the most recent box have the VAL and
 * VAL_LEN attributes set.  Note that an empty value is represented as a
 * non-NULL pointer with a zero VAL_LEN.  All values are zero-terminated for
 * convenience.  */
void remsh_wire_box_extract(remsh_box *box, remsh_box *extract);

/* Return a printable representation of the given box.  Note that this assumes
 * the box's values are strings.  The caller is responsible for freeing the
 * returned string. */
char *remsh_wire_box_repr(remsh_box *box);

/*
 * Operations Layer
 */

/* note that only the slave side of this layer is implemented */

/* call this once before any other operations-layer functions */
void remsh_op_init(void);

/* Perform a single operation and return; returns -1 on error or 0 on success.
 * The EOF output parameter is set to 1 and 0 is returned when an EOF is read
 * from RWIRE. */
int remsh_op_perform(remsh_wire *rwire, remsh_wire *wwire, int *eof);

/* Repeatedly perform operations until EOF or an error occurs.  Returns 0 on
 * EOF or -1 on error. */
int remsh_op_loop(remsh_wire *rwire, remsh_wire *wwire);

#endif /* REMSH_H */
