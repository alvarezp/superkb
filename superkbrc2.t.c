#include <stdlib.h>
#include <stdio.h>

#include "superkbrc2.h"

#define TESTFAIL(str) do { \
	fprintf(stderr, "TEST FAILED at %s:%d: %s\n", __FILE__, __LINE__, str); \
	return EXIT_FAILURE; \
	} while(0);

int i255_low(int original) {

	if (original < 256)
		return original * 256;

	return original;

}

int main() {

	superkbrc2_t *s;

	s = superkbrc2_new();
	if (s == NULL)
		TESTFAIL("Could not create structure;\n");

	superkbrc2_new_attribute("foreground");
	superkbrc2_map_multipleallowed("foreground", OVERRIDE); /* IGNORE, OVERRIDE, APPEND */
	superkbrc2_map_keyword("foreground", "FOREGROUND");
	superkbrc2_map_value_rgb("foreground", "color", REQUIRED);

	superkbrc2_new_attribute("magickeyreleasecancels");
	superkbrc2_map_multipleallowed("magickeyreleasecancels", OVERRIDE);
	superkbrc2_map_keyword("magickeyreleasecancels", "MAGICKEYRELEASECANCELS");
	superkbrc2_map_value_boolean("magickeyreleasecancels", "enabled", REQUIRED);

	superkbrc2_new_attribute("magickey");
	superkbrc2_map_type("magickey", APPEND);
	superkbrc2_map_keyword("magickey", "MAGICKEY.CODE");
	superkbrc2_map_value_integer("magickey", "keycode", REQUIRED);

	superkbrc2_new_attribute("magickey");
	superkbrc2_map_type("magickey", APPEND);
	superkbrc2_map_keyword("magickey", "MAGICKEY.STRING");
	superkbrc2_map_value_string("magickey", "keysym", REQUIRED);

	superkbrc2_new_attribute("squash_states");
	superkbrc2_map_type("squash_states", OVERRIDE);
	superkbrc2_map_keyword("squash_states", "SQUASH_STATES");
	superkbrc2_map_value_integer("squash_states", "enabled", BOOLEAN, REQUIRED);

	superkbrc2_new_attribute("key");
	superkbrc2_map_type("key", APPEND);
	superkbrc2_map_index("key", "key", STRING, REQUIRED);
	superkbrc2_map_index("key", "shift_state", FLAGS, REQUIRED);
	superkbrc2_map_value("key", "type", FLAGS, REQUIRED);

	s = superkbrc2_map("key", APPEND, "KEY", "key", STRING|INDEX, "shift_state", FLAGS|INDEX, "SHIFT,CTRL", "type", FIXED, "COMMAND", "cmdline", STRING|REQUIRED, "icon", STRING|REQUIRED, "feedback", STRING);

	s = superkbrc2_map("key", APPEND, "KEY", "key", STRING|INDEX, "shift_state", FLAGS|INDEX, "SHIFT,CTRL", "type", DOCUMENT, "COMMAND", "file", STRING|REQUIRED, "icon", STRING, "feedback", STRING);

	s = superkbrc2_map("key", APPEND, "KEY", "key:string,index", "shift_state:flags[SHIFT],index" "type:fixed[NOTHING]");

	s = superkbrc2_map("magickey", APPEND, "MAGICKEY.CODE", "integer");

	s = superkbrc2_map("magickey", APPEND, "MAGICKEY.STRING", "string");

	s = superkbrc2_parse("SUPERKEY1_CODE 230");

	s = superkbrc2_get("magickey");

	if (s == NULL)
		TESTFAIL("Could not create structure;\n");

	superkbrc2_destroy(s);

	return EXIT_SUCCESS;
}
