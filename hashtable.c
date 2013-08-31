struct mapper {
	int mapmask;
}

typedef struct mapper mapper_t;

mapper_t * mapper_new(void) {

	mapper_t * this = malloc(sizeof(mapmask_t));

	this->mapmask = 0;

	return this;
	
}

void mapper_delete(this) {
	free(this);
}

void mapper_set_mapmask(mapper_t * this, int mapmask) {
	this->mapmask = mapmask;
}

int mapper_get_mapmask(mapper_t * this, int mapmask) {
	return this->mapmask;
}

int mapper_expand(mapper_t * this, int value) {
	
}

int mapper_contract(mapper_t * this, int value) {
	int shift = 0;
	int bit;
	int ret;

	value = value & ~this->mapmask;

	for (bit = 0; bit < 32; ++bit) {
		ret = 
	}
}

