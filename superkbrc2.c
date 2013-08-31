#include <malloc.h>

#include "superkbrc2.h"

struct superkbrc2 {
	int f;
};

superkbrc2_t * superkbrc2_new() {
	superkbrc2_t *this;

	this = (superkbrc2_t *) malloc(sizeof(superkbrc2_t));

	return this;
}

void superkbrc2_destroy(superkbrc2_t *this) {
	free(this);
}

