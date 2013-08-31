struct screeninfo {
   int   screen_number;
   short x_org;
   short y_org;
   short width;
   short height;
};

int screeninfo_get_screen_number(screeninfo_p this, int i) {
	return this[i].screen_number;
}

short screeninfo_get_x_org(screeninfo_p this, int i) {
	return this[i].x_org;
}

short screeninfo_get_y_org(screeninfo_p this, int i) {
	return this[i].y_org;
}

short screeninfo_get_width(screeninfo_p this, int i) {
	return this[i].width;
}

short screeninfo_get_height(screeninfo_p this, int i) {
	return this[i].height;
}

