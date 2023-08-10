#ifndef KEYB_H_
#define KEYB_H_

#include "app.h"

#define KB_ANY		(-1)
#define KB_ALT		(-2)
#define KB_CTRL		(-3)
#define KB_SHIFT	(-4)

#ifdef __cplusplus
extern "C" {
#endif

void kb_init(void);
void kb_shutdown(void); /* don't forget to call this at the end! */

/* Boolean predicate for testing the current state of a particular key.
 * You may also pass KB_ANY to test if any key is held down.
 */
int kb_isdown(int key);

/* waits for any keypress */
void kb_wait(void);

/* removes and returns a single key from the input buffer.
 * If buffering is disabled (initialized with kb_init(0)), then it always
 * returns the last key pressed.
 */
int kb_getkey(void);

void kb_putback(int key);

#ifdef __cplusplus
}
#endif

#endif	/* KEYB_H_ */
