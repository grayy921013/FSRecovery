/* Turn on or off debug messages here. */
 #define DEBUG_SWITCH 1
 #define DEBUG(text) do { \
         if (DEBUG_SWITCH) {text} \
         } while (0)
