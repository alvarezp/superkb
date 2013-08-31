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

mapper_set_mapmask(mapper_t * this, int mapmask) {
	this->mapmask = mapmask;
}

mapper_get_mapmask(mapper_t * this, int mapmask) {
	return this->mapmask;
}

