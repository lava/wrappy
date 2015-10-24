#include <wrappy/wrappy.h>

void drawTree(double len, double angle, int lvl) {
	if(!lvl) return;

	wrappy::call("turtle.left", angle);
	wrappy::call("turtle.forward", len);
	drawTree(0.5*len,  60, lvl-1);
	drawTree(0.7*len,  0,  lvl-1);
	drawTree(0.5*len, -60, lvl-1);
	wrappy::call("turtle.backward", len);
	wrappy::call("turtle.right", angle);
}

int main() { 
	drawTree(100, 90, 6);
}
